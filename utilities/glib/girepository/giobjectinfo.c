/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Object implementation
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

#include "config.h"

#include <glib.h>

#include <girepository/girepository.h>
#include "gibaseinfo-private.h"
#include "girepository-private.h"
#include "gitypelib-internal.h"
#include "giobjectinfo.h"

/**
 * GIObjectInfo:
 *
 * `GIObjectInfo` represents a classed type.
 *
 * Classed types in [type@GObject.Type] inherit from
 * [type@GObject.TypeInstance]; the most common type is [class@GObject.Object].
 *
 * A `GIObjectInfo` doesn’t represent a specific instance of a classed type,
 * instead this represent the object type (i.e. the class).
 *
 * A `GIObjectInfo` has methods, fields, properties, signals, interfaces,
 * constants and virtual functions.
 *
 * Since: 2.80
 */

/**
 * gi_object_info_get_field_offset:
 * @info: a #GIObjectInfo
 * @n: index of queried field
 *
 * Obtain the offset of the specified field.
 *
 * Returns: field offset, in bytes
 * Since: 2.80
 */
static size_t
gi_object_info_get_field_offset (GIObjectInfo *info,
                                 size_t        n)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header = (Header *)rinfo->typelib->data;
  ObjectBlob *blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];
  uint32_t offset;
  FieldBlob *field_blob;

  offset = rinfo->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2u) * 2u;

  for (size_t i = 0; i < n; i++)
    {
      field_blob = (FieldBlob *)&rinfo->typelib->data[offset];
      offset += header->field_blob_size;
      if (field_blob->has_embedded_type)
        offset += header->callback_blob_size;
    }

  return offset;
}

/**
 * gi_object_info_get_parent:
 * @info: a #GIObjectInfo
 *
 * Obtain the parent of the object type.
 *
 * Returns: (transfer full) (nullable): The `GIObjectInfo`. Free the struct by
 *   calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIObjectInfo *
gi_object_info_get_parent (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->parent)
    return (GIObjectInfo *) gi_info_from_entry (rinfo->repository,
                                                rinfo->typelib, blob->parent);
  else
    return NULL;
}

/**
 * gi_object_info_get_abstract:
 * @info: a #GIObjectInfo
 *
 * Obtain if the object type is an abstract type, i.e. if it cannot be
 * instantiated.
 *
 * Returns: `TRUE` if the object type is abstract
 * Since: 2.80
 */
gboolean
gi_object_info_get_abstract (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), FALSE);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->abstract != 0;
}

/**
 * gi_object_info_get_final:
 * @info: a #GIObjectInfo
 *
 * Checks whether the object type is a final type, i.e. if it cannot
 * be derived.
 *
 * Returns: `TRUE` if the object type is final
 * Since: 2.80
 */
gboolean
gi_object_info_get_final (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *) info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), FALSE);

  blob = (ObjectBlob *) &rinfo->typelib->data[rinfo->offset];

  return blob->final_ != 0;
}

/**
 * gi_object_info_get_fundamental:
 * @info: a #GIObjectInfo
 *
 * Obtain if the object type is of a fundamental type which is not
 * `G_TYPE_OBJECT`.
 *
 * This is mostly for supporting `GstMiniObject`.
 *
 * Returns: `TRUE` if the object type is a fundamental type
 * Since: 2.80
 */
gboolean
gi_object_info_get_fundamental (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), FALSE);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->fundamental != 0;
}

/**
 * gi_object_info_get_type_name:
 * @info: a #GIObjectInfo
 *
 * Obtain the name of the object’s class/type.
 *
 * Returns: name of the object’s type
 * Since: 2.80
 */
const char *
gi_object_info_get_type_name (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return gi_typelib_get_string (rinfo->typelib, blob->gtype_name);
}

/**
 * gi_object_info_get_type_init_function_name:
 * @info: a #GIObjectInfo
 *
 * Obtain the name of the function which, when called, will return the
 * [type@GObject.Type] for this object type.
 *
 * Returns: the type init function name
 * Since: 2.80
 */
const char *
gi_object_info_get_type_init_function_name (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return gi_typelib_get_string (rinfo->typelib, blob->gtype_init);
}

/**
 * gi_object_info_get_n_interfaces:
 * @info: a #GIObjectInfo
 *
 * Obtain the number of interfaces that this object type has.
 *
 * Returns: number of interfaces
 * Since: 2.80
 */
unsigned int
gi_object_info_get_n_interfaces (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), 0);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_interfaces;
}

/**
 * gi_object_info_get_interface:
 * @info: a #GIObjectInfo
 * @n: index of interface to get
 *
 * Obtain an object type interface at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.InterfaceInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIInterfaceInfo *
gi_object_info_get_interface (GIObjectInfo *info,
                              unsigned int  n)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return (GIInterfaceInfo *) gi_info_from_entry (rinfo->repository,
                                                 rinfo->typelib, blob->interfaces[n]);
}

/**
 * gi_object_info_get_n_fields:
 * @info: a #GIObjectInfo
 *
 * Obtain the number of fields that this object type has.
 *
 * Returns: number of fields
 * Since: 2.80
 */
unsigned int
gi_object_info_get_n_fields (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), 0);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_fields;
}

/**
 * gi_object_info_get_field:
 * @info: a #GIObjectInfo
 * @n: index of field to get
 *
 * Obtain an object type field at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.FieldInfo]. Free the struct
 *   by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFieldInfo *
gi_object_info_get_field (GIObjectInfo *info,
                          unsigned int  n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  offset = gi_object_info_get_field_offset(info, n);

  return (GIFieldInfo *) gi_base_info_new (GI_INFO_TYPE_FIELD, (GIBaseInfo*)info, rinfo->typelib, offset);
}

/**
 * gi_object_info_get_n_properties:
 * @info: a #GIObjectInfo
 *
 * Obtain the number of properties that this object type has.
 *
 * Returns: number of properties
 * Since: 2.80
 */
unsigned int
gi_object_info_get_n_properties (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), 0);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];
  return blob->n_properties;
}

/**
 * gi_object_info_get_property:
 * @info: a #GIObjectInfo
 * @n: index of property to get
 *
 * Obtain an object type property at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.PropertyInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIPropertyInfo *
gi_object_info_get_property (GIObjectInfo *info,
                             unsigned int  n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2u) * 2u
    + blob->n_fields * header->field_blob_size
    + blob->n_field_callbacks * header->callback_blob_size
    + n * header->property_blob_size;

  return (GIPropertyInfo *) gi_base_info_new (GI_INFO_TYPE_PROPERTY, (GIBaseInfo*)info,
                                              rinfo->typelib, offset);
}

/**
 * gi_object_info_get_n_methods:
 * @info: a #GIObjectInfo
 *
 * Obtain the number of methods that this object type has.
 *
 * Returns: number of methods
 * Since: 2.80
 */
unsigned int
gi_object_info_get_n_methods (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), 0);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_methods;
}

/**
 * gi_object_info_get_method:
 * @info: a #GIObjectInfo
 * @n: index of method to get
 *
 * Obtain an object type method at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.FunctionInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_object_info_get_method (GIObjectInfo *info,
                           unsigned int  n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];


  offset = rinfo->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2u) * 2u
    + blob->n_fields * header->field_blob_size
    + blob->n_field_callbacks * header->callback_blob_size
    + blob->n_properties * header->property_blob_size
    + n * header->function_blob_size;

    return (GIFunctionInfo *) gi_base_info_new (GI_INFO_TYPE_FUNCTION, (GIBaseInfo*)info,
                                                rinfo->typelib, offset);
}

/**
 * gi_object_info_find_method:
 * @info: a #GIObjectInfo
 * @name: name of method to obtain
 *
 * Obtain a method of the object type given a @name.
 *
 * `NULL` will be returned if there’s no method available with that name.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.FunctionInfo],
 *   or `NULL` if no method could be found. Free the struct by calling
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_object_info_find_method (GIObjectInfo *info,
                            const char   *name)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), 0);

  header = (Header *)rinfo->typelib->data;
  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2u) * 2u
    + blob->n_fields * header->field_blob_size +
    + blob->n_field_callbacks * header->callback_blob_size
    + blob->n_properties * header->property_blob_size;

  return gi_base_info_find_method ((GIBaseInfo*)info, offset, blob->n_methods, name);
}

/**
 * gi_object_info_find_method_using_interfaces:
 * @info: a #GIObjectInfo
 * @name: name of method to obtain
 * @declarer: (out) (transfer full) (optional) (nullable): The
 *   [class@GIRepository.ObjectInfo] or [class@GIRepository.InterfaceInfo] which
 *   declares the method, or `NULL` to ignore. If no method is found, this will
 *   return `NULL`.
 *
 * Obtain a method of the object given a @name, searching both the
 * object @info and any interfaces it implements.
 *
 * `NULL` will be returned if there’s no method available with that name.
 *
 * Note that this function does *not* search parent classes; you will have
 * to chain up if that’s desired.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.FunctionInfo],
 *   or `NULL` if none was found. Free the struct by calling
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_object_info_find_method_using_interfaces (GIObjectInfo  *info,
                                             const char    *name,
                                             GIBaseInfo   **declarer)
{
  GIFunctionInfo *result = NULL;
  GIBaseInfo *declarer_result = NULL;

  result = gi_object_info_find_method (info, name);
  if (result)
    declarer_result = gi_base_info_ref (info);

  if (result == NULL)
    {
      unsigned int n_interfaces, i;

      n_interfaces = gi_object_info_get_n_interfaces (info);
      for (i = 0; i < n_interfaces; ++i)
        {
          GIInterfaceInfo *iface_info;

          iface_info = gi_object_info_get_interface (info, i);

          result = gi_interface_info_find_method (iface_info, name);

          if (result != NULL)
            {
              declarer_result = GI_BASE_INFO (g_steal_pointer (&iface_info));
              break;
            }
          gi_base_info_unref ((GIBaseInfo*) iface_info);
        }
    }

  if (declarer)
    *declarer = g_steal_pointer (&declarer_result);

  g_clear_pointer (&declarer_result, gi_base_info_unref);

  return g_steal_pointer (&result);
}

/**
 * gi_object_info_get_n_signals:
 * @info: a #GIObjectInfo
 *
 * Obtain the number of signals that this object type has.
 *
 * Returns: number of signals
 * Since: 2.80
 */
unsigned int
gi_object_info_get_n_signals (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), 0);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_signals;
}

static uint32_t
object_get_signal_offset (GIObjectInfo *info, unsigned int n)
{
  GIRealInfo *rinfo = (GIRealInfo *) info;
  Header *header = (Header *) rinfo->typelib->data;
  ObjectBlob *blob = (ObjectBlob *) &rinfo->typelib->data[rinfo->offset];

  return rinfo->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2u) * 2u
    + blob->n_fields * header->field_blob_size
    + blob->n_field_callbacks * header->callback_blob_size
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size
    + n * header->signal_blob_size;
}

/**
 * gi_object_info_get_signal:
 * @info: a #GIObjectInfo
 * @n: index of signal to get
 *
 * Obtain an object type signal at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.SignalInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GISignalInfo *
gi_object_info_get_signal (GIObjectInfo *info,
                           unsigned int  n)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  return (GISignalInfo *) gi_base_info_new (GI_INFO_TYPE_SIGNAL, (GIBaseInfo*)info,
                                            rinfo->typelib, object_get_signal_offset (info, n));
}

static GISignalInfo *
find_signal (GIRealInfo  *rinfo,
             uint32_t     offset,
             uint16_t     n_signals,
             const char  *name)
{
  Header *header = (Header *) rinfo->typelib->data;

  for (uint16_t i = 0; i < n_signals; i++)
    {
      const SignalBlob *sblob = (SignalBlob *) &rinfo->typelib->data[offset];
      const char *sname = gi_typelib_get_string (rinfo->typelib, sblob->name);

      if (strcmp (name, sname) == 0)
        return (GISignalInfo *) gi_base_info_new (GI_INFO_TYPE_SIGNAL, (GIBaseInfo *) rinfo,
                                                  rinfo->typelib, offset);

      offset += header->signal_blob_size;
    }

  return NULL;
}

/**
 * gi_object_info_find_signal:
 * @info: a #GIObjectInfo
 * @name: name of signal
 *
 * Obtain a signal of the object type given a @name.
 *
 * `NULL` will be returned if there’s no signal available with that name.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.SignalInfo],
 *   or `NULL` if no signal could be found. Free the struct by calling
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GISignalInfo *
gi_object_info_find_signal (GIObjectInfo *info,
                            const char   *name)
{
  GIRealInfo *rinfo = (GIRealInfo *) info;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  return find_signal (rinfo, object_get_signal_offset (info, 0),
                      gi_object_info_get_n_signals (info), name);
}


/**
 * gi_object_info_get_n_vfuncs:
 * @info: a #GIObjectInfo
 *
 * Obtain the number of virtual functions that this object type has.
 *
 * Returns: number of virtual functions
 * Since: 2.80
 */
unsigned int
gi_object_info_get_n_vfuncs (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), 0);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_vfuncs;
}

/**
 * gi_object_info_get_vfunc:
 * @info: a #GIObjectInfo
 * @n: index of virtual function to get
 *
 * Obtain an object type virtual function at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.VFuncInfo]. Free the struct
 *   by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIVFuncInfo *
gi_object_info_get_vfunc (GIObjectInfo *info,
                          unsigned int  n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2u) * 2u
    + blob->n_fields * header->field_blob_size
    + blob->n_field_callbacks * header->callback_blob_size
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size
    + blob->n_signals * header->signal_blob_size
    + n * header->vfunc_blob_size;

  return (GIVFuncInfo *) gi_base_info_new (GI_INFO_TYPE_VFUNC, (GIBaseInfo*)info,
                                           rinfo->typelib, offset);
}

/**
 * gi_object_info_find_vfunc:
 * @info: a #GIObjectInfo
 * @name: the name of a virtual function to find.
 *
 * Locate a virtual function slot with name @name.
 *
 * Note that the namespace for virtuals is distinct from that of methods; there
 * may or may not be a concrete method associated for a virtual. If there is
 * one, it may be retrieved using [method@GIRepository.VFuncInfo.get_invoker],
 * otherwise that method will return `NULL`.
 *
 * See the documentation for [method@GIRepository.VFuncInfo.get_invoker] for
 * more information on invoking virtuals.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.VFuncInfo], or
 *   `NULL` if none is found. Free it with [method@GIRepository.BaseInfo.unref]
 *   when done.
 * Since: 2.80
 */
GIVFuncInfo *
gi_object_info_find_vfunc (GIObjectInfo *info,
                           const char   *name)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2u) * 2u
    + blob->n_fields * header->field_blob_size
    + blob->n_field_callbacks * header->callback_blob_size
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size
    + blob->n_signals * header->signal_blob_size;

  return gi_base_info_find_vfunc (rinfo, offset, blob->n_vfuncs, name);
}

/**
 * gi_object_info_find_vfunc_using_interfaces:
 * @info: a #GIObjectInfo
 * @name: name of vfunc to obtain
 * @declarer: (out) (transfer full) (optional) (nullable): The
 *   [class@GIRepository.ObjectInfo] or [class@GIRepository.InterfaceInfo] which
 *   declares the vfunc, or `NULL` to ignore. If no vfunc is found, this will
 *   return `NULL`.
 *
 * Locate a virtual function slot with name @name, searching both the object
 * @info and any interfaces it implements.
 *
 * `NULL` will be returned if there’s no vfunc available with that name.
 *
 * Note that the namespace for virtuals is distinct from that of methods; there
 * may or may not be a concrete method associated for a virtual. If there is
 * one, it may be retrieved using [method@GIRepository.VFuncInfo.get_invoker],
 * otherwise that method will return `NULL`.
 *
 * Note that this function does *not* search parent classes; you will have
 * to chain up if that’s desired.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.VFuncInfo],
 *   or `NULL` if none was found. Free the struct by calling
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIVFuncInfo *
gi_object_info_find_vfunc_using_interfaces (GIObjectInfo  *info,
                                            const char    *name,
                                            GIBaseInfo   **declarer)
{
  GIVFuncInfo *result = NULL;
  GIBaseInfo *declarer_result = NULL;

  result = gi_object_info_find_vfunc (info, name);
  if (result)
    declarer_result = gi_base_info_ref (info);

  if (result == NULL)
    {
      unsigned int n_interfaces, i;

      n_interfaces = gi_object_info_get_n_interfaces (info);
      for (i = 0; i < n_interfaces; ++i)
        {
          GIInterfaceInfo *iface_info;

          iface_info = gi_object_info_get_interface (info, i);

          result = gi_interface_info_find_vfunc (iface_info, name);

          if (result != NULL)
            {
              declarer_result = GI_BASE_INFO (g_steal_pointer (&iface_info));
              break;
            }
          gi_base_info_unref ((GIBaseInfo*) iface_info);
        }
    }

  if (declarer)
    *declarer = g_steal_pointer (&declarer_result);

  g_clear_pointer (&declarer_result, gi_base_info_unref);

  return g_steal_pointer (&result);
}

/**
 * gi_object_info_get_n_constants:
 * @info: a #GIObjectInfo
 *
 * Obtain the number of constants that this object type has.
 *
 * Returns: number of constants
 * Since: 2.80
 */
unsigned int
gi_object_info_get_n_constants (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), 0);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_constants;
}

/**
 * gi_object_info_get_constant:
 * @info: a #GIObjectInfo
 * @n: index of constant to get
 *
 * Obtain an object type constant at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.ConstantInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIConstantInfo *
gi_object_info_get_constant (GIObjectInfo *info,
                             unsigned int  n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->object_blob_size
    + (blob->n_interfaces + blob->n_interfaces % 2u) * 2u
    + blob->n_fields * header->field_blob_size
    + blob->n_field_callbacks * header->callback_blob_size
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size
    + blob->n_signals * header->signal_blob_size
    + blob->n_vfuncs * header->vfunc_blob_size
    + n * header->constant_blob_size;

  return (GIConstantInfo *) gi_base_info_new (GI_INFO_TYPE_CONSTANT, (GIBaseInfo*)info,
                                              rinfo->typelib, offset);
}

/**
 * gi_object_info_get_class_struct:
 * @info: a #GIObjectInfo
 *
 * Every [class@GObject.Object] has two structures; an instance structure and a
 * class structure.  This function returns the metadata for the class structure.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.StructInfo] or
 *   `NULL` if it’s unknown. Free with [method@GIRepository.BaseInfo.unref] when
 *   done.
 * Since: 2.80
 */
GIStructInfo *
gi_object_info_get_class_struct (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->gtype_struct)
    return (GIStructInfo *) gi_info_from_entry (rinfo->repository,
                                                rinfo->typelib, blob->gtype_struct);
  else
    return NULL;
}

typedef const char* (*SymbolGetter) (GIObjectInfo *info);

static void *
_get_func(GIObjectInfo *info,
          SymbolGetter getter)
{
  const char* symbol;
  GSList *parents = NULL, *l;
  GIObjectInfo *parent_info;
  void *func = NULL;

  parent_info = (GIObjectInfo *) gi_base_info_ref ((GIBaseInfo *) info);
  while (parent_info != NULL)
    {
      parents = g_slist_prepend (parents, parent_info);
      parent_info = gi_object_info_get_parent (parent_info);
    }

  for (l = parents; l; l = l->next)
    {
      parent_info = l->data;
      symbol = getter (parent_info);
      if (symbol == NULL)
        continue;

      gi_typelib_symbol (((GIRealInfo *)parent_info)->typelib, symbol, (gpointer*) &func);
      if (func)
        break;
    }

  g_slist_free_full (parents, (GDestroyNotify) gi_base_info_unref);
  return func;
}

/**
 * gi_object_info_get_ref_function_name:
 * @info: a #GIObjectInfo
 *
 * Obtain the symbol name of the function that should be called to ref this
 * object type.
 *
 * It’s mainly used for fundamental types. The type signature for
 * the symbol is [type@GIRepository.ObjectInfoRefFunction]. To fetch the
 * function pointer see
 * [method@GIRepository.ObjectInfo.get_ref_function_pointer].
 *
 * Returns: (nullable): the symbol, or `NULL` if the object type has no ref
 *   function
 * Since: 2.80
 */
const char *
gi_object_info_get_ref_function_name (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->ref_func)
    return gi_typelib_get_string (rinfo->typelib, blob->ref_func);

  return NULL;
}

/**
 * gi_object_info_get_ref_function_pointer: (skip)
 * @info: a #GIObjectInfo
 *
 * Obtain a pointer to a function which can be used to
 * increase the reference count an instance of this object type.
 *
 * This takes derivation into account and will reversely traverse
 * the base classes of this type, starting at the top type.
 *
 * Returns: (nullable): the function pointer, or `NULL` if the object type has
 *   no ref function
 * Since: 2.80
 */
GIObjectInfoRefFunction
gi_object_info_get_ref_function_pointer (GIObjectInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  return (GIObjectInfoRefFunction)_get_func(info, (SymbolGetter)gi_object_info_get_ref_function_name);
}

/**
 * gi_object_info_get_unref_function_name:
 * @info: a #GIObjectInfo
 *
 * Obtain the symbol name of the function that should be called to unref this
 * object type.
 *
 * It’s mainly used for fundamental types. The type signature for the symbol is
 * [type@GIRepository.ObjectInfoUnrefFunction]. To fetch the function pointer
 * see [method@GIRepository.ObjectInfo.get_unref_function_pointer].
 *
 * Returns: (nullable): the symbol, or `NULL` if the object type has no unref
 *   function
 * Since: 2.80
 */
const char *
gi_object_info_get_unref_function_name (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->unref_func)
    return gi_typelib_get_string (rinfo->typelib, blob->unref_func);

  return NULL;
}

/**
 * gi_object_info_get_unref_function_pointer: (skip)
 * @info: a #GIObjectInfo
 *
 * Obtain a pointer to a function which can be used to
 * decrease the reference count an instance of this object type.
 *
 * This takes derivation into account and will reversely traverse
 * the base classes of this type, starting at the top type.
 *
 * Returns: (nullable): the function pointer, or `NULL` if the object type has
 *   no unref function
 * Since: 2.80
 */
GIObjectInfoUnrefFunction
gi_object_info_get_unref_function_pointer (GIObjectInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  return (GIObjectInfoUnrefFunction)_get_func(info, (SymbolGetter)gi_object_info_get_unref_function_name);
}

/**
 * gi_object_info_get_set_value_function_name:
 * @info: a #GIObjectInfo
 *
 * Obtain the symbol name of the function that should be called to set a
 * [type@GObject.Value], given an object instance pointer of this object type.
 *
 * It’s mainly used for fundamental types. The type signature for the symbol
 * is [type@GIRepository.ObjectInfoSetValueFunction]. To fetch the function
 * pointer see [method@GIRepository.ObjectInfo.get_set_value_function_pointer].
 *
 * Returns: (nullable): the symbol, or `NULL` if the object type has no
 *   set-value function
 * Since: 2.80
 */
const char *
gi_object_info_get_set_value_function_name (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->set_value_func)
    return gi_typelib_get_string (rinfo->typelib, blob->set_value_func);

  return NULL;
}

/**
 * gi_object_info_get_set_value_function_pointer: (skip)
 * @info: a #GIObjectInfo
 *
 * Obtain a pointer to a function which can be used to set a
 * [type@GObject.Value], given an instance of this object type.
 *
 * This takes derivation into account and will reversely traverse
 * the base classes of this type, starting at the top type.
 *
 * Returns: (nullable): the function pointer, or `NULL` if the object type has
 *   no set-value function
 * Since: 2.80
 */
GIObjectInfoSetValueFunction
gi_object_info_get_set_value_function_pointer (GIObjectInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  return (GIObjectInfoSetValueFunction)_get_func(info, (SymbolGetter)gi_object_info_get_set_value_function_name);
}

/**
 * gi_object_info_get_get_value_function_name:
 * @info: a #GIObjectInfo
 *
 * Obtain the symbol name of the function that should be called to convert
 * an object instance pointer of this object type to a [type@GObject.Value].
 *
 * It’s mainly used for fundamental types. The type signature for the symbol
 * is [type@GIRepository.ObjectInfoGetValueFunction]. To fetch the function
 * pointer see [method@GIRepository.ObjectInfo.get_get_value_function_pointer].
 *
 * Returns: (nullable): the symbol, or `NULL` if the object type has no
 *   get-value function
 * Since: 2.80
 */
const char *
gi_object_info_get_get_value_function_name (GIObjectInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ObjectBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  blob = (ObjectBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->get_value_func)
    return gi_typelib_get_string (rinfo->typelib, blob->get_value_func);

  return NULL;
}

/**
 * gi_object_info_get_get_value_function_pointer: (skip)
 * @info: a #GIObjectInfo
 *
 * Obtain a pointer to a function which can be used to extract an instance of
 * this object type out of a [type@GObject.Value].
 *
 * This takes derivation into account and will reversely traverse
 * the base classes of this type, starting at the top type.
 *
 * Returns: (nullable): the function pointer, or `NULL` if the object type has
 *   no get-value function
 * Since: 2.80
 */
GIObjectInfoGetValueFunction
gi_object_info_get_get_value_function_pointer (GIObjectInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_OBJECT_INFO (info), NULL);

  return (GIObjectInfoGetValueFunction)_get_func(info, (SymbolGetter)gi_object_info_get_get_value_function_name);
}

void
gi_object_info_class_init (gpointer g_class,
                           gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_OBJECT;
}
