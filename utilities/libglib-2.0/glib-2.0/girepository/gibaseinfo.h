/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: GIBaseInfo
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

#include <glib-object.h>
#include <girepository/gitypelib.h>
#include <girepository/gitypes.h>

G_BEGIN_DECLS

/**
 * GIAttributeIter:
 *
 * An opaque structure used to iterate over attributes
 * in a [class@GIRepository.BaseInfo] struct.
 *
 * Since: 2.80
 */
typedef struct {
  /*< private >*/
  void *data;
  void *_dummy[4];
} GIAttributeIter;

/**
 * GI_ATTRIBUTE_ITER_INIT:
 *
 * Initialise a stack-allocated [type@GIRepository.AttributeIter] to a value
 * suitable for passing to the first call to an ‘iterate’ function.
 *
 * Since: 2.80
 */
#define GI_ATTRIBUTE_ITER_INIT { NULL, { NULL, } }

#define GI_TYPE_BASE_INFO        (gi_base_info_get_type ())

/**
 * GI_BASE_INFO:
 * @info: Info object which is subject to casting.
 *
 * Casts a [type@GIRepository.BaseInfo] or derived pointer into a
 * `(GIBaseInfo*)` pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 *
 * Since: 2.80
 */
#define GI_BASE_INFO(info)       (G_TYPE_CHECK_INSTANCE_CAST ((info), GI_TYPE_BASE_INFO, GIBaseInfo))

/**
 * GI_IS_BASE_INFO:
 * @info: Instance to check for being a `GI_TYPE_BASE_INFO`.
 *
 * Checks whether a valid [type@GObject.TypeInstance] pointer is of type
 * `GI_TYPE_BASE_INFO` (or a derived type).
 *
 * Since: 2.80
 */
#define GI_IS_BASE_INFO(info)    (G_TYPE_CHECK_INSTANCE_TYPE ((info), GI_TYPE_BASE_INFO))

GI_AVAILABLE_IN_ALL
GType                  gi_base_info_get_type         (void);

GI_AVAILABLE_IN_ALL
GIBaseInfo *           gi_base_info_ref              (void         *info);

GI_AVAILABLE_IN_ALL
void                   gi_base_info_unref            (void         *info);

GI_AVAILABLE_IN_ALL
void                   gi_base_info_clear            (void         *info);

GI_AVAILABLE_IN_ALL
const char *           gi_base_info_get_name         (GIBaseInfo   *info);

GI_AVAILABLE_IN_ALL
const char *           gi_base_info_get_namespace    (GIBaseInfo   *info);

GI_AVAILABLE_IN_ALL
gboolean               gi_base_info_is_deprecated    (GIBaseInfo   *info);

GI_AVAILABLE_IN_ALL
const char *           gi_base_info_get_attribute    (GIBaseInfo  *info,
                                                      const char  *name);

GI_AVAILABLE_IN_ALL
gboolean               gi_base_info_iterate_attributes (GIBaseInfo       *info,
                                                        GIAttributeIter  *iterator,
                                                        const char      **name,
                                                        const char      **value);

GI_AVAILABLE_IN_ALL
GIBaseInfo *           gi_base_info_get_container    (GIBaseInfo   *info);

GI_AVAILABLE_IN_ALL
GITypelib *            gi_base_info_get_typelib      (GIBaseInfo   *info);

GI_AVAILABLE_IN_ALL
gboolean               gi_base_info_equal            (GIBaseInfo *info1,
                                                      GIBaseInfo *info2);

G_END_DECLS
