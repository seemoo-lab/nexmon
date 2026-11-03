/* Test file for mpfr_abs.

Copyright 2000-2025 Free Software Foundation, Inc.
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

#include <float.h>

#include "mpfr-test.h"

static void
check_inexact (void)
{
  mpfr_prec_t p, q;
  mpfr_t x, y, absx;
  int rnd;
  int inexact, cmp;

  mpfr_init (x);
  mpfr_init (y);
  mpfr_init (absx);

  for (p=2; p<500; p++)
    {
      mpfr_set_prec (x, p);
      mpfr_set_prec (absx, p);
      mpfr_urandomb (x, RANDS);
      if (RAND_BOOL ())
        {
          mpfr_set (absx, x, MPFR_RNDN);
          mpfr_neg (x, x, MPFR_RNDN);
        }
      else
        mpfr_set (absx, x, MPFR_RNDN);
      for (q=2; q<2*p; q++)
        {
          mpfr_set_prec (y, q);
          RND_LOOP_NO_RNDF (rnd)
            {
              inexact = mpfr_abs (y, x, (mpfr_rnd_t) rnd);
              cmp = mpfr_cmp (y, absx);
              if (((inexact == 0) && (cmp != 0)) ||
                  ((inexact > 0) && (cmp <= 0)) ||
                  ((inexact < 0) && (cmp >= 0)))
                {
                  printf ("Wrong inexact flag for %s: expected %d, got %d\n",
                          mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), cmp,
                          inexact);
                  printf ("x="); mpfr_dump (x);
                  printf ("absx="); mpfr_dump (absx);
                  printf ("y="); mpfr_dump (y);
                  exit (1);
                }
            }
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (absx);
}

static void
check_cmp (int argc, char *argv[])
{
  mpfr_t x, y;
  mpfr_ptr p[2];
  int inexact;
  int n, k;

  mpfr_inits2 (53, x, y, (mpfr_ptr) 0);

  /* will test with DEST != SRC and with DEST == SRC */
  p[0] = y;  /* y first */
  p[1] = x;  /* x last since it may be modified */

  for (k = 0; k <= 1; k++)
    {
      mpfr_set_nan (p[k]);
      mpfr_set_ui (x, 1, MPFR_RNDN);
      inexact = mpfr_abs (p[k], x, MPFR_RNDN);
      if (mpfr_cmp_ui (p[k], 1) || inexact != 0)
        {
          printf ("Error in mpfr_abs(1.0) for k = %d\n", k);
          exit (1);
        }

      mpfr_set_nan (p[k]);
      mpfr_set_ui (x, 1, MPFR_RNDN);
      inexact = (mpfr_abs) (p[k], x, MPFR_RNDN);
      if (mpfr_cmp_ui (p[k], 1) || inexact != 0)
        {
          printf ("Error in (mpfr_abs)(1.0) for k = %d\n", k);
          exit (1);
        }

      mpfr_set_nan (p[k]);
      mpfr_set_si (x, -1, MPFR_RNDN);
      inexact = mpfr_abs (p[k], x, MPFR_RNDN);
      if (mpfr_cmp_ui (p[k], 1) || inexact != 0)
        {
          printf ("Error in mpfr_abs(-1.0) for k = %d\n", k);
          exit (1);
        }

      mpfr_set_nan (p[k]);
      mpfr_set_si (x, -1, MPFR_RNDN);
      inexact = (mpfr_abs) (p[k], x, MPFR_RNDN);
      if (mpfr_cmp_ui (p[k], 1) || inexact != 0)
        {
          printf ("Error in (mpfr_abs)(-1.0) for k = %d\n", k);
          exit (1);
        }

      mpfr_set_nan (p[k]);
      mpfr_set_inf (x, 1);
      inexact = mpfr_abs (p[k], x, MPFR_RNDN);
      if (! mpfr_inf_p (p[k]) || mpfr_sgn (p[k]) <= 0 || inexact != 0)
        {
          printf ("Error in mpfr_abs(Inf) for k = %d\n", k);
          exit (1);
        }

      mpfr_set_nan (p[k]);
      mpfr_set_inf (x, 1);
      inexact = (mpfr_abs) (p[k], x, MPFR_RNDN);
      if (! mpfr_inf_p (p[k]) || mpfr_sgn (p[k]) <= 0 || inexact != 0)
        {
          printf ("Error in (mpfr_abs)(Inf) for k = %d\n", k);
          exit (1);
        }

      mpfr_set_nan (p[k]);
      mpfr_set_inf (x, -1);
      inexact = mpfr_abs (p[k], x, MPFR_RNDN);
      if (! mpfr_inf_p (p[k]) || mpfr_sgn (p[k]) <= 0 || inexact != 0)
        {
          printf ("Error in mpfr_abs(-Inf) for k = %d\n", k);
          exit (1);
        }

      mpfr_set_nan (p[k]);
      mpfr_set_inf (x, -1);
      inexact = (mpfr_abs) (p[k], x, MPFR_RNDN);
      if (! mpfr_inf_p (p[k]) || mpfr_sgn (p[k]) <= 0 || inexact != 0)
        {
          printf ("Error in (mpfr_abs)(-Inf) for k = %d\n", k);
          exit (1);
        }

      mpfr_set_zero (p[k], 1);
      MPFR_SET_NAN (x);
      MPFR_SET_POS (x);
      mpfr_clear_nanflag ();
      inexact = mpfr_abs (p[k], x, MPFR_RNDN);
      if (! MPFR_IS_NAN (p[k]) || ! mpfr_nanflag_p () ||
          mpfr_signbit (p[k]) || inexact != 0)
        {
          printf ("Error in mpfr_abs(+NaN).\n");
          exit (1);
        }

      mpfr_set_zero (p[k], 1);
      MPFR_SET_NAN (x);
      MPFR_SET_POS (x);
      mpfr_clear_nanflag ();
      inexact = (mpfr_abs) (p[k], x, MPFR_RNDN);
      if (! MPFR_IS_NAN (p[k]) || ! mpfr_nanflag_p () ||
          mpfr_signbit (p[k]) || inexact != 0)
        {
          printf ("Error in (mpfr_abs)(+NaN).\n");
          exit (1);
        }

      mpfr_set_zero (p[k], 1);
      MPFR_SET_NAN (x);
      MPFR_SET_NEG (x);
      mpfr_clear_nanflag ();
      inexact = mpfr_abs (p[k], x, MPFR_RNDN);
      if (! MPFR_IS_NAN (p[k]) || ! mpfr_nanflag_p () ||
          mpfr_signbit (p[k]) || inexact != 0)
        {
          printf ("Error in mpfr_abs(-NaN).\n");
          exit (1);
        }

      mpfr_set_zero (p[k], 1);
      MPFR_SET_NAN (x);
      MPFR_SET_NEG (x);
      mpfr_clear_nanflag ();
      inexact = (mpfr_abs) (p[k], x, MPFR_RNDN);
      if (! MPFR_IS_NAN (p[k]) || ! mpfr_nanflag_p () ||
          mpfr_signbit (p[k]) || inexact != 0)
        {
          printf ("Error in (mpfr_abs)(-NaN).\n");
          exit (1);
        }
    }

  n = (argc==1) ? 25000 : atoi(argv[1]);
  for (k = 1; k <= n; k++)
    {
      mpfr_rnd_t rnd;
      int sign = RAND_SIGN ();

      mpfr_urandomb (x, RANDS);
      MPFR_SET_SIGN (x, sign);
      rnd = RND_RAND ();
      mpfr_abs (y, x, rnd);
      MPFR_SET_POS (x);
      if (mpfr_cmp (x, y))
        {
          printf ("Mismatch for sign=%d and x=", sign);
          mpfr_dump (x);
          printf ("Results=");
          mpfr_dump (y);
          exit (1);
        }
    }

  mpfr_clears (x, y, (mpfr_ptr) 0);
}

#define TEST_FUNCTION mpfr_abs
#include "tgeneric.c"

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  check_inexact ();
  check_cmp (argc, argv);

  test_generic (MPFR_PREC_MIN, 1000, 10);

  tests_end_mpfr ();
  return 0;
}
