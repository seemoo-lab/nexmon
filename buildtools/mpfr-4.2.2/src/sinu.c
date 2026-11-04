/* mpfr_sinu  -- sinu(x) = sin(2*pi*x/u)
   mpfr_sinpi -- sinpi(x) = sin(pi*x)

Copyright 2020-2025 Free Software Foundation, Inc.
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

/* References:
 * Steve Kargl wrote sinpi and friends for FreeBSD's libm under BSD
   license:
   https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=218514
 */

/* put in y the correctly rounded value of sin(2*pi*x/u) */
int
mpfr_sinu (mpfr_ptr y, mpfr_srcptr x, unsigned long u, mpfr_rnd_t rnd_mode)
{
  mpfr_srcptr xp;
  mpfr_prec_t precy, prec;
  mpfr_exp_t expx, expt, err;
  mpfr_t t, xr;
  int inexact = 0, nloops = 0, underflow = 0;
  MPFR_ZIV_DECL (loop);
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC (
    ("x[%Pd]=%.*Rg u=%lu rnd=%d", mpfr_get_prec (x), mpfr_log_prec, x, u,
     rnd_mode),
    ("y[%Pd]=%.*Rg inexact=%d", mpfr_get_prec (y), mpfr_log_prec, y,
     inexact));

  if (u == 0 || MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      /* for u=0, return NaN */
      if (u == 0 || MPFR_IS_NAN (x) || MPFR_IS_INF (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      else /* x is zero */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (x));
          MPFR_SET_ZERO (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0);
        }
    }

  MPFR_SAVE_EXPO_MARK (expo);

  /* Range reduction. We do not need to reduce the argument if it is
     already reduced (|x| < u).
     Note that the case |x| = u is better in the "else" branch as it
     will give xr = 0. */
  if (mpfr_cmpabs_ui (x, u) < 0)
    {
      xp = x;
    }
  else
    {
      mpfr_exp_t p = MPFR_GET_PREC (x) - MPFR_GET_EXP (x);
      int inex;

      /* Let's compute xr = x mod u, with signbit(xr) = signbit(x), which
         may be important when x is a multiple of u, in which case xr = 0
         (but this property is actually not needed in the code below).
         The precision of xr is chosen to ensure that x mod u is exactly
         representable in xr, e.g., the maximum size of u + the length of
         the fractional part of x. Note that since |x| >= u in this branch,
         the additional memory amount will not be more than the one of x.
      */
      mpfr_init2 (xr, sizeof (unsigned long) * CHAR_BIT + (p < 0 ? 0 : p));
      MPFR_DBGRES (inex = mpfr_fmod_ui (xr, x, u, MPFR_RNDN));  /* exact */
      MPFR_ASSERTD (inex == 0);
      if (MPFR_IS_ZERO (xr))
        {
          mpfr_clear (xr);
          MPFR_SAVE_EXPO_FREE (expo);
          MPFR_SET_ZERO (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0);
        }
      xp = xr;
    }

  /* now |xp/u| < 1 */

  precy = MPFR_GET_PREC (y);
  expx = MPFR_GET_EXP (xp);
  /* For x large, since argument reduction is expensive, we want to avoid
     any failure in Ziv's strategy, thus we take into account expx too. */
  prec = precy + MAX(expx, MPFR_INT_CEIL_LOG2 (precy)) + 8;
  MPFR_ASSERTD(prec >= 2);
  mpfr_init2 (t, prec);
  MPFR_ZIV_INIT (loop, prec);
  for (;;)
    {
      nloops ++;
      /* In the error analysis below, xp stands for x.
         We first compute an approximation t of 2*pi*x/u, then call sin(t).
         If t = 2*pi*x/u + s, then |sin(t) - sin(2*pi*x/u)| <= |s|. */
      mpfr_set_prec (t, prec);
      mpfr_const_pi (t, MPFR_RNDN); /* t = pi * (1 + theta1) where
                                       |theta1| <= 2^-prec */
      mpfr_mul_2ui (t, t, 1, MPFR_RNDN); /* t = 2*pi * (1 + theta1) */
      mpfr_mul (t, t, xp, MPFR_RNDN);    /* t = 2*pi*x * (1 + theta2)^2 where
                                            |theta2| <= 2^-prec */
      mpfr_div_ui (t, t, u, MPFR_RNDN);  /* t = 2*pi*x/u * (1 + theta3)^3 where
                                            |theta3| <= 2^-prec */
      /* if t is zero here, it means the division by u underflows, then
         sin(t) also underflows, since |sin(x)| <= |x|. */
      if (MPFR_UNLIKELY (MPFR_IS_ZERO (t)))
        {
          inexact = mpfr_underflow (y, rnd_mode, MPFR_SIGN(t));
          MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, MPFR_FLAGS_INEXACT
                                       | MPFR_FLAGS_UNDERFLOW);
          underflow = 1;
          goto end;
        }
      /* since prec >= 2, |(1 + theta3)^3 - 1| <= 4*theta3 <= 2^(2-prec) */
      expt = MPFR_GET_EXP (t);
      /* we have |s| <= 2^(expt + 2 - prec) */
      mpfr_sin (t, t, MPFR_RNDA);
      /* t cannot be zero here, since we excluded t=0 before, which is the
         only exact case where sin(t)=0, and we round away from zero */
      err = expt + 2 - prec;
      expt = MPFR_GET_EXP (t); /* new exponent of t */
      /* the total error is bounded by 2^err + ulp(t) = 2^err + 2^(expt-prec)
         thus if err <= expt-prec, it is bounded by 2^(expt-prec+1),
         otherwise it is bounded by 2^(err+1). */
      err = (err <= expt - prec) ? expt - prec + 1 : err + 1;
      /* normalize err for mpfr_can_round */
      err = expt - err;
      if (MPFR_CAN_ROUND (t, err, precy, rnd_mode))
        break;
      /* Check exact cases only after the first level of Ziv' strategy, to
         avoid slowing down the average case. Exact cases are:
         (a) 2*pi*x/u is a multiple of pi/2, i.e., x/u is a multiple of 1/4
         (b) 2*pi*x/u is +/-pi/6 modulo pi, i.e., x/u = +/-1/12 mod 1/2 */
      if (nloops == 1)
        {
          /* detect case (a) */
          inexact = mpfr_div_ui (t, xp, u, MPFR_RNDA);
          mpfr_mul_2ui (t, t, 2, MPFR_RNDA);
          if (inexact == 0 && mpfr_integer_p (t))
            {
              if (!mpfr_odd_p (t))
                /* t is even: we have a multiple of pi, thus sinu = 0,
                   for the sign, we follow IEEE 754-2019: sinPi(+n) is +0
                   and sinPi(-n) is -0 for positive integers n, so that the
                   function is odd. */
                mpfr_set_zero (y, MPFR_IS_POS(t) ? +1 : -1);
              else /* t is odd */
                {
                  inexact = mpfr_sub_ui (t, t, 1, MPFR_RNDZ);
                  MPFR_ASSERTD(inexact == 0);
                  inexact = mpfr_div_2ui (t, t, 1, MPFR_RNDZ);
                  MPFR_ASSERTD(inexact == 0);
                  if (MPFR_IS_ZERO (t) || !mpfr_odd_p (t))
                    /* case pi/2: sinu = 1 */
                    mpfr_set_ui (y, 1, MPFR_RNDZ);
                  else
                    mpfr_set_si (y, -1, MPFR_RNDZ);
                }
              goto end;
            }
          /* detect case (b): this can only occur if u is divisible by 3 */
          if ((u % 3) == 0)
            {
              inexact = mpfr_div_ui (t, xp, u / 3, MPFR_RNDZ);
              /* t should be +/-1/4 mod 3/2 */
              mpfr_mul_2ui (t, t, 2, MPFR_RNDZ);
              /* t should be +/-1 mod 6, i.e., in {1,5,7,11} mod 12:
                 t = 1 mod 6: case pi/6: return 1/2
                 t = 5 mod 6: case 5pi/6: return 1/2
                 t = 7 mod 6: case 7pi/6: return -1/2
                 t = 11 mod 6: case 11pi/6: return -1/2 */
              if (inexact == 0 && mpfr_integer_p (t))
                {
                  mpz_t z;
                  unsigned long mod12;
                  mpz_init (z);
                  inexact = mpfr_get_z (z, t, MPFR_RNDZ);
                  MPFR_ASSERTN(inexact == 0);
                  mod12 = mpz_fdiv_ui (z, 12);
                  mpz_clear (z);
                  if (mod12 == 1 || mod12 == 5)
                    {
                      mpfr_set_ui_2exp (y, 1, -1, MPFR_RNDZ);
                      goto end;
                    }
                  else if (mod12 == 7 || mod12 == 11)
                    {
                      mpfr_set_si_2exp (y, -1, -1, MPFR_RNDZ);
                      goto end;
                    }
                }
            }
        }
      MPFR_ZIV_NEXT (loop, prec);
    }
  MPFR_ZIV_FREE (loop);

  inexact = mpfr_set (y, t, rnd_mode);

 end:
  mpfr_clear (t);
  if (xp != x)
    {
      MPFR_ASSERTD (xp == xr);
      mpfr_clear (xr);
    }
  MPFR_SAVE_EXPO_FREE (expo);
  return underflow ? inexact : mpfr_check_range (y, inexact, rnd_mode);
}

int
mpfr_sinpi (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  return mpfr_sinu (y, x, 2, rnd_mode);
}
