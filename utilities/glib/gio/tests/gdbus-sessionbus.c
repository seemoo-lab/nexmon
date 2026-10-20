/* GLib testing framework examples and tests
 *
 * Copyright (C) 2012 Collabora Ltd. <http://www.collabora.co.uk/>
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
 * Author: Xavier Claessens <xavier.claessens@collabora.co.uk>
 */

#include "gdbus-sessionbus.h"

static GTestDBus *singleton = NULL;

void
session_bus_up (void)
{
  gchar *relative, *servicesdir;
  g_assert (singleton == NULL);
  singleton = g_test_dbus_new (G_TEST_DBUS_NONE);

  /* We ignore deprecations here so that gdbus-test-codegen-old can
   * build successfully despite these two functions not being
   * available in GLib 2.36 */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  relative = g_test_build_filename (G_TEST_BUILT, "services", NULL);
  servicesdir = g_canonicalize_filename (relative, NULL);
  G_GNUC_END_IGNORE_DEPRECATIONS
  g_free (relative);

  g_test_dbus_add_service_dir (singleton, servicesdir);
  g_free (servicesdir);
  g_test_dbus_up (singleton);
}

void
session_bus_stop (void)
{
  g_assert (singleton != NULL);
  g_test_dbus_stop (singleton);
}

void
session_bus_down (void)
{
  g_assert (singleton != NULL);
  g_test_dbus_down (singleton);
  g_clear_object (&singleton);
}

gint
session_bus_run (void)
{
  gint ret;

  session_bus_up ();
  ret = g_test_run ();
  session_bus_down ();

  return ret;
}

const char *
session_bus_get_address (void)
{
  g_assert (singleton != NULL);
  return g_test_dbus_get_bus_address (singleton);
}
