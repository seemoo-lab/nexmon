/* mpfr_get_float128 -- convert a multiple precision floating-point
                        number to a binary128 (a.k.a. float128) number

Copyright 2012-2025 Free Software Foundation, Inc.
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

#ifdef MPFR_WANT_FLOAT128

/* Note: mpfr_get_float128 is a macro defined as the actual binary128 type:
   either _Float128 or __float128. */

/* generic code */
mpfr_float128
mpfr_get_float128 (mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    return (mpfr_float128) mpfr_get_d (x, rnd_mode);
  else /* now x is a normal non-zero number */
    {
      mpfr_float128 r; /* result */
      mpfr_float128 m;
      mpfr_exp_t e;  /* exponent of x (before rounding) */
      mpfr_exp_t sh; /* exponent shift, so that x/2^sh is in the double range */
      const int emin = -16381;
      const int esub = emin - IEEE_FLOAT128_MANT_DIG;
      int sign;

      sign = MPFR_SIGN (x);
      e = MPFR_GET_EXP (x);

      if (MPFR_UNLIKELY (e <= esub))
        {
          if (MPFR_IS_LIKE_RNDZ (rnd_mode, sign < 0) ||
              (rnd_mode == MPFR_RNDN && (e < esub || mpfr_powerof2_raw (x))))
            return sign < 0 ? -0.0 : 0.0;
          r = 1.0;
          sh = esub;
        }
      else
        {
          mpfr_t y;
          mp_limb_t *yp;
          int prec, i;  /* small enough to fit in an int */
          MPFR_SAVE_EXPO_DECL (expo);

          MPFR_SAVE_EXPO_MARK (expo);

          /* First round x to the target binary128 precision, taking the
             reduced precision of the subnormals into account, so that all
             subsequent operations are exact (this avoids double rounding
             problems). */
          prec = e < emin ? e - esub : IEEE_FLOAT128_MANT_DIG;
          MPFR_ASSERTD (prec >= MPFR_PREC_MIN);
          mpfr_init2 (y, prec);

          mpfr_set (y, x, rnd_mode);
          sh = MPFR_GET_EXP (y);
          MPFR_SET_EXP (y, 0);
          MPFR_SET_POS (y);
          yp = MPFR_MANT (y);

          r = 0.0;
          for (i = 0; i < MPFR_LIMB_SIZE (y); i++)
            {
              /* Note: MPFR_LIMB_MAX is avoided below as it might not
                 always work if GMP_NUMB_BITS > IEEE_FLOAT128_MANT_DIG.
                 MPFR_LIMB_HIGHBIT has the advantage to fit on 1 bit. */
              r += yp[i];
              r *= 1 / (2 * (mpfr_float128) MPFR_LIMB_HIGHBIT);
            }

          mpfr_clear (y);

          MPFR_SAVE_EXPO_FREE (expo);
        }

      /* we now have to multiply r by 2^sh */
      MPFR_ASSERTD (r > 0);
      if (sh != 0)
        {
          /* An overflow may occur (example: 0.5*2^1024) */
          while (r < 1.0)
            {
              r += r;
              sh--;
            }

          if (sh > 0)
            m = 2.0;
          else
            {
              m = 0.5;
              sh = -sh;
            }

          for (;;)
            {
              if (sh % 2)
                r = r * m;
              sh >>= 1;
              if (sh == 0)
                break;
              m = m * m;
            }
        }
      if (sign < 0)
        r = -r;
      return r;
    }
}

#endif /* MPFR_WANT_FLOAT128 */
