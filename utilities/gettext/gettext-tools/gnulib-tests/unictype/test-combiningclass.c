/* Test the Unicode character type functions.
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

#include <config.h>

#include "unictype.h"

#include <string.h>

#include "macros.h"

int
main ()
{
  ASSERT (uc_combining_class ('x') == UC_CCC_NR);
  ASSERT (uc_combining_class (0x0300) == UC_CCC_A);
  ASSERT (uc_combining_class (0x0319) == UC_CCC_B);
  ASSERT (uc_combining_class (0x0327) == UC_CCC_ATB);
  ASSERT (uc_combining_class (0x093C) == UC_CCC_NK);

  return test_exit_status;
}
