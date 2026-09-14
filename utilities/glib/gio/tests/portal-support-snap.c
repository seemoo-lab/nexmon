/*
 * GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2022 Canonical Ltd.
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

#include "portal-support-utils.h"

#include "../gportalsupport.h"
#include <gio/gio.h>
#include <glib/gstdio.h>

typedef struct
{
  char *old_path;
  char *old_snap;

  const char *bin_path;
  const char *snap_path;
} SetupData;

static void
tests_setup (SetupData *setup_data,
             gconstpointer data)
{
  setup_data->old_path = g_strdup (g_getenv ("PATH"));
  setup_data->old_snap = g_strdup (g_getenv ("SNAP"));

  setup_data->bin_path = g_get_user_runtime_dir ();
  setup_data->snap_path = g_getenv ("G_TEST_TMPDIR");

  g_assert_nonnull (setup_data->bin_path);
  g_assert_nonnull (setup_data->snap_path);

  g_setenv ("PATH", setup_data->bin_path, TRUE);
  g_setenv ("SNAP", setup_data->snap_path, TRUE);
}

static void
tests_teardown (SetupData *setup_data,
                gconstpointer data)
{
  if (setup_data->old_path)
    g_setenv ("PATH", setup_data->old_path, TRUE);
  else
    g_unsetenv ("PATH");

  if (setup_data->old_snap)
    g_setenv ("SNAP", setup_data->old_snap, TRUE);
  else
    g_unsetenv ("SNAP");

  cleanup_snapfiles (setup_data->snap_path);
  cleanup_snapfiles (setup_data->bin_path);

  g_clear_pointer (&setup_data->old_path, g_free);
  g_clear_pointer (&setup_data->old_snap, g_free);
}

static void
test_portal_support_snap_no_snapctl (SetupData *setup,
                                     gconstpointer data)
{
  create_fake_snap_yaml (setup->snap_path, FALSE);

  g_assert_false (glib_should_use_portal ());
  g_assert_false (glib_network_available_in_sandbox ());
  g_assert_false (glib_has_dconf_access_in_sandbox ());
}

static void
test_portal_support_snap_none (SetupData *setup,
                               gconstpointer data)
{
  create_fake_snap_yaml (setup->snap_path, FALSE);
  create_fake_snapctl (setup->bin_path, NULL);

  g_assert_false (glib_should_use_portal ());
  g_assert_false (glib_network_available_in_sandbox ());
  g_assert_false (glib_has_dconf_access_in_sandbox ());
}

static void
test_portal_support_snap_all (SetupData *setup,
                              gconstpointer data)
{
  create_fake_snap_yaml (setup->snap_path, FALSE);
  create_fake_snapctl (setup->bin_path, "desktop|network-status|gsettings");

  g_assert_true (glib_should_use_portal ());
  g_assert_true (glib_network_available_in_sandbox ());
  g_assert_true (glib_has_dconf_access_in_sandbox ());
}

static void
test_portal_support_snap_desktop_only (SetupData *setup,
                                       gconstpointer data)
{
  create_fake_snap_yaml (setup->snap_path, FALSE);
  create_fake_snapctl (setup->bin_path, "desktop");

  g_assert_true (glib_should_use_portal ());
  g_assert_true (glib_network_available_in_sandbox ());
  g_assert_false (glib_has_dconf_access_in_sandbox ());
}

static void
test_portal_support_snap_network_only (SetupData *setup,
                                       gconstpointer data)
{
  create_fake_snap_yaml (setup->snap_path, FALSE);
  create_fake_snapctl (setup->bin_path, "network-status");

  g_assert_false (glib_should_use_portal ());
  g_assert_true (glib_network_available_in_sandbox ());
  g_assert_false (glib_has_dconf_access_in_sandbox ());
}

static void
test_portal_support_snap_gsettings_only (SetupData *setup,
                                         gconstpointer data)
{
  create_fake_snap_yaml (setup->snap_path, FALSE);
  create_fake_snapctl (setup->bin_path, "gsettings");

  g_assert_false (glib_should_use_portal ());
  g_assert_false (glib_network_available_in_sandbox ());
  g_assert_true (glib_has_dconf_access_in_sandbox ());
}

static void
test_portal_support_snap_updates_dynamically (SetupData *setup,
                                              gconstpointer data)
{
  create_fake_snap_yaml (setup->snap_path, FALSE);
  create_fake_snapctl (setup->bin_path, NULL);

  g_assert_false (glib_should_use_portal ());
  g_assert_false (glib_network_available_in_sandbox ());
  g_assert_false (glib_has_dconf_access_in_sandbox ());

  create_fake_snapctl (setup->bin_path, "desktop");
  g_assert_true (glib_should_use_portal ());
  g_assert_true (glib_network_available_in_sandbox ());
  g_assert_false (glib_has_dconf_access_in_sandbox ());

  create_fake_snapctl (setup->bin_path, "network-status|gsettings");
  g_assert_false (glib_should_use_portal ());
  g_assert_true (glib_network_available_in_sandbox ());
  g_assert_true (glib_has_dconf_access_in_sandbox ());

  create_fake_snapctl (setup->bin_path, "desktop|network-status|gsettings");
  g_assert_true (glib_should_use_portal ());
  g_assert_true (glib_network_available_in_sandbox ());
  g_assert_true (glib_has_dconf_access_in_sandbox ());

  create_fake_snapctl (setup->bin_path, "desktop|gsettings");
  g_assert_true (glib_should_use_portal ());
  g_assert_true (glib_network_available_in_sandbox ());
  g_assert_true (glib_has_dconf_access_in_sandbox ());

  create_fake_snapctl (setup->bin_path, "gsettings");
  g_assert_false (glib_should_use_portal ());
  g_assert_false (glib_network_available_in_sandbox ());
  g_assert_true (glib_has_dconf_access_in_sandbox ());

  create_fake_snapctl (setup->bin_path, NULL);
  g_assert_false (glib_should_use_portal ());
  g_assert_false (glib_network_available_in_sandbox ());
  g_assert_false (glib_has_dconf_access_in_sandbox ());
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add ("/portal-support/snap/no-snapctl", SetupData, NULL,
    tests_setup, test_portal_support_snap_no_snapctl, tests_teardown);
  g_test_add ("/portal-support/snap/none", SetupData, NULL,
    tests_setup, test_portal_support_snap_none, tests_teardown);
  g_test_add ("/portal-support/snap/all", SetupData, NULL,
    tests_setup, test_portal_support_snap_all, tests_teardown);
  g_test_add ("/portal-support/snap/desktop-only", SetupData, NULL,
    tests_setup, test_portal_support_snap_desktop_only, tests_teardown);
  g_test_add ("/portal-support/snap/network-only", SetupData, NULL,
    tests_setup, test_portal_support_snap_network_only, tests_teardown);
  g_test_add ("/portal-support/snap/gsettings-only", SetupData, NULL,
    tests_setup, test_portal_support_snap_gsettings_only, tests_teardown);
  g_test_add ("/portal-support/snap/updates-dynamically", SetupData, NULL,
    tests_setup, test_portal_support_snap_updates_dynamically, tests_teardown);

  return g_test_run ();
}
