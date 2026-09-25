/* GLib testing framework examples and tests
 *
 * Copyright (C) 2010 Red Hat, Inc.
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

#include <gio/gio.h>
#include <glib/gstdio.h>

#ifdef G_OS_UNIX
#include <fcntl.h>
#ifdef HAVE_OPENPTY
#include <pty.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#endif

/* openpty() is non-standard and might not be available on all kernels
 * and libc implementations, but glibc on Linux definitely has it */
#if defined(__linux__) && defined(__GNUC__) && !defined(HAVE_OPENPTY)
#error Should have been able to find openpty on GNU/Linux
#endif

static gboolean
poll_source_callback (GPollableInputStream *input,
		      gpointer              user_data)
{
  GError *error = NULL;
  char buf[2];
  gssize nread;
  gboolean *success = user_data;

  g_assert_true (g_pollable_input_stream_is_readable (G_POLLABLE_INPUT_STREAM (input)));

  nread = g_pollable_input_stream_read_nonblocking (input, buf, 2, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (nread, ==, 2);
  g_assert_cmpstr (buf, ==, "x");
  g_assert_false (g_pollable_input_stream_is_readable (G_POLLABLE_INPUT_STREAM (input)));

  *success = TRUE;
  return G_SOURCE_REMOVE;
}

static gboolean
check_source_not_readable_callback (gpointer user_data)
{
  GPollableInputStream *in = G_POLLABLE_INPUT_STREAM (user_data);

  g_assert_false (g_pollable_input_stream_is_readable (in));

  return G_SOURCE_REMOVE;
}

typedef struct
{
  GPollableInputStream *in;  /* (unowned) */
  GOutputStream *out;  /* (unowned) */
} Streams;

static gboolean
write_callback (gpointer user_data)
{
  Streams *streams = user_data;
  const char *buf = "x";
  gssize nwrote;
  GError *error = NULL;

  g_assert_true (g_pollable_output_stream_is_writable (G_POLLABLE_OUTPUT_STREAM (streams->out)));

  nwrote = g_output_stream_write (streams->out, buf, 2, NULL, &error);
  g_assert_no_error (error);
  g_assert_cmpint (nwrote, ==, 2);
  g_assert_true (g_pollable_output_stream_is_writable (G_POLLABLE_OUTPUT_STREAM (streams->out)));

  /* Wait for the pipe to propagate the write for sockets. */
  while (!g_pollable_input_stream_is_readable (streams->in));
  g_assert_true (g_pollable_input_stream_is_readable (streams->in));

  return G_SOURCE_REMOVE;
}

static gboolean
quit_callback (gpointer user_data)
{
  GMainLoop *loop = user_data;

  g_main_loop_quit (loop);

  return G_SOURCE_REMOVE;
}

static void
test_streams (GPollableInputStream *in,
              GOutputStream        *out)
{
  gboolean readable;
  GError *error = NULL;
  char buf[1];
  gssize nread;
  GSource *poll_source;
  gboolean success = FALSE;
  Streams streams;
  GMainLoop *loop = NULL;

  g_assert_true (g_pollable_input_stream_can_poll (in));
  g_assert_true (g_pollable_output_stream_can_poll (G_POLLABLE_OUTPUT_STREAM (out)));

  readable = g_pollable_input_stream_is_readable (in);
  g_assert_false (readable);

  nread = g_pollable_input_stream_read_nonblocking (in, buf, 1, NULL, &error);
  g_assert_cmpint (nread, ==, -1);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK);
  g_clear_error (&error);

  /* Create 4 sources, in decreasing order of priority:
   *   1. poll source on @in
   *   2. idle source that checks if @in is readable once
   *      (it won't be) and then removes itself
   *   3. idle source that writes a byte to @out, checks that
   *      @in is now readable, and removes itself
   *   4. idle source that checks if @in is readable once
   *      (it won't be, since the poll source will fire before
   *      this one does) and then quits the loop.
   *
   * If the poll source triggers before it should, then it will get a
   * %G_IO_ERROR_WOULD_BLOCK, and if check() fails in either
   * direction, we will catch it at some point.
   */

  poll_source = g_pollable_input_stream_create_source (in, NULL);
  g_source_set_priority (poll_source, 1);
  g_source_set_callback (poll_source, (GSourceFunc) poll_source_callback, &success, NULL);
  g_source_attach (poll_source, NULL);
  g_source_unref (poll_source);

  streams.in = in;
  streams.out = out;
  loop = g_main_loop_new (NULL, FALSE);

  g_idle_add_full (2, check_source_not_readable_callback, in, NULL);
  g_idle_add_full (3, write_callback, &streams, NULL);
  g_idle_add_full (4, check_source_not_readable_callback, in, NULL);
  g_idle_add_full (5, quit_callback, loop, NULL);

  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  g_assert_cmpint (success, ==, TRUE);
}

#ifdef G_OS_UNIX

#define g_assert_not_pollable(fd) \
  G_STMT_START {                                                        \
    GPollableInputStream *in = NULL;                                    \
    GOutputStream *out = NULL;                                          \
                                                                        \
    in = G_POLLABLE_INPUT_STREAM (g_unix_input_stream_new (fd, FALSE)); \
    out = g_unix_output_stream_new (fd, FALSE);                         \
                                                                        \
    g_assert_false (g_pollable_input_stream_can_poll (in));             \
    g_assert_false (g_pollable_output_stream_can_poll (                 \
        G_POLLABLE_OUTPUT_STREAM (out)));                               \
                                                                        \
    g_clear_object (&in);                                               \
    g_clear_object (&out);                                              \
  } G_STMT_END

#define g_assert_pollable(fd) \
  G_STMT_START {                                                        \
    GPollableInputStream *in = NULL;                                    \
    GOutputStream *out = NULL;                                          \
                                                                        \
    in = G_POLLABLE_INPUT_STREAM (g_unix_input_stream_new (fd, FALSE)); \
    out = g_unix_output_stream_new (fd, FALSE);                         \
                                                                        \
    g_assert_true (g_pollable_input_stream_can_poll (in));              \
    g_assert_true (g_pollable_output_stream_can_poll (G_POLLABLE_OUTPUT_STREAM (out))); \
                                                                        \
    g_clear_object (&in);                                               \
    g_clear_object (&out);                                              \
  } G_STMT_END

static void
test_pollable_unix_pipe (void)
{
  int pipefds[2], status;
  GPollableInputStream *in = NULL;
  GOutputStream *out = NULL;

  g_test_summary ("Test that pipes are considered pollable, just like sockets");

  status = pipe (pipefds);
  g_assert_cmpint (status, ==, 0);

  in = G_POLLABLE_INPUT_STREAM (g_unix_input_stream_new (pipefds[0], TRUE));
  out = g_unix_output_stream_new (pipefds[1], TRUE);

  test_streams (in, out);

  g_object_unref (in);
  g_object_unref (out);
}

static void
test_pollable_unix_pty (void)
{
#ifdef HAVE_OPENPTY
  GPollableInputStream *in = NULL;
  GOutputStream *out = NULL;
  int a, b, status;
#endif

  g_test_summary ("Test that PTYs are considered pollable");

#ifdef HAVE_OPENPTY
  status = openpty (&a, &b, NULL, NULL, NULL);

  if (status == -1)
    {
      g_test_skip ("Unable to open PTY");
      return;
    }

  in = G_POLLABLE_INPUT_STREAM (g_unix_input_stream_new (a, TRUE));
  out = g_unix_output_stream_new (b, TRUE);

  test_streams (in, out);

  g_object_unref (in);
  g_object_unref (out);

  close (a);
  close (b);
#else
  g_test_skip ("openpty not found");
#endif
}

static void
test_pollable_unix_file (void)
{
  int fd;

  g_test_summary ("Test that regular files are not considered pollable");

  fd = g_open ("/etc/hosts", O_RDONLY, 0);
  if (fd == -1)
    {
      g_test_skip ("Unable to open /etc/hosts");
      return;
    }

  g_assert_not_pollable (fd);

  close (fd);
}

static void
test_pollable_unix_nulldev (void)
{
  g_test_summary ("Test that /dev/null is not considered pollable, but only if "
                  "on a system where we are able to tell it apart from devices "
                  "that actually implement poll");

#if defined (HAVE_EPOLL_CREATE1) || defined (HAVE_KQUEUE)
  int fd = g_open ("/dev/null", O_RDWR, 0);
  g_assert_cmpint (fd, !=, -1);

#if defined(__FreeBSD__)
  int freebsd_version = getosreldate ();
  g_assert_cmpint (freebsd_version, !=, -1);
  /* /dev/null is actually pollable on FreeBSD 14.4+ with both poll(2) and kevent(2) */
  if (freebsd_version >= 1404000)
    g_assert_pollable (fd);
  else
    g_assert_not_pollable (fd);
#else
  g_assert_not_pollable (fd);
#endif

  close (fd);
#else
  g_test_skip ("Cannot detect /dev/null as non-pollable on this system");
#endif
}

static void
test_pollable_converter (void)
{
  GConverter *converter;
  GError *error = NULL;
  GInputStream *ibase;
  int pipefds[2], status;
  GPollableInputStream *in = NULL;
  GOutputStream *out = NULL;

  status = pipe (pipefds);
  g_assert_cmpint (status, ==, 0);

  ibase = G_INPUT_STREAM (g_unix_input_stream_new (pipefds[0], TRUE));
  converter = G_CONVERTER (g_charset_converter_new ("UTF-8", "UTF-8", &error));
  g_assert_no_error (error);

  in = G_POLLABLE_INPUT_STREAM (g_converter_input_stream_new (ibase, converter));
  g_object_unref (converter);
  g_object_unref (ibase);

  out = g_unix_output_stream_new (pipefds[1], TRUE);

  test_streams (in, out);

  g_object_unref (in);
  g_object_unref (out);
}

#endif

static void
client_connected (GObject      *source,
		  GAsyncResult *result,
		  gpointer      user_data)
{
  GSocketClient *client = G_SOCKET_CLIENT (source);
  GSocketConnection **conn = user_data;
  GError *error = NULL;

  *conn = g_socket_client_connect_finish (client, result, &error);
  g_assert_no_error (error);
}

static void
server_connected (GObject      *source,
		  GAsyncResult *result,
		  gpointer      user_data)
{
  GSocketListener *listener = G_SOCKET_LISTENER (source);
  GSocketConnection **conn = user_data;
  GError *error = NULL;

  *conn = g_socket_listener_accept_finish (listener, result, NULL, &error);
  g_assert_no_error (error);
}

static void
test_pollable_socket (void)
{
  GInetAddress *iaddr;
  GSocketAddress *saddr, *effective_address;
  GSocketListener *listener;
  GSocketClient *client;
  GError *error = NULL;
  GSocketConnection *client_conn = NULL, *server_conn = NULL;
  GPollableInputStream *in = NULL;
  GOutputStream *out = NULL;

  iaddr = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  saddr = g_inet_socket_address_new (iaddr, 0);
  g_object_unref (iaddr);

  listener = g_socket_listener_new ();
  g_socket_listener_add_address (listener, saddr,
				 G_SOCKET_TYPE_STREAM,
				 G_SOCKET_PROTOCOL_TCP,
				 NULL,
				 &effective_address,
				 &error);
  g_assert_no_error (error);
  g_object_unref (saddr);

  client = g_socket_client_new ();

  g_socket_client_connect_async (client,
				 G_SOCKET_CONNECTABLE (effective_address),
				 NULL, client_connected, &client_conn);
  g_socket_listener_accept_async (listener, NULL,
				  server_connected, &server_conn);

  while (!client_conn || !server_conn)
    g_main_context_iteration (NULL, TRUE);

  in = G_POLLABLE_INPUT_STREAM (g_io_stream_get_input_stream (G_IO_STREAM (client_conn)));
  out = g_io_stream_get_output_stream (G_IO_STREAM (server_conn));

  test_streams (in, out);

  g_object_unref (client_conn);
  g_object_unref (server_conn);
  g_object_unref (client);
  g_object_unref (listener);
  g_object_unref (effective_address);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#ifdef G_OS_UNIX
  g_test_add_func ("/pollable/unix/pipe", test_pollable_unix_pipe);
  g_test_add_func ("/pollable/unix/pty", test_pollable_unix_pty);
  g_test_add_func ("/pollable/unix/file", test_pollable_unix_file);
  g_test_add_func ("/pollable/unix/nulldev", test_pollable_unix_nulldev);
  g_test_add_func ("/pollable/converter", test_pollable_converter);
#endif
  g_test_add_func ("/pollable/socket", test_pollable_socket);

  return g_test_run();
}
