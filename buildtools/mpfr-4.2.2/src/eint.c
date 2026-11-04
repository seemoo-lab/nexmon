/* mpfr_eint, mpfr_eint1 -- the exponential integral

Copyright 2005-2025 Free Software Foundation, Inc.
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

/* eint1(x) = -gamma - log(x) - sum((-1)^k*z^k/k/k!, k=1..infinity) for x > 0
            = - eint(-x) for x < 0
   where
   eint (x) = gamma + log(x) + sum(z^k/k/k!, k=1..infinity) for x > 0
   eint (x) is undefined for x < 0.
*/

/* Compute in y an approximation of sum(x^k/k/k!, k=1..infinity),
   assuming x != 0, and return e such that the absolute error is
   bounded by 2^e ulp(y).
   Return PREC(y) when the truncated series does not converge.
*/
static mpfr_exp_t
mpfr_eint_aux (mpfr_ptr y, mpfr_srcptr x)
{
  mpfr_t eps; /* dynamic (absolute) error bound on t */
  mpfr_t erru, errs;
  mpz_t m, s, t, u;
  mpfr_exp_t e, sizeinbase;
  mpfr_prec_t w = MPFR_PREC(y);
  unsigned long k;
  MPFR_GROUP_DECL (group);

  MPFR_LOG_FUNC (
    ("x[%Pd]=%.*Rg", mpfr_get_prec (x), mpfr_log_prec, x),
    ("y[%Pd]=%.*Rg", mpfr_get_prec (y), mpfr_log_prec, y));

  /* for |x| <= 1, we have S := sum(x^k/k/k!, k=1..infinity) = x + R(x)
     where |R(x)| <= (x/2)^2/(1-|x|/2) <= 2*(x/2)^2
     thus |R(x)/x| <= |x|/2
     thus if |x| <= 2^(-PREC(y)) we have |S - o(x)| <= ulp(y) */

  if (MPFR_GET_EXP(x) <= - (mpfr_exp_t) w)
    {
      mpfr_set (y, x, MPFR_RNDN);
      return 0;
    }

  mpz_init (s); /* initializes to 0 */
  mpz_init (t);
  mpz_init (u);
  mpz_init (m);
  MPFR_GROUP_INIT_3 (group, 31, eps, erru, errs);
  e = mpfr_get_z_2exp (m, x);  /* x = m * 2^e with m != 0 */
  MPFR_LOG_MSG (("e=%" MPFR_EXP_FSPEC "d\n", (mpfr_eexp_t) e));
  MPFR_ASSERTD (mpz_sizeinbase (m, 2) == MPFR_PREC (x));  /* since m != 0 */
  if (MPFR_PREC (x) > w)
    {
      e += MPFR_PREC (x) - w;
      mpz_tdiv_q_2exp (m, m, MPFR_PREC (x) - w);  /* one still has m != 0 */
      MPFR_LOG_MSG (("e=%" MPFR_EXP_FSPEC "d\n", (mpfr_eexp_t) e));
    }
  /* Remove trailing zeroes from m: this will speed up much cases where
     x is a small integer divided by a power of 2.
     Note: As shown above, m != 0. This is needed for the "e += ..." below,
     otherwise n would take the largest value of mp_bitcnt_t and could be
     too large. */
  {
    mp_bitcnt_t n = mpz_scan1 (m, 0);
    mpz_tdiv_q_2exp (m, m, n);
    /* Since one initially has mpz_sizeinbase (m, 2) == MPFR_PREC (x)
       and m has not increased, one can deduce that n <= MPFR_PREC (x),
       so that the cast to mpfr_prec_t is valid. This cast is needed to
       ensure that the operand e of the addition below is not converted
       to an unsigned integer type, which could yield incorrect results
       with some C implementations. */
    MPFR_ASSERTD (n <= MPFR_PREC (x));
    e += (mpfr_prec_t) n;
  }
  /* initialize t to 2^w */
  mpz_set_ui (t, 1);
  mpz_mul_2exp (t, t, w);
  mpfr_set_ui (eps, 0, MPFR_RNDN); /* eps[0] = 0 */
  mpfr_set_ui (errs, 0, MPFR_RNDN); /* maximal error on s */
  for (k = 1;; k++)
    {
      /* let t[k] = x^k/k/k!, and eps[k] be the absolute error on t[k]:
         since t[k] = trunc(t[k-1]*m*2^e/k), we have
         eps[k+1] <= 1 + eps[k-1]*|m|*2^e/k + |t[k-1]|*|m|*2^(1-w)*2^e/k
                  =  1 + (eps[k-1] + |t[k-1]|*2^(1-w))*|m|*2^e/k
                  = 1 + (eps[k-1]*2^(w-1) + |t[k-1]|)*2^(1-w)*|m|*2^e/k */
      mpfr_mul_2ui (eps, eps, w - 1, MPFR_RNDU);
      if (mpz_sgn (t) >= 0)
        mpfr_add_z (eps, eps, t, MPFR_RNDU);
      else
        mpfr_sub_z (eps, eps, t, MPFR_RNDU);
      MPFR_MPZ_SIZEINBASE2 (sizeinbase, m);
      mpfr_mul_2si (eps, eps, sizeinbase - (w - 1) + e, MPFR_RNDU);
      mpfr_div_ui (eps, eps, k, MPFR_RNDU);
      mpfr_add_ui (eps, eps, 1, MPFR_RNDU);
      mpz_mul (t, t, m);
      if (e < 0)
        mpz_tdiv_q_2exp (t, t, -e);
      else
        mpz_mul_2exp (t, t, e);
      mpz_tdiv_q_ui (t, t, k);
      mpz_tdiv_q_ui (u, t, k);
      mpz_add (s, s, u);
      /* the absolute error on u is <= 1 + eps[k]/k */
      mpfr_div_ui (erru, eps, k, MPFR_RNDU);
      mpfr_add_ui (erru, erru, 1, MPFR_RNDU);
      /* and that on s is the sum of all errors on u */
      mpfr_add (errs, errs, erru, MPFR_RNDU);
      /* we are done when t is smaller than errs */
      if (mpz_sgn (t) == 0)
        sizeinbase = 0;
      else
        MPFR_MPZ_SIZEINBASE2 (sizeinbase, t);
      if (sizeinbase < MPFR_GET_EXP (errs))
        break;
    }
  /* the truncation error is bounded by (|t|+eps)/k*(|x|/k + |x|^2/k^2 + ...)
     <= (|t|+eps)/k*|x|/(k-|x|) */
  mpz_abs (t, t);
  mpfr_add_z (eps, eps, t, MPFR_RNDU);
  mpfr_div_ui (eps, eps, k, MPFR_RNDU);
  mpfr_abs (erru, x, MPFR_RNDU); /* |x| */
  mpfr_mul (eps, eps, erru, MPFR_RNDU);
  mpfr_ui_sub (erru, k, erru, MPFR_RNDD);
  if (MPFR_IS_NEG (erru))
    {
      /* the truncated series does not converge, return fail */
      e = w;
    }
  else
    {
      mpfr_div (eps, eps, erru, MPFR_RNDU);
      mpfr_add (errs, errs, eps, MPFR_RNDU);
      mpfr_set_z (y, s, MPFR_RNDN);
      mpfr_div_2ui (y, y, w, MPFR_RNDN);
      /* errs was an absolute error bound on s. We must convert it to an error
         in terms of ulp(y). Since ulp(y) = 2^(EXP(y)-PREC(y)), we must
         divide the error by 2^(EXP(y)-PREC(y)), but since we divided also
         y by 2^w = 2^PREC(y), we must simply divide by 2^EXP(y). */
      e = MPFR_GET_EXP (errs) - MPFR_GET_EXP (y);
    }
  MPFR_GROUP_CLEAR (group);
  mpz_clear (s);
  mpz_clear (t);
  mpz_clear (u);
  mpz_clear (m);
  MPFR_LOG_MSG (("e=%" MPFR_EXP_FSPEC "d\n", (mpfr_eexp_t) e));
  return e;
}

/* Return in y an approximation of Ei(x) using the asymptotic expansion:
   Ei(x) = exp(x)/x * (1 + 1/x + 2/x^2 + ... + k!/x^k + ...)
   Assumes |x| >= PREC(y) * log(2).
   Returns the error bound in terms of ulp(y).
*/
static mpfr_exp_t
mpfr_eint_asympt (mpfr_ptr y, mpfr_srcptr x)
{
  mpfr_prec_t p = MPFR_PREC(y);
  mpfr_t invx, t, err;
  unsigned long k;
  mpfr_exp_t err_exp;

  MPFR_LOG_FUNC (
    ("x[%Pd]=%.*Rg", mpfr_get_prec (x), mpfr_log_prec, x),
    ("err_exp=%" MPFR_EXP_FSPEC "d", (mpfr_eexp_t) err_exp));

  mpfr_init2 (t, p);
  mpfr_init2 (invx, p);
  mpfr_init2 (err, 31); /* error in ulps on y */
  mpfr_ui_div (invx, 1, x, MPFR_RNDN); /* invx = 1/x*(1+u) with |u|<=2^(1-p) */
  mpfr_set_ui (t, 1, MPFR_RNDN); /* exact */
  mpfr_set (y, t, MPFR_RNDN);
  mpfr_set_ui (err, 0, MPFR_RNDN);
  for (k = 1; MPFR_GET_EXP(t) + (mpfr_exp_t) p > MPFR_GET_EXP(y); k++)
    {
      mpfr_mul (t, t, invx, MPFR_RNDN); /* 2 more roundings */
      mpfr_mul_ui (t, t, k, MPFR_RNDN); /* 1 more rounding: t = k!/x^k*(1+u)^e
                                          with u=2^{-p} and |e| <= 3*k */
      /* we use the fact that |(1+u)^n-1| <= 2*|n*u| for |n*u| <= 1, thus
         the error on t is less than 6*k*2^{-p}*t <= 6*k*ulp(t) */
      /* err is in terms of ulp(y): transform it in terms of ulp(t) */
      mpfr_mul_2si (err, err, MPFR_GET_EXP(y) - MPFR_GET_EXP(t), MPFR_RNDU);
      mpfr_add_ui (err, err, 6 * k, MPFR_RNDU);
      /* transform back in terms of ulp(y) */
      mpfr_div_2si (err, err, MPFR_GET_EXP(y) - MPFR_GET_EXP(t), MPFR_RNDU);
      mpfr_add (y, y, t, MPFR_RNDN);
    }
  /* add the truncation error bounded by ulp(y): 1 ulp */
  mpfr_mul (y, y, invx, MPFR_RNDN); /* err <= 2*err + 3/2 */
  mpfr_exp (t, x, MPFR_RNDN); /* err(t) <= 1/2*ulp(t) */
  mpfr_mul (y, y, t, MPFR_RNDN); /* again: err <= 2*err + 3/2 */
  mpfr_mul_2ui (err, err, 2, MPFR_RNDU);
  mpfr_add_ui (err, err, 8, MPFR_RNDU);
  err_exp = MPFR_GET_EXP(err);
  mpfr_clear (t);
  mpfr_clear (invx);
  mpfr_clear (err);
  return err_exp;
}

/* mpfr_eint returns Ei(x) for x >= 0,
   and -E1(-x) for x < 0, following https://dlmf.nist.gov/6.2 */
int
mpfr_eint (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd)
{
  int inex;
  mpfr_t tmp, ump, x_abs;
  mpfr_exp_t err, te;
  mpfr_prec_t prec;
  MPFR_SAVE_EXPO_DECL (expo);
  MPFR_ZIV_DECL (loop);

  MPFR_LOG_FUNC (
    ("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (x), mpfr_log_prec, x, rnd),
    ("y[%Pd]=%.*Rg inexact=%d", mpfr_get_prec (y), mpfr_log_prec, y, inex));

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF (x))
        {
          /* eint(+inf) = +inf and eint(-inf) = -0 */
          if (MPFR_IS_POS (x))
            {
              MPFR_SET_INF(y);
              MPFR_SET_POS(y);
            }
          else
            {
              MPFR_SET_ZERO(y);
              MPFR_SET_NEG(y);
            }
          MPFR_RET(0);
        }
      else /* eint(+/-0) = -Inf */
        {
          MPFR_SET_INF(y);
          MPFR_SET_NEG(y);
          MPFR_SET_DIVBY0 ();
          MPFR_RET(0);
        }
    }

  MPFR_TMP_INIT_ABS (x_abs, x);

  MPFR_SAVE_EXPO_MARK (expo);

  /* Init stuff */
  prec = MPFR_PREC (y) + 2 * MPFR_INT_CEIL_LOG2 (MPFR_PREC (y)) + 6;
  mpfr_init2 (tmp, 64);
  mpfr_init2 (ump, 64);

  /* Since eint(x) >= exp(x)/x, we have log2(eint(x)) >= (x-log(x))/log(2).
     Let's compute k <= (x-log(x))/log(2) in a low precision. If k >= emax,
     then log2(eint(x)) >= emax, and eint(x) >= 2^emax, i.e. it overflows. */
  if (MPFR_IS_POS(x))
    {
      mpfr_log (tmp, x, MPFR_RNDU);
      mpfr_sub (ump, x, tmp, MPFR_RNDD);
      mpfr_div (ump, ump, __gmpfr_const_log2_RNDU, MPFR_RNDD);
      /* FIXME: We really need a mpfr_cmp_exp_t function. */
      MPFR_ASSERTN (MPFR_EMAX_MAX <= LONG_MAX);
      if (mpfr_cmp_ui (ump, __gmpfr_emax) >= 0)
        {
          mpfr_clear (tmp);
          mpfr_clear (ump);
          MPFR_SAVE_EXPO_FREE (expo);
          return mpfr_overflow (y, rnd, 1);
        }
    }

  /* Since E1(x) <= exp(-x) for x >= 1, we have log2(E1(x)) <= -x/log(2).
     Let's compute k >= -x/log(2) in a low precision. If k < emin
     then log2(E1(x)) <= emin-1, and E1(x) <= 2^(emin-1): it underflows. */
  if (MPFR_IS_NEG(x) && MPFR_GET_EXP(x) >= 1)
    {
      mpfr_div (ump, x, __gmpfr_const_log2_RNDD, MPFR_RNDU);
      MPFR_ASSERTN (MPFR_EMIN_MIN >= LONG_MIN);
      if (mpfr_cmp_si (ump, __gmpfr_emin) < 0)
        {
          mpfr_clear (tmp);
          mpfr_clear (ump);
          MPFR_SAVE_EXPO_FREE (expo);
          return mpfr_underflow (y, rnd, -1);
        }
    }

  /* eint() has a root 0.37250741078136663446...,
     so if x is near, already take more bits */
  if (MPFR_IS_POS(x) && MPFR_GET_EXP(x) == -1) /* 1/4 <= x < 1/2 */
    {
      mpfr_t y;
      mpfr_init2 (y, 32);
      /* 1599907147/2^32 is a 32-bit approximation of 0.37250741078136663446 */
      mpfr_set_ui_2exp (y, 1599907147UL, -32, MPFR_RNDN);
      mpfr_sub (y, x, y, MPFR_RNDN);
      prec += (mpfr_zero_p (y)) ? 32
        : mpfr_get_exp (y) < 0 ? -mpfr_get_exp (y) : 0;
      mpfr_clear (y);
    }

  mpfr_set_prec (tmp, prec);
  mpfr_set_prec (ump, prec);

  MPFR_ZIV_INIT (loop, prec);           /* Initialize the ZivLoop controller */
  for (;;)                              /* Infinite loop */
    {
      /* For the asymptotic expansion to work, we need that the smallest
         value of k!/|x|^k is smaller than 2^(-p). The minimum is obtained for
         x=k, and it is smaller than e*sqrt(x)/e^x for x>=1. */
      if (MPFR_GET_EXP (x) > 0 &&
          mpfr_cmp_d (x_abs, ((double) prec +
                            0.5 * (double) MPFR_GET_EXP (x)) * LOG2 + 1.0) > 0)
        err = mpfr_eint_asympt (tmp, x);
      else
        {
          err = mpfr_eint_aux (tmp, x); /* error <= 2^err ulp(tmp) */
          te = MPFR_GET_EXP(tmp);
          mpfr_const_euler (ump, MPFR_RNDN); /* 0.577 -> EXP(ump)=0 */
          mpfr_add (tmp, tmp, ump, MPFR_RNDN);
          /* If tmp <> 0:
             error <= 1/2 + 1/2*2^(EXP(ump)-EXP(tmp)) + 2^(te-EXP(tmp)+err)
             <= 1/2 + 2^(MAX(EXP(ump), te+err+1) - EXP(tmp))
             <= 2^(MAX(0, 1 + MAX(EXP(ump), te+err+1) - EXP(tmp))).
             If tmp = 0 we can use the same bound, replacing
             EXP(tmp) by EXP(ump). */
          err = MAX(1, te + err + 2);
          te = MPFR_IS_ZERO(tmp) ? MPFR_GET_EXP(ump) : MPFR_GET_EXP(tmp);
          err = err - te;
          err = MAX(0, err);
          mpfr_log (ump, x_abs, MPFR_RNDN);
          mpfr_add (tmp, tmp, ump, MPFR_RNDN);
          /* same formula as above, except now EXP(ump) is not 0 */
          err += te + 1;
          if (MPFR_LIKELY (!MPFR_IS_ZERO (ump)))
            err = MAX (MPFR_GET_EXP (ump), err);
          /* if tmp is zero, we surely cannot round correctly */
          err = (MPFR_IS_ZERO(tmp)) ? prec :  MAX(0, err - MPFR_GET_EXP (tmp));
        }
      /* Note: we assume here that MPFR_CAN_ROUND returns the same result
         for rnd and MPFR_INVERT_RND(rnd) */
      if (MPFR_LIKELY (MPFR_CAN_ROUND (tmp, prec - err, MPFR_PREC (y), rnd)))
        break;
      MPFR_ZIV_NEXT (loop, prec);        /* Increase used precision */
      mpfr_set_prec (tmp, prec);
      mpfr_set_prec (ump, prec);
    }
  MPFR_ZIV_FREE (loop);                  /* Free the ZivLoop Controller */

  /* Set y to the computed value */
  inex = mpfr_set (y, tmp, rnd);
  mpfr_clear (tmp);
  mpfr_clear (ump);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inex, rnd);
}
