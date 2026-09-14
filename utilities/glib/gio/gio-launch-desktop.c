/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright 2004 Ximian Inc.
 * Copyright 2011-2022 systemd contributors
 * Copyright (C) 2018 Endless Mobile, Inc.
 * Copyright 2022 Collabora Ltd.
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
 * Author: Daniel Drake <drake@endlessm.com>
 */

/*
 * gio-launch-desktop: GDesktopAppInfo helper
 * Executable wrapper to set GIO_LAUNCHED_DESKTOP_FILE_PID
 * There are complications when doing this in a fork()/exec() codepath,
 * and it cannot otherwise be done with posix_spawn().
 * This wrapper is designed to be minimal and lightweight.
 * It does not even link against glib.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__linux__) && !defined(__ANDROID__)
#include <alloca.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gjournal-private.h"
#define GLIB_COMPILATION
#include "gmacros.h" /* For G_STATIC_ASSERT define */
#undef GLIB_COMPILATION

/*
 * write_all:
 * @fd: a file descriptor
 * @vbuf: a buffer
 * @to_write: length of @vbuf
 *
 * Write all bytes from @vbuf to @fd, blocking if necessary.
 *
 * Returns: 0 on success, -1 with errno set on failure
 */
static int
write_all (int fd, const void *vbuf, size_t to_write)
{
  const char *buf = vbuf;

  while (to_write > 0)
    {
      ssize_t count = write (fd, buf, to_write);
      if (count < 0)
        {
          if (errno != EINTR)
            return -1;
        }
      else
        {
          to_write -= count;
          buf += count;
        }
    }

  return 0;
}

/*
 * journal_stream_fd:
 * @identifier: identifier (syslog tag) for logged messages
 * @priority: a priority between `LOG_EMERG` and `LOG_DEBUG` inclusive
 * @level_prefix: if nonzero, journald will interpret prefixes like <0>
 *  as specifying the priority for a line
 *
 * Reimplementation of sd_journal_stream_fd(), to avoid having to link
 * gio-launch-desktop to libsystemd.
 *
 * Note that unlike the libsystemd version, this reports errors by returning
 * -1 with errno set.
 *
 * Returns: a non-negative fd number, or -1 with errno set on error
 */
static int
journal_stream_fd (const char *identifier,
                   int priority,
                   int level_prefix)
{
  union
  {
    struct sockaddr sa;
    struct sockaddr_un un;
  } sa =
  {
    .un.sun_family = AF_UNIX,
    .un.sun_path = "/run/systemd/journal/stdout",
  };
  socklen_t salen;
  char *header;
  int fd;
  size_t l;
  int saved_errno;
  /* Arbitrary large size for the sending buffer, from systemd */
  int large_buffer_size = 8 * 1024 * 1024;

  G_STATIC_ASSERT (LOG_EMERG == 0 && sizeof "Linux ABI defines LOG_EMERG");
  G_STATIC_ASSERT (LOG_DEBUG == 7 && sizeof "Linux ABI defines LOG_DEBUG");

  fd = socket (AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

  if (fd < 0)
    goto fail;

  salen = offsetof (struct sockaddr_un, sun_path) + (socklen_t) strlen (sa.un.sun_path) + 1;

  if (connect (fd, &sa.sa, salen) < 0)
    goto fail;

  if (shutdown (fd, SHUT_RD) < 0)
    goto fail;

  (void) setsockopt (fd, SOL_SOCKET, SO_SNDBUF, &large_buffer_size,
                     (socklen_t) sizeof (large_buffer_size));

  if (identifier == NULL)
    identifier = "";

  if (priority < 0)
    priority = 0;

  if (priority > 7)
    priority = 7;

  l = strlen (identifier);
  if (l > PATH_MAX)
    {
      errno = EINVAL;
      goto fail;
    }

  header = alloca (l + 1  /* identifier, newline */
                   + 1    /* empty unit ID, newline */
                   + 2    /* priority, newline */
                   + 2    /* level prefix, newline */
                   + 2    /* don't forward to syslog */
                   + 2    /* don't forward to kmsg */
                   + 2    /* don't forward to console */);
  memcpy (header, identifier, l);
  header[l++] = '\n';
  header[l++] = '\n';   /* empty unit ID */
  header[l++] = '0' + priority;
  header[l++] = '\n';
  header[l++] = '0' + !!level_prefix;
  header[l++] = '\n';
  header[l++] = '0';    /* don't forward to syslog */
  header[l++] = '\n';
  header[l++] = '0';    /* don't forward to kmsg */
  header[l++] = '\n';
  header[l++] = '0';    /* don't forward to console */
  header[l++] = '\n';

  if (write_all (fd, header, l) < 0)
    goto fail;

  return fd;

fail:
  saved_errno = errno;

  if (fd >= 0)
    close (fd);

  errno = saved_errno;
  return -1;
}

static void
set_up_journal (const char *argv1)
{
  int stdout_is_journal;
  int stderr_is_journal;
  const char *identifier;
  const char *slash;
  int fd;

  stdout_is_journal = _g_fd_is_journal (STDOUT_FILENO);
  stderr_is_journal = _g_fd_is_journal (STDERR_FILENO);

  if (!stdout_is_journal && !stderr_is_journal)
    return;

  identifier = getenv ("GIO_LAUNCHED_DESKTOP_FILE");

  if (identifier == NULL)
    identifier = argv1;

  slash = strrchr (identifier, '/');

  if (slash != NULL && slash[1] != '\0')
    identifier = slash + 1;

  fd = journal_stream_fd (identifier, LOG_INFO, 0);

  /* Silently ignore failure to open the Journal */
  if (fd < 0)
    return;

  if (stdout_is_journal && dup2 (fd, STDOUT_FILENO) != STDOUT_FILENO)
    fprintf (stderr,
             "gio-launch-desktop[%d]: Unable to redirect \"%s\" to Journal: %s",
             getpid (),
             identifier,
             strerror (errno));

  if (stderr_is_journal && dup2 (fd, STDERR_FILENO) != STDERR_FILENO)
    fprintf (stderr,
             "gio-launch-desktop[%d]: Unable to redirect \"%s\" to Journal: %s",
             getpid (),
             identifier,
             strerror (errno));

  close (fd);
}

#endif

int
main (int argc, char *argv[])
{
  pid_t pid = getpid ();
  char buf[50];
  int r;

  if (argc < 2)
    return -1;

  r = snprintf (buf, sizeof (buf), "GIO_LAUNCHED_DESKTOP_FILE_PID=%ld", (long) pid);
  if (r < 0 || (size_t) r >= sizeof (buf))
    return -1;

  putenv (buf);

#if defined(__linux__) && !defined(__ANDROID__)
  set_up_journal (argv[1]);
#endif

  return execvp (argv[1], argv + 1);
}
