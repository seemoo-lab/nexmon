/*
 * Copyright 2025 Philip Chimento <philip.chimento@gmail.com>
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
#include "glib.h"
#include "test-common.h"

static void
test_basic_struct_field (RepositoryFixture *fx,
                         const void *unused)
{
  GIStructInfo *struct_info = NULL;
  GIFieldInfo *field_info = NULL;
  GITypeInfo *type_info = NULL;
  GIFieldInfoFlags flags;

  g_test_summary ("Test basic properties of a GIFieldInfo from a C struct");

  struct_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "GLib", "DebugKey"));
  g_assert_nonnull (struct_info);

  field_info = gi_struct_info_get_field (struct_info, 0);
  g_assert_true (GI_IS_FIELD_INFO (field_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (field_info)), ==, "key");

  flags = gi_field_info_get_flags (field_info);
  g_assert_cmpuint (flags, ==, GI_FIELD_IS_READABLE | GI_FIELD_IS_WRITABLE);

  /* Guaranteed across platforms, because it's the first field */
  g_assert_cmpuint (gi_field_info_get_offset (field_info), ==, 0);

  type_info = gi_field_info_get_type_info (field_info);
  g_assert_cmpuint (gi_type_info_get_tag (type_info), ==, GI_TYPE_TAG_UTF8);
  g_assert_true (gi_type_info_is_pointer (type_info));

  g_clear_pointer (&type_info, gi_base_info_unref);
  g_clear_pointer (&field_info, gi_base_info_unref);
  g_clear_pointer (&struct_info, gi_base_info_unref);
}

static void
test_basic_union_field (RepositoryFixture *fx,
                        const void *unused)
{
  GIUnionInfo *union_info = NULL;
  GIFieldInfo *field_info = NULL;
  GITypeInfo *type_info = NULL;
  GIFieldInfoFlags flags;

  g_test_summary ("Test basic properties of a GIFieldInfo from a C union");

  union_info = GI_UNION_INFO (gi_repository_find_by_name (fx->repository, "GLib", "DoubleIEEE754"));
  g_assert_nonnull (union_info);

  field_info = gi_union_info_get_field (union_info, 0);
  g_assert_true (GI_IS_FIELD_INFO (field_info));
  g_assert_cmpstr (gi_base_info_get_name (GI_BASE_INFO (field_info)), ==, "v_double");

  flags = gi_field_info_get_flags (field_info);
  g_assert_cmpuint (flags, ==, GI_FIELD_IS_READABLE | GI_FIELD_IS_WRITABLE);

  /* Guaranteed across platforms, because union offsets are always 0 */
  g_assert_cmpuint (gi_field_info_get_offset (field_info), ==, 0);

  type_info = gi_field_info_get_type_info (field_info);
  g_assert_cmpuint (gi_type_info_get_tag (type_info), ==, GI_TYPE_TAG_DOUBLE);
  g_assert_false (gi_type_info_is_pointer (type_info));

  g_clear_pointer (&type_info, gi_base_info_unref);
  g_clear_pointer (&field_info, gi_base_info_unref);
  g_clear_pointer (&union_info, gi_base_info_unref);
}

static void
test_read_write_struct_field (RepositoryFixture *fx,
                              const void *unused)
{
  GIStructInfo *struct_info = NULL;
  GIFieldInfo *field_info = NULL;
  GDebugKey instance;
  GIArgument arg;
  gboolean ok;

  g_test_summary ("Test reading and writing of a GIFieldInfo from a C union");

  struct_info = GI_STRUCT_INFO (gi_repository_find_by_name (fx->repository, "GLib", "DebugKey"));
  g_assert_nonnull (struct_info);

  field_info = gi_struct_info_get_field (struct_info, 1);
  g_assert_nonnull (field_info);

  instance.value = 0xfeed;
  ok = gi_field_info_get_field (field_info, &instance, &arg);
  g_assert_true (ok);
  g_assert_cmpuint (arg.v_uint, ==, 0xfeed);

  arg.v_uint = 0x6502;
  ok = gi_field_info_set_field (field_info, &instance, &arg);
  g_assert_true (ok);
  g_assert_cmpuint (instance.value, ==, 0x6502);

  g_clear_pointer (&field_info, gi_base_info_unref);
  g_clear_pointer (&struct_info, gi_base_info_unref);
}

static void
test_read_write_union_field (RepositoryFixture *fx,
                             const void *unused)
{
  GIUnionInfo *union_info = NULL;
  GIFieldInfo *field_info = NULL;
  GDoubleIEEE754 instance;
  GIArgument arg;
  gboolean ok;

  g_test_summary ("Test reading and writing of a GIFieldInfo from a C union");

  union_info = GI_UNION_INFO (gi_repository_find_by_name (fx->repository, "GLib", "DoubleIEEE754"));
  g_assert_nonnull (union_info);

  field_info = gi_union_info_get_field (union_info, 0);
  g_assert_nonnull (field_info);

  instance.v_double = G_PI;
  ok = gi_field_info_get_field (field_info, &instance, &arg);
  g_assert_true (ok);
  g_assert_cmpfloat (arg.v_double, ==, G_PI);

  arg.v_double = G_E;
  ok = gi_field_info_set_field (field_info, &instance, &arg);
  g_assert_true (ok);
  g_assert_cmpfloat (instance.v_double, ==, G_E);

  g_clear_pointer (&field_info, gi_base_info_unref);
  g_clear_pointer (&union_info, gi_base_info_unref);
}

int
main (int argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/field-info/basic-struct-field", test_basic_struct_field, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/field-info/basic-union-field", test_basic_union_field, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/field-info/read-write-struct-field", test_read_write_struct_field, &typelib_load_spec_glib);
  ADD_REPOSITORY_TEST ("/field-info/read-write-union-field", test_read_write_union_field, &typelib_load_spec_glib);

  return g_test_run ();
}
