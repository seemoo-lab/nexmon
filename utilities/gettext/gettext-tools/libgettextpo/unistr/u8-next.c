/* Iterate over next character in UTF-8 string.
   Copyright (C) 2002, 2006, 2009-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2002.

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
#include "unistr.h"

const uint8_t *
u8_next (ucs4_t *puc, const uint8_t *s)
{
  int count;

  count = u8_strmbtouc (puc, s);
  if (count > 0)
    return s + count;
  else
    {
      if (count < 0)
        *puc = 0xfffd;
      return NULL;
    }
}
