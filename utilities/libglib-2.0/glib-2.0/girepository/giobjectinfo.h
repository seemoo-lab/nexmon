/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Object
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

/**
 * GIObjectInfoRefFunction: (skip)
 * @object: object instance pointer
 *
 * Increases the reference count of an object instance.
 *
 * Returns: (transfer full): the object instance
 * Since: 2.80
 */
typedef void * (*GIObjectInfoRefFunction) (void *object);

/**
 * GIObjectInfoUnrefFunction: (skip)
 * @object: (transfer full): object instance pointer
 *
 * Decreases the reference count of an object instance.
 *
 * Since: 2.80
 */
typedef void   (*GIObjectInfoUnrefFunction) (void *object);

/**
 * GIObjectInfoSetValueFunction: (skip)
 * @value: a [type@GObject.Value]
 * @object: object instance pointer
 *
 * Update @value and attach the object instance pointer @object to it.
 *
 * Since: 2.80
 */
typedef void   (*GIObjectInfoSetValueFunction) (GValue *value, void *object);

/**
 * GIObjectInfoGetValueFunction: (skip)
 * @value: a [type@GObject.Value]
 *
 * Extract an object instance out of @value.
 *
 * Returns: (transfer full): the object instance
 * Since: 2.80
 */
typedef void * (*GIObjectInfoGetValueFunction) (const GValue *value);

#define GI_TYPE_OBJECT_INFO (gi_object_info_get_type ())

/**
 * GI_OBJECT_INFO:
 * @info: Info object which is subject to casting.
 *
 * Casts a [type@GIRepository.ObjectInfo] or derived pointer into a
 * `(GIObjectInfo*)` pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 *
 * Since: 2.80
 */
#define GI_OBJECT_INFO(info) (G_TYPE_CHECK_INSTANCE_CAST ((info), GI_TYPE_OBJECT_INFO, GIObjectInfo))

/**
 * GI_IS_OBJECT_INFO:
 * @info: an info structure
 *
 * Checks if @info is a [class@GIRepository.ObjectInfo] (or a derived type).
 *
 * Since: 2.80
 */
#define GI_IS_OBJECT_INFO(info) (G_TYPE_CHECK_INSTANCE_TYPE ((info), GI_TYPE_OBJECT_INFO))


GI_AVAILABLE_IN_ALL
const char *      gi_object_info_get_type_name    (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
const char *      gi_object_info_get_type_init_function_name (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
gboolean          gi_object_info_get_abstract     (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
gboolean          gi_object_info_get_final        (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
gboolean          gi_object_info_get_fundamental  (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIObjectInfo *    gi_object_info_get_parent       (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
unsigned int      gi_object_info_get_n_interfaces (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIInterfaceInfo * gi_object_info_get_interface    (GIObjectInfo *info,
                                                   unsigned int  n);

GI_AVAILABLE_IN_ALL
unsigned int      gi_object_info_get_n_fields     (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIFieldInfo *     gi_object_info_get_field        (GIObjectInfo *info,
                                                   unsigned int  n);

GI_AVAILABLE_IN_ALL
unsigned int      gi_object_info_get_n_properties (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIPropertyInfo *  gi_object_info_get_property     (GIObjectInfo *info,
                                                   unsigned int  n);

GI_AVAILABLE_IN_ALL
unsigned int      gi_object_info_get_n_methods    (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIFunctionInfo *  gi_object_info_get_method       (GIObjectInfo *info,
                                                   unsigned int  n);

GI_AVAILABLE_IN_ALL
GIFunctionInfo *  gi_object_info_find_method      (GIObjectInfo *info,
                                                   const char   *name);


GI_AVAILABLE_IN_ALL
GIFunctionInfo *  gi_object_info_find_method_using_interfaces (GIObjectInfo  *info,
                                                               const char    *name,
                                                               GIBaseInfo   **declarer);


GI_AVAILABLE_IN_ALL
unsigned int      gi_object_info_get_n_signals    (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GISignalInfo *    gi_object_info_get_signal       (GIObjectInfo *info,
                                                   unsigned int  n);


GI_AVAILABLE_IN_ALL
GISignalInfo *    gi_object_info_find_signal      (GIObjectInfo *info,
                                                   const char   *name);


GI_AVAILABLE_IN_ALL
unsigned int      gi_object_info_get_n_vfuncs     (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIVFuncInfo *     gi_object_info_get_vfunc        (GIObjectInfo *info,
                                                   unsigned int  n);

GI_AVAILABLE_IN_ALL
GIVFuncInfo *     gi_object_info_find_vfunc       (GIObjectInfo *info,
                                                   const char   *name);

GI_AVAILABLE_IN_ALL
GIVFuncInfo *     gi_object_info_find_vfunc_using_interfaces (GIObjectInfo  *info,
                                                              const char    *name,
                                                              GIBaseInfo   **declarer);

GI_AVAILABLE_IN_ALL
unsigned int      gi_object_info_get_n_constants  (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIConstantInfo *  gi_object_info_get_constant     (GIObjectInfo *info,
                                                   unsigned int  n);

GI_AVAILABLE_IN_ALL
GIStructInfo *    gi_object_info_get_class_struct (GIObjectInfo *info);


GI_AVAILABLE_IN_ALL
const char *                 gi_object_info_get_ref_function_name          (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIObjectInfoRefFunction      gi_object_info_get_ref_function_pointer       (GIObjectInfo *info);


GI_AVAILABLE_IN_ALL
const char *                 gi_object_info_get_unref_function_name        (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIObjectInfoUnrefFunction    gi_object_info_get_unref_function_pointer     (GIObjectInfo *info);


GI_AVAILABLE_IN_ALL
const char *                 gi_object_info_get_set_value_function_name    (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIObjectInfoSetValueFunction gi_object_info_get_set_value_function_pointer (GIObjectInfo *info);


GI_AVAILABLE_IN_ALL
const char *                 gi_object_info_get_get_value_function_name    (GIObjectInfo *info);

GI_AVAILABLE_IN_ALL
GIObjectInfoGetValueFunction gi_object_info_get_get_value_function_pointer (GIObjectInfo *info);


G_END_DECLS
