/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Union
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

#include <girepository/gitypes.h>

G_BEGIN_DECLS

#define GI_TYPE_UNION_INFO (gi_union_info_get_type ())

/**
 * GI_UNION_INFO:
 * @info: Info object which is subject to casting.
 *
 * Casts a [type@GIRepository.UnionInfo] or derived pointer into a
 * `(GIUnionInfo*)` pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 *
 * Since: 2.80
 */
#define GI_UNION_INFO(info) (G_TYPE_CHECK_INSTANCE_CAST ((info), GI_TYPE_UNION_INFO, GIUnionInfo))

/**
 * GI_IS_UNION_INFO:
 * @info: an info structure
 *
 * Checks if @info is a [struct@GIRepository.UnionInfo] (or a derived type).
 *
 * Since: 2.80
 */
#define GI_IS_UNION_INFO(info) (G_TYPE_CHECK_INSTANCE_TYPE ((info), GI_TYPE_UNION_INFO))

GI_AVAILABLE_IN_ALL
unsigned int     gi_union_info_get_n_fields             (GIUnionInfo *info);

GI_AVAILABLE_IN_ALL
GIFieldInfo *    gi_union_info_get_field                (GIUnionInfo *info,
                                                         unsigned int n);

GI_AVAILABLE_IN_ALL
unsigned int     gi_union_info_get_n_methods            (GIUnionInfo *info);

GI_AVAILABLE_IN_ALL
GIFunctionInfo * gi_union_info_get_method               (GIUnionInfo *info,
                                                         unsigned int n);

GI_AVAILABLE_IN_ALL
gboolean         gi_union_info_is_discriminated         (GIUnionInfo *info);

GI_AVAILABLE_IN_ALL
gboolean         gi_union_info_get_discriminator_offset (GIUnionInfo *info,
                                                         size_t      *out_offset);

GI_AVAILABLE_IN_ALL
GITypeInfo *     gi_union_info_get_discriminator_type   (GIUnionInfo *info);

GI_AVAILABLE_IN_ALL
GIConstantInfo * gi_union_info_get_discriminator        (GIUnionInfo *info,
                                                         size_t       n);

GI_AVAILABLE_IN_ALL
GIFunctionInfo * gi_union_info_find_method              (GIUnionInfo *info,
                                                         const char  *name);

GI_AVAILABLE_IN_ALL
size_t           gi_union_info_get_size                 (GIUnionInfo *info);

GI_AVAILABLE_IN_ALL
size_t           gi_union_info_get_alignment            (GIUnionInfo *info);

GI_AVAILABLE_IN_ALL
const char *     gi_union_info_get_copy_function_name   (GIUnionInfo *info);

GI_AVAILABLE_IN_ALL
const char *     gi_union_info_get_free_function_name   (GIUnionInfo *info);

G_END_DECLS
