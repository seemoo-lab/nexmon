/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Callable
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

#pragma once

#if !defined (__GIREPOSITORY_H_INSIDE__) && !defined (GI_COMPILATION)
#error "Only <girepository.h> can be included directly."
#endif

#include <girepository/gitypes.h>

G_BEGIN_DECLS

#define GI_TYPE_CALLABLE_INFO (gi_callable_info_get_type ())

/**
 * GI_CALLABLE_INFO:
 * @info: Info object which is subject to casting.
 *
 * Casts a [type@GIRepository.CallableInfo] or derived pointer into a
 * `(GICallableInfo*)` pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 *
 * Since: 2.80
 */
#define GI_CALLABLE_INFO(info) (G_TYPE_CHECK_INSTANCE_CAST ((info), GI_TYPE_CALLABLE_INFO, GICallableInfo))

/**
 * GI_IS_CALLABLE_INFO:
 * @info: an info structure
 *
 * Checks if @info is a [class@GIRepository.CallableInfo] or derived from it.
 *
 * Since: 2.80
 */
#define GI_IS_CALLABLE_INFO(info) (G_TYPE_CHECK_INSTANCE_TYPE ((info), GI_TYPE_CALLABLE_INFO))


GI_AVAILABLE_IN_ALL
gboolean               gi_callable_info_is_method (GICallableInfo *info);

GI_AVAILABLE_IN_ALL
gboolean               gi_callable_info_can_throw_gerror (GICallableInfo *info);

GI_AVAILABLE_IN_ALL
GITypeInfo *           gi_callable_info_get_return_type (GICallableInfo *info);

GI_AVAILABLE_IN_ALL
void                   gi_callable_info_load_return_type (GICallableInfo *info,
                                                          GITypeInfo     *type);

GI_AVAILABLE_IN_ALL
const char  *          gi_callable_info_get_return_attribute (GICallableInfo *info,
                                                              const char     *name);

GI_AVAILABLE_IN_ALL
gboolean               gi_callable_info_iterate_return_attributes (GICallableInfo   *info,
                                                                   GIAttributeIter  *iterator,
                                                                   const char      **name,
                                                                   const char      **value);

GI_AVAILABLE_IN_ALL
GITransfer             gi_callable_info_get_caller_owns (GICallableInfo *info);

GI_AVAILABLE_IN_ALL
gboolean               gi_callable_info_may_return_null (GICallableInfo *info);

GI_AVAILABLE_IN_ALL
gboolean               gi_callable_info_skip_return     (GICallableInfo *info);

GI_AVAILABLE_IN_ALL
unsigned int           gi_callable_info_get_n_args      (GICallableInfo *info);

GI_AVAILABLE_IN_ALL
GIArgInfo *            gi_callable_info_get_arg         (GICallableInfo *info,
                                                         unsigned int    n);

GI_AVAILABLE_IN_ALL
void                   gi_callable_info_load_arg        (GICallableInfo *info,
                                                         unsigned int    n,
                                                         GIArgInfo      *arg);

GI_AVAILABLE_IN_ALL
gboolean               gi_callable_info_invoke          (GICallableInfo    *info,
                                                         void              *function,
                                                         const GIArgument  *in_args,
                                                         size_t             n_in_args,
                                                         GIArgument        *out_args,
                                                         size_t             n_out_args,
                                                         GIArgument        *return_value,
                                                         GError           **error);

GI_AVAILABLE_IN_ALL
GITransfer             gi_callable_info_get_instance_ownership_transfer (GICallableInfo *info);

GI_AVAILABLE_IN_2_84
GICallableInfo        *gi_callable_info_get_async_function (GICallableInfo *info);

GI_AVAILABLE_IN_2_84
GICallableInfo        *gi_callable_info_get_sync_function (GICallableInfo *info);

GI_AVAILABLE_IN_2_84
GICallableInfo        *gi_callable_info_get_finish_function (GICallableInfo *info);

GI_AVAILABLE_IN_2_84
gboolean               gi_callable_info_is_async (GICallableInfo *info);

G_END_DECLS
