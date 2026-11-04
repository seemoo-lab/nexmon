/* Test file for mpfr_const_catalan.

Copyright 2005-2025 Free Software Foundation, Inc.
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

/* Wrapper for tgeneric */
static int
my_const_catalan (mpfr_ptr x, mpfr_srcptr y, mpfr_rnd_t r)
{
  return mpfr_const_catalan (x, r);
}

#define RAND_FUNCTION(x) mpfr_set_ui(x,0,MPFR_RNDN)
#define TEST_FUNCTION my_const_catalan
#include "tgeneric.c"

static void
exercise_Ziv (void)
{
  mpfr_t x, y;
  int inex;

  mpfr_init2 (x, 175);
  mpfr_init2 (y, 175);
  inex = mpfr_const_catalan (x, MPFR_RNDN);
  mpfr_set_str_binary (y, "0.1110101001111100101110001001111101000000100110101110100001000101001000010101100000100010111000110111110100110010110100001100011000111110110001000011111000010011100000011100001");
  MPFR_ASSERTN(mpfr_equal_p (x, y));
  MPFR_ASSERTN(inex < 0);
  mpfr_clear (x);
  mpfr_clear (y);
}

int
main (int argc, char *argv[])
{
  mpfr_t x;

  tests_start_mpfr ();

  exercise_Ziv ();
  mpfr_init2 (x, 32);
  (mpfr_const_catalan) (x, MPFR_RNDN);
  mpfr_mul_2ui (x, x, 32, MPFR_RNDN);
  if (mpfr_cmp_ui (x, 3934042271UL))
    {
      printf ("Error in const_catalan for prec=32\n");
      exit (1);
    }
  mpfr_clear (x);

  test_generic (MPFR_PREC_MIN, 200, 1);

  tests_end_mpfr ();
  return 0;
}
