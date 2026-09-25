/*
 * Copyright 2023 Canonical Ltd.
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
 * Author: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "glib.h"
#include "girepository.h"

static char *
test_repository_search_paths_get_expected_libdir_path (void)
{
#if defined(G_PLATFORM_WIN32)
  const char *tests_build_dir = g_getenv ("G_TEST_BUILDDIR");
  char *expected_rel_path = g_build_filename (tests_build_dir, "..", "lib", "girepository-1.0", NULL);
#elif defined(__APPLE__)
  const char *tests_build_dir = g_getenv ("G_TEST_BUILDDIR");
  char *expected_rel_path = g_build_filename (tests_build_dir, "..", "girepository-1.0", NULL);
#else /* !G_PLATFORM_WIN32 && !__APPLE__ */
  char *expected_rel_path = g_build_filename (GOBJECT_INTROSPECTION_LIBDIR, "girepository-1.0", NULL);
#endif
  char *expected_path = g_canonicalize_filename (expected_rel_path, NULL);
  g_clear_pointer (&expected_rel_path, g_free);
  return expected_path;
}

static void
test_repository_search_paths_default (void)
{
  const char * const *search_paths;
  size_t n_search_paths;
  GIRepository *repository = NULL;

  repository = gi_repository_new ();

  search_paths = gi_repository_get_search_path (repository, &n_search_paths);
  g_assert_nonnull (search_paths);
  g_assert_cmpuint (g_strv_length ((char **) search_paths), ==, 2);

  g_assert_cmpstr (search_paths[0], ==, g_get_tmp_dir ());

  char *expected_path = test_repository_search_paths_get_expected_libdir_path ();
  g_assert_cmpstr (search_paths[1], ==, expected_path);
  g_clear_pointer (&expected_path, g_free);

  g_clear_object (&repository);
}

static void
test_repository_search_paths_prepend (void)
{
  const char * const *search_paths;
  size_t n_search_paths;
  GIRepository *repository = NULL;

  repository = gi_repository_new ();

  gi_repository_prepend_search_path (repository, g_test_get_dir (G_TEST_BUILT));
  search_paths = gi_repository_get_search_path (repository, &n_search_paths);
  g_assert_nonnull (search_paths);
  g_assert_cmpuint (g_strv_length ((char **) search_paths), ==, 3);

  g_assert_cmpstr (search_paths[0], ==, g_test_get_dir (G_TEST_BUILT));
  g_assert_cmpstr (search_paths[1], ==, g_get_tmp_dir ());

  char *expected_path = test_repository_search_paths_get_expected_libdir_path ();
  g_assert_cmpstr (search_paths[2], ==, expected_path);
  g_clear_pointer (&expected_path, g_free);

  gi_repository_prepend_search_path (repository, g_test_get_dir (G_TEST_DIST));
  search_paths = gi_repository_get_search_path (repository, &n_search_paths);
  g_assert_nonnull (search_paths);
  g_assert_cmpuint (g_strv_length ((char **) search_paths), ==, 4);

  g_assert_cmpstr (search_paths[0], ==, g_test_get_dir (G_TEST_DIST));
  g_assert_cmpstr (search_paths[1], ==, g_test_get_dir (G_TEST_BUILT));
  g_assert_cmpstr (search_paths[2], ==, g_get_tmp_dir ());

  expected_path = test_repository_search_paths_get_expected_libdir_path ();
  g_assert_cmpstr (search_paths[3], ==, expected_path);
  g_clear_pointer (&expected_path, g_free);

  g_clear_object (&repository);
}

static void
test_repository_library_paths_default (void)
{
  const char * const *library_paths;
  size_t n_library_paths;
  GIRepository *repository = NULL;

  repository = gi_repository_new ();

  library_paths = gi_repository_get_library_path (repository, &n_library_paths);
  g_assert_nonnull (library_paths);
  g_assert_cmpuint (g_strv_length ((char **) library_paths), ==, 0);

  g_clear_object (&repository);
}

static void
test_repository_library_paths_prepend (void)
{
  const char * const *library_paths;
  size_t n_library_paths;
  GIRepository *repository = NULL;

  repository = gi_repository_new ();

  gi_repository_prepend_library_path (repository, g_test_get_dir (G_TEST_BUILT));
  library_paths = gi_repository_get_library_path (repository, &n_library_paths);
  g_assert_nonnull (library_paths);
  g_assert_cmpuint (g_strv_length ((char **) library_paths), ==, 1);

  g_assert_cmpstr (library_paths[0], ==, g_test_get_dir (G_TEST_BUILT));

  gi_repository_prepend_library_path (repository, g_test_get_dir (G_TEST_DIST));
  library_paths = gi_repository_get_library_path (repository, &n_library_paths);
  g_assert_nonnull (library_paths);
  g_assert_cmpuint (g_strv_length ((char **) library_paths), ==, 2);

  g_assert_cmpstr (library_paths[0], ==, g_test_get_dir (G_TEST_DIST));
  g_assert_cmpstr (library_paths[1], ==, g_test_get_dir (G_TEST_BUILT));

  g_clear_object (&repository);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  /* Isolate from the system typelibs and GIRs. */
  g_setenv ("GI_TYPELIB_PATH", g_get_tmp_dir (), TRUE);
  g_setenv ("GI_GIR_PATH", g_get_user_cache_dir (), TRUE);

  g_test_add_func ("/repository/search-paths/default", test_repository_search_paths_default);
  g_test_add_func ("/repository/search-paths/prepend", test_repository_search_paths_prepend);
  g_test_add_func ("/repository/library-paths/default", test_repository_library_paths_default);
  g_test_add_func ("/repository/library-paths/prepend", test_repository_library_paths_prepend);

  return g_test_run ();
}
