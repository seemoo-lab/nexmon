/* mpfr_exp -- exponential of a floating-point number

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

/* Cache for emin and emax bounds.
   Contrary to other caches, it uses a fixed size for the mantissa,
   so there is no dynamic allocation, and no need to free them. */
static MPFR_THREAD_ATTR mpfr_exp_t previous_emin;
static MPFR_THREAD_ATTR mp_limb_t  bound_emin_limb[(32 - 1) / GMP_NUMB_BITS + 1];
static MPFR_THREAD_ATTR mpfr_t     bound_emin;
static MPFR_THREAD_ATTR mpfr_exp_t previous_emax;
static MPFR_THREAD_ATTR mp_limb_t  bound_emax_limb[(32 - 1) / GMP_NUMB_BITS + 1];
static MPFR_THREAD_ATTR mpfr_t     bound_emax;

int
mpfr_exp (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  mpfr_exp_t expx;
  mpfr_prec_t precy;
  int inexact;
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (x), mpfr_log_prec, x, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec (y), mpfr_log_prec, y, inexact));

  if (MPFR_UNLIKELY( MPFR_IS_SINGULAR(x) ))
    {
      if (MPFR_IS_NAN(x))
        {
          MPFR_SET_NAN(y);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF(x))
        {
          if (MPFR_IS_POS(x))
            MPFR_SET_INF(y);
          else
            MPFR_SET_ZERO(y);
          MPFR_SET_POS(y);
          MPFR_RET(0);
        }
      else
        {
          MPFR_ASSERTD(MPFR_IS_ZERO(x));
          return mpfr_set_ui (y, 1, rnd_mode);
        }
    }

  /* First, let's detect most overflow and underflow cases. */
  /* emax bound is cached. Check if the value in cache is ok */
  if (MPFR_UNLIKELY (mpfr_get_emax() != previous_emax))
    {
      /* Recompute the emax bound */
      mp_limb_t e_limb[MPFR_EXP_LIMB_SIZE];
      mpfr_t e;

      /* We extend the exponent range and save the flags. */
      MPFR_SAVE_EXPO_MARK (expo);

      MPFR_TMP_INIT1(e_limb, e, sizeof (mpfr_exp_t) * CHAR_BIT);
      MPFR_TMP_INIT1(bound_emax_limb, bound_emax, 32);

      inexact = mpfr_set_exp_t (e, expo.saved_emax, MPFR_RNDN);
      MPFR_ASSERTD (inexact == 0);
      mpfr_mul (bound_emax, expo.saved_emax < 0 ?
                __gmpfr_const_log2_RNDD : __gmpfr_const_log2_RNDU,
                e, MPFR_RNDU);
      previous_emax = expo.saved_emax;
      MPFR_SAVE_EXPO_FREE (expo);
    }

  /* mpfr_cmp works even in reduced emin,emax range */
  if (MPFR_UNLIKELY (mpfr_cmp (x, bound_emax) >= 0))
    {
      /* x > log(2^emax), thus exp(x) > 2^emax */
      return mpfr_overflow (y, rnd_mode, 1);
    }

  /* emin bound is cached. Check if the value in cache is ok */
  if (MPFR_UNLIKELY (mpfr_get_emin() != previous_emin))
    {
      mp_limb_t e_limb[MPFR_EXP_LIMB_SIZE];
      mpfr_t e;

      /* We extend the exponent range and save the flags. */
      MPFR_SAVE_EXPO_MARK (expo);

      /* avoid using a full limb since arithmetic might be slower */
      MPFR_TMP_INIT1(e_limb, e, sizeof (mpfr_exp_t) * CHAR_BIT - 1);
      MPFR_TMP_INIT1(bound_emin_limb, bound_emin, 32);

      inexact = mpfr_set_exp_t (e, expo.saved_emin, MPFR_RNDN);
      MPFR_ASSERTD (inexact == 0);
      inexact = mpfr_sub_ui (e, e, 2, MPFR_RNDN);
      MPFR_ASSERTD (inexact == 0);
      mpfr_const_log2 (bound_emin, expo.saved_emin < 0 ? MPFR_RNDU : MPFR_RNDD);
      mpfr_mul (bound_emin, bound_emin, e, MPFR_RNDD);
      previous_emin = expo.saved_emin;
      MPFR_SAVE_EXPO_FREE (expo);
    }

  if (MPFR_UNLIKELY (mpfr_cmp (x, bound_emin) <= 0))
    {
      /* x < log(2^(emin - 2)), thus exp(x) < 2^(emin - 2) */
      return mpfr_underflow (y, rnd_mode == MPFR_RNDN ? MPFR_RNDZ : rnd_mode,
                             1);
    }

  expx  = MPFR_GET_EXP (x);
  precy = MPFR_PREC (y);

  /* if x < 2^(-precy), then exp(x) gives 1 +/- 1 ulp(1) */
  if (MPFR_UNLIKELY (expx < 0 && (mpfr_uexp_t) (-expx) > precy))
    {
      mpfr_exp_t emin = __gmpfr_emin;
      mpfr_exp_t emax = __gmpfr_emax;
      int signx = MPFR_SIGN (x);

      /* Make sure that the exponent range is large enough:
       * [0,2] is sufficient in all precisions.
       */
      __gmpfr_emin = 0;
      __gmpfr_emax = 2;

      MPFR_SET_POS (y);
      if (MPFR_IS_NEG_SIGN (signx) && (rnd_mode == MPFR_RNDD ||
                                       rnd_mode == MPFR_RNDZ))
        {
          mpfr_setmax (y, 0);  /* y = 1 - epsilon */
          inexact = -1;
        }
      else
        {
          mpfr_setmin (y, 1);  /* y = 1 */
          if (MPFR_IS_POS_SIGN (signx) && (rnd_mode == MPFR_RNDU ||
                                           rnd_mode == MPFR_RNDA))
            {
              /* Warning: should work for precision 1 bit too! */
              mpfr_nextabove (y);
              inexact = 1;
            }
          else
            inexact = -MPFR_FROM_SIGN_TO_INT(signx);
        }

      __gmpfr_emin = emin;
      __gmpfr_emax = emax;
    }
  else  /* General case */
    {
      if (MPFR_UNLIKELY (precy >= MPFR_EXP_THRESHOLD))
        /* mpfr_exp_3 saves the exponent range and flags itself, otherwise
           the flag changes in mpfr_exp_3 are lost */
        inexact = mpfr_exp_3 (y, x, rnd_mode); /* O(M(n) log(n)^2) */
      else
        {
          MPFR_SAVE_EXPO_MARK (expo);
          inexact = mpfr_exp_2 (y, x, rnd_mode); /* O(n^(1/3) M(n)) */
          MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);
          MPFR_SAVE_EXPO_FREE (expo);
        }
    }

  return mpfr_check_range (y, inexact, rnd_mode);
}
