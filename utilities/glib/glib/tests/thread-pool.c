/* Unit tests for GThreadPool
 * Copyright (C) 2020 Sebastian Dröge <sebastian@centricular.com>
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

#include <config.h>

#include <glib.h>

#ifdef G_OS_UNIX
#include <sys/resource.h>
#include <unistd.h>
#endif

#ifdef HAVE_PRLIMIT
static gpointer
dummy_thread_func (gpointer data)
{
  return NULL;
}

static void
pool_fail_func (gpointer data,
                gpointer user_data)
{
}
#endif

static void
test_pool_fail (void)
{
#ifdef HAVE_PRLIMIT
  struct rlimit ol, nl;
  GThread *thread;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3712");

  getrlimit (RLIMIT_NPROC, &nl);
  nl.rlim_cur = 1;

  if (prlimit (getpid (), RLIMIT_NPROC, &nl, &ol) != 0)
    {
      int errsv = errno;
      g_error ("setting RLIMIT_NPROC to {cur=%ld,max=%ld} failed: %s",
               (long) nl.rlim_cur, (long) nl.rlim_max, g_strerror (errsv));
    }

  thread = g_thread_try_new ("a", dummy_thread_func, NULL, NULL);
  if (thread != NULL)
    {
      g_test_skip ("Cannot test failure to create a thread-pool "
                   "when we cannot block the creation of threads");
      g_thread_unref (thread);
    }
  else
    {
      GError *local_error = NULL;

      for (unsigned int i = 0; i < 2; i++)
        {
          GThreadPool *pool;

          /* The first time, i == 0, so the new pool is exclusive;
           * g_thread_pool_new() tries to populate the pool with the
           * single thread, and fails.
           * The second time the pool is not exclusive;
           * spawn_thread_queue has not been created,
           * so g_thread_pool_new() tries to create the pool_spawner, and fails.
           */
          pool = g_thread_pool_new (pool_fail_func, NULL, 1, (i == 0),
                                    &local_error);
          g_assert_null (pool);
          g_assert_error (local_error, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN);
          g_clear_error (&local_error);
        }
    }

  if (prlimit (getpid (), RLIMIT_NPROC, &ol, NULL) != 0)
    {
      int errsv = errno;
      g_error ("resetting RLIMIT_NPROC failed: %s", g_strerror (errsv));
    }
#else
  g_test_skip ("Cannot test failure to create a thread-pool without prlimit()");
#endif
}

typedef struct {
  GMutex mutex;
  GCond cond;
  gboolean signalled;
} MutexCond;

static void
pool_func (gpointer data, gpointer user_data)
{
  MutexCond *m = user_data;

  g_mutex_lock (&m->mutex);
  g_assert_false (m->signalled);
  g_assert_true (data == GUINT_TO_POINTER (123));
  m->signalled = TRUE;
  g_cond_signal (&m->cond);
  g_mutex_unlock (&m->mutex);
}

static void
test_simple (gconstpointer shared)
{
  GThreadPool *pool;
  GError *err = NULL;
  MutexCond m;
  gboolean success;

  g_mutex_init (&m.mutex);
  g_cond_init (&m.cond);

  if (GPOINTER_TO_INT (shared))
    {
      g_test_summary ("Tests that a shared, non-exclusive thread pool "
                      "generally works.");
      pool = g_thread_pool_new (pool_func, &m, -1, FALSE, &err);
    }
  else
    {
      g_test_summary ("Tests that an exclusive thread pool generally works.");
      pool = g_thread_pool_new (pool_func, &m, 2, TRUE, &err);
    }
  g_assert_no_error (err);
  g_assert_nonnull (pool);

  g_mutex_lock (&m.mutex);
  m.signalled = FALSE;

  success = g_thread_pool_push (pool, GUINT_TO_POINTER (123), &err);
  g_assert_no_error (err);
  g_assert_true (success);

  while (!m.signalled)
    g_cond_wait (&m.cond, &m.mutex);
  g_mutex_unlock (&m.mutex);

  g_thread_pool_free (pool, TRUE, TRUE);
}

static void
dummy_pool_func (gpointer data, gpointer user_data)
{
  g_assert_true (data == GUINT_TO_POINTER (123));
}

static void
test_create_first_pool (gconstpointer shared_first)
{
  GThreadPool *pool;
  GError *err = NULL;
  gboolean success;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/issues/2012");
  if (GPOINTER_TO_INT (shared_first))
    {
      g_test_summary ("Tests that creating an exclusive pool after a "
                      "shared one works.");
    }
  else
    {
      g_test_summary ("Tests that creating a shared pool after an "
                      "exclusive one works.");
    }

  if (!g_test_subprocess ())
    {
      g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
      return;
    }

  g_thread_pool_set_max_unused_threads (0);

  if (GPOINTER_TO_INT (shared_first))
    pool = g_thread_pool_new (dummy_pool_func, NULL, -1, FALSE, &err);
  else
    pool = g_thread_pool_new (dummy_pool_func, NULL, 2, TRUE, &err);
  g_assert_no_error (err);
  g_assert_nonnull (pool);

  success = g_thread_pool_push (pool, GUINT_TO_POINTER (123), &err);
  g_assert_no_error (err);
  g_assert_true (success);

  g_thread_pool_free (pool, TRUE, TRUE);

  if (GPOINTER_TO_INT (shared_first))
    pool = g_thread_pool_new (dummy_pool_func, NULL, 2, TRUE, &err);
  else
    pool = g_thread_pool_new (dummy_pool_func, NULL, -1, FALSE, &err);
  g_assert_no_error (err);
  g_assert_nonnull (pool);

  success = g_thread_pool_push (pool, GUINT_TO_POINTER (123), &err);
  g_assert_no_error (err);
  g_assert_true (success);

  g_thread_pool_free (pool, TRUE, TRUE);
}

typedef struct
{
  GMutex mutex;  /* (owned) */
  GCond cond;  /* (owned) */
  gboolean threads_should_block;  /* protected by mutex, cond */

  guint n_jobs_started;  /* (atomic) */
  guint n_jobs_completed;  /* (atomic) */
  guint n_free_func_calls;  /* (atomic) */
} TestThreadPoolFullData;

static void
full_thread_func (gpointer data,
                  gpointer user_data)
{
  TestThreadPoolFullData *test_data = data;

  g_atomic_int_inc (&test_data->n_jobs_started);

  /* Make the thread block until told to stop blocking. */
  g_mutex_lock (&test_data->mutex);
  while (test_data->threads_should_block)
    g_cond_wait (&test_data->cond, &test_data->mutex);
  g_mutex_unlock (&test_data->mutex);

  g_atomic_int_inc (&test_data->n_jobs_completed);
}

static void
free_func (gpointer user_data)
{
  TestThreadPoolFullData *test_data = user_data;

  g_atomic_int_inc (&test_data->n_free_func_calls);
}

static void
test_thread_pool_full (gconstpointer shared_first)
{
  guint i;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/121");

  g_thread_pool_set_max_unused_threads (0);

  /* Run the test twice, once with a shared pool and once with an exclusive one. */
  for (i = 0; i < 2; i++)
    {
      GThreadPool *pool;
      TestThreadPoolFullData test_data;
      GError *local_error = NULL;
      gboolean success;
      guint j;

      g_mutex_init (&test_data.mutex);
      g_cond_init (&test_data.cond);
      test_data.threads_should_block = TRUE;
      test_data.n_jobs_started = 0;
      test_data.n_jobs_completed = 0;
      test_data.n_free_func_calls = 0;

      /* Create a thread pool with only one worker thread. The pool can be
       * created in shared or exclusive mode. */
      pool = g_thread_pool_new_full (full_thread_func, &test_data, free_func,
                                     1, (i == 0),
                                     &local_error);
      g_assert_no_error (local_error);
      g_assert_nonnull (pool);

      /* Push two jobs into the pool. The first one will start executing and
       * will block, the second one will wait in the queue as there’s only one
       * worker thread. */
      for (j = 0; j < 2; j++)
        {
          success = g_thread_pool_push (pool, &test_data, &local_error);
          g_assert_no_error (local_error);
          g_assert_true (success);
        }

      /* Wait for the first job to start. */
      while (g_atomic_int_get (&test_data.n_jobs_started) == 0);

      /* Free the pool. This won’t actually free the queued second job yet, as
       * the thread pool hangs around until the executing first job has
       * completed. The first job will complete only once @threads_should_block
       * is unset. */
      g_thread_pool_free (pool, TRUE, FALSE);

      g_assert_cmpuint (g_atomic_int_get (&test_data.n_jobs_started), ==, 1);
      g_assert_cmpuint (g_atomic_int_get (&test_data.n_jobs_completed), ==, 0);
      g_assert_cmpuint (g_atomic_int_get (&test_data.n_free_func_calls), ==, 0);

      /* Unblock the job and allow the pool to be freed. */
      g_mutex_lock (&test_data.mutex);
      test_data.threads_should_block = FALSE;
      g_cond_signal (&test_data.cond);
      g_mutex_unlock (&test_data.mutex);

      /* Wait for the first job to complete before freeing the mutex and cond. */
      while (g_atomic_int_get (&test_data.n_jobs_completed) != 1 ||
             g_atomic_int_get (&test_data.n_free_func_calls) != 1);

      g_assert_cmpuint (g_atomic_int_get (&test_data.n_jobs_started), ==, 1);
      g_assert_cmpuint (g_atomic_int_get (&test_data.n_jobs_completed), ==, 1);
      g_assert_cmpuint (g_atomic_int_get (&test_data.n_free_func_calls), ==, 1);

      g_cond_clear (&test_data.cond);
      g_mutex_clear (&test_data.mutex);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  /* Run this test first, before the spawn_thread_queue has been
   * created: */
  g_test_add_func ("/thread_pool/pool_fail", test_pool_fail);
  /* Remaining tests should run fine. */

  g_test_add_data_func ("/thread_pool/shared", GINT_TO_POINTER (TRUE), test_simple);
  g_test_add_data_func ("/thread_pool/exclusive", GINT_TO_POINTER (FALSE), test_simple);
  g_test_add_data_func ("/thread_pool/create_shared_after_exclusive", GINT_TO_POINTER (FALSE), test_create_first_pool);
  g_test_add_data_func ("/thread_pool/create_full", NULL, test_thread_pool_full);
  g_test_add_data_func ("/thread_pool/create_exclusive_after_shared", GINT_TO_POINTER (TRUE), test_create_first_pool);

  return g_test_run ();
}
