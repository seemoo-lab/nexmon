/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Interface implementation
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
#include "giinterfaceinfo.h"

/**
 * GIInterfaceInfo:
 *
 * `GIInterfaceInfo` represents a `GInterface` type.
 *
 * A `GInterface` has methods, fields, properties, signals,
 * interfaces, constants, virtual functions and prerequisites.
 *
 * Since: 2.80
 */

/**
 * gi_interface_info_get_n_prerequisites:
 * @info: a #GIInterfaceInfo
 *
 * Obtain the number of prerequisites for this interface type.
 *
 * A prerequisite is another interface that needs to be implemented for
 * interface, similar to a base class for [class@GObject.Object]s.
 *
 * Returns: number of prerequisites
 * Since: 2.80
 */
unsigned int
gi_interface_info_get_n_prerequisites (GIInterfaceInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), 0);

  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_prerequisites;
}

/**
 * gi_interface_info_get_prerequisite:
 * @info: a #GIInterfaceInfo
 * @n: index of prerequisite to get
 *
 * Obtain an interface type’s prerequisite at index @n.
 *
 * Returns: (transfer full): The prerequisite as a [class@GIRepository.BaseInfo].
 *   Free the struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIBaseInfo *
gi_interface_info_get_prerequisite (GIInterfaceInfo *info,
                                    unsigned int     n)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  return gi_info_from_entry (rinfo->repository,
                             rinfo->typelib, blob->prerequisites[n]);
}


/**
 * gi_interface_info_get_n_properties:
 * @info: a #GIInterfaceInfo
 *
 * Obtain the number of properties that this interface type has.
 *
 * Returns: number of properties
 * Since: 2.80
 */
unsigned int
gi_interface_info_get_n_properties (GIInterfaceInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), 0);

  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_properties;
}

/**
 * gi_interface_info_get_property:
 * @info: a #GIInterfaceInfo
 * @n: index of property to get
 *
 * Obtain an interface type property at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.PropertyInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIPropertyInfo *
gi_interface_info_get_property (GIInterfaceInfo *info,
                                unsigned int     n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2u)) * 2u
    + n * header->property_blob_size;

  return (GIPropertyInfo *) gi_base_info_new (GI_INFO_TYPE_PROPERTY, (GIBaseInfo*)info,
                                              rinfo->typelib, offset);
}

/**
 * gi_interface_info_get_n_methods:
 * @info: a #GIInterfaceInfo
 *
 * Obtain the number of methods that this interface type has.
 *
 * Returns: number of methods
 * Since: 2.80
 */
unsigned int
gi_interface_info_get_n_methods (GIInterfaceInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), 0);

  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_methods;
}

/**
 * gi_interface_info_get_method:
 * @info: a #GIInterfaceInfo
 * @n: index of method to get
 *
 * Obtain an interface type method at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.FunctionInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_interface_info_get_method (GIInterfaceInfo *info,
                              unsigned int     n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2u)) * 2u
    + blob->n_properties * header->property_blob_size
    + n * header->function_blob_size;

  return (GIFunctionInfo *) gi_base_info_new (GI_INFO_TYPE_FUNCTION, (GIBaseInfo*)info,
                                              rinfo->typelib, offset);
}

/**
 * gi_interface_info_find_method:
 * @info: a #GIInterfaceInfo
 * @name: name of method to obtain
 *
 * Obtain a method of the interface type given a @name.
 *
 * `NULL` will be returned if there’s no method available with that name.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.FunctionInfo] or
 *   `NULL` if none found. Free the struct by calling
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_interface_info_find_method (GIInterfaceInfo *info,
                               const char      *name)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header = (Header *)rinfo->typelib->data;
  InterfaceBlob *blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2u)) * 2u
    + blob->n_properties * header->property_blob_size;

  return gi_base_info_find_method ((GIBaseInfo*)info, offset, blob->n_methods, name);
}

/**
 * gi_interface_info_get_n_signals:
 * @info: a #GIInterfaceInfo
 *
 * Obtain the number of signals that this interface type has.
 *
 * Returns: number of signals
 * Since: 2.80
 */
unsigned int
gi_interface_info_get_n_signals (GIInterfaceInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), 0);

  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_signals;
}

/**
 * gi_interface_info_get_signal:
 * @info: a #GIInterfaceInfo
 * @n: index of signal to get
 *
 * Obtain an interface type signal at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.SignalInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GISignalInfo *
gi_interface_info_get_signal (GIInterfaceInfo *info,
                              unsigned int     n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2u)) * 2u
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size
    + n * header->signal_blob_size;

  return (GISignalInfo *) gi_base_info_new (GI_INFO_TYPE_SIGNAL, (GIBaseInfo*)info,
                                            rinfo->typelib, offset);
}

/**
 * gi_interface_info_find_signal:
 * @info: a #GIInterfaceInfo
 * @name: name of signal to find
 *
 * Obtain a signal of the interface type given a @name.
 *
 * `NULL` will be returned if there’s no signal available with that name.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.SignalInfo] or
 *   `NULL` if none found. Free the struct by calling
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GISignalInfo *
gi_interface_info_find_signal (GIInterfaceInfo *info,
                               const char      *name)
{
  uint32_t n_signals;

  n_signals = gi_interface_info_get_n_signals (info);
  for (uint32_t i = 0; i < n_signals; i++)
    {
      GISignalInfo *siginfo = gi_interface_info_get_signal (info, i);

      if (g_strcmp0 (gi_base_info_get_name ((GIBaseInfo *) siginfo), name) != 0)
        {
          gi_base_info_unref ((GIBaseInfo*)siginfo);
          continue;
        }

      return siginfo;
    }
  return NULL;
}

/**
 * gi_interface_info_get_n_vfuncs:
 * @info: a #GIInterfaceInfo
 *
 * Obtain the number of virtual functions that this interface type has.
 *
 * Returns: number of virtual functions
 * Since: 2.80
 */
unsigned int
gi_interface_info_get_n_vfuncs (GIInterfaceInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), 0);

  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_vfuncs;
}

/**
 * gi_interface_info_get_vfunc:
 * @info: a #GIInterfaceInfo
 * @n: index of virtual function to get
 *
 * Obtain an interface type virtual function at index @n.
 *
 * Returns: (transfer full): the [class@GIRepository.VFuncInfo]. Free the struct
 *   by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIVFuncInfo *
gi_interface_info_get_vfunc (GIInterfaceInfo *info,
                             unsigned int     n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2u)) * 2u
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size
    + blob->n_signals * header->signal_blob_size
    + n * header->vfunc_blob_size;

  return (GIVFuncInfo *) gi_base_info_new (GI_INFO_TYPE_VFUNC, (GIBaseInfo*)info,
                                           rinfo->typelib, offset);
}

/**
 * gi_interface_info_find_vfunc:
 * @info: a #GIInterfaceInfo
 * @name: The name of a virtual function to find.
 *
 * Locate a virtual function slot with name @name.
 *
 * See the documentation for [method@GIRepository.ObjectInfo.find_vfunc] for
 * more information on virtuals.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.VFuncInfo], or
 *   `NULL` if none found. Free it with [method@GIRepository.BaseInfo.unref]
 *   when done.
 * Since: 2.80
 */
GIVFuncInfo *
gi_interface_info_find_vfunc (GIInterfaceInfo *info,
                              const char      *name)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->interface_blob_size
    + (blob->n_prerequisites + blob->n_prerequisites % 2u) * 2u
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size
    + blob->n_signals * header->signal_blob_size;

  return gi_base_info_find_vfunc (rinfo, offset, blob->n_vfuncs, name);
}

/**
 * gi_interface_info_get_n_constants:
 * @info: a #GIInterfaceInfo
 *
 * Obtain the number of constants that this interface type has.
 *
 * Returns: number of constants
 * Since: 2.80
 */
unsigned int
gi_interface_info_get_n_constants (GIInterfaceInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), 0);

  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_constants;
}

/**
 * gi_interface_info_get_constant:
 * @info: a #GIInterfaceInfo
 * @n: index of constant to get
 *
 * Obtain an interface type constant at index @n.
 *
 * Returns: (transfer full): The [class@GIRepository.ConstantInfo]. Free the
 *   struct by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIConstantInfo *
gi_interface_info_get_constant (GIInterfaceInfo *info,
                                unsigned int     n)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  header = (Header *)rinfo->typelib->data;
  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->interface_blob_size
    + (blob->n_prerequisites + (blob->n_prerequisites % 2u)) * 2u
    + blob->n_properties * header->property_blob_size
    + blob->n_methods * header->function_blob_size
    + blob->n_signals * header->signal_blob_size
    + blob->n_vfuncs * header->vfunc_blob_size
    + n * header->constant_blob_size;

  return (GIConstantInfo *) gi_base_info_new (GI_INFO_TYPE_CONSTANT, (GIBaseInfo*)info,
                                              rinfo->typelib, offset);
}

/**
 * gi_interface_info_get_iface_struct:
 * @info: a #GIInterfaceInfo
 *
 * Returns the layout C structure associated with this `GInterface`.
 *
 * Returns: (transfer full) (nullable): The [class@GIRepository.StructInfo] or
 *   `NULL` if unknown. Free it with [method@GIRepository.BaseInfo.unref] when
 *   done.
 * Since: 2.80
 */
GIStructInfo *
gi_interface_info_get_iface_struct (GIInterfaceInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  InterfaceBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_INTERFACE_INFO (info), NULL);

  blob = (InterfaceBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->gtype_struct)
    return (GIStructInfo *) gi_info_from_entry (rinfo->repository,
                                                rinfo->typelib, blob->gtype_struct);
  else
    return NULL;
}

void
gi_interface_info_class_init (gpointer g_class,
                              gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_INTERFACE;
}
