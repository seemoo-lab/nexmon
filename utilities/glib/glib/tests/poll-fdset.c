/* Copyright 2026 Ihar Hrachyshka
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <glib.h>

#ifdef G_OS_WIN32

static void
test_high_fd_poll (void)
{
  g_test_summary ("Validate that g_poll() handles descriptors beyond FD_SETSIZE.");
  g_test_skip ("Test exercises Unix-only APIs");
}

#else

#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

static void
test_high_fd_poll (void)
{
  GPollFD poll_fd;
  int high_fd = -1;
  int min_fd;
  int pipefd[2] = { -1, -1 };
  char byte = 'G';

  g_test_summary ("Validate that g_poll() handles descriptors beyond FD_SETSIZE.");

  g_assert_no_errno (pipe (pipefd));

  /* Request a descriptor strictly larger than the stack-backed fd_set. */
  min_fd = FD_SETSIZE + 32;
  high_fd = fcntl (pipefd[0], F_DUPFD, min_fd);

  if (high_fd == -1)
    {
      int errsv = errno;

      close (pipefd[0]);
      close (pipefd[1]);

      if (errsv == EMFILE || errsv == ENFILE || errsv == EINVAL)
        {
          g_test_skip_printf ("Unable to allocate descriptor >= %d: %s",
                              min_fd, g_strerror (errsv));
          return;
        }

      g_error ("fcntl(F_DUPFD) failed: %s", g_strerror (errsv));
    }

  close (pipefd[0]);
  pipefd[0] = -1;

  g_assert_cmpint (write (pipefd[1], &byte, 1), ==, 1);

  poll_fd.fd = high_fd;
  poll_fd.events = G_IO_IN;
  poll_fd.revents = 0;

  g_assert_cmpint (g_poll (&poll_fd, 1, 0), ==, 1);
  g_assert_true ((poll_fd.revents & G_IO_IN) != 0);

  g_assert_cmpint (read (high_fd, &byte, 1), ==, 1);

  close (high_fd);
  close (pipefd[1]);
}

#endif /* !G_OS_WIN32 */

int
main (int argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/glib/poll/high-fd", test_high_fd_poll);
  return g_test_run ();
}
