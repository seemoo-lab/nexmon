/* mpfr_set_uj -- set a MPFR number from a huge machine unsigned integer

Copyright 2004-2025 Free Software Foundation, Inc.
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
#define MPFR_NEED_INTMAX_H
#include "mpfr-impl.h"

#ifdef _MPFR_H_HAVE_INTMAX_T

#define uintmaxpml (sizeof(uintmax_t) / sizeof(mp_limb_t))

int
mpfr_set_uj (mpfr_ptr x, uintmax_t j, mpfr_rnd_t rnd)
{
  return mpfr_set_uj_2exp (x, j, 0, rnd);
}

int
mpfr_set_uj_2exp (mpfr_ptr x, uintmax_t j, intmax_t e, mpfr_rnd_t rnd)
{
  int cnt, inex;
  mp_size_t i, k;
  mp_limb_t limb;
  mp_limb_t yp[uintmaxpml];
  mpfr_t y;
  unsigned long uintmax_bit_size = sizeof(uintmax_t) * CHAR_BIT;
  unsigned long bpml = GMP_NUMB_BITS % uintmax_bit_size;

  /* Special case */
  if (j == 0)
    {
      MPFR_SET_POS(x);
      MPFR_SET_ZERO(x);
      MPFR_RET(0);
    }

  /* early overflow detection to avoid a possible integer overflow below */
  if (MPFR_UNLIKELY(e >= __gmpfr_emax))
    return mpfr_overflow (x, rnd, MPFR_SIGN_POS);

  MPFR_ASSERTN (sizeof(uintmax_t) % sizeof(mp_limb_t) == 0);

  /* Create an auxiliary var */
  MPFR_TMP_INIT1 (yp, y, uintmax_bit_size);
  /* The compiler will optimize the code by removing the useless branch. */
  k = uintmaxpml;
  if (uintmaxpml == 1)
    {
      limb = j;
      count_leading_zeros(cnt, limb);
      /* Normalize the most significant limb */
      yp[0] = limb << cnt;
    }
  else
    {
      mp_size_t len;
      /* Note: either GMP_NUMB_BITS = uintmax_bit_size, then k = 1 the
         shift j >>= bpml is never done, or GMP_NUMB_BITS < uintmax_bit_size
         and bpml = GMP_NUMB_BITS. */
      for (i = 0; i < k; i++, j >>= bpml)
        yp[i] = j; /* Only the low bits are copied */

      /* Find the first limb not equal to zero. */
      do
        {
          MPFR_ASSERTD (k > 0);
          limb = yp[--k];
        }
      while (limb == 0);
      k++;
      len = numberof (yp) - k;
      count_leading_zeros(cnt, limb);

      /* Normalize it: len = number of last zero limbs,
         k = number of previous limbs */
      if (MPFR_LIKELY (cnt != 0))
        mpn_lshift (yp+len, yp, k, cnt);  /* Normalize the high limb */
      else if (len != 0)
        mpn_copyd (yp+len, yp, k);    /* Must use copyd */
      if (len != 0)
        {
          if (len == 1)
            yp[0] = MPFR_LIMB_ZERO;
          else
            MPN_ZERO (yp, len);   /* Zero the last limbs */
        }
    }
  e += k * GMP_NUMB_BITS - cnt;    /* Update Expo */
  MPFR_ASSERTD (MPFR_LIMB_MSB(yp[numberof (yp) - 1]) != 0);

  MPFR_RNDRAW (inex, x, yp, uintmax_bit_size, rnd, MPFR_SIGN_POS, e++);

  /* Check expo underflow / overflow */
  if (MPFR_UNLIKELY(e < __gmpfr_emin))
    {
      /* The following test is necessary because in the rounding to the
       * nearest mode, mpfr_underflow always rounds away from 0. In
       * this rounding mode, we need to round to 0 if:
       *   _ |x| < 2^(emin-2), or
       *   _ |x| = 2^(emin-2) and the absolute value of the exact
       *     result is <= 2^(emin-2). */
      if (rnd == MPFR_RNDN &&
          (e + 1 < __gmpfr_emin ||
           (mpfr_powerof2_raw (x) && inex >= 0)))
        rnd = MPFR_RNDZ;
      return mpfr_underflow (x, rnd, MPFR_SIGN_POS);
    }
  if (MPFR_UNLIKELY(e > __gmpfr_emax))
    return mpfr_overflow (x, rnd, MPFR_SIGN_POS);

  MPFR_SET_SIGN (x, MPFR_SIGN_POS);
  MPFR_SET_EXP (x, e);
  MPFR_RET (inex);
}

#endif
