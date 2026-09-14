/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef __G_CONTENT_TYPE_PRIVATE_H__
#define __G_CONTENT_TYPE_PRIVATE_H__

#include "gcontenttype.h"

G_BEGIN_DECLS

gsize  _g_unix_content_type_get_sniff_len (void);
char * _g_unix_content_type_unalias       (const char *type);
char **_g_unix_content_type_get_parents   (const char *type);

void g_content_type_set_mime_dirs_impl (const gchar * const *dirs);
const gchar * const *g_content_type_get_mime_dirs_impl (void);
gboolean g_content_type_equals_impl (const gchar *type1,
                                     const gchar *type2);
gboolean g_content_type_is_a_impl (const gchar *type,
                                   const gchar *supertype);
gboolean g_content_type_is_mime_type_impl (const gchar *type,
                                           const gchar *mime_type);
gboolean g_content_type_is_unknown_impl (const gchar *type);
gchar *g_content_type_get_description_impl (const gchar *type);
char *g_content_type_get_mime_type_impl (const char *type);
GIcon *g_content_type_get_icon_impl (const gchar *type);
GIcon *g_content_type_get_symbolic_icon_impl (const gchar *type);
gchar *g_content_type_get_generic_icon_name_impl (const gchar *type);
gboolean g_content_type_can_be_executable_impl (const gchar *type);
gchar *g_content_type_from_mime_type_impl (const gchar *mime_type);
gchar *g_content_type_guess_impl (const gchar  *filename,
                                  const guchar *data,
                                  gsize         data_size,
                                  gboolean     *result_uncertain);
GList *g_content_types_get_registered_impl (void);
gchar **g_content_type_guess_for_tree_impl (GFile *root);

G_END_DECLS

#endif /* __G_CONTENT_TYPE_PRIVATE_H__ */
