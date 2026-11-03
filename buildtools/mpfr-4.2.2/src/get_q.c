/* mpfr_get_q -- get a multiple-precision rational from
                 a floating-point number

Copyright 2004, 2006-2025 Free Software Foundation, Inc.
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

#ifndef MPFR_USE_MINI_GMP
/* part of the code was copied from get_z.c */
void
mpfr_get_q (mpq_ptr q, mpfr_srcptr f)
{
  mpfr_exp_t exp;
  mpz_ptr u = mpq_numref (q);
  mpz_ptr v = mpq_denref (q);

  /* v is set to 1 and will not be changed directly.
     This ensures that q will be canonical. */
  mpz_set_ui (v, 1);

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (f)))
    {
      if (MPFR_UNLIKELY (MPFR_NOTZERO (f)))
        MPFR_SET_ERANGEFLAG ();
      mpz_set_ui (u, 0);
      /* The ternary value is 0 even for infinity. Giving the rounding
         direction in this case would not make much sense anyway, and
         the direction would not necessarily match rnd. */
    }
  else
    {
      exp = mpfr_get_z_2exp (u, f);
      if (exp >= 0)
        {
          MPFR_ASSERTN (exp <= (mp_bitcnt_t) -1);
          mpz_mul_2exp (u, u, exp);
        }
      else  /* exp < 0 */
        {
          MPFR_ASSERTN (-exp <= (mp_bitcnt_t) -1);
          mpq_div_2exp (q, q, -exp);
        }
    }
}
#endif
