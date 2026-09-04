/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2026 Sorah Fukumori
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

#ifndef __G_NETWORK_MONITOR_SYSTEMD_H__
#define __G_NETWORK_MONITOR_SYSTEMD_H__

#include "gnetworkmonitornetlink.h"

G_BEGIN_DECLS

#define G_TYPE_NETWORK_MONITOR_SYSTEMD         (g_network_monitor_systemd_get_type ())
G_DECLARE_FINAL_TYPE (GNetworkMonitorSystemd, g_network_monitor_systemd, G, NETWORK_MONITOR_SYSTEMD, GNetworkMonitorNetlink)

G_END_DECLS

#endif /* __G_NETWORK_MONITOR_SYSTEMD_H__ */
