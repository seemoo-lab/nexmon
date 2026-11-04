/* mpfr_set_ld -- convert a machine long double to
                  a multiple precision floating-point number

Copyright 2002-2025 Free Software Foundation, Inc.
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

#include <float.h> /* needed so that MPFR_LDBL_MANT_DIG is correctly defined */

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

/* To check for +inf, one can use the test x > LDBL_MAX, as LDBL_MAX is
   the maximum finite number representable in a long double, according
   to DR 467; see
     https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2092.htm
   If this fails on some platform, a test x - x != 0 might be used. */

#if defined(HAVE_LDOUBLE_IS_DOUBLE)

/* the "long double" format is the same as "double" */
int
mpfr_set_ld (mpfr_ptr r, long double d, mpfr_rnd_t rnd_mode)
{
  return mpfr_set_d (r, (double) d, rnd_mode);
}

#elif defined(HAVE_LDOUBLE_IEEE_EXT_LITTLE)

#if GMP_NUMB_BITS >= 64
# define MPFR_LIMBS_PER_LONG_DOUBLE 1
#elif GMP_NUMB_BITS == 32
# define MPFR_LIMBS_PER_LONG_DOUBLE 2
#elif GMP_NUMB_BITS == 16
# define MPFR_LIMBS_PER_LONG_DOUBLE 4
#elif GMP_NUMB_BITS == 8
# define MPFR_LIMBS_PER_LONG_DOUBLE 8
#else
#error "GMP_NUMB_BITS is assumed to be 8, 16, 32 or >= 64"
#endif
/* The hypothetical GMP_NUMB_BITS == 16 is not supported. It will trigger
   an error below. */

/* IEEE Extended Little Endian Code */
int
mpfr_set_ld (mpfr_ptr r, long double d, mpfr_rnd_t rnd_mode)
{
  int inexact, k, cnt;
  mpfr_t tmp;
  mp_limb_t tmpmant[MPFR_LIMBS_PER_LONG_DOUBLE];
  mpfr_long_double_t x;
  mpfr_exp_t exp;
  int signd;
  MPFR_SAVE_EXPO_DECL (expo);

  /* Check for NAN */
  if (MPFR_UNLIKELY (DOUBLE_ISNAN (d)))
    {
      /* we don't propagate the sign bit */
      MPFR_SET_NAN (r);
      MPFR_RET_NAN;
    }
  /* Check for INF */
  else if (MPFR_UNLIKELY (d > LDBL_MAX))
    {
      MPFR_SET_INF (r);
      MPFR_SET_POS (r);
      return 0;
    }
  else if (MPFR_UNLIKELY (d < -LDBL_MAX))
    {
      MPFR_SET_INF (r);
      MPFR_SET_NEG (r);
      return 0;
    }
  /* Check for ZERO */
  else if (MPFR_UNLIKELY (d == 0.0))
    {
      x.ld = d;
      MPFR_SET_ZERO (r);
      if (x.s.sign == 1)
        MPFR_SET_NEG(r);
      else
        MPFR_SET_POS(r);
      return 0;
    }

  /* now d is neither 0, nor NaN nor Inf */
  MPFR_SAVE_EXPO_MARK (expo);

  MPFR_MANT (tmp) = tmpmant;
  MPFR_PREC (tmp) = 64;

  /* Extract sign */
  x.ld = d;
  signd = MPFR_SIGN_POS;
  if (x.ld < 0.0)
    {
      signd = MPFR_SIGN_NEG;
      x.ld = -x.ld;
    }

  /* Extract and normalize the significand */
#if MPFR_LIMBS_PER_LONG_DOUBLE == 1
  tmpmant[0] = ((mp_limb_t) x.s.manh << 32) | ((mp_limb_t) x.s.manl);
  count_leading_zeros (cnt, tmpmant[0]);
  tmpmant[0] <<= cnt;
  k = 0; /* number of limbs shifted */
#else /* MPFR_LIMBS_PER_LONG_DOUBLE >= 2 */
#if MPFR_LIMBS_PER_LONG_DOUBLE == 2
  tmpmant[0] = (mp_limb_t) x.s.manl;
  tmpmant[1] = (mp_limb_t) x.s.manh;
#elif MPFR_LIMBS_PER_LONG_DOUBLE == 4
  tmpmant[0] = (mp_limb_t) x.s.manl;
  tmpmant[1] = (mp_limb_t) (x.s.manl >> 16);
  tmpmant[2] = (mp_limb_t) x.s.manh;
  tmpmant[3] = (mp_limb_t) (x.s.manh >> 16);
#elif MPFR_LIMBS_PER_LONG_DOUBLE == 8
  tmpmant[0] = (mp_limb_t) x.s.manl;
  tmpmant[1] = (mp_limb_t) (x.s.manl >> 8);
  tmpmant[2] = (mp_limb_t) (x.s.manl >> 16);
  tmpmant[3] = (mp_limb_t) (x.s.manl >> 24);
  tmpmant[4] = (mp_limb_t) x.s.manh;
  tmpmant[5] = (mp_limb_t) (x.s.manh >> 8);
  tmpmant[6] = (mp_limb_t) (x.s.manh >> 16);
  tmpmant[7] = (mp_limb_t) (x.s.manh >> 24);
#else
#error "MPFR_LIMBS_PER_LONG_DOUBLE should be 1, 2, 4 or 8"
#endif /* MPFR_LIMBS_PER_LONG_DOUBLE >= 2 */
  {
    int i = MPFR_LIMBS_PER_LONG_DOUBLE;
    MPN_NORMALIZE_NOT_ZERO (tmpmant, i);
    k = MPFR_LIMBS_PER_LONG_DOUBLE - i;
    count_leading_zeros (cnt, tmpmant[i - 1]);
    if (cnt != 0)
      mpn_lshift (tmpmant + k, tmpmant, i, cnt);
    else if (k != 0)
      /* since we copy {tmpmant, i} into {tmpmant + k, i}, we should work
         decreasingly, thus call mpn_copyd */
      mpn_copyd (tmpmant + k, tmpmant, i);
    if (k != 0)
      MPN_ZERO (tmpmant, k);
  }
#endif /* MPFR_LIMBS_PER_LONG_DOUBLE == 1 */

  /* Set exponent */
  exp = (mpfr_exp_t) ((x.s.exph << 8) + x.s.expl);  /* 15-bit unsigned int */
  if (MPFR_UNLIKELY (exp == 0))
    exp -= 0x3FFD;
  else
    exp -= 0x3FFE;

  MPFR_SET_EXP (tmp, exp - cnt - k * GMP_NUMB_BITS);

  /* tmp is exact */
  inexact = mpfr_set4 (r, tmp, rnd_mode, signd);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (r, inexact, rnd_mode);
}

#elif defined(HAVE_LDOUBLE_MAYBE_DOUBLE_DOUBLE)

/* double-double code, see
   https://gcc.gnu.org/git/?p=gcc.git;a=blob;f=libgcc/config/rs6000/ibm-ldouble-format;h=e8ada17f7696cd942e710d5b67d4149f5fcccf45;hb=HEAD */
int
mpfr_set_ld (mpfr_ptr r, long double d, mpfr_rnd_t rnd_mode)
{
  mpfr_t t, u;
  int inexact;
  double h, l;
  MPFR_SAVE_EXPO_DECL (expo);

  /* Check for NAN. Since we can't use isnan(), we rely on the
     LONGDOUBLE_NAN_ACTION macro. The sign bit is not propagated. */
  LONGDOUBLE_NAN_ACTION (d, { MPFR_SET_NAN(r); MPFR_RET_NAN; });

  /* Check for INF */
  if (d > LDBL_MAX)
    {
      mpfr_set_inf (r, 1);
      return 0;
    }
  else if (d < -LDBL_MAX)
    {
      mpfr_set_inf (r, -1);
      return 0;
    }
  /* Check for ZERO */
  else if (d == 0.0)
    return mpfr_set_d (r, (double) d, rnd_mode);

  if (d >= LDBL_MAX || d <= -LDBL_MAX)
    h = (d >= LDBL_MAX) ? LDBL_MAX : -LDBL_MAX;
  else
    h = (double) d; /* should not overflow */
  l = (double) (d - (long double) h);

  MPFR_SAVE_EXPO_MARK (expo);

  mpfr_init2 (t, IEEE_DBL_MANT_DIG);
  mpfr_init2 (u, IEEE_DBL_MANT_DIG);

  inexact = mpfr_set_d (t, h, MPFR_RNDN);
  MPFR_ASSERTN(inexact == 0);
  inexact = mpfr_set_d (u, l, MPFR_RNDN);
  MPFR_ASSERTN(inexact == 0);
  inexact = mpfr_add (r, t, u, rnd_mode);

  mpfr_clear (t);
  mpfr_clear (u);

  MPFR_SAVE_EXPO_FREE (expo);
  inexact = mpfr_check_range (r, inexact, rnd_mode);
  return inexact;
}

#else

/* Generic code */
int
mpfr_set_ld (mpfr_ptr r, long double d, mpfr_rnd_t rnd_mode)
{
  mpfr_t t, u;
  int inexact, shift_exp;
  long double x;
  MPFR_SAVE_EXPO_DECL (expo);

  /* Check for NAN. Since we can't use isnan(), we rely on the
     LONGDOUBLE_NAN_ACTION macro. The sign bit is not propagated. */
  LONGDOUBLE_NAN_ACTION (d, { MPFR_SET_NAN(r); MPFR_RET_NAN; });

  /* Check for INF */
  if (d > LDBL_MAX)
    {
      mpfr_set_inf (r, 1);
      return 0;
    }
  else if (d < -LDBL_MAX)
    {
      mpfr_set_inf (r, -1);
      return 0;
    }
  /* Check for ZERO */
  else if (d == 0.0)
    return mpfr_set_d (r, (double) d, rnd_mode);

  mpfr_init2 (t, MPFR_LDBL_MANT_DIG);
  mpfr_init2 (u, IEEE_DBL_MANT_DIG);

  MPFR_SAVE_EXPO_MARK (expo);

 convert:
  x = d;
  MPFR_SET_ZERO (t);  /* The sign doesn't matter. */
  shift_exp = 0; /* invariant: remainder to deal with is d*2^shift_exp */
  while (x != (long double) 0.0)
    {
      /* Check overflow of double */
      if (x > (long double) DBL_MAX || (-x) > (long double) DBL_MAX)
        {
          long double div9, div10, div11, div12, div13;

#define TWO_64 18446744073709551616.0 /* 2^64 */
#define TWO_128 (TWO_64 * TWO_64)
#define TWO_256 (TWO_128 * TWO_128)
          div9 = (long double) (double) (TWO_256 * TWO_256); /* 2^(2^9) */
          div10 = div9 * div9;
          div11 = div10 * div10; /* 2^(2^11) */
          div12 = div11 * div11; /* 2^(2^12) */
          div13 = div12 * div12; /* 2^(2^13) */
          if (ABS (x) >= div13)
            {
              x /= div13; /* exact */
              shift_exp += 8192;
              mpfr_div_2si (t, t, 8192, MPFR_RNDZ);
            }
          if (ABS (x) >= div12)
            {
              x /= div12; /* exact */
              shift_exp += 4096;
              mpfr_div_2si (t, t, 4096, MPFR_RNDZ);
            }
          if (ABS (x) >= div11)
            {
              x /= div11; /* exact */
              shift_exp += 2048;
              mpfr_div_2si (t, t, 2048, MPFR_RNDZ);
            }
          if (ABS (x) >= div10)
            {
              x /= div10; /* exact */
              shift_exp += 1024;
              mpfr_div_2si (t, t, 1024, MPFR_RNDZ);
            }
          /* warning: we may have DBL_MAX=2^1024*(1-2^(-53)) < x < 2^1024,
             therefore we have one extra exponent reduction step */
          if (ABS (x) >= div9)
            {
              x /= div9; /* exact */
              shift_exp += 512;
              mpfr_div_2si (t, t, 512, MPFR_RNDZ);
            }
        } /* Check overflow of double */
      else /* no overflow on double */
        {
          long double div9, div10, div11;

          div9 = (long double) (double) 7.4583407312002067432909653e-155;
          /* div9 = 2^(-2^9) */
          div10 = div9  * div9;  /* 2^(-2^10) */
          div11 = div10 * div10; /* 2^(-2^11) if extended precision */
          /* since -DBL_MAX <= x <= DBL_MAX, the cast to double should not
             overflow here */
          if (ABS(x) < div10 &&
              div11 != (long double) 0.0 &&
              div11 / div10 == div10) /* possible underflow */
            {
              long double div12, div13;
              /* After the divisions, any bit of x must be >= div10,
                 hence the possible division by div9. */
              div12 = div11 * div11; /* 2^(-2^12) */
              div13 = div12 * div12; /* 2^(-2^13) */
              if (ABS (x) <= div13)
                {
                  x /= div13; /* exact */
                  shift_exp -= 8192;
                  mpfr_mul_2si (t, t, 8192, MPFR_RNDZ);
                }
              if (ABS (x) <= div12)
                {
                  x /= div12; /* exact */
                  shift_exp -= 4096;
                  mpfr_mul_2si (t, t, 4096, MPFR_RNDZ);
                }
              if (ABS (x) <= div11)
                {
                  x /= div11; /* exact */
                  shift_exp -= 2048;
                  mpfr_mul_2si (t, t, 2048, MPFR_RNDZ);
                }
              if (ABS (x) <= div10)
                {
                  x /= div10; /* exact */
                  shift_exp -= 1024;
                  mpfr_mul_2si (t, t, 1024, MPFR_RNDZ);
                }
              if (ABS(x) <= div9)
                {
                  x /= div9;  /* exact */
                  shift_exp -= 512;
                  mpfr_mul_2si (t, t, 512, MPFR_RNDZ);
                }
            }
          else /* no underflow */
            {
              inexact = mpfr_set_d (u, (double) x, MPFR_RNDZ);
              MPFR_ASSERTD (inexact == 0);
              if (mpfr_add (t, t, u, MPFR_RNDZ) != 0)
                {
                  if (!mpfr_number_p (t))
                    break;
                  /* Inexact. This cannot happen unless the C implementation
                     "lies" on the precision or when long doubles are
                     implemented with FP expansions like double-double on
                     PowerPC. */
                  if (MPFR_PREC (t) != MPFR_PREC (r) + 1)
                    {
                      /* We assume that MPFR_PREC (r) < MPFR_PREC_MAX.
                         The precision MPFR_PREC (r) + 1 allows us to
                         deduce the rounding bit and the sticky bit. */
                      mpfr_set_prec (t, MPFR_PREC (r) + 1);
                      goto convert;
                    }
                  else
                    {
                      mp_limb_t *tp;
                      int rb_mask;

                      /* Since mpfr_add was inexact, the sticky bit is 1. */
                      tp = MPFR_MANT (t);
                      rb_mask = MPFR_LIMB_ONE <<
                        (GMP_NUMB_BITS - 1 -
                         (MPFR_PREC (r) & (GMP_NUMB_BITS - 1)));
                      if (rnd_mode == MPFR_RNDN)
                        rnd_mode = (*tp & rb_mask) ^ MPFR_IS_NEG (t) ?
                          MPFR_RNDU : MPFR_RNDD;
                      *tp |= rb_mask;
                      break;
                    }
                }
              x -= (long double) mpfr_get_d1 (u); /* exact */
            }
        }
    }
  inexact = mpfr_mul_2si (r, t, shift_exp, rnd_mode);
  mpfr_clear (t);
  mpfr_clear (u);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (r, inexact, rnd_mode);
}

#endif
