/* Test posix_spawn_file_actions_addclose() function.
   Copyright (C) 2011-2016 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include <spawn.h>

#include "signature.h"
SIGNATURE_CHECK (posix_spawn_file_actions_addclose, int,
                 (posix_spawn_file_actions_t *, int));

#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "macros.h"

/* Return a file descriptor that is too big to use.
   Prefer the smallest such fd, except use OPEN_MAX if it is defined
   and is greater than getdtablesize (), as that's how OS X works.  */
static int
big_fd (void)
{
  int fd = getdtablesize ();
#ifdef OPEN_MAX
  if (fd < OPEN_MAX)
    fd = OPEN_MAX;
#endif
  return fd;
}

int
main (void)
{
  posix_spawn_file_actions_t actions;

  ASSERT (posix_spawn_file_actions_init (&actions) == 0);

  /* Test behaviour for invalid file descriptors.  */
  {
    errno = 0;
    ASSERT (posix_spawn_file_actions_addclose (&actions, -1) == EBADF);
  }
  {
    int bad_fd = big_fd ();
    errno = 0;
    ASSERT (posix_spawn_file_actions_addclose (&actions, bad_fd) == EBADF);
  }

  return 0;
}
