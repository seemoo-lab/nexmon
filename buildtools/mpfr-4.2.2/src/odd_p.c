/* mpfr_odd_p -- check for odd integers

Copyright 2001-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

/* Return 1 if y is an odd integer, 0 otherwise.
   Assumes y is not singular. */
int
mpfr_odd_p (mpfr_srcptr y)
{
  mpfr_exp_t expo;
  mpfr_prec_t prec;
  mp_size_t yn;
  mp_limb_t *yp;

  /* NAN, INF or ZERO are not allowed */
  MPFR_ASSERTD (!MPFR_IS_SINGULAR (y));

  expo = MPFR_GET_EXP (y);
  if (expo <= 0)
    return 0;  /* |y| < 1 and not 0 */

  prec = MPFR_PREC(y);
  if ((mpfr_prec_t) expo > prec)
    return 0;  /* y is a multiple of 2^(expo-prec), thus not odd */

  /* 0 < expo <= prec:
     y = 1xxxxxxxxxt.zzzzzzzzzzzzzzzzzz[000]
          expo bits   (prec-expo) bits

     We have to check that:
     (a) the bit 't' is set
     (b) all the 'z' bits are zero
  */

  prec = MPFR_PREC2LIMBS (prec) * GMP_NUMB_BITS - expo;
  /* number of z+0 bits */

  yn = prec / GMP_NUMB_BITS;
  MPFR_ASSERTN(yn >= 0);
  /* yn is the index of limb containing the 't' bit */

  yp = MPFR_MANT(y);
  /* if expo is a multiple of GMP_NUMB_BITS, t is bit 0 */
  if (expo % GMP_NUMB_BITS == 0 ? (yp[yn] & 1) == 0
      : MPFR_LIMB_LSHIFT(yp[yn], (expo % GMP_NUMB_BITS) - 1) != MPFR_LIMB_HIGHBIT)
    return 0;
  while (--yn >= 0)
    if (yp[yn] != 0)
      return 0;
  return 1;
}

