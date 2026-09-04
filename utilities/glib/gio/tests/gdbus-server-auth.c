/*
 * Copyright 2019 Collabora Ltd.
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

#include "config.h"

#include <errno.h>

#include <glib/gstdio.h>
#include <gio/gio.h>

/* For G_CREDENTIALS_*_SUPPORTED */
#include <gio/gcredentialsprivate.h>

#ifdef HAVE_DBUS1
#include <dbus/dbus.h>
#endif

typedef enum
{
  INTEROP_FLAGS_EXTERNAL = (1 << 0),
  INTEROP_FLAGS_ANONYMOUS = (1 << 1),
  INTEROP_FLAGS_SHA1 = (1 << 2),
  INTEROP_FLAGS_TCP = (1 << 3),
  INTEROP_FLAGS_LIBDBUS = (1 << 4),
  INTEROP_FLAGS_ABSTRACT = (1 << 5),
  INTEROP_FLAGS_REQUIRE_SAME_USER = (1 << 6),
  INTEROP_FLAGS_NONE = 0
} G_GNUC_FLAG_ENUM InteropFlags;

static gboolean
allow_external_cb (G_GNUC_UNUSED GDBusAuthObserver *observer,
                   const char *mechanism,
                   G_GNUC_UNUSED gpointer user_data)
{
  if (g_strcmp0 (mechanism, "EXTERNAL") == 0)
    {
      g_debug ("Accepting EXTERNAL authentication");
      return TRUE;
    }
  else
    {
      g_debug ("Rejecting \"%s\" authentication: not EXTERNAL", mechanism);
      return FALSE;
    }
}

static gboolean
allow_anonymous_cb (G_GNUC_UNUSED GDBusAuthObserver *observer,
                    const char *mechanism,
                    G_GNUC_UNUSED gpointer user_data)
{
  if (g_strcmp0 (mechanism, "ANONYMOUS") == 0)
    {
      g_debug ("Accepting ANONYMOUS authentication");
      return TRUE;
    }
  else
    {
      g_debug ("Rejecting \"%s\" authentication: not ANONYMOUS", mechanism);
      return FALSE;
    }
}

static gboolean
allow_sha1_cb (G_GNUC_UNUSED GDBusAuthObserver *observer,
               const char *mechanism,
               G_GNUC_UNUSED gpointer user_data)
{
  if (g_strcmp0 (mechanism, "DBUS_COOKIE_SHA1") == 0)
    {
      g_debug ("Accepting DBUS_COOKIE_SHA1 authentication");
      return TRUE;
    }
  else
    {
      g_debug ("Rejecting \"%s\" authentication: not DBUS_COOKIE_SHA1",
               mechanism);
      return FALSE;
    }
}

static gboolean
allow_any_mechanism_cb (G_GNUC_UNUSED GDBusAuthObserver *observer,
                        const char *mechanism,
                        G_GNUC_UNUSED gpointer user_data)
{
  g_debug ("Accepting \"%s\" authentication", mechanism);
  return TRUE;
}

static gboolean
authorize_any_authenticated_peer_cb (G_GNUC_UNUSED GDBusAuthObserver *observer,
                                     G_GNUC_UNUSED GIOStream *stream,
                                     GCredentials *credentials,
                                     G_GNUC_UNUSED gpointer user_data)
{
  if (credentials == NULL)
    {
      g_debug ("Authorizing peer with no credentials");
    }
  else
    {
      gchar *str = g_credentials_to_string (credentials);

      g_debug ("Authorizing peer with credentials: %s", str);
      g_free (str);
    }

  return TRUE;
}

static GDBusMessage *
whoami_filter_cb (GDBusConnection *connection,
                  GDBusMessage *message,
                  gboolean incoming,
                  G_GNUC_UNUSED gpointer user_data)
{
  if (!incoming)
    return message;

  if (g_dbus_message_get_message_type (message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL &&
      g_strcmp0 (g_dbus_message_get_member (message), "WhoAmI") == 0)
    {
      GDBusMessage *reply = g_dbus_message_new_method_reply (message);
      gint64 uid = -1;
      gint64 pid = -1;
#ifdef G_OS_UNIX
      GCredentials *credentials = g_dbus_connection_get_peer_credentials (connection);

      if (credentials != NULL)
        {
          uid = (gint64) g_credentials_get_unix_user (credentials, NULL);
          pid = (gint64) g_credentials_get_unix_pid (credentials, NULL);
        }
#endif

      g_dbus_message_set_body (reply,
                               g_variant_new ("(xx)", uid, pid));
      g_dbus_connection_send_message (connection, reply,
                                      G_DBUS_SEND_MESSAGE_FLAGS_NONE,
                                      NULL, NULL);
      g_object_unref (reply);

      /* handled */
      g_object_unref (message);
      return NULL;
    }

  return message;
}

static gboolean
new_connection_cb (G_GNUC_UNUSED GDBusServer *server,
                   GDBusConnection *connection,
                   G_GNUC_UNUSED gpointer user_data)
{
  GCredentials *credentials = g_dbus_connection_get_peer_credentials (connection);

  if (credentials == NULL)
    {
      g_debug ("New connection from peer with no credentials");
    }
  else
    {
      gchar *str = g_credentials_to_string (credentials);

      g_debug ("New connection from peer with credentials: %s", str);
      g_free (str);
    }

  g_object_ref (connection);
  g_dbus_connection_add_filter (connection, whoami_filter_cb, NULL, NULL);
  return TRUE;
}

#ifdef HAVE_DBUS1
typedef struct
{
  DBusError error;
  DBusConnection *conn;
  DBusMessage *call;
  DBusMessage *reply;
} LibdbusCall;

static void
libdbus_call_task_cb (GTask *task,
                      G_GNUC_UNUSED gpointer source_object,
                      gpointer task_data,
                      G_GNUC_UNUSED GCancellable *cancellable)
{
  LibdbusCall *libdbus_call = task_data;

  libdbus_call->reply = dbus_connection_send_with_reply_and_block (libdbus_call->conn,
                                                                   libdbus_call->call,
                                                                   -1,
                                                                   &libdbus_call->error);
  g_task_return_boolean (task, TRUE);
}
#endif /* HAVE_DBUS1 */

static void
store_result_cb (G_GNUC_UNUSED GObject *source_object,
                 GAsyncResult *res,
                 gpointer user_data)
{
  GAsyncResult **result = user_data;

  g_assert_nonnull (result);
  g_assert_null (*result);
  *result = g_object_ref (res);
}

static void
assert_expected_uid_pid (InteropFlags flags,
                         gint64 uid,
                         gint64 pid)
{
#ifdef G_OS_UNIX
  if (flags & (INTEROP_FLAGS_ANONYMOUS | INTEROP_FLAGS_SHA1 | INTEROP_FLAGS_TCP))
    {
      /* No assertion. There is no guarantee whether credentials will be
       * passed even though we didn't send them. Conversely, if
       * credentials were not passed,
       * g_dbus_connection_get_peer_credentials() always returns the
       * credentials of the socket, and not the uid that a
       * client might have proved it has by using DBUS_COOKIE_SHA1. */
    }
  else    /* We should prefer EXTERNAL whenever it is allowed. */
    {
#ifdef __linux__
      /* We know that both GDBus and libdbus support full credentials-passing
       * on Linux. */
      g_assert_cmpint (uid, ==, getuid ());
      g_assert_cmpint (pid, ==, getpid ());
#elif defined(__APPLE__)
      /* We know (or at least suspect) that both GDBus and libdbus support
       * passing the uid only on macOS. */
      g_assert_cmpint (uid, ==, getuid ());
      /* No pid here */
#else
      g_test_message ("Please open a merge request to add appropriate "
                      "assertions for your platform");
#endif
    }
#endif /* G_OS_UNIX */
}

static void
do_test_server_auth (InteropFlags flags)
{
  GError *error = NULL;
  gchar *tmpdir = NULL;
  gchar *listenable_address = NULL;
  GDBusServer *server = NULL;
  GDBusAuthObserver *observer = NULL;
  GDBusServerFlags server_flags = G_DBUS_SERVER_FLAGS_RUN_IN_THREAD;
  gchar *guid = NULL;
  const char *connectable_address;
  GDBusConnection *client = NULL;
  GAsyncResult *result = NULL;
  GVariant *tuple = NULL;
  gint64 uid, pid;
#ifdef HAVE_DBUS1
  /* GNOME/glib#1831 seems to involve a race condition, so try a few times
   * to see if we can trigger it. */
  gsize i;
  gsize n = 20;
#endif

  if (flags & INTEROP_FLAGS_TCP)
    {
      listenable_address = g_strdup ("tcp:host=127.0.0.1");
    }
  else
    {
#ifdef G_OS_UNIX
      gchar *escaped;

      tmpdir = g_dir_make_tmp ("gdbus-server-auth-XXXXXX", &error);
      g_assert_no_error (error);
      escaped = g_dbus_address_escape_value (tmpdir);
      listenable_address = g_strdup_printf ("unix:%s=%s",
                                            (flags & INTEROP_FLAGS_ABSTRACT) ? "tmpdir" : "dir",
                                            escaped);
      g_free (escaped);
#else
      g_test_skip ("unix: addresses only work on Unix");
      goto out;
#endif
    }

  g_test_message ("Testing GDBus server at %s / libdbus client, with flags: "
                  "external:%s "
                  "anonymous:%s "
                  "sha1:%s "
                  "abstract:%s "
                  "tcp:%s",
                  listenable_address,
                  (flags & INTEROP_FLAGS_EXTERNAL) ? "true" : "false",
                  (flags & INTEROP_FLAGS_ANONYMOUS) ? "true" : "false",
                  (flags & INTEROP_FLAGS_SHA1) ? "true" : "false",
                  (flags & INTEROP_FLAGS_ABSTRACT) ? "true" : "false",
                  (flags & INTEROP_FLAGS_TCP) ? "true" : "false");

#if !defined(G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED) \
  && !defined(G_CREDENTIALS_SOCKET_GET_CREDENTIALS_SUPPORTED)
  if (flags & INTEROP_FLAGS_EXTERNAL)
    {
      g_test_skip ("EXTERNAL authentication not implemented on this platform");
      goto out;
    }
#endif

  if (flags & INTEROP_FLAGS_ANONYMOUS)
    server_flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;
  if (flags & INTEROP_FLAGS_REQUIRE_SAME_USER)
    server_flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_REQUIRE_SAME_USER;

  observer = g_dbus_auth_observer_new ();

  if (flags & INTEROP_FLAGS_EXTERNAL)
    g_signal_connect (observer, "allow-mechanism",
                      G_CALLBACK (allow_external_cb), NULL);
  else if (flags & INTEROP_FLAGS_ANONYMOUS)
    g_signal_connect (observer, "allow-mechanism",
                      G_CALLBACK (allow_anonymous_cb), NULL);
  else if (flags & INTEROP_FLAGS_SHA1)
    g_signal_connect (observer, "allow-mechanism",
                      G_CALLBACK (allow_sha1_cb), NULL);
  else
    g_signal_connect (observer, "allow-mechanism",
                      G_CALLBACK (allow_any_mechanism_cb), NULL);

  g_signal_connect (observer, "authorize-authenticated-peer",
                    G_CALLBACK (authorize_any_authenticated_peer_cb),
                    NULL);

  guid = g_dbus_generate_guid ();
  server = g_dbus_server_new_sync (listenable_address,
                                   server_flags,
                                   guid,
                                   observer,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (server);
  g_signal_connect (server, "new-connection", G_CALLBACK (new_connection_cb), NULL);
  g_dbus_server_start (server);
  connectable_address = g_dbus_server_get_client_address (server);
  g_test_message ("Connectable address: %s", connectable_address);

  result = NULL;
  g_dbus_connection_new_for_address (connectable_address,
                                     G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                     NULL, NULL, store_result_cb, &result);

  while (result == NULL)
    g_main_context_iteration (NULL, TRUE);

  client = g_dbus_connection_new_for_address_finish (result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (client);
  g_clear_object (&result);

  g_dbus_connection_call (client, NULL, "/", "com.example.Test", "WhoAmI",
                          NULL, G_VARIANT_TYPE ("(xx)"),
                          G_DBUS_CALL_FLAGS_NONE, -1, NULL, store_result_cb,
                          &result);

  while (result == NULL)
    g_main_context_iteration (NULL, TRUE);

  tuple = g_dbus_connection_call_finish (client, result, &error);
  g_assert_no_error (error);
  g_assert_nonnull (tuple);
  g_clear_object (&result);
  g_clear_object (&client);

  uid = -2;
  pid = -2;
  g_variant_get (tuple, "(xx)", &uid, &pid);

  g_debug ("Server says GDBus client is uid %" G_GINT64_FORMAT ", pid %" G_GINT64_FORMAT,
           uid, pid);

  assert_expected_uid_pid (flags, uid, pid);

  g_clear_pointer (&tuple, g_variant_unref);

#ifdef HAVE_DBUS1
  for (i = 0; i < n; i++)
    {
      LibdbusCall libdbus_call = { DBUS_ERROR_INIT, NULL, NULL, NULL };
      GTask *task;

      /* The test suite uses %G_TEST_OPTION_ISOLATE_DIRS, which sets
       * `HOME=/dev/null` and leaves g_get_home_dir() pointing to the per-test
       * temp home directory. Unfortunately, libdbus doesn’t allow the home dir
       * to be overridden except using the environment, so copy the per-test
       * temp home directory back there so that libdbus uses the same
       * `$HOME/.dbus-keyrings` path as GLib. This is not thread-safe. */
      g_setenv ("HOME", g_get_home_dir (), TRUE);

      libdbus_call.conn = dbus_connection_open_private (connectable_address,
                                                        &libdbus_call.error);
      g_assert_cmpstr (libdbus_call.error.name, ==, NULL);
      g_assert_nonnull (libdbus_call.conn);

      libdbus_call.call = dbus_message_new_method_call (NULL, "/",
                                                        "com.example.Test",
                                                        "WhoAmI");

      if (libdbus_call.call == NULL)
        g_error ("Out of memory");

      result = NULL;
      task = g_task_new (NULL, NULL, store_result_cb, &result);
      g_task_set_task_data (task, &libdbus_call, NULL);
      g_task_run_in_thread (task, libdbus_call_task_cb);

      while (result == NULL)
        g_main_context_iteration (NULL, TRUE);

      g_clear_object (&result);

      g_assert_cmpstr (libdbus_call.error.name, ==, NULL);
      g_assert_nonnull (libdbus_call.reply);

      uid = -2;
      pid = -2;
      dbus_message_get_args (libdbus_call.reply, &libdbus_call.error,
                             DBUS_TYPE_INT64, &uid,
                             DBUS_TYPE_INT64, &pid,
                             DBUS_TYPE_INVALID);
      g_assert_cmpstr (libdbus_call.error.name, ==, NULL);

      g_debug ("Server says libdbus client %" G_GSIZE_FORMAT " is uid %" G_GINT64_FORMAT ", pid %" G_GINT64_FORMAT,
               i, uid, pid);
      assert_expected_uid_pid (flags | INTEROP_FLAGS_LIBDBUS, uid, pid);

      dbus_connection_close (libdbus_call.conn);
      dbus_connection_unref (libdbus_call.conn);
      dbus_message_unref (libdbus_call.call);
      dbus_message_unref (libdbus_call.reply);
      g_clear_object (&task);
    }
#else /* !HAVE_DBUS1 */
  g_test_skip ("Testing interop with libdbus not supported");
#endif /* !HAVE_DBUS1 */

  /* No practical effect, just to avoid -Wunused-label under some
   * combinations of #ifdefs */
  goto out;

out:
  if (server != NULL)
    g_dbus_server_stop (server);

  if (tmpdir != NULL)
    g_assert_cmpstr (g_rmdir (tmpdir) == 0 ? "OK" : g_strerror (errno),
                     ==, "OK");

  g_clear_object (&server);
  g_clear_object (&observer);
  g_free (guid);
  g_free (listenable_address);
  g_free (tmpdir);
}

static void
test_server_auth (void)
{
  do_test_server_auth (INTEROP_FLAGS_NONE);
}

static void
test_server_auth_abstract (void)
{
  do_test_server_auth (INTEROP_FLAGS_ABSTRACT);
}

static void
test_server_auth_tcp (void)
{
  do_test_server_auth (INTEROP_FLAGS_TCP);
}

static void
test_server_auth_anonymous (void)
{
  do_test_server_auth (INTEROP_FLAGS_ANONYMOUS);
}

static void
test_server_auth_anonymous_tcp (void)
{
  do_test_server_auth (INTEROP_FLAGS_ANONYMOUS | INTEROP_FLAGS_TCP);
}

static void
test_server_auth_external (void)
{
  do_test_server_auth (INTEROP_FLAGS_EXTERNAL);
}

static void
test_server_auth_external_require_same_user (void)
{
  do_test_server_auth (INTEROP_FLAGS_EXTERNAL | INTEROP_FLAGS_REQUIRE_SAME_USER);
}

static void
test_server_auth_sha1 (void)
{
  do_test_server_auth (INTEROP_FLAGS_SHA1);
}

static void
test_server_auth_sha1_tcp (void)
{
  do_test_server_auth (INTEROP_FLAGS_SHA1 | INTEROP_FLAGS_TCP);
}

static void
test_server_path_in_use (void)
{
#ifdef G_OS_UNIX
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX G_SIZEOF_MEMBER (struct sockaddr_un, sun_path)
#endif
  GError *error = NULL;
  GString *path = NULL;
  gchar *listenable_address = NULL;
  GDBusServer *server = NULL;
  gchar *guid = NULL;
  gchar *escaped = NULL;

  path = g_string_new (g_get_tmp_dir ());
  guid = g_dbus_generate_guid ();
  while (path->len < UNIX_PATH_MAX - 1)
    g_string_append_c (path, 'x');
  escaped = g_dbus_address_escape_value (path->str);
  listenable_address = g_strdup_printf ("unix:%s=%s", "dir", escaped);
  g_assert_cmpint (g_mkdir_with_parents (path->str, 0700), ==, 0);
  g_string_free (path, TRUE);

  server = g_dbus_server_new_sync (listenable_address,
                                   G_DBUS_SERVER_FLAGS_RUN_IN_THREAD,
                                   guid,
                                   NULL,
                                   NULL,
                                   &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_ADDRESS_IN_USE);
  g_error_free (error);

  if (server != NULL)
    g_dbus_server_stop (server);

  g_clear_object (&server);
  g_free (guid);
  g_free (escaped);
  g_free (listenable_address);
#else
  g_test_skip ("unix: addresses only work on Unix");
#endif
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/gdbus/server-auth", test_server_auth);
  g_test_add_func ("/gdbus/server-auth/abstract", test_server_auth_abstract);
  g_test_add_func ("/gdbus/server-auth/tcp", test_server_auth_tcp);
  g_test_add_func ("/gdbus/server-auth/anonymous", test_server_auth_anonymous);
  g_test_add_func ("/gdbus/server-auth/anonymous/tcp", test_server_auth_anonymous_tcp);
  g_test_add_func ("/gdbus/server-auth/external", test_server_auth_external);
  g_test_add_func ("/gdbus/server-auth/external/require-same-user", test_server_auth_external_require_same_user);
  g_test_add_func ("/gdbus/server-auth/sha1", test_server_auth_sha1);
  g_test_add_func ("/gdbus/server-auth/sha1/tcp", test_server_auth_sha1_tcp);
  g_test_add_func ("/gdbus/server-auth/path-in-use", test_server_path_in_use);

  return g_test_run();
}
