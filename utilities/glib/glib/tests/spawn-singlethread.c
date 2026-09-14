/* 
 * Copyright (C) 2011 Red Hat, Inc.
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

#include <glib.h>
#include <locale.h>
#include <string.h>
#include <fcntl.h>
#include <glib/gstdio.h>

#ifdef G_OS_UNIX
#include <glib-unix.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <winsock2.h>
#include <io.h>
#define LINEEND "\r\n"
#else
#define LINEEND "\n"
#endif

/* MinGW builds are likely done using a BASH-style shell, so run the
 * normal script there, as on non-Windows builds, as it is more likely
 * that one will run 'make check' in such shells to test the code
 */
#if defined (G_OS_WIN32) && defined (_MSC_VER)
#define SCRIPT_EXT ".bat"
#else
#define SCRIPT_EXT
#endif

static char *echo_prog_path;
static char *echo_script_path;

typedef struct {
  GMainLoop *loop;
  gboolean child_exited;
  gboolean stdout_done;
  GString *stdout_buf;
} SpawnAsyncMultithreadedData;

static gboolean
on_child_exited (GPid     pid,
		 gint     status,
		 gpointer datap)
{
  SpawnAsyncMultithreadedData *data = datap;

  data->child_exited = TRUE;
  if (data->child_exited && data->stdout_done)
    g_main_loop_quit (data->loop);
  
  return G_SOURCE_REMOVE;
}

static gboolean
on_child_stdout (GIOChannel   *channel,
		 GIOCondition  condition,
		 gpointer      datap)
{
  char buf[1024];
  GError *error = NULL;
  gsize bytes_read;
  SpawnAsyncMultithreadedData *data = datap;

  if (condition & G_IO_IN)
    {
      GIOStatus status;
      status = g_io_channel_read_chars (channel, buf, sizeof (buf), &bytes_read, &error);
      g_assert_no_error (error);
      g_string_append_len (data->stdout_buf, buf, (gssize) bytes_read);
      if (status == G_IO_STATUS_EOF)
	data->stdout_done = TRUE;
    }
  if (condition & G_IO_HUP)
    data->stdout_done = TRUE;
  if (condition & G_IO_ERR)
    g_error ("Error reading from child stdin");

  if (data->child_exited && data->stdout_done)
    g_main_loop_quit (data->loop);

  return !data->stdout_done;
}

static void
test_spawn_async (void)
{
  int tnum = 1;
  GError *error = NULL;
  GPtrArray *argv;
  char *arg;
  GPid pid;
  GMainContext *context;
  GMainLoop *loop;
  GIOChannel *channel;
  GSource *source;
  int child_stdout_fd;
  SpawnAsyncMultithreadedData data;

  context = g_main_context_new ();
  loop = g_main_loop_new (context, TRUE);

  arg = g_strdup_printf ("thread %d", tnum);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, echo_prog_path);
  g_ptr_array_add (argv, arg);
  g_ptr_array_add (argv, NULL);

  g_spawn_async_with_pipes (NULL, (char**)argv->pdata, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL,
			    &child_stdout_fd, NULL, &error);
  g_assert_no_error (error);
  g_ptr_array_free (argv, TRUE);

  data.loop = loop;
  data.stdout_done = FALSE;
  data.child_exited = FALSE;
  data.stdout_buf = g_string_new (0);

  source = g_child_watch_source_new (pid);
  g_source_set_callback (source, (GSourceFunc)on_child_exited, &data, NULL);
  g_source_attach (source, context);
  g_source_unref (source);

  channel = g_io_channel_unix_new (child_stdout_fd);
  source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR);
  g_source_set_callback (source, (GSourceFunc)on_child_stdout, &data, NULL);
  g_source_attach (source, context);
  g_source_unref (source);

  g_main_loop_run (loop);

  g_assert (data.child_exited);
  g_assert (data.stdout_done);
  g_assert_cmpstr (data.stdout_buf->str, ==, arg);
  g_string_free (data.stdout_buf, TRUE);

  g_io_channel_unref (channel);
  g_main_context_unref (context);
  g_main_loop_unref (loop);

  g_free (arg);
}

/* Windows close() causes failure through the Invalid Parameter Handler
 * Routine if the file descriptor does not exist.
 */
static void
safe_close (int fd)
{
  if (fd >= 0)
    close (fd);
}

/* Test g_spawn_async_with_fds() with a variety of different inputs */
static void
test_spawn_async_with_fds (void)
{
  int tnum = 1;
  GPtrArray *argv;
  char *arg;
  gsize i;

  /* Each test has 3 variable parameters: stdin, stdout, stderr */
  enum fd_type {
    NO_FD,        /* pass fd -1 (unset) */
    FD_NEGATIVE,  /* pass fd of negative value (equivalent to unset) */
    PIPE,         /* pass fd of new/unique pipe */
    STDOUT_PIPE,  /* pass the same pipe as stdout */
  } tests[][3] = {
    { NO_FD, NO_FD, NO_FD },       /* Test with no fds passed */
    { NO_FD, FD_NEGATIVE, NO_FD }, /* Test another negative fd value */
    { PIPE, PIPE, PIPE },          /* Test with unique fds passed */
    { NO_FD, PIPE, STDOUT_PIPE },  /* Test the same fd for stdout + stderr */
  };

  arg = g_strdup_printf ("# thread %d\n", tnum);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, echo_prog_path);
  g_ptr_array_add (argv, arg);
  g_ptr_array_add (argv, NULL);

  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      GError *error = NULL;
      GPid pid;
      GMainContext *context;
      GMainLoop *loop;
      GIOChannel *channel = NULL;
      GSource *source;
      SpawnAsyncMultithreadedData data;
      enum fd_type *fd_info = tests[i];
      gint test_pipe[3][2];
      int j;

      for (j = 0; j < 3; j++)
        {
          switch (fd_info[j])
            {
            case NO_FD:
              test_pipe[j][0] = -1;
              test_pipe[j][1] = -1;
              break;
            case FD_NEGATIVE:
              test_pipe[j][0] = -5;
              test_pipe[j][1] = -5;
              break;
            case PIPE:
#ifdef G_OS_UNIX
              g_unix_open_pipe (test_pipe[j], O_CLOEXEC, &error);
              g_assert_no_error (error);
#else
              g_assert_cmpint (_pipe (test_pipe[j], 4096, _O_BINARY), >=, 0);
#endif
              break;
            case STDOUT_PIPE:
              g_assert_cmpint (j, ==, 2); /* only works for stderr */
              test_pipe[j][0] = test_pipe[1][0];
              test_pipe[j][1] = test_pipe[1][1];
              break;
            default:
              g_assert_not_reached ();
            }
        }

      context = g_main_context_new ();
      loop = g_main_loop_new (context, TRUE);

      g_spawn_async_with_fds (NULL, (char**)argv->pdata, NULL,
			      G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid,
			      test_pipe[0][0], test_pipe[1][1], test_pipe[2][1],
			      &error);
      g_assert_no_error (error);
      safe_close (test_pipe[0][0]);
      safe_close (test_pipe[1][1]);
      if (fd_info[2] != STDOUT_PIPE)
        safe_close (test_pipe[2][1]);

      data.loop = loop;
      data.stdout_done = FALSE;
      data.child_exited = FALSE;
      data.stdout_buf = g_string_new (0);

      source = g_child_watch_source_new (pid);
      g_source_set_callback (source, (GSourceFunc)on_child_exited, &data, NULL);
      g_source_attach (source, context);
      g_source_unref (source);

      if (test_pipe[1][0] >= 0)
        {
          channel = g_io_channel_unix_new (test_pipe[1][0]);
          source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR);
          g_source_set_callback (source, (GSourceFunc)on_child_stdout,
                                 &data, NULL);
          g_source_attach (source, context);
          g_source_unref (source);
        }
      else
        {
          /* Don't check stdout data if we didn't pass a fd */
          data.stdout_done = TRUE;
        }

      g_main_loop_run (loop);

      g_assert_true (data.child_exited);

      if (test_pipe[1][0] >= 0)
        {
          gchar *tmp = g_strdup_printf ("# thread %d" LINEEND, tnum);
          /* Check for echo on stdout */
          g_assert_true (data.stdout_done);
          g_assert_cmpstr (data.stdout_buf->str, ==, tmp);
          g_io_channel_unref (channel);
          g_free (tmp);
        }
      g_string_free (data.stdout_buf, TRUE);

      g_main_context_unref (context);
      g_main_loop_unref (loop);
      safe_close (test_pipe[0][1]);
      safe_close (test_pipe[1][0]);
      if (fd_info[2] != STDOUT_PIPE)
        safe_close (test_pipe[2][0]);
    }

  g_ptr_array_free (argv, TRUE);
  g_free (arg);
}

static void
test_spawn_async_with_invalid_fds (void)
{
  const gchar *argv[] = { echo_prog_path, "thread 0", NULL };
  gint source_fds[1000];
  GError *local_error = NULL;
  gboolean retval;
  gsize i;

  /* Create an identity mapping from [0, …, 999]. This is very likely going to
   * conflict with the internal FDs, as it covers a lot of the FD space
   * (including stdin, stdout and stderr, though we don’t care about them in
   * this test).
   *
   * Skip the test if we somehow avoid a collision. */
  for (i = 0; i < G_N_ELEMENTS (source_fds); i++)
    source_fds[i] = i;

  retval = g_spawn_async_with_pipes_and_fds (NULL, argv, NULL, G_SPAWN_DEFAULT,
                                             NULL, NULL, -1, -1, -1,
                                             source_fds, source_fds, G_N_ELEMENTS (source_fds),
                                             NULL, NULL, NULL, NULL,
                                             &local_error);
  if (retval)
    {
      g_test_skip ("Skipping internal FDs check as test didn’t manage to trigger a collision");
      return;
    }
  g_assert_false (retval);
  g_assert_error (local_error, G_SPAWN_ERROR, G_SPAWN_ERROR_INVAL);
  g_error_free (local_error);
}

static void
test_spawn_sync (void)
{
  int tnum = 1;
  GError *error = NULL;
  char *arg = g_strdup_printf ("thread %d", tnum);
  /* Include arguments with special symbols to test that they are correctly passed to child.
   * This is tested on all platforms, but the most prone to failure is win32,
   * where args are specially escaped during spawning.
   */
  const char * const argv[] = {
    echo_prog_path,
    arg,
    "doublequotes\\\"after\\\\\"\"backslashes", /* this would be special escaped on win32 */
    "\\\"\"doublequotes spaced after backslashes\\\\\"", /* this would be special escaped on win32 */
    "even$$dollars",
    "even%%percents",
    "even\"\"doublequotes",
    "even''singlequotes",
    "even\\\\backslashes",
    "even//slashes",
    "$odd spaced$dollars$",
    "%odd spaced%spercents%",
    "\"odd spaced\"doublequotes\"",
    "'odd spaced'singlequotes'",
    "\\odd spaced\\backslashes\\", /* this wasn't handled correctly on win32 in glib <=2.58 */
    "/odd spaced/slashes/",
    NULL
  };
  char *joined_args_str = g_strjoinv ("", (char**)argv + 1);
  char *stdout_str;
  int estatus;

  g_spawn_sync (NULL, (char**)argv, NULL, 0, NULL, NULL, &stdout_str, NULL, &estatus, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (joined_args_str, ==, stdout_str);
  g_free (arg);
  g_free (stdout_str);
  g_free (joined_args_str);
}

static void
init_networking (void)
{
#ifdef G_OS_WIN32
  WSADATA wsadata;

  if (WSAStartup (MAKEWORD (2, 0), &wsadata) != 0)
    g_error ("Windows Sockets could not be initialized");
#endif
}

static void
test_spawn_stderr_socket (void)
{
  GError *error = NULL;
  GPtrArray *argv;
  int estatus;
  int fd;

  g_test_summary ("Test calling g_spawn_sync() with its stderr FD set to a socket");

  if (g_test_subprocess ())
    {
      init_networking ();
      fd = socket (AF_INET, SOCK_STREAM, 0);
      g_assert_cmpint (fd, >=, 0);
#ifdef G_OS_WIN32
      fd = _open_osfhandle (fd, 0);
      g_assert_cmpint (fd, >=, 0);
#endif
      /* Set the socket as FD 2, stderr */
      estatus = dup2 (fd, 2);
      g_assert_cmpint (estatus, >=, 0);

      argv = g_ptr_array_new ();
      g_ptr_array_add (argv, echo_script_path);
      g_ptr_array_add (argv, NULL);

      g_spawn_sync (NULL, (char**) argv->pdata, NULL, 0, NULL, NULL, NULL, NULL, NULL, &error);
      g_assert_no_error (error);
      g_ptr_array_free (argv, TRUE);
      g_close (fd, &error);
      g_assert_no_error (error);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
}

/* Like test_spawn_sync but uses spawn flags that trigger the optimized
 * posix_spawn codepath.
 */
static void
test_posix_spawn (void)
{
  int tnum = 1;
  GError *error = NULL;
  GPtrArray *argv;
  char *arg;
  char *stdout_str;
  int estatus;
  GSpawnFlags flags = G_SPAWN_CLOEXEC_PIPES | G_SPAWN_LEAVE_DESCRIPTORS_OPEN;

  arg = g_strdup_printf ("thread %d", tnum);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, echo_prog_path);
  g_ptr_array_add (argv, arg);
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char**)argv->pdata, NULL, flags, NULL, NULL, &stdout_str, NULL, &estatus, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (arg, ==, stdout_str);
  g_free (arg);
  g_free (stdout_str);
  g_ptr_array_free (argv, TRUE);
}

static void
test_spawn_script (void)
{
  GError *error = NULL;
  GPtrArray *argv;
  char *stdout_str;
  int estatus;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, echo_script_path);
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char**)argv->pdata, NULL, 0, NULL, NULL, &stdout_str, NULL, &estatus, &error);
  g_assert_no_error (error);
  g_assert_cmpstr ("echo" LINEEND, ==, stdout_str);
  g_free (stdout_str);
  g_ptr_array_free (argv, TRUE);
}

/* Test that spawning a non-existent executable returns %G_SPAWN_ERROR_NOENT. */
static void
test_spawn_nonexistent (void)
{
  GError *error = NULL;
  GPtrArray *argv = NULL;
  gchar *stdout_str = NULL;
  gint wait_status = -1;

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, "this does not exist");
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char**) argv->pdata, NULL, 0, NULL, NULL, &stdout_str,
                NULL, &wait_status, &error);
  g_assert_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);
  g_assert_null (stdout_str);
  g_assert_cmpint (wait_status, ==, -1);

  g_ptr_array_free (argv, TRUE);

  g_clear_error (&error);
}

/* Test that FD assignments in a spawned process don’t overwrite and break the
 * child_err_report_fd which is used to report error information back from the
 * intermediate child process to the parent.
 *
 * https://gitlab.gnome.org/GNOME/glib/-/issues/2097 */
static void
test_spawn_fd_assignment_clash (void)
{
  int tmp_fd;
  guint i;
#define N_FDS 10
  gint source_fds[N_FDS];
  gint target_fds[N_FDS];
  const gchar *argv[] = { "/nonexistent", NULL };
  gboolean retval;
  GError *local_error = NULL;
  struct stat statbuf;

  /* Open a temporary file and duplicate its FD several times so we have several
   * FDs to remap in the child process. */
  tmp_fd = g_file_open_tmp ("glib-spawn-test-XXXXXX", NULL, NULL);
  g_assert_cmpint (tmp_fd, >=, 0);

  for (i = 0; i < (N_FDS - 1); ++i)
    {
      int source;
#ifdef F_DUPFD_CLOEXEC
      source = fcntl (tmp_fd, F_DUPFD_CLOEXEC, 3);
#else
      source = dup (tmp_fd);
#endif
      g_assert_cmpint (source, >=, 0);
      source_fds[i] = source;
      target_fds[i] = source + N_FDS;
    }

  source_fds[i] = tmp_fd;
  target_fds[i] = tmp_fd + N_FDS;

  /* Print out the FD map. */
  g_test_message ("FD map:");
  for (i = 0; i < N_FDS; i++)
    g_test_message (" • %d → %d", source_fds[i], target_fds[i]);

  /* Spawn the subprocess. This should fail because the executable doesn’t
   * exist. */
  retval = g_spawn_async_with_pipes_and_fds (NULL, argv, NULL, G_SPAWN_DEFAULT,
                                             NULL, NULL, -1, -1, -1,
                                             source_fds, target_fds, N_FDS,
                                             NULL, NULL, NULL, NULL,
                                             &local_error);
  g_assert_error (local_error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);
  g_assert_false (retval);

  g_clear_error (&local_error);

  /* Check nothing was written to the temporary file, as would happen if the FD
   * mapping was messed up to conflict with the child process error reporting FD.
   * See https://gitlab.gnome.org/GNOME/glib/-/issues/2097 */
  g_assert_no_errno (fstat (tmp_fd, &statbuf));
  g_assert_cmpuint (statbuf.st_size, ==, 0);

  /* Clean up. */
  for (i = 0; i < N_FDS; i++)
    g_close (source_fds[i], NULL);
}

int
main (int   argc,
      char *argv[])
{
  char *dirname;
  int ret;

  setlocale (LC_ALL, "");

  g_test_init (&argc, &argv, NULL);

  dirname = g_path_get_dirname (argv[0]);
  echo_prog_path = g_build_filename (dirname, "test-spawn-echo" EXEEXT, NULL);
  echo_script_path = g_build_filename (dirname, "echo-script" SCRIPT_EXT, NULL);
  if (!g_file_test (echo_script_path, G_FILE_TEST_EXISTS))
    {
      g_free (echo_script_path);
      echo_script_path = g_test_build_filename (G_TEST_DIST, "echo-script" SCRIPT_EXT, NULL);
    }
  g_free (dirname);

  g_assert (g_file_test (echo_prog_path, G_FILE_TEST_EXISTS));
  g_assert (g_file_test (echo_script_path, G_FILE_TEST_EXISTS));

  g_test_add_func ("/gthread/spawn-single-sync", test_spawn_sync);
  g_test_add_func ("/gthread/spawn-stderr-socket", test_spawn_stderr_socket);
  g_test_add_func ("/gthread/spawn-single-async", test_spawn_async);
  g_test_add_func ("/gthread/spawn-single-async-with-fds", test_spawn_async_with_fds);
  g_test_add_func ("/gthread/spawn-async-with-invalid-fds", test_spawn_async_with_invalid_fds);
  g_test_add_func ("/gthread/spawn-script", test_spawn_script);
  g_test_add_func ("/gthread/spawn/nonexistent", test_spawn_nonexistent);
  g_test_add_func ("/gthread/spawn-posix-spawn", test_posix_spawn);
  g_test_add_func ("/gthread/spawn/fd-assignment-clash", test_spawn_fd_assignment_clash);

  ret = g_test_run();

  g_free (echo_script_path);
  g_free (echo_prog_path);

  return ret;
}
