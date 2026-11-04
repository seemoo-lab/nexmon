/* mpfr_print_mant_binary -- print the internal binary representation
   of a significand of floating-point number (for the tests and
   debugging purpose)

Copyright 1999-2025 Free Software Foundation, Inc.
Contributed by the Pascaline and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.
If not, see <https://www.gnu.org/licenses/>. */

#include "mpfr-impl.h"

void
mpfr_print_mant_binary (const char *str, const mp_limb_t *p, mpfr_prec_t r)
{
  int i;
  mpfr_prec_t count = 0;
  mp_size_t n = MPFR_PREC2LIMBS (r);

  printf ("%s ", str);
  for (n-- ; n >= 0 ; n--)
    {
      for (i = GMP_NUMB_BITS - 1 ; i >=0 ; i--)
        {
          putchar ((p[n] & (MPFR_LIMB_ONE << i)) ? '1' : '0');
          if (++count == r)
            putchar ('[');
        }
      putchar ('.');
    }
  if (count >= r)
    putchar (']');
  putchar ('\n');
}
