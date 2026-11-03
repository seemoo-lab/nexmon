/* mpfr_acosu  -- acosu(x)  = acos(x)*u/(2*pi)
   mpfr_acospi -- acospi(x) = acos(x)/pi

Copyright 2021-2025 Free Software Foundation, Inc.
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

/* put in y the correctly rounded value of acos(x)*u/(2*pi) */
int
mpfr_acosu (mpfr_ptr y, mpfr_srcptr x, unsigned long u, mpfr_rnd_t rnd_mode)
{
  mpfr_t tmp, pi;
  mpfr_prec_t prec;
  mpfr_exp_t expx;
  int compared, inexact;
  MPFR_SAVE_EXPO_DECL (expo);
  MPFR_ZIV_DECL (loop);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg u=%lu rnd=%d", mpfr_get_prec(x), mpfr_log_prec, x, u,
      rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d", mpfr_get_prec (y), mpfr_log_prec, y,
      inexact));

  /* Singular cases */
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x) || MPFR_IS_INF (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      else /* necessarily x=0 */
        {
          MPFR_ASSERTD(MPFR_IS_ZERO(x));
          /* acos(0)=Pi/2 thus acosu(0)=u/4 */
          return mpfr_set_ui_2exp (y, u, -2, rnd_mode);
        }
    }

  compared = mpfr_cmpabs_ui (x, 1);
  if (compared > 0)
    {
      /* acosu(x) = NaN for |x| > 1, included for u=0, since NaN*0 = NaN */
      MPFR_SET_NAN (y);
      MPFR_RET_NAN;
    }

  if (u == 0) /* return +0 since acos(x)>=0 */
    {
      MPFR_SET_ZERO (y);
      MPFR_SET_POS (y);
      MPFR_RET (0);
    }

  if (compared == 0)
    {
      /* |x| = 1: acosu(1,u) = +0, acosu(-1,u)=u/2 */
      if (MPFR_SIGN(x) > 0) /* IEEE-754 2019: acosPi(1) = +0 */
        return mpfr_set_ui (y, 0, rnd_mode);
      else
        return mpfr_set_ui_2exp (y, u, -1, rnd_mode);
    }

  /* acos(1/2) = pi/6 and acos(-1/2) = pi/3, thus in these cases acos(x,u)
     is exact when u is a multiple of 3 */
  if (mpfr_cmp_si_2exp (x, MPFR_SIGN(x), -1) == 0 && (u % 3) == 0)
    return mpfr_set_si_2exp (y, u / 3, MPFR_IS_NEG (x) ? 0 : -1, rnd_mode);

  prec = MPFR_PREC (y);

  MPFR_SAVE_EXPO_MARK (expo);

  /* For |x|<0.5, we have acos(x) = pi/2 - x*r(x) with |r(x)| < 1.05
     thus acosu(x,u) = u/4*(1 - x*s(x)) with 0 <= s(x) < 1.
     If EXP(x) <= -prec-3, then |u/4*x*s(x)| < u/4*2^(-prec-3) < ulp(u/4)/8
     <= ulp(RN(u/4))/4, thus the result will be u/4, nextbelow(u/4) or
     nextabove(u/4).
     Warning: when u/4 is a power of two, the difference between u/4 and
     nextbelow(u/4) is only 1/4*ulp(u/4).
     We also require x < 2^-64, so that in the case u/4 is not exact,
     the contribution of x*s(x) is smaller compared to the last bit of u. */
  expx = MPFR_GET_EXP(x);
  if (expx <= -64 && expx <= - (mpfr_exp_t) prec - 3)
    {
      prec = (MPFR_PREC(y) <= 63) ? 65 : MPFR_PREC(y) + 2;
      /* now prec > 64 and prec > MPFR_PREC(y)+1 */
      mpfr_init2 (tmp, prec);
      inexact = mpfr_set_ui (tmp, u, MPFR_RNDN); /* exact since prec >= 64 */
      MPFR_ASSERTD(inexact == 0);
      /* for x>0, we have acos(x) < pi/2; for x<0, we have acos(x) > pi/2 */
      if (MPFR_IS_POS(x))
        mpfr_nextbelow (tmp);
      else
        mpfr_nextabove (tmp);
      /* Since prec >= 65, the last significant bit of tmp is 1, and since
         prec > PREC(y), tmp is not representable in the target precision,
         which ensures we will get a correct ternary value below. */
      MPFR_ASSERTD(mpfr_min_prec(tmp) > MPFR_PREC(y));
      /* since prec >= PREC(y)+2, the rounding of tmp is correct */
      inexact = mpfr_div_2ui (y, tmp, 2, rnd_mode);
      mpfr_clear (tmp);
      goto end;
    }

  prec += MPFR_INT_CEIL_LOG2(prec) + 10;

  mpfr_init2 (tmp, prec);
  mpfr_init2 (pi, prec);

  MPFR_ZIV_INIT (loop, prec);
  for (;;)
    {
      /* In the error analysis below, each thetax denotes a variable such that
         |thetax| <= 2^-prec */
      mpfr_acos (tmp, x, MPFR_RNDN);
      /* tmp = acos(x) * (1 + theta1) */
      mpfr_const_pi (pi, MPFR_RNDN);
      /* pi = Pi * (1 + theta2) */
      mpfr_div (tmp, tmp, pi, MPFR_RNDN);
      /* tmp = acos(x)/Pi * (1 + theta3)^3 */
      mpfr_mul_ui (tmp, tmp, u, MPFR_RNDN);
      /* tmp = acos(x)*u/Pi * (1 + theta4)^4 */
      mpfr_div_2ui (tmp, tmp, 1, MPFR_RNDN); /* exact */
      /* tmp = acos(x)*u/(2*Pi) * (1 + theta4)^4 */
      /* since |(1 + theta4)^4 - 1| <= 8*|theta4| for prec >= 2,
         the relative error is less than 2^(3-prec) */
      if (MPFR_LIKELY (MPFR_CAN_ROUND (tmp, prec - 3,
                                       MPFR_PREC (y), rnd_mode)))
        break;
      MPFR_ZIV_NEXT (loop, prec);
      mpfr_set_prec (tmp, prec);
      mpfr_set_prec (pi, prec);
    }
  MPFR_ZIV_FREE (loop);

  inexact = mpfr_set (y, tmp, rnd_mode);
  mpfr_clear (tmp);
  mpfr_clear (pi);

 end:
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}

int
mpfr_acospi (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  return mpfr_acosu (y, x, 2, rnd_mode);
}
