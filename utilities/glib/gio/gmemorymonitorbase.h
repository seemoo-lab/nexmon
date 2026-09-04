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

#ifndef __G_MEMORY_MONITOR_BASE_H__
#define __G_MEMORY_MONITOR_BASE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define G_TYPE_MEMORY_MONITOR_BASE         (g_memory_monitor_base_get_type ())

G_DECLARE_DERIVABLE_TYPE (GMemoryMonitorBase, g_memory_monitor_base, G, MEMORY_MONITOR_BASE, GObject)

typedef enum
{
  G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_INVALID = -1,
  G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_LOW = 0,
  G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_MEDIUM,
  G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_CRITICAL,
  G_MEMORY_MONITOR_LOW_MEMORY_LEVEL_COUNT
} GMemoryMonitorLowMemoryLevel;

struct _GMemoryMonitorBaseClass
{
  GObjectClass parent_class;
};

void                        g_memory_monitor_base_send_event_to_user (GMemoryMonitorBase              *monitor,
                                                                      GMemoryMonitorLowMemoryLevel     warning_level);
GMemoryMonitorWarningLevel  g_memory_monitor_base_level_enum_to_byte (GMemoryMonitorLowMemoryLevel     level);
gdouble                     g_memory_monitor_base_query_mem_ratio    (void);

G_END_DECLS

#endif /* __G_MEMORY_MONITOR_BASE_H__ */
