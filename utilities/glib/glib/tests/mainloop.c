/* Unit tests for GMainLoop
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
#include <glib/gstdio.h>
#include "glib-private.h"
#include "gvalgrind.h"
#include <stdio.h>
#include <string.h>

static void
clear_ready_time (GSource *source)
{
  if (g_test_rand_bit ())
    g_source_clear_ready_time (source);
  else
    g_source_set_ready_time (source, -1);
}

static gboolean
cb (gpointer data)
{
  return FALSE;
}

static gboolean
prepare (GSource *source, gint *time)
{
  g_assert_nonnull (time);
  g_assert_cmpint (*time, ==, -1);
  return FALSE;
}
static gboolean
check (GSource *source)
{
  return FALSE;
}
static gboolean
dispatch (GSource *source, GSourceFunc cb, gpointer date)
{
  return FALSE;
}

static GSourceFuncs global_funcs = {
  prepare,
  check,
  dispatch,
  NULL,
  NULL,
  NULL
};

static void
test_maincontext_basic (void)
{
  GMainContext *ctx, *source_ctx;
  GSource *source;
  guint id;
  gpointer data = &global_funcs;

  ctx = g_main_context_new ();

  g_assert_false (g_main_context_pending (ctx));
  g_assert_false (g_main_context_iteration (ctx, FALSE));

  source = g_source_new (&global_funcs, sizeof (GSource));
  g_assert_cmpint (g_source_get_priority (source), ==, G_PRIORITY_DEFAULT);
  g_assert_false (g_source_is_destroyed (source));

  g_assert_false (g_source_get_can_recurse (source));
  g_assert_null (g_source_get_name (source));

  g_source_set_can_recurse (source, TRUE);
  g_source_set_static_name (source, "d");

  g_assert_true (g_source_get_can_recurse (source));
  g_assert_cmpstr (g_source_get_name (source), ==, "d");

  g_source_set_static_name (source, "still d");
  g_assert_cmpstr (g_source_get_name (source), ==, "still d");

  g_assert_null (g_main_context_find_source_by_user_data (ctx, NULL));
  g_assert_null (g_main_context_find_source_by_funcs_user_data (ctx, &global_funcs, NULL));

  id = g_source_attach (source, ctx);
  g_assert_cmpint (g_source_get_id (source), ==, id);
  g_assert_true (g_main_context_find_source_by_id (ctx, id) == source);

  g_source_set_priority (source, G_PRIORITY_HIGH);
  g_assert_cmpint (g_source_get_priority (source), ==, G_PRIORITY_HIGH);

  g_source_destroy (source);
  g_assert_true (g_source_get_context (source) == ctx);
  g_assert_null (g_main_context_find_source_by_id (ctx, id));
  source_ctx = g_source_dup_context (source);
  g_assert_true (source_ctx == ctx);
  g_main_context_unref (source_ctx);
  source_ctx = NULL;

  g_main_context_unref (ctx);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion*source->context != NULL*failed*");
      g_assert_null (g_source_get_context (source));
      g_test_assert_expected_messages ();
    }

  g_source_unref (source);

  ctx = g_main_context_default ();
  source = g_source_new (&global_funcs, sizeof (GSource));
  g_source_set_funcs (source, &global_funcs);
  g_source_set_callback (source, cb, data, NULL);
  id = g_source_attach (source, ctx);
  g_source_unref (source);
  g_source_set_name_by_id (id, "e");
  g_assert_cmpstr (g_source_get_name (source), ==, "e");
  g_assert_true (g_source_get_context (source) == ctx);
  g_assert_true (g_source_remove_by_funcs_user_data (&global_funcs, data));

  source = g_source_new (&global_funcs, sizeof (GSource));
  g_source_set_funcs (source, &global_funcs);
  g_source_set_callback (source, cb, data, NULL);
  id = g_source_attach (source, ctx);
  g_assert_cmpint (id, >, 0);
  g_source_unref (source);
  g_assert_true (g_source_remove_by_user_data (data));
  g_assert_false (g_source_remove_by_user_data ((gpointer)0x1234));

  g_idle_add (cb, data);
  g_assert_true (g_idle_remove_by_data (data));
}

static void
test_mainloop_basic (void)
{
  GMainLoop *loop;
  GMainContext *ctx;

  loop = g_main_loop_new (NULL, FALSE);

  g_assert_false (g_main_loop_is_running (loop));

  g_main_loop_ref (loop);

  ctx = g_main_loop_get_context (loop);
  g_assert_true (ctx == g_main_context_default ());

  g_main_loop_unref (loop);

  g_assert_cmpint (g_main_depth (), ==, 0);

  g_main_loop_unref (loop);
}

static void
test_ownerless_polling (gconstpointer test_data)
{
  gboolean attach_first = GPOINTER_TO_INT (test_data);
  GMainContext *ctx = g_main_context_new_with_flags (
    G_MAIN_CONTEXT_FLAGS_OWNERLESS_POLLING);

  GPollFD fds[20];
  gint fds_size;
  gint max_priority;
  GSource *source = NULL;

  g_assert_true (ctx != g_main_context_default ());

  g_main_context_push_thread_default (ctx);

  /* Drain events */
  for (;;)
    {
      gboolean ready_to_dispatch = g_main_context_prepare (ctx, &max_priority);
      gint timeout, nready;
      fds_size = g_main_context_query (ctx, max_priority, &timeout, fds, G_N_ELEMENTS (fds));
      nready = g_poll (fds, fds_size, /*timeout=*/0);
      if (!ready_to_dispatch && nready == 0)
        {
          if (timeout == -1)
            break;
          else
            g_usleep (timeout * 1000);
        }
      ready_to_dispatch = g_main_context_check (ctx, max_priority, fds, fds_size);
      if (ready_to_dispatch)
        g_main_context_dispatch (ctx);
    }

  if (!attach_first)
    g_main_context_pop_thread_default (ctx);

  source = g_idle_source_new ();
  g_source_attach (source, ctx);
  g_source_unref (source);

  if (attach_first)
    g_main_context_pop_thread_default (ctx);

  g_assert_cmpint (g_poll (fds, fds_size, 0), >, 0);

  g_main_context_unref (ctx);
}

static gint global_a;
static gint global_b;
static gint global_c;

static gboolean
count_calls (gpointer data)
{
  gint *i = data;

  (*i)++;

  return TRUE;
}

static void
test_timeouts (void)
{
  GMainContext *ctx;
  GMainLoop *loop;
  GSource *source;

  if (!g_test_thorough ())
    {
      g_test_skip ("Not running timing heavy test");
      return;
    }

  global_a = global_b = global_c = 0;

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  source = g_timeout_source_new (100);
  g_source_set_callback (source, count_calls, &global_a, NULL);
  g_source_attach (source, ctx);
  g_source_unref (source);

  source = g_timeout_source_new (250);
  g_source_set_callback (source, count_calls, &global_b, NULL);
  g_source_attach (source, ctx);
  g_source_unref (source);

  source = g_timeout_source_new (330);
  g_source_set_callback (source, count_calls, &global_c, NULL);
  g_source_attach (source, ctx);
  g_source_unref (source);

  source = g_timeout_source_new (1050);
  g_source_set_callback (source, (GSourceFunc)g_main_loop_quit, loop, NULL);
  g_source_attach (source, ctx);
  g_source_unref (source);

  g_main_loop_run (loop);

  /* We may be delayed for an arbitrary amount of time - for example,
   * it's possible for all timeouts to fire exactly once.
   */
  g_assert_cmpint (global_a, >, 0);
  g_assert_cmpint (global_a, >=, global_b);
  g_assert_cmpint (global_b, >=, global_c);

  g_assert_cmpint (global_a, <=, 10);
  g_assert_cmpint (global_b, <=, 4);
  g_assert_cmpint (global_c, <=, 3);

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}

static void
test_timeouts_ns (void)
{
  const uint64_t test_duration = G_NSEC_PER_SEC;
  GMainContext *ctx;
  GMainLoop *loop;
  GSource *source;
  gsize i;
  int counters[31] = { 0, };

  if (!g_test_thorough ())
    {
      g_test_skip ("Not running timing heavy test");
      return;
    }

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  for (i = 0; i < G_N_ELEMENTS (counters); i++)
    {
      if (i == G_N_ELEMENTS (counters) - 1)
        source = g_timeout_source_new_ns (UINT64_MAX);
      else
        source = g_timeout_source_new_ns (((uint64_t) 1) << i);
      g_source_set_callback (source, count_calls, &counters[i], NULL);
      g_source_attach (source, ctx);
      g_source_unref (source);
    }

  source = g_timeout_source_new_ns (test_duration);
  g_source_set_callback (source, (GSourceFunc) g_main_loop_quit, loop, NULL);
  g_source_attach (source, ctx);
  g_source_unref (source);

  g_main_loop_run (loop);

  for (i = 0; i < G_N_ELEMENTS (counters) - 1; i++)
    {
      /* We may be delayed for an arbitrary amount of time - for example,
       * it's possible for all timeouts to fire exactly once, when the
       * source fires that quits the main loop.
       */
      g_assert_cmpint (counters[i], >, 0);
      /* Counters must not fire more than there was time available. */
      g_assert_cmpint (counters[i], <=, (test_duration >> i) + 1);
      /* Each counter has a lower timeout than the next, so they must fire
       * at least as often.
       */
      g_assert_cmpint (counters[i], >=, counters[i+1]);

      if (g_test_verbose())
        g_test_message ("%9uns source fired%3d%% - %9d/%u",
                        1 << i,
                        100 * counters[i] / ((int) (test_duration >> i) + 1),
                        counters[i], (unsigned) (test_duration >> i) + 1);
    }

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}

static void
test_priorities (void)
{
  GMainContext *ctx;
  GSource *sourcea;
  GSource *sourceb;

  global_a = global_b = global_c = 0;

  ctx = g_main_context_new ();

  sourcea = g_idle_source_new ();
  g_source_set_callback (sourcea, count_calls, &global_a, NULL);
  g_source_set_priority (sourcea, 1);
  g_source_attach (sourcea, ctx);
  g_source_unref (sourcea);

  sourceb = g_idle_source_new ();
  g_source_set_callback (sourceb, count_calls, &global_b, NULL);
  g_source_set_priority (sourceb, 0);
  g_source_attach (sourceb, ctx);
  g_source_unref (sourceb);

  g_assert_true (g_main_context_pending (ctx));
  g_assert_true (g_main_context_iteration (ctx, FALSE));
  g_assert_cmpint (global_a, ==, 0);
  g_assert_cmpint (global_b, ==, 1);

  g_assert_true (g_main_context_iteration (ctx, FALSE));
  g_assert_cmpint (global_a, ==, 0);
  g_assert_cmpint (global_b, ==, 2);

  g_source_destroy (sourceb);

  g_assert_true (g_main_context_iteration (ctx, FALSE));
  g_assert_cmpint (global_a, ==, 1);
  g_assert_cmpint (global_b, ==, 2);

  g_assert_true (g_main_context_pending (ctx));
  g_source_destroy (sourcea);
  g_assert_false (g_main_context_pending (ctx));

  g_main_context_unref (ctx);
}

static void
test_prepare_timeout (void)
{
  GMainContext *context = NULL;
  GSource *source = NULL;
  int priority = 0;
  gboolean ready;

  g_test_summary ("Test that a GTimeoutSource which is ready is noticed during prepare");
  g_test_bug ("https://gitlab.gnome.org/GNOME/mutter/-/work_items/4994");

  context = g_main_context_new ();

  source = g_timeout_source_new (0);
  g_source_set_priority (source, 666);
  g_source_attach (source, context);
  g_clear_pointer (&source, g_source_unref);

  ready = g_main_context_prepare (context, &priority);

  g_assert_true (ready);
  g_assert_cmpint (priority, ==, 666);

  g_clear_pointer (&context, g_main_context_unref);
}

static gboolean
quit_loop (gpointer data)
{
  GMainLoop *loop = data;

  g_main_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static gint count;

static gboolean
func (gpointer data)
{
  if (data != NULL)
    g_assert_true (data == g_thread_self ());

  count++;

  return FALSE;
}

static gboolean
call_func (gpointer data)
{
  func (g_thread_self ());

  return G_SOURCE_REMOVE;
}

static GMutex mutex;
static GCond cond;
static gboolean thread_ready;

static gpointer
thread_func (gpointer data)
{
  GMainContext *ctx = data;
  GMainLoop *loop;
  GSource *source;

  g_main_context_push_thread_default (ctx);
  loop = g_main_loop_new (ctx, FALSE);

  g_mutex_lock (&mutex);
  thread_ready = TRUE;
  g_cond_signal (&cond);
  g_mutex_unlock (&mutex);

  source = g_timeout_source_new (500);
  g_source_set_callback (source, quit_loop, loop, NULL);
  g_source_attach (source, ctx);
  g_source_unref (source);

  g_main_loop_run (loop);

  g_main_context_pop_thread_default (ctx);
  g_main_loop_unref (loop);

  return NULL;
}

static void
test_invoke (void)
{
  GMainContext *ctx;
  GThread *thread;

  count = 0;

  /* this one gets invoked directly */
  g_main_context_invoke (NULL, func, g_thread_self ());
  g_assert_cmpint (count, ==, 1);

  /* invoking out of an idle works too */
  g_idle_add (call_func, NULL);
  g_main_context_iteration (g_main_context_default (), FALSE);
  g_assert_cmpint (count, ==, 2);

  /* test thread-default forcing the invocation to go
   * to another thread
   */
  ctx = g_main_context_new ();
  thread = g_thread_new ("worker", thread_func, ctx);

  g_mutex_lock (&mutex);
  while (!thread_ready)
    g_cond_wait (&cond, &mutex);
  g_mutex_unlock (&mutex);

  g_main_context_invoke (ctx, func, thread);

  g_thread_join (thread);
  g_assert_cmpint (count, ==, 3);

  g_main_context_unref (ctx);
}

/* We can't use timeout sources here because on slow or heavily-loaded
 * machines, the test program might not get enough cycles to hit the
 * timeouts at the expected times. So instead we define a source that
 * is based on the number of GMainContext iterations.
 */

static gint counter;
static gint64 last_counter_update;

typedef struct {
  GSource source;
  gint    interval;
  gint    timeout;
} CounterSource;

static gboolean
counter_source_prepare (GSource *source,
                        gint    *timeout)
{
  CounterSource *csource = (CounterSource *)source;
  gint64 now;

  now = g_source_get_time (source);
  if (now != last_counter_update)
    {
      last_counter_update = now;
      counter++;
    }

  *timeout = 1;
  return counter >= csource->timeout;
}

static gboolean
counter_source_dispatch (GSource    *source,
                         GSourceFunc callback,
                         gpointer    user_data)
{
  CounterSource *csource = (CounterSource *) source;
  gboolean again;

  again = callback (user_data);

  if (again)
    csource->timeout = counter + csource->interval;

  return again;
}

static GSourceFuncs counter_source_funcs = {
  counter_source_prepare,
  NULL,
  counter_source_dispatch,
  NULL,
  NULL,
  NULL
};

static GSource *
counter_source_new (gint interval)
{
  GSource *source = g_source_new (&counter_source_funcs, sizeof (CounterSource));
  CounterSource *csource = (CounterSource *) source;

  csource->interval = interval;
  csource->timeout = counter + interval;

  return source;
}


static gboolean
run_inner_loop (gpointer user_data)
{
  GMainContext *ctx = user_data;
  GMainLoop *inner;
  GSource *timeout;

  global_a++;

  inner = g_main_loop_new (ctx, FALSE);
  timeout = counter_source_new (100);
  g_source_set_callback (timeout, quit_loop, inner, NULL);
  g_source_attach (timeout, ctx);
  g_source_unref (timeout);

  g_main_loop_run (inner);
  g_main_loop_unref (inner);

  return G_SOURCE_CONTINUE;
}

static void
test_child_sources (void)
{
  GMainContext *ctx;
  GMainLoop *loop;
  GSource *parent, *child_b, *child_c, *end;

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  global_a = global_b = global_c = 0;

  parent = counter_source_new (2000);
  g_source_set_callback (parent, run_inner_loop, ctx, NULL);
  g_source_set_priority (parent, G_PRIORITY_LOW);
  g_source_attach (parent, ctx);

  child_b = counter_source_new (250);
  g_source_set_callback (child_b, count_calls, &global_b, NULL);
  g_source_add_child_source (parent, child_b);

  child_c = counter_source_new (330);
  g_source_set_callback (child_c, count_calls, &global_c, NULL);
  g_source_set_priority (child_c, G_PRIORITY_HIGH);
  g_source_add_child_source (parent, child_c);

  /* Child sources always have the priority of the parent */
  g_assert_cmpint (g_source_get_priority (parent), ==, G_PRIORITY_LOW);
  g_assert_cmpint (g_source_get_priority (child_b), ==, G_PRIORITY_LOW);
  g_assert_cmpint (g_source_get_priority (child_c), ==, G_PRIORITY_LOW);
  g_source_set_priority (parent, G_PRIORITY_DEFAULT);
  g_assert_cmpint (g_source_get_priority (parent), ==, G_PRIORITY_DEFAULT);
  g_assert_cmpint (g_source_get_priority (child_b), ==, G_PRIORITY_DEFAULT);
  g_assert_cmpint (g_source_get_priority (child_c), ==, G_PRIORITY_DEFAULT);

  end = counter_source_new (1050);
  g_source_set_callback (end, quit_loop, loop, NULL);
  g_source_attach (end, ctx);
  g_source_unref (end);

  g_main_loop_run (loop);

  /* The parent source's own timeout will never trigger, so "a" will
   * only get incremented when "b" or "c" does. And when timeouts get
   * blocked, they still wait the full interval next time rather than
   * "catching up". So the timing is:
   *
   *  250 - b++ -> a++, run_inner_loop
   *  330 - (c is blocked)
   *  350 - inner_loop ends
   *  350 - c++ belatedly -> a++, run_inner_loop
   *  450 - inner loop ends
   *  500 - b++ -> a++, run_inner_loop
   *  600 - inner_loop ends
   *  680 - c++ -> a++, run_inner_loop
   *  750 - (b is blocked)
   *  780 - inner loop ends
   *  780 - b++ belatedly -> a++, run_inner_loop
   *  880 - inner loop ends
   * 1010 - c++ -> a++, run_inner_loop
   * 1030 - (b is blocked)
   * 1050 - end runs, quits outer loop, which has no effect yet
   * 1110 - inner loop ends, a returns, outer loop exits
   */

  g_assert_cmpint (global_a, ==, 6);
  g_assert_cmpint (global_b, ==, 3);
  g_assert_cmpint (global_c, ==, 3);

  g_source_destroy (parent);
  g_source_unref (parent);
  g_source_unref (child_b);
  g_source_unref (child_c);

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}

static void
test_recursive_child_sources (void)
{
  GMainContext *ctx;
  GMainLoop *loop;
  GSource *parent, *child_b, *child_c, *end;

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  global_a = global_b = global_c = 0;

  parent = counter_source_new (500);
  g_source_set_callback (parent, count_calls, &global_a, NULL);

  child_b = counter_source_new (220);
  g_source_set_callback (child_b, count_calls, &global_b, NULL);
  g_source_add_child_source (parent, child_b);

  child_c = counter_source_new (430);
  g_source_set_callback (child_c, count_calls, &global_c, NULL);
  g_source_add_child_source (child_b, child_c);

  g_source_attach (parent, ctx);

  end = counter_source_new (2010);
  g_source_set_callback (end, (GSourceFunc)g_main_loop_quit, loop, NULL);
  g_source_attach (end, ctx);
  g_source_unref (end);

  g_main_loop_run (loop);

  /* Sequence of events:
   *  220 b (b -> 440, a -> 720)
   *  430 c (c -> 860, b -> 650, a -> 930)
   *  650 b (b -> 870, a -> 1150)
   *  860 c (c -> 1290, b -> 1080, a -> 1360)
   * 1080 b (b -> 1300, a -> 1580)
   * 1290 c (c -> 1720, b -> 1510, a -> 1790)
   * 1510 b (b -> 1730, a -> 2010)
   * 1720 c (c -> 2150, b -> 1940, a -> 2220)
   * 1940 b (b -> 2160, a -> 2440)
   */

  g_assert_cmpint (global_a, ==, 9);
  g_assert_cmpint (global_b, ==, 9);
  g_assert_cmpint (global_c, ==, 4);

  g_source_destroy (parent);
  g_source_unref (parent);
  g_source_unref (child_b);
  g_source_unref (child_c);

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}

typedef struct {
  GSource *parent, *old_child, *new_child;
  GMainLoop *loop;
} SwappingTestData;

static gboolean
swap_sources (gpointer user_data)
{
  SwappingTestData *data = user_data;

  if (data->old_child)
    {
      g_source_remove_child_source (data->parent, data->old_child);
      g_clear_pointer (&data->old_child, g_source_unref);
    }

  if (!data->new_child)
    {
      data->new_child = g_timeout_source_new (0);
      g_source_set_callback (data->new_child, quit_loop, data->loop, NULL);
      g_source_add_child_source (data->parent, data->new_child);
    }

  return G_SOURCE_CONTINUE;
}

static gboolean
assert_not_reached_callback (gpointer user_data)
{
  g_assert_not_reached ();

  return G_SOURCE_REMOVE;
}

static void
test_swapping_child_sources (void)
{
  GMainContext *ctx;
  GMainLoop *loop;
  SwappingTestData data;

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  data.parent = counter_source_new (50);
  data.loop = loop;
  g_source_set_callback (data.parent, swap_sources, &data, NULL);
  g_source_attach (data.parent, ctx);

  data.old_child = counter_source_new (100);
  g_source_add_child_source (data.parent, data.old_child);
  g_source_set_callback (data.old_child, assert_not_reached_callback, NULL, NULL);

  data.new_child = NULL;
  g_main_loop_run (loop);

  g_source_destroy (data.parent);
  g_source_unref (data.parent);
  g_source_unref (data.new_child);

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}

static gboolean
add_source_callback (gpointer user_data)
{
  GMainLoop *loop = user_data;
  GSource *self = g_main_current_source (), *child;
  GIOChannel *io;

  /* It doesn't matter whether this is a valid fd or not; it never
   * actually gets polled; the test is just checking that
   * g_source_add_child_source() doesn't crash.
   */
  io = g_io_channel_unix_new (0);
  child = g_io_create_watch (io, G_IO_IN);
  g_source_add_child_source (self, child);
  g_source_unref (child);
  g_io_channel_unref (io);

  g_main_loop_quit (loop);
  return FALSE;
}

static void
test_blocked_child_sources (void)
{
  GMainContext *ctx;
  GMainLoop *loop;
  GSource *source;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=701283");

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  source = g_idle_source_new ();
  g_source_set_callback (source, add_source_callback, loop, NULL);
  g_source_attach (source, ctx);

  g_main_loop_run (loop);

  g_source_destroy (source);
  g_source_unref (source);

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}

typedef struct {
  GMainContext *ctx;
  GMainLoop *loop;

  GSource *timeout1, *timeout2;
  gint64 time1;
  uint64_t time1_ns;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  GTimeVal tv;  /* needed for g_source_get_current_time() */
G_GNUC_END_IGNORE_DEPRECATIONS
} TimeTestData;

static gboolean
timeout1_callback (gpointer user_data)
{
  TimeTestData *data = user_data;
  GSource *source;
  gint64 mtime1, mtime2, time2;
  uint64_t mtime1_ns, mtime2_ns, time2_ns;

  source = g_main_current_source ();
  g_assert_true (source == data->timeout1);

  if (data->time1 == -1)
    {
      /* First iteration */
      g_assert_false (g_source_is_destroyed (data->timeout2));

      mtime1 = g_get_monotonic_time ();
      data->time1 = g_source_get_time (source);
      mtime1_ns = g_get_monotonic_time_ns ();
      data->time1_ns = g_source_get_time_ns (source);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_source_get_current_time (source, &data->tv);
G_GNUC_END_IGNORE_DEPRECATIONS

      /* g_source_get_time() does not change during a single callback */
      g_usleep (1000000);
      mtime2 = g_get_monotonic_time ();
      time2 = g_source_get_time (source);
      mtime2_ns = g_get_monotonic_time_ns ();
      time2_ns = g_source_get_time_ns (source);

      g_assert_cmpint (mtime1, <, mtime2);
      g_assert_cmpint (data->time1, ==, time2);
      g_assert_cmpuint (mtime1_ns, <, mtime2_ns);
      g_assert_cmpuint (data->time1_ns, ==, time2_ns);
    }
  else
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      GTimeVal tv;
G_GNUC_END_IGNORE_DEPRECATIONS

      /* Second iteration */
      g_assert_true (g_source_is_destroyed (data->timeout2));

      /* g_source_get_time() MAY change between iterations; in this
       * case we know for sure that it did because of the g_usleep()
       * last time.
       */
      time2 = g_source_get_time (source);
      g_assert_cmpint (data->time1, <, time2);
      time2_ns = g_source_get_time_ns (source);
      g_assert_cmpuint (data->time1_ns, <, time2_ns);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_source_get_current_time (source, &tv);
G_GNUC_END_IGNORE_DEPRECATIONS

      g_assert_true (tv.tv_sec > data->tv.tv_sec ||
                     (tv.tv_sec == data->tv.tv_sec &&
                      tv.tv_usec > data->tv.tv_usec));

      g_main_loop_quit (data->loop);
    }

  return TRUE;
}

static gboolean
timeout2_callback (gpointer user_data)
{
  TimeTestData *data = user_data;
  GSource *source;
  gint64 time2, time3;
  uint64_t time2_ns, time3_ns;

  source = g_main_current_source ();
  g_assert_true (source == data->timeout2);

  g_assert_false (g_source_is_destroyed (data->timeout1));

  /* g_source_get_time() does not change between different sources in
   * a single iteration of the mainloop.
   */
  time2 = g_source_get_time (source);
  g_assert_cmpint (data->time1, ==, time2);
  time2_ns = g_source_get_time_ns (source);
  g_assert_cmpuint (data->time1_ns, ==, time2_ns);

  /* The source should still have a valid time even after being
   * destroyed, since it's currently running.
   */
  g_source_destroy (source);
  time3 = g_source_get_time (source);
  g_assert_cmpint (time2, ==, time3);
  time3_ns = g_source_get_time_ns (source);
  g_assert_cmpuint (time2_ns, ==, time3_ns);

  return FALSE;
}

static void
test_source_time (void)
{
  TimeTestData data;

  data.ctx = g_main_context_new ();
  data.loop = g_main_loop_new (data.ctx, FALSE);

  data.timeout1 = g_timeout_source_new (0);
  g_source_set_callback (data.timeout1, timeout1_callback, &data, NULL);
  g_source_attach (data.timeout1, data.ctx);

  data.timeout2 = g_timeout_source_new (0);
  g_source_set_callback (data.timeout2, timeout2_callback, &data, NULL);
  g_source_attach (data.timeout2, data.ctx);

  data.time1 = -1;

  g_main_loop_run (data.loop);

  g_assert_false (g_source_is_destroyed (data.timeout1));
  g_assert_true (g_source_is_destroyed (data.timeout2));

  g_source_destroy (data.timeout1);
  g_source_unref (data.timeout1);
  g_source_unref (data.timeout2);

  g_main_loop_unref (data.loop);
  g_main_context_unref (data.ctx);
}

typedef struct {
  guint outstanding_ops;
  GMainLoop *loop;
} TestOverflowData;

static gboolean
on_source_fired_cb (gpointer user_data)
{
  TestOverflowData *data = user_data;
  GSource *current_source;
  GMainContext *current_context;
  guint source_id;

  data->outstanding_ops--;

  current_source = g_main_current_source ();
  current_context = g_source_get_context (current_source);
  source_id = g_source_get_id (current_source);
  g_assert_nonnull (g_main_context_find_source_by_id (current_context, source_id));
  g_source_destroy (current_source);
  g_assert_null (g_main_context_find_source_by_id (current_context, source_id));

  if (data->outstanding_ops == 0)
    g_main_loop_quit (data->loop);
  return FALSE;
}

static GSource *
add_idle_source (GMainContext *ctx,
                 TestOverflowData *data)
{
  GSource *source;

  source = g_idle_source_new ();
  g_source_set_callback (source, on_source_fired_cb, data, NULL);
  g_source_attach (source, ctx);
  g_source_unref (source);
  data->outstanding_ops++;

  return source;
}

static void
test_mainloop_overflow (void)
{
  GMainContext *ctx;
  GMainLoop *loop;
  GSource *source;
  TestOverflowData data;
  guint i;

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=687098");

  memset (&data, 0, sizeof (data));

  ctx = GLIB_PRIVATE_CALL (g_main_context_new_with_next_id) (G_MAXUINT-1);

  loop = g_main_loop_new (ctx, TRUE);
  data.outstanding_ops = 0;
  data.loop = loop;

  source = add_idle_source (ctx, &data);
  g_assert_cmpint (source->source_id, ==, G_MAXUINT-1);

  source = add_idle_source (ctx, &data);
  g_assert_cmpint (source->source_id, ==, G_MAXUINT);

  source = add_idle_source (ctx, &data);
  g_assert_cmpint (source->source_id, !=, 0);

  /* Now, a lot more sources */
  for (i = 0; i < 50; i++)
    {
      source = add_idle_source (ctx, &data);
      g_assert_cmpint (source->source_id, !=, 0);
    }

  g_main_loop_run (loop);
  g_assert_cmpint (data.outstanding_ops, ==, 0);

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}

static gint ready_time_dispatched;  /* (atomic) */

static gboolean
ready_time_dispatch (GSource     *source,
                     GSourceFunc  callback,
                     gpointer     user_data)
{
  g_atomic_int_set (&ready_time_dispatched, TRUE);

  clear_ready_time (source);

  return TRUE;
}

static gpointer
run_context (gpointer user_data)
{
  g_main_loop_run (user_data);

  return NULL;
}

static void
test_ready_time (void)
{
  GThread *thread;
  GSource *source;
  GSourceFuncs source_funcs = {
    NULL, NULL, ready_time_dispatch, NULL, NULL, NULL
  };
  GMainLoop *loop;
  uint64_t ready_time_ns;

  source = g_source_new (&source_funcs, sizeof (GSource));
  g_source_attach (source, NULL);
  g_source_unref (source);

  /* Unfortunately we can't do too many things with respect to timing
   * without getting into trouble on slow systems or heavily loaded
   * builders.
   *
   * We can test that the basics are working, though.
   */

  /* A source with no ready time set should not fire */
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_false (g_atomic_int_get (&ready_time_dispatched));

  /* The ready time should not have been changed */
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* Of course this shouldn't change anything either */
  clear_ready_time (source);
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* A source with a ready time set to tomorrow should not fire on any
   * builder, no matter how badly loaded...
   */
  g_source_set_ready_time (source, g_get_monotonic_time () + G_TIME_SPAN_DAY);
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_false (g_atomic_int_get (&ready_time_dispatched));
  /* Make sure it didn't get reset */
  g_assert_cmpint (g_source_get_ready_time (source), !=, -1);
  g_assert_true (g_source_get_ready_time_ns (source, &ready_time_ns));
  g_assert_cmpuint (ready_time_ns, <=, g_get_monotonic_time_ns () + G_TIME_SPAN_DAY * 1000);

  /* Ready time of -1 -> don't fire */
  clear_ready_time (source);
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_false (g_atomic_int_get (&ready_time_dispatched));
  /* Not reset, but should still be -1 from above */
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* A ready time of the current time should fire immediately */
  g_source_set_ready_time (source, g_get_monotonic_time ());
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  /* Should have gotten reset by the handler function */
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* This should also work in nanoseconds */
  g_source_set_ready_time_ns (source, g_get_monotonic_time_ns ());
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  /* Should have gotten reset by the handler function */
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* As well as one in the recent past... */
  g_source_set_ready_time (source, g_get_monotonic_time () - G_TIME_SPAN_SECOND);
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* ... also works in nanoseconds */
  g_source_set_ready_time_ns (source, g_get_monotonic_time_ns () - G_NSEC_PER_SEC);
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* Zero is the 'official' way to get a source to fire immediately */
  g_source_set_ready_time (source, 0);
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* Zero is also 'official' for immediate firing in nanoseconds */
  g_source_set_ready_time_ns (source, 0);
  while (g_main_context_iteration (NULL, FALSE));
  g_assert_true (g_atomic_int_get (&ready_time_dispatched));
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  /* Now do some tests of cross-thread wakeups.
   *
   * Make sure it wakes up right away from the start.
   */
  g_source_set_ready_time (source, 0);
  loop = g_main_loop_new (NULL, FALSE);
  thread = g_thread_new ("context thread", run_context, loop);
  while (!g_atomic_int_get (&ready_time_dispatched));

  /* Now let's see if it can wake up from sleeping. */
  g_usleep (G_TIME_SPAN_SECOND / 2);
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  g_source_set_ready_time (source, 0);
  while (!g_atomic_int_get (&ready_time_dispatched));

  /* Play the same game, but this time in nanoseconds */
  g_usleep (G_TIME_SPAN_SECOND / 2);
  g_atomic_int_set (&ready_time_dispatched, FALSE);
  g_source_set_ready_time_ns (source, 0);
  while (!g_atomic_int_get (&ready_time_dispatched));

  /* kill the thread */
  g_main_loop_quit (loop);
  g_thread_join (thread);
  g_main_loop_unref (loop);

  g_source_destroy (source);
}

static gboolean
clear_ready_time_dispatch (GSource     *source,
                           GSourceFunc  callback,
                           gpointer     user_data)
{
  uint64_t ready_time_ns;

  g_assert_cmpint (g_source_get_ready_time (source), !=, -1);
  g_assert_cmpint (g_source_get_ready_time (source), <=, g_source_get_time (source));
  g_assert_true (g_source_get_ready_time_ns (source, &ready_time_ns));
  g_assert_cmpuint (ready_time_ns, <=, g_source_get_time_ns (source));

  clear_ready_time (source);

  g_assert_cmpint (g_source_get_ready_time (source), ==, -1);
  g_assert_false (g_source_get_ready_time_ns (source, &ready_time_ns));

  return G_SOURCE_CONTINUE;
}

static void
test_various_ready_times (void)
{
  GSourceFuncs source_funcs = {
    NULL, NULL, clear_ready_time_dispatch, NULL, NULL, NULL
  };
  gint64 ready_times[] = {
    G_MININT64,
    G_MININT64 + 1,
    -2,
    -1,
    0,
    1,
    G_MAXINT64 - 1,
    G_MAXINT64,
  };
  uint64_t ready_times_ns[] = {
    0,
    1,
    1189998819991197253,
    UINT64_MAX - 1,
    UINT64_MAX,
  };
  GSource *sources[G_N_ELEMENTS (ready_times) + G_N_ELEMENTS (ready_times_ns) + 100];
  GMainContext *ctx;
  GMainLoop *loop;
  GSource *source;
  gsize i;
  uint64_t now;

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  now = g_get_monotonic_time_ns ();

  for (i = 0; i < G_N_ELEMENTS (sources); i++)
    {
      sources[i] = g_source_new (&source_funcs, sizeof (GSource));
      if (i < G_N_ELEMENTS (ready_times))
        g_source_set_ready_time (sources[i], ready_times[i]);
      else if (i < G_N_ELEMENTS (ready_times) + G_N_ELEMENTS (ready_times_ns))
        g_source_set_ready_time_ns (sources[i], ready_times_ns[i - G_N_ELEMENTS (ready_times)]);
      else
        g_source_set_ready_time_ns (sources[i], now + g_test_rand_int_range (0, G_MAXINT) - G_NSEC_PER_SEC / 2);

      g_source_attach (sources[i], ctx);
    }

  source = g_timeout_source_new_ns (G_NSEC_PER_SEC);
  g_source_set_callback (source, (GSourceFunc) g_main_loop_quit, loop, NULL);
  g_source_attach (source, ctx);
  g_source_unref (source);

  g_main_loop_run (loop);

  for (i = 0; i < G_N_ELEMENTS (sources); i++)
    {
      uint64_t ready_time_ns;

      if (g_source_get_ready_time_ns (sources[i], &ready_time_ns))
        {
          /* hasn't triggered yet, so ready time must be in the future */
          g_assert_cmpuint (ready_time_ns, >, g_source_get_time_ns (sources[i]));
        }
      g_source_unref (sources[i]);
    }

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);
}

static void
test_wakeup (void)
{
  GMainContext *ctx;
  int i;

  ctx = g_main_context_new ();

  /* run a random large enough number of times because 
   * main contexts tend to wake up a few times after creation.
   */
  for (i = 0; i < 100; i++)
    {
      /* This is the invariant we care about:
       * g_main_context_wakeup(ctx,) ensures that the next call to
       * g_main_context_iteration (ctx, TRUE) returns and doesn't
       * block.
       * This is important in threaded apps where we might not know
       * if the thread calls g_main_context_wakeup() before or after
       * we enter g_main_context_iteration().
       */
      g_main_context_wakeup (ctx);
      g_main_context_iteration (ctx, TRUE);
    }

  g_main_context_unref (ctx);
}

static void
test_remove_invalid (void)
{
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL, "Source ID 3000000000 was not found*");
  g_source_remove (3000000000u);
  g_test_assert_expected_messages ();
}

static gboolean
trivial_prepare (GSource *source,
                 gint    *timeout)
{
  *timeout = 0;
  return TRUE;
}

static gint n_finalized;

static void
trivial_finalize (GSource *source)
{
  n_finalized++;
}

static void
test_unref_while_pending (void)
{
  static GSourceFuncs funcs = {
    trivial_prepare, NULL, NULL, trivial_finalize, NULL, NULL
  };
  GMainContext *context;
  GSource *source;

  context = g_main_context_new ();

  source = g_source_new (&funcs, sizeof (GSource));
  g_source_attach (source, context);
  g_source_unref (source);

  /* Do incomplete main iteration -- get a pending source but don't dispatch it. */
  g_main_context_prepare (context, NULL);
  g_main_context_query (context, 0, NULL, NULL, 0);
  g_main_context_check (context, 1000, NULL, 0);

  /* Destroy the context */
  g_main_context_unref (context);

  /* Make sure we didn't leak the source */
  g_assert_cmpint (n_finalized, ==, 1);
}

static void
test_null_default_context (void)
{
  int n_poll_fds;
  GPollFD poll_fds[10];

  g_test_message ("Test that the global default main context is used if NULL is passed to various methods");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3818");

  g_assert_true (g_main_context_acquire (NULL));
  g_assert_false (g_main_context_prepare (NULL, NULL));
  n_poll_fds = g_main_context_query (NULL, 0, NULL, poll_fds, G_N_ELEMENTS (poll_fds));
  g_assert_cmpint (n_poll_fds, ==, 1);  /* one pollfd always exists for gwakeup */
  g_assert_false (g_main_context_check (NULL, 1000, poll_fds, n_poll_fds));
  g_main_context_dispatch (NULL);
  g_main_context_release (NULL);
}

typedef struct {
  GSource parent;
  GMainLoop *loop;
} LoopedSource;

static gboolean
prepare_loop_run (GSource *source, gint *time)
{
  LoopedSource *looped_source = (LoopedSource*) source;
  *time = 0;

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*called recursively from within a source's check() "
                         "or prepare() member*");
  g_main_loop_run (looped_source->loop);
  g_test_assert_expected_messages ();

  return FALSE;
}

static gboolean
check_loop_run (GSource *source)
{
  LoopedSource *looped_source = (LoopedSource*) source;

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "*called recursively from within a source's check() "
                         "or prepare() member*");
  g_main_loop_run (looped_source->loop);
  g_test_assert_expected_messages ();

  return TRUE;
}

static gboolean
dispatch_loop_run (GSource    *source,
                   GSourceFunc callback,
                   gpointer    user_data)
{
  LoopedSource *looped_source = (LoopedSource*) source;

  g_main_loop_quit (looped_source->loop);

  return FALSE;
}

static void
test_recursive_loop_child_sources (void)
{
  GMainLoop *loop;
  GSource *source;
  GSourceFuncs loop_run_funcs = {
    prepare_loop_run, check_loop_run, dispatch_loop_run, NULL, NULL, NULL,
  };

  loop = g_main_loop_new (NULL, FALSE);

  source = g_source_new (&loop_run_funcs, sizeof (LoopedSource));
  ((LoopedSource*)source)->loop = loop;

  g_source_attach (source, NULL);

  g_main_loop_run (loop);
  g_source_unref (source);

  g_main_loop_unref (loop);
}


#ifdef G_OS_UNIX

#include <glib-unix.h>
#include <unistd.h>

static gchar zeros[1024];

static gsize
fill_a_pipe (gint fd)
{
  gsize written = 0;
  GPollFD pfd;

  pfd.fd = fd;
  pfd.events = G_IO_OUT;
  while (g_poll (&pfd, 1, 0) == 1)
    /* we should never see -1 here */
    written += write (fd, zeros, sizeof zeros);

  return written;
}

static gboolean
write_bytes (gint         fd,
             GIOCondition condition,
             gpointer     user_data)
{
  gssize *to_write = user_data;
  gint limit;

  if (*to_write == 0)
    return FALSE;

  /* Detect if we run before we should */
  g_assert_cmpint (*to_write, >=, 0);

  limit = MIN ((gsize) *to_write, sizeof zeros);
  *to_write -= write (fd, zeros, limit);

  return TRUE;
}

static gboolean
read_bytes (gint         fd,
            GIOCondition condition,
            gpointer     user_data)
{
  static gchar buffer[1024];
  gssize *to_read = user_data;

  *to_read -= read (fd, buffer, sizeof buffer);

  /* The loop will exit when there is nothing else to read, then we will
   * use g_source_remove() to destroy this source.
   */
  return TRUE;
}

#ifdef G_OS_UNIX
static void
test_unix_fd (void)
{
  gssize to_write = -1;
  gssize to_read;
  gint fds[2];
  gint a, b;
  gint s;
  GSource *source_a;
  GSource *source_b;

  s = pipe (fds);
  g_assert_cmpint (s, ==, 0);

  to_read = fill_a_pipe (fds[1]);
  /* write at higher priority to keep the pipe full... */
  a = g_unix_fd_add_full (G_PRIORITY_HIGH, fds[1], G_IO_OUT, write_bytes, &to_write, NULL);
  source_a = g_source_ref (g_main_context_find_source_by_id (NULL, a));
  /* make sure no 'writes' get dispatched yet */
  while (g_main_context_iteration (NULL, FALSE));

  to_read += 128 * 1024 * 1024;
  to_write = 128 * 1024 * 1024;
  b = g_unix_fd_add (fds[0], G_IO_IN, read_bytes, &to_read);
  source_b = g_source_ref (g_main_context_find_source_by_id (NULL, b));

  /* Assuming the kernel isn't internally 'laggy' then there will always
   * be either data to read or room in which to write.  That will keep
   * the loop running until all data has been read and written.
   *
   * We can’t rely on the data being available in exactly one `GMainContext`
   * iteration, though, as it may be potentially deferred in favour of higher
   * priority sources.
   */
  while (to_write > 0 || to_read > 0)
    {
      gssize to_write_was = to_write;
      gssize to_read_was = to_read;

      g_main_context_iteration (NULL, TRUE);

      /* Since the sources are at different priority, only one of them
       * should possibly have run.
       */
      g_assert_true (to_write == to_write_was || to_read == to_read_was);
    }

  g_assert_cmpint (to_write, ==, 0);
  g_assert_cmpint (to_read, ==, 0);

  /* 'a' is already removed by itself */
  g_assert_true (g_source_is_destroyed (source_a));
  g_source_unref (source_a);
  g_source_remove (b);
  g_assert_true (g_source_is_destroyed (source_b));
  g_source_unref (source_b);
  close (fds[1]);
  close (fds[0]);
}
#endif

static void
assert_main_context_state (gint n_to_poll,
                           ...)
{
  GMainContext *context;
  gboolean consumed[10] = { };
  GPollFD poll_fds[10];
  gboolean acquired;
  gboolean immediate;
  gint max_priority;
  gint timeout;
  gint n;
  gint i, j;
  va_list ap;

  context = g_main_context_default ();

  acquired = g_main_context_acquire (context);
  g_assert_true (acquired);

  immediate = g_main_context_prepare (context, &max_priority);
  g_assert_false (immediate);
  n = g_main_context_query (context, max_priority, &timeout, poll_fds, 10);
  g_assert_cmpint (n, ==, n_to_poll + 1); /* one will be the gwakeup */

  va_start (ap, n_to_poll);
  for (i = 0; i < n_to_poll; i++)
    {
      gint expected_fd = va_arg (ap, gint);
      GIOCondition expected_events = va_arg (ap, GIOCondition);
      GIOCondition report_events = va_arg (ap, GIOCondition);

      for (j = 0; j < n; j++)
        if (!consumed[j] && poll_fds[j].fd == expected_fd && poll_fds[j].events == expected_events)
          {
            poll_fds[j].revents = report_events;
            consumed[j] = TRUE;
            break;
          }

      if (j == n)
        g_error ("Unable to find fd %d (index %d) with events 0x%x", expected_fd, i, (guint) expected_events);
    }
  va_end (ap);

  /* find the gwakeup, flag as non-ready */
  for (i = 0; i < n; i++)
    if (!consumed[i])
      poll_fds[i].revents = 0;

  if (g_main_context_check (context, max_priority, poll_fds, n))
    g_main_context_dispatch (context);

  g_main_context_release (context);
}

static gboolean
flag_bool (gint         fd,
           GIOCondition condition,
           gpointer     user_data)
{
  gboolean *flag = user_data;

  *flag = TRUE;

  return TRUE;
}

static void
test_unix_fd_source (void)
{
  GSource *out_source;
  GSource *in_source;
  GSource *source;
  gboolean out, in;
  gint fds[2];
  gint s;

  assert_main_context_state (0);

  s = pipe (fds);
  g_assert_cmpint (s, ==, 0);

  source = g_unix_fd_source_new (fds[1], G_IO_OUT);
  g_source_attach (source, NULL);

  /* Check that a source with no callback gets successfully detached
   * with a warning printed.
   */
  g_test_expect_message ("GLib", G_LOG_LEVEL_WARNING, "*GUnixFDSource dispatched without callback*");
  while (g_main_context_iteration (NULL, FALSE));
  g_test_assert_expected_messages ();
  g_assert_true (g_source_is_destroyed (source));
  g_source_unref (source);

  out = in = FALSE;
  out_source = g_unix_fd_source_new (fds[1], G_IO_OUT);
  /* -Wcast-function-type complains about casting 'flag_bool' to GSourceFunc.
   * GCC has no way of knowing that it will be cast back to GUnixFDSourceFunc
   * before being called. Although GLib itself is not compiled with
   * -Wcast-function-type, applications that use GLib may well be (since
   * -Wextra includes it), so we provide a G_SOURCE_FUNC() macro to suppress
   * the warning. We check that it works here.
   */
#if G_GNUC_CHECK_VERSION(8, 0)
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wcast-function-type"
#endif
  g_source_set_callback (out_source, G_SOURCE_FUNC (flag_bool), &out, NULL);
#if G_GNUC_CHECK_VERSION(8, 0)
#pragma GCC diagnostic pop
#endif
  g_source_attach (out_source, NULL);
  assert_main_context_state (1,
                             fds[1], G_IO_OUT, 0);
  g_assert_true (!in && !out);

  in_source = g_unix_fd_source_new (fds[0], G_IO_IN);
  g_source_set_callback (in_source, (GSourceFunc) flag_bool, &in, NULL);
  g_source_set_priority (in_source, G_PRIORITY_DEFAULT_IDLE);
  g_source_attach (in_source, NULL);
  assert_main_context_state (2,
                             fds[0], G_IO_IN, G_IO_IN,
                             fds[1], G_IO_OUT, G_IO_OUT);
  /* out is higher priority so only it should fire */
  g_assert_true (!in && out);

  /* raise the priority of the in source to higher than out*/
  in = out = FALSE;
  g_source_set_priority (in_source, G_PRIORITY_HIGH);
  assert_main_context_state (2,
                             fds[0], G_IO_IN, G_IO_IN,
                             fds[1], G_IO_OUT, G_IO_OUT);
  g_assert_true (in && !out);

  /* now, let them be equal */
  in = out = FALSE;
  g_source_set_priority (in_source, G_PRIORITY_DEFAULT);
  assert_main_context_state (2,
                             fds[0], G_IO_IN, G_IO_IN,
                             fds[1], G_IO_OUT, G_IO_OUT);
  g_assert_true (in && out);

  g_source_destroy (out_source);
  g_source_unref (out_source);
  g_source_destroy (in_source);
  g_source_unref (in_source);
  close (fds[1]);
  close (fds[0]);
}

typedef struct
{
  GSource parent;
  gboolean flagged;
} FlagSource;

static gboolean
return_true (GSource *source, GSourceFunc callback, gpointer user_data)
{
  FlagSource *flag_source = (FlagSource *) source;

  flag_source->flagged = TRUE;

  return TRUE;
}

#define assert_flagged(s) g_assert_true (((FlagSource *) (s))->flagged);
#define assert_not_flagged(s) g_assert_true (!((FlagSource *) (s))->flagged);
#define clear_flag(s) ((FlagSource *) (s))->flagged = 0

static void
test_source_unix_fd_api (void)
{
  GSourceFuncs no_funcs = {
    NULL, NULL, return_true, NULL, NULL, NULL
  };
  GSource *source_a;
  GSource *source_b;
  gpointer tag1, tag2;
  gint fds_a[2];
  gint fds_b[2];

  g_assert_cmpint (pipe (fds_a), ==, 0);
  g_assert_cmpint (pipe (fds_b), ==, 0);

  source_a = g_source_new (&no_funcs, sizeof (FlagSource));
  source_b = g_source_new (&no_funcs, sizeof (FlagSource));

  /* attach a source with more than one fd */
  g_source_add_unix_fd (source_a, fds_a[0], G_IO_IN);
  g_source_add_unix_fd (source_a, fds_a[1], G_IO_OUT);
  g_source_attach (source_a, NULL);
  assert_main_context_state (2,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, 0);
  assert_not_flagged (source_a);

  /* attach a higher priority source with no fds */
  g_source_set_priority (source_b, G_PRIORITY_HIGH);
  g_source_attach (source_b, NULL);
  assert_main_context_state (2,
                             fds_a[0], G_IO_IN, G_IO_IN,
                             fds_a[1], G_IO_OUT, 0);
  assert_flagged (source_a);
  assert_not_flagged (source_b);
  clear_flag (source_a);

  /* add some fds to the second source, while attached */
  tag1 = g_source_add_unix_fd (source_b, fds_b[0], G_IO_IN);
  tag2 = g_source_add_unix_fd (source_b, fds_b[1], G_IO_OUT);
  assert_main_context_state (4,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, G_IO_OUT,
                             fds_b[0], G_IO_IN, 0,
                             fds_b[1], G_IO_OUT, G_IO_OUT);
  /* only 'b' (higher priority) should have dispatched */
  assert_not_flagged (source_a);
  assert_flagged (source_b);
  clear_flag (source_b);

  /* change our events on b to the same as they were before */
  g_source_modify_unix_fd (source_b, tag1, G_IO_IN);
  g_source_modify_unix_fd (source_b, tag2, G_IO_OUT);
  assert_main_context_state (4,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, G_IO_OUT,
                             fds_b[0], G_IO_IN, 0,
                             fds_b[1], G_IO_OUT, G_IO_OUT);
  assert_not_flagged (source_a);
  assert_flagged (source_b);
  clear_flag (source_b);

  /* now reverse them */
  g_source_modify_unix_fd (source_b, tag1, G_IO_OUT);
  g_source_modify_unix_fd (source_b, tag2, G_IO_IN);
  assert_main_context_state (4,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, G_IO_OUT,
                             fds_b[0], G_IO_OUT, 0,
                             fds_b[1], G_IO_IN, 0);
  /* 'b' had no events, so 'a' can go this time */
  assert_flagged (source_a);
  assert_not_flagged (source_b);
  clear_flag (source_a);

  /* remove one of the fds from 'b' */
  g_source_remove_unix_fd (source_b, tag1);
  assert_main_context_state (3,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, 0,
                             fds_b[1], G_IO_IN, 0);
  assert_not_flagged (source_a);
  assert_not_flagged (source_b);

  /* remove the other */
  g_source_remove_unix_fd (source_b, tag2);
  assert_main_context_state (2,
                             fds_a[0], G_IO_IN, 0,
                             fds_a[1], G_IO_OUT, 0);
  assert_not_flagged (source_a);
  assert_not_flagged (source_b);

  /* destroy the sources */
  g_source_destroy (source_a);
  g_source_destroy (source_b);
  assert_main_context_state (0);

  g_source_unref (source_a);
  g_source_unref (source_b);
  close (fds_a[0]);
  close (fds_a[1]);
  close (fds_b[0]);
  close (fds_b[1]);
}

static gboolean
unixfd_quit_loop (gint         fd,
                  GIOCondition condition,
                  gpointer     user_data)
{
  GMainLoop *loop = user_data;

  g_main_loop_quit (loop);

  return FALSE;
}

static void
test_unix_file_poll (void)
{
  gint fd;
  GSource *source;
  GMainLoop *loop;

  fd = open ("/dev/null", O_RDONLY);
  g_assert_cmpint (fd, >=, 0);

  loop = g_main_loop_new (NULL, FALSE);

  source = g_unix_fd_source_new (fd, G_IO_IN);
  g_source_set_callback (source, (GSourceFunc) unixfd_quit_loop, loop, NULL);
  g_source_attach (source, NULL);

  /* Should not block */
  g_main_loop_run (loop);

  g_source_destroy (source);

  assert_main_context_state (0);

  g_source_unref (source);

  g_main_loop_unref (loop);

  close (fd);
}

static void
test_unix_fd_priority (void)
{
  gint fd1, fd2;
  GMainLoop *loop;
  GSource *source;

  gint s1 = 0;
  gboolean s2 = FALSE, s3 = FALSE;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/1592");

  loop = g_main_loop_new (NULL, FALSE);

  source = g_idle_source_new ();
  g_source_set_callback (source, count_calls, &s1, NULL);
  g_source_set_priority (source, 0);
  g_source_attach (source, NULL);
  g_source_unref (source);

  fd1 = open ("/dev/random", O_RDONLY);
  g_assert_cmpint (fd1, >=, 0);
  source = g_unix_fd_source_new (fd1, G_IO_IN);
  g_source_set_callback (source, G_SOURCE_FUNC (flag_bool), &s2, NULL);
  g_source_set_priority (source, 10);
  g_source_attach (source, NULL);
  g_source_unref (source);

  fd2 = open ("/dev/random", O_RDONLY);
  g_assert_cmpint (fd2, >=, 0);
  source = g_unix_fd_source_new (fd2, G_IO_IN);
  g_source_set_callback (source, G_SOURCE_FUNC (flag_bool), &s3, NULL);
  g_source_set_priority (source, 0);
  g_source_attach (source, NULL);
  g_source_unref (source);

  /* This tests a bug that depends on the source with the lowest FD
     identifier to have the lowest priority. Make sure that this is
     the case. */
  g_assert_cmpint (fd1, <, fd2);

  g_assert_true (g_main_context_iteration (NULL, FALSE));

  /* Idle source should have been dispatched. */
  g_assert_cmpint (s1, ==, 1);
  /* Low priority FD source shouldn't have been dispatched. */
  g_assert_false (s2);
  /* Default priority FD source should have been dispatched. */
  g_assert_true (s3);

  g_main_loop_unref (loop);

  close (fd1);
  close (fd2);
}

#endif

#ifdef G_OS_UNIX
static gboolean
timeout_cb (gpointer data)
{
  GMainLoop *loop = data;
  GMainContext *context;

  context = g_main_loop_get_context (loop);
  g_assert_true (g_main_loop_is_running (loop));
  g_assert_true (g_main_context_is_owner (context));

  g_main_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static gpointer
threadf (gpointer data)
{
  GMainContext *context = data;
  GMainLoop *loop;
  GSource *source;

  loop = g_main_loop_new (context, FALSE);
  source = g_timeout_source_new (250);
  g_source_set_callback (source, timeout_cb, loop, NULL);
  g_source_attach (source, context);
 
  g_main_loop_run (loop);

  g_source_destroy (source);
  g_source_unref (source);
  g_main_loop_unref (loop);

  return NULL;
}

static void
test_mainloop_wait (void)
{
  GMainContext *context;
  GThread *t1, *t2;

  context = g_main_context_new ();

  t1 = g_thread_new ("t1", threadf, context);
  t2 = g_thread_new ("t2", threadf, context);

  g_thread_join (t1);
  g_thread_join (t2);

  g_main_context_unref (context);
}
#endif

static gboolean
nfds_in_cb (GIOChannel   *io,
            GIOCondition  condition,
            gpointer      user_data)
{
  gboolean *in_cb_ran = user_data;

  *in_cb_ran = TRUE;
  g_assert_cmpint (condition, ==, G_IO_IN);
  return FALSE;
}

static gboolean
nfds_out_cb (GIOChannel   *io,
             GIOCondition  condition,
             gpointer      user_data)
{
  gboolean *out_cb_ran = user_data;

  *out_cb_ran = TRUE;
  g_assert_cmpint (condition, ==, G_IO_OUT);
  return FALSE;
}

static gboolean
nfds_out_low_cb (GIOChannel   *io,
                 GIOCondition  condition,
                 gpointer      user_data)
{
  g_assert_not_reached ();
  return FALSE;
}

static void
test_nfds (void)
{
  GMainContext *ctx;
  GPollFD out_fds[3];
  gint fd, nfds;
  GIOChannel *io;
  GSource *source1, *source2, *source3;
  gboolean source1_ran = FALSE, source3_ran = FALSE;
  gchar *tmpfile;
  GError *error = NULL;

  ctx = g_main_context_new ();
  nfds = g_main_context_query (ctx, G_MAXINT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  /* An "empty" GMainContext will have a single GPollFD, for its
   * internal GWakeup.
   */
  g_assert_cmpint (nfds, ==, 1);

  fd = g_file_open_tmp (NULL, &tmpfile, &error);
  g_assert_no_error (error);

  io = g_io_channel_unix_new (fd);
#ifdef G_OS_WIN32
  /* The fd in the pollfds won't be the same fd we passed in */
  g_io_channel_win32_make_pollfd (io, G_IO_IN, out_fds);
  fd = out_fds[0].fd;
#endif

  /* Add our first pollfd */
  source1 = g_io_create_watch (io, G_IO_IN);
  g_source_set_priority (source1, G_PRIORITY_DEFAULT);
  g_source_set_callback (source1, (GSourceFunc) nfds_in_cb,
                         &source1_ran, NULL);
  g_source_attach (source1, ctx);

  nfds = g_main_context_query (ctx, G_MAXINT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  g_assert_cmpint (nfds, ==, 2);
  if (out_fds[0].fd == fd)
    g_assert_cmpint (out_fds[0].events, ==, G_IO_IN);
  else if (out_fds[1].fd == fd)
    g_assert_cmpint (out_fds[1].events, ==, G_IO_IN);
  else
    g_assert_not_reached ();

  /* Add a second pollfd with the same fd but different event, and
   * lower priority.
   */
  source2 = g_io_create_watch (io, G_IO_OUT);
  g_source_set_priority (source2, G_PRIORITY_LOW);
  g_source_set_callback (source2, (GSourceFunc) nfds_out_low_cb,
                         NULL, NULL);
  g_source_attach (source2, ctx);

  /* g_main_context_query() should still return only 2 pollfds,
   * one of which has our fd, and a combined events field.
   */
  nfds = g_main_context_query (ctx, G_MAXINT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  g_assert_cmpint (nfds, ==, 2);
  if (out_fds[0].fd == fd)
    g_assert_cmpint (out_fds[0].events, ==, G_IO_IN | G_IO_OUT);
  else if (out_fds[1].fd == fd)
    g_assert_cmpint (out_fds[1].events, ==, G_IO_IN | G_IO_OUT);
  else
    g_assert_not_reached ();

  /* But if we query with a max priority, we won't see the
   * lower-priority one.
   */
  nfds = g_main_context_query (ctx, G_PRIORITY_DEFAULT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  g_assert_cmpint (nfds, ==, 2);
  if (out_fds[0].fd == fd)
    g_assert_cmpint (out_fds[0].events, ==, G_IO_IN);
  else if (out_fds[1].fd == fd)
    g_assert_cmpint (out_fds[1].events, ==, G_IO_IN);
  else
    g_assert_not_reached ();

  /* Third pollfd */
  source3 = g_io_create_watch (io, G_IO_OUT);
  g_source_set_priority (source3, G_PRIORITY_DEFAULT);
  g_source_set_callback (source3, (GSourceFunc) nfds_out_cb,
                         &source3_ran, NULL);
  g_source_attach (source3, ctx);

  nfds = g_main_context_query (ctx, G_MAXINT, NULL,
                               out_fds, G_N_ELEMENTS (out_fds));
  g_assert_cmpint (nfds, ==, 2);
  if (out_fds[0].fd == fd)
    g_assert_cmpint (out_fds[0].events, ==, G_IO_IN | G_IO_OUT);
  else if (out_fds[1].fd == fd)
    g_assert_cmpint (out_fds[1].events, ==, G_IO_IN | G_IO_OUT);
  else
    g_assert_not_reached ();

  /* Now actually iterate the loop; the fd should be readable and
   * writable, so source1 and source3 should be triggered, but *not*
   * source2, since it's lower priority than them.
   */
  g_main_context_iteration (ctx, FALSE);

  /* FIXME:
   * On win32, giowin32.c uses blocking threads for read/write on channels. They
   * may not have yet triggered an event after one loop iteration. Hence, the
   * following asserts are racy and disabled.
   */
#ifndef G_OS_WIN32
  g_assert_true (source1_ran);
  g_assert_true (source3_ran);
#endif

  g_source_destroy (source1);
  g_source_unref (source1);
  g_source_destroy (source2);
  g_source_unref (source2);
  g_source_destroy (source3);
  g_source_unref (source3);

  g_io_channel_unref (io);
  remove (tmpfile);
  g_free (tmpfile);

  g_main_context_unref (ctx);
}

static gboolean
nsources_cb (gpointer user_data)
{
  g_assert_not_reached ();
  return FALSE;
}

static void
shuffle_nsources (GSource **sources, int num)
{
  int i, a, b;
  GSource *tmp;

  for (i = 0; i < num * 10; i++)
    {
      a = g_random_int_range (0, num);
      b = g_random_int_range (0, num);
      tmp = sources[a];
      sources[a] = sources[b];
      sources[b] = tmp;
    }
}

static void
test_nsources_same_priority (void)
{
  GMainContext *context;
  GSource **sources;
  gint64 start, end;
  gsize n_sources = 50000, i;

  context = g_main_context_default ();
  sources = g_new0 (GSource *, n_sources);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      sources[i] = g_idle_source_new ();
      g_source_set_callback (sources[i], nsources_cb, NULL, NULL);
      g_source_attach (sources[i], context);
    }
  end = g_get_monotonic_time ();
  g_test_message ("Add same-priority sources: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    g_assert_true (sources[i] == g_main_context_find_source_by_id (context, g_source_get_id (sources[i])));
  end = g_get_monotonic_time ();
  g_test_message ("Find each source: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  shuffle_nsources (sources, n_sources);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      g_source_destroy (sources[i]);
      g_source_unref (sources[i]);
    }
  end = g_get_monotonic_time ();
  g_test_message ("Remove in random order: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  /* Make sure they really did get removed */
  g_main_context_iteration (context, FALSE);

  g_free (sources);
}

static void
test_nsources_different_priority (void)
{
  GMainContext *context;
  GSource **sources;
  gint64 start, end;
  gsize n_sources = 50000, i;

  context = g_main_context_default ();
  sources = g_new0 (GSource *, n_sources);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      sources[i] = g_idle_source_new ();
      g_source_set_callback (sources[i], nsources_cb, NULL, NULL);
      g_source_set_priority (sources[i], i % 100);
      g_source_attach (sources[i], context);
    }
  end = g_get_monotonic_time ();
  g_test_message ("Add different-priority sources: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    g_assert_true (sources[i] == g_main_context_find_source_by_id (context, g_source_get_id (sources[i])));
  end = g_get_monotonic_time ();
  g_test_message ("Find each source: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  shuffle_nsources (sources, n_sources);

  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      g_source_destroy (sources[i]);
      g_source_unref (sources[i]);
    }
  end = g_get_monotonic_time ();
  g_test_message ("Remove in random order: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  /* Make sure they really did get removed */
  g_main_context_iteration (context, FALSE);

  g_free (sources);
}

static void
thread_pool_attach_func (gpointer data,
                         gpointer user_data)
{
  GMainContext *context = user_data;
  GSource *source = data;

  g_source_attach (source, context);
  g_source_unref (source);
}

static void
thread_pool_destroy_func (gpointer data,
                          gpointer user_data)
{
  GSource *source = data;

  g_source_destroy (source);
}

static void
test_nsources_threadpool (void)
{
  GMainContext *context;
  GSource **sources;
  GThreadPool *pool;
  GError *error = NULL;
  gint64 start, end;
  gsize n_sources = 50000, i;

  context = g_main_context_default ();
  sources = g_new0 (GSource *, n_sources);

  pool = g_thread_pool_new (thread_pool_attach_func, context,
                            20, TRUE, NULL);
  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      sources[i] = g_idle_source_new ();
      g_source_set_callback (sources[i], nsources_cb, NULL, NULL);
      g_thread_pool_push (pool, sources[i], &error);
      g_assert_no_error (error);
    }
  g_thread_pool_free (pool, FALSE, TRUE);
  end = g_get_monotonic_time ();
  g_test_message ("Add sources from threads: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  pool = g_thread_pool_new (thread_pool_destroy_func, context,
                            20, TRUE, NULL);
  start = g_get_monotonic_time ();
  for (i = 0; i < n_sources; i++)
    {
      g_thread_pool_push (pool, sources[i], &error);
      g_assert_no_error (error);
    }
  g_thread_pool_free (pool, FALSE, TRUE);
  end = g_get_monotonic_time ();
  g_test_message ("Remove sources from threads: %" G_GINT64_FORMAT,
                  (end - start) / 1000);

  /* Make sure they really did get removed */
  g_main_context_iteration (context, FALSE);

  g_free (sources);
}

static gboolean source_finalize_called = FALSE;
static guint source_dispose_called = 0;
static gboolean source_dispose_recycle = FALSE;

static void
finalize (GSource *source)
{
  g_assert_false (source_finalize_called);
  source_finalize_called = TRUE;
}

static void
dispose (GSource *source)
{
  /* Dispose must always be called before finalize */
  g_assert_false (source_finalize_called);

  if (source_dispose_recycle)
    g_source_ref (source);
  source_dispose_called++;
}

static GSourceFuncs source_funcs = {
  prepare,
  check,
  dispatch,
  finalize,
  NULL,
  NULL
};

static void
test_maincontext_source_finalization (void)
{
  GSource *source;

  /* Check if GSource destruction without dispose function works and calls the
   * finalize function as expected */
  source_finalize_called = FALSE;
  source_dispose_called = 0;
  source_dispose_recycle = FALSE;
  source = g_source_new (&source_funcs, sizeof (GSource));
  g_source_unref (source);
  g_assert_cmpint (source_dispose_called, ==, 0);
  g_assert_true (source_finalize_called);

  /* Check if GSource destruction with dispose function works and calls the
   * dispose and finalize function as expected */
  source_finalize_called = FALSE;
  source_dispose_called = 0;
  source_dispose_recycle = FALSE;
  source = g_source_new (&source_funcs, sizeof (GSource));
  g_source_set_dispose_function (source, dispose);
  g_source_unref (source);
  g_assert_cmpint (source_dispose_called, ==, 1);
  g_assert_true (source_finalize_called);

  /* Check if GSource destruction with dispose function works and recycling
   * the source from dispose works without calling the finalize function */
  source_finalize_called = FALSE;
  source_dispose_called = 0;
  source_dispose_recycle = TRUE;
  source = g_source_new (&source_funcs, sizeof (GSource));
  g_source_set_dispose_function (source, dispose);
  g_source_unref (source);
  g_assert_cmpint (source_dispose_called, ==, 1);
  g_assert_false (source_finalize_called);

  /* Check if the source is properly recycled */
  g_assert_cmpint (source->ref_count, ==, 1);

  /* And then get rid of it properly */
  source_dispose_recycle = FALSE;
  g_source_unref (source);
  g_assert_cmpint (source_dispose_called, ==, 2);
  g_assert_true (source_finalize_called);
}

/* GSource implementation which optionally keeps a strong reference to another
 * GSource until finalization, when it destroys and unrefs the other source.
 */
typedef struct {
  GSource source;

  GSource *other_source;
} SourceWithSource;

static void
finalize_source_with_source (GSource *source)
{
  SourceWithSource *s = (SourceWithSource *) source;

  if (s->other_source)
    {
      g_source_destroy (s->other_source);
      g_source_unref (s->other_source);
      s->other_source = NULL;
    }
}

static GSourceFuncs source_with_source_funcs = {
  NULL,
  NULL,
  NULL,
  finalize_source_with_source,
  NULL,
  NULL
};

static void
test_maincontext_source_finalization_from_source (gconstpointer user_data)
{
  GMainContext *c = g_main_context_new ();
  GSource *s1, *s2;

  g_test_summary ("Tests if freeing a GSource as part of another GSource "
                  "during main context destruction works.");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/merge_requests/1353");

  s1 = g_source_new (&source_with_source_funcs, sizeof (SourceWithSource));
  s2 = g_source_new (&source_with_source_funcs, sizeof (SourceWithSource));
  ((SourceWithSource *)s1)->other_source = g_source_ref (s2);

  /* Attach sources in a different order so they end up being destroyed
   * in a different order by the main context */
  if (GPOINTER_TO_INT (user_data) < 5)
    {
      g_source_attach (s1, c);
      g_source_attach (s2, c);
    }
  else
    {
      g_source_attach (s2, c);
      g_source_attach (s1, c);
    }

  /* Test a few different permutations here */
  if (GPOINTER_TO_INT (user_data) % 5 == 0)
    {
      /* Get rid of our references so that destroying the context
       * will unref them immediately */
      g_source_unref (s1);
      g_source_unref (s2);
      g_main_context_unref (c);
    }
  else if (GPOINTER_TO_INT (user_data) % 5 == 1)
    {
      /* Destroy and free the sources before the context */
      g_source_destroy (s1);
      g_source_unref (s1);
      g_source_destroy (s2);
      g_source_unref (s2);
      g_main_context_unref (c);
    }
  else if (GPOINTER_TO_INT (user_data) % 5 == 2)
    {
      /* Destroy and free the sources before the context */
      g_source_destroy (s2);
      g_source_unref (s2);
      g_source_destroy (s1);
      g_source_unref (s1);
      g_main_context_unref (c);
    }
  else if (GPOINTER_TO_INT (user_data) % 5 == 3)
    {
      /* Destroy and free the context before the sources */
      g_main_context_unref (c);
      g_source_unref (s2);
      g_source_unref (s1);
    }
  else if (GPOINTER_TO_INT (user_data) % 5 == 4)
    {
      /* Destroy and free the context before the sources */
      g_main_context_unref (c);
      g_source_unref (s1);
      g_source_unref (s2);
    }
}

static gboolean
dispatch_source_with_source (GSource *source, GSourceFunc callback, gpointer user_data)
{
  return G_SOURCE_REMOVE;
}

static GSourceFuncs source_with_source_funcs_dispatch = {
  NULL,
  NULL,
  dispatch_source_with_source,
  finalize_source_with_source,
  NULL,
  NULL
};

static void
test_maincontext_source_finalization_from_dispatch (gconstpointer user_data)
{
  GMainContext *c = g_main_context_new ();
  GSource *s1, *s2;

  g_test_summary ("Tests if freeing a GSource as part of another GSource "
                  "during main context iteration works.");

  s1 = g_source_new (&source_with_source_funcs_dispatch, sizeof (SourceWithSource));
  s2 = g_source_new (&source_with_source_funcs_dispatch, sizeof (SourceWithSource));
  ((SourceWithSource *)s1)->other_source = g_source_ref (s2);

  g_source_attach (s1, c);
  g_source_attach (s2, c);

  if (GPOINTER_TO_INT (user_data) == 0)
    {
      /* This finalizes s1 as part of the iteration, which then destroys and
       * frees s2 too */
      g_source_set_ready_time (s1, 0);
    }
  else if (GPOINTER_TO_INT (user_data) == 1)
    {
      /* This destroys s2 as part of the iteration but does not free it as
       * it's still referenced by s1 */
      g_source_set_ready_time (s2, 0);
    }
  else if (GPOINTER_TO_INT (user_data) == 2)
    {
      /* This destroys both s1 and s2 as part of the iteration */
      g_source_set_ready_time (s1, 0);
      g_source_set_ready_time (s2, 0);
    }

  /* Get rid of our references so only the main context has one now */
  g_source_unref (s1);
  g_source_unref (s2);

  /* Iterate as long as there are sources to dispatch */
  while (g_main_context_iteration (c, FALSE))
    {
      /* Do nothing here */
    }

  g_main_context_unref (c);
}

static void
callback_source_unref (gpointer cb_data)
{
  GSource *s = (GSource *) cb_data;

  g_source_destroy (s);
};

static GSourceCallbackFuncs callback_funcs = {
  NULL,
  callback_source_unref,
  NULL,
};

static void
test_context_ref_while_in_source_callbackfuncs_unref (void)
{
  GMainContext *c = g_main_context_new ();
  GSource *s;

  g_test_summary ("Tests if calling GSource API in GSourceCallbackFuncs.unref "
                  "does not deadlock attempting to retrieve the relevant GMainContext.");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/3725");

  s = g_source_new (&source_with_source_funcs, sizeof (SourceWithSource));
  g_source_set_callback_indirect (s, s, &callback_funcs);
  g_source_attach (s, c);
  g_source_unref (s);

  g_main_context_unref (c);
}

static void
once_cb (gpointer user_data)
{
  guint *counter = user_data;

  *counter = *counter + 1;
}

static void
test_maincontext_idle_once (void)
{
  guint counter = 0;
  guint source_id;
  GSource *source;

  g_test_summary ("Test g_idle_add_once() works");

  source_id = g_idle_add_once (once_cb, &counter);
  source = g_main_context_find_source_by_id (NULL, source_id);
  g_assert_nonnull (source);
  g_source_ref (source);

  /* Iterating the main context should dispatch the source. */
  g_assert_cmpuint (counter, ==, 0);
  g_main_context_iteration (NULL, FALSE);
  g_assert_cmpuint (counter, ==, 1);

  /* Iterating it again should not dispatch the source again. */
  g_main_context_iteration (NULL, FALSE);
  g_assert_cmpuint (counter, ==, 1);
  g_assert_true (g_source_is_destroyed (source));

  g_clear_pointer (&source, g_source_unref);
}

static void
test_maincontext_timeout_once (void)
{
  guint counter = 0, check_counter = 0;
  guint source_id;
  gint64 t;
  GSource *source;

  g_test_summary ("Test g_timeout_add_once() works");

  source_id = g_timeout_add_once (10 /* ms */, once_cb, &counter);
  source = g_main_context_find_source_by_id (NULL, source_id);
  g_assert_nonnull (source);
  g_source_ref (source);

  /* Iterating the main context should dispatch the source, though we have to block. */
  g_assert_cmpuint (counter, ==, 0);
  t = g_get_monotonic_time ();
  while (g_get_monotonic_time () - t < 50 * 1000 && counter == 0)
    g_main_context_iteration (NULL, TRUE);
  g_assert_cmpuint (counter, ==, 1);

  /* Iterating it again should not dispatch the source again. We add a second
   * timeout and block until that is dispatched. Given the ordering guarantees,
   * we should then know whether the first one would have re-dispatched by then. */
  g_timeout_add_once (30 /* ms */, once_cb, &check_counter);
  t = g_get_monotonic_time ();
  while (g_get_monotonic_time () - t < 50 * 1000 && check_counter == 0)
    g_main_context_iteration (NULL, TRUE);
  g_assert_cmpuint (check_counter, ==, 1);
  g_assert_cmpuint (counter, ==, 1);
  g_assert_true (g_source_is_destroyed (source));

  g_clear_pointer (&source, g_source_unref);
}

static void
test_steal_fd (void)
{
  GError *error = NULL;
  gchar *tmpfile = NULL;
  int fd = -42;
  int borrowed;
  int stolen;

  g_assert_cmpint (g_steal_fd (&fd), ==, -42);
  g_assert_cmpint (fd, ==, -1);
  g_assert_cmpint (g_steal_fd (&fd), ==, -1);
  g_assert_cmpint (fd, ==, -1);

  fd = g_file_open_tmp (NULL, &tmpfile, &error);
  g_assert_cmpint (fd, >=, 0);
  g_assert_no_error (error);
  borrowed = fd;
  stolen = g_steal_fd (&fd);
  g_assert_cmpint (fd, ==, -1);
  g_assert_cmpint (borrowed, ==, stolen);

  g_close (g_steal_fd (&stolen), &error);
  g_assert_no_error (error);
  g_assert_cmpint (stolen, ==, -1);

  g_assert_no_errno (remove (tmpfile));
  g_free (tmpfile);
}

typedef enum
{
  INITIAL = 0,
  MAIN_CONTEXT_READY = (1 << 0),
  SOURCE_READY = (1 << 1),
} G_GNUC_FLAG_ENUM SimultaneousDestructionTestState;

typedef struct
{
  SimultaneousDestructionTestState state; /* (mutex lock) */
  GMainContext *main_context; /* (mutex lock) */
  GSource *source; /* (mutex lock) */
  GMutex lock;
  GCond cond;
  GThread *main_context_thread;
  GThread *source_thread; /* (mutex lock) */
} SimultaneousDestructionTest;

static SimultaneousDestructionTest *
create_simultaneous_destruction_test (void)
{
  SimultaneousDestructionTest *test;

  test = g_new0 (SimultaneousDestructionTest, 1);

  g_mutex_init (&test->lock);
  g_cond_init (&test->cond);

  return test;
}

static void
free_simultaneous_destruction_test (SimultaneousDestructionTest * test)
{
  g_mutex_clear (&test->lock);
  g_cond_clear (&test->cond);

  /* Should have already been cleared in wait_simultaneous_destruction_test() */
  g_assert (test->main_context == NULL);
  g_assert (test->source == NULL);
  g_assert (test->main_context_thread == NULL);
  g_assert (test->source_thread == NULL);

  g_free (test);
}

static gpointer
source_create_unref_thread_func (gpointer data)
{
  SimultaneousDestructionTest *test = data;
  GMainContext *main_context;

  g_mutex_lock (&test->lock);
  test->source = g_timeout_source_new_seconds (100);
  main_context = g_main_context_ref (test->main_context);
  g_source_attach (test->source, main_context);
  test->state |= SOURCE_READY;
  g_cond_broadcast (&test->cond);
  while ((test->state & MAIN_CONTEXT_READY) == 0)
    g_cond_wait (&test->cond, &test->lock);
  g_mutex_unlock (&test->lock);

  g_thread_yield ();
  g_source_destroy (test->source);

  g_mutex_lock (&test->lock);
  g_source_unref (test->source);
  test->source = NULL;
  g_cond_broadcast (&test->cond);
  g_mutex_unlock (&test->lock);
  g_main_context_unref (main_context);

  return NULL;
}

static gpointer
main_context_create_unref_thread_func (gpointer data)
{
  SimultaneousDestructionTest *test = data;

  g_mutex_lock (&test->lock);
  test->main_context = g_main_context_new ();
  test->source_thread = g_thread_new (NULL, source_create_unref_thread_func, test);

  test->state |= MAIN_CONTEXT_READY;
  while ((test->state & SOURCE_READY) == 0)
    g_cond_wait (&test->cond, &test->lock);
  g_mutex_unlock (&test->lock);

  g_thread_yield ();
  g_main_context_unref (test->main_context);

  g_mutex_lock (&test->lock);
  test->main_context = NULL;
  g_cond_broadcast (&test->cond);
  g_mutex_unlock (&test->lock);

  return NULL;
}

static void
start_simultaneous_destruction_test (SimultaneousDestructionTest * test)
{
  test->main_context_thread = g_thread_new (NULL, main_context_create_unref_thread_func, test);

  g_mutex_lock (&test->lock);
  while (test->main_context)
    g_cond_wait (&test->cond, &test->lock);
  g_mutex_unlock (&test->lock);
}

static void
wait_simultaneous_destruction_test (SimultaneousDestructionTest * test)
{
  g_mutex_lock (&test->lock);
  while (test->main_context || test->source)
    g_cond_wait (&test->cond, &test->lock);
  g_mutex_unlock (&test->lock);

  g_thread_join (g_steal_pointer (&test->main_context_thread));
  g_thread_join (g_steal_pointer (&test->source_thread));
}

static void
test_simultaneous_source_context_destruction (void)
{
  guint64 n_concurrent = 120, n_iterations = 100;
  SimultaneousDestructionTest **test;
  guint64 i;

  /* The race in this test is very hard to reproduce under valgrind, so skip it.
   * Otherwise the test can run for tens of minutes. */
#if defined (ENABLE_VALGRIND)
  if (RUNNING_ON_VALGRIND && !g_test_thorough ())
    {
      g_test_skip ("Skipping hard-to-reproduce race under valgrind");
      return;
    }
#endif

  if (g_test_thorough ())
    {
      n_concurrent = 512;
      n_iterations = 2000;
    }

  test = g_new0 (SimultaneousDestructionTest *, n_concurrent);

  for (i = 0; i < n_iterations; i++)
    {
      gsize j = 0;
      for (j = 0; j < n_concurrent; j++)
        test[j] = create_simultaneous_destruction_test ();

      for (j = 0; j < n_concurrent; j++)
        start_simultaneous_destruction_test (test[j]);

      for (j = 0; j < n_concurrent; j++)
        {
          wait_simultaneous_destruction_test (test[j]);
          free_simultaneous_destruction_test (test[j]);
        }

      if (g_test_verbose() && i % 100 == 0)
        g_test_message ("# %" G_GUINT64_FORMAT " / %" G_GUINT64_FORMAT, i, n_iterations);
    }

  if (g_test_verbose ())
    g_test_message ("%" G_GUINT64_FORMAT " / %" G_GUINT64_FORMAT, n_iterations, n_iterations);

  g_free (test);
}

int
main (int argc, char *argv[])
{
  gint i;

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/maincontext/basic", test_maincontext_basic);
  g_test_add_func ("/maincontext/nsources_same_priority", test_nsources_same_priority);
  g_test_add_func ("/maincontext/nsources_different_priority", test_nsources_different_priority);
  g_test_add_func ("/maincontext/nsources_threadpool", test_nsources_threadpool);
  g_test_add_func ("/maincontext/source_finalization", test_maincontext_source_finalization);
  for (i = 0; i < 10; i++)
    {
      gchar *name = g_strdup_printf ("/maincontext/source_finalization_from_source/%d", i);
      g_test_add_data_func (name, GINT_TO_POINTER (i), test_maincontext_source_finalization_from_source);
      g_free (name);
    }
  for (i = 0; i < 3; i++)
    {
      gchar *name = g_strdup_printf ("/maincontext/source_finalization_from_dispatch/%d", i);
      g_test_add_data_func (name, GINT_TO_POINTER (i), test_maincontext_source_finalization_from_dispatch);
      g_free (name);
    }
  g_test_add_func ("/maincontext/idle-once", test_maincontext_idle_once);
  g_test_add_func ("/maincontext/timeout-once", test_maincontext_timeout_once);
  g_test_add_func ("/maincontext/context-ref-in-source-callbackfuncs-unref", test_context_ref_while_in_source_callbackfuncs_unref);

  g_test_add_func ("/mainloop/basic", test_mainloop_basic);
  g_test_add_func ("/mainloop/timeouts", test_timeouts);
  g_test_add_func ("/mainloop/timeouts_ns", test_timeouts_ns);
  g_test_add_func ("/mainloop/priorities", test_priorities);
  g_test_add_func ("/mainloop/prepare-timeout", test_prepare_timeout);
  g_test_add_func ("/mainloop/invoke", test_invoke);
  g_test_add_func ("/mainloop/child_sources", test_child_sources);
  g_test_add_func ("/mainloop/recursive_child_sources", test_recursive_child_sources);
  g_test_add_func ("/mainloop/recursive_loop_child_sources", test_recursive_loop_child_sources);
  g_test_add_func ("/mainloop/swapping_child_sources", test_swapping_child_sources);
  g_test_add_func ("/mainloop/blocked_child_sources", test_blocked_child_sources);
  g_test_add_func ("/mainloop/source_time", test_source_time);
  g_test_add_func ("/mainloop/overflow", test_mainloop_overflow);
  g_test_add_func ("/mainloop/ready-time", test_ready_time);
  g_test_add_func ("/mainloop/various-ready-times", test_various_ready_times);
  g_test_add_func ("/mainloop/wakeup", test_wakeup);
  g_test_add_func ("/mainloop/remove-invalid", test_remove_invalid);
  g_test_add_func ("/mainloop/unref-while-pending", test_unref_while_pending);
  g_test_add_func ("/mainloop/null-default-context", test_null_default_context);
#ifdef G_OS_UNIX
  g_test_add_func ("/mainloop/unix-fd", test_unix_fd);
  g_test_add_func ("/mainloop/unix-fd-source", test_unix_fd_source);
  g_test_add_func ("/mainloop/source-unix-fd-api", test_source_unix_fd_api);
  g_test_add_func ("/mainloop/wait", test_mainloop_wait);
  g_test_add_func ("/mainloop/unix-file-poll", test_unix_file_poll);
  g_test_add_func ("/mainloop/unix-fd-priority", test_unix_fd_priority);
#endif
  g_test_add_func ("/mainloop/nfds", test_nfds);
  g_test_add_func ("/mainloop/steal-fd", test_steal_fd);
  g_test_add_data_func ("/mainloop/ownerless-polling/attach-first", GINT_TO_POINTER (TRUE), test_ownerless_polling);
  g_test_add_data_func ("/mainloop/ownerless-polling/pop-first", GINT_TO_POINTER (FALSE), test_ownerless_polling);
  g_test_add_func ("/mainloop/simultaneous-source-context-destruction", test_simultaneous_source_context_destruction);

  return g_test_run ();
}
