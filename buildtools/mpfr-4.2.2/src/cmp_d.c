/* mpfr_cmp_d -- compare a floating-point number with a double

Copyright 2003-2004, 2006-2025 Free Software Foundation, Inc.
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
mpfr_cmp_d (mpfr_srcptr b, double d)
{
  mpfr_t tmp;
  int res;
  mp_limb_t tmp_man[MPFR_LIMBS_PER_DOUBLE];
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_SAVE_EXPO_MARK (expo);

  MPFR_TMP_INIT1(tmp_man, tmp, IEEE_DBL_MANT_DIG);
  res = mpfr_set_d (tmp, d, MPFR_RNDN);
  MPFR_ASSERTD (res == 0);

  MPFR_CLEAR_FLAGS ();
  res = mpfr_cmp (b, tmp);
  MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);

  MPFR_SAVE_EXPO_FREE (expo);
  return res;
}
