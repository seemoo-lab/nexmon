/* mpfr_div_d -- divide a multiple precision floating-point number
                 by a machine double precision float

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
mpfr_div_d (mpfr_ptr a, mpfr_srcptr b, double c, mpfr_rnd_t rnd_mode)
{
  int inexact;
  mpfr_t d;
  mp_limb_t tmp_man[MPFR_LIMBS_PER_DOUBLE];
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC (
    ("b[%Pd]=%.*Rg c%.20g rnd=%d", mpfr_get_prec (b), mpfr_log_prec, b, c,
     rnd_mode),
    ("a[%Pd]=%.*Rg", mpfr_get_prec (a), mpfr_log_prec, a));

  MPFR_SAVE_EXPO_MARK (expo);

  MPFR_TMP_INIT1(tmp_man, d, IEEE_DBL_MANT_DIG);
  inexact = mpfr_set_d (d, c, rnd_mode);
  MPFR_ASSERTD (inexact == 0);

  MPFR_CLEAR_FLAGS ();
  inexact = mpfr_div (a, b, d, rnd_mode);
  MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);

  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (a, inexact, rnd_mode);
}
