/*
 * Copyright 2008 litl, LLC
 * Copyright 2014 Simon Feltman <sfeltman@gnome.org>
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
 */

#include "config.h"

#include "girepository.h"
#include "test-common.h"

static void
test_invoke_gerror (RepositoryFixture *fx,
                    const void *unused)
{
  GIFunctionInfo *func_info = NULL;
  GIArgument in_arg[1];
  GIArgument ret_arg;
  GError *error = NULL;
  gboolean invoke_return;

  g_test_summary ("Test invoking a function that throws a GError");

  func_info = GI_FUNCTION_INFO (gi_repository_find_by_name (fx->repository, "GLib", "file_read_link"));
  g_assert_nonnull (func_info);
  g_assert_true (gi_callable_info_can_throw_gerror (GI_CALLABLE_INFO (func_info)));

  in_arg[0].v_string = g_strdup ("non-existent-file/hope");
  invoke_return = gi_function_info_invoke (func_info, in_arg, 1, NULL, 0, &ret_arg, &error);
  g_clear_pointer (&in_arg[0].v_string, g_free);

  g_assert_false (invoke_return);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);

  g_clear_error (&error);
  g_clear_pointer (&func_info, gi_base_info_unref);
}

static void
test_vfunc_can_throw_gerror (RepositoryFixture *fx,
                             const void *unused)
{
  GIInterfaceInfo *interface_info = NULL;
  GIFunctionInfo *invoker_info = NULL;
  GIVFuncInfo *vfunc_info = NULL;

  g_test_summary ("Test gi_callable_info_can_throw_gerror() on a vfunc");

  interface_info = GI_INTERFACE_INFO (gi_repository_find_by_name (fx->repository, "Gio", "AppInfo"));
  g_assert_nonnull (interface_info);

  invoker_info = gi_interface_info_find_method (interface_info, "launch");
  g_assert_nonnull (invoker_info);
  g_assert_true (gi_callable_info_can_throw_gerror (GI_CALLABLE_INFO (invoker_info)));

  vfunc_info = gi_interface_info_find_vfunc (interface_info, "launch");
  g_assert_nonnull (vfunc_info);
  g_assert_true (gi_callable_info_can_throw_gerror (GI_CALLABLE_INFO (vfunc_info)));

  g_clear_pointer (&vfunc_info, gi_base_info_unref);
  g_clear_pointer (&invoker_info, gi_base_info_unref);
  g_clear_pointer (&interface_info, gi_base_info_unref);
}

static void
test_callback_can_throw_gerror (RepositoryFixture *fx,
                                const void *unused)
{
  GIStructInfo *class_info = NULL;
  GIFieldInfo *field_info = NULL;
  GITypeInfo *field_type = NULL;
  GICallbackInfo *callback_info = NULL;

  g_test_summary ("Test gi_callable_info_can_throw_gerror() on a callback");

  class_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "AppInfoIface"));
  g_assert_nonnull (class_info);

  field_info = gi_struct_info_find_field (class_info, "launch");
  g_assert_nonnull (field_info);
  g_assert_true (GI_IS_FIELD_INFO (field_info));

  field_type = gi_field_info_get_type_info (field_info);
  g_assert_nonnull (field_type);
  g_assert_true (GI_IS_TYPE_INFO (field_type));
  g_assert_cmpint (gi_type_info_get_tag (field_type), ==, GI_TYPE_TAG_INTERFACE);

  callback_info = GI_CALLBACK_INFO (gi_type_info_get_interface (field_type));
  g_assert_nonnull (callback_info);
  g_assert (gi_callable_info_can_throw_gerror (GI_CALLABLE_INFO (callback_info)));

  g_clear_pointer (&callback_info, gi_base_info_unref);
  g_clear_pointer (&field_type, gi_base_info_unref);
  g_clear_pointer (&field_info, gi_base_info_unref);
  g_clear_pointer (&class_info, gi_base_info_unref);
}

int
main (int argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/throws/invoke-gerror", test_invoke_gerror, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/throws/vfunc-can-throw-gerror", test_vfunc_can_throw_gerror, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/throws/callback-can-throw-gerror", test_callback_can_throw_gerror, &typelib_load_spec_gio);

  return g_test_run ();
}
