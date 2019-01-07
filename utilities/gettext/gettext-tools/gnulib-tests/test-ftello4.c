/* Test of ftello() function.
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

#include <stdio.h>

#include <errno.h>
#include <unistd.h>

#include "macros.h"

int
main (int argc, char **argv)
{
  const char *filename = argv[1];

  /* Test that ftello() sets errno if someone else closes the stream
     fd behind the back of stdio.  */
  {
    FILE *fp = fopen (filename, "r");
    ASSERT (fp != NULL);
    setvbuf (fp, NULL, _IONBF, 0);
    ASSERT (close (fileno (fp)) == 0);
    errno = 0;
    ASSERT (ftello (fp) == (off_t)-1);
    ASSERT (errno == EBADF);
    fclose (fp);
  }

  /* Test that ftello() sets errno if the stream was constructed with
     an invalid file descriptor.  */
  {
    FILE *fp = fdopen (-1, "w");
    if (fp != NULL)
      {
        errno = 0;
        ASSERT (ftello (fp) == (off_t)-1);
        ASSERT (errno == EBADF);
        fclose (fp);
      }
  }
  {
    FILE *fp;
    close (99);
    fp = fdopen (99, "w");
    if (fp != NULL)
      {
        errno = 0;
        ASSERT (ftello (fp) == (off_t)-1);
        ASSERT (errno == EBADF);
        fclose (fp);
      }
  }

  return 0;
}
