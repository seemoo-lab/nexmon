/* Conversion UTF-8 to UCS-4.
   Copyright (C) 2001-2002, 2006-2007, 2009-2016 Free Software Foundation, Inc.
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
#include "unistr.h"

#if defined IN_LIBUNISTRING || HAVE_INLINE

int
u8_mbtouc_unsafe_aux (ucs4_t *puc, const uint8_t *s, size_t n)
{
  uint8_t c = *s;

  if (c >= 0xc2)
    {
      if (c < 0xe0)
        {
          if (n >= 2)
            {
#if CONFIG_UNICODE_SAFETY
              if ((s[1] ^ 0x80) < 0x40)
#endif
                {
                  *puc = ((unsigned int) (c & 0x1f) << 6)
                         | (unsigned int) (s[1] ^ 0x80);
                  return 2;
                }
#if CONFIG_UNICODE_SAFETY
              /* invalid multibyte character */
#endif
            }
          else
            {
              /* incomplete multibyte character */
              *puc = 0xfffd;
              return 1;
            }
        }
      else if (c < 0xf0)
        {
          if (n >= 3)
            {
#if CONFIG_UNICODE_SAFETY
              if ((s[1] ^ 0x80) < 0x40)
                {
                  if ((s[2] ^ 0x80) < 0x40)
                    {
                      if ((c >= 0xe1 || s[1] >= 0xa0)
                          && (c != 0xed || s[1] < 0xa0))
#endif
                        {
                          *puc = ((unsigned int) (c & 0x0f) << 12)
                                 | ((unsigned int) (s[1] ^ 0x80) << 6)
                                 | (unsigned int) (s[2] ^ 0x80);
                          return 3;
                        }
#if CONFIG_UNICODE_SAFETY
                      /* invalid multibyte character */
                      *puc = 0xfffd;
                      return 3;
                    }
                  /* invalid multibyte character */
                  *puc = 0xfffd;
                  return 2;
                }
              /* invalid multibyte character */
#endif
            }
          else
            {
              /* incomplete multibyte character */
              *puc = 0xfffd;
              if (n == 1 || (s[1] ^ 0x80) >= 0x40)
                return 1;
              else
                return 2;
            }
        }
      else if (c < 0xf8)
        {
          if (n >= 4)
            {
#if CONFIG_UNICODE_SAFETY
              if ((s[1] ^ 0x80) < 0x40)
                {
                  if ((s[2] ^ 0x80) < 0x40)
                    {
                      if ((s[3] ^ 0x80) < 0x40)
                        {
                          if ((c >= 0xf1 || s[1] >= 0x90)
#if 1
                              && (c < 0xf4 || (c == 0xf4 && s[1] < 0x90))
#endif
                             )
#endif
                            {
                              *puc = ((unsigned int) (c & 0x07) << 18)
                                     | ((unsigned int) (s[1] ^ 0x80) << 12)
                                     | ((unsigned int) (s[2] ^ 0x80) << 6)
                                     | (unsigned int) (s[3] ^ 0x80);
                              return 4;
                            }
#if CONFIG_UNICODE_SAFETY
                          /* invalid multibyte character */
                          *puc = 0xfffd;
                          return 4;
                        }
                      /* invalid multibyte character */
                      *puc = 0xfffd;
                      return 3;
                    }
                  /* invalid multibyte character */
                  *puc = 0xfffd;
                  return 2;
                }
              /* invalid multibyte character */
#endif
            }
          else
            {
              /* incomplete multibyte character */
              *puc = 0xfffd;
              if (n == 1 || (s[1] ^ 0x80) >= 0x40)
                return 1;
              else if (n == 2 || (s[2] ^ 0x80) >= 0x40)
                return 2;
              else
                return 3;
            }
        }
#if 0
      else if (c < 0xfc)
        {
          if (n >= 5)
            {
#if CONFIG_UNICODE_SAFETY
              if ((s[1] ^ 0x80) < 0x40)
                {
                  if ((s[2] ^ 0x80) < 0x40)
                    {
                      if ((s[3] ^ 0x80) < 0x40)
                        {
                          if ((s[4] ^ 0x80) < 0x40)
                            {
                              if (c >= 0xf9 || s[1] >= 0x88)
#endif
                                {
                                  *puc = ((unsigned int) (c & 0x03) << 24)
                                         | ((unsigned int) (s[1] ^ 0x80) << 18)
                                         | ((unsigned int) (s[2] ^ 0x80) << 12)
                                         | ((unsigned int) (s[3] ^ 0x80) << 6)
                                         | (unsigned int) (s[4] ^ 0x80);
                                  return 5;
                                }
#if CONFIG_UNICODE_SAFETY
                              /* invalid multibyte character */
                              *puc = 0xfffd;
                              return 5;
                            }
                          /* invalid multibyte character */
                          *puc = 0xfffd;
                          return 4;
                        }
                      /* invalid multibyte character */
                      *puc = 0xfffd;
                      return 3;
                    }
                  /* invalid multibyte character */
                  return 2;
                }
              /* invalid multibyte character */
#endif
            }
          else
            {
              /* incomplete multibyte character */
              *puc = 0xfffd;
              return n;
            }
        }
      else if (c < 0xfe)
        {
          if (n >= 6)
            {
#if CONFIG_UNICODE_SAFETY
              if ((s[1] ^ 0x80) < 0x40)
                {
                  if ((s[2] ^ 0x80) < 0x40)
                    {
                      if ((s[3] ^ 0x80) < 0x40)
                        {
                          if ((s[4] ^ 0x80) < 0x40)
                            {
                              if ((s[5] ^ 0x80) < 0x40)
                                {
                                  if (c >= 0xfd || s[1] >= 0x84)
#endif
                                    {
                                      *puc = ((unsigned int) (c & 0x01) << 30)
                                             | ((unsigned int) (s[1] ^ 0x80) << 24)
                                             | ((unsigned int) (s[2] ^ 0x80) << 18)
                                             | ((unsigned int) (s[3] ^ 0x80) << 12)
                                             | ((unsigned int) (s[4] ^ 0x80) << 6)
                                             | (unsigned int) (s[5] ^ 0x80);
                                      return 6;
                                    }
#if CONFIG_UNICODE_SAFETY
                                  /* invalid multibyte character */
                                  *puc = 0xfffd;
                                  return 6;
                                }
                              /* invalid multibyte character */
                              *puc = 0xfffd;
                              return 5;
                            }
                          /* invalid multibyte character */
                          *puc = 0xfffd;
                          return 4;
                        }
                      /* invalid multibyte character */
                      *puc = 0xfffd;
                      return 3;
                    }
                  /* invalid multibyte character */
                  return 2;
                }
              /* invalid multibyte character */
#endif
            }
          else
            {
              /* incomplete multibyte character */
              *puc = 0xfffd;
              return n;
            }
        }
#endif
    }
  /* invalid multibyte character */
  *puc = 0xfffd;
  return 1;
}

#endif
