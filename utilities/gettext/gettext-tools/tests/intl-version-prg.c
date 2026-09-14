/* Test of public API of <libintl.h>.
   Copyright (C) 2019-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */

#include <config.h>

/* Make sure we use the included libintl, not the system's one. */
#undef _LIBINTL_H
#include "libgnuintl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Use the system functions, not the gnulib overrides in this file.  */
#undef fflush
#undef fprintf

#define ASSERT(expr) \
  do                                                                         \
    {                                                                        \
      if (!(expr))                                                           \
        {                                                                    \
          fprintf (stderr, "%s:%d: assertion failed\n",                      \
                   __FILE__, __LINE__);                                      \
          fflush (stderr);                                                   \
          abort ();                                                          \
        }                                                                    \
    }                                                                        \
  while (0)

int
main (int argc, char *argv[])
{
  /* Test LIBINTL_VERSION.  */
  {
    enum { version = LIBINTL_VERSION };
  }

  /* Test libintl_version.  */
  ASSERT (libintl_version == LIBINTL_VERSION);

  return 0;
}
