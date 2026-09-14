/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Virtual Function implementation
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

#include <string.h>

#include <glib.h>

#include <girepository/girepository.h>
#include "gibaseinfo-private.h"
#include "girepository-private.h"
#include "gitypelib-internal.h"
#include "givfuncinfo.h"

/**
 * GIVFuncInfo:
 *
 * `GIVFuncInfo` represents a virtual function.
 *
 * A virtual function is a callable object that belongs to either a
 * [type@GIRepository.ObjectInfo] or a [type@GIRepository.InterfaceInfo].
 *
 * Since: 2.80
 */

GIVFuncInfo *
gi_base_info_find_vfunc (GIRealInfo  *rinfo,
                         uint32_t     offset,
                         uint16_t     n_vfuncs,
                         const char  *name)
{
  /* FIXME hash */
  Header *header = (Header *)rinfo->typelib->data;

  for (uint16_t i = 0; i < n_vfuncs; i++)
    {
      VFuncBlob *fblob = (VFuncBlob *)&rinfo->typelib->data[offset];
      const char *fname = (const char *)&rinfo->typelib->data[fblob->name];

      if (strcmp (name, fname) == 0)
        return (GIVFuncInfo *) gi_base_info_new (GI_INFO_TYPE_VFUNC, (GIBaseInfo*) rinfo,
                                                 rinfo->typelib, offset);

      offset += header->vfunc_blob_size;
    }

  return NULL;
}

/**
 * gi_vfunc_info_get_flags:
 * @info: a #GIVFuncInfo
 *
 * Obtain the flags for this virtual function info.
 *
 * See [flags@GIRepository.VFuncInfoFlags] for more information about possible
 * flag values.
 *
 * Returns: the flags
 * Since: 2.80
 */
GIVFuncInfoFlags
gi_vfunc_info_get_flags (GIVFuncInfo *info)
{
  GIVFuncInfoFlags flags;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  VFuncBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_VFUNC_INFO (info), 0);

  blob = (VFuncBlob *)&rinfo->typelib->data[rinfo->offset];

  flags = 0;

  if (blob->must_chain_up)
    flags = flags | GI_VFUNC_MUST_CHAIN_UP;

  if (blob->must_be_implemented)
    flags = flags | GI_VFUNC_MUST_OVERRIDE;

  if (blob->must_not_be_implemented)
    flags = flags | GI_VFUNC_MUST_NOT_OVERRIDE;

  return flags;
}

/**
 * gi_vfunc_info_get_offset:
 * @info: a #GIVFuncInfo
 *
 * Obtain the offset of the function pointer in the class struct.
 *
 * The value `0xFFFF` indicates that the struct offset is unknown.
 *
 * Returns: the struct offset or `0xFFFF` if it’s unknown
 * Since: 2.80
 */
size_t
gi_vfunc_info_get_offset (GIVFuncInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  VFuncBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_VFUNC_INFO (info), 0);

  blob = (VFuncBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->struct_offset;
}

/**
 * gi_vfunc_info_get_signal:
 * @info: a #GIVFuncInfo
 *
 * Obtain the signal for the virtual function if one is set.
 *
 * The signal comes from the object or interface to which
 * this virtual function belongs.
 *
 * Returns: (transfer full) (nullable): the signal, or `NULL` if none is set
 * Since: 2.80
 */
GISignalInfo *
gi_vfunc_info_get_signal (GIVFuncInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  VFuncBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_VFUNC_INFO (info), 0);

  blob = (VFuncBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->class_closure)
    return gi_interface_info_get_signal ((GIInterfaceInfo *)rinfo->container, blob->signal);

  return NULL;
}

/**
 * gi_vfunc_info_get_invoker:
 * @info: a #GIVFuncInfo
 *
 * If this virtual function has an associated invoker method, this
 * method will return it.  An invoker method is a C entry point.
 *
 * Not all virtuals will have invokers.
 *
 * Returns: (transfer full) (nullable): The [type@GIRepository.FunctionInfo] or
 *   `NULL` if none is set. Free it with [method@GIRepository.BaseInfo.unref]
 *   when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_vfunc_info_get_invoker (GIVFuncInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  VFuncBlob *blob;
  GIBaseInfo *container;
  GIInfoType parent_type;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_VFUNC_INFO (info), 0);

  blob = (VFuncBlob *)&rinfo->typelib->data[rinfo->offset];

  /* 1023 = 0x3ff is the maximum of the 10 bits for invoker index */
  if (blob->invoker == 1023)
    return NULL;

  container = rinfo->container;
  parent_type = gi_base_info_get_info_type (container);
  if (parent_type == GI_INFO_TYPE_OBJECT)
    return gi_object_info_get_method ((GIObjectInfo*)container, blob->invoker);
  else if (parent_type == GI_INFO_TYPE_INTERFACE)
    return gi_interface_info_get_method ((GIInterfaceInfo*)container, blob->invoker);
  else
    g_assert_not_reached ();
}

/**
 * gi_vfunc_info_get_address:
 * @info: a #GIVFuncInfo
 * @implementor_gtype: [type@GObject.Type] implementing this virtual function
 * @error: return location for a [type@GLib.Error], or `NULL`
 *
 * Looks up where the implementation for @info is inside the type struct of
 * @implementor_gtype.
 *
 * Returns: address to a function
 * Since: 2.80
 */
void *
gi_vfunc_info_get_address (GIVFuncInfo  *vfunc_info,
                           GType         implementor_gtype,
                           GError      **error)
{
  GIBaseInfo *container_info;
  GIInterfaceInfo *interface_info;
  GIObjectInfo *object_info;
  GIStructInfo *struct_info;
  GIFieldInfo *field_info = NULL;
  size_t offset;
  unsigned int length, i;
  void *implementor_class, *implementor_vtable;
  void *func = NULL;

  g_return_val_if_fail (vfunc_info != NULL, NULL);
  g_return_val_if_fail (GI_IS_VFUNC_INFO (vfunc_info), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  container_info = gi_base_info_get_container ((GIBaseInfo *) vfunc_info);
  if (gi_base_info_get_info_type (container_info) == GI_INFO_TYPE_OBJECT)
    {
      object_info = (GIObjectInfo*) container_info;
      interface_info = NULL;
      struct_info = gi_object_info_get_class_struct (object_info);
    }
  else
    {
      interface_info = (GIInterfaceInfo*) container_info;
      object_info = NULL;
      struct_info = gi_interface_info_get_iface_struct (interface_info);
    }

  length = gi_struct_info_get_n_fields (struct_info);
  for (i = 0; i < length; i++)
    {
      field_info = gi_struct_info_get_field (struct_info, i);

      if (strcmp (gi_base_info_get_name ( (GIBaseInfo*) field_info),
                  gi_base_info_get_name ( (GIBaseInfo*) vfunc_info)) != 0) {
          gi_base_info_unref ((GIBaseInfo *) field_info);
          field_info = NULL;
          continue;
      }

      break;
    }

  if (field_info == NULL)
    {
      g_set_error (error,
                   GI_INVOKE_ERROR,
                   GI_INVOKE_ERROR_SYMBOL_NOT_FOUND,
                   "Couldn't find struct field for this vfunc");
      goto out;
    }

  implementor_class = g_type_class_ref (implementor_gtype);

  if (object_info)
    {
      implementor_vtable = implementor_class;
    }
  else
    {
      GType interface_type;

      interface_type = gi_registered_type_info_get_g_type ((GIRegisteredTypeInfo*) interface_info);
      implementor_vtable = g_type_interface_peek (implementor_class, interface_type);
    }

  offset = gi_field_info_get_offset (field_info);
  func = *(void**) G_STRUCT_MEMBER_P (implementor_vtable, offset);
  g_type_class_unref (implementor_class);
  gi_base_info_unref ((GIBaseInfo *) field_info);

  if (func == NULL)
    {
      g_set_error (error,
                   GI_INVOKE_ERROR,
                   GI_INVOKE_ERROR_SYMBOL_NOT_FOUND,
                   "Class %s doesn't implement %s",
                   g_type_name (implementor_gtype),
                   gi_base_info_get_name ( (GIBaseInfo*) vfunc_info));
      goto out;
    }

 out:
  gi_base_info_unref ((GIBaseInfo*) struct_info);

  return func;
}

/**
 * gi_vfunc_info_invoke: (skip)
 * @info: a #GIVFuncInfo describing the virtual function to invoke
 * @implementor: [type@GObject.Type] of the type that implements this virtual
 *   function
 * @in_args: (array length=n_in_args) (nullable): an array of
 *   [struct@GIRepository.Argument]s, one for each ‘in’ parameter of @info. If
 *   there are no ‘in’ parameters, @in_args can be `NULL`
 * @n_in_args: the length of the @in_args array
 * @out_args: (array length=n_out_args) (nullable): an array of
 *   [struct@GIRepository.Argument]s allocated by the caller, one for each
 *   ‘out’ parameter of @info. If there are no ‘out’ parameters, @out_args may
 *   be `NULL`
 * @n_out_args: the length of the @out_args array
 * @return_value: (out caller-allocates) (not optional) (nullable): return
 *   location for the return value from the vfunc; `NULL` may be returned if
 *   the vfunc returns that
 * @error: return location for detailed error information, or `NULL`
 *
 * Invokes the function described in @info with the given
 * arguments.
 *
 * Note that ‘inout’ parameters must appear in both argument lists.
 *
 * Returns: `TRUE` if the vfunc was executed successfully and didn’t throw
 *   a [type@GLib.Error]; `FALSE` if @error is set
 * Since: 2.80
 */
gboolean
gi_vfunc_info_invoke (GIVFuncInfo      *info,
                      GType             implementor,
                      const GIArgument *in_args,
                      size_t            n_in_args,
                      GIArgument       *out_args,
                      size_t            n_out_args,
                      GIArgument       *return_value,
                      GError          **error)
{
  void *func;
  GError *local_error = NULL;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_VFUNC_INFO (info), FALSE);
  g_return_val_if_fail (in_args != NULL || n_in_args == 0, FALSE);
  g_return_val_if_fail (out_args != NULL || n_out_args == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  func = gi_vfunc_info_get_address (info, implementor, &local_error);
  if (local_error != NULL)
    {
      g_propagate_error (error, g_steal_pointer (&local_error));
      return FALSE;
    }

  return gi_callable_info_invoke ((GICallableInfo*) info,
                                  func,
                                  in_args,
                                  n_in_args,
                                  out_args,
                                  n_out_args,
                                  return_value,
                                  error);
}

void
gi_vfunc_info_class_init (gpointer g_class,
                          gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_VFUNC;
}
