/* Test file for mpfr_asinu.

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

#define TEST_FUNCTION mpfr_asinu
#define ULONG_ARG2
#include "tgeneric.c"

static void
check_underflow (void)
{
  mpfr_t x, y;
  mpfr_exp_t emin = mpfr_get_emin ();

  set_emin (mpfr_get_emin_min ());

  mpfr_init2 (x, MPFR_PREC_MIN);
  mpfr_init2 (y, MPFR_PREC_MIN);
  mpfr_set_ui_2exp (x, 1, mpfr_get_emin_min () - 1, MPFR_RNDN);
  /* asinu(x,1) = asin(x)/(2*pi) will underflow */
  mpfr_asinu (y, x, 1, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  mpfr_asinu (y, x, 1, MPFR_RNDZ);
  MPFR_ASSERTN(mpfr_zero_p (y) && mpfr_signbit (y) == 0);
  mpfr_asinu (y, x, 1, MPFR_RNDU);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 1, mpfr_get_emin_min () - 1) == 0);
  mpfr_clear (x);
  mpfr_clear (y);

  set_emin (emin);
}

int
main (void)
{
  mpfr_t x, y;
  int r, inex;
  unsigned long u;

  tests_start_mpfr ();

  check_underflow ();

  mpfr_init (x);
  mpfr_init (y);

  /* check singular cases */
  MPFR_SET_NAN(x);
  mpfr_asinu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: asinu (NaN, 1) != NaN\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  mpfr_asinu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: asinu (+Inf, 1) != NaN\n");
      exit (1);
    }

  mpfr_set_inf (x, -1);
  mpfr_asinu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: asinu (-Inf, 1) != NaN\n");
      exit (1);
    }

  /* asinu (+0,u) = +0 */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_asinu (y, x, 1, MPFR_RNDN);
  if (MPFR_NOTZERO (y) || MPFR_IS_NEG (y))
    {
      printf ("Error: asinu(+0,1) != +0\n");
      exit (1);
    }

  /* asinu (-0,u) = -0 */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN);
  mpfr_asinu (y, x, 1, MPFR_RNDN);
  if (MPFR_NOTZERO (y) || MPFR_IS_POS (y))
    {
      printf ("Error: asinu(-0,1) != -0\n");
      exit (1);
    }

  /* tests for |x| > 1 */
  mpfr_set_ui (x, 2, MPFR_RNDN);
  mpfr_asinu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: asinu (2, 1) != NaN\n");
      exit (1);
    }

  mpfr_set_si (x, -2, MPFR_RNDN);
  mpfr_asinu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: asinu (-2, 1) != NaN\n");
      exit (1);
    }

  mpfr_set_ui (x, 2, MPFR_RNDN);
  mpfr_asinu (y, x, 0, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: asinu (2, 0) != NaN\n");
      exit (1);
    }

  mpfr_set_si (x, -2, MPFR_RNDN);
  mpfr_asinu (y, x, 0, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: asinu (-2, 0) != NaN\n");
      exit (1);
    }

  /* asinu (1,u) = u/4 */
  RND_LOOP (r)
    {
      mpfr_set_si (x, 1, MPFR_RNDN); /* exact */
      mpfr_asinu (y, x, 17, (mpfr_rnd_t) r);
      mpfr_set_ui_2exp (x, 17, -2, (mpfr_rnd_t) r);
      if (!mpfr_equal_p (x, y))
        {
          printf ("Error: asinu(1,17) != 17/4 for rnd=%s\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          exit (1);
        }
    }

  /* asinu (-1,u) = -u/4 */
  RND_LOOP (r)
    {
      mpfr_set_si (x, -1, MPFR_RNDN); /* exact */
      mpfr_asinu (y, x, 17, (mpfr_rnd_t) r);
      mpfr_set_si_2exp (x, -17, -2, (mpfr_rnd_t) r);
      if (!mpfr_equal_p (x, y))
        {
          printf ("Error: asinu(-1,17) != -17/4 for rnd=%s\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          exit (1);
        }
    }

  /* asinu (1/2,u) = u/12 */
  for (u = 1; u < 100; u++)
     RND_LOOP (r)
       {
         mpfr_set_ui_2exp (x, 1, -1, MPFR_RNDN); /* exact */
         mpfr_asinu (y, x, u, (mpfr_rnd_t) r);
         inex = mpfr_set_ui (x, u, MPFR_RNDN);
         MPFR_ASSERTN(inex == 0);
         inex = mpfr_div_ui (x, x, 12, (mpfr_rnd_t) r);
         if ((r != MPFR_RNDF || inex == 0) && !mpfr_equal_p (x, y))
           {
             printf ("Error: asinu(1/2,u) != u/12 for u=%lu and rnd=%s\n",
                     u, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
             printf ("got: "); mpfr_dump (y);
             exit (1);
           }
       }

  /* asinu (-1/2,u) = -u/12 */
  for (u = 1; u < 100; u++)
     RND_LOOP (r)
       {
         mpfr_set_si_2exp (x, -1, -1, MPFR_RNDN); /* exact */
         mpfr_asinu (y, x, u, (mpfr_rnd_t) r);
         inex = mpfr_set_ui (x, u, MPFR_RNDN);
         MPFR_ASSERTN(inex == 0);
         inex = mpfr_div_si (x, x, -12, (mpfr_rnd_t) r);
         if ((r != MPFR_RNDF || inex == 0) && !mpfr_equal_p (x, y))
           {
             printf ("Error: asinu(-1/2,u) != -u/12 for u=%lu and rnd=%s\n",
                     u, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
             printf ("expected: "); mpfr_dump (x);
             printf ("got:      "); mpfr_dump (y);
             exit (1);
           }
       }

  test_generic (MPFR_PREC_MIN, 100, 100);

  mpfr_clear (x);
  mpfr_clear (y);

  tests_end_mpfr ();
  return 0;
}
