/*
 * Copyright © 2020 Red Hat, Inc.
 *
 * Author: Jakub Jelen <jjelen@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * See the included COPYING file for more information.
 */

#include <glib.h>

/* The function g_get_user_database_entry() is called from the
 * g_get_real_name(), g_get_user_name() and g_build_home_dir()
 * functions. These two calls are here just to invoke the code
 * paths. The real-test is the ld_preload used to inject the
 * NULL in place of pw->pw_name.
 */
static void
test_get_user_database_entry (void)
{
  const gchar *r = NULL;

  r = g_get_user_name ();
  g_assert_cmpstr (r, ==, "somebody");

  r = g_get_real_name ();
  g_assert_cmpstr (r, ==, "Unknown");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gutils/get_user_database_entry", test_get_user_database_entry);

  return g_test_run ();
}
