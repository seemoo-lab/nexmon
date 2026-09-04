/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/.
 */

/* We are testing some deprecated APIs here */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <string.h>

#include "glib.h"

static void
test_completion (void)
{
  static const char *const a1 = "a\302\243";
  static const char *const a2 = "a\302\244";
  static const char *const bb = "bb";
  static const char *const bc = "bc";

  GCompletion *cmp;
  GList *items;
  gchar *prefix;

  cmp = g_completion_new (NULL);
  g_completion_set_compare (cmp, strncmp);

  items = NULL;
  items = g_list_append (items, (gpointer) a1);
  items = g_list_append (items, (gpointer) a2);
  items = g_list_append (items, (gpointer) bb);
  items = g_list_append (items, (gpointer) bc);
  g_completion_add_items (cmp, items);
  g_list_free (items);

  items = g_completion_complete (cmp, "a", &prefix);
  g_assert_cmpstr (prefix, ==, "a\302");
  g_assert_cmpint (g_list_length (items), ==, 2);
  g_free (prefix);

  items = g_completion_complete_utf8 (cmp, "a", &prefix);
  g_assert_cmpstr (prefix, ==, "a");
  g_assert_cmpint (g_list_length (items), ==, 2);
  g_free (prefix);

  items = g_completion_complete (cmp, "b", &prefix);
  g_assert_cmpstr (prefix, ==, "b");
  g_assert_cmpint (g_list_length (items), ==, 2);
  g_free (prefix);

  items = g_completion_complete_utf8 (cmp, "b", &prefix);
  g_assert_cmpstr (prefix, ==, "b");
  g_assert_cmpint (g_list_length (items), ==, 2);
  g_free (prefix);

  items = g_completion_complete (cmp, "a", NULL);
  g_assert_cmpint (g_list_length (items), ==, 2);

  items = g_completion_complete_utf8 (cmp, "a", NULL);
  g_assert_cmpint (g_list_length (items), ==, 2);

  items = g_list_append (NULL, (gpointer) bb);
  g_completion_remove_items (cmp, items);
  g_list_free (items);

  items = g_completion_complete_utf8 (cmp, "b", &prefix);
  g_assert_cmpint (g_list_length (items), ==, 1);
  g_free (prefix);

  g_completion_free (cmp);
}

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/completion/test-completion", test_completion);

  return g_test_run ();
}
