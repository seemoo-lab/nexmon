/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Enum and Enum values
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

#define GI_TYPE_ENUM_INFO (gi_enum_info_get_type ())

/**
 * GI_ENUM_INFO:
 * @info: Info object which is subject to casting.
 *
 * Casts a [type@GIRepository.EnumInfo] or derived pointer into a
 * `(GIEnumInfo*)` pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 *
 * Since: 2.80
 */
#define GI_ENUM_INFO(info) (G_TYPE_CHECK_INSTANCE_CAST ((info), GI_TYPE_ENUM_INFO, GIEnumInfo))

/**
 * GI_IS_ENUM_INFO:
 * @info: an info structure
 *
 * Checks if @info is a [class@GIRepository.EnumInfo] (or a derived type).
 *
 * Since: 2.80
 */
#define GI_IS_ENUM_INFO(info) (G_TYPE_CHECK_INSTANCE_TYPE ((info), GI_TYPE_ENUM_INFO))


GI_AVAILABLE_IN_ALL
unsigned int   gi_enum_info_get_n_values      (GIEnumInfo  *info);

GI_AVAILABLE_IN_ALL
GIValueInfo  * gi_enum_info_get_value         (GIEnumInfo  *info,
                                               unsigned int n);

GI_AVAILABLE_IN_ALL
unsigned int      gi_enum_info_get_n_methods  (GIEnumInfo  *info);

GI_AVAILABLE_IN_ALL
GIFunctionInfo  * gi_enum_info_get_method     (GIEnumInfo  *info,
                                               unsigned int n);

GI_AVAILABLE_IN_ALL
GITypeTag      gi_enum_info_get_storage_type  (GIEnumInfo  *info);

GI_AVAILABLE_IN_ALL
const char *   gi_enum_info_get_error_domain   (GIEnumInfo  *info);

G_END_DECLS
