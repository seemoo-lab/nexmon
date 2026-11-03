/* mpfr_check -- Check if a floating-point number has not been corrupted.

Copyright 2003-2004, 2006-2025 Free Software Foundation, Inc.
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

/*
 * Check if x is a valid mpfr_t initialized by mpfr_init
 * Returns 0 if isn't valid
 *
 * Note: Due to the MPFR_GET_ALLOC_SIZE test, this function must not
 * be called on statically allocated numbers (only used inside MPFR).
 * Anyway, this test should not be useful on such numbers.
 */
int
mpfr_check (mpfr_srcptr x)
{
  mp_size_t s, i;
  mp_limb_t tmp;
  volatile mp_limb_t *xm;
  mpfr_prec_t prec;
  int rw;

  /* Check sign */
  if (MPFR_SIGN(x) != MPFR_SIGN_POS &&
      MPFR_SIGN(x) != MPFR_SIGN_NEG)
    return 0;
  /* Check precision */
  prec = MPFR_PREC(x);
  if (! MPFR_PREC_COND (prec))
    return 0;
  /* Check mantissa */
  xm = MPFR_MANT(x);
  if (xm == NULL)
    return 0;
  /* Check size of mantissa */
  s = MPFR_GET_ALLOC_SIZE(x);
  if (s <= 0 || s > MP_SIZE_T_MAX ||
      prec > (mpfr_prec_t) s * GMP_NUMB_BITS)
    return 0;
  /* Access all the mp_limb of the mantissa: may do a seg fault */
  for (i = 0 ; i < s ; i++)
    tmp = xm[i];
  /* Check singular numbers (do not use MPFR_IS_PURE_FP() in order to avoid
     any assertion checking, as this function mpfr_check() does something
     similar by returning a Boolean instead of doing an abort if the format
     is incorrect). */
  if (MPFR_IS_SINGULAR (x))
    return MPFR_IS_ZERO(x) || MPFR_IS_NAN(x) || MPFR_IS_INF(x);
  /* Check the most significant limb (its MSB must be 1) */
  if (! MPFR_IS_NORMALIZED (x))
    return 0;
  /* Check the least significant limb (the trailing bits must be 0) */
  rw = prec % GMP_NUMB_BITS;
  if (rw != 0)
    {
      tmp = MPFR_LIMB_MASK (GMP_NUMB_BITS - rw);
      if ((xm[0] & tmp) != 0)
        return 0;
    }
  /* Check exponent range */
  return MPFR_EXP_IN_RANGE (MPFR_EXP (x));
}
