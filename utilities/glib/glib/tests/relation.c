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

/* we know we are deprecated here, no need for warnings */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>

int array[10000];
gboolean failed = FALSE;

#define	TEST(m,cond)	G_STMT_START { failed = !(cond); \
if (failed) \
  { if (!m) \
      g_print ("\n(%s:%d) failed for: %s\n", __FILE__, __LINE__, ( # cond )); \
    else \
      g_print ("\n(%s:%d) failed for: %s: (%s)\n", __FILE__, __LINE__, ( # cond ), (gchar*)m); \
  } \
else \
  g_print ("."); fflush (stdout); \
} G_STMT_END

#define	C2P(c)		((gpointer) ((long) (c)))
#define	P2C(p)		((gchar) ((long) (p)))

#define GLIB_TEST_STRING "el dorado "
#define GLIB_TEST_STRING_5 "el do"

typedef struct {
  guint age;
  gchar name[40];
} GlibTestInfo;

static void
test_relation (void)
{
  gint i;
  GRelation *relation;
  GTuples *tuples;
  gint data [1024];

  relation = g_relation_new (2);

  g_relation_index (relation, 0, g_int_hash, g_int_equal);
  g_relation_index (relation, 1, g_int_hash, g_int_equal);

  for (i = 0; i < 1024; i += 1)
    data[i] = i;

  for (i = 1; i < 1023; i += 1)
    {
      g_relation_insert (relation, data + i, data + i + 1);
      g_relation_insert (relation, data + i, data + i - 1);
    }

  for (i = 2; i < 1022; i += 1)
    {
      g_assert_false (g_relation_exists (relation, data + i, data + i));
      g_assert_false (g_relation_exists (relation, data + i, data + i + 2));
      g_assert_false (g_relation_exists (relation, data + i, data + i - 2));
    }

  for (i = 1; i < 1023; i += 1)
    {
      g_assert_true (g_relation_exists (relation, data + i, data + i + 1));
      g_assert_true (g_relation_exists (relation, data + i, data + i - 1));
    }

  for (i = 2; i < 1022; i += 1)
    {
      g_assert_cmpint (g_relation_count (relation, data + i, 0), ==, 2);
      g_assert_cmpint (g_relation_count (relation, data + i, 1), ==, 2);
    }

  g_assert_cmpint (g_relation_count (relation, data, 0), ==, 0);

  g_assert_cmpint (g_relation_count (relation, data + 42, 0), ==, 2);
  g_assert_cmpint (g_relation_count (relation, data + 43, 1), ==, 2);
  g_assert_cmpint (g_relation_count (relation, data + 41, 1), ==, 2);

  g_relation_delete (relation, data + 42, 0);

  g_assert_cmpint (g_relation_count (relation, data + 42, 0), ==, 0);
  g_assert_cmpint (g_relation_count (relation, data + 43, 1), ==, 1);
  g_assert_cmpint (g_relation_count (relation, data + 41, 1), ==, 1);

  tuples = g_relation_select (relation, data + 200, 0);

  g_assert_cmpint (tuples->len, ==, 2);

  g_assert_true (g_relation_exists (relation, data + 300, data + 301));
  g_relation_delete (relation, data + 300, 0);
  g_assert_false (g_relation_exists (relation, data + 300, data + 301));

  g_tuples_destroy (tuples);
  g_relation_destroy (relation);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/glib/relation", test_relation);

  return g_test_run ();
}
