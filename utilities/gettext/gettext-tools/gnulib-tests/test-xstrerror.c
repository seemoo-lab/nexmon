/* Test of xstrerror function.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2023.  */

#include <config.h>

#include "xstrerror.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

int
main ()
{
  /* Test in the "C" locale.  */
  {
    char *s = xstrerror ("can't steal", EACCES);
    ASSERT (strcmp (s, "can't steal: Permission denied") == 0);
    free (s);
  }
  {
    char *s = xstrerror (NULL, EACCES);
    ASSERT (strcmp (s, "Permission denied") == 0);
    free (s);
  }

  return test_exit_status;
}
