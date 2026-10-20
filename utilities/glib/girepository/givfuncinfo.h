/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Virtual Functions
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

#define GI_TYPE_VFUNC_INFO (gi_vfunc_info_get_type ())

/**
 * GI_VFUNC_INFO:
 * @info: Info object which is subject to casting.
 *
 * Casts a [type@GIRepository.VFuncInfo] or derived pointer into a
 * `(GIVFuncInfo*)` pointer.
 *
 * Depending on the current debugging level, this function may invoke
 * certain runtime checks to identify invalid casts.
 *
 * Since: 2.80
 */
#define GI_VFUNC_INFO(info) (G_TYPE_CHECK_INSTANCE_CAST ((info), GI_TYPE_VFUNC_INFO, GIVFuncInfo))

/**
 * GI_IS_VFUNC_INFO:
 * @info: an info structure
 *
 * Checks if @info is a [struct@GIRepository.VFuncInfo] (or a derived type).
 *
 * Since: 2.80
 */
#define GI_IS_VFUNC_INFO(info) (G_TYPE_CHECK_INSTANCE_TYPE ((info), GI_TYPE_VFUNC_INFO))

GI_AVAILABLE_IN_ALL
GIVFuncInfoFlags  gi_vfunc_info_get_flags   (GIVFuncInfo *info);

GI_AVAILABLE_IN_ALL
size_t            gi_vfunc_info_get_offset  (GIVFuncInfo *info);

GI_AVAILABLE_IN_ALL
GISignalInfo *    gi_vfunc_info_get_signal  (GIVFuncInfo *info);

GI_AVAILABLE_IN_ALL
GIFunctionInfo *  gi_vfunc_info_get_invoker (GIVFuncInfo *info);

GI_AVAILABLE_IN_ALL
void *            gi_vfunc_info_get_address (GIVFuncInfo *info,
                                             GType        implementor_gtype,
                                             GError     **error);

GI_AVAILABLE_IN_ALL
gboolean          gi_vfunc_info_invoke      (GIVFuncInfo      *info,
                                             GType             implementor,
                                             const GIArgument *in_args,
                                             size_t            n_in_args,
                                             GIArgument       *out_args,
                                             size_t            n_out_args,
                                             GIArgument       *return_value,
                                             GError          **error);

G_END_DECLS
