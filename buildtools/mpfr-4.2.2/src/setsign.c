/* mpfr_setsign -- Produce a value with the magnitude of x and sign bit s

Copyright 2007-2025 Free Software Foundation, Inc.
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

#undef mpfr_setsign
int
mpfr_setsign (mpfr_ptr z, mpfr_srcptr x, int s, mpfr_rnd_t rnd_mode)
{
  if (MPFR_UNLIKELY (z != x))
    return mpfr_set4 (z, x, rnd_mode, s ? MPFR_SIGN_NEG : MPFR_SIGN_POS);
  else
    {
      MPFR_SET_SIGN (z, s ? MPFR_SIGN_NEG : MPFR_SIGN_POS);
      if (MPFR_UNLIKELY (MPFR_IS_NAN (x)))
        MPFR_RET_NAN;
      else
        MPFR_RET (0);
    }
}
