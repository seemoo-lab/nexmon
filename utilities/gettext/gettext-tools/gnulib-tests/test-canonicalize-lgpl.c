/* Test of execution of program termination handlers.
   Copyright (C) 2007-2016 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <stdlib.h>

#include "signature.h"
SIGNATURE_CHECK (realpath, char *, (const char *, char *));
SIGNATURE_CHECK (canonicalize_file_name, char *, (const char *));

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "same-inode.h"
#include "ignore-value.h"
#include "macros.h"

#define BASE "t-can-lgpl.tmp"

static void *
null_ptr (void)
{
  return NULL;
}

int
main (void)
{
#if GNULIB_TEST_CANONICALIZE
  /* No need to test canonicalize-lgpl module if canonicalize is also
     in use.  */
  return 0;
#endif

  /* Setup some hierarchy to be used by this test.  Start by removing
     any leftovers from a previous partial run.  */
  {
    int fd;
    ignore_value (system ("rm -rf " BASE " ise"));
    ASSERT (mkdir (BASE, 0700) == 0);
    fd = creat (BASE "/tra", 0600);
    ASSERT (0 <= fd);
    ASSERT (close (fd) == 0);
  }

  /* Check for ., .., intermediate // handling, and for error cases.  */
  {
    char *result = canonicalize_file_name (BASE "//./..//" BASE "/tra");
    ASSERT (result != NULL);
    ASSERT (strstr (result, "/" BASE "/tra")
            == result + strlen (result) - strlen ("/" BASE "/tra"));
    free (result);
    errno = 0;
    result = canonicalize_file_name ("");
    ASSERT (result == NULL);
    ASSERT (errno == ENOENT);
    errno = 0;
    result = canonicalize_file_name (null_ptr ());
    ASSERT (result == NULL);
    ASSERT (errno == EINVAL);
  }

  /* Check that a non-directory with trailing slash yields NULL.  */
  {
    char *result;
    errno = 0;
    result = canonicalize_file_name (BASE "/tra/");
    ASSERT (result == NULL);
    ASSERT (errno == ENOTDIR);
  }

  /* Check that a missing directory yields NULL.  */
  {
    char *result;
    errno = 0;
    result = canonicalize_file_name (BASE "/zzz/..");
    ASSERT (result == NULL);
    ASSERT (errno == ENOENT);
  }

  /* From here on out, tests involve symlinks.  */
  if (symlink (BASE "/ket", "ise") != 0)
    {
      ASSERT (remove (BASE "/tra") == 0);
      ASSERT (rmdir (BASE) == 0);
      fputs ("skipping test: symlinks not supported on this file system\n",
             stderr);
      return 77;
    }
  ASSERT (symlink ("bef", BASE "/plo") == 0);
  ASSERT (symlink ("tra", BASE "/huk") == 0);
  ASSERT (symlink ("lum", BASE "/bef") == 0);
  ASSERT (symlink ("wum", BASE "/ouk") == 0);
  ASSERT (symlink ("../ise", BASE "/ket") == 0);
  ASSERT (mkdir (BASE "/lum", 0700) == 0);
  ASSERT (symlink ("//.//../..", BASE "/droot") == 0);

  /* Check that the symbolic link to a file can be resolved.  */
  {
    char *result1 = canonicalize_file_name (BASE "/huk");
    char *result2 = canonicalize_file_name (BASE "/tra");
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (strcmp (result1, result2) == 0);
    ASSERT (strcmp (result1 + strlen (result1) - strlen ("/" BASE "/tra"),
                    "/" BASE "/tra") == 0);
    free (result1);
    free (result2);
  }

  /* Check that the symbolic link to a directory can be resolved.  */
  {
    char *result1 = canonicalize_file_name (BASE "/plo");
    char *result2 = canonicalize_file_name (BASE "/bef");
    char *result3 = canonicalize_file_name (BASE "/lum");
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (result3 != NULL);
    ASSERT (strcmp (result1, result2) == 0);
    ASSERT (strcmp (result2, result3) == 0);
    ASSERT (strcmp (result1 + strlen (result1) - strlen ("/" BASE "/lum"),
                    "/" BASE "/lum") == 0);
    free (result1);
    free (result2);
    free (result3);
  }

  /* Check that a symbolic link to a nonexistent file yields NULL.  */
  {
    char *result;
    errno = 0;
    result = canonicalize_file_name (BASE "/ouk");
    ASSERT (result == NULL);
    ASSERT (errno == ENOENT);
  }

  /* Check that a non-directory symlink with trailing slash yields NULL.  */
  {
    char *result;
    errno = 0;
    result = canonicalize_file_name (BASE "/huk/");
    ASSERT (result == NULL);
    ASSERT (errno == ENOTDIR);
  }

  /* Check that a missing directory via symlink yields NULL.  */
  {
    char *result;
    errno = 0;
    result = canonicalize_file_name (BASE "/ouk/..");
    ASSERT (result == NULL);
    ASSERT (errno == ENOENT);
  }

  /* Check that a loop of symbolic links is detected.  */
  {
    char *result;
    errno = 0;
    result = canonicalize_file_name ("ise");
    ASSERT (result == NULL);
    ASSERT (errno == ELOOP);
  }

  /* Check that leading // is honored correctly.  */
  {
    struct stat st1;
    struct stat st2;
    char *result1 = canonicalize_file_name ("//.");
    char *result2 = canonicalize_file_name (BASE "/droot");
    ASSERT (result1);
    ASSERT (result2);
    ASSERT (stat ("/", &st1) == 0);
    ASSERT (stat ("//", &st2) == 0);
    if (SAME_INODE (st1, st2))
      {
        ASSERT (strcmp (result1, "/") == 0);
        ASSERT (strcmp (result2, "/") == 0);
      }
    else
      {
        ASSERT (strcmp (result1, "//") == 0);
        ASSERT (strcmp (result2, "//") == 0);
      }
    free (result1);
    free (result2);
  }


  /* Cleanup.  */
  ASSERT (remove (BASE "/droot") == 0);
  ASSERT (remove (BASE "/plo") == 0);
  ASSERT (remove (BASE "/huk") == 0);
  ASSERT (remove (BASE "/bef") == 0);
  ASSERT (remove (BASE "/ouk") == 0);
  ASSERT (remove (BASE "/ket") == 0);
  ASSERT (remove (BASE "/lum") == 0);
  ASSERT (remove (BASE "/tra") == 0);
  ASSERT (remove (BASE) == 0);
  ASSERT (remove ("ise") == 0);

  return 0;
}
