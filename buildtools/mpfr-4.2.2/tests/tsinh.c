/* Test file for mpfr_sinh.

Copyright 2001-2002, 2004, 2006-2025 Free Software Foundation, Inc.
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

#define TEST_FUNCTION mpfr_sinh
#define TEST_RANDOM_EMIN -36
#define TEST_RANDOM_EMAX 36
#include "tgeneric.c"

static void
special (void)
{
  mpfr_t x;
  int i;

  mpfr_init (x);

  mpfr_set_nan (x);
  mpfr_sinh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (x));

  mpfr_set_inf (x, 1);
  mpfr_sinh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (x) && mpfr_sgn (x) > 0);
  mpfr_set_inf (x, -1);
  mpfr_sinh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (x) && mpfr_sgn (x) < 0);

  mpfr_set_prec (x, 10);
  mpfr_set_str_binary (x, "-0.1001011001");
  mpfr_sinh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_si_2exp (x, -159, -8) == 0);

  /* corner case */
  mpfr_set_prec (x, 2);
  mpfr_set_str_binary (x, "1E-6");
  mpfr_sinh (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (x, 1, -6) == 0);

  mpfr_clear_flags ();
  mpfr_set_str_binary (x, "1E1000000000");
  i = mpfr_sinh (x, x, MPFR_RNDN);
  MPFR_ASSERTN (MPFR_IS_INF (x) && MPFR_IS_POS (x));
  MPFR_ASSERTN (mpfr_overflow_p ());
  MPFR_ASSERTN (i == 1);

  mpfr_clear_flags ();
  mpfr_set_str_binary (x, "-1E1000000000");
  i = mpfr_sinh (x, x, MPFR_RNDN);
  MPFR_ASSERTN (MPFR_IS_INF (x) && MPFR_IS_NEG (x));
  MPFR_ASSERTN (mpfr_overflow_p () && !mpfr_underflow_p ());
  MPFR_ASSERTN (i == -1);

  mpfr_clear_flags ();
  mpfr_set_str_binary (x, "-1E1000000000");
  i = mpfr_sinh (x, x, MPFR_RNDD);
  MPFR_ASSERTN (MPFR_IS_INF (x) && MPFR_IS_NEG (x));
  MPFR_ASSERTN (mpfr_overflow_p () && !mpfr_underflow_p ());
  MPFR_ASSERTN (i == -1);

  mpfr_clear_flags ();
  mpfr_set_str_binary (x, "-1E1000000000");
  i = mpfr_sinh (x, x, MPFR_RNDU);
  MPFR_ASSERTN (!MPFR_IS_INF (x) && MPFR_IS_NEG (x));
  MPFR_ASSERTN (mpfr_overflow_p () && !mpfr_underflow_p ());
  MPFR_ASSERTN (i == 1);

  mpfr_clear (x);
}

/* exercise mpfr_sinh near the overflow limit */
static void
coverage (void)
{
#if _MPFR_EXP_FORMAT >= 3 && ULONG_MAX == 0xffffffffffffffff
  mpfr_t x, y;
  int inex;
  mpfr_exp_t emax;

  emax = mpfr_get_emax ();
  mpfr_init2 (x, 64);
  mpfr_init2 (y, 64);

  MPFR_ASSERTN(mpfr_get_emax_max () == 4611686018427387903UL); /* 2^62-1 */
  set_emax (mpfr_get_emax_max ());

  mpfr_set_ui (x, 3196577161300663915UL, MPFR_RNDN);
  mpfr_clear_overflow ();
  /* exp(x) overflows, cosh(x/2)*sinh(x/2) does not overflow,
     but 2*cosh(x/2)*sinh(x/2) overflows */
  inex = mpfr_sinh (y, x, MPFR_RNDZ);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 18446744073709551615UL, 4611686018427387839UL) == 0);
  MPFR_ASSERTN(inex < 0);
  MPFR_ASSERTN(mpfr_overflow_p ());

  mpfr_set_prec (x, 65);
  mpfr_set_str (x, "3196577161300663914.5", 10, MPFR_RNDN);
  mpfr_clear_overflow ();
  /* exp(x) overflows, cosh(x/2)*sinh(x/2) does not overflow,
     and 2*cosh(x/2)*sinh(x/2) does not overflow */
  inex = mpfr_sinh (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 11795642775994293059UL, 4611686018427387839UL) == 0);
  MPFR_ASSERTN(inex > 0);
  MPFR_ASSERTN(!mpfr_overflow_p ());

  mpfr_clear (x);
  mpfr_clear (y);
  set_emax (emax);
#endif
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  coverage ();
  special ();

  test_generic (MPFR_PREC_MIN, 100, 100);

  data_check ("data/sinh", mpfr_sinh, "mpfr_sinh");
  bad_cases (mpfr_sinh, mpfr_asinh, "mpfr_sinh", 256, -256, 255,
             4, 128, 800, 100);

  tests_end_mpfr ();
  return 0;
}
