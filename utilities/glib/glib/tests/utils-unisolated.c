/* Unit tests for utilities
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 *
 * Author: Matthias Clasen
 */

#include "glib.h"

static void
test_xdg_dirs (void)
{
#ifdef G_OS_UNIX
  char *xdg = NULL;
  const char *dir;
  const char * const *dirs;
  char *s = NULL;

  xdg = g_strdup (g_getenv ("XDG_CONFIG_HOME"));
  if (!xdg)
    xdg = g_build_filename (g_get_home_dir (), ".config", NULL);

  dir = g_get_user_config_dir ();

  g_assert_cmpstr (dir, ==, xdg);
  g_free (xdg);

  xdg = g_strdup (g_getenv ("XDG_DATA_HOME"));
  if (!xdg)
    xdg = g_build_filename (g_get_home_dir (), ".local", "share", NULL);

  dir = g_get_user_data_dir ();

  g_assert_cmpstr (dir, ==, xdg);
  g_free (xdg);

  xdg = g_strdup (g_getenv ("XDG_CACHE_HOME"));
  if (!xdg)
    xdg = g_build_filename (g_get_home_dir (), ".cache", NULL);

  dir = g_get_user_cache_dir ();

  g_assert_cmpstr (dir, ==, xdg);
  g_free (xdg);

  xdg = g_strdup (g_getenv ("XDG_STATE_HOME"));
  if (!xdg)
    xdg = g_build_filename (g_get_home_dir (), ".local/state", NULL);

  dir = g_get_user_state_dir ();

  g_assert_cmpstr (dir, ==, xdg);
  g_free (xdg);

  xdg = g_strdup (g_getenv ("XDG_RUNTIME_DIR"));
  if (!xdg)
    xdg = g_strdup (g_get_user_cache_dir ());

  dir = g_get_user_runtime_dir ();

  g_assert_cmpstr (dir, ==, xdg);
  g_free (xdg);

  xdg = (gchar *)g_getenv ("XDG_CONFIG_DIRS");
  if (!xdg)
    xdg = "/etc/xdg";

  dirs = g_get_system_config_dirs ();

  s = g_strjoinv (":", (gchar **)dirs);

  g_assert_cmpstr (s, ==, xdg);

  g_free (s);
#else
  g_test_skip ("User special dirs are not defined using environment variables on non-Unix systems");
#endif
}

int
main (int   argc,
      char *argv[])
{
  /* Note: We do *not* use G_TEST_OPTION_ISOLATE_DIRS here, as we want to test
   * the calculation of the XDG dirs from a real environment. */
  g_test_init (&argc, &argv, NULL);

  /* You almost certainly want to add a test to glib/tests/utils.c instead of here. */
  g_test_add_func ("/utils/xdgdirs", test_xdg_dirs);

  return g_test_run ();
}
