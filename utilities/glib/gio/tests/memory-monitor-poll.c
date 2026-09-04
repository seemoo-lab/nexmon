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

#include "gio.h"

#include "gmemorymonitorbase.h"
#include "gmemorymonitorpoll.h"

typedef struct
{
  double simulated_mem_free_ratio;
  int expected_warning_level;
} TestData;

static void
memory_monitor_signal_cb (GMemoryMonitor             *m,
                          GMemoryMonitorWarningLevel  level,
                          void                       *user_data)
{
  int *out_warning_level = user_data;

  g_assert_cmpint (*out_warning_level, ==, -1);
  *out_warning_level = level;

  g_main_context_wakeup (NULL);
}

static void
test_dup_default (void)
{
  GMemoryMonitor *monitor;

  monitor = g_memory_monitor_dup_default ();
  g_assert_nonnull (monitor);
  g_object_unref (monitor);
}

static void
test_event (const void *data)
{
  const TestData *test_data = data;
  GMemoryMonitorPoll *monitor;
  GError *local_error = NULL;
  int warning_level = -1;
  unsigned long warning_id;

  monitor = g_object_new (G_TYPE_MEMORY_MONITOR_POLL,
                          "poll-interval-ms", 50,
                          "mem-free-ratio", test_data->simulated_mem_free_ratio,
                          NULL);
  warning_id = g_signal_connect (monitor, "low-memory-warning",
                                 G_CALLBACK (memory_monitor_signal_cb), &warning_level);
  g_initable_init (G_INITABLE (monitor), NULL, &local_error);
  g_assert_no_error (local_error);

  while (warning_level == -1)
    g_main_context_iteration (NULL, TRUE);

  g_assert_cmpint (warning_level, ==, test_data->expected_warning_level);

  g_signal_handler_disconnect (monitor, warning_id);
  g_object_unref (monitor);
}

static void
warning_cb (GMemoryMonitor *m,
            GMemoryMonitorWarningLevel level,
            GMainLoop *test_loop)
{
  char *str = g_enum_to_string (G_TYPE_MEMORY_MONITOR_WARNING_LEVEL, level);
  g_free (str);
  g_main_loop_quit (test_loop);
}

static void
do_watch_memory (void)
{
  GMemoryMonitor *m;
  GMainLoop *loop;

  m = g_memory_monitor_dup_default ();
  loop = g_main_loop_new (NULL, TRUE);
  g_signal_connect (G_OBJECT (m), "low-memory-warning",
                    G_CALLBACK (warning_cb), loop);

  g_main_loop_run (loop);

  g_signal_handlers_disconnect_by_func (G_OBJECT (m),
                                        G_CALLBACK (warning_cb), loop);

  g_main_loop_unref (loop);
  g_clear_object (&m);
}

int
main (int argc, char **argv)
{
  if (argc == 2 && strcmp (argv[1], "--watch") == 0)
    {
      do_watch_memory ();
      return 0;
    }

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/memory-monitor-poll/dup-default", test_dup_default);

  g_test_add_data_func ("/memory-monitor-poll/critical-event",
                        &((const TestData) {
                          .simulated_mem_free_ratio = 0.19,
                          .expected_warning_level = 255,
                        }), test_event);
  g_test_add_data_func ("/memory-monitor-poll/medium-event",
                        &((const TestData) {
                          .simulated_mem_free_ratio = 0.29,
                          .expected_warning_level = 100,
                        }), test_event);
  g_test_add_data_func ("/memory-monitor-poll/low-event",
                        &((const TestData) {
                          .simulated_mem_free_ratio = 0.39,
                          .expected_warning_level = 50,
                        }), test_event);

  return g_test_run ();
}
