/* mpfr_exp10m1 -- Compute 10^x-1

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

/* The computation of exp10m1 is done by expm1(x) = 10^x-1 */

/* In case x is small in absolute value, 10^x - 1 ~ x*log(10).
   If this is enough to deduce correct rounding, put in the auxiliary variable
   t the approximation that will be rounded to get y, and return non-zero.
   If we put 0 in t, it means underflow.
   Otherwise return 0. */
static int
mpfr_exp10m1_small (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode,
                    mpfr_ptr t)
{
  mpfr_prec_t prec;
  mpfr_exp_t e;

  /* for |x| < 0.25, we have |10^x-1-x*log(10)| < 4*x^2 */
  if (MPFR_EXP(x) > -2)
    return 0;
  /* now EXP(x) <= -2, thus x < 0.25 */
  prec = MPFR_PREC(t);
  mpfr_log_ui (t, 10, MPFR_RNDN);
  /* t = log(10)*(1 + theta) with |theta| <= 2^(-prec) */
  mpfr_mul (t, t, x, MPFR_RNDN);
  /* no underflow can occur, since log(10) > 1 */
  /* t = x*log(10)*(1 + theta)^2 with |theta| <= 2^(-prec) */
  /* |t - x*log(10)| <= ((1 + theta)^2 - 1) * |t| <= 3*2^(-prec)*|t| */
  /* |t - x*log(10)| < 3*2^(EXP(t)-prec) */
  e = 2 * MPFR_GET_EXP (x) + 2 + prec - MPFR_GET_EXP(t);
  /* |4*x^2| < 2^e*2^(EXP(t)-prec) thus
     |t - exp10m1(x)| < (3+2^e)*2^(EXP(t)-prec) */
  e = (e <= 1) ? 2 + (e == 1) : e + 1;
  /* now |t - exp10m1(x)| < 2^e*2^(EXP(t)-prec) */
  return MPFR_CAN_ROUND (t, prec - e, MPFR_PREC(y), rnd_mode);
}

int
mpfr_exp10m1 (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  int inexact, nloop;
  mpfr_t t;
  mpfr_prec_t Ny = MPFR_PREC(y);   /* target precision */
  mpfr_prec_t Nt;                  /* working precision */
  mpfr_exp_t err, exp_te;          /* error */
  MPFR_ZIV_DECL (loop);
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (x), mpfr_log_prec, x, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d", mpfr_get_prec (y), mpfr_log_prec, y,
      inexact));

  if (MPFR_IS_SINGULAR (x))
    return mpfr_expm1 (y, x, rnd_mode); /* singular cases are identical */

  MPFR_ASSERTN(!MPFR_IS_ZERO(x));

  MPFR_SAVE_EXPO_MARK (expo);

  /* Check case where result is -1 or nextabove(-1) because x is a huge
     negative number. */
  if (MPFR_IS_NEG(x) && mpfr_cmpabs_ui (x, 2 + (MPFR_PREC(y) - 1) / 3) > 0)
    {
      MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, MPFR_FLAGS_INEXACT);
      /* 1/2*ulp(-1) = 2^(-PREC(y)):
         since |x| >= PREC(y)/3 + 1, then 3*|x| >= PREC(y) + 3,
         thus 10^x < 8^x <= 2^(-PREC(y)-3) <= 1/2*ulp(-1), thus the
         result is -1 for RNDA,RNDD,RNDN, and nextabove(-1) for RNDZ,RNDU */
      mpfr_set_si (y, -1, MPFR_RNDZ);
      if (!MPFR_IS_LIKE_RNDZ(rnd_mode,1))
        inexact = -1;
      else
        {
          mpfr_nextabove (y);
          inexact = 1;
        }
      goto end;
    }

  Nt = Ny + MPFR_INT_CEIL_LOG2 (Ny) + 6;

  mpfr_init2 (t, Nt);

  MPFR_ZIV_INIT (loop, Nt);
  for (nloop = 0;; nloop++)
    {
      int inex1;

      MPFR_BLOCK_DECL (flags);

      /* 10^x may overflow and underflow */
      MPFR_BLOCK (flags, inex1 = mpfr_exp10 (t, x, MPFR_RNDN));

      if (MPFR_OVERFLOW (flags)) /* overflow case */
        {
          inexact = mpfr_overflow (y, rnd_mode, MPFR_SIGN_POS);
          MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, MPFR_FLAGS_OVERFLOW);
          goto clear;
        }

      /* integer case */
      if (inex1 == 0)
        {
          inexact = mpfr_sub_ui (y, t, 1, rnd_mode);
          goto clear;
        }

      /* The case of underflow in 10^x (huge negative x)
         was already detected before Ziv's loop. */
      MPFR_ASSERTD(!MPFR_UNDERFLOW (flags));

      MPFR_ASSERTN(!MPFR_IS_ZERO(t));
      exp_te = MPFR_GET_EXP (t);
      mpfr_sub_ui (t, t, 1, MPFR_RNDN);   /* 10^x-1 */

      /* error estimate */
      /* err = __gmpfr_ceil_log2(1+pow(2,MPFR_EXP(te)-MPFR_EXP(t))) */
      if (!MPFR_IS_ZERO(t))
        {
          err = MAX (exp_te - MPFR_GET_EXP (t), 0) + 1;
          /* if inex1=0, this means that t=o(10^x) is exact, thus the correct
             rounding is simply o(t-1) */
          if (inex1 == 0 ||
              MPFR_LIKELY (MPFR_CAN_ROUND (t, Nt - err, Ny, rnd_mode)))
            break;
        }

      /* check small case: we need to do it at each step of Ziv's loop,
         since the multiplication x*log(10) might not enable correct
         rounding at the first loop */
      if (mpfr_exp10m1_small (y, x, rnd_mode, t))
        {
          if (MPFR_IS_ZERO(t)) /* underflow */
            {
              mpfr_clear (t);
              MPFR_SAVE_EXPO_FREE (expo);
              return mpfr_underflow (y, (rnd_mode == MPFR_RNDN) ? MPFR_RNDZ
                                     : rnd_mode, 1);
            }
          break;
        }

      /* increase the precision */
      MPFR_ZIV_NEXT (loop, Nt);
      mpfr_set_prec (t, Nt);
    }

  inexact = mpfr_set (y, t, rnd_mode);
 clear:
  MPFR_ZIV_FREE (loop);
  mpfr_clear (t);

 end:
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}
