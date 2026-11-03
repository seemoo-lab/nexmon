/* mpfr_reldiff -- compute relative difference of two floating-point numbers.

Copyright 2000-2001, 2004-2025 Free Software Foundation, Inc.
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

/* reldiff(b, c) = abs(b-c)/b */
void
mpfr_reldiff (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mpfr_t b_copy;

  if (MPFR_ARE_SINGULAR (b, c))
    {
      if (MPFR_IS_NAN (b) || MPFR_IS_INF (b) || MPFR_IS_NAN (c) ||
          (MPFR_IS_ZERO (b) && MPFR_IS_ZERO (c)))
        {
          MPFR_SET_NAN (a);
          return;
        }
      if (MPFR_IS_ZERO (b) || MPFR_IS_INF (c))
        {
          MPFR_SET_SAME_SIGN (a, b);
          MPFR_SET_INF (a);
          return;
        }
      /* The case c = 0 with b regular, which should give sign(b) exactly,
         cannot be optimized here as it is documented in the MPFR manual
         that this function just computes abs(b-c)/b using the precision
         of a and the rounding mode rnd_mode for all operations. So let's
         prefer the potentially "incorrect" result. Note that the correct
         result is not necessarily better because if could break properties
         (like monotonicity?) implied by the documentation. */
    }

  if (a == b)
    {
      mpfr_init2 (b_copy, MPFR_PREC(b));
      mpfr_set (b_copy, b, MPFR_RNDN);
    }

  mpfr_sub (a, b, c, rnd_mode);
  MPFR_SET_SIGN (a, 1);
  mpfr_div (a, a, a == b ? b_copy : b, rnd_mode);

  if (a == b)
    mpfr_clear (b_copy);

}
