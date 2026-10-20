/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2025 Red Hat, Inc.
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

#include "gcancellable.h"
#include "ginitable.h"
#include "gioerror.h"
#include "giomodule-priv.h"
#include "glib/gstdio.h"
#include "glibintl.h"
#include "gmemorymonitor.h"
#include "gmemorymonitorbase.h"

#ifdef HAVE_SYSINFO
#include <sys/sysinfo.h>
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

/**
 * GMemoryMonitorBase:
 *
 * An abstract base class for implementations of [iface@Gio.MemoryMonitor] which
 * provides several defined warning levels (`GLowMemoryLevel`) and tracks how
 * often they are notified to the user via [signal@Gio.MemoryMonitor::low-memory-warning]
 * to limit the number of signal emissions to one every 15 seconds for each level.
 * [method@Gio.MemoryMonitorBase.send_event_to_user] is provided for this purpose.
 */

/* The interval between sending a signal in second */
#define RECOVERY_INTERVAL_SEC 15

#define G_MEMORY_MONITOR_BASE_GET_INITABLE_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), G_TYPE_INITABLE, GInitable))

static void g_memory_monitor_base_iface_init (GMemoryMonitorInterface *iface);
static void g_memory_monitor_base_initable_iface_init (GInitableIface *iface);

typedef struct
{
  GObject parent_instance;

  guint64 last_trigger_us[G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_COUNT];
} GMemoryMonitorBasePrivate;


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GMemoryMonitorBase, g_memory_monitor_base, G_TYPE_OBJECT,
                                  G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                    g_memory_monitor_base_initable_iface_init)
                                  G_IMPLEMENT_INTERFACE (G_TYPE_MEMORY_MONITOR,
                                      g_memory_monitor_base_iface_init)
                                  G_ADD_PRIVATE (GMemoryMonitorBase))

gdouble
g_memory_monitor_base_query_mem_ratio (void)
{
#ifdef HAVE_SYSINFO
  struct sysinfo info;

  if (sysinfo (&info))
    return -1.0;

  if (info.totalram == 0)
    return -1.0;

  return (gdouble) ((gdouble) info.freeram / (gdouble) info.totalram);
#elif defined(_SC_PHYS_PAGES) && defined(_SC_AVPHYS_PAGES)
  /* Implementation for Solaris, where sysinfo() does not return RAM usage information */
  long totalram = sysconf (_SC_PHYS_PAGES);
  long freeram = sysconf (_SC_AVPHYS_PAGES);

  if (totalram <= 0 || freeram < 0)
    return -1.0;

  return (gdouble) ((gdouble) freeram / (gdouble) totalram);
#else
  return -1.0;
#endif
}

GMemoryMonitorWarningLevel
g_memory_monitor_base_level_enum_to_byte (GMemoryMonitorLowMemoryLevel level)
{
  const GMemoryMonitorWarningLevel level_bytes[G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_COUNT] = {
    [G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_LOW] = 50,
    [G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_MEDIUM] = 100,
    [G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_CRITICAL] = 255
  };

  if ((int) level < G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_INVALID ||
      (int) level >= G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_COUNT)
    g_assert_not_reached ();

  if (level == G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_INVALID)
    return 0;

  return level_bytes[level];
}

typedef struct
{
  GWeakRef monitor_weak;
  GMemoryMonitorWarningLevel level;
} SendEventData;

static void
send_event_data_free (SendEventData *data)
{
  g_weak_ref_clear (&data->monitor_weak);
  g_free (data);
}

/* Invoked in the global default main context */
static gboolean
send_event_cb (void *user_data)
{
  SendEventData *data = user_data;
  GMemoryMonitor *monitor = g_weak_ref_get (&data->monitor_weak);

  if (monitor != NULL)
    g_signal_emit_by_name (monitor, "low-memory-warning", data->level);

  g_clear_object (&monitor);

  return G_SOURCE_REMOVE;
}

void
g_memory_monitor_base_send_event_to_user (GMemoryMonitorBase              *monitor,
                                          GMemoryMonitorLowMemoryLevel     warning_level)
{
  gint64 current_time;
  GMemoryMonitorBasePrivate *priv = g_memory_monitor_base_get_instance_private (monitor);

  current_time = g_get_monotonic_time ();

  if (priv->last_trigger_us[warning_level] == 0 ||
      (current_time - priv->last_trigger_us[warning_level]) > (RECOVERY_INTERVAL_SEC * G_USEC_PER_SEC))
    {
      SendEventData *data = NULL;

      g_debug ("Send low memory signal with warning level %u", warning_level);

      /* The signal has to be emitted in the global default main context,
       * because the `GMemoryMonitor` is a singleton which may have been created
       * in an arbitrary thread, or which may be calling this function from the
       * GLib worker thread. */
      data = g_new0 (SendEventData, 1);
      g_weak_ref_init (&data->monitor_weak, monitor);
      data->level = g_memory_monitor_base_level_enum_to_byte (warning_level);
      g_main_context_invoke_full (NULL, G_PRIORITY_DEFAULT, send_event_cb,
                                  g_steal_pointer (&data), (GDestroyNotify) send_event_data_free);
      priv->last_trigger_us[warning_level] = current_time;
    }
}

static gboolean
g_memory_monitor_base_initable_init (GInitable     *initable,
                                        GCancellable  *cancellable,
                                        GError       **error)
{
  return TRUE;
}

static void
g_memory_monitor_base_init (GMemoryMonitorBase *monitor)
{
}

static void
g_memory_monitor_base_class_init (GMemoryMonitorBaseClass *klass)
{
}

static void
g_memory_monitor_base_iface_init (GMemoryMonitorInterface *monitor_iface)
{
}

static void
g_memory_monitor_base_initable_iface_init (GInitableIface *iface)
{
  iface->init = g_memory_monitor_base_initable_init;
}
