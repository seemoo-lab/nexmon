/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2023 Leo Zi-You Assini <leoziyou@amazon.it>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <net/route.h>
#include <sys/sysctl.h>

#include "ginetaddress.h"
#include "ginetaddressmask.h"
#include "ginitable.h"
#include "gio.h"
#include "gioerror.h"
#include "giomodule-priv.h"
#include "gnetworkmonitor.h"
#include "gosxnetworkmonitor.h"
#include "gstdio.h"

static GInitableIface *initable_parent_iface;
static void g_osx_network_monitor_iface_init (GNetworkMonitorInterface *iface);
static void g_osx_network_monitor_initable_iface_init (GInitableIface *iface);

typedef struct
{
  GSource source;
  gint fd;
  gpointer tag;
} GOsxNetworkMonitorSource;

static gboolean
osx_network_monitor_source_dispatch (GSource *source,
                                     GSourceFunc callback,
                                     gpointer user_data)
{
  gboolean ret;

  g_return_val_if_fail (callback != NULL, FALSE);

  ret = callback (user_data);

  return ret;
}

static GSource *
osx_network_monitor_source_new (gint sockfd)
{
  static GSourceFuncs source_funcs = {
    NULL, /* prepare */
    NULL, /* check */
    osx_network_monitor_source_dispatch,
    NULL, /* finalize */
    NULL, /* closure */
    NULL  /* marshal */
  };

  GSource *source;
  GOsxNetworkMonitorSource *network_monitor_source;

  source = g_source_new (&source_funcs, sizeof (GOsxNetworkMonitorSource));
  network_monitor_source = (GOsxNetworkMonitorSource *) source;

  network_monitor_source->fd = sockfd;
  network_monitor_source->tag = g_source_add_unix_fd (source, sockfd, G_IO_IN);

  g_debug ("Created source for fd=%d", sockfd);

  return source;
}

struct _GOsxNetworkMonitor
{
  GNetworkMonitorBase parent_instance;

  gint sockfd;
  char msg_buffer[sizeof (struct rt_msghdr) + sizeof (struct sockaddr) * 8];
  GSource *route_change_source; /* (owned) (nullable) */
};

G_DEFINE_TYPE_WITH_CODE (GOsxNetworkMonitor, g_osx_network_monitor, G_TYPE_NETWORK_MONITOR_BASE, G_IMPLEMENT_INTERFACE (G_TYPE_NETWORK_MONITOR, g_osx_network_monitor_iface_init) G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, g_osx_network_monitor_initable_iface_init) _g_io_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_NETWORK_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "osx",
                                                         20))

#define ROUNDUP(a, size) (((a) & ((size) -1)) ? (1 + ((a) | ((size) -1))) : (a))

#define NEXT_SA(ap) ap = (struct sockaddr *) ((caddr_t) ap + (ap->sa_len ? ROUNDUP (ap->sa_len, sizeof (u_long)) : sizeof (u_long)))

/* Extract the sockaddrs from @sa into @rti_info according to the mask in @addrs.
 * @rti_info must be allocated by the caller and have at least as many elements
 * as there are high bits in @addrs, up to `RTAX_MAX` elements.
 */
static void
get_rtaddrs (unsigned int addrs_mask,
             const struct sockaddr *sa,
             const struct sockaddr **rti_info)
{
  for (unsigned int i = 0; i < RTAX_MAX; i++)
    {
      if (addrs_mask & (1 << i))
        {
          rti_info[i] = sa;
          NEXT_SA (sa);
        }
      else
        rti_info[i] = NULL;
    }
}

/* Returns the position of the last positive bit
 *
 * 0.0.0.0 (00000000.00000000.0000000.0000000) => 0
 * 255.255.255.255 (11111111.11111111.11111111.11111111) => 32
 * 0.0.0.1 (00000000.00000000.0000000.0000001) => 32
 * 32.0.0.0 (00100001.00000000.0000000.0000000) => 8
 * 0.16.0.16 (00000000.00010000.0000000.00010000) => 28
 */
static gsize
get_last_bit_position (const guint8 *ip,
                       gsize len_in_bits)
{
  gssize i;
  gssize bytes = (gssize) len_in_bits / 8;
  gulong ip_in_binary = 0;

  for (i = 0; i < bytes; i++)
    ip_in_binary = (ip_in_binary << 8) | ip[i];

  if (ip_in_binary == 0)
    return 0;

  gsize last_bit_position = len_in_bits - g_bit_nth_lsf (ip_in_binary, -1);

  return (gsize) last_bit_position;
}

static GInetAddressMask *
get_network_mask (const struct rt_msghdr *rtm)
{
  GInetAddressMask *network = NULL;
  GInetAddress *dest_addr;
  const struct sockaddr *sa = (struct sockaddr *) (rtm + 1);
  const struct sockaddr *nm;
  const struct sockaddr *rti_info[RTAX_MAX];
  GSocketFamily family;
  const guint8 *dest;
  gsize len;
  GError *error = NULL;

  get_rtaddrs (rtm->rtm_addrs, sa, rti_info);

  sa = rti_info[RTAX_DST];
  if (sa == NULL)
    return NULL;

  nm = rti_info[RTAX_NETMASK];
  if (nm == NULL)
    return NULL;

  /* Get IP information */
  switch (sa->sa_family)
    {
    case AF_UNSPEC:
      /* Fall-through: AF_UNSPEC delivers both IPv4 and IPv6 infos, let's stick with IPv4 here */
    case AF_INET:
      family = G_SOCKET_FAMILY_IPV4;
      /* For IPv4 sin_addr is a guint8 array[4], e.g [255, 255, 255, 255] */
      dest = (guint8 *) &((struct sockaddr_in *) sa)->sin_addr;
      len = get_last_bit_position (dest, 32);
      break;
    case AF_INET6:
      /* Skip IPv6 here as OSX keeps a default route to a tunneling device even if disconnected */
      return NULL;
    default:
      return NULL;
    }

  /* Create dest address */
  if (dest != NULL)
    dest_addr = g_inet_address_new_from_bytes (dest, family);
  else
    dest_addr = g_inet_address_new_any (family);

  /* Create and return network mask */
  network = g_inet_address_mask_new (dest_addr, len, &error);

  if (network == NULL)
    {
      g_warning ("Unable to create network mask: %s", error->message);
      g_error_free (error);
    }

  g_object_unref (dest_addr);

  return g_steal_pointer (&network);
}

static gboolean
osx_network_manager_process_table (GOsxNetworkMonitor *self,
                                   GError **error)
{
  GPtrArray *networks;
  gsize n_networks;
  struct rt_msghdr *rtm;
  gint mib[6];
  gsize needed;
  gchar *buf;
  gchar *limit;
  gchar *next;

  /* Create Management Information Base
   * System information is stored in a hierarchial tree structure. By specifying each array element the search can be refined.
   * First array element is the top level name which is always prefixed by CTL_ (examples include CTL_VFS for file system information, CTL_HW for user-level information etc...)
   * https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man3/sysctl.3.html
   */
  mib[0] = CTL_NET;  /* CTL_NET = Network related information */
  mib[1] = PF_ROUTE; /* PF_ROUTE = Retrieve entire routing table */

  mib[2] = 0; /* 0 = protocol number, which is currently always 0 */
  mib[3] = 0; /* 0 = Retrieve all address families */
  mib[4] = NET_RT_DUMP;
  mib[5] = 0;

  /* Request size of buffer */
  if (sysctl (mib, G_N_ELEMENTS (mib), NULL, &needed, NULL, 0) < 0 || needed == 0)
    {
      int saved_errno = errno;

      g_set_error_literal (error, G_IO_ERROR, g_io_error_from_errno (saved_errno),
                           "Could not request buffer size");

      return FALSE;
    }

  /* Allocate memory */
  buf = g_malloc0 (needed);

  /* Request needed bytes in buffer for routing table */
  if (sysctl (mib, G_N_ELEMENTS (mib), buf, &needed, NULL, 0) < 0)
    {
      int saved_errno = errno;

      g_set_error_literal (error, G_IO_ERROR, g_io_error_from_errno (saved_errno),
                           "Could not request buffer");

      g_free (buf);

      return FALSE;
    }

  limit = buf + needed;

  networks = g_ptr_array_new_with_free_func (g_object_unref);
  for (next = buf; next < limit; next += rtm->rtm_msglen)
    {
      GInetAddressMask *network;

      rtm = (struct rt_msghdr *) next;

      network = get_network_mask (rtm);
      if (network == NULL)
        continue;

      g_ptr_array_add (networks, g_steal_pointer (&network));
    }

  n_networks = networks->len;
  g_network_monitor_base_set_networks (G_NETWORK_MONITOR_BASE (self),
                                       (GInetAddressMask **) g_ptr_array_steal (networks, &n_networks),
                                       n_networks);

  g_free (buf);

  return TRUE;
}

static void
clear_network_monitor (GOsxNetworkMonitor *self)
{
  g_debug ("Clearing source for fd=%d", self->sockfd);

  if (self->route_change_source != NULL)
    {
      g_source_destroy (self->route_change_source);
      g_clear_pointer (&self->route_change_source, g_source_unref);
    }

  g_clear_fd (&self->sockfd, NULL);
}

static gboolean
osx_network_monitor_callback (gpointer user_data)
{
  GOsxNetworkMonitor *self = user_data;
  gint32 read_msg;
  GInetAddressMask *network;

  memset (&self->msg_buffer, 0, sizeof (self->msg_buffer));
  read_msg = read (self->sockfd, self->msg_buffer, sizeof (self->msg_buffer));

  /* Skip read if we have no data */
  if (read_msg == -1 && errno == EAGAIN)
    {
      return G_SOURCE_CONTINUE;
    }

  if (read_msg <= 0)
    {
      g_warning ("Unable to monitor network change: failed to read from socket");
      clear_network_monitor (self);
      return G_SOURCE_REMOVE;
    }

  /* Check if it is a type of interest */
  switch (((struct rt_msghdr *) self->msg_buffer)->rtm_type)
    {
    case RTM_ADD:
      network = get_network_mask ((struct rt_msghdr *) self->msg_buffer);
      if (network != NULL)
        {
          g_network_monitor_base_add_network (G_NETWORK_MONITOR_BASE (self), network);
          g_object_unref (network);
        }
      break;
    case RTM_DELETE:
      network = get_network_mask ((struct rt_msghdr *) self->msg_buffer);
      if (network != NULL)
        {
          g_network_monitor_base_remove_network (G_NETWORK_MONITOR_BASE (self), network);
          g_object_unref (network);
        }
      break;
    default:
      break;
    }

  return G_SOURCE_CONTINUE;
}

static gboolean
g_osx_network_monitor_start_monitoring (GOsxNetworkMonitor *self,
                                        GError **error)
{
  GSource *source;

  self->sockfd = socket (PF_ROUTE, SOCK_RAW, 0);
  if (self->sockfd == -1)
    {
      int saved_errno = errno;

      g_set_error_literal (error, G_IO_ERROR, g_io_error_from_errno (saved_errno),
                           "Failed to create PF_ROUTE socket");

      return FALSE;
    }

  /* FIXME: Currently it is not possible to set SOCK_NONBLOCK and SOCK_CLOEXEC
   * in the socket constructor so workaround is this racy call to fcntl. Should be
   * replaced once the flags are supported. */
  fcntl (self->sockfd, F_SETFL, fcntl (self->sockfd, F_GETFL, 0) | O_NONBLOCK | O_CLOEXEC);

  source = osx_network_monitor_source_new (self->sockfd);

  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_set_callback (source,
                         osx_network_monitor_callback,
                         self,
                         NULL);
  g_source_attach (source, NULL);

  self->route_change_source = g_steal_pointer (&source);

  return TRUE;
}

static void
g_osx_network_monitor_init (GOsxNetworkMonitor *self)
{
  self->sockfd = -1;
}

static gboolean
g_osx_network_monitor_initable_init (GInitable *initable,
                                     GCancellable *cancellable,
                                     GError **error)
{
  GOsxNetworkMonitor *self = G_OSX_NETWORK_MONITOR (initable);

  /* Read current IP routing table. */
  if (!osx_network_manager_process_table (self, error))
    {
      return FALSE;
    }

  /* Start monitoring */
  if (!g_osx_network_monitor_start_monitoring (self, error))
    {
      return FALSE;
    }

  return initable_parent_iface->init (initable, cancellable, error);
}

static void
g_osx_network_monitor_finalize (GObject *object)
{
  GOsxNetworkMonitor *self = G_OSX_NETWORK_MONITOR (object);

  clear_network_monitor (self);

  G_OBJECT_CLASS (g_osx_network_monitor_parent_class)->finalize (object);
}

static void
g_osx_network_monitor_class_init (GOsxNetworkMonitorClass *osx_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (osx_class);

  gobject_class->finalize = g_osx_network_monitor_finalize;
}

static void
g_osx_network_monitor_iface_init (GNetworkMonitorInterface *monitor_iface)
{
}

static void
g_osx_network_monitor_initable_iface_init (GInitableIface *iface)
{
  initable_parent_iface = g_type_interface_peek_parent (iface);

  iface->init = g_osx_network_monitor_initable_init;
}