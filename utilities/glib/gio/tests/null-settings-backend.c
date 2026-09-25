/*
 * Copyright (C) 2022 Ryan Hope
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
 * Authors: Ryan Hope <ryanhope97@gmail.com>
 */

#include <gio/gio.h>
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

/* Test that the "gsettings-backend" extension point has been registered.
 * Must be run first and separately from other GSettingsBackend,
 * as they will register the extension point making the test useless.
 */
static void
test_extension_point_registered (void)
{
  GSettingsBackend *backend;
  GIOExtensionPoint *extension_point;

  backend = g_null_settings_backend_new ();
  g_assert_true (G_IS_SETTINGS_BACKEND (backend));
  extension_point = g_io_extension_point_lookup (G_SETTINGS_BACKEND_EXTENSION_POINT_NAME);

  g_assert_nonnull (extension_point);

  g_object_unref (backend);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  /* Must be run first */
  g_test_add_func ("/null-settings-backend/extension-point-registered", test_extension_point_registered);

  return g_test_run ();
}
