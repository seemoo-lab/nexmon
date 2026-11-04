/* mpfr_sub_ui -- subtract a floating-point number and a machine integer

Copyright 2000-2025 Free Software Foundation, Inc.
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
mpfr_sub_ui (mpfr_ptr y, mpfr_srcptr x, unsigned long int u, mpfr_rnd_t rnd_mode)
{
  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg u=%lu rnd=%d",
      mpfr_get_prec(x), mpfr_log_prec, x, u, rnd_mode),
     ("y[%Pd]=%.*Rg", mpfr_get_prec (y), mpfr_log_prec, y));

  /* (unsigned long) 0 is assumed to be a real 0 (unsigned) */
  if (MPFR_UNLIKELY (u == 0))
    return mpfr_set (y, x, rnd_mode);

  /* This block is actually useless, but this is a minor optimization. */
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      if (MPFR_IS_INF (x))
        {
          MPFR_SET_INF (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0); /* +/-infinity is exact */
        }
      /* The case x being zero is handled below: we can't use mpfr_set_si
         as the opposite of u does not necessarily fit in a long. */
    }

  /* Main code */
  {
    int inex;
    MPFR_SAVE_EXPO_DECL (expo);

    /* Optimization note: Exponent save/restore operations may be
       removed if mpfr_sub works even when uu is out-of-range. */
    MPFR_SAVE_EXPO_MARK (expo);

#ifdef MPFR_LONG_WITHIN_LIMB
    {
      mpfr_t uu;
      mp_limb_t up[1];
      int cnt;

      MPFR_TMP_INIT1 (up, uu, GMP_NUMB_BITS);
      /* So, u fits in a mp_limb_t, which justifies the casts below. */
      MPFR_ASSERTD (u != 0);
      count_leading_zeros (cnt, (mp_limb_t) u);
      up[0] = (mp_limb_t) u << cnt;
      MPFR_SET_EXP (uu, GMP_NUMB_BITS - cnt);
      inex = mpfr_sub (y, x, uu, rnd_mode);
    }
#else
    {
      mpfr_t uu;

      mpfr_init2 (uu, sizeof (unsigned long) * CHAR_BIT);
      mpfr_set_ui (uu, u, MPFR_RNDZ);
      inex = mpfr_sub (y, x, uu, rnd_mode);
      mpfr_clear (uu);
    }
#endif

    MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);
    MPFR_SAVE_EXPO_FREE (expo);
    return mpfr_check_range (y, inex, rnd_mode);
  }
}
