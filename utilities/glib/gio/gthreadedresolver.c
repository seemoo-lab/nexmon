/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2018 Igalia S.L.
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
#include <glib.h>
#include "glibintl.h"

#include <stdio.h>
#include <string.h>

#include "glib/glib-private.h"
#include "gthreadedresolver.h"
#include "gthreadedresolver-private.h"
#include "gnetworkingprivate.h"

#include "gcancellable.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gnetworkmonitorbase.h"
#include "gtask.h"
#include "gsocketaddress.h"
#include "gsrvtarget.h"

#if HAVE_GETIFADDRS
#include <ifaddrs.h>
#endif

/*
 * GThreadedResolver is a threaded wrapper around the system libc’s
 * `getaddrinfo()`.
 *
 * It has to be threaded, as `getaddrinfo()` is synchronous. libc does provide
 * `getaddrinfo_a()` as an asynchronous version of `getaddrinfo()`, but it does
 * not integrate with a poll loop. It requires use of sigevent to notify of
 * completion of an asynchronous operation. That either emits a signal, or calls
 * a callback function in a newly spawned thread.
 *
 * A signal (`SIGEV_SIGNAL`) can’t be used for completion as (aside from being
 * another expensive round trip into the kernel) GLib cannot pick a `SIG*`
 * number which is guaranteed to not be in use elsewhere in the process. Various
 * other things could be interfering with signal dispositions, such as gdb or
 * other libraries in the process. Using a `signalfd()`
 * [cannot improve this situation](https://ldpreload.com/blog/signalfd-is-useless).
 *
 * A callback function in a newly spawned thread (`SIGEV_THREAD`) could be used,
 * but that is very expensive. Internally, glibc currently also just implements
 * `getaddrinfo_a()`
 * [using its own thread pool](https://github.com/bminor/glibc/blob/master/resolv/gai_misc.c),
 * and then
 * [spawns an additional thread for each completion callback](https://github.com/bminor/glibc/blob/master/resolv/gai_notify.c).
 * That is very expensive.
 *
 * No other appropriate sigevent callback types
 * [currently exist](https://sourceware.org/bugzilla/show_bug.cgi?id=30287), and
 * [others agree that sigevent is not great](http://davmac.org/davpage/linux/async-io.html#posixaio).
 *
 * Hence, #GThreadedResolver calls the normal synchronous `getaddrinfo()` in its
 * own thread pool. Previously, #GThreadedResolver used the thread pool which is
 * internal to #GTask by calling g_task_run_in_thread(). That lead to exhaustion
 * of the #GTask thread pool in some situations, though, as DNS lookups are
 * quite frequent leaf operations in some use cases. Now, #GThreadedResolver
 * uses its own private thread pool.
 *
 * This is similar to what
 * [libasyncns](http://git.0pointer.net/libasyncns.git/tree/libasyncns/asyncns.h)
 * and other multi-threaded users of `getaddrinfo()` do.
 */

struct _GThreadedResolver
{
  GResolver parent_instance;

  GThreadPool *thread_pool;  /* (owned) */

  GMutex interface_mutex;
  GNetworkMonitor *network_monitor; /* (owned) */
  gboolean monitor_supports_caching;
  int network_is_loopback_only;
};

G_DEFINE_TYPE (GThreadedResolver, g_threaded_resolver, G_TYPE_RESOLVER)

static void run_task_in_thread_pool_async (GThreadedResolver *self,
                                           GTask             *task);
static void run_task_in_thread_pool_sync (GThreadedResolver *self,
                                          GTask             *task);
static void threaded_resolver_worker_cb (gpointer task_data,
                                         gpointer user_data);

static void
g_threaded_resolver_init (GThreadedResolver *self)
{
  self->thread_pool = g_thread_pool_new_full (threaded_resolver_worker_cb,
                                              self,
                                              (GDestroyNotify) g_object_unref,
                                              20,
                                              FALSE,
                                              NULL);

  self->network_is_loopback_only = -1;

  g_mutex_init (&self->interface_mutex);
}

static void
g_threaded_resolver_finalize (GObject *object)
{
  GThreadedResolver *self = G_THREADED_RESOLVER (object);

  g_thread_pool_free (self->thread_pool, TRUE, FALSE);
  self->thread_pool = NULL;

  if (self->network_monitor)
    g_signal_handlers_disconnect_by_data (self->network_monitor, object);

  g_clear_object (&self->network_monitor);
  g_mutex_clear (&self->interface_mutex);

  G_OBJECT_CLASS (g_threaded_resolver_parent_class)->finalize (object);
}

static GResolverError
g_resolver_error_from_addrinfo_error (gint err)
{
  switch (err)
    {
    case EAI_FAIL:
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
    case EAI_NODATA:
#endif
    case EAI_NONAME:
      return G_RESOLVER_ERROR_NOT_FOUND;

    case EAI_AGAIN:
      return G_RESOLVER_ERROR_TEMPORARY_FAILURE;

    default:
      return G_RESOLVER_ERROR_INTERNAL;
    }
}

typedef struct {
  enum {
    LOOKUP_BY_NAME,
    LOOKUP_BY_ADDRESS,
    LOOKUP_RECORDS,
  } lookup_type;

  union {
    struct {
      char *hostname;
      int address_family;
    } lookup_by_name;
    struct {
      GInetAddress *address;  /* (owned) */
    } lookup_by_address;
    struct {
      char *rrname;
      GResolverRecordType record_type;
    } lookup_records;
  };

  GCond cond;  /* used for signalling completion of the task when running it sync */
  GMutex lock;

  GSource *timeout_source;  /* (nullable) (owned) */
  GSource *cancellable_source;  /* (nullable) (owned) */

  /* This enum indicates that a particular code path has claimed the
   * task and is shortly about to call g_task_return_*() on it.
   * This must be accessed with GThreadedResolver.lock held. */
  enum
    {
      NOT_YET,
      COMPLETED,  /* libc lookup call has completed successfully or errored */
      TIMED_OUT,
      CANCELLED,
    } will_return;

  /* Whether the thread pool thread executing this lookup has finished executing
   * it and g_task_return_*() has been called on it already.
   * This must be accessed with GThreadedResolver.lock held. */
  gboolean has_returned;
} LookupData;

static LookupData *
lookup_data_new_by_name (const char *hostname,
                         int         address_family)
{
  LookupData *data = g_new0 (LookupData, 1);
  data->lookup_type = LOOKUP_BY_NAME;
  g_cond_init (&data->cond);
  g_mutex_init (&data->lock);
  data->lookup_by_name.hostname = g_strdup (hostname);
  data->lookup_by_name.address_family = address_family;
  return g_steal_pointer (&data);
}

static LookupData *
lookup_data_new_by_address (GInetAddress *address)
{
  LookupData *data = g_new0 (LookupData, 1);
  data->lookup_type = LOOKUP_BY_ADDRESS;
  g_cond_init (&data->cond);
  g_mutex_init (&data->lock);
  data->lookup_by_address.address = g_object_ref (address);
  return g_steal_pointer (&data);
}

static LookupData *
lookup_data_new_records (const gchar         *rrname,
                         GResolverRecordType  record_type)
{
  LookupData *data = g_new0 (LookupData, 1);
  data->lookup_type = LOOKUP_RECORDS;
  g_cond_init (&data->cond);
  g_mutex_init (&data->lock);
  data->lookup_records.rrname = g_strdup (rrname);
  data->lookup_records.record_type = record_type;
  return g_steal_pointer (&data);
}

static void
lookup_data_free (LookupData *data)
{
  switch (data->lookup_type) {
  case LOOKUP_BY_NAME:
    g_free (data->lookup_by_name.hostname);
    break;
  case LOOKUP_BY_ADDRESS:
    g_clear_object (&data->lookup_by_address.address);
    break;
  case LOOKUP_RECORDS:
    g_free (data->lookup_records.rrname);
    break;
  default:
    g_assert_not_reached ();
  }

  if (data->timeout_source != NULL)
    {
      g_source_destroy (data->timeout_source);
      g_clear_pointer (&data->timeout_source, g_source_unref);
    }

  if (data->cancellable_source != NULL)
    {
      g_source_destroy (data->cancellable_source);
      g_clear_pointer (&data->cancellable_source, g_source_unref);
    }

  g_mutex_clear (&data->lock);
  g_cond_clear (&data->cond);

  g_free (data);
}

static gboolean
check_only_has_loopback_interfaces (void)
{
#if HAVE_GETIFADDRS
  struct ifaddrs *addrs;
  gboolean only_loopback = TRUE;

  if (getifaddrs (&addrs) != 0)
    {
      int saved_errno = errno;
      g_debug ("getifaddrs() failed: %s", g_strerror (saved_errno));
      return FALSE;
    }

  for (struct ifaddrs *addr = addrs; addr; addr = addr->ifa_next)
    {
      struct sockaddr *sa = addr->ifa_addr;
      size_t addrlen;
      GSocketAddress *saddr;
      if (!sa)
        continue;

      if (sa->sa_family == AF_INET)
        addrlen = sizeof (struct sockaddr_in);
      else if (sa->sa_family == AF_INET6)
        addrlen = sizeof (struct sockaddr_in6);
      else
        continue;

      saddr = g_socket_address_new_from_native (sa, addrlen);
      if (!saddr)
        continue;

      if (!G_IS_INET_SOCKET_ADDRESS (saddr))
        {
          g_object_unref (saddr);
          continue;
        }

      GInetAddress *inetaddr = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (saddr));
      if (!g_inet_address_get_is_loopback (inetaddr))
        {
          only_loopback = FALSE;
          g_object_unref (saddr);
          break;
        }

      g_object_unref (saddr);
    }

  freeifaddrs (addrs);
  return only_loopback;
#else /* FIXME: Check GetAdaptersAddresses() on win32. */
  return FALSE;
#endif
}

static void
network_changed_cb (GNetworkMonitor   *monitor,
                    gboolean           network_available,
                    GThreadedResolver *resolver)
{
  g_mutex_lock (&resolver->interface_mutex);
  resolver->network_is_loopback_only = -1;
  g_mutex_unlock (&resolver->interface_mutex);
}

static gboolean
only_has_loopback_interfaces_cached (GThreadedResolver *resolver)
{
  g_mutex_lock (&resolver->interface_mutex);

  if (!resolver->network_monitor)
    {
      resolver->network_monitor = g_object_ref (g_network_monitor_get_default ());
      resolver->monitor_supports_caching = G_TYPE_FROM_INSTANCE (resolver->network_monitor) != G_TYPE_NETWORK_MONITOR_BASE;
      g_signal_connect_object (resolver->network_monitor, "network-changed", G_CALLBACK (network_changed_cb), resolver, G_CONNECT_DEFAULT);
    }

  if (!resolver->monitor_supports_caching || resolver->network_is_loopback_only == -1)
    resolver->network_is_loopback_only = check_only_has_loopback_interfaces ();

  g_mutex_unlock (&resolver->interface_mutex);

  return resolver->network_is_loopback_only;
}

static GList *
do_lookup_by_name (GThreadedResolver  *resolver,
                   const gchar        *hostname,
                   int                 address_family,
                   GCancellable       *cancellable,
                   GError            **error)
{
  struct addrinfo *res = NULL;
  GList *addresses;
  gint retval;
  struct addrinfo addrinfo_hints = { 0 };

  /* In general we only want IPs for valid interfaces.
   * However this will return nothing if you only have loopback interfaces.
   * Instead in this case we will manually filter out invalid IPs. */
  gboolean only_loopback = only_has_loopback_interfaces_cached (resolver);

#ifdef AI_ADDRCONFIG
  if (!only_loopback)
    addrinfo_hints.ai_flags = AI_ADDRCONFIG;
#endif
  /* socktype and protocol don't actually matter, they just get copied into the
  * returned addrinfo structures (and then we ignore them). But if
  * we leave them unset, we'll get back duplicate answers.
  */
  addrinfo_hints.ai_socktype = SOCK_STREAM;
  addrinfo_hints.ai_protocol = IPPROTO_TCP;

  addrinfo_hints.ai_family = address_family;
  retval = getaddrinfo (hostname, NULL, &addrinfo_hints, &res);

  if (retval == 0)
    {
      addresses = NULL;
      for (struct addrinfo *ai = res; ai; ai = ai->ai_next)
        {
          GInetAddress *addr;
          GSocketAddress *sockaddr = g_socket_address_new_from_native (ai->ai_addr, ai->ai_addrlen);

          if (!sockaddr)
            continue;
          if (!G_IS_INET_SOCKET_ADDRESS (sockaddr))
            {
              g_clear_object (&sockaddr);
              continue;
            }

          addr = g_inet_socket_address_get_address ((GInetSocketAddress *) sockaddr);
          if (only_loopback && !g_inet_address_get_is_loopback (addr))
            {
              g_object_unref (sockaddr);
              continue;
            }

          addresses = g_list_prepend (addresses, g_object_ref (addr));
          g_object_unref (sockaddr);
        }

      g_clear_pointer (&res, freeaddrinfo);

      if (addresses != NULL)
        {
          addresses = g_list_reverse (addresses);
          return g_steal_pointer (&addresses);
        }
      else
        {
          /* All addresses failed to be converted to GSocketAddresses. */
          g_set_error (error,
                       G_RESOLVER_ERROR,
                       G_RESOLVER_ERROR_NOT_FOUND,
                       _("Error resolving “%s”: %s"),
                       hostname,
                       _("No valid addresses were found"));
          return NULL;
        }
    }
  else
    {
#ifdef G_OS_WIN32
      gchar *error_message = g_win32_error_message (WSAGetLastError ());
#else
      gchar *error_message = g_locale_to_utf8 (gai_strerror (retval), -1, NULL, NULL, NULL);
      if (error_message == NULL)
        error_message = g_strdup ("[Invalid UTF-8]");
#endif

      g_clear_pointer (&res, freeaddrinfo);

      g_set_error (error,
                   G_RESOLVER_ERROR,
                   g_resolver_error_from_addrinfo_error (retval),
                   _("Error resolving “%s”: %s"),
                   hostname, error_message);
      g_free (error_message);

      return NULL;
    }
}

static GList *
lookup_by_name (GResolver     *resolver,
                const gchar   *hostname,
                GCancellable  *cancellable,
                GError       **error)
{
  GThreadedResolver *self = G_THREADED_RESOLVER (resolver);
  GTask *task;
  GList *addresses;
  LookupData *data;

  data = lookup_data_new_by_name (hostname, AF_UNSPEC);
  task = g_task_new (resolver, cancellable, NULL, NULL);
  g_task_set_source_tag (task, lookup_by_name);
  g_task_set_name (task, "[gio] resolver lookup");
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) lookup_data_free);

  run_task_in_thread_pool_sync (self, task);

  addresses = g_task_propagate_pointer (task, error);
  g_object_unref (task);

  return addresses;
}

static int
flags_to_family (GResolverNameLookupFlags flags)
{
  int address_family = AF_UNSPEC;

  if (flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY)
    address_family = AF_INET;

  if (flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV6_ONLY)
    {
      address_family = AF_INET6;
      /* You can only filter by one family at a time */
      g_return_val_if_fail (!(flags & G_RESOLVER_NAME_LOOKUP_FLAGS_IPV4_ONLY), address_family);
    }

  return address_family;
}

static GList *
lookup_by_name_with_flags (GResolver                 *resolver,
                           const gchar               *hostname,
                           GResolverNameLookupFlags   flags,
                           GCancellable              *cancellable,
                           GError                   **error)
{
  GThreadedResolver *self = G_THREADED_RESOLVER (resolver);
  GTask *task;
  GList *addresses;
  LookupData *data;

  data = lookup_data_new_by_name (hostname, flags_to_family (flags));
  task = g_task_new (resolver, cancellable, NULL, NULL);
  g_task_set_source_tag (task, lookup_by_name_with_flags);
  g_task_set_name (task, "[gio] resolver lookup");
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) lookup_data_free);

  run_task_in_thread_pool_sync (self, task);

  addresses = g_task_propagate_pointer (task, error);
  g_object_unref (task);

  return addresses;
}

static void
lookup_by_name_with_flags_async (GResolver                *resolver,
                                 const gchar              *hostname,
                                 GResolverNameLookupFlags  flags,
                                 GCancellable             *cancellable,
                                 GAsyncReadyCallback       callback,
                                 gpointer                  user_data)
{
  GThreadedResolver *self = G_THREADED_RESOLVER (resolver);
  GTask *task;
  LookupData *data;

  data = lookup_data_new_by_name (hostname, flags_to_family (flags));
  task = g_task_new (resolver, cancellable, callback, user_data);

  g_debug ("%s: starting new lookup for %s with GTask %p, LookupData %p",
           G_STRFUNC, hostname, task, data);

  g_task_set_source_tag (task, lookup_by_name_with_flags_async);
  g_task_set_name (task, "[gio] resolver lookup");
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) lookup_data_free);

  run_task_in_thread_pool_async (self, task);

  g_object_unref (task);
}

static void
lookup_by_name_async (GResolver           *resolver,
                      const gchar         *hostname,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  lookup_by_name_with_flags_async (resolver,
                                   hostname,
                                   G_RESOLVER_NAME_LOOKUP_FLAGS_DEFAULT,
                                   cancellable,
                                   callback,
                                   user_data);
}

static GList *
lookup_by_name_finish (GResolver     *resolver,
                       GAsyncResult  *result,
                       GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, resolver), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

static GList *
lookup_by_name_with_flags_finish (GResolver     *resolver,
                                  GAsyncResult  *result,
                                  GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, resolver), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

static gchar *
do_lookup_by_address (GInetAddress  *address,
                      GCancellable  *cancellable,
                      GError       **error)
{
  struct sockaddr_storage sockaddr_address;
  gsize sockaddr_address_size;
  GSocketAddress *gsockaddr;
  gchar name[NI_MAXHOST];
  gint retval;

  gsockaddr = g_inet_socket_address_new (address, 0);
  g_socket_address_to_native (gsockaddr, (struct sockaddr *)&sockaddr_address,
                              sizeof (sockaddr_address), NULL);
  sockaddr_address_size = g_socket_address_get_native_size (gsockaddr);
  g_object_unref (gsockaddr);

  retval = getnameinfo ((struct sockaddr *) &sockaddr_address, sockaddr_address_size,
                        name, sizeof (name), NULL, 0, NI_NAMEREQD);
  if (retval == 0)
    return g_strdup (name);
  else
    {
      gchar *phys;

#ifdef G_OS_WIN32
      gchar *error_message = g_win32_error_message (WSAGetLastError ());
#else
      gchar *error_message = g_locale_to_utf8 (gai_strerror (retval), -1, NULL, NULL, NULL);
      if (error_message == NULL)
        error_message = g_strdup ("[Invalid UTF-8]");
#endif

      phys = g_inet_address_to_string (address);
      g_set_error (error,
                   G_RESOLVER_ERROR,
                   g_resolver_error_from_addrinfo_error (retval),
                   _("Error reverse-resolving “%s”: %s"),
                   phys ? phys : "(unknown)",
                   error_message);
      g_free (phys);
      g_free (error_message);

      return NULL;
    }
}

static gchar *
lookup_by_address (GResolver        *resolver,
                   GInetAddress     *address,
                   GCancellable     *cancellable,
                   GError          **error)
{
  GThreadedResolver *self = G_THREADED_RESOLVER (resolver);
  LookupData *data = NULL;
  GTask *task;
  gchar *name;

  data = lookup_data_new_by_address (address);
  task = g_task_new (resolver, cancellable, NULL, NULL);
  g_task_set_source_tag (task, lookup_by_address);
  g_task_set_name (task, "[gio] resolver lookup");
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) lookup_data_free);

  run_task_in_thread_pool_sync (self, task);

  name = g_task_propagate_pointer (task, error);
  g_object_unref (task);

  return name;
}

static void
lookup_by_address_async (GResolver           *resolver,
                         GInetAddress        *address,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
  GThreadedResolver *self = G_THREADED_RESOLVER (resolver);
  LookupData *data = NULL;
  GTask *task;

  data = lookup_data_new_by_address (address);
  task = g_task_new (resolver, cancellable, callback, user_data);
  g_task_set_source_tag (task, lookup_by_address_async);
  g_task_set_name (task, "[gio] resolver lookup");
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) lookup_data_free);

  run_task_in_thread_pool_async (self, task);

  g_object_unref (task);
}

static gchar *
lookup_by_address_finish (GResolver     *resolver,
                          GAsyncResult  *result,
                          GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, resolver), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}


#if defined(G_OS_UNIX)

#if defined __BIONIC__ && !defined BIND_4_COMPAT
/* Copy from bionic/libc/private/arpa_nameser_compat.h
 * and bionic/libc/private/arpa_nameser.h */
typedef struct {
	unsigned	id :16;		/* query identification number */
#if BYTE_ORDER == BIG_ENDIAN
			/* fields in third byte */
	unsigned	qr: 1;		/* response flag */
	unsigned	opcode: 4;	/* purpose of message */
	unsigned	aa: 1;		/* authoritative answer */
	unsigned	tc: 1;		/* truncated message */
	unsigned	rd: 1;		/* recursion desired */
			/* fields in fourth byte */
	unsigned	ra: 1;		/* recursion available */
	unsigned	unused :1;	/* unused bits (MBZ as of 4.9.3a3) */
	unsigned	ad: 1;		/* authentic data from named */
	unsigned	cd: 1;		/* checking disabled by resolver */
	unsigned	rcode :4;	/* response code */
#endif
#if BYTE_ORDER == LITTLE_ENDIAN || BYTE_ORDER == PDP_ENDIAN
			/* fields in third byte */
	unsigned	rd :1;		/* recursion desired */
	unsigned	tc :1;		/* truncated message */
	unsigned	aa :1;		/* authoritative answer */
	unsigned	opcode :4;	/* purpose of message */
	unsigned	qr :1;		/* response flag */
			/* fields in fourth byte */
	unsigned	rcode :4;	/* response code */
	unsigned	cd: 1;		/* checking disabled by resolver */
	unsigned	ad: 1;		/* authentic data from named */
	unsigned	unused :1;	/* unused bits (MBZ as of 4.9.3a3) */
	unsigned	ra :1;		/* recursion available */
#endif
			/* remaining bytes */
	unsigned	qdcount :16;	/* number of question entries */
	unsigned	ancount :16;	/* number of answer entries */
	unsigned	nscount :16;	/* number of authority entries */
	unsigned	arcount :16;	/* number of resource entries */
} HEADER;

#define NS_INT32SZ	4	/* #/bytes of data in a uint32_t */
#define NS_INT16SZ	2	/* #/bytes of data in a uint16_t */

#define NS_GET16(s, cp) do { \
	const u_char *t_cp = (const u_char *)(cp); \
	(s) = ((uint16_t)t_cp[0] << 8) \
	    | ((uint16_t)t_cp[1]) \
	    ; \
	(cp) += NS_INT16SZ; \
} while (/*CONSTCOND*/0)

#define NS_GET32(l, cp) do { \
	const u_char *t_cp = (const u_char *)(cp); \
	(l) = ((uint32_t)t_cp[0] << 24) \
	    | ((uint32_t)t_cp[1] << 16) \
	    | ((uint32_t)t_cp[2] << 8) \
	    | ((uint32_t)t_cp[3]) \
	    ; \
	(cp) += NS_INT32SZ; \
} while (/*CONSTCOND*/0)

#define	GETSHORT		NS_GET16
#define	GETLONG			NS_GET32

#define C_IN 1

/* From bionic/libc/private/resolv_private.h */
int dn_expand(const u_char *, const u_char *, const u_char *, char *, int);
#define dn_skipname __dn_skipname
int dn_skipname(const u_char *, const u_char *);

/* From bionic/libc/private/arpa_nameser_compat.h */
#define T_MX		ns_t_mx
#define T_TXT		ns_t_txt
#define T_SOA		ns_t_soa
#define T_NS		ns_t_ns

/* From bionic/libc/private/arpa_nameser.h */
typedef enum __ns_type {
	ns_t_invalid = 0,	/* Cookie. */
	ns_t_a = 1,		/* Host address. */
	ns_t_ns = 2,		/* Authoritative server. */
	ns_t_md = 3,		/* Mail destination. */
	ns_t_mf = 4,		/* Mail forwarder. */
	ns_t_cname = 5,		/* Canonical name. */
	ns_t_soa = 6,		/* Start of authority zone. */
	ns_t_mb = 7,		/* Mailbox domain name. */
	ns_t_mg = 8,		/* Mail group member. */
	ns_t_mr = 9,		/* Mail rename name. */
	ns_t_null = 10,		/* Null resource record. */
	ns_t_wks = 11,		/* Well known service. */
	ns_t_ptr = 12,		/* Domain name pointer. */
	ns_t_hinfo = 13,	/* Host information. */
	ns_t_minfo = 14,	/* Mailbox information. */
	ns_t_mx = 15,		/* Mail routing information. */
	ns_t_txt = 16,		/* Text strings. */
	ns_t_rp = 17,		/* Responsible person. */
	ns_t_afsdb = 18,	/* AFS cell database. */
	ns_t_x25 = 19,		/* X_25 calling address. */
	ns_t_isdn = 20,		/* ISDN calling address. */
	ns_t_rt = 21,		/* Router. */
	ns_t_nsap = 22,		/* NSAP address. */
	ns_t_nsap_ptr = 23,	/* Reverse NSAP lookup (deprecated). */
	ns_t_sig = 24,		/* Security signature. */
	ns_t_key = 25,		/* Security key. */
	ns_t_px = 26,		/* X.400 mail mapping. */
	ns_t_gpos = 27,		/* Geographical position (withdrawn). */
	ns_t_aaaa = 28,		/* Ip6 Address. */
	ns_t_loc = 29,		/* Location Information. */
	ns_t_nxt = 30,		/* Next domain (security). */
	ns_t_eid = 31,		/* Endpoint identifier. */
	ns_t_nimloc = 32,	/* Nimrod Locator. */
	ns_t_srv = 33,		/* Server Selection. */
	ns_t_atma = 34,		/* ATM Address */
	ns_t_naptr = 35,	/* Naming Authority PoinTeR */
	ns_t_kx = 36,		/* Key Exchange */
	ns_t_cert = 37,		/* Certification record */
	ns_t_a6 = 38,		/* IPv6 address (deprecates AAAA) */
	ns_t_dname = 39,	/* Non-terminal DNAME (for IPv6) */
	ns_t_sink = 40,		/* Kitchen sink (experimental) */
	ns_t_opt = 41,		/* EDNS0 option (meta-RR) */
	ns_t_apl = 42,		/* Address prefix list (RFC 3123) */
	ns_t_tkey = 249,	/* Transaction key */
	ns_t_tsig = 250,	/* Transaction signature. */
	ns_t_ixfr = 251,	/* Incremental zone transfer. */
	ns_t_axfr = 252,	/* Transfer zone of authority. */
	ns_t_mailb = 253,	/* Transfer mailbox records. */
	ns_t_maila = 254,	/* Transfer mail agent records. */
	ns_t_any = 255,		/* Wildcard match. */
	ns_t_zxfr = 256,	/* BIND-specific, nonstandard. */
	ns_t_max = 65536
} ns_type;

#endif /* __BIONIC__ */

/* Wrapper around dn_expand() which does associated length checks and returns
 * errors as #GError. */
static gboolean
expand_name (const gchar   *rrname,
             const guint8  *answer,
             const guint8  *end,
             const guint8 **p,
             gchar         *namebuf,
             gsize          namebuf_len,
             GError       **error)
{
  int expand_result;

  expand_result = dn_expand (answer, end, *p, namebuf, namebuf_len);
  if (expand_result < 0 || end - *p < expand_result)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                   /* Translators: the placeholder is a DNS record type, such as ‘MX’ or ‘SRV’ */
                   _("Error parsing DNS %s record: malformed DNS packet"), rrname);
      return FALSE;
    }

  *p += expand_result;

  return TRUE;
}

static GVariant *
parse_res_srv (const guint8  *answer,
               const guint8  *end,
               const guint8 **p,
               GError       **error)
{
  gchar namebuf[1024];
  guint16 priority, weight, port;

  if (end - *p < 6)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                   /* Translators: the placeholder is a DNS record type, such as ‘MX’ or ‘SRV’ */
                   _("Error parsing DNS %s record: malformed DNS packet"), "SRV");
      return NULL;
    }

  GETSHORT (priority, *p);
  GETSHORT (weight, *p);
  GETSHORT (port, *p);

  /* RFC 2782 says (on page 4) that “Unless and until permitted by future
   * standards action, name compression is not to be used for this field.”, so
   * technically we shouldn’t be expanding names here for SRV records.
   *
   * However, other DNS resolvers (such as systemd[1]) do, and it seems in
   * keeping with the principle of being liberal in what you accept and strict
   * in what you emit. It also seems harmless.
   *
   * An earlier version of the RFC, RFC 2052 (now obsolete) specified that name
   * compression *was* to be used for SRV targets[2].
   *
   * See discussion on https://gitlab.gnome.org/GNOME/glib/-/issues/2622.
   *
   * [1]: https://github.com/yuwata/systemd/blob/2d23cc3c07c49722ce93170737b3efd2692a2d08/src/resolve/resolved-dns-packet.c#L1674
   * [2]: https://datatracker.ietf.org/doc/html/rfc2052#page-3
   */
  if (!expand_name ("SRV", answer, end, p, namebuf, sizeof (namebuf), error))
    return NULL;

  return g_variant_new ("(qqqs)",
                        priority,
                        weight,
                        port,
                        namebuf);
}

static GVariant *
parse_res_soa (const guint8  *answer,
               const guint8  *end,
               const guint8 **p,
               GError       **error)
{
  gchar mnamebuf[1024];
  gchar rnamebuf[1024];
  guint32 serial, refresh, retry, expire, ttl;

  if (!expand_name ("SOA", answer, end, p, mnamebuf, sizeof (mnamebuf), error))
    return NULL;

  if (!expand_name ("SOA", answer, end, p, rnamebuf, sizeof (rnamebuf), error))
    return NULL;

  if (end - *p < 20)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                   /* Translators: the placeholder is a DNS record type, such as ‘MX’ or ‘SRV’ */
                   _("Error parsing DNS %s record: malformed DNS packet"), "SOA");
      return NULL;
    }

  GETLONG (serial, *p);
  GETLONG (refresh, *p);
  GETLONG (retry, *p);
  GETLONG (expire, *p);
  GETLONG (ttl, *p);

  return g_variant_new ("(ssuuuuu)",
                        mnamebuf,
                        rnamebuf,
                        serial,
                        refresh,
                        retry,
                        expire,
                        ttl);
}

static GVariant *
parse_res_ns (const guint8  *answer,
              const guint8  *end,
              const guint8 **p,
              GError       **error)
{
  gchar namebuf[1024];

  if (!expand_name ("NS", answer, end, p, namebuf, sizeof (namebuf), error))
    return NULL;

  return g_variant_new ("(s)", namebuf);
}

static GVariant *
parse_res_mx (const guint8  *answer,
              const guint8  *end,
              const guint8 **p,
              GError       **error)
{
  gchar namebuf[1024];
  guint16 preference;

  if (end - *p < 2)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                   /* Translators: the placeholder is a DNS record type, such as ‘MX’ or ‘SRV’ */
                   _("Error parsing DNS %s record: malformed DNS packet"), "MX");
      return NULL;
    }

  GETSHORT (preference, *p);

  if (!expand_name ("MX", answer, end, p, namebuf, sizeof (namebuf), error))
    return NULL;

  return g_variant_new ("(qs)",
                        preference,
                        namebuf);
}

static GVariant *
parse_res_txt (const guint8  *answer,
               const guint8  *end,
               const guint8 **p,
               GError       **error)
{
  GVariant *record;
  GPtrArray *array;
  const guint8 *at = *p;
  gsize len;

  if (end - *p == 0)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                   /* Translators: the placeholder is a DNS record type, such as ‘MX’ or ‘SRV’ */
                   _("Error parsing DNS %s record: malformed DNS packet"), "TXT");
      return NULL;
    }

  array = g_ptr_array_new_with_free_func (g_free);
  while (at < end)
    {
      len = *(at++);
      if (len > (gsize) (end - at))
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                       /* Translators: the placeholder is a DNS record type, such as ‘MX’ or ‘SRV’ */
                       _("Error parsing DNS %s record: malformed DNS packet"), "TXT");
          g_ptr_array_free (array, TRUE);
          return NULL;
        }

      g_ptr_array_add (array, g_strndup ((gchar *)at, len));
      at += len;
    }

  *p = at;
  record = g_variant_new ("(@as)",
                          g_variant_new_strv ((const gchar **)array->pdata, array->len));
  g_ptr_array_free (array, TRUE);
  return record;
}

gint
g_resolver_record_type_to_rrtype (GResolverRecordType type)
{
  switch (type)
  {
    case G_RESOLVER_RECORD_SRV:
      return T_SRV;
    case G_RESOLVER_RECORD_TXT:
      return T_TXT;
    case G_RESOLVER_RECORD_SOA:
      return T_SOA;
    case G_RESOLVER_RECORD_NS:
      return T_NS;
    case G_RESOLVER_RECORD_MX:
      return T_MX;
  }
  g_return_val_if_reached (-1);
}

GList *
g_resolver_records_from_res_query (const gchar      *rrname,
                                   gint              rrtype,
                                   const guint8     *answer,
                                   gssize            len,
                                   gint              herr,
                                   GError          **error)
{
  uint16_t count;
  gchar namebuf[1024];
  const guint8 *end, *p;
  guint16 type, qclass, rdlength;
  const HEADER *header;
  GList *records;
  GVariant *record;
  gsize len_unsigned;
  GError *parsing_error = NULL;

  if (len <= 0)
    {
      if (len == 0 || herr == HOST_NOT_FOUND || herr == NO_DATA)
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                       _("No DNS record of the requested type for “%s”"), rrname);
        }
      else if (herr == TRY_AGAIN)
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_TEMPORARY_FAILURE,
                       _("Temporarily unable to resolve “%s”"), rrname);
        }
      else
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                       _("Error resolving “%s”"), rrname);
        }

      return NULL;
    }

  /* We know len ≥ 0 now. */
  len_unsigned = (gsize) len;

  if (len_unsigned < sizeof (HEADER))
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                   /* Translators: the first placeholder is a domain name, the
                    * second is an error message */
                   _("Error resolving “%s”: %s"), rrname, _("Malformed DNS packet"));
      return NULL;
    }

  records = NULL;

  header = (HEADER *)answer;
  p = answer + sizeof (HEADER);
  end = answer + len_unsigned;

  /* Skip query */
  count = ntohs (header->qdcount);
  while (count-- && p < end)
    {
      int expand_result;

      expand_result = dn_expand (answer, end, p, namebuf, sizeof (namebuf));
      if (expand_result < 0 || end - p < expand_result + 4)
        {
          /* Not possible to recover parsing as the length of the rest of the
           * record is unknown or is too short. */
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                       /* Translators: the first placeholder is a domain name, the
                        * second is an error message */
                       _("Error resolving “%s”: %s"), rrname, _("Malformed DNS packet"));
          return NULL;
        }

      p += expand_result;
      p += 4;  /* skip TYPE and CLASS */

      /* To silence gcc warnings */
      namebuf[0] = namebuf[1];
    }

  /* Read answers */
  count = ntohs (header->ancount);
  while (count-- && p < end)
    {
      int expand_result;

      expand_result = dn_expand (answer, end, p, namebuf, sizeof (namebuf));
      if (expand_result < 0 || end - p < expand_result + 10)
        {
          /* Not possible to recover parsing as the length of the rest of the
           * record is unknown or is too short. */
          g_set_error (&parsing_error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                       /* Translators: the first placeholder is a domain name, the
                        * second is an error message */
                       _("Error resolving “%s”: %s"), rrname, _("Malformed DNS packet"));
          break;
        }

      p += expand_result;
      GETSHORT (type, p);
      GETSHORT (qclass, p);
      p += 4; /* ignore the ttl (type=long) value */
      GETSHORT (rdlength, p);

      if (end - p < rdlength)
        {
          g_set_error (&parsing_error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                       /* Translators: the first placeholder is a domain name, the
                        * second is an error message */
                       _("Error resolving “%s”: %s"), rrname, _("Malformed DNS packet"));
          break;
        }

      if (type != rrtype || qclass != C_IN)
        {
          p += rdlength;
          continue;
        }

      switch (rrtype)
        {
        case T_SRV:
          record = parse_res_srv (answer, p + rdlength, &p, &parsing_error);
          break;
        case T_MX:
          record = parse_res_mx (answer, p + rdlength, &p, &parsing_error);
          break;
        case T_SOA:
          record = parse_res_soa (answer, p + rdlength, &p, &parsing_error);
          break;
        case T_NS:
          record = parse_res_ns (answer, p + rdlength, &p, &parsing_error);
          break;
        case T_TXT:
          record = parse_res_txt (answer, p + rdlength, &p, &parsing_error);
          break;
        default:
          g_debug ("Unrecognized DNS record type %u", rrtype);
          record = NULL;
          break;
        }

      if (record != NULL)
        records = g_list_prepend (records, g_variant_ref_sink (record));

      if (parsing_error != NULL)
        break;
    }

  if (parsing_error != NULL)
    {
      g_propagate_prefixed_error (error, parsing_error, _("Failed to parse DNS response for “%s”: "), rrname);
      g_list_free_full (records, (GDestroyNotify)g_variant_unref);
      return NULL;
    }
  else if (records == NULL)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                   _("No DNS record of the requested type for “%s”"), rrname);

      return NULL;
    }
  else
    return records;
}

#elif defined(G_OS_WIN32)

static GVariant *
parse_dns_srv (DNS_RECORDA *rec)
{
  return g_variant_new ("(qqqs)",
                        (guint16)rec->Data.SRV.wPriority,
                        (guint16)rec->Data.SRV.wWeight,
                        (guint16)rec->Data.SRV.wPort,
                        rec->Data.SRV.pNameTarget);
}

static GVariant *
parse_dns_soa (DNS_RECORDA *rec)
{
  return g_variant_new ("(ssuuuuu)",
                        rec->Data.SOA.pNamePrimaryServer,
                        rec->Data.SOA.pNameAdministrator,
                        (guint32)rec->Data.SOA.dwSerialNo,
                        (guint32)rec->Data.SOA.dwRefresh,
                        (guint32)rec->Data.SOA.dwRetry,
                        (guint32)rec->Data.SOA.dwExpire,
                        (guint32)rec->Data.SOA.dwDefaultTtl);
}

static GVariant *
parse_dns_ns (DNS_RECORDA *rec)
{
  return g_variant_new ("(s)", rec->Data.NS.pNameHost);
}

static GVariant *
parse_dns_mx (DNS_RECORDA *rec)
{
  return g_variant_new ("(qs)",
                        (guint16)rec->Data.MX.wPreference,
                        rec->Data.MX.pNameExchange);
}

static GVariant *
parse_dns_txt (DNS_RECORDA *rec)
{
  GVariant *record;
  GPtrArray *array;
  DWORD i;

  array = g_ptr_array_new ();
  for (i = 0; i < rec->Data.TXT.dwStringCount; i++)
    g_ptr_array_add (array, rec->Data.TXT.pStringArray[i]);
  record = g_variant_new ("(@as)",
                          g_variant_new_strv ((const gchar **)array->pdata, array->len));
  g_ptr_array_free (array, TRUE);
  return record;
}

static WORD
g_resolver_record_type_to_dnstype (GResolverRecordType type)
{
  switch (type)
  {
    case G_RESOLVER_RECORD_SRV:
      return DNS_TYPE_SRV;
    case G_RESOLVER_RECORD_TXT:
      return DNS_TYPE_TEXT;
    case G_RESOLVER_RECORD_SOA:
      return DNS_TYPE_SOA;
    case G_RESOLVER_RECORD_NS:
      return DNS_TYPE_NS;
    case G_RESOLVER_RECORD_MX:
      return DNS_TYPE_MX;
  }
  g_return_val_if_reached (-1);
}

static GList *
g_resolver_records_from_DnsQuery (const gchar  *rrname,
                                  WORD          dnstype,
                                  DNS_STATUS    status,
                                  DNS_RECORDA  *results,
                                  GError      **error)
{
  DNS_RECORDA *rec;
  gpointer record;
  GList *records;

  if (status != ERROR_SUCCESS)
    {
      if (status == DNS_ERROR_RCODE_NAME_ERROR)
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                       _("No DNS record of the requested type for “%s”"), rrname);
        }
      else if (status == DNS_ERROR_RCODE_SERVER_FAILURE)
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_TEMPORARY_FAILURE,
                       _("Temporarily unable to resolve “%s”"), rrname);
        }
      else
        {
          g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                       _("Error resolving “%s”"), rrname);
        }

      return NULL;
    }

  records = NULL;
  for (rec = results; rec; rec = rec->pNext)
    {
      if (rec->wType != dnstype)
        continue;
      switch (dnstype)
        {
        case DNS_TYPE_SRV:
          record = parse_dns_srv (rec);
          break;
        case DNS_TYPE_SOA:
          record = parse_dns_soa (rec);
          break;
        case DNS_TYPE_NS:
          record = parse_dns_ns (rec);
          break;
        case DNS_TYPE_MX:
          record = parse_dns_mx (rec);
          break;
        case DNS_TYPE_TEXT:
          record = parse_dns_txt (rec);
          break;
        default:
          g_warn_if_reached ();
          record = NULL;
          break;
        }
      if (record != NULL)
        records = g_list_prepend (records, g_variant_ref_sink (record));
    }

  if (records == NULL)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND,
                   _("No DNS record of the requested type for “%s”"), rrname);

      return NULL;
    }
  else
    return records;
}

#endif

static void
free_records (GList *records)
{
  g_list_free_full (records, (GDestroyNotify) g_variant_unref);
}

#if defined(G_OS_UNIX)
#ifdef __BIONIC__
#ifndef C_IN
#define C_IN 1
#endif
int res_query(const char *, int, int, u_char *, int);
#endif
#endif

static GList *
do_lookup_records (const gchar          *rrname,
                   GResolverRecordType   record_type,
                   GCancellable         *cancellable,
                   GError              **error)
{
  GList *records;

#if defined(G_OS_UNIX)
  gint len = 512;
  gint herr;
  GByteArray *answer;
  gint rrtype;

#ifdef HAVE_RES_NQUERY
  /* Load the resolver state. This is done once per worker thread, and the
   * #GResolver::reload signal is ignored (since we always reload). This could
   * be improved by having an explicit worker thread pool, with each thread
   * containing some state which is initialised at thread creation time and
   * updated in response to #GResolver::reload.
   *
   * What we have currently is not particularly worse than using res_query() in
   * worker threads, since it would transparently call res_init() for each new
   * worker thread. (Although the workers would get reused by the
   * #GThreadPool.)
   *
   * FreeBSD requires the state to be zero-filled before calling res_ninit(). */
  struct __res_state res = { 0, };
  if (res_ninit (&res) != 0)
    {
      g_set_error (error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL,
                   _("Error resolving “%s”"), rrname);
      return NULL;
    }
#endif

  rrtype = g_resolver_record_type_to_rrtype (record_type);
  answer = g_byte_array_new ();
  for (;;)
    {
      g_byte_array_set_size (answer, len * 2);
#if defined(HAVE_RES_NQUERY)
      len = res_nquery (&res, rrname, C_IN, rrtype, answer->data, answer->len);
#else
      len = res_query (rrname, C_IN, rrtype, answer->data, answer->len);
#endif

      /* If answer fit in the buffer then we're done */
      if (len < 0 || (guint) len < answer->len)
        break;

      /*
       * On overflow some res_query's return the length needed, others
       * return the full length entered. This code works in either case.
       */
    }

  herr = h_errno;
  records = g_resolver_records_from_res_query (rrname, rrtype, answer->data, len, herr, error);
  g_byte_array_free (answer, TRUE);

#ifdef HAVE_RES_NQUERY

#if defined(HAVE_RES_NDESTROY)
  res_ndestroy (&res);
#elif defined(HAVE_RES_NCLOSE)
  res_nclose (&res);
#elif defined(HAVE_RES_NINIT)
#error "Your platform has res_ninit() but not res_nclose() or res_ndestroy(). Please file a bug at https://gitlab.gnome.org/GNOME/glib/issues/new"
#endif

#endif  /* HAVE_RES_NQUERY */

#else

  DNS_STATUS status;
  DNS_RECORDA *results = NULL;
  WORD dnstype;

  /* Work around differences in Windows SDK and mingw-w64 headers */
#ifdef _MSC_VER
  typedef DNS_RECORDW * PDNS_RECORD_UTF8_;
#else
  typedef DNS_RECORDA * PDNS_RECORD_UTF8_;
#endif

  dnstype = g_resolver_record_type_to_dnstype (record_type);
  status = DnsQuery_UTF8 (rrname, dnstype, DNS_QUERY_STANDARD, NULL, (PDNS_RECORD_UTF8_*)&results, NULL);
  records = g_resolver_records_from_DnsQuery (rrname, dnstype, status, results, error);
  if (results != NULL)
    DnsRecordListFree (results, DnsFreeRecordList);

#endif

  return g_steal_pointer (&records);
}

static GList *
lookup_records (GResolver              *resolver,
                const gchar            *rrname,
                GResolverRecordType     record_type,
                GCancellable           *cancellable,
                GError                **error)
{
  GThreadedResolver *self = G_THREADED_RESOLVER (resolver);
  GTask *task;
  GList *records;
  LookupData *data = NULL;

  task = g_task_new (resolver, cancellable, NULL, NULL);
  g_task_set_source_tag (task, lookup_records);
  g_task_set_name (task, "[gio] resolver lookup records");

  data = lookup_data_new_records (rrname, record_type);
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) lookup_data_free);

  run_task_in_thread_pool_sync (self, task);

  records = g_task_propagate_pointer (task, error);
  g_object_unref (task);

  return records;
}

static void
lookup_records_async (GResolver           *resolver,
                      const char          *rrname,
                      GResolverRecordType  record_type,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  GThreadedResolver *self = G_THREADED_RESOLVER (resolver);
  GTask *task;
  LookupData *data = NULL;

  task = g_task_new (resolver, cancellable, callback, user_data);
  g_task_set_source_tag (task, lookup_records_async);
  g_task_set_name (task, "[gio] resolver lookup records");

  data = lookup_data_new_records (rrname, record_type);
  g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) lookup_data_free);

  run_task_in_thread_pool_async (self, task);

  g_object_unref (task);
}

static GList *
lookup_records_finish (GResolver     *resolver,
                       GAsyncResult  *result,
                       GError       **error)
{
  g_return_val_if_fail (g_task_is_valid (result, resolver), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/* Will be called in the GLib worker thread, so must lock all accesses to shared
 * data. */
static gboolean
timeout_cb (gpointer user_data)
{
  GWeakRef *weak_task = user_data;
  GTask *task = NULL;  /* (owned) */
  LookupData *data;
  gboolean should_return;

  task = g_weak_ref_get (weak_task);
  if (task == NULL)
    return G_SOURCE_REMOVE;

  data = g_task_get_task_data (task);

  g_mutex_lock (&data->lock);

  should_return = g_atomic_int_compare_and_exchange (&data->will_return, NOT_YET, TIMED_OUT);
  g_clear_pointer (&data->timeout_source, g_source_unref);

  g_mutex_unlock (&data->lock);

  if (should_return)
    {
      g_task_return_new_error_literal (task, G_IO_ERROR, G_IO_ERROR_TIMED_OUT,
                                       _("Socket I/O timed out"));
    }

  /* Signal completion of the task. */
  g_mutex_lock (&data->lock);
  data->has_returned = TRUE;
  g_cond_broadcast (&data->cond);
  g_mutex_unlock (&data->lock);

  g_object_unref (task);

  return G_SOURCE_REMOVE;
}

/* Will be called in the GLib worker thread, so must lock all accesses to shared
 * data. */
static gboolean
cancelled_cb (GCancellable *cancellable,
              gpointer      user_data)
{
  GWeakRef *weak_task = user_data;
  GTask *task = NULL;  /* (owned) */
  LookupData *data;
  gboolean should_return;

  task = g_weak_ref_get (weak_task);
  if (task == NULL)
    return G_SOURCE_REMOVE;

  data = g_task_get_task_data (task);

  g_mutex_lock (&data->lock);

  g_assert (g_cancellable_is_cancelled (cancellable));
  should_return = g_atomic_int_compare_and_exchange (&data->will_return, NOT_YET, CANCELLED);
  g_clear_pointer (&data->cancellable_source, g_source_unref);

  g_mutex_unlock (&data->lock);

  if (should_return)
    g_task_return_error_if_cancelled (task);

  /* Signal completion of the task. */
  g_mutex_lock (&data->lock);
  data->has_returned = TRUE;
  g_cond_broadcast (&data->cond);
  g_mutex_unlock (&data->lock);

  g_object_unref (task);

  return G_SOURCE_REMOVE;
}

static void
weak_ref_clear_and_free (GWeakRef *weak_ref)
{
  g_weak_ref_clear (weak_ref);
  g_free (weak_ref);
}

static void
run_task_in_thread_pool_async (GThreadedResolver *self,
                               GTask             *task)
{
  LookupData *data = g_task_get_task_data (task);
  guint timeout_ms = g_resolver_get_timeout (G_RESOLVER (self));
  GCancellable *cancellable = g_task_get_cancellable (task);

  g_mutex_lock (&data->lock);

  g_thread_pool_push (self->thread_pool, g_object_ref (task), NULL);

  if (timeout_ms != 0)
    {
      GWeakRef *weak_task = g_new0 (GWeakRef, 1);
      g_weak_ref_set (weak_task, task);

      data->timeout_source = g_timeout_source_new (timeout_ms);
      g_source_set_static_name (data->timeout_source, "[gio] threaded resolver timeout");
      g_source_set_callback (data->timeout_source, G_SOURCE_FUNC (timeout_cb), g_steal_pointer (&weak_task), (GDestroyNotify) weak_ref_clear_and_free);
      g_source_attach (data->timeout_source, GLIB_PRIVATE_CALL (g_get_worker_context) ());
    }

  if (cancellable != NULL)
    {
      GWeakRef *weak_task = g_new0 (GWeakRef, 1);
      g_weak_ref_set (weak_task, task);

      data->cancellable_source = g_cancellable_source_new (cancellable);
      g_source_set_static_name (data->cancellable_source, "[gio] threaded resolver cancellable");
      g_source_set_callback (data->cancellable_source, G_SOURCE_FUNC (cancelled_cb), g_steal_pointer (&weak_task), (GDestroyNotify) weak_ref_clear_and_free);
      g_source_attach (data->cancellable_source, GLIB_PRIVATE_CALL (g_get_worker_context) ());
    }

  g_mutex_unlock (&data->lock);
}

static void
run_task_in_thread_pool_sync (GThreadedResolver *self,
                              GTask             *task)
{
  LookupData *data = g_task_get_task_data (task);

  run_task_in_thread_pool_async (self, task);

  g_mutex_lock (&data->lock);
  while (!data->has_returned)
    g_cond_wait (&data->cond, &data->lock);
  g_mutex_unlock (&data->lock);
}

static void
threaded_resolver_worker_cb (gpointer task_data,
                             gpointer user_data)
{
  GTask *task = G_TASK (g_steal_pointer (&task_data));
  LookupData *data = g_task_get_task_data (task);
  GCancellable *cancellable = g_task_get_cancellable (task);
  GThreadedResolver *resolver = G_THREADED_RESOLVER (user_data);
  GError *local_error = NULL;
  gboolean should_return;

  switch (data->lookup_type) {
  case LOOKUP_BY_NAME:
    {
      GList *addresses = do_lookup_by_name (resolver,
                                            data->lookup_by_name.hostname,
                                            data->lookup_by_name.address_family,
                                            cancellable,
                                            &local_error);

      g_mutex_lock (&data->lock);
      should_return = g_atomic_int_compare_and_exchange (&data->will_return, NOT_YET, COMPLETED);
      g_mutex_unlock (&data->lock);

      if (should_return)
        {
          if (addresses != NULL)
            g_task_return_pointer (task, g_steal_pointer (&addresses), (GDestroyNotify) g_resolver_free_addresses);
          else
            g_task_return_error (task, g_steal_pointer (&local_error));
        }

      g_clear_pointer (&addresses, g_resolver_free_addresses);
      g_clear_error (&local_error);
    }
    break;
  case LOOKUP_BY_ADDRESS:
    {
      gchar *name = do_lookup_by_address (data->lookup_by_address.address,
                                          cancellable,
                                          &local_error);

      g_mutex_lock (&data->lock);
      should_return = g_atomic_int_compare_and_exchange (&data->will_return, NOT_YET, COMPLETED);
      g_mutex_unlock (&data->lock);

      if (should_return)
        {
          if (name != NULL)
            g_task_return_pointer (task, g_steal_pointer (&name), g_free);
          else
            g_task_return_error (task, g_steal_pointer (&local_error));
        }

      g_clear_pointer (&name, g_free);
      g_clear_error (&local_error);
    }
    break;
  case LOOKUP_RECORDS:
    {
      GList *records = do_lookup_records (data->lookup_records.rrname,
                                          data->lookup_records.record_type,
                                          cancellable,
                                          &local_error);

      g_mutex_lock (&data->lock);
      should_return = g_atomic_int_compare_and_exchange (&data->will_return, NOT_YET, COMPLETED);
      g_mutex_unlock (&data->lock);

      if (should_return)
        {
          if (records != NULL)
            g_task_return_pointer (task, g_steal_pointer (&records), (GDestroyNotify) free_records);
          else
            g_task_return_error (task, g_steal_pointer (&local_error));
        }

      g_clear_pointer (&records, free_records);
      g_clear_error (&local_error);
    }
    break;
  default:
    g_assert_not_reached ();
  }

  /* Signal completion of a task. */
  g_mutex_lock (&data->lock);
  data->has_returned = TRUE;
  g_cond_broadcast (&data->cond);
  g_mutex_unlock (&data->lock);

  g_object_unref (task);
}

static void
g_threaded_resolver_class_init (GThreadedResolverClass *threaded_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (threaded_class);
  GResolverClass *resolver_class = G_RESOLVER_CLASS (threaded_class);

  object_class->finalize = g_threaded_resolver_finalize;

  resolver_class->lookup_by_name                   = lookup_by_name;
  resolver_class->lookup_by_name_async             = lookup_by_name_async;
  resolver_class->lookup_by_name_finish            = lookup_by_name_finish;
  resolver_class->lookup_by_name_with_flags        = lookup_by_name_with_flags;
  resolver_class->lookup_by_name_with_flags_async  = lookup_by_name_with_flags_async;
  resolver_class->lookup_by_name_with_flags_finish = lookup_by_name_with_flags_finish;
  resolver_class->lookup_by_address                = lookup_by_address;
  resolver_class->lookup_by_address_async          = lookup_by_address_async;
  resolver_class->lookup_by_address_finish         = lookup_by_address_finish;
  resolver_class->lookup_records                   = lookup_records;
  resolver_class->lookup_records_async             = lookup_records_async;
  resolver_class->lookup_records_finish            = lookup_records_finish;
}
