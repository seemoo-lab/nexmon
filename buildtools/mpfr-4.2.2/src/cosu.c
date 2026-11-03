/* mpfr_cosu  -- cosu(x) = cos(2*pi*x/u)
   mpfr_cospi -- cospi(x) = cos(pi*x)

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

/* put in y the correctly rounded value of cos(2*pi*x/u) */
int
mpfr_cosu (mpfr_ptr y, mpfr_srcptr x, unsigned long u, mpfr_rnd_t rnd_mode)
{
  mpfr_srcptr xp;
  mpfr_prec_t precy, prec;
  mpfr_exp_t expx, expt, err, log2u, erra, errb;
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
      else /* x is zero: cos(0) = 1 */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (x));
          return mpfr_set_ui (y, 1, rnd_mode);
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

      /* Let's compute xr = x mod u, with signbit(xr) = signbit(x), though
         this doesn't matter.
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
          return mpfr_set_ui (y, 1, rnd_mode);
        }
      xp = xr;
    }

#define CLEAR_XR                       \
  do                                   \
    if (xp != x)                       \
      {                                \
        MPFR_ASSERTD (xp == xr);       \
        mpfr_clear (xr);               \
      }                                \
  while (0)

  /* now |xp/u| < 1 */

  /* for x small, we have |cos(2*pi*x/u)-1| < 1/2*(2*pi*x/u)^2 < 2^5*(x/u)^2 */
  expx = MPFR_GET_EXP (xp);
  log2u = u == 1 ? 0 : MPFR_INT_CEIL_LOG2 (u) - 1;
  /* u >= 2^log2u thus 1/u <= 2^(-log2u) */
  erra = -2 * expx;
  errb = 5 - 2 * log2u;
  /* The 3rd argument (err1) of MPFR_SMALL_INPUT_AFTER_SAVE_EXPO should be
     erra - errb, but it may overflow. The negative overflow is avoided by
     the test erra > errb: if erra - errb <= 0, the macro is no-op.
     Saturate to MPFR_EXP_MAX in case of positive overflow, as the error
     test in MPFR_SMALL_INPUT_AFTER_SAVE_EXPO will always be true for
     any value >= MPFR_PREC_MAX + 1, and this includes MPFR_EXP_MAX (from
     the definition of MPFR_PREC_MAX and mpfr_exp_t >= mpfr_prec_t). */
  if (erra > errb)
    {
      mpfr_exp_t err1 = errb >= 0 || erra < MPFR_EXP_MAX + errb ?
        erra - errb : MPFR_EXP_MAX;
      MPFR_SMALL_INPUT_AFTER_SAVE_EXPO (y, __gmpfr_one, err1, 0, 0,
                                        rnd_mode, expo, CLEAR_XR);
    }

  precy = MPFR_GET_PREC (y);
  /* For x large, since argument reduction is expensive, we want to avoid
     any failure in Ziv's strategy, thus we take into account expx too. */
  prec = precy + MAX(expx,MPFR_INT_CEIL_LOG2 (precy)) + 8;
  MPFR_ASSERTD(prec >= 2);
  mpfr_init2 (t, prec);
  MPFR_ZIV_INIT (loop, prec);
  for (;;)
    {
      nloops ++;
      /* In the error analysis below, xp stands for x.
         We first compute an approximation t of 2*pi*x/u, then call cos(t).
         If t = 2*pi*x/u + s, then |cos(t) - cos(2*pi*x/u)| <= |s|. */
      mpfr_set_prec (t, prec);
      mpfr_const_pi (t, MPFR_RNDN); /* t = pi * (1 + theta1) where
                                       |theta1| <= 2^-prec */
      mpfr_mul_2ui (t, t, 1, MPFR_RNDN); /* t = 2*pi * (1 + theta1) */
      mpfr_mul (t, t, xp, MPFR_RNDN);    /* t = 2*pi*x * (1 + theta2)^2 where
                                            |theta2| <= 2^-prec */
      mpfr_div_ui (t, t, u, MPFR_RNDN);  /* t = 2*pi*x/u * (1 + theta3)^3 where
                                            |theta3| <= 2^-prec */
      /* if t is zero here, it means the division by u underflowd */
      if (MPFR_UNLIKELY (MPFR_IS_ZERO (t)))
        {
          mpfr_set_ui (y, 1, MPFR_RNDZ);
          if (MPFR_IS_LIKE_RNDZ(rnd_mode,0))
            {
              inexact = -1;
              mpfr_nextbelow (y);
            }
          else
            inexact = 1;
          goto end;
        }
      /* since prec >= 2, |(1 + theta3)^3 - 1| <= 4*theta3 <= 2^(2-prec) */
      expt = MPFR_GET_EXP (t);
      /* we have |s| <= 2^(expt + 2 - prec) */
      mpfr_cos (t, t, MPFR_RNDN);
      err = expt + 2 - prec;
      expt = MPFR_GET_EXP (t); /* new exponent of t */
      /* the total error is at most 2^err + ulp(t)/2 = 2^err + 2^(expt-prec-1)
         thus if err <= expt-prec-1, it is bounded by 2^(expt-prec),
         otherwise it is bounded by 2^(err+1). */
      err = (err <= expt - prec - 1) ? expt - prec : err + 1;
      /* normalize err for mpfr_can_round */
      err = expt - err;
      if (MPFR_CAN_ROUND (t, err, precy, rnd_mode))
        break;
      /* Check exact cases only after the first level of Ziv' strategy, to
         avoid slowing down the average case. Exact cases are:
         (a) 2*pi*x/u is a multiple of pi/2, i.e., x/u is a multiple of 1/4
         (b) 2*pi*x/u is {pi/3,2pi/3,4pi/3,5pi/3} mod 2pi */
      if (nloops == 1)
        {
          /* detect case (a) */
          inexact = mpfr_div_ui (t, xp, u, MPFR_RNDZ);
          mpfr_mul_2ui (t, t, 2, MPFR_RNDZ);
          if (inexact == 0 && mpfr_integer_p (t))
            {
              if (mpfr_odd_p (t))
                /* t is odd: we have kpi+pi/2, thus cosu = 0,
                   for the sign, we always return +0, following IEEE 754-2019:
                   cosPi(n + 1/2) is +0 for any integer n when n + 1/2 is
                   representable. */
                mpfr_set_zero (y, +1);
              else /* t is even: case kpi */
                {
                  mpfr_div_2ui (t, t, 1, MPFR_RNDZ);
                  if (!mpfr_odd_p (t))
                    /* case 2kpi: cosu = 1 */
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
              /* t should be in {1/2,2/2,4/2,5/2} */
              mpfr_mul_2ui (t, t, 1, MPFR_RNDZ);
              /* t should be {1,2,4,5} mod 6:
                 t = 1 mod 6: case pi/3: return 1/2
                 t = 2 mod 6: case 2pi/3: return -1/2
                 t = 4 mod 6: case 4pi/3: return -1/2
                 t = 5 mod 6: case 5pi/3: return 1/2 */
              if (inexact == 0 && mpfr_integer_p (t))
                {
                  mpz_t z;
                  unsigned long mod6;
                  mpz_init (z);
                  inexact = mpfr_get_z (z, t, MPFR_RNDZ);
                  MPFR_ASSERTN(inexact == 0);
                  mod6 = mpz_fdiv_ui (z, 6);
                  mpz_clear (z);
                  if (mod6 == 1 || mod6 == 5)
                    {
                      mpfr_set_ui_2exp (y, 1, -1, MPFR_RNDZ);
                      goto end;
                    }
                  else /* we cannot have mod6 = 0 or 3 since those
                          case belong to (a) */
                    {
                      MPFR_ASSERTD(mod6 == 2 || mod6 == 4);
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
  CLEAR_XR;
  MPFR_SAVE_EXPO_FREE (expo);
  return underflow ? inexact : mpfr_check_range (y, inexact, rnd_mode);
}

int
mpfr_cospi (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  return mpfr_cosu (y, x, 2, rnd_mode);
}
