/* Test file for mpfr_log10.

Copyright 2001-2025 Free Software Foundation, Inc.
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

#ifdef CHECK_EXTERNAL
static int
test_log10 (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode)
{
  int res;
  int ok = rnd_mode == MPFR_RNDN && mpfr_number_p (b) && mpfr_get_prec (a)>=53;
  if (ok)
    {
      mpfr_print_raw (b);
    }
  res = mpfr_log10 (a, b, rnd_mode);
  if (ok)
    {
      printf (" ");
      mpfr_print_raw (a);
      printf ("\n");
    }
  return res;
}
#else
#define test_log10 mpfr_log10
#endif

#define TEST_FUNCTION test_log10
#define TEST_RANDOM_POS 8
#include "tgeneric.c"

/* On 2023-02-13, one gets an infinite loop in mpfr_log10 on both
   32-bit and 64-bit hosts when the precision is not large enough
   (precision 12 and below). */
static void
bug20230213 (void)
{
  mpfr_exp_t old_emin, old_emax, e;
  mpfr_t t, x, y0, y1, y2;
  int prec;

  old_emin = mpfr_get_emin ();
  old_emax = mpfr_get_emax ();

  set_emin (MPFR_EMIN_MIN);
  set_emax (MPFR_EMAX_MAX);
  e = mpfr_get_emax () - 1;

  /* The precisions of t and y0 should be large enough to avoid
     a hard-to-round case for the target precisions. */
  mpfr_inits2 (64, t, y0, (mpfr_ptr) 0);
  mpfr_set_exp_t (y0, e, MPFR_RNDN);
  mpfr_log_ui (t, 10, MPFR_RNDN);
  mpfr_div (y0, y0, t, MPFR_RNDN);
  mpfr_log_ui (t, 2, MPFR_RNDN);
  mpfr_mul (y0, y0, t, MPFR_RNDN);

  for (prec = 16; prec >= MPFR_PREC_MIN; prec--)
    {
      mpfr_inits2 (prec, x, y1, y2, (mpfr_ptr) 0);
      mpfr_set (y1, y0, MPFR_RNDN);

      mpfr_set_ui_2exp (x, 1, e, MPFR_RNDN);
      mpfr_log10 (y2, x, MPFR_RNDN);
      MPFR_ASSERTN (MPFR_IS_PURE_FP (y2));
      MPFR_ASSERTN (MPFR_IS_POS (y2));

      if (! mpfr_equal_p (y1, y2))
        {
          printf ("Error in bug20230213.\n");
          printf ("Expected ");
          mpfr_dump (y1);
          printf ("Got      ");
          mpfr_dump (y2);
          exit (1);
        }
      mpfr_clears (x, y1, y2, (mpfr_ptr) 0);
    }

  mpfr_clears (t, y0, (mpfr_ptr) 0);

  set_emin (old_emin);
  set_emax (old_emax);
}

int
main (int argc, char *argv[])
{
  mpfr_t x, y;
  unsigned int n;
  int inex, r;

  tests_start_mpfr ();

  test_generic (MPFR_PREC_MIN, 100, 20);

  mpfr_init2 (x, 53);
  mpfr_init2 (y, 53);

  RND_LOOP (r)
    {
      /* check NaN */
      mpfr_set_nan (x);
      inex = test_log10 (y, x, (mpfr_rnd_t) r);
      MPFR_ASSERTN (mpfr_nan_p (y) && inex == 0);

      /* check Inf */
      mpfr_set_inf (x, -1);
      inex = test_log10 (y, x, (mpfr_rnd_t) r);
      MPFR_ASSERTN (mpfr_nan_p (y) && inex == 0);

      mpfr_set_inf (x, 1);
      inex = test_log10 (y, x, (mpfr_rnd_t) r);
      MPFR_ASSERTN (mpfr_inf_p (y) && mpfr_sgn (y) > 0 && inex == 0);

      mpfr_set_ui (x, 0, MPFR_RNDN);
      inex = test_log10 (x, x, (mpfr_rnd_t) r);
      MPFR_ASSERTN (mpfr_inf_p (x) && mpfr_sgn (x) < 0 && inex == 0);
      mpfr_set_ui (x, 0, MPFR_RNDN);
      mpfr_neg (x, x, MPFR_RNDN);
      inex = test_log10 (x, x, (mpfr_rnd_t) r);
      MPFR_ASSERTN (mpfr_inf_p (x) && mpfr_sgn (x) < 0 && inex == 0);

      /* check negative argument */
      mpfr_set_si (x, -1, MPFR_RNDN);
      inex = test_log10 (y, x, (mpfr_rnd_t) r);
      MPFR_ASSERTN (mpfr_nan_p (y) && inex == 0);

      /* check log10(1) = 0 */
      mpfr_set_ui (x, 1, MPFR_RNDN);
      inex = test_log10 (y, x, (mpfr_rnd_t) r);
      MPFR_ASSERTN (MPFR_IS_ZERO (y) && MPFR_IS_POS (y) && inex == 0);

      /* check log10(10^n)=n */
      mpfr_set_ui (x, 1, MPFR_RNDN);
      for (n = 1; n <= 15; n++)
        {
          mpfr_mul_ui (x, x, 10, MPFR_RNDN); /* x = 10^n */
          inex = test_log10 (y, x, (mpfr_rnd_t) r);
          if (mpfr_cmp_ui (y, n))
            {
              printf ("log10(10^n) <> n for n=%u\n", n);
              exit (1);
            }
          MPFR_ASSERTN (inex == 0);
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);

  bug20230213 ();

  data_check ("data/log10", mpfr_log10, "mpfr_log10");
  bad_cases (mpfr_log10, mpfr_exp10, "mpfr_log10",
             256, -30, 30, 4, 128, 800, 50);

  tests_end_mpfr ();
  return 0;
}
