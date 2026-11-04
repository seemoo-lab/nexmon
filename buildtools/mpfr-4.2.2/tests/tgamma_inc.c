/* Test file for mpfr_gamma_inc

Copyright 2016-2025 Free Software Foundation, Inc.
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

#define TEST_FUNCTION mpfr_gamma_inc
#define TWO_ARGS
#define TEST_RANDOM_POS2 0 /* the 2nd argument is never negative */
#define TGENERIC_NOWARNING 1
#define TEST_RANDOM_EMAX 8
#define TEST_RANDOM_EMIN -32
#define REDUCE_EMAX TEST_RANDOM_EMAX
#define REDUCE_EMIN TEST_RANDOM_EMIN
#include "tgeneric.c"

/* do k random tests at precision p */
static void
test_random (mpfr_prec_t p, int k)
{
  mpfr_t a, x, y, z, t;

  mpfr_inits2 (p, a, x, y, z, (mpfr_ptr) 0);
  mpfr_init2 (t, p + 20);
  while (k--)
    {
      do mpfr_urandomb (a, RANDS); while (mpfr_zero_p (a));
      if (RAND_BOOL ())
        mpfr_neg (a, a, MPFR_RNDN);
      do mpfr_urandomb (x, RANDS); while (mpfr_zero_p (x));
      mpfr_gamma_inc (y, a, x, MPFR_RNDN);
      mpfr_gamma_inc (t, a, x, MPFR_RNDN);
      if (mpfr_can_round (t, mpfr_get_prec (z), MPFR_RNDN, MPFR_RNDN, p))
        {
          mpfr_set (z, t, MPFR_RNDN);
          if (mpfr_cmp (y, z))
            {
              printf ("mpfr_gamma_inc failed for a=");
              mpfr_out_str (stdout, 10, 0, a, MPFR_RNDN);
              printf (" x=");
              mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);
              printf ("\nexpected ");
              mpfr_out_str (stdout, 10, 0, z, MPFR_RNDN);
              printf ("\ngot      ");
              mpfr_out_str (stdout, 10, 0, y, MPFR_RNDN);
              printf ("\n");
              exit (1);
            }
        }
    }
  mpfr_clears (a, x, y, z, (mpfr_ptr) 0);
  mpfr_clear (t);
}

static void
specials (void)
{
  mpfr_t a, x;
  int inex;

  mpfr_init2 (a, 2);
  mpfr_init2 (x, 2);

  mpfr_set_inf (a, 1);
  mpfr_set_inf (x, 1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  mpfr_set_inf (a, 1);
  mpfr_set_inf (x, -1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  mpfr_set_inf (a, -1);
  mpfr_set_inf (x, -1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  mpfr_set_inf (a, -1);
  mpfr_set_inf (x, 1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_zero_p (a) || mpfr_signbit (a) == 0);

  mpfr_set_inf (a, 1);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (a) || mpfr_signbit (a) == 0);

  mpfr_set_inf (a, -1);
  mpfr_set_ui (x, 2, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_zero_p (a) || mpfr_signbit (a) == 0);

  mpfr_set_inf (a, -1);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_zero_p (a) || mpfr_signbit (a) == 0);

  mpfr_set_inf (a, -1);
  mpfr_set_ui_2exp (x, 1, -1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (a) || mpfr_signbit (a) == 0);

  mpfr_set_ui (a, 1, MPFR_RNDN);
  mpfr_set_inf (x, 1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_zero_p (a) || mpfr_signbit (a) == 0);

  /* gamma_inc(1,-x) = exp(x) tends to +Inf */
  mpfr_set_ui (a, 1, MPFR_RNDN);
  mpfr_set_inf (x, -1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (a) || mpfr_signbit (a) == 0);

  /* gamma_inc(0,x) for x < 0 has imaginary part -Pi and thus gives NaN
     over the reals */
  mpfr_set_zero (a, 1);
  mpfr_set_inf (x, -1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  /* gamma_inc(-1,x) for x < 0 has imaginary part +Pi and thus gives NaN */
  mpfr_set_si (a, -1, MPFR_RNDN);
  mpfr_set_inf (x, -1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  /* gamma_inc(-2,x) for x < 0 has imaginary part -Pi/2 and thus gives NaN */
  mpfr_set_si (a, -2, MPFR_RNDN);
  mpfr_set_inf (x, -1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  mpfr_set_ui_2exp (a, 1, -1, MPFR_RNDN);
  mpfr_set_inf (x, -1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  mpfr_set_si_2exp (a, -1, -1, MPFR_RNDN);
  mpfr_set_inf (x, -1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  /* gamma_inc(0,x) = -Ei(-x) */
  mpfr_set_zero (a, 1);
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  /* gamma_inc(a,0) = gamma(a) thus gamma_inc(-Inf,0) = gamma(-Inf) = NaN */
  mpfr_set_inf (a, -1);
  mpfr_set_zero (x, 1);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  mpfr_set_inf (a, -1);
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (a));

  /* check gamma_inc(2,0) = 1 is exact */
  mpfr_set_ui (a, 2, MPFR_RNDN);
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_clear_inexflag ();
  inex = mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  if (mpfr_cmp_ui (a, 1))
    {
      printf ("Error for gamma_inc(2,0)\n");
      printf ("expected 1\n");
      printf ("got      ");
      mpfr_out_str (stdout, 10, 0, a, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  if (inex != 0)
    {
      printf ("Wrong ternary value for gamma_inc(2,0)\n");
      printf ("expected 0\n");
      printf ("got      %d\n", inex);
      exit (1);
    }
  if (mpfr_inexflag_p ())
    {
      printf ("Wrong inexact flag for gamma_inc(2,0)\n");
      printf ("expected 0\n");
      printf ("got      1\n");
      exit (1);
    }

  /* check gamma_inc(0,1) = 0.219383934395520 */
  mpfr_set_ui (a, 0, MPFR_RNDN);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  if (mpfr_cmp_ui_2exp (a, 1, -2))
    {
      printf ("Error for gamma_inc(0,1)\n");
      printf ("expected 0.25\n");
      printf ("got      ");
      mpfr_out_str (stdout, 10, 0, a, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  /* check gamma_inc(-1,1) = 0.148495506775922 */
  mpfr_set_si (a, -1, MPFR_RNDN);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  if (mpfr_cmp_ui_2exp (a, 1, -3))
    {
      printf ("Error for gamma_inc(-1,1)\n");
      printf ("expected 0.125\n");
      printf ("got      ");
      mpfr_out_str (stdout, 10, 0, a, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  /* check gamma_inc(-2,1) = 0.109691967197760 */
  mpfr_set_si (a, -2, MPFR_RNDN);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  if (mpfr_cmp_ui_2exp (a, 1, -3))
    {
      printf ("Error for gamma_inc(-2,1)\n");
      printf ("expected 0.125\n");
      printf ("got      ");
      mpfr_out_str (stdout, 10, 0, a, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  /* check gamma_inc(-3,1) = 0.109691967197760 */
  mpfr_set_si (a, -3, MPFR_RNDN);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  if (mpfr_cmp_ui_2exp (a, 3, -5))
    {
      printf ("Error for gamma_inc(-3,1)\n");
      printf ("expected 3/32\n");
      printf ("got      ");
      mpfr_out_str (stdout, 10, 0, a, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  /* check gamma_inc(-100,1) = 0.00364201018241046 */
  mpfr_set_si (a, -100, MPFR_RNDN);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);
  if (mpfr_cmp_ui_2exp (a, 1, -8))
    {
      printf ("Error for gamma_inc(-100,1)\n");
      printf ("expected 1/256\n");
      printf ("got      ");
      mpfr_out_str (stdout, 10, 0, a, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  /* TODO: Once internal overflow is supported, add new tests with
     larger exponents, e.g. 64 (in addition to 25). */
  mpfr_set_prec (a, 1);
  mpfr_set_prec (x, 1);
  mpfr_set_ui_2exp (a, 1, 25, MPFR_RNDN);
  mpfr_set_ui_2exp (x, 1, -25, MPFR_RNDN);
  mpfr_gamma_inc (a, a, x, MPFR_RNDN);

  mpfr_clear (a);
  mpfr_clear (x);
}

/* tests for negative integer a: for -n <= a <= -1, perform k tests
   with random x in 0..|a| and precision 'prec' */
static void
test_negint (long n, unsigned long k, mpfr_prec_t prec)
{
  long i, j;
  mpfr_t a, x, y;

  mpfr_init2 (a, prec);
  mpfr_init2 (x, prec);
  mpfr_init2 (y, prec);
  for (i = 1; i <= n; i++)
    {
      mpfr_set_si (a, -i, MPFR_RNDN);
      for (j = 1; j <= k; j++)
        {
          mpfr_urandomb (x, RANDS);
          mpfr_mul_si (x, x, j, MPFR_RNDN);
          mpfr_set_prec (y, prec + 20);
          mpfr_gamma_inc (y, a, x, MPFR_RNDZ);
          mpfr_gamma_inc (x, a, x, MPFR_RNDZ);
          mpfr_prec_round (y, prec, MPFR_RNDZ);
          if (mpfr_cmp (x, y))
            {
              printf ("Error in mpfr_gamma_inc(%ld,%ld) with MPFR_RNDZ\n",
                      -i, j);
              printf ("expected ");
              mpfr_out_str (stdout, 10, 0, y, MPFR_RNDN);
              printf ("\ngot      ");
              mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);
              printf ("\n");
              exit (1);
            }
        }
    }
  mpfr_clear (a);
  mpfr_clear (x);
  mpfr_clear (y);
}

static void
coverage (void)
{
  mpfr_t a, x, y;
  int inex;

  mpfr_init2 (a, MPFR_PREC_MIN);
  mpfr_init2 (x, MPFR_PREC_MIN);
  mpfr_init2 (y, MPFR_PREC_MIN);

  /* exercise test MPFR_GET_EXP(a) + 3 > w in mpfr_gamma_inc_negint */
  mpfr_set_si (a, -256, MPFR_RNDN);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  inex = mpfr_gamma_inc (y, a, x, MPFR_RNDN);
  /* gamma_inc(-256,1) = 0.00143141575826821 is rounded to 2^(-10) */
  MPFR_ASSERTN(inex < 0);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 1, -10) == 0);

  /* exercise the case MPFR_IS_ZERO(s) in mpfr_gamma_inc_negint */
  mpfr_set_prec (a, 4);
  mpfr_set_prec (x, 4);
  mpfr_set_prec (y, 4);
  inex = mpfr_set_si (a, -15, MPFR_RNDN);
  MPFR_ASSERTN (inex == 0);
  inex = mpfr_set_ui (x, 15, MPFR_RNDN);
  MPFR_ASSERTN (inex == 0);
  /* gamma_inc(-15,15) = 0.2290433491e-25, rounded to 14*2^(-89) */
  inex = mpfr_gamma_inc (y, a, x, MPFR_RNDN);
  MPFR_ASSERTN(inex < 0);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 14, -89) == 0);

  mpfr_clear (a);
  mpfr_clear (x);
  mpfr_clear (y);
}

int
main (int argc, char *argv[])
{
  mpfr_prec_t p;

  tests_start_mpfr ();

  if (argc == 4) /* tgamma_inc a x prec: print gamma_inc(a,x) to prec bits */
    {
      mpfr_prec_t p = atoi (argv[3]);
      mpfr_t a, x;
      mpfr_init2 (a, p);
      mpfr_init2 (x, p);
      mpfr_set_str (a, argv[1], 10, MPFR_RNDN);
      mpfr_set_str (x, argv[2], 10, MPFR_RNDN);
      mpfr_gamma_inc (x, a, x, MPFR_RNDN);
      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);
      printf ("\n");
      mpfr_clear (a);
      mpfr_clear (x);
      return 0;
    }

  coverage ();

  specials ();

  test_negint (30, 10, 53);

  for (p = MPFR_PREC_MIN; p < 100; p++)
    test_random (p, 10);

  test_generic (MPFR_PREC_MIN, 100, 5);

  tests_end_mpfr ();
  return 0;
}
