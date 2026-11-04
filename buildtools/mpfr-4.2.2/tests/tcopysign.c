/* Test file for mpfr_copysign, mpfr_setsign and mpfr_signbit.

Copyright 2004, 2006-2025 Free Software Foundation, Inc.
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

#include "mpfr-test.h"

static void
copysign_variant (mpfr_ptr z, mpfr_srcptr x, mpfr_srcptr y,
                  mpfr_rnd_t rnd_mode, int k)
{
  mpfr_srcptr p;
  int a = 0, b = 0, c = 0;

  /* invalid sign, to test that the sign is always correctly set */
  MPFR_SIGN (z) = 0;

  if (k >= 8)
    {
      MPFR_ASSERTN (MPFR_PREC (z) >= MPFR_PREC (x));
      mpfr_set (z, x, MPFR_RNDN);
      p = z;
      k -= 8;
    }
  else
    p = x;

  mpfr_clear_flags ();
  switch (k)
    {
    case 0:
      mpfr_copysign (z, p, y, rnd_mode);
      return;
    case 1:
      (mpfr_copysign) (z, p, y, rnd_mode);
      return;
    case 2:
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
      mpfr_copysign ((a++, VOIDP_CAST(z)),
                     (b++, VOIDP_CAST(p)),
                     (c++, VOIDP_CAST(y)), rnd_mode);
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
      MPFR_ASSERTN (a == 1);
      MPFR_ASSERTN (b == 1);
      MPFR_ASSERTN (c == 1);
      return;
    case 3:
      mpfr_setsign (z, p, mpfr_signbit (y), rnd_mode);
      return;
    case 4:
      mpfr_setsign (z, p, (mpfr_signbit) (y), rnd_mode);
      return;
    case 5:
      (mpfr_setsign) (z, p, mpfr_signbit (y), rnd_mode);
      return;
    case 6:
      (mpfr_setsign) (z, p, (mpfr_signbit) (y), rnd_mode);
      return;
    case 7:
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
      mpfr_setsign ((a++, VOIDP_CAST(z)),
                    (b++, VOIDP_CAST(p)),
                    mpfr_signbit ((c++, VOIDP_CAST(y))), rnd_mode);
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
      MPFR_ASSERTN (a == 1);
      MPFR_ASSERTN (b == 1);
      MPFR_ASSERTN (c == 1);
      return;
    }
}

int
main (void)
{
  mpfr_t x, y, z;
  int i, j, k;

  tests_start_mpfr ();

  mpfr_init (x);
  mpfr_init (y);
  mpfr_init (z);

  for (i = 0; i <= 1; i++)
    for (j = 0; j <= 1; j++)
      for (k = 0; k < 16; k++)
        {
          mpfr_set_nan (x);
          i ? MPFR_SET_NEG (x) : MPFR_SET_POS (x);
          mpfr_set_nan (y);
          j ? MPFR_SET_NEG (y) : MPFR_SET_POS (y);
          copysign_variant (z, x, y, MPFR_RNDN, k);
          if (MPFR_SIGN (z) != MPFR_SIGN (y) || !mpfr_nanflag_p ())
            {
              printf ("Error in mpfr_copysign (%cNaN, %cNaN)\n",
                      i ? '-' : '+', j ? '-' : '+');
              exit (1);
            }

          mpfr_set_si (x, i ? -1250 : 1250, MPFR_RNDN);
          mpfr_set_nan (y);
          j ? MPFR_SET_NEG (y) : MPFR_SET_POS (y);
          copysign_variant (z, x, y, MPFR_RNDN, k);
          if (i != j)
            mpfr_neg (x, x, MPFR_RNDN);
          if (! mpfr_equal_p (z, x) || mpfr_nanflag_p ())
            {
              printf ("Error in mpfr_copysign (%c1250, %cNaN)\n",
                      i ? '-' : '+', j ? '-' : '+');
              exit (1);
            }

          mpfr_set_si (x, i ? -1250 : 1250, MPFR_RNDN);
          mpfr_set_si (y, j ? -1717 : 1717, MPFR_RNDN);
          copysign_variant (z, x, y, MPFR_RNDN, k);
          if (i != j)
            mpfr_neg (x, x, MPFR_RNDN);
          if (! mpfr_equal_p (z, x) || mpfr_nanflag_p ())
            {
              printf ("Error in mpfr_copysign (%c1250, %c1717)\n",
                      i ? '-' : '+', j ? '-' : '+');
              exit (1);
            }
        }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);

  tests_end_mpfr ();
  return 0;
}
