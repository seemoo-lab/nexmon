/* mpfr_cmpabs_ui -- compare the absolute value of FP to an unsigned long

Copyright 2020-2025 Free Software Foundation, Inc.
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

/* Return a positive value if abs(b) > c, 0 if abs(b) = c, and
   a negative value if abs(b) < c. */

int
mpfr_cmpabs_ui (mpfr_srcptr b, unsigned long c)
{
  mpfr_t absb;

  MPFR_TMP_INIT_ABS (absb, b);
  return mpfr_cmp_ui (absb, c);
}
