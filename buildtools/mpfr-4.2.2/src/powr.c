/* mpfr_powr -- powr(x,y) = exp(y*log(x))

Copyright 2021-2025 Free Software Foundation, Inc.
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
mpfr_powr (mpfr_ptr z, mpfr_srcptr x, mpfr_srcptr y, mpfr_rnd_t rnd_mode)
{
  /* powr(x,y) is NaN for x=NaN or x < 0 (a)
     powr(+/-0,+/-0) is NaN whereas pow(x,+/-0) = 1 if x is not NaN (b)
     powr(+Inf,+/-0) is NaN whereas pow(x,+/-0) = 1 if x is not NaN (b)
     We can use MPFR_IS_SINGULAR since we already excluded x=NaN and x=-Inf.
     powr(1,+/-Inf) = NaN whereas pow(1,+/-Inf) = 1 (c) */
  if (MPFR_IS_NAN (x) || MPFR_IS_STRICTNEG(x) ||      /* case (a) */
      (MPFR_IS_SINGULAR(x) && MPFR_IS_ZERO(y)) ||     /* case (b) */
      (mpfr_cmp_ui (x, 1) == 0 && MPFR_IS_INF(y)))    /* case (c) */
    {
      MPFR_SET_NAN (z);
      MPFR_RET_NAN;
    }
  /* for finite y < 0, powr(-0,y) is +Inf, whereas pow(-0,y) is -Inf for y an
     odd negativeinteger, and +Inf otherwise */
  if (MPFR_IS_ZERO(x) && MPFR_SIGN(x) < 0 && !MPFR_IS_SINGULAR (y)
      && MPFR_IS_NEG(y))
    {
      MPFR_SET_INF (z);
      MPFR_SET_POS (z);
      MPFR_SET_DIVBY0 ();
      MPFR_RET (0);
    }
  /* for y > 0, powr(-0,y) is +0, whereas pow gives -0 for y an odd integer */
  if (MPFR_IS_ZERO(x) && MPFR_SIGN(x) < 0 && !MPFR_IS_SINGULAR (y)
      && MPFR_IS_POS(y))
    {
      MPFR_SET_ZERO (z);
      MPFR_SET_POS (z);
      MPFR_RET (0);
    }
  /* otherwise powr coincides with pow */
  return mpfr_pow (z, x, y, rnd_mode);
}
