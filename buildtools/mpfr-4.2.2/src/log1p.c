/* mpfr_log1p -- Compute log(1+x)

Copyright 2001-2025 Free Software Foundation, Inc.
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

/* Put in y an approximation of log(1+x) for x small.
   We assume |x| < 1/2, in which case:
   |x/2| <= |log(1+x)| <= |2x|.
   Return k such that the error is bounded by 2^k*ulp(y).
*/
static int
mpfr_log1p_small (mpfr_ptr y, mpfr_srcptr x)
{
  mpfr_prec_t p = MPFR_PREC(y), err;
  mpfr_t t, u;
  unsigned long i;
  int k;

  MPFR_ASSERTD(MPFR_GET_EXP (x) <= -1); /* ensures |x| < 1/2 */

  /* in the following, theta represents a value with |theta| <= 2^(1-p)
     (might be a different value each time) */

  mpfr_init2 (t, p);
  mpfr_init2 (u, p);
  mpfr_set (t, x, MPFR_RNDF); /* t = x * (1 + theta) */
  mpfr_set (y, t, MPFR_RNDF); /* exact */
  for (i = 2; ; i++)
    {
      mpfr_mul (t, t, x, MPFR_RNDF);    /* t = x^i * (1 + theta)^i */
      mpfr_div_ui (u, t, i, MPFR_RNDF); /* u = x^i/i * (1 + theta)^(i+1) */
      if (MPFR_GET_EXP (u) <= MPFR_GET_EXP (y) - p) /* |u| < ulp(y) */
        break;
      if (i & 1)
        mpfr_add (y, y, u, MPFR_RNDF); /* error <= ulp(y) */
      else
        mpfr_sub (y, y, u, MPFR_RNDF); /* error <= ulp(y) */
    }
  /* We assume |(1 + theta)^(i+1)| <= 2.
     The neglected part is at most |u| + |u|/2 + ... <= 2|u| < 2 ulp(y)
     which has to be multiplied by |(1 + theta)^(i+1)| <= 2, thus at most
     4 ulp(y).
     The rounding error on y is bounded by:
     * for the (i-2) add/sub, each error is bounded by ulp(y),
       and since |y| <= |x|, this yields (i-2)*ulp(x)
     * from Lemma 3.1 from [Higham02] (see algorithms.tex),
       the relative error on u at step i is bounded by:
       (i+1)*epsilon/(1-(i+1)*epsilon) where epsilon = 2^(1-p).
       If (i+1)*epsilon <= 1/2, then the relative error on u at
       step i is bounded by 2*(i+1)*epsilon, and since |u| <= 1/2^(i+1)
       at step i, this gives an absolute error bound of;
       2*epsilon*x*(3/2^3 + 4/2^4 + 5/2^5 + ...) <= 2*2^(1-p)*x =
       4*2^(-p)*x <= 4*ulp(x).

     If (i+1)*epsilon <= 1/2, then the relative error on u at step i
     is bounded by (i+1)*epsilon/(1-(i+1)*epsilon) <= 1, thus it follows
     |(1 + theta)^(i+1)| <= 2.

     Finally the total error is bounded by 4*ulp(y) + (i-2)*ulp(x) + 4*ulp(x)
     = 4*ulp(y) + (i+2)*ulp(x).
     Since x/2 <= y, we have ulp(x) <= 2*ulp(y), thus the error is bounded by:
     (2*i+8)*ulp(y).
  */
  err = 2 * i + 8;
  k = __gmpfr_int_ceil_log2 (err);
  MPFR_ASSERTN(k < p);
  /* if k < p, since k = ceil(log2(err)), we have err <= 2^k <= 2^(p-1),
     thus i+4 = err/2 <= 2^(p-2), thus (i+4)*epsilon <= 1/2, which implies
     our assumption (i+1)*epsilon <= 1/2. */
  mpfr_clear (t);
  mpfr_clear (u);
  return k;
}

/* The computation of log1p is done by
   log1p(x) = log(1+x)
   except when x is very small, in which case log1p(x) = x + tiny error,
   or when x is small, where we use directly the Taylor expansion.
*/

int
mpfr_log1p (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  int comp, inexact;
  mpfr_exp_t ex;
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (x), mpfr_log_prec, x, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d", mpfr_get_prec (y), mpfr_log_prec, y,
      inexact));

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      /* check for inf or -inf (result is not defined) */
      else if (MPFR_IS_INF (x))
        {
          if (MPFR_IS_POS (x))
            {
              MPFR_SET_INF (y);
              MPFR_SET_POS (y);
              MPFR_RET (0);
            }
          else
            {
              MPFR_SET_NAN (y);
              MPFR_RET_NAN;
            }
        }
      else /* x is zero */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (x));
          MPFR_SET_ZERO (y);   /* log1p(+/- 0) = +/- 0 */
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0);
        }
    }

  ex = MPFR_GET_EXP (x);
  if (ex < 0)  /* -0.5 < x < 0.5 */
    {
      /* For x > 0,    abs(log(1+x)-x) < x^2/2.
         For x > -0.5, abs(log(1+x)-x) < x^2. */
      if (MPFR_IS_POS (x))
        MPFR_FAST_COMPUTE_IF_SMALL_INPUT (y, x, - ex - 1, 0, 0, rnd_mode, {});
      else
        MPFR_FAST_COMPUTE_IF_SMALL_INPUT (y, x, - ex, 0, 1, rnd_mode, {});
    }

  comp = mpfr_cmp_si (x, -1);
  /* log1p(x) is undefined for x < -1 */
  if (MPFR_UNLIKELY(comp <= 0))
    {
      if (comp == 0)
        /* x=0: log1p(-1)=-inf (divide-by-zero exception) */
        {
          MPFR_SET_INF (y);
          MPFR_SET_NEG (y);
          MPFR_SET_DIVBY0 ();
          MPFR_RET (0);
        }
      MPFR_SET_NAN (y);
      MPFR_RET_NAN;
    }

  MPFR_SAVE_EXPO_MARK (expo);

  /* General case */
  {
    /* Declaration of the intermediary variable */
    mpfr_t t;
    /* Declaration of the size variable */
    mpfr_prec_t Ny = MPFR_PREC(y);             /* target precision */
    mpfr_prec_t Nt;                            /* working precision */
    mpfr_exp_t err;                            /* error */
    MPFR_ZIV_DECL (loop);

    /* compute the precision of intermediary variable */
    /* the optimal number of bits : see algorithms.tex */
    Nt = Ny + MPFR_INT_CEIL_LOG2 (Ny) + 6;

    /* if |x| is smaller than 2^(-e), we will loose about e bits
       in log(1+x) */
    if (MPFR_EXP(x) < 0)
      Nt += -MPFR_EXP(x);

    /* initialize of intermediary variable */
    mpfr_init2 (t, Nt);

    /* First computation of log1p */
    MPFR_ZIV_INIT (loop, Nt);
    for (;;)
      {
        int k;
        /* small case: assuming the AGM algorithm used by mpfr_log uses
           log2(p) steps for a precision of p bits, we try the special
           variant whenever EXP(x) <= -p/log2(p). */
        k = 1 + __gmpfr_int_ceil_log2 (Ny); /* the +1 avoids a division by 0
                                               when Ny=1 */
        if (MPFR_GET_EXP (x) + 1 <= - (mpfr_exp_t) (Ny / k))
          /* this implies EXP(x) <= -1 thus x < 1/2 */
          err = Nt - mpfr_log1p_small (t, x);
        else
          {
            /* compute log1p */
            inexact = mpfr_add_ui (t, x, 1, MPFR_RNDN);      /* 1+x */
            /* if inexact = 0, then t = x+1, and the result is simply log(t) */
            if (inexact == 0)
              {
                inexact = mpfr_log (y, t, rnd_mode);
                goto end;
              }
            mpfr_log (t, t, MPFR_RNDN);        /* log(1+x) */

            /* the error is bounded by (1/2+2^(1-EXP(t))*ulp(t)
               (cf algorithms.tex)
               if EXP(t)>=2, then error <= ulp(t)
               if EXP(t)<=1, then error <= 2^(2-EXP(t))*ulp(t) */
            err = Nt - MAX (0, 2 - MPFR_GET_EXP (t));
          }

        if (MPFR_LIKELY (MPFR_CAN_ROUND (t, err, Ny, rnd_mode)))
          break;

        /* increase the precision */
        MPFR_ZIV_NEXT (loop, Nt);
        mpfr_set_prec (t, Nt);
      }
    inexact = mpfr_set (y, t, rnd_mode);

  end:
    MPFR_ZIV_FREE (loop);
    mpfr_clear (t);
  }

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}
