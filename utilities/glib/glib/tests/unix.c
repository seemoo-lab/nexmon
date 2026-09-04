/* 
 * Copyright (C) 2011 Red Hat, Inc.
 * Copyright 2023 Collabora Ltd.
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
 *
 * Author: Colin Walters <walters@verbum.org> 
 */

#include "config.h"

#include "glib-private.h"
#include "glib-unix.h"
#include "gstdio.h"
#include "gvalgrind.h"

#include <signal.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

#include "testutils.h"

static void
async_signal_safe_message (const char *message)
{
  if (write (2, message, strlen (message)) < 0 ||
      write (2, "\n", 1) < 0)
    {
      /* ignore: not much we can do */
    }
}

static void
test_closefrom (void)
{
  /* Enough file descriptors to be confident that we're operating on
   * all of them */
  const int N_FDS = 20;
  int *fds;
  int fd;
  int i;
  pid_t child;
  int wait_status;

  /* The loop that populates @fds with pipes assumes this */
  g_assert (N_FDS % 2 == 0);

  g_test_summary ("Test g_closefrom(), g_fdwalk_set_cloexec()");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3247");

  for (fd = 0; fd <= 2; fd++)
    {
      int flags;

      g_assert_no_errno ((flags = fcntl (fd, F_GETFD)));
      g_assert_no_errno (fcntl (fd, F_SETFD, flags & ~FD_CLOEXEC));
    }

  fds = g_new0 (int, N_FDS);

  for (i = 0; i < N_FDS; i += 2)
    {
      GError *error = NULL;
      int pipefd[2];
      int res;

      /* Intentionally neither O_CLOEXEC nor FD_CLOEXEC */
      res = g_unix_open_pipe (pipefd, 0, &error);
      g_assert (res);
      g_assert_no_error (error);
      g_clear_error (&error);
      fds[i] = pipefd[0];
      fds[i + 1] = pipefd[1];
    }

  child = fork ();

  /* Child process exits with status = 100 + the first wrong fd,
   * or 0 if all were correct */
  if (child == 0)
    {
      for (i = 0; i < N_FDS; i++)
        {
          int flags = fcntl (fds[i], F_GETFD);

          if (flags == -1)
            {
              int exit_code = 100 + fds[i];
              async_signal_safe_message ("fd should not have been closed");
              g_free (fds);
              _exit (exit_code);
            }

          if (flags & FD_CLOEXEC)
            {
              int exit_code = 100 + fds[i];
              async_signal_safe_message ("fd should not have been close-on-exec yet");
              g_free (fds);
              _exit (exit_code);
            }
        }

      g_fdwalk_set_cloexec (3);

      for (i = 0; i < N_FDS; i++)
        {
          int flags = fcntl (fds[i], F_GETFD);

          if (flags == -1)
            {
              int exit_code = 100 + fds[i];
              async_signal_safe_message ("fd should not have been closed");
              g_free (fds);
              _exit (exit_code);
            }

          if (!(flags & FD_CLOEXEC))
            {
              int exit_code = 100 + fds[i];
              async_signal_safe_message ("fd should have been close-on-exec");
              g_free (fds);
              _exit (exit_code);
            }
        }

      g_closefrom (3);

      for (fd = 0; fd <= 2; fd++)
        {
          int flags = fcntl (fd, F_GETFD);

          if (flags == -1)
            {
              async_signal_safe_message ("fd should not have been closed");
              g_free (fds);
              _exit (100 + fd);
            }

          if (flags & FD_CLOEXEC)
            {
              async_signal_safe_message ("fd should not have been close-on-exec");
              g_free (fds);
              _exit (100 + fd);
            }
        }

      for (i = 0; i < N_FDS; i++)
        {
          if (fcntl (fds[i], F_GETFD) != -1 || errno != EBADF)
            {
              async_signal_safe_message ("fd should have been closed");
              g_free (fds);
              _exit (100 + fds[i]);
            }
        }

      g_free (fds);
      _exit (0);
    }

  g_assert_no_errno (waitpid (child, &wait_status, 0));

  if (WIFEXITED (wait_status))
    {
      int exit_status = WEXITSTATUS (wait_status);

      if (exit_status != 0)
        g_test_fail_printf ("File descriptor %d in incorrect state", exit_status - 100);
    }
  else
    {
      g_test_fail_printf ("Unexpected wait status %d", wait_status);
    }

  for (i = 0; i < N_FDS; i++)
    {
      GError *error = NULL;

      g_close (fds[i], &error);
      g_assert_no_error (error);
      g_clear_error (&error);
    }

  g_free (fds);

  if (g_test_undefined ())
    {
      g_test_trap_subprocess ("/glib-unix/closefrom/subprocess/einval",
                              0, G_TEST_SUBPROCESS_DEFAULT);
      g_test_trap_assert_passed ();
    }
}

static void
test_closefrom_subprocess_einval (void)
{
  int res;
  int errsv;

  g_log_set_always_fatal (G_LOG_FATAL_MASK);
  g_log_set_fatal_mask ("GLib", G_LOG_FATAL_MASK);

  errno = 0;
  res = g_closefrom (-1);
  errsv = errno;
  g_assert_cmpint (res, ==, -1);
  g_assert_cmpint (errsv, ==, EINVAL);

  errno = 0;
  res = g_fdwalk_set_cloexec (-42);
  errsv = errno;
  g_assert_cmpint (res, ==, -1);
  g_assert_cmpint (errsv, ==, EINVAL);
}

static void
test_pipe (void)
{
  GError *error = NULL;
  int pipefd[2];
  char buf[1024];
  gssize bytes_read;
  gboolean res;

  res = g_unix_open_pipe (pipefd, O_CLOEXEC, &error);
  g_assert (res);
  g_assert_no_error (error);

  g_assert_cmpint (write (pipefd[1], "hello", sizeof ("hello")), ==, sizeof ("hello"));
  memset (buf, 0, sizeof (buf));
  bytes_read = read (pipefd[0], buf, sizeof(buf) - 1);
  g_assert_cmpint (bytes_read, >, 0);
  buf[bytes_read] = '\0';

  close (pipefd[0]);
  close (pipefd[1]);

  g_assert (g_str_has_prefix (buf, "hello"));
}

static void
test_pipe_fd_cloexec (void)
{
  GError *error = NULL;
  int pipefd[2];
  char buf[1024];
  gssize bytes_read;
  gboolean res;

  g_test_summary ("Test that FD_CLOEXEC is still accepted as an argument to g_unix_open_pipe()");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/merge_requests/3459");

  res = g_unix_open_pipe (pipefd, FD_CLOEXEC, &error);
  g_assert (res);
  g_assert_no_error (error);

  g_assert_cmpint (write (pipefd[1], "hello", sizeof ("hello")), ==, sizeof ("hello"));
  memset (buf, 0, sizeof (buf));
  bytes_read = read (pipefd[0], buf, sizeof(buf) - 1);
  g_assert_cmpint (bytes_read, >, 0);
  buf[bytes_read] = '\0';

  close (pipefd[0]);
  close (pipefd[1]);

  g_assert_true (g_str_has_prefix (buf, "hello"));
}

static void
test_pipe_stdio_overwrite (void)
{
  GError *error = NULL;
  int pipefd[2], ret;
  gboolean res;
  int stdin_fd;


  g_test_summary ("Test that g_unix_open_pipe() will use the first available FD, even if it’s stdin/stdout/stderr");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2795");

  stdin_fd = dup (STDIN_FILENO);
  g_assert_cmpint (stdin_fd, >, 0);

  g_close (STDIN_FILENO, &error);
  g_assert_no_error (error);

  res = g_unix_open_pipe (pipefd, O_CLOEXEC, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  g_assert_cmpint (pipefd[0], ==, STDIN_FILENO);

  g_close (pipefd[0], &error);
  g_assert_no_error (error);

  g_close (pipefd[1], &error);
  g_assert_no_error (error);

  ret = dup2 (stdin_fd, STDIN_FILENO);
  g_assert_cmpint (ret, >=, 0);

  g_close (stdin_fd, &error);
  g_assert_no_error (error);
}

static void
test_pipe_struct (void)
{
  GError *error = NULL;
  GUnixPipe pair = G_UNIX_PIPE_INIT;
  char buf[1024];
  gssize bytes_read;
  gboolean res;
  int read_end = -1;    /* owned */
  int write_end = -1;   /* unowned */
  int errsv;

  g_test_summary ("Test GUnixPipe structure");

  res = g_unix_pipe_open (&pair, O_CLOEXEC, &error);
  g_assert_no_error (error);
  g_assert_true (res);

  read_end = g_unix_pipe_steal (&pair, G_UNIX_PIPE_END_READ);
  g_assert_cmpint (read_end, >=, 0);
  g_assert_cmpint (g_unix_pipe_steal (&pair, G_UNIX_PIPE_END_READ), ==, -1);
  g_assert_cmpint (g_unix_pipe_get (&pair, G_UNIX_PIPE_END_READ), ==, -1);
  write_end = g_unix_pipe_get (&pair, G_UNIX_PIPE_END_WRITE);
  g_assert_cmpint (write_end, >=, 0);
  g_assert_cmpint (g_unix_pipe_get (&pair, G_UNIX_PIPE_END_WRITE), ==, write_end);

  g_assert_cmpint (write (write_end, "hello", sizeof ("hello")), ==, sizeof ("hello"));
  memset (buf, 0, sizeof (buf));
  bytes_read = read (read_end, buf, sizeof(buf) - 1);
  g_assert_cmpint (bytes_read, ==, sizeof ("hello"));
  buf[bytes_read] = '\0';

  /* This is one of the few errno values guaranteed by Standard C.
   * We set it here to check that g_unix_pipe_clear doesn't alter errno. */
  errno = EILSEQ;

  g_unix_pipe_clear (&pair);
  errsv = errno;
  g_assert_cmpint (errsv, ==, EILSEQ);

  g_assert_cmpint (pair.fds[0], ==, -1);
  g_assert_cmpint (pair.fds[1], ==, -1);

  /* The read end wasn't closed, because it was stolen first */
  g_clear_fd (&read_end, &error);
  g_assert_no_error (error);

  /* The write end was closed, because it wasn't stolen */
  assert_fd_was_closed (write_end);

  g_assert_cmpstr (buf, ==, "hello");
}

static void
test_pipe_struct_auto (void)
{
#ifdef g_autofree
  int i;

  g_test_summary ("Test g_auto(GUnixPipe)");

  /* Let g_auto close the read end, the write end, neither, or both */
  for (i = 0; i < 4; i++)
    {
      int read_end = -1;    /* unowned */
      int write_end = -1;   /* unowned */
      int errsv;

        {
          g_auto(GUnixPipe) pair = G_UNIX_PIPE_INIT;
          GError *error = NULL;
          gboolean res;

          res = g_unix_pipe_open (&pair, O_CLOEXEC, &error);
          g_assert_no_error (error);
          g_assert_true (res);

          read_end = pair.fds[G_UNIX_PIPE_END_READ];
          g_assert_cmpint (read_end, >=, 0);
          write_end = pair.fds[G_UNIX_PIPE_END_WRITE];
          g_assert_cmpint (write_end, >=, 0);

          if (i & 1)
            {
              /* This also exercises g_unix_pipe_close() with error */
              res = g_unix_pipe_close (&pair, G_UNIX_PIPE_END_READ, &error);
              g_assert_no_error (error);
              g_assert_true (res);
            }

          /* This also exercises g_unix_pipe_close() without error */
          if (i & 2)
            g_unix_pipe_close (&pair, G_UNIX_PIPE_END_WRITE, NULL);

          /* This is one of the few errno values guaranteed by Standard C.
           * We set it here to check that a g_auto(GUnixPipe) close doesn't
           * alter errno. */
          errno = EILSEQ;
        }

      errsv = errno;
      g_assert_cmpint (errsv, ==, EILSEQ);
      assert_fd_was_closed (read_end);
      assert_fd_was_closed (write_end);
    }
#else
  g_test_skip ("g_auto not supported by compiler");
#endif
}

static void
test_error (void)
{
  GError *error = NULL;
  gboolean res;

  res = g_unix_set_fd_nonblocking (123456, TRUE, &error);
  g_assert_cmpint (errno, ==, EBADF);
  g_assert (!res);
  g_assert_error (error, G_UNIX_ERROR, 0);
  g_clear_error (&error);
}

static void
test_nonblocking (void)
{
  GError *error = NULL;
  int pipefd[2];
  gboolean res;
  int flags;

  res = g_unix_open_pipe (pipefd, O_CLOEXEC, &error);
  g_assert (res);
  g_assert_no_error (error);

  res = g_unix_set_fd_nonblocking (pipefd[0], TRUE, &error);
  g_assert (res);
  g_assert_no_error (error);
 
  flags = fcntl (pipefd[0], F_GETFL);
  g_assert_cmpint (flags, !=, -1);
  g_assert (flags & O_NONBLOCK);

  res = g_unix_set_fd_nonblocking (pipefd[0], FALSE, &error);
  g_assert (res);
  g_assert_no_error (error);
 
  flags = fcntl (pipefd[0], F_GETFL);
  g_assert_cmpint (flags, !=, -1);
  g_assert (!(flags & O_NONBLOCK));

  close (pipefd[0]);
  close (pipefd[1]);
}

static gboolean sig_received = FALSE;
static gboolean sig_timeout = FALSE;
static int sig_counter = 0;

static gboolean
on_sig_received (gpointer user_data)
{
  GMainLoop *loop = user_data;
  g_main_loop_quit (loop);
  sig_received = TRUE;
  sig_counter ++;
  return G_SOURCE_REMOVE;
}

static gboolean
on_sig_timeout (gpointer data)
{
  GMainLoop *loop = data;
  g_main_loop_quit (loop);
  sig_timeout = TRUE;
  return G_SOURCE_REMOVE;
}

static gboolean
exit_mainloop (gpointer data)
{
  GMainLoop *loop = data;
  g_main_loop_quit (loop);
  return G_SOURCE_REMOVE;
}

static gboolean
on_sig_received_2 (gpointer data)
{
  GMainLoop *loop = data;

  sig_counter ++;
  if (sig_counter == 2)
    g_main_loop_quit (loop);
  return G_SOURCE_REMOVE;
}

static void
test_signal (int signum)
{
  struct sigaction action;
  GMainLoop *mainloop;
  int id;

  mainloop = g_main_loop_new (NULL, FALSE);

  sig_received = FALSE;
  sig_counter = 0;
  g_unix_signal_add (signum, on_sig_received, mainloop);

  g_assert_no_errno (sigaction (signum, NULL, &action));

  if (signum == SIGCHLD)
    g_assert_true (action.sa_flags & SA_NOCLDSTOP);

#ifdef SA_RESTART
  g_assert_true (action.sa_flags & SA_RESTART);
#endif
#ifdef SA_ONSTACK
  g_assert_true (action.sa_flags & SA_ONSTACK);
#endif

  kill (getpid (), signum);
  g_assert (!sig_received);
  id = g_timeout_add (5000, on_sig_timeout, mainloop);
  g_main_loop_run (mainloop);
  g_assert (sig_received);
  sig_received = FALSE;
  g_source_remove (id);

  /* Ensure we don't get double delivery */
  g_timeout_add (500, exit_mainloop, mainloop);
  g_main_loop_run (mainloop);
  g_assert (!sig_received);

  /* Ensure that two sources for the same signal get it */
  sig_counter = 0;
  g_unix_signal_add (signum, on_sig_received_2, mainloop);
  g_unix_signal_add (signum, on_sig_received_2, mainloop);
  id = g_timeout_add (5000, on_sig_timeout, mainloop);

  kill (getpid (), signum);
  g_main_loop_run (mainloop);
  g_assert_cmpint (sig_counter, ==, 2);
  g_source_remove (id);

  g_main_loop_unref (mainloop);
}

static void
test_sighup (void)
{
  test_signal (SIGHUP);
}

static void
test_sigterm (void)
{
  test_signal (SIGTERM);
}

static void
test_signal_alternate_stack (int signal)
{
#ifndef SA_ONSTACK
  g_test_skip ("alternate stack is not supported");
#else
  size_t minsigstksz = MINSIGSTKSZ;
  guint8 *stack_memory = NULL;
  guint8 *zero_mem = NULL;
  stack_t stack = { 0 };
  stack_t old_stack = { 0 };

#ifdef _SC_MINSIGSTKSZ
  /* Use the kernel-provided minimum stack size, if available. Otherwise default
   * to MINSIGSTKSZ. Unfortunately that might not be big enough for huge
   * register files for big CPU instruction set extensions. */
  minsigstksz = sysconf (_SC_MINSIGSTKSZ);
  if (minsigstksz == (size_t) -1 || minsigstksz < (size_t) MINSIGSTKSZ)
    minsigstksz = MINSIGSTKSZ;
#endif

  stack_memory = g_malloc0 (minsigstksz);
  zero_mem = g_malloc0 (minsigstksz);
  g_assert_cmpmem (stack_memory, minsigstksz, zero_mem, minsigstksz);

  stack.ss_sp = stack_memory;
  stack.ss_size = minsigstksz;

  g_assert_no_errno (sigaltstack (&stack, &old_stack));

  test_signal (signal);

#if defined (ENABLE_VALGRIND)
  if (RUNNING_ON_VALGRIND)
    {
      /* When running under valgrind, checking for memory differences does
       * not work with a weird read error happening way before than the
       * stack memory size, it's unclear why but it may be related to how
       * valgrind internally implements it.
       * However, the point of the test is to make sure that even using an
       * alternative stack (that we blindly trust is used), the signals are
       * properly delivered, and this can be still tested properly.
       *
       * See:
       *  - https://gitlab.gnome.org/GNOME/glib/-/issues/3337
       *  - https://bugs.kde.org/show_bug.cgi?id=486812
       */
      g_test_message ("Running a limited test version under valgrind");

      stack.ss_flags = SS_DISABLE;
      g_assert_no_errno (sigaltstack (&stack, &old_stack));

      g_free (zero_mem);
      g_free (stack_memory);

      return;
    }
#endif

  /* Very stupid check to ensure that the alternate stack is used instead of
   * the default one. This test would fail if SA_ONSTACK wouldn't be set.
   */
  g_assert_cmpint (memcmp (stack_memory, zero_mem, minsigstksz), !=, 0);

  /* We need to memset again zero_mem since compiler may have optimized it out
   * as we've seen in freebsd CI.
   */
  memset (zero_mem, 0, minsigstksz);
  memset (stack_memory, 0, minsigstksz);
  g_assert_cmpmem (stack_memory, minsigstksz, zero_mem, minsigstksz);

  stack.ss_flags = SS_DISABLE;
  g_assert_no_errno (sigaltstack (&stack, &old_stack));

  test_signal (signal);
  g_assert_cmpmem (stack_memory, minsigstksz, zero_mem, minsigstksz);

  g_free (zero_mem);
  g_free (stack_memory);
#endif
}

static void
test_sighup_alternate_stack (void)
{
  test_signal_alternate_stack (SIGHUP);
}

static void
test_sigterm_alternate_stack (void)
{
  test_signal_alternate_stack (SIGTERM);
}

static void
test_sighup_add_remove (void)
{
  guint id;
  struct sigaction action;

  sig_received = FALSE;
  id = g_unix_signal_add (SIGHUP, on_sig_received, NULL);
  g_source_remove (id);

  sigaction (SIGHUP, NULL, &action);
  g_assert (action.sa_handler == SIG_DFL);
}

static gboolean
nested_idle (gpointer data)
{
  GMainLoop *nested;
  GMainContext *context;
  GSource *source;

  context = g_main_context_new ();
  nested = g_main_loop_new (context, FALSE);

  source = g_unix_signal_source_new (SIGHUP);
  g_source_set_callback (source, on_sig_received, nested, NULL);
  g_source_attach (source, context);
  g_source_unref (source);

  kill (getpid (), SIGHUP);
  g_main_loop_run (nested);
  g_assert_cmpint (sig_counter, ==, 1);

  g_main_loop_unref (nested);
  g_main_context_unref (context);

  return G_SOURCE_REMOVE;
}

static void
test_sighup_nested (void)
{
  GMainLoop *mainloop;

  mainloop = g_main_loop_new (NULL, FALSE);

  sig_counter = 0;
  sig_received = FALSE;
  g_unix_signal_add (SIGHUP, on_sig_received, mainloop);
  g_idle_add (nested_idle, mainloop);

  g_main_loop_run (mainloop);
  g_assert_cmpint (sig_counter, ==, 2);

  g_main_loop_unref (mainloop);
}

static gboolean
on_sigwinch_received (gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  sig_counter ++;

  if (sig_counter == 1)
    kill (getpid (), SIGWINCH);
  else if (sig_counter == 2)
    g_main_loop_quit (loop);
  else if (sig_counter > 2)
    g_assert_not_reached ();

  /* Increase the time window in which an issue could happen. */
  g_usleep (G_USEC_PER_SEC);

  return G_SOURCE_CONTINUE;
}

static void
test_callback_after_signal (void)
{
  /* Checks that user signal callback is invoked *after* receiving a signal.
   * In other words a new signal is never merged with the one being currently
   * dispatched or whose dispatch had already finished. */

  GMainLoop *mainloop;
  GMainContext *context;
  GSource *source;

  sig_counter = 0;

  context = g_main_context_new ();
  mainloop = g_main_loop_new (context, FALSE);

  source = g_unix_signal_source_new (SIGWINCH);
  g_source_set_callback (source, on_sigwinch_received, mainloop, NULL);
  g_source_attach (source, context);
  g_source_unref (source);

  g_assert_cmpint (sig_counter, ==, 0);
  kill (getpid (), SIGWINCH);
  g_main_loop_run (mainloop);
  g_assert_cmpint (sig_counter, ==, 2);

  g_main_loop_unref (mainloop);
  g_main_context_unref (context);
}

static void
test_get_passwd_entry_root (void)
{
  struct passwd *pwd;
  GError *local_error = NULL;

  g_test_summary ("Tests that g_unix_get_passwd_entry() works for a "
                  "known-existing username.");

  pwd = g_unix_get_passwd_entry ("root", &local_error);
  g_assert_no_error (local_error);

  g_assert_cmpstr (pwd->pw_name, ==, "root");
  g_assert_cmpuint (pwd->pw_uid, ==, 0);

  g_free (pwd);
}

static void
test_get_passwd_entry_nonexistent (void)
{
  struct passwd *pwd;
  GError *local_error = NULL;

  g_test_summary ("Tests that g_unix_get_passwd_entry() returns an error for a "
                  "nonexistent username.");

  pwd = g_unix_get_passwd_entry ("thisusernamedoesntexist", &local_error);
  g_assert_error (local_error, G_UNIX_ERROR, 0);
  g_assert_null (pwd);

  g_clear_error (&local_error);
}

static void
_child_wait_watch_cb (GPid pid,
                      gint wait_status,
                      gpointer user_data)
{
  gboolean *p_got_callback = user_data;

  g_assert_nonnull (p_got_callback);
  g_assert_false (*p_got_callback);
  *p_got_callback = TRUE;
}

static void
test_child_wait (void)
{
  gboolean r;
  GPid pid;
  guint id;
  pid_t pid2;
  int wstatus;
  gboolean got_callback = FALSE;
  gboolean iterate_maincontext = g_test_rand_bit ();
  char **argv;
  int errsv;

  /* - We spawn a trivial child process that exits after a short time.
   * - We schedule a g_child_watch_add()
   * - we may iterate the GMainContext a bit. Randomly we either get the
   *   child-watcher callback or not.
   * - if we didn't get the callback, we g_source_remove() the child watcher.
   *
   * Afterwards, if the callback didn't fire, we check that we are able to waitpid()
   * on the process ourselves. Of course, if the child watcher notified, the waitpid()
   * will fail with ECHILD.
   */

  argv = g_test_rand_bit () ? ((char *[]){ "/bin/sleep", "0.05", NULL }) : ((char *[]){ "/bin/true", NULL });

  r = g_spawn_async (NULL,
                     argv,
                     NULL,
                     G_SPAWN_DO_NOT_REAP_CHILD,
                     NULL,
                     NULL,
                     &pid,
                     NULL);
  if (!r)
    {
      /* Some odd system without /bin/sleep? Skip the test. */
      g_test_skip ("failure to spawn test process in test_child_wait()");
      return;
    }

  g_assert_cmpint (pid, >=, 1);

  if (g_test_rand_bit ())
    g_usleep (g_test_rand_int_range (0, (G_USEC_PER_SEC / 10)));

  id = g_child_watch_add (pid, _child_wait_watch_cb, &got_callback);

  if (g_test_rand_bit ())
    g_usleep (g_test_rand_int_range (0, (G_USEC_PER_SEC / 10)));

  if (iterate_maincontext)
    {
      gint64 start_usec = g_get_monotonic_time ();
      gint64 end_usec = start_usec + g_test_rand_int_range (0, (G_USEC_PER_SEC / 10));

      while (!got_callback && g_get_monotonic_time () < end_usec)
        g_main_context_iteration (NULL, FALSE);
    }

  if (!got_callback)
    g_source_remove (id);

  errno = 0;
  pid2 = waitpid (pid, &wstatus, 0);
  errsv = errno;
  if (got_callback)
    {
      g_assert_true (iterate_maincontext);
      g_assert_cmpint (errsv, ==, ECHILD);
      g_assert_cmpint (pid2, <, 0);
    }
  else
    {
      g_assert_cmpint (errsv, ==, 0);
      g_assert_cmpint (pid2, ==, pid);
      g_assert_true (WIFEXITED (wstatus));
      g_assert_cmpint (WEXITSTATUS (wstatus), ==, 0);
    }
}

static void
test_fd_query_path (void)
{
  int fd;
  GError *error = NULL;
  char *fd_path;

#if defined (__GNU__)
  g_test_skip ("Querying file path from FD is not supported in GNU hurd");
  return;
#endif

  fd = g_open ("/dev/null", O_RDONLY);
  g_assert_no_errno (errno);
  g_assert_cmpint (fd, >=, 0);

  g_test_message ("Checking FD %d for /dev/null", fd);

  fd_path = g_unix_fd_query_path (fd, &error);
  g_assert_no_error (error);
#ifdef __sun
  /* /dev/null is a symlink on Solaris, so follow the link */
  char *dev_null_path = realpath ("/dev/null", NULL);
  g_assert_no_errno (errno);
  g_assert_nonnull (dev_null_path);
  g_assert_cmpstr (fd_path, ==, dev_null_path);
  g_free (dev_null_path);
#else
  g_assert_cmpstr (fd_path, ==, "/dev/null");
#endif

  g_clear_fd (&fd, &error);
  g_assert_no_error (error);
  g_clear_pointer (&fd_path, g_free);
}

static void
test_fd_query_path_error (void)
{
  GError *error = NULL;
  char *fd_path;

  fd_path = g_unix_fd_query_path (G_MAXINT, &error);
  g_assert_null (fd_path);
  g_assert_nonnull (error);
  g_assert_cmpint (error->domain, ==, G_FILE_ERROR);

#ifdef __linux__
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);
#endif

  g_clear_error (&error);
  g_clear_pointer (&fd_path, g_free);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/glib-unix/closefrom", test_closefrom);
  g_test_add_func ("/glib-unix/closefrom/subprocess/einval",
                   test_closefrom_subprocess_einval);
  g_test_add_func ("/glib-unix/pipe", test_pipe);
  g_test_add_func ("/glib-unix/pipe/fd-cloexec", test_pipe_fd_cloexec);
  g_test_add_func ("/glib-unix/pipe-stdio-overwrite", test_pipe_stdio_overwrite);
  g_test_add_func ("/glib-unix/pipe-struct", test_pipe_struct);
  g_test_add_func ("/glib-unix/pipe-struct-auto", test_pipe_struct_auto);
  g_test_add_func ("/glib-unix/error", test_error);
  g_test_add_func ("/glib-unix/nonblocking", test_nonblocking);
  g_test_add_func ("/glib-unix/sighup", test_sighup);
  g_test_add_func ("/glib-unix/sigterm", test_sigterm);
  g_test_add_func ("/glib-unix/sighup_again", test_sighup);
  g_test_add_func ("/glib-unix/sighup/alternate-stack", test_sighup_alternate_stack);
  g_test_add_func ("/glib-unix/sigterm/alternate-stack", test_sigterm_alternate_stack);
  g_test_add_func ("/glib-unix/sighup_again/alternate-stack", test_sighup_alternate_stack);
  g_test_add_func ("/glib-unix/sighup_add_remove", test_sighup_add_remove);
  g_test_add_func ("/glib-unix/sighup_nested", test_sighup_nested);
  g_test_add_func ("/glib-unix/callback_after_signal", test_callback_after_signal);
  g_test_add_func ("/glib-unix/get-passwd-entry/root", test_get_passwd_entry_root);
  g_test_add_func ("/glib-unix/get-passwd-entry/nonexistent", test_get_passwd_entry_nonexistent);
  g_test_add_func ("/glib-unix/child-wait", test_child_wait);
  g_test_add_func ("/glib-unix/fd-query-path", test_fd_query_path);
  g_test_add_func ("/glib-unix/fd-query-path-error", test_fd_query_path_error);

  return g_test_run();
}
