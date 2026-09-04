/* Test of searching a string for a character outside a given set of characters.
   Copyright (C) 2007-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include <string.h>

#include <locale.h>

#include "macros.h"

/* The mcel-based implementation of mbsnlen behaves differently than the
   original one.  Namely, for invalid/incomplete byte sequences:
   Where we ideally should have multi-byte-per-encoding-error (MEE) behaviour
   everywhere, mcel implements single-byte-per-encoding-error (SEE) behaviour.
   See <https://lists.gnu.org/archive/html/bug-gnulib/2023-07/msg00131.html>,
       <https://lists.gnu.org/archive/html/bug-gnulib/2023-07/msg00145.html>.
   Therefore, here we have different expected results, depending on the
   implementation.  */
#if GNULIB_MCEL_PREFER
# define OR(a,b) b
#else
# define OR(a,b) a
#endif

int
main ()
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  ASSERT (mbsnlen ("", 0) == 0);
  ASSERT (mbsnlen ("", 1) == 1);
  ASSERT (mbsnlen ("\0", 2) == 2);

  ASSERT (mbsnlen ("H", 0) == 0);
  ASSERT (mbsnlen ("H", 1) == 1);
  ASSERT (mbsnlen ("H", 2) == 2);

  ASSERT (mbsnlen ("Hello", 0) == 0);
  ASSERT (mbsnlen ("Hello", 1) == 1);
  ASSERT (mbsnlen ("Hello", 2) == 2);
  ASSERT (mbsnlen ("Hello", 5) == 5);
  ASSERT (mbsnlen ("Hello", 6) == 6);

  /* The following tests shows how mbsnlen() is different from strnlen().  */
  /* "äö" */
  ASSERT (mbsnlen ("\303\244\303\266", 0) == 0);
  ASSERT (mbsnlen ("\303\244\303\266", 1) == 1); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("\303\244\303\266", 2) == 1);
  ASSERT (mbsnlen ("\303\244\303\266", 3) == 2); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("\303\244\303\266", 4) == 2);
  ASSERT (mbsnlen ("\303\244\303\266", 5) == 3);
  /* "7€" */
  ASSERT (mbsnlen ("7\342\202\254", 0) == 0);
  ASSERT (mbsnlen ("7\342\202\254", 1) == 1);
  ASSERT (mbsnlen ("7\342\202\254", 2) == 2); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("7\342\202\254", 3) == OR(2,3)); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("7\342\202\254", 4) == 2);
  ASSERT (mbsnlen ("7\342\202\254", 5) == 3);
  /* "🐃" */
  ASSERT (mbsnlen ("\360\237\220\203", 0) == 0);
  ASSERT (mbsnlen ("\360\237\220\203", 1) == 1); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("\360\237\220\203", 2) == OR(1,2)); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("\360\237\220\203", 3) == OR(1,3)); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("\360\237\220\203", 4) == 1);
  ASSERT (mbsnlen ("\360\237\220\203", 5) == 2);

  ASSERT (mbsnlen ("\303", 1) == 1); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("\342\202", 2) == OR(1,2)); /* invalid multibyte sequence */
  ASSERT (mbsnlen ("\360\237\220", 3) == OR(1,3)); /* invalid multibyte sequence */

  return test_exit_status;
}
