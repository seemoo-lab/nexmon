/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2021 Igalia S.L.
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

#include <gio/gio.h>

static void
test_dup_default (void)
{
  GPowerProfileMonitor *monitor;

  monitor = g_power_profile_monitor_dup_default ();
  g_assert_nonnull (monitor);
  g_object_unref (monitor);
}

static void
power_saver_enabled_cb (GPowerProfileMonitor *monitor,
                        GParamSpec           *pspec,
                        gpointer              user_data)
{
  gboolean enabled;

  enabled = g_power_profile_monitor_get_power_saver_enabled (monitor);
  g_debug ("Power Saver %s (%d)", enabled ? "enabled" : "disabled", enabled);
}

static void
do_watch_power_profile (void)
{
  GPowerProfileMonitor *monitor;
  GMainLoop *loop;
  gulong signal_id;

  monitor = g_power_profile_monitor_dup_default ();
  signal_id = g_signal_connect (G_OBJECT (monitor), "notify::power-saver-enabled",
		                G_CALLBACK (power_saver_enabled_cb), NULL);

  loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (loop);

  g_signal_handler_disconnect (monitor, signal_id);
  g_object_unref (monitor);
  g_main_loop_unref (loop);
}

int
main (int argc, char **argv)
{
  int ret;

  if (argc == 2 && !strcmp (argv[1], "--watch"))
    {
      do_watch_power_profile ();
      return 0;
    }

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/power-profile-monitor/default", test_dup_default);

  ret = g_test_run ();

  return ret;
}
