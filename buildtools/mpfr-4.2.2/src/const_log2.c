/* mpfr_const_log2 -- compute natural logarithm of 2

Copyright 1999, 2001-2025 Free Software Foundation, Inc.
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

/* Declare the cache */
#ifndef MPFR_USE_LOGGING
MPFR_DECL_INIT_CACHE (__gmpfr_cache_const_log2, mpfr_const_log2_internal)
#else
MPFR_DECL_INIT_CACHE (__gmpfr_normal_log2, mpfr_const_log2_internal)
MPFR_DECL_INIT_CACHE (__gmpfr_logging_log2, mpfr_const_log2_internal)
MPFR_THREAD_VAR (mpfr_cache_ptr, __gmpfr_cache_const_log2, __gmpfr_normal_log2)
#endif

/* Set User interface */
#undef mpfr_const_log2
int
mpfr_const_log2 (mpfr_ptr x, mpfr_rnd_t rnd_mode) {
  return mpfr_cache (x, __gmpfr_cache_const_log2, rnd_mode);
}

/* Auxiliary function: Compute the terms from n1 to n2 (excluded)
   3/4*sum((-1)^n*n!^2/2^n/(2*n+1)!, n = n1..n2-1).
   Numerator is T[0], denominator is Q[0],
   Compute P[0] only when need_P is non-zero.
   Need 1+ceil(log(n2-n1)/log(2)) cells in T[],P[],Q[].
*/
static void
S (mpz_t *T, mpz_t *P, mpz_t *Q, unsigned long n1, unsigned long n2, int need_P)
{
  if (n2 == n1 + 1)
    {
      if (n1 == 0)
        mpz_set_ui (P[0], 3);
      else
        {
          mpz_set_ui (P[0], n1);
          mpz_neg (P[0], P[0]);
        }
      /* since n1 <= N, where N is the value from mpfr_const_log2_internal(),
         and N = w / 3 + 1, where w <= PREC_MAX <= ULONG_MAX, then
         N <= floor(ULONG_MAX/3) + 1, thus 2*N+1 <= ULONG_MAX */
      MPFR_STAT_STATIC_ASSERT (MPFR_PREC_MAX <= ULONG_MAX);
      mpz_set_ui (Q[0], 2 * n1 + 1);
      mpz_mul_2exp (Q[0], Q[0], 2);
      mpz_set (T[0], P[0]);
    }
  else
    {
      unsigned long m = (n1 / 2) + (n2 / 2) + (n1 & 1UL & n2);
      mp_bitcnt_t v, w;

      S (T, P, Q, n1, m, 1);
      S (T + 1, P + 1, Q + 1, m, n2, need_P);
      mpz_mul (T[0], T[0], Q[1]);
      mpz_mul (T[1], T[1], P[0]);
      mpz_add (T[0], T[0], T[1]);
      if (need_P)
        mpz_mul (P[0], P[0], P[1]);
      mpz_mul (Q[0], Q[0], Q[1]);

      /* remove common trailing zeroes if any */
      v = mpz_scan1 (T[0], 0);
      if (v > 0)
        {
          w = mpz_scan1 (Q[0], 0);
          if (w < v)
            v = w;
          if (need_P)
            {
              w = mpz_scan1 (P[0], 0);
              if (w < v)
                v = w;
            }
          /* now v = min(val(T), val(Q), val(P)) */
          if (v > 0)
            {
              mpz_fdiv_q_2exp (T[0], T[0], v);
              mpz_fdiv_q_2exp (Q[0], Q[0], v);
              if (need_P)
                mpz_fdiv_q_2exp (P[0], P[0], v);
            }
        }
    }
}

/* Don't need to save / restore exponent range: the cache does it */
int
mpfr_const_log2_internal (mpfr_ptr x, mpfr_rnd_t rnd_mode)
{
  unsigned long n = MPFR_PREC (x);
  mpfr_prec_t w; /* working precision */
  unsigned long N;
  mpz_t *T, *P, *Q;
  mpfr_t t, q;
  int inexact;
  unsigned long lgN, i;
  MPFR_GROUP_DECL(group);
  MPFR_TMP_DECL(marker);
  MPFR_ZIV_DECL(loop);

  MPFR_LOG_FUNC (
    ("rnd_mode=%d", rnd_mode),
    ("x[%Pd]=%.*Rg inex=%d", mpfr_get_prec(x), mpfr_log_prec, x, inexact));

  w = n + MPFR_INT_CEIL_LOG2 (n) + 3;

  MPFR_TMP_MARK(marker);
  MPFR_GROUP_INIT_2(group, w, t, q);

  MPFR_ZIV_INIT (loop, w);
  for (;;)
    {
      N = w / 3 + 1;

      /* the following are needed for error analysis (see algorithms.tex) */
      MPFR_ASSERTD(w >= 3 && N >= 2);

      lgN = MPFR_INT_CEIL_LOG2 (N) + 1;
      T  = (mpz_t *) MPFR_TMP_ALLOC (3 * lgN * sizeof (mpz_t));
      P  = T + lgN;
      Q  = T + 2*lgN;
      for (i = 0; i < lgN; i++)
        {
          mpz_init (T[i]);
          mpz_init (P[i]);
          mpz_init (Q[i]);
        }

      S (T, P, Q, 0, N, 0);

      mpfr_set_z (t, T[0], MPFR_RNDN);
      mpfr_set_z (q, Q[0], MPFR_RNDN);
      mpfr_div (t, t, q, MPFR_RNDN);

      for (i = 0; i < lgN; i++)
        {
          mpz_clear (T[i]);
          mpz_clear (P[i]);
          mpz_clear (Q[i]);
        }

      if (MPFR_CAN_ROUND (t, w - 2, n, rnd_mode))
        break;

      MPFR_ZIV_NEXT (loop, w);
      MPFR_GROUP_REPREC_2(group, w, t, q);
    }
  MPFR_ZIV_FREE (loop);

  inexact = mpfr_set (x, t, rnd_mode);

  MPFR_GROUP_CLEAR(group);
  MPFR_TMP_FREE(marker);

  return inexact;
}
