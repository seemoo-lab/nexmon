/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Property implementation
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
#include "gipropertyinfo.h"

/**
 * GIPropertyInfo:
 *
 * `GIPropertyInfo` represents a property in a [class@GObject.Object].
 *
 * A property belongs to either a [class@GIRepository.ObjectInfo] or a
 * [class@GIRepository.InterfaceInfo].
 *
 * Since: 2.80
 */

/**
 * gi_property_info_get_flags:
 * @info: a #GIPropertyInfo
 *
 * Obtain the flags for this property info.
 *
 * See [type@GObject.ParamFlags] for more information about possible flag
 * values.
 *
 * Returns: the flags
 * Since: 2.80
 */
GParamFlags
gi_property_info_get_flags (GIPropertyInfo *info)
{
  GParamFlags flags;
  GIRealInfo *rinfo = (GIRealInfo *)info;
  PropertyBlob *blob;

  g_return_val_if_fail (info != NULL, 0);
  g_return_val_if_fail (GI_IS_PROPERTY_INFO (info), 0);

  blob = (PropertyBlob *)&rinfo->typelib->data[rinfo->offset];

  flags = 0;

  if (blob->readable)
    flags = flags | G_PARAM_READABLE;

  if (blob->writable)
    flags = flags | G_PARAM_WRITABLE;

  if (blob->construct)
    flags = flags | G_PARAM_CONSTRUCT;

  if (blob->construct_only)
    flags = flags | G_PARAM_CONSTRUCT_ONLY;

  return flags;
}

/**
 * gi_property_info_get_type_info:
 * @info: a #GIPropertyInfo
 *
 * Obtain the type information for the property @info.
 *
 * Returns: (transfer full): The [class@GIRepository.TypeInfo]. Free it with
 *   [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GITypeInfo *
gi_property_info_get_type_info (GIPropertyInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_PROPERTY_INFO (info), NULL);

  return gi_type_info_new ((GIBaseInfo*)info,
                           rinfo->typelib,
                           rinfo->offset + G_STRUCT_OFFSET (PropertyBlob, type));
}

/**
 * gi_property_info_get_ownership_transfer:
 * @info: a #GIPropertyInfo
 *
 * Obtain the ownership transfer for this property.
 *
 * See [type@GIRepository.Transfer] for more information about transfer values.
 *
 * Returns: the transfer
 * Since: 2.80
 */
GITransfer
gi_property_info_get_ownership_transfer (GIPropertyInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  PropertyBlob *blob;

  g_return_val_if_fail (info != NULL, GI_TRANSFER_NOTHING);
  g_return_val_if_fail (GI_IS_PROPERTY_INFO (info), GI_TRANSFER_NOTHING);

  blob = (PropertyBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->transfer_ownership)
    return GI_TRANSFER_EVERYTHING;
  else if (blob->transfer_container_ownership)
    return GI_TRANSFER_CONTAINER;
  else
    return GI_TRANSFER_NOTHING;
}

/**
 * gi_property_info_get_setter:
 * @info: a #GIPropertyInfo
 *
 * Obtains the setter function associated with this `GIPropertyInfo`.
 *
 * The setter is only available for `G_PARAM_WRITABLE` properties that
 * are also not `G_PARAM_CONSTRUCT_ONLY`.
 *
 * Returns: (transfer full) (nullable): The function info, or `NULL` if not set.
 *   Free it with [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_property_info_get_setter (GIPropertyInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  PropertyBlob *blob;
  GIBaseInfo *container;
  GIInfoType parent_type;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_PROPERTY_INFO (info), NULL);

  blob = (PropertyBlob *)&rinfo->typelib->data[rinfo->offset];
  if (!blob->writable || blob->construct_only)
    return NULL;

  if (blob->setter == ACCESSOR_SENTINEL)
    return NULL;

  container = rinfo->container;
  parent_type = gi_base_info_get_info_type (container);
  if (parent_type == GI_INFO_TYPE_OBJECT)
    return gi_object_info_get_method ((GIObjectInfo *) container, blob->setter);
  else if (parent_type == GI_INFO_TYPE_INTERFACE)
    return gi_interface_info_get_method ((GIInterfaceInfo *) container, blob->setter);
  else
    return NULL;
}

/**
 * gi_property_info_get_getter:
 * @info: a #GIPropertyInfo
 *
 * Obtains the getter function associated with this `GIPropertyInfo`.
 *
 * The setter is only available for `G_PARAM_READABLE` properties.
 *
 * Returns: (transfer full) (nullable): The function info, or `NULL` if not set.
 *   Free it with [method@GIRepository.BaseInfo.unref] when done.
 * Since: 2.80
 */
GIFunctionInfo *
gi_property_info_get_getter (GIPropertyInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  PropertyBlob *blob;
  GIBaseInfo *container;
  GIInfoType parent_type;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (GI_IS_PROPERTY_INFO (info), NULL);

  blob = (PropertyBlob *)&rinfo->typelib->data[rinfo->offset];
  if (!blob->readable)
    return NULL;

  if (blob->getter == ACCESSOR_SENTINEL)
    return NULL;

  container = rinfo->container;
  parent_type = gi_base_info_get_info_type (container);
  if (parent_type == GI_INFO_TYPE_OBJECT)
    return gi_object_info_get_method ((GIObjectInfo *) container, blob->getter);
  else if (parent_type == GI_INFO_TYPE_INTERFACE)
    return gi_interface_info_get_method ((GIInterfaceInfo *) container, blob->getter);
  else
    return NULL;
}

void
gi_property_info_class_init (gpointer g_class,
                             gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_PROPERTY;
}
