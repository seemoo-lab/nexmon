/* Unit tests for GThread
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

#include <config.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include <glib.h>

#include "glib/glib-private.h"

#ifdef G_OS_UNIX
#include <unistd.h>
#include <sys/resource.h>
#endif

#ifdef THREADS_POSIX
#include <pthread.h>
#endif

static gpointer
thread1_func (gpointer data)
{
  g_thread_exit (GINT_TO_POINTER (1));

  g_assert_not_reached ();

  return NULL;
}

/* test that g_thread_exit() works */
static void
test_thread1 (void)
{
  gpointer result;
  GThread *thread;
  GError *error = NULL;

  thread = g_thread_try_new ("test", thread1_func, NULL, &error);
  g_assert_no_error (error);

  result = g_thread_join (thread);

  g_assert_cmpint (GPOINTER_TO_INT (result), ==, 1);
}

static gpointer
thread2_func (gpointer data)
{
  return g_thread_self ();
}

/* test that g_thread_self() works */
static void
test_thread2 (void)
{
  gpointer result;
  GThread *thread;

  thread = g_thread_new ("test", thread2_func, NULL);

  g_assert (g_thread_self () != thread);

  result = g_thread_join (thread);

  g_assert (result == thread);
}

static gpointer
thread3_func (gpointer data)
{
  GThread *peer = data;
  gint retval;

  retval = 3;

  if (peer)
    {
      gpointer result;

      result = g_thread_join (peer);

      retval += GPOINTER_TO_INT (result);
    }

  return GINT_TO_POINTER (retval);
}

/* test that g_thread_join() works across peers */
static void
test_thread3 (void)
{
  gpointer result;
  GThread *thread1, *thread2, *thread3;

  thread1 = g_thread_new ("a", thread3_func, NULL);
  thread2 = g_thread_new ("b", thread3_func, thread1);
  thread3 = g_thread_new ("c", thread3_func, thread2);

  result = g_thread_join (thread3);

  g_assert_cmpint (GPOINTER_TO_INT(result), ==, 9);
}

/* test that thread creation fails as expected,
 * by setting RLIMIT_NPROC ridiculously low
 */
static void
test_thread4 (void)
{
#ifdef _GLIB_ADDRESS_SANITIZER
  g_test_incomplete ("FIXME: Leaks a GSystemThread's name, see glib#2308");
#elif defined(HAVE_PRLIMIT)
  struct rlimit ol, nl;
  GThread *thread;
  GError *error;

  getrlimit (RLIMIT_NPROC, &nl);
  nl.rlim_cur = 1;

  if (prlimit (getpid (), RLIMIT_NPROC, &nl, &ol) != 0)
    g_error ("setting RLIMIT_NPROC to {cur=%ld,max=%ld} failed: %s",
             (long) nl.rlim_cur, (long) nl.rlim_max, g_strerror (errno));

  error = NULL;
  thread = g_thread_try_new ("a", thread1_func, NULL, &error);

  if (thread != NULL)
    {
      gpointer result;

      /* Privileged processes might be able to create new threads even
       * though the rlimit is too low. There isn't much we can do about
       * this; we just can't test this failure mode in this situation. */
      g_test_skip ("Unable to test g_thread_try_new() failing with EAGAIN "
                   "while privileged (CAP_SYS_RESOURCE, CAP_SYS_ADMIN or "
                   "euid 0?)");
      result = g_thread_join (thread);
      g_assert_cmpint (GPOINTER_TO_INT (result), ==, 1);
    }
  else
    {
      g_assert (thread == NULL);
      g_assert_error (error, G_THREAD_ERROR, G_THREAD_ERROR_AGAIN);
      g_error_free (error);
    }

  if (prlimit (getpid (), RLIMIT_NPROC, &ol, NULL) != 0)
    g_error ("resetting RLIMIT_NPROC failed: %s", g_strerror (errno));
#endif
}

static void
test_thread5 (void)
{
  GThread *thread;

  thread = g_thread_new ("a", thread3_func, NULL);
  g_thread_ref (thread);
  g_thread_join (thread);
  g_thread_unref (thread);
}

static gpointer
thread6_func (gpointer data)
{
#if defined (HAVE_PTHREAD_SETNAME_NP_WITH_TID) && defined (HAVE_PTHREAD_GETNAME_NP)
  char name[16];
  const char *name2;

  pthread_getname_np (pthread_self(), name, 16);

  g_assert_cmpstr (name, ==, data);

  name2 = g_thread_get_name (g_thread_self ());

  g_assert_cmpstr (name2, ==, data);
#endif

  return NULL;
}

static void
test_thread6 (void)
{
  GThread *thread;

  thread = g_thread_new ("abc", thread6_func, "abc");
  g_thread_join (thread);
}

#if defined(_SC_NPROCESSORS_ONLN) && defined(THREADS_POSIX) && defined(HAVE_PTHREAD_GETAFFINITY_NP)
static gpointer
thread7_func (gpointer data)
{
  int idx = 0, err;
  int ncores = sysconf (_SC_NPROCESSORS_ONLN);

  cpu_set_t old_mask, new_mask;

  err = pthread_getaffinity_np (pthread_self (), sizeof (old_mask), &old_mask);
  CPU_ZERO (&new_mask);
  g_assert_cmpint (err, ==, 0);

  for (idx = 0; idx < ncores; ++idx)
    if (CPU_ISSET (idx, &old_mask))
      {
        CPU_SET (idx, &new_mask);
        break;
      }

  err = pthread_setaffinity_np (pthread_self (), sizeof (new_mask), &new_mask);
  g_assert_cmpint (err, ==, 0);

  int af_count = g_get_num_processors ();
  return GINT_TO_POINTER (af_count);
}
#endif

static void
test_thread7 (void)
{
#if defined(_SC_NPROCESSORS_ONLN) && defined(THREADS_POSIX) && defined(HAVE_PTHREAD_GETAFFINITY_NP)
  GThread *thread = g_thread_new ("mask", thread7_func, NULL);
  gpointer result = g_thread_join (thread);

  g_assert_cmpint (GPOINTER_TO_INT (result), ==, 1);
#else
  g_test_skip ("Skipping because pthread_getaffinity_np() is not available");
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/thread/thread1", test_thread1);
  g_test_add_func ("/thread/thread2", test_thread2);
  g_test_add_func ("/thread/thread3", test_thread3);
  g_test_add_func ("/thread/thread4", test_thread4);
  g_test_add_func ("/thread/thread5", test_thread5);
  g_test_add_func ("/thread/thread6", test_thread6);
  g_test_add_func ("/thread/thread7", test_thread7);

  return g_test_run ();
}
