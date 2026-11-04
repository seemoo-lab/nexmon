/* mpfr_zeta -- compute the Riemann Zeta function

Copyright 2003-2025 Free Software Foundation, Inc.
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

#include <float.h> /* for DBL_MAX */

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

/*
   Parameters:
   s - the input floating-point number
   n, p - parameters from the algorithm
   tc - an array of p floating-point numbers tc[1]..tc[p]
   Output:
   b is the result, i.e.
   sum(tc[i]*product((s+2j)*(s+2j-1)/n^2,j=1..i-1), i=1..p)*s*n^(-s-1)
*/
static void
mpfr_zeta_part_b (mpfr_ptr b, mpfr_srcptr s, int n, int p, mpfr_t *tc)
{
  mpfr_t s1, d, u;
  unsigned long n2;
  int l, t;
  MPFR_GROUP_DECL (group);

  if (p == 0)
    {
      MPFR_SET_ZERO (b);
      MPFR_SET_POS (b);
      return;
    }

  n2 = n * n;
  MPFR_GROUP_INIT_3 (group, MPFR_PREC (b), s1, d, u);

  /* t equals 2p-2, 2p-3, ... ; s1 equals s+t */
  t = 2 * p - 2;
  mpfr_set (d, tc[p], MPFR_RNDN);
  for (l = 1; l < p; l++)
    {
      mpfr_add_ui (s1, s, t, MPFR_RNDN); /* s + (2p-2l) */
      mpfr_mul (d, d, s1, MPFR_RNDN);
      t = t - 1;
      mpfr_add_ui (s1, s, t, MPFR_RNDN); /* s + (2p-2l-1) */
      mpfr_mul (d, d, s1, MPFR_RNDN);
      t = t - 1;
      mpfr_div_ui (d, d, n2, MPFR_RNDN);
      mpfr_add (d, d, tc[p-l], MPFR_RNDN);
      /* since s is positive and the tc[i] have alternate signs,
         the following is unlikely */
      if (MPFR_UNLIKELY (mpfr_cmpabs (d, tc[p-l]) > 0))
        mpfr_set (d, tc[p-l], MPFR_RNDN);
    }
  mpfr_mul (d, d, s, MPFR_RNDN);
  mpfr_add (s1, s, __gmpfr_one, MPFR_RNDN);
  mpfr_neg (s1, s1, MPFR_RNDN);
  mpfr_ui_pow (u, n, s1, MPFR_RNDN);
  mpfr_mul (b, d, u, MPFR_RNDN);

  MPFR_GROUP_CLEAR (group);
}

/* Input: p - an integer
   Output: fills tc[1..p], tc[i] = bernoulli(2i)/(2i)!
   tc[1]=1/12, tc[2]=-1/720, tc[3]=1/30240, ...
   Assumes all the tc[i] have the same precision.

   Uses the recurrence (4.60) from the book "Modern Computer Arithmetic"
   by Brent and Zimmermann for C_k = bernoulli(2k)/(2k)!:
   sum(C_k/(2k+1-2j)!/4^(k-j), j=0..k) = 1/(2k)!/4^k
   If we put together the terms involving C_0 and C_1 we get:
   sum(D_k/(2k+1-2j)!/4^(k-j), j=1..k) = 0
   with D_1 = C_0/4/(2k+1)/(2k)+C_1-1/(2k)/4=(k-1)/(12k+6),
   and D_k = C_k for k >= 2.

   FIXME: we have C_k = (-1)^(k-1) 2/(2pi)^(2k) * zeta(2k),
   see for example formula (4.65) from the above book,
   thus since |zeta(2k)-1| < 2^(1-2k) for k >= 2, we have:
   |C_k - E_k| < E_k * 2^(1-2k) for k >= 2 and E_k := (-1)^(k-1) 2/(2pi)^(2k).
   Then if 2k-1 >= prec we can evaluate E_k instead, which only requires one
   multiplication per term, instead of O(k) small divisions.
*/
static void
mpfr_zeta_c (int p, mpfr_t *tc)
{
  if (p > 0)
    {
      mpfr_t d;
      int k, l;
      mpfr_prec_t prec = MPFR_PREC (tc[1]);

      mpfr_init2 (d, prec);
      mpfr_div_ui (tc[1], __gmpfr_one, 12, MPFR_RNDN);
      for (k = 2; k <= p; k++)
        {
          mpfr_set_ui (d, k-1, MPFR_RNDN);
          mpfr_div_ui (d, d, 12*k+6, MPFR_RNDN);
          for (l=2; l < k; l++)
            {
              mpfr_div_ui (d, d, 4*(2*k-2*l+3)*(2*k-2*l+2), MPFR_RNDN);
              mpfr_add (d, d, tc[l], MPFR_RNDN);
            }
          mpfr_div_ui (tc[k], d, 24, MPFR_RNDN);
          MPFR_CHANGE_SIGN (tc[k]);
        }
      mpfr_clear (d);
    }
}

/* Input: s - a floating-point number
          n - an integer
   Output: sum - a floating-point number approximating sum(1/i^s, i=1..n-1) */
static void
mpfr_zeta_part_a (mpfr_ptr sum, mpfr_srcptr s, int n)
{
  mpfr_t u, s1;
  int i;
  MPFR_GROUP_DECL (group);

  MPFR_GROUP_INIT_2 (group, MPFR_PREC (sum), u, s1);

  mpfr_neg (s1, s, MPFR_RNDN);
  mpfr_ui_pow (u, n, s1, MPFR_RNDN);
  mpfr_div_2ui (u, u, 1, MPFR_RNDN);
  mpfr_set (sum, u, MPFR_RNDN);
  for (i=n-1; i>1; i--)
    {
      mpfr_ui_pow (u, i, s1, MPFR_RNDN);
      mpfr_add (sum, sum, u, MPFR_RNDN);
    }
  mpfr_add (sum, sum, __gmpfr_one, MPFR_RNDN);

  MPFR_GROUP_CLEAR (group);
}

/* Input: s - a floating-point number >= 1/2.
          rnd_mode - a rounding mode.
          Assumes s is neither NaN nor Infinite.
   Output: z - Zeta(s) rounded to the precision of z with direction rnd_mode
*/
static int
mpfr_zeta_pos (mpfr_ptr z, mpfr_srcptr s, mpfr_rnd_t rnd_mode)
{
  mpfr_t b, c, z_pre, f, s1;
  double beta, sd, dnep;
  mpfr_t *tc1;
  mpfr_prec_t precz, precs, d, dint;
  int p, n, l, add;
  int inex;
  MPFR_GROUP_DECL (group);
  MPFR_ZIV_DECL (loop);

  MPFR_ASSERTD (MPFR_IS_POS (s) && MPFR_GET_EXP (s) >= 0);

  precz = MPFR_PREC (z);
  precs = MPFR_PREC (s);

  /* Zeta(x) = 1+1/2^x+1/3^x+1/4^x+1/5^x+O(1/6^x)
     so with 2^(EXP(x)-1) <= x < 2^EXP(x)
     So for x > 2^3, k^x > k^8, so 2/k^x < 2/k^8
     Zeta(x) = 1 + 1/2^x*(1+(2/3)^x+(2/4)^x+...)
             = 1 + 1/2^x*(1+sum((2/k)^x,k=3..infinity))
            <= 1 + 1/2^x*(1+sum((2/k)^8,k=3..infinity))
     And sum((2/k)^8,k=3..infinity) = -257+128*Pi^8/4725 ~= 0.0438035
     So Zeta(x) <= 1 + 1/2^x*2 for x >= 8
     The error is < 2^(-x+1) <= 2^(-2^(EXP(x)-1)+1) */
  if (MPFR_GET_EXP (s) > 3)
    {
      mpfr_exp_t err;
      err = MPFR_GET_EXP (s) - 1;
      if (err > (mpfr_exp_t) (sizeof (mpfr_exp_t)*CHAR_BIT-2))
        err = MPFR_EMAX_MAX;
      else
        err = ((mpfr_exp_t)1) << err;
      err = 1 - (-err+1); /* GET_EXP(one) - (-err+1) = err :) */
      MPFR_FAST_COMPUTE_IF_SMALL_INPUT (z, __gmpfr_one, err, 0, 1,
                                        rnd_mode, {});
    }

  d = precz + MPFR_INT_CEIL_LOG2(precz) + 10;

  /* we want that s1 = s-1 is exact, i.e. we should have PREC(s1) >= EXP(s) */
  dint = (mpfr_uexp_t) MPFR_GET_EXP (s);
  mpfr_init2 (s1, MAX (precs, dint));
  inex = mpfr_sub (s1, s, __gmpfr_one, MPFR_RNDN);
  MPFR_ASSERTD (inex == 0);

  /* case s=1 should have already been handled */
  MPFR_ASSERTD (!MPFR_IS_ZERO (s1));

  MPFR_GROUP_INIT_4 (group, MPFR_PREC_MIN, b, c, z_pre, f);

  MPFR_ZIV_INIT (loop, d);
  for (;;)
    {
      /* Principal loop: we compute, in z_pre,
         an approximation of Zeta(s), that we send to can_round */
      if (MPFR_GET_EXP (s1) <= -(mpfr_exp_t) ((mpfr_prec_t) (d-3)/2))
        /* Branch 1: when s-1 is very small, one
           uses the approximation Zeta(s)=1/(s-1)+gamma,
           where gamma is Euler's constant */
        {
          dint = MAX (d + 3, precs);
          /* branch 1, with internal precision dint */
          MPFR_GROUP_REPREC_4 (group, dint, b, c, z_pre, f);
          mpfr_div (z_pre, __gmpfr_one, s1, MPFR_RNDN);
          mpfr_const_euler (f, MPFR_RNDN);
          mpfr_add (z_pre, z_pre, f, MPFR_RNDN);
        }
      else /* Branch 2 */
        {
          size_t size;

          /* branch 2 */
          /* Computation of parameters n, p and working precision */
          dnep = (double) d * LOG2;
          sd = mpfr_get_d (s, MPFR_RNDN);
          /* beta = dnep + 0.61 + sd * log (6.2832 / sd);
             but a larger value is OK */
#define LOG6dot2832 1.83787940484160805532
          beta = dnep + 0.61 + sd * (LOG6dot2832 - LOG2 *
                                     __gmpfr_floor_log2 (sd));
          if (beta <= 0.0)
            {
              p = 0;
              /* n = 1 + (int) (exp ((dnep - LOG2) / sd)); */
              n = 1 + (int) __gmpfr_ceil_exp2 ((d - 1.0) / sd);
            }
          else
            {
              p = 1 + (int) beta / 2;
              n = 1 + (int) ((sd + 2.0 * (double) p - 1.0) / 6.2832);
            }
          /* add = 4 + floor(1.5 * log(d) / log (2)).
             We should have add >= 10, which is always fulfilled since
             d = precz + 11 >= 12, thus ceil(log2(d)) >= 4 */
          add = 4 + (3 * MPFR_INT_CEIL_LOG2 (d)) / 2;
          MPFR_ASSERTD(add >= 10);
          dint = d + add;
          if (dint < precs)
            dint = precs;

          /* internal precision is dint */

          size = (p + 1) * sizeof(mpfr_t);
          tc1 = (mpfr_t*) mpfr_allocate_func (size);
          for (l=1; l<=p; l++)
            mpfr_init2 (tc1[l], dint);
          MPFR_GROUP_REPREC_4 (group, dint, b, c, z_pre, f);

          /* precision of z is precz */

          /* Computation of the coefficients c_k */
          mpfr_zeta_c (p, tc1);
          /* Computation of the 3 parts of the function Zeta. */
          mpfr_zeta_part_a (z_pre, s, n);
          mpfr_zeta_part_b (b, s, n, p, tc1);
          /* s1 = s-1 is already computed above */
          mpfr_div (c, __gmpfr_one, s1, MPFR_RNDN);
          mpfr_ui_pow (f, n, s1, MPFR_RNDN);
          mpfr_div (c, c, f, MPFR_RNDN);
          mpfr_add (z_pre, z_pre, c, MPFR_RNDN);
          mpfr_add (z_pre, z_pre, b, MPFR_RNDN);
          for (l=1; l<=p; l++)
            mpfr_clear (tc1[l]);
          mpfr_free_func (tc1, size);
          /* End branch 2 */
        }

      if (MPFR_LIKELY (MPFR_CAN_ROUND (z_pre, d-3, precz, rnd_mode)))
        break;
      MPFR_ZIV_NEXT (loop, d);
    }
  MPFR_ZIV_FREE (loop);

  inex = mpfr_set (z, z_pre, rnd_mode);

  MPFR_GROUP_CLEAR (group);
  mpfr_clear (s1);

  return inex;
}

/* TODO: Check the error analysis. The following (undocumented?) one
   does not take into account the replacement of sin(Pi*s/2) by sinpi(s/2)
   in commit fd5d811d81f6d1839d4099cc1bb2cde705981648, which could have
   reduced the error bound since the multiplication by Pi is now exact. */
/* return add = 1 + floor(log(c^3*(13+m1))/log(2))
   where c = (1+eps)*(1+eps*max(8,m1)),
   m1 = 1 + max(1/eps,2*sd)*(1+eps),
   eps = 2^(-precz-14)
   sd = abs(s-1)
*/
static long
compute_add (mpfr_srcptr s, mpfr_prec_t precz)
{
  mpfr_t t, u, m1;
  long add;

  mpfr_inits2 (64, t, u, m1, (mpfr_ptr) 0);
  if (mpfr_cmp_ui (s, 1) >= 0)
    mpfr_sub_ui (t, s, 1, MPFR_RNDU);
  else
    mpfr_ui_sub (t, 1, s, MPFR_RNDU);
  /* now t = sd = abs(s-1), rounded up */
  mpfr_set_ui_2exp (u, 1, - precz - 14, MPFR_RNDU);
  /* u = eps */
  /* since 1/eps = 2^(precz+14), if EXP(sd) >= precz+14, then
     sd >= 1/2*2^(precz+14) thus 2*sd >= 2^(precz+14) >= 1/eps */
  if (mpfr_get_exp (t) >= precz + 14)
    mpfr_mul_2ui (t, t, 1, MPFR_RNDU);
  else
    mpfr_set_ui_2exp (t, 1, precz + 14, MPFR_RNDU);
  /* now t = max(1/eps,2*sd) */
  mpfr_add_ui (u, u, 1, MPFR_RNDU); /* u = 1+eps, rounded up */
  mpfr_mul (t, t, u, MPFR_RNDU); /* t = max(1/eps,2*sd)*(1+eps) */
  mpfr_add_ui (m1, t, 1, MPFR_RNDU);
  if (mpfr_get_exp (m1) <= 3)
    mpfr_set_ui (t, 8, MPFR_RNDU);
  else
    mpfr_set (t, m1, MPFR_RNDU);
  /* now t = max(8,m1) */
  mpfr_div_2ui (t, t, precz + 14, MPFR_RNDU); /* eps*max(8,m1) */
  mpfr_add_ui (t, t, 1, MPFR_RNDU); /* 1+eps*max(8,m1) */
  mpfr_mul (t, t, u, MPFR_RNDU); /* t = c */
  mpfr_add_ui (u, m1, 13, MPFR_RNDU); /* 13+m1 */
  mpfr_mul (u, u, t, MPFR_RNDU); /* c*(13+m1) */
  mpfr_sqr (t, t, MPFR_RNDU); /* c^2 */
  mpfr_mul (u, u, t, MPFR_RNDU); /* c^3*(13+m1) */
  add = mpfr_get_exp (u);
  mpfr_clears (t, u, m1, (mpfr_ptr) 0);
  return add;
}

/* return in z a lower bound (for rnd = RNDD) or upper bound (for rnd = RNDU)
   of |zeta(s)|/2, using:
   log(|zeta(s)|/2) = (s-1)*log(2*Pi) + lngamma(1-s)
   + log(|sin(Pi*s/2)| * zeta(1-s)).
   Assumes s < 1/2 and s1 = 1-s exactly, thus s1 > 1/2.
   y and p are temporary variables.
   At input, p is Pi rounded down.
   The comments in the code are for rnd = RNDD. */
static void
mpfr_reflection_overflow (mpfr_ptr z, mpfr_ptr s1, mpfr_srcptr s, mpfr_ptr y,
                          mpfr_ptr p, mpfr_rnd_t rnd)
{
  mpz_t sint;

  MPFR_ASSERTD (rnd == MPFR_RNDD || rnd == MPFR_RNDU);

  /* Since log is increasing, we want lower bounds on |sin(Pi*s/2)| and
     zeta(1-s). */
  mpz_init (sint);
  mpfr_get_z (sint, s, MPFR_RNDD); /* sint = floor(s) */
  /* We first compute a lower bound of |sin(Pi*s/2)|, which is a periodic
     function of period 2. Thus:
     if 2k < s < 2k+1, then |sin(Pi*s/2)| is increasing;
     if 2k-1 < s < 2k, then |sin(Pi*s/2)| is decreasing.
     These cases are distinguished by testing bit 0 of floor(s) as if
     represented in two's complement (or equivalently, as an unsigned
     integer mod 2):
     0: sint = 0 mod 2, thus 2k < s < 2k+1 and |sin(Pi*s/2)| is increasing;
     1: sint = 1 mod 2, thus 2k-1 < s < 2k and |sin(Pi*s/2)| is decreasing.
     Let's recall that the comments are for rnd = RNDD. */
  if (mpz_tstbit (sint, 0) == 0) /* |sin(Pi*s/2)| is increasing: round down
                                    Pi*s to get a lower bound. */
    {
      mpfr_mul (y, p, s, rnd);
      if (rnd == MPFR_RNDD)
        mpfr_nextabove (p); /* we will need p rounded above afterwards */
    }
  else /* |sin(Pi*s/2)| is decreasing: round up Pi*s to get a lower bound. */
    {
      if (rnd == MPFR_RNDD)
        mpfr_nextabove (p);
      mpfr_mul (y, p, s, MPFR_INVERT_RND(rnd));
    }
  mpfr_div_2ui (y, y, 1, MPFR_RNDN); /* exact, rounding mode doesn't matter */
  /* The rounding direction of sin depends on its sign. We have:
     if -4k-2 < s < -4k, then -2k-1 < s/2 < -2k, thus sin(Pi*s/2) < 0;
     if -4k < s < -4k+2, then -2k < s/2 < -2k+1, thus sin(Pi*s/2) > 0.
     These cases are distinguished by testing bit 1 of floor(s) as if
     represented in two's complement (or equivalently, as an unsigned
     integer mod 4):
     0: sint = {0,1} mod 4, thus -2k < s/2 < -2k+1 and sin(Pi*s/2) > 0;
     1: sint = {2,3} mod 4, thus -2k-1 < s/2 < -2k and sin(Pi*s/2) < 0.
     Let's recall that the comments are for rnd = RNDD. */
  if (mpz_tstbit (sint, 1) == 0) /* -2k < s/2 < -2k+1; sin(Pi*s/2) > 0 */
    {
      /* Round sin down to get a lower bound of |sin(Pi*s/2)|. */
      mpfr_sin (y, y, rnd);
    }
  else /* -2k-1 < s/2 < -2k; sin(Pi*s/2) < 0 */
    {
      /* Round sin up to get a lower bound of |sin(Pi*s/2)|. */
      mpfr_sin (y, y, MPFR_INVERT_RND(rnd));
      mpfr_abs (y, y, MPFR_RNDN); /* exact, rounding mode doesn't matter */
    }
  mpz_clear (sint);
  /* now y <= |sin(Pi*s/2)| when rnd=RNDD, y >= |sin(Pi*s/2)| when rnd=RNDU */
  mpfr_zeta_pos (z, s1, rnd); /* zeta(1-s) */
  mpfr_mul (z, z, y, rnd);
  /* now z <= |sin(Pi*s/2)|*zeta(1-s) */
  mpfr_log (z, z, rnd);
  /* now z <= log(|sin(Pi*s/2)|*zeta(1-s)) */
  mpfr_lngamma (y, s1, rnd);
  mpfr_add (z, z, y, rnd);
  /* z <= lngamma(1-s) + log(|sin(Pi*s/2)|*zeta(1-s)) */
  /* since s-1 < 0, we want to round log(2*pi) upwards */
  mpfr_mul_2ui (y, p, 1, MPFR_INVERT_RND(rnd));
  mpfr_log (y, y, MPFR_INVERT_RND(rnd));
  mpfr_mul (y, y, s1, MPFR_INVERT_RND(rnd));
  mpfr_sub (z, z, y, rnd);
  mpfr_exp (z, z, rnd);
  if (rnd == MPFR_RNDD)
    mpfr_nextbelow (p); /* restore original p */
}

int
mpfr_zeta (mpfr_ptr z, mpfr_srcptr s, mpfr_rnd_t rnd_mode)
{
  mpfr_t z_pre, s1, y, p;
  long add;
  mpfr_prec_t precz, prec1, precs, precs1;
  int inex;
  MPFR_GROUP_DECL (group);
  MPFR_ZIV_DECL (loop);
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC (
    ("s[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (s), mpfr_log_prec, s, rnd_mode),
    ("z[%Pd]=%.*Rg inexact=%d", mpfr_get_prec (z), mpfr_log_prec, z, inex));

  /* Zero, Nan or Inf ? */
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (s)))
    {
      if (MPFR_IS_NAN (s))
        {
          MPFR_SET_NAN (z);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF (s))
        {
          if (MPFR_IS_POS (s))
            return mpfr_set_ui (z, 1, MPFR_RNDN); /* Zeta(+Inf) = 1 */
          MPFR_SET_NAN (z); /* Zeta(-Inf) = NaN */
          MPFR_RET_NAN;
        }
      else /* s iz zero */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (s));
          return mpfr_set_si_2exp (z, -1, -1, rnd_mode);
        }
    }

  /* s is neither Nan, nor Inf, nor Zero */

  /* check tiny s: we have zeta(s) = -1/2 - 1/2 log(2 Pi) s + ... around s=0,
     and for |s| <= 2^(-4), we have |zeta(s) + 1/2| <= |s|.
     EXP(s) + 1 < -PREC(z) is a sufficient condition to be able to round
     correctly, for any PREC(z) >= 1 (see algorithms.tex for details). */
  if (MPFR_GET_EXP (s) + 1 < - (mpfr_exp_t) MPFR_PREC(z))
    {
      int signs = MPFR_SIGN(s);

      MPFR_SAVE_EXPO_MARK (expo);
      mpfr_set_si_2exp (z, -1, -1, rnd_mode); /* -1/2 */
      if (rnd_mode == MPFR_RNDA)
        rnd_mode = MPFR_RNDD; /* the result is around -1/2, thus negative */
      if ((rnd_mode == MPFR_RNDU || rnd_mode == MPFR_RNDZ) && signs < 0)
        {
          mpfr_nextabove (z); /* z = -1/2 + epsilon */
          inex = 1;
        }
      else if (rnd_mode == MPFR_RNDD && signs > 0)
        {
          mpfr_nextbelow (z); /* z = -1/2 - epsilon */
          inex = -1;
        }
      else
        {
          if (rnd_mode == MPFR_RNDU) /* s > 0: z = -1/2 */
            inex = 1;
          else if (rnd_mode == MPFR_RNDD)
            inex = -1;              /* s < 0: z = -1/2 */
          else /* (MPFR_RNDZ and s > 0) or MPFR_RNDN: z = -1/2 */
            inex = (signs > 0) ? 1 : -1;
        }
      MPFR_SAVE_EXPO_FREE (expo);
      return mpfr_check_range (z, inex, rnd_mode);
    }

  /* Check for case s= -2n */
  if (MPFR_IS_NEG (s))
    {
      mpfr_t tmp;
      tmp[0] = *s;
      MPFR_EXP (tmp) = MPFR_GET_EXP (s) - 1;
      if (mpfr_integer_p (tmp))
        {
          MPFR_SET_ZERO (z);
          MPFR_SET_POS (z);
          MPFR_RET (0);
        }
    }

  /* Check for case s=1 before changing the exponent range */
  if (mpfr_equal_p (s, __gmpfr_one))
    {
      MPFR_SET_INF (z);
      MPFR_SET_POS (z);
      MPFR_SET_DIVBY0 ();
      MPFR_RET (0);
    }

  MPFR_SAVE_EXPO_MARK (expo);

  /* Compute Zeta */
  if (MPFR_IS_POS (s) && MPFR_GET_EXP (s) >= 0) /* Case s >= 1/2 */
    inex = mpfr_zeta_pos (z, s, rnd_mode);
  else /* use reflection formula
          zeta(s) = 2^s*Pi^(s-1)*sin(Pi*s/2)*gamma(1-s)*zeta(1-s) */
    {
      int overflow = 0;

      precz = MPFR_PREC (z);
      precs = MPFR_PREC (s);

      /* Precision precs1 needed to represent 1 - s, and s + 2,
         without any truncation */
      precs1 = precs + 2 + MAX (0, - MPFR_GET_EXP (s));
      /* Precision prec1 is the precision on elementary computations;
         it ensures a final precision prec1 - add for zeta(s) */
      add = compute_add (s, precz);
      prec1 = precz + add;
      /* FIXME: To avoid that the working precision (prec1) depends on the
         input precision, one would need to take into account the error made
         when s1 is not exactly 1-s when computing zeta(s1) and gamma(s1)
         below, and also in the case y=Inf (i.e. when gamma(s1) overflows).
         Make sure that underflows do not occur in intermediate computations.
         Due to the limited precision, they are probably not possible
         in practice; add some MPFR_ASSERTN's to be sure that problems
         do not remain undetected? */
      prec1 = MAX (prec1, precs1) + 10;

      MPFR_GROUP_INIT_4 (group, prec1, z_pre, s1, y, p);
      MPFR_ZIV_INIT (loop, prec1);
      for (;;)
        {
          mpfr_t z_up;

          mpfr_const_pi (p, MPFR_RNDD); /* p is Pi */

          mpfr_sub (s1, __gmpfr_one, s, MPFR_RNDN); /* s1 = 1-s */
          mpfr_gamma (y, s1, MPFR_RNDN);          /* gamma(1-s) */
          if (MPFR_IS_INF (y)) /* zeta(s) < 0 for -4k-2 < s < -4k,
                                  zeta(s) > 0 for -4k < s < -4k+2 */
            {
              /* FIXME: An overflow in gamma(s1) does not imply that
                 zeta(s) will overflow. A solution:
                 1. Compute
                   log(|zeta(s)|/2) = (s-1)*log(2*pi) + lngamma(1-s)
                     + log(abs(sin(Pi*s/2)) * zeta(1-s))
                 (possibly sharing computations with the normal case)
                 with a rather good accuracy (see (2)).
                 Memorize the sign of sin(...) for the final sign.
                 2. Take the exponential, ~= |zeta(s)|/2. If there is an
                 overflow, then this means an overflow on the final result
                 (due to the multiplication by 2, which has not been done
                 yet).
                 3. Ziv test.
                 4. Correct the sign from the sign of sin(...).
                 5. Round then multiply by 2. Here, an overflow in either
                 operation means a real overflow. */
              mpfr_reflection_overflow (z_pre, s1, s, y, p, MPFR_RNDD);
              /* z_pre is a lower bound of |zeta(s)|/2, thus if it overflows,
                 or has exponent emax, then |zeta(s)| overflows too. */
              if (MPFR_IS_INF (z_pre) || MPFR_GET_EXP(z_pre) == __gmpfr_emax)
                { /* determine the sign of overflow */
                  mpfr_div_2ui (s1, s, 2, MPFR_RNDN); /* s/4, exact */
                  mpfr_frac (s1, s1, MPFR_RNDN); /* exact, -1 < s1 < 0 */
                  overflow = (mpfr_cmp_si_2exp (s1, -1, -1) > 0) ? -1 : 1;
                  break;
                }
              else /* EXP(z_pre) < __gmpfr_emax */
                {
                  int ok = 0;
                  mpfr_t z_down;
                  mpfr_init2 (z_up, mpfr_get_prec (z_pre));
                  mpfr_reflection_overflow (z_up, s1, s, y, p, MPFR_RNDU);
                  /* if the lower approximation z_pre does not overflow, but
                     z_up does, we need more precision */
                  if (MPFR_IS_INF (z_up) || MPFR_GET_EXP(z_up) == __gmpfr_emax)
                    goto next_loop;
                  /* check if z_pre and z_up round to the same number */
                  mpfr_init2 (z_down, precz);
                  mpfr_set (z_down, z_pre, rnd_mode);
                  /* Note: it might be that EXP(z_down) = emax here, in that
                     case we will have overflow below when we multiply by 2 */
                  mpfr_prec_round (z_up, precz, rnd_mode);
                  ok = mpfr_equal_p (z_down, z_up);
                  mpfr_clear (z_up);
                  mpfr_clear (z_down);
                  if (ok)
                    {
                      /* get correct sign and multiply by 2 */
                      mpfr_div_2ui (s1, s, 2, MPFR_RNDN); /* s/4, exact */
                      mpfr_frac (s1, s1, MPFR_RNDN); /* exact, -1 < s1 < 0 */
                      if (mpfr_cmp_si_2exp (s1, -1, -1) > 0)
                        mpfr_neg (z_pre, z_pre, rnd_mode);
                      mpfr_mul_2ui (z_pre, z_pre, 1, rnd_mode);
                      break;
                    }
                  else
                    goto next_loop;
                }
            }
          mpfr_zeta_pos (z_pre, s1, MPFR_RNDN);   /* zeta(1-s)  */
          mpfr_mul (z_pre, z_pre, y, MPFR_RNDN);  /* gamma(1-s)*zeta(1-s) */

          /* multiply z_pre by 2^s*Pi^(s-1) where p=Pi, s1=1-s */
          mpfr_mul_2ui (y, p, 1, MPFR_RNDN);      /* 2*Pi */
          mpfr_neg (s1, s1, MPFR_RNDN);           /* s-1 */
          mpfr_pow (y, y, s1, MPFR_RNDN);         /* (2*Pi)^(s-1) */
          mpfr_mul (z_pre, z_pre, y, MPFR_RNDN);
          mpfr_mul_2ui (z_pre, z_pre, 1, MPFR_RNDN);

          /* multiply z_pre by sin(Pi*s/2) */
          mpfr_div_2ui (p, s, 1, MPFR_RNDN);      /* p = s/2 */
          /* Can mpfr_sinpi underflow? While with mpfr_sin, we could not
             answer in any precision without a theoretical study (though
             an underflow would have been very unlikely as we have a
             huge exponent range), with mpfr_sinpi, an underflow could
             occur only in a huge, unsupported precision. Indeed, if
             mpfr_sinpi underflows, this means that 0 < |sinpi(s/2)| < m,
             where m is the minimum representable positive number, and in
             this case, r being the reduced argument such that |r| <= 1/2,
             one has |sinpi(r)| > |2r|, so that |2r| < m; this can be
             possible only if |s/2| > 1/2 (otherwise |s| = |2r| < m and
             s would not be representable as an MPFR number) and s has
             non-zero bits of exponent less than the minimum exponent
             (s/2 - r being an integer), i.e. the precision is at least
             MPFR_EMAX_MAX + 2. With such a huge precision, there would
             probably be failures before reaching this point. */
          mpfr_sinpi (y, p, MPFR_RNDN);           /* y = sin(Pi*s/2) */
          mpfr_mul (z_pre, z_pre, y, MPFR_RNDN);

          if (MPFR_LIKELY (MPFR_CAN_ROUND (z_pre, prec1 - add, precz,
                                           rnd_mode)))
            break;

        next_loop:
          MPFR_ZIV_NEXT (loop, prec1);
          MPFR_GROUP_REPREC_4 (group, prec1, z_pre, s1, y, p);
        }
      MPFR_ZIV_FREE (loop);
      if (overflow != 0)
        {
          inex = mpfr_overflow (z, rnd_mode, overflow);
          MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, MPFR_FLAGS_OVERFLOW);
        }
      else
        inex = mpfr_set (z, z_pre, rnd_mode);
      MPFR_GROUP_CLEAR (group);
    }

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (z, inex, rnd_mode);
}
