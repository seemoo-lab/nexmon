/* mpfr_div_2ui -- divide a floating-point number by a power of two

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

#include "mpfr-impl.h"

int
mpfr_div_2ui (mpfr_ptr y, mpfr_srcptr x, unsigned long n, mpfr_rnd_t rnd_mode)
{
  int inexact;

  MPFR_LOG_FUNC (
    ("x[%Pd]=%.*Rg n=%lu rnd=%d", mpfr_get_prec(x), mpfr_log_prec, x, n,
     rnd_mode),
    ("y[%Pd]=%.*Rg inexact=%d", mpfr_get_prec(y), mpfr_log_prec, y, inexact));

  if (MPFR_UNLIKELY (n == 0 || MPFR_IS_SINGULAR (x)))
    return mpfr_set (y, x, rnd_mode);
  else
    {
      mpfr_exp_t exp = MPFR_GET_EXP (x);
      mpfr_uexp_t diffexp;

      MPFR_SETRAW (inexact, y, x, exp, rnd_mode);
      /* Warning! exp may have increased by 1 due to rounding. Thus the
         difference below may overflow in a mpfr_exp_t; but mpfr_uexp_t
         is OK to hold the value, with the difference done in unsigned
         integer arithmetic in this type. */
      diffexp = (mpfr_uexp_t) exp - (mpfr_uexp_t) (__gmpfr_emin - 1);
      if (MPFR_UNLIKELY (n >= diffexp))  /* exp - n <= emin - 1 */
        {
          if (rnd_mode == MPFR_RNDN &&
              (n > diffexp ||
               ((MPFR_IS_NEG (y) ? inexact <= 0 : inexact >= 0) &&
                mpfr_powerof2_raw (y))))
            rnd_mode = MPFR_RNDZ;
          return mpfr_underflow (y, rnd_mode, MPFR_SIGN (y));
        }
      /* Now, n < diffexp, i.e. n <= exp - emin, which a difference of
       * two valid exponents + 0 or 1, thus fits in a mpfr_exp_t (from
       * the constraints on valid exponents). Moreover, there cannot be
       * an overflow (if exp had been increased by 1 due to rounding)
       * since the case n = 0 has been filtered out.
       */
      MPFR_ASSERTD (n <= MPFR_EXP_MAX);
      MPFR_ASSERTD (n >= 1);
      MPFR_SET_EXP (y, exp - (mpfr_exp_t) n);
    }

  MPFR_RET (inexact);
}
