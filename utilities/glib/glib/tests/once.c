
/* Unit tests for GOnce and friends
 * Copyright (C) 2011 Red Hat, Inc
 * Author: Matthias Clasen
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <glib.h>
#include "../gvalgrind.h"

#if GLIB_SIZEOF_VOID_P > 4 && !defined(ENABLE_VALGRIND)
#define THREADS 1000
#else
#define THREADS 100
#endif

static gpointer
do_once (gpointer data)
{
  static gint i = 0;

  i++;

  return GINT_TO_POINTER (i);
}

static void
test_once_single_threaded (void)
{
  GOnce once = G_ONCE_INIT;
  gpointer res;

  g_test_summary ("Test g_once() usage from a single thread");

  g_assert_cmpint (once.status, ==, G_ONCE_STATUS_NOTCALLED);

  res = g_once (&once, do_once, NULL);
  g_assert_cmpint (GPOINTER_TO_INT (res), ==, 1);

  g_assert_cmpint (once.status, ==, G_ONCE_STATUS_READY);

  res = g_once (&once, do_once, NULL);
  g_assert_cmpint (GPOINTER_TO_INT (res), ==, 1);
}

static GOnce once_multi_threaded = G_ONCE_INIT;
static gint once_multi_threaded_counter = 0;
static GCond once_multi_threaded_cond;
static GMutex once_multi_threaded_mutex;
static guint once_multi_threaded_n_threads_waiting = 0;

static gpointer
do_once_multi_threaded (gpointer data)
{
  gint old_value;

  /* While this function should only ever be executed once, by one thread,
   * we should use atomics to ensure that if there were a bug, writes to
   * `once_multi_threaded_counter` from multiple threads would not get lost and
   * mean the test erroneously succeeded. */
  old_value = g_atomic_int_add (&once_multi_threaded_counter, 1);

  return GINT_TO_POINTER (old_value + 1);
}

static gpointer
once_thread_func (gpointer data)
{
  gpointer res;
  guint n_threads_expected = GPOINTER_TO_UINT (data);

  /* Don’t immediately call g_once(), otherwise the first thread to be created
   * will end up calling the once-function, and there will be very little
   * contention. */
  g_mutex_lock (&once_multi_threaded_mutex);

  once_multi_threaded_n_threads_waiting++;
  g_cond_broadcast (&once_multi_threaded_cond);

  while (once_multi_threaded_n_threads_waiting < n_threads_expected)
    g_cond_wait (&once_multi_threaded_cond, &once_multi_threaded_mutex);
  g_mutex_unlock (&once_multi_threaded_mutex);

  /* Actually run the test. */
  res = g_once (&once_multi_threaded, do_once_multi_threaded, NULL);
  g_assert_cmpint (GPOINTER_TO_INT (res), ==, 1);

  return NULL;
}

static void
test_once_multi_threaded (void)
{
  guint i;
  GThread *threads[THREADS];

  g_test_summary ("Test g_once() usage from multiple threads");

  for (i = 0; i < G_N_ELEMENTS (threads); i++)
    threads[i] = g_thread_new ("once-multi-threaded",
                               once_thread_func,
                               GUINT_TO_POINTER (G_N_ELEMENTS (threads)));

  /* All threads have started up, so start the test. */
  g_cond_broadcast (&once_multi_threaded_cond);

  for (i = 0; i < G_N_ELEMENTS (threads); i++)
    g_thread_join (threads[i]);

  g_assert_cmpint (g_atomic_int_get (&once_multi_threaded_counter), ==, 1);
}

static void
test_once_init_single_threaded (void)
{
  static gsize init = 0;

  g_test_summary ("Test g_once_init_{enter,leave}() usage from a single thread");

  if (g_once_init_enter (&init))
    {
      g_assert (TRUE);
      g_once_init_leave (&init, 1);
    }

  g_assert_cmpint (init, ==, 1);
  if (g_once_init_enter (&init))
    {
      g_assert_not_reached ();
      g_once_init_leave (&init, 2);
    }
  g_assert_cmpint (init, ==, 1);
}

static gint64 shared;

static void
init_shared (void)
{
  static gsize init = 0;

  if (g_once_init_enter (&init))
    {
      shared += 42;

      g_once_init_leave (&init, 1);
    }
}

static gpointer
thread_func (gpointer data)
{
  init_shared ();

  return NULL;
}

static void
test_once_init_multi_threaded (void)
{
  gsize i;
  GThread *threads[THREADS];

  g_test_summary ("Test g_once_init_{enter,leave}() usage from multiple threads");

  shared = 0;

  for (i = 0; i < G_N_ELEMENTS (threads); i++)
    threads[i] = g_thread_new ("once-init-multi-threaded", thread_func, NULL);

  for (i = 0; i < G_N_ELEMENTS (threads); i++)
    g_thread_join (threads[i]);

  g_assert_cmpint (shared, ==, 42);
}

static void
test_once_init_string (void)
{
  static gchar *val;

  g_test_summary ("Test g_once_init_{enter,leave}() usage with a string");

  if (g_once_init_enter_pointer (&val))
    g_once_init_leave_pointer (&val, "foo");

  g_assert_cmpstr (val, ==, "foo");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/once/single-threaded", test_once_single_threaded);
  g_test_add_func ("/once/multi-threaded", test_once_multi_threaded);
  g_test_add_func ("/once-init/single-threaded", test_once_init_single_threaded);
  g_test_add_func ("/once-init/multi-threaded", test_once_init_multi_threaded);
  g_test_add_func ("/once-init/string", test_once_init_string);

  return g_test_run ();
}
