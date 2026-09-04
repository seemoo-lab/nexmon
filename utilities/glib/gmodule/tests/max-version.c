/*
 * Copyright 2022 Collabora Ltd.
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
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef GLIB_VERSION_MAX_ALLOWED
/* This is the oldest version macro available */
#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_26
#define GLIB_VERSION_MAX_ALLOWED GLIB_VERSION_2_26
#endif

#include <glib.h>

#include <gmodule.h>

static void
nothing (void)
{
  /* This doesn't really do anything: the real "test" is at compile time.
   * Just make sure the GModule library gets linked. */
  g_debug ("GModule supported: %s", g_module_supported () ? "yes" : "no");
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/max-version/tested-at-compile-time", nothing);
  return g_test_run ();
}
