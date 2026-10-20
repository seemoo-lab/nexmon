/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2025 Marco Trevisan (Treviño)
 */

#include <stdlib.h>
#include <gio/gio.h>

int
main (int argc, char *argv[])
{
  const char *envvar;
  const char *document_portal_mount;
  char *expected;
  char *expected_files[3] = {0};
  gint pid_from_env;

  g_test_init (&argc, &argv, NULL);

  envvar = g_getenv ("GIO_LAUNCHED_DESKTOP_FILE");
  g_assert_nonnull (envvar);

  expected = g_test_build_filename (G_TEST_BUILT, "snap-app_appinfo-test.desktop", NULL);
  g_assert_cmpstr (envvar, ==, expected);
  g_free (expected);

  envvar = g_getenv ("GIO_LAUNCHED_DESKTOP_FILE_PID");
  g_assert (envvar != NULL);
  pid_from_env = atoi (envvar);
  g_assert_cmpint (pid_from_env, ==, getpid ());

  document_portal_mount = g_getenv ("DOCUMENT_PORTAL_MOUNT_POINT");
  g_assert_nonnull (document_portal_mount);

  expected_files[0] = g_build_filename (document_portal_mount,
                                        "document-id-0", "snap-app_appinfo-test.desktop",
                                        NULL);
  expected_files[1] = g_build_filename (document_portal_mount,
                                        "document-id-1", "appinfo-test.desktop",
                                        NULL);

  g_assert_cmpint (argc, ==, 3);

  for (size_t i = 0; i < G_N_ELEMENTS (expected_files); ++i)
    {
      g_assert_cmpstr (argv[i+1], ==, expected_files[i]);
      g_clear_pointer (&expected_files[i], g_free);
    }

  return 0;
}
