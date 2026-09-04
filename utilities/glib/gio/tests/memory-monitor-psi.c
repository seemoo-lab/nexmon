/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2025 Red Hat, Inc.
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
  g_free (proc_contents);
}

static void
tests_teardown (SetupData *setup_data,
                gconstpointer data)
{
  g_unlink (setup_data->mock_proc_path);
  g_free (setup_data->mock_proc_path);
  g_unlink (setup_data->mock_psi_path);
  g_free (setup_data->mock_psi_path);
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

  monitor = g_object_new (G_TYPE_MEMORY_MONITOR_PSI,
                          "proc-path", setup->mock_proc_path,
                          NULL);
  warning_id = g_signal_connect (monitor, "low-memory-warning",
                                 G_CALLBACK (memory_monitor_psi_signal_cb), &warning_level);
  g_initable_init (G_INITABLE (monitor), NULL, &local_error);
  g_assert_no_error (local_error);

  g_file_set_contents (setup->mock_psi_path,
                       "test",
                       -1,
                       &local_error);
  g_assert_no_error (local_error);

  while (warning_level == -1)
    g_main_context_iteration (NULL, TRUE);

  g_assert_true (warning_level == 50 ||
                 warning_level == 100 ||
                 warning_level == 255);

  g_signal_handler_disconnect (monitor, warning_id);
  g_object_unref (monitor);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
  g_test_add ("/memory-monitor-psi/receive-signal", SetupData, NULL,
              tests_setup, test_receive_signals, tests_teardown);

  return g_test_run ();
}
