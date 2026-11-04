/* Test file for mpfr_tan.

Copyright 2001-2004, 2006-2025 Free Software Foundation, Inc.
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

#define TEST_FUNCTION mpfr_tan
#define REDUCE_EMAX 262143 /* otherwise arg. reduction is too expensive */
#include "tgeneric.c"

static void
check_nans (void)
{
  mpfr_t  x, y;

  mpfr_init2 (x, 123L);
  mpfr_init2 (y, 123L);

  mpfr_set_nan (x);
  mpfr_tan (y, x, MPFR_RNDN);
  if (! mpfr_nan_p (y))
    {
      printf ("Error: tan(NaN) != NaN\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  mpfr_tan (y, x, MPFR_RNDN);
  if (! mpfr_nan_p (y))
    {
      printf ("Error: tan(Inf) != NaN\n");
      exit (1);
    }

  mpfr_set_inf (x, -1);
  mpfr_tan (y, x, MPFR_RNDN);
  if (! mpfr_nan_p (y))
    {
      printf ("Error: tan(-Inf) != NaN\n");
      exit (1);
    }

  /* exercise recomputation */
  mpfr_set_prec (x, 14);
  mpfr_set_str_binary (x, "0.10100000101010E0");
  mpfr_set_prec (y, 24);
  mpfr_tan (y, x, MPFR_RNDU);
  mpfr_set_prec (x, 24);
  mpfr_set_str_binary (x, "101110011011001100100001E-24");
  MPFR_ASSERTN(mpfr_cmp (x, y) == 0);

  /* Compute ~Pi/2 to check overflow */
  mpfr_set_prec (x, 20000);
  mpfr_const_pi (x, MPFR_RNDD);
  mpfr_div_2ui (x, x, 1, MPFR_RNDN);
  mpfr_set_prec (y, 24);
  mpfr_tan (y, x, MPFR_RNDN);
  if (mpfr_cmp_str (y, "0.100011101101011000100011E20001", 2, MPFR_RNDN))
    {
      printf("Error computing tan(~Pi/2)\n");
      mpfr_dump (y);
      exit (1);
    }

  /* bug found by Kaveh Ghazi on 13 Jul 2007 */
  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);
  mpfr_set_str_binary (x, "0.10011100110111000001000010110100101000000000000000000E34");
  mpfr_tan (y, x, MPFR_RNDN);
  mpfr_set_str_binary (x, "0.1000010011001010001000010100000110100111000011010101E41");
  MPFR_ASSERTN(mpfr_cmp (x, y) == 0);

  mpfr_clear (x);
  mpfr_clear (y);
}

/* This test was generated from bad_cases, with GMP_CHECK_RANDOMIZE=1514257254.
   For the x value below, we have tan(x) = -1.875 + epsilon,
   thus with RNDZ it should be rounded to -1.c. */
static void
bug20171218 (void)
{
  mpfr_t x, y, z;
  mpfr_init2 (x, 804);
  mpfr_init2 (y, 4);
  mpfr_init2 (z, 4);
  mpfr_set_str (x, "-1.14b1dd5f90ce0eded2a8f59c05e72daf7cc4c78f5075d73246fa420e2c026291d9377e67e7f54e925c4aed39c7b2f917424033c8612f00a19821890558d6c2cef9c60f4ad2c3b061ed53a1709dc1ec8e139627c2119c36d7ebdff0d715e559b47f740c534", 16, MPFR_RNDN);
  mpfr_tan (y, x, MPFR_RNDZ);
  mpfr_set_str (z, "-1.c", 16, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (y, z));
  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

static void
coverage (void)
{
  mpfr_t x, y;
  int inex;
  mpfr_exp_t emax;

  /* exercise mpfr_round_near_x when rounding gives an overflow */
  emax = mpfr_get_emax ();
  set_emax (-2);
  mpfr_init2 (x, 2);
  mpfr_init2 (y, 2);
  mpfr_setmax (x, mpfr_get_emax ());
  inex = mpfr_tan (y, x, MPFR_RNDA);
  MPFR_ASSERTN(inex > 0);
  MPFR_ASSERTN(mpfr_inf_p (y) && mpfr_signbit (y) == 0);
  mpfr_clear (x);
  mpfr_clear (y);
  set_emax (emax);
}

int
main (int argc, char *argv[])
{
  mpfr_t x;
  unsigned int i;
  unsigned int prec[10] = {14, 15, 19, 22, 23, 24, 25, 40, 41, 52};
  unsigned int prec2[10] = {4, 5, 6, 19, 70, 95, 100, 106, 107, 108};

  tests_start_mpfr ();

  coverage ();
  bug20171218 ();
  check_nans ();

  mpfr_init (x);

  mpfr_set_prec (x, 2);
  mpfr_set_str (x, "0.5", 10, MPFR_RNDN);
  mpfr_tan (x, x, MPFR_RNDD);
  if (mpfr_cmp_ui_2exp(x, 1, -1))
    {
      printf ("mpfr_tan(0.5, MPFR_RNDD) failed\n"
              "expected 0.5, got ");
      mpfr_dump (x);
      exit (1);
    }

  /* check that tan(3*Pi/4) ~ -1 */
  for (i=0; i<10; i++)
    {
      mpfr_set_prec (x, prec[i]);
      mpfr_const_pi (x, MPFR_RNDN);
      mpfr_mul_ui (x, x, 3, MPFR_RNDN);
      mpfr_div_ui (x, x, 4, MPFR_RNDN);
      mpfr_tan (x, x, MPFR_RNDN);
      if (mpfr_cmp_si (x, -1))
        {
          printf ("tan(3*Pi/4) fails for prec=%u\n", prec[i]);
          exit (1);
        }
    }

  /* check that tan(7*Pi/4) ~ -1 */
  for (i=0; i<10; i++)
    {
      mpfr_set_prec (x, prec2[i]);
      mpfr_const_pi (x, MPFR_RNDN);
      mpfr_mul_ui (x, x, 7, MPFR_RNDN);
      mpfr_div_ui (x, x, 4, MPFR_RNDN);
      mpfr_tan (x, x, MPFR_RNDN);
      if (mpfr_cmp_si (x, -1))
        {
          printf ("tan(3*Pi/4) fails for prec=%u\n", prec2[i]);
          exit (1);
        }
    }

  mpfr_clear (x);

  test_generic (MPFR_PREC_MIN, 100, 10);

  data_check ("data/tan", mpfr_tan, "mpfr_tan");
  bad_cases (mpfr_tan, mpfr_atan, "mpfr_tan", 256, -256, 255, 4, 128, 800, 40);

  tests_end_mpfr ();
  return 0;
}
