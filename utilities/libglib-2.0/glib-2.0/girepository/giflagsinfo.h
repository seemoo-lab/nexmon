/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Flags type
 *
 * Copyright 2024 GNOME Foundation, Inc.
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

#pragma once

#if !defined (__GIREPOSITORY_H_INSIDE__) && !defined (GI_COMPILATION)
#error "Only <girepository.h> can be included directly."
#endif

#include <girepository/gitypes.h>

G_BEGIN_DECLS

#define GI_TYPE_FLAGS_INFO (gi_flags_info_get_type ())

/**
 * GI_FLAGS_INFO:
 * @info: Info object which is subject to casting.
 *
 * Casts a [type@GIRepository.FlagsInfo] or derived pointer into a
 * `(GIFlagsInfo*)` pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 *
 * Since: 2.80
 */
#define GI_FLAGS_INFO(info) (G_TYPE_CHECK_INSTANCE_CAST ((info), GI_TYPE_FLAGS_INFO, GIFlagsInfo))

/**
 * GI_IS_FLAGS_INFO:
 * @info: an info structure
 *
 * Checks if @info is a [class@GIRepository.FlagsInfo] (or a derived type).
 *
 * Since: 2.80
 */
#define GI_IS_FLAGS_INFO(info) (G_TYPE_CHECK_INSTANCE_TYPE ((info), GI_TYPE_FLAGS_INFO))

G_END_DECLS
