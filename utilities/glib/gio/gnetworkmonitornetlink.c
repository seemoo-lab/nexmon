/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2011 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#if defined(HAVE_SYS_SYSCTL_H) && defined(HAVE_SYSCTLBYNAME)
#include <sys/sysctl.h>
#endif

#include "gnetworkmonitornetlink.h"
#include "gcredentials.h"
#include "ginetaddress.h"
#include "ginetaddressmask.h"
#include "ginetsocketaddress.h"
#include "ginitable.h"
#include "giomodule-priv.h"
#include "gioerror.h"
#include "glibintl.h"
#include "glib/gstdio.h"
#include "gnetworkingprivate.h"
#include "gnetworkmonitor.h"
#include "gsocket.h"
#include "gsocketaddressenumerator.h"
#include "gsocketconnectable.h"
#include "gtask.h"
#include "gunixcredentialsmessage.h"

/* must come at the end to pick system includes from
 * gnetworkingprivate.h */
#ifdef HAVE_LINUX_NETLINK_H
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
/* <linux/netlink.h> defines NETLINK_GET_STRICT_CHK but not its setsockopt
 * level SOL_NETLINK. */
#ifndef SOL_NETLINK
#define SOL_NETLINK 270
#endif
#endif
#if defined(HAVE_NETLINK_NETLINK_H) && defined(HAVE_NETLINK_NETLINK_ROUTE_H)
#include <netlink/netlink.h>
#include <netlink/netlink_route.h>
#endif

/* GNetworkMonitorNetlink is the default GNetworkMonitor on Linux when neither
 * NetworkManager nor systemd-networkd is in charge.
 *
 * The kernel routing table (FIB) can be enormous on routers and network-facing
 * servers (millions of routes), so this backend must never mirror or re-dump the
 * whole table. It deliberately ignores non-default routes:
 *
 *  - network-available is derived only from default routes. A single streaming
 *    dump at construction records just the default routes (every route with
 *    rtm_dst_len != 0 is skipped), and the set is then maintained incrementally
 *    from RTM_NEWROUTE/RTM_DELROUTE deltas. The table is re-dumped only to resync
 *    after an ENOBUFS overflow.
 *
 *  - can_reach() uses an on-demand per-destination RTM_GETROUTE lookup (the
 *    "ip route get" primitive), which is O(prefix length) and independent of the
 *    table size.
 *
 * If you change this code, keep it that way: never enumerate or cache the full
 * FIB. */

static GInitableIface *initable_parent_iface;
static void g_network_monitor_netlink_iface_init (GNetworkMonitorInterface *iface);
static void g_network_monitor_netlink_initable_iface_init (GInitableIface *iface);

struct _GNetworkMonitorNetlinkPrivate
{
  GSocket *query_sock;  /* (mutex query_lock) */
  guint    query_seq;  /* (mutex query_lock) */
  GMutex   query_lock;

  GMainContext *context;

  /* Availability tracking from the kernel routing table. These members are set
   * up only by setup_monitor(), which runs solely for the plain
   * GNetworkMonitorNetlink type. Subclasses (NetworkManager, systemd) source
   * availability elsewhere (e.g. D-Bus) and inherit only can_reach(), so for
   * them these stay NULL/empty: any code touching them must handle that. */
  GSocket *monitor_sock;  /* (nullable) */
  GSource *source;  /* (nullable) */
  GSource *dump_source;  /* (nullable) */
  gboolean dumping;
  GHashTable *ipv4_defaults;  /* (element-type utf8) (owned) (nullable) */
  GHashTable *ipv6_defaults;  /* (element-type utf8) (owned) (nullable) */
  GHashTable *dump_ipv4_defaults;  /* (element-type utf8) (owned) (nullable) */
  GHashTable *dump_ipv6_defaults;  /* (element-type utf8) (owned) (nullable) */
  GInetAddressMask *ipv4_default_mask;  /* (nullable) */
  GInetAddressMask *ipv6_default_mask;  /* (nullable) */
};

static gboolean read_netlink_messages (GNetworkMonitorNetlink  *nl,
                                       GError                 **error);
static gboolean read_netlink_messages_callback (GSocket             *socket,
                                                GIOCondition         condition,
                                                gpointer             user_data);
static gboolean request_dump (GNetworkMonitorNetlink  *nl,
                              GError                 **error);

#define g_network_monitor_netlink_get_type _g_network_monitor_netlink_get_type
G_DEFINE_TYPE_WITH_CODE (GNetworkMonitorNetlink, g_network_monitor_netlink, G_TYPE_NETWORK_MONITOR_BASE,
                         G_ADD_PRIVATE (GNetworkMonitorNetlink)
                         G_IMPLEMENT_INTERFACE (G_TYPE_NETWORK_MONITOR,
                                                g_network_monitor_netlink_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                g_network_monitor_netlink_initable_iface_init)
                         _g_io_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "netlink",
                                                         20))

static void
g_network_monitor_netlink_init (GNetworkMonitorNetlink *nl)
{
  nl->priv = g_network_monitor_netlink_get_instance_private (nl);
}

/* Create a NETLINK_ROUTE socket bound to the given multicast groups (0 for a
 * request/response socket that is not interested in unsolicited events). */
static GSocket *
create_netlink_socket (guint32   groups,
                       GError  **error)
{
  gint sockfd;
  struct sockaddr_nl snl;
  GSocket *sock;
  const int rcvbuf = 1024 * 1024;

  /* We create the socket the old-school way because sockaddr_netlink
   * can't be represented as a GSocketAddress
   */
  sockfd = g_socket (PF_NETLINK, SOCK_RAW, NETLINK_ROUTE, NULL);
  if (sockfd == -1)
    {
      int errsv = errno;
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
                   _("Could not create network monitor: %s"),
                   g_strerror (errsv));
      return NULL;
    }

  /* A host running dynamic routing protocols such as BGP can generate many
   * route events in a short time; a larger buffer reduces ENOBUFS overflows
   * that force running the full route enumeration again. */
  if (setsockopt (sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof (rcvbuf)) != 0)
    g_debug ("Could not enlarge netlink receive buffer: %s", g_strerror (errno));

  snl.nl_family = AF_NETLINK;
  snl.nl_pid = snl.nl_pad = 0;
  snl.nl_groups = groups;
  if (bind (sockfd, (struct sockaddr *)&snl, sizeof (snl)) != 0)
    {
      int errsv = errno;
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
                   _("Could not create network monitor: %s"),
                   g_strerror (errsv));
      (void) g_close (sockfd, NULL);
      return NULL;
    }

  sock = g_socket_new_from_fd (sockfd, error);
  if (!sock)
    {
      g_prefix_error (error, "%s", _("Could not create network monitor: "));
      (void) g_close (sockfd, NULL);
      return NULL;
    }

  return sock;
}

static gboolean
setup_monitor (GNetworkMonitorNetlink  *nl,
               GError                 **error)
{
  nl->priv->monitor_sock = create_netlink_socket (RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE,
                                                  error);
  if (!nl->priv->monitor_sock)
    return FALSE;

#ifdef SO_PASSCRED
  if (!g_socket_set_option (nl->priv->monitor_sock, SOL_SOCKET, SO_PASSCRED,
                            TRUE, NULL))
    {
      int errsv = errno;
      g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
                   _("Could not create network monitor: %s"),
                   g_strerror (errsv));
      return FALSE;
    }
#endif

  nl->priv->ipv4_defaults = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  nl->priv->ipv6_defaults = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* Request the current state to find existing default routes */
  if (!request_dump (nl, error))
    return FALSE;

  /* And read responses; since we haven't yet marked the socket
   * non-blocking, each call will block until a message is received.
   */
  while (nl->priv->dumping)
    {
      GError *local_error = NULL;
      if (!read_netlink_messages (nl, &local_error))
        {
          g_warning ("%s", local_error->message);
          g_clear_error (&local_error);
          break;
        }
    }

  g_socket_set_blocking (nl->priv->monitor_sock, FALSE);
  nl->priv->source = g_socket_create_source (nl->priv->monitor_sock, G_IO_IN, NULL);
  g_source_set_callback (nl->priv->source,
                         (GSourceFunc) read_netlink_messages_callback, nl, NULL);
  g_source_attach (nl->priv->source, nl->priv->context);

  return TRUE;
}

static gboolean
g_network_monitor_netlink_initable_init (GInitable     *initable,
                                         GCancellable  *cancellable,
                                         GError       **error)
{
  GNetworkMonitorNetlink *nl = G_NETWORK_MONITOR_NETLINK (initable);

  g_mutex_init (&nl->priv->query_lock);
  nl->priv->context = g_main_context_ref_thread_default ();

  /* A socket for on-demand per-destination route lookups, used by can_reach()
   * for this backend and every subclass. */
  nl->priv->query_sock = create_netlink_socket (0, error);
  if (!nl->priv->query_sock)
    return FALSE;

  /* Availability tracking is set up only for the plain netlink type; see the
   * GNetworkMonitorNetlinkPrivate definition. Subclasses inherit can_reach(). */
  if (G_OBJECT_TYPE (nl) == G_TYPE_NETWORK_MONITOR_NETLINK &&
      !setup_monitor (nl, error))
    return FALSE;

  return initable_parent_iface->init (initable, cancellable, error);
}

static gboolean
request_dump (GNetworkMonitorNetlink  *nl,
              GError                 **error)
{
  struct nlmsghdr *n;
  struct rtmsg *rtm;
  gchar buf[NLMSG_SPACE (sizeof (*rtm))];

#ifdef NETLINK_GET_STRICT_CHK
  /* Strict checking with rtm_flags == 0 lets the kernel dump faster by
   * skipping unnecessary data, e.g. the route-cache exceptions. */
  {
    const int strict = 1;
    if (setsockopt (g_socket_get_fd (nl->priv->monitor_sock), SOL_NETLINK,
                    NETLINK_GET_STRICT_CHK, &strict, sizeof (strict)) != 0)
      g_debug ("Could not set NETLINK_GET_STRICT_CHK: %s", g_strerror (errno));
  }
#endif

  memset (buf, 0, sizeof (buf));
  n = (struct nlmsghdr*) buf;
  n->nlmsg_len = NLMSG_LENGTH (sizeof (*rtm));
  n->nlmsg_type = RTM_GETROUTE;
  n->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  n->nlmsg_pid = 0;
  rtm = NLMSG_DATA (n);
  rtm->rtm_family = AF_UNSPEC;
  /* Keep this request minimal: a strict dump requires the selector fields to
   * stay zero, and extra attributes can also hurt dump performance. */

  if (g_socket_send (nl->priv->monitor_sock, buf, sizeof (buf),
                     NULL, error) < 0)
    {
      g_prefix_error (error, "%s", _("Could not get network status: "));
      return FALSE;
    }

  nl->priv->dumping = TRUE;
  nl->priv->dump_ipv4_defaults = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  nl->priv->dump_ipv6_defaults = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  return TRUE;
}

static gboolean
timeout_request_dump (gpointer user_data)
{
  GNetworkMonitorNetlink *nl = user_data;

  g_source_destroy (nl->priv->dump_source);
  g_source_unref (nl->priv->dump_source);
  nl->priv->dump_source = NULL;

  request_dump (nl, NULL);

  return FALSE;
}

/* Schedule a full re-dump to rebuild default-route state after the monitor
 * socket overflows (ENOBUFS). Normal default-route changes are tracked
 * incrementally and do not trigger a dump. */
static void
queue_request_dump (GNetworkMonitorNetlink *nl)
{
  if (nl->priv->dumping)
    return;

  if (nl->priv->dump_source)
    {
      g_source_destroy (nl->priv->dump_source);
      g_source_unref (nl->priv->dump_source);
    }

  nl->priv->dump_source = g_timeout_source_new_seconds (1);
  g_source_set_callback (nl->priv->dump_source,
                         (GSourceFunc) timeout_request_dump, nl, NULL);
  g_source_attach (nl->priv->dump_source, nl->priv->context);
}

static GInetAddressMask *
default_route_mask (GNetworkMonitorNetlink *nl,
                    int                     family)
{
  GInetAddressMask **maskp;
  GSocketFamily socket_family;

  if (family == AF_INET)
    {
      maskp = &nl->priv->ipv4_default_mask;
      socket_family = G_SOCKET_FAMILY_IPV4;
    }
  else if (family == AF_INET6)
    {
      maskp = &nl->priv->ipv6_default_mask;
      socket_family = G_SOCKET_FAMILY_IPV6;
    }
  else
    {
      g_assert_not_reached ();
    }

  if (*maskp == NULL)
    {
      GInetAddress *any = g_inet_address_new_any (socket_family);
      *maskp = g_inet_address_mask_new (any, 0, NULL);
      g_object_unref (any);
    }

  return *maskp;
}

/* Reflect presence of a default route for @family into the base monitor's
 * network set, which drives the network-available property. */
static void
update_default_route_available (GNetworkMonitorNetlink *nl,
                                int                     family,
                                gboolean                available)
{
  GInetAddressMask *mask = default_route_mask (nl, family);

  if (mask == NULL)
    return;

  if (available)
    g_network_monitor_base_add_network (G_NETWORK_MONITOR_BASE (nl), mask);
  else
    g_network_monitor_base_remove_network (G_NETWORK_MONITOR_BASE (nl), mask);
}

static void
add_default_route (GNetworkMonitorNetlink *nl,
                   int                     family,
                   gchar                  *stolen_key /* (transfer full) */)
{
  GHashTable *set;

  if (nl->priv->dumping)
    set = (family == AF_INET) ? nl->priv->dump_ipv4_defaults : nl->priv->dump_ipv6_defaults;
  else
    set = (family == AF_INET) ? nl->priv->ipv4_defaults : nl->priv->ipv6_defaults;

  if (!nl->priv->dumping && g_hash_table_size (set) == 0)
    update_default_route_available (nl, family, TRUE);

  g_hash_table_add (set, g_steal_pointer (&stolen_key));
}

static void
remove_default_route (GNetworkMonitorNetlink *nl,
                      int                     family,
                      const gchar            *key)
{
  GHashTable *set;

  /* An RTM_DELROUTE is always a multicast event and can interleave with a
   * (re-)dump, so this may run while dumping; update the live set, which
   * finish_dump() then reconciles against the freshly dumped set. */
  set = (family == AF_INET) ? nl->priv->ipv4_defaults : nl->priv->ipv6_defaults;

  if (g_hash_table_remove (set, key) &&
      g_hash_table_size (set) == 0)
    update_default_route_available (nl, family, FALSE);
}

static void
finish_dump (GNetworkMonitorNetlink *nl)
{
  int families[2] = { AF_INET, AF_INET6 };
  size_t i;

  /* Reconcile the freshly-dumped default routes with the live state, emitting
   * an availability change only when the per-family presence actually flips. */
  for (i = 0; i < G_N_ELEMENTS (families); i++)
    {
      int family = families[i];
      GHashTable **livep = (family == AF_INET) ? &nl->priv->ipv4_defaults : &nl->priv->ipv6_defaults;
      GHashTable **dumpp = (family == AF_INET) ? &nl->priv->dump_ipv4_defaults : &nl->priv->dump_ipv6_defaults;
      gboolean was_available = g_hash_table_size (*livep) > 0;
      gboolean now_available = g_hash_table_size (*dumpp) > 0;

      g_hash_table_unref (*livep);
      *livep = g_steal_pointer (dumpp);

      if (was_available != now_available)
        update_default_route_available (nl, family, now_available);
    }

  nl->priv->dumping = FALSE;

  /* FreeBSD features "jailing" functionality, which can be approximated to
   * Linux namespaces. A jail may or may not share the host's network stack,
   * which includes routing tables.
   * When jail runs in non-vnet mode and has a shared stack with the host,
   * the kernel prevents jailed processes from getting full view on a routing
   * table. This makes GNetworkManager believe that we're offline and return
   * FALSE for the "available" property.
   * To workaround this problem, do the same thing as GNetworkMonitorBase -
   * add a fake network of 0 length.
   */
#ifdef __FreeBSD__
  gboolean is_jailed = FALSE;
  gsize len = sizeof (is_jailed);

  if (sysctlbyname ("security.jail.jailed", &is_jailed, &len, NULL, 0) != 0)
    return;

  if (!is_jailed)
    return;

  if (is_jailed && !g_network_monitor_get_network_available (G_NETWORK_MONITOR (nl)))
    {
      GInetAddressMask *network;
      network = g_inet_address_mask_new_from_string ("0.0.0.0/0", NULL);
      g_network_monitor_base_add_network (G_NETWORK_MONITOR_BASE (nl), network);
      g_object_unref (network);
    }
#endif
}

/* A route whose type does not actually deliver packets to the destination
 * (unreachable, blackhole, prohibit, throw) does not count as reachable. */
static gboolean
route_type_is_reachable (unsigned char rtm_type)
{
  return (rtm_type != RTN_UNREACHABLE &&
          rtm_type != RTN_BLACKHOLE &&
          rtm_type != RTN_PROHIBIT &&
          rtm_type != RTN_THROW);
}

/* Sane upper bound on a single netlink datagram before g_malloc(): a route-get
 * reply is one message (<= NLMSG_GOODSIZE, < 8 KiB), while a dump datagram packs
 * many messages and is hard-capped by the kernel at SKB_WITH_OVERHEAD(32768)
 * (~32 KiB, see netlink_recvmsg()). 64 KiB leaves headroom over both. */
#define NETLINK_MESSAGE_MAX_SIZE (64 * 1024)

static gboolean
read_netlink_messages (GNetworkMonitorNetlink  *nl,
                       GError                 **error)
{
  GInputVector iv;
  gssize len;
  gint flags;
  GError *local_error = NULL;
  GSocketAddress *addr = NULL;
  struct nlmsghdr *msg;
  struct rtmsg *rtmsg;
  struct rtattr *attr;
  struct sockaddr_nl source_sockaddr;
  gsize attrlen;
  guint32 table, metric, oif;
  gboolean have_nexthop;

  /* The kernel is aware of the read buffer size, so a larger buffer lets it
   * deliver more data per datagram. */
  iv.buffer = g_malloc (NETLINK_MESSAGE_MAX_SIZE);
  iv.size = NETLINK_MESSAGE_MAX_SIZE;
  flags = 0;
  len = g_socket_receive_message (nl->priv->monitor_sock, &addr, &iv, 1,
                                  NULL, NULL, &flags, NULL, &local_error);
  if (len < 0)
    goto done;
  if (len == 0 || (flags & MSG_TRUNC))
    {
      g_set_error_literal (&local_error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                           "netlink message length is invalid");
      goto done;
    }

  if (!g_socket_address_to_native (addr, &source_sockaddr, sizeof (source_sockaddr), &local_error))
    goto done;

  /* If the sender port id is 0 (not fakeable) then the message is from the kernel */
  if (source_sockaddr.nl_pid != 0)
    goto done;

  msg = (struct nlmsghdr *) iv.buffer;
  for (; len > 0; msg = NLMSG_NEXT (msg, len))
    {
      if (!NLMSG_OK (msg, (size_t) len))
        {
          g_set_error_literal (&local_error,
                               G_IO_ERROR,
                               G_IO_ERROR_PARTIAL_INPUT,
                               "netlink message was truncated; shouldn't happen...");
          goto done;
        }

      switch (msg->nlmsg_type)
        {
        case RTM_NEWROUTE:
        case RTM_DELROUTE:
          rtmsg = NLMSG_DATA (msg);

          if (rtmsg->rtm_family != AF_INET && rtmsg->rtm_family != AF_INET6)
            continue;
          /* Only default routes (0-length prefix) affect network-available. */
          if (rtmsg->rtm_dst_len != 0)
            continue;
          if (!route_type_is_reachable (rtmsg->rtm_type))
            continue;

          table = rtmsg->rtm_table;
          metric = 0;
          oif = 0;
          have_nexthop = FALSE;

          attrlen = NLMSG_PAYLOAD (msg, sizeof (struct rtmsg));
          attr = RTM_RTA (rtmsg);
          while (RTA_OK (attr, attrlen))
            {
              if (attr->rta_type == RTA_GATEWAY)
                have_nexthop = TRUE;
              else if (attr->rta_type == RTA_OIF)
                {
                  oif = *(guint32 *) RTA_DATA (attr);
                  have_nexthop = TRUE;
                }
              else if (attr->rta_type == RTA_PRIORITY)
                metric = *(guint32 *) RTA_DATA (attr);
              else if (attr->rta_type == RTA_TABLE)
                table = *(guint32 *) RTA_DATA (attr);
              else if (attr->rta_type == RTA_MULTIPATH)
                have_nexthop = TRUE;
              attr = RTA_NEXT (attr, attrlen);
            }

          /* A default route reaches a nexthop: a gateway, an output interface,
           * or a set of multipath nexthops. */
          if (have_nexthop)
            {
              /* Key by attributes that survive "ip route replace" (which emits
               * RTM_NEWROUTE with no matching RTM_DELROUTE), so the set tracks
               * distinct default routes without drifting. */
              gchar *key = g_strdup_printf ("%u:%u:%u", table, metric, oif);

              if (msg->nlmsg_type == RTM_NEWROUTE)
                add_default_route (nl, rtmsg->rtm_family, g_steal_pointer (&key));
              else
                remove_default_route (nl, rtmsg->rtm_family, key);
              g_free (key);
            }
          break;

        case NLMSG_DONE:
          finish_dump (nl);
          goto done;

        case NLMSG_ERROR:
          {
            struct nlmsgerr *e = NLMSG_DATA (msg);

            /* ENOBUFS means the socket buffer overflowed and we may have missed
             * default-route changes; resync by re-dumping. */
            if (e->error == -ENOBUFS)
              {
                queue_request_dump (nl);
                goto done;
              }

            g_set_error (&local_error,
                         G_IO_ERROR,
                         g_io_error_from_errno (-e->error),
                         "netlink error: %s",
                         g_strerror (-e->error));
          }
          goto done;

        default:
          g_set_error (&local_error,
                       G_IO_ERROR,
                       G_IO_ERROR_INVALID_DATA,
                       "unexpected netlink message %d",
                       msg->nlmsg_type);
          goto done;
        }
    }

 done:
  g_free (iv.buffer);
  g_clear_object (&addr);

  if (local_error != NULL && nl->priv->dumping)
    finish_dump (nl);

  if (local_error != NULL)
    {
      g_propagate_prefixed_error (error, local_error, "Error on netlink socket: ");
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}

static gboolean
read_netlink_messages_callback (GSocket      *socket,
                                GIOCondition  condition,
                                gpointer      user_data)
{
  GError *error = NULL;
  GNetworkMonitorNetlink *nl = G_NETWORK_MONITOR_NETLINK (user_data);

  if (!read_netlink_messages (nl, &error))
    {
      g_warning ("Error reading netlink message: %s", error->message);
      g_clear_error (&error);
      return FALSE;
    }

  return TRUE;
}

/* Map a GSocketFamily to its AF_* address family constant. */
static int
socket_family_to_af_family (GSocketFamily socket_family)
{
  switch (socket_family)
    {
    case G_SOCKET_FAMILY_IPV4:
      return AF_INET;
    case G_SOCKET_FAMILY_IPV6:
      return AF_INET6;
    default:
      g_assert_not_reached ();
    }
}

/* Map a kernel route-lookup errno to the GIOError reported by can_reach(). */
static GIOErrorEnum
route_get_error_from_errno (int err)
{
  switch (err)
    {
    case ENETUNREACH:
    case ENETDOWN:
      return G_IO_ERROR_NETWORK_UNREACHABLE;
    case EHOSTUNREACH:
    case EHOSTDOWN:
    case EACCES:
      return G_IO_ERROR_HOST_UNREACHABLE;
    default:
      return g_io_error_from_errno (err);
    }
}

/* Perform a single per-destination route lookup ("ip route get"). Returns TRUE
 * if @addr is reachable, otherwise FALSE with @error set. The error is usually
 * G_IO_ERROR_HOST_UNREACHABLE or G_IO_ERROR_NETWORK_UNREACHABLE, but any other
 * GIOError that route_get_error_from_errno() maps from the kernel's errno is
 * possible. Blocking, and safe to call from a worker thread. */
static gboolean
route_get (GNetworkMonitorNetlink  *nl,
           GInetAddress            *addr,
           GCancellable            *cancellable,
           GError                 **error)
{
  GSocketFamily socket_family = g_inet_address_get_family (addr);
  int family = socket_family_to_af_family (socket_family);
  gsize addrlen = g_inet_address_get_native_size (addr);
  const guint8 *addrbytes = g_inet_address_to_bytes (addr);
  struct {
    struct nlmsghdr n;
    struct rtmsg    r;
    gchar           buf[64];
  } req;
  struct nlmsghdr *msg;
  struct rtmsg *rtmsg;
  struct rtattr *rta;
  GInputVector iv = { NULL, 0 };
  struct sockaddr_nl source_sockaddr;
  GSocketAddress *src_addr = NULL;
  GError *local_error = NULL;
  gssize len;
  gint flags;
  guint seq;
  gboolean reachable = FALSE;
  gboolean got_reply = FALSE;

  memset (&req, 0, sizeof (req));
  req.n.nlmsg_len = NLMSG_LENGTH (sizeof (struct rtmsg));
  req.n.nlmsg_type = RTM_GETROUTE;
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.r.rtm_family = family;
  req.r.rtm_dst_len = addrlen * 8;  /* this field is a CIDR length in bits */

  /* We want the kernel to resolve the route to @addr. RTM_F_FIB_MATCH
   * (Linux 4.11+) makes it return the matching FIB entry; without the flag the
   * kernel returns a per-destination cloned route instead, which still works
   * as we are currently interested in a route existence. */
#ifdef RTM_F_FIB_MATCH
  req.r.rtm_flags = RTM_F_FIB_MATCH;
#endif

  rta = (struct rtattr *) (((char *) &req) + NLMSG_ALIGN (req.n.nlmsg_len));
  rta->rta_type = RTA_DST;
  rta->rta_len = RTA_LENGTH (addrlen);
  g_assert ((char *) rta + rta->rta_len <= (char *) &req + sizeof (req));
  memcpy (RTA_DATA (rta), addrbytes, addrlen);
  req.n.nlmsg_len = NLMSG_ALIGN (req.n.nlmsg_len) + rta->rta_len;

  g_mutex_lock (&nl->priv->query_lock);

  seq = ++nl->priv->query_seq;
  req.n.nlmsg_seq = seq;

  if (g_socket_send (nl->priv->query_sock, (const gchar *) &req, req.n.nlmsg_len,
                     cancellable, &local_error) < 0)
    goto done;

  flags = MSG_PEEK | MSG_TRUNC;
  len = g_socket_receive_message (nl->priv->query_sock, NULL, &iv, 1,
                                  NULL, NULL, &flags, cancellable, &local_error);
  if (len < 0)
    goto done;
  if (len == 0 || len > NETLINK_MESSAGE_MAX_SIZE)
    {
      g_set_error_literal (&local_error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                           "netlink route reply length is invalid");
      goto done;
    }

  iv.buffer = g_malloc (len);
  iv.size = len;
  len = g_socket_receive_message (nl->priv->query_sock, &src_addr, &iv, 1,
                                  NULL, NULL, NULL, cancellable, &local_error);
  if (len < 0)
    goto done;

  /* Only trust messages from the kernel (sender port id 0). */
  if (!g_socket_address_to_native (src_addr, &source_sockaddr, sizeof (source_sockaddr), &local_error))
    goto done;
  if (source_sockaddr.nl_pid != 0)
    goto done;

  msg = (struct nlmsghdr *) iv.buffer;
  for (; len > 0; msg = NLMSG_NEXT (msg, len))
    {
      if (!NLMSG_OK (msg, (size_t) len))
        break;
      if (msg->nlmsg_seq != seq)
        continue;

      if (msg->nlmsg_type == NLMSG_ERROR)
        {
          struct nlmsgerr *e = NLMSG_DATA (msg);

          got_reply = TRUE;
          /* error == 0 is a success acknowledgement, which the kernel never sends for our request. */
          if (e->error != 0)
            g_set_error_literal (&local_error, G_IO_ERROR,
                                 route_get_error_from_errno (-e->error),
                                 g_strerror (-e->error));
          break;
        }
      else if (msg->nlmsg_type == RTM_NEWROUTE)
        {
          rtmsg = NLMSG_DATA (msg);
          got_reply = TRUE;
          reachable = route_type_is_reachable (rtmsg->rtm_type);
          if (!reachable)
            g_set_error_literal (&local_error, G_IO_ERROR,
                                 G_IO_ERROR_HOST_UNREACHABLE,
                                 _("Host unreachable"));
          break;
        }
    }

 done:
  g_mutex_unlock (&nl->priv->query_lock);

  g_free (iv.buffer);
  g_clear_object (&src_addr);

  if (reachable)
    {
      g_assert (local_error == NULL);
      return TRUE;
    }

  if (local_error != NULL)
    g_propagate_error (error, g_steal_pointer (&local_error));
  else if (!got_reply)
    g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_HOST_UNREACHABLE,
                         _("Host unreachable"));
  return FALSE;
}

static gboolean
g_network_monitor_netlink_can_reach (GNetworkMonitor     *monitor,
                                     GSocketConnectable  *connectable,
                                     GCancellable        *cancellable,
                                     GError             **error)
{
  GNetworkMonitorNetlink *nl = G_NETWORK_MONITOR_NETLINK (monitor);
  GSocketAddressEnumerator *enumerator;
  GSocketAddress *addr;
  GError *local_error = NULL;

  enumerator = g_socket_connectable_proxy_enumerate (connectable);

  /* Try each resolved address; the host is reachable if any of them is. A
   * failed route_get() is cleared so the next g_socket_address_enumerator_next()
   * sees a clean @local_error, except on cancellation, which we propagate. */
  while ((addr = g_socket_address_enumerator_next (enumerator, cancellable, &local_error)))
    {
      if (G_IS_INET_SOCKET_ADDRESS (addr))
        {
          GInetAddress *iaddr;

          iaddr = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (addr));
          if (route_get (nl, iaddr, cancellable, &local_error))
            {
              g_object_unref (addr);
              g_object_unref (enumerator);
              return TRUE;
            }

          if (g_cancellable_is_cancelled (cancellable))
            {
              g_object_unref (addr);
              break;
            }
          g_clear_error (&local_error);
        }

      g_object_unref (addr);
    }
  g_object_unref (enumerator);

  /* An enumeration failure (DNS) or cancellation is reported as-is. */
  if (local_error != NULL)
    {
      g_propagate_error (error, g_steal_pointer (&local_error));
      return FALSE;
    }

  /* Reached when no address was routable (each route_get() error was cleared to
   * try the next one) or none were `GInetSocketAddress`es; either way report a
   * generic error, matching the base monitor. */
  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_HOST_UNREACHABLE,
                       _("Host unreachable"));
  return FALSE;
}

static void
can_reach_thread (GTask        *task,
                  gpointer      source_object,
                  gpointer      task_data,
                  GCancellable *cancellable)
{
  GNetworkMonitor *monitor = G_NETWORK_MONITOR (source_object);
  GSocketConnectable *connectable = G_SOCKET_CONNECTABLE (task_data);
  GError *error = NULL;

  if (g_network_monitor_netlink_can_reach (monitor, connectable, cancellable, &error))
    g_task_return_boolean (task, TRUE);
  else
    g_task_return_error (task, g_steal_pointer (&error));
}

static void
g_network_monitor_netlink_can_reach_async (GNetworkMonitor     *monitor,
                                           GSocketConnectable  *connectable,
                                           GCancellable        *cancellable,
                                           GAsyncReadyCallback  callback,
                                           gpointer             user_data)
{
  GTask *task;

  task = g_task_new (monitor, cancellable, callback, user_data);
  g_task_set_source_tag (task, g_network_monitor_netlink_can_reach_async);
  g_task_set_static_name (task, "[gio] network monitor can_reach");
  g_task_set_task_data (task, g_object_ref (connectable), g_object_unref);

  /* A route lookup is a quick kernel round-trip, so the shared GTask thread
   * pool is fine here. */
  g_task_run_in_thread (task, can_reach_thread);
  g_object_unref (task);
}

static gboolean
g_network_monitor_netlink_can_reach_finish (GNetworkMonitor  *monitor,
                                            GAsyncResult     *result,
                                            GError          **error)
{
  g_return_val_if_fail (g_task_is_valid (result, monitor), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
g_network_monitor_netlink_finalize (GObject *object)
{
  GNetworkMonitorNetlink *nl = G_NETWORK_MONITOR_NETLINK (object);

  if (nl->priv->source)
    {
      g_source_destroy (nl->priv->source);
      g_source_unref (nl->priv->source);
    }

  if (nl->priv->dump_source)
    {
      g_source_destroy (nl->priv->dump_source);
      g_source_unref (nl->priv->dump_source);
    }

  if (nl->priv->query_sock)
    {
      g_socket_close (nl->priv->query_sock, NULL);
      g_object_unref (nl->priv->query_sock);
    }

  if (nl->priv->monitor_sock)
    {
      g_socket_close (nl->priv->monitor_sock, NULL);
      g_object_unref (nl->priv->monitor_sock);
    }

  g_mutex_clear (&nl->priv->query_lock);

  g_clear_pointer (&nl->priv->context, g_main_context_unref);
  g_clear_pointer (&nl->priv->ipv4_defaults, g_hash_table_unref);
  g_clear_pointer (&nl->priv->ipv6_defaults, g_hash_table_unref);
  g_clear_pointer (&nl->priv->dump_ipv4_defaults, g_hash_table_unref);
  g_clear_pointer (&nl->priv->dump_ipv6_defaults, g_hash_table_unref);
  g_clear_object (&nl->priv->ipv4_default_mask);
  g_clear_object (&nl->priv->ipv6_default_mask);

  G_OBJECT_CLASS (g_network_monitor_netlink_parent_class)->finalize (object);
}

static void
g_network_monitor_netlink_class_init (GNetworkMonitorNetlinkClass *nl_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (nl_class);

  gobject_class->finalize = g_network_monitor_netlink_finalize;
}

static void
g_network_monitor_netlink_iface_init (GNetworkMonitorInterface *monitor_iface)
{
  monitor_iface->can_reach = g_network_monitor_netlink_can_reach;
  monitor_iface->can_reach_async = g_network_monitor_netlink_can_reach_async;
  monitor_iface->can_reach_finish = g_network_monitor_netlink_can_reach_finish;
}

static void
g_network_monitor_netlink_initable_iface_init (GInitableIface *iface)
{
  initable_parent_iface = g_type_interface_peek_parent (iface);

  iface->init = g_network_monitor_netlink_initable_init;
}
