/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Argument implementation
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

#include "gibaseinfo-private.h"
#include "gitypelib-internal.h"
#include "girepository-private.h"
#include "giarginfo.h"


/* GIArgInfo functions */

/**
 * GIArgInfo:
 *
 * `GIArgInfo` represents an argument of a callable.
 *
 * An argument is always part of a [class@GIRepository.CallableInfo].
 *
 * Since: 2.80
 */

/**
 * gi_arg_info_get_direction:
 * @info: a #GIArgInfo
 *
 * Obtain the direction of the argument. Check [type@GIRepository.Direction]
 * for possible direction values.
 *
 * Returns: The direction
 * Since: 2.80
 */
GIDirection
gi_arg_info_get_direction (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;

  g_return_val_if_fail (info != NULL, GI_DIRECTION_OUT);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), GI_DIRECTION_OUT);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->in && blob->out)
    return GI_DIRECTION_INOUT;
  else if (blob->out)
    return GI_DIRECTION_OUT;
  else
    return GI_DIRECTION_IN;
}

/**
 * gi_arg_info_is_return_value:
 * @info: a #GIArgInfo
 *
 * Obtain if the argument is a return value. It can either be a
 * parameter or a return value.
 *
 * Returns: `TRUE` if it is a return value
 * Since: 2.80
 */
gboolean
gi_arg_info_is_return_value (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), FALSE);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->return_value;
}

/**
 * gi_arg_info_is_caller_allocates:
 * @info: a #GIArgInfo
 *
 * Obtain if the argument is a pointer to a struct or object that will
 * receive an output of a function.
 *
 * The default assumption for `GI_DIRECTION_OUT` arguments which have allocation
 * is that the callee allocates; if this is `TRUE`, then the caller must
 * allocate.
 *
 * Returns: `TRUE` if caller is required to have allocated the argument
 * Since: 2.80
 */
gboolean
gi_arg_info_is_caller_allocates (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), FALSE);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->caller_allocates;
}

/**
 * gi_arg_info_is_optional:
 * @info: a #GIArgInfo
 *
 * Obtain if the argument is optional.
 *
 * For ‘out’ arguments this means that you can pass `NULL` in order to ignore
 * the result.
 *
 * Returns: `TRUE` if it is an optional argument
 * Since: 2.80
 */
gboolean
gi_arg_info_is_optional (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), FALSE);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->optional;
}

/**
 * gi_arg_info_may_be_null:
 * @info: a #GIArgInfo
 *
 * Obtain if the type of the argument includes the possibility of `NULL`.
 *
 * For ‘in’ values this means that `NULL` is a valid value.  For ‘out’
 * values, this means that `NULL` may be returned.
 *
 * See also [method@GIRepository.ArgInfo.is_optional].
 *
 * Returns: `TRUE` if the value may be `NULL`
 * Since: 2.80
 */
gboolean
gi_arg_info_may_be_null (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), FALSE);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->nullable;
}

/**
 * gi_arg_info_is_skip:
 * @info: a #GIArgInfo
 *
 * Obtain if an argument is only useful in C.
 *
 * Returns: `TRUE` if argument is only useful in C.
 * Since: 2.80
 */
gboolean
gi_arg_info_is_skip (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), FALSE);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->skip;
}

/**
 * gi_arg_info_get_ownership_transfer:
 * @info: a #GIArgInfo
 *
 * Obtain the ownership transfer for this argument.
 * [type@GIRepository.Transfer] contains a list of possible values.
 *
 * Returns: The transfer
 * Since: 2.80
 */
GITransfer
gi_arg_info_get_ownership_transfer (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;

  g_return_val_if_fail (info != NULL, GI_TRANSFER_NOTHING);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), GI_TRANSFER_NOTHING);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->transfer_ownership)
    return GI_TRANSFER_EVERYTHING;
  else if (blob->transfer_container_ownership)
    return GI_TRANSFER_CONTAINER;
  else
    return GI_TRANSFER_NOTHING;
}

/**
 * gi_arg_info_get_scope:
 * @info: a #GIArgInfo
 *
 * Obtain the scope type for this argument.
 *
 * The scope type explains how a callback is going to be invoked, most
 * importantly when the resources required to invoke it can be freed.
 *
 * [type@GIRepository.ScopeType] contains a list of possible values.
 *
 * Returns: The scope type
 * Since: 2.80
 */
GIScopeType
gi_arg_info_get_scope (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;

  g_return_val_if_fail (info != NULL, GI_SCOPE_TYPE_INVALID);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), GI_SCOPE_TYPE_INVALID);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->scope;
}

/**
 * gi_arg_info_get_closure_index:
 * @info: a #GIArgInfo
 * @out_closure_index: (out) (optional): return location for the closure index
 *
 * Obtain the index of the user data argument. This is only valid
 * for arguments which are callbacks.
 *
 * Returns: `TRUE` if the argument has a user data argument
 * Since: 2.80
 */
gboolean
gi_arg_info_get_closure_index (GIArgInfo    *info,
                               unsigned int *out_closure_index)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;
  gboolean has_closure_index;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), FALSE);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  has_closure_index = (blob->closure >= 0);

  if (out_closure_index != NULL)
    *out_closure_index = has_closure_index ? (unsigned int) blob->closure : 0;
  return has_closure_index;
}

/**
 * gi_arg_info_get_destroy_index:
 * @info: a #GIArgInfo
 * @out_destroy_index: (out) (optional): return location for the destroy index
 *
 * Obtains the index of the [type@GLib.DestroyNotify] argument. This is only
 * valid for arguments which are callbacks.
 *
 * Returns: `TRUE` if the argument has a [type@GLib.DestroyNotify] argument
 * Since: 2.80
 */
gboolean
gi_arg_info_get_destroy_index (GIArgInfo    *info,
                               unsigned int *out_destroy_index)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ArgBlob *blob;
  gboolean has_destroy_index;

  g_return_val_if_fail (info != NULL, FALSE);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), FALSE);

  blob = (ArgBlob *)&rinfo->typelib->data[rinfo->offset];

  has_destroy_index = (blob->destroy >= 0);

  if (out_destroy_index != NULL)
    *out_destroy_index = has_destroy_index ? (unsigned int) blob->destroy : 0;
  return has_destroy_index;
}

/**
 * gi_arg_info_get_type_info:
 * @info: a #GIArgInfo
 *
 * Obtain the type information for @info.
 *
 * Returns: (transfer full): The [class@GIRepository.TypeInfo] holding the type
 *   information for @info, free it with [method@GIRepository.BaseInfo.unref]
 *   when done
 * Since: 2.80
 */
GITypeInfo *
gi_arg_info_get_type_info (GIArgInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_ARG_INFO (info), NULL);

  return gi_type_info_new ((GIBaseInfo*)info, rinfo->typelib, rinfo->offset + G_STRUCT_OFFSET (ArgBlob, arg_type));
}

/**
 * gi_arg_info_load_type_info:
 * @info: a #GIArgInfo
 * @type: (out caller-allocates): Initialized with information about type of @info
 *
 * Obtain information about a the type of given argument @info; this
 * function is a variant of [method@GIRepository.ArgInfo.get_type_info] designed
 * for stack allocation.
 *
 * The initialized @type must not be referenced after @info is deallocated.
 *
 * Once you are done with @type, it must be cleared using
 * [method@GIRepository.BaseInfo.clear].
 *
 * Since: 2.80
 */
void
gi_arg_info_load_type_info (GIArgInfo  *info,
                            GITypeInfo *type)
{
  GIRealInfo *rinfo = (GIRealInfo*) info;

  g_return_if_fail (info != NULL);
  g_return_if_fail (GI_IS_ARG_INFO (info));

  gi_type_info_init (type, (GIBaseInfo*)info, rinfo->typelib, rinfo->offset + G_STRUCT_OFFSET (ArgBlob, arg_type));
}

void
gi_arg_info_class_init (gpointer g_class,
                        gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_ARG;
}
