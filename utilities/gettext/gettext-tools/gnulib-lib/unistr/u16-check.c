/* Check UTF-16 string.
   Copyright (C) 2002, 2006-2007, 2009-2026 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2002.

   This file is free software.
   It is dual-licensed under "the GNU LGPLv3+ or the GNU GPLv2+".
   You can redistribute it and/or modify it under either
     - the terms of the GNU Lesser General Public License as published
       by the Free Software Foundation, either version 3, or (at your
       option) any later version, or
     - the terms of the GNU General Public License as published by the
       Free Software Foundation; either version 2, or (at your option)
       any later version, or
     - the same dual license "the GNU LGPLv3+ or the GNU GPLv2+".

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License and the GNU General Public License
   for more details.

   You should have received a copy of the GNU Lesser General Public
   License and of the GNU General Public License along with this
   program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Don't use the const-improved function macros in this compilation unit.  */
#define _LIBUNISTRING_NO_CONST_GENERICS

#include <config.h>

/* Specification.  */
#include "unistr.h"

const uint16_t *
u16_check (const uint16_t *s, size_t n)
{
  const uint16_t *s_end = s + n;

  while (s < s_end)
    {
      /* Keep in sync with unistr.h and u16-mbtouc-aux.c.  */
      uint16_t c = *s;

      if (c < 0xd800 || c >= 0xe000)
        {
          s++;
          continue;
        }
      if (c < 0xdc00)
        {
          if (s + 2 <= s_end
              && s[1] >= 0xdc00 && s[1] < 0xe000)
            {
              s += 2;
              continue;
            }
        }
      /* invalid or incomplete multibyte character */
      return s;
    }
  return NULL;
}
