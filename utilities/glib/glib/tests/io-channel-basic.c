/* GLIB - Library of useful routines for C programming
 * Copyright (C) 2000  Tor Lillqvist
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/* A test program for the main loop and IO channel code.
 * Just run it. Optional parameter is number of sub-processes.
 */

/* We are using g_io_channel_read() which is deprecated */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include "config.h"

#include <glib.h>

#include <stdio.h>

#ifdef G_OS_WIN32
  #include <io.h>
  #include <fcntl.h>
  #include <process.h>
  #include <windows.h>
  #define pipe(fds) _pipe(fds, 4096, _O_BINARY)
#endif

#ifdef G_OS_UNIX
  #include <unistd.h>
#endif

static int nrunning;
static GMainLoop *main_loop;

/* Larger than the circular buffer in giowin32.c on purpose */
#define BUFSIZE 5000

static int nkiddies;
static char *exec_name;

static struct {
  int fd;
  int seq;
} *seqtab;

static GIOError
read_all (int         fd,
          GIOChannel *channel,
          char       *buffer,
          guint       nbytes,
          guint      *bytes_read)
{
  guint left = nbytes;
  gsize nb;
  GIOError error = G_IO_ERROR_NONE;
  char *bufp = buffer;

  /* g_io_channel_read() doesn't necessarily return all the
   * data we want at once.
   */
  *bytes_read = 0;
  while (left)
    {
      error = g_io_channel_read (channel, bufp, left, &nb);

      if (error != G_IO_ERROR_NONE)
        {
          g_test_message ("io-channel-basic: ...from %d: %d", fd, error);
          if (error == G_IO_ERROR_AGAIN)
            continue;
          break;
        }
      if (nb == 0)
        return error;
      left -= nb;
      bufp += nb;
      *bytes_read += nb;
    }
  return error;
}

static void
shutdown_source (gpointer data)
{
  guint *fd_ptr = data;

  if (*fd_ptr != 0)
    {
      g_source_remove (*fd_ptr);
      *fd_ptr = 0;

      nrunning--;
      if (nrunning == 0)
        g_main_loop_quit (main_loop);
    }
}

static gboolean
recv_message (GIOChannel  *channel,
              GIOCondition cond,
              gpointer    data)
{
  gint fd = g_io_channel_unix_get_fd (channel);
  gboolean retval = TRUE;

  g_debug ("io-channel-basic: ...from %d:%s%s%s%s", fd,
           (cond & G_IO_ERR) ? " ERR" : "",
           (cond & G_IO_HUP) ? " HUP" : "",
           (cond & G_IO_IN)  ? " IN"  : "",
           (cond & G_IO_PRI) ? " PRI" : "");

  if (cond & (G_IO_ERR | G_IO_HUP))
    {
      shutdown_source (data);
      retval = FALSE;
    }

  if (cond & G_IO_IN)
    {
      char buf[BUFSIZE];
      guint nbytes = 0;
      guint nb;
      guint j;
      int i, seq;
      GIOError error;

      error = read_all (fd, channel, (gchar *) &seq, sizeof (seq), &nb);
      if (error == G_IO_ERROR_NONE)
        {
          if (nb == 0)
            {
              g_debug ("io-channel-basic: ...from %d: EOF", fd);
              shutdown_source (data);
              return FALSE;
            }
          g_assert_cmpuint (nb, ==, sizeof (nbytes));

          for (i = 0; i < nkiddies; i++)
            if (seqtab[i].fd == fd)
              {
                g_assert_cmpint (seq, ==, seqtab[i].seq);
                seqtab[i].seq++;
                break;
              }

          error =
            read_all (fd, channel, (gchar *) &nbytes, sizeof (nbytes), &nb);
        }

      if (error != G_IO_ERROR_NONE)
        return FALSE;

      if (nb == 0)
        {
          g_debug ("io-channel-basic: ...from %d: EOF", fd);
          shutdown_source (data);
          return FALSE;
        }
      g_assert_cmpuint (nb, ==, sizeof (nbytes));

      g_assert_cmpuint (nbytes, <, BUFSIZE);
      g_debug ("io-channel-basic: ...from %d: %d bytes", fd, nbytes);
      if (nbytes > 0)
        {
          error = read_all (fd, channel, buf, nbytes, &nb);

          if (error != G_IO_ERROR_NONE)
            return FALSE;

          if (nb == 0)
            {
              g_debug ("io-channel-basic: ...from %d: EOF", fd);
              shutdown_source (data);
              return FALSE;
            }

          for (j = 0; j < nbytes; j++)
            g_assert_cmpint (buf[j], ==, ' ' + (char) ((nbytes + j) % 95));
          g_debug ("io-channel-basic: ...from %d: OK", fd);
        }
    }
  return retval;
}

#ifdef G_OS_WIN32
static gboolean
recv_windows_message (GIOChannel  *channel,
                      GIOCondition cond,
                      gpointer    data)
{
  GIOError error;
  MSG msg;
  gsize nb;

  while (1)
    {
      error = g_io_channel_read (channel, (gchar *) &msg, sizeof (MSG), &nb);

      if (error != G_IO_ERROR_NONE)
        {
          g_test_message ("io-channel-basic: ...reading Windows message: G_IO_ERROR_%s",
                          (error == G_IO_ERROR_AGAIN ? "AGAIN" : (error == G_IO_ERROR_INVAL ? "INVAL" : (error == G_IO_ERROR_UNKNOWN ? "UNKNOWN" : "???"))));
          if (error == G_IO_ERROR_AGAIN)
            continue;
        }
      break;
    }

  g_test_message ("io-channel-basic: ...Windows message for 0x%p: %d,%" G_GUINTPTR_FORMAT ",%" G_GINTPTR_FORMAT,
                  msg.hwnd, msg.message, msg.wParam, (gintptr) msg.lParam);

  return TRUE;
}

LRESULT CALLBACK window_procedure (HWND   hwnd,
                                   UINT   message,
                                   WPARAM wparam,
                                   LPARAM lparam);

LRESULT CALLBACK
window_procedure (HWND hwnd,
                  UINT message,
                  WPARAM wparam,
                  LPARAM lparam)
{
  g_test_message ("io-channel-basic: window_procedure for 0x%p: %d,%" G_GUINTPTR_FORMAT ",%" G_GINTPTR_FORMAT,
                  hwnd, message, wparam, (gintptr) lparam);
  return DefWindowProc (hwnd, message, wparam, lparam);
}
#endif

static void
spawn_process (int children_nb)
{
  GIOChannel *my_read_channel;
  gchar *cmdline;
  int i;

#ifdef G_OS_WIN32
  gint64 start, end;
  GPollFD pollfd;
  int pollresult;
  ATOM klass;
  static WNDCLASS wcl;
  HWND hwnd;
  GIOChannel *windows_messages_channel;

  wcl.style = 0;
  wcl.lpfnWndProc = window_procedure;
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  wcl.hInstance = GetModuleHandle (NULL);
  wcl.hIcon = NULL;
  wcl.hCursor = NULL;
  wcl.hbrBackground = NULL;
  wcl.lpszMenuName = NULL;
  wcl.lpszClassName = L"io-channel-basic";

  klass = RegisterClass (&wcl);
  g_assert_cmpint (klass, !=, 0);

  hwnd = CreateWindow (MAKEINTATOM (klass), L"io-channel-basic", 0, 0, 0, 10, 10,
                       NULL, NULL, wcl.hInstance, NULL);
  g_assert_nonnull (hwnd);

  windows_messages_channel =
    g_io_channel_win32_new_messages ((guint) (guintptr) hwnd);
  g_io_add_watch (windows_messages_channel, G_IO_IN, recv_windows_message, 0);
#endif

  nkiddies = (children_nb > 0 ? children_nb : 1);
  seqtab = g_malloc (nkiddies * 2 * sizeof (int));

  for (i = 0; i < nkiddies; i++)
    {
      guint *id;
      int pipe_to_sub[2], pipe_from_sub[2];

      if (pipe (pipe_to_sub) == -1 || pipe (pipe_from_sub) == -1)
        {
          perror ("pipe");
          exit (1);
        }

      seqtab[i].fd = pipe_from_sub[0];
      seqtab[i].seq = 0;

      my_read_channel = g_io_channel_unix_new (pipe_from_sub[0]);

      id = g_new (guint, 1);
      *id = g_io_add_watch_full (my_read_channel,
                                 G_PRIORITY_DEFAULT,
                                 G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                                 recv_message,
                                 id, g_free);
      nrunning++;

#ifdef G_OS_WIN32
      /* Spawn new Win32 process */
      cmdline =
        g_strdup_printf ("%d:%d:0x%p", pipe_to_sub[0], pipe_from_sub[1], hwnd);
      _spawnl (_P_NOWAIT, exec_name, exec_name, "--child", cmdline, NULL);
#else
      /* Spawn new Unix process */
      cmdline = g_strdup_printf ("%s --child %d:%d &",
                                 exec_name, pipe_to_sub[0], pipe_from_sub[1]);
      g_assert_no_errno (system (cmdline));
#endif
      g_free (cmdline);

      /* Closing pipes */
      close (pipe_to_sub[0]);
      close (pipe_from_sub[1]);

#ifdef G_OS_WIN32
      start = g_get_monotonic_time();
      g_io_channel_win32_make_pollfd (my_read_channel, G_IO_IN, &pollfd);
      pollresult = g_io_channel_win32_poll (&pollfd, 1, 100);
      end = g_get_monotonic_time();

      g_test_message ("io-channel-basic: had to wait %" G_GINT64_FORMAT "s, result:%d",
                      (end - start) / 1000000, pollresult);
#endif
      g_io_channel_unref (my_read_channel);
    }

  main_loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (main_loop);

  g_main_loop_unref (main_loop);
  g_free (seqtab);
}

static void
run_process (int argc, char *argv[])
{
  int readfd, writefd;
  gint64 dt;
  char buf[BUFSIZE];
  int buflen, i, j, n;
#ifdef G_OS_WIN32
  HWND hwnd;
#endif

  /* Extract parameters */
  sscanf (argv[2], "%d:%d%n", &readfd, &writefd, &n);
#ifdef G_OS_WIN32
  sscanf (argv[2] + n, ":0x%p", &hwnd);
#endif

  dt = g_get_monotonic_time();
  srand (dt ^ (dt / 1000) ^ readfd ^ (writefd << 4));

  for (i = 0; i < 20 + rand () % 10; i++)
    {
      g_usleep ((100 + rand () % 10) * 2500);
      buflen = rand () % BUFSIZE;
      for (j = 0; j < buflen; j++)
        buf[j] = ' ' + ((buflen + j) % 95);
      g_debug ("io-channel-basic: child writing %d+%d bytes to %d",
               (int) (sizeof (i) + sizeof (buflen)), buflen, writefd);
      g_assert_cmpint (write (writefd, &i, sizeof (i)), ==, sizeof (i));
      g_assert_cmpint (write (writefd, &buflen, sizeof (buflen)), ==, sizeof (buflen));
      g_assert_cmpint (write (writefd, buf, buflen), ==, buflen);

#ifdef G_OS_WIN32
      if (i % 10 == 0)
        {
          int msg = WM_USER + (rand () % 100);
          WPARAM wparam = rand ();
          LPARAM lparam = rand ();
          g_test_message ("io-channel-basic: child posting message %d,%" G_GUINTPTR_FORMAT ",%" G_GINTPTR_FORMAT " to 0x%p",
                          msg, wparam, (gintptr) lparam, hwnd);
          PostMessage (hwnd, msg, wparam, lparam);
        }
#endif
    }
  g_debug ("io-channel-basic: child exiting, closing %d", writefd);
  close (writefd);
}

static void
test_io_basics (void)
{
  spawn_process (1);
#ifndef G_OS_WIN32
  spawn_process (5);
#endif
}

int
main (int argc, char *argv[])
{
  /* Get executable name */
  exec_name = argv[0];

  /* Run the tests */
  g_test_init (&argc, &argv, NULL);

  /* Run subprocess, if it is the case */
  if (argc > 2)
    {
      run_process (argc, argv);
      return 0;
    }

  g_test_add_func ("/gio/io-basics", test_io_basics);

  return g_test_run ();
}
