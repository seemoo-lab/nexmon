/* Test file for mpfr_tanh.

Copyright 2001-2016 Free Software Foundation, Inc.
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

#include "mpfr-test.h"

#define TEST_FUNCTION mpfr_tanh
#define TEST_RANDOM_EMIN -36
#define TEST_RANDOM_EMAX 36
#include "tgeneric.c"

static void
special (void)
{
  mpfr_t x;

  mpfr_init (x);

  mpfr_set_nan (x);
  mpfr_tanh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (x));

  mpfr_set_inf (x, 1);
  mpfr_tanh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (x, 1) == 0);

  mpfr_set_inf (x, -1);
  mpfr_tanh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si (x, -1) == 0);

  mpfr_set_prec (x, 10);
  mpfr_set_str_binary (x, "-0.1001011001");
  mpfr_tanh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si_2exp (x, -135, -8) == 0);

  mpfr_clear (x);
}

static void
special_overflow (void)
{
  mpfr_t x, y;
  int i;
  mpfr_exp_t emin, emax;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  mpfr_clear_overflow ();
  set_emin (-125);
  set_emax (128);
  mpfr_init2 (x, 24);
  mpfr_init2 (y, 24);

  mpfr_set_str_binary (x, "0.101100100000000000110100E7");
  i = mpfr_tanh (y, x, MPFR_RNDN);
  if (mpfr_cmp_ui (y, 1) || i != 1)
    {
      printf("Overflow error (1). i=%d\ny=", i);
      mpfr_dump (y);
      exit (1);
    }
  MPFR_ASSERTN (!mpfr_overflow_p ());

  i = mpfr_tanh (y, x, MPFR_RNDZ);
  if (mpfr_cmp_str (y, "0.111111111111111111111111E0", 2, MPFR_RNDN)
      || i != -1)
    {
      printf("Overflow error (2).i=%d\ny=", i);
      mpfr_dump (y);
      exit (1);
    }
  MPFR_ASSERTN (!mpfr_overflow_p ());

  set_emin (emin);
  set_emax (emax);

  mpfr_set_str_binary (x, "0.1E1000000000");
  i = mpfr_tanh (y, x, MPFR_RNDN);
  if (mpfr_cmp_ui (y, 1) || i != 1)
    {
      printf("Overflow error (3). i=%d\ny=", i);
      mpfr_dump (y);
      exit (1);
    }
  MPFR_ASSERTN (!mpfr_overflow_p ());
  mpfr_set_str_binary (x, "-0.1E1000000000");
  i = mpfr_tanh (y, x, MPFR_RNDU);
  if (mpfr_cmp_str (y, "-0.111111111111111111111111E0", 2, MPFR_RNDN)
      || i != 1)
    {
      printf("Overflow error (4). i=%d\ny=", i);
      mpfr_dump (y);
      exit (1);
    }

  mpfr_clear (y);
  mpfr_clear (x);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  special_overflow ();
  special ();

  test_generic (2, 100, 100);

  data_check ("data/tanh", mpfr_tanh, "mpfr_tanh");
  bad_cases (mpfr_tanh, mpfr_atanh, "mpfr_tanh", 256, -128, 0,
             4, 128, 800, 100);

  tests_end_mpfr ();
  return 0;
}
