/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Clemens N. Buss <cebuzz@gmail.com>
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
 */

#ifndef __G_EMBLEM_H__
#define __G_EMBLEM_H__

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#include <gio/gioenums.h>

G_BEGIN_DECLS

#define G_TYPE_EMBLEM (g_emblem_get_type ())
GIO_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (GEmblem, g_emblem, G, EMBLEM, GObject)

GIO_AVAILABLE_IN_ALL
GEmblem       *g_emblem_new             (GIcon         *icon);
GIO_AVAILABLE_IN_ALL
GEmblem       *g_emblem_new_with_origin (GIcon         *icon,
                                         GEmblemOrigin  origin);
GIO_AVAILABLE_IN_ALL
GIcon         *g_emblem_get_icon        (GEmblem       *emblem);
GIO_AVAILABLE_IN_ALL
GEmblemOrigin  g_emblem_get_origin      (GEmblem       *emblem);

G_END_DECLS

#endif /* __G_EMBLEM_H__ */
