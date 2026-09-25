/* GLib testing framework examples and tests
 *
 * Copyright © 2013 Red Hat, Inc.
 * Copyright © 2015, 2017, 2018 Endless Mobile, Inc.
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
#include <gstdio.h>

#if defined (G_OS_UNIX) && !defined (__APPLE__)
#include <gio/gdesktopappinfo.h>
#endif

typedef struct
{
  gchar *applications_dir;
} Fixture;

static void
setup (Fixture       *fixture,
       gconstpointer  user_data)
{
  fixture->applications_dir = g_build_filename (g_get_user_data_dir (), "applications", NULL);
  g_assert_no_errno (g_mkdir_with_parents (fixture->applications_dir, 0755));

  g_test_message ("Using data directory: %s", g_get_user_data_dir ());
}

static void
teardown (Fixture       *fixture,
          gconstpointer  user_data)
{
  g_assert_no_errno (g_rmdir (fixture->applications_dir));
  g_clear_pointer (&fixture->applications_dir, g_free);
}

#if defined (G_OS_UNIX) && !defined (__APPLE__)
static gboolean
create_app (gpointer data)
{
  const gchar *path = data;
  GError *error = NULL;
  const gchar *contents = 
    "[Desktop Entry]\n"
    "Name=Application\n"
    "Version=1.0\n"
    "Type=Application\n"
    "Exec=true\n";

  g_file_set_contents (path, contents, -1, &error);
  g_assert_no_error (error);

  return G_SOURCE_REMOVE;
}

static void
delete_app (gpointer data)
{
  const gchar *path = data;

  g_remove (path);
}

static void
changed_cb (GAppInfoMonitor *monitor,
            gpointer         user_data)
{
  gboolean *changed_fired = user_data;

  *changed_fired = TRUE;
  g_main_context_wakeup (g_main_context_get_thread_default ());
}

static gboolean
timeout_cb (gpointer user_data)
{
  gboolean *timed_out = user_data;

  g_assert (!*timed_out);
  *timed_out = TRUE;
  g_main_context_wakeup (g_main_context_get_thread_default ());

  return G_SOURCE_REMOVE;
}
#endif  /* defined (G_OS_UNIX) && !defined (__APPLE__) */

static void
test_app_monitor (Fixture       *fixture,
                  gconstpointer  user_data)
{
#if defined (G_OS_UNIX) && !defined (__APPLE__)
  gchar *app_path;
  GAppInfoMonitor *monitor;
  GMainContext *context = NULL;  /* use the global default main context */
  GSource *timeout_source = NULL;
  GDesktopAppInfo *app = NULL;
  gboolean changed_fired = FALSE;
  gboolean timed_out = FALSE;

  app_path = g_build_filename (fixture->applications_dir, "app.desktop", NULL);

  /* FIXME: this shouldn't be required */
  g_list_free_full (g_app_info_get_all (), g_object_unref);

  /* Create an app monitor and check that its ::changed signal is emitted when
   * a new app is installed. */
  monitor = g_app_info_monitor_get ();

  g_signal_connect (monitor, "changed", G_CALLBACK (changed_cb), &changed_fired);

  g_idle_add (create_app, app_path);
  timeout_source = g_timeout_source_new_seconds (3);
  g_source_set_callback (timeout_source, timeout_cb, &timed_out, NULL);
  g_source_attach (timeout_source, NULL);

  while (!changed_fired && !timed_out)
    g_main_context_iteration (context, TRUE);

  g_assert_true (changed_fired);
  changed_fired = FALSE;

  g_source_destroy (timeout_source);
  g_clear_pointer (&timeout_source, g_source_unref);

  /* Check that the app is now queryable. This has the side-effect of re-arming
   * the #GAppInfoMonitor::changed signal for the next part of the test. */
  app = g_desktop_app_info_new ("app.desktop");
  g_assert_nonnull (app);
  g_clear_object (&app);

  /* Now check that ::changed is emitted when an app is uninstalled. */
  timeout_source = g_timeout_source_new_seconds (3);
  g_source_set_callback (timeout_source, timeout_cb, &timed_out, NULL);
  g_source_attach (timeout_source, NULL);

  delete_app (app_path);

  while (!changed_fired && !timed_out)
    g_main_context_iteration (context, TRUE);

  g_assert_true (changed_fired);

  g_source_destroy (timeout_source);
  g_clear_pointer (&timeout_source, g_source_unref);
  g_remove (app_path);

  g_object_unref (monitor);

  g_free (app_path);
#elif defined (__APPLE__)
  g_test_skip (".desktop monitor on macos");
#else  /* if !(defined (G_OS_UNIX) && !defined (__APPLE__)) */
  g_test_skip (".desktop monitor on win32");
#endif  /* !(defined (G_OS_UNIX) && !defined (__APPLE__)) */
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add ("/monitor/app", Fixture, NULL, setup, test_app_monitor, teardown);

  return g_test_run ();
}
