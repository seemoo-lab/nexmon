/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Union implementation
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
#include "giunioninfo.h"

/**
 * GIUnionInfo:
 *
 * `GIUnionInfo` represents a union type.
 *
 * A union has methods and fields.  Unions can optionally have a
 * discriminator, which is a field deciding what type of real union
 * fields is valid for specified instance.
 *
 * Since: 2.80
 */

/**
 * gi_union_info_get_n_fields:
 * @info: a #GIUnionInfo
 *
 * Obtain the number of fields this union has.
 *
 * Returns: number of fields
 * Since: 2.80
 */
unsigned int
gi_union_info_get_n_fields  (GIUnionInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_fields;
}

/**
 * gi_union_info_get_field:
 * @info: a #GIUnionInfo
 * @n: a field index
 *
 * Obtain the type information for the field with the specified index.
 *
 * Returns: (transfer full): the [type@GIRepository.FieldInfo], free it with
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFieldInfo *
gi_union_info_get_field (GIUnionInfo *info,
                         unsigned int n)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header = (Header *)rinfo->typelib->data;

  return (GIFieldInfo *) gi_base_info_new (GI_INFO_TYPE_FIELD, (GIBaseInfo*)info, rinfo->typelib,
                                           rinfo->offset + header->union_blob_size +
                                           n * header->field_blob_size);
}

/**
 * gi_union_info_get_n_methods:
 * @info: a #GIUnionInfo
 *
 * Obtain the number of methods this union has.
 *
 * Returns: number of methods
 * Since: 2.80
 */
unsigned int
gi_union_info_get_n_methods (GIUnionInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->n_functions;
}

/**
 * gi_union_info_get_method:
 * @info: a #GIUnionInfo
 * @n: a method index
 *
 * Obtain the type information for the method with the specified index.
 *
 * Returns: (transfer full): the [type@GIRepository.FunctionInfo], free it
 *   with [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_union_info_get_method (GIUnionInfo *info,
                          unsigned int n)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];
  Header *header = (Header *)rinfo->typelib->data;
  size_t offset;

  offset = rinfo->offset + header->union_blob_size
    + blob->n_fields * header->field_blob_size
    + n * header->function_blob_size;
  return (GIFunctionInfo *) gi_base_info_new (GI_INFO_TYPE_FUNCTION, (GIBaseInfo*)info,
                                              rinfo->typelib, offset);
}

/**
 * gi_union_info_is_discriminated:
 * @info: a #GIUnionInfo
 *
 * Return `TRUE` if this union contains a discriminator field.
 *
 * Returns: `TRUE` if this is a discriminated union, `FALSE` otherwise
 * Since: 2.80
 */
gboolean
gi_union_info_is_discriminated (GIUnionInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->discriminated;
}

/**
 * gi_union_info_get_discriminator_offset:
 * @info: a #GIUnionInfo
 * @out_offset: (out) (optional): return location for the offset, in bytes, of
 *   the discriminator
 *
 * Obtain the offset of the discriminator field within the structure.
 *
 * The union must be discriminated, or `FALSE` will be returned.
 *
 * Returns: `TRUE` if the union is discriminated
 * Since: 2.80
 */
gboolean
gi_union_info_get_discriminator_offset (GIUnionInfo *info,
                                        size_t      *out_offset)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];
  size_t discriminator_offset;

  discriminator_offset = (blob->discriminated) ? (size_t) blob->discriminator_offset : 0;

  if (out_offset != NULL)
    *out_offset = discriminator_offset;

  return blob->discriminated;
}

/**
 * gi_union_info_get_discriminator_type:
 * @info: a #GIUnionInfo
 *
 * Obtain the type information of the union discriminator.
 *
 * Returns: (transfer full) (nullable): the [type@GIRepository.TypeInfo], or
 *   `NULL` if the union is not discriminated. Free it with
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GITypeInfo *
gi_union_info_get_discriminator_type (GIUnionInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  if (!blob->discriminated)
    return NULL;

  return gi_type_info_new ((GIBaseInfo*)info, rinfo->typelib, rinfo->offset + 24);
}

/**
 * gi_union_info_get_discriminator:
 * @info: a #GIUnionInfo
 * @n: a union field index
 *
 * Obtain the discriminator value assigned for n-th union field, i.e. the n-th
 * union field is the active one if the discriminator contains this
 * constant.
 *
 * If the union is not discriminated, `NULL` is returned.
 *
 * Returns: (transfer full) (nullable): The [type@GIRepository.ConstantInfo], or
 *   `NULL` if the union is not discriminated. Free it with
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIConstantInfo *
gi_union_info_get_discriminator (GIUnionInfo *info,
                                 size_t       n)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->discriminated)
    {
      Header *header = (Header *)rinfo->typelib->data;
      size_t offset;

      offset = rinfo->offset + header->union_blob_size
        + blob->n_fields * header->field_blob_size
        + blob->n_functions * header->function_blob_size
        + n * header->constant_blob_size;

      return (GIConstantInfo *) gi_base_info_new (GI_INFO_TYPE_CONSTANT, (GIBaseInfo*)info,
                                                  rinfo->typelib, offset);
    }

  return NULL;
}

/**
 * gi_union_info_find_method:
 * @info: a #GIUnionInfo
 * @name: a method name
 *
 * Obtain the type information for the method named @name.
 *
 * Returns: (transfer full) (nullable): The [type@GIRepository.FunctionInfo], or
 *   `NULL` if none was found. Free it with [method@GIRepository.BaseInfo.unref]
 *   when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_union_info_find_method (GIUnionInfo *info,
                           const char  *name)
{
  size_t offset;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  Header *header = (Header *)rinfo->typelib->data;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  offset = rinfo->offset + header->union_blob_size
    + blob->n_fields * header->field_blob_size;

  return gi_base_info_find_method ((GIBaseInfo*)info, offset, blob->n_functions, name);
}

/**
 * gi_union_info_get_size:
 * @info: a #GIUnionInfo
 *
 * Obtain the total size of the union.
 *
 * Returns: size of the union, in bytes
 * Since: 2.80
 */
size_t
gi_union_info_get_size (GIUnionInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->size;
}

/**
 * gi_union_info_get_alignment:
 * @info: a #GIUnionInfo
 *
 * Obtain the required alignment of the union.
 *
 * Returns: required alignment, in bytes
 * Since: 2.80
 */
size_t
gi_union_info_get_alignment (GIUnionInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  return blob->alignment;
}

/**
 * gi_union_info_get_copy_function_name:
 * @info: a union information blob
 *
 * Retrieves the name of the copy function for @info, if any is set.
 *
 * Returns: (transfer none) (nullable): the name of the copy function, or `NULL`
 *   if none is set
 * Since: 2.80
 */
const char *
gi_union_info_get_copy_function_name (GIUnionInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_UNION_INFO (info), NULL);

  blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->copy_func)
    return gi_typelib_get_string (rinfo->typelib, blob->copy_func);

  return NULL;
}

/**
 * gi_union_info_get_free_function_name:
 * @info: a union information blob
 *
 * Retrieves the name of the free function for @info, if any is set.
 *
 * Returns: (transfer none) (nullable): the name of the free function, or `NULL`
 *   if none is set
 * Since: 2.80
 */
const char *
gi_union_info_get_free_function_name (GIUnionInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  UnionBlob *blob;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_UNION_INFO (info), NULL);

  blob = (UnionBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->free_func)
    return gi_typelib_get_string (rinfo->typelib, blob->free_func);

  return NULL;
}

void
gi_union_info_class_init (gpointer g_class,
                          gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_UNION;
}
