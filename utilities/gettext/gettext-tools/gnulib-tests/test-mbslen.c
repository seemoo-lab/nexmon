/* Test of determining the number of multibyte characters in a string.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#include <config.h>

#include <string.h>

#include <locale.h>

#include "macros.h"

int
main ()
{
  /* configure should already have checked that the locale is supported.  */
  if (setlocale (LC_ALL, "") == NULL)
    return 1;

  ASSERT (mbslen ("") == 0);
  ASSERT (mbslen ("Hello") == 5);

  /* The following tests shows how mbslen() is different from strlen().  */
  ASSERT (mbslen ("\303\244\303\266") == 2); /* "äö" */
  ASSERT (mbslen ("7\342\202\254") == 2); /* "7€" */
  ASSERT (mbslen ("\360\237\220\203") == 1); /* "🐃" */

  ASSERT (mbslen ("\303") == 1); /* invalid multibyte sequence */
  ASSERT (mbslen ("\342\202") == 2); /* 2x invalid multibyte sequence */
  ASSERT (mbslen ("\360\237\220") == 3); /* 3x invalid multibyte sequence */

  return test_exit_status;
}
