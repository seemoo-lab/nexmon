/* Test of memmove() function.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <string.h>

#include <stddef.h>

#include "macros.h"

/* Test the library, not the compiler+library.  */
static void *
lib_memmove (void *s1, void const *s2, size_t n)
{
  return memmove (s1, s2, n);
}
static void *(*volatile volatile_memmove) (void *, void const *, size_t)
  = lib_memmove;
#undef memmove
#define memmove volatile_memmove

int
main (void)
{
  /* Test zero-length operations on NULL pointers, allowed by
     <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3322.pdf>.  */

  ASSERT (memmove (NULL, "x", 0) == NULL);

  {
    char y[1];
    ASSERT (memmove (y, NULL, 0) == y);
  }

  return test_exit_status;
}
