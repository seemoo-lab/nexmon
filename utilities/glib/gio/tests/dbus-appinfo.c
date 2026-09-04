/*
 * Copyright © 2013 Canonical Limited
 * Copyright © 2024 GNOME Foundation Inc.
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
 *
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Julian Sparber <jsparber@gnome.org>
 */

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "gdbus-sessionbus.h"
#include "fake-desktop-portal.h"
#include "fake-document-portal.h"

static GDesktopAppInfo *appinfo;
static int current_state;
static gboolean saw_startup_id;
static gboolean requested_startup_id;


static GType test_app_launch_context_get_type (void);
typedef GAppLaunchContext TestAppLaunchContext;
typedef GAppLaunchContextClass TestAppLaunchContextClass;
G_DEFINE_TYPE (TestAppLaunchContext, test_app_launch_context, G_TYPE_APP_LAUNCH_CONTEXT)

static gchar *
test_app_launch_context_get_startup_notify_id (GAppLaunchContext *context,
                                               GAppInfo          *info,
                                               GList             *uris)
{
  requested_startup_id = TRUE;
  return g_strdup ("expected startup id");
}

static void
test_app_launch_context_init (TestAppLaunchContext *ctx)
{
}

static void
test_app_launch_context_class_init (GAppLaunchContextClass *class)
{
  class->get_startup_notify_id = test_app_launch_context_get_startup_notify_id;
}

static GType test_application_get_type (void);
typedef GApplication TestApplication;
typedef GApplicationClass TestApplicationClass;
G_DEFINE_TYPE (TestApplication, test_application, G_TYPE_APPLICATION)

static void
saw_action (const gchar *action)
{
  /* This is the main driver of the test.  It's a bit of a state
   * machine.
   *
   * Each time some event arrives on the app, it calls here to report
   * which event it was.  The initial activation of the app is what
   * starts everything in motion (starting from state 0).  At each
   * state, we assert that we receive the expected event, send the next
   * event, then update the current_state variable so we do the correct
   * thing next time.
   */

  switch (current_state)
    {
      case 0: g_assert_cmpstr (action, ==, "activate");

      /* Let's try another activation... */
      g_app_info_launch (G_APP_INFO (appinfo), NULL, NULL, NULL);
      current_state = 1; return; case 1: g_assert_cmpstr (action, ==, "activate");


      /* Now let's try opening some files... */
      {
        GList *files;

        files = g_list_prepend (NULL, g_file_new_for_uri ("file:///a/b"));
        files = g_list_append (files, g_file_new_for_uri ("file:///c/d"));
        g_app_info_launch (G_APP_INFO (appinfo), files, NULL, NULL);
        g_list_free_full (files, g_object_unref);
      }
      current_state = 2; return; case 2: g_assert_cmpstr (action, ==, "open");

      /* Now action activations... */
      g_desktop_app_info_launch_action (appinfo, "frob", NULL);
      current_state = 3; return; case 3: g_assert_cmpstr (action, ==, "frob");

      g_desktop_app_info_launch_action (appinfo, "tweak", NULL);
      current_state = 4; return; case 4: g_assert_cmpstr (action, ==, "tweak");

      g_desktop_app_info_launch_action (appinfo, "twiddle", NULL);
      current_state = 5; return; case 5: g_assert_cmpstr (action, ==, "twiddle");

      /* Now launch the app with startup notification */
      {
        GAppLaunchContext *ctx;

        g_assert (saw_startup_id == FALSE);
        ctx = g_object_new (test_app_launch_context_get_type (), NULL);
        g_app_info_launch (G_APP_INFO (appinfo), NULL, ctx, NULL);
        g_assert (requested_startup_id);
        requested_startup_id = FALSE;
        g_object_unref (ctx);
      }
      current_state = 6; return; case 6: g_assert_cmpstr (action, ==, "activate"); g_assert (saw_startup_id);
      saw_startup_id = FALSE;

      /* Now do the same for an action */
      {
        GAppLaunchContext *ctx;

        g_assert (saw_startup_id == FALSE);
        ctx = g_object_new (test_app_launch_context_get_type (), NULL);
        g_desktop_app_info_launch_action (appinfo, "frob", ctx);
        g_assert (requested_startup_id);
        requested_startup_id = FALSE;
        g_object_unref (ctx);
      }
      current_state = 7; return; case 7: g_assert_cmpstr (action, ==, "frob"); g_assert (saw_startup_id);
      saw_startup_id = FALSE;

      /* Now quit... */
      g_desktop_app_info_launch_action (appinfo, "quit", NULL);
      current_state = 8; return; case 8: g_assert_not_reached ();
    }
}

static void
test_application_frob (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  g_assert (parameter == NULL);
  saw_action ("frob");
}

static void
test_application_tweak (GSimpleAction *action,
                        GVariant      *parameter,
                        gpointer       user_data)
{
  g_assert (parameter == NULL);
  saw_action ("tweak");
}

static void
test_application_twiddle (GSimpleAction *action,
                          GVariant      *parameter,
                          gpointer       user_data)
{
  g_assert (parameter == NULL);
  saw_action ("twiddle");
}

static void
test_application_quit (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  GApplication *application = user_data;

  g_application_quit (application);
}

static const GActionEntry app_actions[] = {
  { "frob",         test_application_frob,    NULL, NULL, NULL, { 0 } },
  { "tweak",        test_application_tweak,   NULL, NULL, NULL, { 0 } },
  { "twiddle",      test_application_twiddle, NULL, NULL, NULL, { 0 } },
  { "quit",         test_application_quit,    NULL, NULL, NULL, { 0 } }
};

static void
test_application_activate (GApplication *application)
{
  /* Unbalanced, but that's OK because we will quit() */
  g_application_hold (application);

  saw_action ("activate");
}

static void
test_application_open (GApplication  *application,
                       GFile        **files,
                       gint           n_files,
                       const gchar   *hint)
{
  GFile *f;

  g_assert_cmpstr (hint, ==, "");

  g_assert_cmpint (n_files, ==, 2);
  f = g_file_new_for_uri ("file:///a/b");
  g_assert (g_file_equal (files[0], f));
  g_object_unref (f);
  f = g_file_new_for_uri ("file:///c/d");
  g_assert (g_file_equal (files[1], f));
  g_object_unref (f);

  saw_action ("open");
}

static void
test_application_startup (GApplication *application)
{
  G_APPLICATION_CLASS (test_application_parent_class)
    ->startup (application);

  g_action_map_add_action_entries (G_ACTION_MAP (application), app_actions, G_N_ELEMENTS (app_actions), application);
}

static void
test_application_before_emit (GApplication *application,
                              GVariant     *platform_data)
{
  const gchar *startup_id;
  gsize i;

  const gchar *startup_id_keys[] = {
    "desktop-startup-id",
    "activation-token",
    NULL,
  };

  for (i = 0; startup_id_keys[i] != NULL; i++)
    {
      if (!g_variant_lookup (platform_data, startup_id_keys[i], "&s", &startup_id))
        return;

      g_assert_cmpstr (startup_id, ==, "expected startup id");
    }

  saw_startup_id = TRUE;
}

static void
test_application_init (TestApplication *app)
{
}

static void
test_application_class_init (GApplicationClass *class)
{
  class->before_emit = test_application_before_emit;
  class->startup = test_application_startup;
  class->activate = test_application_activate;
  class->open = test_application_open;
}

static void
test_dbus_appinfo (void)
{
  const gchar *argv[] = { "myapp", NULL };
  TestApplication *app;
  int status;
  gchar *desktop_file = NULL;

  desktop_file = g_test_build_filename (G_TEST_DIST,
                                        "org.gtk.test.dbusappinfo.desktop",
                                        NULL);
  appinfo = g_desktop_app_info_new_from_filename (desktop_file);
  g_assert (appinfo != NULL);
  g_free (desktop_file);

  app = g_object_new (test_application_get_type (),
                      "application-id", "org.gtk.test.dbusappinfo",
                      "flags", G_APPLICATION_HANDLES_OPEN,
                      NULL);
  status = g_application_run (app, 1, (gchar **) argv);

  g_assert_cmpint (status, ==, 0);
  g_assert_cmpint (current_state, ==, 8);

  g_object_unref (appinfo);
  g_object_unref (app);
}

typedef struct {
  GApplication parent;

  gboolean opened;
} TestSandboxedApplication;

static GType test_sandboxed_application_get_type (void);
typedef GApplicationClass TestSandboxedApplicationClass;
G_DEFINE_TYPE (TestSandboxedApplication, test_sandboxed_application, G_TYPE_APPLICATION)

static void
on_sandboxed_app_launch_uris_finish (GObject *object,
                                     GAsyncResult *result,
                                     gpointer user_data)
{
  GApplication *app = user_data;
  GError *error = NULL;

  g_app_info_launch_uris_finish (G_APP_INFO (object), result, &error);
  g_assert_no_error (error);
  g_assert_true (requested_startup_id);
  g_assert_true (saw_startup_id);

  g_application_release (app);
}

static void
on_sandboxed_app_activate (GApplication *app,
                           gpointer user_data)
{
  GAppLaunchContext *ctx;
  GDesktopAppInfo *sandboxed_app_appinfo = user_data;
  char *uri;
  GList *uris;

  /* The app will be released in on_sandboxed_app_launch_uris_finish */
  g_application_hold (app);

  uri = g_filename_to_uri (g_desktop_app_info_get_filename (sandboxed_app_appinfo), NULL, NULL);
  g_assert_nonnull (uri);
  uris = g_list_prepend (NULL, uri);
  ctx = g_object_new (test_app_launch_context_get_type (), NULL);
  requested_startup_id = FALSE;
  saw_startup_id = FALSE;
  g_app_info_launch_uris_async (G_APP_INFO (sandboxed_app_appinfo), uris, ctx,
                                NULL, on_sandboxed_app_launch_uris_finish, app);
  g_object_unref (ctx);
  g_list_free (uris);
  g_free (uri);
}

static void
on_sandboxed_app_open (GApplication  *app,
                       GFile        **files,
                       gint           n_files,
                       const char    *hint,
                       gpointer       user_data)
{
  GFakeDocumentPortalThread *portal = user_data;
  TestSandboxedApplication *sandboxed_app = (TestSandboxedApplication *) app;
  GFile *f;
  char *desktop_id;

  g_assert_cmpint (n_files, ==, 1);
  g_test_message ("on_sandboxed_app_open received file '%s'", g_file_peek_path (files[0]));

  desktop_id = g_strconcat (g_application_get_application_id (app), ".desktop", NULL);

  /* The file has been exported via the document portal */
  f = g_file_new_build_filename (g_fake_document_portal_thread_get_mount_point (portal),
                                 "document-id-0",
                                 desktop_id,
                                 NULL);
  g_assert_cmpstr (g_file_peek_path (files[0]), == , g_file_peek_path (f));
  g_assert_true (g_file_equal (files[0], f));
  sandboxed_app->opened = TRUE;

  g_object_unref (f);
  g_free (desktop_id);
}

static void
test_sandboxed_application_init (TestSandboxedApplication *app)
{
}

static void
test_sandboxed_application_class_init (GApplicationClass *class)
{
  class->before_emit = test_application_before_emit;
}

static void
test_sandboxed_application_doc_export (const char *app_id,
                                       const char *expected_portal_app_id)
{
#ifdef __GNU__
  /* g_unix_fd_query_path() is not supported on Hurd, so the fake document
   * portal cannot determine filenames from file descriptors. */
  g_test_skip ("g_unix_fd_query_path() not supported on Hurd");
  return;
#endif

  const gchar *argv[] = { "myapp", NULL };
  gchar *desktop_file = NULL;
  gchar *desktop_id;
  GDesktopAppInfo *appinfo;
  GApplication *app;
  GFakeDocumentPortalThread *thread = NULL;
  int status;

  /* Run a fake-document-portal */
  thread = g_fake_document_portal_thread_new (session_bus_get_address (),
                                              expected_portal_app_id);
  g_fake_document_portal_thread_run (thread);

  desktop_id = g_strconcat (app_id, ".desktop", NULL);
  desktop_file = g_test_build_filename (G_TEST_DIST, desktop_id, NULL);
  appinfo = g_desktop_app_info_new_from_filename (desktop_file);
  g_assert_nonnull (appinfo);
  g_free (desktop_file);
  g_free (desktop_id);

  app = g_object_new (test_sandboxed_application_get_type (),
                      "application-id", app_id,
                      "flags", G_APPLICATION_HANDLES_OPEN,
                      NULL);
  g_signal_connect (app, "activate", G_CALLBACK (on_sandboxed_app_activate),
                    appinfo);
  g_signal_connect_object (app, "open", G_CALLBACK (on_sandboxed_app_open),
                           thread, G_CONNECT_DEFAULT);

  g_assert_false (((TestSandboxedApplication *) app)->opened);
  status = g_application_run (app, 1, (gchar **) argv);
  g_assert_cmpint (status, ==, 0);
  g_assert_true (((TestSandboxedApplication *) app)->opened);

  g_object_unref (app);
  g_object_unref (appinfo);
  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
}

static void
test_flatpak_doc_export (void)
{
  g_test_summary ("Test that files opened by launching flatpak apps are made "
                  "available via the document portal.");

  test_sandboxed_application_doc_export ("org.gtk.test.dbusappinfo.flatpak",
                                         "org.gtk.test.dbusappinfo.flatpak");
}

static void
test_snap_doc_export (void)
{
  g_test_summary ("Test that files opened by launching snap apps are made "
                  "available via the document portal.");

  test_sandboxed_application_doc_export ("org.gtk.test.dbusappinfo.snap",
                                         "snap.snap-app");
}

static void
on_sandboxed_app_launch_invalid_uri_finish (GObject *object,
                                            GAsyncResult *result,
                                            gpointer user_data)
{
  GApplication *app = user_data;
  GError *error = NULL;

  g_app_info_launch_uris_finish (G_APP_INFO (object), result, &error);
  g_assert_no_error (error);
  g_assert_true (requested_startup_id);
  g_assert_true (saw_startup_id);

  g_application_release (app);
}

static void
on_sandboxed_app_activate_invalid_uri (GApplication *app,
                                       gpointer user_data)
{
  GAppLaunchContext *ctx;
  GDesktopAppInfo *sandboxed_app_appinfo = user_data;
  GList *uris;

  /* The app will be released in on_sandboxed_app_launch_uris_finish */
  g_application_hold (app);

  uris = g_list_prepend (NULL, "file:///hopefully/an/invalid/path.desktop");
  ctx = g_object_new (test_app_launch_context_get_type (), NULL);
  requested_startup_id = FALSE;
  saw_startup_id = FALSE;
  g_app_info_launch_uris_async (G_APP_INFO (sandboxed_app_appinfo), uris, ctx,
                                NULL, on_sandboxed_app_launch_invalid_uri_finish, app);
  g_object_unref (ctx);
  g_list_free (uris);
}

static void
on_sandboxed_app_open_invalid_uri (GApplication  *app,
                                    GFile        **files,
                                    gint           n_files,
                                    const char    *hint)
{
  GFile *f;

  g_assert_cmpint (n_files, ==, 1);
  g_test_message ("on_sandboxed_app_open received file '%s'", g_file_peek_path (files[0]));

  /* The file has been exported via the document portal */
  f = g_file_new_for_uri ("file:///hopefully/an/invalid/path.desktop");
  g_assert_cmpstr (g_file_peek_path (files[0]), == , g_file_peek_path (f));
  g_assert_true (g_file_equal (files[0], f));
  g_object_unref (f);
}

static void
test_sandboxed_app_missing_doc_export (const char *app_id)
{
  const gchar *argv[] = { "myapp", NULL };
  gchar *desktop_file = NULL;
  gchar *desktop_id;
  GDesktopAppInfo *appinfo;
  GApplication *app;
  int status;
  GFakeDocumentPortalThread *thread = NULL;

  /* Run a fake-document-portal */
  thread = g_fake_document_portal_thread_new (session_bus_get_address (),
                                              "%%_NO_PORTAL_CALLED_%%");
  g_fake_document_portal_thread_run (thread);

  desktop_id = g_strconcat (app_id, ".desktop", NULL);
  desktop_file = g_test_build_filename (G_TEST_DIST, desktop_id, NULL);
  appinfo = g_desktop_app_info_new_from_filename (desktop_file);
  g_assert_nonnull (appinfo);

  app = g_object_new (test_sandboxed_application_get_type (),
                      "application-id", app_id,
                      "flags", G_APPLICATION_HANDLES_OPEN,
                      NULL);
  g_signal_connect (app, "activate", G_CALLBACK (on_sandboxed_app_activate_invalid_uri),
                    appinfo);
  g_signal_connect (app, "open", G_CALLBACK (on_sandboxed_app_open_invalid_uri), NULL);

  g_assert_false (((TestSandboxedApplication *) app)->opened);
  status = g_application_run (app, 1, (gchar **) argv);
  g_assert_cmpint (status, ==, 0);
  g_assert_false (((TestSandboxedApplication *) app)->opened);

  g_object_unref (app);
  g_object_unref (appinfo);
  g_free (desktop_file);
  g_free (desktop_id);
  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
}

static void
test_flatpak_missing_doc_export (void)
{
  g_test_summary ("Test that files opened by launching flatpak apps are not made "
                  "available via the document portal.");

  test_sandboxed_app_missing_doc_export ("org.gtk.test.dbusappinfo.flatpak");
}

static void
test_snap_missing_doc_export (void)
{
  g_test_summary ("Test that files opened by launching snap apps are not made "
                  "available via the document portal.");

  test_sandboxed_app_missing_doc_export ("org.gtk.test.dbusappinfo.snap");
}

static void
check_portal_openuri_call (const char               *expected_uri,
                           GFakeDesktopPortalThread *thread)
{
  const char *activation_token = NULL;
  GFile *expected_file = NULL;
  GFile *file = NULL;
  const char *uri = NULL;

  activation_token = g_fake_desktop_portal_thread_get_last_request_activation_token (thread);
  uri = g_fake_desktop_portal_thread_get_last_request_uri (thread);

  g_assert_cmpstr (activation_token, ==, "expected startup id");
  g_assert_nonnull (uri);

  expected_file = g_file_new_for_uri (expected_uri);
  file = g_file_new_for_uri (uri);
  g_assert_true (g_file_equal (expected_file, file));

  g_object_unref (file);
  g_object_unref (expected_file);
}

static void
test_portal_open_file (void)
{
  GAppLaunchContext *ctx;
  GError *error = NULL;
  char *uri;
  GFakeDesktopPortalThread *thread = NULL;

  if (!g_fake_desktop_portal_is_supported ())
    {
      g_test_skip ("fake-desktop-portal not currently supported on this platform");
      return;
    }

  /* Run a fake-desktop-portal */
  thread = g_fake_desktop_portal_thread_new (session_bus_get_address ());
  g_fake_desktop_portal_thread_run (thread);

  uri = g_filename_to_uri (g_test_get_filename (G_TEST_DIST,
                                                "org.gtk.test.dbusappinfo.flatpak.desktop",
                                                NULL),
                           NULL,
                           NULL);

  ctx = g_object_new (test_app_launch_context_get_type (), NULL);

  requested_startup_id = FALSE;

  g_app_info_launch_default_for_uri (uri, ctx, &error);

  g_assert_no_error (error);
  g_assert_true (requested_startup_id);

  g_fake_desktop_portal_thread_stop (thread);
  check_portal_openuri_call (uri, thread);

  g_clear_object (&ctx);
  g_clear_object (&thread);
  g_clear_pointer (&uri, g_free);
}

static void
test_portal_open_uri (void)
{
  GAppLaunchContext *ctx;
  GError *error = NULL;
  const char *uri = "http://example.com";
  GFakeDesktopPortalThread *thread = NULL;

  if (!g_fake_desktop_portal_is_supported ())
    {
      g_test_skip ("fake-desktop-portal not currently supported on this platform");
      return;
    }

  /* Run a fake-desktop-portal */
  thread = g_fake_desktop_portal_thread_new (session_bus_get_address ());
  g_fake_desktop_portal_thread_run (thread);

  ctx = g_object_new (test_app_launch_context_get_type (), NULL);

  requested_startup_id = FALSE;

  g_app_info_launch_default_for_uri (uri, ctx, &error);

  g_assert_no_error (error);
  g_assert_true (requested_startup_id);

  g_fake_desktop_portal_thread_stop (thread);
  check_portal_openuri_call (uri, thread);

  g_clear_object (&ctx);
  g_clear_object (&thread);
}

static void
on_launch_default_for_uri_finished (GObject      *object,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  GError *error = NULL;
  gboolean *called = user_data;

  g_app_info_launch_default_for_uri_finish (result, &error);
  g_assert_no_error (error);

  *called = TRUE;

  g_main_context_wakeup (NULL);
}

static void
test_portal_open_file_async (void)
{
  GAppLaunchContext *ctx;
  gboolean called = FALSE;
  char *uri;
  GFakeDesktopPortalThread *thread = NULL;

  if (!g_fake_desktop_portal_is_supported ())
    {
      g_test_skip ("fake-desktop-portal not currently supported on this platform");
      return;
    }

  /* Run a fake-desktop-portal */
  thread = g_fake_desktop_portal_thread_new (session_bus_get_address ());
  g_fake_desktop_portal_thread_run (thread);

  uri = g_filename_to_uri (g_test_get_filename (G_TEST_DIST,
                                                "org.gtk.test.dbusappinfo.flatpak.desktop",
                                                NULL),
                           NULL,
                           NULL);

  ctx = g_object_new (test_app_launch_context_get_type (), NULL);

  requested_startup_id = FALSE;

  g_app_info_launch_default_for_uri_async (uri, ctx,
                                           NULL, on_launch_default_for_uri_finished, &called);

  while (!called)
    g_main_context_iteration (NULL, TRUE);

  g_assert_true (requested_startup_id);

  g_fake_desktop_portal_thread_stop (thread);
  check_portal_openuri_call (uri, thread);

  g_clear_pointer (&uri, g_free);
  g_clear_object (&ctx);
  g_clear_object (&thread);
}

static void
test_portal_open_uri_async (void)
{
  GAppLaunchContext *ctx;
  gboolean called = FALSE;
  const char *uri = "http://example.com";
  GFakeDesktopPortalThread *thread = NULL;

  if (!g_fake_desktop_portal_is_supported ())
    {
      g_test_skip ("fake-desktop-portal not currently supported on this platform");
      return;
    }

  /* Run a fake-desktop-portal */
  thread = g_fake_desktop_portal_thread_new (session_bus_get_address ());
  g_fake_desktop_portal_thread_run (thread);

  ctx = g_object_new (test_app_launch_context_get_type (), NULL);

  requested_startup_id = FALSE;

  g_app_info_launch_default_for_uri_async (uri, ctx,
                                           NULL, on_launch_default_for_uri_finished, &called);

  while (!called)
    g_main_context_iteration (NULL, TRUE);

  g_assert_true (requested_startup_id);

  g_fake_desktop_portal_thread_stop (thread);
  check_portal_openuri_call (uri, thread);

  g_clear_object (&ctx);
  g_clear_object (&thread);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_setenv ("GIO_USE_PORTALS", "1", TRUE);

  g_test_add_func ("/appinfo/dbusappinfo", test_dbus_appinfo);
  g_test_add_func ("/appinfo/flatpak-doc-export", test_flatpak_doc_export);
  g_test_add_func ("/appinfo/flatpak-missing-doc-export", test_flatpak_missing_doc_export);
  g_test_add_func ("/appinfo/snap-doc-export", test_snap_doc_export);
  g_test_add_func ("/appinfo/snap-missing-doc-export", test_snap_missing_doc_export);
  g_test_add_func ("/appinfo/portal-open-file", test_portal_open_file);
  g_test_add_func ("/appinfo/portal-open-uri", test_portal_open_uri);
  g_test_add_func ("/appinfo/portal-open-file-async", test_portal_open_file_async);
  g_test_add_func ("/appinfo/portal-open-uri-async", test_portal_open_uri_async);

  return session_bus_run ();
}
