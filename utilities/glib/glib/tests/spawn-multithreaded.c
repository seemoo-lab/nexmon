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

#include <stdlib.h>

#include <glib.h>
#include <string.h>

#include <sys/types.h>

static char *echo_prog_path;

#ifdef G_OS_WIN32
static char *sleep_prog_path;
#endif

#ifdef G_OS_UNIX
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#endif

typedef struct
{
  GMainLoop *main_loop;
  gint *n_alive;  /* (atomic) */
  gint ttl;  /* seconds */
  GMainLoop *thread_main_loop;  /* (nullable) */
} SpawnChildsData;

static GPid
get_a_child (gint ttl)
{
  GPid pid;

#ifdef G_OS_WIN32
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  gchar *cmdline;
  wchar_t *cmdline_utf16;

  memset (&si, 0, sizeof (si));
  si.cb = sizeof (&si);
  memset (&pi, 0, sizeof (pi));

  cmdline = g_strdup_printf ("%s %d", sleep_prog_path, ttl);

  cmdline_utf16 = g_utf8_to_utf16 (cmdline, -1, NULL, NULL, NULL);
  g_assert_nonnull (cmdline_utf16);

  if (!CreateProcess (NULL, cmdline_utf16, NULL, NULL,
                      FALSE, 0, NULL, NULL, &si, &pi))
    g_error ("CreateProcess failed: %s",
             g_win32_error_message (GetLastError ()));

  g_free (cmdline_utf16);
  g_free (cmdline);

  CloseHandle (pi.hThread);
  pid = pi.hProcess;

  return pid;
#else
  pid = fork ();
  if (pid < 0)
    exit (1);

  if (pid > 0)
    return pid;

  sleep (ttl);
  _exit (0);
#endif /* G_OS_WIN32 */
}

static void
child_watch_callback (GPid pid, gint status, gpointer user_data)
{
  SpawnChildsData *data = user_data;

  g_test_message ("Child %" G_PID_FORMAT " (ttl %d) exited, status %d",
                  pid, data->ttl, status);

  g_spawn_close_pid (pid);

  if (g_atomic_int_dec_and_test (data->n_alive))
    g_main_loop_quit (data->main_loop);
  if (data->thread_main_loop != NULL)
    g_main_loop_quit (data->thread_main_loop);
}

static gpointer
start_thread (gpointer user_data)
{
  GMainLoop *new_main_loop;
  GSource *source;
  GPid pid;
  SpawnChildsData *data = user_data;
  gint ttl = data->ttl;
  GMainContext *new_main_context = NULL;

  new_main_context = g_main_context_new ();
  new_main_loop = g_main_loop_new (new_main_context, FALSE);
  data->thread_main_loop = new_main_loop;

  pid = get_a_child (ttl);
  source = g_child_watch_source_new (pid);
  g_source_set_callback (source,
                         (GSourceFunc) child_watch_callback, data, NULL);
  g_source_attach (source, g_main_loop_get_context (new_main_loop));
  g_source_unref (source);

  g_test_message ("Created pid: %" G_PID_FORMAT " (ttl %d)", pid, ttl);

  g_main_loop_run (new_main_loop);
  g_main_loop_unref (new_main_loop);
  g_main_context_unref (new_main_context);

  return NULL;
}

static gboolean
quit_loop (gpointer data)
{
  GMainLoop *main_loop = data;

  g_main_loop_quit (main_loop);

  return TRUE;
}

static void
test_spawn_childs (void)
{
  GPid pid;
  GMainLoop *main_loop = NULL;
  SpawnChildsData child1_data = { 0, }, child2_data = { 0, };
  gint n_alive;
  guint timeout_id;

  main_loop = g_main_loop_new (NULL, FALSE);

#ifdef G_OS_WIN32
  g_assert_no_errno (system ("cd ."));
#else
  g_assert_no_errno (system ("true"));
#endif

  n_alive = 2;
  timeout_id = g_timeout_add_seconds (30, quit_loop, main_loop);

  child1_data.main_loop = main_loop;
  child1_data.ttl = 1;
  child1_data.n_alive = &n_alive;
  pid = get_a_child (child1_data.ttl);
  g_child_watch_add (pid,
                     (GChildWatchFunc) child_watch_callback,
                     &child1_data);

  child2_data.main_loop = main_loop;
  child2_data.ttl = 2;
  child2_data.n_alive = &n_alive;
  pid = get_a_child (child2_data.ttl);
  g_child_watch_add (pid,
                     (GChildWatchFunc) child_watch_callback,
                     &child2_data);

  g_main_loop_run (main_loop);
  g_main_loop_unref (main_loop);
  g_source_remove (timeout_id);

  g_assert_cmpint (g_atomic_int_get (&n_alive), ==, 0);
}

static void
test_spawn_childs_threads (void)
{
  GMainLoop *main_loop = NULL;
  SpawnChildsData thread1_data = { 0, }, thread2_data = { 0, };
  gint n_alive;
  guint timeout_id;
  GThread *thread1, *thread2;

  main_loop = g_main_loop_new (NULL, FALSE);

#ifdef G_OS_WIN32
  g_assert_no_errno (system ("cd ."));
#else
  g_assert_no_errno (system ("true"));
#endif

  n_alive = 2;
  timeout_id = g_timeout_add_seconds (30, quit_loop, main_loop);

  thread1_data.main_loop = main_loop;
  thread1_data.n_alive = &n_alive;
  thread1_data.ttl = 1;  /* seconds */
  thread1 = g_thread_new (NULL, start_thread, &thread1_data);

  thread2_data.main_loop = main_loop;
  thread2_data.n_alive = &n_alive;
  thread2_data.ttl = 2;  /* seconds */
  thread2 = g_thread_new (NULL, start_thread, &thread2_data);

  g_main_loop_run (main_loop);
  g_main_loop_unref (main_loop);
  g_source_remove (timeout_id);

  g_assert_cmpint (g_atomic_int_get (&n_alive), ==, 0);

  g_thread_join (g_steal_pointer (&thread2));
  g_thread_join (g_steal_pointer (&thread1));
}

static void
multithreaded_test_run (GThreadFunc function)
{
  guint i;
  GPtrArray *threads = g_ptr_array_new ();
  guint n_threads;

  /* Limit to 64, otherwise we may hit file descriptor limits and such */
  n_threads = MIN (g_get_num_processors () * 2, 64);

  for (i = 0; i < n_threads; i++)
    {
      GThread *thread;

      thread = g_thread_new ("test", function, GUINT_TO_POINTER (i));
      g_ptr_array_add (threads, thread);
    }

  for (i = 0; i < n_threads; i++)
    {
      gpointer ret;
      ret = g_thread_join (g_ptr_array_index (threads, i));
      g_assert_cmpint (GPOINTER_TO_UINT (ret), ==, i);
    }
  g_ptr_array_free (threads, TRUE);
}

static gpointer
test_spawn_sync_multithreaded_instance (gpointer data)
{
  guint tnum = GPOINTER_TO_UINT (data);
  GError *error = NULL;
  GPtrArray *argv;
  char *arg;
  char *stdout_str;
  int estatus;

  arg = g_strdup_printf ("thread %u", tnum);

  argv = g_ptr_array_new ();
  g_ptr_array_add (argv, echo_prog_path);
  g_ptr_array_add (argv, arg);
  g_ptr_array_add (argv, NULL);

  g_spawn_sync (NULL, (char**)argv->pdata, NULL, G_SPAWN_DEFAULT, NULL, NULL, &stdout_str, NULL, &estatus, &error);
  g_assert_no_error (error);
  g_assert_cmpstr (arg, ==, stdout_str);
  g_free (arg);
  g_free (stdout_str);
  g_ptr_array_free (argv, TRUE);

  return GUINT_TO_POINTER (tnum);
}

static void
test_spawn_sync_multithreaded (void)
{
  multithreaded_test_run (test_spawn_sync_multithreaded_instance);
}

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
  GIOStatus status;
  SpawnAsyncMultithreadedData *data = datap;

 read:
  status = g_io_channel_read_chars (channel, buf, sizeof (buf), &bytes_read, &error);
  if (status == G_IO_STATUS_NORMAL)
    {
      g_string_append_len (data->stdout_buf, buf, (gssize) bytes_read);
      if (bytes_read == sizeof (buf))
	goto read;
    }
  else if (status == G_IO_STATUS_EOF)
    {
      g_string_append_len (data->stdout_buf, buf, (gssize) bytes_read);
      data->stdout_done = TRUE;
    }
  else if (status == G_IO_STATUS_ERROR)
    {
      g_error ("Error reading from child stdin: %s", error->message);
    }

  if (data->child_exited && data->stdout_done)
    g_main_loop_quit (data->loop);

  return !data->stdout_done;
}

static gpointer
test_spawn_async_multithreaded_instance (gpointer thread_data)
{
  guint tnum = GPOINTER_TO_UINT (thread_data);
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

  arg = g_strdup_printf ("thread %u", tnum);

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
  source = g_io_create_watch (channel, G_IO_IN | G_IO_HUP);
  g_source_set_callback (source, (GSourceFunc)on_child_stdout, &data, NULL);
  g_source_attach (source, context);
  g_source_unref (source);

  g_main_loop_run (loop);

  g_assert_true (data.child_exited);
  g_assert_true (data.stdout_done);
  g_assert_cmpstr (data.stdout_buf->str, ==, arg);
  g_string_free (data.stdout_buf, TRUE);

  g_io_channel_unref (channel);
  g_main_context_unref (context);
  g_main_loop_unref (loop);

  g_free (arg);

  return GUINT_TO_POINTER (tnum);
}

static void
test_spawn_async_multithreaded (void)
{
  multithreaded_test_run (test_spawn_async_multithreaded_instance);
}

int
main (int   argc,
      char *argv[])
{
  char *dirname;
  int ret;

  g_test_init (&argc, &argv, NULL);

  dirname = g_path_get_dirname (argv[0]);
  echo_prog_path = g_build_filename (dirname, "test-spawn-echo" EXEEXT, NULL);

  g_assert (g_file_test (echo_prog_path, G_FILE_TEST_EXISTS));
#ifdef G_OS_WIN32
  sleep_prog_path = g_build_filename (dirname, "test-spawn-sleep" EXEEXT, NULL);
  g_assert (g_file_test (sleep_prog_path, G_FILE_TEST_EXISTS));
#endif

  g_clear_pointer (&dirname, g_free);

  g_test_add_func ("/gthread/spawn-childs", test_spawn_childs);
  g_test_add_func ("/gthread/spawn-childs-threads", test_spawn_childs_threads);
  g_test_add_func ("/gthread/spawn-sync", test_spawn_sync_multithreaded);
  g_test_add_func ("/gthread/spawn-async", test_spawn_async_multithreaded);

  ret = g_test_run();

  g_free (echo_prog_path);

#ifdef G_OS_WIN32
  g_free (sleep_prog_path);
#endif

  return ret;
}
