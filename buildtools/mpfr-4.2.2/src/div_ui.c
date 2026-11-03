/* mpfr_div_ui -- divide a floating-point number by a machine integer

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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

#ifdef MPFR_COV_CHECK
int __gmpfr_cov_div_ui_sb[10][2] = { 0 };
#endif

/* returns 0 if result exact, non-zero otherwise */
#undef mpfr_div_ui
MPFR_HOT_FUNCTION_ATTR int
mpfr_div_ui (mpfr_ptr y, mpfr_srcptr x, unsigned long int u,
             mpfr_rnd_t rnd_mode)
{
  int inexact;

#ifdef MPFR_LONG_WITHIN_LIMB

  int sh;
  mp_size_t i, xn, yn, dif;
  mp_limb_t *xp, *yp, *tmp, c, d;
  mpfr_exp_t exp;
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  MPFR_TMP_DECL(marker);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg u=%lu rnd=%d",
      mpfr_get_prec(x), mpfr_log_prec, x, u, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec(y), mpfr_log_prec, y, inexact));

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF (x))
        {
          MPFR_SET_INF (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0);
        }
      else
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (x));
          if (u == 0) /* 0/0 is NaN */
            {
              MPFR_SET_NAN (y);
              MPFR_RET_NAN;
            }
          else
            {
              MPFR_SET_ZERO(y);
              MPFR_SET_SAME_SIGN (y, x);
              MPFR_RET(0);
            }
        }
    }
  else if (MPFR_UNLIKELY (u <= 1))
    {
      if (u < 1)
        {
          /* x/0 is Inf since x != 0 */
          MPFR_SET_INF (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_SET_DIVBY0 ();
          MPFR_RET (0);
        }
      else /* y = x/1 = x */
        return mpfr_set (y, x, rnd_mode);
    }
  else if (MPFR_UNLIKELY (IS_POW2 (u)))
    return mpfr_div_2si (y, x, MPFR_INT_CEIL_LOG2 (u), rnd_mode);

  MPFR_SET_SAME_SIGN (y, x);

  MPFR_TMP_MARK (marker);

  xn = MPFR_LIMB_SIZE (x);
  yn = MPFR_LIMB_SIZE (y);

  xp = MPFR_MANT (x);
  yp = MPFR_MANT (y);
  exp = MPFR_GET_EXP (x);

  dif = yn + 1 - xn;

  /* we need to store yn + 1 = xn + dif limbs of the quotient */
  tmp = MPFR_TMP_LIMBS_ALLOC (yn + 1);

  /* Notation: {p, n} denotes the integer formed by the n limbs
     from p[0] to p[n-1]. Let B = 2^GMP_NUMB_BITS.
     One has: 0 <= {p, n} < B^n. */

  if (dif >= 0)
    {
      c = mpn_divrem_1 (tmp, dif, xp, xn, u); /* used all the dividend */
      /* {xp, xn} = ({tmp, xn+dif} * u + c) * B^(-dif)
                  = ({tmp, yn+1} * u + c) * B^(-dif) */
    }
  else /* dif < 0, i.e. xn > yn+1; ignore the (-dif) low limbs from x */
    {
      c = mpn_divrem_1 (tmp, 0, xp - dif, yn + 1, u);
      /* {xp-dif, yn+1} = {tmp, yn+1} * u + c
         thus
         {xp, xn} = {xp, -dif} + {xp-dif, yn+1} * B^(-dif)
                  = {xp, -dif} + ({tmp, yn+1} * u + c) * B^(-dif) */
    }

  /* Let r = {xp, -dif} / B^(-dif) if dif < 0, r = 0 otherwise; 0 <= r < 1.
     Then {xp, xn} = ({tmp, yn+1} * u + c + r) * B^(-dif).
     x / u = ({xp, xn} / u) * B^(-xn) * 2^exp
           = ({tmp, yn+1} + (c + r) / u) * B^(-(yn+1)) * 2^exp
     where 0 <= (c + r) / u < 1. */

  for (sb = 0, i = 0; sb == 0 && i < -dif; i++)
    if (xp[i])
      sb = 1;
  /* sb != 0 iff r != 0 */

  /*
     If the highest limb of the result is 0 (xp[xn-1] < u), remove it.
     Otherwise, compute the left shift to be performed to normalize.
     In the latter case, we discard some low bits computed. They
     contain information useful for the rounding, hence the updating
     of middle and inexact.
  */

  MPFR_UNSIGNED_MINUS_MODULO (sh, MPFR_PREC (y));
  /* sh: number of the trailing bits of y */

  if (tmp[yn] == 0)
    {
      MPN_COPY(yp, tmp, yn);
      exp -= GMP_NUMB_BITS;
      if (sh == 0) /* round bit is 1 iff (c + r) / u >= 1/2 */
        {
          /* In this case tmp[yn]=0 and sh=0, the round bit is not in
             {tmp,yn+1}. It is 1 iff 2*(c+r) - u >= 0. This means that in
             some cases, we should look at the most significant bit of r. */
          if (c >= u - c) /* i.e. 2c >= u: round bit is always 1 */
            {
              rb = 1;
              /* The sticky bit is 1 unless 2c-u = 0 and r = 0. */
              sb |= 2 * c - u;
              MPFR_COV_SET (div_ui_sb[0][!!sb]);
            }
          else /* 2*c < u */
            {
              /* The round bit is 1 iff r >= 1/2 and 2*(c+1/2) = u. */
              rb = (c == u/2) && (dif < 0) && (xp[-dif-1] & MPFR_LIMB_HIGHBIT);
              /* If rb is set, we need to recompute sb, since it might have
                 taken into account the msb of xp[-dif-1]. */
              if (rb)
                {
                  sb = xp[-dif-1] << 1; /* discard the most significant bit */
                  for (i = 0; sb == 0 && i < -dif-1; i++)
                    if (xp[i])
                      sb = 1;
                  /* The dif < -1 case with sb = 0, i.e. [2][0], will
                     ensure that the body of the loop is covered. */
                  MPFR_COV_SET (div_ui_sb[1 + (dif < -1)][!!sb]);
                }
              else
                {
                  sb |= c;
                  MPFR_COV_SET (div_ui_sb[3][!!sb]);
                }
            }
        }
      else
        {
          /* round bit is in tmp[0] */
          rb = tmp[0] & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (tmp[0] & MPFR_LIMB_MASK(sh - 1)) | c;
          MPFR_COV_SET (div_ui_sb[4+!!rb][!!sb]);
        }
    }
  else  /* tmp[yn] != 0 */
    {
      int shlz;
      mp_limb_t w;

      MPFR_ASSERTD (tmp[yn] != 0);
      count_leading_zeros (shlz, tmp[yn]);

      MPFR_ASSERTD (u >= 2);    /* see special cases at the beginning */
      MPFR_ASSERTD (shlz > 0);  /* since u >= 2 */

      /* shift left to normalize */
      w = tmp[0] << shlz;
      mpn_lshift (yp, tmp + 1, yn, shlz);
      yp[0] |= tmp[0] >> (GMP_NUMB_BITS - shlz);
      /* now {yp, yn} is the approximate quotient, w is the next limb */

      if (sh == 0) /* round bit is upper bit from w */
        {
          rb = w & MPFR_LIMB_HIGHBIT;
          sb |= (w - rb) | c;
          MPFR_COV_SET (div_ui_sb[6+!!rb][!!sb]);
        }
      else
        {
          rb = yp[0] & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (yp[0] & MPFR_LIMB_MASK(sh - 1)) | w | c;
          MPFR_COV_SET (div_ui_sb[8+!!rb][!!sb]);
        }

      exp -= shlz;
    }

  d = yp[0] & MPFR_LIMB_MASK (sh);
  yp[0] ^= d; /* clear the lowest sh bits */

  MPFR_TMP_FREE (marker);

  if (MPFR_UNLIKELY (exp < __gmpfr_emin - 1))
    return mpfr_underflow (y, rnd_mode == MPFR_RNDN ? MPFR_RNDZ : rnd_mode,
                           MPFR_SIGN (y));

  if (MPFR_UNLIKELY (rb == 0 && sb == 0))
    inexact = 0;  /* result is exact */
  else
    {
      int nexttoinf;

      MPFR_UPDATE2_RND_MODE (rnd_mode, MPFR_SIGN (y));
      switch (rnd_mode)
        {
        case MPFR_RNDZ:
        case MPFR_RNDF:
          inexact = - MPFR_INT_SIGN (y);  /* result is inexact */
          nexttoinf = 0;
          break;

        case MPFR_RNDA:
          inexact = MPFR_INT_SIGN (y);
          nexttoinf = 1;
          break;

        default: /* should be MPFR_RNDN */
          MPFR_ASSERTD (rnd_mode == MPFR_RNDN);
          /* We have one more significant bit in yn. */
          if (rb == 0)
            {
              inexact = - MPFR_INT_SIGN (y);
              nexttoinf = 0;
            }
          else if (sb != 0) /* necessarily rb != 0 */
            {
              inexact = MPFR_INT_SIGN (y);
              nexttoinf = 1;
            }
          else /* middle case */
            {
              if (yp[0] & (MPFR_LIMB_ONE << sh))
                {
                  inexact = MPFR_INT_SIGN (y);
                  nexttoinf = 1;
                }
              else
                {
                  inexact = - MPFR_INT_SIGN (y);
                  nexttoinf = 0;
                }
            }
        }
      if (nexttoinf &&
          MPFR_UNLIKELY (mpn_add_1 (yp, yp, yn, MPFR_LIMB_ONE << sh)))
        {
          exp++;
          yp[yn-1] = MPFR_LIMB_HIGHBIT;
        }
    }

  /* Set the exponent. Warning! One may still have an underflow. */
  MPFR_EXP (y) = exp;
#else /* MPFR_LONG_WITHIN_LIMB */
  mpfr_t uu;
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_SAVE_EXPO_MARK (expo);
  mpfr_init2 (uu, sizeof (unsigned long) * CHAR_BIT);
  mpfr_set_ui (uu, u, MPFR_RNDZ);
  inexact = mpfr_div (y, x, uu, rnd_mode);
  mpfr_clear (uu);
  MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);
  MPFR_SAVE_EXPO_FREE (expo);
#endif

  return mpfr_check_range (y, inexact, rnd_mode);
}
