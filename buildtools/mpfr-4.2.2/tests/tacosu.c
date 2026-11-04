/* Test file for mpfr_acosu.

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

#define TEST_FUNCTION mpfr_acosu
#define ULONG_ARG2
#include "tgeneric.c"

int
main (void)
{
  mpfr_t x, y;
  int r, inex;
  int unsigned long u;

  tests_start_mpfr ();

  mpfr_init (x);
  mpfr_init (y);

  /* check singular cases */
  MPFR_SET_NAN(x);
  mpfr_acosu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: acosu (NaN, 1) != NaN\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  mpfr_acosu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: acosu (+Inf, 1) != NaN\n");
      exit (1);
    }

  mpfr_set_inf (x, -1);
  mpfr_acosu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: acosu (-Inf, 1) != NaN\n");
      exit (1);
    }

  /* acosu (0,u) = u/4 */
  RND_LOOP (r)
    {
      mpfr_set_ui (x, 0, MPFR_RNDN); /* exact */
      mpfr_acosu (y, x, 17, (mpfr_rnd_t) r);
      mpfr_set_ui_2exp (x, 17, -2, MPFR_RNDN);
      if (!mpfr_equal_p (x, y))
        {
          printf ("Error: acosu(0,17) != 17/4 for rnd=%s\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          exit (1);
        }
    }

  /* check case |x| > 1 */
  mpfr_set_ui (x, 2, MPFR_RNDN);
  mpfr_acosu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: acosu (2, 1) != NaN\n");
      exit (1);
    }

  mpfr_set_si (x, -2, MPFR_RNDN);
  mpfr_acosu (y, x, 1, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: acosu (-2, 1) != NaN\n");
      exit (1);
    }

  /* check case |x| > 1 with u=0 */
  mpfr_set_ui (x, 2, MPFR_RNDN);
  mpfr_acosu (y, x, 0, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: acosu (2, 0) != NaN\n");
      exit (1);
    }

  mpfr_set_si (x, -2, MPFR_RNDN);
  mpfr_acosu (y, x, 0, MPFR_RNDN);
  if (mpfr_nan_p (y) == 0)
    {
      printf ("Error: acosu (-2, 0) != NaN\n");
      exit (1);
    }

  /* check case u=0 */
  mpfr_set_ui_2exp (x, 1, -1, MPFR_RNDN);
  mpfr_acosu (y, x, 0, MPFR_RNDN);
  if (!mpfr_zero_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error: acosu (1/2, 0) != +0\n");
      printf ("got: "); mpfr_dump (y);
      exit (1);
    }

  /* acosu (1,u) = +0 */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_acosu (y, x, 1, MPFR_RNDN);
  if (MPFR_NOTZERO (y) || MPFR_IS_NEG (y))
    {
      printf ("Error: acosu(1,1) != +0.0\n");
      exit (1);
    }

  /* acosu (-1,u) = u/2 */
  RND_LOOP (r)
    {
      mpfr_set_si (x, -1, MPFR_RNDN); /* exact */
      mpfr_acosu (y, x, 17, (mpfr_rnd_t) r);
      mpfr_set_ui_2exp (x, 17, -1, MPFR_RNDN);
      if (!mpfr_equal_p (x, y))
        {
          printf ("Error: acosu(1,17) != 17/2 for rnd=%s\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          exit (1);
        }
    }

  /* acosu (1/2,u) = u/6 */
  for (u = 1; u < 100; u++)
     RND_LOOP (r)
       {
         mpfr_set_ui_2exp (x, 1, -1, MPFR_RNDN); /* exact */
         mpfr_acosu (y, x, u, (mpfr_rnd_t) r);
         inex = mpfr_set_ui (x, u, MPFR_RNDN);
         MPFR_ASSERTN(inex == 0);
         inex = mpfr_div_ui (x, x, 6, (mpfr_rnd_t) r);
         if ((r != MPFR_RNDF || inex == 0) && !mpfr_equal_p (x, y))
           {
             printf ("Error: acosu(1/2,u) != u/6 for u=%lu and rnd=%s\n",
                     u, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
             printf ("got: "); mpfr_dump (y);
             exit (1);
           }
       }

  /* acosu (-1/2,u) = u/3 */
  for (u = 1; u < 100; u++)
     RND_LOOP (r)
       {
         mpfr_set_si_2exp (x, -1, -1, MPFR_RNDN); /* exact */
         mpfr_acosu (y, x, u, (mpfr_rnd_t) r);
         inex = mpfr_set_ui (x, u, MPFR_RNDN);
         MPFR_ASSERTN(inex == 0);
         inex = mpfr_div_ui (x, x, 3, (mpfr_rnd_t) r);
         if ((r != MPFR_RNDF || inex == 0) && !mpfr_equal_p (x, y))
           {
             printf ("Error: acosu(-1/2,u) != u/3 for u=%lu and rnd=%s\n",
                     u, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
             printf ("got: "); mpfr_dump (y);
             exit (1);
           }
       }

  test_generic (MPFR_PREC_MIN, 100, 100);

  mpfr_clear (x);
  mpfr_clear (y);

  tests_end_mpfr ();
  return 0;
}
