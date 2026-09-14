/* Tests of fstat() function.
   Copyright (C) 2011-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include <sys/stat.h>

#include "signature.h"
SIGNATURE_CHECK (fstat, int, (int, struct stat *));

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "macros.h"

int
main ()
{
  /* Test behaviour for invalid file descriptors.  */
  {
    struct stat statbuf;

    errno = 0;
    ASSERT (fstat (-1, &statbuf) == -1);
    ASSERT (errno == EBADF);
  }
  {
    struct stat statbuf;

    close (99);
    errno = 0;
    ASSERT (fstat (99, &statbuf) == -1);
    ASSERT (errno == EBADF);
  }

  /* /dev/null is a character device.  */
  {
    int fd;
    struct stat statbuf;

    fd = open ("/dev/null", O_RDWR);
    ASSERT (fstat (fd, &statbuf) == 0);
    close (fd);
    ASSERT (!S_ISREG (statbuf.st_mode));
    ASSERT (S_ISCHR (statbuf.st_mode));
  }

  return test_exit_status;
}
