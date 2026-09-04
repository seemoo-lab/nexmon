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
#include "gdbusnamewatching.h"
#include "gdbusproxy.h"
#include "ginitable.h"
#include "gioerror.h"
#include "giomodule-priv.h"
#include "glib/gstdio.h"
#include "glib/glib-private.h"
#include "glibintl.h"
#include "gmemorymonitor.h"
#include "gmemorymonitorpoll.h"

#include <fcntl.h>
#include <unistd.h>

/**
 * GMemoryMonitorPoll:
 *
 * A [iface@Gio.MemoryMonitor] which polls the system free/used
 * memory ratio on a fixed timer.
 *
 * It polls, for example, every 10 seconds, and emits different
 * [signal@Gio.MemoryMonitor::low-memory-warning] signals if it falls below several
 * ‘low’ thresholds.
 *
 * The system free/used memory ratio is queried using [`sysinfo()`](man:sysinfo(2)).
 *
 * This is intended as a fallback implementation of [iface@Gio.MemoryMonitor] in case
 * other, more performant, implementations are not supported on the system.
 *
 * Since: 2.86
 */

typedef enum {
  PROP_MEM_FREE_RATIO = 1,
  PROP_POLL_INTERVAL_MS,
} GMemoryMonitorPollProperty;

#define G_MEMORY_MONITOR_POLL_GET_INITABLE_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), G_TYPE_INITABLE, GInitable))

static void g_memory_monitor_poll_iface_init (GMemoryMonitorInterface *iface);
static void g_memory_monitor_poll_initable_iface_init (GInitableIface *iface);

struct _GMemoryMonitorPoll
{
  GMemoryMonitorBase parent_instance;

  GMainContext *worker;  /* (unowned) */
  GSource *source_timeout;  /* (owned) */

  /* it overrides the default timeout when running the test */
  unsigned int poll_interval_ms;  /* zero to use the default */
  gdouble mem_free_ratio;
};

/* the default monitor timeout */
#define G_MEMORY_MONITOR_PSI_DEFAULT_SEC 10

G_DEFINE_TYPE_WITH_CODE (GMemoryMonitorPoll, g_memory_monitor_poll, G_TYPE_MEMORY_MONITOR_BASE,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE,
                                                g_memory_monitor_poll_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_MEMORY_MONITOR,
                                                g_memory_monitor_poll_iface_init)
                         _g_io_modules_ensure_extension_points_registered ();
                         g_io_extension_point_implement (G_MEMORY_MONITOR_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "poll",
                                                         10))

static void
g_memory_monitor_poll_init (GMemoryMonitorPoll *mem_poll)
{
  mem_poll->mem_free_ratio = -1.0;
}

static void
g_memory_monitor_poll_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GMemoryMonitorPoll *monitor = G_MEMORY_MONITOR_POLL (object);

  switch ((GMemoryMonitorPollProperty) prop_id)
    {
    case PROP_MEM_FREE_RATIO:
      monitor->mem_free_ratio = g_value_get_double (value);
      break;
    case PROP_POLL_INTERVAL_MS:
      monitor->poll_interval_ms = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_memory_monitor_poll_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GMemoryMonitorPoll *monitor = G_MEMORY_MONITOR_POLL (object);

  switch ((GMemoryMonitorPollProperty) prop_id)
    {
    case PROP_MEM_FREE_RATIO:
      g_value_set_double (value, monitor->mem_free_ratio);
      break;
    case PROP_POLL_INTERVAL_MS:
      g_value_set_uint (value, monitor->poll_interval_ms);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

typedef struct
{
  GWeakRef monitor_weak;  /* (element-type GMemoryMonitorPoll) */
} CallbackData;

static CallbackData *
callback_data_new (GMemoryMonitorPoll *monitor)
{
  CallbackData *data = g_new0 (CallbackData, 1);
  g_weak_ref_init (&data->monitor_weak, monitor);
  return g_steal_pointer (&data);
}

static void
callback_data_free (CallbackData *data)
{
  g_weak_ref_clear (&data->monitor_weak);
  g_free (data);
}

static gboolean
g_memory_monitor_mem_ratio_cb (gpointer user_data)
{
  CallbackData *data = user_data;
  GMemoryMonitorPoll *monitor = NULL;
  gdouble mem_ratio;
  GMemoryMonitorLowMemoryLevel warning_level = G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_INVALID;

  /* It’s possible for the dispatch of this callback to race with finalising
   * the `GMemoryMonitorPoll`, hence the use of a thread-safe weak ref. */
  monitor = g_weak_ref_get (&data->monitor_weak);
  if (monitor == NULL)
    return G_SOURCE_REMOVE;

  /* Should be executed in the worker context */
  g_assert (g_main_context_is_owner (monitor->worker));

  mem_ratio = g_memory_monitor_base_query_mem_ratio ();

  /* free ratio override */
  if (monitor->mem_free_ratio >= 0.0)
    mem_ratio = monitor->mem_free_ratio;

  if (mem_ratio < 0.0)
    {
      g_clear_object (&monitor);
      return G_SOURCE_REMOVE;
    }

  if (mem_ratio > 0.5)
    {
      g_clear_object (&monitor);
      return G_SOURCE_CONTINUE;
    }

  g_debug ("memory free ratio %f", mem_ratio);

  if (mem_ratio < 0.2)
    warning_level = G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_CRITICAL;
  else if (mem_ratio < 0.3)
    warning_level = G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_MEDIUM;
  else if (mem_ratio < 0.4)
    warning_level = G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_LOW;

  if (warning_level != G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_INVALID)
    g_memory_monitor_base_send_event_to_user (G_MEMORY_MONITOR_BASE (monitor), warning_level);

  g_clear_object (&monitor);

  return G_SOURCE_CONTINUE;
}

static gboolean
g_memory_monitor_poll_initable_init (GInitable     *initable,
                                     GCancellable  *cancellable,
                                     GError       **error)
{
  GMemoryMonitorPoll *monitor = G_MEMORY_MONITOR_POLL (initable);
  GSource *source;

  if (monitor->poll_interval_ms > 0)
    {
      if (monitor->poll_interval_ms < G_TIME_SPAN_MILLISECOND)
        source = g_timeout_source_new (monitor->poll_interval_ms);
      else
        source = g_timeout_source_new_seconds (monitor->poll_interval_ms / G_TIME_SPAN_MILLISECOND);
    }
  else
    {
      /* default 10 second */
      source = g_timeout_source_new_seconds (G_MEMORY_MONITOR_PSI_DEFAULT_SEC);
    }

  g_source_set_callback (source, g_memory_monitor_mem_ratio_cb,
                         callback_data_new (monitor), (GDestroyNotify) callback_data_free);
  monitor->worker = GLIB_PRIVATE_CALL (g_get_worker_context) ();
  g_source_attach (source, monitor->worker);
  monitor->source_timeout = g_steal_pointer (&source);

  return TRUE;
}

static void
g_memory_monitor_poll_finalize (GObject *object)
{
  GMemoryMonitorPoll *monitor = G_MEMORY_MONITOR_POLL (object);

  g_source_destroy (monitor->source_timeout);
  g_source_unref (monitor->source_timeout);

  G_OBJECT_CLASS (g_memory_monitor_poll_parent_class)->finalize (object);
}

static void
g_memory_monitor_poll_class_init (GMemoryMonitorPollClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = g_memory_monitor_poll_set_property;
  object_class->get_property = g_memory_monitor_poll_get_property;
  object_class->finalize = g_memory_monitor_poll_finalize;

  /**
   * GMemoryMonitorPoll:mem-free-ratio:
   *
   * Override the memory free ratio
   *
   * Since: 2.86
   */
  g_object_class_install_property (object_class,
                                   PROP_MEM_FREE_RATIO,
                                   g_param_spec_double ("mem-free-ratio", NULL, NULL,
                                                        -1.0, 1.0,
                                                        -1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * GMemoryMonitorPoll:poll-interval-ms:
   *
   * Override the poll interval for monitoring the memory usage.
   *
   * The interval is in milliseconds. Zero means to use the default interval.
   *
   * Since: 2.86
   */
  g_object_class_install_property (object_class,
                                   PROP_POLL_INTERVAL_MS,
                                   g_param_spec_uint ("poll-interval-ms", NULL, NULL,
                                                      0, G_MAXUINT,
                                                      0,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS));

}

static void
g_memory_monitor_poll_iface_init (GMemoryMonitorInterface *monitor_iface)
{
}

static void
g_memory_monitor_poll_initable_iface_init (GInitableIface *iface)
{
  iface->init = g_memory_monitor_poll_initable_init;
}

