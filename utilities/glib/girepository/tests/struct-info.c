/*
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
test_field_iterators (RepositoryFixture *fx,
                      const void *unused)
{
  GIStructInfo *class_info = NULL;
  GIFieldInfo *field_info = NULL;
  unsigned ix;

  g_test_summary ("Test iterating through a struct's fields with gi_struct_info_get_field()");

  class_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "ObjectClass"));
  g_assert_nonnull (class_info);

  for (ix = 0; ix < gi_struct_info_get_n_fields (class_info); ix++)
    {
      const char *field_name = NULL;
      GIFieldInfo *found = NULL;

      field_info = gi_struct_info_get_field (class_info, ix);
      g_assert_nonnull (field_info);

      field_name = gi_base_info_get_name (GI_BASE_INFO (field_info));
      g_assert_nonnull (field_name);

      found = gi_struct_info_find_field (class_info, field_name);
      g_assert_nonnull (found);
      g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (found)), ==, field_name);

      g_clear_pointer (&found, gi_base_info_unref);
      g_clear_pointer (&field_info, gi_base_info_unref);
    }

  field_info = gi_struct_info_find_field (class_info, "not_a_real_field_name");
  g_assert_null (field_info);

  g_clear_pointer (&class_info, gi_base_info_unref);
}

static void
test_size_of_gvalue (RepositoryFixture *fx,
                     const void *unused)
{
  GIStructInfo *struct_info;

  g_test_summary ("Test that gi_struct_info_get_size() reports the correct sizeof GValue");

  struct_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "Value"));
  g_assert_nonnull (struct_info);

  g_assert_cmpuint (gi_struct_info_get_size (struct_info), ==, sizeof (GValue));

  g_clear_pointer (&struct_info, gi_base_info_unref);
}

static void
test_is_pointer_for_struct_method_arg (RepositoryFixture *fx,
                                       const void *unused)
{
  GIStructInfo *variant_info = NULL;
  GIFunctionInfo *equal_info = NULL;
  GIArgInfo *arg_info = NULL;
  GITypeInfo *type_info = NULL;

  g_test_summary ("Test that a struct method reports the correct type with gi_type_info_is_pointer()");

  variant_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "GLib", "Variant"));
  g_assert_nonnull (variant_info);

  equal_info = gi_struct_info_find_method (variant_info, "equal");
  g_assert_nonnull (equal_info);

  arg_info = gi_callable_info_get_arg (GI_CALLABLE_INFO (equal_info), 0);
  g_assert_nonnull (arg_info);

  type_info = gi_arg_info_get_type_info (arg_info);
  g_assert_nonnull (type_info);
  g_assert_true (gi_type_info_is_pointer (type_info));

  g_clear_pointer (&type_info, gi_base_info_unref);
  g_clear_pointer (&arg_info, gi_base_info_unref);
  g_clear_pointer (&equal_info, gi_base_info_unref);
  g_clear_pointer (&variant_info, gi_base_info_unref);
}

static void
test_boxed (RepositoryFixture *fx,
            const void        *unused)
{
  GIStructInfo *struct_info = NULL;

  g_test_summary ("Test that a boxed struct is recognised as such");

  struct_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "GObject", "BookmarkFile"));
  g_assert_nonnull (struct_info);
  g_assert_true (gi_registered_type_info_is_boxed (GI_REGISTERED_TYPE_INFO (struct_info)));

  g_clear_pointer (&struct_info, gi_base_info_unref);
}

int
main (int argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/struct-info/field-iterators", test_field_iterators, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/struct-info/sizeof-gvalue", test_size_of_gvalue, &typelib_load_spec_gobject);
  ADD_REPOSITORY_TEST ("/struct-info/is-pointer-for-struct-method-arg", test_is_pointer_for_struct_method_arg, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/struct-info/boxed", test_boxed, &typelib_load_spec_gobject);

  return g_test_run ();
}
