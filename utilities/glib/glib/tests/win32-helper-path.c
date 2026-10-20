/*
 * Unit test for _g_win32_find_helper_walk_up()
 *
 * Copyright (C) 2026 Andrew Ziem
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib.h>
#include <glib/glib-init.h>
#include <glib/gstdio.h>

#include <string.h>
#include <windows.h>

/*
 * Regression test for the UNC path crash in
 * g_win32_find_helper_executable_path().
 *
 * The walk-up loop strips one path component per iteration when the
 * helper executable is not found. For drive-letter paths ("C:\foo\bar")
 * the loop terminates gracefully at "C:" because strrchr("C:", '\\')
 * returns NULL. For UNC paths ("\\server\share\dir") the loop stripped
 * past "\\server\share" down to "\\server", "\", and finally "" (empty
 * string), causing g_build_filename("", "", "gdbus.exe") to produce
 * "gdbus.exe" (a relative path), which tripped the
 * g_assert(g_path_is_absolute(...)) and crashed the process.
 *
 * The fix adds a guard that stops the loop at the UNC root
 * ("\\server\share"). This test injects a UNC base path directly into
 * the extracted walk-up helper and verifies it returns NULL (not found)
 * without crashing.
 */
static void
test_unc_walk_up_no_crash (void)
{
  const gchar *const cases[] = {
    "\\\\server\\share\\dir\\subdir",
    "\\\\server\\share\\dir",
    "\\\\server\\share",
    "\\\\192.168.1.1\\temp\\a\\b\\c",
  };
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
      gchar *base = g_strdup (cases[i]);
      gchar *result;

      /* If the bug is present, this call will abort the process via
       * g_assert() before returning. */
      result = _g_win32_find_helper_walk_up (base, "nonexistent-helper.exe");

      /* Helper not found -> should return NULL (fall through to PATH). */
      g_assert_null (result);
    }
}

/*
 * Regression test: drive-letter paths must still work identically.
 * The loop should walk up to "C:" and then break (strrchr == NULL),
 * returning NULL when the helper is not found.
 */
static void
test_drive_letter_walk_up_no_crash (void)
{
  const gchar *const cases[] = {
    "C:\\projects\\foo\\bar",
    "C:\\projects\\foo",
    "C:\\foo",
    "D:\\",
  };
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (cases); i++)
    {
      gchar *base = g_strdup (cases[i]);
      gchar *result;

      result = _g_win32_find_helper_walk_up (base, "nonexistent-helper.exe");
      g_assert_null (result);
    }
}

/*
 * Test that the walk-up actually finds a helper when it exists in one
 * of the searched subdirectories ("", "bin", "lib", "glib", "gio").
 */
static void
test_walk_up_finds_helper (void)
{
  gchar *tmp_dir = NULL;
  gchar *subdir = NULL;
  gchar *helper_path = NULL;
  gchar *base = NULL;
  gchar *result = NULL;
  GError *error = NULL;

  /* Create a temp directory structure:
   *   <tmp>/walktest/bin/helper.exe
   *   <tmp>/walktest/subdir/   (to walk up from)
   */
  tmp_dir = g_dir_make_tmp ("walktest-XXXXXX", &error);
  g_assert_no_error (error);

  subdir = g_build_filename (tmp_dir, "subdir", NULL);
  g_assert_true (g_mkdir_with_parents (subdir, 0700) == 0);

  /* Place helper in "bin" subdirectory of the parent */
  gchar *bin_dir = g_build_filename (tmp_dir, "bin", NULL);
  g_assert_true (g_mkdir_with_parents (bin_dir, 0700) == 0);

  helper_path = g_build_filename (bin_dir, "helper.exe", NULL);
  g_file_set_contents (helper_path, "", 0, &error);
  g_assert_no_error (error);

  /* Walk up from <tmp>/walktest/subdir — should find <tmp>/walktest/bin/helper.exe */
  base = g_build_filename (subdir, NULL);
  result = _g_win32_find_helper_walk_up (base, "helper.exe");

  g_assert_nonnull (result);
  g_assert_true (g_path_is_absolute (result));
  g_assert (g_file_test (result, G_FILE_TEST_IS_REGULAR));

  /* Clean up: remove files and directories in reverse order so that
   * each directory is empty before being removed. */
  g_remove (helper_path);
  g_rmdir (bin_dir);
  g_rmdir (subdir);
  g_rmdir (tmp_dir);

  g_free (result);
  g_free (helper_path);
  g_free (bin_dir);
  g_free (subdir);
  g_free (tmp_dir);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/win32-helper-path/unc-walk-up-no-crash",
                   test_unc_walk_up_no_crash);
  g_test_add_func ("/win32-helper-path/drive-letter-walk-up-no-crash",
                   test_drive_letter_walk_up_no_crash);
  g_test_add_func ("/win32-helper-path/walk-up-finds-helper",
                   test_walk_up_finds_helper);

  return g_test_run ();
}
