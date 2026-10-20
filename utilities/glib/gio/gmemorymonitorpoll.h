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

#ifndef __G_MEMORY_MONITOR_POLL_H__
#define __G_MEMORY_MONITOR_POLL_H__

#include "gmemorymonitorbase.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define G_TYPE_MEMORY_MONITOR_POLL         (g_memory_monitor_poll_get_type ())
G_DECLARE_FINAL_TYPE (GMemoryMonitorPoll, g_memory_monitor_poll, G, MEMORY_MONITOR_POLL, GMemoryMonitorBase)

GIO_AVAILABLE_IN_2_86
GType g_memory_monitor_poll_get_type (void);

G_END_DECLS

#endif /* __G_MEMORY_MONITOR_PSI_H__ */
