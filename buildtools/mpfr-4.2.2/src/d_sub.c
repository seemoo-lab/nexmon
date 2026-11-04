/* mpfr_d_sub -- subtract a multiple precision floating-point number
                 from a machine double precision float

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

int
mpfr_d_sub (mpfr_ptr a, double b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  int inexact;
  mpfr_t d;
  mp_limb_t tmp_man[MPFR_LIMBS_PER_DOUBLE];
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC (
    ("b=%.20g c[%Pd]=%.*Rg rnd=%d", b, mpfr_get_prec (c), mpfr_log_prec, c, rnd_mode),
    ("a[%Pd]=%.*Rg", mpfr_get_prec (a), mpfr_log_prec, a));

  MPFR_SAVE_EXPO_MARK (expo);

  MPFR_TMP_INIT1(tmp_man, d, IEEE_DBL_MANT_DIG);
  inexact = mpfr_set_d (d, b, rnd_mode);
  MPFR_ASSERTD (inexact == 0);

  MPFR_CLEAR_FLAGS ();
  inexact = mpfr_sub (a, d, c, rnd_mode);
  MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (a, inexact, rnd_mode);
}
