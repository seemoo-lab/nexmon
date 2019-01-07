/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <stdio.h>
#include <string.h>
#include "glib.h"

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



int
main (int   argc,
      char *argv[])
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
      g_assert (! g_relation_exists (relation, data + i, data + i));
      g_assert (! g_relation_exists (relation, data + i, data + i + 2));
      g_assert (! g_relation_exists (relation, data + i, data + i - 2));
    }

  for (i = 1; i < 1023; i += 1)
    {
      g_assert (g_relation_exists (relation, data + i, data + i + 1));
      g_assert (g_relation_exists (relation, data + i, data + i - 1));
    }

  for (i = 2; i < 1022; i += 1)
    {
      g_assert (g_relation_count (relation, data + i, 0) == 2);
      g_assert (g_relation_count (relation, data + i, 1) == 2);
    }

  g_assert (g_relation_count (relation, data, 0) == 0);

  g_assert (g_relation_count (relation, data + 42, 0) == 2);
  g_assert (g_relation_count (relation, data + 43, 1) == 2);
  g_assert (g_relation_count (relation, data + 41, 1) == 2);
  g_relation_delete (relation, data + 42, 0);
  g_assert (g_relation_count (relation, data + 42, 0) == 0);
  g_assert (g_relation_count (relation, data + 43, 1) == 1);
  g_assert (g_relation_count (relation, data + 41, 1) == 1);

  tuples = g_relation_select (relation, data + 200, 0);

  g_assert (tuples->len == 2);

#if 0
  for (i = 0; i < tuples->len; i += 1)
    {
      printf ("%d %d\n",
	      *(gint*) g_tuples_index (tuples, i, 0),
	      *(gint*) g_tuples_index (tuples, i, 1));
    }
#endif

  g_assert (g_relation_exists (relation, data + 300, data + 301));
  g_relation_delete (relation, data + 300, 0);
  g_assert (!g_relation_exists (relation, data + 300, data + 301));

  g_tuples_destroy (tuples);

  g_relation_destroy (relation);

  relation = NULL;

  return 0;
}

