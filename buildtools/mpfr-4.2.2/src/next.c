/* mpfr_nextabove, mpfr_nextbelow, mpfr_nexttoward -- next representable
floating-point number

Copyright 1999, 2001-2025 Free Software Foundation, Inc.
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

/* Note concerning the exceptions: In case of NaN result, the NaN flag is
 * set as usual. No underflow or overflow is generated (this contradicts
 * the obsolete IEEE 754-1985 standard for nextafter, but conforms to the
 * IEEE 754-2008/2019 standards, where the nextUp and nextDown operations
 * replaced nextafter).
 *
 * For mpfr_nexttoward where x and y are zeros of different signs, the
 * behavior is the same as the nextafter function from IEEE 754-1985
 * (x is returned), but differs from the ISO C99 nextafter and nexttoward
 * functions (where y is returned).
 *
 * In short:
 *   - mpfr_nextabove and mpfr_nextbelow are similar to nextUp and nextDown
 *     respectively (IEEE 2008+, ISO C2x), but with the usual MPFR rule for
 *     the NaN flag (because MPFR has a single kind of NaN);
 *   - mpfr_nexttoward is also similar to these functions concerning the
 *     exceptions and the sign of 0, and for the particular case x = y, it
 *     follows the old nextafter function from IEEE 754-1985.
 */

#include "mpfr-impl.h"

void
mpfr_nexttozero (mpfr_ptr x)
{
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_INF (x))
        {
          mpfr_setmax (x, __gmpfr_emax);
        }
      else
        {
          MPFR_ASSERTN (MPFR_IS_ZERO (x));
          MPFR_CHANGE_SIGN(x);
          mpfr_setmin (x, __gmpfr_emin);
        }
    }
  else
    {
      mp_size_t xn;
      int sh;
      mp_limb_t *xp;

      xn = MPFR_LIMB_SIZE (x);
      MPFR_UNSIGNED_MINUS_MODULO (sh, MPFR_PREC(x));
      xp = MPFR_MANT(x);
      mpn_sub_1 (xp, xp, xn, MPFR_LIMB_ONE << sh);
      if (MPFR_UNLIKELY (MPFR_LIMB_MSB (xp[xn-1]) == 0))
        { /* was an exact power of two: not normalized any more,
             thus do not use MPFR_GET_EXP. */
          mpfr_exp_t exp = MPFR_EXP (x);
          if (MPFR_UNLIKELY (exp == __gmpfr_emin))
            MPFR_SET_ZERO(x);
          else
            {
              MPFR_SET_EXP (x, exp - 1);
              /* The following is valid whether xn = 1 or xn > 1. */
              xp[xn-1] |= MPFR_LIMB_HIGHBIT;
            }
        }
    }
}

void
mpfr_nexttoinf (mpfr_ptr x)
{
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_ZERO (x))
        mpfr_setmin (x, __gmpfr_emin);
    }
  else
    {
      mp_size_t xn;
      int sh;
      mp_limb_t *xp;

      xn = MPFR_LIMB_SIZE (x);
      MPFR_UNSIGNED_MINUS_MODULO (sh, MPFR_PREC(x));
      xp = MPFR_MANT(x);
      if (MPFR_UNLIKELY (mpn_add_1 (xp, xp, xn, MPFR_LIMB_ONE << sh)))
        /* got 1.0000... */
        {
          mpfr_exp_t exp = MPFR_EXP (x);
          if (MPFR_UNLIKELY (exp == __gmpfr_emax))
            MPFR_SET_INF (x);
          else
            {
              MPFR_SET_EXP (x, exp + 1);
              xp[xn-1] = MPFR_LIMB_HIGHBIT;
            }
        }
    }
}

void
mpfr_nextabove (mpfr_ptr x)
{
  if (MPFR_UNLIKELY(MPFR_IS_NAN(x)))
    {
      __gmpfr_flags |= MPFR_FLAGS_NAN;
      return;
    }
  if (MPFR_IS_NEG(x))
    mpfr_nexttozero (x);
  else
    mpfr_nexttoinf (x);
}

void
mpfr_nextbelow (mpfr_ptr x)
{
  if (MPFR_UNLIKELY(MPFR_IS_NAN(x)))
    {
      __gmpfr_flags |= MPFR_FLAGS_NAN;
      return;
    }

  if (MPFR_IS_NEG(x))
    mpfr_nexttoinf (x);
  else
    mpfr_nexttozero (x);
}

void
mpfr_nexttoward (mpfr_ptr x, mpfr_srcptr y)
{
  int s;

  if (MPFR_UNLIKELY(MPFR_IS_NAN(x)))
    {
      __gmpfr_flags |= MPFR_FLAGS_NAN;
      return;
    }
  else if (MPFR_UNLIKELY(MPFR_IS_NAN(x) || MPFR_IS_NAN(y)))
    {
      MPFR_SET_NAN(x);
      __gmpfr_flags |= MPFR_FLAGS_NAN;
      return;
    }

  s = mpfr_cmp (x, y);
  if (s == 0)
    return;
  else if (s < 0)
    mpfr_nextabove (x);
  else
    mpfr_nextbelow (x);
}
