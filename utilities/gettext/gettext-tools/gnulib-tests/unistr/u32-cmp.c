/* Compare pieces of UTF-32 strings.
   Copyright (C) 1999, 2002, 2006, 2009-2026 Free Software Foundation, Inc.
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

#include <config.h>

/* Specification.  */
#include "unistr.h"

int
u32_cmp (const uint32_t *s1, const uint32_t *s2, size_t n)
{
  for (; n > 0; n--)
    {
      uint32_t uc1 = *s1++;
      uint32_t uc2 = *s2++;
      if (uc1 != uc2)
        {
          /* Note that uc1 and uc2 each have at most 31 bits. */
          return (int)uc1 - (int)uc2;
          /* > 0 if uc1 > uc2, < 0 if uc1 < uc2. */
        }
    }
  return 0;
}
