/* Test of canonical composition of Unicode characters.
   Copyright (C) 2009-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2009.  */

#include <config.h>

#include "uninorm.h"

#include "macros.h"

int
main ()
{
  /* LATIN CAPITAL LETTER A WITH DIAERESIS */
  ASSERT (uc_composition (0x0041, 0x0308) == 0x00C4);

  /* LATIN CAPITAL LETTER A WITH RING ABOVE */
  ASSERT (uc_composition (0x0041, 0x030A) == 0x00C5);

  /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
  ASSERT (uc_composition (0x00C4, 0x0304) == 0x01DE);

  /* GREEK DIALYTIKA AND PERISPOMENI */
  ASSERT (uc_composition (0x00A8, 0x0342) == 0x1FC1);

  /* CIRCLED NUMBER FIFTEEN */
  ASSERT (uc_composition (0x0031, 0x0035) == 0);

  /* TRADE MARK SIGN */
  ASSERT (uc_composition (0x0054, 0x004D) == 0);

  /* HANGUL SYLLABLE GEU */
  ASSERT (uc_composition (0x1100, 0x1173) == 0xADF8);

  /* HANGUL SYLLABLE GEUL */
  ASSERT (uc_composition (0xADF8, 0x11AF) == 0xAE00);

  return test_exit_status;
}
