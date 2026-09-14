/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2026 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/glib.h>
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "gio.h"
#include "gmemorymonitorpsi.h"

typedef struct
{
  char *mock_psi_path;
  char *mock_proc_path;
  char *mock_write_env_path;
  char *mock_write_env_no_write;
  char *mock_socket_dir;
  char *mock_socket_path;
  GSocket *mock_write_socket;

  GMutex socket_mutex;
  GCond socket_cond;
} SetupData;

static void
tests_setup (SetupData *setup_data,
             gconstpointer data)
{
  char *proc_contents = NULL;
  GError *local_error = NULL;

  setup_data->mock_psi_path = g_build_filename (g_get_tmp_dir (), "cgroup", NULL);
  g_assert_no_errno (mkfifo (setup_data->mock_psi_path, S_IRUSR | S_IWUSR));

  setup_data->mock_proc_path = g_build_filename (g_get_tmp_dir (), "psi-proc", NULL);
  proc_contents = g_strdup_printf ("0::%s", setup_data->mock_psi_path);
  g_file_set_contents_full (setup_data->mock_proc_path, proc_contents,
                            -1, G_FILE_SET_CONTENTS_NONE,
                            S_IRUSR | S_IWUSR, &local_error);
  g_assert_no_error (local_error);
  g_clear_error (&local_error);

  setup_data->mock_write_env_path = g_build_filename (g_get_tmp_dir (), "write-env", NULL);
  g_file_set_contents_full (setup_data->mock_write_env_path, "",
                            -1, G_FILE_SET_CONTENTS_NONE,
                            S_IRUSR | S_IWUSR, &local_error);

  setup_data->mock_write_env_no_write = g_build_filename (g_get_tmp_dir (), "write-env-no-write", NULL);
  g_file_set_contents_full (setup_data->mock_write_env_no_write, "",
                            -1, G_FILE_SET_CONTENTS_NONE,
                            S_IRUSR | S_IWUSR, &local_error);

  g_free (proc_contents);
}

static void
tests_unix_domain_setup (SetupData *setup_data,
                         gconstpointer data)
{
  char *proc_contents = NULL;
  GSocketAddress *socket_addr;
  GError *local_error = NULL;

  setup_data->mock_psi_path = g_build_filename (g_get_tmp_dir (), "cgroup", NULL);
  g_assert_no_errno (mkfifo (setup_data->mock_psi_path, S_IRUSR | S_IWUSR));

  setup_data->mock_proc_path = g_build_filename (g_get_tmp_dir (), "psi-proc", NULL);
  proc_contents = g_strdup_printf ("0::%s", setup_data->mock_psi_path);
  g_file_set_contents_full (setup_data->mock_proc_path, proc_contents,
                            -1, G_FILE_SET_CONTENTS_NONE,
                            S_IRUSR | S_IWUSR, &local_error);
  g_assert_no_error (local_error);
  g_clear_error (&local_error);

  g_free (proc_contents);

  setup_data->mock_socket_dir = g_dir_make_tmp ("memory-monitor-psi-env-set-XXXXXX", &local_error);
  g_assert_no_error (local_error);
  setup_data->mock_socket_path = g_build_filename (setup_data->mock_socket_dir,
                                                   "memory-monitor-socket", NULL);
  setup_data->mock_write_socket = g_socket_new (G_SOCKET_FAMILY_UNIX,
                                                G_SOCKET_TYPE_STREAM,
                                                0, &local_error);
  g_assert_no_error (local_error);
  socket_addr = g_unix_socket_address_new (setup_data->mock_socket_path);
  g_socket_bind (setup_data->mock_write_socket,
                 socket_addr,
                 TRUE, &local_error);
  g_object_unref (socket_addr);
  g_assert_no_error (local_error);

  g_socket_listen (setup_data->mock_write_socket, &local_error);
  g_assert_no_error (local_error);

  g_clear_error (&local_error);

  g_mutex_init (&setup_data->socket_mutex);
  g_cond_init (&setup_data->socket_cond);
}

static void
tests_unix_domain_teardown (SetupData *setup_data,
                            gconstpointer data)
{
  g_unlink (setup_data->mock_proc_path);
  g_free (setup_data->mock_proc_path);
  g_unlink (setup_data->mock_psi_path);
  g_free (setup_data->mock_psi_path);
  g_socket_close (setup_data->mock_write_socket, NULL);
  g_object_unref (setup_data->mock_write_socket);
  g_unlink (setup_data->mock_socket_path);
  g_rmdir (setup_data->mock_socket_dir);
  g_free (setup_data->mock_socket_path);
  g_free (setup_data->mock_socket_dir);
  g_cond_clear (&setup_data->socket_cond);
  g_mutex_clear (&setup_data->socket_mutex);
}

static void
tests_teardown (SetupData *setup_data,
                gconstpointer data)
{
  g_unlink (setup_data->mock_proc_path);
  g_free (setup_data->mock_proc_path);
  g_unlink (setup_data->mock_psi_path);
  g_free (setup_data->mock_psi_path);
  g_unlink (setup_data->mock_write_env_path);
  g_free (setup_data->mock_write_env_path);
  g_unlink (setup_data->mock_write_env_no_write);
  g_free (setup_data->mock_write_env_no_write);
}

static void
memory_monitor_psi_signal_cb (GMemoryMonitor *m,
                              GMemoryMonitorWarningLevel level,
                              gpointer user_data)
{
  int *out_warning_level = user_data;

  *out_warning_level = level;

  g_main_context_wakeup (NULL);
}

static void
test_receive_signals (SetupData *setup, gconstpointer data)
{
  GMemoryMonitorPsi *monitor;
  unsigned long warning_id;
  int warning_level = -1;
  GError *local_error = NULL;

  g_setenv ("MEMORY_PRESSURE_WATCH", setup->mock_psi_path, TRUE);
  g_setenv ("MEMORY_PRESSURE_WRITE", "c29tZSAxNTAwMDAgMTAwMDAwMA==", TRUE);

  g_file_set_contents (setup->mock_psi_path,
                       "some 150000 1000000",
                       -1,
                       &local_error);
  g_assert_no_error (local_error);

  monitor = g_object_new (G_TYPE_MEMORY_MONITOR_PSI,
                          "proc-path", setup->mock_proc_path,
                          NULL);
  warning_id = g_signal_connect (monitor, "low-memory-warning",
                                 G_CALLBACK (memory_monitor_psi_signal_cb), &warning_level);
  g_initable_init (G_INITABLE (monitor), NULL, &local_error);
  g_assert_no_error (local_error);

  while (warning_level == -1)
    g_main_context_iteration (NULL, TRUE);

  g_assert_true (warning_level == 50);

  g_signal_handler_disconnect (monitor, warning_id);
  g_object_unref (monitor);
}

static void
test_write_env (SetupData *setup, gconstpointer data)
{
  GMemoryMonitorPsi *monitor;
  unsigned long warning_id;
  int warning_level = -1;
  GError *local_error = NULL;
  gchar *read_data = NULL;
  gsize read_data_len = 0;
  gchar *base64_data = NULL;
  /* The contents is generated by running "dd if=/dev/random of=/foo/bar bs=128 count=1" */
  const gchar *env_write = "hVh9eV0BZsh/GWEs9V6zKyO3VUe2MnxTxLZbA7BRo7GJKHAAayIRsgO8EI4m8iFskhx/O4cAfpyL"
                           "YQ8LFHMG5XBuBmqD9spZghu0eVYF2X8lCCT+iBlEFw5dWiSa6XwJN++AmHi1tp4uJj0UBR7CwV5c"
                           "+ms9ZDUqOQrvlFHUSxw=";

  g_setenv ("MEMORY_PRESSURE_WATCH", setup->mock_write_env_path, TRUE);
  g_setenv ("MEMORY_PRESSURE_WRITE", env_write, TRUE);

  monitor = g_object_new (G_TYPE_MEMORY_MONITOR_PSI,
                          "proc-path", setup->mock_proc_path,
                          NULL);
  warning_id = g_signal_connect (monitor, "low-memory-warning",
                                 G_CALLBACK (memory_monitor_psi_signal_cb), &warning_level);
  g_initable_init (G_INITABLE (monitor), NULL, &local_error);
  g_assert_no_error (local_error);

  g_signal_handler_disconnect (monitor, warning_id);
  g_object_unref (monitor);

  /* Verify the write contents */
  g_file_get_contents (setup->mock_write_env_path, &read_data, &read_data_len, &local_error);
  g_assert_no_error (local_error);
  base64_data = g_base64_encode ((guchar *) read_data, read_data_len);
  g_assert_cmpstr (base64_data, ==, env_write);
  g_assert_cmpuint (read_data_len, ==, 128);
  g_free (read_data);
  g_free (base64_data);
  g_clear_error (&local_error);
}

static void
test_write_env_no_write (SetupData *setup, gconstpointer data)
{
  GMemoryMonitorPsi *monitor;
  unsigned long warning_id;
  int warning_level = -1;
  GError *local_error = NULL;
  gchar *read_data = NULL;
  gsize read_data_len = 0;
  gchar *base64_data = NULL;

  g_setenv ("MEMORY_PRESSURE_WATCH", setup->mock_write_env_no_write, TRUE);
  g_unsetenv ("MEMORY_PRESSURE_WRITE");

  monitor = g_object_new (G_TYPE_MEMORY_MONITOR_PSI,
                          "proc-path", setup->mock_proc_path,
                          NULL);
  warning_id = g_signal_connect (monitor, "low-memory-warning",
                                 G_CALLBACK (memory_monitor_psi_signal_cb), &warning_level);
  g_initable_init (G_INITABLE (monitor), NULL, &local_error);
  g_assert_no_error (local_error);

  g_signal_handler_disconnect (monitor, warning_id);
  g_object_unref (monitor);

  /* Verify the write contents */
  g_file_get_contents (setup->mock_write_env_no_write, &read_data, &read_data_len, &local_error);
  g_assert_no_error (local_error);
  base64_data = g_base64_encode ((guchar *) read_data, read_data_len);
  g_assert_cmpuint (read_data_len, ==, 0);
  g_free (read_data);
  g_free (base64_data);
  g_clear_error (&local_error);
}

static void
test_write_env_null (SetupData *setup, gconstpointer data)
{
  GMemoryMonitorPsi *monitor;
  unsigned long warning_id;
  int warning_level = -1;
  GError *local_error = NULL;

  /* The contents is generated by running "dd if=/dev/random of=/foo/bar bs=128 count=1" */
  const gchar *env_write = "hVh9eV0BZsh/GWEs9V6zKyO3VUe2MnxTxLZbA7BRo7GJKHAAayIRsgO8EI4m8iFskhx/O4cAfpyL"
                           "YQ8LFHMG5XBuBmqD9spZghu0eVYF2X8lCCT+iBlEFw5dWiSa6XwJN++AmHi1tp4uJj0UBR7CwV5c"
                           "+ms9ZDUqOQrvlFHUSxw=";

  g_setenv ("MEMORY_PRESSURE_WATCH", "/dev/null", TRUE);
  g_setenv ("MEMORY_PRESSURE_WRITE", env_write, TRUE);

  monitor = g_object_new (G_TYPE_MEMORY_MONITOR_PSI,
                          "proc-path", setup->mock_proc_path,
                          NULL);
  warning_id = g_signal_connect (monitor, "low-memory-warning",
                                 G_CALLBACK (memory_monitor_psi_signal_cb), &warning_level);
  g_initable_init (G_INITABLE (monitor), NULL, &local_error);
  g_assert_no_error (local_error);

  g_signal_handler_disconnect (monitor, warning_id);
  g_object_unref (monitor);

  /* warning_level shouldn't be set */
  g_assert_cmpint (warning_level, ==, -1);

  g_clear_error (&local_error);
}

static gpointer
test_write_env_unix_domain_socket_thread (gpointer user_data)
{
  SetupData *setup = user_data;
  GError *local_error = NULL;
  GSocket *socket = NULL;
  gchar read_data[256];
  gssize ret_size = 0;

  g_mutex_lock (&setup->socket_mutex);
  g_cond_signal (&setup->socket_cond);
  g_mutex_unlock (&setup->socket_mutex);

  socket = g_socket_accept (setup->mock_write_socket,
                            NULL, &local_error);
  g_assert_no_error (local_error);

  ret_size = g_socket_receive (socket, read_data, sizeof (read_data), NULL, &local_error);

  g_assert_no_error (local_error);
  g_assert_cmpint (ret_size, ==, 128);

  g_socket_send (socket, read_data, ret_size, NULL, &local_error);

  g_socket_close (socket, &local_error);
  g_assert_no_error (local_error);
  g_object_unref (socket);

  return NULL;
}

static void
test_write_env_unix_domain_socket (SetupData *setup, gconstpointer data)
{
  GMemoryMonitorPsi *monitor;
  int warning_level = -1;
  GError *local_error = NULL;
  GThread *thread = NULL;

  /* The contents is generated by running "dd if=/dev/random of=/foo/bar bs=128 count=1" */
  const gchar *env_write = "hVh9eV0BZsh/GWEs9V6zKyO3VUe2MnxTxLZbA7BRo7GJKHAAayIRsgO8EI4m8iFskhx/O4cAfpyL"
                           "YQ8LFHMG5XBuBmqD9spZghu0eVYF2X8lCCT+iBlEFw5dWiSa6XwJN++AmHi1tp4uJj0UBR7CwV5c"
                           "+ms9ZDUqOQrvlFHUSxw=";

  g_setenv ("MEMORY_PRESSURE_WATCH", setup->mock_socket_path, TRUE);
  g_setenv ("MEMORY_PRESSURE_WRITE", env_write, TRUE);

  g_mutex_lock (&setup->socket_mutex);

  thread = g_thread_new ("test_write_env_unix_domain_socket_thread",
                         test_write_env_unix_domain_socket_thread, setup);

  g_cond_wait (&setup->socket_cond, &setup->socket_mutex);

  monitor = g_object_new (G_TYPE_MEMORY_MONITOR_PSI,
                          "proc-path", setup->mock_proc_path,
                          NULL);
  g_initable_init (G_INITABLE (monitor), NULL, &local_error);

  g_mutex_unlock (&setup->socket_mutex);

  g_assert_no_error (local_error);

  g_signal_connect (monitor, "low-memory-warning",
                    G_CALLBACK (memory_monitor_psi_signal_cb), &warning_level);
  g_assert_no_error (local_error);

  g_thread_join (thread);

  while (warning_level == -1)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpint (warning_level, ==, 50);

  g_object_unref (monitor);
  g_clear_error (&local_error);
}

int
main (int argc, char **argv)
{
  g_setenv ("GIO_USE_MEMORY_MONITOR", "psi", TRUE);

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
  g_test_add ("/memory-monitor-psi-env-set/receive-signal", SetupData, NULL,
              tests_setup, test_receive_signals, tests_teardown);

  g_test_add ("/memory-monitor-psi-env-set/write-env-no-write", SetupData, NULL,
              tests_setup, test_write_env_no_write, tests_teardown);

  g_test_add ("/memory-monitor-psi-env-set/write-env", SetupData, NULL,
              tests_setup, test_write_env, tests_teardown);

  g_test_add ("/memory-monitor-psi-env-set/write-env-null", SetupData, NULL,
              tests_setup, test_write_env_null, tests_teardown);

  g_test_add ("/memory-monitor-psi-env-set/write-env-unix-domain-socket", SetupData, NULL,
              tests_unix_domain_setup, test_write_env_unix_domain_socket, tests_unix_domain_teardown);

  return g_test_run ();
}
