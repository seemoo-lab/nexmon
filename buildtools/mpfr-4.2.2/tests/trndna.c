/* Test file for mpfr_round_nearest_away.

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

#include "mpfr-test.h"

static void
test_special (void)
{
  mpfr_t x, y;
  int inex;

  mpfr_init2 (x, MPFR_PREC_MIN);
  mpfr_init2 (y, MPFR_PREC_MIN);

  mpfr_set_nan (x);
  inex = mpfr_round_nearest_away (mpfr_sin, y, x);
  if (inex != 0)
    {
      printf ("Wrong ternary value for sin(NaN)\n");
      exit (1);
    }
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Wrong output for sin(NaN)\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  inex = mpfr_round_nearest_away (mpfr_exp, y, x);
  if (inex != 0)
    {
      printf ("Wrong ternary value for exp(+Inf)\n");
      printf ("expected 0, got %d\n", inex);
      exit (1);
    }
  if (mpfr_inf_p (y) == 0 || mpfr_sgn (y) <= 0)
    {
      printf ("Wrong output for exp(+Inf)\n");
      exit (1);
    }

  mpfr_set_inf (x, -1);
  inex = mpfr_round_nearest_away (mpfr_cbrt, y, x);
  if (inex != 0)
    {
      printf ("Wrong ternary value for cbrt(-Inf)\n");
      exit (1);
    }
  if (mpfr_inf_p (y) == 0 || mpfr_sgn (y) >= 0)
    {
      printf ("Wrong output for cbrt(-Inf)\n");
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
test_nonspecial (void)
{
  mpfr_t x, y;
  int inex;

  mpfr_init2 (x, 10);
  mpfr_init2 (y, 10);

  /* case where the computation on n+1 bits ends with a '0' */
  mpfr_set_ui (x, 2, MPFR_RNDN);
  inex = mpfr_round_nearest_away (mpfr_sin, y, x);
  if (inex >= 0)
    {
      printf ("Wrong ternary value for sin(2)\n");
      exit (1);
    }
  if (mpfr_cmp_ui_2exp (y, 931, -10) != 0)
    {
      printf ("Wrong output for sin(2)\n");
      exit (1);
    }

  /* case where the computation on n+1 bits ends with a '1' and is exact */
  mpfr_set_ui (x, 37, MPFR_RNDN);
  inex = mpfr_round_nearest_away (mpfr_sqr, y, x);
  if (inex <= 0)
    {
      printf ("Wrong ternary value for sqr(37)\n");
      exit (1);
    }
  if (mpfr_cmp_ui (y, 1370) != 0)
    {
      printf ("Wrong output for sqr(37)\n");
      exit (1);
    }

  /* case where the computation on n+1 bits ends with a '1' but is inexact */
  mpfr_set_ui (x, 91, MPFR_RNDN);
  inex = mpfr_round_nearest_away (mpfr_sqr, y, x);
  if (inex <= 0)
    {
      printf ("Wrong ternary value for sqr(91)\n");
      exit (1);
    }
  if (mpfr_cmp_ui (y, 8288) != 0)
    {
      printf ("Wrong output for sqr(91)\n");
      exit (1);
    }

  mpfr_set_ui (x, 131, MPFR_RNDN);
  inex = mpfr_round_nearest_away (mpfr_sqr, y, x);
  if (inex >= 0)
    {
      printf ("Wrong ternary value for sqr(131)\n");
      exit (1);
    }
  if (mpfr_cmp_ui (y, 17152) != 0)
    {
      printf ("Wrong output for sqr(131)\n");
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
}

int
main (int argc, char *argv[])
{
  mpfr_exp_t emin;

  tests_start_mpfr ();

  /* mpfr_round_nearest_away requires emin is not the smallest possible */
  if ((emin = mpfr_get_emin ()) == mpfr_get_emin_min ())
    set_emin (mpfr_get_emin_min () + 1);

  test_special ();

  test_nonspecial ();

  set_emin (emin);

  tests_end_mpfr ();
  return 0;
}

