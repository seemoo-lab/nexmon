/* Test the version information.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

#include <libintl.h>

#include <stdio.h>

int
main ()
{
#if defined LIBINTL_VERSION /* GNU libintl, not GNU libc */
  if (!(libintl_version == LIBINTL_VERSION))
    {
      fprintf (stderr, "Installation problem: include file says version 0x%x, whereas library says version 0x%x\n",
               LIBINTL_VERSION, libintl_version);
      return 1;
    }
#endif

  return 0;
}
