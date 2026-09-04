/* Copyright (C) 2022 Marco Trevisan
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

#include "config.h"

#include <glib/glib.h>
#include <glib/gstdio.h>

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

/* Test that all of the well-known directories returned by GLib
 * are returned as children of test_tmpdir when running with
 * %G_TEST_OPTION_ISOLATE_DIRS. This ensures that tests should
 * not interfere with each other in `/tmp` while running.
 */

const char *test_tmpdir;

static void
test_tmp_dir (void)
{
  g_assert_cmpstr (g_get_tmp_dir (), ==, test_tmpdir);
}

static void
test_home_dir (void)
{
  g_assert_true (g_str_has_prefix (g_get_home_dir (), test_tmpdir));
}

static void
test_user_cache_dir (void)
{
  g_assert_true (g_str_has_prefix (g_get_user_cache_dir (), test_tmpdir));
}

static void
test_system_config_dirs (void)
{
  const char *const *dir;

  for (dir = g_get_system_config_dirs (); *dir != NULL; dir++)
    g_assert_true (g_str_has_prefix (*dir, test_tmpdir));
}

static void
test_user_config_dir (void)
{
  g_assert_true (g_str_has_prefix (g_get_user_config_dir (), test_tmpdir));
}

static void
test_system_data_dirs (void)
{
  const char *const *dir;

  for (dir = g_get_system_data_dirs (); *dir != NULL; dir++)
    g_assert_true (g_str_has_prefix (*dir, test_tmpdir));
}

static void
test_user_data_dir (void)
{
  g_assert_true (g_str_has_prefix (g_get_user_data_dir (), test_tmpdir));
}

static void
test_user_state_dir (void)
{
  g_assert_true (g_str_has_prefix (g_get_user_state_dir (), test_tmpdir));
}

static void
test_user_runtime_dir (void)
{
  g_assert_true (g_str_has_prefix (g_get_user_runtime_dir (), test_tmpdir));
}

static void
test_cleanup_handles_errors (void)
{
  const gchar *runtime_dir = g_get_user_runtime_dir ();
  gchar *subdir = g_build_filename (runtime_dir, "b", NULL);

  if (g_test_subprocess ())
    {

      g_assert_no_errno (g_mkdir_with_parents (subdir, 0755));
      /* Normally this is enough to prevent us from deleting
       * $XDG_RUNTIME_DIR and its contents. If we're root (specifically,
       * CAP_DAC_OVERRIDE) then this will not prevent deletion,
       * making this test ineffective, but in the common case where the
       * tests aren't run as root, the test will work as intended.
       */
      g_assert_no_errno (g_chmod (runtime_dir, 0));

      g_clear_pointer (&subdir, g_free);
      /* Now let the harness clean up. Not being able to delete part of the
       * test's isolated temporary directory should not cause the test to
       * fail.
       */
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
  /* No assertion about the test logging anything to stderr: we don't
   * guarantee this, and one of the cleanup implementations doesn't log
   * anything.
   */

  /* Now that we have verified that a failure to delete part of the isolated
   * temporary directory hierarchy does not cause the test to fail, clean up
   * after ourselves.
   *
   * Note that in the case where we have CAP_DAC_OVERRIDE or equivalent,
   * we will find that it *was* deleted.
   */
  if (g_chmod (runtime_dir, 0755) != 0)
    {
      if (errno == ENOENT)
        g_test_skip_printf ("g_chmod(%s, 0) was not enough to prevent cleanup, "
                            "perhaps we have CAP_DAC_OVERRIDE?",
                            runtime_dir);
      else
        g_error ("g_chmod(%s, 0755) -> %s", runtime_dir, g_strerror (errno));
    }

  g_free (subdir);
}

static void
test_cleanup_doesnt_follow_symlinks (void)
{
#ifdef G_OS_WIN32
  g_test_skip ("Symlinks not generally available on Windows");
#else
  const gchar *test_tmpdir = g_getenv ("G_TEST_TMPDIR");
  const gchar *runtime_dir = g_get_user_runtime_dir ();
  g_assert_cmpstr (test_tmpdir, !=, runtime_dir);
  g_assert_true (g_str_has_prefix (runtime_dir, test_tmpdir));
  gchar *symlink_path = g_build_filename (runtime_dir, "symlink", NULL);
  gchar *target_path = g_build_filename (test_tmpdir, "target", NULL);
  gchar *file_within_target = g_build_filename (target_path, "precious-data", NULL);

  if (g_test_subprocess ())
    {
      g_assert_no_errno (g_mkdir_with_parents (runtime_dir, 0755));
      g_assert_no_errno (symlink (target_path, symlink_path));

      g_free (symlink_path);
      g_free (target_path);
      g_free (file_within_target);

      return;
    }
  else
    {
      GError *error = NULL;

      g_assert_no_errno (g_mkdir_with_parents (target_path, 0755));
      g_file_set_contents (file_within_target, "Precious Data", -1, &error);
      g_assert_no_error (error);

      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDERR);
      g_test_trap_assert_passed ();

      /* There was a symbolic link in the test's isolated directory which
       * pointed to a directory outside it. That directory and its contents
       * should not have been deleted: the symbolic link should not have been
       * followed.
       */
      g_assert_true (g_file_test (file_within_target, G_FILE_TEST_EXISTS));
      g_assert_true (g_file_test (target_path, G_FILE_TEST_IS_DIR));

      /* The symlink itself should have been deleted. */
      g_assert_false (g_file_test (symlink_path, G_FILE_TEST_EXISTS));
      g_assert_false (g_file_test (symlink_path, G_FILE_TEST_IS_SYMLINK));

      g_free (symlink_path);
      g_free (target_path);
      g_free (file_within_target);
    }
#endif
}

int
main (int   argc,
      char *argv[])
{
  g_setenv ("LC_ALL", "C", TRUE);
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  test_tmpdir = g_getenv ("G_TEST_TMPDIR");
  g_assert_nonnull (test_tmpdir);

  g_test_add_func ("/utils-isolated/tmp-dir", test_tmp_dir);
  g_test_add_func ("/utils-isolated/home-dir", test_home_dir);
  g_test_add_func ("/utils-isolated/user-cache-dir", test_user_cache_dir);
  g_test_add_func ("/utils-isolated/system-config-dirs", test_system_config_dirs);
  g_test_add_func ("/utils-isolated/user-config-dir", test_user_config_dir);
  g_test_add_func ("/utils-isolated/system-data-dirs", test_system_data_dirs);
  g_test_add_func ("/utils-isolated/user-data-dir", test_user_data_dir);
  g_test_add_func ("/utils-isolated/user-state-dir", test_user_state_dir);
  g_test_add_func ("/utils-isolated/user-runtime-dir", test_user_runtime_dir);
  g_test_add_func ("/utils-isolated/cleanup/handles-errors", test_cleanup_handles_errors);
  g_test_add_func ("/utils-isolated/cleanup/doesnt-follow-symlinks", test_cleanup_doesnt_follow_symlinks);
  return g_test_run ();
}
