/*
 * Copyright 2024 GNOME Foundation, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Philip Withnall <pwithnall@gnome.org>
 */

#if !defined (__GIREPOSITORY_H_INSIDE__) && !defined (GI_COMPILATION)
#error "Only <girepository.h> can be included directly."
#endif

#ifndef __GI_SCANNER__

/* GIRepository already has its cleanups defined by G_DECLARE_FINAL_TYPE */
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GITypelib, gi_typelib_unref)

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIBaseInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIArgInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GICallableInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GICallbackInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIConstantInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIEnumInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIFieldInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIFlagsInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIFunctionInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIInterfaceInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIObjectInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIPropertyInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIRegisteredTypeInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GISignalInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIStructInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GITypeInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIUnionInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIUnresolvedInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIValueInfo, gi_base_info_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GIVFuncInfo, gi_base_info_unref)

/* These types can additionally be stack allocated and cleared */
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (GIArgInfo, gi_base_info_clear)
G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (GITypeInfo, gi_base_info_clear)

#endif /* __GI_SCANNER__ */
