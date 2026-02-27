/* Line breaking auxiliary functions.
   Copyright (C) 2001-2003, 2006-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2001.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "unilbrk/ulc-common.h"

#include "c-ctype.h"
#include "c-strcaseeq.h"

int
is_utf8_encoding (const char *encoding)
{
  if (STRCASEEQ (encoding, "UTF-8", 'U', 'T', 'F', '-', '8', 0, 0, 0, 0))
    return 1;
  return 0;
}

#if C_CTYPE_ASCII

/* Tests whether a string is entirely ASCII.  Returns 1 if yes.
   Returns 0 if the string is in an 8-bit encoding or an ISO-2022 encoding.  */
int
is_all_ascii (const char *s, size_t n)
{
  for (; n > 0; s++, n--)
    {
      unsigned char c = (unsigned char) *s;

      if (!(c_isprint (c) || c_isspace (c)))
        return 0;
    }
  return 1;
}

#endif /* C_CTYPE_ASCII */
