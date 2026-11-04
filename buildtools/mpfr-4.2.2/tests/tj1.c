/* tj1 -- test file for the Bessel function of first kind (order 1)

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

#define TEST_FUNCTION mpfr_j1
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), 8, RANDS)
#define REDUCE_EMAX 262143 /* otherwise arg. reduction is too expensive */
#include "tgeneric.c"

/* test for small input, where j1(x) = x/2 - x^3/16 + ... */
static void
test_small (void)
{
  mpfr_t x, y;
  int inex, sign;
  mpfr_exp_t e, emin;

  mpfr_init2 (x, 10);
  mpfr_init2 (y, 9);
  emin = mpfr_get_emin ();
  for (e = -4; e >= -30; e--)
    {
      if (e == -30)
        {
          e = mpfr_get_emin_min () - 1;
          set_emin (e + 1);
        }
      for (sign = -1; sign <= 1; sign += 2)
        {
          mpfr_set_si_2exp (x, sign, e, MPFR_RNDN);
          mpfr_nexttoinf (x);
          inex = mpfr_j1 (y, x, MPFR_RNDN);
          if (e >= -29)
            {
              /* since |x| is just above 2^e, |j1(x)| is just above 2^(e-1),
                 thus y should be 2^(e-1) and the inexact flag should be
                 of opposite sign of x */
              MPFR_ASSERTN(mpfr_cmp_si_2exp0 (y, sign, e - 1) == 0);
              MPFR_ASSERTN(VSIGN (inex) * sign < 0);
            }
          else
            {
              /* here |y| should be 0.5*2^emin and the inexact flag should
                 have the sign of x */
              MPFR_ASSERTN(mpfr_cmp_si_2exp0 (y, sign, e) == 0);
              MPFR_ASSERTN(VSIGN (inex) * sign > 0);
            }
        }
    }
  set_emin (emin);
  mpfr_clear (x);
  mpfr_clear (y);
}

/* a test that fails with GMP_CHECK_RANDOMIZE=1613146232984428
   on revision 14429 */
static void
bug20210215 (void)
{
  mpfr_t x, y;
  int inex;

  mpfr_init2 (x, 221);
  mpfr_init2 (y, 1);
  mpfr_set_str (x, "1.6484611511696130037307738844228498447763863563070374544054791168614e+01", 10, MPFR_RNDN);
  mpfr_clear_flags ();
  inex = mpfr_j1 (y, x, MPFR_RNDZ);
  MPFR_ASSERTN (mpfr_cmp_si_2exp0 (y, -1, -9) == 0);
  MPFR_ASSERTN (inex > 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_INEXACT);
  mpfr_clear (x);
  mpfr_clear (y);
}

int
main (int argc, char *argv[])
{
  mpfr_t x, y;

  tests_start_mpfr ();

  bug20210215 ();

  test_small ();

  mpfr_init (x);
  mpfr_init (y);

  /* special values */
  mpfr_set_nan (x);
  mpfr_j1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (y));

  mpfr_set_inf (x, 1); /* +Inf */
  mpfr_j1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 0) == 0 && MPFR_IS_POS (y));

  mpfr_set_inf (x, -1); /* -Inf */
  mpfr_j1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 0) == 0 && MPFR_IS_POS (y));

  mpfr_set_ui (x, 0, MPFR_RNDN); /* +0 */
  mpfr_j1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 0) == 0 && MPFR_IS_POS (y)); /* j1(+0)=+0 */

  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN); /* -0 */
  mpfr_j1 (y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 0) == 0 && MPFR_IS_NEG (y)); /* j1(-0)=-0 */

  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);

  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_j1 (y, x, MPFR_RNDN);
  mpfr_set_str_binary (x, "0.0111000010100111001001111011101001011100001100011011");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_j1 for x=1, rnd=MPFR_RNDN\n");
      printf ("Expected "); mpfr_dump (x);
      printf ("Got      "); mpfr_dump (y);
      exit (1);
    }
  mpfr_clear (x);
  mpfr_clear (y);

  test_generic (MPFR_PREC_MIN, 100, 10);

  data_check ("data/j1", mpfr_j1, "mpfr_j1");

  tests_end_mpfr ();

  return 0;
}
