/* mpfr_log10p1 -- Compute log10(1+x)

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

/* Return non-zero if log10(1+x) is exactly representable in infinite
   precision, and in such case the returned value is k such that 1+x = 10^k
   (the case k=0 cannot happen since we assume x<>0). */
static mpfr_exp_t
mpfr_log10p1_exact_p (mpfr_srcptr x)
{
  /* log10(1+x) is exactly representable when 1+x is a power of 10,
     we thus simply compute 1+x with enough precision and check whether
     the addition is exact. This routine is called with extended exponent
     range, thus no need to extend it. */
  mpfr_t t;
  int inex, ret = 0;

  MPFR_ASSERTD(!MPFR_IS_SINGULAR(x));
  if (MPFR_IS_NEG(x) || MPFR_EXP(x) <= 3) /* x < 8 */
    return 0;
  mpfr_init2 (t, MPFR_PREC(x));
  inex = mpfr_add_ui (t, x, 1, MPFR_RNDZ);
  if (inex == 0) /* otherwise 1+x = 2^k, and cannot be a power of 10 */
    {
      mpfr_prec_t trailing_x = mpfr_min_prec (x);
      mpfr_prec_t trailing_t = mpfr_min_prec (t);
      if (trailing_x > trailing_t)
        {
          mpfr_prec_t k = trailing_x - trailing_t;
          /* if 1+x = 10^k, then t has k more trailing zeros than x */
          mpz_t z;
          mpfr_t y;
          mpz_init (z);
          mpz_ui_pow_ui (z, 5, k);
          mpfr_init2 (y, mpz_sizeinbase (z, 2));
          mpfr_set_z_2exp (y, z, k, MPFR_RNDZ);
          if (mpfr_equal_p (t, y))
            ret = k;
          mpfr_clear (y);
          mpz_clear (z);
        }
    }
  mpfr_clear (t);
  return ret;
}

/* Deal with the case where x is small, so that log10(1+x) ~ x/log(10).
   In case we can round correctly, put in y the correctly-rounded value,
   and return the corresponding ternary value (which cannot be zero).
   Otherwise return 0.
   This routine cannot be called only once after the first failure of Ziv's
   strategy, since it might be that it fails the first time, thus we need
   to pass the (increasing) working precision 'prec'.
   In case of underflow, we set y to 0, and let the caller call
   mpfr_underflow. */
static int
mpfr_log10p1_small (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode,
                    mpfr_prec_t prec)
{
  mpfr_t t;
  mpfr_exp_t e = MPFR_GET_EXP(x);
  int inex;

  /* for |x| < 1/2, |log10(x+1) - x/log(10)| < x^2/log(10) */
  if (e > - (mpfr_exp_t) MPFR_PREC(y))
    return 0; /* the term in x^2 will contribute */
  /* now e = EXP(x) <= -PREC(y) <= -1 which ensures |x| < 1/2 */
  mpfr_init2 (t, prec);
  mpfr_log_ui (t, 10, MPFR_RNDN);
  MPFR_SET_EXP (t, MPFR_GET_EXP (t) - 2);
  /* we divide x by log(10)/4 which is smaller than 1 to avoid any underflow */
  mpfr_div (t, x, t, MPFR_RNDN);
  if (MPFR_GET_EXP (t) < __gmpfr_emin + 2) /* underflow case */
    {
      MPFR_SET_ZERO(y);  /* the sign does not matter */
      inex = 1;
    }
  else
    {
      MPFR_SET_EXP (t, MPFR_GET_EXP (t) - 2);
      /* t = x/log(10) * (1 + theta)^2 where |theta| < 2^-prec.
         For prec>=2, |(1 + theta)^2 - 1| < 3*theta thus the error is
         bounded by 3 ulps. The error term in x^2 is bounded by |t*x|,
         which is less than |t|*2^e < 2^(EXP(t)+e). */
      e += prec;
      /* now the error is bounded by 2^e+3 ulps */
      e = (e >= 2) ? e + 1 : 3;
      /* now the error is bounded by 2^e ulps */
      if (MPFR_CAN_ROUND (t, prec - e, MPFR_PREC(y), rnd_mode))
        inex = mpfr_set (y, t, rnd_mode);
      else
        inex = 0;
    }
  mpfr_clear (t);
  return inex;
}

/* The computation of log10p1 is done by log10p1(x) = log1p(x)/log(2) */
int
mpfr_log10p1 (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  int comp, inexact, nloop;
  mpfr_t t, lg10;
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
  /* log10p1(x) is undefined for x < -1 */
  if (MPFR_UNLIKELY(comp <= 0))
    {
      if (comp == 0)
        /* x=0: log10p1(-1)=-inf (divide-by-zero exception) */
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
  mpfr_init2 (lg10, prec);

  MPFR_ZIV_INIT (loop, prec);
  for (nloop = 0; ; nloop++)
    {
      mpfr_log1p (t, x, MPFR_RNDN);
      mpfr_log_ui (lg10, 10, MPFR_RNDN);
      mpfr_div (t, t, lg10, MPFR_RNDN);
      /* t = log10(1+x) * (1 + theta)^3 where |theta| < 2^-prec,
         for prec >= 2 we have |(1 + theta)^3 - 1| < 4*theta. */
      if (MPFR_LIKELY (MPFR_CAN_ROUND (t, prec - 2, Ny, rnd_mode)))
        break;

      if (nloop == 0)
        {
          /* check for exact cases */
          mpfr_exp_t k;

          MPFR_LOG_MSG (("check for exact cases\n", 0));
          k = mpfr_log10p1_exact_p (x);
          if (k != 0) /* 1+x = 10^k */
            {
              inexact = mpfr_set_si (y, k, rnd_mode);
              goto end;
            }
        }

      /* inexact will be the non-zero ternary value if rounding could be
         done, otherwise it is set to 0. */
      inexact = mpfr_log10p1_small (y, x, rnd_mode, prec);
      if (inexact)
        goto end;

      MPFR_ZIV_NEXT (loop, prec);
      mpfr_set_prec (t, prec);
      mpfr_set_prec (lg10, prec);
    }
  inexact = mpfr_set (y, t, rnd_mode);

 end:
  MPFR_ZIV_FREE (loop);
  mpfr_clear (t);
  mpfr_clear (lg10);

  MPFR_SAVE_EXPO_FREE (expo);
  if (MPFR_IS_ZERO(y)) /* underflow from mpfr_log10p1_small */
    return mpfr_underflow (y, (rnd_mode == MPFR_RNDN) ? MPFR_RNDZ : rnd_mode,
                           1);
  else
    return mpfr_check_range (y, inexact, rnd_mode);
}
