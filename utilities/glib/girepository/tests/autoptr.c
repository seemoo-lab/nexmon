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

#include "girepository.h"
#include "girffi.h"
#include "glib.h"
#include "test-common.h"

static void
test_autoptr_repository (RepositoryFixture *fx,
                         const void        *unused)
{
  g_autoptr(GIRepository) repository = gi_repository_new ();
  g_assert_nonnull (repository);
}

static void
test_autoptr_typelib (RepositoryFixture *fx,
                      const void        *unused)
{
  g_autoptr(GITypelib) typelib = NULL;
  GError *local_error = NULL;

  typelib = gi_repository_require (fx->repository, "Gio", "2.0",
                                   GI_REPOSITORY_LOAD_FLAG_NONE, &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (typelib);

  /* gi_repository_require() doesn’t return a reference so let’s add one */
  gi_typelib_ref (typelib);
}

static void
test_autoptr_base_info (RepositoryFixture *fx,
                        const void        *unused)
{
  g_autoptr(GIBaseInfo) base_info = gi_repository_find_by_name (fx->repository, "Gio", "Resolver");
  g_assert_nonnull (base_info);
}

static void
test_autoptr_arg_info (RepositoryFixture *fx,
                       const void        *unused)
{
  GIObjectInfo *object_info = NULL;
  GIFunctionInfo *method_info = NULL;
  g_autoptr(GIArgInfo) arg_info = NULL;

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);
  method_info = gi_object_info_find_method (object_info, "get_property");
  g_assert_nonnull (method_info);
  arg_info = gi_callable_info_get_arg (GI_CALLABLE_INFO (method_info), 0);
  g_assert_nonnull (arg_info);

  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&object_info, gi_base_info_unref);
}

static void
test_autoptr_callable_info (RepositoryFixture *fx,
                            const void        *unused)
{
  g_autoptr(GICallableInfo) callable_info = GI_CALLABLE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "tls_server_connection_new"));
  g_assert_nonnull (callable_info);
}

static void
test_autoptr_callback_info (RepositoryFixture *fx,
                            const void        *unused)
{
  g_autoptr(GICallbackInfo) callback_info = GI_CALLBACK_INFO (gi_repository_find_by_name (fx->repository, "Gio", "AsyncReadyCallback"));
  g_assert_nonnull (callback_info);
}

static void
test_autoptr_constant_info (RepositoryFixture *fx,
                            const void        *unused)
{
  g_autoptr(GIConstantInfo) constant_info = GI_CONSTANT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "DBUS_METHOD_INVOCATION_HANDLED"));
  g_assert_nonnull (constant_info);
}

static void
test_autoptr_enum_info (RepositoryFixture *fx,
                        const void        *unused)
{
  g_autoptr(GIEnumInfo) enum_info = GI_ENUM_INFO (gi_repository_find_by_name (fx->repository, "Gio", "DBusError"));
  g_assert_nonnull (enum_info);
}

static void
test_autoptr_field_info (RepositoryFixture *fx,
                         const void        *unused)
{
  GIStructInfo *struct_info = NULL;
  g_autoptr(GIFieldInfo) field_info = NULL;

  struct_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "ActionEntry"));
  g_assert_nonnull (struct_info);
  field_info = gi_struct_info_find_field (struct_info, "name");
  g_assert_nonnull (field_info);

  g_clear_pointer (&struct_info, gi_base_info_unref);
}

static void
test_autoptr_flags_info (RepositoryFixture *fx,
                         const void        *unused)
{
  g_autoptr(GIFlagsInfo) flags_info = GI_FLAGS_INFO (gi_repository_find_by_name (fx->repository, "Gio", "AppInfoCreateFlags"));
  g_assert_nonnull (flags_info);
}

static void
test_autoptr_function_info (RepositoryFixture *fx,
                            const void        *unused)
{
  g_autoptr(GIFunctionInfo) function_info = GI_FUNCTION_INFO (gi_repository_find_by_name (fx->repository, "Gio", "tls_server_connection_new"));
  g_assert_nonnull (function_info);
}

static void
test_autoptr_interface_info (RepositoryFixture *fx,
                             const void        *unused)
{
  g_autoptr(GIInterfaceInfo) interface_info = GI_INTERFACE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "AsyncInitable"));
  g_assert_nonnull (interface_info);
}

static void
test_autoptr_object_info (RepositoryFixture *fx,
                          const void        *unused)
{
  g_autoptr(GIObjectInfo) object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "BufferedInputStream"));
  g_assert_nonnull (object_info);
}

static void
test_autoptr_property_info (RepositoryFixture *fx,
                            const void        *unused)
{
  GIObjectInfo *object_info = NULL;
  g_autoptr(GIPropertyInfo) property_info = NULL;

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "BufferedInputStream"));
  g_assert_nonnull (object_info);
  property_info = gi_object_info_get_property (object_info, 0);
  g_assert_nonnull (property_info);

  g_clear_pointer (&object_info, gi_base_info_unref);
}

static void
test_autoptr_registered_type_info (RepositoryFixture *fx,
                                   const void        *unused)
{
  g_autoptr(GIRegisteredTypeInfo) registered_type_info =
      GI_REGISTERED_TYPE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "SrvTarget"));
  g_assert_nonnull (registered_type_info);
}

static void
test_autoptr_signal_info (RepositoryFixture *fx,
                          const void        *unused)
{
  GIObjectInfo *object_info = NULL;
  g_autoptr(GISignalInfo) signal_info = NULL;

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "Cancellable"));
  g_assert_nonnull (object_info);
  signal_info = gi_object_info_find_signal (object_info, "cancelled");
  g_assert_nonnull (signal_info);

  g_clear_pointer (&object_info, gi_base_info_unref);
}

static void
test_autoptr_struct_info (RepositoryFixture *fx,
                          const void        *unused)
{
  g_autoptr(GIStructInfo) struct_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "DBusAnnotationInfo"));
  g_assert_nonnull (struct_info);
}

static void
test_autoptr_type_info (RepositoryFixture *fx,
                        const void        *unused)
{
  GIStructInfo *struct_info = NULL;
  GIFieldInfo *field_info = NULL;
  g_autoptr(GITypeInfo) type_info = NULL;

  struct_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "ActionEntry"));
  g_assert_nonnull (struct_info);
  field_info = gi_struct_info_find_field (struct_info, "name");
  g_assert_nonnull (field_info);
  type_info = gi_field_info_get_type_info (field_info);
  g_assert_nonnull (type_info);

  g_clear_pointer (&field_info, gi_base_info_unref);
  g_clear_pointer (&struct_info, gi_base_info_unref);
}

static void
test_autoptr_union_info (RepositoryFixture *fx,
                         const void        *unused)
{
  g_autoptr(GIUnionInfo) union_info = GI_UNION_INFO (gi_repository_find_by_name (fx->repository, "GLib", "DoubleIEEE754"));
  g_assert_nonnull (union_info);
}

static void
test_autoptr_value_info (RepositoryFixture *fx,
                         const void        *unused)
{
  GIEnumInfo *enum_info = NULL;
  g_autoptr(GIValueInfo) value_info = NULL;

  enum_info = GI_ENUM_INFO (gi_repository_find_by_name (fx->repository, "Gio", "ZlibCompressorFormat"));
  g_assert_nonnull (enum_info);
  value_info = gi_enum_info_get_value (enum_info, 0);
  g_assert_nonnull (value_info);

  g_clear_pointer (&enum_info, gi_base_info_unref);
}

static void
test_autoptr_vfunc_info (RepositoryFixture *fx,
                         const void        *unused)
{
  GIInterfaceInfo *interface_info = NULL;
  g_autoptr(GIVFuncInfo) vfunc_info = NULL;

  interface_info = GI_INTERFACE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "Action"));
  g_assert_nonnull (interface_info);
  vfunc_info = gi_interface_info_find_vfunc (interface_info, "activate");
  g_assert_nonnull (vfunc_info);

  g_clear_pointer (&interface_info, gi_base_info_unref);
}

static void
test_auto_arg_info (RepositoryFixture *fx,
                    const void        *unused)
{
  GIObjectInfo *object_info = NULL;
  GIFunctionInfo *method_info = NULL;
  g_auto(GIArgInfo) arg_info = { 0, };

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);
  method_info = gi_object_info_find_method (object_info, "get_property");
  g_assert_nonnull (method_info);
  gi_callable_info_load_arg (GI_CALLABLE_INFO (method_info), 0, &arg_info);
  g_assert_true (GI_IS_ARG_INFO (&arg_info));

  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&object_info, gi_base_info_unref);
}

static void
test_auto_type_info (RepositoryFixture *fx,
                     const void        *unused)
{
  GIObjectInfo *object_info = NULL;
  GIFunctionInfo *method_info = NULL;
  GIArgInfo *arg_info = NULL;
  g_auto(GITypeInfo) type_info = { 0, };

  object_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Object"));
  g_assert_nonnull (object_info);
  method_info = gi_object_info_find_method (object_info, "get_property");
  g_assert_nonnull (method_info);
  arg_info = gi_callable_info_get_arg (GI_CALLABLE_INFO (method_info), 0);
  g_assert_nonnull (arg_info);
  gi_arg_info_load_type_info (arg_info, &type_info);
  g_assert_true (GI_IS_TYPE_INFO (&type_info));

  g_clear_pointer (&arg_info, gi_base_info_unref);
  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&object_info, gi_base_info_unref);
}

static void
test_auto_function_invoker (RepositoryFixture *fx,
                            const void        *unused)
{
  GIFunctionInfo *function_info = NULL;
  g_auto(GIFunctionInvoker) invoker = { 0, };
  GError *local_error = NULL;

  function_info = GI_FUNCTION_INFO (gi_repository_find_by_name (fx->repository, "Gio", "tls_server_connection_new"));
  g_assert_nonnull (function_info);
  gi_function_info_prep_invoker (function_info, &invoker, &local_error);
  g_assert_no_error (local_error);

  g_clear_pointer (&function_info, gi_base_info_unref);
}

int
main (int   argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/autoptr/repository", test_autoptr_repository, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/typelib", test_autoptr_typelib, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/base-info", test_autoptr_base_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/arg-info", test_autoptr_arg_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/callable-info", test_autoptr_callable_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/callback-info", test_autoptr_callback_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/constant-info", test_autoptr_constant_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/enum-info", test_autoptr_enum_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/field-info", test_autoptr_field_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/flags-info", test_autoptr_flags_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/function-info", test_autoptr_function_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/interface-info", test_autoptr_interface_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/object-info", test_autoptr_object_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/property-info", test_autoptr_property_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/registered-type-info", test_autoptr_registered_type_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/signal-info", test_autoptr_signal_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/struct-info", test_autoptr_struct_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/type-info", test_autoptr_type_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/union-info", test_autoptr_union_info, &typelib_load_spec_glib);
  /* no easy way to test GIUnresolvedInfo */
  ADD_REPOSITORY_TEST ("/autoptr/value-info", test_autoptr_value_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/autoptr/vfunc-info", test_autoptr_vfunc_info, &typelib_load_spec_gio);

  ADD_REPOSITORY_TEST ("/auto/arg-info", test_auto_arg_info, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/auto/type-info", test_auto_type_info, &typelib_load_spec_gio);

  ADD_REPOSITORY_TEST ("/auto/function-invoker", test_auto_function_invoker, &typelib_load_spec_gio);

  return g_test_run ();
}
