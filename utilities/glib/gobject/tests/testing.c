/* GLib testing framework examples and tests
 *
 * Copyright © 2019 Endless Mobile, Inc.
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
 * Author: Philip Withnall <withnall@endlessm.com>
 */

#include "config.h"

/* We want to distinguish between messages originating from libglib
 * and messages originating from this program.
 */
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "testing"

#include <glib.h>
#include <glib-object.h>
#include <locale.h>
#include <stdlib.h>

static void
test_assert_finalize_object_subprocess_bad (void)
{
  GObject *obj = g_object_new (G_TYPE_OBJECT, NULL);
  g_object_ref (obj);

  /* This should emit an assertion failure. */
  g_assert_finalize_object (obj);

  g_object_unref (obj);

  exit (0);
}

static void
test_assert_finalize_object (void)
{
  GObject *obj = g_object_new (G_TYPE_OBJECT, NULL);

  g_assert_finalize_object (obj);

  g_test_trap_subprocess ("/assert/finalize_object/subprocess/bad", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*g_assert_finalize_object:*'weak_pointer' should be NULL*");
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/assert/finalize_object", test_assert_finalize_object);
  g_test_add_func ("/assert/finalize_object/subprocess/bad",
                   test_assert_finalize_object_subprocess_bad);

  return g_test_run ();
}
