#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gdbus-tests.h"
#include "gdbus-sessionbus.h"

#if 0
/* These tests are racy -- there is no guarantee about the order of data
 * arriving over D-Bus.
 *
 * They're also a bit ridiculous -- GApplication was never meant to be
 * abused in this way...
 *
 * We need new tests.
 */
static gint outstanding_watches;
static GMainLoop *main_loop;

typedef struct
{
  gchar *expected_stdout;
  gint stdout_pipe;
  gchar *expected_stderr;
  gint stderr_pipe;
} ChildData;

static void
check_data (gint fd, const gchar *expected)
{
  gssize len, actual;
  gchar *buffer;
  
  len = strlen (expected);
  buffer = g_alloca (len + 100);
  actual = read (fd, buffer, len + 100);

  g_assert_cmpint (actual, >=, 0);

  if (actual != len ||
      memcmp (buffer, expected, len) != 0)
    {
      buffer[MIN(len + 100, actual)] = '\0';

      g_error ("\nExpected\n-----\n%s-----\nGot (%s)\n-----\n%s-----\n",
               expected,
               (actual > len) ? "truncated" : "full", buffer);
    }
}

static void
child_quit (GPid     pid,
            gint     status,
            gpointer data)
{
  ChildData *child = data;

  g_assert_cmpint (status, ==, 0);

  if (--outstanding_watches == 0)
    g_main_loop_quit (main_loop);

  check_data (child->stdout_pipe, child->expected_stdout);
  close (child->stdout_pipe);
  g_free (child->expected_stdout);

  if (child->expected_stderr)
    {
      check_data (child->stderr_pipe, child->expected_stderr);
      close (child->stderr_pipe);
      g_free (child->expected_stderr);
    }

  g_slice_free (ChildData, child);
}

static void
spawn (const gchar *expected_stdout,
       const gchar *expected_stderr,
       const gchar *first_arg,
       ...)
{
  GError *error = NULL;
  const gchar *arg;
  GPtrArray *array;
  ChildData *data;
  gchar **args;
  va_list ap;
  GPid pid;
  GPollFD fd;
  gchar **env;

  va_start (ap, first_arg);
  array = g_ptr_array_new ();
  g_ptr_array_add (array, g_test_build_filename (G_TEST_BUILT, "basic-application", NULL));
  for (arg = first_arg; arg; arg = va_arg (ap, const gchar *))
    g_ptr_array_add (array, g_strdup (arg));
  g_ptr_array_add (array, NULL);
  args = (gchar **) g_ptr_array_free (array, FALSE);
  va_end (ap);

  env = g_environ_setenv (g_get_environ (), "TEST", "1", TRUE);

  data = g_slice_new (ChildData);
  data->expected_stdout = g_strdup (expected_stdout);
  data->expected_stderr = g_strdup (expected_stderr);

  g_spawn_async_with_pipes (NULL, args, env,
                            G_SPAWN_DO_NOT_REAP_CHILD,
                            NULL, NULL, &pid, NULL,
                            &data->stdout_pipe,
                            expected_stderr ? &data->stderr_pipe : NULL,
                            &error);
  g_assert_no_error (error);

  g_strfreev (env);

  g_child_watch_add (pid, child_quit, data);
  outstanding_watches++;

  /* we block until the children write to stdout to make sure
   * they have started, as they need to be executed in order;
   * see https://bugzilla.gnome.org/show_bug.cgi?id=664627
   */
  fd.fd = data->stdout_pipe;
  fd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
  g_poll (&fd, 1, -1);
}

static void
basic (void)
{
  GDBusConnection *c;

  g_assert (outstanding_watches == 0);

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  main_loop = g_main_loop_new (NULL, 0);

  /* spawn the main instance */
  spawn ("activated\n"
         "open file:///a file:///b\n"
         "exit status: 0\n", NULL,
         "./app", NULL);

  /* send it some files */
  spawn ("exit status: 0\n", NULL,
         "./app", "/a", "/b", NULL);

  g_main_loop_run (main_loop);

  g_object_unref (c);
  session_bus_down ();

  g_main_loop_unref (main_loop);
}

static void
test_remote_command_line (void)
{
  GDBusConnection *c;
  GFile *file;
  gchar *replies;
  gchar *cwd;

  g_assert (outstanding_watches == 0);

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  main_loop = g_main_loop_new (NULL, 0);

  file = g_file_new_for_commandline_arg ("foo");
  cwd = g_get_current_dir ();

  replies = g_strconcat ("got ./cmd 0\n",
                         "got ./cmd 1\n",
                         "cmdline ./cmd echo --abc -d\n",
                         "environment TEST=1\n",
                         "getenv TEST=1\n",
                         "file ", g_file_get_path (file), "\n",
                         "properties ok\n",
                         "cwd ", cwd, "\n",
                         "busy\n",
                         "idle\n",
                         "stdin ok\n",        
                         "exit status: 0\n",
                         NULL);
  g_object_unref (file);

  /* spawn the main instance */
  spawn (replies, NULL,
         "./cmd", NULL);

  g_free (replies);

  /* send it a few commandlines */
  spawn ("exit status: 0\n", NULL,
         "./cmd", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "echo", "--abc", "-d", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "env", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "getenv", NULL);

  spawn ("print test\n"
         "exit status: 0\n", NULL,
         "./cmd", "print", "test", NULL);

  spawn ("exit status: 0\n", "printerr test\n",
         "./cmd", "printerr", "test", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "file", "foo", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "properties", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "cwd", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "busy", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "idle", NULL);

  spawn ("exit status: 0\n", NULL,
         "./cmd", "stdin", NULL);

  g_main_loop_run (main_loop);

  g_object_unref (c);
  session_bus_down ();

  g_main_loop_unref (main_loop);
}

static void
test_remote_actions (void)
{
  GDBusConnection *c;

  g_assert (outstanding_watches == 0);

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  main_loop = g_main_loop_new (NULL, 0);

  /* spawn the main instance */
  spawn ("got ./cmd 0\n"
         "activate action1\n"
         "change action2 1\n"
         "exit status: 0\n", NULL,
         "./cmd", NULL);

  spawn ("actions quit new action1 action2\n"
         "exit status: 0\n", NULL,
         "./actions", "list", NULL);

  spawn ("exit status: 0\n", NULL,
         "./actions", "activate", NULL);

  spawn ("exit status: 0\n", NULL,
         "./actions", "set-state", NULL);

  g_main_loop_run (main_loop);

  g_object_unref (c);
  session_bus_down ();

  g_main_loop_unref (main_loop);
}
#endif

#if 0
/* Now that we register non-unique apps on the bus we need to fix the
 * following test not to assume that it's safe to create multiple instances
 * of the same app in one process.
 *
 * See https://bugzilla.gnome.org/show_bug.cgi?id=647986 for the patch that
 * introduced this problem.
 */

static GApplication *recently_activated;
static GMainLoop *loop;

static void
nonunique_activate (GApplication *application)
{
  recently_activated = application;

  if (loop != NULL)
    g_main_loop_quit (loop);
}

static GApplication *
make_app (gboolean non_unique)
{
  GApplication *app;
  gboolean ok;

  app = g_application_new ("org.gtk.Test-Application",
                           non_unique ? G_APPLICATION_NON_UNIQUE : 0);
  g_signal_connect (app, "activate", G_CALLBACK (nonunique_activate), NULL);
  ok = g_application_register (app, NULL, NULL);
  if (!ok)
    {
      g_object_unref (app);
      return NULL;
    }

  g_application_activate (app);

  return app;
}

static void
test_nonunique (void)
{
  GApplication *first, *second, *third, *fourth;

  session_bus_up ();

  first = make_app (TRUE);
  /* non-remote because it is non-unique */
  g_assert (!g_application_get_is_remote (first));
  g_assert (recently_activated == first);
  recently_activated = NULL;

  second = make_app (FALSE);
  /* non-remote because it is first */
  g_assert (!g_application_get_is_remote (second));
  g_assert (recently_activated == second);
  recently_activated = NULL;

  third = make_app (TRUE);
  /* non-remote because it is non-unique */
  g_assert (!g_application_get_is_remote (third));
  g_assert (recently_activated == third);
  recently_activated = NULL;

  fourth = make_app (FALSE);
  /* should have failed to register due to being
   * unable to register the object paths
   */
  g_assert (fourth == NULL);
  g_assert (recently_activated == NULL);

  g_object_unref (first);
  g_object_unref (second);
  g_object_unref (third);

  session_bus_down ();
}
#endif

static void
properties (void)
{
  GDBusConnection *c;
  GObject *app;
  gchar *id;
  gchar *version;
  GApplicationFlags flags;
  gboolean registered;
  guint timeout;
  gboolean remote;
  gboolean ret;
  GError *error = NULL;

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  app = g_object_new (G_TYPE_APPLICATION,
                      "application-id", "org.gtk.TestApplication",
                      "version", "1.0",
                      NULL);

  g_object_get (app,
                "application-id", &id,
                "version", &version,
                "flags", &flags,
                "is-registered", &registered,
                "inactivity-timeout", &timeout,
                NULL);

  g_assert_cmpstr (id, ==, "org.gtk.TestApplication");
  g_assert_cmpstr (version, ==, "1.0");
  g_assert_cmpint (flags, ==, G_APPLICATION_DEFAULT_FLAGS);
  g_assert (!registered);
  g_assert_cmpint (timeout, ==, 0);

  g_clear_pointer (&version, g_free);

  ret = g_application_register (G_APPLICATION (app), NULL, &error);
  g_assert (ret);
  g_assert_no_error (error);

  g_object_get (app,
                "is-registered", &registered,
                "is-remote", &remote,
                NULL);

  g_assert (registered);
  g_assert (!remote);

  g_object_set (app,
                "inactivity-timeout", 1000,
                NULL);

  g_application_quit (G_APPLICATION (app));

  g_object_unref (c);
  g_object_unref (app);
  g_free (id);

  session_bus_down ();
}

static void
appid (void)
{
  gchar *id;

  g_assert_false (g_application_id_is_valid (""));
  g_assert_false (g_application_id_is_valid ("."));
  g_assert_false (g_application_id_is_valid ("a"));
  g_assert_false (g_application_id_is_valid ("abc"));
  g_assert_false (g_application_id_is_valid (".abc"));
  g_assert_false (g_application_id_is_valid ("abc."));
  g_assert_false (g_application_id_is_valid ("a..b"));
  g_assert_false (g_application_id_is_valid ("a/b"));
  g_assert_false (g_application_id_is_valid ("a\nb"));
  g_assert_false (g_application_id_is_valid ("a\nb"));
  g_assert_false (g_application_id_is_valid ("emoji_picker"));
  g_assert_false (g_application_id_is_valid ("emoji-picker"));
  g_assert_false (g_application_id_is_valid ("emojipicker"));
  g_assert_false (g_application_id_is_valid ("my.Terminal.0123"));
  id = g_new0 (gchar, 261);
  memset (id, 'a', 260);
  id[1] = '.';
  id[260] = 0;
  g_assert_false (g_application_id_is_valid (id));
  g_free (id);

  g_assert_true (g_application_id_is_valid ("a.b"));
  g_assert_true (g_application_id_is_valid ("A.B"));
  g_assert_true (g_application_id_is_valid ("A-.B"));
  g_assert_true (g_application_id_is_valid ("a_b.c-d"));
  g_assert_true (g_application_id_is_valid ("_a.b"));
  g_assert_true (g_application_id_is_valid ("-a.b"));
  g_assert_true (g_application_id_is_valid ("org.gnome.SessionManager"));
  g_assert_true (g_application_id_is_valid ("my.Terminal._0123"));
  g_assert_true (g_application_id_is_valid ("com.example.MyApp"));
  g_assert_true (g_application_id_is_valid ("com.example.internal_apps.Calculator"));
  g_assert_true (g_application_id_is_valid ("org._7_zip.Archiver"));
}

static gboolean nodbus_activated;

static gboolean
release_app (gpointer user_data)
{
  g_application_release (user_data);
  return G_SOURCE_REMOVE;
}

static void
nodbus_activate (GApplication *app)
{
  nodbus_activated = TRUE;
  g_application_hold (app);

  g_assert (g_application_get_dbus_connection (app) == NULL);
  g_assert (g_application_get_dbus_object_path (app) == NULL);

  g_idle_add (release_app, app);
}

static void
test_nodbus (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  gchar *argv[] = { binpath, NULL };
  GApplication *app;

  app = g_application_new ("org.gtk.Unimportant", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (nodbus_activate), NULL);
  g_application_run (app, 1, argv);
  g_object_unref (app);

  g_assert (nodbus_activated);
  g_free (binpath);
}

static gboolean noappid_activated;

static void
noappid_activate (GApplication *app)
{
  noappid_activated = TRUE;
  g_application_hold (app);

  g_assert (g_application_get_flags (app) & G_APPLICATION_NON_UNIQUE);

  g_idle_add (release_app, app);
}

/* test that no appid -> non-unique */
static void
test_noappid (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  gchar *argv[] = { binpath, NULL };
  GApplication *app;

  app = g_application_new (NULL, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (noappid_activate), NULL);
  g_application_run (app, 1, argv);
  g_object_unref (app);

  g_assert (noappid_activated);
  g_free (binpath);
}

static gboolean activated;
static gboolean quitted;

static gboolean
quit_app (gpointer user_data)
{
  quitted = TRUE;
  g_application_quit (user_data);
  return G_SOURCE_REMOVE;
}

static void
quit_activate (GApplication *app)
{
  activated = TRUE;
  g_application_hold (app);

  g_assert (g_application_get_dbus_connection (app) != NULL);
  g_assert (g_application_get_dbus_object_path (app) != NULL);

  g_idle_add (quit_app, app);
}

static void
test_quit (void)
{
  GDBusConnection *c;
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  gchar *argv[] = { binpath, NULL };
  GApplication *app;

  session_bus_up ();
  c = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  app = g_application_new ("org.gtk.Unimportant",
                           G_APPLICATION_DEFAULT_FLAGS);
  activated = FALSE;
  quitted = FALSE;
  g_signal_connect (app, "activate", G_CALLBACK (quit_activate), NULL);
  g_application_run (app, 1, argv);
  g_object_unref (app);
  g_object_unref (c);

  g_assert (activated);
  g_assert (quitted);

  session_bus_down ();
  g_free (binpath);
}

typedef struct
{
  gboolean shutdown;
  GParamSpec *notify_spec; /* (owned) (nullable) */
} RegisteredData;

static void
on_registered_shutdown (GApplication *app,
                        gpointer user_data)
{
  RegisteredData *registered_data = user_data;

  registered_data->shutdown = TRUE;
}

static void
on_registered_notify (GApplication *app,
                      GParamSpec *spec,
                      gpointer user_data)
{
  RegisteredData *registered_data = user_data;
  registered_data->notify_spec = g_param_spec_ref (spec);

  if (g_application_get_is_registered (app))
    g_assert_false (registered_data->shutdown);
  else
    g_assert_true (registered_data->shutdown);
}

static void
test_registered (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  gchar *argv[] = { binpath, NULL };
  RegisteredData registered_data = { FALSE, NULL };
  GApplication *app;

  app = g_application_new (NULL, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (noappid_activate), NULL);
  g_signal_connect (app, "shutdown", G_CALLBACK (on_registered_shutdown), &registered_data);
  g_signal_connect (app, "notify::is-registered", G_CALLBACK (on_registered_notify), &registered_data);

  g_assert_null (registered_data.notify_spec);

  g_assert_true (g_application_register (app, NULL, NULL));
  g_assert_true (g_application_get_is_registered (app));

  g_assert_nonnull (registered_data.notify_spec);
  g_assert_cmpstr (registered_data.notify_spec->name, ==, "is-registered");
  g_clear_pointer (&registered_data.notify_spec, g_param_spec_unref);

  g_assert_false (registered_data.shutdown);

  g_application_run (app, 1, argv);

  g_assert_true (registered_data.shutdown);
  g_assert_false (g_application_get_is_registered (app));
  g_assert_nonnull (registered_data.notify_spec);
  g_assert_cmpstr (registered_data.notify_spec->name, ==, "is-registered");
  g_clear_pointer (&registered_data.notify_spec, g_param_spec_unref);

  /* Register it again */
  registered_data.shutdown = FALSE;
  g_assert_true (g_application_register (app, NULL, NULL));
  g_assert_true (g_application_get_is_registered (app));
  g_assert_nonnull (registered_data.notify_spec);
  g_assert_cmpstr (registered_data.notify_spec->name, ==, "is-registered");
  g_clear_pointer (&registered_data.notify_spec, g_param_spec_unref);
  g_assert_false (registered_data.shutdown);

  g_object_unref (app);

  g_free (binpath);
}

static void
on_activate (GApplication *app)
{
  gchar **actions;
  GAction *action;
  GVariant *state;

  g_assert (!g_application_get_is_remote (app));

  actions = g_action_group_list_actions (G_ACTION_GROUP (app));
  g_assert (g_strv_length (actions) == 0);
  g_strfreev (actions);

  action = (GAction*)g_simple_action_new_stateful ("test", G_VARIANT_TYPE_BOOLEAN, g_variant_new_boolean (FALSE));
  g_action_map_add_action (G_ACTION_MAP (app), action);

  actions = g_action_group_list_actions (G_ACTION_GROUP (app));
  g_assert (g_strv_length (actions) == 1);
  g_strfreev (actions);

  g_action_group_change_action_state (G_ACTION_GROUP (app), "test", g_variant_new_boolean (TRUE));
  state = g_action_group_get_action_state (G_ACTION_GROUP (app), "test");
  g_assert (g_variant_get_boolean (state) == TRUE);

  action = g_action_map_lookup_action (G_ACTION_MAP (app), "test");
  g_assert (action != NULL);

  g_action_map_remove_action (G_ACTION_MAP (app), "test");

  actions = g_action_group_list_actions (G_ACTION_GROUP (app));
  g_assert (g_strv_length (actions) == 0);
  g_strfreev (actions);
}

static void
test_local_actions (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  gchar *argv[] = { binpath, NULL };
  GApplication *app;

  app = g_application_new ("org.gtk.Unimportant",
                           G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);
  g_application_run (app, 1, argv);
  g_object_unref (app);
  g_free (binpath);
}

typedef GApplication TestLocCmdApp;
typedef GApplicationClass TestLocCmdAppClass;

static GType test_loc_cmd_app_get_type (void);
G_DEFINE_TYPE (TestLocCmdApp, test_loc_cmd_app, G_TYPE_APPLICATION)

static void
test_loc_cmd_app_init (TestLocCmdApp *app)
{
}

static void
test_loc_cmd_app_startup (GApplication *app)
{
  g_assert_not_reached ();
}

static void
test_loc_cmd_app_shutdown (GApplication *app)
{
  g_assert_not_reached ();
}

static gboolean
test_loc_cmd_app_local_command_line (GApplication   *application,
                                     gchar        ***arguments,
                                     gint           *exit_status)
{
  return TRUE;
}

static void
test_loc_cmd_app_class_init (TestLocCmdAppClass *klass)
{
  G_APPLICATION_CLASS (klass)->startup = test_loc_cmd_app_startup;
  G_APPLICATION_CLASS (klass)->shutdown = test_loc_cmd_app_shutdown;
  G_APPLICATION_CLASS (klass)->local_command_line = test_loc_cmd_app_local_command_line;
}

static void
test_local_command_line (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  gchar *argv[] = { binpath, "-invalid", NULL };
  GApplication *app;

  app = g_object_new (test_loc_cmd_app_get_type (),
                      "application-id", "org.gtk.Unimportant",
                      "flags", G_APPLICATION_DEFAULT_FLAGS,
                      NULL);
  g_application_run (app, 1, argv);
  g_object_unref (app);
  g_free (binpath);
}

static void
test_resource_path (void)
{
  GApplication *app;

  app = g_application_new ("x.y.z", 0);
  g_assert_cmpstr (g_application_get_resource_base_path (app), ==, "/x/y/z");

  /* this should not change anything */
  g_application_set_application_id (app, "a.b.c");
  g_assert_cmpstr (g_application_get_resource_base_path (app), ==, "/x/y/z");

  /* but this should... */
  g_application_set_resource_base_path (app, "/x");
  g_assert_cmpstr (g_application_get_resource_base_path (app), ==, "/x");

  /* ... and this */
  g_application_set_resource_base_path (app, NULL);
  g_assert_cmpstr (g_application_get_resource_base_path (app), ==, NULL);

  g_object_unref (app);

  /* Make sure that overriding at construction time works properly */
  app = g_object_new (G_TYPE_APPLICATION, "application-id", "x.y.z", "resource-base-path", "/a", NULL);
  g_assert_cmpstr (g_application_get_resource_base_path (app), ==, "/a");
  g_object_unref (app);

  /* ... particularly if we override to NULL */
  app = g_object_new (G_TYPE_APPLICATION, "application-id", "x.y.z", "resource-base-path", NULL, NULL);
  g_assert_cmpstr (g_application_get_resource_base_path (app), ==, NULL);
  g_object_unref (app);
}

static gint
test_help_command_line (GApplication            *app,
                        GApplicationCommandLine *command_line,
                        gpointer                 user_data)
{
  gboolean *called = user_data;

  *called = TRUE;

  return 0;
}

/* Test whether --help is handled when HANDLES_COMMND_LINE is set and
 * options have been added.
 */
static void
test_help (void)
{
  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      gchar *argv[] = { binpath, "--help", NULL };
      GApplication *app;
      gboolean called = FALSE;
      int status;

      app = g_application_new ("org.gtk.TestApplication", G_APPLICATION_HANDLES_COMMAND_LINE);
      g_application_add_main_option (app, "foo", 'f', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      g_signal_connect (app, "command-line", G_CALLBACK (test_help_command_line), &called);

      status = g_application_run (app, G_N_ELEMENTS (argv) -1, argv);
      g_assert (called == TRUE);
      g_assert_cmpint (status, ==, 0);

      g_object_unref (app);
      g_free (binpath);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stdout ("*Application options*");
}

static gint
command_line_done_callback (GApplication            *app,
                            GApplicationCommandLine *command_line,
                            gpointer                 user_data)
{
  gboolean *called = user_data;

  *called = TRUE;

  g_application_command_line_set_exit_status (command_line, 42);
  g_application_command_line_done (command_line);

  return 0;
}

/* Test whether 'command-line' handler return value is ignored
 * after g_application_command_line_done()
 */
static void
test_command_line_done (void)
{
  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  const gchar *const argv[] = { binpath, "arg", NULL };
  GApplication *app;
  gboolean called = FALSE;
  int status;
  gulong command_line_id;

  app = g_application_new ("org.gtk.TestApplication", G_APPLICATION_HANDLES_COMMAND_LINE);
  command_line_id = g_signal_connect (app, "command-line", G_CALLBACK (command_line_done_callback), &called);

  status = g_application_run (app, G_N_ELEMENTS (argv) - 1, (gchar **) argv);

  g_signal_handler_disconnect (app, command_line_id);
  g_object_unref (app);
  g_free (binpath);

  g_assert_true (called);
  g_assert_cmpint (status, ==, 42);
}

typedef GApplication TestCommandLineArgumentsApp;
typedef GApplicationClass TestCommandLineArgumentsAppClass;

static GType test_command_line_arguments_app_get_type (void);
G_DEFINE_TYPE (TestCommandLineArgumentsApp, test_command_line_arguments_app, G_TYPE_APPLICATION)

static void
test_command_line_arguments_app_init (TestCommandLineArgumentsApp *app)
{
}

static gboolean
test_command_line_arguments_app_local_command_line (GApplication   *application,
                                                    char         ***arguments,
                                                    int            *exit_status)
{
  /* Pretend we couldn’t parse the arguments */
  return FALSE;
}

static void
test_command_line_arguments_app_class_init (TestCommandLineArgumentsAppClass *klass)
{
  G_APPLICATION_CLASS (klass)->local_command_line = test_command_line_arguments_app_local_command_line;
}

static gint
command_line_arguments_callback (GApplication            *app,
                                 GApplicationCommandLine *command_line,
                                 void                    *user_data)
{
  char ***out_args = user_data;
  int argc;

  *out_args = g_application_command_line_get_arguments (command_line, &argc);
  g_assert_cmpint (argc, ==, g_strv_length (*out_args));

  return 42;  /* exit status */
}

static void
test_command_line_arguments (void)
{
  g_test_summary ("Test HANDLES_COMMAND_LINE locally with a ->local_command_line "
                  "vfunc which forces g_application_run() to take a fallback error handling path");

  char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
  const char *const argv[] = { binpath, "spam", "eggs", NULL };
  GApplication *app;
  int status;
  unsigned long command_line_id;
  char **args = NULL;

  app = g_object_new (test_command_line_arguments_app_get_type (),
                      "application-id", "org.gtk.TestApplication",
                      "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                      NULL);
  command_line_id = g_signal_connect (app, "command-line", G_CALLBACK (command_line_arguments_callback), &args);

  status = g_application_run (app, G_N_ELEMENTS (argv) - 1, (char **) argv);

  g_assert_cmpint (status, ==, 42);
  g_assert_cmpstrv (args, argv);

  g_signal_handler_disconnect (app, command_line_id);
  g_object_unref (app);
  g_free (binpath);
  g_strfreev (args);
}

static void
test_busy (void)
{
  GApplication *app;

  /* use GSimpleAction to bind to the busy state, because it's easy to
   * create and has an easily modifiable boolean property */
  GSimpleAction *action1;
  GSimpleAction *action2;

  session_bus_up ();

  app = g_application_new ("org.gtk.TestApplication", G_APPLICATION_NON_UNIQUE);
  g_assert (g_application_register (app, NULL, NULL));

  g_assert (!g_application_get_is_busy (app));
  g_application_mark_busy (app);
  g_assert (g_application_get_is_busy (app));
  g_application_unmark_busy (app);
  g_assert (!g_application_get_is_busy (app));

  action1 = g_simple_action_new ("action", NULL);
  g_application_bind_busy_property (app, action1, "enabled");
  g_assert (g_application_get_is_busy (app));

  g_simple_action_set_enabled (action1, FALSE);
  g_assert (!g_application_get_is_busy (app));

  g_application_mark_busy (app);
  g_assert (g_application_get_is_busy (app));

  action2 = g_simple_action_new ("action", NULL);
  g_application_bind_busy_property (app, action2, "enabled");
  g_assert (g_application_get_is_busy (app));

  g_application_unmark_busy (app);
  g_assert (g_application_get_is_busy (app));

  g_object_unref (action2);
  g_assert (!g_application_get_is_busy (app));

  g_simple_action_set_enabled (action1, TRUE);
  g_assert (g_application_get_is_busy (app));

  g_application_mark_busy (app);
  g_assert (g_application_get_is_busy (app));

  g_application_unbind_busy_property (app, action1, "enabled");
  g_assert (g_application_get_is_busy (app));

  g_application_unmark_busy (app);
  g_assert (!g_application_get_is_busy (app));

  g_object_unref (action1);
  g_object_unref (app);

  session_bus_down ();
}

/*
 * Test that handle-local-options works as expected
 */

static gint
test_local_options (GApplication *app,
                    GVariantDict *options,
                    gpointer      data)
{
  gboolean *called = data;

  *called = TRUE;

  if (g_variant_dict_contains (options, "success"))
    return 0;
  else if (g_variant_dict_contains (options, "failure"))
    return 1;
  else
    return -1;
}

static gint
second_handler (GApplication *app,
                GVariantDict *options,
                gpointer      data)
{
  gboolean *called = data;

  *called = TRUE;

  return 2;
}

static void
test_handle_local_options_success (void)
{
  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      gchar *argv[] = { binpath, "--success", NULL };
      GApplication *app;
      gboolean called = FALSE;
      gboolean called2 = FALSE;
      int status;

      app = g_application_new ("org.gtk.TestApplication", 0);
      g_application_add_main_option (app, "success", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      g_application_add_main_option (app, "failure", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      g_signal_connect (app, "handle-local-options", G_CALLBACK (test_local_options), &called);
      g_signal_connect (app, "handle-local-options", G_CALLBACK (second_handler), &called2);

      status = g_application_run (app, G_N_ELEMENTS (argv) -1, argv);
      g_assert (called);
      g_assert (!called2);
      g_assert_cmpint (status, ==, 0);

      g_object_unref (app);
      g_free (binpath);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
}

static void
test_handle_local_options_failure (void)
{
  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      gchar *argv[] = { binpath, "--failure", NULL };
      GApplication *app;
      gboolean called = FALSE;
      gboolean called2 = FALSE;
      int status;

      app = g_application_new ("org.gtk.TestApplication", 0);
      g_application_add_main_option (app, "success", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      g_application_add_main_option (app, "failure", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      g_signal_connect (app, "handle-local-options", G_CALLBACK (test_local_options), &called);
      g_signal_connect (app, "handle-local-options", G_CALLBACK (second_handler), &called2);

      status = g_application_run (app, G_N_ELEMENTS (argv) -1, argv);
      g_assert (called);
      g_assert (!called2);
      g_assert_cmpint (status, ==, 1);

      g_object_unref (app);
      g_free (binpath);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
}

static void
test_handle_local_options_passthrough (void)
{
  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      gchar *argv[] = { binpath, NULL };
      GApplication *app;
      gboolean called = FALSE;
      gboolean called2 = FALSE;
      int status;

      app = g_application_new ("org.gtk.TestApplication", 0);
      g_application_add_main_option (app, "success", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      g_application_add_main_option (app, "failure", 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE, "", "");
      g_signal_connect (app, "handle-local-options", G_CALLBACK (test_local_options), &called);
      g_signal_connect (app, "handle-local-options", G_CALLBACK (second_handler), &called2);

      status = g_application_run (app, G_N_ELEMENTS (argv) -1, argv);
      g_assert (called);
      g_assert (called2);
      g_assert_cmpint (status, ==, 2);

      g_object_unref (app);
      g_free (binpath);
      return;
    }

  g_test_trap_subprocess (NULL, 0, G_TEST_SUBPROCESS_INHERIT_STDOUT | G_TEST_SUBPROCESS_INHERIT_STDERR);
  g_test_trap_assert_passed ();
}

static void
test_api (void)
{
  GApplication *app;
  GSimpleAction *action;

  app = g_application_new ("org.gtk.TestApplication", 0);

  /* add an action without a name */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*assertion*failed*");
  action = g_simple_action_new (NULL, NULL);
  g_assert (action == NULL);
  g_test_assert_expected_messages ();

  /* also, gapplication shouldn't accept actions without names */
  action = g_object_new (G_TYPE_SIMPLE_ACTION, NULL);
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL, "*action has no name*");
  g_action_map_add_action (G_ACTION_MAP (app), G_ACTION (action));
  g_test_assert_expected_messages ();

  g_object_unref (action);
  g_object_unref (app);
}

static void
test_version (void)
{
  GApplication *app;
  gchar *version = NULL;
  gchar *version_orig = NULL;

  app = g_application_new ("org.gtk.TestApplication", 0);

  version_orig = "1.2";
  g_object_set (G_OBJECT (app), "version", version_orig, NULL);
  g_object_get (app, "version", &version, NULL);
  g_assert_cmpstr (version, ==, version_orig);
  g_free (version);

  /* test attempting to set the same version again */
  version_orig = "1.2";
  g_object_set (G_OBJECT (app), "version", version_orig, NULL);
  g_object_get (app, "version", &version, NULL);
  g_assert_cmpstr (version, ==, version_orig);
  g_free (version);

  version_orig = "2.4";
  g_object_set (G_OBJECT (app), "version", version_orig, NULL);
  g_object_get (app, "version", &version, NULL);
  g_assert_cmpstr (version, ==, version_orig);
  g_free (version);

  g_object_unref (app);
}

/* Check that G_APPLICATION_ALLOW_REPLACEMENT works. To do so, we launch
 * a GApplication in this process that allows replacement, and then
 * launch a subprocess with --gapplication-replace. We have to do our
 * own async version of g_test_trap_subprocess() here since we need
 * the main process to keep spinning its mainloop.
 */

static gboolean
name_was_lost (GApplication *app,
               gboolean     *called)
{
  *called = TRUE;
  g_application_quit (app);
  return TRUE;
}

static void
startup_in_subprocess (GApplication *app,
                       gboolean     *called)
{
  *called = TRUE;
}

typedef struct
{
  gboolean allow_replacement;
  GSubprocess *subprocess;
  GApplication *app;  /* (not owned) */
  guint timeout_id;
} TestReplaceData;

static void
startup_cb (GApplication *app,
            TestReplaceData *data)
{
  const char *argv[] = { NULL, "--verbose", "--quiet", "-p", NULL, "--GTestSubprocess", NULL };
  GSubprocessLauncher *launcher;
  GError *local_error = NULL;

  g_application_hold (app);

  argv[0] = g_get_prgname ();

  if (data->allow_replacement)
    argv[4] = "/gapplication/replace";
  else
    argv[4] = "/gapplication/no-replace";

  /* Now that we are the primary instance, launch our replacement.
   * We inherit the environment to share the test session bus.
   */
  g_test_message ("launching subprocess");

  launcher = g_subprocess_launcher_new (G_SUBPROCESS_FLAGS_NONE);
  g_subprocess_launcher_set_environ (launcher, NULL);
  data->subprocess = g_subprocess_launcher_spawnv (launcher, argv, &local_error);
  g_assert_no_error (local_error);
  g_object_unref (launcher);

  if (!data->allow_replacement)
    {
      /* make sure we exit after a bit, if the subprocess is not replacing us */
      g_application_set_inactivity_timeout (app, 500);
      g_application_release (app);
    }
}

static void
activate (gpointer data)
{
  /* GApplication complains if we don't connect to ::activate */
}

static void
quit_already (gpointer user_data)
{
  TestReplaceData *data = user_data;

  g_application_quit (data->app);
  data->timeout_id = 0;
}

static void
test_replace (gconstpointer data)
{
  gboolean allow = GPOINTER_TO_INT (data);

  if (g_test_subprocess ())
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      char *argv[] = { binpath, "--gapplication-replace", NULL };
      GApplication *app;
      gboolean startup = FALSE;

      app = g_application_new ("org.gtk.TestApplication.Replace", G_APPLICATION_ALLOW_REPLACEMENT);
      g_signal_connect (app, "startup", G_CALLBACK (startup_in_subprocess), &startup);
      g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

      g_application_run (app, G_N_ELEMENTS (argv) - 1, argv);

      if (allow)
        g_assert_true (startup);
      else
        g_assert_false (startup);

      g_object_unref (app);
      g_free (binpath);
    }
  else
    {
      char *binpath = g_test_build_filename (G_TEST_BUILT, "unimportant", NULL);
      gchar *argv[] = { binpath, NULL };
      GApplication *app;
      gboolean name_lost = FALSE;
      TestReplaceData data;
      GTestDBus *bus;

      data.allow_replacement = allow;
      data.subprocess = NULL;
      data.timeout_id = 0;

      bus = g_test_dbus_new (0);
      g_test_dbus_up (bus);

      app = data.app = g_application_new ("org.gtk.TestApplication.Replace", allow ? G_APPLICATION_ALLOW_REPLACEMENT : G_APPLICATION_DEFAULT_FLAGS);
      g_application_set_inactivity_timeout (app, 500);
      g_signal_connect (app, "name-lost", G_CALLBACK (name_was_lost), &name_lost);
      g_signal_connect (app, "startup", G_CALLBACK (startup_cb), &data);
      g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

      if (!allow)
        data.timeout_id = g_timeout_add_seconds_once (1, quit_already, &data);

      g_application_run (app, G_N_ELEMENTS (argv) - 1, argv);

      g_assert_nonnull (data.subprocess);
      if (allow)
        g_assert_true (name_lost);
      else
        g_assert_false (name_lost);

      g_clear_handle_id (&data.timeout_id, g_source_remove);
      g_object_unref (app);
      g_free (binpath);

      g_subprocess_wait (data.subprocess, NULL, NULL);
      g_clear_object (&data.subprocess);

      g_test_dbus_down (bus);
      g_object_unref (bus);
    }
}

static void
dbus_activate_cb (GApplication *app,
                  gpointer      user_data)
{
  guint *n_activations = user_data;

  *n_activations = *n_activations + 1;
  g_main_context_wakeup (NULL);
}

static void dbus_startup_reply_cb (GObject      *source_object,
                                   GAsyncResult *result,
                                   gpointer      user_data);
static gboolean dbus_startup_reply_idle_cb (gpointer user_data);

static void
dbus_startup_cb (GApplication *app,
                 gpointer      user_data)
{
  GDBusConnection *connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  GDBusMessage *message = G_DBUS_MESSAGE (user_data);

  g_assert_nonnull (connection);

  g_dbus_connection_send_message_with_reply (connection, message,
                                             G_DBUS_SEND_MESSAGE_FLAGS_NONE, -1,
                                             NULL, NULL,
                                             dbus_startup_reply_cb, g_object_ref (app));

  g_clear_object (&connection);
}

static void
dbus_startup_reply_cb (GObject      *source_object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
  GDBusConnection *connection = G_DBUS_CONNECTION (source_object);
  GApplication *app = G_APPLICATION (user_data);
  GDBusMessage *reply = NULL;
  GError *local_error = NULL;

  reply = g_dbus_connection_send_message_with_reply_finish (connection, result, &local_error);
  g_assert_no_error (local_error);

  g_object_set_data_full (G_OBJECT (app), "dbus-command-line-reply", g_steal_pointer (&reply), g_object_unref);

  /* Release the app in an idle callback, so there’s time to process other
   * pending sources first. */
  g_idle_add_full (G_PRIORITY_LOW, dbus_startup_reply_idle_cb, g_steal_pointer (&app), g_object_unref);
}

static gboolean
dbus_startup_reply_idle_cb (gpointer user_data)
{
  GApplication *app = G_APPLICATION (user_data);

  g_application_release (app);

  return G_SOURCE_REMOVE;
}

static void
test_dbus_activate (void)
{
  GTestDBus *bus = NULL;
  GVariantBuilder builder;
  GDBusMessage *message = NULL;
  GPtrArray *messages = NULL;  /* (element-type GDBusMessage) (owned) */
  gsize i;

  g_test_summary ("Test that calling the Activate D-Bus method works");

  /* Try various different messages */
  messages = g_ptr_array_new_with_free_func (g_object_unref);

  /* Via org.gtk.Application */
  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.Activate",
                                            "/org/gtk/TestApplication/Activate",
                                            "org.gtk.Application",
                                            "Activate");
  g_dbus_message_set_body (message, g_variant_new ("(a{sv})", NULL));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* Via org.freedesktop.Application */
  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.Activate",
                                            "/org/gtk/TestApplication/Activate",
                                            "org.freedesktop.Application",
                                            "Activate");
  g_dbus_message_set_body (message, g_variant_new ("(a{sv})", NULL));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* With some platform data */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&builder, "{sv}", "cwd", g_variant_new_bytestring ("/home/henry"));

  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.Activate",
                                            "/org/gtk/TestApplication/Activate",
                                            "org.gtk.Application",
                                            "Activate");
  g_dbus_message_set_body (message, g_variant_new ("(a{sv})", &builder));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* Try each message */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  for (i = 0; i < messages->len; i++)
    {
      GApplication *app = NULL;
      gulong activate_id, startup_id;
      guint n_activations = 0;

      g_test_message ("Message %" G_GSIZE_FORMAT, i);

      app = g_application_new ("org.gtk.TestApplication.Activate", G_APPLICATION_DEFAULT_FLAGS);
      activate_id = g_signal_connect (app, "activate", G_CALLBACK (dbus_activate_cb), &n_activations);
      startup_id = g_signal_connect (app, "startup", G_CALLBACK (dbus_startup_cb), messages->pdata[i]);

      g_application_hold (app);
      g_application_run (app, 0, NULL);

      /* It’ll be activated once as normal, and once due to the D-Bus call */
      g_assert_cmpuint (n_activations, ==, 2);

      g_signal_handler_disconnect (app, startup_id);
      g_signal_handler_disconnect (app, activate_id);
      g_clear_object (&app);
    }

  g_ptr_array_unref (messages);

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

static void
dbus_activate_noop_cb (GApplication *app,
                       gpointer      user_data)
{
  /* noop */
}

static void
dbus_open_cb (GApplication *app,
              gpointer      files,
              int           n_files,
              char         *hint,
              gpointer      user_data)
{
  guint *n_opens = user_data;

  *n_opens = *n_opens + 1;
  g_main_context_wakeup (NULL);
}

static void
test_dbus_open (void)
{
  GTestDBus *bus = NULL;
  GVariantBuilder builder, builder2;
  GDBusMessage *message = NULL;
  GPtrArray *messages = NULL;  /* (element-type GDBusMessage) (owned) */
  gsize i;

  g_test_summary ("Test that calling the Open D-Bus method works");

  /* Try various different messages */
  messages = g_ptr_array_new_with_free_func (g_object_unref);

  /* Via org.gtk.Application */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("as"));
  g_variant_builder_add (&builder, "s", "file:///home/henry/test");

  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.Open",
                                            "/org/gtk/TestApplication/Open",
                                            "org.gtk.Application",
                                            "Open");
  g_dbus_message_set_body (message, g_variant_new ("(assa{sv})", &builder, "hint", NULL));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* Via org.freedesktop.Application (which has no hint parameter) */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("as"));
  g_variant_builder_add (&builder, "s", "file:///home/henry/test");

  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.Open",
                                            "/org/gtk/TestApplication/Open",
                                            "org.freedesktop.Application",
                                            "Open");
  g_dbus_message_set_body (message, g_variant_new ("(asa{sv})", &builder, NULL));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* With some platform data and more than one file */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("as"));
  g_variant_builder_add (&builder, "s", "file:///home/henry/test");
  g_variant_builder_add (&builder, "s", "file:///home/henry/test2");

  g_variant_builder_init_static (&builder2, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&builder2, "{sv}", "cwd", g_variant_new_bytestring ("/home/henry"));

  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.Open",
                                            "/org/gtk/TestApplication/Open",
                                            "org.gtk.Application",
                                            "Open");
  g_dbus_message_set_body (message, g_variant_new ("(assa{sv})", &builder, "", &builder2));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* No files */
  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.Open",
                                            "/org/gtk/TestApplication/Open",
                                            "org.gtk.Application",
                                            "Open");
  g_dbus_message_set_body (message, g_variant_new ("(assa{sv})", NULL, "", NULL));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* Try each message */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  for (i = 0; i < messages->len; i++)
    {
      GApplication *app = NULL;
      gulong activate_id, open_id, startup_id;
      guint n_opens = 0;

      g_test_message ("Message %" G_GSIZE_FORMAT, i);

      app = g_application_new ("org.gtk.TestApplication.Open", G_APPLICATION_HANDLES_OPEN);
      activate_id = g_signal_connect (app, "activate", G_CALLBACK (dbus_activate_noop_cb), NULL);
      open_id = g_signal_connect (app, "open", G_CALLBACK (dbus_open_cb), &n_opens);
      startup_id = g_signal_connect (app, "startup", G_CALLBACK (dbus_startup_cb), messages->pdata[i]);

      g_application_hold (app);
      g_application_run (app, 0, NULL);

      g_assert_cmpuint (n_opens, ==, 1);

      g_signal_handler_disconnect (app, startup_id);
      g_signal_handler_disconnect (app, open_id);
      g_signal_handler_disconnect (app, activate_id);
      g_clear_object (&app);
    }

  g_ptr_array_unref (messages);

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

static void
dbus_command_line_cb (GApplication            *app,
                      GApplicationCommandLine *command_line,
                      gpointer                 user_data)
{
  guint *n_command_lines = user_data;

  *n_command_lines = *n_command_lines + 1;
  g_main_context_wakeup (NULL);
}

static void
test_dbus_command_line (void)
{
  GTestDBus *bus = NULL;
  GVariantBuilder builder, builder2;
  GDBusMessage *message = NULL;
  GPtrArray *messages = NULL;  /* (element-type GDBusMessage) (owned) */
  gsize i;

  g_test_summary ("Test that calling the CommandLine D-Bus method works");

  /* Try various different messages */
  messages = g_ptr_array_new_with_free_func (g_object_unref);

  /* Via org.gtk.Application */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("aay"));
  g_variant_builder_add (&builder, "^ay", "test-program");
  g_variant_builder_add (&builder, "^ay", "--open");
  g_variant_builder_add (&builder, "^ay", "/path/to/something");

  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.CommandLine",
                                            "/org/gtk/TestApplication/CommandLine",
                                            "org.gtk.Application",
                                            "CommandLine");
  g_dbus_message_set_body (message, g_variant_new ("(oaaya{sv})",
                                                   "/my/org/gtk/private/CommandLine",
                                                   &builder, NULL));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* With platform data */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("aay"));
  g_variant_builder_add (&builder, "^ay", "test-program");
  g_variant_builder_add (&builder, "^ay", "--open");
  g_variant_builder_add (&builder, "^ay", "/path/to/something");

  g_variant_builder_init_static (&builder2, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&builder2, "{sv}", "cwd", g_variant_new_bytestring ("/home"));
  g_variant_builder_add_parsed (&builder2, "{'environ', <@aay [ b'HOME=/home/bloop', b'PATH=/blah']>}");
  g_variant_builder_add_parsed (&builder2, "{'options', <{'a': <@u 32>, 'b': <'bloop'>}>}");

  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.CommandLine",
                                            "/org/gtk/TestApplication/CommandLine",
                                            "org.gtk.Application",
                                            "CommandLine");
  g_dbus_message_set_body (message, g_variant_new ("(oaaya{sv})",
                                                   "/my/org/gtk/private/CommandLine",
                                                   &builder, &builder2));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* With invalid typed platform data */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("aay"));
  g_variant_builder_add (&builder, "^ay", "test-program");
  g_variant_builder_add (&builder, "^ay", "--open");
  g_variant_builder_add (&builder, "^ay", "/path/to/something");

  g_variant_builder_init_static (&builder2, G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (&builder2, "{sv}", "cwd", g_variant_new_string ("/home should be a bytestring"));
  g_variant_builder_add_parsed (&builder2, "{'environ', <['HOME=should be a bytestring', 'PATH=this also']>}");
  g_variant_builder_add_parsed (&builder2, "{'options', <['should be a', 'dict']>}");

  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.CommandLine",
                                            "/org/gtk/TestApplication/CommandLine",
                                            "org.gtk.Application",
                                            "CommandLine");
  g_dbus_message_set_body (message, g_variant_new ("(oaaya{sv})",
                                                   "/my/org/gtk/private/CommandLine",
                                                   &builder, &builder2));
  g_ptr_array_add (messages, g_steal_pointer (&message));

  /* Try each message */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  for (i = 0; i < messages->len; i++)
    {
      GApplication *app = NULL;
      gulong activate_id, command_line_id, startup_id;
      guint n_command_lines = 0;

      g_test_message ("Message %" G_GSIZE_FORMAT, i);

      app = g_application_new ("org.gtk.TestApplication.CommandLine", G_APPLICATION_HANDLES_COMMAND_LINE);
      activate_id = g_signal_connect (app, "activate", G_CALLBACK (dbus_activate_noop_cb), NULL);
      command_line_id = g_signal_connect (app, "command-line", G_CALLBACK (dbus_command_line_cb), &n_command_lines);
      startup_id = g_signal_connect (app, "startup", G_CALLBACK (dbus_startup_cb), messages->pdata[i]);

      g_application_hold (app);
      g_application_run (app, 0, NULL);

      /* It’s called once for handling the local command line on startup, and again
       * for the D-Bus call */
      g_assert_cmpuint (n_command_lines, ==, 2);

      g_signal_handler_disconnect (app, startup_id);
      g_signal_handler_disconnect (app, command_line_id);
      g_signal_handler_disconnect (app, activate_id);
      g_clear_object (&app);
    }

  g_ptr_array_unref (messages);

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

static gint
dbus_command_line_done_cb (GApplication            *app,
                           GApplicationCommandLine *command_line,
                           gpointer                 user_data)
{
  guint *n_command_lines = user_data;

  *n_command_lines = *n_command_lines + 1;

  if (*n_command_lines == 1)
    return 0;

  g_object_set_data_full (G_OBJECT (app), "command-line", g_object_ref (command_line), g_object_unref);

  g_application_command_line_set_exit_status (command_line, 42);
  g_application_command_line_done (command_line);

  return 1; /* ignored - after g_application_command_line_done () */
}

static void
test_dbus_command_line_done (void)
{
  GTestDBus *bus = NULL;
  GVariantBuilder builder;
  GDBusMessage *message = NULL;
  GDBusMessage *reply = NULL;
  GApplication *app = NULL;
  guint n_command_lines = 0;
  gint exit_status;

  g_test_summary ("Test that GDBusCommandLine.done() works");

  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("aay"));
  g_variant_builder_add (&builder, "^ay", "test-program");
  g_variant_builder_add (&builder, "^ay", "/path/to/something");

  message = g_dbus_message_new_method_call ("org.gtk.TestApplication.CommandLine",
                                            "/org/gtk/TestApplication/CommandLine",
                                            "org.gtk.Application",
                                            "CommandLine");
  g_dbus_message_set_body (message, g_variant_new ("(oaaya{sv})",
                                                   "/my/org/gtk/private/CommandLine",
                                                   &builder, NULL));

  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  app = g_application_new ("org.gtk.TestApplication.CommandLine", G_APPLICATION_HANDLES_COMMAND_LINE);
  g_signal_connect (app, "activate", G_CALLBACK (dbus_activate_noop_cb), NULL);
  g_signal_connect (app, "command-line", G_CALLBACK (dbus_command_line_done_cb), &n_command_lines);
  g_signal_connect (app, "startup", G_CALLBACK (dbus_startup_cb), message);

  g_application_hold (app);
  exit_status = g_application_run (app, 0, NULL);

  g_assert_cmpuint (n_command_lines, ==, 2);
  g_assert_cmpint (exit_status, ==, 0);

  reply = g_object_get_data (G_OBJECT (app), "dbus-command-line-reply");
  g_variant_get (g_dbus_message_get_body (reply), "(i)", &exit_status);
  g_assert_cmpint (exit_status, ==, 42);

  g_signal_handlers_disconnect_by_func (app, G_CALLBACK (dbus_activate_noop_cb), NULL);
  g_signal_handlers_disconnect_by_func (app, G_CALLBACK (dbus_command_line_done_cb), &n_command_lines);
  g_signal_handlers_disconnect_by_func (app, G_CALLBACK (dbus_startup_cb), message);

  g_clear_object (&app);
  g_clear_object (&message);

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

typedef struct _ActivationData {
  unsigned n_activations;
  GVariant *parameter;
} ActivationData;

static void
dbus_activate_action_cb (GSimpleAction *action,
                         GVariant      *parameter,
                         gpointer       user_data)
{
  ActivationData *activation_data = user_data;

  activation_data->n_activations++;

  if (parameter)
    {
      char *parameter_str = g_variant_print (parameter, TRUE);

      activation_data->parameter = g_variant_ref (parameter);
      g_test_message ("Activating action '%s' with parameter: %s",
                      g_action_get_name (G_ACTION (action)), parameter_str);
      g_clear_pointer (&parameter_str, g_free);
    }
  else
    {
      g_test_message ("Activating action '%s' with no parameter",
                      g_action_get_name (G_ACTION (action)));
    }

  g_main_context_wakeup (NULL);
}

static GVariant *
get_expected_action_parameter (GVariant *parameters)
{
  GVariant *parameter = NULL;
  guint n_children;

  g_assert_true (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("av")));

  if ((n_children = g_variant_n_children (parameters)) > 1)
    {
      GPtrArray *children;

      children = g_ptr_array_new_full (n_children,
                                       (GDestroyNotify) g_variant_unref);

      for (guint i = 0; i < n_children; ++i)
        {
          GVariant *variant;

          variant = g_variant_get_child_value (parameters, i);
          g_ptr_array_add (children, g_variant_get_variant (variant));
          g_clear_pointer (&variant, g_variant_unref);
        }

      parameter = g_variant_new_tuple ((GVariant **) children->pdata, n_children);
      g_clear_pointer (&children, g_ptr_array_unref);
    }
  else if (n_children > 0)
    {
      GVariant *variant;

      variant = g_variant_get_child_value (parameters, 0);
      parameter = g_variant_get_variant (variant);
      g_clear_pointer (&variant, g_variant_unref);
    }

  return g_steal_pointer (&parameter);
}

static void
test_dbus_activate_action (void)
{
  GTestDBus *bus = NULL;
  GVariantBuilder builder;
  GVariant *parameter;
  struct
    {
      GDBusMessage *message;  /* (not nullable) (owned) */
      guint n_expected_activations;
      GVariant *expected_parameter;
    } messages[12] = {0};
  gsize i;

  g_test_summary ("Test that calling the ActivateAction D-Bus method works");

  /* Action without parameter */
  messages[0].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  g_dbus_message_set_body (messages[0].message, g_variant_new ("(sava{sv})", "undo", NULL, NULL));
  messages[0].n_expected_activations = 1;

  /* Action with parameter */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new_string ("spanish"));

  messages[1].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  parameter = g_variant_ref_sink (g_variant_builder_end (&builder));
  g_dbus_message_set_body (messages[1].message, g_variant_new ("(s@ava{sv})", "lang", parameter, NULL));
  messages[1].n_expected_activations = 1;
  messages[1].expected_parameter = get_expected_action_parameter (parameter);
  g_clear_pointer (&parameter, g_variant_unref);

  /* Action with unexpected parameter */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new_string ("should not be passed"));

  messages[2].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  g_dbus_message_set_body (messages[2].message, g_variant_new ("(sava{sv})", "undo", &builder, NULL));
  messages[2].n_expected_activations = 0;
  messages[2].expected_parameter = NULL;

  /* Action without required parameter */
  messages[3].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  g_dbus_message_set_body (messages[3].message, g_variant_new ("(sava{sv})", "lang", NULL, NULL));
  messages[3].n_expected_activations = 0;
  messages[3].expected_parameter = NULL;

  /* Action with wrong parameter type */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new_uint32 (42));

  messages[4].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  g_dbus_message_set_body (messages[4].message, g_variant_new ("(sava{sv})", "lang", &builder, NULL));
  messages[4].n_expected_activations = 0;
  messages[4].expected_parameter = NULL;

  /* Nonexistent action */
  messages[5].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  g_dbus_message_set_body (messages[5].message, g_variant_new ("(sava{sv})", "nonexistent", NULL, NULL));
  messages[5].n_expected_activations = 0;
  messages[5].expected_parameter = NULL;

  /* Action with tuple as parameter given as two items to the interface */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new_string ("first"));
  g_variant_builder_add (&builder, "v", g_variant_new_string ("second"));

  messages[6].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");

  parameter = g_variant_ref_sink (g_variant_builder_end (&builder));
  g_dbus_message_set_body (messages[6].message, g_variant_new ("(s@ava{sv})", "multi", parameter, NULL));
  messages[6].n_expected_activations = 1;
  messages[6].expected_parameter = get_expected_action_parameter (parameter);
  g_clear_pointer (&parameter, g_variant_unref);

  /* Action with tuple as parameter given as two items to the interface but with a wrong type */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new_string ("first"));
  g_variant_builder_add (&builder, "v", g_variant_new_uint32 (42));

  messages[7].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  g_dbus_message_set_body (messages[7].message, g_variant_new ("(sava{sv})", "multi", &builder, NULL));
  messages[7].n_expected_activations = 0;
  messages[7].expected_parameter = NULL;

  /* Action with tuple as parameter given as a single item to the interface */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new ("(ss)", "first", "second"));

  messages[8].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  parameter = g_variant_ref_sink (g_variant_builder_end (&builder));
  g_dbus_message_set_body (messages[8].message, g_variant_new ("(s@ava{sv})", "multi", parameter, NULL));
  messages[8].n_expected_activations = 1;
  messages[8].expected_parameter = get_expected_action_parameter (parameter);
  g_clear_pointer (&parameter, g_variant_unref);

  /* Action with tuple as parameter given as a single item to the interface but with additional items */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new ("(ss)", "first", "second"));
  g_variant_builder_add (&builder, "v", g_variant_new_uint32 (42));
  g_variant_builder_add (&builder, "v", g_variant_new_uint32 (42));

  messages[9].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  g_dbus_message_set_body (messages[9].message, g_variant_new ("(sava{sv})", "multi", &builder, NULL));
  messages[9].n_expected_activations = 0;
  messages[9].expected_parameter = NULL;

  /* Action with tuple with single item as parameter */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new ("(s)", "first"));

  messages[10].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  parameter = g_variant_ref_sink (g_variant_builder_end (&builder));
  g_dbus_message_set_body (messages[10].message, g_variant_new ("(s@ava{sv})", "single", parameter, NULL));
  messages[10].n_expected_activations = 1;
  messages[10].expected_parameter = get_expected_action_parameter (parameter);
  g_clear_pointer (&parameter, g_variant_unref);

  /* Action with tuple with single item as parameter with additional items */
  g_variant_builder_init_static (&builder, G_VARIANT_TYPE ("av"));
  g_variant_builder_add (&builder, "v", g_variant_new ("(s)", "first"));
  g_variant_builder_add (&builder, "v", g_variant_new_uint32 (42));
  g_variant_builder_add (&builder, "v", g_variant_new_uint32 (43));

  messages[11].message = g_dbus_message_new_method_call ("org.gtk.TestApplication.ActivateAction",
                                                        "/org/gtk/TestApplication/ActivateAction",
                                                        "org.freedesktop.Application",
                                                        "ActivateAction");
  g_dbus_message_set_body (messages[11].message, g_variant_new ("(sava{sv})", "single", &builder, NULL));
  messages[11].n_expected_activations = 0;
  messages[11].expected_parameter = NULL;

  /* Try each message */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  for (i = 0; i < G_N_ELEMENTS (messages); i++)
    {
      GApplication *app = NULL;
      gulong activate_id, startup_id;
      const GActionEntry entries[] =
        {
          { "undo", dbus_activate_action_cb, NULL, NULL,      NULL, { 0 } },
          { "lang", dbus_activate_action_cb,  "s",  "'latin'", NULL, { 0 } },
          { "single", dbus_activate_action_cb,  "(s)",  NULL, NULL, { 0 } },
          { "multi", dbus_activate_action_cb,  "(ss)",  NULL, NULL, { 0 } },
        };
      ActivationData activation_data = {0};

      g_test_message ("Message %" G_GSIZE_FORMAT, i);

      app = g_application_new ("org.gtk.TestApplication.ActivateAction", G_APPLICATION_DEFAULT_FLAGS);
      activate_id = g_signal_connect (app, "activate", G_CALLBACK (dbus_activate_noop_cb), NULL);
      startup_id = g_signal_connect (app, "startup", G_CALLBACK (dbus_startup_cb), messages[i].message);

      /* Export some actions. */
      g_action_map_add_action_entries (G_ACTION_MAP (app), entries, G_N_ELEMENTS (entries), &activation_data);

      g_application_hold (app);
      g_application_run (app, 0, NULL);

      g_assert_cmpuint (activation_data.n_activations, ==, messages[i].n_expected_activations);

      if (messages[i].expected_parameter)
        g_assert_cmpvariant (activation_data.parameter, messages[i].expected_parameter);
      else
        g_assert_null (activation_data.parameter);

      g_signal_handler_disconnect (app, startup_id);
      g_signal_handler_disconnect (app, activate_id);
      g_clear_object (&app);
      g_clear_pointer (&activation_data.parameter, g_variant_unref);
      g_clear_pointer (&messages[i].expected_parameter, g_variant_unref);
      g_clear_object (&messages[i].message);
    }

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

int
main (int argc, char **argv)
{
  g_setenv ("LC_ALL", "C", TRUE);

  g_log_writer_default_set_use_stderr (TRUE);

  g_test_init (&argc, &argv, NULL);

  if (!g_test_subprocess ())
    g_test_dbus_unset ();

  g_test_add_func ("/gapplication/no-dbus", test_nodbus);
/*  g_test_add_func ("/gapplication/basic", basic); */
  g_test_add_func ("/gapplication/no-appid", test_noappid);
/*  g_test_add_func ("/gapplication/non-unique", test_nonunique); */
  g_test_add_func ("/gapplication/properties", properties);
  g_test_add_func ("/gapplication/app-id", appid);
  g_test_add_func ("/gapplication/quit", test_quit);
  g_test_add_func ("/gapplication/registered", test_registered);
  g_test_add_func ("/gapplication/local-actions", test_local_actions);
/*  g_test_add_func ("/gapplication/remote-actions", test_remote_actions); */
  g_test_add_func ("/gapplication/local-command-line", test_local_command_line);
/*  g_test_add_func ("/gapplication/remote-command-line", test_remote_command_line); */
  g_test_add_func ("/gapplication/resource-path", test_resource_path);
  g_test_add_func ("/gapplication/test-help", test_help);
  g_test_add_func ("/gapplication/command-line-done", test_command_line_done);
  g_test_add_func ("/gapplication/command-line/arguments", test_command_line_arguments);
  g_test_add_func ("/gapplication/test-busy", test_busy);
  g_test_add_func ("/gapplication/test-handle-local-options1", test_handle_local_options_success);
  g_test_add_func ("/gapplication/test-handle-local-options2", test_handle_local_options_failure);
  g_test_add_func ("/gapplication/test-handle-local-options3", test_handle_local_options_passthrough);
  g_test_add_func ("/gapplication/api", test_api);
  g_test_add_func ("/gapplication/version", test_version);
  g_test_add_data_func ("/gapplication/replace", GINT_TO_POINTER (TRUE), test_replace);
  g_test_add_data_func ("/gapplication/no-replace", GINT_TO_POINTER (FALSE), test_replace);
  g_test_add_func ("/gapplication/dbus/activate", test_dbus_activate);
  g_test_add_func ("/gapplication/dbus/open", test_dbus_open);
  g_test_add_func ("/gapplication/dbus/command-line", test_dbus_command_line);
  g_test_add_func ("/gapplication/dbus/command-line-done", test_dbus_command_line_done);
  g_test_add_func ("/gapplication/dbus/activate-action", test_dbus_activate_action);

  return g_test_run ();
}
