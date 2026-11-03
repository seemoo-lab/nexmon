/* mpfr_beta -- beta function

Copyright 2017-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H /* for MPFR_INT_CEIL_LOG2 */
#include "mpfr-impl.h"

/* use formula (6.2.2) from Abramowitz & Stegun:
   beta(z,w) = gamma(z)*gamma(w)/gamma(z+w) */
int
mpfr_beta (mpfr_ptr r, mpfr_srcptr z, mpfr_srcptr w, mpfr_rnd_t rnd_mode)
{
  mpfr_exp_t emin, emax;
  mpfr_uexp_t pmin;
  mpfr_prec_t prec;
  mpfr_t z_plus_w, tmp, tmp2;
  int inex, w_integer;
  MPFR_GROUP_DECL (group);
  MPFR_ZIV_DECL (loop);
  MPFR_SAVE_EXPO_DECL (expo);

  if (mpfr_less_p (z, w))
    return mpfr_beta (r, w, z, rnd_mode);

  /* Now, either z and w are unordered (at least one is a NaN), or z >= w. */

  if (MPFR_ARE_SINGULAR (z, w))
    {
      /* if z or w is NaN, return NaN */
      if (MPFR_IS_NAN (z) || MPFR_IS_NAN (w))
        {
          MPFR_SET_NAN (r);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF (z) || MPFR_IS_INF (w))
        {
          /* Since we have z >= w:
             if z = +Inf and w > 0, then r = +0 (including w = +Inf);
             if z = +Inf and w = 0, then r = NaN
               [beta(z,1/log(z)) tends to +Inf whereas
                beta(z,1/log(log(z))) tends to +0]
             if z = +Inf and w < 0:
                if w is an integer or -Inf: r = NaN
                if -2k-1 < w < -2k:   r = -Inf
                if -2k-2 < w < -2k-1: r = +Inf
             if w = -Inf and z is finite and not an integer:
                beta(z,t) for t going to -Inf oscillates between positive and
                negative values, with poles around integer values of t, thus
                beta(z,w) gives NaN;
             if w = -Inf and z is an integer:
                beta(z,w) gives +0 for z even > 0, -0 for z odd > 0,
                NaN for z <= 0;
             if z = -Inf (then w = -Inf too): r = NaN */
          if (MPFR_IS_INF (z) && MPFR_IS_POS(z)) /* z = +Inf */
            {
              if (mpfr_cmp_ui (w, 0) > 0)
                {
                  MPFR_SET_ZERO(r);
                  MPFR_SET_POS(r);
                  MPFR_RET(0);
                }
              else if (MPFR_IS_ZERO(w) || MPFR_IS_INF(w) || mpfr_integer_p (w))
                {
                  MPFR_SET_NAN(r);
                  MPFR_RET_NAN;
                }
              else
                {
                  long q;
                  mpfr_t t;

                  MPFR_SAVE_EXPO_MARK (expo);
                  mpfr_init2 (t, MPFR_PREC_MIN);
                  mpfr_set_ui (t, 1, MPFR_RNDN);
                  mpfr_fmodquo (t, &q, w, t, MPFR_RNDD);
                  mpfr_clear (t);
                  MPFR_SAVE_EXPO_FREE (expo);
                  /* q contains the low bits of trunc(w) where trunc() rounds
                     toward zero, thus if q is odd, then -2k-2 < w < -2k-1 */
                  MPFR_SET_INF(r);
                  if ((unsigned long) q & 1)
                    MPFR_SET_NEG(r);
                  else
                    MPFR_SET_POS(r);
                  MPFR_RET(0);
                }
            }
          else if (MPFR_IS_INF(w)) /* w = -Inf */
            {
              if (mpfr_cmp_ui (z, 0) <= 0 || !mpfr_integer_p (z))
                {
                  MPFR_SET_NAN(r);
                  MPFR_RET_NAN;
                }
              else
                {
                  MPFR_SET_ZERO(r);
                  if (mpfr_odd_p (z))
                    MPFR_SET_NEG(r);
                  else
                    MPFR_SET_POS(r);
                  MPFR_RET(0);
                }
            }
        }
      else /* z or w is 0 */
        {
          /* If x is not a non-positive integer, Gamma(x) is regular, so that
             when y -> 0 with either y >= 0 or y <= 0,
               Beta(x,y) ~ Gamma(x) * Gamma(y) / Gamma(x) = Gamma(y)
             Gamma(y) tends to an infinity of the same sign as y.
             Thus Beta(x,y) should be an infinity of the same sign as y.
           */
          if (mpfr_cmp_ui (z, 0) != 0) /* then w is +0 or -0 and z > 0 */
            {
              /* beta(z,+0) = +Inf, beta(z,-0) = -Inf (see above) */
              MPFR_SET_INF(r);
              MPFR_SET_SAME_SIGN(r,w);
              MPFR_SET_DIVBY0 ();
              MPFR_RET(0);
            }
          else if (mpfr_cmp_ui (w, 0) != 0) /* then z is +0 or -0 and w < 0 */
            {
              if (mpfr_integer_p (w))
                {
                  /* For small u > 0, Beta(2u,w+u) and Beta(2u,w-u) have
                     opposite signs, so that they tend to infinities of
                     opposite signs when u -> 0. Thus the result is NaN. */
                  MPFR_SET_NAN(r);
                  MPFR_RET_NAN;
                }
              else
                {
                  /* beta(+0,w) = +Inf, beta(-0,w) = -Inf (see above) */
                  MPFR_SET_INF(r);
                  MPFR_SET_SAME_SIGN(r,z);
                  MPFR_SET_DIVBY0 ();
                  MPFR_RET(0);
                }
            }
          else /* w = z = 0:
                  beta(+0,+0) = +Inf
                  beta(-0,-0) = -Inf
                  beta(+0,-0) = NaN */
            {
              if (MPFR_SIGN(z) == MPFR_SIGN(w))
                {
                  MPFR_SET_INF(r);
                  MPFR_SET_SAME_SIGN(r,z);
                  MPFR_SET_DIVBY0 ();
                  MPFR_RET(0);
                }
              else
                {
                  MPFR_SET_NAN(r);
                  MPFR_RET_NAN;
                }
            }
        }
    }

  /* special case when w is a negative integer */
  w_integer = mpfr_integer_p (w);
  if (w_integer && MPFR_IS_NEG(w))
    {
      /* if z < 0 or z+w > 0, or z is not an integer, return NaN */
      if (MPFR_IS_NEG(z) || mpfr_cmpabs (z, w) > 0 || !mpfr_integer_p (z))
        {
          MPFR_SET_NAN(r);
          MPFR_RET_NAN;
        }
      /* If z+w = 0, the result is 1/z. */
      if (mpfr_cmpabs (z, w) == 0)
        return mpfr_ui_div (r, 1, z, rnd_mode);
      /* Now z is an integer and z+w <= 0: return (-1)^z*beta(z,1-w-z).
         Since z and w are of opposite signs, |z+w| <= max(|z|,|w|). */
      emax = MAX (MPFR_EXP(z), MPFR_EXP(w));
      mpfr_init2 (z_plus_w, (mpfr_prec_t) emax);
      inex = mpfr_add (z_plus_w, z, w, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);
      inex = mpfr_ui_sub (z_plus_w, 1, z_plus_w, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);
      if (mpfr_odd_p (z))
        {
          inex = -mpfr_beta (r, z, z_plus_w, MPFR_INVERT_RND (rnd_mode));
          MPFR_CHANGE_SIGN(r);
        }
      else
        inex = mpfr_beta (r, z, z_plus_w, rnd_mode);
      mpfr_clear (z_plus_w);
      return inex;
    }

  /* special case when z is a negative integer: here w < z and w is not an
     integer */
  if (mpfr_integer_p (z) && MPFR_IS_NEG(z))
    {
      MPFR_SET_NAN(r);
      MPFR_RET_NAN;
    }

  MPFR_SAVE_EXPO_MARK (expo);

  /* compute the smallest precision such that z + w is exact */
  emax = MAX (MPFR_EXP(z), MPFR_EXP(w));
  emin = MIN (MPFR_EXP(z) - MPFR_PREC(z), MPFR_EXP(w) - MPFR_PREC(w));
  MPFR_ASSERTD (emax >= emin);
  /* Thus the math value of emax - emin is representable in mpfr_uexp_t. */
  pmin = (mpfr_uexp_t) emax - emin;
  /* If z and w have same sign, their sum can have exponent emax + 1. */
  pmin += 1;
  if (pmin > MPFR_PREC_MAX) /* FIXME: check if result can differ from NaN. */
    {
      MPFR_SAVE_EXPO_FREE (expo);
      MPFR_SET_NAN(r);
      MPFR_RET_NAN;
    }
  MPFR_ASSERTN (pmin <= MPFR_PREC_MAX);  /* detect integer overflow */
  mpfr_init2 (z_plus_w, (mpfr_prec_t) pmin);
  inex = mpfr_add (z_plus_w, z, w, MPFR_RNDN);
  /* if z+w overflows with rounding to nearest, then w must be larger than
     1/2*ulp(z), thus we have an underflow. */
  if (MPFR_IS_INF(z_plus_w))
    {
      mpfr_clear (z_plus_w);
      MPFR_SAVE_EXPO_FREE (expo);
      return mpfr_underflow (r, rnd_mode, 1);
    }
  MPFR_ASSERTN(inex == 0);

  /* If z+w is 0 or a negative integer, return +0 when w (and thus z) is not
     an integer. Indeed, gamma(z) and gamma(w) are regular numbers, and
     gamma(z+w) is Inf, thus 1/gamma(z+w) is zero. Unless there is a rule
     to choose the sign of 0, we choose +0. */
  if (mpfr_cmp_ui (z_plus_w, 0) <= 0 && !w_integer
      && mpfr_integer_p (z_plus_w))
    {
      mpfr_clear (z_plus_w);
      MPFR_SAVE_EXPO_FREE (expo);
      MPFR_SET_ZERO(r);
      MPFR_SET_POS(r);
      MPFR_RET(0);
    }

  prec = MPFR_PREC(r);
  prec += MPFR_INT_CEIL_LOG2 (prec);
  MPFR_GROUP_INIT_2 (group, prec, tmp, tmp2);
  MPFR_ZIV_INIT (loop, prec);
  for (;;)
    {
      unsigned int inex2;  /* unsigned due to bitwise operations */

      MPFR_GROUP_REPREC_2 (group, prec, tmp, tmp2);
      inex2 = mpfr_gamma (tmp, z, MPFR_RNDN);
      /* tmp = gamma(z) * (1 + theta) with |theta| <= 2^-prec */
      inex2 |= mpfr_gamma (tmp2, w, MPFR_RNDN);
      /* tmp2 = gamma(w) * (1 + theta2) with |theta2| <= 2^-prec */
      inex2 |= mpfr_mul (tmp, tmp, tmp2, MPFR_RNDN);
      /* tmp = gamma(z)*gamma(w) * (1 + theta3)^3 with |theta3| <= 2^-prec */
      inex2 |= mpfr_gamma (tmp2, z_plus_w, MPFR_RNDN);
      /* tmp2 = gamma(z+w) * (1 + theta4) with |theta4| <= 2^-prec */
      inex2 |= mpfr_div (tmp, tmp, tmp2, MPFR_RNDN);
      /* tmp = gamma(z)*gamma(w)/gamma(z+w) * (1 + theta5)^5
         with |theta5| <= 2^-prec. For prec >= 3, we have
         |(1 + theta5)^5 - 1| <= 7 * 2^(-prec), thus the error is bounded
         by 7 ulps */

      if (MPFR_IS_NAN(tmp)) /* FIXME: most probably gamma(z)*gamma(w) = +-Inf,
                               and gamma(z+w) = +-Inf, can we do better? */
        {
          mpfr_clear (z_plus_w);
          MPFR_ZIV_FREE (loop);
          MPFR_GROUP_CLEAR (group);
          MPFR_SAVE_EXPO_FREE (expo);
          MPFR_SET_NAN(r);
          MPFR_RET_NAN;
        }

      MPFR_ASSERTN(mpfr_regular_p (tmp));

      /* if inex2 = 0, then tmp is exactly beta(z,w) */
      if (inex2 == 0 ||
          MPFR_LIKELY (MPFR_CAN_ROUND (tmp, prec - 3, MPFR_PREC(r), rnd_mode)))
        break;

      /* beta(1,+/-2^(-k)) = +/-2^k is exact, and cannot be detected above
         since gamma(+/-2^(-k)) is not exact */
      if (mpfr_cmp_ui (z, 1) == 0)
        {
          mpfr_exp_t expw = mpfr_get_exp (w);
          if (mpfr_cmp_ui_2exp (w, 1, expw - 1) == 0)
            {
              /* since z >= w, this will only match w <= 1 */
              mpfr_set_ui_2exp (tmp, 1, 1 - expw, MPFR_RNDN);
              break;
            }
          else if (mpfr_cmp_si_2exp (w, -1, expw - 1) == 0)
            {
              mpfr_set_si_2exp (tmp, -1, 1 - expw, MPFR_RNDN);
              break;
            }
        }

      /* beta(2^k,1) = 1/2^k for k > 0 (k <= 0 was already tested above) */
      if (mpfr_cmp_ui (w, 1) == 0 &&
          mpfr_cmp_ui_2exp (z, 1, MPFR_EXP(z) - 1) == 0)
        {
          mpfr_set_ui_2exp (tmp, 1, 1 - MPFR_EXP(z), MPFR_RNDN);
          break;
        }

      /* beta(2,-0.5) = -4 */
      if (mpfr_cmp_ui (z, 2) == 0 && mpfr_cmp_si_2exp (w, -1, -1) == 0)
        {
          mpfr_set_si_2exp (tmp, -1, 2, MPFR_RNDN);
          break;
        }

      MPFR_ZIV_NEXT (loop, prec);
    }
  MPFR_ZIV_FREE (loop);
  inex = mpfr_set (r, tmp, rnd_mode);
  MPFR_GROUP_CLEAR (group);
  mpfr_clear (z_plus_w);
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (r, inex, rnd_mode);
}
