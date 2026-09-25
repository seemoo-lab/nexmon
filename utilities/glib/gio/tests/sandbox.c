/*
 * Copyright 2022 Canonical Ltd
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

#include "portal-support-utils.h"

#include "../gsandbox.h"
#include <gio/gio.h>
#include <glib/gstdio.h>

static void
test_sandbox_none (void)
{
  g_assert_cmpint (glib_get_sandbox_type (), ==, G_SANDBOX_TYPE_UNKNOWN);
}

static void
test_sandbox_snap (void)
{
  const char *temp_dir;
  gchar *snap_path;

  temp_dir = g_getenv ("G_TEST_TMPDIR");
  g_assert_nonnull (temp_dir);

  snap_path = g_build_filename (temp_dir, "snap", "current", NULL);
  create_fake_snap_yaml (snap_path, FALSE);
  g_setenv ("SNAP", snap_path, TRUE);

  g_assert_cmpint (glib_get_sandbox_type (), ==, G_SANDBOX_TYPE_SNAP);

  g_unsetenv ("SNAP");
  g_free (snap_path);
}

static void
test_sandbox_snap_classic (void)
{
  const char *temp_dir;
  char *snap_path;

  temp_dir = g_getenv ("G_TEST_TMPDIR");
  g_assert_nonnull (temp_dir);

  snap_path = g_build_filename (temp_dir, "snap", "current", NULL);
  create_fake_snap_yaml (snap_path, TRUE);
  g_setenv ("SNAP", snap_path, TRUE);

  g_assert_cmpint (glib_get_sandbox_type (), ==, G_SANDBOX_TYPE_UNKNOWN);

  g_unsetenv ("SNAP");
  g_free (snap_path);
}

static void
test_sandbox_flatpak (void)
{
  create_fake_flatpak_info (g_get_user_runtime_dir (), NULL, NULL);
  g_assert_cmpint (glib_get_sandbox_type (), ==, G_SANDBOX_TYPE_FLATPAK);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/sandbox/none", test_sandbox_none);
  g_test_add_func ("/sandbox/snap", test_sandbox_snap);
  g_test_add_func ("/sandbox/classic-snap", test_sandbox_snap_classic);
  g_test_add_func ("/sandbox/flatpak", test_sandbox_flatpak);

  return g_test_run ();
}
