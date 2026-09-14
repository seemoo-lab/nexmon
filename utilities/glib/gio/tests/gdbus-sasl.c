/*
 * Copyright 2019-2022 Collabora Ltd.
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
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

/* For G_CREDENTIALS_*_SUPPORTED */
#include <gio/gcredentialsprivate.h>

static const char * const explicit_external_initial_response_fail[] =
{
  "EXTERNAL with incorrect initial response",
  "C:AUTH EXTERNAL <wrong-uid>",
  "S:REJECTED.*$",
  NULL
};

static const char * const explicit_external_fail[] =
{
  "EXTERNAL without initial response, failing to authenticate",
  "C:AUTH EXTERNAL",
  "S:DATA$",
  "C:DATA <wrong-uid>",
  "S:REJECTED.*$",
  NULL
};

#if defined(G_CREDENTIALS_SOCKET_GET_CREDENTIALS_SUPPORTED) || defined(G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED)
static const char * const explicit_external_initial_response[] =
{
  "EXTERNAL with initial response",
  /* This is what most older D-Bus libraries do. */
  "C:AUTH EXTERNAL <uid>",          /* I claim to be <uid> */
  "S:OK [0-9a-f]+$",
  NULL
};

static const char * const explicit_external[] =
{
  "EXTERNAL without initial response",
  /* In theory this is equally valid, although many D-Bus libraries
   * probably don't support it correctly. */
  "C:AUTH EXTERNAL",                /* Start EXTERNAL, no initial response */
  "S:DATA$",                        /* Who are you? */
  "C:DATA <uid>",                   /* I claim to be <uid> */
  "S:OK [0-9a-f]+$",
  NULL
};

static const char * const implicit_external[] =
{
  "EXTERNAL with empty authorization identity",
  /* This is what sd-bus does. */
  "C:AUTH EXTERNAL",                /* Start EXTERNAL, no initial response */
  "S:DATA$",                        /* Who are you? */
  "C:DATA",                         /* I'm whoever the kernel says I am */
  "S:OK [0-9a-f]+$",
  NULL
};

static const char * const implicit_external_space[] =
{
  "EXTERNAL with empty authorization identity and whitespace",
  /* GDBus used to represent empty data blocks like this, although it
   * isn't interoperable to do so (in particular sd-bus would reject this). */
  "C:AUTH EXTERNAL",                /* Start EXTERNAL, no initial response */
  "S:DATA$",                        /* Who are you? */
  "C:DATA ",                        /* I'm whoever the kernel says I am */
  "S:OK [0-9a-f]+$",
  NULL
};
#endif

static const char * const * const handshakes[] =
{
  explicit_external_initial_response_fail,
  explicit_external_fail,
#if defined(G_CREDENTIALS_SOCKET_GET_CREDENTIALS_SUPPORTED) || defined(G_CREDENTIALS_UNIX_CREDENTIALS_MESSAGE_SUPPORTED)
  explicit_external_initial_response,
  explicit_external,
  implicit_external,
  implicit_external_space,
#endif
};

static void
encode_uid (guint uid,
            GString *dest)
{
  gchar *str = g_strdup_printf ("%u", uid);
  gchar *p;

  g_string_assign (dest, "");

  for (p = str; *p != '\0'; p++)
    g_string_append_printf (dest, "%02x", (unsigned char) *p);

  g_free (str);
}

typedef struct
{
  GCond cond;
  GMutex mutex;
  GDBusServerFlags server_flags;
  GMainContext *ctx;
  GMainLoop *loop;
  gchar *guid;
  gchar *listenable_address;
  gboolean ready;
} ServerInfo;

static gboolean
idle_in_server_thread_cb (gpointer user_data)
{
  ServerInfo *info = user_data;

  g_mutex_lock (&info->mutex);
  info->ready = TRUE;
  g_cond_broadcast (&info->cond);
  g_mutex_unlock (&info->mutex);
  return G_SOURCE_REMOVE;
}

static gpointer
server_thread_cb (gpointer user_data)
{
  GDBusServer *server = NULL;
  GError *error = NULL;
  GSource *source;
  ServerInfo *info = user_data;

  g_main_context_push_thread_default (info->ctx);
  server = g_dbus_server_new_sync (info->listenable_address,
                                   info->server_flags,
                                   info->guid,
                                   NULL,
                                   NULL,
                                   &error);
  g_assert_no_error (error);
  g_assert_nonnull (server);
  g_dbus_server_start (server);

  /* Tell the main thread when the server is ready to accept connections */
  source = g_idle_source_new ();
  g_source_set_callback (source, idle_in_server_thread_cb, info, NULL);
  g_source_attach (source, info->ctx);
  g_source_unref (source);

  g_main_loop_run (info->loop);

  g_main_context_pop_thread_default (info->ctx);
  g_dbus_server_stop (server);
  g_clear_object (&server);
  return NULL;
}

static void
test_sasl_server (void)
{
  GError *error = NULL;
  GSocketAddress *addr = NULL;
  GString *buf = g_string_new ("");
  GString *encoded_uid = g_string_new ("");
  GString *encoded_wrong_uid = g_string_new ("");
  GThread *server_thread = NULL;
  ServerInfo info =
  {
    .server_flags = G_DBUS_SERVER_FLAGS_RUN_IN_THREAD,
  };
  gchar *escaped = NULL;
  gchar *path = NULL;
  gchar *tmpdir = NULL;
  gsize i;

  tmpdir = g_dir_make_tmp ("gdbus-server-auth-XXXXXX", &error);
  g_assert_no_error (error);
  escaped = g_dbus_address_escape_value (tmpdir);

  path = g_build_filename (tmpdir, "socket", NULL);
  g_cond_init (&info.cond);
  g_mutex_init (&info.mutex);
  info.ctx = g_main_context_new ();
  info.guid = g_dbus_generate_guid ();
  info.listenable_address = g_strdup_printf ("unix:path=%s/socket", escaped);
  info.loop = g_main_loop_new (info.ctx, FALSE);
  info.ready = FALSE;
  server_thread = g_thread_new ("GDBusServer", server_thread_cb, &info);

  g_mutex_lock (&info.mutex);

  while (!info.ready)
    g_cond_wait (&info.cond, &info.mutex);

  g_mutex_unlock (&info.mutex);

  addr = g_unix_socket_address_new (path);

  encode_uid (geteuid (), encoded_uid);
  encode_uid (geteuid () == 0 ? 65534 : 0, encoded_wrong_uid);

  for (i = 0; i < G_N_ELEMENTS (handshakes); i++)
    {
      const char * const *handshake = handshakes[i];
      GSocketClient *client;
      GSocketConnection *conn;
      GUnixConnection *conn_unix;       /* unowned */
      GInputStream *istream;            /* unowned */
      GDataInputStream *istream_data;
      GOutputStream *ostream;           /* unowned */
      GError *error = NULL;
      gsize j;

      g_test_message ("New handshake: %s", handshake[0]);

      client = g_socket_client_new ();
      conn = g_socket_client_connect (client, G_SOCKET_CONNECTABLE (addr),
                                      NULL, &error);
      g_assert_no_error (error);

      g_assert_true (G_IS_UNIX_CONNECTION (conn));
      conn_unix = G_UNIX_CONNECTION (conn);
      istream = g_io_stream_get_input_stream (G_IO_STREAM (conn));
      ostream = g_io_stream_get_output_stream (G_IO_STREAM (conn));
      istream_data = g_data_input_stream_new (istream);
      g_data_input_stream_set_newline_type (istream_data, G_DATA_STREAM_NEWLINE_TYPE_CR_LF);

      g_unix_connection_send_credentials (conn_unix, NULL, &error);
      g_assert_no_error (error);

      for (j = 1; handshake[j] != NULL; j++)
        {
          if (j % 2 == 1)
            {
              /* client to server */
              const char *line = handshake[j];

              g_assert_cmpint (line[0], ==, 'C');
              g_assert_cmpint (line[1], ==, ':');
              g_string_assign (buf, line + 2);
              g_string_replace (buf, "<uid>", encoded_uid->str, 0);
              g_string_replace (buf, "<wrong-uid>", encoded_wrong_uid->str, 0);
              g_test_message ("C:“%s”", buf->str);
              g_string_append (buf, "\r\n");

              g_output_stream_write_all (ostream, buf->str, buf->len, NULL, NULL, &error);
              g_assert_no_error (error);
            }
          else
            {
              /* server to client */
              const char *pattern = handshake[j];
              char *line;
              gsize len;

              g_assert_cmpint (pattern[0], ==, 'S');
              g_assert_cmpint (pattern[1], ==, ':');

              g_test_message ("Expect: /^%s/", pattern + 2);
              line = g_data_input_stream_read_line (istream_data, &len, NULL, &error);
              g_assert_no_error (error);
              g_test_message ("S:“%s”", line);
              g_assert_cmpuint (len, ==, strlen (line));

              if (!g_regex_match_simple (pattern + 2, line,
                                         G_REGEX_ANCHORED,
                                         G_REGEX_MATCH_ANCHORED))
                g_error ("Expected /^%s/, got “%s”", pattern + 2, line);

              g_free (line);
            }
        }

      g_object_unref (istream_data);
      g_object_unref (conn);
      g_object_unref (client);
    }

  g_main_loop_quit (info.loop);
  g_thread_join (server_thread);

  if (tmpdir != NULL)
    g_assert_no_errno (g_rmdir (tmpdir));

  g_clear_pointer (&info.ctx, g_main_context_unref);
  g_clear_pointer (&info.loop, g_main_loop_unref);
  g_clear_object (&addr);
  g_string_free (buf, TRUE);
  g_string_free (encoded_uid, TRUE);
  g_string_free (encoded_wrong_uid, TRUE);
  g_free (escaped);
  g_free (info.guid);
  g_free (info.listenable_address);
  g_free (path);
  g_free (tmpdir);
  g_cond_clear (&info.cond);
  g_mutex_clear (&info.mutex);
}

int
main (int   argc,
      char *argv[])
{
  setlocale (LC_ALL, "");
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/gdbus/sasl/server", test_sasl_server);

  return g_test_run();
}
