/* mpfr_log2p1 -- Compute log2(1+x)

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

#define MPFR_NEED_LONGLONG_H /* needed for MPFR_INT_CEIL_LOG2 */
#include "mpfr-impl.h"

#define ULSIZE (sizeof (unsigned long) * CHAR_BIT)

/* return non-zero if log2(1+x) is exactly representable in infinite precision,
   and in such case the returned value is k such that 1+x = 2^k (the case k=0
   cannot happen since we assume x<>0) */
static mpfr_exp_t
mpfr_log2p1_isexact (mpfr_srcptr x)
{
  /* log2(1+x) is exactly representable when 1+x is a power of two,
     we thus simply compute 1+x with 1-bit precision and check whether
     the addition is exact. This routine is called with extended exponent
     range, thus no need to extend it. */
  mpfr_t t;
  int inex;
  mpfr_exp_t e;

  mpfr_init2 (t, 1);
  inex = mpfr_add_ui (t, x, 1, MPFR_RNDZ);
  e = MPFR_GET_EXP (t);
  mpfr_clear (t);
  return inex == 0 ? e - 1 : 0;
}

/* in case x=2^k and we can decide of the correct rounding,
   put the correctly-rounded value in y and return the corresponding
   ternary value (which is necessarily non-zero),
   otherwise return 0 */
static int
mpfr_log2p1_special (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  mpfr_exp_t expx = MPFR_GET_EXP(x);
  mpfr_exp_t k = expx - 1, expk;
  mpfr_prec_t prec;
  mpfr_t t;
  int inex;

  if (k <= 0 || mpfr_cmp_si_2exp (x, 1, k) != 0)
    return 0;
  /* k < log2(1+x) < k + 1/x/log(2) < k + 2/x */
  expk = MPFR_INT_CEIL_LOG2(k); /* exponent of k */
  /* 2/x < 2^(2-EXP(x)) thus if 2-EXP(x) < expk - PREC(y) - 1,
     we have 2/x < 1/4*ulp(k) and we can decide the correct rounding */
  if (2 - expx >= expk - MPFR_PREC(y) - 1)
    return 0;
  prec = (MPFR_PREC(y) + 2 <= ULSIZE) ? ULSIZE : MPFR_PREC(y) + 2;
  mpfr_init2 (t, prec);
  mpfr_set_ui (t, k, MPFR_RNDZ); /* exact since prec >= ULSIZE */
  mpfr_nextabove (t);
  /* now k < t < k + 2/x and round(t) = round(log2(1+x)) */
  inex = mpfr_set (y, t, rnd_mode);
  mpfr_clear (t);
  /* Warning: for RNDF, the mpfr_set calls above might return 0 */
  return (rnd_mode == MPFR_RNDF) ? 1 : inex;
}

/* The computation of log2p1 is done by log2p1(x) = log1p(x)/log(2) */
int
mpfr_log2p1 (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  int comp, inexact, nloop;
  mpfr_t t, lg2;
  mpfr_prec_t Ny = MPFR_PREC(y), prec;
  MPFR_ZIV_DECL (loop);
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (x), mpfr_log_prec, x, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d", mpfr_get_prec (y), mpfr_log_prec, y,
      inexact));

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    return mpfr_log1p (y, x, rnd_mode); /* same result for singular cases */

  comp = mpfr_cmp_si (x, -1);
  /* log2p1(x) is undefined for x < -1 */
  if (MPFR_UNLIKELY(comp <= 0))
    {
      if (comp == 0)
        /* x=0: log2p1(-1)=-inf (divide-by-zero exception) */
        {
          MPFR_SET_INF (y);
          MPFR_SET_NEG (y);
          MPFR_SET_DIVBY0 ();
          MPFR_RET (0);
        }
      MPFR_SET_NAN (y);
      MPFR_RET_NAN;
    }

  MPFR_SAVE_EXPO_MARK (expo);

  prec = Ny + MPFR_INT_CEIL_LOG2 (Ny) + 6;

  mpfr_init2 (t, prec);
  mpfr_init2 (lg2, prec);

  MPFR_ZIV_INIT (loop, prec);
  for (nloop = 0; ; nloop++)
    {
      mpfr_log1p (t, x, MPFR_RNDN);
      mpfr_const_log2 (lg2, MPFR_RNDN);
      mpfr_div (t, t, lg2, MPFR_RNDN);
      /* t = log2(1+x) * (1 + theta)^3 where |theta| < 2^-prec,
         for prec >= 2 we have |(1 + theta)^3 - 1| < 4*theta.
         Note: contrary to log10p1, no underflow is possible in extended
         exponent range, since for tiny x, |log2(1+x)| ~ |x|/log(2) >= |x|,
         and x is representable, thus x/log(2) too. */
      if (MPFR_LIKELY (MPFR_CAN_ROUND (t, prec - 2, Ny, rnd_mode)))
        break;

      if (nloop == 0)
        {
          /* check for exact cases */
          mpfr_exp_t k;

          MPFR_LOG_MSG (("check for exact cases\n", 0));
          k = mpfr_log2p1_isexact (x);
          if (k != 0) /* 1+x = 2^k */
            {
              inexact = mpfr_set_si (y, k, rnd_mode);
              goto end;
            }

          /* if x = 2^k with huge k, Ziv's loop will fail */
          inexact = mpfr_log2p1_special (y, x, rnd_mode);
          if (inexact != 0)
            goto end;
        }

      MPFR_ZIV_NEXT (loop, prec);
      mpfr_set_prec (t, prec);
      mpfr_set_prec (lg2, prec);
    }
  inexact = mpfr_set (y, t, rnd_mode);

 end:
  MPFR_ZIV_FREE (loop);
  mpfr_clear (t);
  mpfr_clear (lg2);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}
