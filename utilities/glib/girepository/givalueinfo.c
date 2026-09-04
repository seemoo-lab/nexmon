/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Enum implementation
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
#include "givalueinfo.h"

/**
 * GIValueInfo:
 *
 * A `GIValueInfo` represents a value in an enumeration.
 *
 * The `GIValueInfo` is fetched by calling
 * [method@GIRepository.EnumInfo.get_value] on a [class@GIRepository.EnumInfo].
 *
 * Since: 2.80
 */

/**
 * gi_value_info_get_value:
 * @info: a #GIValueInfo
 *
 * Obtain the enumeration value of the `GIValueInfo`.
 *
 * Returns: the enumeration value. This will always be representable
 *   as a 32-bit signed or unsigned value. The use of `int64_t` as the
 *   return type is to allow both.
 * Since: 2.80
 */
int64_t
gi_value_info_get_value (GIValueInfo *info)
{
  GIRealInfo *rinfo = (GIRealInfo *)info;
  ValueBlob *blob;

  g_return_val_if_fail (info != NULL, -1);
  g_return_val_if_fail (GI_IS_VALUE_INFO (info), -1);

  blob = (ValueBlob *)&rinfo->typelib->data[rinfo->offset];

  if (blob->unsigned_value)
    return (int64_t)(uint32_t)blob->value;
  else
    return (int64_t)blob->value;
}

void
gi_value_info_class_init (gpointer g_class,
                          gpointer class_data)
{
  GIBaseInfoClass *info_class = g_class;

  info_class->info_type = GI_INFO_TYPE_VALUE;
}
