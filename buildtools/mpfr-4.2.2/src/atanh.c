/* mpfr_atanh -- Inverse Hyperbolic Tangente

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

/* Put in y an approximation of atanh(x) for x small.
   We assume x <= 1/2, in which case:
   x <= y ~ atanh(x) = x + x^3/3 + x^5/5 + x^7/7 + ... <= 2*x.
   Return k such that the error is bounded by 2^k*ulp(y).
*/
static int
mpfr_atanh_small (mpfr_ptr y, mpfr_srcptr x)
{
  mpfr_prec_t p = MPFR_PREC(y), err;
  mpfr_t x2, t, u;
  unsigned long i;
  int k;

  MPFR_ASSERTD(MPFR_GET_EXP (x) <= -1);

  /* in the following, theta represents a value with |theta| <= 2^(1-p)
     (might be a different value each time) */

  mpfr_init2 (t, p);
  mpfr_init2 (u, p);
  mpfr_init2 (x2, p);
  mpfr_set (t, x, MPFR_RNDF);  /* t = x * (1 + theta) */
  mpfr_set (y, t, MPFR_RNDF);  /* exact */
  mpfr_sqr (x2, x, MPFR_RNDF); /* x2 = x^2 * (1 + theta) */
  for (i = 3; ; i += 2)
    {
      mpfr_mul (t, t, x2, MPFR_RNDF);    /* t = x^i * (1 + theta)^i */
      mpfr_div_ui (u, t, i, MPFR_RNDF); /* u = x^i/i * (1 + theta)^(i+1) */
      if (MPFR_GET_EXP (u) <= MPFR_GET_EXP (y) - p) /* |u| < ulp(y) */
        break;
      mpfr_add (y, y, u, MPFR_RNDF); /* error <= ulp(y) */
    }
  /* We assume |(1 + theta)^(i+1)| <= 2.
     The neglected part is at most |u| + |u|/4 + |u|/16 + ... <= 4/3*|u|,
     which has to be multiplied by |(1 + theta)^(i+1)| <= 2, thus at most
     3 ulp(y).
     The rounding error on y is bounded by:
     * for the (i-3)/2 add/sub, each error is bounded by ulp(y_i),
       where y_i is the current value of y, which is bounded by ulp(y)
       for y the final value (since it increases in absolute value),
       this yields (i-3)/2*ulp(y)
     * from Lemma 3.1 from [Higham02] (see algorithms.tex),
       the relative error on u at step i is bounded by:
       (i+1)*epsilon/(1-(i+1)*epsilon) where epsilon = 2^(1-p).
       If (i+1)*epsilon <= 1/2, then the relative error on u at
       step i is bounded by 2*(i+1)*epsilon, and since |u| <= 1/2^(i+1)
       at step i, this gives an absolute error bound of;
       2*epsilon*x*(4/2^4 + 6/2^6 + 8/2^8 + ...) = 2*2^(1-p)*x*(7/18) =
       14/9*2^(-p)*x <= 2*ulp(x).

     If (i+1)*epsilon <= 1/2, then the relative error on u at step i
     is bounded by (i+1)*epsilon/(1-(i+1)*epsilon) <= 1, thus it follows
     |(1 + theta)^(i+1)| <= 2.

     Finally the total error is bounded by 3*ulp(y) + (i-3)/2*ulp(y) +2*ulp(x).
     Since x <= 2*y, we have ulp(x) <= 2*ulp(y), thus the error is bounded by:
     (i+7)/2*ulp(y).
  */
  err = (i + 8) / 2; /* ceil((i+7)/2) */
  k = __gmpfr_int_ceil_log2 (err);
  MPFR_ASSERTN(k + 2 < p);
  /* if k + 2 < p, since k = ceil(log2(err)), we have err <= 2^k <= 2^(p-3),
     thus i+7 <= 2*err <= 2^(p-2), thus (i+7)*epsilon <= 1/2, which implies
     our assumption (i+1)*epsilon <= 1/2. */
  mpfr_clear (t);
  mpfr_clear (u);
  mpfr_clear (x2);
  return k;
}

/* The computation of atanh is done by:
   atanh = ln((1+x)/(1-x)) / 2
   except when x is very small, in which case atanh = x + tiny error,
   and when x is small, where we use directly the Taylor expansion.
*/

int
mpfr_atanh (mpfr_ptr y, mpfr_srcptr xt, mpfr_rnd_t rnd_mode)
{
  int inexact;
  mpfr_t x, t, te;
  mpfr_prec_t Nx, Ny, Nt;
  mpfr_exp_t err;
  MPFR_ZIV_DECL (loop);
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (xt), mpfr_log_prec, xt, rnd_mode),
    ("y[%Pd]=%.*Rg inexact=%d",
     mpfr_get_prec (y), mpfr_log_prec, y, inexact));

  /* Special cases */
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (xt)))
    {
      /* atanh(NaN) = NaN, and atanh(+/-Inf) = NaN since tanh gives a result
         between -1 and 1 */
      if (MPFR_IS_NAN (xt) || MPFR_IS_INF (xt))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      else /* necessarily xt is 0 */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (xt));
          MPFR_SET_ZERO (y);   /* atanh(0) = 0 */
          MPFR_SET_SAME_SIGN (y,xt);
          MPFR_RET (0);
        }
    }

  /* atanh (x) = NaN as soon as |x| > 1, and arctanh(+/-1) = +/-Inf */
  if (MPFR_UNLIKELY (MPFR_GET_EXP (xt) > 0))
    {
      if (MPFR_GET_EXP (xt) == 1 && mpfr_powerof2_raw (xt))
        {
          MPFR_SET_INF (y);
          MPFR_SET_SAME_SIGN (y, xt);
          MPFR_SET_DIVBY0 ();
          MPFR_RET (0);
        }
      MPFR_SET_NAN (y);
      MPFR_RET_NAN;
    }

  /* atanh(x) = x + x^3/3 + ... so the error is < 2^(3*EXP(x)-1) */
  MPFR_FAST_COMPUTE_IF_SMALL_INPUT (y, xt, -2 * MPFR_GET_EXP (xt), 1, 1,
                                    rnd_mode, {});

  MPFR_SAVE_EXPO_MARK (expo);

  /* Compute initial precision */
  Nx = MPFR_PREC (xt);
  MPFR_TMP_INIT_ABS (x, xt);
  Ny = MPFR_PREC (y);
  Nt = MAX (Nx, Ny);
  Nt = Nt + MPFR_INT_CEIL_LOG2 (Nt) + 4;

  /* initialize of intermediary variable */
  mpfr_init2 (t, Nt);
  mpfr_init2 (te, Nt);

  MPFR_ZIV_INIT (loop, Nt);
  for (;;)
    {
      int k;

        /* small case: assuming the AGM algorithm used by mpfr_log uses
           log2(p) steps for a precision of p bits, we try the special
           variant whenever EXP(x) <= -p/log2(p). */
        k = 1 + __gmpfr_int_ceil_log2 (Ny); /* the +1 avoids a division by 0
                                               when Ny=1 */
        if (MPFR_GET_EXP (x) <= - 1 - (mpfr_exp_t) (Ny / k))
          /* this implies EXP(x) <= -1 thus x < 1/2 */
          {
            err = Nt - mpfr_atanh_small (t, x);
            goto round;
          }

      /* compute atanh */
      mpfr_ui_sub (te, 1, x, MPFR_RNDU);   /* (1-x) with x = |xt| */
      mpfr_add_ui (t, x, 1, MPFR_RNDD);    /* (1+x) */
      mpfr_div (t, t, te, MPFR_RNDN);      /* (1+x)/(1-x) */
      mpfr_log (t, t, MPFR_RNDN);          /* ln((1+x)/(1-x)) */
      mpfr_div_2ui (t, t, 1, MPFR_RNDN);   /* ln((1+x)/(1-x)) / 2 */

      /* error estimate: see algorithms.tex */
      /* FIXME: this does not correspond to the value in algorithms.tex!!! */
      /* err = Nt - __gmpfr_ceil_log2(1+5*pow(2,1-MPFR_EXP(t))); */
      err = Nt - (MAX (4 - MPFR_GET_EXP (t), 0) + 1);

    round:
      if (MPFR_LIKELY (MPFR_IS_ZERO (t)
                       || MPFR_CAN_ROUND (t, err, Ny, rnd_mode)))
        break;

      /* reactualisation of the precision */
      MPFR_ZIV_NEXT (loop, Nt);
      mpfr_set_prec (t, Nt);
      mpfr_set_prec (te, Nt);
    }
  MPFR_ZIV_FREE (loop);

  inexact = mpfr_set4 (y, t, rnd_mode, MPFR_SIGN (xt));

  mpfr_clear (t);
  mpfr_clear (te);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}
