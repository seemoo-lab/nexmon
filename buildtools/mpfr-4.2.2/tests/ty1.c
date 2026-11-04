/* ty1 -- test file for the Bessel function of second kind (order 1)

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

#include "mpfr-test.h"

#define TEST_FUNCTION mpfr_y1
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), 8, RANDS)
#define REDUCE_EMAX 262143 /* otherwise arg. reduction is too expensive */
#include "tgeneric.c"

int
main (int argc, char *argv[])
{
  mpfr_t x, y;

  tests_start_mpfr ();

  mpfr_init (x);
  mpfr_init (y);

  /* special values */
  mpfr_set_nan (x);
  mpfr_y1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  mpfr_set_inf (x, 1); /* +Inf */
  mpfr_y1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 0) == 0 && MPFR_IS_POS (y));

  mpfr_set_inf (x, -1); /* -Inf */
  mpfr_y1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  mpfr_set_ui (x, 0, MPFR_RNDN); /* +0 */
  mpfr_y1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (y) && MPFR_IS_NEG (y)); /* y1(+0)=-Inf */

  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN); /* -0 */
  mpfr_y1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (y) && MPFR_IS_NEG (y)); /* y1(-0)=-Inf */

  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);

  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_y1 (y, x, MPFR_RNDN);
  mpfr_set_str_binary (x, "-0.110001111111110110010000001111101011001101011100101");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_y1 for x=1, rnd=MPFR_RNDN\n");
      printf ("Expected "); mpfr_dump (x);
      printf ("Got      "); mpfr_dump (y);
      exit (1);
    }

  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_y1 (y, x, MPFR_RNDN);
  if (!mpfr_nan_p (y))
    {
      printf ("Error in mpfr_y1 for x=-1, rnd=MPFR_RNDN\n");
      printf ("Expected NaN\n");
      printf ("Got      "); mpfr_dump (y);
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);

  test_generic (MPFR_PREC_MIN, 100, 1);

  data_check ("data/y1", mpfr_y1, "mpfr_y1");

  tests_end_mpfr ();

  return 0;
}
