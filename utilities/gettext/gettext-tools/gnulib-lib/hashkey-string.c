/* Support for using a string as a hash key.
   Copyright (C) 2006-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "hashkey-string.h"

#include <limits.h>
#include <string.h>

bool
hashkey_string_equals (const void *x1, const void *x2)
{
  const char *s1 = (const char *) x1;
  const char *s2 = (const char *) x2;
  return streq (s1, s2);
}

#define SIZE_BITS (sizeof (size_t) * CHAR_BIT)

/* A hash function for NUL-terminated 'const char *' strings using
   the method described by Bruno Haible.
   See https://www.haible.de/bruno/hashfunc.html.  */
size_t
hashkey_string_hash (const void *x)
{
  const char *s = (const char *) x;
  size_t h = 0;

  for (; *s; s++)
    h = *s + ((h << 9) | (h >> (SIZE_BITS - 9)));

  return h;
}
