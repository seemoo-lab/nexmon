/*
 * Copyright 2024 GNOME Foundation, Inc.
 * Copyright 2024 Evan Welsh
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
 * Author: Evan Welsh <contact@evanwelsh.com>
 * Author: Philip Chimento <philip.chimento@gmail.com>
 */

#include "girepository.h"
#include "girffi.h"
#include "test-common.h"

static void
test_callable_get_sync_function_for_async_function (RepositoryFixture *fx,
                                                    const void        *unused)
{
  GIBaseInfo *info;

  info = gi_repository_find_by_name (fx->repository, "Gio", "File");
  g_assert_nonnull (info);
  g_assert_true (GI_IS_INTERFACE_INFO (info));

  GICallableInfo *callable_info = (GICallableInfo *) gi_interface_info_find_method ((GIInterfaceInfo *) info, "load_contents_async");
  g_assert_nonnull (callable_info);

  g_assert_true (gi_callable_info_is_async (callable_info));

  GICallableInfo *sync_info = gi_callable_info_get_sync_function (callable_info);
  g_assert_nonnull (sync_info);

  GICallableInfo *finish_info = gi_callable_info_get_finish_function (callable_info);
  g_assert_nonnull (finish_info);

  g_assert_cmpstr (gi_base_info_get_name ((GIBaseInfo *) sync_info), ==, "load_contents");
  g_assert_cmpstr (gi_base_info_get_name ((GIBaseInfo *) finish_info), ==, "load_contents_finish");

  GICallableInfo *async_info = gi_callable_info_get_async_function (sync_info);

  g_assert_cmpstr (gi_base_info_get_name ((GIBaseInfo *) async_info), ==, "load_contents_async");

  gi_base_info_unref (async_info);
  gi_base_info_unref (sync_info);
  gi_base_info_unref (finish_info);
  gi_base_info_unref (callable_info);
  gi_base_info_unref (info);
}

static void
test_callable_get_async_function_for_sync_function (RepositoryFixture *fx,
                                                    const void        *unused)
{
  GIBaseInfo *info;

  info = gi_repository_find_by_name (fx->repository, "Gio", "File");
  g_assert_nonnull (info);
  g_assert_true (g_type_is_a (G_TYPE_FROM_INSTANCE (info), gi_interface_info_get_type ()));

  GICallableInfo *callable_info = (GICallableInfo *) gi_interface_info_find_method ((GIInterfaceInfo *) info, "load_contents");

  {
    GICallableInfo *async_func = gi_callable_info_get_async_function (callable_info);
    g_assert_nonnull (async_func);

    GICallableInfo *finish_func = gi_callable_info_get_finish_function (callable_info);
    g_assert_null (finish_func);

    GICallableInfo *sync_func = gi_callable_info_get_sync_function (callable_info);
    g_assert_null (sync_func);

    gi_base_info_unref ((GIBaseInfo *) async_func);
  }

  GICallableInfo *async_info = gi_callable_info_get_async_function (callable_info);

  {
    GICallableInfo *async_func = gi_callable_info_get_async_function (async_info);
    g_assert_null (async_func);

    GICallableInfo *finish_func = gi_callable_info_get_finish_function (async_info);
    g_assert_nonnull (finish_func);

    GICallableInfo *sync_func = gi_callable_info_get_sync_function (async_info);
    g_assert_nonnull (sync_func);

    gi_base_info_unref ((GIBaseInfo *) finish_func);
    gi_base_info_unref ((GIBaseInfo *) sync_func);
  }

  g_assert_cmpstr (gi_base_info_get_name ((GIBaseInfo *) async_info), ==, "load_contents_async");

  GICallableInfo *sync_info = gi_callable_info_get_sync_function (async_info);

  {
    GICallableInfo *async_func = gi_callable_info_get_async_function (sync_info);
    g_assert_nonnull (async_func);

    GICallableInfo *finish_func = gi_callable_info_get_finish_function (sync_info);
    g_assert_null (finish_func);

    GICallableInfo *sync_func = gi_callable_info_get_sync_function (sync_info);
    g_assert_null (sync_func);

    gi_base_info_unref ((GIBaseInfo *) async_func);
  }

  g_assert_cmpstr (gi_base_info_get_name ((GIBaseInfo *) sync_info), ==, "load_contents");

  gi_base_info_unref ((GIBaseInfo *) async_info);
  gi_base_info_unref ((GIBaseInfo *) sync_info);
  gi_base_info_unref ((GIBaseInfo *) callable_info);
  gi_base_info_unref (info);
}

static void
test_callable_info_is_method (RepositoryFixture *fx,
                              const void *unused)
{
  GIBaseInfo *info;
  GIFunctionInfo *func_info;
  GIVFuncInfo *vfunc_info;
  GISignalInfo *sig_info;
  GIBaseInfo *cb_info;

  info = gi_repository_find_by_name (fx->repository, "Gio", "ActionGroup");
  g_assert_nonnull (info);

  func_info = gi_interface_info_find_method (GI_INTERFACE_INFO (info), "action_added");
  g_assert_nonnull (func_info);

  g_assert_true (gi_callable_info_is_method (GI_CALLABLE_INFO (func_info)));

  vfunc_info = gi_interface_info_find_vfunc (GI_INTERFACE_INFO (info), "action_added");
  g_assert_nonnull (vfunc_info);

  g_assert_true (gi_callable_info_is_method (GI_CALLABLE_INFO (vfunc_info)));

  sig_info = gi_interface_info_find_signal (GI_INTERFACE_INFO (info), "action-added");
  g_assert_nonnull (sig_info);

  g_assert_true (gi_callable_info_is_method (GI_CALLABLE_INFO (sig_info)));

  cb_info = gi_repository_find_by_name (fx->repository, "Gio", "AsyncReadyCallback");
  g_assert_nonnull (cb_info);

  g_assert_false (gi_callable_info_is_method (GI_CALLABLE_INFO (cb_info)));

  gi_base_info_unref (info);
  gi_base_info_unref ((GIBaseInfo *) func_info);
  gi_base_info_unref ((GIBaseInfo *) vfunc_info);
  gi_base_info_unref ((GIBaseInfo *) sig_info);
  gi_base_info_unref (cb_info);
}

static void
test_callable_info_static_method (RepositoryFixture *fx,
                                  const void *unused)
{
  GIBaseInfo *info;
  GIFunctionInfo *func_info;

  info = gi_repository_find_by_name (fx->repository, "Gio", "Application");
  g_assert_nonnull (info);

  func_info = gi_object_info_find_method (GI_OBJECT_INFO (info), "get_default");
  g_assert_nonnull (func_info);

  g_assert_false (gi_callable_info_is_method (GI_CALLABLE_INFO (func_info)));

  gi_base_info_unref (info);
  gi_base_info_unref ((GIBaseInfo *) func_info);
}

static void
test_callable_info_static_vfunc (RepositoryFixture *fx,
                                 const void *unused)
{
  GIBaseInfo *info;
  GIVFuncInfo *vfunc_info;

  g_test_bug ("https://gitlab.gnome.org/GNOME/gobject-introspection/-/merge_requests/361");

  info = gi_repository_find_by_name (fx->repository, "Gio", "Icon");
  g_assert_nonnull (info);

  vfunc_info = gi_interface_info_find_vfunc (GI_INTERFACE_INFO (info), "from_tokens");
  if (!vfunc_info)
    {
      g_test_skip ("g-ir-scanner is not new enough");
      gi_base_info_unref (info);
      return;
    }
  g_assert_nonnull (vfunc_info);

  g_assert_false (gi_callable_info_is_method (GI_CALLABLE_INFO (vfunc_info)));

  gi_base_info_unref (info);
  gi_base_info_unref ((GIBaseInfo *) vfunc_info);
}

static ffi_cif cif;

static void
compare_func_callback (ffi_cif *passed_cif,
                       void *ret,
                       void **args,
                       void *user_data)
{
  ffi_sarg *return_location = ret;
  unsigned *call_count = user_data;
  int arg1 = GPOINTER_TO_INT (*(void **) args[0]);
  int arg2 = GPOINTER_TO_INT (*(void **) args[1]);

  /* cif is not needed in this simple test, but check that it is what the
   * documentation says it is */
  g_assert_true (passed_cif == &cif);

  *return_location = arg1 - arg2;
  ++*call_count;
}

static void
test_callable_info_native_address (RepositoryFixture *fx,
                                   const void *unused)
{
  GICallableInfo *compare_func_info;
  GList *list = NULL;
  ffi_closure *closure;
  unsigned call_count = 0;
  void *compare_func;
  int raw_result;

  compare_func_info = GI_CALLABLE_INFO (gi_repository_find_by_name (fx->repository, "GLib", "CompareFunc"));
  g_assert_true (GI_IS_CALLBACK_INFO (compare_func_info));

  /* Create an unsorted list */
  list = g_list_prepend (list, GINT_TO_POINTER (1));
  list = g_list_prepend (list, GINT_TO_POINTER (3));
  list = g_list_prepend (list, GINT_TO_POINTER (2));

  closure = gi_callable_info_create_closure (compare_func_info, &cif, compare_func_callback, &call_count);

  /* Check that FFI closure information is prepared correctly */
  g_assert_true (cif.rtype == &ffi_type_sint);
  g_assert_cmpuint (cif.nargs, ==, 2);
  g_assert_true (cif.arg_types[0] == &ffi_type_pointer);
  g_assert_true (cif.arg_types[1] == &ffi_type_pointer);

  compare_func = gi_callable_info_get_closure_native_address (compare_func_info, closure);

  /* Sort the list, passing the generated closure as the callback function pointer */
  list = g_list_sort (list, compare_func);

  g_assert_cmpuint (call_count, >, 0);

  /* Check that the list is now sorted */
  g_assert_cmpint (GPOINTER_TO_INT (list->data), ==, 1);
  g_assert_cmpint (GPOINTER_TO_INT (list->next->data), ==, 2);
  g_assert_cmpint (GPOINTER_TO_INT (list->next->next->data), ==, 3);
  g_assert_null (list->next->next->next);

  /* Test invoking the closure directly */
  raw_result = ((int (*) (const void *, const void *)) compare_func) (GINT_TO_POINTER (6), GINT_TO_POINTER (7));
  g_assert_cmpint (raw_result, <, 0);

  g_list_free (list);
  gi_callable_info_destroy_closure (compare_func_info, closure);
  gi_base_info_unref (GI_BASE_INFO (compare_func_info));
}

#ifdef G_OS_UNIX
static void
test_callable_info_platform_unix_is_method (RepositoryFixture *fx,
                                            const void *unused)
{
  GIBaseInfo *info;
  GIFunctionInfo *func_info;

  g_test_message ("Checking DesktopAppInfo in Gio");
  info = gi_repository_find_by_name (fx->repository, "Gio", "DesktopAppInfo");
  g_assert_null (info);

  g_test_message ("Checking DesktopAppInfo in GioUnix");
  info = gi_repository_find_by_name (fx->repository, "GioUnix", "DesktopAppInfo");
  g_assert_nonnull (info);

  /* Must provide Gio.DesktopAppInfo methods */
  func_info = gi_object_info_find_method (GI_OBJECT_INFO (info), "has_key");
  g_assert_true (gi_callable_info_is_method (GI_CALLABLE_INFO (func_info)));
  g_assert_nonnull (func_info);
  g_clear_pointer (&func_info, gi_base_info_unref);

  gi_base_info_unref (info);
}
#endif

int
main (int argc, char **argv)
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/callable-info/sync-function", test_callable_get_sync_function_for_async_function, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/callable-info/async-function", test_callable_get_async_function_for_sync_function, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/callable-info/is-method", test_callable_info_is_method, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/callable-info/static-method", test_callable_info_static_method, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/callable-info/static-vfunc", test_callable_info_static_vfunc, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/callable-info/native-address", test_callable_info_native_address, &typelib_load_spec_glib);

#ifdef G_OS_UNIX
  ADD_REPOSITORY_TEST ("/callable-info/platform/unix/is-method", test_callable_info_platform_unix_is_method, &typelib_load_spec_gio);
#endif

  return g_test_run ();
}
