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

#include "../gportalsupport.h"
#include <gio/gio.h>

static void
test_portal_support_none (void)
{
  g_assert_false (glib_should_use_portal ());
  g_assert_true (glib_network_available_in_sandbox ());
  g_assert_true (glib_has_dconf_access_in_sandbox ());
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/portal-support/none", test_portal_support_none);

  return g_test_run ();
}
