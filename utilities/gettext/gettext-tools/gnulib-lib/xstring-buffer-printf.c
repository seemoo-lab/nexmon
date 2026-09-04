/* Error-checking functions on a string buffer.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#include <config.h>

/* Specification.  */
#include "string-buffer.h"

#include <errno.h>

#include "xalloc.h"

int
sb_xappendvf (struct string_buffer *buffer,
              const char *formatstring, va_list list)
{
  if (sb_appendvf (buffer, formatstring, list) < 0)
    {
      if (errno == ENOMEM)
        xalloc_die ();
      return -1;
    }
  return 0;
}

int
sb_xappendf (struct string_buffer *buffer, const char *formatstring, ...)
{
  va_list args;
  va_start (args, formatstring);
  int ret = sb_xappendvf (buffer, formatstring, args);
  va_end (args);
  return ret;
}
