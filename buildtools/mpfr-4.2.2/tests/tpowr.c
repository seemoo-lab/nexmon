/* Test file for mpfr_powr.

Copyright 2021-2025 Free Software Foundation, Inc.
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

/* check the special rules from IEEE 754-2019 */
static void
check_ieee754_2019 (void)
{
  mpfr_t x, y, z;
  mpfr_prec_t p = 5;

  mpfr_init2 (x, p);
  mpfr_init2 (y, p);
  mpfr_init2 (z, p);

  /* powr (x, ±0) is 1 for finite x > 0 */
  mpfr_set_ui (x, 17, MPFR_RNDN);
  mpfr_set_zero (y, 1);
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (z, 1) == 0);
  mpfr_set_zero (y, -1);
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (z, 1) == 0);

  /* powr (±0, y) is +∞ and signals divideByZero for finite y < 0 */
  mpfr_set_si (y, -17, MPFR_RNDN);
  mpfr_set_zero (x, 1);
  mpfr_clear_divby0 ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0 && mpfr_divby0_p ());
  mpfr_set_zero (x, -1);
  mpfr_clear_divby0 ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0 && mpfr_divby0_p ());
  /* try also y an even negative integer */
  mpfr_set_si (y, -42, MPFR_RNDN);
  mpfr_set_zero (x, 1);
  mpfr_clear_divby0 ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0 && mpfr_divby0_p ());
  mpfr_set_zero (x, -1);
  mpfr_clear_divby0 ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0 && mpfr_divby0_p ());
  /* and y not an integer */
  mpfr_set_si_2exp (y, -17, -1, MPFR_RNDN);
  mpfr_set_zero (x, 1);
  mpfr_clear_divby0 ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0 && mpfr_divby0_p ());
  mpfr_set_zero (x, -1);
  mpfr_clear_divby0 ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0 && mpfr_divby0_p ());

  /* powr (±0, −∞) is +∞ */
  mpfr_set_inf (y, -1);
  mpfr_set_zero (x, 1);
  mpfr_clear_divby0 ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0 && !mpfr_divby0_p ());
  mpfr_set_zero (x, -1);
  mpfr_clear_divby0 ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0 && !mpfr_divby0_p ());

  /* powr (±0, y) is +0 for y > 0 */
  mpfr_set_ui (y, 17, MPFR_RNDN);
  mpfr_set_zero (x, 1);
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (z) && !mpfr_signbit (z));
  mpfr_set_zero (x, -1);
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (z) && !mpfr_signbit (z));

  /* powr (+1, y) is 1 for finite y */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_set_ui (y, 17, MPFR_RNDN);
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (z, 1) == 0);

  /* powr (x, y) signals the invalid operation exception for x < 0 */
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_set_si (y, 1, MPFR_RNDN);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());

  /* powr (±0, ±0) signals the invalid operation exception */
  mpfr_set_zero (x, 1);
  mpfr_set_zero (y, 1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_zero (y, -1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_zero (x, -1);
  mpfr_set_zero (y, 1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_zero (y, -1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());

  /* powr (+∞, ±0) signals the invalid operation exception */
  mpfr_set_inf (x, 1);
  mpfr_set_zero (y, 1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_zero (y, -1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());

  /* powr (+1, ±∞) signals the invalid operation exception */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_set_inf (y, 1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_inf (y, -1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());

  /* powr (x, qNaN) is qNaN for x ≥ 0 */
  mpfr_set_nan (y);
  mpfr_set_zero (x, -1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_zero (x, 1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_ui (x, 17, MPFR_RNDN);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_inf (x, 1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());

  /* powr (qNaN, y) is qNaN */
  mpfr_set_nan (x);
  mpfr_set_inf (y, 1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_inf (y, -1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_zero (y, 1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_zero (y, -1);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_si (y, 17, MPFR_RNDN);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());
  mpfr_set_si (y, -17, MPFR_RNDN);
  mpfr_clear_nanflag ();
  mpfr_powr (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (z) && mpfr_nanflag_p ());

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

#define TEST_FUNCTION mpfr_powr
#define TWO_ARGS
#define TEST_RANDOM_POS 16 /* the 2nd argument is negative with prob. 16/512 */
#include "tgeneric.c"

int
main (int argc, char **argv)
{
  tests_start_mpfr ();

  check_ieee754_2019 ();

  test_generic (MPFR_PREC_MIN, 100, 100);

  tests_end_mpfr ();
  return 0;
}
