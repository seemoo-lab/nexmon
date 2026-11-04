/* mpfr_extract -- bit-extraction function for the binary splitting algorithm

Copyright 2000-2002, 2004-2025 Free Software Foundation, Inc.
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

/* given 0 <= |p| < 1, this function extracts limbs of p and puts them in y.
   It is mainly designed for the "binary splitting" algorithm.

   More precisely, if B = 2^GMP_NUMB_BITS:
   - for i=0, y = floor(p * B)
   - for i>0, y = (p * B^(2^i)) mod B^(2^(i-1))
 */

void
mpfr_extract (mpz_ptr y, mpfr_srcptr p, unsigned int i)
{
  unsigned long two_i = 1UL << i;
  unsigned long two_i_2 = i ? two_i / 2 : 1;
  mp_size_t size_p = MPFR_LIMB_SIZE (p);

  MPFR_ASSERTN (two_i != 0 && two_i_2 <= INT_MAX);  /* overflow check */

  /* as 0 <= |p| < 1, we don't have to care with infinities, NaN, ... */
  MPFR_ASSERTD (!MPFR_IS_SINGULAR (p));

  mpz_realloc2 (y, (mp_bitcnt_t) two_i_2 * GMP_NUMB_BITS);
  if (size_p < two_i)
    {
      MPN_ZERO (PTR(y), two_i_2);
      if (size_p >= two_i_2)
        MPN_COPY (PTR(y) + (two_i - size_p), MPFR_MANT(p), size_p - two_i_2);
    }
  else
    MPN_COPY (PTR(y), MPFR_MANT(p) + size_p - two_i, two_i_2);

  MPN_NORMALIZE (PTR(y), two_i_2);
  SIZ(y) = MPFR_IS_NEG (p) ? - (int) two_i_2 : (int) two_i_2;
}
