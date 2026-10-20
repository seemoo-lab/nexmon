/* Test of u32_check() function.
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

/* Written by Bruno Haible <bruno@clisp.org>, 2010.  */

#include <config.h>

#include "unistr.h"

#include "macros.h"

int
main ()
{
  /* Test empty string.  */
  {
    static const uint32_t input[] = { 0 };
    ASSERT (u32_check (input, 0) == NULL);
  }

  /* Test valid non-empty string.  */
  {
    static const uint32_t input[] = /* "Данило Шеган" */
      { 0x0414, 0x0430, 0x043D, 0x0438, 0x043B, 0x043E, 0x0020, 0x0428, 0x0435, 0x0433, 0x0430, 0x043D };
    ASSERT (u32_check (input, SIZEOF (input)) == NULL);
  }

  /* Test out-of-range character with 1 unit: U+110000.  */
  {
    static const uint32_t input[] = { 0x0414, 0x0430, 0x110000 };
    ASSERT (u32_check (input, SIZEOF (input)) == input + 2);
  }

  /* Test surrogate codepoints.  */
  {
    static const uint32_t input[] = { 0x0414, 0x0430, 0xDBFF, 0xDFFF };
    ASSERT (u32_check (input, SIZEOF (input)) == input + 2);
  }
  {
    static const uint32_t input[] = { 0x0414, 0x0430, 0xDBFF };
    ASSERT (u32_check (input, SIZEOF (input)) == input + 2);
  }
  {
    static const uint32_t input[] = { 0x0414, 0x0430, 0xDFFF };
    ASSERT (u32_check (input, SIZEOF (input)) == input + 2);
  }
  {
    static const uint32_t input[] = { 0x0414, 0x0430, 0xDFFF, 0xDBFF };
    ASSERT (u32_check (input, SIZEOF (input)) == input + 2);
  }

  return test_exit_status;
}
