/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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

#ifndef __G_NETWORKINGPRIVATE_H__
#define __G_NETWORKINGPRIVATE_H__

#include "gnetworking.h"
#include "gresolver.h"

G_BEGIN_DECLS

guint64  g_resolver_get_serial             (GResolver        *resolver);

gint g_socket (gint     domain,
               gint     type,
               gint     protocol,
               GError **error);

gboolean g_getservbyname_ntohs (const char *name, const char *proto, guint16 *out_port);

G_END_DECLS

#endif /* __G_NETWORKINGPRIVATE_H__ */
