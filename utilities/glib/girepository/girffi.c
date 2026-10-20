/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Helper functions for ffi integration
 *
 * Copyright (C) 2008 Red Hat, Inc
 * Copyright (C) 2005 Matthias Clasen
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

#include <sys/types.h>

#include <errno.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "girffi.h"
#include "gibaseinfo-private.h"
#include "girepository.h"
#include "girepository-private.h"

static ffi_type *
gi_type_tag_get_ffi_type_internal (GITypeTag   tag,
                                   gboolean    is_pointer,
                                   gboolean    is_enum)
{
  switch (tag)
    {
    case GI_TYPE_TAG_BOOLEAN:
      return &ffi_type_uint;
    case GI_TYPE_TAG_INT8:
      return &ffi_type_sint8;
    case GI_TYPE_TAG_UINT8:
      return &ffi_type_uint8;
    case GI_TYPE_TAG_INT16:
      return &ffi_type_sint16;
    case GI_TYPE_TAG_UINT16:
      return &ffi_type_uint16;
    case GI_TYPE_TAG_INT32:
      return &ffi_type_sint32;
    case GI_TYPE_TAG_UINT32:
    case GI_TYPE_TAG_UNICHAR:
      return &ffi_type_uint32;
    case GI_TYPE_TAG_INT64:
      return &ffi_type_sint64;
    case GI_TYPE_TAG_UINT64:
      return &ffi_type_uint64;
    case GI_TYPE_TAG_GTYPE:
#if GLIB_SIZEOF_SIZE_T == 4
      return &ffi_type_uint32;
#elif GLIB_SIZEOF_SIZE_T == 8
      return &ffi_type_uint64;
#else
#  error "Unexpected size for size_t: not 4 or 8"
#endif
    case GI_TYPE_TAG_FLOAT:
      return &ffi_type_float;
    case GI_TYPE_TAG_DOUBLE:
      return &ffi_type_double;
    case GI_TYPE_TAG_UTF8:
    case GI_TYPE_TAG_FILENAME:
    case GI_TYPE_TAG_ARRAY:
    case GI_TYPE_TAG_GLIST:
    case GI_TYPE_TAG_GSLIST:
    case GI_TYPE_TAG_GHASH:
    case GI_TYPE_TAG_ERROR:
      return &ffi_type_pointer;
    case GI_TYPE_TAG_INTERFACE:
      {
        /* We need to handle enums specially:
         * https://bugzilla.gnome.org/show_bug.cgi?id=665150
         */
        if (!is_enum)
          return &ffi_type_pointer;
        else
          return &ffi_type_sint32;
      }
    case GI_TYPE_TAG_VOID:
      if (is_pointer)
        return &ffi_type_pointer;
      else
        return &ffi_type_void;
    default:
      break;
    }

  g_assert_not_reached ();

  return NULL;
}

/**
 * gi_type_tag_get_ffi_type:
 * @type_tag: a #GITypeTag
 * @is_pointer: whether this is a pointer type
 *
 * Get the `ffi_type` corresponding to @type_tag.
 *
 * Returns: (transfer none): an `ffi_type` corresponding to the platform default
 *   C ABI for @tag and @is_pointer.
 * Since: 2.80
 */
ffi_type *
gi_type_tag_get_ffi_type (GITypeTag   type_tag,
                          gboolean    is_pointer)
{
  return gi_type_tag_get_ffi_type_internal (type_tag, is_pointer, FALSE);
}

/**
 * gi_type_info_get_ffi_type:
 * @info: a #GITypeInfo
 *
 * Get the `ffi_type` corresponding to @info.
 *
 * Returns: (transfer none): a `ffi_type` corresponding to the platform default
 *   C ABI for @info.
 * Since: 2.80
 */
ffi_type *
gi_type_info_get_ffi_type (GITypeInfo *info)
{
  gboolean is_enum = FALSE;
  GIBaseInfo *iinfo;

  if (gi_type_info_get_tag (info) == GI_TYPE_TAG_INTERFACE)
    {
      iinfo = gi_type_info_get_interface (info);
      switch (gi_base_info_get_info_type (iinfo))
        {
        case GI_INFO_TYPE_ENUM:
        case GI_INFO_TYPE_FLAGS:
          is_enum = TRUE;
          break;
        default:
          break;
        }
      gi_base_info_unref (iinfo);
    }

  return gi_type_tag_get_ffi_type_internal (gi_type_info_get_tag (info), gi_type_info_is_pointer (info), is_enum);
}

/**
 * gi_callable_info_get_ffi_arg_types:
 * @callable_info: a callable info from a typelib
 * @n_args_p: (out) (optional): the number of arguments returned
 *
 * Get the `ffi_type`s for the arguments of @callable_info.
 *
 * Returns: (transfer container) (array length=n_args_p): an array of
 *   `ffi_type*`. The array itself should be freed using [func@GLib.free] after
 *   use.
 * Since: 2.80
 */
static ffi_type **
gi_callable_info_get_ffi_arg_types (GICallableInfo *callable_info,
                                    size_t         *n_args_p)
{
    ffi_type **arg_types;
    gboolean is_method, throws;
    size_t n_args, n_invoke_args, i, offset;

    g_return_val_if_fail (callable_info != NULL, NULL);

    n_args = gi_callable_info_get_n_args (callable_info);
    is_method = gi_callable_info_is_method (callable_info);
    throws = gi_callable_info_can_throw_gerror (callable_info);
    offset = is_method ? 1 : 0;

    n_invoke_args = n_args;

    if (is_method)
      n_invoke_args++;
    if (throws)
      n_invoke_args++;

    if (n_args_p)
      *n_args_p = n_invoke_args;

    arg_types = (ffi_type **) g_new0 (ffi_type *, n_invoke_args + 1);

    if (is_method)
      arg_types[0] = &ffi_type_pointer;
    if (throws)
      arg_types[n_invoke_args - 1] = &ffi_type_pointer;

    for (i = 0; i < n_args; ++i)
      {
        GIArgInfo arg_info;
        GITypeInfo arg_type;

        gi_callable_info_load_arg (callable_info, i, &arg_info);
        gi_arg_info_load_type_info (&arg_info, &arg_type);
        switch (gi_arg_info_get_direction (&arg_info))
          {
            case GI_DIRECTION_IN:
              arg_types[i + offset] = gi_type_info_get_ffi_type (&arg_type);
              break;
            case GI_DIRECTION_OUT:
            case GI_DIRECTION_INOUT:
              arg_types[i + offset] = &ffi_type_pointer;
              break;
            default:
              g_assert_not_reached ();
          }

        gi_base_info_clear (&arg_type);
        gi_base_info_clear (&arg_info);
      }

    arg_types[n_invoke_args] = NULL;

    return arg_types;
}

/**
 * gi_callable_info_get_ffi_return_type:
 * @callable_info: a callable info from a typelib
 *
 * Fetches the `ffi_type` for a corresponding return value of
 * a [class@GIRepository.CallableInfo].
 *
 * Returns: (transfer none): the `ffi_type` for the return value
 * Since: 2.80
 */
static ffi_type *
gi_callable_info_get_ffi_return_type (GICallableInfo *callable_info)
{
  GITypeInfo *return_type;
  ffi_type *return_ffi_type;

  g_return_val_if_fail (callable_info != NULL, NULL);

  return_type = gi_callable_info_get_return_type (callable_info);
  return_ffi_type = gi_type_info_get_ffi_type (return_type);
  gi_base_info_unref((GIBaseInfo*)return_type);

  return return_ffi_type;
}

/**
 * gi_function_info_prep_invoker:
 * @info: A #GIFunctionInfo
 * @invoker: (out caller-allocates): Output invoker structure
 * @error: A #GError
 *
 * Initialize the caller-allocated @invoker structure with a cache
 * of information needed to invoke the C function corresponding to
 * @info with the platform’s default ABI.
 *
 * A primary intent of this function is that a dynamic structure allocated
 * by a language binding could contain a [type@GIRepository.FunctionInvoker]
 * structure inside the binding’s function mapping.
 *
 * @invoker must be freed using [method@GIRepository.FunctionInvoker.clear]
 * when it’s finished with.
 *
 * Returns: `TRUE` on success, `FALSE` otherwise with @error set.
 * Since: 2.80
 */
gboolean
gi_function_info_prep_invoker (GIFunctionInfo     *info,
                               GIFunctionInvoker  *invoker,
                               GError            **error)
{
  const char *symbol;
  void *addr;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (invoker != NULL, FALSE);

  symbol = gi_function_info_get_symbol ((GIFunctionInfo*) info);

  if (!gi_typelib_symbol (gi_base_info_get_typelib ((GIBaseInfo *) info),
                          symbol, &addr))
    {
      g_set_error (error,
                   GI_INVOKE_ERROR,
                   GI_INVOKE_ERROR_SYMBOL_NOT_FOUND,
                   "Could not locate %s: %s", symbol, g_module_error ());

      return FALSE;
    }

  return gi_function_invoker_new_for_address (addr, (GICallableInfo *) info, invoker, error);
}

/**
 * gi_function_invoker_new_for_address:
 * @addr: The address
 * @info: A #GICallableInfo
 * @invoker: (out caller-allocates): Output invoker structure
 * @error: A #GError
 *
 * Initialize the caller-allocated @invoker structure with a cache
 * of information needed to invoke the C function corresponding to
 * @info with the platform’s default ABI.
 *
 * A primary intent of this function is that a dynamic structure allocated
 * by a language binding could contain a [type@GIRepository.FunctionInvoker]
 * structure inside the binding’s function mapping.
 *
 * Returns: `TRUE` on success, `FALSE` otherwise with @error set.
 * Since: 2.80
 */
gboolean
gi_function_invoker_new_for_address (void               *addr,
                                     GICallableInfo     *info,
                                     GIFunctionInvoker  *invoker,
                                     GError            **error)
{
  ffi_type **atypes;
  size_t n_args;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (invoker != NULL, FALSE);

  invoker->native_address = addr;

  atypes = gi_callable_info_get_ffi_arg_types (info, &n_args);

  return ffi_prep_cif (&(invoker->cif), FFI_DEFAULT_ABI, n_args,
                       gi_callable_info_get_ffi_return_type (info),
                       g_steal_pointer (&atypes)) == FFI_OK;
}

/**
 * gi_function_invoker_clear:
 * @invoker: (transfer none): A #GIFunctionInvoker
 *
 * Release all resources allocated for the internals of @invoker.
 *
 * Callers are responsible for freeing any resources allocated for the structure
 * itself however.
 *
 * Since: 2.80
 */
void
gi_function_invoker_clear (GIFunctionInvoker *invoker)
{
  g_free (invoker->cif.arg_types);
}

typedef struct {
  ffi_closure ffi_closure;
  void *writable_self;
  void *native_address;
} GIClosureWrapper;

/**
 * gi_callable_info_create_closure:
 * @callable_info: a callable info from a typelib
 * @cif: a `ffi_cif` structure
 * @callback: the ffi callback
 * @user_data: data to be passed into the callback
 *
 * Prepares a callback for ffi invocation.
 *
 * Returns: (transfer full) (nullable): the `ffi_closure`, or `NULL` on error.
 *   The return value should be freed by calling
 *   [method@GIRepository.CallableInfo.destroy_closure].
 * Since: 2.80
 */
ffi_closure *
gi_callable_info_create_closure (GICallableInfo       *callable_info,
                                 ffi_cif              *cif,
                                 GIFFIClosureCallback  callback,
                                 void                 *user_data)
{
  void *exec_ptr;
  size_t n_args;
  ffi_type **atypes;
  GIClosureWrapper *closure;
  ffi_status status;

  g_return_val_if_fail (callable_info != NULL, FALSE);
  g_return_val_if_fail (cif != NULL, FALSE);
  g_return_val_if_fail (callback != NULL, FALSE);

  closure = ffi_closure_alloc (sizeof (GIClosureWrapper), &exec_ptr);
  if (!closure)
    {
      g_warning ("could not allocate closure");
      return NULL;
    }
  closure->writable_self = closure;
  closure->native_address = exec_ptr;


  atypes = gi_callable_info_get_ffi_arg_types (callable_info, &n_args);
  status = ffi_prep_cif (cif, FFI_DEFAULT_ABI, n_args,
                         gi_callable_info_get_ffi_return_type (callable_info),
                         atypes);

  /* Explicitly store atypes to satisfy static analysers, which can’t see inside
   * ffi_prep_cif(), and hence assume that it’s leaked. */
  cif->arg_types = g_steal_pointer (&atypes);

  if (status != FFI_OK)
    {
      g_warning ("ffi_prep_cif failed: %d", status);
      gi_callable_info_destroy_closure (callable_info, &closure->ffi_closure);
      return NULL;
    }

  status = ffi_prep_closure_loc (&closure->ffi_closure, cif, callback, user_data, exec_ptr);
  if (status != FFI_OK)
    {
      g_warning ("ffi_prep_closure failed: %d", status);
      gi_callable_info_destroy_closure (callable_info, &closure->ffi_closure);
      return NULL;
    }

  return &closure->ffi_closure;
}

/**
 * gi_callable_info_get_closure_native_address:
 * @callable_info: a callable info from a typelib
 * @closure: ffi closure
 *
 * Gets a function pointer from `closure`, which must have been prepared by
 * [method@GIRepository.CallableInfo.create_closure].
 * This function pointer is the address of the closure in executable memory.
 * Before calling it, it must be cast to the correct function pointer type,
 * matching the C type of `callable_info`.
 * Calling this function pointer will cause the `callback` passed to
 * [method@GIRepository.CallableInfo.create_closure] to be called with the
 * appropriate arguments.
 *
 * Returns: (transfer none): a function pointer
 * Since: 2.80
 */
void *
gi_callable_info_get_closure_native_address (GICallableInfo *callable_info,
                                             ffi_closure *closure)
{
  GIClosureWrapper *wrapper = (GIClosureWrapper *)closure;
  return wrapper->native_address;
}

/**
 * gi_callable_info_destroy_closure:
 * @callable_info: a callable info from a typelib
 * @closure: (transfer full): ffi closure
 *
 * Frees a `ffi_closure` returned from
 * [method@GIRepository.CallableInfo.create_closure].
 *
 * Since: 2.80
 */
void
gi_callable_info_destroy_closure (GICallableInfo *callable_info,
                                  ffi_closure    *closure)
{
  GIClosureWrapper *wrapper = (GIClosureWrapper *)closure;

  g_free (wrapper->ffi_closure.cif->arg_types);
  ffi_closure_free (wrapper->writable_self);
}
