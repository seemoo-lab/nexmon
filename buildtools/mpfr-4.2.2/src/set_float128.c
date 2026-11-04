/* mpfr_set_float128 -- convert a binary128 (a.k.a. float128) number to
                        a multiple precision floating-point number

Copyright 2012-2025 Free Software Foundation, Inc.
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

#ifdef MPFR_WANT_FLOAT128

/* Note: mpfr_get_float128 is a macro defined as the actual binary128 type:
   either _Float128 or __float128. */

#if MPFR_WANT_FLOAT128 == 1
/* _Float128 type from ISO C23 */
# define MPFR_FLOAT128_MAX 0x1.ffffffffffffffffffffffffffffp+16383f128
#elif MPFR_WANT_FLOAT128 == 2
/* __float128 type (GNU C extension) */
# define MPFR_FLOAT128_MAX 0x1.ffffffffffffffffffffffffffffp+16383q
#else
# error "Unsupported value for MPFR_WANT_FLOAT128"
#endif

int
mpfr_set_float128 (mpfr_ptr r, mpfr_float128 d, mpfr_rnd_t rnd_mode)
{
  mpfr_t t;
  mp_limb_t *tp;
  int inexact, shift_exp, neg, e, i;
  mpfr_float128 p[14], q[14];
  MPFR_SAVE_EXPO_DECL (expo);

  /* Check for NaN */
  if (MPFR_UNLIKELY (DOUBLE_ISNAN (d)))
    {
      MPFR_SET_NAN(r);
      MPFR_RET_NAN;
    }

  /* Check for INF */
  if (MPFR_UNLIKELY (d > MPFR_FLOAT128_MAX))
    {
      MPFR_SET_INF (r);
      MPFR_SET_POS (r);
      return 0;
    }
  else if (MPFR_UNLIKELY (d < -MPFR_FLOAT128_MAX))
    {
      MPFR_SET_INF (r);
      MPFR_SET_NEG (r);
      return 0;
    }
  /* Check for ZERO */
  else if (MPFR_UNLIKELY (d == (mpfr_float128) 0.0))
    return mpfr_set_d (r, (double) d, rnd_mode);

  shift_exp = 0; /* invariant: remainder to deal with is d*2^shift_exp */
  neg = d < 0;
  if (d < 0)
    d = -d;

  /* Scaling, avoiding (slow) divisions. Should the tables be cached? */
  if (d >= 1.0)
    {
      p[0] = 2.0;
      q[0] = 0.5;
      e = 1;
      /* p[i] = 2^(2^i), q[i] = 1/p[i] */
      for (i = 0; i < 13 && d >= p[i]; i++)
        {
          p[i+1] = p[i] * p[i];
          q[i+1] = q[i] * q[i];
          e <<= 1;
        }
      for (; i >= 0; e >>= 1, i--)
        if (d >= p[i])
          {
            d *= q[i];
            shift_exp += e;
          }
      d *= 0.5;
      shift_exp++;
    }
  else if (d < 0.5)
    {
      p[0] = 2.0;
      q[0] = 0.5;
      e = 1;
      /* p[i] = 2^(2^i), q[i] = 1/p[i] */
      for (i = 0; i < 13 && d < q[i]; i++)
        {
          p[i+1] = p[i] * p[i];
          q[i+1] = q[i] * q[i];
          e <<= 1;
        }
      /* The while() may be needed for i = 13 due to subnormals.
         This can probably be improved without yielding an underflow. */
      for (; i >= 0; e >>= 1, i--)
        while (d < q[i])
          {
            d *= p[i];
            shift_exp -= e;
          }
    }

  MPFR_ASSERTD (d >= 0.5 && d < 1.0);

  mpfr_init2 (t, IEEE_FLOAT128_MANT_DIG);
  tp = MPFR_MANT (t);

  MPFR_SAVE_EXPO_MARK (expo);
  MPFR_SET_EXP (t, shift_exp);
  MPFR_SET_SIGN (t, neg ? MPFR_SIGN_NEG : MPFR_SIGN_POS);

  for (i = MPFR_LAST_LIMB (t); i >= 0; i--)
    {
      d *= 2 * (mpfr_float128) MPFR_LIMB_HIGHBIT;
      tp[i] = (mp_limb_t) d;
      d -= tp[i];
    }

  inexact = mpfr_set (r, t, rnd_mode);
  mpfr_clear (t);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (r, inexact, rnd_mode);
}

#endif /* MPFR_WANT_FLOAT128 */
