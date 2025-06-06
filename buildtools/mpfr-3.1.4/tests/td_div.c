/* Test file for mpfr_d_div

Copyright 2007-2016 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

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
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "mpfr-test.h"

#if MPFR_VERSION >= MPFR_VERSION_NUM(2,4,0)

static void
check_nans (void)
{
  mpfr_t  x, y;
  int inexact;

  mpfr_init2 (x, 123);
  mpfr_init2 (y, 123);

  /* 1.0 / nan is nan */
  mpfr_set_nan (x);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, 1.0, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);
  MPFR_ASSERTN (mpfr_nan_p (y));

  /* 1.0 / +inf == +0 */
  mpfr_set_inf (x, 1);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, 1.0, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  MPFR_ASSERTN (mpfr_zero_p (y));
  MPFR_ASSERTN (MPFR_IS_POS (y));

  /* 1.0 / -inf == -0 */
  mpfr_set_inf (x, -1);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, 1.0, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  MPFR_ASSERTN (mpfr_zero_p (y));
  MPFR_ASSERTN (MPFR_IS_NEG (y));

#if !defined(MPFR_ERRDIVZERO)

  /* 1.0 / 0 == +inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, 1.0, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_POS (y));

  /* -1.0 / 0 == -inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, -1.0, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_NEG (y));

  /* 1.0 / -0 == -inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, 1.0, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_NEG (y));

  /* -1.0 / -0 == +inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, -1.0, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_POS (y));

  /* +inf / 0 == +inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, DBL_POS_INF, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_POS (y));

  /* -inf / 0 == -inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, DBL_NEG_INF, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_NEG (y));

  /* +inf / -0 == -inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, DBL_POS_INF, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_NEG (y));

  /* -inf / -0 == +inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN);
  mpfr_clear_flags ();
  inexact = mpfr_d_div (y, DBL_NEG_INF, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);
  MPFR_ASSERTN (mpfr_inf_p (y));
  MPFR_ASSERTN (MPFR_IS_POS (y));

#endif

  mpfr_clear (x);
  mpfr_clear (y);
}

#define TEST_FUNCTION mpfr_d_div
#define DOUBLE_ARG1
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), 1, RANDS)
#include "tgeneric.c"

int
main (void)
{
  mpfr_t x, y, z;
  double d;
  int inexact;

  tests_start_mpfr ();

  /* check with enough precision */
  mpfr_init2 (x, IEEE_DBL_MANT_DIG);
  mpfr_init2 (y, IEEE_DBL_MANT_DIG);
  mpfr_init2 (z, IEEE_DBL_MANT_DIG);

  mpfr_set_str (y, "4096", 10, MPFR_RNDN);
  d = 0.125;
  mpfr_clear_flags ();
  inexact = mpfr_d_div (x, d, y, MPFR_RNDN);
  if (inexact != 0)
    {
      printf ("Inexact flag error in mpfr_d_div\n");
      exit (1);
    }
  mpfr_set_str (z, " 0.000030517578125", 10, MPFR_RNDN);
  if (mpfr_cmp (z, x))
    {
      printf ("Error in mpfr_d_div (");
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

  test_generic (2, 1000, 100);

  tests_end_mpfr ();
  return 0;
}

#else

int
main (void)
{
  printf ("Warning! Test disabled for this MPFR version.\n");
  return 0;
}

#endif
