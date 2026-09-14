/* GLib testing framework examples and tests
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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

#define _POSIX_C_SOURCE 200809L  /* for F_DUPFD_CLOEXEC */

#include "config.h"

/* We want to distinguish between messages originating from libglib
 * and messages originating from this program.
 */
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "testing"

#include <glib.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#ifdef G_OS_UNIX
#include <fcntl.h>
#include <glib-unix.h>
#include <unistd.h>
#endif

#define TAP_VERSION G_STRINGIFY (14)
#define TAP_SUBTEST_PREFIX "    "

/* test assertion variants */
static void
test_assertions_bad_cmpvariant_types (void)
{
  GVariant *v1, *v2;

  v1 = g_variant_new_boolean (TRUE);
  v2 = g_variant_new_string ("hello");

  g_assert_cmpvariant (v1, v2);

  g_variant_unref (v2);
  g_variant_unref (v1);

  exit (0);
}

static void
test_assertions_bad_cmpvariant_values (void)
{
  GVariant *v1, *v2;

  v1 = g_variant_new_string ("goodbye");
  v2 = g_variant_new_string ("hello");

  g_assert_cmpvariant (v1, v2);

  g_variant_unref (v2);
  g_variant_unref (v1);

  exit (0);
}

static void
test_assertions_bad_cmpstrv_null1 (void)
{
  const char *strv[] = { "one", "two", "three", NULL };
  g_assert_cmpstrv (strv, NULL);
  exit (0);
}

static void
test_assertions_bad_cmpstrv_null2 (void)
{
  const char *strv[] = { "one", "two", "three", NULL };
  g_assert_cmpstrv (NULL, strv);
  exit (0);
}

static void
test_assertions_bad_cmpstrv_length (void)
{
  const char *strv1[] = { "one", "two", "three", NULL };
  const char *strv2[] = { "one", "two", NULL };
  g_assert_cmpstrv (strv1, strv2);
  exit (0);
}

static void
test_assertions_bad_cmpstrv_values (void)
{
  const char *strv1[] = { "one", "two", "three", NULL };
  const char *strv2[] = { "one", "too", "three", NULL };
  g_assert_cmpstrv (strv1, strv2);
  exit (0);
}

static void
test_assertions_bad_cmpstr (void)
{
  g_assert_cmpstr ("fzz", !=, "fzz");
  exit (0);
}

static void
test_assertions_bad_cmpint (void)
{
  g_assert_cmpint (4, !=, 4);
  exit (0);
}

static void
test_assertions_bad_cmpmem_len (void)
{
  g_assert_cmpmem ("foo", 3, "foot", 4);
  exit (0);
}

static void
test_assertions_bad_cmpmem_data (void)
{
  g_assert_cmpmem ("foo", 3, "fzz", 3);
  exit (0);
}

static void
test_assertions_bad_cmpmem_null (void)
{
  g_assert_cmpmem (NULL, 3, NULL, 3);
  exit (0);
}

static void
test_assertions_bad_cmpfloat_epsilon (void)
{
  g_assert_cmpfloat_with_epsilon (3.14, 3.15, 0.001);
  exit (0);
}

/* Emulates something like rmdir() failing. */
static int
return_errno (void)
{
  errno = ERANGE;  /* arbitrary non-zero value */
  return -1;
}

/* Emulates something like rmdir() succeeding. */
static int
return_no_errno (void)
{
  return 0;
}

static void
test_assertions_bad_no_errno (void)
{
  g_assert_no_errno (return_errno ());
}

static void
test_assertions (void)
{
  const char *strv1[] = { "one", "two", "three", NULL };
  const char *strv2[] = { "one", "two", "three", NULL };
  GVariant *v1, *v2;
  gchar *fuu;

  g_assert_cmpint (1, >, 0);
  g_assert_cmphex (2, ==, 2);
  g_assert_cmpfloat (3.3, !=, 7);
  g_assert_cmpfloat (7, <=, 3 + 4);
  g_assert_cmpfloat_with_epsilon (3.14, 3.15, 0.01);
  g_assert_cmpfloat_with_epsilon (3.14159, 3.1416, 0.0001);
  g_assert (TRUE);
  g_assert_true (TRUE);
  g_assert_cmpstr ("foo", !=, "faa");
  fuu = g_strdup_printf ("f%s", "uu");
  g_test_queue_free (fuu);
  g_assert_cmpstr ("foo", !=, fuu);
  g_assert_cmpstr ("fuu", ==, fuu);
  g_assert_cmpstr (NULL, <, "");
  g_assert_cmpstr (NULL, ==, NULL);
  g_assert_cmpstr ("", >, NULL);
  g_assert_cmpstr ("foo", <, "fzz");
  g_assert_cmpstr ("fzz", >, "faa");
  g_assert_cmpstr ("fzz", ==, "fzz");
  g_assert_cmpmem ("foo", 3, "foot", 3);
  g_assert_cmpmem (NULL, 0, NULL, 0);
  g_assert_cmpmem (NULL, 0, "foot", 0);
  g_assert_cmpmem ("foo", 0, NULL, 0);
  g_assert_no_errno (return_no_errno ());

  g_assert_cmpstrv (NULL, NULL);
  g_assert_cmpstrv (strv1, strv2);

  v1 = g_variant_new_parsed ("['hello', 'there']");
  v2 = g_variant_new_parsed ("['hello', 'there']");

  g_assert_cmpvariant (v1, v1);
  g_assert_cmpvariant (v1, v2);

  g_variant_unref (v2);
  g_variant_unref (v1);

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpvariant_types", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpvariant_values", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpstr", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpstrv_null1", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpstrv_null2", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpstrv_length", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpstrv_values", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpint", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpmem_len", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*len*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpmem_data", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");
  g_test_trap_assert_stderr_unmatched ("*assertion failed*len*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpmem_null", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*NULL*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_cmpfloat_epsilon", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");

  g_test_trap_subprocess ("/misc/assertions/subprocess/bad_no_errno", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*");
}

/* test g_test_timer* API */
static void
test_timer (void)
{
  double ttime;
  g_test_timer_start();
  g_assert_cmpfloat (g_test_timer_last(), ==, 0);
  g_usleep (25 * 1000);
  ttime = g_test_timer_elapsed();
  g_assert_cmpfloat (ttime, >, 0);
  g_assert_cmpfloat (g_test_timer_last(), ==, ttime);
  g_test_minimized_result (ttime, "timer-test-time: %fsec", ttime);
  g_test_maximized_result (5, "bogus-quantity: %ddummies", 5); /* simple API test */
}

#ifdef G_OS_UNIX
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* fork out for a failing test */
static void
test_fork_fail (void)
{
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
    {
      g_assert_not_reached();
    }
  g_test_trap_assert_failed();
  g_test_trap_assert_stderr ("*ERROR*test_fork_fail*should not be reached*");
}

/* fork out to assert stdout and stderr patterns */
static void
test_fork_patterns (void)
{
  if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR))
    {
      g_print ("some stdout text: somagic17\n");
      g_printerr ("some stderr text: semagic43\n");
      exit (0);
    }
  g_test_trap_assert_passed();
  g_test_trap_assert_stdout ("*somagic17*");
  g_test_trap_assert_stderr ("*semagic43*");
}

/* fork out for a timeout test */
static void
test_fork_timeout (void)
{
  /* allow child to run for only a fraction of a second */
  if (g_test_trap_fork ((guint64) (0.11 * G_USEC_PER_SEC), G_TEST_TRAP_DEFAULT))
    {
      /* loop and sleep forever */
      while (TRUE)
        g_usleep (1000 * 1000);
    }
  g_test_trap_assert_failed();
  g_assert_true (g_test_trap_reached_timeout());
}

G_GNUC_END_IGNORE_DEPRECATIONS
#endif /* G_OS_UNIX */

static void
test_subprocess_fail (void)
{
  if (g_test_subprocess ())
    {
      g_assert_not_reached ();
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*ERROR*test_subprocess_fail*should not be reached*");
}

static void
test_subprocess_skip (void)
{
  if (g_test_subprocess ())
    {
      g_test_skip ("");
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_assert_true (g_test_trap_has_skipped ());
  g_assert_true (!g_test_trap_has_passed ());
}

static void
test_subprocess_no_such_test (void)
{
  if (g_test_subprocess ())
    {
      g_test_trap_subprocess ("/trap_subprocess/this-test-does-not-exist", 0,
                              G_TEST_SUBPROCESS_DEFAULT);
      g_assert_not_reached ();
      return;
    }
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*test does not exist*");
  g_test_trap_assert_stderr_unmatched ("*should not be reached*");
}

static void
test_subprocess_patterns (void)
{
  if (g_test_subprocess ())
    {
      g_print ("some stdout text: somagic17\n");
      g_printerr ("some stderr text: semagic43\n");
      exit (0);
    }
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*somagic17*");
  g_test_trap_assert_stderr ("*semagic43*");
}

static void
test_subprocess_timeout (void)
{
  if (g_test_subprocess ())
    {
      /* loop and sleep forever */
      while (TRUE)
        g_usleep (G_USEC_PER_SEC);
      return;
    }
  /* allow child to run for only a fraction of a second */
  g_test_trap_subprocess (NULL, (guint64) (0.05 * G_USEC_PER_SEC), G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_assert_true (g_test_trap_reached_timeout ());
}

static void
test_subprocess_envp (void)
{
  char **envp = NULL;

  if (g_test_subprocess ())
    {
      g_assert_cmpstr (g_getenv ("TEST_SUBPROCESS_VARIABLE"), ==, "definitely set");
      return;
    }

  envp = g_get_environ ();
  envp = g_environ_setenv (g_steal_pointer (&envp), "TEST_SUBPROCESS_VARIABLE", "definitely set", TRUE);
  g_test_trap_subprocess_with_envp (NULL, (const gchar * const *) envp,
                                    0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_strfreev (envp);
}

static void
test_subprocess_stdin (void)
{
#ifdef G_OS_UNIX
  int old_stdin_fd = -1;
  int pipe_fd[2] = { -1, -1 };
  const char *test_string = "*hello there*";
  GError *local_error = NULL;

  if (g_test_subprocess ())
    {
      char buf[100];
      ssize_t n_read;

      g_assert_no_errno (n_read = read (STDIN_FILENO, buf, sizeof (buf)));
      g_assert_cmpint (n_read, >, 0);

      g_print ("Read: %.*s\n", (int) n_read, buf);

      return;
    }

  /* Temporarily override this process’ stdin with a pipe. */
  old_stdin_fd = fcntl (STDIN_FILENO, F_DUPFD_CLOEXEC, 0);
  g_assert_cmpint (old_stdin_fd, >=, 0);

  g_unix_open_pipe (pipe_fd, O_CLOEXEC, &local_error);
  g_assert_no_error (local_error);

  g_assert_no_errno (dup2 (pipe_fd[0], STDIN_FILENO));

  /* Write something into it for the subprocess to read. */
  g_assert_no_errno (write (pipe_fd[1], test_string, strlen (test_string) + 1));

  /* Run the subprocess. */
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDIN);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout (test_string);

  /* Restore the old stdin */
  g_assert_no_errno (dup2 (old_stdin_fd, STDIN_FILENO));

  g_assert_no_errno (close (old_stdin_fd));
  g_assert_no_errno (close (pipe_fd[0]));
  g_assert_no_errno (close (pipe_fd[1]));
#else
  g_test_skip ("Testing stdin for subprocesses can only be done on Unix at the moment");
#endif
}

/* run a test with fixture setup and teardown */
typedef struct {
  guint  seed;
  guint  prime;
  gchar *msg;
} Fixturetest;
static void
fixturetest_setup (Fixturetest  *fix,
                   gconstpointer test_data)
{
  g_assert_true (test_data == (void*) 0xc0cac01a);
  fix->seed = 18;
  fix->prime = 19;
  fix->msg = g_strdup_printf ("%d", fix->prime);
}
static void
fixturetest_test (Fixturetest  *fix,
                  gconstpointer test_data)
{
  guint prime = g_spaced_primes_closest (fix->seed);
  g_assert_cmpint (prime, ==, fix->prime);
  prime = g_ascii_strtoull (fix->msg, NULL, 0);
  g_assert_cmpint (prime, ==, fix->prime);
  g_assert_true (test_data == (void*) 0xc0cac01a);
}
static void
fixturetest_teardown (Fixturetest  *fix,
                      gconstpointer test_data)
{
  g_assert_true (test_data == (void*) 0xc0cac01a);
  g_free (fix->msg);
}

static struct {
  int bit, vint1, vint2, irange;
  long double vdouble, drange;
} shared_rand_state;

static void
test_rand1 (void)
{
  shared_rand_state.bit = g_test_rand_bit();
  shared_rand_state.vint1 = g_test_rand_int();
  shared_rand_state.vint2 = g_test_rand_int();
  g_assert_cmpint (shared_rand_state.vint1, !=, shared_rand_state.vint2);
  shared_rand_state.irange = g_test_rand_int_range (17, 35);
  g_assert_cmpint (shared_rand_state.irange, >=, 17);
  g_assert_cmpint (shared_rand_state.irange, <=, 35);
  shared_rand_state.vdouble = g_test_rand_double();
  shared_rand_state.drange = g_test_rand_double_range (-999, +17);
  g_assert_cmpfloat (shared_rand_state.drange, >=, -999);
  g_assert_cmpfloat (shared_rand_state.drange, <=, +17);
}

static void
test_rand2 (void)
{
  /* this test only works if run after test1.
   * we do this to check that random number generators
   * are reseeded upon fixture setup.
   */
  g_assert_cmpint (shared_rand_state.bit, ==, g_test_rand_bit());
  g_assert_cmpint (shared_rand_state.vint1, ==, g_test_rand_int());
  g_assert_cmpint (shared_rand_state.vint2, ==, g_test_rand_int());
  g_assert_cmpint (shared_rand_state.irange, ==, g_test_rand_int_range (17, 35));
  g_assert_cmpfloat (shared_rand_state.vdouble, ==, g_test_rand_double());
  g_assert_cmpfloat (shared_rand_state.drange, ==, g_test_rand_double_range (-999, +17));
}

static void
test_data_test (gconstpointer test_data)
{
  g_assert_true (test_data == (void*) 0xc0c0baba);
}

static void
test_random_conversions (void)
{
  /* very simple conversion test using random numbers */
  int vint = g_test_rand_int();
  char *err, *str = g_strdup_printf ("%d", vint);
  gint64 vint64 = g_ascii_strtoll (str, &err, 10);
  g_assert_cmpint (vint, ==, vint64);
  g_assert_true (!err || *err == 0);
  g_free (str);
}

static gboolean
fatal_handler (const gchar    *log_domain,
               GLogLevelFlags  log_level,
               const gchar    *message,
               gpointer        user_data)
{
  return FALSE;
}

static void
test_fatal_log_handler_critical_pass (void)
{
  g_test_log_set_fatal_handler (fatal_handler, NULL);
  g_str_has_prefix (NULL, "file://");
  g_critical ("Test passing");
  exit (0);
}

static void
test_fatal_log_handler_error_fail (void)
{
  g_error ("Test failing");
  exit (0);
}

static void
test_fatal_log_handler_critical_fail (void)
{
  g_str_has_prefix (NULL, "file://");
  g_critical ("Test passing");
  exit (0);
}

static void
test_fatal_log_handler (void)
{
  g_test_trap_subprocess ("/misc/fatal-log-handler/subprocess/critical-pass", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("*CRITICAL*g_str_has_prefix*");
  g_test_trap_assert_stderr ("*CRITICAL*Test passing*");

  g_test_trap_subprocess ("/misc/fatal-log-handler/subprocess/error-fail", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*ERROR*Test failing*");

  g_test_trap_subprocess ("/misc/fatal-log-handler/subprocess/critical-fail", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*CRITICAL*g_str_has_prefix*");
  g_test_trap_assert_stderr_unmatched ("*CRITICAL*Test passing*");
}

static void
test_expected_messages_warning (void)
{
  g_warning ("This is a %d warning", g_random_int ());
  g_return_if_reached ();
}

static void
test_expected_messages_expect_warning (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "This is a * warning");
  test_expected_messages_warning ();
}

static void
test_expected_messages_wrong_warning (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*should not be *");
  test_expected_messages_warning ();
}

static void
test_expected_messages_expected (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "This is a * warning");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*should not be reached");

  test_expected_messages_warning ();

  g_test_assert_expected_messages ();
  exit (0);
}

static void
test_expected_messages_null_domain (void)
{
  g_test_expect_message (NULL, G_LOG_LEVEL_WARNING, "no domain");
  g_log (NULL, G_LOG_LEVEL_WARNING, "no domain");
  g_test_assert_expected_messages ();
}

static void
test_expected_messages_expect_error (void)
{
  /* make sure we can't try to expect a g_error() */
  g_test_expect_message ("GLib", G_LOG_LEVEL_CRITICAL, "*G_LOG_LEVEL_ERROR*");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR, "this won't work");
  g_test_assert_expected_messages ();
}

static void
test_expected_messages_extra_warning (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "This is a * warning");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*should not be reached");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "nope");

  test_expected_messages_warning ();

  /* If we don't assert, it won't notice the missing message */
  exit (0);
}

static void
test_expected_messages_unexpected_extra_warning (void)
{
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
                         "This is a * warning");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*should not be reached");
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "nope");

  test_expected_messages_warning ();

  g_test_assert_expected_messages ();
  exit (0);
}

static void
test_expected_messages (void)
{
  g_test_trap_subprocess ("/misc/expected-messages/subprocess/warning", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*This is a * warning*");
  g_test_trap_assert_stderr_unmatched ("*should not be reached*");

  g_test_trap_subprocess ("/misc/expected-messages/subprocess/expect-warning", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr_unmatched ("*This is a * warning*");
  g_test_trap_assert_stderr ("*should not be reached*");

  g_test_trap_subprocess ("/misc/expected-messages/subprocess/wrong-warning", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr_unmatched ("*should not be reached*");
  g_test_trap_assert_stderr ("*GLib-CRITICAL*Did not see expected message testing-CRITICAL*should not be *WARNING*This is a * warning*");

  g_test_trap_subprocess ("/misc/expected-messages/subprocess/expected", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("");

  g_test_trap_subprocess ("/misc/expected-messages/subprocess/null-domain", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("");

  g_test_trap_subprocess ("/misc/expected-messages/subprocess/extra-warning", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("");

  g_test_trap_subprocess ("/misc/expected-messages/subprocess/unexpected-extra-warning", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*GLib:ERROR*Did not see expected message testing-CRITICAL*nope*");
}

static void
test_messages (void)
{
  g_test_trap_subprocess ("/misc/messages/subprocess/use-stderr", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_stderr ("*message is in stderr*");
  g_test_trap_assert_stderr ("*warning is in stderr*");
  g_test_trap_has_passed ();
}

static void
test_messages_use_stderr (void)
{
  g_message ("message is in stderr");
  g_warning ("warning is in stderr");
}

static void
test_expected_messages_debug (void)
{
  g_test_expect_message ("Test", G_LOG_LEVEL_WARNING, "warning message");
  g_log ("Test", G_LOG_LEVEL_DEBUG, "should be ignored");
  g_log ("Test", G_LOG_LEVEL_WARNING, "warning message");
  g_test_assert_expected_messages ();

  g_test_expect_message ("Test", G_LOG_LEVEL_DEBUG, "debug message");
  g_log ("Test", G_LOG_LEVEL_DEBUG, "debug message");
  g_test_assert_expected_messages ();
}

static void
test_dash_p_hidden (void)
{
  if (!g_test_subprocess ())
    g_assert_not_reached ();

  g_print ("Test /misc/dash-p/subprocess/hidden ran\n");
}

static void
test_dash_p_hidden_sub (void)
{
  if (!g_test_subprocess ())
    g_assert_not_reached ();

  g_print ("Test /misc/dash-p/subprocess/hidden/sub ran\n");
}

/* The rest of the dash_p tests will get run by the toplevel test
 * process, but they shouldn't do anything there.
 */

static void
test_dash_p_child (void)
{
  if (!g_test_subprocess ())
    return;

  g_print ("Test /misc/dash-p/child ran\n");
}

static void
test_dash_p_child_sub (void)
{
  if (!g_test_subprocess ())
    return;

  g_print ("Test /misc/dash-p/child/sub ran\n");
}

static void
test_dash_p_child_sub2 (void)
{
  if (!g_test_subprocess ())
    return;

  g_print ("Test /misc/dash-p/child/sub2 ran\n");
}

static void
test_dash_p_child_sub_child (void)
{
  if (!g_test_subprocess ())
    return;

  g_print ("Test /misc/dash-p/child/subprocess ran\n");
}

static void
test_dash_p (void)
{
  g_test_trap_subprocess ("/misc/dash-p/subprocess/hidden", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*Test /misc/dash-p/subprocess/hidden ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/subprocess/hidden/sub ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/subprocess/hidden/sub2 ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/subprocess/hidden/sub/subprocess ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/child*");

  g_test_trap_subprocess ("/misc/dash-p/subprocess/hidden/sub", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*Test /misc/dash-p/subprocess/hidden/sub ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/subprocess/hidden ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/subprocess/hidden/sub2 ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/subprocess/hidden/subprocess ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/child*");

  g_test_trap_subprocess ("/misc/dash-p/child", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*Test /misc/dash-p/child ran*");
  g_test_trap_assert_stdout ("*Test /misc/dash-p/child/sub ran*");
  g_test_trap_assert_stdout ("*Test /misc/dash-p/child/sub2 ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/child/subprocess ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/subprocess/hidden*");

  g_test_trap_subprocess ("/misc/dash-p/child/sub", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*Test /misc/dash-p/child/sub ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/child ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/child/sub2 ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/child/subprocess ran*");
  g_test_trap_assert_stdout_unmatched ("*Test /misc/dash-p/subprocess/hidden*");
}

static void
test_nonfatal (void)
{
  if (g_test_subprocess ())
    {
      g_test_set_nonfatal_assertions ();
      g_assert_cmpint (4, ==, 5);
      g_print ("The End\n");
      return;
    }
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("*assertion failed*4 == 5*");
  g_test_trap_assert_stdout ("*The End*");
}

static void
test_skip (void)
{
  g_test_skip ("Skipped should count as passed, not failed");
  /* This function really means "the test concluded with a non-successful
   * status" rather than "the test failed": it is documented to return
   * true for skipped and incomplete tests, not just for failures. */
  g_assert_true (g_test_failed ());
}

static void
test_pass (void)
{
}

static void
subprocess_fail (void)
{
  /* Exit 1 instead of raising SIGABRT so that we can make assertions about
   * how this combines with skipped/incomplete tests */
  g_test_set_nonfatal_assertions ();
  g_test_fail ();
  g_assert_true (g_test_failed ());
}

static void
test_fail (void)
{
  if (g_test_subprocess ())
    {
      subprocess_fail ();
      return;
    }
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
}

static void
subprocess_incomplete (void)
{
  g_test_incomplete ("not done");
  /* This function really means "the test concluded with a non-successful
   * status" rather than "the test failed": it is documented to return
   * true for skipped and incomplete tests, not just for failures. */
  g_assert_true (g_test_failed ());
}

static void
test_incomplete (void)
{
  if (g_test_subprocess ())
    {
      subprocess_incomplete ();
      return;
    }
  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  /* An incomplete test represents functionality that is known not to be
   * implemented yet (an expected failure), so it does not cause test
   * failure; but it does count as the test having been skipped, which
   * causes nonzero exit status 77, which is treated as failure by
   * g_test_trap_subprocess(). */
  g_test_trap_assert_failed ();
}

static void
test_path_first (void)
{
  g_assert_cmpstr (g_test_get_path (), ==, "/misc/path/first");
}

static void
test_path_second (void)
{
  g_assert_cmpstr (g_test_get_path (), ==, "/misc/path/second");
}

static const char *argv0;

static void
test_combining (void)
{
  GPtrArray *argv;
  GError *error = NULL;
  int status;

  g_test_message ("single test case skipped -> overall status 77");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/skip");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 77);
  g_clear_error (&error);

  g_test_message ("each test case skipped -> overall status 77");
  g_ptr_array_set_size (argv, 0);
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/skip");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/skip1");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/skip2");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 77);
  g_clear_error (&error);

  g_test_message ("single test case incomplete -> overall status 77");
  g_ptr_array_set_size (argv, 0);
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/incomplete");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 77);
  g_clear_error (&error);

  g_test_message ("one pass and some skipped -> overall status 0");
  g_ptr_array_set_size (argv, 0);
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/skip");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/pass");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/skip1");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_test_message ("one pass and some incomplete -> overall status 0");
  g_ptr_array_set_size (argv, 0);
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/pass");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/incomplete");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_test_message ("one pass and mix of skipped and incomplete -> overall status 0");
  g_ptr_array_set_size (argv, 0);
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/pass");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/skip1");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/incomplete");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_test_message ("one fail and some skipped -> overall status fail");
  g_ptr_array_set_size (argv, 0);
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/skip");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/fail");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/skip1");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (&error);

  g_test_message ("one fail and some incomplete -> overall status fail");
  g_ptr_array_set_size (argv, 0);
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/fail");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/incomplete");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (&error);

  g_test_message ("one fail and mix of skipped and incomplete -> overall status fail");
  g_ptr_array_set_size (argv, 0);
  g_ptr_array_add (argv, (char *) argv0);
  g_ptr_array_add (argv, "--GTestSubprocess");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/fail");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/skip1");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/misc/combining/subprocess/incomplete");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (&error);

  g_ptr_array_unref (argv);
}

/* Test the TAP output when a test suite is run with --tap. */
static void
test_tap (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **envp;

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  g_test_message ("pass");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "pass");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  /* Remove the G_TEST_ROOT_PROCESS env so it will be considered a standalone test */
  envp = g_get_environ ();
  g_assert_nonnull (g_environ_getenv (envp, "G_TEST_ROOT_PROCESS"));
  envp = g_environ_unsetenv (g_steal_pointer (&envp), "G_TEST_ROOT_PROCESS");

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nok 1 /pass\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("skip");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nok 1 /skip # SKIP not enough tea\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("skip with printf format");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-printf");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nok 1 /skip-printf # SKIP not enough coffee\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("incomplete");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "incomplete");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nnot ok 1 /incomplete # TODO mind reading not implemented yet\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("incomplete with printf format");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "incomplete-printf");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nnot ok 1 /incomplete-printf # TODO telekinesis not implemented yet\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("fail");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "fail");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nnot ok 1 /fail\n"));
  g_free (output);
  g_clear_error (&error);
  g_ptr_array_unref (argv);

  g_test_message ("fail with message");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "fail-printf");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nnot ok 1 /fail-printf - this test intentionally left failing\n"));
  g_free (output);
  g_clear_error (&error);
  g_ptr_array_unref (argv);

  g_test_message ("all");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "all");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (&error);
  g_ptr_array_unref (argv);

  g_test_message ("all-non-failures");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "all-non-failures");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_ptr_array_unref (argv);

  g_test_message ("--GTestSkipCount");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "--GTestSkipCount");
  g_ptr_array_add (argv, "2");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "1..10\n"));
  g_assert_nonnull (strstr (output, "\nok 1 /a # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 2 /b # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 3 /b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 4 /b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 5 /b/b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 6 /prefix/a\n"));
  g_assert_nonnull (strstr (output, "\nok 7 /prefix/b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 8 /prefix-long/a\n"));
  g_assert_nonnull (strstr (output, "\nok 9 /c/a\n"));
  g_assert_nonnull (strstr (output, "\nok 10 /d/a\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--GTestSkipCount=0 is the same as omitting it");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "--GTestSkipCount");
  g_ptr_array_add (argv, "0");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "1..10\n"));
  g_assert_nonnull (strstr (output, "\nok 1 /a\n"));
  g_assert_nonnull (strstr (output, "\nok 2 /b\n"));
  g_assert_nonnull (strstr (output, "\nok 3 /b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 4 /b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 5 /b/b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 6 /prefix/a\n"));
  g_assert_nonnull (strstr (output, "\nok 7 /prefix/b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 8 /prefix-long/a\n"));
  g_assert_nonnull (strstr (output, "\nok 9 /c/a\n"));
  g_assert_nonnull (strstr (output, "\nok 10 /d/a\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--GTestSkipCount > number of tests skips all");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "--GTestSkipCount");
  g_ptr_array_add (argv, "11");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "1..10\n"));
  g_assert_nonnull (strstr (output, "\nok 1 /a # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 2 /b # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 3 /b/a # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 4 /b/b # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 5 /b/b/a # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 6 /prefix/a # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 7 /prefix/b/b # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 8 /prefix-long/a # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 9 /c/a # SKIP\n"));
  g_assert_nonnull (strstr (output, "\nok 10 /d/a # SKIP\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("-p");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/b");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nok 1 /c/a\n"));
  g_assert_nonnull (strstr (output, "\nok 2 /c/a\n"));
  g_assert_nonnull (strstr (output, "\nok 3 /b\n"));
  g_assert_nonnull (strstr (output, "\nok 4 /b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 5 /b/b\n"));
  g_assert_nonnull (strstr (output, "\n1..5\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--run-prefix");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-r");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "-r");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "--run-prefix");
  g_ptr_array_add (argv, "/b");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nok 1 /c/a\n"));
  g_assert_nonnull (strstr (output, "\nok 2 /c/a\n"));
  g_assert_nonnull (strstr (output, "\nok 3 /b\n"));
  g_assert_nonnull (strstr (output, "\nok 4 /b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 5 /b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 6 /b/b/a\n"));
  g_assert_nonnull (strstr (output, "\n1..6\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--run-prefix 2");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-r");
  g_ptr_array_add (argv, "/pre");
  g_ptr_array_add (argv, "--run-prefix");
  g_ptr_array_add (argv, "/b/b");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\nok 1 /b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 2 /b/b/a\n"));
  g_assert_nonnull (strstr (output, "\n1..2\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--run-prefix conflict");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-r");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "--run-prefix");
  g_ptr_array_add (argv, "/b");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_spawn_check_wait_status (status, &error);
  g_assert_nonnull (error);
  g_assert_false (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_nonnull (strstr (output, "do not mix [-r | --run-prefix] with '-p'\n"));
  g_clear_error (&error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("-s");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/a");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/b");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/pre");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "1..10\n"));
  g_assert_nonnull (strstr (output, "\nok 1 /a # SKIP by request"));
  g_assert_nonnull (strstr (output, "\nok 2 /b # SKIP by request"));
  /* "-s /b" would skip a test named exactly /b, but not a test named
   * /b/anything */
  g_assert_nonnull (strstr (output, "\nok 3 /b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 4 /b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 5 /b/b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 6 /prefix/a\n"));
  g_assert_nonnull (strstr (output, "\nok 7 /prefix/b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 8 /prefix-long/a\n"));
  g_assert_nonnull (strstr (output, "\nok 9 /c/a # SKIP by request"));
  g_assert_nonnull (strstr (output, "\nok 10 /d/a\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--skip-prefix");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-x");
  g_ptr_array_add (argv, "/a");
  g_ptr_array_add (argv, "--skip-prefix");
  g_ptr_array_add (argv, "/pre");
  g_ptr_array_add (argv, "-x");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_true (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "1..10\n"));
  g_assert_nonnull (strstr (output, "\nok 1 /a # SKIP by request"));
  g_assert_nonnull (strstr (output, "\nok 2 /b\n"));
  g_assert_nonnull (strstr (output, "\nok 3 /b/a\n"));
  g_assert_nonnull (strstr (output, "\nok 4 /b/b\n"));
  g_assert_nonnull (strstr (output, "\nok 5 /b/b/a\n"));
  /* "--skip-prefix /pre" will skip all test path which begins with /pre */
  g_assert_nonnull (strstr (output, "\nok 6 /prefix/a # SKIP by request"));
  g_assert_nonnull (strstr (output, "\nok 7 /prefix/b/b # SKIP by request"));
  g_assert_nonnull (strstr (output, "\nok 8 /prefix-long/a # SKIP by request"));
  g_assert_nonnull (strstr (output, "\nok 9 /c/a # SKIP by request"));
  g_assert_nonnull (strstr (output, "\nok 10 /d/a\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--skip-prefix conflict");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/a");
  g_ptr_array_add (argv, "--skip-prefix");
  g_ptr_array_add (argv, "/pre");
  g_ptr_array_add (argv, "-x");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_spawn_check_wait_status (status, &error);
  g_assert_nonnull (error);
  g_assert_false (g_str_has_prefix (output, "TAP version " TAP_VERSION));
  g_assert_nonnull (strstr (output, "do not mix [-x | --skip-prefix] with '-s'\n"));
  g_clear_error (&error);

  g_free (output);
  g_ptr_array_unref (argv);
  g_strfreev (envp);
}

/* Test the TAP output when a test suite is run with --tap. */
static void
test_tap_subtest (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char** envp = NULL;

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  g_test_message ("pass");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "pass");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  envp = g_get_environ ();
  g_assert_nonnull (g_environ_getenv (envp, "G_TEST_ROOT_PROCESS"));
  g_clear_pointer (&envp, g_strfreev);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_true (g_str_has_prefix (output, "# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /pass\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("skip");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_true (g_str_has_prefix (output, "# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /skip # SKIP not enough tea\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("skip with printf format");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-printf");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_true (g_str_has_prefix (output, "# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /skip-printf # SKIP not enough coffee\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("incomplete");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "incomplete");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null (strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "not ok 1 /incomplete # TODO mind reading not implemented yet\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("incomplete with printf format");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "incomplete-printf");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "not ok 1 /incomplete-printf # TODO telekinesis not implemented yet\n"));
  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("fail");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "fail");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "not ok 1 /fail\n"));
  g_free (output);
  g_clear_error (&error);
  g_ptr_array_unref (argv);

  g_test_message ("fail with message");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "fail-printf");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "not ok 1 /fail-printf - this test intentionally left failing\n"));
  g_free (output);
  g_clear_error (&error);
  g_ptr_array_unref (argv);

  g_test_message ("all");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "all");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_error (error, G_SPAWN_EXIT_ERROR, 1);
  g_clear_error (&error);
  g_ptr_array_unref (argv);

  g_test_message ("all-non-failures");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "all-non-failures");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, NULL, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_ptr_array_unref (argv);

  g_test_message ("--GTestSkipCount");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "--GTestSkipCount");
  g_ptr_array_add (argv, "2");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output, TAP_SUBTEST_PREFIX "1..10\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /a # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 2 /b # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 3 /b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 4 /b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 5 /b/b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 6 /prefix/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 7 /prefix/b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 8 /prefix-long/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 9 /c/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 10 /d/a\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--GTestSkipCount=0 is the same as omitting it");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "--GTestSkipCount");
  g_ptr_array_add (argv, "0");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output, TAP_SUBTEST_PREFIX "1..10\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 2 /b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 3 /b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 4 /b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 5 /b/b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 6 /prefix/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 7 /prefix/b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 8 /prefix-long/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 9 /c/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 10 /d/a\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--GTestSkipCount > number of tests skips all");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "--GTestSkipCount");
  g_ptr_array_add (argv, "11");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output, TAP_SUBTEST_PREFIX "1..10\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /a # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 2 /b # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 3 /b/a # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 4 /b/b # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 5 /b/b/a # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 6 /prefix/a # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 7 /prefix/b/b # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 8 /prefix-long/a # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 9 /c/a # SKIP\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 10 /d/a # SKIP\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("-p");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/b");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /c/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 2 /c/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 3 /b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 4 /b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 5 /b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "1..5\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--run-prefix");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-r");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "-r");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "--run-prefix");
  g_ptr_array_add (argv, "/b");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /c/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 2 /c/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 3 /b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 4 /b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 5 /b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 6 /b/b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "1..6\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--run-prefix 2");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-r");
  g_ptr_array_add (argv, "/pre");
  g_ptr_array_add (argv, "--run-prefix");
  g_ptr_array_add (argv, "/b/b");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 2 /b/b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "1..2\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--run-prefix conflict");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-r");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "-p");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, "--run-prefix");
  g_ptr_array_add (argv, "/b");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_spawn_check_wait_status (status, &error);
  g_assert_nonnull (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output, "do not mix [-r | --run-prefix] with '-p'\n"));
  g_clear_error (&error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("-s");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/a");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/b");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/pre");
  g_ptr_array_add (argv, "-s");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_null( strstr (output, "\n# Subtest: "));
  g_assert_nonnull (strstr (output, TAP_SUBTEST_PREFIX "1..10\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /a # SKIP by request"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 2 /b # SKIP by request"));
  /* "-s /b" would skip a test named exactly /b, but not a test named
   * /b/anything */
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 3 /b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 4 /b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 5 /b/b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 6 /prefix/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 7 /prefix/b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 8 /prefix-long/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 9 /c/a # SKIP by request"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 10 /d/a\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);

  g_test_message ("--skip-prefix");
  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "skip-options");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, "-x");
  g_ptr_array_add (argv, "/a");
  g_ptr_array_add (argv, "--skip-prefix");
  g_ptr_array_add (argv, "/pre");
  g_ptr_array_add (argv, "-x");
  g_ptr_array_add (argv, "/c/a");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "TAP version " TAP_VERSION));
  g_assert_true (g_str_has_prefix (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, TAP_SUBTEST_PREFIX "1..10\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 1 /a # SKIP by request"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 2 /b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 3 /b/a\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 4 /b/b\n"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 5 /b/b/a\n"));
  /* "--skip-prefix /pre" will skip all test path which begins with /pre */
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 6 /prefix/a # SKIP by request"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 7 /prefix/b/b # SKIP by request"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 8 /prefix-long/a # SKIP by request"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 9 /c/a # SKIP by request"));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX "ok 10 /d/a\n"));

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_free (output);
  g_ptr_array_unref (argv);
}

static void
test_tap_summary (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **envp;

  g_test_summary ("Test the output of g_test_summary() from the TAP output of a test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "summary");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  /* Remove the G_TEST_ROOT_PROCESS env so it will be considered a standalone test */
  envp = g_get_environ ();
  g_assert_nonnull (g_environ_getenv (envp, "G_TEST_ROOT_PROCESS"));
  envp = g_environ_unsetenv (g_steal_pointer (&envp), "G_TEST_ROOT_PROCESS");

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_null (strstr (output, "# Subtest: "));

  /* Note: The test path in the output is not `/tap/summary` because it’s the
   * test path from testing-helper, not from this function. */g_assert_null (strstr (output, "# Subtest: "));
  g_assert_nonnull (strstr (output, "\n# /summary summary: Tests that g_test_summary() "
                                    "works with TAP, by outputting a known "
                                    "summary message in testing-helper, and "
                                    "checking for it in the TAP output later.\n"));
  g_free (output);
  g_ptr_array_unref (argv);
  g_strfreev (envp);
}

static void
test_tap_subtest_summary (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;

  g_test_summary ("Test the output of g_test_summary() from the TAP output of a test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "summary");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  /* Note: The test path in the output is not `/tap/summary` because it’s the
   * test path from testing-helper, not from this function. */
  g_assert_true (g_str_has_prefix (output, "# Subtest: "));
  g_assert_nonnull (strstr (output,
    "\n" TAP_SUBTEST_PREFIX
    "# /summary summary: Tests that g_test_summary() "
    "works with TAP, by outputting a known "
    "summary message in testing-helper, and "
    "checking for it in the TAP output later.\n"));
  g_free (output);
  g_ptr_array_unref (argv);
}

static void
test_tap_message (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **output_lines;
  char **envp;

  g_test_summary ("Test the output of g_test_message() from the TAP output of a test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "message");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  /* Remove the G_TEST_ROOT_PROCESS env so it will be considered a standalone test */
  envp = g_get_environ ();
  g_assert_nonnull (g_environ_getenv (envp, "G_TEST_ROOT_PROCESS"));
  envp = g_environ_unsetenv (g_steal_pointer (&envp), "G_TEST_ROOT_PROCESS");

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_assert_null (strstr (output, "# Subtest: "));

  const char *expected_tap_header = "\n1..1\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);
  interesting_lines += strlen (expected_tap_header);

  output_lines = g_strsplit (interesting_lines, "\n", -1);
  g_assert_cmpuint (g_strv_length (output_lines), >=, 12);

  guint i = 0;
  g_assert_cmpstr (output_lines[i++], ==, "# Tests that single line message works");
  g_assert_cmpstr (output_lines[i++], ==, "# Tests that multi");
  g_assert_cmpstr (output_lines[i++], ==, "# ");
  g_assert_cmpstr (output_lines[i++], ==, "# line");
  g_assert_cmpstr (output_lines[i++], ==, "# message");
  g_assert_cmpstr (output_lines[i++], ==, "# works");
  g_assert_cmpstr (output_lines[i++], ==, "# ");
  g_assert_cmpstr (output_lines[i++], ==, "# Tests that multi");
  g_assert_cmpstr (output_lines[i++], ==, "# line");
  g_assert_cmpstr (output_lines[i++], ==, "# message");
  g_assert_cmpstr (output_lines[i++], ==, "# works with leading and trailing too");
  g_assert_cmpstr (output_lines[i++], ==, "# ");

  g_free (output);
  g_strfreev (output_lines);
  g_strfreev (envp);
  g_ptr_array_unref (argv);
}

static void
test_tap_subtest_message (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **output_lines;

  g_test_summary ("Test the output of g_test_message() from the TAP output of a sub-test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "message");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  g_assert_true (g_str_has_prefix (output, "# Subtest: "));

  const char *expected_tap_header = "\n" TAP_SUBTEST_PREFIX "1..1\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);
  interesting_lines += strlen (expected_tap_header);

  output_lines = g_strsplit (interesting_lines, "\n", -1);
  g_assert_cmpuint (g_strv_length (output_lines), >=, 12);

  guint i = 0;
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# Tests that single line message works");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# Tests that multi");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# ");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# line");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# message");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# works");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# ");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# Tests that multi");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# line");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# message");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# works with leading and trailing too");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# ");

  g_free (output);
  g_strfreev (output_lines);
  g_ptr_array_unref (argv);
}

static void
test_tap_print (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **output_lines;
  char **envp;

  g_test_summary ("Test the output of g_print() from the TAP output of a test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "print");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  /* Remove the G_TEST_ROOT_PROCESS env so it will be considered a standalone test */
  envp = g_get_environ ();
  g_assert_nonnull (g_environ_getenv (envp, "G_TEST_ROOT_PROCESS"));
  envp = g_environ_unsetenv (g_steal_pointer (&envp), "G_TEST_ROOT_PROCESS");

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  const char *expected_tap_header = "\n1..1\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);
  interesting_lines += strlen (expected_tap_header);

  output_lines = g_strsplit (interesting_lines, "\n", -1);
  g_assert_cmpuint (g_strv_length (output_lines), >=, 3);

  guint i = 0;
  g_assert_cmpstr (output_lines[i++], ==, "# Tests that single line message works");
  g_assert_cmpstr (output_lines[i++], ==, "# test that multiple");
  g_assert_cmpstr (output_lines[i++], ==, "# lines can be written separately");

  g_free (output);
  g_strfreev (envp);
  g_strfreev (output_lines);
  g_ptr_array_unref (argv);
}

static void
test_tap_subtest_print (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **output_lines;

  g_test_summary ("Test the output of g_test_print() from the TAP output of a sub-test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "print");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  const char *expected_tap_header = "\n" TAP_SUBTEST_PREFIX "1..1\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);
  interesting_lines += strlen (expected_tap_header);

  output_lines = g_strsplit (interesting_lines, "\n", -1);
  g_assert_cmpuint (g_strv_length (output_lines), >=, 3);

  guint i = 0;
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# Tests that single line message works");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# test that multiple");
  g_assert_cmpstr (output_lines[i++], ==, TAP_SUBTEST_PREFIX "# lines can be written separately");

  g_free (output);
  g_strfreev (output_lines);
  g_ptr_array_unref (argv);
}

static void
test_tap_subtest_stdout (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **output_lines;

  g_test_summary ("Test the stdout from the TAP output of a sub-test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "subprocess-stdout");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  const char *expected_tap_header = "\n" TAP_SUBTEST_PREFIX "1..1\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);

  interesting_lines = strstr (interesting_lines, TAP_SUBTEST_PREFIX "# /sub-stdout");
  g_assert_nonnull (interesting_lines);

  output_lines = g_strsplit (interesting_lines, "\n", -1);
  g_assert_cmpuint (g_strv_length (output_lines), >=, 5);

  guint i = 0;
  g_assert_cmpstr (output_lines[i++], ==,
                   TAP_SUBTEST_PREFIX "# /sub-stdout: Tests that single line message works");
  g_assert_cmpstr (output_lines[i++], ==,
                   TAP_SUBTEST_PREFIX "# test that multiple");
  g_assert_cmpstr (output_lines[i++], ==,
                   TAP_SUBTEST_PREFIX "# lines can be written separately");
  g_assert_cmpstr (output_lines[i++], ==,
                   TAP_SUBTEST_PREFIX "# And another line has been put");
  g_assert_cmpstr (output_lines[i++], ==,
                   TAP_SUBTEST_PREFIX "ok 1 /sub-stdout");

  g_free (output);
  g_strfreev (output_lines);
  g_ptr_array_unref (argv);
}

static void
test_tap_subtest_stdout_no_new_line (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **output_lines;

  g_test_summary ("Test the stdout from the TAP output of a sub-test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "subprocess-stdout-no-nl");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);

  const char *expected_tap_header = "\n" TAP_SUBTEST_PREFIX "1..1\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);

  interesting_lines = strstr (interesting_lines, TAP_SUBTEST_PREFIX "# /sub-stdout-no-nl");
  g_assert_nonnull (interesting_lines);

  output_lines = g_strsplit (interesting_lines, "\n", -1);
  g_assert_cmpuint (g_strv_length (output_lines), >=, 2);

  guint i = 0;
  g_assert_cmpstr (output_lines[i++], ==,
                   TAP_SUBTEST_PREFIX "# /sub-stdout-no-nl: A message without trailing new line");
  g_assert_cmpstr (output_lines[i++], ==,
                   TAP_SUBTEST_PREFIX "ok 1 /sub-stdout-no-nl");

  g_free (output);
  g_strfreev (output_lines);
  g_ptr_array_unref (argv);
}

static void
test_tap_error (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **envp;

  g_test_summary ("Test that g_error() generates Bail out TAP output of a test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "error");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  /* Remove the G_TEST_ROOT_PROCESS env so it will be considered a standalone test */
  envp = g_get_environ ();
  g_assert_nonnull (g_environ_getenv (envp, "G_TEST_ROOT_PROCESS"));
  envp = g_environ_unsetenv (g_steal_pointer (&envp), "G_TEST_ROOT_PROCESS");

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_nonnull (error);

  g_assert_false (g_str_has_prefix (output, "# Subtest: "));

  const char *expected_tap_header = "\n1..1\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);
  interesting_lines += strlen (expected_tap_header);

  g_assert_cmpstr (interesting_lines, ==, "not ok /error - GLib-FATAL-ERROR: This should error out "
                   "Because it's just wrong!\n"
                   "Bail out!\n");

  g_free (output);
  g_strfreev (envp);
  g_ptr_array_unref (argv);
  g_clear_error (&error);
}

static void
test_tap_subtest_error (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;

  g_test_summary ("Test that g_error() generates Bail out TAP output of a test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "error");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_nonnull (error);

  g_assert_true (g_str_has_prefix (output, "# Subtest: "));

  const char *expected_tap_header = "\n" TAP_SUBTEST_PREFIX "1..1\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);
  interesting_lines += strlen (expected_tap_header);

  g_assert_cmpstr (interesting_lines, ==,
                   TAP_SUBTEST_PREFIX "not ok /error - GLib-FATAL-ERROR: This should error out "
                   "Because it's just wrong!\n"
                   TAP_SUBTEST_PREFIX "Bail out!\n");

  g_free (output);
  g_ptr_array_unref (argv);
  g_clear_error (&error);
}

static void
test_tap_error_and_pass (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;
  char **envp;

  g_test_summary ("Test that g_error() generates Bail out TAP output of a test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  /* Remove the G_TEST_ROOT_PROCESS env so it will be considered a standalone test */
  envp = g_get_environ ();
  g_assert_nonnull (g_environ_getenv (envp, "G_TEST_ROOT_PROCESS"));
  envp = g_environ_unsetenv (g_steal_pointer (&envp), "G_TEST_ROOT_PROCESS");

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "error-and-pass");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, envp,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_nonnull (error);

  const char *expected_tap_header = "\n1..2\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);
  interesting_lines += strlen (expected_tap_header);

  g_assert_cmpstr (interesting_lines, ==, "not ok /error - GLib-FATAL-ERROR: This should error out "
                   "Because it's just wrong!\n"
                   "Bail out!\n");

  g_free (output);
  g_strfreev (envp);
  g_ptr_array_unref (argv);
  g_clear_error (&error);
}

static void
test_tap_subtest_error_and_pass (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;

  g_test_summary ("Test that g_error() generates Bail out TAP output of a test.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "error-and-pass");
  g_ptr_array_add (argv, "--tap");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_nonnull (error);

  g_assert_true (g_str_has_prefix (output, "# Subtest: "));

  const char *expected_tap_header = "\n" TAP_SUBTEST_PREFIX "1..2\n";
  const char *interesting_lines = strstr (output, expected_tap_header);
  g_assert_nonnull (interesting_lines);
  interesting_lines += strlen (expected_tap_header);

  g_assert_cmpstr (interesting_lines, ==,
                   TAP_SUBTEST_PREFIX "not ok /error - GLib-FATAL-ERROR: This should error out "
                   "Because it's just wrong!\n"
                   TAP_SUBTEST_PREFIX "Bail out!\n");

  g_free (output);
  g_ptr_array_unref (argv);
  g_clear_error (&error);
}

static void
test_init_no_argv0 (void)
{
  const char *testing_helper;
  GPtrArray *argv;
  GError *error = NULL;
  int status;
  gchar *output;

  g_test_summary ("Test that g_test_init() can be called safely with argc == 0.");

  testing_helper = g_test_get_filename (G_TEST_BUILT, "testing-helper" EXEEXT, NULL);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, (char *) testing_helper);
  g_ptr_array_add (argv, "init-null-argv0");
  g_ptr_array_add (argv, NULL);

  /* This has to be spawned manually and can’t be run with g_test_subprocess()
   * because the test helper can’t be run after `g_test_init()` has been called
   * in the process. */
  g_spawn_sync (NULL, (char **) argv->pdata, NULL,
                G_SPAWN_STDERR_TO_DEV_NULL,
                NULL, NULL, &output, NULL, &status,
                &error);
  g_assert_no_error (error);

  g_spawn_check_wait_status (status, &error);
  g_assert_no_error (error);
  g_assert_nonnull (strstr (output, "# random seed:"));
  g_free (output);
  g_ptr_array_unref (argv);
}

int
main (int   argc,
      char *argv[])
{
  int ret;
  char *filename, *filename2;

  argv0 = argv[0];

  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  /* Part of a test for
   * https://gitlab.gnome.org/GNOME/glib/-/issues/2563, see below */
  filename = g_test_build_filename (G_TEST_BUILT, "nonexistent", NULL);

  g_test_add_func ("/random-generator/rand-1", test_rand1);
  g_test_add_func ("/random-generator/rand-2", test_rand2);
  g_test_add_func ("/random-generator/random-conversions", test_random_conversions);
  g_test_add_func ("/misc/assertions", test_assertions);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpvariant_types", test_assertions_bad_cmpvariant_types);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpvariant_values", test_assertions_bad_cmpvariant_values);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpstr", test_assertions_bad_cmpstr);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpstrv_null1", test_assertions_bad_cmpstrv_null1);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpstrv_null2", test_assertions_bad_cmpstrv_null2);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpstrv_length", test_assertions_bad_cmpstrv_length);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpstrv_values", test_assertions_bad_cmpstrv_values);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpint", test_assertions_bad_cmpint);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpmem_len", test_assertions_bad_cmpmem_len);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpmem_data", test_assertions_bad_cmpmem_data);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpmem_null", test_assertions_bad_cmpmem_null);
  g_test_add_func ("/misc/assertions/subprocess/bad_cmpfloat_epsilon", test_assertions_bad_cmpfloat_epsilon);
  g_test_add_func ("/misc/assertions/subprocess/bad_no_errno", test_assertions_bad_no_errno);
  g_test_add_data_func ("/misc/test-data", (void*) 0xc0c0baba, test_data_test);
  g_test_add ("/misc/primetoul", Fixturetest, (void*) 0xc0cac01a, fixturetest_setup, fixturetest_test, fixturetest_teardown);
  if (g_test_perf())
    g_test_add_func ("/misc/timer", test_timer);

#ifdef G_OS_UNIX
  g_test_add_func ("/forking/fail assertion", test_fork_fail);
  g_test_add_func ("/forking/patterns", test_fork_patterns);
  if (g_test_slow())
    g_test_add_func ("/forking/timeout", test_fork_timeout);
#endif

  g_test_add_func ("/trap_subprocess/fail", test_subprocess_fail);
  g_test_add_func ("/trap_subprocess/skip", test_subprocess_skip);
  g_test_add_func ("/trap_subprocess/no-such-test", test_subprocess_no_such_test);
  g_test_add_func ("/trap_subprocess/timeout", test_subprocess_timeout);
  g_test_add_func ("/trap_subprocess/envp", test_subprocess_envp);
  g_test_add_func ("/trap_subprocess/stdin", test_subprocess_stdin);

  g_test_add_func ("/trap_subprocess/patterns", test_subprocess_patterns);

  g_test_add_func ("/misc/fatal-log-handler", test_fatal_log_handler);
  g_test_add_func ("/misc/fatal-log-handler/subprocess/critical-pass", test_fatal_log_handler_critical_pass);
  g_test_add_func ("/misc/fatal-log-handler/subprocess/error-fail", test_fatal_log_handler_error_fail);
  g_test_add_func ("/misc/fatal-log-handler/subprocess/critical-fail", test_fatal_log_handler_critical_fail);

  g_test_add_func ("/misc/expected-messages", test_expected_messages);
  g_test_add_func ("/misc/expected-messages/subprocess/warning", test_expected_messages_warning);
  g_test_add_func ("/misc/expected-messages/subprocess/expect-warning", test_expected_messages_expect_warning);
  g_test_add_func ("/misc/expected-messages/subprocess/wrong-warning", test_expected_messages_wrong_warning);
  g_test_add_func ("/misc/expected-messages/subprocess/expected", test_expected_messages_expected);
  g_test_add_func ("/misc/expected-messages/subprocess/null-domain", test_expected_messages_null_domain);
  g_test_add_func ("/misc/expected-messages/subprocess/extra-warning", test_expected_messages_extra_warning);
  g_test_add_func ("/misc/expected-messages/subprocess/unexpected-extra-warning", test_expected_messages_unexpected_extra_warning);
  g_test_add_func ("/misc/expected-messages/expect-error", test_expected_messages_expect_error);
  g_test_add_func ("/misc/expected-messages/skip-debug", test_expected_messages_debug);

  g_test_add_func ("/misc/messages", test_messages);
  g_test_add_func ("/misc/messages/subprocess/use-stderr", test_messages_use_stderr);

  g_test_add_func ("/misc/dash-p", test_dash_p);
  g_test_add_func ("/misc/dash-p/child", test_dash_p_child);
  g_test_add_func ("/misc/dash-p/child/sub", test_dash_p_child_sub);
  g_test_add_func ("/misc/dash-p/child/sub/subprocess", test_dash_p_child_sub_child);
  g_test_add_func ("/misc/dash-p/child/sub/subprocess/child", test_dash_p_child_sub_child);
  g_test_add_func ("/misc/dash-p/child/sub2", test_dash_p_child_sub2);
  g_test_add_func ("/misc/dash-p/subprocess/hidden", test_dash_p_hidden);
  g_test_add_func ("/misc/dash-p/subprocess/hidden/sub", test_dash_p_hidden_sub);

  g_test_add_func ("/misc/nonfatal", test_nonfatal);

  g_test_add_func ("/misc/skip", test_skip);
  g_test_add_func ("/misc/combining", test_combining);
  g_test_add_func ("/misc/combining/subprocess/fail", subprocess_fail);
  g_test_add_func ("/misc/combining/subprocess/skip1", test_skip);
  g_test_add_func ("/misc/combining/subprocess/skip2", test_skip);
  g_test_add_func ("/misc/combining/subprocess/incomplete", subprocess_incomplete);
  g_test_add_func ("/misc/combining/subprocess/pass", test_pass);
  g_test_add_func ("/misc/fail", test_fail);
  g_test_add_func ("/misc/incomplete", test_incomplete);

  g_test_add_func ("/misc/path/first", test_path_first);
  g_test_add_func ("/misc/path/second", test_path_second);

  g_test_add_func ("/tap", test_tap);
  g_test_add_func ("/tap/subtest", test_tap_subtest);
  g_test_add_func ("/tap/summary", test_tap_summary);
  g_test_add_func ("/tap/subtest/summary", test_tap_subtest_summary);
  g_test_add_func ("/tap/message", test_tap_message);
  g_test_add_func ("/tap/subtest/message", test_tap_subtest_message);
  g_test_add_func ("/tap/print", test_tap_print);
  g_test_add_func ("/tap/subtest/print", test_tap_subtest_print);
  g_test_add_func ("/tap/subtest/stdout", test_tap_subtest_stdout);
  g_test_add_func ("/tap/subtest/stdout-no-new-line", test_tap_subtest_stdout_no_new_line);
  g_test_add_func ("/tap/error", test_tap_error);
  g_test_add_func ("/tap/subtest/error", test_tap_subtest_error);
  g_test_add_func ("/tap/error-and-pass", test_tap_error_and_pass);
  g_test_add_func ("/tap/subtest/error-and-pass", test_tap_subtest_error_and_pass);

  g_test_add_func ("/init/no_argv0", test_init_no_argv0);

  ret = g_test_run ();

  /* We can't test for https://gitlab.gnome.org/GNOME/glib/-/issues/2563
   * from a test-case, because the whole point of that issue is that it's
   * about whether certain patterns are valid after g_test_run() has
   * returned... so put an ad-hoc test here, and just crash if it fails. */
  filename2 = g_test_build_filename (G_TEST_BUILT, "nonexistent", NULL);
  g_assert_cmpstr (filename, ==, filename2);

  g_free (filename);
  g_free (filename2);
  return ret;
}
