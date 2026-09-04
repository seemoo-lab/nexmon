/* Test of next_prime() function.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible <bruno@clisp.org>, 2025.  */

#include <config.h>

/* Specification.  */
#include "next-prime.h"

#include "macros.h"

int
main ()
{
  ASSERT (next_prime (1) >= 1 && next_prime (1) <= 11);
  ASSERT (next_prime (3) >= 3 && next_prime (3) <= 11);
  ASSERT (next_prime (5) >= 5 && next_prime (5) <= 11);
  ASSERT (next_prime (7) >= 7 && next_prime (7) <= 11);
  ASSERT (next_prime (10) == 11);
  ASSERT (next_prime (11) == 11);
  ASSERT (next_prime (12) == 13);
  ASSERT (next_prime (640) == 641);

  ASSERT (next_prime (8647) == 8647);

  ASSERT (next_prime (9551) == 9551);
  ASSERT (next_prime (9552) == 9587);

  ASSERT (next_prime (43331) == 43331);
  ASSERT (next_prime (43332) == 43391);

  ASSERT (next_prime (1000000) == 1000003);

  return test_exit_status;
}
