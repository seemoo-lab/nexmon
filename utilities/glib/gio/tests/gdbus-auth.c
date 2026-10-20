/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008-2013 Red Hat, Inc.
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
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include <locale.h>
#include <gio/gio.h>

#include <string.h>
#include <unistd.h>

#include "gdbus-tests.h"

#ifdef G_OS_UNIX
#include <gio/gunixconnection.h>
#include <gio/gnetworkingprivate.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gunixfdlist.h>
#endif

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
server_on_allow_mechanism (GDBusAuthObserver *observer,
                           const gchar       *mechanism,
                           gpointer           user_data)
{
  const gchar *allowed_mechanism = user_data;
  if (allowed_mechanism == NULL || g_strcmp0 (mechanism, allowed_mechanism) == 0)
    return TRUE;
  else
    return FALSE;
}

/* pass NULL to allow any mechanism */
static GDBusServer *
server_new_for_mechanism (const gchar *allowed_mechanism)
{
  gchar *addr;
  gchar *guid;
  GDBusServer *server;
  GDBusAuthObserver *auth_observer;
  GError *error;
  GDBusServerFlags flags;

  guid = g_dbus_generate_guid ();

#ifdef G_OS_UNIX
  gchar *tmpdir;
  tmpdir = g_dir_make_tmp ("gdbus-test-XXXXXX", NULL);
  addr = g_strdup_printf ("unix:tmpdir=%s", tmpdir);
  g_free (tmpdir);
#else
  addr = g_strdup ("nonce-tcp:");
#endif

  auth_observer = g_dbus_auth_observer_new ();

  flags = G_DBUS_SERVER_FLAGS_NONE;
  if (g_strcmp0 (allowed_mechanism, "ANONYMOUS") == 0)
    flags |= G_DBUS_SERVER_FLAGS_AUTHENTICATION_ALLOW_ANONYMOUS;

  error = NULL;
  server = g_dbus_server_new_sync (addr,
                                   flags,
                                   guid,
                                   auth_observer,
                                   NULL, /* cancellable */
                                   &error);
  g_assert_no_error (error);
  g_assert (server != NULL);

  g_signal_connect (auth_observer,
                    "allow-mechanism",
                    G_CALLBACK (server_on_allow_mechanism),
                    (gpointer) allowed_mechanism);

  g_free (addr);
  g_free (guid);
  g_object_unref (auth_observer);

  return server;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
test_auth_on_new_connection (GDBusServer     *server,
                             GDBusConnection *connection,
                             gpointer         user_data)
{
  GMainLoop *loop = user_data;
  g_main_loop_quit (loop);
  return FALSE;
}

typedef struct
{
  const gchar *address;
  const gchar *allowed_client_mechanism;
  const gchar *allowed_server_mechanism;
} TestAuthData;

static gpointer
test_auth_client_thread_func (gpointer user_data)
{
  TestAuthData *data = user_data;
  GDBusConnection *c = NULL;
  GError *error = NULL;
  GDBusAuthObserver *auth_observer = NULL;

  auth_observer = g_dbus_auth_observer_new ();

  g_signal_connect (auth_observer,
                    "allow-mechanism",
                    G_CALLBACK (server_on_allow_mechanism),
                    (gpointer) data->allowed_client_mechanism);

  c = g_dbus_connection_new_for_address_sync (data->address,
                                              G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
                                              auth_observer,
                                              NULL, /* GCancellable */
                                              &error);
  g_assert_no_error (error);
  g_assert (c != NULL);
  g_clear_object (&c);
  g_clear_object (&auth_observer);
  return NULL;
}

static void
test_auth_mechanism (const gchar *allowed_client_mechanism,
                     const gchar *allowed_server_mechanism)
{
  GDBusServer *server;
  GMainLoop *loop;
  GThread *client_thread;
  TestAuthData data;

  server = server_new_for_mechanism (allowed_server_mechanism);

  loop = g_main_loop_new (NULL, FALSE);

  g_signal_connect (server,
                    "new-connection",
                    G_CALLBACK (test_auth_on_new_connection),
                    loop);

  data.allowed_client_mechanism = allowed_client_mechanism;
  data.allowed_server_mechanism = allowed_server_mechanism;
  data.address = g_dbus_server_get_client_address (server);

  /* Run the D-Bus client in a thread. If this hangs forever, the test harness
   * (typically Meson) will eventually kill the test. */
  client_thread = g_thread_new ("gdbus-client-thread",
                                test_auth_client_thread_func,
                                &data);

  g_dbus_server_start (server);

  g_main_loop_run (loop);

  g_dbus_server_stop (server);

  g_thread_join (client_thread);

  while (g_main_context_iteration (NULL, FALSE));
  g_main_loop_unref (loop);

  g_object_unref (server);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
auth_client_external (void)
{
  test_auth_mechanism ("EXTERNAL", NULL);
}

static void
auth_client_dbus_cookie_sha1 (void)
{
  test_auth_mechanism ("DBUS_COOKIE_SHA1", NULL);
}

static void
auth_server_anonymous (void)
{
  test_auth_mechanism (NULL, "ANONYMOUS");
}

static void
auth_server_external (void)
{
  test_auth_mechanism (NULL, "EXTERNAL");
}

static void
auth_server_dbus_cookie_sha1 (void)
{
  test_auth_mechanism (NULL, "DBUS_COOKIE_SHA1");
}

/* ---------------------------------------------------------------------------------------------------- */

static gchar *temp_dbus_keyrings_dir = NULL;

static void
temp_dbus_keyrings_setup (void)
{
  GError *error = NULL;

  g_assert (temp_dbus_keyrings_dir == NULL);
  temp_dbus_keyrings_dir = g_dir_make_tmp ("gdbus-test-dbus-keyrings-XXXXXX", &error);
  g_assert_no_error (error);
  g_assert (temp_dbus_keyrings_dir != NULL);
  g_setenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR", temp_dbus_keyrings_dir, TRUE);
  g_setenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR_IGNORE_PERMISSION", "1", TRUE);
}

static void
temp_dbus_keyrings_teardown (void)
{
  GDir *dir;
  GError *error = NULL;
  const gchar *name;

  g_assert (temp_dbus_keyrings_dir != NULL);

  dir = g_dir_open (temp_dbus_keyrings_dir, 0, &error);
  g_assert_no_error (error);
  g_assert (dir != NULL);
  while ((name = g_dir_read_name (dir)) != NULL)
    {
      gchar *path = g_build_filename (temp_dbus_keyrings_dir, name, NULL);
      g_assert (unlink (path) == 0);
      g_free (path);
    }
  g_dir_close (dir);
  g_assert (rmdir (temp_dbus_keyrings_dir) == 0);

  g_free (temp_dbus_keyrings_dir);
  temp_dbus_keyrings_dir = NULL;
  g_unsetenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR");
  g_unsetenv ("G_DBUS_COOKIE_SHA1_KEYRING_DIR_IGNORE_PERMISSION");
}

static void
async_result_cb (GObject      *obj,
                 GAsyncResult *result,
                 void         *user_data)
{
  GAsyncResult **result_out = user_data;

  g_assert (result_out != NULL);
  g_assert (*result_out == NULL);

  *result_out = g_object_ref (result);
  g_main_context_wakeup (g_main_context_get_thread_default ());
}

static gboolean
server_new_connection_unexpected_cb (GDBusServer     *server,
                                     GDBusConnection *connection,
                                     void            *user_data)
{
  g_assert_not_reached ();
  return FALSE;
}

static void
test_auth_server_read_limit (void)
{
  GDBusServer *server = NULL;
  unsigned long new_connection_id = 0;
  const char *server_address;
  GIOStream *client_stream = NULL;
  GOutputStream *client_output_stream;
  GInputStream *client_input_stream;
  GAsyncResult *result = NULL;
  char *write_buffer = NULL;
  char read_buffer[100];
  ssize_t read_len;
  size_t bytes_written;
  GError *local_error = NULL;

  g_test_summary ("Test that GDBusServer limits the lengths of reads it does during auth from a client");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3985");

  server = server_new_for_mechanism (NULL);

  new_connection_id = g_signal_connect (server,
                                        "new-connection",
                                        G_CALLBACK (server_new_connection_unexpected_cb),
                                        NULL);
  server_address = g_dbus_server_get_client_address (server);
  g_dbus_server_start (server);

  /* Start connecting as a client */
  g_dbus_address_get_stream (server_address, NULL, async_result_cb, &result);

  while (result == NULL)
    g_main_context_iteration (NULL, TRUE);

  client_stream = g_dbus_address_get_stream_finish (result, NULL, &local_error);
  g_assert_no_error (local_error);
  g_clear_object (&result);

  /* Send an over-long AUTH line, maliciously */
  client_output_stream = g_io_stream_get_output_stream (client_stream);
  client_input_stream = g_io_stream_get_input_stream (client_stream);

  write_buffer = g_strdup_printf ("AUTH DBUS_COOKIE_SHA1 context%0*d 123 456\r\n", 5000, 0);

  g_output_stream_write_all_async (client_output_stream,
                                   write_buffer,
                                   strlen (write_buffer),
                                   G_PRIORITY_DEFAULT,
                                   NULL,
                                   async_result_cb,
                                   &result);

  while (result == NULL)
    g_main_context_iteration (NULL, TRUE);

  g_output_stream_write_all_finish (client_output_stream, result, &bytes_written, &local_error);
  g_assert_no_error (local_error);
  g_assert_cmpuint (bytes_written, ==, strlen (write_buffer));
  g_clear_object (&result);

  g_clear_pointer (&write_buffer, g_free);

  /* Authentication should have been rejected, so reading or writing the stream
   * should now fail. */
  read_len = g_input_stream_read (client_input_stream,
                                  read_buffer,
                                  sizeof (read_buffer),
                                  NULL,
                                  &local_error);
  if (read_len != 0)
    {
      g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED);
      g_assert_cmpint (read_len, <, 0);
    }
  g_clear_error (&local_error);

  write_buffer = g_strdup_printf ("AUTH\r\n");

  g_output_stream_write_all (client_output_stream,
                             write_buffer,
                             strlen (write_buffer),
                             &bytes_written,
                             NULL,
                             &local_error);
  g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED);
  g_assert_cmpuint (bytes_written, ==, 0);
  g_clear_error (&local_error);

  g_clear_pointer (&write_buffer, g_free);

  /* Cleanup */
  g_clear_object (&client_stream);
  g_dbus_server_stop (server);

  g_clear_signal_handler (&new_connection_id, server);
  g_clear_object (&server);
}

/* ---------------------------------------------------------------------------------------------------- */

int
main (int   argc,
      char *argv[])
{
  gint ret;

  setlocale (LC_ALL, "C");

  temp_dbus_keyrings_setup ();

  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/gdbus/auth/client/EXTERNAL",         auth_client_external);
  g_test_add_func ("/gdbus/auth/client/DBUS_COOKIE_SHA1", auth_client_dbus_cookie_sha1);
  g_test_add_func ("/gdbus/auth/server/ANONYMOUS",        auth_server_anonymous);
  g_test_add_func ("/gdbus/auth/server/EXTERNAL",         auth_server_external);
  g_test_add_func ("/gdbus/auth/server/DBUS_COOKIE_SHA1", auth_server_dbus_cookie_sha1);
  g_test_add_func ("/gdbus/auth/server/read-limit", test_auth_server_read_limit);

  /* TODO: we currently don't have tests for
   *
   *  - DBUS_COOKIE_SHA1 timeouts (and clock changes etc)
   *  - interoperability with libdbus-1 implementations of authentication methods (both client and server)
   */

  ret = g_test_run();

  temp_dbus_keyrings_teardown ();

  return ret;
}
