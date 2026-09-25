/* String descriptors.
   Copyright (C) 2023-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2023.  */

#include <config.h>

/* Specification.  */
#include "string-desc.h"

#include <string.h>


/* This function is in a separate compilation unit, because not all users
   of the 'string-desc' module need this function and it depends on 'memmem'
   which — depending on platforms — costs up to 2 KB of binary code.  */

ptrdiff_t
_sd_contains (idx_t haystack_nbytes, const char *haystack_data,
              idx_t needle_nbytes, const char *needle_data)
{
  if (needle_nbytes == 0)
    return 0;
  if (haystack_nbytes == 0)
    return -1;
  void *found =
    memmem (haystack_data, haystack_nbytes, needle_data, needle_nbytes);
  if (found != NULL)
    return (char *) found - haystack_data;
  else
    return -1;
}
