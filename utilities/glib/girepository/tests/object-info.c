/*
 * Copyright 2024 Philip Chimento <philip.chimento@gmail.com>
 * Copyright 2024 GNOME Foundation
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
test_object_info_find_method_using_interfaces (RepositoryFixture *fx,
                                               const void        *unused)
{
  GIObjectInfo *class_info = NULL;
  GIFunctionInfo *method_info = NULL;
  GIBaseInfo *declarer_info = NULL;

  class_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "DBusProxy"));
  g_assert_nonnull (class_info);

  method_info = gi_object_info_find_method_using_interfaces (class_info, "init", &declarer_info);

  g_assert_nonnull (declarer_info);
  g_assert_cmpstr (gi_base_info_get_namespace (declarer_info), ==, "Gio");
  g_assert_cmpstr (gi_base_info_get_name (declarer_info), ==, "Initable");
  g_assert_true (GI_IS_INTERFACE_INFO (declarer_info));

  g_clear_pointer (&class_info, gi_base_info_unref);
  g_clear_pointer (&method_info, gi_base_info_unref);
  g_clear_pointer (&declarer_info, gi_base_info_unref);
}

static void
test_object_info_find_vfunc_using_interfaces (RepositoryFixture *fx,
                                              const void        *unused)
{
  GIObjectInfo *class_info = NULL;
  GIVFuncInfo *vfunc_info = NULL;
  GIBaseInfo *declarer_info = NULL;

  class_info = GI_OBJECT_INFO (gi_repository_find_by_name (fx->repository, "Gio", "Application"));
  g_assert_nonnull (class_info);

  vfunc_info = gi_object_info_find_vfunc_using_interfaces (class_info, "after_emit", &declarer_info);

  g_assert_nonnull (declarer_info);
  g_assert_cmpstr (gi_base_info_get_namespace (declarer_info), ==, "Gio");
  g_assert_cmpstr (gi_base_info_get_name (declarer_info), ==, "Application");
  g_assert_true (GI_IS_OBJECT_INFO (declarer_info));

  g_clear_pointer (&class_info, gi_base_info_unref);
  g_clear_pointer (&vfunc_info, gi_base_info_unref);
  g_clear_pointer (&declarer_info, gi_base_info_unref);
}

int
main (int argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/object-info/find-method-using-interfaces", test_object_info_find_method_using_interfaces, &typelib_load_spec_gio);
  ADD_REPOSITORY_TEST ("/object-info/find-vfunc-using-interfaces", test_object_info_find_vfunc_using_interfaces, &typelib_load_spec_gio);

  return g_test_run ();
}
