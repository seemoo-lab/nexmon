/* Test file for mpfr_mul_d

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

#include <float.h>

#include "mpfr-test.h"

static void
check_nans (void)
{
  mpfr_t  x, y;
  int inexact;

  mpfr_init2 (x, 123);
  mpfr_init2 (y, 123);

  /* nan * 1.0 is nan */
  mpfr_set_nan (x);
  mpfr_clear_flags();
  inexact = mpfr_mul_d (y, x, 1.0, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);
  MPFR_ASSERTN (mpfr_nan_p (y));

  /* +inf * 1.0 == +inf */
  mpfr_set_inf (x, 1);
  mpfr_clear_flags();
  inexact = mpfr_mul_d (y, x, 1.0, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_POS (y));

  /* +inf * 0.0 is nan */
  mpfr_clear_flags();
  inexact = mpfr_mul_d (y, x, 0.0, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);
  MPFR_ASSERTN (mpfr_nan_p (y));

  /* -inf * 1.0 == -inf */
  mpfr_set_inf (x, -1);
  mpfr_clear_flags();
  inexact = mpfr_mul_d (y, x, 1.0, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_NEG (y));

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
bug20171218 (void)
{
  mpfr_t a, b;
  mpfr_init2 (a, 22);
  mpfr_init2 (b, 1312);
  mpfr_set_str (b, "1.ffffffffffffffffffc3e0007ffe000700fff8001fff800000000007e0fffe0007fffffff00001f800000003ffffffe1fc00000003fffe7c03ffffffffffffffe000000fffffffff83f0000007ffc03ffffffffc0007ff0000ffcfffffe00000f80000000003c007ffffffff3ff807ffffffffff000000000000001fc000fffe000600000ff0003ffe00fffffffc00001ffc0fffffffffff00000807fe03ffffffc01ffe", 16, MPFR_RNDN);
  mpfr_mul_d (a, b, 0.5, MPFR_RNDF);
  /* a should be 1 or nextbelow(1) */
  if (mpfr_cmp_ui (a, 1) != 0)
    {
      mpfr_nextabove (a);
      MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);
    }
  mpfr_clear (a);
  mpfr_clear (b);
}

#define TEST_FUNCTION mpfr_mul_d
#define DOUBLE_ARG2
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), 1, RANDS)
#include "tgeneric.c"

int
main (void)
{
  mpfr_t x, y, z;
  double d;
  int inexact;

  tests_start_mpfr ();

  bug20171218 ();

  /* check with enough precision */
  mpfr_init2 (x, IEEE_DBL_MANT_DIG);
  mpfr_init2 (y, IEEE_DBL_MANT_DIG);
  mpfr_init2 (z, IEEE_DBL_MANT_DIG);

  mpfr_set_str (y, "4096", 10, MPFR_RNDN);
  d = 0.125;
  mpfr_clear_flags ();
  inexact = mpfr_mul_d (x, y, d, MPFR_RNDN);
  if (inexact != 0)
    {
      printf ("Inexact flag error in mpfr_mul_d\n");
      exit (1);
    }
  mpfr_set_str (z, "512", 10, MPFR_RNDN);
  if (mpfr_cmp (z, x))
    {
      printf ("Error in mpfr_mul_d (");
      mpfr_out_str (stdout, 10, 0, y, MPFR_RNDN);
      printf (" + %.20g)\nexpected ", d);
      mpfr_out_str (stdout, 10, 0, z, MPFR_RNDN);
      printf ("\ngot     ");
      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  mpfr_clears (x, y, z, (mpfr_ptr) 0);

  check_nans ();

  test_generic (MPFR_PREC_MIN, 1000, 100);

  tests_end_mpfr ();
  return 0;
}
