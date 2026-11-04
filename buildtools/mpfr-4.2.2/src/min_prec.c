/* mpfr_min_prec -- minimal size in bits to hold the mantissa

Copyright 2009-2025 Free Software Foundation, Inc.
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

mpfr_prec_t
mpfr_min_prec (mpfr_srcptr x)
{
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    return 0;

  /* from a suggestion by Andreas Enge (2010-11-18) */
  return MPFR_LIMB_SIZE (x) * GMP_NUMB_BITS - mpn_scan1 (MPFR_MANT (x), 0);
}
