/* mpfr_const_euler -- Euler's constant

Copyright 2001-2025 Free Software Foundation, Inc.
Contributed by Fredrik Johansson.

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

/* The approximation error bound uses Theorem 1 and Remark 2 in
   https://arxiv.org/pdf/1312.0039v1.pdf */

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

/* Declare the cache */
MPFR_DECL_INIT_CACHE (__gmpfr_cache_const_euler, mpfr_const_euler_internal)

/* Set User Interface */
#undef mpfr_const_euler
int
mpfr_const_euler (mpfr_ptr x, mpfr_rnd_t rnd_mode) {
  return mpfr_cache (x, __gmpfr_cache_const_euler, rnd_mode);
}


typedef struct
{
  mpz_t P;
  mpz_t Q;
  mpz_t T;
  mpz_t C;
  mpz_t D;
  mpz_t V;
} mpfr_const_euler_bs_struct;

typedef mpfr_const_euler_bs_struct mpfr_const_euler_bs_t[1];

static void
mpfr_const_euler_bs_init (mpfr_const_euler_bs_t s)
{
  mpz_init (s->P);
  mpz_init (s->Q);
  mpz_init (s->T);
  mpz_init (s->C);
  mpz_init (s->D);
  mpz_init (s->V);
}

static void
mpfr_const_euler_bs_clear (mpfr_const_euler_bs_t s)
{
  mpz_clear (s->P);
  mpz_clear (s->Q);
  mpz_clear (s->T);
  mpz_clear (s->C);
  mpz_clear (s->D);
  mpz_clear (s->V);
}

static void
mpfr_const_euler_bs_1 (mpfr_const_euler_bs_t s,
                       unsigned long n1, unsigned long n2, unsigned long N,
                       int cont)
{
  if (n2 - n1 == 1)
    {
      mpz_set_ui (s->P, N);
      mpz_mul (s->P, s->P, s->P);
      mpz_set_ui (s->Q, n1 + 1);
      mpz_mul (s->Q, s->Q, s->Q);
      mpz_set_ui (s->C, 1);
      mpz_set_ui (s->D, n1 + 1);
      mpz_set (s->T, s->P);
      mpz_set (s->V, s->P);
    }
  else
    {
      mpfr_const_euler_bs_t L, R;
      mpz_t t, u, v;
      unsigned long m = (n1 + n2) / 2;

      mpfr_const_euler_bs_init (L);
      mpfr_const_euler_bs_init (R);
      mpfr_const_euler_bs_1 (L, n1, m, N, 1);
      mpfr_const_euler_bs_1 (R, m, n2, N, 1);

      mpz_init (t);
      mpz_init (u);
      mpz_init (v);

      if (cont)
        mpz_mul (s->P, L->P, R->P);

      mpz_mul (s->Q, L->Q, R->Q);
      mpz_mul (s->D, L->D, R->D);

      /* T = LP RT + RQ LT*/
      mpz_mul (t, L->P, R->T);
      mpz_mul (v, R->Q, L->T);
      mpz_add (s->T, t, v);

      /* C = LC RD + RC LD */
      if (cont)
        {
          mpz_mul (s->C, L->C, R->D);
          mpz_addmul (s->C, R->C, L->D);
        }

      /* V = RD (RQ LV + LC LP RT) + LD LP RV */
      mpz_mul (u, L->P, R->V);
      mpz_mul (u, u, L->D);
      mpz_mul (v, R->Q, L->V);
      mpz_addmul (v, t, L->C);
      mpz_mul (v, v, R->D);
      mpz_add (s->V, u, v);

      mpfr_const_euler_bs_clear (L);
      mpfr_const_euler_bs_clear (R);
      mpz_clear (t);
      mpz_clear (u);
      mpz_clear (v);
  }
}

static void
mpfr_const_euler_bs_2 (mpz_t P, mpz_t Q, mpz_t T,
                       unsigned long n1, unsigned long n2, unsigned long N,
                       int cont)
{
  if (n2 - n1 == 1)
    {
      if (n1 == 0)
        {
          mpz_set_ui (P, 1);
          mpz_set_ui (Q, 4 * N);
        }
      else
        {
          mpz_set_ui (P, 2 * n1 - 1);
          mpz_pow_ui (P, P, 3);
          mpz_set_ui (Q, 32 * n1);
          mpz_mul_ui (Q, Q, N);
          mpz_mul_ui (Q, Q, N);
        }
      mpz_set (T, P);
    }
  else
    {
      mpz_t P2, Q2, T2;
      unsigned long m = (n1 + n2) / 2;

      mpz_init (P2);
      mpz_init (Q2);
      mpz_init (T2);
      mpfr_const_euler_bs_2 (P, Q, T, n1, m, N, 1);
      mpfr_const_euler_bs_2 (P2, Q2, T2, m, n2, N, 1);
      mpz_mul (T, T, Q2);
      mpz_mul (T2, T2, P);
      mpz_add (T, T, T2);
      if (cont)
        mpz_mul (P, P, P2);
      mpz_mul (Q, Q, Q2);
      mpz_clear (P2);
      mpz_clear (Q2);
      mpz_clear (T2);
    }
}

int
mpfr_const_euler_internal (mpfr_ptr x, mpfr_rnd_t rnd)
{
  mpfr_const_euler_bs_t sum;
  mpz_t t, u, v;
  unsigned long n, N;
  mpfr_prec_t prec, wp, magn;
  mpfr_t y;
  int inexact;
  MPFR_ZIV_DECL (loop);

  prec = mpfr_get_prec (x);
  wp = prec + MPFR_INT_CEIL_LOG2 (prec) + 5;

  mpfr_init2 (y, wp);
  mpfr_const_euler_bs_init (sum);
  mpz_init (t);
  mpz_init (u);
  mpz_init (v);

  MPFR_ZIV_INIT (loop, wp);
  for (;;)
    {
      /* The approximation error is bounded by 24 exp(-8n) when
         n > 1, which is smaller than 2^-wp if
         n > (wp + log_2(24)) * (log(2)/8).
         Note log2(24) < 5 and log(2)/8 < 866434 / 10000000. */
      mpz_set_ui (t, wp + 5);
      mpz_mul_ui (t, t, 866434);
      mpz_cdiv_q_ui (t, t, 10000000);
      n = mpz_get_ui (t);

      /* It is sufficient to take N >= alpha*n + 1
         where alpha = 3/LambertW(3/e) = 4.970625759544... */
      mpz_set_ui (t, n);
      mpz_mul_ui (t, t, 4970626);
      mpz_cdiv_q_ui (t, t, 1000000);
      mpz_add_ui (t, t, 1);
      N = mpz_get_ui (t);

      /* V / ((T + Q) * D) = S / I
         where S = sum_{k=0}^{N-1} H_k n^(2k) / (k!)^2,
               I = sum_{k=0}^{N-1} n^(2k) / (k!)^2 */
      mpfr_const_euler_bs_1 (sum, 0, N, n, 0);
      mpz_add (sum->T, sum->T, sum->Q);
      mpz_mul (t, sum->T, sum->D);
      mpz_mul_2exp (u, sum->V, wp);
      mpz_tdiv_q (v, u, t);
      /* v * 2^-wp = S/I with error < 1 */

      /* C / (D * V) = U where
         U = (1/(4n)) sum_{k=0}^{2n-1} [(2k)!]^3 / ((k!)^4 8^(2k) (2n)^(2k)) */
      mpfr_const_euler_bs_2 (sum->C, sum->D, sum->V, 0, 2*n, n, 0);
      mpz_mul (t, sum->Q, sum->Q);
      mpz_mul (t, t, sum->V);
      mpz_mul (u, sum->T, sum->T);
      mpz_mul (u, u, sum->D);
      mpz_mul_2exp (t, t, wp);
      mpz_tdiv_q (t, t, u);
      /* t * 2^-wp = U/I^2 with error < 1 */

      /* gamma = S/I - U/I^2 - log(n) with error at most 2^-wp */
      mpz_sub (v, v, t);
      /* v * 2^-wp now equals gamma + log(n) with error at most 3*2^-wp */

      /* log(n) < 2^ceil(log2(n)) */
      magn = MPFR_INT_CEIL_LOG2(n);
      mpfr_set_prec (y, wp + magn);
      mpfr_set_ui (y, n, MPFR_RNDZ); /* exact */
      mpfr_log (y, y, MPFR_RNDZ); /* error < 2^-wp */

      mpfr_mul_2ui (y, y, wp, MPFR_RNDZ);
      mpfr_z_sub (y, v, y, MPFR_RNDZ);
      mpfr_div_2ui (y, y, wp, MPFR_RNDZ);
      /* rounding error from the last subtraction < 2^-wp */
      /* so y = gamma with error < 5*2^-wp */

      if (MPFR_LIKELY (MPFR_CAN_ROUND (y, wp - 3, prec, rnd)))
        break;

      MPFR_ZIV_NEXT (loop, wp);
    }

  MPFR_ZIV_FREE (loop);
  inexact = mpfr_set (x, y, rnd);

  mpfr_clear (y);
  mpz_clear (t);
  mpz_clear (u);
  mpz_clear (v);
  mpfr_const_euler_bs_clear (sum);

  return inexact; /* always inexact */
}
