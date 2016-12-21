#include <gio/gio.h>
#include <gstdio.h>

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

static gboolean changed_fired;

static void
changed_cb (GAppInfoMonitor *monitor, GMainLoop *loop)
{
  changed_fired = TRUE;
  g_main_loop_quit (loop);
}

static gboolean
quit_loop (gpointer data)
{
  GMainLoop *loop = data;

  if (g_main_loop_is_running (loop))
    g_main_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static void
test_app_monitor (void)
{
  gchar *path, *app_path;
  GAppInfoMonitor *monitor;
  GMainLoop *loop;

  path = g_build_filename (g_get_user_data_dir (), "applications", NULL);
  g_mkdir (path, 0755);

  app_path = g_build_filename (path, "app.desktop", NULL);

  /* FIXME: this shouldn't be required */
  g_list_free_full (g_app_info_get_all (), g_object_unref);

  monitor = g_app_info_monitor_get ();
  loop = g_main_loop_new (NULL, FALSE);

  g_signal_connect (monitor, "changed", G_CALLBACK (changed_cb), loop);

  g_idle_add (create_app, app_path);
  g_timeout_add_seconds (3, quit_loop, loop);

  g_main_loop_run (loop);
  g_assert (changed_fired);
  changed_fired = FALSE;

  /* FIXME: this shouldn't be required */
  g_list_free_full (g_app_info_get_all (), g_object_unref);

  g_timeout_add_seconds (3, quit_loop, loop);

  delete_app (app_path);

  g_main_loop_run (loop);

  g_assert (changed_fired);

  g_main_loop_unref (loop);
  g_remove (app_path);

  g_object_unref (monitor);

  g_free (path);
  g_free (app_path);
}

int
main (int argc, char *argv[])
{
  gchar *path;

  path = g_mkdtemp (g_strdup ("app_monitor_XXXXXX"));
  g_setenv ("XDG_DATA_DIRS", path, TRUE);
  g_setenv ("XDG_DATA_HOME", path, TRUE);

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/monitor/app", test_app_monitor);

  return g_test_run ();
}
