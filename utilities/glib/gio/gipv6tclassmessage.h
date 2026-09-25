/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2025 Collabora Ltd.
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
 *
 * Authors: Jakub Adam <jakub.adam@collabora.com>
 */

#ifndef __G_IPV6_TCLASS_MESSAGE_H__
#define __G_IPV6_TCLASS_MESSAGE_H__

#include <gio/gsocketcontrolmessage.h>

G_BEGIN_DECLS

#define G_TYPE_IPV6_TCLASS_MESSAGE (g_ipv6_tclass_message_get_type ())

GIO_AVAILABLE_IN_2_88
G_DECLARE_FINAL_TYPE (GIPv6TclassMessage, g_ipv6_tclass_message, G, IPV6_TCLASS_MESSAGE, GSocketControlMessage)

GIO_AVAILABLE_IN_2_88
GSocketControlMessage *g_ipv6_tclass_message_new (guint8 dscp, GEcnCodePoint ecn);

GIO_AVAILABLE_IN_2_88
guint8 g_ipv6_tclass_message_get_dscp (GIPv6TclassMessage *message);

GIO_AVAILABLE_IN_2_88
GEcnCodePoint g_ipv6_tclass_message_get_ecn (GIPv6TclassMessage *message);

G_END_DECLS

#endif /* __G_IPV6_TCLASS_MESSAGE_H__ */
