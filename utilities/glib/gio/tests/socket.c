/* GLib testing framework examples and tests
 *
 * Copyright (C) 2008-2011 Red Hat, Inc.
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
#include <glib/gstdio.h>
#include "glib-private.h"

#include <gio/gcredentialsprivate.h>
#include <gio/gunixconnection.h>

#ifdef G_OS_UNIX
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <gio/gnetworking.h>
#endif

#ifdef G_OS_WIN32
#include "giowin32-afunix.h"
#include <io.h>
#endif

#include "gnetworkingprivate.h"

static gboolean ipv6_supported;

typedef struct {
  GSocket *server;  /* (owned) (not nullable) */
  GSocket *client;  /* (owned) (nullable) */
  GSocketFamily family;
  GThread *thread;  /* (owned) (not nullable) */
  GMainLoop *loop;  /* (owned) (nullable) */
  GCancellable *cancellable; /* to shut down dgram echo server thread; (owned) (nullable) */
} IPTestData;

static void
ip_test_data_free (IPTestData *data)
{
  g_clear_object (&data->server);
  g_clear_object (&data->client);
  g_clear_pointer (&data->loop, g_main_loop_unref);
  g_clear_object (&data->cancellable);

  g_slice_free (IPTestData, data);
}

static gpointer
echo_server_dgram_thread (gpointer user_data)
{
  IPTestData *data = user_data;
  GSocketAddress *sa;
  GCancellable *cancellable = data->cancellable;
  GSocket *sock;
  GError *error = NULL;
  gssize nread, nwrote;
  gchar buf[128];

  sock = data->server;

  while (TRUE)
    {
      nread = g_socket_receive_from (sock, &sa, buf, sizeof (buf), cancellable, &error);
      if (error && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        break;
      g_assert_no_error (error);
      g_assert_cmpint (nread, >=, 0);

      nwrote = g_socket_send_to (sock, sa, buf, nread, cancellable, &error);
      if (error && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        break;
      g_assert_no_error (error);
      g_assert_cmpint (nwrote, ==, nread);

      g_object_unref (sa);
    }

  g_clear_error (&error);

  return NULL;
}

static gpointer
echo_server_thread (gpointer user_data)
{
  IPTestData *data = user_data;
  GSocket *sock;
  GError *error = NULL;
  gssize nread, nwrote;
  gchar buf[128];

  sock = g_socket_accept (data->server, NULL, &error);
  g_assert_no_error (error);

  while (TRUE)
    {
      nread = g_socket_receive (sock, buf, sizeof (buf), NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (nread, >=, 0);

      if (nread == 0)
	break;

      nwrote = g_socket_send (sock, buf, nread, NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (nwrote, ==, nread);
    }

  g_socket_close (sock, &error);
  g_assert_no_error (error);
  g_object_unref (sock);
  return NULL;
}

static IPTestData *
create_server_full (GSocketFamily   family,
                    GSocketType     socket_type,
                    GThreadFunc     server_thread,
                    gboolean        v4mapped,
                    GError        **error)
{
  IPTestData *data;
  GSocket *server;
  GSocketAddress *addr;
  GInetAddress *iaddr;

  data = g_slice_new0 (IPTestData);
  data->family = family;

  data->server = server = g_socket_new (family,
					socket_type,
					G_SOCKET_PROTOCOL_DEFAULT,
					error);
  if (server == NULL)
    goto error;

  g_assert_cmpint (g_socket_get_family (server), ==, family);
  g_assert_cmpint (g_socket_get_socket_type (server), ==, socket_type);
  g_assert_cmpint (g_socket_get_protocol (server), ==, G_SOCKET_PROTOCOL_DEFAULT);
#ifdef G_OS_WIN32
  g_assert_true (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) ((HANDLE)(gintptr) g_socket_get_fd (server)));
#endif

  g_socket_set_blocking (server, TRUE);

#if defined (IPPROTO_IPV6) && defined (IPV6_V6ONLY)
  if (v4mapped)
    {
      g_socket_set_option (data->server, IPPROTO_IPV6, IPV6_V6ONLY, FALSE, NULL);
      if (!g_socket_speaks_ipv4 (data->server))
        {
          g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                               "IPv6-only server cannot speak IPv4");
          goto error;
        }
    }
#endif

  if (v4mapped)
    iaddr = g_inet_address_new_any (family);
  else
    iaddr = g_inet_address_new_loopback (family);
  addr = g_inet_socket_address_new (iaddr, 0);
  g_object_unref (iaddr);

  g_assert_cmpint (g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr)), ==, 0);
  if (!g_socket_bind (server, addr, TRUE, error))
    {
      g_object_unref (addr);
      goto error;
    }
  g_object_unref (addr);

  addr = g_socket_get_local_address (server, error);
  if (addr == NULL)
    goto error;
  g_assert_cmpint (g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr)), !=, 0);
  g_object_unref (addr);

  if (socket_type == G_SOCKET_TYPE_STREAM)
    {
      if (!g_socket_listen (server, error))
        goto error;
    }
  else
    {
      data->cancellable = g_cancellable_new ();
    }

  data->thread = g_thread_new ("server", server_thread, data);

  return data;

error:
  ip_test_data_free (data);

  return NULL;
}

static IPTestData *
create_server (GSocketFamily   family,
               GThreadFunc     server_thread,
               gboolean        v4mapped,
               GError        **error)
{
  return create_server_full (family, G_SOCKET_TYPE_STREAM, server_thread, v4mapped, error);
}

static const gchar *testbuf = "0123456789abcdef";

static gboolean
test_ip_async_read_ready (GSocket      *client,
			  GIOCondition  cond,
			  gpointer      user_data)
{
  IPTestData *data = user_data;
  GError *error = NULL;
  gssize len;
  gchar buf[128];

  g_assert_cmpint (cond, ==, G_IO_IN);

  len = g_socket_receive (client, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  g_assert_cmpstr (testbuf, ==, buf);

  g_main_loop_quit (data->loop);

  return FALSE;
}

static gboolean
test_ip_async_write_ready (GSocket      *client,
			   GIOCondition  cond,
			   gpointer      user_data)
{
  IPTestData *data = user_data;
  GError *error = NULL;
  GSource *source;
  gssize len;

  g_assert_cmpint (cond, ==, G_IO_OUT);

  len = g_socket_send (client, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  source = g_socket_create_source (client, G_IO_IN, NULL);
  g_source_set_callback (source, (GSourceFunc)test_ip_async_read_ready,
			 data, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);

  return FALSE;
}

static gboolean
test_ip_async_timed_out (GSocket      *client,
			 GIOCondition  cond,
			 gpointer      user_data)
{
  IPTestData *data = user_data;
  GError *error = NULL;
  GSource *source;
  gssize len;
  gchar buf[128];

  if (data->family == G_SOCKET_FAMILY_IPV4)
    {
      g_assert_cmpint (cond, ==, G_IO_IN);
      len = g_socket_receive (client, buf, sizeof (buf), NULL, &error);
      g_assert_cmpint (len, ==, -1);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
      g_clear_error (&error);
    }

  source = g_socket_create_source (client, G_IO_OUT, NULL);
  g_source_set_callback (source, (GSourceFunc)test_ip_async_write_ready,
			 data, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);

  return FALSE;
}

static gboolean
test_ip_async_connected (GSocket      *client,
			 GIOCondition  cond,
			 gpointer      user_data)
{
  IPTestData *data = user_data;
  GError *error = NULL;
  GSource *source;
  gssize len;
  gchar buf[128];

  g_socket_check_connect_result (client, &error);
  g_assert_no_error (error);
  /* We do this after the check_connect_result, since that will give a
   * more useful assertion in case of error.
   */
  g_assert_cmpint (cond, ==, G_IO_OUT);

  g_assert_true (g_socket_is_connected (client));

  /* This adds 1 second to "make check", so let's just only do it once. */
  if (data->family == G_SOCKET_FAMILY_IPV4)
    {
      len = g_socket_receive (client, buf, sizeof (buf), NULL, &error);
      g_assert_cmpint (len, ==, -1);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
      g_clear_error (&error);

      source = g_socket_create_source (client, G_IO_IN, NULL);
      g_source_set_callback (source, (GSourceFunc)test_ip_async_timed_out,
			     data, NULL);
      g_source_attach (source, NULL);
      g_source_unref (source);
    }
  else
    test_ip_async_timed_out (client, 0, data);

  return FALSE;
}

static gboolean
idle_test_ip_async_connected (gpointer user_data)
{
  IPTestData *data = user_data;

  return test_ip_async_connected (data->client, G_IO_OUT, data);
}

static void
test_ip_async (GSocketFamily family)
{
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocketAddress *addr;
  GSource *source;
  gssize len;
  gchar buf[128];

  data = create_server (family, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }
  g_assert_nonnull (data);

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (family,
			 G_SOCKET_TYPE_STREAM,
			 G_SOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);
  data->client = client;

  g_assert_cmpint (g_socket_get_family (client), ==, family);
  g_assert_cmpint (g_socket_get_socket_type (client), ==, G_SOCKET_TYPE_STREAM);
  g_assert_cmpint (g_socket_get_protocol (client), ==, G_SOCKET_PROTOCOL_DEFAULT);

  g_socket_set_blocking (client, FALSE);
  g_socket_set_timeout (client, 1);

  if (g_socket_connect (client, addr, NULL, &error))
    {
      g_assert_no_error (error);
      g_idle_add (idle_test_ip_async_connected, data);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_PENDING);
      g_clear_error (&error);
      source = g_socket_create_source (client, G_IO_OUT, NULL);
      g_source_set_callback (source, (GSourceFunc)test_ip_async_connected,
			     data, NULL);
      g_source_attach (source, NULL);
      g_source_unref (source);
    }
  g_object_unref (addr);

  data->loop = g_main_loop_new (NULL, TRUE);
  g_main_loop_run (data->loop);
  g_clear_pointer (&data->loop, g_main_loop_unref);

  g_socket_shutdown (client, FALSE, TRUE, &error);
  g_assert_no_error (error);

  g_thread_join (data->thread);

  if (family == G_SOCKET_FAMILY_IPV4)
    {
      /* Test that reading on a remote-closed socket gets back 0 bytes. */
      len = g_socket_receive_with_blocking (client, buf, sizeof (buf),
					    TRUE, NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (len, ==, 0);
    }
  else
    {
      /* Test that writing to a remote-closed socket gets back CONNECTION_CLOSED. */
      len = g_socket_send_with_blocking (client, testbuf, strlen (testbuf) + 1,
					 TRUE, NULL, &error);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED);
      g_assert_cmpint (len, ==, -1);
      g_clear_error (&error);
    }

  g_socket_close (client, &error);
  g_assert_no_error (error);
  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  ip_test_data_free (data);
}

static void
test_ipv4_async (void)
{
  test_ip_async (G_SOCKET_FAMILY_IPV4);
}

static void
test_ipv6_async (void)
{
  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  test_ip_async (G_SOCKET_FAMILY_IPV6);
}

static const gchar testbuf2[] = "0123456789abcdefghijklmnopqrstuvwxyz";

static void
test_ip_sync (GSocketFamily family)
{
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocketAddress *addr;
  gssize len;
  gchar buf[128];

  data = create_server (family, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (family,
			 G_SOCKET_TYPE_STREAM,
			 G_SOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_assert_cmpint (g_socket_get_family (client), ==, family);
  g_assert_cmpint (g_socket_get_socket_type (client), ==, G_SOCKET_TYPE_STREAM);
  g_assert_cmpint (g_socket_get_protocol (client), ==, G_SOCKET_PROTOCOL_DEFAULT);
#ifdef G_OS_WIN32
  g_assert_true (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) ((HANDLE)(gintptr) g_socket_get_fd (client)));
#endif
  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 1);

  g_socket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (g_socket_is_connected (client));
  g_object_unref (addr);

  /* This adds 1 second to "make check", so let's just only do it once. */
  if (family == G_SOCKET_FAMILY_IPV4)
    {
      len = g_socket_receive (client, buf, sizeof (buf), NULL, &error);
      g_assert_cmpint (len, ==, -1);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
      g_clear_error (&error);
    }

  len = g_socket_send (client, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);
  
  len = g_socket_receive (client, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  g_assert_cmpstr (testbuf, ==, buf);

  {
    GOutputVector v[7] = { { NULL, 0 }, };

    v[0].buffer = testbuf2 + 0;
    v[0].size = 3;
    v[1].buffer = testbuf2 + 3;
    v[1].size = 5;
    v[2].buffer = testbuf2 + 3 + 5;
    v[2].size = 0;
    v[3].buffer = testbuf2 + 3 + 5;
    v[3].size = 6;
    v[4].buffer = testbuf2 + 3 + 5 + 6;
    v[4].size = 2;
    v[5].buffer = testbuf2 + 3 + 5 + 6 + 2;
    v[5].size = 1;
    v[6].buffer = testbuf2 + 3 + 5 + 6 + 2 + 1;
    v[6].size = strlen (testbuf2) - (3 + 5 + 6 + 2 + 1);

    len = g_socket_send_message (client, NULL, v, G_N_ELEMENTS (v), NULL, 0, 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, strlen (testbuf2));

    memset (buf, 0, sizeof (buf));
    len = g_socket_receive (client, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, strlen (testbuf2));
    g_assert_cmpstr (testbuf2, ==, buf);
  }

  g_socket_shutdown (client, FALSE, TRUE, &error);
  g_assert_no_error (error);

  g_thread_join (data->thread);

  if (family == G_SOCKET_FAMILY_IPV4)
    {
      /* Test that reading on a remote-closed socket gets back 0 bytes. */
      len = g_socket_receive (client, buf, sizeof (buf), NULL, &error);
      g_assert_no_error (error);
      g_assert_cmpint (len, ==, 0);
    }
  else
    {
      /* Test that writing to a remote-closed socket gets back CONNECTION_CLOSED. */
      len = g_socket_send (client, testbuf, strlen (testbuf) + 1, NULL, &error);
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CONNECTION_CLOSED);
      g_assert_cmpint (len, ==, -1);
      g_clear_error (&error);
    }

  g_socket_close (client, &error);
  g_assert_no_error (error);
  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  g_object_unref (client);

  ip_test_data_free (data);
}

static void
test_ipv4_sync (void)
{
  test_ip_sync (G_SOCKET_FAMILY_IPV4);
}

static void
test_ipv6_sync (void)
{
  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  test_ip_sync (G_SOCKET_FAMILY_IPV6);
}

static void
test_ip_sync_dgram (GSocketFamily family)
{
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocketAddress *dest_addr;
  gssize len;
  gchar buf[128];

  data = create_server_full (family, G_SOCKET_TYPE_DATAGRAM,
                             echo_server_dgram_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  dest_addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (family,
			 G_SOCKET_TYPE_DATAGRAM,
			 G_SOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_assert_cmpint (g_socket_get_family (client), ==, family);
  g_assert_cmpint (g_socket_get_socket_type (client), ==, G_SOCKET_TYPE_DATAGRAM);
  g_assert_cmpint (g_socket_get_protocol (client), ==, G_SOCKET_PROTOCOL_DEFAULT);
#ifdef G_OS_WIN32
  g_assert_true (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) ((HANDLE)(gintptr) g_socket_get_fd (client)));
#endif

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 1);

  len = g_socket_send_to (client, dest_addr, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  len = g_socket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  g_assert_cmpstr (testbuf, ==, buf);

  {
    GOutputMessage m[3] = { { NULL, NULL, 0, 0, NULL, 0 }, };
    GInputMessage im[3] = { { NULL, NULL, 0, 0, 0, NULL, 0 }, };
    GOutputVector v[7] = { { NULL, 0 }, };
    GInputVector iv[7] = { { NULL, 0 }, };

    v[0].buffer = testbuf2 + 0;
    v[0].size = 3;
    v[1].buffer = testbuf2 + 3;
    v[1].size = 5;
    v[2].buffer = testbuf2 + 3 + 5;
    v[2].size = 0;
    v[3].buffer = testbuf2 + 3 + 5;
    v[3].size = 6;
    v[4].buffer = testbuf2 + 3 + 5 + 6;
    v[4].size = 2;
    v[5].buffer = testbuf2 + 3 + 5 + 6 + 2;
    v[5].size = 1;
    v[6].buffer = testbuf2 + 3 + 5 + 6 + 2 + 1;
    v[6].size = strlen (testbuf2) - (3 + 5 + 6 + 2 + 1);

    iv[0].buffer = buf + 0;
    iv[0].size = 3;
    iv[1].buffer = buf + 3;
    iv[1].size = 5;
    iv[2].buffer = buf + 3 + 5;
    iv[2].size = 0;
    iv[3].buffer = buf + 3 + 5;
    iv[3].size = 6;
    iv[4].buffer = buf + 3 + 5 + 6;
    iv[4].size = 2;
    iv[5].buffer = buf + 3 + 5 + 6 + 2;
    iv[5].size = 1;
    iv[6].buffer = buf + 3 + 5 + 6 + 2 + 1;
    iv[6].size = sizeof (buf) - (3 + 5 + 6 + 2 + 1);

    len = g_socket_send_message (client, dest_addr, v, G_N_ELEMENTS (v), NULL, 0, 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, strlen (testbuf2));

    memset (buf, 0, sizeof (buf));
    len = g_socket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, strlen (testbuf2));
    g_assert_cmpstr (testbuf2, ==, buf);

    m[0].vectors = &v[0];
    m[0].num_vectors = 1;
    m[0].address = dest_addr;
    m[1].vectors = &v[0];
    m[1].num_vectors = 6;
    m[1].address = dest_addr;
    m[2].vectors = &v[6];
    m[2].num_vectors = 1;
    m[2].address = dest_addr;

    len = g_socket_send_messages (client, m, G_N_ELEMENTS (m), 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, G_N_ELEMENTS (m));
    g_assert_cmpint (m[0].bytes_sent, ==, 3);
    g_assert_cmpint (m[1].bytes_sent, ==, 17);
    g_assert_cmpint (m[2].bytes_sent, ==, v[6].size);

    memset (buf, 0, sizeof (buf));
    len = g_socket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, 3);

    memset (buf, 0, sizeof (buf));
    len = g_socket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    /* v[0].size + v[1].size + v[2].size + v[3].size + v[4].size + v[5].size */
    g_assert_cmpint (len, ==, 17);
    g_assert_cmpmem (testbuf2, 17, buf, 17);

    memset (buf, 0, sizeof (buf));
    len = g_socket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, v[6].size);
    g_assert_cmpstr (buf, ==, v[6].buffer);

    /* reset since we're re-using the message structs */
    m[0].bytes_sent = 0;
    m[1].bytes_sent = 0;
    m[2].bytes_sent = 0;

    /* now try receiving multiple messages */
    len = g_socket_send_messages (client, m, G_N_ELEMENTS (m), 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, G_N_ELEMENTS (m));
    g_assert_cmpint (m[0].bytes_sent, ==, 3);
    g_assert_cmpint (m[1].bytes_sent, ==, 17);
    g_assert_cmpint (m[2].bytes_sent, ==, v[6].size);

    im[0].vectors = &iv[0];
    im[0].num_vectors = 1;
    im[1].vectors = &iv[0];
    im[1].num_vectors = 6;
    im[2].vectors = &iv[6];
    im[2].num_vectors = 1;

    memset (buf, 0, sizeof (buf));
    len = g_socket_receive_messages (client, im, G_N_ELEMENTS (im), 0,
                                     NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, G_N_ELEMENTS (im));

    g_assert_cmpuint (im[0].bytes_received, ==, 3);
    /* v[0].size + v[1].size + v[2].size + v[3].size + v[4].size + v[5].size */
    g_assert_cmpuint (im[1].bytes_received, ==, 17);
    g_assert_cmpuint (im[2].bytes_received, ==, v[6].size);

    /* reset since we're re-using the message structs */
    m[0].bytes_sent = 0;
    m[1].bytes_sent = 0;
    m[2].bytes_sent = 0;

    /* now try to generate an early return by omitting the destination address on [1] */
    m[1].address = NULL;
    len = g_socket_send_messages (client, m, G_N_ELEMENTS (m), 0, NULL, &error);
    g_assert_no_error (error);
    g_assert_cmpint (len, ==, 1);

    g_assert_cmpint (m[0].bytes_sent, ==, 3);
    g_assert_cmpint (m[1].bytes_sent, ==, 0);
    g_assert_cmpint (m[2].bytes_sent, ==, 0);

    /* reset since we're re-using the message structs */
    m[0].bytes_sent = 0;
    m[1].bytes_sent = 0;
    m[2].bytes_sent = 0;

    /* now try to generate an error by omitting all destination addresses */
    m[0].address = NULL;
    m[1].address = NULL;
    m[2].address = NULL;
    len = g_socket_send_messages (client, m, G_N_ELEMENTS (m), 0, NULL, &error);
    /* This error code may vary between platforms and over time; it is not guaranteed API: */
#ifndef G_OS_WIN32
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_DESTINATION_UNSET);
#else
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_CONNECTED);
#endif
    g_clear_error (&error);
    g_assert_cmpint (len, ==, -1);

    g_assert_cmpint (m[0].bytes_sent, ==, 0);
    g_assert_cmpint (m[1].bytes_sent, ==, 0);
    g_assert_cmpint (m[2].bytes_sent, ==, 0);

    len = g_socket_receive_from (client, NULL, buf, sizeof (buf), NULL, &error);
    g_assert_cmpint (len, ==, 3);
  }

  g_cancellable_cancel (data->cancellable);

  g_thread_join (data->thread);

  g_socket_close (client, &error);
  g_assert_no_error (error);
  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  g_object_unref (client);
  g_object_unref (dest_addr);

  ip_test_data_free (data);
}

static void
test_ipv4_sync_dgram (void)
{
  test_ip_sync_dgram (G_SOCKET_FAMILY_IPV4);
}

static void
test_ipv6_sync_dgram (void)
{
  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  test_ip_sync_dgram (G_SOCKET_FAMILY_IPV6);
}

static gpointer
cancellable_thread_cb (gpointer data)
{
  GCancellable *cancellable = data;

  g_usleep ((guint64) (0.1 * G_USEC_PER_SEC));
  g_cancellable_cancel (cancellable);
  g_object_unref (cancellable);

  return NULL;
}

static void
test_ip_sync_dgram_timeouts (GSocketFamily family)
{
  GError *error = NULL;
  GSocket *client = NULL;
  GCancellable *cancellable = NULL;
  GThread *cancellable_thread = NULL;
  gssize len;
#ifdef G_OS_WIN32
  GInetAddress *iaddr;
  GSocketAddress *addr;
#endif

  client = g_socket_new (family,
                         G_SOCKET_TYPE_DATAGRAM,
                         G_SOCKET_PROTOCOL_DEFAULT,
                         &error);
  g_assert_no_error (error);

  g_assert_cmpint (g_socket_get_family (client), ==, family);
  g_assert_cmpint (g_socket_get_socket_type (client), ==, G_SOCKET_TYPE_DATAGRAM);
  g_assert_cmpint (g_socket_get_protocol (client), ==, G_SOCKET_PROTOCOL_DEFAULT);
#ifdef G_OS_WIN32
  g_assert_true (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) ((HANDLE)(gintptr) g_socket_get_fd (client)));
#endif

#ifdef G_OS_WIN32
  /* Winsock can't recv() on unbound udp socket */
  iaddr = g_inet_address_new_loopback (family);
  addr = g_inet_socket_address_new (iaddr, 0);
  g_object_unref (iaddr);
  g_socket_bind (client, addr, TRUE, &error);
  g_object_unref (addr);
  g_assert_no_error (error);
#endif

  /* No overall timeout: test the per-operation timeouts instead. */
  g_socket_set_timeout (client, 0);

  cancellable = g_cancellable_new ();

  /* Check for timeouts when no server is running. */
  {
    gint64 start_time;
    GInputMessage im = { NULL, NULL, 0, 0, 0, NULL, 0 };
    GInputVector iv = { NULL, 0 };
    guint8 buf[128];

    iv.buffer = buf;
    iv.size = sizeof (buf);

    im.vectors = &iv;
    im.num_vectors = 1;

    memset (buf, 0, sizeof (buf));

    /* Try a non-blocking read. */
    g_socket_set_blocking (client, FALSE);
    len = g_socket_receive_messages (client, &im, 1, 0  /* flags */,
                                     NULL, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
    g_assert_cmpint (len, ==, -1);
    g_clear_error (&error);

    /* Try a timeout read. Can’t really validate the time taken more than
     * checking it’s positive. */
    g_socket_set_timeout (client, 1);
    g_socket_set_blocking (client, TRUE);
    start_time = g_get_monotonic_time ();
    len = g_socket_receive_messages (client, &im, 1, 0  /* flags */,
                                     NULL, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
    g_assert_cmpint (len, ==, -1);
    g_assert_cmpint (g_get_monotonic_time () - start_time, >, 0);
    g_clear_error (&error);

    /* Try a blocking read, cancelled from another thread. */
    g_socket_set_timeout (client, 0);
    cancellable_thread = g_thread_new ("cancellable",
                                       cancellable_thread_cb,
                                       g_object_ref (cancellable));

    start_time = g_get_monotonic_time ();
    len = g_socket_receive_messages (client, &im, 1, 0  /* flags */,
                                     cancellable, &error);
    g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
    g_assert_cmpint (len, ==, -1);
    g_assert_cmpint (g_get_monotonic_time () - start_time, >, 0);
    g_clear_error (&error);

    g_thread_join (cancellable_thread);
  }

  g_socket_close (client, &error);
  g_assert_no_error (error);

  g_object_unref (client);
  g_object_unref (cancellable);
}

static void
test_ipv4_sync_dgram_timeouts (void)
{
  test_ip_sync_dgram_timeouts (G_SOCKET_FAMILY_IPV4);
}

static void
test_ipv6_sync_dgram_timeouts (void)
{
  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  test_ip_sync_dgram_timeouts (G_SOCKET_FAMILY_IPV6);
}

static gpointer
graceful_server_thread (gpointer user_data)
{
  IPTestData *data = user_data;
  GSocket *sock;
  GError *error = NULL;
  gssize len;

  sock = g_socket_accept (data->server, NULL, &error);
  g_assert_no_error (error);

  len = g_socket_send (sock, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  return sock;
}

static void
test_close_graceful (void)
{
  GSocketFamily family = G_SOCKET_FAMILY_IPV4;
  IPTestData *data;
  GError *error = NULL;
  GSocket *client, *server;
  GSocketAddress *addr;
  gssize len;
  gchar buf[128];

  data = create_server (family, graceful_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (family,
			 G_SOCKET_TYPE_STREAM,
			 G_SOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_assert_cmpint (g_socket_get_family (client), ==, family);
  g_assert_cmpint (g_socket_get_socket_type (client), ==, G_SOCKET_TYPE_STREAM);
  g_assert_cmpint (g_socket_get_protocol (client), ==, G_SOCKET_PROTOCOL_DEFAULT);
#ifdef G_OS_WIN32
  g_assert_true (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) ((HANDLE)(gintptr) g_socket_get_fd (client)));
#endif

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 1);

  g_socket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (g_socket_is_connected (client));
  g_object_unref (addr);

  server = g_thread_join (data->thread);

  /* similar to g_tcp_connection_set_graceful_disconnect(), but explicit */
  g_socket_shutdown (server, FALSE, TRUE, &error);
  g_assert_no_error (error);

  /* we must timeout */
  g_socket_condition_wait (client, G_IO_HUP, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
  g_clear_error (&error);

  /* check that the remaining data is received */
  len = g_socket_receive (client, buf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  /* and only then the connection is closed */
  len = g_socket_receive (client, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, 0);

  g_socket_close (server, &error);
  g_assert_no_error (error);

  g_socket_close (client, &error);
  g_assert_no_error (error);

  g_object_unref (server);
  g_object_unref (client);

  ip_test_data_free (data);
}

#if defined (IPPROTO_IPV6) && defined (IPV6_V6ONLY)
static gpointer
v4mapped_server_thread (gpointer user_data)
{
  IPTestData *data = user_data;
  GSocket *sock;
  GError *error = NULL;
  GSocketAddress *addr;

  sock = g_socket_accept (data->server, NULL, &error);
  g_assert_no_error (error);

  g_assert_cmpint (g_socket_get_family (sock), ==, G_SOCKET_FAMILY_IPV6);

  addr = g_socket_get_local_address (sock, &error);
  g_assert_no_error (error);
  g_assert_cmpint (g_socket_address_get_family (addr), ==, G_SOCKET_FAMILY_IPV4);
  g_object_unref (addr);

  addr = g_socket_get_remote_address (sock, &error);
  g_assert_no_error (error);
  g_assert_cmpint (g_socket_address_get_family (addr), ==, G_SOCKET_FAMILY_IPV4);
  g_object_unref (addr);

  g_socket_close (sock, &error);
  g_assert_no_error (error);
  g_object_unref (sock);
  return NULL;
}

static void
test_ipv6_v4mapped (void)
{
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocketAddress *addr, *v4addr;
  GInetAddress *iaddr;

  if (!ipv6_supported)
    {
      g_test_skip ("No support for IPv6");
      return;
    }

  data = create_server (G_SOCKET_FAMILY_IPV6, v4mapped_server_thread, TRUE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  client = g_socket_new (G_SOCKET_FAMILY_IPV4,
			 G_SOCKET_TYPE_STREAM,
			 G_SOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 1);

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);
  iaddr = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  v4addr = g_inet_socket_address_new (iaddr, g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (addr)));
  g_object_unref (iaddr);
  g_object_unref (addr);

  g_socket_connect (client, v4addr, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (g_socket_is_connected (client));

  g_thread_join (data->thread);

  g_socket_close (client, &error);
  g_assert_no_error (error);
  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  g_object_unref (client);
  g_object_unref (v4addr);

  ip_test_data_free (data);
}
#endif

static void
test_timed_wait (void)
{
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocketAddress *addr;
  gint64 start_time;
  gint poll_duration;

  if (!g_test_thorough ())
    {
      g_test_skip ("Not running timing heavy test");
      return;
    }

  data = create_server (G_SOCKET_FAMILY_IPV4, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (G_SOCKET_FAMILY_IPV4,
			 G_SOCKET_TYPE_STREAM,
			 G_SOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 1);

  g_socket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (addr);

  start_time = g_get_monotonic_time ();
  g_socket_condition_timed_wait (client, G_IO_IN, 100000 /* 100 ms */,
				 NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
  g_clear_error (&error);
  poll_duration = g_get_monotonic_time () - start_time;

  g_assert_cmpint (poll_duration, >=, 98000);
  g_assert_cmpint (poll_duration, <, 112000);

  g_socket_close (client, &error);
  g_assert_no_error (error);

  g_thread_join (data->thread);

  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  g_object_unref (client);

  ip_test_data_free (data);
}

static int
duplicate_socket_fd (int fd)
{
#ifdef G_OS_WIN32
  WSAPROTOCOL_INFO info;

  if (WSADuplicateSocket ((SOCKET)fd,
                          GetCurrentProcessId (),
                          &info))
    {
      gchar *emsg = g_win32_error_message (WSAGetLastError ());
      g_test_message ("Error duplicating socket: %s", emsg);
      g_free (emsg);
      return -1;
    }

  return (int)WSASocket (FROM_PROTOCOL_INFO,
                         FROM_PROTOCOL_INFO,
                         FROM_PROTOCOL_INFO,
                         &info, 0, 0);
#else
  return dup (fd);
#endif
}

static void
test_fd_reuse (void)
{
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocket *client2;
  GSocketAddress *addr;
  int fd;
  gssize len;
  gchar buf[128];

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=741707");

  data = create_server (G_SOCKET_FAMILY_IPV4, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (G_SOCKET_FAMILY_IPV4,
                         G_SOCKET_TYPE_STREAM,
                         G_SOCKET_PROTOCOL_DEFAULT,
                         &error);
  g_assert_no_error (error);

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 1);

  g_socket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_assert_true (g_socket_is_connected (client));
  g_object_unref (addr);

  /* we have to dup otherwise the fd gets closed twice on unref */
  fd = duplicate_socket_fd (g_socket_get_fd (client));
  client2 = g_socket_new_from_fd (fd, &error);
  g_assert_no_error (error);

  g_assert_cmpint (g_socket_get_family (client2), ==, g_socket_get_family (client));
  g_assert_cmpint (g_socket_get_socket_type (client2), ==, g_socket_get_socket_type (client));
  g_assert_cmpint (g_socket_get_protocol (client2), ==, G_SOCKET_PROTOCOL_TCP);
#ifdef G_OS_WIN32
  g_assert_true (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) ((HANDLE)(gintptr) g_socket_get_fd (client)));
#endif

  len = g_socket_send (client2, testbuf, strlen (testbuf) + 1, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  len = g_socket_receive (client2, buf, sizeof (buf), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen (testbuf) + 1);

  g_assert_cmpstr (testbuf, ==, buf);

  g_socket_shutdown (client, FALSE, TRUE, &error);
  g_assert_no_error (error);
  /* The semantics of dup()+shutdown() are ambiguous; this call will succeed
   * on Linux, but return ENOTCONN on OS X.
   */
  g_socket_shutdown (client2, FALSE, TRUE, NULL);

  g_thread_join (data->thread);

  g_socket_close (client, &error);
  g_assert_no_error (error);
  g_socket_close (client2, &error);
  g_assert_no_error (error);
  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  g_assert_cmpint (g_socket_get_fd (client), ==, -1);
  g_assert_cmpint (g_socket_get_fd (client2), ==, -1);
  g_assert_cmpint (g_socket_get_fd (data->server), ==, -1);

  g_object_unref (client);
  g_object_unref (client2);

  ip_test_data_free (data);
}

static void
test_sockaddr (void)
{
  struct sockaddr_in6 sin6, gsin6;
  GSocketAddress *saddr;
  GInetSocketAddress *isaddr;
  GInetAddress *iaddr;
  GError *error = NULL;

  memset (&sin6, 0, sizeof (sin6));
  sin6.sin6_family = AF_INET6;
  sin6.sin6_addr = in6addr_loopback;
  sin6.sin6_port = g_htons (42);
  sin6.sin6_scope_id = 17;
  sin6.sin6_flowinfo = 1729;

  saddr = g_socket_address_new_from_native (&sin6, sizeof (sin6));
  g_assert_true (G_IS_INET_SOCKET_ADDRESS (saddr));

  isaddr = G_INET_SOCKET_ADDRESS (saddr);
  iaddr = g_inet_socket_address_get_address (isaddr);
  g_assert_cmpint (g_inet_address_get_family (iaddr), ==, G_SOCKET_FAMILY_IPV6);
  g_assert_true (g_inet_address_get_is_loopback (iaddr));

  g_assert_cmpint (g_inet_socket_address_get_port (isaddr), ==, 42);
  g_assert_cmpint (g_inet_socket_address_get_scope_id (isaddr), ==, 17);
  g_assert_cmpint (g_inet_socket_address_get_flowinfo (isaddr), ==, 1729);

  g_socket_address_to_native (saddr, &gsin6, sizeof (gsin6), &error);
  g_assert_no_error (error);

  g_assert_cmpmem (&sin6.sin6_addr, sizeof (struct in6_addr), &gsin6.sin6_addr, sizeof (struct in6_addr));
  g_assert_cmpint (sin6.sin6_port, ==, gsin6.sin6_port);
  g_assert_cmpint (sin6.sin6_scope_id, ==, gsin6.sin6_scope_id);
  g_assert_cmpint (sin6.sin6_flowinfo, ==, gsin6.sin6_flowinfo);

  g_object_unref (saddr);
}

static void
bind_win32_unixfd (int fd)
{
#ifdef G_OS_WIN32
  gsize len;
  int ret;
  struct sockaddr_un addr;

  memset (&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  len = g_snprintf (addr.sun_path, sizeof addr.sun_path, "%s" G_DIR_SEPARATOR_S "%d.sock", g_get_tmp_dir (), fd);
  g_assert_cmpint (len, <=, sizeof addr.sun_path);
  ret = bind (fd, (struct sockaddr *)&addr, sizeof addr);
  g_assert_cmpint (ret, ==, 0);
  g_remove (addr.sun_path);
#endif
}

static void
test_unix_from_fd (void)
{
  gint fd;
  GError *error;
  GSocket *s;

  fd = socket (AF_UNIX, SOCK_STREAM, 0);
#ifdef G_OS_WIN32
  if (fd == -1)
    {
      g_test_skip ("AF_UNIX not supported on this Windows system.");
      return;
    }
#endif
  g_assert_cmpint (fd, !=, -1);

  bind_win32_unixfd (fd);

  error = NULL;
  s = g_socket_new_from_fd (fd, &error);
  g_assert_no_error (error);
  g_assert_cmpint (g_socket_get_family (s), ==, G_SOCKET_FAMILY_UNIX);
  g_assert_cmpint (g_socket_get_socket_type (s), ==, G_SOCKET_TYPE_STREAM);
  g_assert_cmpint (g_socket_get_protocol (s), ==, G_SOCKET_PROTOCOL_DEFAULT);
#ifdef G_OS_WIN32
  g_assert_true (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) ((HANDLE)(gintptr) g_socket_get_fd (s)));
#endif
  g_object_unref (s);
}

static void
test_unix_connection (void)
{
  gint fd;
  GError *error;
  GSocket *s;
  GSocketConnection *c;

  fd = socket (AF_UNIX, SOCK_STREAM, 0);
#ifdef G_OS_WIN32
  if (fd == -1)
    {
      g_test_skip ("AF_UNIX not supported on this Windows system.");
      return;
    }
#endif
  g_assert_cmpint (fd, !=, -1);

  bind_win32_unixfd (fd);

  error = NULL;
  s = g_socket_new_from_fd (fd, &error);
  g_assert_no_error (error);
  c = g_socket_connection_factory_create_connection (s);
  g_assert_true (G_IS_UNIX_CONNECTION (c));
  g_object_unref (c);
  g_object_unref (s);
}

#ifdef G_OS_UNIX
static GSocketConnection *
create_connection_for_fd (int fd)
{
  GError *err = NULL;
  GSocket *socket;
  GSocketConnection *connection;

  socket = g_socket_new_from_fd (fd, &err);
  g_assert_no_error (err);
  g_assert_true (G_IS_SOCKET (socket));
  connection = g_socket_connection_factory_create_connection (socket);
  g_assert_true (G_IS_UNIX_CONNECTION (connection));
  g_object_unref (socket);
  return connection;
}

#define TEST_DATA "failure to say failure to say 'i love gnome-panel!'."

static void
test_unix_connection_ancillary_data (void)
{
  GError *err = NULL;
  gint pv[2], sv[3];
  gint status, fd, len;
  char buffer[1024];
  pid_t pid;

  status = pipe (pv);
  g_assert_cmpint (status, ==, 0);

  status = socketpair (PF_UNIX, SOCK_STREAM, 0, sv);
  g_assert_cmpint (status, ==, 0);

  pid = fork ();
  g_assert_cmpint (pid, >=, 0);

  /* Child: close its copy of the write end of the pipe, receive it
   * again from the parent over the socket, and write some text to it.
   *
   * Parent: send the write end of the pipe (still open for the
   * parent) over the socket, close it, and read some text from the
   * read end of the pipe.
   */
  if (pid == 0)
    {
      GSocketConnection *connection;

      close (sv[1]);
      connection = create_connection_for_fd (sv[0]);

      status = close (pv[1]);
      g_assert_cmpint (status, ==, 0);

      err = NULL;
      fd = g_unix_connection_receive_fd (G_UNIX_CONNECTION (connection), NULL,
					 &err);
      g_assert_no_error (err);
      g_assert_cmpint (fd, >, -1);
      g_object_unref (connection);

      do
	len = write (fd, TEST_DATA, sizeof (TEST_DATA));
      while (len == -1 && errno == EINTR);
      g_assert_cmpint (len, ==, sizeof (TEST_DATA));
      exit (0);
    }
  else
    {
      GSocketConnection *connection;

      close (sv[0]);
      connection = create_connection_for_fd (sv[1]);

      err = NULL;
      g_unix_connection_send_fd (G_UNIX_CONNECTION (connection), pv[1], NULL,
				 &err);
      g_assert_no_error (err);
      g_object_unref (connection);

      status = close (pv[1]);
      g_assert_cmpint (status, ==, 0);

      memset (buffer, 0xff, sizeof buffer);
      do
	len = read (pv[0], buffer, sizeof buffer);
      while (len == -1 && errno == EINTR);

      g_assert_cmpint (len, ==, sizeof (TEST_DATA));
      g_assert_cmpstr (buffer, ==, TEST_DATA);

      waitpid (pid, &status, 0);
      g_assert_true (WIFEXITED (status));
      g_assert_cmpint (WEXITSTATUS (status), ==, 0);
    }

  /* TODO: add test for g_unix_connection_send_credentials() and
   * g_unix_connection_receive_credentials().
   */
}
#endif

#ifdef G_OS_WIN32
static void
test_handle_not_socket (void)
{
  GError *err = NULL;
  gchar *name = NULL;
  HANDLE hReadPipe, hWritePipe, h;
  int fd;

  g_assert_true (CreatePipe (&hReadPipe, &hWritePipe, NULL, 2048));
  g_assert_false (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) (hReadPipe));
  g_assert_false (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) (hWritePipe));
  CloseHandle (hReadPipe);
  CloseHandle (hWritePipe);

  h = (HANDLE) _get_osfhandle (1);
  g_assert_false (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) (h));

  fd = g_file_open_tmp (NULL, &name, &err);
  g_assert_no_error (err);
  h = (HANDLE) _get_osfhandle (fd);
  g_assert_false (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) (h));
  g_close (fd, &err);
  g_assert_no_error (err);
  g_unlink (name);
  g_free (name);
}
#endif

static gboolean
postmortem_source_cb (GSocket      *socket,
                      GIOCondition  condition,
                      gpointer      user_data)
{
  gboolean *been_here = user_data;

  g_assert_cmpint (condition, ==, G_IO_NVAL);

  *been_here = TRUE;
  return FALSE;
}

static void
test_source_postmortem (void)
{
  GMainContext *context;
  GSocket *socket;
  GSource *source;
  GError *error = NULL;
  gboolean callback_visited = FALSE;

  socket = g_socket_new (G_SOCKET_FAMILY_UNIX, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_DEFAULT, &error);
#ifdef G_OS_WIN32
  if (error)
    {
      g_test_skip_printf ("AF_UNIX not supported on this Windows system: %s", error->message);
      g_clear_error (&error);
      return;
    }
#endif
  g_assert_no_error (error);

  context = g_main_context_new ();

  source = g_socket_create_source (socket, G_IO_IN, NULL);
  g_source_set_callback (source, (GSourceFunc) postmortem_source_cb,
                         &callback_visited, NULL);
  g_source_attach (source, context);
  g_source_unref (source);

  g_socket_close (socket, &error);
  g_assert_no_error (error);
  g_object_unref (socket);

  /* Test that, after a socket is closed, its source callback should be called
   * exactly once. */
  g_main_context_iteration (context, FALSE);
  g_assert_true (callback_visited);
  g_assert_false (g_main_context_pending (context));

  g_main_context_unref (context);
}

static void
test_reuse_tcp (void)
{
  GSocket *sock1, *sock2;
  GError *error = NULL;
  GInetAddress *iaddr;
  GSocketAddress *addr;

  sock1 = g_socket_new (G_SOCKET_FAMILY_IPV4,
                        G_SOCKET_TYPE_STREAM,
                        G_SOCKET_PROTOCOL_DEFAULT,
                        &error);
  g_assert_no_error (error);

  iaddr = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  addr = g_inet_socket_address_new (iaddr, 0);
  g_object_unref (iaddr);
  g_socket_bind (sock1, addr, TRUE, &error);
  g_object_unref (addr);
  g_assert_no_error (error);

  g_socket_listen (sock1, &error);
  g_assert_no_error (error);

  sock2 = g_socket_new (G_SOCKET_FAMILY_IPV4,
                        G_SOCKET_TYPE_STREAM,
                        G_SOCKET_PROTOCOL_DEFAULT,
                        &error);
  g_assert_no_error (error);

  addr = g_socket_get_local_address (sock1, &error);
  g_assert_no_error (error);
  g_socket_bind (sock2, addr, TRUE, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_ADDRESS_IN_USE);
  g_clear_error (&error);
  g_object_unref (addr);

  g_object_unref (sock1);
  g_object_unref (sock2);
}

static void
test_reuse_udp (void)
{
  GSocket *sock1, *sock2;
  GError *error = NULL;
  GInetAddress *iaddr;
  GSocketAddress *addr;

  sock1 = g_socket_new (G_SOCKET_FAMILY_IPV4,
                        G_SOCKET_TYPE_DATAGRAM,
                        G_SOCKET_PROTOCOL_DEFAULT,
                        &error);
  g_assert_no_error (error);

  iaddr = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  addr = g_inet_socket_address_new (iaddr, 0);
  g_object_unref (iaddr);
  g_socket_bind (sock1, addr, TRUE, &error);
  g_object_unref (addr);
  g_assert_no_error (error);

  sock2 = g_socket_new (G_SOCKET_FAMILY_IPV4,
                        G_SOCKET_TYPE_DATAGRAM,
                        G_SOCKET_PROTOCOL_DEFAULT,
                        &error);
  g_assert_no_error (error);

  addr = g_socket_get_local_address (sock1, &error);
  g_assert_no_error (error);
  g_socket_bind (sock2, addr, TRUE, &error);
  g_object_unref (addr);
  g_assert_no_error (error);

  g_object_unref (sock1);
  g_object_unref (sock2);
}

static void
test_get_available (gconstpointer user_data)
{
  GSocketType socket_type = GPOINTER_TO_UINT (user_data);
  GError *err = NULL;
  GSocket *listener, *server, *client;
  GInetAddress *addr;
  GSocketAddress *saddr, *boundaddr;
  gchar data[] = "0123456789abcdef";
  gchar buf[34];
  gssize nread;

  listener = g_socket_new (G_SOCKET_FAMILY_IPV4,
                           socket_type,
                           G_SOCKET_PROTOCOL_DEFAULT,
                           &err);
  g_assert_no_error (err);
  g_assert_true (G_IS_SOCKET (listener));

  client = g_socket_new (G_SOCKET_FAMILY_IPV4,
                         socket_type,
                         G_SOCKET_PROTOCOL_DEFAULT,
                         &err);
  g_assert_no_error (err);
  g_assert_true (G_IS_SOCKET (client));

  if (socket_type == G_SOCKET_TYPE_STREAM)
    {
      g_socket_set_option (client, IPPROTO_TCP, TCP_NODELAY, TRUE, &err);
      g_assert_no_error (err);
    }

  addr = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (addr, 0);

  g_socket_bind (listener, saddr, TRUE, &err);
  g_assert_no_error (err);
  g_object_unref (saddr);
  g_object_unref (addr);

  boundaddr = g_socket_get_local_address (listener, &err);
  g_assert_no_error (err);

  addr = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (addr, g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (boundaddr)));
  g_object_unref (addr);
  g_object_unref (boundaddr);

  if (socket_type == G_SOCKET_TYPE_STREAM)
    {
      g_socket_listen (listener, &err);
      g_assert_no_error (err);
      g_socket_connect (client, saddr, NULL, &err);
      g_assert_no_error (err);

      server = g_socket_accept (listener, NULL, &err);
      g_assert_no_error (err);
      g_socket_set_blocking (server, FALSE);
      g_object_unref (listener);
    }
  else
    server = listener;

  g_socket_send_to (client, saddr, data, sizeof (data), NULL, &err);
  g_assert_no_error (err);

  while (!g_socket_condition_wait (server, G_IO_IN, NULL, NULL))
    ;
  g_assert_cmpint (g_socket_get_available_bytes (server), ==, sizeof (data));

  g_socket_send_to (client, saddr, data, sizeof (data), NULL, &err);
  g_assert_no_error (err);

  /* We need to wait until the data has actually been copied into the
   * server socket's buffers, but g_socket_condition_wait() won't help
   * here since the socket is definitely already readable. So there's
   * a race condition in checking its available bytes. In the TCP
   * case, we poll for a bit until the new data shows up. In the UDP
   * case, there's not much we can do, but at least the failure mode
   * is passes-when-it-shouldn't, not fails-when-it-shouldn't.
   */
  if (socket_type == G_SOCKET_TYPE_STREAM)
    {
      int tries;

      for (tries = 0; tries < 100; tries++)
        {
          gssize res = g_socket_get_available_bytes (server);
          if ((res == -1) || ((gsize) res > sizeof (data)))
            break;
          g_usleep (100000);
        }

      g_assert_cmpint (g_socket_get_available_bytes (server), ==, 2 * sizeof (data));
    }
  else
    {
      g_usleep (100000);
      g_assert_cmpint (g_socket_get_available_bytes (server), ==, sizeof (data));
    }

  g_assert_cmpint (sizeof (buf), >=, 2 * sizeof (data));
  nread = g_socket_receive (server, buf, sizeof (buf), NULL, &err);
  g_assert_no_error (err);

  if (socket_type == G_SOCKET_TYPE_STREAM)
    {
      g_assert_cmpint (nread, ==, 2 * sizeof (data));
      g_assert_cmpint (g_socket_get_available_bytes (server), ==, 0);
    }
  else
    {
      g_assert_cmpint (nread, ==, sizeof (data));
      g_assert_cmpint (g_socket_get_available_bytes (server), ==, sizeof (data));
    }

  nread = g_socket_receive (server, buf, sizeof (buf), NULL, &err);
  if (socket_type == G_SOCKET_TYPE_STREAM)
    {
      g_assert_cmpint (nread, ==, -1);
      g_assert_error (err, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
      g_clear_error (&err);
    }
  else
    {
      g_assert_cmpint (nread, ==, sizeof (data));
      g_assert_no_error (err);
    }

  g_assert_cmpint (g_socket_get_available_bytes (server), ==, 0);

  g_socket_close (server, &err);
  g_assert_no_error (err);

  g_object_unref (saddr);
  g_object_unref (server);
  g_object_unref (client);
}

typedef struct {
  GInputStream *is;
  GOutputStream *os;
  const guint8 *write_data;
  guint8 *read_data;
} TestReadWriteData;

static gpointer
test_read_write_write_thread (gpointer user_data)
{
  TestReadWriteData *data = user_data;
  gsize bytes_written;
  GError *error = NULL;
  gboolean res;

  res = g_output_stream_write_all (data->os, data->write_data, 1024, &bytes_written, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpint (bytes_written, ==, 1024);

  return NULL;
}

static gpointer
test_read_write_read_thread (gpointer user_data)
{
  TestReadWriteData *data = user_data;
  gsize bytes_read;
  GError *error = NULL;
  gboolean res;

  res = g_input_stream_read_all (data->is, data->read_data, 1024, &bytes_read, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpint (bytes_read, ==, 1024);

  return NULL;
}

static gpointer
test_read_write_writev_thread (gpointer user_data)
{
  TestReadWriteData *data = user_data;
  gsize bytes_written;
  GError *error = NULL;
  gboolean res;
  GOutputVector vectors[3];

  vectors[0].buffer = data->write_data;
  vectors[0].size = 256;
  vectors[1].buffer = data->write_data + 256;
  vectors[1].size = 256;
  vectors[2].buffer = data->write_data + 512;
  vectors[2].size = 512;

  res = g_output_stream_writev_all (data->os, vectors, G_N_ELEMENTS (vectors), &bytes_written, NULL, &error);
  g_assert_true (res);
  g_assert_no_error (error);
  g_assert_cmpint (bytes_written, ==, 1024);

  return NULL;
}

/* test if normal read/write/writev via the GSocket*Streams works on TCP sockets */
static void
test_read_write (gconstpointer user_data)
{
  gboolean writev = GPOINTER_TO_INT (user_data);
  GError *err = NULL;
  GSocket *listener, *server, *client;
  GInetAddress *addr;
  GSocketAddress *saddr, *boundaddr;
  TestReadWriteData data;
  guint8 data_write[1024], data_read[1024];
  GSocketConnection *server_stream, *client_stream;
  GThread *write_thread, *read_thread;
  guint i;

  listener = g_socket_new (G_SOCKET_FAMILY_IPV4,
                           G_SOCKET_TYPE_STREAM,
                           G_SOCKET_PROTOCOL_DEFAULT,
                           &err);
  g_assert_no_error (err);
  g_assert_true (G_IS_SOCKET (listener));

  client = g_socket_new (G_SOCKET_FAMILY_IPV4,
                         G_SOCKET_TYPE_STREAM,
                         G_SOCKET_PROTOCOL_DEFAULT,
                         &err);
  g_assert_no_error (err);
  g_assert_true (G_IS_SOCKET (client));

  addr = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (addr, 0);

  g_socket_bind (listener, saddr, TRUE, &err);
  g_assert_no_error (err);
  g_object_unref (saddr);
  g_object_unref (addr);

  boundaddr = g_socket_get_local_address (listener, &err);
  g_assert_no_error (err);

  g_socket_listen (listener, &err);
  g_assert_no_error (err);

  addr = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (addr, g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (boundaddr)));
  g_object_unref (addr);
  g_object_unref (boundaddr);

  g_socket_connect (client, saddr, NULL, &err);
  g_assert_no_error (err);

  server = g_socket_accept (listener, NULL, &err);
  g_assert_no_error (err);
  g_socket_set_blocking (server, FALSE);
  g_object_unref (listener);

  server_stream = g_socket_connection_factory_create_connection (server);
  g_assert_nonnull (server_stream);
  client_stream = g_socket_connection_factory_create_connection (client);
  g_assert_nonnull (client_stream);

  for (i = 0; i < sizeof (data_write); i++)
    data_write[i] = i;

  data.is = g_io_stream_get_input_stream (G_IO_STREAM (server_stream));
  data.os = g_io_stream_get_output_stream (G_IO_STREAM (client_stream));
  data.read_data = data_read;
  data.write_data = data_write;

  if (writev)
    write_thread = g_thread_new ("writer", test_read_write_writev_thread, &data);
  else
    write_thread = g_thread_new ("writer", test_read_write_write_thread, &data);
  read_thread = g_thread_new ("reader", test_read_write_read_thread, &data);

  g_thread_join (write_thread);
  g_thread_join (read_thread);

  g_assert_cmpmem (data_write, sizeof data_write, data_read, sizeof data_read);

  g_socket_close (server, &err);
  g_assert_no_error (err);

  g_object_unref (server_stream);
  g_object_unref (client_stream);

  g_object_unref (saddr);
  g_object_unref (server);
  g_object_unref (client);
}

#ifdef SO_NOSIGPIPE
static void
test_nosigpipe (void)
{
  GSocket *sock;
  GError *error = NULL;
  gint value;

  sock = g_socket_new (AF_INET,
                       G_SOCKET_TYPE_STREAM,
                       G_SOCKET_PROTOCOL_DEFAULT,
                       &error);
  g_assert_no_error (error);

  g_socket_get_option (sock, SOL_SOCKET, SO_NOSIGPIPE, &value, &error);
  g_assert_no_error (error);
  g_assert_true (value);

  g_object_unref (sock);
}
#endif

#if G_CREDENTIALS_SUPPORTED
static gpointer client_setup_thread (gpointer user_data);

static void
test_credentials_tcp_client (void)
{
  const GSocketFamily family = G_SOCKET_FAMILY_IPV4;
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocketAddress *addr;
  GCredentials *creds;

  data = create_server (family, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (family,
			 G_SOCKET_TYPE_STREAM,
			 G_SOCKET_PROTOCOL_DEFAULT,
			 &error);
  g_assert_no_error (error);

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 1);

  g_socket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (addr);

  creds = g_socket_get_credentials (client, &error);
  if (creds != NULL)
    {
      gchar *str = g_credentials_to_string (creds);
      g_test_message ("Supported on this OS: %s", str);
      g_free (str);
      g_clear_object (&creds);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_test_message ("Unsupported on this OS: %s", error->message);
      g_clear_error (&error);
    }

  g_socket_close (client, &error);
  g_assert_no_error (error);

  g_thread_join (data->thread);

  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  g_object_unref (client);

  ip_test_data_free (data);
}

static void
test_credentials_tcp_server (void)
{
  const GSocketFamily family = G_SOCKET_FAMILY_IPV4;
  IPTestData *data;
  GSocket *server;
  GError *error = NULL;
  GSocketAddress *addr = NULL;
  GInetAddress *iaddr = NULL;
  GSocket *sock = NULL;
  GCredentials *creds;

  data = g_slice_new0 (IPTestData);
  data->family = family;
  data->server = server = g_socket_new (family,
					G_SOCKET_TYPE_STREAM,
					G_SOCKET_PROTOCOL_DEFAULT,
					&error);
  if (error != NULL)
    goto skip;

  g_socket_set_blocking (server, TRUE);

  iaddr = g_inet_address_new_loopback (family);
  addr = g_inet_socket_address_new (iaddr, 0);

  if (!g_socket_bind (server, addr, TRUE, &error))
    goto skip;

  if (!g_socket_listen (server, &error))
    goto skip;

  data->thread = g_thread_new ("client", client_setup_thread, data);

  sock = g_socket_accept (server, NULL, &error);
  g_assert_no_error (error);

  creds = g_socket_get_credentials (sock, &error);
  if (creds != NULL)
    {
      gchar *str = g_credentials_to_string (creds);
      g_test_message ("Supported on this OS: %s", str);
      g_free (str);
      g_clear_object (&creds);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_test_message ("Unsupported on this OS: %s", error->message);
      g_clear_error (&error);
    }

  goto beach;

skip:
  g_test_skip_printf ("Failed to create server: %s", error->message);
  goto beach;

beach:
  {
    g_clear_error (&error);

    g_clear_object (&sock);
    g_clear_object (&addr);
    g_clear_object (&iaddr);

    g_clear_pointer (&data->thread, g_thread_join);

    ip_test_data_free (data);
  }
}

static gpointer
client_setup_thread (gpointer user_data)
{
  IPTestData *data = user_data;
  GSocketAddress *addr;
  GSocket *client;
  GError *error = NULL;

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  data->client = client = g_socket_new (data->family,
					G_SOCKET_TYPE_STREAM,
					G_SOCKET_PROTOCOL_DEFAULT,
					&error);
  g_assert_no_error (error);

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 1);

  g_socket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);

  g_object_unref (addr);

  return NULL;
}

#ifdef G_OS_WIN32
/*
 * _g_win32_socketpair:
 *
 * Create a pair of connected sockets, similar to POSIX/BSD socketpair().
 *
 * Windows does not (yet) provide a socketpair() function. However, since the
 * introduction of AF_UNIX sockets, it is possible to implement a fairly close
 * function.
 */
static gint
_g_win32_socketpair (gint            domain,
                     gint            type,
                     gint            protocol,
                     gint            sv[2])
{
  struct sockaddr_un addr = { 0, };
  socklen_t socklen;
  SOCKET listener = INVALID_SOCKET;
  SOCKET client = INVALID_SOCKET;
  SOCKET server = INVALID_SOCKET;
  gchar *path = NULL;
  wchar_t *path_utf16 = NULL;
  int tmpfd, rv = -1;
  u_long arg, br;

  g_return_val_if_fail (sv != NULL, -1);

  addr.sun_family = AF_UNIX;
  socklen = sizeof (addr);

  tmpfd = g_file_open_tmp (NULL, &path, NULL);
  if (tmpfd == -1)
    {
      WSASetLastError (WSAEACCES);
      goto out;
    }

  g_close (tmpfd, NULL);

  if (strlen (path) >= sizeof (addr.sun_path))
    {
      WSASetLastError (WSAEACCES);
      goto out;
    }

  strncpy (addr.sun_path, path, sizeof (addr.sun_path) - 1);

  listener = socket (domain, type, protocol);
  if (listener == INVALID_SOCKET)
    goto out;

  path_utf16 = g_utf8_to_utf16 (path, -1, NULL, NULL, NULL);
  if (!path_utf16)
    goto out;

  if (DeleteFile (path_utf16) == 0)
    {
      if (GetLastError () != ERROR_FILE_NOT_FOUND)
        goto out;
    }

  if (bind (listener, (struct sockaddr *) &addr, socklen) == SOCKET_ERROR)
    goto out;

  if (listen (listener, 1) == SOCKET_ERROR)
    goto out;

  client = socket (domain, type, protocol);
  if (client == INVALID_SOCKET)
    goto out;

  arg = 1;
  if (ioctlsocket (client, FIONBIO, &arg) == SOCKET_ERROR)
    goto out;

  if (connect (client, (struct sockaddr *) &addr, socklen) == SOCKET_ERROR &&
      WSAGetLastError () != WSAEWOULDBLOCK)
    goto out;

  server = accept (listener, NULL, NULL);
  if (server == INVALID_SOCKET)
    goto out;

  arg = 0;
  if (ioctlsocket (client, FIONBIO, &arg) == SOCKET_ERROR)
    goto out;

  if (WSAIoctl (server, SIO_AF_UNIX_GETPEERPID,
                NULL, 0U,
                &arg, sizeof (arg), &br,
                NULL, NULL) == SOCKET_ERROR || arg != GetCurrentProcessId ())
    {
      WSASetLastError (WSAEACCES);
      goto out;
    }

  sv[0] = server;
  server = INVALID_SOCKET;
  sv[1] = client;
  client = INVALID_SOCKET;
  rv = 0;

 out:
  if (listener != INVALID_SOCKET)
    closesocket (listener);
  if (client != INVALID_SOCKET)
    closesocket (client);
  if (server != INVALID_SOCKET)
    closesocket (server);

  if (path_utf16)
    DeleteFile (path_utf16);

  g_free (path_utf16);
  g_free (path);
  return rv;
}
#endif /* G_OS_WIN32 */

static void
test_credentials_unix_socketpair (void)
{
  gint fds[2];
  gint status;
  GSocket *sock[2];
  GError *error = NULL;
  GCredentials *creds;

#ifdef G_OS_WIN32
  status = _g_win32_socketpair (PF_UNIX, SOCK_STREAM, 0, fds);
  if (status != 0)
    {
      g_test_skip ("AF_UNIX not supported on this Windows system.");
      return;
    }
#else
  status = socketpair (PF_UNIX, SOCK_STREAM, 0, fds);
#endif
  g_assert_cmpint (status, ==, 0);

  sock[0] = g_socket_new_from_fd (fds[0], &error);
  g_assert_no_error (error);
  sock[1] = g_socket_new_from_fd (fds[1], &error);
  g_assert_no_error (error);

  creds = g_socket_get_credentials (sock[0], &error);
  if (creds != NULL)
    {
      gchar *str = g_credentials_to_string (creds);
      g_test_message ("Supported on this OS: %s", str);
      g_free (str);
      g_clear_object (&creds);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_test_message ("Unsupported on this OS: %s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (sock[0]);
  g_object_unref (sock[1]);
}
#endif

static void
test_receive_bytes (void)
{
  const GSocketFamily family = G_SOCKET_FAMILY_IPV4;
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocketAddress *addr;
  gssize len;
  GBytes *bytes = NULL;
  gint64 time_start;
  GCancellable *cancellable = NULL;

  g_test_summary ("Test basic functionality of g_socket_receive_bytes()");

  data = create_server (family, echo_server_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (family,
                         G_SOCKET_TYPE_STREAM,
                         G_SOCKET_PROTOCOL_DEFAULT,
                         &error);
  g_assert_no_error (error);

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 10);

  g_socket_connect (client, addr, NULL, &error);
  g_assert_no_error (error);
  g_object_unref (addr);

  /* Send something. */
  len = g_socket_send (client, "hello", strlen ("hello"), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen ("hello"));

  /* And receive it back again. */
  bytes = g_socket_receive_bytes (client, 5, -1, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (bytes);
  g_assert_cmpuint (g_bytes_get_size (bytes), ==, 5);
  g_assert_cmpmem (g_bytes_get_data (bytes, NULL), 5, "hello", 5);
  g_bytes_unref (bytes);

  /* Try again with a receive buffer which is bigger than the sent bytes, to
   * test sub-buffer handling. */
  len = g_socket_send (client, "hello", strlen ("hello"), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen ("hello"));

  /* And receive it back again. */
  bytes = g_socket_receive_bytes (client, 500, -1, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (bytes);
  g_assert_cmpuint (g_bytes_get_size (bytes), ==, 5);
  g_assert_cmpmem (g_bytes_get_data (bytes, NULL), 5, "hello", 5);
  g_bytes_unref (bytes);

  /* Try receiving when there’s nothing to receive, with a timeout. This should
   * be the per-operation timeout, not the socket’s overall timeout */
  time_start = g_get_real_time ();
  bytes = g_socket_receive_bytes (client, 500, 10, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
  g_assert_null (bytes);
  g_assert_cmpint (g_get_real_time () - time_start, <, g_socket_get_timeout (client) * G_USEC_PER_SEC);
  g_clear_error (&error);

  /* And try receiving when already cancelled. */
  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  bytes = g_socket_receive_bytes (client, 500, -1, cancellable, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_null (bytes);
  g_clear_error (&error);
  g_clear_object (&cancellable);

  /* Tidy up. */
  g_socket_close (client, &error);
  g_assert_no_error (error);

  g_cancellable_cancel (data->cancellable);
  g_thread_join (data->thread);

  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  g_object_unref (client);

  ip_test_data_free (data);
}

static void
test_receive_bytes_from (void)
{
  const GSocketFamily family = G_SOCKET_FAMILY_IPV4;
  IPTestData *data;
  GError *error = NULL;
  GSocket *client;
  GSocketAddress *dest_addr = NULL, *sender_addr = NULL;
  gssize len;
  GBytes *bytes = NULL;
  gint64 time_start;
  GCancellable *cancellable = NULL;

  g_test_summary ("Test basic functionality of g_socket_receive_bytes_from()");

  data = create_server_full (family, G_SOCKET_TYPE_DATAGRAM,
                             echo_server_dgram_thread, FALSE, &error);
  if (error != NULL)
    {
      g_test_skip_printf ("Failed to create server: %s", error->message);
      g_clear_error (&error);
      return;
    }

  dest_addr = g_socket_get_local_address (data->server, &error);
  g_assert_no_error (error);

  client = g_socket_new (family,
                         G_SOCKET_TYPE_DATAGRAM,
                         G_SOCKET_PROTOCOL_DEFAULT,
                         &error);
  g_assert_no_error (error);

  g_socket_set_blocking (client, TRUE);
  g_socket_set_timeout (client, 10);

  /* Send something. */
  len = g_socket_send_to (client, dest_addr, "hello", strlen ("hello"), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen ("hello"));

  /* And receive it back again. */
  bytes = g_socket_receive_bytes_from (client, &sender_addr, 5, -1, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (bytes);
  g_assert_cmpuint (g_bytes_get_size (bytes), ==, 5);
  g_assert_cmpmem (g_bytes_get_data (bytes, NULL), 5, "hello", 5);
  g_assert_nonnull (sender_addr);
  g_clear_object (&sender_addr);
  g_bytes_unref (bytes);

  /* Try again with a receive buffer which is bigger than the sent bytes, to
   * test sub-buffer handling. */
  len = g_socket_send_to (client, dest_addr, "hello", strlen ("hello"), NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (len, ==, strlen ("hello"));

  /* And receive it back again. */
  bytes = g_socket_receive_bytes_from (client, NULL, 500, -1, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (bytes);
  g_assert_cmpuint (g_bytes_get_size (bytes), ==, 5);
  g_assert_cmpmem (g_bytes_get_data (bytes, NULL), 5, "hello", 5);
  g_bytes_unref (bytes);

  /* Try receiving when there’s nothing to receive, with a timeout. This should
   * be the per-operation timeout, not the socket’s overall timeout */
  time_start = g_get_real_time ();
  bytes = g_socket_receive_bytes_from (client, &sender_addr, 500, 10, NULL, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_TIMED_OUT);
  g_assert_null (bytes);
  g_assert_null (sender_addr);
  g_assert_cmpint (g_get_real_time () - time_start, <, g_socket_get_timeout (client) * G_USEC_PER_SEC);
  g_clear_error (&error);

  /* And try receiving when already cancelled. */
  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  bytes = g_socket_receive_bytes_from (client, &sender_addr, 500, -1, cancellable, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_null (bytes);
  g_assert_null (sender_addr);
  g_clear_error (&error);
  g_clear_object (&cancellable);

  /* Tidy up. */
  g_socket_close (client, &error);
  g_assert_no_error (error);

  g_cancellable_cancel (data->cancellable);
  g_thread_join (data->thread);

  g_socket_close (data->server, &error);
  g_assert_no_error (error);

  g_object_unref (dest_addr);
  g_object_unref (client);

  ip_test_data_free (data);
}

static void
test_accept_cancelled (void)
{
  GSocket *socket = NULL;
  GError *local_error = NULL;
  GCancellable *cancellable = NULL;
  GSocket *socket2 = NULL;

  g_test_summary ("Calling g_socket_accept() with a cancelled cancellable "
                  "should return immediately regardless of whether the socket "
                  "is blocking");

  socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
                         G_SOCKET_TYPE_STREAM,
                         G_SOCKET_PROTOCOL_DEFAULT,
                         &local_error);
  g_assert_no_error (local_error);

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);

  for (unsigned int i = 0; i < 2; i++)
    {
      g_socket_set_blocking (socket, i);

      socket2 = g_socket_accept (socket, cancellable, &local_error);
      g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
      g_assert_null (socket2);
      g_clear_error (&local_error);
    }

  g_clear_object (&cancellable);
  g_clear_object (&socket);
}

static void
test_connect_cancelled (void)
{
  GSocket *socket = NULL;
  GError *local_error = NULL;
  GCancellable *cancellable = NULL;
  GInetAddress *inet_addr = NULL;
  GSocketAddress *addr = NULL;

  g_test_summary ("Calling g_socket_connect() with a cancelled cancellable "
                  "should return immediately regardless of whether the socket "
                  "is blocking");

  socket = g_socket_new (G_SOCKET_FAMILY_IPV4,
                         G_SOCKET_TYPE_STREAM,
                         G_SOCKET_PROTOCOL_DEFAULT,
                         &local_error);
  g_assert_no_error (local_error);

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);

  inet_addr = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  addr = g_inet_socket_address_new (inet_addr, 0);

  for (unsigned int i = 0; i < 2; i++)
    {
      gboolean retval;

      g_socket_set_blocking (socket, i);

      retval = g_socket_connect (socket, addr, cancellable, &local_error);
      g_assert_error (local_error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
      g_assert_false (retval);
      g_clear_error (&local_error);
    }

  g_clear_object (&addr);
  g_clear_object (&inet_addr);
  g_clear_object (&cancellable);
  g_clear_object (&socket);
}

#ifdef G_OS_UNIX
#define H_TYPE_APP_MESSAGE (h_app_message_get_type ())

G_DECLARE_FINAL_TYPE (HAppMessage, h_app_message, H, APP_MESSAGE, GSocketControlMessage)

struct _HAppMessage
{
  GSocketControlMessage parent_instance;

  gint fds[2];
};

G_DEFINE_TYPE (HAppMessage, h_app_message, G_TYPE_SOCKET_CONTROL_MESSAGE);

static void
h_app_message_init (HAppMessage *message)
{
}

static gsize
h_app_message_get_size (GSocketControlMessage *message)
{
  return 2 * sizeof (gint);
}

static int
h_app_message_get_level (GSocketControlMessage *message)
{
  return SOL_SOCKET;
}

static int
h_app_message_get_msg_type (GSocketControlMessage *message)
{
  return SCM_RIGHTS;
}

static void
h_app_message_serialize (GSocketControlMessage *message, gpointer data)
{
  memcpy (data, H_APP_MESSAGE (message)->fds, 2 * sizeof (gint));
}

static GSocketControlMessage *
h_app_message_deserialize (gint level, gint type, gsize size, gpointer data)
{
  HAppMessage *message;

  if (level != SOL_SOCKET || type != SCM_RIGHTS)
    return NULL;

  message = g_object_new (H_TYPE_APP_MESSAGE, NULL);
  memcpy (message->fds, data, 2 * sizeof (gint));

  return G_SOCKET_CONTROL_MESSAGE (message);
}

static void
h_app_message_class_init (HAppMessageClass *klass)
{
  GSocketControlMessageClass *scm_class = G_SOCKET_CONTROL_MESSAGE_CLASS (klass);

  scm_class->get_size = h_app_message_get_size;
  scm_class->get_level = h_app_message_get_level;
  scm_class->get_type = h_app_message_get_msg_type;
  scm_class->serialize = h_app_message_serialize;
  scm_class->deserialize = h_app_message_deserialize;
}

static void
test_socket_control_message_custom (void)
{
  gint fds[2];
  int res;
  GSocket *sockets[2];
  HAppMessage *appmsg;
  GOutputVector ov;
  GInputVector iv;
  HAppMessage **mv;
  gint num_msg;
  char buffer[1];
  gssize s;
  GError *err = NULL;

  g_test_summary ("Tests that an application can create its own subclass "
                  "of GSocketControlMessage and that the message can be sent and "
                  "received using GSocket APIs");

  res = socketpair (PF_UNIX, SOCK_STREAM, 0, fds);
  g_assert_cmpint (res, ==, 0);

  sockets[0] = g_socket_new_from_fd (fds[0], &err);
  g_assert_no_error (err);
  sockets[1] = g_socket_new_from_fd (fds[1], &err);
  g_assert_no_error (err);

  appmsg = g_object_new (H_TYPE_APP_MESSAGE, NULL);
  memcpy (appmsg->fds, fds, sizeof (fds));

  buffer[0] = 0xff;
  ov.buffer = buffer;
  ov.size = 1;

  s = g_socket_send_message (sockets[0], NULL, &ov, 1, (GSocketControlMessage **) &appmsg, 1, 0, NULL, &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 1);

  iv.buffer = buffer;
  iv.size = 1;
  s = g_socket_receive_message (sockets[1], NULL, &iv, 1, (GSocketControlMessage ***) &mv, &num_msg, NULL, NULL, &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 1);
  g_assert_cmpint (num_msg, ==, 1);
  g_assert_true (H_IS_APP_MESSAGE (mv[0]));

  g_clear_object (&mv[0]);
  g_clear_pointer (&mv, g_free);

  g_clear_object (&appmsg);
  g_clear_object (&sockets[0]);
  g_clear_object (&sockets[1]);
}
#endif

static void
test_unbound_from_fd (void)
{
  GSocket *s = NULL;
  GError *error = NULL;
  gint fd;

  g_test_summary ("g_socket_new_from_fd() should succeed and return correct data "
                  "regardless of the socket state");

  fd = socket (AF_INET, SOCK_STREAM, 0);
  g_assert_cmpint (fd, !=, -1);

  s = g_socket_new_from_fd (fd, &error);
  g_assert_no_error (error);

  g_assert_cmpint (g_socket_get_family (s), ==, G_SOCKET_FAMILY_IPV4);
  g_assert_cmpint (g_socket_get_socket_type (s), ==, G_SOCKET_TYPE_STREAM);
  g_assert_cmpint (g_socket_get_protocol (s), ==, G_SOCKET_PROTOCOL_TCP);
#ifdef G_OS_WIN32
  g_assert_true (GLIB_PRIVATE_CALL (g_win32_handle_is_socket) ((HANDLE)(gintptr) g_socket_get_fd (s)));
#endif

  g_clear_object (&s);
}

int
main (int   argc,
      char *argv[])
{
  GSocket *sock;
  GError *error = NULL;

  g_test_init (&argc, &argv, NULL);

  sock = g_socket_new (G_SOCKET_FAMILY_IPV6,
                       G_SOCKET_TYPE_STREAM,
                       G_SOCKET_PROTOCOL_DEFAULT,
                       &error);
  if (sock != NULL)
    {
      ipv6_supported = TRUE;
      g_object_unref (sock);
    }
  else
    {
      g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
      g_clear_error (&error);
    }

  g_test_add_func ("/socket/ipv4_sync", test_ipv4_sync);
  g_test_add_func ("/socket/ipv4_async", test_ipv4_async);
  g_test_add_func ("/socket/ipv6_sync", test_ipv6_sync);
  g_test_add_func ("/socket/ipv6_async", test_ipv6_async);
  g_test_add_func ("/socket/ipv4_sync/datagram", test_ipv4_sync_dgram);
  g_test_add_func ("/socket/ipv4_sync/datagram/timeouts", test_ipv4_sync_dgram_timeouts);
  g_test_add_func ("/socket/ipv6_sync/datagram", test_ipv6_sync_dgram);
  g_test_add_func ("/socket/ipv6_sync/datagram/timeouts", test_ipv6_sync_dgram_timeouts);
#if defined (IPPROTO_IPV6) && defined (IPV6_V6ONLY)
  g_test_add_func ("/socket/ipv6_v4mapped", test_ipv6_v4mapped);
#endif
  g_test_add_func ("/socket/close_graceful", test_close_graceful);
  g_test_add_func ("/socket/timed_wait", test_timed_wait);
  g_test_add_func ("/socket/fd_reuse", test_fd_reuse);
  g_test_add_func ("/socket/address", test_sockaddr);
  g_test_add_func ("/socket/unix-from-fd", test_unix_from_fd);
  g_test_add_func ("/socket/unix-connection", test_unix_connection);
#ifdef G_OS_UNIX
  g_test_add_func ("/socket/unix-connection-ancillary-data", test_unix_connection_ancillary_data);
#endif
#ifdef G_OS_WIN32
  g_test_add_func ("/socket/win32-handle-not-socket", test_handle_not_socket);
#endif
  g_test_add_func ("/socket/source-postmortem", test_source_postmortem);
  g_test_add_func ("/socket/reuse/tcp", test_reuse_tcp);
  g_test_add_func ("/socket/reuse/udp", test_reuse_udp);
  g_test_add_data_func ("/socket/get_available/datagram", GUINT_TO_POINTER (G_SOCKET_TYPE_DATAGRAM),
                        test_get_available);
  g_test_add_data_func ("/socket/get_available/stream", GUINT_TO_POINTER (G_SOCKET_TYPE_STREAM),
                        test_get_available);
  g_test_add_data_func ("/socket/read_write", GUINT_TO_POINTER (FALSE),
                        test_read_write);
  g_test_add_data_func ("/socket/read_writev", GUINT_TO_POINTER (TRUE),
                        test_read_write);
#ifdef SO_NOSIGPIPE
  g_test_add_func ("/socket/nosigpipe", test_nosigpipe);
#endif
#if G_CREDENTIALS_SUPPORTED
  g_test_add_func ("/socket/credentials/tcp_client", test_credentials_tcp_client);
  g_test_add_func ("/socket/credentials/tcp_server", test_credentials_tcp_server);
  g_test_add_func ("/socket/credentials/unix_socketpair", test_credentials_unix_socketpair);
#endif
  g_test_add_func ("/socket/receive_bytes", test_receive_bytes);
  g_test_add_func ("/socket/receive_bytes_from", test_receive_bytes_from);

  g_test_add_func ("/socket/accept/cancelled", test_accept_cancelled);
  g_test_add_func ("/socket/connect/cancelled", test_connect_cancelled);
#ifdef G_OS_UNIX
  g_test_add_func ("/socket/control-message/custom", test_socket_control_message_custom);
#endif
  g_test_add_func ("/socket/new_from_fd/unbound", test_unbound_from_fd);

  return g_test_run();
}
