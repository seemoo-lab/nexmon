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

#include <glib.h>

static void
test_basic (void)
{
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  GMemChunk *mem_chunk = g_mem_chunk_new ("test mem chunk", 50, 100, G_ALLOC_AND_FREE);
  gchar *mem[10000];
  guint i;
  for (i = 0; i < 10000; i++)
    {
      guint j;
      mem[i] = g_chunk_new (gchar, mem_chunk);
      for (j = 0; j < 50; j++)
	mem[i][j] = i * j;
    }
  for (i = 0; i < 10000; i++)
    g_mem_chunk_free (mem_chunk, mem[i]);

  g_mem_chunk_destroy (mem_chunk);

  G_GNUC_END_IGNORE_DEPRECATIONS
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/memchunk/basic", test_basic);

  return g_test_run ();
}
