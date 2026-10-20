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
test_boxed (RepositoryFixture *fx,
            const void        *unused)
{
  const struct
    {
      const char *name;
      GType expect_info_type;
      gboolean expect_nonnull_gtype_info;
      gboolean expect_is_gtype_struct;
      gboolean expect_boxed;
    }
  types[] =
    {
      {
        /* POD struct */
        .name = "CClosure",
        .expect_info_type = GI_TYPE_STRUCT_INFO,
        .expect_nonnull_gtype_info = FALSE,
        .expect_is_gtype_struct = FALSE,
        .expect_boxed = FALSE,
      },
      {
        /* POD union */
        .name = "TypeCValue",
        .expect_info_type = GI_TYPE_UNION_INFO,
        .expect_nonnull_gtype_info = FALSE,
        .expect_is_gtype_struct = FALSE,
        .expect_boxed = FALSE,
      },
      {
        /* struct for a different non-boxed GType */
        .name = "InitiallyUnownedClass",
        .expect_info_type = GI_TYPE_STRUCT_INFO,
        .expect_nonnull_gtype_info = FALSE,
        .expect_is_gtype_struct = TRUE,
        .expect_boxed = FALSE,
      },
      {
        /* boxed struct */
        .name = "BookmarkFile",
        .expect_info_type = GI_TYPE_STRUCT_INFO,
        .expect_nonnull_gtype_info = TRUE,
        .expect_is_gtype_struct = FALSE,
        .expect_boxed = TRUE,
      },
      {
        /* boxed struct */
        .name = "Closure",
        .expect_info_type = GI_TYPE_STRUCT_INFO,
        .expect_nonnull_gtype_info = TRUE,
        .expect_is_gtype_struct = FALSE,
        .expect_boxed = TRUE,
      },
      {
        /* non-boxed GType */
        .name = "Object",
        .expect_info_type = GI_TYPE_OBJECT_INFO,
        .expect_nonnull_gtype_info = TRUE,
        .expect_is_gtype_struct = FALSE,
        .expect_boxed = FALSE,
      },
    };

  g_test_summary ("Test various boxed and non-boxed types for GIRegisteredTypeInfo");

  for (size_t i = 0; i < G_N_ELEMENTS (types); i++)
    {
      GIRegisteredTypeInfo *type_info = GI_REGISTERED_TYPE_INFO (gi_repository_find_by_name (fx->repository, "GObject", types[i].name));
      g_assert_nonnull (type_info);

      g_test_message ("Expecting %s to %s", types[i].name, types[i].expect_boxed ? "be boxed" : "not be boxed");

      g_assert_cmpuint (G_TYPE_FROM_INSTANCE (type_info), ==, types[i].expect_info_type);

      if (types[i].expect_nonnull_gtype_info)
        {
          g_assert_nonnull (gi_registered_type_info_get_type_name (type_info));
          g_assert_nonnull (gi_registered_type_info_get_type_init_function_name (type_info));
        }
      else
        {
          g_assert_null (gi_registered_type_info_get_type_name (type_info));
          g_assert_null (gi_registered_type_info_get_type_init_function_name (type_info));
        }

      if (GI_IS_STRUCT_INFO (type_info))
        {
          if (types[i].expect_is_gtype_struct)
            g_assert_true (gi_struct_info_is_gtype_struct (GI_STRUCT_INFO (type_info)));
          else
            g_assert_false (gi_struct_info_is_gtype_struct (GI_STRUCT_INFO (type_info)));
        }

      if (types[i].expect_boxed)
        g_assert_true (gi_registered_type_info_is_boxed (type_info));
      else
        g_assert_false (gi_registered_type_info_is_boxed (type_info));

      g_clear_pointer (&type_info, gi_base_info_unref);
    }
}

int
main (int argc,
      char *argv[])
{
  repository_init (&argc, &argv);

  ADD_REPOSITORY_TEST ("/registered-type-info/boxed", test_boxed, &typelib_load_spec_gobject);

  return g_test_run ();
}
