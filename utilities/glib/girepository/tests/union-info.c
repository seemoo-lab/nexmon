/*
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
test_basic (RepositoryFixture *fx,
            const void        *unused)
{
  GIUnionInfo *double_info = NULL;
  GIFieldInfo *field_info = NULL;
  size_t offset = 123;

  g_test_summary ("Test basic properties of GIUnionInfo");

  double_info = GI_UNION_INFO (gi_repository_find_by_name (fx->repository, "GLib", "DoubleIEEE754"));
  g_assert_nonnull (double_info);

  g_assert_cmpuint (gi_union_info_get_n_fields (double_info), ==, 1);

  field_info = gi_union_info_get_field (double_info, 0);
  g_assert_true (GI_IS_FIELD_INFO (field_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (field_info)), ==, "v_double");
  g_clear_pointer (&field_info, gi_base_info_unref);

  g_assert_cmpuint (gi_union_info_get_n_methods (double_info), ==, 0);
  g_assert_null (gi_union_info_find_method (double_info, "not_exist"));

  g_assert_false (gi_union_info_is_discriminated (double_info));
  g_assert_false (gi_union_info_get_discriminator_offset (double_info, &offset));
  g_assert_cmpuint (offset, ==, 0);
  g_assert_null (gi_union_info_get_discriminator_type (double_info));
  g_assert_null (gi_union_info_get_discriminator (double_info, 0));

  g_assert_cmpuint (gi_union_info_get_size (double_info), ==, 8);
  g_assert_cmpuint (gi_union_info_get_alignment (double_info), ==, G_ALIGNOF (GDoubleIEEE754));

  g_assert_null (gi_union_info_get_copy_function_name (double_info));
  g_assert_null (gi_union_info_get_free_function_name (double_info));

  g_clear_pointer (&double_info, gi_base_info_unref);
}

static void
test_methods (RepositoryFixture *fx,
              const void        *unused)
{
  GIUnionInfo *mutex_info = NULL;
  GIFunctionInfo *method_info = NULL;

  g_test_summary ("Test retrieving methods from GIUnionInfo");

  mutex_info = GI_UNION_INFO (gi_repository_find_by_name (fx->repository, "GLib", "Mutex"));
  g_assert_nonnull (mutex_info);

  g_assert_cmpuint (gi_union_info_get_n_methods (mutex_info), ==, 5);

  method_info = gi_union_info_get_method (mutex_info, 0);
  g_assert_true (GI_IS_FUNCTION_INFO (method_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (method_info)), ==, "clear");
  g_clear_pointer (&method_info, gi_base_info_unref);

  method_info = gi_union_info_find_method (mutex_info, "trylock");
  g_assert_true (GI_IS_FUNCTION_INFO (method_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (method_info)), ==, "trylock");
  g_clear_pointer (&method_info, gi_base_info_unref);

  g_clear_pointer (&mutex_info, gi_base_info_unref);
}

int
main (int argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/union-info/basic", test_basic, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/union-info/methods", test_methods, &typelib_load_spec_glib);

  return g_test_run ();
}
