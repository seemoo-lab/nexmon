/* Copyright (C) 2001 Sebastian Wilhelmi <wilhelmi@google.com>
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* A trivial C++ program to be compiled in C++ mode, which
 * smoketests that the GIO headers are valid C++ headers. */

#include <gio/gio.h>

static void
test_name (void)
{
  GTask *task = NULL;
  char *orig = g_strdup ("some task");

  task = g_task_new (NULL, NULL, NULL, NULL);
  (g_task_set_name) (task, orig);
  g_assert_cmpstr (g_task_get_name (task), ==, "some task");

  (g_task_set_name) (task, "some other name");
  g_assert_cmpstr (g_task_get_name (task), ==, "some other name");

  g_clear_object (&task);
  g_free (orig);
}

static void
test_name_macro_wrapper (void)
{
  GTask *task = NULL;
  char *orig = g_strdup ("some task");

  task = g_task_new (NULL, NULL, NULL, NULL);
  g_task_set_name (task, orig);
  g_assert_cmpstr (g_task_get_name (task), ==, "some task");

  g_task_set_name (task, "some other name");
  g_assert_cmpstr (g_task_get_name (task), ==, "some other name");

  g_clear_object (&task);
  g_free (orig);
}

int
main (int argc, char **argv)
{
#if G_CXX_STD_CHECK_VERSION (11)
  g_test_init (&argc, &argv, nullptr);
#else
  g_test_init (&argc, &argv, static_cast<void *>(NULL));
#endif

  g_test_add_func ("/gtask/name", test_name);
  g_test_add_func ("/gtask/name/macro-wrapper", test_name_macro_wrapper);

  return g_test_run ();
}
