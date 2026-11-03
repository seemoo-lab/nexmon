/* mpfr_fma -- Floating multiply-add

Copyright 2001-2002, 2004, 2006-2025 Free Software Foundation, Inc.
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

/* The fused-multiply-add (fma) of x, y and z is defined by:
   fma(x,y,z)= x*y + z
*/

/* this function deals with all cases where inputs are singular, i.e.,
   either NaN, Inf or zero */
static int
mpfr_fma_singular (mpfr_ptr s, mpfr_srcptr x, mpfr_srcptr y, mpfr_srcptr z,
                   mpfr_rnd_t rnd_mode)
{
  if (MPFR_IS_NAN(x) || MPFR_IS_NAN(y) || MPFR_IS_NAN(z))
    {
      MPFR_SET_NAN(s);
      MPFR_RET_NAN;
    }
  /* now neither x, y or z is NaN */
  else if (MPFR_IS_INF(x) || MPFR_IS_INF(y))
    {
      /* cases Inf*0+z, 0*Inf+z, Inf-Inf */
      if ((MPFR_IS_ZERO(y)) ||
          (MPFR_IS_ZERO(x)) ||
          (MPFR_IS_INF(z) &&
           ((MPFR_MULT_SIGN(MPFR_SIGN(x), MPFR_SIGN(y))) != MPFR_SIGN(z))))
        {
          MPFR_SET_NAN(s);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF(z)) /* case Inf-Inf already checked above */
        {
          MPFR_SET_INF(s);
          MPFR_SET_SAME_SIGN(s, z);
          MPFR_RET(0);
        }
      else /* z is finite */
        {
          MPFR_SET_INF(s);
          MPFR_SET_SIGN(s, MPFR_MULT_SIGN(MPFR_SIGN(x), MPFR_SIGN(y)));
          MPFR_RET(0);
        }
    }
  /* now x and y are finite */
  else if (MPFR_IS_INF(z))
    {
      MPFR_SET_INF(s);
      MPFR_SET_SAME_SIGN(s, z);
      MPFR_RET(0);
    }
  else if (MPFR_IS_ZERO(x) || MPFR_IS_ZERO(y))
    {
      if (MPFR_IS_ZERO(z))
        {
          int sign_p;
          sign_p = MPFR_MULT_SIGN(MPFR_SIGN(x), MPFR_SIGN(y));
          MPFR_SET_SIGN(s, (rnd_mode != MPFR_RNDD ?
                            (MPFR_IS_NEG_SIGN(sign_p) && MPFR_IS_NEG(z) ?
                             MPFR_SIGN_NEG : MPFR_SIGN_POS) :
                            (MPFR_IS_POS_SIGN(sign_p) && MPFR_IS_POS(z) ?
                             MPFR_SIGN_POS : MPFR_SIGN_NEG)));
          MPFR_SET_ZERO(s);
          MPFR_RET(0);
        }
      else
        return mpfr_set (s, z, rnd_mode);
    }
  else /* necessarily z is zero here */
    {
      MPFR_ASSERTD(MPFR_IS_ZERO(z));
      return (x == y) ? mpfr_sqr (s, x, rnd_mode)
        : mpfr_mul (s, x, y, rnd_mode);
    }
}

/* s <- x*y + z */
int
mpfr_fma (mpfr_ptr s, mpfr_srcptr x, mpfr_srcptr y, mpfr_srcptr z,
          mpfr_rnd_t rnd_mode)
{
  int inexact;
  mpfr_t u;
  mp_size_t n;
  mpfr_exp_t e;
  mpfr_prec_t precx, precy;
  MPFR_SAVE_EXPO_DECL (expo);
  MPFR_GROUP_DECL(group);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg y[%Pd]=%.*Rg  z[%Pd]=%.*Rg rnd=%d",
      mpfr_get_prec (x), mpfr_log_prec, x,
      mpfr_get_prec (y), mpfr_log_prec, y,
      mpfr_get_prec (z), mpfr_log_prec, z, rnd_mode),
     ("s[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec (s), mpfr_log_prec, s, inexact));

  /* particular cases */
  if (MPFR_UNLIKELY( MPFR_IS_SINGULAR(x) || MPFR_IS_SINGULAR(y) ||
                     MPFR_IS_SINGULAR(z) ))
    return mpfr_fma_singular (s, x, y, z, rnd_mode);

  e = MPFR_GET_EXP (x) + MPFR_GET_EXP (y);

  precx = MPFR_PREC (x);
  precy = MPFR_PREC (y);

  /* First deal with special case where prec(x) = prec(y) and x*y does
     not overflow nor underflow. Do it only for small sizes since for large
     sizes x*y is faster using Mulders' algorithm (as a rule of thumb,
     we assume mpn_mul_n is faster up to 4*MPFR_MUL_THRESHOLD).
     Since |EXP(x)|, |EXP(y)| < 2^(k-2) on a k-bit computer,
     |EXP(x)+EXP(y)| < 2^(k-1), thus cannot overflow nor underflow. */
  if (precx == precy && e <= __gmpfr_emax && e > __gmpfr_emin)
    {
      if (precx < GMP_NUMB_BITS &&
          MPFR_PREC(z) == precx &&
          MPFR_PREC(s) == precx)
        {
          mp_limb_t umant[2], zmant[2];
          mpfr_t zz;
          int inex;

          umul_ppmm (umant[1], umant[0], MPFR_MANT(x)[0], MPFR_MANT(y)[0]);
          MPFR_PREC(u) = MPFR_PREC(zz) = 2 * precx;
          MPFR_MANT(u) = umant;
          MPFR_MANT(zz) = zmant;
          MPFR_SIGN(u) = MPFR_MULT_SIGN(MPFR_SIGN(x), MPFR_SIGN(y));
          MPFR_SIGN(zz) = MPFR_SIGN(z);
          MPFR_EXP(zz) = MPFR_EXP(z);
          if (MPFR_PREC(zz) <= GMP_NUMB_BITS) /* zz fits in one limb */
            {
              if ((umant[1] & MPFR_LIMB_HIGHBIT) == 0)
                {
                  umant[0] = umant[1] << 1;
                  MPFR_EXP(u) = e - 1;
                }
              else
                {
                  umant[0] = umant[1];
                  MPFR_EXP(u) = e;
                }
              zmant[0] = MPFR_MANT(z)[0];
            }
          else
            {
              zmant[1] = MPFR_MANT(z)[0];
              zmant[0] = MPFR_LIMB_ZERO;
              if ((umant[1] & MPFR_LIMB_HIGHBIT) == 0)
                {
                  umant[1] = (umant[1] << 1) |
                    (umant[0] >> (GMP_NUMB_BITS - 1));
                  umant[0] = umant[0] << 1;
                  MPFR_EXP(u) = e - 1;
                }
              else
                MPFR_EXP(u) = e;
            }
          inex = mpfr_add (u, u, zz, rnd_mode);
          /* mpfr_set_1_2 requires PREC(u) = 2*PREC(s),
             thus we need PREC(s) = PREC(x) = PREC(y) = PREC(z) */
          return mpfr_set_1_2 (s, u, rnd_mode, inex);
        }
      else if ((n = MPFR_LIMB_SIZE(x)) <= 4 * MPFR_MUL_THRESHOLD)
        {
          mpfr_limb_ptr up;
          mp_size_t un = n + n;
          MPFR_TMP_DECL(marker);

          MPFR_TMP_MARK(marker);
          MPFR_TMP_INIT (up, u, un * GMP_NUMB_BITS, un);
          up = MPFR_MANT(u);
          /* multiply x*y exactly into u */
          if (x == y)
            mpn_sqr (up, MPFR_MANT(x), n);
          else
            mpn_mul_n (up, MPFR_MANT(x), MPFR_MANT(y), n);
          if (MPFR_LIMB_MSB (up[un - 1]) == 0)
            {
              mpn_lshift (up, up, un, 1);
              MPFR_EXP(u) = e - 1;
            }
          else
            MPFR_EXP(u) = e;
          MPFR_SIGN(u) = MPFR_MULT_SIGN(MPFR_SIGN(x), MPFR_SIGN(y));
          /* The above code does not generate any exception.
             The exceptions will come only from mpfr_add. */
          inexact = mpfr_add (s, u, z, rnd_mode);
          MPFR_TMP_FREE(marker);
          return inexact;
        }
    }

  /* If we take prec(u) >= prec(x) + prec(y), the product u <- x*y
     is exact, except in case of overflow or underflow. */
  MPFR_ASSERTN (precx + precy <= MPFR_PREC_MAX);
  MPFR_GROUP_INIT_1 (group, precx + precy, u);
  MPFR_SAVE_EXPO_MARK (expo);

  if (MPFR_UNLIKELY (mpfr_mul (u, x, y, MPFR_RNDN)))
    {
      /* overflow or underflow - this case is regarded as rare, thus
         does not need to be very efficient (even if some tests below
         could have been done earlier).
         It is an overflow iff u is an infinity (since MPFR_RNDN was used).
         Alternatively, we could test the overflow flag, but in this case,
         mpfr_clear_flags would have been necessary. */

      if (MPFR_IS_INF (u))  /* overflow */
        {
          int sign_u = MPFR_SIGN (u);

          MPFR_LOG_MSG (("Overflow on x*y\n", 0));
          MPFR_GROUP_CLEAR (group);  /* we no longer need u */

          /* Let's eliminate the obvious case where x*y and z have the
             same sign. No possible cancellation -> real overflow.
             Also, we know that |z| < 2^emax. If E(x) + E(y) >= emax+3,
             then |x*y| >= 2^(emax+1), and |x*y + z| > 2^emax. This case
             is also an overflow. */
          if (sign_u == MPFR_SIGN (z) || e >= __gmpfr_emax + 3)
            {
              MPFR_SAVE_EXPO_FREE (expo);
              return mpfr_overflow (s, rnd_mode, sign_u);
            }
        }
      else  /* underflow: one has |x*y| < 2^(emin-1). */
        {
          MPFR_LOG_MSG (("Underflow on x*y\n", 0));

          /* Easy cases: when 2^(emin-1) <= 1/2 * min(ulp(z),ulp(s)),
             one can replace x*y by sign(x*y) * 2^(emin-1). Note that
             this is even true in case of equality for MPFR_RNDN thanks
             to the even-rounding rule.
             The + 1 on MPFR_PREC (s) is necessary because the exponent
             of the result can be EXP(z) - 1. */
          if (MPFR_GET_EXP (z) - __gmpfr_emin >=
              MAX (MPFR_PREC (z), MPFR_PREC (s) + 1))
            {
              MPFR_PREC (u) = MPFR_PREC_MIN;
              mpfr_setmin (u, __gmpfr_emin);
              MPFR_SET_SIGN (u, MPFR_MULT_SIGN (MPFR_SIGN (x),
                                                MPFR_SIGN (y)));
              mpfr_clear_flags ();
              goto add;
            }

          MPFR_GROUP_CLEAR (group);  /* we no longer need u */
        }

      /* Let's use UBF to resolve the overflow/underflow issues. */
      {
        mpfr_ubf_t uu;
        mp_size_t un;
        mpfr_limb_ptr up;
        MPFR_TMP_DECL(marker);

        MPFR_LOG_MSG (("Use UBF\n", 0));

        MPFR_TMP_MARK (marker);
        un = MPFR_LIMB_SIZE (x) + MPFR_LIMB_SIZE (y);
        MPFR_TMP_INIT (up, uu, (mpfr_prec_t) un * GMP_NUMB_BITS, un);
        mpfr_ubf_mul_exact (uu, x, y);
        mpfr_clear_flags ();
        inexact = mpfr_add (s, (mpfr_srcptr) uu, z, rnd_mode);
        MPFR_UBF_CLEAR_EXP (uu);
        MPFR_TMP_FREE (marker);
      }
    }
  else
    {
    add:
      inexact = mpfr_add (s, u, z, rnd_mode);
      MPFR_GROUP_CLEAR (group);
    }

  MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (s, inexact, rnd_mode);
}
