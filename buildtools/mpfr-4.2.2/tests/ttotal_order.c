/* Test file for mpfr_total_order_p.

Copyright 2018-2025 Free Software Foundation, Inc.
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
check1 (mpfr_ptr x, mpfr_ptr y)
{
  if (! mpfr_total_order_p (x, x))
    {
      printf ("Error on mpfr_total_order_p (x, x) with\n");
      printf ("x = ");
      mpfr_dump (x);
      exit (1);
    }

  mpfr_set (y, x, MPFR_RNDN);

  if (! mpfr_total_order_p (x, y))
    {
      printf ("Error on mpfr_total_order_p (x, y) with\n");
      printf ("x = ");
      mpfr_dump (x);
      printf ("y = ");
      mpfr_dump (y);
      exit (1);
    }
}

static void
check2 (mpfr_ptr x, mpfr_ptr y)
{
  if (! mpfr_total_order_p (x, y))
    {
      printf ("Error on mpfr_total_order_p (x, y) with\n");
      printf ("x = ");
      mpfr_dump (x);
      printf ("y = ");
      mpfr_dump (y);
      exit (1);
    }

  if (mpfr_total_order_p (y, x))
    {
      printf ("Error on mpfr_total_order_p (y, x) with\n");
      printf ("x = ");
      mpfr_dump (x);
      printf ("y = ");
      mpfr_dump (y);
      exit (1);
    }
}

int
main (void)
{
  mpfr_t x[13];
  int i, j;

  tests_start_mpfr ();

  for (i = 0; i < 13; i++)
    mpfr_init2 (x[i], 32);

  mpfr_set_nan (x[1]);
  MPFR_SET_NEG (x[1]);
  mpfr_set_inf (x[2], -1);
  mpfr_set_si (x[3], -3, MPFR_RNDN);
  mpfr_set_si (x[4], -2, MPFR_RNDN);
  mpfr_set_si (x[5], -1, MPFR_RNDN);
  mpfr_set_zero (x[6], -1);
  mpfr_set_zero (x[7], 1);
  mpfr_set_si (x[8], 1, MPFR_RNDN);
  mpfr_set_si (x[9], 2, MPFR_RNDN);
  mpfr_set_si (x[10], 3, MPFR_RNDN);
  mpfr_set_inf (x[11], 1);
  mpfr_set_nan (x[12]);
  MPFR_SET_POS (x[12]);

  for (i = 1; i <= 12; i++)
    {
      check1 (x[i], x[0]);
      for (j = i+1; j <= 12; j++)
        check2 (x[i], x[j]);
    }

  for (i = 0; i < 13; i++)
    mpfr_clear (x[i]);

  tests_end_mpfr ();
  return 0;
}
