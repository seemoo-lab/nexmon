#include "config.h"

#include <glib.h>

#define WAIT                5    /* seconds */
#define MAX_THREADS         10

/* if > 0 the test will run continuously (since the test ends when
 * thread count is 0), if -1 it means no limit to unused threads
 * if 0 then no unused threads are possible */
#define MAX_UNUSED_THREADS -1

G_LOCK_DEFINE_STATIC (thread_counter_pools);

static gulong abs_thread_counter = 0;
static gulong running_thread_counter = 0;
static gulong leftover_task_counter = 0;

static GThreadPool *idle_pool = NULL;

static void
test_threadpool_functions (void)
{
  gint max_unused_threads;
  guint max_idle_time;

  /* This function attempts to call functions which don't need a
   * threadpool to operate to make sure no uninitialised pointers
   * accessed and no errors occur.
   */

  max_unused_threads = 3;

  g_thread_pool_set_max_unused_threads (max_unused_threads);

  g_assert_cmpint (g_thread_pool_get_max_unused_threads (), ==,
                   max_unused_threads);

  g_assert_cmpint (g_thread_pool_get_num_unused_threads (), ==, 0);

  g_thread_pool_stop_unused_threads ();

  max_idle_time = 10 * G_USEC_PER_SEC;

  g_thread_pool_set_max_idle_time (max_idle_time);

  g_assert_cmpint (g_thread_pool_get_max_idle_time (), ==, max_idle_time);

  g_thread_pool_set_max_idle_time (0);

  g_assert_cmpint (g_thread_pool_get_max_idle_time (), ==, 0);
}

static void
test_threadpool_stop_unused (void)
{
  GThreadPool *pool;
  guint i;
  guint limit = 100;

  /* Spawn a few threads. */
  g_thread_pool_set_max_unused_threads (-1);
  pool = g_thread_pool_new ((GFunc) g_usleep, NULL, -1, FALSE, NULL);

  for (i = 0; i < limit; i++)
    g_thread_pool_push (pool, GUINT_TO_POINTER (1000), NULL);

  /* Wait for the threads to migrate. */
  while (g_thread_pool_get_num_threads (pool) != 0)
    g_usleep (100);

  g_assert_cmpuint (g_thread_pool_get_num_threads (pool), ==, 0);
  g_assert_cmpuint (g_thread_pool_get_num_unused_threads (), >, 0);

  /* Wait for threads to die. */
  g_thread_pool_stop_unused_threads ();

  while (g_thread_pool_get_num_unused_threads () != 0)
    g_usleep (100);

  g_assert_cmpuint (g_thread_pool_get_num_unused_threads (), ==, 0);

  g_thread_pool_set_max_unused_threads (MAX_THREADS);

  g_assert_cmpuint (g_thread_pool_get_num_threads (pool), ==, 0);
  g_assert_cmpuint (g_thread_pool_get_num_unused_threads (), ==, 0);

  g_thread_pool_free (pool, FALSE, TRUE);
}

static void
test_threadpool_stop_unused_multiple (void)
{
  GThreadPool *pools[10];
  guint i, j;
  const guint limit = 10;
  gboolean all_stopped;

  /* Spawn a few threads. */
  g_thread_pool_set_max_unused_threads (-1);

  for (i = 0; i < G_N_ELEMENTS (pools); i++)
    {
      pools[i] = g_thread_pool_new ((GFunc) g_usleep, NULL, -1, FALSE, NULL);

      for (j = 0; j < limit; j++)
        g_thread_pool_push (pools[i], GUINT_TO_POINTER (100), NULL);
    }

  all_stopped = FALSE;
  while (!all_stopped)
    {
      all_stopped = TRUE;
      for (i = 0; i < G_N_ELEMENTS (pools); i++)
        all_stopped &= (g_thread_pool_get_num_threads (pools[i]) == 0);
    }

  for (i = 0; i < G_N_ELEMENTS (pools); i++)
    {
      g_assert_cmpuint (g_thread_pool_get_num_threads (pools[i]), ==, 0);
      g_assert_cmpuint (g_thread_pool_get_num_unused_threads (), >, 0);
    }

  /* Wait for threads to die. */
  g_thread_pool_stop_unused_threads ();

  while (g_thread_pool_get_num_unused_threads () != 0)
    g_usleep (100);

  g_assert_cmpuint (g_thread_pool_get_num_unused_threads (), ==, 0);

  for (i = 0; i < G_N_ELEMENTS (pools); i++)
    g_thread_pool_free (pools[i], FALSE, TRUE);
}

static void
test_threadpool_pools_entry_func (gpointer data, gpointer user_data)
{
  G_LOCK (thread_counter_pools);
  abs_thread_counter++;
  running_thread_counter++;
  G_UNLOCK (thread_counter_pools);

  g_usleep (g_random_int_range (0, 4000));

  G_LOCK (thread_counter_pools);
  running_thread_counter--;
  leftover_task_counter--;

  G_UNLOCK (thread_counter_pools);
}

static void
test_threadpool_pools (void)
{
  GThreadPool *pool1, *pool2, *pool3;
  guint runs;
  guint i;

  pool1 = g_thread_pool_new ((GFunc) test_threadpool_pools_entry_func, NULL, 3, FALSE, NULL);
  pool2 = g_thread_pool_new ((GFunc) test_threadpool_pools_entry_func, NULL, 5, TRUE, NULL);
  pool3 = g_thread_pool_new ((GFunc) test_threadpool_pools_entry_func, NULL, 7, TRUE, NULL);

  runs = 300;
  for (i = 0; i < runs; i++)
    {
      g_thread_pool_push (pool1, GUINT_TO_POINTER (i + 1), NULL);
      g_thread_pool_push (pool2, GUINT_TO_POINTER (i + 1), NULL);
      g_thread_pool_push (pool3, GUINT_TO_POINTER (i + 1), NULL);

      G_LOCK (thread_counter_pools);
      leftover_task_counter += 3;
      G_UNLOCK (thread_counter_pools);
    }

  g_thread_pool_free (pool1, TRUE, TRUE);
  g_thread_pool_free (pool2, FALSE, TRUE);
  g_thread_pool_free (pool3, FALSE, TRUE);

  g_assert_cmpint (runs * 3, ==, abs_thread_counter + leftover_task_counter);
  g_assert_cmpint (running_thread_counter, ==, 0);
}

static gint
test_threadpool_sort_compare_func (gconstpointer a, gconstpointer b, gpointer user_data)
{
  guint32 id1, id2;

  id1 = GPOINTER_TO_UINT (a);
  id2 = GPOINTER_TO_UINT (b);

  return (id1 > id2 ? +1 : id1 == id2 ? 0 : -1);
}

static void
test_threadpool_sort_entry_func (gpointer data, gpointer user_data)
{
  guint thread_id;
  gboolean is_sorted;
  static GMutex last_thread_mutex;
  static guint last_thread_id = 0;

  g_mutex_lock (&last_thread_mutex);

  thread_id = GPOINTER_TO_UINT (data);
  is_sorted = GPOINTER_TO_INT (user_data);

  if (is_sorted)
    {
      if (last_thread_id != 0)
        g_assert_cmpint (last_thread_id, <=, thread_id);

      last_thread_id = thread_id;
    }

  g_mutex_unlock (&last_thread_mutex);

  g_usleep (WAIT * 1000);
}

static void
test_threadpool_sort (gconstpointer data)
{
  gboolean sort = GPOINTER_TO_UINT (data);
  GThreadPool *pool;
  guint limit;
  guint max_threads;
  guint i;
  GError *local_error = NULL;

  limit = MAX_THREADS * 10;

  if (sort) {
    max_threads = 1;
  } else {
    max_threads = MAX_THREADS;
  }

  /* It is important that we only have a maximum of 1 thread for this
   * test since the results can not be guaranteed to be sorted if > 1.
   *
   * Threads are scheduled by the operating system and are executed at
   * random. It cannot be assumed that threads are executed in the
   * order they are created. This was discussed in bug #334943.
   *
   * However, if testing sorting, we start with max-threads=0 so that all the
   * work can be enqueued before starting the pool. This prevents a race between
   * new work being enqueued out of sorted order, and work being pulled off the
   * queue.
   */

  pool = g_thread_pool_new (test_threadpool_sort_entry_func,
			    GINT_TO_POINTER (sort),
			    sort ? 0 : max_threads,
			    FALSE,
			    NULL);

  g_thread_pool_set_max_unused_threads (MAX_UNUSED_THREADS);

  if (sort) {
    g_thread_pool_set_sort_function (pool,
				     test_threadpool_sort_compare_func,
				     NULL);
  }

  for (i = 0; i < limit; i++) {
    guint id;

    id = g_random_int_range (1, limit) + 1;
    g_thread_pool_push (pool, GUINT_TO_POINTER (id), NULL);
    g_test_message ("%s ===> pushed new thread with id:%d, number "
                    "of threads:%d, unprocessed:%d",
                    sort ? "[  sorted]" : "[unsorted]",
                    id,
                    g_thread_pool_get_num_threads (pool),
                    g_thread_pool_unprocessed (pool));
  }

  if (sort)
    {
      g_test_message ("Starting thread pool processing");
      g_thread_pool_set_max_threads (pool, max_threads, &local_error);
      g_assert_no_error (local_error);
    }

  g_assert_cmpint (g_thread_pool_get_max_threads (pool), ==, (gint) max_threads);
  g_assert_cmpuint (g_thread_pool_get_num_threads (pool), <=,
                    (guint) g_thread_pool_get_max_threads (pool));
  g_thread_pool_free (pool, TRUE, TRUE);
}

static void
test_threadpool_idle_time_entry_func (gpointer data, gpointer user_data)
{
  g_usleep (WAIT * 1000);
}

static gboolean
test_threadpool_idle_timeout (gpointer data)
{
  gboolean *idle_timeout_called = data;
  gint i;

  *idle_timeout_called = TRUE;

  for (i = 0; i < 2; i++) {
    g_thread_pool_push (idle_pool, GUINT_TO_POINTER (100 + i), NULL);
  }

  g_main_context_wakeup (NULL);

  return G_SOURCE_REMOVE;
}

static gboolean
poll_cb (gpointer data)
{
  g_main_context_wakeup (NULL);
  return G_SOURCE_CONTINUE;
}

static void
test_threadpool_idle_time (void)
{
  guint limit = 50;
  guint interval = 10000;
  guint i;
  guint idle;
  gboolean idle_timeout_called = FALSE;
  GSource *timeout_source = NULL;
  GSource *poll_source = NULL;

  idle_pool = g_thread_pool_new (test_threadpool_idle_time_entry_func,
				 NULL,
				 0,
				 FALSE,
				 NULL);

  g_thread_pool_set_max_threads (idle_pool, MAX_THREADS, NULL);
  g_thread_pool_set_max_unused_threads (MAX_UNUSED_THREADS);
  g_thread_pool_set_max_idle_time (interval);

  g_assert_cmpint (g_thread_pool_get_max_threads (idle_pool), ==,
                   MAX_THREADS);
  g_assert_cmpint (g_thread_pool_get_max_unused_threads (), ==,
                   MAX_UNUSED_THREADS);
  g_assert_cmpint (g_thread_pool_get_max_idle_time (), ==, interval);

  for (i = 0; i < limit; i++) {
    g_thread_pool_push (idle_pool, GUINT_TO_POINTER (i + 1), NULL);
  }

  g_assert_cmpint (g_thread_pool_unprocessed (idle_pool), <=, limit);

  timeout_source = g_timeout_source_new (interval - 1000);
  g_source_set_callback (timeout_source, test_threadpool_idle_timeout, &idle_timeout_called, NULL);
  g_source_attach (timeout_source, NULL);

  /* Wait until the idle timeout has been called at least once and there are no
   * unused threads. We need a second timeout for this, to periodically wake
   * the main context up, as there’s no way to be notified of changes to `idle`.
   */
  poll_source = g_timeout_source_new (500);
  g_source_set_callback (poll_source, poll_cb, NULL, NULL);
  g_source_attach (poll_source, NULL);

  idle = g_thread_pool_get_num_unused_threads ();
  while (!idle_timeout_called || idle > 0)
    {
      g_test_message ("Pool idle thread count: %d, unprocessed jobs: %d",
                      idle, g_thread_pool_unprocessed (idle_pool));
      g_main_context_iteration (NULL, TRUE);
      idle = g_thread_pool_get_num_unused_threads ();
    }

  g_thread_pool_free (idle_pool, FALSE, TRUE);
  g_source_destroy (poll_source);
  g_source_unref (poll_source);
  g_source_destroy (timeout_source);
  g_source_unref (timeout_source);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/threadpool/functions", test_threadpool_functions);
  g_test_add_func ("/threadpool/stop-unused", test_threadpool_stop_unused);
  g_test_add_func ("/threadpool/pools", test_threadpool_pools);
  g_test_add_data_func ("/threadpool/no-sort", GUINT_TO_POINTER (FALSE), test_threadpool_sort);
  g_test_add_data_func ("/threadpool/sort", GUINT_TO_POINTER (TRUE), test_threadpool_sort);
  g_test_add_func ("/threadpool/stop-unused-multiple", test_threadpool_stop_unused_multiple);
  g_test_add_func ("/threadpool/idle-time", test_threadpool_idle_time);

  return g_test_run ();
}
