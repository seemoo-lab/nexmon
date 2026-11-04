/* mpfr_copysign -- Produce a value with the magnitude of x and sign bit of y

Copyright 2001-2004, 2006-2025 Free Software Foundation, Inc.
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

 /*
   The computation of z with magnitude of x and sign of y:
   z = (-1)^signbit(y) * abs(x), i.e. with the same sign bit as y,
   even if z is a NaN.
   Note: This function implements copysign from the IEEE-754 standard
   when no rounding occurs (e.g. if PREC(z) >= PREC(x)).
 */

#undef mpfr_copysign
int
mpfr_copysign (mpfr_ptr z, mpfr_srcptr x, mpfr_srcptr y, mpfr_rnd_t rnd_mode)
{
  if (MPFR_UNLIKELY (z != x))
    return mpfr_set4 (z, x, rnd_mode, MPFR_SIGN (y));
  else
    {
      MPFR_SET_SAME_SIGN (z, y);
      if (MPFR_UNLIKELY (MPFR_IS_NAN (x)))
        MPFR_RET_NAN;
      else
        MPFR_RET (0);
    }
}
