/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Private headers
 *
 * Copyright (C) 2010 Johan Dahlin
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

#include <glib.h>

#define __GIREPOSITORY_H_INSIDE__

#include <girepository/gibaseinfo.h>
#include <girepository/girepository.h>
#include <girepository/gitypelib.h>

#include "gitypelib-internal.h"

/* FIXME: For now, GIRealInfo is a compatibility define. This will eventually
 * be removed. */
typedef struct _GIBaseInfo GIRealInfo;

/*
 * We just use one structure for all of the info object
 * types; in general, we should be reading data directly
 * from the typelib, and not having computed data in
 * per-type structures.
 */
struct _GIBaseInfo
{
  /*< private >*/
  GTypeInstance parent_instance;
  gatomicrefcount ref_count;

  /* @repository is never reffed, as that would lead to a refcount cycle with the repository */
  GIRepository *repository;
  /* @container is reffed if the GIBaseInfo is heap-allocated, but not reffed if it’s stack-allocated */
  GIBaseInfo *container;

  GITypelib *typelib;
  uint32_t offset;

  unsigned int type_is_embedded : 1;  /* Used by GITypeInfo */
  unsigned int padding_bitfield : 31; /* For future expansion */

  /* A copy of GIBaseInfo is exposed publicly for stack-allocated derivatives
   * such as GITypeInfo, so its size is now ABI. */
  void *padding[6];
};

G_STATIC_ASSERT (sizeof (GIBaseInfo) == sizeof (GIBaseInfoStack));
G_STATIC_ASSERT (G_ALIGNOF (GIBaseInfo) == G_ALIGNOF (GIBaseInfoStack));

/**
 * GIInfoType:
 * @GI_INFO_TYPE_INVALID: invalid type
 * @GI_INFO_TYPE_FUNCTION: function, see [class@GIRepository.FunctionInfo]
 * @GI_INFO_TYPE_CALLBACK: callback, see [class@GIRepository.FunctionInfo]
 * @GI_INFO_TYPE_STRUCT: struct, see [class@GIRepository.StructInfo]
 * @GI_INFO_TYPE_ENUM: enum, see [class@GIRepository.EnumInfo]
 * @GI_INFO_TYPE_FLAGS: flags, see [class@GIRepository.EnumInfo]
 * @GI_INFO_TYPE_OBJECT: object, see [class@GIRepository.ObjectInfo]
 * @GI_INFO_TYPE_INTERFACE: interface, see [class@GIRepository.InterfaceInfo]
 * @GI_INFO_TYPE_CONSTANT: constant, see [class@GIRepository.ConstantInfo]
 * @GI_INFO_TYPE_UNION: union, see [class@GIRepository.UnionInfo]
 * @GI_INFO_TYPE_VALUE: enum value, see [class@GIRepository.ValueInfo]
 * @GI_INFO_TYPE_SIGNAL: signal, see [class@GIRepository.SignalInfo]
 * @GI_INFO_TYPE_VFUNC: virtual function, see [class@GIRepository.VFuncInfo]
 * @GI_INFO_TYPE_PROPERTY: [class@GObject.Object] property, see
 *   [class@GIRepository.PropertyInfo]
 * @GI_INFO_TYPE_FIELD: struct or union field, see
 *   [class@GIRepository.FieldInfo]
 * @GI_INFO_TYPE_ARG: argument of a function or callback, see
 *   [class@GIRepository.ArgInfo]
 * @GI_INFO_TYPE_TYPE: type information, see [class@GIRepository.TypeInfo]
 * @GI_INFO_TYPE_UNRESOLVED: unresolved type, a type which is not present in
 *   the typelib, or any of its dependencies, see
 *   [class@GIRepository.UnresolvedInfo]
 * @GI_INFO_TYPE_CALLABLE: an abstract type representing any callable (function,
 *   callback, vfunc), see [class@GIRepository.CallableInfo]
 * @GI_INFO_TYPE_REGISTERED_TYPE: an abstract type representing any registered
 *   type (enum, interface, object, struct, union), see
 *   [class@GIRepository.RegisteredTypeInfo]
 *
 * The type of a [class@GIRepository.BaseInfo] struct.
 *
 * See [const@GIRepository.INFO_TYPE_N_TYPES] for the total number of elements
 * in this enum.
 *
 * Since: 2.80
 */
typedef enum
{
  /* The values here must be kept in sync with GITypelibBlobType */
  GI_INFO_TYPE_INVALID,
  GI_INFO_TYPE_FUNCTION,
  GI_INFO_TYPE_CALLBACK,
  GI_INFO_TYPE_STRUCT,
  /* 4 is skipped, it used to be BOXED, but was removed in girepository 2.80.
   * It is still part of the binary format in GITypelibBlobType. */
  GI_INFO_TYPE_ENUM = 5,             /*  5 */
  GI_INFO_TYPE_FLAGS = 6,
  GI_INFO_TYPE_OBJECT = 7,
  GI_INFO_TYPE_INTERFACE = 8,
  GI_INFO_TYPE_CONSTANT = 9,
  /* 10 is skipped, it used to be used, but was removed before girepository-2.0
   * It is, however, part of the binary format in GITypelibBlobType */
  GI_INFO_TYPE_UNION = 11,
  GI_INFO_TYPE_VALUE = 12,
  GI_INFO_TYPE_SIGNAL = 13,
  GI_INFO_TYPE_VFUNC = 14,
  GI_INFO_TYPE_PROPERTY = 15,
  GI_INFO_TYPE_FIELD = 16,
  GI_INFO_TYPE_ARG = 17,
  GI_INFO_TYPE_TYPE = 18,
  GI_INFO_TYPE_UNRESOLVED = 19,
  GI_INFO_TYPE_CALLABLE = 20,
  GI_INFO_TYPE_REGISTERED_TYPE = 21,
  /* keep GI_INFO_TYPE_N_TYPES in sync with this */
} GIInfoType;

GIInfoType gi_typelib_blob_type_to_info_type (GITypelibBlobType blob_type);

/**
 * GI_INFO_TYPE_N_TYPES:
 *
 * Number of entries in [enum@GIRepository.InfoType].
 *
 * Since: 2.80
 */
#define GI_INFO_TYPE_N_TYPES (GI_INFO_TYPE_REGISTERED_TYPE + 1)

const char *           gi_info_type_to_string           (GIInfoType  type);

/* Subtypes */
struct _GICallableInfo
{
  GIBaseInfo parent;
};

void gi_callable_info_class_init (gpointer g_class,
                                  gpointer class_data);

struct _GIFunctionInfo
{
  GICallableInfo parent;
};

void gi_function_info_class_init (gpointer g_class,
                                  gpointer class_data);

struct _GICallbackInfo
{
  GICallableInfo parent;
};

void gi_callback_info_class_init (gpointer g_class,
                                  gpointer class_data);

struct _GIRegisteredTypeInfo
{
  GIBaseInfo parent;
};

void gi_registered_type_info_class_init (gpointer g_class,
                                         gpointer class_data);

struct _GIStructInfo
{
  GIRegisteredTypeInfo parent;
};

void gi_struct_info_class_init (gpointer g_class,
                                gpointer class_data);

struct _GIUnionInfo
{
  GIRegisteredTypeInfo parent;
};

void gi_union_info_class_init (gpointer g_class,
                               gpointer class_data);

struct _GIEnumInfo
{
  GIRegisteredTypeInfo parent;
};

void gi_enum_info_class_init (gpointer g_class,
                              gpointer class_data);

struct _GIFlagsInfo
{
  GIEnumInfo parent;
};

void gi_flags_info_class_init (gpointer g_class,
                               gpointer class_data);

struct _GIObjectInfo
{
  GIRegisteredTypeInfo parent;
};

void gi_object_info_class_init (gpointer g_class,
                                gpointer class_data);

struct _GIInterfaceInfo
{
  GIRegisteredTypeInfo parent;
};

void gi_interface_info_class_init (gpointer g_class,
                                   gpointer class_data);

struct _GIConstantInfo
{
  GIBaseInfo parent;
};

void gi_constant_info_class_init (gpointer g_class,
                                  gpointer class_data);

struct _GIValueInfo
{
  GIBaseInfo parent;
};

void gi_value_info_class_init (gpointer g_class,
                               gpointer class_data);

struct _GISignalInfo
{
  GICallableInfo parent;
};

void gi_signal_info_class_init (gpointer g_class,
                                gpointer class_data);

struct _GIVFuncInfo
{
  GICallableInfo parent;
};

void gi_vfunc_info_class_init (gpointer g_class,
                               gpointer class_data);

struct _GIPropertyInfo
{
  GIBaseInfo parent;
};

void gi_property_info_class_init (gpointer g_class,
                                  gpointer class_data);

struct _GIFieldInfo
{
  GIBaseInfo parent;
};

void gi_field_info_class_init (gpointer g_class,
                               gpointer class_data);

/* GIArgInfo is stack-allocatable so it can be used with
 * gi_callable_info_load_return_type() and gi_callable_info_load_arg(), so its
 * definition is actually public in gitypes.h. */

void gi_arg_info_class_init (gpointer g_class,
                             gpointer class_data);

/* GITypeInfo is stack-allocatable so it can be used with
 * gi_arg_info_load_type_info(), so its definition is actually public in
 * gitypes.h. */

void gi_type_info_class_init (gpointer g_class,
                              gpointer class_data);

struct _GIUnresolvedInfo
{
  GIBaseInfo parent;

  const char *name;
  const char *namespace;
};

void gi_unresolved_info_class_init (gpointer g_class,
                                    gpointer class_data);

void         gi_info_init       (GIRealInfo   *info,
                                 GType         type,
                                 GIRepository *repository,
                                 GIBaseInfo   *container,
                                 GITypelib    *typelib,
                                 uint32_t      offset);

GIBaseInfo * gi_info_from_entry (GIRepository *repository,
                                 GITypelib    *typelib,
                                 uint16_t      index);

GIBaseInfo * gi_info_new_full   (GIInfoType    type,
                                 GIRepository *repository,
                                 GIBaseInfo   *container,
                                 GITypelib    *typelib,
                                 uint32_t      offset);

GITypeInfo * gi_type_info_new   (GIBaseInfo *container,
                                 GITypelib  *typelib,
                                 uint32_t    offset);

void         gi_type_info_init  (GITypeInfo *info,
                                 GIBaseInfo *container,
                                 GITypelib  *typelib,
                                 uint32_t    offset);

GIFunctionInfo * gi_base_info_find_method (GIBaseInfo  *base,
                                           uint32_t     offset,
                                           uint16_t     n_methods,
                                           const char  *name);

GIVFuncInfo * gi_base_info_find_vfunc (GIRealInfo  *rinfo,
                                       uint32_t     offset,
                                       uint16_t     n_vfuncs,
                                       const char  *name);
