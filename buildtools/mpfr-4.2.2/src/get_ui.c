/* mpfr_get_ui -- convert a floating-point number to an unsigned long.

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

unsigned long
mpfr_get_ui (mpfr_srcptr f, mpfr_rnd_t rnd)
{
  mpfr_prec_t prec;
  unsigned long s;
  mpfr_t x;
  mp_size_t n;
  mpfr_exp_t exp;
  MPFR_SAVE_EXPO_DECL (expo);

  if (MPFR_UNLIKELY (!mpfr_fits_ulong_p (f, rnd)))
    {
      MPFR_SET_ERANGEFLAG ();
      return MPFR_IS_NAN (f) || MPFR_IS_NEG (f) ?
        (unsigned long) 0 : ULONG_MAX;
    }

  if (MPFR_IS_ZERO (f))
    return (unsigned long) 0;

  for (s = ULONG_MAX, prec = 0; s != 0; s /= 2, prec ++)
    { }

  MPFR_SAVE_EXPO_MARK (expo);

  /* first round to prec bits */
  mpfr_init2 (x, prec);
  mpfr_rint (x, f, rnd);

  /* The flags from mpfr_rint are the wanted ones. In particular,
     it sets the inexact flag when necessary. */
  MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);

  /* warning: if x=0, taking its exponent is illegal */
  if (MPFR_NOTZERO (x))
    {
      exp = MPFR_GET_EXP (x);
      MPFR_ASSERTD (exp >= 1); /* since |x| >= 1 */
      n = MPFR_LIMB_SIZE (x);
#ifdef MPFR_LONG_WITHIN_LIMB
      MPFR_ASSERTD (exp <= GMP_NUMB_BITS);
#else
      while (exp > GMP_NUMB_BITS)
        {
          MPFR_ASSERTD (n > 0);
          s += (unsigned long) MPFR_MANT(x)[n - 1] << (exp - GMP_NUMB_BITS);
          n--;
          exp -= GMP_NUMB_BITS;
        }
#endif
      MPFR_ASSERTD (n > 0);
      s += MPFR_MANT(x)[n - 1] >> (GMP_NUMB_BITS - exp);
    }

  mpfr_clear (x);

  MPFR_SAVE_EXPO_FREE (expo);

  return s;
}
