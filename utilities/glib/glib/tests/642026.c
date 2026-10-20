/*
 * Author: Simon McVittie <simon.mcvittie@collabora.co.uk>
 * Copyright © 2011 Nokia Corporation
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

#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include <glib.h>

static GStaticPrivate sp;
static GMutex *mutex;
static GCond *cond;
static guint i;

static gint freed = 0;  /* (atomic) */

static void
notify (gpointer p)
{
  if (!g_atomic_int_compare_and_exchange (&freed, 0, 1))
    {
      g_error ("someone already freed it after %u iterations", i);
    }
}

static gpointer thread_func (gpointer nil)
{
  /* wait for main thread to reach its g_cond_wait call */
  g_mutex_lock (mutex);

  g_static_private_set (&sp, &sp, notify);
  g_cond_broadcast (cond);
  g_mutex_unlock (mutex);

  return nil;
}

static void
testcase (void)
{
  /* On smcv's laptop, 1e4 iterations didn't always exhibit the bug, but 1e5
   * iterations exhibited it 10/10 times in practice. YMMV.
   *
   * If running with `-m slow` we want to try hard to reproduce the bug 10/10
   * times. However, as of 2022 this takes around 240s on a CI machine, which
   * is a long time to tie up those resources to verify that a bug fixed 10
   * years ago is still fixed.
   *
   * So if running without `-m slow`, try 100× less hard to reproduce the bug,
   * and rely on the fact that this is run under CI often enough to have a good
   * chance of reproducing the bug in 1% of CI runs. */
  const guint n_iterations = g_test_slow () ? 100000 : 1000;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=642026");

  mutex = g_mutex_new ();
  cond = g_cond_new ();

  g_mutex_lock (mutex);

  for (i = 0; i < n_iterations; i++)
    {
      GThread *t1;

      g_static_private_init (&sp);
      g_atomic_int_set (&freed, 0);

      t1 = g_thread_create (thread_func, NULL, TRUE, NULL);
      g_assert (t1 != NULL);

      /* wait for t1 to set up its thread-private data */
      g_cond_wait (cond, mutex);

      /* exercise the bug, by racing with t1 to free the private data */
      g_static_private_free (&sp);
      g_thread_join (t1);
    }

  g_cond_free (cond);
  g_mutex_unlock (mutex);
  g_mutex_free (mutex);
}

int
main (int argc,
    char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/glib/642026", testcase);

  return g_test_run ();
}
