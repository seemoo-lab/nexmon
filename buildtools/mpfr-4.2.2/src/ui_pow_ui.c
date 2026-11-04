/*  mpfr_ui_pow_ui -- compute the power between two machine integers

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

#include "mpfr-impl.h"

int
mpfr_ui_pow_ui (mpfr_ptr x, unsigned long int k, unsigned long int n,
                mpfr_rnd_t rnd)
{
  mpfr_exp_t err;
  unsigned long m;
  mpfr_t res;
  mpfr_prec_t prec;
  int size_n;
  int inexact;
  MPFR_ZIV_DECL (loop);
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC
    (("k=%lu n=%lu rnd=%d", k, n, rnd),
     ("y[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec (x), mpfr_log_prec, x, inexact));

  if (MPFR_UNLIKELY (n <= 1))
    {
      if (n == 1)
        return mpfr_set_ui (x, k, rnd);     /* k^1 = k */
      else
        return mpfr_set_ui (x, 1, rnd);     /* k^0 = 1 for any k */
    }
  else if (MPFR_UNLIKELY (k <= 1))
    {
      if (k == 1)
        return mpfr_set_ui (x, 1, rnd);     /* 1^n = 1 for any n > 0 */
      else
        return mpfr_set_ui (x, 0, rnd);     /* 0^n = 0 for any n > 0 */
    }

  for (size_n = 0, m = n; m != 0; size_n++, m >>= 1)
    ;

  MPFR_SAVE_EXPO_MARK (expo);
  prec = MPFR_PREC (x) + 3 + size_n;
  mpfr_init2 (res, prec);

  MPFR_ZIV_INIT (loop, prec);
  for (;;)
    {
      int i = size_n;
      unsigned int inex_res;

      inex_res = mpfr_set_ui (res, k, MPFR_RNDU);
      err = 1;
      /* now 2^(i-1) <= n < 2^i: i=1+floor(log2(n)) */
      for (i -= 2; i >= 0; i--)
        {
          inex_res |= mpfr_sqr (res, res, MPFR_RNDU);
          err++;
          if (n & (1UL << i))
            inex_res |= mpfr_mul_ui (res, res, k, MPFR_RNDU);
        }

      if (MPFR_UNLIKELY (MPFR_IS_INF (res)))
        {
          mpfr_t kf;
          mpz_t z;
          int size_k;
          MPFR_BLOCK_DECL (flags);

          /* Let's handle the overflow by calling mpfr_pow_z.
             Alternatively, we could call mpfr_pow_ui; this would
             need a bit shorter code below, but mpfr_pow_ui handles
             the overflow by calling mpfr_pow_z, so that calling
             mpfr_pow_z directly should be a bit more efficient. */

          MPFR_ZIV_FREE (loop);
          mpfr_clear (res);
          for (size_k = 0, m = k; m != 0; size_k++, m >>= 1)
            ;
          mpfr_init2 (kf, size_k);
          inexact = mpfr_set_ui (kf, k, MPFR_RNDN);
          MPFR_ASSERTD (inexact == 0);
          mpz_init (z);
          mpz_set_ui (z, n);
          MPFR_BLOCK (flags, inexact = mpfr_pow_z (x, kf, z, rnd););
          mpz_clear (z);
          mpfr_clear (kf);
          MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, flags);
          goto end;
        }

      /* since the loop is executed floor(log2(n)) times,
         we have err = 1+floor(log2(n)).
         Since prec >= MPFR_PREC(x) + 4 + floor(log2(n)), prec > err */
      err = prec - err;

      MPFR_LOG_VAR (res);
      if (MPFR_LIKELY (!inex_res
                       || MPFR_CAN_ROUND (res, err, MPFR_PREC (x), rnd)))
        break;

      /* Actualisation of the precision */
      MPFR_ZIV_NEXT (loop, prec);
      mpfr_set_prec (res, prec);
    }
  MPFR_ZIV_FREE (loop);

  inexact = mpfr_set (x, res, rnd);

  mpfr_clear (res);

 end:
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (x, inexact, rnd);
}
