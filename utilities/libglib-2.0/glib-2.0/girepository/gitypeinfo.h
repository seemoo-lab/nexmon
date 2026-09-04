/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Type
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

#define GI_TYPE_TYPE_INFO (gi_type_info_get_type ())

/**
 * GI_TYPE_INFO:
 * @info: Info object which is subject to casting.
 *
 * Casts a [type@GIRepository.TypeInfo] or derived pointer into a
 * `(GITypeInfo*)` pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 *
 * Since: 2.80
 */
#define GI_TYPE_INFO(info) (G_TYPE_CHECK_INSTANCE_CAST ((info), GI_TYPE_TYPE_INFO, GITypeInfo))

/**
 * GI_IS_TYPE_INFO:
 * @info: an info structure
 *
 * Checks if @info is a [alias@GIRepository.TypeInfo] (or a derived type).
 *
 * Since: 2.80
 */
#define GI_IS_TYPE_INFO(info) (G_TYPE_CHECK_INSTANCE_TYPE ((info), GI_TYPE_TYPE_INFO))

/**
 * GI_TYPE_TAG_IS_BASIC:
 * @tag: a type tag
 *
 * Checks if @tag is a basic type.
 *
 * Since: 2.80
 */
#define GI_TYPE_TAG_IS_BASIC(tag) ((tag) < GI_TYPE_TAG_ARRAY || (tag) == GI_TYPE_TAG_UNICHAR)

/**
 * GI_TYPE_TAG_IS_NUMERIC:
 * @tag: a type tag
 *
 * Checks if @tag is a numeric type. That is, integer or floating point.
 *
 * Since: 2.80
 */
#define GI_TYPE_TAG_IS_NUMERIC(tag) ((tag) >= GI_TYPE_TAG_INT8 && (tag) <= GI_TYPE_TAG_DOUBLE)

/**
 * GI_TYPE_TAG_IS_CONTAINER:
 * @tag: a type tag
 *
 * Checks if @tag is a container type. That is, a type which may have a nonnull
 * return from [method@GIRepository.TypeInfo.get_param_type].
 *
 * Since: 2.80
 */
 #define GI_TYPE_TAG_IS_CONTAINER(tag) ((tag) == GI_TYPE_TAG_ARRAY || \
    ((tag) >= GI_TYPE_TAG_GLIST && (tag) <= GI_TYPE_TAG_GHASH))

GI_AVAILABLE_IN_ALL
const char *           gi_type_tag_to_string            (GITypeTag   type);

GI_AVAILABLE_IN_ALL
gboolean               gi_type_info_is_pointer          (GITypeInfo *info);

GI_AVAILABLE_IN_ALL
GITypeTag              gi_type_info_get_tag             (GITypeInfo *info);

GI_AVAILABLE_IN_ALL
GITypeInfo *           gi_type_info_get_param_type      (GITypeInfo  *info,
                                                         unsigned int n);

GI_AVAILABLE_IN_ALL
GIBaseInfo *           gi_type_info_get_interface       (GITypeInfo *info);

GI_AVAILABLE_IN_ALL
gboolean               gi_type_info_get_array_length_index (GITypeInfo   *info,
                                                            unsigned int *out_length_index);

GI_AVAILABLE_IN_ALL
gboolean               gi_type_info_get_array_fixed_size (GITypeInfo *info,
                                                          size_t     *out_size);

GI_AVAILABLE_IN_ALL
gboolean               gi_type_info_is_zero_terminated  (GITypeInfo *info);

GI_AVAILABLE_IN_ALL
GIArrayType            gi_type_info_get_array_type      (GITypeInfo *info);

GI_AVAILABLE_IN_ALL
GITypeTag              gi_type_info_get_storage_type    (GITypeInfo *info);

GI_AVAILABLE_IN_ALL
void                   gi_type_info_argument_from_hash_pointer (GITypeInfo *info,
                                                                void       *hash_pointer,
                                                                GIArgument *arg);

GI_AVAILABLE_IN_ALL
void *                 gi_type_info_hash_pointer_from_argument (GITypeInfo *info,
                                                                GIArgument *arg);

GI_AVAILABLE_IN_ALL
void                   gi_type_tag_argument_from_hash_pointer (GITypeTag   storage_type,
                                                               void       *hash_pointer,
                                                               GIArgument *arg);

GI_AVAILABLE_IN_ALL
void *                 gi_type_tag_hash_pointer_from_argument (GITypeTag   storage_type,
                                                               GIArgument *arg);

G_END_DECLS
