/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Callable implementation
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

#include <stdlib.h>

#include <glib.h>

#include <girepository/girepository.h>
#include "gibaseinfo-private.h"
#include "girepository-private.h"
#include "gitypelib-internal.h"
#include "girffi.h"
#include "gicallableinfo.h"

#define GET_BLOB(Type, rinfo) ((Type *) &rinfo->typelib->data[rinfo->offset])

/* GICallableInfo functions */

/**
 * GICallableInfo:
 *
 * `GICallableInfo` represents an entity which is callable.
 *
 * Examples of callable are:
 *
 *  - functions ([class@GIRepository.FunctionInfo])
 *  - virtual functions ([class@GIRepository.VFuncInfo])
 *  - callbacks ([class@GIRepository.CallbackInfo]).
 *
 * A callable has a list of arguments ([class@GIRepository.ArgInfo]), a return
 * type, direction and a flag which decides if it returns `NULL`.
 *
 * Since: 2.80
 */

static uint32_t
signature_offset (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo*)info;
  int sigoff = -1;

  switch (gi_base_info_get_info_type ((GIBaseInfo *) info))
    {
    case GI_INFO_TYPE_FUNCTION:
      sigoff = G_STRUCT_OFFSET (FunctionBlob, signature);
      break;
    case GI_INFO_TYPE_VFUNC:
      sigoff = G_STRUCT_OFFSET (VFuncBlob, signature);
      break;
    case GI_INFO_TYPE_CALLBACK:
      sigoff = G_STRUCT_OFFSET (CallbackBlob, signature);
      break;
    case GI_INFO_TYPE_SIGNAL:
      sigoff = G_STRUCT_OFFSET (SignalBlob, signature);
      break;
    default:
      g_assert_not_reached ();
    }
  if (sigoff >= 0)
    return *(uint32_t *)&rinfo->typelib->data[rinfo->offset + (unsigned) sigoff];
  return 0;
}

/**
 * gi_callable_info_can_throw_gerror:
 * @info: a #GICallableInfo
 *
 * Whether the callable can throw a [type@GLib.Error]
 *
 * Returns: `TRUE` if this `GICallableInfo` can throw a [type@GLib.Error]
 * Since: 2.80
 */
gboolean
gi_callable_info_can_throw_gerror (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo*)info;
  SignatureBlob *signature;

  signature = (SignatureBlob *)&rinfo->typelib->data[signature_offset (info)];
  if (signature->throws)
    return TRUE;

  /* Functions and VFuncs store "throws" in their own blobs.
   * This info was additionally added to the SignatureBlob
   * to support the other callables. For Functions and VFuncs,
   * also check their legacy flag for compatibility.
   */
  switch (gi_base_info_get_info_type ((GIBaseInfo *) info)) {
  case GI_INFO_TYPE_FUNCTION:
    {
      FunctionBlob *blob;
      blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];
      return blob->throws;
    }
  case GI_INFO_TYPE_VFUNC:
    {
      VFuncBlob *blob;
      blob = (VFuncBlob *)&rinfo->typelib->data[rinfo->offset];
      return blob->throws;
    }
  case GI_INFO_TYPE_CALLBACK:
  case GI_INFO_TYPE_SIGNAL:
    return FALSE;
  default:
    g_assert_not_reached ();
  }
}

/**
 * gi_callable_info_is_method:
 * @info: a #GICallableInfo
 *
 * Determines if the callable info is a method.
 *
 * For [class@GIRepository.SignalInfo]s, this is always true, and for
 * [class@GIRepository.CallbackInfo]s always false.
 * For [class@GIRepository.FunctionInfo]s this looks at the
 * `GI_FUNCTION_IS_METHOD` flag on the [class@GIRepository.FunctionInfo].
 * For [class@GIRepository.VFuncInfo]s this is true when the virtual function
 * has an instance parameter.
 *
 * Concretely, this function returns whether
 * [method@GIRepository.CallableInfo.get_n_args] matches the number of arguments
 * in the raw C method. For methods, there is one more C argument than is
 * exposed by introspection: the `self` or `this` object.
 *
 * Returns: `TRUE` if @info is a method, `FALSE` otherwise
 * Since: 2.80
 */
gboolean
gi_callable_info_is_method (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo*)info;
  switch (gi_base_info_get_info_type ((GIBaseInfo *) info)) {
  case GI_INFO_TYPE_FUNCTION:
    {
      FunctionBlob *blob;
      blob = (FunctionBlob *)&rinfo->typelib->data[rinfo->offset];
      return (!blob->constructor && !blob->is_static);
    }
  case GI_INFO_TYPE_VFUNC:
    {
      VFuncBlob *blob;
      blob = (VFuncBlob *) &rinfo->typelib->data[rinfo->offset];
      return !blob->is_static;
    }
  case GI_INFO_TYPE_SIGNAL:
    return TRUE;
  case GI_INFO_TYPE_CALLBACK:
    return FALSE;
  default:
    g_assert_not_reached ();
  }
}

/**
 * gi_callable_info_get_return_type:
 * @info: a #GICallableInfo
 *
 * Obtain the return type of a callable item as a [class@GIRepository.TypeInfo].
 *
 * If the callable doesn’t return anything, a [class@GIRepository.TypeInfo] of
 * type [enum@GIRepository.TypeTag.VOID] will be returned.
 *
 * Returns: (transfer full): the [class@GIRepository.TypeInfo]. Free the struct
 *   by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GITypeInfo *
gi_callable_info_get_return_type (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  uint32_t offset;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_CALLABLE_INFO (info), NULL);

  offset = signature_offset (info);

  return gi_type_info_new ((GIBaseInfo*)info, rinfo->typelib, offset);
}

/**
 * gi_callable_info_load_return_type:
 * @info: a #GICallableInfo
 * @type: (out caller-allocates): Initialized with return type of @info
 *
 * Obtain information about a return value of callable; this
 * function is a variant of [method@GIRepository.CallableInfo.get_return_type]
 * designed for stack allocation.
 *
 * The initialized @type must not be referenced after @info is deallocated.
 *
 * Once you are done with @type, it must be cleared using
 * [method@GIRepository.BaseInfo.clear].
 *
 * Since: 2.80
 */
void
gi_callable_info_load_return_type (GICallableInfo *info,
                                   GITypeInfo     *type)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  uint32_t offset;

  g_return_if_fail (info != NULL);
  g_return_if_fail (GI_IS_CALLABLE_INFO (info));

  offset = signature_offset (info);

  gi_type_info_init (type, (GIBaseInfo*)info, rinfo->typelib, offset);
}

/**
 * gi_callable_info_may_return_null:
 * @info: a #GICallableInfo
 *
 * See if a callable could return `NULL`.
 *
 * Returns: `TRUE` if callable could return `NULL`
 * Since: 2.80
 */
gboolean
gi_callable_info_may_return_null (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  SignatureBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_CALLABLE_INFO (info), FALSE);

  blob = (SignatureBlob *)&rinfo->typelib->data[signature_offset (info)];

  return blob->may_return_null;
}

/**
 * gi_callable_info_skip_return:
 * @info: a #GICallableInfo
 *
 * See if a callable’s return value is only useful in C.
 *
 * Returns: `TRUE` if return value is only useful in C.
 * Since: 2.80
 */
gboolean
gi_callable_info_skip_return (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  SignatureBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_CALLABLE_INFO (info), FALSE);

  blob = (SignatureBlob *)&rinfo->typelib->data[signature_offset (info)];

  return blob->skip_return;
}

/**
 * gi_callable_info_get_caller_owns:
 * @info: a #GICallableInfo
 *
 * See whether the caller owns the return value of this callable.
 *
 * [type@GIRepository.Transfer] contains a list of possible transfer values.
 *
 * Returns: the transfer mode for the return value of the callable
 * Since: 2.80
 */
GITransfer
gi_callable_info_get_caller_owns (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo*) info;
  SignatureBlob *blob;

  g_return_val_if_fail (info != NULL, GI_TRANSFER_NOTHING);
  g_return_val_if_fail (GI_IS_CALLABLE_INFO (info), GI_TRANSFER_NOTHING);

  blob = (SignatureBlob *)&rinfo->typelib->data[signature_offset (info)];

  if (blob->caller_owns_return_value)
    return GI_TRANSFER_EVERYTHING;
  else if (blob->caller_owns_return_container)
    return GI_TRANSFER_CONTAINER;
  else
    return GI_TRANSFER_NOTHING;
}

/**
 * gi_callable_info_get_instance_ownership_transfer:
 * @info: a #GICallableInfo
 *
 * Obtains the ownership transfer for the instance argument.
 *
 * [type@GIRepository.Transfer] contains a list of possible transfer values.
 *
 * Returns: the transfer mode of the instance argument
 * Since: 2.80
 */
GITransfer
gi_callable_info_get_instance_ownership_transfer (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo*) info;
  SignatureBlob *blob;

  g_return_val_if_fail (info != NULL, GI_TRANSFER_NOTHING);
  g_return_val_if_fail (GI_IS_CALLABLE_INFO (info), GI_TRANSFER_NOTHING);

  blob = (SignatureBlob *)&rinfo->typelib->data[signature_offset (info)];

  if (blob->instance_transfer_ownership)
    return GI_TRANSFER_EVERYTHING;
  else
    return GI_TRANSFER_NOTHING;
}

/**
 * gi_callable_info_get_n_args:
 * @info: a #GICallableInfo
 *
 * Obtain the number of arguments (both ‘in’ and ‘out’) for this callable.
 *
 * Returns: The number of arguments this callable expects.
 * Since: 2.80
 */
unsigned int
gi_callable_info_get_n_args (GICallableInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  uint32_t offset;
  SignatureBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_CALLABLE_INFO (info), 0);

  offset = signature_offset (info);
  blob = (SignatureBlob *)&rinfo->typelib->data[offset];

  return blob->n_arguments;
}

/**
 * gi_callable_info_get_arg:
 * @info: a #GICallableInfo
 * @n: the argument index to fetch
 *
 * Obtain information about a particular argument of this callable.
 *
 * Returns: (transfer full): the [class@GIRepository.ArgInfo]. Free it with
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIArgInfo *
gi_callable_info_get_arg (GICallableInfo *info,
                          unsigned int    n)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  uint32_t offset;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_CALLABLE_INFO (info), NULL);
  g_return_val_if_fail (n <= G_MAXUINT16, NULL);

  offset = signature_offset (info);
  header = (Header *)rinfo->typelib->data;

  return (GIArgInfo *) gi_base_info_new (GI_INFO_TYPE_ARG, (GIBaseInfo*)info, rinfo->typelib,
                                         offset + header->signature_blob_size + n * header->arg_blob_size);
}

/**
 * gi_callable_info_load_arg:
 * @info: a #GICallableInfo
 * @n: the argument index to fetch
 * @arg: (out caller-allocates): Initialize with argument number @n
 *
 * Obtain information about a particular argument of this callable; this
 * function is a variant of [method@GIRepository.CallableInfo.get_arg] designed
 * for stack allocation.
 *
 * The initialized @arg must not be referenced after @info is deallocated.
 *
 * Once you are done with @arg, it must be cleared using
 * [method@GIRepository.BaseInfo.clear].
 *
 * Since: 2.80
 */
void
gi_callable_info_load_arg (GICallableInfo *info,
                           unsigned int    n,
                           GIArgInfo      *arg)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header;
  uint32_t offset;

  g_return_if_fail (info != NULL);
  g_return_if_fail (GI_IS_CALLABLE_INFO (info));
  g_return_if_fail (n <= G_MAXUINT16);

  offset = signature_offset (info);
  header = (Header *)rinfo->typelib->data;

  gi_info_init ((GIRealInfo*)arg, GI_TYPE_ARG_INFO, rinfo->repository, (GIBaseInfo*)info, rinfo->typelib,
                offset + header->signature_blob_size + n * header->arg_blob_size);
}

/**
 * gi_callable_info_get_return_attribute:
 * @info: a #GICallableInfo
 * @name: a freeform string naming an attribute
 *
 * Retrieve an arbitrary attribute associated with the return value.
 *
 * Returns: (nullable): The value of the attribute, or `NULL` if no such
 *   attribute exists
 * Since: 2.80
 */
const char *
gi_callable_info_get_return_attribute (GICallableInfo *info,
                                       const char     *name)
{
  GIAttributeIter iter = GI_ATTRIBUTE_ITER_INIT;
  const char *curname, *curvalue;
  while (gi_callable_info_iterate_return_attributes (info, &iter, &curname, &curvalue))
    {
      if (g_strcmp0 (name, curname) == 0)
        return (const char*) curvalue;
    }

  return NULL;
}

/**
 * gi_callable_info_iterate_return_attributes:
 * @info: a #GICallableInfo
 * @iterator: (inout): a [type@GIRepository.AttributeIter] structure, must be
 *   initialized; see below
 * @name: (out) (transfer none): Returned name, must not be freed
 * @value: (out) (transfer none): Returned name, must not be freed
 *
 * Iterate over all attributes associated with the return value.
 *
 * The iterator structure is typically stack allocated, and must have its
 * first member initialized to `NULL`.
 *
 * Both the @name and @value should be treated as constants
 * and must not be freed.
 *
 * See [method@GIRepository.BaseInfo.iterate_attributes] for an example of how
 * to use a similar API.
 *
 * Returns: `TRUE` if there are more attributes
 * Since: 2.80
 */
gboolean
gi_callable_info_iterate_return_attributes (GICallableInfo   *info,
                                            GIAttributeIter  *iterator,
                                            const char      **name,
                                            const char      **value)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header = (Header *)rinfo->typelib->data;
  AttributeBlob *next, *after;
  uint32_t blob_offset;

  after = (AttributeBlob *) &rinfo->typelib->data[header->attributes +
                                                  header->n_attributes * header->attribute_blob_size];

  blob_offset = signature_offset (info);

  if (iterator->data != NULL)
    next = (AttributeBlob *) iterator->data;
  else
    next = _attribute_blob_find_first ((GIBaseInfo *) info, blob_offset);

  if (next == NULL || next->offset != blob_offset || next >= after)
    return FALSE;

  *name = gi_typelib_get_string (rinfo->typelib, next->name);
  *value = gi_typelib_get_string (rinfo->typelib, next->value);
  iterator->data = next + 1;

  return TRUE;
}

/**
 * gi_type_tag_extract_ffi_return_value:
 * @return_tag: [type@GIRepository.TypeTag] of the return value
 * @interface_type: [type@GObject.Type] of the underlying interface type
 * @ffi_value: pointer to [type@GIRepository.FFIReturnValue] union containing
 *   the return value from `ffi_call()`
 * @arg: (out caller-allocates): pointer to an allocated
 *   [class@GIRepository.Argument]
 *
 * Extract the correct bits from an `ffi_arg` return value into
 * [class@GIRepository.Argument].
 *
 * See: https://bugzilla.gnome.org/show_bug.cgi?id=665152
 *
 * Also see [`ffi_call()`](man:ffi_call(3)): the storage requirements for return
 * values are ‘special’.
 *
 * The @interface_type argument only applies if @return_tag is
 * `GI_TYPE_TAG_INTERFACE`. Otherwise it is ignored.
 *
 * Since: 2.80
 */
void
gi_type_tag_extract_ffi_return_value (GITypeTag         return_tag,
                                      GType             interface_type,
                                      GIFFIReturnValue *ffi_value,
                                      GIArgument       *arg)
{
    switch (return_tag) {
    case GI_TYPE_TAG_INT8:
        arg->v_int8 = (int8_t) ffi_value->v_long;
        break;
    case GI_TYPE_TAG_UINT8:
        arg->v_uint8 = (uint8_t) ffi_value->v_ulong;
        break;
    case GI_TYPE_TAG_INT16:
        arg->v_int16 = (int16_t) ffi_value->v_long;
        break;
    case GI_TYPE_TAG_UINT16:
        arg->v_uint16 = (uint16_t) ffi_value->v_ulong;
        break;
    case GI_TYPE_TAG_INT32:
        arg->v_int32 = (int32_t) ffi_value->v_long;
        break;
    case GI_TYPE_TAG_UINT32:
    case GI_TYPE_TAG_BOOLEAN:
    case GI_TYPE_TAG_UNICHAR:
        arg->v_uint32 = (uint32_t) ffi_value->v_ulong;
        break;
    case GI_TYPE_TAG_INT64:
        arg->v_int64 = (int64_t) ffi_value->v_int64;
        break;
    case GI_TYPE_TAG_UINT64:
        arg->v_uint64 = (uint64_t) ffi_value->v_uint64;
        break;
    case GI_TYPE_TAG_FLOAT:
        arg->v_float = ffi_value->v_float;
        break;
    case GI_TYPE_TAG_DOUBLE:
        arg->v_double = ffi_value->v_double;
        break;
    case GI_TYPE_TAG_INTERFACE:
        if (interface_type == GI_TYPE_ENUM_INFO ||
            interface_type == GI_TYPE_FLAGS_INFO)
          arg->v_int32 = (int32_t) ffi_value->v_long;
        else
          arg->v_pointer = (void *) ffi_value->v_pointer;
        break;
    default:
        arg->v_pointer = (void *) ffi_value->v_pointer;
        break;
    }
}

/**
 * gi_type_info_extract_ffi_return_value:
 * @return_info: [type@GIRepository.TypeInfo] describing the return type
 * @ffi_value: pointer to [type@GIRepository.FFIReturnValue] union containing
 *   the return value from `ffi_call()`
 * @arg: (out caller-allocates): pointer to an allocated
 *   [class@GIRepository.Argument]
 *
 * Extract the correct bits from an `ffi_arg` return value into
 * [class@GIRepository.Argument].
 *
 * See: https://bugzilla.gnome.org/show_bug.cgi?id=665152
 *
 * Also see [`ffi_call()`](man:ffi_call(3)): the storage requirements for return
 * values are ‘special’.
 *
 * Since: 2.80
 */
void
gi_type_info_extract_ffi_return_value (GITypeInfo       *return_info,
                                       GIFFIReturnValue *ffi_value,
                                       GIArgument       *arg)
{
  GITypeTag return_tag = gi_type_info_get_tag (return_info);
  GType interface_type = G_TYPE_INVALID;

  if (return_tag == GI_TYPE_TAG_INTERFACE)
    {
      GIBaseInfo *interface_info = gi_type_info_get_interface (return_info);
      interface_type = G_TYPE_FROM_INSTANCE (interface_info);
      gi_base_info_unref (interface_info);
    }

  gi_type_tag_extract_ffi_return_value (return_tag, interface_type,
                                        ffi_value, arg);
}

/**
 * gi_callable_info_invoke:
 * @info: a #GICallableInfo
 * @function: function pointer to call
 * @in_args: (array length=n_in_args): array of ‘in’ arguments
 * @n_in_args: number of arguments in @in_args
 * @out_args: (array length=n_out_args): array of ‘out’ arguments allocated by
 *   the caller, to be populated with outputted values
 * @n_out_args: number of arguments in @out_args
 * @return_value: (out caller-allocates) (not optional) (nullable): return
 *   location for the return value from the callable; `NULL` may be returned if
 *   the callable returns that
 * @error: return location for a [type@GLib.Error], or `NULL`
 *
 * Invoke the given `GICallableInfo` by calling the given @function pointer.
 *
 * The set of arguments passed to @function will be constructed according to the
 * introspected type of the `GICallableInfo`, using @in_args, @out_args
 * and @error.
 *
 * Returns: `TRUE` if the callable was executed successfully and didn’t throw
 *   a [type@GLib.Error]; `FALSE` if @error is set
 * Since: 2.80
 */
gboolean
gi_callable_info_invoke (GICallableInfo    *info,
                         void              *function,
                         const GIArgument  *in_args,
                         size_t             n_in_args,
                         GIArgument        *out_args,
                         size_t             n_out_args,
                         GIArgument        *return_value,
                         GError           **error)
{
  ffi_cif cif;
  ffi_type *rtype;
  ffi_type **atypes;
  GITypeInfo *tinfo;
  GITypeInfo *rinfo;
  GITypeTag rtag;
  GIArgInfo *ainfo;
  unsigned int n_args, n_invoke_args, in_pos, out_pos, i;
  void **args;
  gboolean success = FALSE;
  GError *local_error = NULL;
  void *error_address = &local_error;
  GIFFIReturnValue ffi_return_value;
  void *return_value_p; /* Will point inside the union return_value */
  gboolean is_method, throws;

  rinfo = gi_callable_info_get_return_type ((GICallableInfo *)info);
  rtype = gi_type_info_get_ffi_type (rinfo);
  rtag = gi_type_info_get_tag(rinfo);
  is_method = gi_callable_info_is_method (info);
  throws = gi_callable_info_can_throw_gerror (info);

  in_pos = 0;
  out_pos = 0;

  n_args = gi_callable_info_get_n_args ((GICallableInfo *)info);
  if (is_method)
    {
      if (n_in_args == 0)
        {
          g_set_error (error,
                       GI_INVOKE_ERROR,
                       GI_INVOKE_ERROR_ARGUMENT_MISMATCH,
                       "Too few \"in\" arguments (handling this)");
          goto out;
        }
      n_invoke_args = n_args+1;
      in_pos++;
    }
  else
    n_invoke_args = n_args;

  if (throws)
    /* Add an argument for the GError */
    n_invoke_args ++;

  atypes = g_alloca (sizeof (ffi_type*) * n_invoke_args);
  args = g_alloca (sizeof (void *) * n_invoke_args);

  if (is_method)
    {
      atypes[0] = &ffi_type_pointer;
      args[0] = (void *) &in_args[0];
    }
  for (i = 0; i < n_args; i++)
    {
      size_t offset = (is_method ? 1 : 0);
      ainfo = gi_callable_info_get_arg ((GICallableInfo *)info, i);
      switch (gi_arg_info_get_direction (ainfo))
        {
        case GI_DIRECTION_IN:
          tinfo = gi_arg_info_get_type_info (ainfo);
          atypes[i+offset] = gi_type_info_get_ffi_type (tinfo);
          gi_base_info_unref ((GIBaseInfo *)ainfo);
          gi_base_info_unref ((GIBaseInfo *)tinfo);

          if (in_pos >= n_in_args)
            {
              g_set_error (error,
                           GI_INVOKE_ERROR,
                           GI_INVOKE_ERROR_ARGUMENT_MISMATCH,
                           "Too few \"in\" arguments (handling in)");
              goto out;
            }

          args[i+offset] = (void *)&in_args[in_pos];
          in_pos++;

          break;
        case GI_DIRECTION_OUT:
          atypes[i+offset] = &ffi_type_pointer;
          gi_base_info_unref ((GIBaseInfo *)ainfo);

          if (out_pos >= n_out_args)
            {
              g_set_error (error,
                           GI_INVOKE_ERROR,
                           GI_INVOKE_ERROR_ARGUMENT_MISMATCH,
                           "Too few \"out\" arguments (handling out)");
              goto out;
            }

          args[i+offset] = (void *)&out_args[out_pos];
          out_pos++;
          break;
        case GI_DIRECTION_INOUT:
          atypes[i+offset] = &ffi_type_pointer;
          gi_base_info_unref ((GIBaseInfo *)ainfo);

          if (in_pos >= n_in_args)
            {
              g_set_error (error,
                           GI_INVOKE_ERROR,
                           GI_INVOKE_ERROR_ARGUMENT_MISMATCH,
                           "Too few \"in\" arguments (handling inout)");
              goto out;
            }

          if (out_pos >= n_out_args)
            {
              g_set_error (error,
                           GI_INVOKE_ERROR,
                           GI_INVOKE_ERROR_ARGUMENT_MISMATCH,
                           "Too few \"out\" arguments (handling inout)");
              goto out;
            }

          args[i+offset] = (void *)&in_args[in_pos];
          in_pos++;
          out_pos++;
          break;
        default:
          gi_base_info_unref ((GIBaseInfo *)ainfo);
          g_assert_not_reached ();
        }
    }

  if (throws)
    {
      args[n_invoke_args - 1] = &error_address;
      atypes[n_invoke_args - 1] = &ffi_type_pointer;
    }

  if (in_pos < n_in_args)
    {
      g_set_error (error,
                   GI_INVOKE_ERROR,
                   GI_INVOKE_ERROR_ARGUMENT_MISMATCH,
                   "Too many \"in\" arguments (at end)");
      goto out;
    }
  if (out_pos < n_out_args)
    {
      g_set_error (error,
                   GI_INVOKE_ERROR,
                   GI_INVOKE_ERROR_ARGUMENT_MISMATCH,
                   "Too many \"out\" arguments (at end)");
      goto out;
    }

  if (ffi_prep_cif (&cif, FFI_DEFAULT_ABI, n_invoke_args, rtype, atypes) != FFI_OK)
    goto out;

  g_return_val_if_fail (return_value, FALSE);
  /* See comment for GIFFIReturnValue above */
  switch (rtag)
    {
    case GI_TYPE_TAG_FLOAT:
      return_value_p = &ffi_return_value.v_float;
      break;
    case GI_TYPE_TAG_DOUBLE:
      return_value_p = &ffi_return_value.v_double;
      break;
    case GI_TYPE_TAG_INT64:
    case GI_TYPE_TAG_UINT64:
      return_value_p = &ffi_return_value.v_uint64;
      break;
    default:
      return_value_p = &ffi_return_value.v_long;
    }
  ffi_call (&cif, function, return_value_p, args);

  if (local_error)
    {
      g_propagate_error (error, local_error);
      success = FALSE;
    }
  else
    {
      gi_type_info_extract_ffi_return_value (rinfo, &ffi_return_value, return_value);
      success = TRUE;
    }
 out:
  gi_base_info_unref ((GIBaseInfo *)rinfo);
  return success;
}

void
gi_callable_info_class_init (gpointer g_class,
                             gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_CALLABLE;
}

static GICallableInfo *
get_method_callable_info_for_index (GIBaseInfo   *rinfo,
                                    unsigned int  index)
{
  GIBaseInfo *container = rinfo->container;

  g_assert (container);

  if (GI_IS_OBJECT_INFO (container))
    return (GICallableInfo *) gi_object_info_get_method ((GIObjectInfo *) container, index);

  if (GI_IS_INTERFACE_INFO (container))
    return (GICallableInfo *) gi_interface_info_get_method ((GIInterfaceInfo *) container,
                                                            index);

  return NULL;
}

static GICallableInfo *
get_callable_info_for_index (GIBaseInfo   *rinfo,
                             unsigned int  index)
{
  if (!rinfo->container)
    {
      GIBaseInfo *info = gi_info_from_entry (rinfo->repository, rinfo->typelib, index);

      g_assert (GI_IS_CALLABLE_INFO (info));

      return (GICallableInfo *) info;
    }

  return get_method_callable_info_for_index (rinfo, index);
}

/**
 * gi_callable_info_get_async_function
 * @info: a callable info structure
 *
 * Gets the callable info for the callable's asynchronous version
 *
 * Returns: (transfer full) (nullable): a [class@GIRepository.CallableInfo] for the
 *   async function or `NULL` if not defined.
 * Since: 2.84
 */
GICallableInfo *
gi_callable_info_get_async_function (GICallableInfo *info)
{
  GIBaseInfo *rinfo = (GIBaseInfo *) info;

  switch (gi_base_info_get_info_type (rinfo))
    {
    case GI_INFO_TYPE_FUNCTION:
      {
        FunctionBlob *blob = GET_BLOB (FunctionBlob, rinfo);
        if (blob->is_async || blob->sync_or_async == ASYNC_SENTINEL)
          return NULL;

        return get_callable_info_for_index (rinfo, blob->sync_or_async);
      }
    case GI_INFO_TYPE_VFUNC:
      {
        VFuncBlob *blob = GET_BLOB (VFuncBlob, rinfo);
        if (blob->is_async || blob->sync_or_async == ASYNC_SENTINEL)
          return NULL;

        return get_method_callable_info_for_index (rinfo, blob->sync_or_async);
      }
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_SIGNAL:
      return NULL;
    default:
      g_assert_not_reached ();
    }
}

/**
 * gi_callable_info_get_sync_function
 * @info: a callable info structure
 *
 * Gets the callable info for the callable's synchronous version
 *
 * Returns: (transfer full) (nullable): a [class@GIRepository.CallableInfo] for the
 *   sync function or `NULL` if not defined.
 * Since: 2.84
 */
GICallableInfo *
gi_callable_info_get_sync_function (GICallableInfo *info)
{
  GIBaseInfo *rinfo = (GIBaseInfo *) info;

  switch (gi_base_info_get_info_type (rinfo))
    {
    case GI_INFO_TYPE_FUNCTION:
      {
        FunctionBlob *blob = GET_BLOB (FunctionBlob, rinfo);
        if (!blob->is_async || blob->sync_or_async == ASYNC_SENTINEL)
          return NULL;

        return get_callable_info_for_index (rinfo, blob->sync_or_async);
      }
    case GI_INFO_TYPE_VFUNC:
      {
        VFuncBlob *blob = GET_BLOB (VFuncBlob, rinfo);
        if (!blob->is_async || blob->sync_or_async == ASYNC_SENTINEL)
          return NULL;

        return get_method_callable_info_for_index (rinfo, blob->sync_or_async);
      }
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_SIGNAL:
      return NULL;
    default:
      g_assert_not_reached ();
    }
}

/**
 * gi_callable_info_get_finish_function
 * @info: a callable info structure
 *
 * Gets the info for an async function's corresponding finish function
 *
 * Returns: (transfer full) (nullable): a [class@GIRepository.CallableInfo] for the
 *   finish function or `NULL` if not defined.
 * Since: 2.84
 */
GICallableInfo *
gi_callable_info_get_finish_function (GICallableInfo *info)
{
  GIBaseInfo *rinfo = (GIBaseInfo *) info;

  switch (gi_base_info_get_info_type (rinfo))
    {
    case GI_INFO_TYPE_FUNCTION:
      {
        FunctionBlob *blob = GET_BLOB (FunctionBlob, rinfo);
        if (!blob->is_async || blob->finish == ASYNC_SENTINEL)
          return NULL;

        return get_callable_info_for_index (rinfo, blob->finish);
      }
    case GI_INFO_TYPE_VFUNC:
      {
        VFuncBlob *blob = GET_BLOB (VFuncBlob, rinfo);
        if (!blob->is_async || blob->finish == ASYNC_SENTINEL)
          return NULL;

        return get_method_callable_info_for_index (rinfo, blob->finish);
      }
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_SIGNAL:
      return NULL;
    default:
      g_assert_not_reached ();
    }
}

/**
 * gi_callable_info_is_async
 * @info: a callable info structure
 *
 * Gets whether a callable is ‘async’. Async callables have a
 * [type@Gio.AsyncReadyCallback] parameter and user data.
 *
 * Returns: true if the callable is async
 * Since: 2.84
 */
gboolean
gi_callable_info_is_async (GICallableInfo *callable_info)
{
  GIBaseInfo *info = (GIBaseInfo *) callable_info;
  switch (gi_base_info_get_info_type ((GIBaseInfo *) callable_info))
    {
    case GI_INFO_TYPE_FUNCTION:
      {
        return GET_BLOB (FunctionBlob, info)->is_async;
      }
    case GI_INFO_TYPE_VFUNC:
      {
        return GET_BLOB (VFuncBlob, info)->is_async;
      }
    case GI_INFO_TYPE_CALLBACK:
    case GI_INFO_TYPE_SIGNAL:
      return FALSE;
    default:
      g_assert_not_reached ();
    }
}

