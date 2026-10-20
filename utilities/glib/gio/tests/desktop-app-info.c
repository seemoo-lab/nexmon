/* 
 * Copyright (C) 2008 Red Hat, Inc
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
 * Author: Matthias Clasen
 */

#include <locale.h>

#include <glib/glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gio/gunixinputstream.h>
#include <glib-unix.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fake-document-portal.h"
#include "gdbus-sessionbus.h"

G_DECLARE_FINAL_TYPE (TestLaunchContext, test_launch_context, TEST,
                      LAUNCH_CONTEXT, GAppLaunchContext);

struct _TestLaunchContext {
  GAppLaunchContext parent;

  char *overriden_startup_notify_id;
};

struct _TestLaunchContextClass {
  GAppLaunchContextClass parent;
};

G_DEFINE_FINAL_TYPE (TestLaunchContext, test_launch_context,
                     G_TYPE_APP_LAUNCH_CONTEXT);

static void
test_launch_context_init (TestLaunchContext *test_context)
{
}

static char *
test_launch_context_get_startup_notify_id (GAppLaunchContext *context,
                                           GAppInfo *app_info,
                                           GList *files)
{
  TestLaunchContext *test_context = TEST_LAUNCH_CONTEXT (context);

  if (test_context->overriden_startup_notify_id)
    return g_strdup (test_context->overriden_startup_notify_id);

  if (g_app_info_get_id (app_info))
    return g_strdup (g_app_info_get_id (app_info));

  if (g_app_info_get_display_name (app_info))
    return g_strdup (g_app_info_get_display_name (app_info));

  return g_strdup (g_app_info_get_commandline (app_info));
}

static void
test_launch_context_get_startup_notify_dispose (GObject *object)
{
  TestLaunchContext *test_context = TEST_LAUNCH_CONTEXT (object);

  g_clear_pointer (&test_context->overriden_startup_notify_id, g_free);
  G_OBJECT_CLASS (test_launch_context_parent_class)->dispose (object);
}

static void
test_launch_context_class_init (TestLaunchContextClass *klass)
{
  G_APP_LAUNCH_CONTEXT_CLASS (klass)->get_startup_notify_id = test_launch_context_get_startup_notify_id;
  G_OBJECT_CLASS (klass)->dispose = test_launch_context_get_startup_notify_dispose;
}

static GAppInfo *
create_command_line_app_info (const char *name,
                              const char *command_line,
                              const char *default_for_type)
{
  GAppInfo *info;
  GError *error = NULL;

  info = g_app_info_create_from_commandline (command_line,
                                             name,
                                             G_APP_INFO_CREATE_NONE,
                                             &error);
  g_assert_no_error (error);

  g_app_info_set_as_default_for_type (info, default_for_type, &error);
  g_assert_no_error (error);

  return g_steal_pointer (&info);
}

static GAppInfo *
create_app_info (const char *name)
{
  GError *error = NULL;
  GAppInfo *info;

  info = create_command_line_app_info (name, "true blah", "application/x-blah");

  /* this is necessary to ensure that the info is saved */
  g_app_info_remove_supports_type (info, "application/x-blah", &error);
  g_assert_no_error (error);
  g_app_info_reset_type_associations ("application/x-blah");
  
  return info;
}

static gboolean
skip_missing_update_desktop_database (void)
{
  gchar *path = g_find_program_in_path ("update-desktop-database");

  if (path == NULL)
    {
      g_test_skip ("update-desktop-database is required to run this test");
      return TRUE;
    }
  g_free (path);
  return FALSE;
}

static void
test_delete (void)
{
  GAppInfo *info;

  const char *id;
  char *filename;
  gboolean res;

  if (skip_missing_update_desktop_database ())
    return;

  info = create_app_info ("Blah");
 
  id = g_app_info_get_id (info);
  g_assert_nonnull (id);

  filename = g_build_filename (g_get_user_data_dir (), "applications", id, NULL);

  res = g_file_test (filename, G_FILE_TEST_EXISTS);
  g_assert_true (res);

  res = g_app_info_can_delete (info);
  g_assert_true (res);

  res = g_app_info_delete (info);
  g_assert_true (res);

  res = g_file_test (filename, G_FILE_TEST_EXISTS);
  g_assert_false (res);

  g_object_unref (info);

  if (g_file_test ("/usr/share/applications/gedit.desktop", G_FILE_TEST_EXISTS))
    {
      info = (GAppInfo*)g_desktop_app_info_new_from_filename ("/usr/share/applications/gedit.desktop");
      g_assert_nonnull (info);
     
      res = g_app_info_can_delete (info);
      g_assert_false (res);
 
      res = g_app_info_delete (info);
      g_assert_false (res);
    }

  g_free (filename);
}

static void
test_default (void)
{
  GAppInfo *info, *info1, *info2, *info3;
  GList *list;
  GError *error = NULL;  

if (skip_missing_update_desktop_database ())
    return;

  info1 = create_app_info ("Blah1");
  info2 = create_app_info ("Blah2");
  info3 = create_app_info ("Blah3");

  g_app_info_set_as_default_for_type (info1, "application/x-test", &error);
  g_assert_no_error (error);

  g_app_info_set_as_default_for_type (info2, "application/x-test", &error);
  g_assert_no_error (error);

  info = g_app_info_get_default_for_type ("application/x-test", FALSE);
  g_assert_nonnull (info);
  g_assert_cmpstr (g_app_info_get_id (info), ==, g_app_info_get_id (info2));
  g_object_unref (info);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*uri_scheme*failed*");
  g_assert_null (g_app_info_get_default_for_uri_scheme (NULL));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*uri_scheme*failed*");
  g_assert_null (g_app_info_get_default_for_uri_scheme (""));
  g_test_assert_expected_messages ();

  g_app_info_set_as_default_for_type (info3, "x-scheme-handler/glib", &error);
  g_assert_no_error (error);
  info = g_app_info_get_default_for_uri_scheme ("glib");
  g_assert_nonnull (info);
  g_assert_true (g_app_info_equal (info, info3));
  g_object_unref (info);

  /* now try adding something, but not setting as default */
  g_app_info_add_supports_type (info3, "application/x-test", &error);
  g_assert_no_error (error);

  /* check that info2 is still default */
  info = g_app_info_get_default_for_type ("application/x-test", FALSE);
  g_assert_nonnull (info);
  g_assert_cmpstr (g_app_info_get_id (info), ==, g_app_info_get_id (info2));
  g_object_unref (info);

  /* now remove info1 again */
  g_app_info_remove_supports_type (info1, "application/x-test", &error);
  g_assert_no_error (error);

  /* and make sure info2 is still default */
  info = g_app_info_get_default_for_type ("application/x-test", FALSE);
  g_assert_nonnull (info);
  g_assert_cmpstr (g_app_info_get_id (info), ==, g_app_info_get_id (info2));
  g_object_unref (info);

  /* now clean it all up */
  g_app_info_reset_type_associations ("application/x-test");
  g_app_info_reset_type_associations ("x-scheme-handler/glib");

  list = g_app_info_get_all_for_type ("application/x-test");
  g_assert_null (list);

  list = g_app_info_get_all_for_type ("x-scheme-handler/glib");
  g_assert_null (list);

  g_app_info_delete (info1);
  g_app_info_delete (info2);
  g_app_info_delete (info3);

  g_object_unref (info1);
  g_object_unref (info2);
  g_object_unref (info3);
}

typedef struct
{
  GAppInfo *expected_info;
  GMainLoop *loop;
} DefaultForTypeData;

static void
ensure_default_type_result (GAppInfo           *info,
                            DefaultForTypeData *data,
                            GError             *error)
{
  if (data->expected_info)
    {
      g_assert_nonnull (info);
      g_assert_no_error (error);
      g_assert_true (g_app_info_equal (info, data->expected_info));
    }
  else
    {
      g_assert_null (info);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND);
    }

  g_main_loop_quit (data->loop);
  g_clear_object (&info);
  g_clear_error (&error);
}

static void
on_default_for_type_cb (GObject      *object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  GAppInfo *info;
  GError *error = NULL;
  DefaultForTypeData *data = user_data;

  g_assert_null (object);

  info = g_app_info_get_default_for_type_finish (result, &error);

  ensure_default_type_result (info, data, error);
}

static void
on_default_for_uri_cb (GObject      *object,
                       GAsyncResult *result,
                       gpointer      user_data)
{
  GAppInfo *info;
  GError *error = NULL;
  DefaultForTypeData *data = user_data;

  g_assert_null (object);

  info = g_app_info_get_default_for_uri_scheme_finish (result, &error);

  ensure_default_type_result (info, data, error);
}

static void
test_default_async (void)
{
  DefaultForTypeData data;
  GAppInfo *info1, *info2, *info3;
  GList *list;
  GError *error = NULL;

  if (skip_missing_update_desktop_database ())
    return;

  data.loop = g_main_loop_new (NULL, TRUE);

  info1 = create_app_info ("Blah1");
  info2 = create_app_info ("Blah2");
  info3 = create_app_info ("Blah3");

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*content_type*failed*");
  g_app_info_get_default_for_type_async (NULL, FALSE, NULL, NULL, NULL);
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*content_type*failed*");
  g_app_info_get_default_for_type_async ("", FALSE, NULL, NULL, NULL);
  g_test_assert_expected_messages ();

  g_app_info_set_as_default_for_type (info1, "application/x-test", &error);
  g_assert_no_error (error);

  g_app_info_set_as_default_for_type (info2, "application/x-test", &error);
  g_assert_no_error (error);

  data.expected_info = info2;
  g_app_info_get_default_for_type_async ("application/x-test", FALSE,
                                         NULL, on_default_for_type_cb, &data);
  g_main_loop_run (data.loop);

  /* now try adding something, but not setting as default */
  g_app_info_add_supports_type (info3, "application/x-test", &error);
  g_assert_no_error (error);

  /* check that info2 is still default */
  data.expected_info = info2;
  g_app_info_get_default_for_type_async ("application/x-test", FALSE,
                                         NULL, on_default_for_type_cb, &data);
  g_main_loop_run (data.loop);

  /* now remove info1 again */
  g_app_info_remove_supports_type (info1, "application/x-test", &error);
  g_assert_no_error (error);

  /* and make sure info2 is still default */
  data.expected_info = info2;
  g_app_info_get_default_for_type_async ("application/x-test", FALSE,
                                         NULL, on_default_for_type_cb, &data);
  g_main_loop_run (data.loop);

  g_app_info_set_as_default_for_type (info3, "x-scheme-handler/glib-async", &error);
  g_assert_no_error (error);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*uri_scheme*failed*");
  g_assert_null (g_app_info_get_default_for_uri_scheme (NULL));
  g_test_assert_expected_messages ();

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                         "*assertion*uri_scheme*failed*");
  g_assert_null (g_app_info_get_default_for_uri_scheme (""));
  g_test_assert_expected_messages ();

  data.expected_info = info3;
  g_app_info_get_default_for_uri_scheme_async ("glib-async", NULL,
                                               on_default_for_uri_cb, &data);
  g_main_loop_run (data.loop);

  /* now clean it all up */
  g_app_info_reset_type_associations ("application/x-test");

  data.expected_info = NULL;
  g_app_info_get_default_for_type_async ("application/x-test", FALSE,
                                         NULL, on_default_for_type_cb, &data);
  g_main_loop_run (data.loop);

  g_app_info_reset_type_associations ("x-scheme-handler/glib-async");

  data.expected_info = NULL;
  g_app_info_get_default_for_uri_scheme_async ("glib-async", NULL,
                                               on_default_for_uri_cb, &data);
  g_main_loop_run (data.loop);

  list = g_app_info_get_all_for_type ("application/x-test");
  g_assert_null (list);

  g_app_info_delete (info1);
  g_app_info_delete (info2);
  g_app_info_delete (info3);

  g_object_unref (info1);
  g_object_unref (info2);
  g_object_unref (info3);

  g_main_loop_unref (data.loop);
}

static void
test_fallback (void)
{
  GAppInfo *info1, *info2, *app = NULL;
  GList *apps, *recomm, *fallback, *list, *l, *m;
  GError *error = NULL;
  gint old_length;

  if (skip_missing_update_desktop_database ())
    return;

  info1 = create_app_info ("Test1");
  info2 = create_app_info ("Test2");

  g_assert_true (g_content_type_is_a ("text/x-python", "text/plain"));

  apps = g_app_info_get_all_for_type ("text/x-python");
  old_length = g_list_length (apps);
  g_list_free_full (apps, g_object_unref);

  g_app_info_add_supports_type (info1, "text/x-python", &error);
  g_assert_no_error (error);

  g_app_info_add_supports_type (info2, "text/plain", &error);
  g_assert_no_error (error);

  /* check that both apps are registered */
  apps = g_app_info_get_all_for_type ("text/x-python");
  g_assert_cmpint (g_list_length (apps), ==, old_length + 2);

  /* check that Test1 is among the recommended apps */
  recomm = g_app_info_get_recommended_for_type ("text/x-python");
  g_assert_nonnull (recomm);
  for (l = recomm; l; l = l->next)
    {
      app = l->data;
      if (g_app_info_equal (info1, app))
        break;
    }
  g_assert_nonnull (app);
  g_assert_true (g_app_info_equal (info1, app));

  /* and that Test2 is among the fallback apps */
  fallback = g_app_info_get_fallback_for_type ("text/x-python");
  g_assert_nonnull (fallback);
  for (l = fallback; l; l = l->next)
    {
      app = l->data;
      if (g_app_info_equal (info2, app))
        break;
    }
  g_assert_cmpstr (g_app_info_get_name (app), ==, "Test2");

  /* check that recomm + fallback = all applications */
  list = g_list_concat (g_list_copy (recomm), g_list_copy (fallback));
  g_assert_cmpuint (g_list_length (list), ==, g_list_length (apps));

  for (l = list, m = apps; l != NULL && m != NULL; l = l->next, m = m->next)
    {
      g_assert_true (g_app_info_equal (l->data, m->data));
    }

  g_list_free (list);

  g_list_free_full (apps, g_object_unref);
  g_list_free_full (recomm, g_object_unref);
  g_list_free_full (fallback, g_object_unref);

  g_app_info_reset_type_associations ("text/x-python");
  g_app_info_reset_type_associations ("text/plain");

  g_app_info_delete (info1);
  g_app_info_delete (info2);

  g_object_unref (info1);
  g_object_unref (info2);
}

static void
test_last_used (void)
{
  GList *applications;
  GAppInfo *info1, *info2, *default_app;
  GError *error = NULL;

  if (skip_missing_update_desktop_database ())
    return;

  info1 = create_app_info ("Test1");
  info2 = create_app_info ("Oh No / A Slash");

  g_app_info_set_as_default_for_type (info1, "application/x-test", &error);
  g_assert_no_error (error);

  g_app_info_add_supports_type (info2, "application/x-test", &error);
  g_assert_no_error (error);

  applications = g_app_info_get_recommended_for_type ("application/x-test");
  g_assert_cmpuint (g_list_length (applications), ==, 2);

  /* the first should be the default app now */
  g_assert_true (g_app_info_equal (g_list_nth_data (applications, 0), info1));
  g_assert_true (g_app_info_equal (g_list_nth_data (applications, 1), info2));

  g_list_free_full (applications, g_object_unref);

  g_app_info_set_as_last_used_for_type (info2, "application/x-test", &error);
  g_assert_no_error (error);

  applications = g_app_info_get_recommended_for_type ("application/x-test");
  g_assert_cmpuint (g_list_length (applications), ==, 2);

  default_app = g_app_info_get_default_for_type ("application/x-test", FALSE);
  g_assert_true (g_app_info_equal (default_app, info1));

  /* the first should be the other app now */
  g_assert_true (g_app_info_equal (g_list_nth_data (applications, 0), info2));
  g_assert_true (g_app_info_equal (g_list_nth_data (applications, 1), info1));

  g_list_free_full (applications, g_object_unref);

  g_app_info_reset_type_associations ("application/x-test");

  g_app_info_delete (info1);
  g_app_info_delete (info2);

  g_object_unref (info1);
  g_object_unref (info2);
  g_object_unref (default_app);
}

static void
test_extra_getters (void)
{
  GDesktopAppInfo *appinfo;
  const gchar *lang;
  gchar *s;
  gboolean b;

  lang = setlocale (LC_ALL, NULL);
  g_setenv ("LANGUAGE", "de_DE.UTF8", TRUE);
  setlocale (LC_ALL, "");

  appinfo = g_desktop_app_info_new_from_filename (g_test_get_filename (G_TEST_DIST, "appinfo-test-static.desktop", NULL));
  g_assert_nonnull (appinfo);

  g_assert_true (g_desktop_app_info_has_key (appinfo, "Terminal"));
  g_assert_false (g_desktop_app_info_has_key (appinfo, "Bratwurst"));

  s = g_desktop_app_info_get_string (appinfo, "StartupWMClass");
  g_assert_cmpstr (s, ==, "appinfo-class");
  g_free (s);

  s = g_desktop_app_info_get_locale_string (appinfo, "X-JunkFood");
  g_assert_cmpstr (s, ==, "Bratwurst");
  g_free (s);

  g_setenv ("LANGUAGE", "sv_SE.UTF8", TRUE);
  setlocale (LC_ALL, "");

  s = g_desktop_app_info_get_locale_string (appinfo, "X-JunkFood");
  g_assert_cmpstr (s, ==, "Burger"); /* fallback */
  g_free (s);

  b = g_desktop_app_info_get_boolean (appinfo, "Terminal");
  g_assert_true (b);

  g_object_unref (appinfo);

  g_setenv ("LANGUAGE", lang, TRUE);
  setlocale (LC_ALL, "");
}

static void
wait_for_file (const gchar *want_this,
               const gchar *but_not_this,
               const gchar *or_this)
{
  while (access (want_this, F_OK) != 0)
    g_usleep (100000); /* 100ms */

  g_assert_cmpuint (access (but_not_this, F_OK), !=, 0);
  g_assert_cmpuint (access (or_this, F_OK), !=, 0);

  unlink (want_this);
  unlink (but_not_this);
  unlink (or_this);
}

static gboolean
skip_missing_dbus_daemon (void)
{
  gchar *path = g_find_program_in_path ("dbus-daemon");
  if (path == NULL)
    {
      g_test_skip ("dbus-daemon is required to run this test");
      return TRUE;
    }
  g_free (path);
  return FALSE;
}

static void
test_actions (void)
{
  GTestDBus *bus = NULL;
  const char *expected[] = { "frob", "tweak", "twiddle", "broken", NULL };
  const gchar * const *actions;
  GDesktopAppInfo *appinfo;
  const gchar *tmpdir;
  gchar *name;
  gchar *frob_path;
  gchar *tweak_path;
  gchar *twiddle_path;

  if (skip_missing_dbus_daemon ())
    return;

  /* Set up a test session bus to keep D-Bus traffic off the real session bus. */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  appinfo = g_desktop_app_info_new_from_filename (g_test_get_filename (G_TEST_DIST, "appinfo-test-actions.desktop", NULL));
  g_assert_nonnull (appinfo);

  actions = g_desktop_app_info_list_actions (appinfo);
  g_assert_cmpstrv (actions, expected);

  name = g_desktop_app_info_get_action_name (appinfo, "frob");
  g_assert_cmpstr (name, ==, "Frobnicate");
  g_free (name);

  name = g_desktop_app_info_get_action_name (appinfo, "tweak");
  g_assert_cmpstr (name, ==, "Tweak");
  g_free (name);

  name = g_desktop_app_info_get_action_name (appinfo, "twiddle");
  g_assert_cmpstr (name, ==, "Twiddle");
  g_free (name);

  name = g_desktop_app_info_get_action_name (appinfo, "broken");
  g_assert_nonnull (name);
  g_assert_true (g_utf8_validate (name, -1, NULL));
  g_free (name);

  tmpdir = g_getenv ("G_TEST_TMPDIR");
  g_assert_nonnull (tmpdir);
  frob_path = g_build_filename (tmpdir, "frob", NULL);
  tweak_path = g_build_filename (tmpdir, "tweak", NULL);
  twiddle_path = g_build_filename (tmpdir, "twiddle", NULL);

  g_assert_false (g_file_test (frob_path, G_FILE_TEST_EXISTS));
  g_assert_false (g_file_test (tweak_path, G_FILE_TEST_EXISTS));
  g_assert_false (g_file_test (twiddle_path, G_FILE_TEST_EXISTS));

  g_desktop_app_info_launch_action (appinfo, "frob", NULL);
  wait_for_file (frob_path, tweak_path, twiddle_path);

  g_desktop_app_info_launch_action (appinfo, "tweak", NULL);
  wait_for_file (tweak_path, frob_path, twiddle_path);

  g_desktop_app_info_launch_action (appinfo, "twiddle", NULL);
  wait_for_file (twiddle_path, frob_path, tweak_path);

  g_free (frob_path);
  g_free (tweak_path);
  g_free (twiddle_path);
  g_object_unref (appinfo);

  g_test_dbus_down (bus);
  g_clear_object (&bus);
}

static gchar *
run_apps (const gchar *command,
          const gchar *arg,
          gboolean     with_usr,
          gboolean     with_home,
          const gchar *locale_name,
          const gchar *language,
          const gchar *xdg_current_desktop)
{
  gboolean success;
  gchar **envp;
  gchar **argv;
  gint status;
  gchar *out;
  gchar *argv_str = NULL;

  argv = g_new (gchar *, 4);
  argv[0] = g_test_build_filename (G_TEST_BUILT, "apps", NULL);
  argv[1] = g_strdup (command);
  argv[2] = g_strdup (arg);
  argv[3] = NULL;

  g_assert_true (g_file_test (argv[0], G_FILE_TEST_IS_EXECUTABLE));
  envp = g_get_environ ();

  if (with_usr)
    {
      gchar *tmp = g_test_build_filename (G_TEST_DIST, "desktop-files", "usr", NULL);
      envp = g_environ_setenv (envp, "XDG_DATA_DIRS", tmp, TRUE);
      g_free (tmp);
    }
  else
    envp = g_environ_setenv (envp, "XDG_DATA_DIRS", "/does-not-exist", TRUE);

  if (with_home)
    {
      gchar *tmp = g_test_build_filename (G_TEST_DIST, "desktop-files", "home", NULL);
      envp = g_environ_setenv (envp, "XDG_DATA_HOME", tmp, TRUE);
      g_free (tmp);
    }
  else
    envp = g_environ_setenv (envp, "XDG_DATA_HOME", "/does-not-exist", TRUE);

  if (locale_name)
    envp = g_environ_setenv (envp, "LC_ALL", locale_name, TRUE);
  else
    envp = g_environ_setenv (envp, "LC_ALL", "C", TRUE);

  if (language)
    envp = g_environ_setenv (envp, "LANGUAGE", language, TRUE);
  else
    envp = g_environ_unsetenv (envp, "LANGUAGE");

  if (xdg_current_desktop)
    envp = g_environ_setenv (envp, "XDG_CURRENT_DESKTOP", xdg_current_desktop, TRUE);
  else
    envp = g_environ_unsetenv (envp, "XDG_CURRENT_DESKTOP");

  envp = g_environ_setenv (envp, "G_MESSAGES_DEBUG", "", TRUE);

  success = g_spawn_sync (NULL, argv, envp, 0, NULL, NULL, &out, NULL, &status, NULL);
  g_assert_true (success);
  g_assert_cmpuint (status, ==, 0);

  argv_str = g_strjoinv (" ", argv);
  g_test_message ("%s: `%s` returned: %s", G_STRFUNC, argv_str, out);
  g_free (argv_str);

  g_strfreev (envp);
  g_strfreev (argv);

  return out;
}

static void
assert_strings_equivalent (const gchar *expected,
                           const gchar *result)
{
  gchar **expected_words;
  gchar **result_words;
  gint i, j;

  expected_words = g_strsplit (expected, " ", 0);
  result_words = g_strsplit_set (result, " \n", 0);

  for (i = 0; expected_words[i]; i++)
    {
      for (j = 0; result_words[j]; j++)
        if (g_str_equal (expected_words[i], result_words[j]))
          goto got_it;

      g_test_fail_printf ("Unable to find expected string '%s' in result '%s'", expected_words[i], result);

got_it:
      continue;
    }

  g_assert_cmpint (g_strv_length (expected_words), ==, g_strv_length (result_words));
  g_strfreev (expected_words);
  g_strfreev (result_words);
}

static void
assert_list (const gchar *expected,
             gboolean     with_usr,
             gboolean     with_home,
             const gchar *locale_name,
             const gchar *language)
{
  gchar *result;

  result = run_apps ("list", NULL, with_usr, with_home, locale_name, language, NULL);
  g_strchomp (result);
  assert_strings_equivalent (expected, result);
  g_free (result);
}

static void
assert_info (const gchar *desktop_id,
             const gchar *expected,
             gboolean     with_usr,
             gboolean     with_home,
             const gchar *locale_name,
             const gchar *language)
{
  gchar *result;

  result = run_apps ("show-info", desktop_id, with_usr, with_home, locale_name, language, NULL);
  g_assert_cmpstr (result, ==, expected);
  g_free (result);
}

static void
assert_search (const gchar *search_string,
               const gchar *expected,
               gboolean     with_usr,
               gboolean     with_home,
               const gchar *locale_name,
               const gchar *language)
{
  gchar **expected_lines;
  gchar **result_lines;
  gchar *result;
  gint i;

  expected_lines = g_strsplit (expected, "\n", -1);
  result = run_apps ("search", search_string, with_usr, with_home, locale_name, language, NULL);
  result_lines = g_strsplit (result, "\n", -1);
  g_assert_cmpint (g_strv_length (expected_lines), ==, g_strv_length (result_lines));
  for (i = 0; expected_lines[i]; i++)
    assert_strings_equivalent (expected_lines[i], result_lines[i]);
  g_strfreev (expected_lines);
  g_strfreev (result_lines);
  g_free (result);
}

static void
assert_implementations (const gchar *interface,
                        const gchar *expected,
                        gboolean     with_usr,
                        gboolean     with_home)
{
  gchar *result;

  result = run_apps ("implementations", interface, with_usr, with_home, NULL, NULL, NULL);
  g_strchomp (result);
  assert_strings_equivalent (expected, result);
  g_free (result);
}

#define ALL_USR_APPS "evince-previewer.desktop nautilus-classic.desktop gnome-font-viewer.desktop "         \
                     "baobab.desktop yelp.desktop eog.desktop cheese.desktop org.gnome.clocks.desktop "     \
                     "gnome-contacts.desktop kde4-kate.desktop gcr-prompter.desktop totem.desktop "         \
                     "gnome-terminal.desktop nautilus-autorun-software.desktop gcr-viewer.desktop "         \
                     "nautilus-connect-server.desktop kde4-dolphin.desktop gnome-music.desktop "            \
                     "kde4-konqbrowser.desktop gucharmap.desktop kde4-okular.desktop nautilus.desktop "     \
                     "gedit.desktop evince.desktop file-roller.desktop dconf-editor.desktop glade.desktop " \
                     "invalid-desktop.desktop org.gnome.Calculator.desktop libreoffice-calc.desktop"
#define HOME_APPS    "epiphany-weather-for-toronto-island-9c6a4e022b17686306243dada811d550d25eb1fb.desktop"
#define ALL_HOME_APPS HOME_APPS " eog.desktop"

static void
test_search (void)
{
  assert_list ("", FALSE, FALSE, NULL, NULL);
  assert_list (ALL_USR_APPS, TRUE, FALSE, NULL, NULL);
  assert_list (ALL_HOME_APPS, FALSE, TRUE, NULL, NULL);
  assert_list (ALL_USR_APPS " " HOME_APPS, TRUE, TRUE, NULL, NULL);

  /* The user has "installed" their own version of eog.desktop which
   * calls it "Eye of GNOME".  Do some testing based on that.
   *
   * We should always find "Pictures" keyword no matter where we look.
   */
  assert_search ("Picture", "eog.desktop\n", TRUE, TRUE, NULL, NULL);
  assert_search ("Picture", "eog.desktop\n", TRUE, FALSE, NULL, NULL);
  assert_search ("Picture", "eog.desktop\n", FALSE, TRUE, NULL, NULL);
  assert_search ("Picture", "", FALSE, FALSE, NULL, NULL);

  /* We should only find it called "eye of gnome" when using the user's
   * directory.
   */
  assert_search ("eye gnome", "", TRUE, FALSE, NULL, NULL);
  assert_search ("eye gnome", "eog.desktop\n", FALSE, TRUE, NULL, NULL);
  assert_search ("eye gnome", "eog.desktop\n", TRUE, TRUE, NULL, NULL);

  /* We should only find it called "image viewer" when _not_ using the
   * user's directory.
   */
  assert_search ("image viewer", "eog.desktop\n", TRUE, FALSE, NULL, NULL);
  assert_search ("image viewer", "", FALSE, TRUE, NULL, NULL);
  assert_search ("image viewer", "", TRUE, TRUE, NULL, NULL);

  /* There're "flatpak" apps (clocks) installed as well - they should *not*
   * match the prefix command ("/bin/sh") in the Exec= line though. Then with
   * substring matching, Image Viewer (eog) should be in next group because it
   * contains "Slideshow" in its keywords.
   *
   * Finally we have LibreOffice Calc, which contains "OpenDocument Spreadsheet".
   * It is sorted last because its match ("sh" in "Spreadsheet") occurs in a
   * later token.
   */
  assert_search ("sh", "gnome-terminal.desktop\n"
                       "eog.desktop\n"
                       "libreoffice-calc.desktop\n", TRUE, FALSE, NULL, NULL);

  /* "frobnicator.desktop" is ignored by get_all() because the binary is
   * missing, but search should still find it (to avoid either stale results
   * from the cache or expensive stat() calls for each potential result)
   */
  assert_search ("frobni", "frobnicator.desktop\n", TRUE, FALSE, NULL, NULL);

  /* Obvious multi-word search */
  assert_search ("doc hel", "yelp.desktop\n", TRUE, TRUE, NULL, NULL);

  /* Repeated search terms should do nothing... */
  assert_search ("files file fil fi f", "nautilus.desktop\n", TRUE, TRUE, NULL, NULL);

  /* "con" will match "connect" and "contacts" on name with prefix match in
   * first group, then second group is a Keyword prefix match for "configuration" in dconf-editor.desktop
   * and third group is a substring match for "Desktop Icons" in Name of nautilus-classic.desktop.
   */
  assert_search ("con", "gnome-contacts.desktop nautilus-connect-server.desktop\n"
                        "dconf-editor.desktop\n"
                        "nautilus-classic.desktop\n", TRUE, TRUE, NULL, NULL);

  /* We prefer matches of tokens that come earlier in a string. In this case
   * "LibreOffice Calc" and "Calculator" both have a name that contains a prefix
   * match "cal", but the one in Calculator occurs in the first token.
   */
  assert_search ("cal", "org.gnome.Calculator.desktop\nlibreoffice-calc.desktop\n", TRUE, TRUE, NULL, NULL);

  /* Same as above, but ensure that substring matches are sorted after prefix matches */
  assert_search ("ca", "org.gnome.Calculator.desktop\n"
                       "libreoffice-calc.desktop\n"
                       "frobnicator.desktop\n"
                       "cheese.desktop\n", TRUE, TRUE, NULL, NULL);

  /* "gnome" will match "eye of gnome" from the user's directory, plus
   * matching "GNOME Clocks" X-GNOME-FullName.
   */
  assert_search ("gnome", "eog.desktop\n"
                          "org.gnome.clocks.desktop\n", TRUE, TRUE, NULL, NULL);

  /* eog has exec name 'false' in usr only */
  assert_search ("false", "eog.desktop\n", TRUE, FALSE, NULL, NULL);
  assert_search ("false", "", FALSE, TRUE, NULL, NULL);
  assert_search ("false", "", TRUE, TRUE, NULL, NULL);
  assert_search ("false", "", FALSE, FALSE, NULL, NULL);

  /* make sure we only search the first component */
  assert_search ("nonsearchable", "", TRUE, FALSE, NULL, NULL);

  /* "gnome con" will match only gnome contacts; via the name for
   * "contacts" and keywords for "friend"
   */
  assert_search ("friend con", "gnome-contacts.desktop\n", TRUE, TRUE, NULL, NULL);

  /* make sure we get the correct kde4- prefix on the application IDs
   * from subdirectories
   */
  assert_search ("konq", "kde4-konqbrowser.desktop\n", TRUE, TRUE, NULL, NULL);
  assert_search ("kate", "kde4-kate.desktop\n", TRUE, TRUE, NULL, NULL);

  /* make sure we can look up apps by name properly */
  assert_info ("kde4-kate.desktop",
               "kde4-kate.desktop\n"
               "Kate\n"
               "Kate\n"
               "nil\n", TRUE, TRUE, NULL, NULL);

  assert_info ("nautilus.desktop",
               "nautilus.desktop\n"
               "Files\n"
               "Files\n"
               "Access and organize files\n", TRUE, TRUE, NULL, NULL);

  /* make sure localised searching works properly */
  assert_search ("foliumi", "nautilus.desktop\n"
                            "kde4-konqbrowser.desktop\n", TRUE, FALSE, "en_US.UTF-8", "eo");
  /* the user's eog.desktop has no translations... */
  assert_search ("foliumi", "nautilus.desktop\n"
                            "kde4-konqbrowser.desktop\n", TRUE, TRUE, "en_US.UTF-8", "eo");
}

static void
test_implements (void)
{
  /* Make sure we can find our search providers... */
  assert_implementations ("org.gnome.Shell.SearchProvider2",
                       "gnome-music.desktop gnome-contacts.desktop eog.desktop",
                       TRUE, FALSE);

  /* And our image acquisition possibilities... */
  assert_implementations ("org.freedesktop.ImageProvider",
                       "cheese.desktop",
                       TRUE, FALSE);

  /* Make sure the user's eog is properly masking the system one */
  assert_implementations ("org.gnome.Shell.SearchProvider2",
                       "gnome-music.desktop gnome-contacts.desktop",
                       TRUE, TRUE);

  /* Make sure we get nothing if we have nothing */
  assert_implementations ("org.gnome.Shell.SearchProvider2", "", FALSE, FALSE);
}

static void
assert_shown (const gchar *desktop_id,
              gboolean     expected,
              const gchar *xdg_current_desktop)
{
  gchar *result;

  result = run_apps ("should-show", desktop_id, TRUE, TRUE, NULL, NULL, xdg_current_desktop);
  g_assert_cmpstr (result, ==, expected ? "true\n" : "false\n");
  g_free (result);
}

static void
test_show_in (void)
{
  assert_shown ("gcr-prompter.desktop", FALSE, NULL);
  assert_shown ("gcr-prompter.desktop", FALSE, "GNOME");
  assert_shown ("gcr-prompter.desktop", FALSE, "KDE");
  assert_shown ("gcr-prompter.desktop", FALSE, "GNOME:GNOME-Classic");
  assert_shown ("gcr-prompter.desktop", TRUE, "GNOME-Classic:GNOME");
  assert_shown ("gcr-prompter.desktop", TRUE, "GNOME-Classic");
  assert_shown ("gcr-prompter.desktop", TRUE, "GNOME-Classic:KDE");
  assert_shown ("gcr-prompter.desktop", TRUE, "KDE:GNOME-Classic");
  assert_shown ("invalid-desktop.desktop", TRUE, "GNOME");
  assert_shown ("invalid-desktop.desktop", FALSE, "../invalid/desktop");
  assert_shown ("invalid-desktop.desktop", FALSE, "../invalid/desktop:../invalid/desktop");
}

static void
on_launch_started (GAppLaunchContext *context, GAppInfo *info, GVariant *platform_data, gpointer data)
{
  gboolean *invoked = data;

  g_assert_true (G_IS_APP_LAUNCH_CONTEXT (context));

  if (TEST_IS_LAUNCH_CONTEXT (context))
    {
      GVariantDict dict;
      const char *sni;
      char *expected_sni;

      g_assert_nonnull (platform_data);
      g_variant_dict_init (&dict, platform_data);
      g_assert_true (
        g_variant_dict_lookup (&dict, "startup-notification-id", "&s", &sni));
      expected_sni = g_app_launch_context_get_startup_notify_id (context, info, NULL);
      g_assert_cmpstr (sni, ==, expected_sni);

      g_free (expected_sni);
      g_variant_dict_clear (&dict);
    }
  else
    {
      /* Our default context doesn't fill in any platform data */
      g_assert_null (platform_data);
    }

  g_assert_false (*invoked);
  *invoked = TRUE;
}

static void
on_launched (GAppLaunchContext *context, GAppInfo *info, GVariant *platform_data, gpointer data)
{
  gboolean *launched = data;
  GVariantDict dict;
  int pid;

  g_assert_true (G_IS_APP_LAUNCH_CONTEXT (context));
  g_assert_true (G_IS_APP_INFO (info));
  g_assert_nonnull (platform_data);
  g_variant_dict_init (&dict, platform_data);
  g_assert_true (g_variant_dict_lookup (&dict, "pid", "i", &pid, NULL));
  g_assert_cmpint (pid, >, 1);

  g_assert_false (*launched);
  *launched = TRUE;

  g_variant_dict_clear (&dict);
}

static void
on_launch_failed (GAppLaunchContext *context, const char *startup_notify_id, gpointer data)
{
  gboolean *invoked = data;

  g_assert_true (G_IS_APP_LAUNCH_CONTEXT (context));
  g_assert_nonnull (startup_notify_id);
  g_test_message ("Application launch failed: %s", startup_notify_id);

  g_assert_false (*invoked);
  *invoked = TRUE;
}

static void
wait_child_completed (GDesktopAppInfo *appinfo,
                      GPid             pid,
                      gpointer         user_data)
{
  gboolean *child_waited = user_data;
  int wait_status = 0;
  GError *error = NULL;

  while (waitpid (pid, &wait_status, 0) != pid);
  g_spawn_check_wait_status (wait_status, &error);
  g_assert_no_error (error);
  g_clear_error (&error);
  *child_waited = TRUE;
}

/* Test g_desktop_app_info_launch_uris_as_manager() and
 * g_desktop_app_info_launch_uris_as_manager_with_fds()
 */
static void
test_launch_as_manager (void)
{
  GDesktopAppInfo *appinfo;
  GError *error = NULL;
  gboolean retval;
  const gchar *path;
  gboolean invoked = FALSE;
  gboolean launched = FALSE;
  gboolean failed = FALSE;
  gboolean child_waited = FALSE;
  GAppLaunchContext *context;

  path = g_test_get_filename (G_TEST_BUILT, "appinfo-test.desktop", NULL);
  appinfo = g_desktop_app_info_new_from_filename (path);
  g_assert_true (G_IS_APP_INFO (appinfo));

  context = g_object_new (test_launch_context_get_type (), NULL);
  g_signal_connect (context, "launch-started",
                    G_CALLBACK (on_launch_started),
                    &invoked);
  g_signal_connect (context, "launched",
                    G_CALLBACK (on_launched),
                    &launched);
  g_signal_connect (context, "launch-failed",
                    G_CALLBACK (on_launch_failed),
                    &failed);
  retval = g_desktop_app_info_launch_uris_as_manager (appinfo, NULL, context,
                                                      G_SPAWN_DO_NOT_REAP_CHILD,
                                                      NULL,
                                                      NULL,
                                                      wait_child_completed,
                                                      &child_waited,
                                                      &error);
  g_assert_no_error (error);
  g_assert_true (retval);
  g_assert_true (invoked);
  g_assert_true (launched);
  g_assert_true (child_waited);
  g_assert_false (failed);

  invoked = FALSE;
  launched = FALSE;
  failed = FALSE;
  child_waited = FALSE;
  retval = g_desktop_app_info_launch_uris_as_manager_with_fds (appinfo,
                                                               NULL, context,
                                                               G_SPAWN_DO_NOT_REAP_CHILD,
                                                               NULL,
                                                               NULL,
                                                               wait_child_completed,
                                                               &child_waited,
                                                               -1, -1, -1,
                                                               &error);
  g_assert_no_error (error);
  g_assert_true (retval);
  g_assert_true (invoked);
  g_assert_true (launched);
  g_assert_true (child_waited);
  g_assert_false (failed);

  g_object_unref (appinfo);
  g_assert_finalize_object (context);
}

static void
test_launch_as_manager_fail (void)
{
  GAppLaunchContext *context;
  GDesktopAppInfo *appinfo;
  GError *error = NULL;
  gboolean retval;
  const gchar *path;
  gboolean launch_started = FALSE;
  gboolean launched = FALSE;
  gboolean failed = FALSE;
  gboolean child_waited = FALSE;

  g_test_summary ("Tests that launch-errors are properly handled, we force " \
                  "this by using invalid FD's values when launching as manager");

  path = g_test_get_filename (G_TEST_BUILT, "appinfo-test.desktop", NULL);
  appinfo = g_desktop_app_info_new_from_filename (path);
  g_assert_true (G_IS_APP_INFO (appinfo));

  context = g_object_new (test_launch_context_get_type (), NULL);
  g_signal_connect (context, "launch-started",
                    G_CALLBACK (on_launch_started),
                    &launch_started);
  g_signal_connect (context, "launched",
                    G_CALLBACK (on_launched),
                    &launched);
  g_signal_connect (context, "launch-failed",
                    G_CALLBACK (on_launch_failed),
                    &failed);

  retval = g_desktop_app_info_launch_uris_as_manager_with_fds (appinfo,
                                                               NULL, context,
                                                               G_SPAWN_DO_NOT_REAP_CHILD,
                                                               NULL,
                                                               NULL,
                                                               wait_child_completed,
                                                               &child_waited,
                                                               3000, 3001, 3002,
                                                               &error);
  g_assert_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED);
  g_assert_false (retval);
  g_assert_true (launch_started);
  g_assert_false (launched);
  g_assert_false (child_waited);
  g_assert_true (failed);

  g_clear_error (&error);
  g_object_unref (appinfo);
  g_assert_finalize_object (context);
}

static GAppInfo *
create_app_info_toucher (const char  *name,
                         const char  *touched_file_name,
                         const char  *handled_type,
                         char       **out_file_path)
{
  GError *error = NULL;
  GAppInfo *info;
  gchar *command_line;
  gchar *file_path;
  gchar *tmpdir;

  g_assert_nonnull (out_file_path);

  tmpdir = g_dir_make_tmp ("desktop-app-info-launch-XXXXXX", &error);
  g_assert_no_error (error);

  file_path = g_build_filename (tmpdir, touched_file_name, NULL);
  command_line = g_strdup_printf ("touch %s", file_path);

  info = create_command_line_app_info (name, command_line, handled_type);
  *out_file_path = g_steal_pointer (&file_path);

  g_free (tmpdir);
  g_free (command_line);

  return info;
}

static void
test_default_uri_handler (void)
{
  GError *error = NULL;
  gchar *file_path = NULL;
  GAppInfo *info;

  if (skip_missing_update_desktop_database ())
    return;

  info = create_app_info_toucher ("Touch Handled", "handled",
                                  "x-scheme-handler/glib-touch",
                                  &file_path);
  g_assert_true (G_IS_APP_INFO (info));
  g_assert_nonnull (file_path);

  g_assert_true (g_app_info_launch_default_for_uri ("glib-touch://touch-me",
                                                    NULL, &error));
  g_assert_no_error (error);

  while (!g_file_test (file_path, G_FILE_TEST_IS_REGULAR));
  g_assert_true (g_file_test (file_path, G_FILE_TEST_IS_REGULAR));

  g_assert_false (g_app_info_launch_default_for_uri ("glib-INVALID-touch://touch-me",
                                                     NULL, &error));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  g_object_unref (info);
  g_free (file_path);
}

static void
on_launch_default_for_uri_success_cb (GObject      *object,
                                      GAsyncResult *result,
                                      gpointer      user_data)
{
  GError *error = NULL;
  gboolean *called = user_data;

  g_assert_true (g_app_info_launch_default_for_uri_finish (result, &error));
  g_assert_no_error (error);

  *called = TRUE;
}

static void
on_launch_default_for_uri_not_found_cb (GObject      *object,
                                        GAsyncResult *result,
                                        gpointer      user_data)
{
  GError *error = NULL;
  GMainLoop *loop = user_data;

  g_assert_false (g_app_info_launch_default_for_uri_finish (result, &error));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
  g_clear_error (&error);

  g_main_loop_quit (loop);
}

static void
on_launch_default_for_uri_cancelled_cb (GObject      *object,
                                        GAsyncResult *result,
                                        gpointer      user_data)
{
  GError *error = NULL;
  GMainLoop *loop = user_data;

  g_assert_false (g_app_info_launch_default_for_uri_finish (result, &error));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_clear_error (&error);

  g_main_loop_quit (loop);
}

static void
test_default_uri_handler_async (void)
{
  GCancellable *cancellable;
  gchar *file_path = NULL;
  GAppInfo *info;
  GMainLoop *loop;
  gboolean called = FALSE;
  gint64 start_time, touch_time;

  if (skip_missing_update_desktop_database ())
    return;

  loop = g_main_loop_new (NULL, FALSE);
  info = create_app_info_toucher ("Touch Handled", "handled-async",
                                  "x-scheme-handler/glib-async-touch",
                                  &file_path);
  g_assert_true (G_IS_APP_INFO (info));
  g_assert_nonnull (file_path);

  start_time = g_get_real_time ();
  g_app_info_launch_default_for_uri_async ("glib-async-touch://touch-me", NULL,
                                           NULL,
                                           on_launch_default_for_uri_success_cb,
                                           &called);

  while (!g_file_test (file_path, G_FILE_TEST_IS_REGULAR) || !called)
    g_main_context_iteration (NULL, FALSE);

  touch_time = g_get_real_time () - start_time;
  g_assert_true (called);
  g_assert_true (g_file_test (file_path, G_FILE_TEST_IS_REGULAR));

  g_unlink (file_path);
  g_assert_false (g_file_test (file_path, G_FILE_TEST_IS_REGULAR));

  g_app_info_launch_default_for_uri_async ("glib-async-INVALID-touch://touch-me",
                                           NULL, NULL,
                                           on_launch_default_for_uri_not_found_cb,
                                           loop);
  g_main_loop_run (loop);

  cancellable = g_cancellable_new ();
  g_app_info_launch_default_for_uri_async ("glib-async-touch://touch-me", NULL,
                                           cancellable,
                                           on_launch_default_for_uri_cancelled_cb,
                                           loop);
  g_cancellable_cancel (cancellable);
  g_main_loop_run (loop);

  /* If started, our touch app would take some time to actually write the
   * file to disk, so let's wait a bit here to ensure that the file isn't
   * inadvertently getting created when a launch operation is canceled up
   * front. Give it 3× as long as the successful case took, to allow for 
   * some variance.
   */
  g_usleep (touch_time * 3);
  g_assert_false (g_file_test (file_path, G_FILE_TEST_IS_REGULAR));

  g_object_unref (info);
  g_main_loop_unref (loop);
  g_free (file_path);
}

static void
launch_snap_uris_with_portal (void)
{
  GDesktopAppInfo *appinfo;
  GError *error = NULL;
  gboolean retval;
  const gchar *path;
  const gchar *path2;
  gboolean invoked = FALSE;
  gboolean launched = FALSE;
  gboolean failed = FALSE;
  gboolean child_waited = FALSE;
  GAppLaunchContext *context;
  GList *uris = NULL;
  GFakeDocumentPortalThread *thread = NULL;

  /* Run a fake-document-portal */
  session_bus_up ();
  thread = g_fake_document_portal_thread_new (session_bus_get_address (),
                                              "snap.snap-app");
  g_fake_document_portal_thread_run (thread);

  path = g_test_get_filename (G_TEST_BUILT, "snap-app_appinfo-test.desktop", NULL);
  appinfo = g_desktop_app_info_new_from_filename (path);
  g_assert_true (G_IS_APP_INFO (appinfo));

  path2 = g_test_get_filename (G_TEST_BUILT, "appinfo-test.desktop", NULL);
  g_assert_true (g_file_test (path2, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR));

  context = g_object_new (test_launch_context_get_type (), NULL);
  g_signal_connect (context, "launch-started",
                    G_CALLBACK (on_launch_started),
                    &invoked);
  g_signal_connect (context, "launched",
                    G_CALLBACK (on_launched),
                    &launched);
  g_signal_connect (context, "launch-failed",
                    G_CALLBACK (on_launch_failed),
                    &failed);
  g_app_launch_context_setenv (context, "DOCUMENT_PORTAL_MOUNT_POINT",
                               g_fake_document_portal_thread_get_mount_point (thread));

  uris = g_list_append (uris, g_strconcat ("file://", path, NULL));
  uris = g_list_append (uris, g_strconcat ("file://", path2, NULL));

  retval = g_desktop_app_info_launch_uris_as_manager (appinfo, uris,
                                                      context,
                                                      G_SPAWN_DO_NOT_REAP_CHILD,
                                                      NULL,
                                                      NULL,
                                                      wait_child_completed,
                                                      &child_waited,
                                                      &error);

  g_assert_no_error (error);
  g_assert_true (retval);
  g_assert_true (invoked);
  g_assert_true (launched);
  g_assert_true (child_waited);
  g_assert_false (failed);

  g_clear_list (&uris, g_free);
  g_object_unref (appinfo);
  g_assert_finalize_object (context);

  g_fake_document_portal_thread_stop (thread);
  g_clear_object (&thread);
  session_bus_down ();
}

/* Test if Desktop-File Id is correctly formed */
static void
test_id (void)
{
  gchar *result;

  result = run_apps ("default-for-type", "application/vnd.kde.okular-archive",
                     TRUE, FALSE, NULL, NULL, NULL);
  g_assert_cmpstr (result, ==, "kde4-okular.desktop\n");
  g_free (result);
}

static const char *
get_terminal_divider (const char *terminal_name)
{
  if (g_str_equal (terminal_name, "xdg-terminal-exec"))
    return NULL;
  if (g_str_equal (terminal_name, "kgx"))
    return "-e";
  if (g_str_equal (terminal_name, "gnome-terminal"))
    return "--";
  if (g_str_equal (terminal_name, "tilix"))
    return "-e";
  if (g_str_equal (terminal_name, "konsole"))
    return "-e";
  if (g_str_equal (terminal_name, "nxterm"))
    return "-e";
  if (g_str_equal (terminal_name, "color-xterm"))
    return "-e";
  if (g_str_equal (terminal_name, "rxvt"))
    return "-e";
  if (g_str_equal (terminal_name, "dtterm"))
    return "-e";
  if (g_str_equal (terminal_name, "xterm"))
    return "-e";
  if (g_str_equal (terminal_name, "mate-terminal"))
    return "-x";
  if (g_str_equal (terminal_name, "xfce4-terminal"))
    return "-x";

  g_return_val_if_reached (NULL);
}

typedef enum {
  TERMINAL_LAUNCH_TYPE_COMMAND_LINE_WITH_PATH_OVERRIDE,
  TERMINAL_LAUNCH_TYPE_COMMAND_LINE_WITH_CONTEXT,
  TERMINAL_LAUNCH_TYPE_KEY_FILE_WITH_PATH,
} TerminalLaunchType;

typedef struct {
  const char *exec;
  TerminalLaunchType type;
} TerminalLaunchData;

static TerminalLaunchData *
terminal_launch_data_new (const char *exec, TerminalLaunchType type)
{
  TerminalLaunchData *d = NULL;

  d = g_new0 (TerminalLaunchData, 1);
  d->exec = exec;
  d->type = type;

  return d;
}

static void
test_launch_uris_with_terminal (gconstpointer data)
{
  int fd;
  int ret;
  int flags;
  int terminal_divider_arg_length;
  const TerminalLaunchData *launch_data = data;
  const char *terminal_exec = launch_data->exec;
  char *old_path = NULL;
  char *command_line;
  char *bin_path;
  char *terminal_path;
  char *output_fd_path;
  char *script_contents;
  char *output_contents = NULL;
  char *sh;
  GAppInfo *app_info;
  GList *uris;
  GList *paths;
  GStrv output_args;
  GError *error = NULL;
  GInputStream *input_stream;
  GDataInputStream *data_input_stream;
  GAppLaunchContext *launch_context;

  sh = g_find_program_in_path ("sh");
  g_assert_nonnull (sh);

  bin_path = g_dir_make_tmp ("bin-path-XXXXXX", &error);
  g_assert_no_error (error);

  launch_context = g_object_new (test_launch_context_get_type (), NULL);

  switch (launch_data->type)
    {
    case TERMINAL_LAUNCH_TYPE_COMMAND_LINE_WITH_PATH_OVERRIDE:
      old_path = g_strdup (g_getenv ("PATH"));
      g_assert_true (g_setenv ("PATH", bin_path, TRUE));
      break;

    case TERMINAL_LAUNCH_TYPE_COMMAND_LINE_WITH_CONTEXT:
      g_app_launch_context_setenv (launch_context, "PATH", bin_path);
      break;

    case TERMINAL_LAUNCH_TYPE_KEY_FILE_WITH_PATH:
      g_app_launch_context_setenv (launch_context, "PATH", "/not/valid");
      break;

    default:
      g_assert_not_reached ();
    }

  terminal_path = g_build_filename (bin_path, terminal_exec, NULL);
  output_fd_path = g_build_filename (bin_path, "fifo", NULL);

  ret = mkfifo (output_fd_path, 0600);
  g_assert_cmpint (ret, ==, 0);

  fd = g_open (output_fd_path, O_RDONLY | O_CLOEXEC | O_NONBLOCK, 0);
  g_assert_cmpint (fd, >=, 0);

  flags = fcntl (fd, F_GETFL);
  g_assert_cmpint (flags, >=, 0);

  ret = fcntl (fd, F_SETFL,  flags & ~O_NONBLOCK);
  g_assert_cmpint (ret, ==, 0);

  input_stream = g_unix_input_stream_new (fd, TRUE);
  data_input_stream = g_data_input_stream_new (input_stream);
  script_contents = g_strdup_printf ("#!%s\n" \
                                     "out='%s'\n"
                                     "printf '%%s\\n' \"$*\" > \"$out\"\n",
                                     sh,
                                     output_fd_path);
  g_file_set_contents (terminal_path, script_contents, -1, &error);
  g_assert_no_error (error);
  g_assert_cmpint (g_chmod (terminal_path, 0500), ==, 0);

  g_test_message ("Fake '%s' terminal created as: %s", terminal_exec, terminal_path);

  command_line = g_strdup_printf ("true %s-argument", terminal_exec);

  if (launch_data->type == TERMINAL_LAUNCH_TYPE_KEY_FILE_WITH_PATH)
    {
      GKeyFile *key_file;
      char *key_file_contents;
      const char base_file[] =
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=terminal launched app\n"
        "Terminal=true\n"
        "Path=%s\n"
        "Exec=%s\n";

      key_file = g_key_file_new ();
      key_file_contents = g_strdup_printf (base_file, bin_path, command_line);

      g_assert_true (
        g_key_file_load_from_data (key_file, key_file_contents, -1,
                                   G_KEY_FILE_NONE, NULL));

      app_info = (GAppInfo*) g_desktop_app_info_new_from_keyfile (key_file);
      g_assert_true (G_IS_DESKTOP_APP_INFO (app_info));
      g_assert_true (
        g_desktop_app_info_get_boolean (G_DESKTOP_APP_INFO (app_info), "Terminal"));

      g_key_file_unref (key_file);
      g_free (key_file_contents);
    }
  else
    {
      app_info = g_app_info_create_from_commandline (command_line,
                                                     "Test App on Terminal",
                                                     G_APP_INFO_CREATE_NEEDS_TERMINAL |
                                                     G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                     &error);
      g_assert_no_error (error);
    }

  paths = g_list_prepend (NULL, bin_path);
  uris = g_list_prepend (NULL, g_filename_to_uri (bin_path, NULL, &error));
  g_assert_no_error (error);

  paths = g_list_prepend (paths, (gpointer) g_get_user_data_dir ());
  uris = g_list_append (uris, g_filename_to_uri (g_get_user_data_dir (), NULL, &error));
  g_assert_no_error (error);

  g_assert_cmpint (g_list_length (paths), ==, 2);
  g_app_info_launch_uris (app_info, uris, launch_context, &error);
  g_assert_no_error (error);

  while (output_contents == NULL)
    {
      output_contents =
        g_data_input_stream_read_upto (data_input_stream, "\n", 1, NULL, NULL, &error);
      g_assert_no_error (error);

      if (output_contents == NULL)
        g_usleep (G_USEC_PER_SEC / 10);
    }
  g_test_message ("'%s' called with arguments: '%s'", terminal_exec, output_contents);

  g_data_input_stream_read_byte (data_input_stream, NULL, &error);
  g_assert_no_error (error);

  output_args = g_strsplit (output_contents, " ", -1);
  g_clear_pointer (&output_contents, g_free);

  terminal_divider_arg_length = (get_terminal_divider (terminal_exec) != NULL) ? 1 : 0;
  g_assert_cmpuint (g_strv_length (output_args), ==, 3 + terminal_divider_arg_length);
  if (terminal_divider_arg_length == 1)
    {
      g_assert_cmpstr (output_args[0], ==, get_terminal_divider (terminal_exec));
      g_assert_cmpstr (output_args[1], ==, "true");
      g_assert_cmpstr (output_args[2], ==, command_line + 5);
    }
  else
    {
      g_assert_cmpstr (output_args[0], ==, "true");
      g_assert_cmpstr (output_args[1], ==, command_line + 5);
    }
  paths = g_list_delete_link (paths,
    g_list_find_custom (paths, output_args[2 + terminal_divider_arg_length], g_str_equal));
  g_assert_cmpint (g_list_length (paths), ==, 1);
  g_clear_pointer (&output_args, g_strfreev);

  while (output_contents == NULL)
    {
      output_contents =
        g_data_input_stream_read_upto (data_input_stream, "\n", 1, NULL, NULL, &error);
      g_assert_no_error (error);

      if (output_contents == NULL)
        g_usleep (G_USEC_PER_SEC / 10);
    }
  g_test_message ("'%s' called with arguments: '%s'", terminal_exec, output_contents);

  g_data_input_stream_read_byte (data_input_stream, NULL, &error);
  g_assert_no_error (error);

  output_args = g_strsplit (output_contents, " ", -1);
  g_clear_pointer (&output_contents, g_free);
  g_assert_cmpuint (g_strv_length (output_args), ==, 3 + terminal_divider_arg_length);
  if (terminal_divider_arg_length > 0)
    {
      g_assert_cmpstr (output_args[0], ==, get_terminal_divider (terminal_exec));
      g_assert_cmpstr (output_args[1], ==, "true");
      g_assert_cmpstr (output_args[2], ==, command_line + 5);
    }
  else
    {
      g_assert_cmpstr (output_args[0], ==, "true");
      g_assert_cmpstr (output_args[1], ==, command_line + 5);
    }
  paths = g_list_delete_link (paths,
    g_list_find_custom (paths, output_args[2 + terminal_divider_arg_length], g_str_equal));
  g_assert_cmpint (g_list_length (paths), ==, 0);
  g_clear_pointer (&output_args, g_strfreev);

  g_assert_null (paths);

  if (launch_data->type == TERMINAL_LAUNCH_TYPE_COMMAND_LINE_WITH_PATH_OVERRIDE)
    g_assert_true (g_setenv ("PATH", old_path, TRUE));

  g_close (fd, &error);
  g_assert_no_error (error);

  g_free (sh);
  g_free (command_line);
  g_free (bin_path);
  g_free (terminal_path);
  g_free (output_fd_path);
  g_free (script_contents);
  g_free (old_path);
  g_clear_pointer (&output_args, g_strfreev);
  g_clear_pointer (&output_contents, g_free);
  g_clear_object (&data_input_stream);
  g_clear_object (&input_stream);
  g_clear_object (&app_info);
  g_clear_object (&launch_context);
  g_clear_error (&error);
  g_clear_list (&paths, NULL);
  g_clear_list (&uris, g_free);
}

static void
test_launch_uris_with_invalid_terminal (void)
{
  char *old_path;
  char *bin_path;
  GAppInfo *app_info;
  GError *error = NULL;

  bin_path = g_dir_make_tmp ("bin-path-XXXXXX", &error);
  g_assert_no_error (error);

  old_path = g_strdup (g_getenv ("PATH"));
  g_assert_true (g_setenv ("PATH", bin_path, TRUE));

  app_info = g_app_info_create_from_commandline ("true invalid-glib-terminal",
                                                 "Test App on Invalid Terminal",
                                                 G_APP_INFO_CREATE_NEEDS_TERMINAL |
                                                 G_APP_INFO_CREATE_SUPPORTS_URIS,
                                                 &error);
  g_assert_no_error (error);

  g_app_info_launch_uris (app_info, NULL, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_FAILED);
  g_clear_error (&error);

  g_assert_true (g_setenv ("PATH", old_path, TRUE));

  g_clear_object (&app_info);
  g_clear_error (&error);
  g_free (bin_path);
  g_free (old_path);
}

static void
test_app_path (void)
{
  GDesktopAppInfo *appinfo;
  const char *desktop_path;

  desktop_path = g_test_get_filename (G_TEST_BUILT, "appinfo-test-path.desktop", NULL);
  appinfo = g_desktop_app_info_new_from_filename (desktop_path);

  g_assert_true (G_IS_DESKTOP_APP_INFO (appinfo));

  g_clear_object (&appinfo);
}

static void
test_app_path_wrong (void)
{
  GKeyFile *key_file;
  GDesktopAppInfo *appinfo;
  const gchar bad_try_exec_file_contents[] =
    "[Desktop Entry]\n"
    "Type=Application\n"
    "Name=appinfo-test\n"
    "TryExec=appinfo-test\n"
    "Path=this-must-not-exist‼\n"
    "Exec=true\n";
  const gchar bad_exec_file_contents[] =
    "[Desktop Entry]\n"
    "Type=Application\n"
    "Name=appinfo-test\n"
    "TryExec=true\n"
    "Path=this-must-not-exist‼\n"
    "Exec=appinfo-test\n";

  g_assert_true (
    g_file_test (g_test_get_filename (G_TEST_BUILT, "appinfo-test", NULL),
      G_FILE_TEST_IS_REGULAR | G_FILE_TEST_IS_EXECUTABLE));

  key_file = g_key_file_new ();

  g_assert_true (
    g_key_file_load_from_data (key_file, bad_try_exec_file_contents, -1,
                               G_KEY_FILE_NONE, NULL));

  appinfo = g_desktop_app_info_new_from_keyfile (key_file);
  g_assert_false (G_IS_DESKTOP_APP_INFO (appinfo));

  g_assert_true (
    g_key_file_load_from_data (key_file, bad_exec_file_contents, -1,
                               G_KEY_FILE_NONE, NULL));

  appinfo = g_desktop_app_info_new_from_keyfile (key_file);
  g_assert_false (G_IS_DESKTOP_APP_INFO (appinfo));

  g_clear_pointer (&key_file, g_key_file_unref);
  g_clear_object (&appinfo);
}

static void
test_invalid_key_file (void)
{
  const char *vectors[] =
    {
      /* This .desktop file has invalid escaping in its Exec= line; `\"` is an
       * invalid escape sequence in the Desktop Entry Spec
       * (https://specifications.freedesktop.org/desktop-entry-spec/latest/value-types.html): */
      "[Desktop Entry]\n"
      "Type=Application\n"
      "Name=Example2\n"
      "Exec=gedit \"/home/dkondor/program/file with \\\"weird\\\" name\"\n",
      /* This one has invalid escaping at the end of its Path= line: */
      "[Desktop Entry]\n"
      "Type=Application\n"
      "Name=Example2\n"
      "Path=some/path\\\n"
      "Exec=gedit",
      /* This one has invalid UTF-8 in its TryExec= line: */
      "[Desktop Entry]\n"
      "Type=Application\n"
      "Name=Example2\n"
      "TryExec=gedit \"\xc3\x28\"\n",
    };

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3771");
  g_test_summary ("Test that loading invalid key files does not succeed");

  for (size_t i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      int fd = -1;
      char *tmp_filename = NULL;
      GError *local_error = NULL;

      /* Create a temp desktop file containing the invalid key file data */
      fd = g_file_open_tmp ("desktop-app-info-test-XXXXXX.desktop", &tmp_filename, NULL);
      g_assert_cmpint (fd, >, -1);
      g_close (fd, NULL);

      g_file_set_contents (tmp_filename, vectors[i], -1, &local_error);
      g_assert_no_error (local_error);

      /* Try loading the desktop file; it should fail */
      g_assert_null (g_desktop_app_info_new_from_filename (tmp_filename));

      g_unlink (tmp_filename);
      g_free (tmp_filename);
    }
}

static void
test_launch_startup_notify_fail (void)
{
  GAppInfo *app_info;
  GAppLaunchContext *context;
  GError *error = NULL;
  gboolean launch_started;
  gboolean launch_failed;
  gboolean launched;
  GList *uris;

  app_info = g_app_info_create_from_commandline ("this-must-not-exist‼",
                                                 "failing app",
                                                 G_APP_INFO_CREATE_NONE |
                                                 G_APP_INFO_CREATE_SUPPORTS_STARTUP_NOTIFICATION,
                                                 &error);
  g_assert_no_error (error);

  context = g_object_new (test_launch_context_get_type (), NULL);
  g_signal_connect (context, "launch-started",
                    G_CALLBACK (on_launch_started),
                    &launch_started);
  g_signal_connect (context, "launched",
                    G_CALLBACK (on_launch_started),
                    &launched);
  g_signal_connect (context, "launch-failed",
                    G_CALLBACK (on_launch_failed),
                    &launch_failed);

  launch_started = FALSE;
  launch_failed = FALSE;
  launched = FALSE;
  uris = g_list_prepend (NULL, g_file_new_for_uri ("foo://bar"));
  uris = g_list_prepend (uris, g_file_new_for_uri ("bar://foo"));
  g_assert_false (g_app_info_launch (app_info, uris, context, &error));
  g_assert_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);
  g_assert_true (launch_started);
  g_assert_true (launch_failed);
  g_assert_false (launched);

  g_clear_error (&error);
  g_clear_object (&app_info);
  g_clear_object (&context);
  g_clear_list (&uris, g_object_unref);
}

static void
test_launch_fail (void)
{
  GAppInfo *app_info;
  GError *error = NULL;

  app_info = g_app_info_create_from_commandline ("this-must-not-exist‼",
                                                 "failing app",
                                                 G_APP_INFO_CREATE_NONE,
                                                 &error);
  g_assert_no_error (error);

  g_assert_false (g_app_info_launch (app_info, NULL, NULL, &error));
  g_assert_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);

  g_clear_error (&error);
  g_clear_object (&app_info);
}

static void
test_launch_fail_argument (void)
{
  GAppInfo *app_info;
  GList *args = NULL;
  GFile *file;
  GFileIOStream *iostream;
  GError *error = NULL;

  app_info = g_app_info_create_from_commandline ("this-must-not-exist‼",
                                                 "failing app with argument",
                                                 G_APP_INFO_CREATE_NONE,
                                                 &error);
  g_assert_no_error (error);

  file = g_file_new_tmp ("test_desktop_app_info_launch_fail_argument_XXXXXX",
                         &iostream, NULL);
  g_assert_nonnull (file);
  g_assert_nonnull (iostream);
  g_clear_object (&iostream);

  args = g_list_prepend (args, file);
  g_assert_false (g_app_info_launch (app_info, args, NULL, &error));
  g_assert_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);

  g_clear_error (&error);
  g_clear_object (&app_info);

  g_file_delete (file, NULL, NULL);
  g_clear_object (&file);
  g_list_free (args);
}

static void
test_launch_fail_absolute_path (void)
{
  GAppInfo *app_info;
  GError *error = NULL;

  app_info = g_app_info_create_from_commandline ("/nothing/of/this-must-exist‼",
                                                 NULL,
                                                 G_APP_INFO_CREATE_NONE,
                                                 &error);
  g_assert_no_error (error);

  g_assert_false (g_app_info_launch (app_info, NULL, NULL, &error));
  g_assert_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);

  g_clear_error (&error);
  g_clear_object (&app_info);

  app_info = g_app_info_create_from_commandline ("/",
                                                 NULL,
                                                 G_APP_INFO_CREATE_NONE,
                                                 &error);
  g_assert_no_error (error);

  g_assert_false (g_app_info_launch (app_info, NULL, NULL, &error));
  g_assert_error (error, G_SPAWN_ERROR, G_SPAWN_ERROR_NOENT);

  g_clear_error (&error);
  g_clear_object (&app_info);
}

static void
async_result_cb (GObject      *source_object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  GAsyncResult **result_out = user_data;

  g_assert (*result_out == NULL);
  *result_out = g_object_ref (result);
  g_main_context_wakeup (g_main_context_get_thread_default ());
}

static void
test_launch_fail_dbus (void)
{
  GTestDBus *bus = NULL;
  GDesktopAppInfo *app_info = NULL;
  GAppLaunchContext *context = NULL;
  GAsyncResult *result = NULL;
  GError *error = NULL;

  if (skip_missing_dbus_daemon ())
    return;

  /* Set up a test session bus to ensure that launching the app happens using
   * D-Bus rather than spawning. */
  bus = g_test_dbus_new (G_TEST_DBUS_NONE);
  g_test_dbus_up (bus);

  app_info = g_desktop_app_info_new_from_filename (g_test_get_filename (G_TEST_DIST, "org.gtk.test.dbusappinfo.desktop", NULL));
  g_assert_nonnull (app_info);

  g_assert_true (g_desktop_app_info_has_key (app_info, "DBusActivatable"));

  context = g_app_launch_context_new ();

  g_app_info_launch_uris_async (G_APP_INFO (app_info), NULL, context, NULL, async_result_cb, &result);

  while (result == NULL)
    g_main_context_iteration (NULL, TRUE);

  g_assert_false (g_app_info_launch_uris_finish (G_APP_INFO (app_info), result, &error));
  g_assert_error (error, G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN);

  g_test_dbus_down (bus);
  g_clear_object (&bus);

  g_clear_error (&error);
  g_clear_object (&result);
  g_clear_object (&context);
  g_clear_object (&app_info);
}

int
main (int   argc,
      char *argv[])
{
  guint i;
  const gchar *supported_terminals[] = {
    "xdg-terminal-exec",
    "kgx",
    "gnome-terminal",
    "mate-terminal",
    "xfce4-terminal",
    "tilix",
    "konsole",
    "nxterm",
    "color-xterm",
    "rxvt",
    "dtterm",
    "xterm",
  };

  /* While we use %G_TEST_OPTION_ISOLATE_DIRS to create temporary directories
   * for each of the tests, we want to use the system MIME registry, assuming
   * that it exists and correctly has shared-mime-info installed. */
  g_content_type_set_mime_dirs (NULL);

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/desktop-app-info/delete", test_delete);
  g_test_add_func ("/desktop-app-info/default", test_default);
  g_test_add_func ("/desktop-app-info/default-async", test_default_async);
  g_test_add_func ("/desktop-app-info/fallback", test_fallback);
  g_test_add_func ("/desktop-app-info/lastused", test_last_used);
  g_test_add_func ("/desktop-app-info/extra-getters", test_extra_getters);
  g_test_add_func ("/desktop-app-info/actions", test_actions);
  g_test_add_func ("/desktop-app-info/search", test_search);
  g_test_add_func ("/desktop-app-info/implements", test_implements);
  g_test_add_func ("/desktop-app-info/show-in", test_show_in);
  g_test_add_func ("/desktop-app-info/app-path", test_app_path);
  g_test_add_func ("/desktop-app-info/app-path/wrong", test_app_path_wrong);
  g_test_add_func ("/desktop-app-info/invalid-key-file", test_invalid_key_file);
  g_test_add_func ("/desktop-app-info/launch/fail", test_launch_fail);
  g_test_add_func ("/desktop-app-info/launch/fail-argument", test_launch_fail_argument);
  g_test_add_func ("/desktop-app-info/launch/fail-absolute-path", test_launch_fail_absolute_path);
  g_test_add_func ("/desktop-app-info/launch/fail-startup-notify", test_launch_startup_notify_fail);
  g_test_add_func ("/desktop-app-info/launch/fail-dbus", test_launch_fail_dbus);
  g_test_add_func ("/desktop-app-info/launch-as-manager", test_launch_as_manager);
  g_test_add_func ("/desktop-app-info/launch-as-manager/fail", test_launch_as_manager_fail);
  g_test_add_func ("/desktop-app-info/launch-default-uri-handler", test_default_uri_handler);
  g_test_add_func ("/desktop-app-info/launch-default-uri-handler-async", test_default_uri_handler_async);
  g_test_add_func ("/desktop-app-info/launch-snap-uri-with-portal", launch_snap_uris_with_portal);
  g_test_add_func ("/desktop-app-info/id", test_id);

  for (i = 0; i < G_N_ELEMENTS (supported_terminals); i++)
    {
      char *path;

      path = g_strdup_printf ("/desktop-app-info/launch-uris-with-terminal/with-path/%s",
                              supported_terminals[i]);
      g_test_add_data_func_full (path,
                                 terminal_launch_data_new (supported_terminals[i],
                                                           TERMINAL_LAUNCH_TYPE_COMMAND_LINE_WITH_PATH_OVERRIDE),
                                 test_launch_uris_with_terminal, g_free);
      g_clear_pointer (&path, g_free);

      path = g_strdup_printf ("/desktop-app-info/launch-uris-with-terminal/with-context/%s",
                              supported_terminals[i]);
      g_test_add_data_func_full (path,
                                 terminal_launch_data_new (supported_terminals[i],
                                                           TERMINAL_LAUNCH_TYPE_COMMAND_LINE_WITH_CONTEXT),
                                 test_launch_uris_with_terminal, g_free);
      g_clear_pointer (&path, g_free);

      path = g_strdup_printf ("/desktop-app-info/launch-uris-with-terminal/with-desktop-path/%s",
                              supported_terminals[i]);
      g_test_add_data_func_full (path,
                                 terminal_launch_data_new (supported_terminals[i],
                                                           TERMINAL_LAUNCH_TYPE_KEY_FILE_WITH_PATH),
                                 test_launch_uris_with_terminal, g_free);
      g_clear_pointer (&path, g_free);
    }

  g_test_add_func ("/desktop-app-info/launch-uris-with-terminal/invalid-glib-terminal",
                   test_launch_uris_with_invalid_terminal);

  return g_test_run ();
}
