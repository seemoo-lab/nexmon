#include <gio/gio.h>
#include <gio/gnetworking.h>
#include <gio/gunixfdmessage.h>
#include <gio/gunixsocketaddress.h>
#ifdef G_OS_UNIX
#include <glib-unix.h>
#include <unistd.h>
#endif
#include <glib/gstdio.h>
#include <fcntl.h>
#include <string.h>
#ifdef G_OS_WIN32
#include <io.h>
#endif

/* ensures that no FDs are left open at the end */
static void
check_fd_list (const gint *fd_list)
{
  gint i;

  for (i = 0; i < 40; i++)
    {
      int my_fd;

      my_fd = dup (0);
      g_assert_cmpint (fd_list[i], ==, my_fd);
    }

  for (i = 0; i < 40; i++)
    g_close (fd_list[i], NULL);
}

static void
create_fd_list (gint *fd_list)
{
  gint i;

  for (i = 0; i < 40; i++)
    {
      fd_list[i] = dup (0);
      g_assert_cmpint (fd_list[i], >, 0);
    }

  for (i = 0; i < 40; i++)
    g_close (fd_list[i], NULL);
}

static void
test_fd_list (void)
{
  GError *err = NULL;
  GUnixFDList *list;
  const gint *peek;
  gint *stolen;
  gint fd_list[40];
  gint sv[3];
  gint s;
  gint sv0, sv1;

  create_fd_list (fd_list);
  sv[2] = -1;
#ifdef G_OS_WIN32
  s = _pipe (sv, 4096, _O_NOINHERIT | _O_BINARY);
  g_assert_cmpint (s, ==, 0);
#else
  g_unix_open_pipe (sv, O_CLOEXEC, &err);
  g_assert_no_error (err);
#endif
  sv0 = sv[0];
  sv1 = sv[1];

  list = g_unix_fd_list_new_from_array (sv, -1);
  peek = g_unix_fd_list_peek_fds (list, &s);
  g_assert_cmpint (s, ==, 2);
  g_assert_cmpint (peek[0], ==, sv0);
  g_assert_cmpint (peek[1], ==, sv1);

  s = g_unix_fd_list_get (list, 0, &err);
  g_assert_no_error (err);
  g_close (s, &err);
  g_assert_no_error (err);
  s = g_unix_fd_list_get (list, 1, &err);
  g_assert_no_error (err);
  g_close (s, &err);
  g_assert_no_error (err);

  s = g_unix_fd_list_append (list, sv0, &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, >=, 0);
  stolen = g_unix_fd_list_steal_fds (list, &s);
  g_assert_cmpint (s, ==, 3);
  g_assert_cmpint (stolen[0], ==, sv0);
  g_assert_cmpint (stolen[1], ==, sv1);
  g_assert_cmpint (stolen[2], >=, 0);
  g_close (stolen[0], &err);
  g_assert_no_error (err);
  g_close (stolen[1], &err);
  g_assert_no_error (err);
  g_close (stolen[2], &err);
  g_assert_no_error (err);
  g_free (stolen);

#ifdef G_OS_WIN32
  s = _pipe (sv, 4096, _O_NOINHERIT | _O_BINARY);
  g_assert_cmpint (s, ==, 0);
#else
  g_unix_open_pipe (sv, O_CLOEXEC, &err);
  g_assert_no_error (err);
#endif
  sv0 = sv[0];
  sv1 = sv[1];

  g_assert_cmpuint (g_unix_fd_list_append_take (list, g_steal_fd (&sv[0])), ==, 0);
  g_assert_cmpuint (g_unix_fd_list_append_take (list, g_steal_fd (&sv[1])), ==, 1);
  g_assert_cmpint (g_unix_fd_list_get_length (list), ==, 2);

  g_assert_cmpint (g_unix_fd_list_peek (list, 0), ==, sv0);
  g_assert_cmpint (g_unix_fd_list_peek (list, 1), ==, sv1);

  g_assert_cmpint (g_unix_fd_list_lookup (list, 0), ==, sv0);
  g_assert_cmpint (g_unix_fd_list_lookup (list, 1), ==, sv1);
  g_assert_cmpint (g_unix_fd_list_lookup (list, 2), ==, -1);

  g_object_unref (list);
  check_fd_list (fd_list);
}

static void
test_scm (void)
{
#ifndef G_OS_WIN32
  GError *err = NULL;
  GUnixFDMessage *message;
  GUnixFDMessage **mv;
  GUnixFDList *list, *l2;
  GSocket *sockets[2];
  GSocketAddress *addr;
  const gint *peek;
  char buffer[1024];
  gint fd_list[40];
  GOutputVector ov;
  GInputVector iv;
  gint *stolen;
  gint sv[3];
  gint flags;
  gint nm;
  gint s, i;
  gchar *path;
  GByteArray *array;
  gboolean abstract;
  GUnixSocketAddressType type;

  create_fd_list (fd_list);

  s = socketpair (PF_UNIX, SOCK_STREAM, 0, sv);
  g_assert_cmpint (s, ==, 0);
  sv[2] = -1;

  list = g_unix_fd_list_new_from_array (sv, -1);
  message = G_UNIX_FD_MESSAGE (g_unix_fd_message_new_with_fd_list (list));
  g_object_unref (list);

  g_assert (g_unix_fd_message_get_fd_list (message) == list);
  g_object_get (message, "fd-list", &l2, NULL);
  g_assert (l2 == list);
  g_assert_cmpint (g_unix_fd_list_get_length (list), ==, 2);

  peek = g_unix_fd_list_peek_fds (list, &s);
  g_assert_cmpint (s, ==, 2);
  stolen = g_unix_fd_message_steal_fds (message, &s);
  g_assert_cmpint (s, ==, 2);
  g_assert (stolen == peek);

  g_assert_cmpint (stolen[0], ==, sv[0]);
  g_assert_cmpint (stolen[1], ==, sv[1]);
  g_assert_cmpint (stolen[2], ==, sv[2]);
  g_free (stolen);

  g_unix_fd_message_append_fd (message, sv[0], &err);
  g_assert_no_error (err);
  g_close (sv[0], &err);
  g_assert_no_error (err);
  g_unix_fd_message_append_fd (message, sv[1], &err);
  g_assert_no_error (err);
  g_close (sv[1], &err);
  g_assert_no_error (err);

  for (i = 0; i < 3; i++)
    {
      s = g_unix_fd_list_get (list, 0, &err);
      g_assert_no_error (err);
      g_close (s, &err);
      g_assert_no_error (err);
      s = g_unix_fd_list_get (list, 1, &err);
      g_assert_no_error (err);
      g_close (s, &err);
      g_assert_no_error (err);
    }

  g_object_unref (message);
  g_object_unref (list);

  message = G_UNIX_FD_MESSAGE (g_unix_fd_message_new ());
  list = g_unix_fd_message_get_fd_list (message);
  s = pipe (sv);
  g_assert_cmpint (s, ==, 0);

  s = g_unix_fd_list_append (list, sv[0], &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, >=, 0);
  s = g_unix_fd_list_append (list, sv[1], &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, >=, 0);

  g_close (sv[0], &err);
  g_assert_no_error (err);
  g_close (sv[1], &err);
  g_assert_no_error (err);
  s = g_unix_fd_list_get (list, 0, &err);
  g_assert_no_error (err);
  g_close (s, &err);
  g_assert_no_error (err);
  s = g_unix_fd_list_get (list, 1, &err);
  g_assert_no_error (err);
  g_close (s, &err);
  g_assert_no_error (err);

  s = socketpair (PF_UNIX, SOCK_STREAM, 0, sv);
  g_assert_cmpint (s, ==, 0);

  sockets[0] = g_socket_new_from_fd (sv[0], &err);
  g_assert_no_error (err);
  g_assert (G_IS_SOCKET (sockets[0]));
  sockets[1] = g_socket_new_from_fd (sv[1], &err);
  g_assert_no_error (err);
  g_assert (G_IS_SOCKET (sockets[1]));

  addr = g_socket_get_local_address (sockets[0], &err);
  g_assert_no_error (err);
  g_assert (G_IS_UNIX_SOCKET_ADDRESS (addr));
  g_assert_cmpint (g_unix_socket_address_get_address_type (G_UNIX_SOCKET_ADDRESS (addr)), ==, G_UNIX_SOCKET_ADDRESS_ANONYMOUS);
  g_assert_cmpint (g_unix_socket_address_get_path_len (G_UNIX_SOCKET_ADDRESS (addr)), ==, 0);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_assert (!g_unix_socket_address_get_is_abstract (G_UNIX_SOCKET_ADDRESS (addr)));
G_GNUC_END_IGNORE_DEPRECATIONS

  g_object_get (addr,
                "path", &path,
                "path-as-array", &array,
                "abstract", &abstract,
                "address-type", &type,
                NULL);
  g_assert_cmpstr (path, ==, "");
  g_assert_cmpint (array->len, ==, 0);
  g_assert (!abstract);
  g_assert (type == G_UNIX_SOCKET_ADDRESS_ANONYMOUS);
  g_free (path);
  g_byte_array_free (array, TRUE);

  g_object_unref (addr);

  buffer[0] = 0xff;
  ov.buffer = buffer;
  ov.size = 1;
  s = g_socket_send_message (sockets[0], NULL, &ov, 1,
                             (GSocketControlMessage **) &message,
                             1, 0, NULL, &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 1);
  g_object_unref (message);

  message = NULL;

  flags = 0;
  iv.buffer = buffer;
  iv.size = 1;
  s = g_socket_receive_message (sockets[1], NULL, &iv, 1,
                                (GSocketControlMessage ***) &mv,
                                &nm, &flags, NULL, &err);
  g_assert_no_error (err);
  g_assert_cmpint (s, ==, 1);
  g_object_unref (sockets[0]);
  g_object_unref (sockets[1]);

  g_assert_cmpint (nm, ==, 1);
  message = mv[0];
  g_free (mv);

  g_assert (G_IS_UNIX_FD_MESSAGE (message));
  list = g_object_ref (g_unix_fd_message_get_fd_list (message));
  g_object_unref (message);

  peek = g_unix_fd_list_peek_fds (list, &s);
  g_assert_cmpint (s, ==, 2);
  sv[0] = g_unix_fd_list_get (list, 1, &err);
  g_assert_no_error (err);

  strcpy (buffer, "failure to say failure to say 'i love gnome-panel!'.");
  s = write (sv[0], buffer, strlen (buffer) + 1);
  g_assert_cmpint (s, ==, strlen (buffer) + 1);

  g_close (sv[0], NULL);
  memset (buffer, 0xff, sizeof buffer);

  s = read (peek[0], buffer, sizeof buffer);
  g_assert_cmpint (s, ==, 53);
  g_assert_cmpstr (buffer, ==,
                   "failure to say failure to say 'i love gnome-panel!'.");

  g_object_unref (list);

  check_fd_list (fd_list);
#else
  g_test_skip ("FD SCM support doesn’t exist on Windows");
#endif
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unix-fd/fd-list", test_fd_list);
  g_test_add_func ("/unix-fd/scm", test_scm);

  return g_test_run();
}
