/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: Helper functions for ffi integration
 *
 * Copyright (C) 2008 Red Hat, Inc
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

#include <ffi.h>
#include "girepository.h"

G_BEGIN_DECLS

/**
 * GIFFIClosureCallback:
 * @cif: the `ffi_cif` passed to
 *   [method@GIRepository.CallableInfo.create_closure]
 * @ret: (out caller-allocates): a pointer to the memory used for the function’s
 *   return value
 * @args: a vector of pointers to memory holding the arguments to the function
 * @user_data: the user data passed to
 *   [method@GIRepository.CallableInfo.create_closure]
 *
 * The function which will be called when a closure created with
 * [method@GIRepository.CallableInfo.create_closure] is invoked.
 *
 * The value of @ret is undefined if the function returns `void`.
 *
 * Since: 2.80
 */
typedef void (*GIFFIClosureCallback) (ffi_cif  *cif,
                                      void     *ret,
                                      void    **args,
                                      void     *user_data);

/**
 * GIFunctionInvoker:
 * @cif: the cif
 * @native_address: the native address
 *
 * Structure containing the data necessary to invoke a callable function.
 *
 * Since: 2.80
 */
typedef struct {
  ffi_cif cif;
  void *native_address;
  /*< private >*/
  void *padding[3];
} GIFunctionInvoker;

/**
 * GIFFIReturnValue:
 *
 * The type of a return value from a callable invocation closure.
 *
 * Since: 2.80
 */
typedef GIArgument GIFFIReturnValue;

GI_AVAILABLE_IN_ALL
ffi_type *    gi_type_tag_get_ffi_type            (GITypeTag type_tag, gboolean is_pointer);

GI_AVAILABLE_IN_ALL
ffi_type *    gi_type_info_get_ffi_type           (GITypeInfo           *info);

GI_AVAILABLE_IN_ALL
void          gi_type_info_extract_ffi_return_value (GITypeInfo                  *return_info,
                                                     GIFFIReturnValue            *ffi_value,
                                                     GIArgument                  *arg);

GI_AVAILABLE_IN_ALL
void          gi_type_tag_extract_ffi_return_value (GITypeTag         return_tag,
                                                    GType             interface_type,
                                                    GIFFIReturnValue *ffi_value,
                                                    GIArgument       *arg);

GI_AVAILABLE_IN_ALL
gboolean      gi_function_info_prep_invoker        (GIFunctionInfo       *info,
                                                    GIFunctionInvoker    *invoker,
                                                    GError              **error);

GI_AVAILABLE_IN_ALL
gboolean      gi_function_invoker_new_for_address  (void                 *addr,
                                                    GICallableInfo       *info,
                                                    GIFunctionInvoker    *invoker,
                                                    GError              **error);

GI_AVAILABLE_IN_ALL
void          gi_function_invoker_clear            (GIFunctionInvoker    *invoker);

G_DEFINE_AUTO_CLEANUP_CLEAR_FUNC (GIFunctionInvoker, gi_function_invoker_clear)

GI_AVAILABLE_IN_ALL
ffi_closure * gi_callable_info_create_closure (GICallableInfo       *callable_info,
                                               ffi_cif              *cif,
                                               GIFFIClosureCallback  callback,
                                               void                 *user_data);

GI_AVAILABLE_IN_ALL
void *gi_callable_info_get_closure_native_address (GICallableInfo *callable_info,
                                                   ffi_closure *closure);

GI_AVAILABLE_IN_ALL
void          gi_callable_info_destroy_closure (GICallableInfo       *callable_info,
                                                ffi_closure          *closure);

G_END_DECLS
