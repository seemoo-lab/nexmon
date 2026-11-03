/* mpfr_log_ui -- compute natural logarithm of an unsigned long

Copyright 2014-2025 Free Software Foundation, Inc.
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

/* FIXME: mpfr_log_ui is much slower than mpfr_log on some values of n,
   e.g. about 4 times as slow for n around ULONG_MAX/3 on an
   x86_64 Linux machine, for 10^6 bits of precision. The reason is that
   for say n=6148914691236517205 and prec=10^6, the value of T computed
   has more than 50M bits, which is much more than needed. Indeed the
   binary splitting algorithm for series with a finite radius of convergence
   gives rationals of size n*log(n) for a target precision n. One might
   truncate the rationals inside the algorithm, but then the error analysis
   should be redone. */

/* Cf https://www.ginac.de/CLN/binsplit.pdf - the Taylor series of log(1+x)
   up to order N for x=p/2^k is T/(B*Q).
   P[0] <- (-p)^(n2-n1) [with opposite sign when n1=1]
   q <- k*(n2-n1) [corresponding to Q[0] = 2^q]
   B[0] <- n1 * (n1+1) * ... * (n2-1)
   T[0] <- B[0]*Q[0] * S(n1,n2)
   where S(n1,n2) = -sum((-x)^(i-n1+1)/i, i=n1..n2-1)
   Assumes p is odd or zero, and -1/3 <= x = p/2^k <= 1/3.
*/
static void
S (mpz_t *P, unsigned long *q, mpz_t *B, mpz_t *T, unsigned long n1,
   unsigned long n2, long p, unsigned long k, int need_P)
{
  MPFR_ASSERTD (n1 < n2);
  MPFR_ASSERTD (p == 0 || ((unsigned long) p & 1) != 0);
  if (n2 == n1 + 1)
    {
      mpz_set_si (P[0], (n1 == 1) ? p : -p);
      *q = k;
      mpz_set_ui (B[0], n1);
      /* T = B*Q*S where S = P/(B*Q) thus T = P */
      mpz_set (T[0], P[0]);
      /* since p is odd (or zero), there is no common factor 2 between
         P and Q, or T and B */
    }
  else
    {
      unsigned long m = (n1 / 2) + (n2 / 2) + (n1 & 1UL & n2), q1;
      /* m = floor((n1+n2)/2) */

      MPFR_ASSERTD (n1 < m && m < n2);
      S (P, q, B, T, n1, m, p, k, 1);
      S (P + 1, &q1, B + 1, T + 1, m, n2, p, k, need_P);

      /* T0 <- T0*B1*Q1 + P0*B0*T1 */
      mpz_mul (T[1], T[1], P[0]);
      mpz_mul (T[1], T[1], B[0]);
      mpz_mul (T[0], T[0], B[1]);
      /* Q[1] = 2^q1 */
      mpz_mul_2exp (T[0], T[0], q1); /* mpz_mul (T[0], T[0], Q[1]) */
      mpz_add (T[0], T[0], T[1]);
      if (need_P)
        mpz_mul (P[0], P[0], P[1]);
      *q += q1; /* mpz_mul (Q[0], Q[0], Q[1]) */
      mpz_mul (B[0], B[0], B[1]);

      /* there should be no common factors 2 between P, Q and T,
         since P is odd (or zero) */
    }
}

int
mpfr_log_ui (mpfr_ptr x, unsigned long n, mpfr_rnd_t rnd_mode)
{
  unsigned long k;
  mpfr_prec_t w; /* working precision */
  mpz_t three_n, *P, *B, *T;
  mpfr_t t, q;
  int inexact;
  unsigned long N, lgN, i, kk;
  long p;
  MPFR_GROUP_DECL(group);
  MPFR_TMP_DECL(marker);
  MPFR_ZIV_DECL(loop);
  MPFR_SAVE_EXPO_DECL (expo);

  if (n <= 2)
    {
      if (n == 0)
        {
          MPFR_SET_INF (x);
          MPFR_SET_NEG (x);
          MPFR_SET_DIVBY0 ();
          MPFR_RET (0); /* log(0) is an exact -infinity */
        }
      else if (n == 1)
        {
          MPFR_SET_ZERO (x);
          MPFR_SET_POS (x);
          MPFR_RET (0); /* only "normal" case where the result is exact */
        }
      /* now n=2 */
      return mpfr_const_log2 (x, rnd_mode);
    }

  /* here n >= 3 */

  /* Argument reduction: compute k such that 2/3 < n/2^k < 4/3,
     i.e., 2^(k+1) < 3n < 2^(k+2).

     FIXME: we could do better by considering n/(2^k*3^i*5^j),
     which reduces the maximal distance to 1 from 1/3 to 1/8,
     thus needing about 1.89 less terms in the Taylor expansion of
     the reduced argument. Then log(2^k*3^i*5^j) can be computed
     using a combination of log(16/15), log(25/24) and log(81/80),
     see Section 6.5 of "A Fortran Multiple-Precision Arithmetic Package",
     Richard P. Brent, ACM Transactions on Mathematical Software, 1978. */

  mpz_init_set_ui (three_n, n);
  mpz_mul_ui (three_n, three_n, 3);
  k = mpz_sizeinbase (three_n, 2) - 2;
  MPFR_ASSERTD (k >= 2);
  mpz_clear (three_n);

  /* The reduced argument is n/2^k - 1 = (n-2^k)/2^k.
     Compute p = n-2^k. One has: |p| = |n-2^k| < 2^k/3 < n/2 <= LONG_MAX,
     so that p and -p both fit in a long. */
  if (k < sizeof (unsigned long) * CHAR_BIT)  /* assume no padding bits */
    n -= 1UL << k;
  /* n is now the value of p mod ULONG_MAX+1.
     Since |p| <= LONG_MAX, if n > LONG_MAX, this means that p < 0 and
     -n as an unsigned long value is at most LONG_MAX, thus fits in a
     long. */
  p = ULONG2LONG (n);

  MPFR_TMP_MARK(marker);
  w = MPFR_PREC(x) + MPFR_INT_CEIL_LOG2 (MPFR_PREC(x)) + 10;
  MPFR_GROUP_INIT_2(group, w, t, q);
  MPFR_SAVE_EXPO_MARK (expo);

  kk = k;
  if (p != 0)
    while ((p % 2) == 0) /* replace p/2^kk by (p/2)/2^(kk-1) */
      {
        p /= 2;
        kk --;
      }

  MPFR_ZIV_INIT (loop, w);
  for (;;)
    {
      mpfr_t tmp;
      unsigned int err;
      unsigned long q0;

      /* we need at most w/log2(2^kk/|p|) terms for an accuracy of w bits */
      mpfr_init2 (tmp, 32);
      mpfr_set_ui (tmp, (p > 0) ? p : -p, MPFR_RNDU);
      mpfr_log2 (tmp, tmp, MPFR_RNDU);
      mpfr_ui_sub (tmp, kk, tmp, MPFR_RNDD);
      MPFR_ASSERTN (w <= ULONG_MAX);
      mpfr_ui_div (tmp, w, tmp, MPFR_RNDU);
      N = mpfr_get_ui (tmp, MPFR_RNDU);
      if (N < 2)
        N = 2;
      lgN = MPFR_INT_CEIL_LOG2 (N) + 1;
      mpfr_clear (tmp);
      P = (mpz_t *) MPFR_TMP_ALLOC (3 * lgN * sizeof (mpz_t));
      B = P + lgN;
      T = B + lgN;
      for (i = 0; i < lgN; i++)
        {
          mpz_init (P[i]);
          mpz_init (B[i]);
          mpz_init (T[i]);
        }

      S (P, &q0, B, T, 1, N, p, kk, 0);
      /* mpz_mul (Q[0], B[0], Q[0]); */
      /* mpz_mul_2exp (B[0], B[0], q0); */

      mpfr_set_z (t, T[0], MPFR_RNDN); /* t = P[0] * (1 + theta_1) */
      mpfr_set_z (q, B[0], MPFR_RNDN); /* q = B[0] * (1 + theta_2) */
      mpfr_mul_2ui (q, q, q0, MPFR_RNDN); /* B[0]*Q[0] */
      mpfr_div (t, t, q, MPFR_RNDN);   /* t = T[0]/(B[0]*Q[0])*(1 + theta_3)^3
                                            = log(n/2^k) * (1 + theta_4)^4
                                            for |theta_i| < 2^(-w) */

      /* argument reconstruction: add k*log(2) */
      mpfr_const_log2 (q, MPFR_RNDN);
      mpfr_mul_ui (q, q, k, MPFR_RNDN);
      mpfr_add (t, t, q, MPFR_RNDN);
      for (i = 0; i < lgN; i++)
        {
          mpz_clear (P[i]);
          mpz_clear (B[i]);
          mpz_clear (T[i]);
        }
      /* The maximal error is 5 ulps for P/Q, since |(1+/-u)^4 - 1| < 5*u
         for u < 2^(-12), k ulps for k*log(2), and 1 ulp for the addition,
         thus at most k+6 ulps.
         Note that there might be some cancellation in the addition: the worst
         case is when log(1 + p/2^kk) = log(2/3) ~ -0.405, and with n=3 which
         gives k=2, thus we add 2*log(2) = 1.386. Thus in the worst case we
         have an exponent decrease of 1, which accounts for +1 in the error. */
      err = MPFR_INT_CEIL_LOG2 (k + 6) + 1;
      if (MPFR_LIKELY (MPFR_CAN_ROUND (t, w - err, MPFR_PREC(x), rnd_mode)))
        break;

      MPFR_ZIV_NEXT (loop, w);
      MPFR_GROUP_REPREC_2(group, w, t, q);
    }
  MPFR_ZIV_FREE (loop);

  inexact = mpfr_set (x, t, rnd_mode);

  MPFR_GROUP_CLEAR(group);
  MPFR_TMP_FREE(marker);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (x, inexact, rnd_mode);
}
