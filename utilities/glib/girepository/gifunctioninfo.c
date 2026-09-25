/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Function implementation
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
#include "gifunctioninfo.h"

/**
 * GIFunctionInfo:
 *
 * `GIFunctionInfo` represents a function, method or constructor.
 *
 * To find out what kind of entity a `GIFunctionInfo` represents, call
 * [method@GIRepository.FunctionInfo.get_flags].
 *
 * See also [class@GIRepository.CallableInfo] for information on how to retrieve
 * arguments and other metadata.
 *
 * Since: 2.80
 */

GIFunctionInfo *
gi_base_info_find_method (GIBaseInfo  *base,
                          uint32_t     offset,
                          uint16_t     n_methods,
                          const char  *name)
{
  /* FIXME hash */
  GIRealInfo *rinfo = (GIRealInfo*)base;
  Header *header = (Header *)rinfo->typelib->data;

  for (uint16_t i = 0; i < n_methods; i++)
    {
      FunctionBlob *fblob = (FunctionBlob *)&rinfo->typelib->data[offset];
      const char *fname = (const char *)&rinfo->typelib->data[fblob->name];

      if (strcmp (name, fname) == 0)
        return (GIFunctionInfo *) gi_base_info_new (GI_INFO_TYPE_FUNCTION, base,
                                                    rinfo->typelib, offset);

      offset += header->function_blob_size;
    }

  return NULL;
}

/**
 * gi_function_info_get_symbol:
 * @info: a #GIFunctionInfo
 *
 * Obtain the symbol of the function.
 *
 * The symbol is the name of the exported function, suitable to be used as an
 * argument to [method@GModule.Module.symbol].
 *
 * Returns: the symbol
 * Since: 2.80
 */
const char *
gi_function_info_get_symbol (GIFunctionInfo *info)
{
  GIRealInfo *rinfo;
  FunctionBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_FUNCTION_INFO (info), NULL);

  rinfo = (GIRealInfo *)info;
  blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];

  return gi_typelib_get_string (rinfo->typelib, blob->symbol);
}

/**
 * gi_function_info_get_flags:
 * @info: a #GIFunctionInfo
 *
 * Obtain the [type@GIRepository.FunctionInfoFlags] for the @info.
 *
 * Returns: the flags
 * Since: 2.80
 */
GIFunctionInfoFlags
gi_function_info_get_flags (GIFunctionInfo *info)
{
  GIFunctionInfoFlags flags;
  GIRealInfo *rinfo;
  FunctionBlob *blob;

  g_return_val_if_fail (info != NULL, GI_FUNCTION_INFO_FLAGS_NONE);
  g_return_val_if_fail (GI_IS_FUNCTION_INFO (info), GI_FUNCTION_INFO_FLAGS_NONE);

  rinfo = (GIRealInfo *)info;
  blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];

  flags = 0;

  /* Make sure we don't flag Constructors as methods */
  if (!blob->constructor && !blob->is_static)
    flags = flags | GI_FUNCTION_IS_METHOD;

  if (blob->constructor)
    flags = flags | GI_FUNCTION_IS_CONSTRUCTOR;

  if (blob->getter)
    flags = flags | GI_FUNCTION_IS_GETTER;

  if (blob->setter)
    flags = flags | GI_FUNCTION_IS_SETTER;

  if (blob->wraps_vfunc)
    flags = flags | GI_FUNCTION_WRAPS_VFUNC;

  if (blob->is_async)
    flags = flags | GI_FUNCTION_IS_ASYNC;

  return flags;
}

/**
 * gi_function_info_get_property:
 * @info: a #GIFunctionInfo
 *
 * Obtain the property associated with this `GIFunctionInfo`.
 *
 * Only `GIFunctionInfo`s with the flag `GI_FUNCTION_IS_GETTER` or
 * `GI_FUNCTION_IS_SETTER` have a property set. For other cases,
 * `NULL` will be returned.
 *
 * Returns: (transfer full) (nullable): The property or `NULL` if not set. Free
 *   it with [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIPropertyInfo *
gi_function_info_get_property (GIFunctionInfo *info)
{
  GIRealInfo *rinfo;
  FunctionBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_FUNCTION_INFO (info), NULL);

  rinfo = (GIRealInfo *)info;
  blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];

  if (gi_base_info_get_info_type ((GIBaseInfo *) rinfo->container) == GI_INFO_TYPE_INTERFACE)
    {
      GIInterfaceInfo *container = (GIInterfaceInfo *)rinfo->container;

      return gi_interface_info_get_property (container, blob->index);
    }
  else if (gi_base_info_get_info_type ((GIBaseInfo *) rinfo->container) == GI_INFO_TYPE_OBJECT)
    {
      GIObjectInfo *container = (GIObjectInfo *)rinfo->container;

      return gi_object_info_get_property (container, blob->index);
    }
  else
    return NULL;
}

/**
 * gi_function_info_get_vfunc:
 * @info: a #GIFunctionInfo
 *
 * Obtain the virtual function associated with this `GIFunctionInfo`.
 *
 * Only `GIFunctionInfo`s with the flag `GI_FUNCTION_WRAPS_VFUNC` have
 * a virtual function set. For other cases, `NULL` will be returned.
 *
 * Returns: (transfer full) (nullable): The virtual function or `NULL` if not
 *   set. Free it by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIVFuncInfo *
gi_function_info_get_vfunc (GIFunctionInfo *info)
{
  GIRealInfo *rinfo;
  FunctionBlob *blob;
  GIInterfaceInfo *container;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_FUNCTION_INFO (info), NULL);

  rinfo = (GIRealInfo *)info;
  blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];
  container = (GIInterfaceInfo *)rinfo->container;

  return gi_interface_info_get_vfunc (container, blob->index);
}

/**
 * gi_invoke_error_quark:
 *
 * Get the error quark which represents [type@GIRepository.InvokeError].
 *
 * Returns: error quark
 * Since: 2.80
 */
GQuark
gi_invoke_error_quark (void)
{
  static GQuark quark = 0;
  if (quark == 0)
    quark = g_quark_from_static_string ("gi-invoke-error-quark");
  return quark;
}

/**
 * gi_function_info_invoke: (skip)
 * @info: a #GIFunctionInfo describing the function to invoke
 * @in_args: (array length=n_in_args) (nullable): An array of
 *   [type@GIRepository.Argument]s, one for each ‘in’ parameter of @info. If
 *   there are no ‘in’ parameters, @in_args can be `NULL`.
 * @n_in_args: the length of the @in_args array
 * @out_args: (array length=n_out_args) (nullable): An array of
 *   [type@GIRepository.Argument]s, one for each ‘out’ parameter of @info. If
 *   there are no ‘out’ parameters, @out_args may be `NULL`.
 * @n_out_args: the length of the @out_args array
 * @return_value: (out caller-allocates) (not optional): return location for the
 *   return value of the function.
 * @error: return location for detailed error information, or `NULL`
 *
 * Invokes the function described in @info with the given
 * arguments.
 *
 * Note that ‘inout’ parameters must appear in both argument lists. This
 * function uses [`dlsym()`](man:dlsym(3)) to obtain a pointer to the function,
 * so the library or shared object containing the described function must either
 * be linked to the caller, or must have been loaded with
 * [method@GModule.Module.symbol] before calling this function.
 *
 * Returns: `TRUE` if the function has been invoked, `FALSE` if an
 *   error occurred.
 * Since: 2.80
 */
gboolean
gi_function_info_invoke (GIFunctionInfo    *info,
                         const GIArgument  *in_args,
                         size_t             n_in_args,
                         GIArgument        *out_args,
                         size_t             n_out_args,
                         GIArgument        *return_value,
                         GError           **error)
{
  const char *symbol;
  void *func;

  symbol = gi_function_info_get_symbol (info);

  if (!gi_typelib_symbol (gi_base_info_get_typelib ((GIBaseInfo *) info),
                          symbol, &func))
    {
      g_set_error (error,
                   GI_INVOKE_ERROR,
                   GI_INVOKE_ERROR_SYMBOL_NOT_FOUND,
                   "Could not locate %s: %s", symbol, g_module_error ());

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
gi_function_info_class_init (gpointer g_class,
                             gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_FUNCTION;
}
