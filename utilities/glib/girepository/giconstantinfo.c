/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Constant implementation
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
#include <string.h> // memcpy

#include <girepository/girepository.h>
#include "gibaseinfo-private.h"
#include "girepository-private.h"
#include "gitypelib-internal.h"
#include "giconstantinfo.h"

/**
 * GIConstantInfo:
 *
 * `GIConstantInfo` represents a constant.
 *
 * A constant has a type associated – which can be obtained by calling
 * [method@GIRepository.ConstantInfo.get_type_info] – and a value – which can be
 * obtained by calling [method@GIRepository.ConstantInfo.get_value].
 *
 * Since: 2.80
 */

/**
 * gi_constant_info_get_type_info:
 * @info: a #GIConstantInfo
 *
 * Obtain the type of the constant as a [class@GIRepository.TypeInfo].
 *
 * Returns: (transfer full): The [class@GIRepository.TypeInfo]. Free the struct
 *   by calling [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GITypeInfo *
gi_constant_info_get_type_info (GIConstantInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_CONSTANT_INFO (info), NULL);

  return gi_type_info_new ((GIBaseInfo*)info, rinfo->typelib, rinfo->offset + 8);
}

#define DO_ALIGNED_COPY(dest_addr, src_addr, type) \
        memcpy((dest_addr), (src_addr), sizeof(type))

/**
 * gi_constant_info_free_value: (skip)
 * @info: a #GIConstantInfo
 * @value: the argument
 *
 * Free the value returned from [method@GIRepository.ConstantInfo.get_value].
 *
 * Since: 2.80
 */
void
gi_constant_info_free_value (GIConstantInfo *info,
                             GIArgument     *value)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ConstantBlob *blob;

  g_return_if_fail (info != NULL);
  g_return_if_fail (GI_IS_CONSTANT_INFO (info));

  blob = (ConstantBlob *)&rinfo->typelib->data[rinfo->offset];

  /* FIXME non-basic types ? */
  if (blob->type.flags.reserved == 0 && blob->type.flags.reserved2 == 0)
    {
      if (blob->type.flags.pointer)
        g_free (value->v_pointer);
    }
}

/**
 * gi_constant_info_get_value: (skip)
 * @info: a #GIConstantInfo
 * @value: (out caller-allocates): an argument
 *
 * Obtain the value associated with the `GIConstantInfo` and store it in the
 * @value parameter.
 *
 * @argument needs to be allocated before passing it in.
 *
 * The size of the constant value (in bytes) stored in @argument will be
 * returned.
 *
 * Free the value with [method@GIRepository.ConstantInfo.free_value].
 *
 * Returns: size of the constant, in bytes
 * Since: 2.80
 */
size_t
gi_constant_info_get_value (GIConstantInfo *info,
                            GIArgument     *value)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ConstantBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_CONSTANT_INFO (info), 0);

  blob = (ConstantBlob *)&rinfo->typelib->data[rinfo->offset];

  /* FIXME non-basic types ? */
  if (blob->type.flags.reserved == 0 && blob->type.flags.reserved2 == 0)
    {
      if (blob->type.flags.pointer)
        {
          size_t blob_size = blob->size;

          value->v_pointer = g_memdup2 (&rinfo->typelib->data[blob->offset], blob_size);
        }
      else
        {
          switch (blob->type.flags.tag)
            {
            case GI_TYPE_TAG_BOOLEAN:
              value->v_boolean = *(gboolean*)&rinfo->typelib->data[blob->offset];
              break;
            case GI_TYPE_TAG_INT8:
              value->v_int8 = *(int8_t*)&rinfo->typelib->data[blob->offset];
              break;
            case GI_TYPE_TAG_UINT8:
              value->v_uint8 = *(uint8_t*)&rinfo->typelib->data[blob->offset];
              break;
            case GI_TYPE_TAG_INT16:
              value->v_int16 = *(int16_t*)&rinfo->typelib->data[blob->offset];
              break;
            case GI_TYPE_TAG_UINT16:
              value->v_uint16 = *(uint16_t*)&rinfo->typelib->data[blob->offset];
              break;
            case GI_TYPE_TAG_INT32:
              value->v_int32 = *(int32_t*)&rinfo->typelib->data[blob->offset];
              break;
            case GI_TYPE_TAG_UINT32:
              value->v_uint32 = *(uint32_t*)&rinfo->typelib->data[blob->offset];
              break;
            case GI_TYPE_TAG_INT64:
              DO_ALIGNED_COPY (&value->v_int64, &rinfo->typelib->data[blob->offset], int64_t);
              break;
            case GI_TYPE_TAG_UINT64:
              DO_ALIGNED_COPY (&value->v_uint64, &rinfo->typelib->data[blob->offset], uint64_t);
              break;
            case GI_TYPE_TAG_FLOAT:
              DO_ALIGNED_COPY (&value->v_float, &rinfo->typelib->data[blob->offset], float);
              break;
            case GI_TYPE_TAG_DOUBLE:
              DO_ALIGNED_COPY (&value->v_double, &rinfo->typelib->data[blob->offset], double);
              break;
            default:
              g_assert_not_reached ();
            }
        }
    }

  return blob->size;
}

void
gi_constant_info_class_init (gpointer g_class,
                             gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_CONSTANT;
}
