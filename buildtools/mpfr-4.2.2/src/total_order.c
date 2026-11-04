/* mpfr_total_order_p -- total order of two floating-point numbers

Copyright 2018-2025 Free Software Foundation, Inc.
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

/* follows IEEE 754-2008, Section 5.10 */
int
mpfr_total_order_p (mpfr_srcptr x, mpfr_srcptr y)
{
  if (MPFR_SIGN (x) != MPFR_SIGN (y))
    return MPFR_IS_POS (y);

  if (MPFR_IS_NAN (x))
    return MPFR_IS_NAN (y) || MPFR_IS_NEG (x);

  /* Now x is not NaN. */
  if (MPFR_IS_NAN (y))
    return MPFR_IS_POS (y); /* x < +NaN, x > -NaN */

  /* Now neither x nor y is NaN, and zeros of different sign have
     already been taken into account. */
  return mpfr_lessequal_p (x, y);
}
