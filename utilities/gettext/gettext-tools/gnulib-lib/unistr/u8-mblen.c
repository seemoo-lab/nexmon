/* Look at first character in UTF-8 string.
   Copyright (C) 1999-2000, 2002, 2006-2007, 2009-2016 Free Software
   Foundation, Inc.
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

int
u8_mblen (const uint8_t *s, size_t n)
{
  if (n > 0)
    {
      /* Keep in sync with unistr.h and u8-mbtouc-aux.c.  */
      uint8_t c = *s;

      if (c < 0x80)
        return (c != 0 ? 1 : 0);
      if (c >= 0xc2)
        {
          if (c < 0xe0)
            {
              if (n >= 2
#if CONFIG_UNICODE_SAFETY
                  && (s[1] ^ 0x80) < 0x40
#endif
                 )
                return 2;
            }
          else if (c < 0xf0)
            {
              if (n >= 3
#if CONFIG_UNICODE_SAFETY
                  && (s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
                  && (c >= 0xe1 || s[1] >= 0xa0)
                  && (c != 0xed || s[1] < 0xa0)
#endif
                 )
                return 3;
            }
          else if (c < 0xf8)
            {
              if (n >= 4
#if CONFIG_UNICODE_SAFETY
                  && (s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
                  && (s[3] ^ 0x80) < 0x40
                  && (c >= 0xf1 || s[1] >= 0x90)
#if 1
                  && (c < 0xf4 || (c == 0xf4 && s[1] < 0x90))
#endif
#endif
                 )
                return 4;
            }
#if 0
          else if (c < 0xfc)
            {
              if (n >= 5
#if CONFIG_UNICODE_SAFETY
                  && (s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
                  && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
                  && (c >= 0xf9 || s[1] >= 0x88)
#endif
                 )
                return 5;
            }
          else if (c < 0xfe)
            {
              if (n >= 6
#if CONFIG_UNICODE_SAFETY
                  && (s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
                  && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
                  && (s[5] ^ 0x80) < 0x40
                  && (c >= 0xfd || s[1] >= 0x84)
#endif
                 )
                return 6;
            }
#endif
        }
    }
  /* invalid or incomplete multibyte character */
  return -1;
}
