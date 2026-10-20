/* Test of strncpy() function.
   Copyright (C) 2010-2026 Free Software Foundation, Inc.

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

#include "zerosize-ptr.h"
#include "macros.h"

/* Test the library, not the compiler+library.  */
static char *
lib_strncpy (char *s1, char const *s2, size_t n)
{
  return strncpy (s1, s2, n);
}
static char *(*volatile volatile_strncpy) (char *, char const *, size_t)
  = lib_strncpy;
#undef strncpy
#define strncpy volatile_strncpy

#define MAGIC (char)0xBA

static void
check_single (const char *input, size_t length, size_t n)
{
  char *dest;
  char *result;

  dest = (char *) malloc (1 + n + 1);
  ASSERT (dest != NULL);

  for (size_t i = 0; i < 1 + n + 1; i++)
    dest[i] = MAGIC;

  result = strncpy (dest + 1, input, n);
  ASSERT (result == dest + 1);

  ASSERT (dest[0] == MAGIC);
  {
    size_t i;
    for (i = 0; i < (n <= length ? n : length + 1); i++)
      ASSERT (dest[1 + i] == input[i]);
    for (; i < n; i++)
      ASSERT (dest[1 + i] == 0);
  }
  ASSERT (dest[1 + n] == MAGIC);

  free (dest);
}

static void
check (const char *input, size_t input_length)
{
  size_t length;

  ASSERT (input_length > 0);
  ASSERT (input[input_length - 1] == 0);
  length = input_length - 1; /* = strlen (input) */

  for (size_t n = 0; n <= 2 * length + 2; n++)
    check_single (input, length, n);

  /* Check that strncpy (D, S, N) does not look at more than
     MIN (strlen (S) + 1, N) units.  */
  {
    char *page_boundary = (char *) zerosize_ptr ();

    if (page_boundary != NULL)
      {
        for (size_t n = 0; n <= 2 * length + 2; n++)
          {
            size_t n_to_copy = (n <= length ? n : length + 1);
            char *copy;

            copy = (char *) page_boundary - n_to_copy;
            for (size_t i = 0; i < n_to_copy; i++)
              copy[i] = input[i];

            check_single (copy, length, n);
          }
      }
  }
}

int
main (void)
{
  /* Simple string.  */
  { /* "Grüß Gott." */
    static const char input[] = "Gr\303\274\303\237 Gott.";
    check (input, SIZEOF (input));
  }

  /* Test zero-length operations on NULL pointers, allowed by
     <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3322.pdf>.  */

  ASSERT (strncpy (NULL, "x", 0) == NULL);

  {
    char y[1];
    ASSERT (strncpy (y, NULL, 0) == y);
  }

  return test_exit_status;
}
