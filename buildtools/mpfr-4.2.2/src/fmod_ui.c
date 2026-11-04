/* mpfr_fmod_ui -- modulo a machine integer

Copyright 2000-2004, 2006-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

int
mpfr_fmod_ui (mpfr_ptr r, mpfr_srcptr x, unsigned long u, mpfr_rnd_t rnd_mode)
{
  int inex;

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg u=%lu rnd=%d",
      mpfr_get_prec(x), mpfr_log_prec, x, u, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec(r), mpfr_log_prec, r, inex));

  if (MPFR_UNLIKELY (u != 0))
    {
      mpfr_t uu;
#ifdef MPFR_LONG_WITHIN_LIMB
      mp_limb_t up[1];
      int cnt;
      MPFR_SAVE_EXPO_DECL (expo);

      MPFR_TMP_INIT1 (up, uu, GMP_NUMB_BITS);
      MPFR_ASSERTN (u == (mp_limb_t) u);
      count_leading_zeros (cnt, (mp_limb_t) u);
      *up = (mp_limb_t) u << cnt;

      MPFR_SAVE_EXPO_MARK (expo);
      MPFR_SET_EXP (uu, GMP_NUMB_BITS - cnt);
      inex = mpfr_fmod (r, x, uu, rnd_mode);
#else
      MPFR_SAVE_EXPO_DECL (expo);

      mpfr_init2 (uu, sizeof (unsigned long) * CHAR_BIT);
      /* Warning: u might be outside the current exponent range! */
      MPFR_SAVE_EXPO_MARK (expo);
      mpfr_set_ui (uu, u, MPFR_RNDZ);
      inex = mpfr_fmod (r, x, uu, rnd_mode);
      mpfr_clear (uu);
#endif /* MPFR_LONG_WITHIN_LIMB */
      MPFR_SAVE_EXPO_FREE (expo);
      inex = mpfr_check_range (r, inex, rnd_mode);
      return inex;
    }
  else
    {
      MPFR_SET_NAN (r);
      MPFR_RET_NAN;
    }
}
