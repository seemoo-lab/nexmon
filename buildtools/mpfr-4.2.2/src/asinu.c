/* mpfr_asinu  -- asinu(x) = asin(x)*u/(2*pi)
   mpfr_asinpi -- asinpi(x) = asin(x)/pi

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

/* put in y the correctly rounded value of asin(x)*u/(2*pi) */
int
mpfr_asinu (mpfr_ptr y, mpfr_srcptr x, unsigned long u, mpfr_rnd_t rnd_mode)
{
  mpfr_t tmp, pi;
  mpfr_prec_t prec;
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
          /* asin(0)=0 with same sign, even when u=0 to ensure
             asinu(-x,u) = -asinu(x,u) */
          MPFR_SET_ZERO (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0); /* exact result */
        }
    }

  compared = mpfr_cmpabs_ui (x, 1);
  if (compared > 0)
    {
      /* asinu(x) = NaN for |x| > 1, included for u=0, since NaN*0 = NaN */
      MPFR_SET_NAN (y);
      MPFR_RET_NAN;
    }

  if (u == 0) /* return 0 with sign of x */
    {
      MPFR_SET_ZERO (y);
      MPFR_SET_POS (y);
      MPFR_RET (0);
    }

  if (compared == 0)
    {
      /* |x| = 1: asinu(1,u) = u/4, asinu(-1,u)=-u/4 */
      /* we can't use mpfr_set_si_2exp with -u since -u might not be
         representable as long */
      if (MPFR_SIGN(x) > 0)
        return mpfr_set_ui_2exp (y, u, -2, rnd_mode);
      else
        {
          inexact = mpfr_set_ui_2exp (y, u, -2, MPFR_INVERT_RND(rnd_mode));
          MPFR_CHANGE_SIGN(y);
          return -inexact;
        }
    }

  /* asin(+/-1/2) = +/-pi/6, thus asin(+/-1/2,u) = +/-u/12 is exact when u is
     a multiple of 3 */
  if (mpfr_cmp_si_2exp (x, MPFR_SIGN(x), -1) == 0 && (u % 3) == 0)
    {
      long v = u / 3;
      if (MPFR_IS_NEG (x))
        v = -v;
      return mpfr_set_si_2exp (y, v, -2, rnd_mode);
    }

  prec = MPFR_PREC (y);

  MPFR_SAVE_EXPO_MARK (expo);

  prec += MPFR_INT_CEIL_LOG2(prec) + 10;

  mpfr_init2 (tmp, prec);
  mpfr_init2 (pi, prec);

  MPFR_ZIV_INIT (loop, prec);
  for (;;)
    {
      /* In the error analysis below, each thetax denotes a variable such that
         |thetax| <= 2^(1-prec) */
      mpfr_asin (tmp, x, MPFR_RNDA);
      /* tmp = asin(x) * (1 + theta1), and tmp cannot be zero since we rounded
         away from zero, and the case x=0 was treated before */
      /* first multiply by u to avoid underflow issues */
      mpfr_mul_ui (tmp, tmp, u, MPFR_RNDA);
      /* tmp = asin(x)*u * (1 + theta2)^2, and |tmp| >= 0.5*2^emin */
      mpfr_const_pi (pi, MPFR_RNDZ); /* round toward zero since we we will
                                        divide by pi, to round tmp away */
      /* pi = Pi * (1 + theta3) */
      mpfr_div (tmp, tmp, pi, MPFR_RNDA);
      /* tmp = asin(x)*u/Pi * (1 + theta4)^4, with |tmp| > 0 */
      /* since we rounded away from 0, if we get 0.5*2^emin here, it means
         |asinu(x,u)| < 0.25*2^emin (pi is not exact) thus we have underflow */
      if (MPFR_EXP(tmp) == __gmpfr_emin)
        {
          /* mpfr_underflow rounds away for RNDN */
          mpfr_clear (tmp);
          mpfr_clear (pi);
          MPFR_SAVE_EXPO_FREE (expo);
          return mpfr_underflow (y,
                            (rnd_mode == MPFR_RNDN) ? MPFR_RNDZ : rnd_mode, 1);
        }
      mpfr_div_2ui (tmp, tmp, 1, MPFR_RNDA); /* exact */
      /* tmp = asin(x)*u/(2*Pi) * (1 + theta4)^4 */
      /* since |(1 + theta4)^4 - 1| <= 8*|theta4| for prec >= 3,
         the relative error is less than 2^(4-prec) */
      MPFR_ASSERTD(!MPFR_IS_ZERO(tmp));
      if (MPFR_LIKELY (MPFR_CAN_ROUND (tmp, prec - 4,
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

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}

int
mpfr_asinpi (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  return mpfr_asinu (y, x, 2, rnd_mode);
}
