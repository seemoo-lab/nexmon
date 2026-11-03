/* mpfr_gamma_inc -- incomplete gamma function

Copyright 2016-2025 Free Software Foundation, Inc.
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

/* The incomplete gamma function is defined for x >= 0 and a not a negative
   integer by:

   gamma_inc(a,x) := Gamma(a,x) = int(t^(a-1) * exp(-t), t=x..infinity)

                  = Gamma(a) - gamma(a,x) with:

   gamma(a,x) = int(t^(a-1) * exp(-t), t=0..x).

   The function gamma(a,x) satisfies the Taylor expansions (we use the second
   one in the code below):

   gamma(a,x) = x^a * sum((-x)^k/k!/(a+k), k=0..infinity)

   gamma(a,x) = x^a * exp(-x) * sum(x^k/(a*(a+1)*...*(a+k)), k=0..infinity)
*/

static int
mpfr_gamma_inc_negint (mpfr_ptr y, mpfr_srcptr a, mpfr_srcptr x, mpfr_rnd_t r);

int
mpfr_gamma_inc (mpfr_ptr y, mpfr_srcptr a, mpfr_srcptr x, mpfr_rnd_t rnd)
{
  mpfr_prec_t w;
  mpfr_t s, t, u;
  int inex;
  unsigned long k;
  mpfr_exp_t e0, e1, e2, err;
  MPFR_GROUP_DECL(group);
  MPFR_ZIV_DECL(loop);
  MPFR_SAVE_EXPO_DECL (expo);

  if (MPFR_ARE_SINGULAR (a, x))
    {
      /* if a or x is NaN, return NaN */
      if (MPFR_IS_NAN (a) || MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }

      /* Note: for x < 0, gamma_inc (a, x) is a complex number */

      if (MPFR_IS_INF (a) || MPFR_IS_INF (x))
        {
          if (MPFR_IS_INF (a) && MPFR_IS_INF (x))
            {
              if ((MPFR_IS_POS (a) && MPFR_IS_POS (x)) || MPFR_IS_NEG (x))
                {
                  /* (a) gamma_inc(+Inf,+Inf) = NaN because
                         gamma_inc(x,x) tends to +Inf but
                         gamma_inc(x,x^2) tends to +0.
                     (b) gamma_inc(+/-Inf,-Inf) = NaN, for example
                         gamma_inc (a, -a) is a complex number
                         for a not an integer */
                  MPFR_SET_NAN (y);
                  MPFR_RET_NAN;
                }
              else
                {
                  /* gamma_inc(-Inf,+Inf) = +0 */
                  MPFR_SET_ZERO (y);
                  MPFR_SET_POS (y);
                  MPFR_RET (0);  /* exact */
                }
            }
          else /* only one of a, x is infinite */
            {
              if (MPFR_IS_INF (a))
                {
                  MPFR_ASSERTD (MPFR_IS_INF (a) && MPFR_IS_FP (x));
                  if (MPFR_IS_POS (a))
                    {
                      /* gamma_inc(+Inf, x) = +Inf */
                      MPFR_SET_INF (y);
                      MPFR_SET_POS (y);
                      MPFR_RET (0);  /* exact */
                    }
                  else /* a = -Inf */
                    {
                      /* gamma_inc(-Inf, x) = NaN for x <= 0
                                              +Inf for 0 < x < 1
                                              +0 for 1 <= x */
                      if (mpfr_cmp_ui (x, 0) <= 0)
                        {
                          MPFR_SET_NAN (y);
                          MPFR_RET_NAN;
                        }
                      else if (mpfr_cmp_ui (x, 1) < 0)
                        {
                          MPFR_SET_INF (y);
                          MPFR_SET_POS (y);
                          MPFR_RET (0);  /* exact */
                        }
                      else
                        {
                          MPFR_SET_ZERO (y);
                          MPFR_SET_POS (y);
                          MPFR_RET (0);  /* exact */
                        }
                    }
                }
              else
                {
                  MPFR_ASSERTD (MPFR_IS_FP (a) && MPFR_IS_INF (x));
                  if (MPFR_IS_POS (x))
                    {
                      /* x is +Inf: integral tends to zero */
                      MPFR_SET_ZERO (y);
                      MPFR_SET_POS (y);
                      MPFR_RET (0);  /* exact */
                    }
                  else /* NaN for x < 0 */
                    {
                      MPFR_SET_NAN (y);
                      MPFR_RET_NAN;
                    }
                }
            }
        }

      if (MPFR_IS_ZERO (a) || MPFR_IS_ZERO (x))
        {
          if (MPFR_IS_ZERO (a))
            {
              if (mpfr_cmp_ui (x, 0) < 0)
                {
                  /* gamma_inc(a,x) = NaN for x < 0 */
                  MPFR_SET_NAN (y);
                  MPFR_RET_NAN;
                }
              else if (MPFR_IS_ZERO (x))
                /* gamma_inc(a,0) = gamma(a) */
                return mpfr_gamma (y, a, rnd); /* a=+0->+Inf, a=-0->-Inf */
              else
                {
                  /* gamma_inc (0, x) = int (exp(-t), t=x..infinity) = E1(x) */
                  mpfr_t minus_x;
                  MPFR_TMP_INIT_NEG(minus_x, x);
                  /* mpfr_eint(x) for x < 0 returns -E1(-x) */
                  inex = mpfr_eint (y, minus_x, MPFR_INVERT_RND(rnd));
                  MPFR_CHANGE_SIGN(y);
                  return -inex;
                }
            }
          else /* x = 0: gamma_inc(a,0) = gamma(a) */
            return mpfr_gamma (y, a, rnd);
        }
    }

  /* for x < 0 return NaN */
  if (MPFR_SIGN(x) < 0)
    {
      MPFR_SET_NAN (y);
      MPFR_RET_NAN;
    }

  if (mpfr_integer_p (a) && MPFR_SIGN(a) < 0)
    return mpfr_gamma_inc_negint (y, a, x, rnd);

  MPFR_SAVE_EXPO_MARK (expo);

  w = MPFR_PREC(y) + 13; /* working precision */

  MPFR_GROUP_INIT_2(group, w, s, t);
  mpfr_init2 (u, 2); /* u is special (see below) */
  MPFR_ZIV_INIT (loop, w);
  for (;;)
    {
      mpfr_exp_t expu, precu, exps;
      mpfr_t s_abs;
      mpfr_exp_t decay = 0;
      MPFR_BLOCK_DECL (flags);

      /* Note: in the error analysis below, theta represents any value of
         absolute value less than 2^(-w) where w is the working precision (two
         instances of theta may represent different values), cf Higham's book.
      */

      /* to ensure that u = a + k is exact, we have three cases:
         (1) EXP(a) <= 0, then we need PREC(u) >= 1 - EXP(a) + PREC(a)
         (2) EXP(a) - PREC(a) <= 0 < E(a), then PREC(u) >= PREC(a)
         (3) 0 < EXP(a) - PREC(a), then PREC(u) >= EXP(a) */
      precu = MPFR_GET_EXP(a) <= 0 ?
        MPFR_ADD_PREC (MPFR_PREC(a), 1 - MPFR_EXP(a))
        : (MPFR_EXP(a) <= MPFR_PREC(a)) ? MPFR_PREC(a) : MPFR_EXP(a);
      MPFR_ASSERTD (precu + 1 <= MPFR_PREC_MAX);
      mpfr_set_prec (u, precu + 1);
      expu = (MPFR_EXP(a) > 0) ? MPFR_EXP(a) : 1;

      /* estimate Taylor series */
      mpfr_ui_div (t, 1, a, MPFR_RNDA); /* t = 1/a * (1 + theta) */
      mpfr_set (s, t, MPFR_RNDA);       /* s = 1/a * (1 + theta) */
      if (MPFR_IS_NEG(a))
        {
          mpfr_init2 (s_abs, 32);
          mpfr_abs (s_abs, s, MPFR_RNDU);
        }
      for (k = 1;; k++)
        {
          mpfr_mul (t, t, x, MPFR_RNDU); /* t = x^k/(a * ... * (a+k-1))
                                          * (1 + theta)^(2k) */
          inex = mpfr_add_ui (u, a, k, MPFR_RNDZ); /* u = a+k exactly */
          MPFR_ASSERTD(inex == 0);
          mpfr_div (t, t, u, MPFR_RNDA); /* t = x^k/(a * ... * (a+k))
                                          * (1 + theta)^(2k+1) */
          mpfr_add (s, s, t, MPFR_RNDZ);
          /* when s is zero, we consider ulp(s) = ulp(t) */
          exps = (MPFR_IS_ZERO(s)) ? MPFR_GET_EXP(t) : MPFR_GET_EXP(s);
          if (MPFR_IS_NEG(a))
            {
              if (MPFR_IS_POS(t))
                mpfr_add (s_abs, s_abs, t, MPFR_RNDU);
              else
                mpfr_sub (s_abs, s_abs, t, MPFR_RNDU);
            }
          /* we stop when |t| < ulp(s), u > 0 and |x/u| < 1/2, which ensures
             that the tail is at most 2*ulp(s) */
          MPFR_ASSERTD (MPFR_NOTZERO(t));
          if (MPFR_GET_EXP(t) + w <= exps && MPFR_IS_POS(u) &&
              MPFR_GET_EXP(x) + 1 < MPFR_GET_EXP(u))
            break;

          /* if there was an exponent shift in u, increase the precision of
             u so that mpfr_add_ui (u, a, k) remains exact */
          if (MPFR_EXP(u) > expu) /* exponent shift in u */
            {
              MPFR_ASSERTD(MPFR_EXP(u) == expu + 1);
              expu = MPFR_EXP(u);
              mpfr_set_prec (u, mpfr_get_prec (u) + 1);
            }
        }
      if (MPFR_IS_NEG(a))
        {
          decay = MPFR_GET_EXP(s_abs) - MPFR_GET_EXP(s);
          mpfr_clear (s_abs);
        }
      /* For a > 0, since all terms are positive, we have
         s = S * (1 + theta)^(2k+3) with S being the infinite Taylor series.
         For a < 0, the error is bounded by that on the sum s_abs of absolute
         values of the terms, i.e., S_abs * [(1 + theta)^(2k+3) - 1]. Thus we
         can simply use the same error analysis as for a > 0, adding an error
         corresponding to the decay of exponent between s_abs and s. */

      /* multiply by exp(-x) */
      mpfr_exp (t, x, MPFR_RNDZ);    /* t = exp(x) * (1+theta) */
      mpfr_div (s, s, t, MPFR_RNDZ); /* s = <exact value> * (1+theta)^(2k+5) */

      /* multiply by x^a */
      mpfr_pow (t, x, a, MPFR_RNDZ); /* t = x^a * (1+theta) */
      mpfr_mul (s, s, t, MPFR_RNDZ); /* s = Gamma(a,x) * (1+theta)^(2k+7) */

      /* Since |theta| < 2^(-w) using the Taylor expansion of log(1+x)
         we have log(1+theta) = theta1 with |theta1| < 1.16*2^(-w) for w >= 2,
         thus (1+theta)^(2k+7) = exp((2k+7)*theta1).
         Assuming 2k+7 = t*2^w for |t| < 0.5, we have
         |(2k+7)*theta1| = |t*2^w*theta1| < 0.58.
         For |u| < 0.58 we have |exp(u)-1| < 1.36*|u|
         thus |(1+theta)^(2k+7) - 1| < 1.36*0.58*(2k+7)/2^w < 0.79*(2k+7)/2^w.
         Since one ulp is at worst a relative error of 2^(1-w),
         the error on s is at most 2^(decay+1)*(2k+7) ulps. */

      /* subtract from gamma(a) */
      MPFR_BLOCK (flags, mpfr_gamma (t, a, MPFR_RNDZ));
      MPFR_ASSERTN (!MPFR_OVERFLOW (flags));  /* FIXME: support overflow */
      /* t = gamma(a) * (1+theta) */
      e0 = MPFR_GET_EXP (t);
      e1 = (MPFR_IS_ZERO(s)) ? __gmpfr_emin : MPFR_GET_EXP (s);
      mpfr_sub (s, t, s, MPFR_RNDZ);
      /* if s is zero, we can assume ulp(s) = ulp(t), but anyway we won't
         be able to round */
      e2 = (MPFR_IS_ZERO(s)) ? e0 : MPFR_GET_EXP (s);
      /* the final error is at most 1 ulp (for the final subtraction)
         + 2^(e0-e2) ulps # for the error in t
         + 2^(decay+1)*(2k+7) ulps * 2^(e1-e2) # for the error in gamma(a,x) */

      e1 += decay + 1 + MPFR_INT_CEIL_LOG2 (2*k+7);
      /* Now the error is <= 1 + 2^(e0-e2) + 2^(e1-e2).
         Since the formula is symmetric in e0 and e1, we can assume without
         loss of generality e0 >= e1, then:
         if e0 = e1: err <= 1 + 2*2^(e0-e2) <= 2^(e0-e2+2)
         if e0 > e1: err <= 1 + 1.5*2^(e0-e2)
                         <= 2^(e0-e2+1) if e0 > e2
                         <= 2^2 otherwise */
      if (e0 == e1)
        {
          /* Check that e0 - e2 + 2 <= MPFR_EXP_MAX */
          MPFR_ASSERTD (e2 >= 2 || e0 <= (MPFR_EXP_MAX - 2) + e2);
          /* Check that e0 - e2 + 2 >= MPFR_EXP_MIN */
          MPFR_ASSERTD (e2 <= 2 || e0 >= MPFR_EXP_MIN + (e2 - 2));
          err = e0 - e2 + 2;
        }
      else
        {
          e0 = (e0 > e1) ? e0 : e1; /* max(e0,e1) */
          MPFR_ASSERTD (e0 <= e2 || e2 >= 1 || e0 <= (MPFR_EXP_MAX - 1) + e2);
          err = (e0 > e2) ? e0 - e2 + 1 : 2;
        }

      if (MPFR_LIKELY (MPFR_CAN_ROUND (s, w - err, MPFR_PREC(y), rnd)))
        break;

      MPFR_ZIV_NEXT (loop, w);
      MPFR_GROUP_REPREC_2(group, w, s, t);
    }
  MPFR_ZIV_FREE (loop);
  mpfr_clear (u);

  inex = mpfr_set (y, s, rnd);
  MPFR_GROUP_CLEAR(group);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inex, rnd);
}

/* For a negative integer, we have (formula 6.5.19):

   gamma(-n,x) = (-1)^n/n! [E_1(x) - exp(-x) sum((-1)^j*j!/x^(j+1), j=0..n-1)]

   See also https://arxiv.org/pdf/1407.0349v1.pdf.

   Assumes 'a' is a negative integer.
*/
static int
mpfr_gamma_inc_negint (mpfr_ptr y, mpfr_srcptr a, mpfr_srcptr x,
                       mpfr_rnd_t rnd)
{
  mpfr_t s, t, abs_a, neg_x;
  unsigned long j;
  mpfr_prec_t w;
  int inex;
  mpfr_exp_t exp_s, new_exp_s, exp_t, err_s, logj;
  MPFR_GROUP_DECL(group);
  MPFR_ZIV_DECL(loop);
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_ASSERTD(mpfr_integer_p (a));
  MPFR_ASSERTD(mpfr_cmp_ui (a, 0) < 0);

  MPFR_TMP_INIT_ABS(abs_a, a);

  /* below, theta represents any value such that |theta| <= 2^(-w) */

  w = MPFR_PREC(y) + 10; /* initial working precision */

  MPFR_SAVE_EXPO_MARK (expo);
  MPFR_GROUP_INIT_2(group, w, s, t);
  MPFR_ZIV_INIT (loop, w);
  for (;;)
    {
      /* we require |a| <= 2^(w-3) for the error analysis below */
      if (MPFR_GET_EXP(a) + 3 > w)
        w = MPFR_GET_EXP(a) + 3;

      mpfr_ui_div (t, 1, x, MPFR_RNDN); /* t = 1/x * (1 + theta) */
      mpfr_set (s, t, MPFR_RNDN);
      MPFR_ASSERTD (MPFR_NOTZERO(s));
      exp_t = exp_s = MPFR_GET_EXP(s); /* max. exponent of s/t during loop */
      new_exp_s = exp_s;

      for (j = 1; mpfr_cmp_ui (abs_a, j) > 0; j++)
        {
          /* invariant: t = (-1)^(j-1)*(j-1)!/x^j * (1 + theta)^(2j-1) */
          mpfr_mul_ui (t, t, j, MPFR_RNDN);
          mpfr_neg (t, t, MPFR_RNDN); /* exact */
          mpfr_div (t, t, x, MPFR_RNDN);
          /* now t = (-1)^j*j!/x^(j+1) * (1 + theta)^(2j+1).
             We have (1 + theta)^(2j+1) = exp((2j+1)*log(1+theta)).
             For |u| <= 1/2, we have |log(1+u)| <= 1.4 |u| thus:
             |(1+theta)^(2j+1)-1| <= max |exp(1.4*(2j+1)*u)-1| for |u|<=2^(-w).
             Now for |v| <= 1/2 we have |exp(v)-1| <= 0.7*|v| thus:
             |(1+theta)^(2j+1) - 1| <= 2*(2j+1)*2^(-w)
             as long as 1.4*(2j+1)*2^(-w) <= 1/2, which is true when j<2^(w-3).
             Since j < |a| it suffices that |a| <= 2^(w-3).
             In that case the rel. error on t is bounded by 2*(2j+1)*2^(-w),
             thus the error in ulps is bounded by 2*(2j+1) ulp(t). */
          if (MPFR_IS_ZERO(t)) /* underflow on t */
            break;
          if (MPFR_GET_EXP(t) > exp_t)
            exp_t = MPFR_GET_EXP(t);
          mpfr_add (s, s, t, MPFR_RNDN);
          /* if s is zero, we can assume its ulp is that of t */
          new_exp_s = (MPFR_IS_ZERO(s)) ? MPFR_GET_EXP(t) : MPFR_GET_EXP(s);
          if (new_exp_s > exp_s)
            exp_s = new_exp_s;
        }

      /* the error on s is bounded by (j-1) * 2^(exp_s - EXP(s)) * 1/2
         for the mpfr_add roundings, plus
         sum(2*(2i+1), i=1..j-1) * 2^(exp_t - EXP(s)) for the error on t.
         The latter sum is (2*j^2-2) * 2^(exp_t - EXP(s)). */

      logj = MPFR_INT_CEIL_LOG2(j);
      exp_s += logj - 1;
      exp_t += 1 + 2 * logj;

      /* now the error on s is bounded by 2^(exp_s-EXP(s))+2^(exp_t-EXP(s)) */

      exp_s = (exp_s >= exp_t) ? exp_s + 1 : exp_t + 1;
      err_s = exp_s - new_exp_s;

      /* now the error on the sum S := sum((-1)^j*j!/x^(j+1), j=0..n-1)
         is bounded by 2^err_s ulp(s) */

      MPFR_TMP_INIT_NEG(neg_x, x);

      mpfr_exp (t, neg_x, MPFR_RNDN); /* t = exp(-x) * (1 + theta) */
      mpfr_mul (s, s, t, MPFR_RNDN);
      if (MPFR_IS_ZERO(s))
        {
          MPFR_ASSERTD (MPFR_NOTZERO(t));
          new_exp_s += MPFR_GET_EXP(t);
        }
      /* s = exp(-x) * (S +/- 2^err_s ulp(S)) * (1 + theta)^2.
         = exp(-x) * (S +/- 2^err_s ulp(S)) * (1 +/- 3 ulp(1))
         The error on s is bounded by:
         exp(-x) * [2^err_s*ulp(S) + S*3*ulp(1) + 2^err_s*ulp(S)*3*ulp(1)]
         <= ulp(s) * [2^(err_s+1) + 6 + 1]
         <= ulp(s) * 2^(err_s+2) as long as err_s >= 2. */

      err_s = (err_s >= 2) ? err_s + 2 : 4;
      /* now the error on s is bounded by 2^err_s ulp(s) */

      mpfr_eint (t, neg_x, MPFR_RNDN); /* t = -E1(-x) * (1 + theta) */
      mpfr_neg (t, t, MPFR_RNDN); /* exact */

      exp_s = (MPFR_IS_ZERO(s)) ? new_exp_s : MPFR_GET_EXP(s);
      MPFR_ASSERTD (MPFR_NOTZERO(t));
      exp_t = MPFR_GET_EXP(t);
      mpfr_sub (s, t, s, MPFR_RNDN); /* E_1(x) - exp(-x) * S */
      if (MPFR_IS_ZERO(s)) /* cancellation: increase working precision */
        goto next_w;

      /* err(s) <= 1/2 * ulp(s) [mpfr_sub]
         + 2^err_s * 2^(exp_s-EXP(s)) * ulp(s) [previous error on s]
         + 1/2 * 2^(exp_t-EXP(s)) * ulp(s) [error on t] */

      exp_s += err_s;
      exp_t -= 1;
      exp_s = (exp_s >= exp_t) ? exp_s + 1 : exp_t + 1;
      MPFR_ASSERTD (MPFR_NOTZERO(s));
      err_s = exp_s - MPFR_GET_EXP(s);
      /* err(s) <= 1/2 * ulp(s) + 2^err_s * ulp(s) */

      /* divide by n! */
      mpfr_gamma (t, abs_a, MPFR_RNDN); /* t = (n-1)! * (1 + theta) */
      mpfr_mul (t, t, abs_a, MPFR_RNDN); /* t = n! * (1 + theta)^2 */
      mpfr_div (s, s, t, MPFR_RNDN);
      /* since (1 + theta)^2 converts to an error of at most 3 ulps
         for w >= 2, the final error is at most:
         2 * (1/2 + 2^err_s) * ulp(s) [error on previous s]
         + 2 * 3 * ulp(s)           [error on t]
         + 1 * ulp(s)                 [product of errors]
         = ulp(s) * (2^(err_s+1) + 8) */
      err_s = (err_s >= 2) ? err_s + 1 : 4;

      /* the final error is bounded by 2^err_s * ulp(s) */

      /* Is there a better way to compute (-1)^n? */
      mpfr_set_si (t, -1, MPFR_RNDN);
      mpfr_pow (t, t, abs_a, MPFR_RNDN);
      if (MPFR_IS_NEG(t))
        mpfr_neg (s, s, MPFR_RNDN);

      if (MPFR_LIKELY (MPFR_CAN_ROUND (s, w - err_s, MPFR_PREC(y), rnd)))
        break;

    next_w:
      MPFR_ZIV_NEXT (loop, w);
      MPFR_GROUP_REPREC_2(group, w, s, t);
    }
  MPFR_ZIV_FREE (loop);

  inex = mpfr_set (y, s, rnd);
  MPFR_GROUP_CLEAR(group);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inex, rnd);
}
