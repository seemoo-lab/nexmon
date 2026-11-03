/* mpfr_mul_2ui -- multiply a floating-point number by a power of two

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
mpfr_mul_2ui (mpfr_ptr y, mpfr_srcptr x, unsigned long int n, mpfr_rnd_t rnd_mode)
{
  int inexact;

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg n=%lu rnd=%d",
      mpfr_get_prec (x), mpfr_log_prec, x, n, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec (y), mpfr_log_prec, y, inexact));

  inexact = (y != x) ? mpfr_set (y, x, rnd_mode) : 0;

  if (MPFR_LIKELY (MPFR_IS_PURE_FP (y)))
    {
      mpfr_exp_t exp = MPFR_GET_EXP (y);
      mpfr_exp_t diffexp;

      diffexp = __gmpfr_emax - exp;  /* diff of two valid exponents */
      if (MPFR_UNLIKELY (n > diffexp))  /* exp + n > emax */
        return mpfr_overflow (y, rnd_mode, MPFR_SIGN (y));
      /* Now, 0 <= n <= diffexp, thus n fits in a mpfr_exp_t,
         and exp + n <= emax (no integer overflow). */
      MPFR_SET_EXP (y, exp + (mpfr_exp_t) n);
    }

  return inexact;
}
