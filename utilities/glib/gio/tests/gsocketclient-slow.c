/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2018 Igalia S.L.
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
#include <gio/gnetworking.h> /* For IPV6_V6ONLY */

static void
on_connected (GObject      *source_object,
              GAsyncResult *result,
              gpointer      user_data)
{
  GSocketConnection *conn;
  GError *error = NULL;

  conn = g_socket_client_connect_to_uri_finish (G_SOCKET_CLIENT (source_object), result, &error);
  g_assert_no_error (error);

  g_object_unref (conn);
  g_main_loop_quit (user_data);
}

static void
test_happy_eyeballs (void)
{
  GSocketClient *client;
  GSocketService *service;
  GError *error = NULL;
  guint16 port;
  GMainLoop *loop;

  loop = g_main_loop_new (NULL, FALSE);

  service = g_socket_service_new ();
  port = g_socket_listener_add_any_inet_port (G_SOCKET_LISTENER (service), NULL, &error);
  g_assert_no_error (error);
  g_socket_service_start (service);

  /* All of the magic here actually happens in slow-connect-preload.c
   * which as you would guess is preloaded. So this is just making a
   * normal connection that happens to take 600ms each time. This will
   * trigger the logic to make multiple parallel connections.
   */
  client = g_socket_client_new ();
  g_socket_client_connect_to_host_async (client, "localhost", port, NULL, on_connected, loop);
  g_main_loop_run (loop);

  g_main_loop_unref (loop);
  g_object_unref (service);
  g_object_unref (client);
}

static void
on_connected_cancelled (GObject      *source_object,
                        GAsyncResult *result,
                        gpointer      user_data)
{
  GSocketConnection *conn;
  GError *error = NULL;

  conn = g_socket_client_connect_to_uri_finish (G_SOCKET_CLIENT (source_object), result, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_null (conn);

  g_error_free (error);
  g_main_loop_quit (user_data);
}

typedef struct
{
  GCancellable *cancellable;
  gboolean completed;
} EventCallbackData;

static void
on_event (GSocketClient      *client,
          GSocketClientEvent  event,
          GSocketConnectable *connectable,
          GIOStream          *connection,
          EventCallbackData  *data)
{
  if (data->cancellable && event == G_SOCKET_CLIENT_CONNECTED)
    {
      g_cancellable_cancel (data->cancellable);
    }
  else if (event == G_SOCKET_CLIENT_COMPLETE)
    {
      data->completed = TRUE;
      g_assert_null (connection);
    }
}

static void
test_happy_eyeballs_cancel_delayed (void)
{
  GSocketClient *client;
  GSocketService *service;
  GError *error = NULL;
  guint16 port;
  GMainLoop *loop;
  EventCallbackData data = { NULL, FALSE };

  /* This just tests that cancellation works as expected, still emits the completed signal,
   * and never returns a connection */

  loop = g_main_loop_new (NULL, FALSE);

  service = g_socket_service_new ();
  port = g_socket_listener_add_any_inet_port (G_SOCKET_LISTENER (service), NULL, &error);
  g_assert_no_error (error);
  g_socket_service_start (service);

  client = g_socket_client_new ();
  data.cancellable = g_cancellable_new ();
  g_socket_client_connect_to_host_async (client, "localhost", port, data.cancellable, on_connected_cancelled, loop);
  g_signal_connect (client, "event", G_CALLBACK (on_event), &data);
  g_main_loop_run (loop);

  g_assert_true (data.completed);
  g_main_loop_unref (loop);
  g_object_unref (service);
  g_object_unref (client);
  g_object_unref (data.cancellable);
}

static void
test_happy_eyeballs_cancel_instant (void)
{
  GSocketClient *client;
  GSocketService *service;
  GError *error = NULL;
  guint16 port;
  GMainLoop *loop;
  GCancellable *cancel;
  EventCallbackData data = { NULL, FALSE };

  /* This tests the same things as above, test_happy_eyeballs_cancel_delayed(), but
   * with different timing since it sends an already cancelled cancellable */

  loop = g_main_loop_new (NULL, FALSE);

  service = g_socket_service_new ();
  port = g_socket_listener_add_any_inet_port (G_SOCKET_LISTENER (service), NULL, &error);
  g_assert_no_error (error);
  g_socket_service_start (service);

  client = g_socket_client_new ();
  cancel = g_cancellable_new ();
  g_cancellable_cancel (cancel);
  g_socket_client_connect_to_host_async (client, "localhost", port, cancel, on_connected_cancelled, loop);
  g_signal_connect (client, "event", G_CALLBACK (on_event), &data);
  g_main_loop_run (loop);

  g_assert_true (data.completed);
  g_main_loop_unref (loop);
  g_object_unref (service);
  g_object_unref (client);
  g_object_unref (cancel);
}

static void
async_result_cb (GObject      *source,
                 GAsyncResult *res,
                 gpointer      user_data)
{
  GAsyncResult **result_out = user_data;

  g_assert_null (*result_out);
  *result_out = g_object_ref (res);

  g_main_context_wakeup (NULL);
}

static void
test_connection_failed (void)
{
  GSocketClient *client = NULL;
  GInetAddress *inet_address = NULL;
  GSocketAddress *address = NULL;
  GSocket *socket = NULL;
  guint16 port;
  GAsyncResult *async_result = NULL;
  GSocketConnection *conn = NULL;
  GError *local_error = NULL;

  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3184");

  inet_address = g_inet_address_new_any (G_SOCKET_FAMILY_IPV6);
  address = g_inet_socket_address_new (inet_address, 0);
  g_object_unref (inet_address);

  socket = g_socket_new (G_SOCKET_FAMILY_IPV6, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &local_error);
  g_assert_no_error (local_error);
  g_socket_set_option (socket, IPPROTO_IPV6, IPV6_V6ONLY, FALSE, NULL);
  g_socket_bind (socket, address, TRUE, &local_error);
  g_assert_no_error (local_error);

  /* reserve a port without listening so we know that connecting to it will fail */
  g_object_unref (address);
  address = g_socket_get_local_address (socket, &local_error);
  g_assert_no_error (local_error);
  port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (address));

  client = g_socket_client_new ();
  /* Connect to the port we have reserved but do not listen to. Because of the slow connection
   * caused by slow-connect-preload.c and the fact that we try to connect to both IPv4 and IPv6
   * we will in some way exercise the code path in try_next_connection_or_finish() that ends
   * with a call to complete_connection_with_error(). This path previously had a memory leak.
   * Note that the slowness is important, because without it we could bail out already in the
   * address enumeration phase because it finishes when there are no connection attempts in
   * progress. */
  g_socket_client_connect_to_host_async (client, "localhost", port, NULL, async_result_cb, &async_result);

  while (async_result == NULL)
    g_main_context_iteration (NULL, TRUE);

  conn = g_socket_client_connect_to_uri_finish (client, async_result, &local_error);
  g_assert_nonnull (local_error);
  g_assert_cmpint (local_error->domain, ==, G_IO_ERROR);
  g_assert_null (conn);
  g_clear_error (&local_error);
  g_clear_object (&async_result);

  g_clear_object (&client);
  g_clear_object (&socket);
  g_clear_object (&address);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/socket-client/happy-eyeballs/slow", test_happy_eyeballs);
  g_test_add_func ("/socket-client/happy-eyeballs/cancellation/instant", test_happy_eyeballs_cancel_instant);
  g_test_add_func ("/socket-client/happy-eyeballs/cancellation/delayed", test_happy_eyeballs_cancel_delayed);
  g_test_add_func ("/socket-client/connection-fail", test_connection_failed);

  return g_test_run ();
}
