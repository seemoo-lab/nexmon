/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Public typelib API
 *
 * Copyright (C) 2005 Matthias Clasen
 * Copyright (C) 2008,2009 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#pragma once

#if !defined (__GIREPOSITORY_H_INSIDE__) && !defined (GI_COMPILATION)
#error "Only <girepository.h> can be included directly."
#endif

#include <glib.h>

#include <girepository/gi-visibility.h>

G_BEGIN_DECLS

typedef struct _GITypelib GITypelib;

#define GI_TYPE_TYPELIB (gi_typelib_get_type ())
GI_AVAILABLE_IN_ALL
GType gi_typelib_get_type (void);

GI_AVAILABLE_IN_ALL
GITypelib *    gi_typelib_new_from_bytes       (GBytes  *bytes,
                                                GError **error);

GI_AVAILABLE_IN_ALL
GITypelib *   gi_typelib_ref                   (GITypelib     *typelib);
GI_AVAILABLE_IN_ALL
void          gi_typelib_unref                 (GITypelib     *typelib);

GI_AVAILABLE_IN_ALL
gboolean      gi_typelib_symbol                (GITypelib     *typelib,
                                                const char    *symbol_name,
                                                void         **symbol);

GI_AVAILABLE_IN_ALL
const char *  gi_typelib_get_namespace         (GITypelib     *typelib);


G_END_DECLS
