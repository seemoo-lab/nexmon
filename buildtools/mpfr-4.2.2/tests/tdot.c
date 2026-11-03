/* tdot -- test file for mpfr_dot

Copyright 2018-2025 Free Software Foundation, Inc.
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

static void
check_simple (void)
{
  mpfr_t tab[3], r;
  mpfr_ptr tabp[3];
  int i;

  mpfr_init2 (r, 16);
  for (i = 0; i < 3; i++)
    {
      mpfr_init2 (tab[i], 16);
      mpfr_set_ui (tab[i], 1, MPFR_RNDN);
      tabp[i] = tab[i];
    }

  i = mpfr_dot (r, tabp, tabp, 3, MPFR_RNDN);
  if (mpfr_cmp_ui0 (r, 3) || i != 0)
    {
      printf ("Error in check_simple\n");
      exit (1);
    }

  mpfr_clears (tab[0], tab[1], tab[2], r, (mpfr_ptr) 0);
}

static void
check_special (void)
{
  mpfr_t tab[3], r;
  mpfr_ptr tabp[3];
  int i;
  int rnd;

  mpfr_inits2 (53, tab[0], tab[1], tab[2], r, (mpfr_ptr) 0);
  tabp[0] = tab[0];
  tabp[1] = tab[1];
  tabp[2] = tab[2];

  RND_LOOP (rnd)
    {
      i = mpfr_dot (r, tabp, tabp, 0, (mpfr_rnd_t) rnd);
      if (!MPFR_IS_ZERO (r) || !MPFR_IS_POS (r) || i != 0)
        {
          printf ("Special case n==0 failed for %s!\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
          exit (1);
        }
    }

  mpfr_set_ui (tab[0], 42, MPFR_RNDN);
  RND_LOOP (rnd)
    {
      i = mpfr_dot (r, tabp, tabp, 1, (mpfr_rnd_t) rnd);
      if (mpfr_cmp_ui0 (r, 42*42) || i != 0)
        {
          printf ("Special case n==1 failed for %s!\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
          exit (1);
        }
    }

  mpfr_set_ui (tab[1], 17, MPFR_RNDN);
  MPFR_SET_NAN (tab[2]);
  RND_LOOP (rnd)
    {
      i = mpfr_dot (r, tabp, tabp, 3, (mpfr_rnd_t) rnd);
      if (!MPFR_IS_NAN (r) || i != 0)
        {
          printf ("Special case NAN failed for %s!\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
          exit (1);
        }
    }

  MPFR_SET_INF (tab[2]);
  MPFR_SET_POS (tab[2]);
  RND_LOOP (rnd)
    {
      i = mpfr_dot (r, tabp, tabp, 3, (mpfr_rnd_t) rnd);
      if (!MPFR_IS_INF (r) || !MPFR_IS_POS (r) || i != 0)
        {
          printf ("Special case +INF failed for %s!\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
          exit (1);
        }
    }

  MPFR_SET_INF (tab[2]);
  MPFR_SET_NEG (tab[2]);
  RND_LOOP (rnd)
    {
      i = mpfr_dot (r, tabp, tabp, 3, (mpfr_rnd_t) rnd);
      if (!MPFR_IS_INF (r) || !MPFR_IS_POS (r) || i != 0)
        {
          printf ("Special case +INF failed for %s!\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
          exit (1);
        }
    }

  MPFR_SET_ZERO (tab[1]);
  RND_LOOP (rnd)
    {
      i = mpfr_dot (r, tabp, tabp, 2, (mpfr_rnd_t) rnd);
      if (mpfr_cmp_ui0 (r, 42*42) || i != 0)
        {
          printf ("Special case 42+0 failed for %s!\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
          exit (1);
        }
    }

  MPFR_SET_NAN (tab[0]);
  RND_LOOP (rnd)
    {
      i = mpfr_dot (r, tabp, tabp, 3, (mpfr_rnd_t) rnd);
      if (!MPFR_IS_NAN (r) || i != 0)
        {
          printf ("Special case NAN+0+-INF failed for %s!\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
          exit (1);
        }
    }

  mpfr_set_inf (tab[0], 1);
  mpfr_set_inf (tab[0], 1);
  mpfr_set_inf (tab[2], -1);
  RND_LOOP (rnd)
    {
      i = mpfr_dot (r, tabp, tabp + 1, 2, (mpfr_rnd_t) rnd);
      if (!MPFR_IS_NAN (r) || i != 0)
        {
          printf ("Special case inf*inf-inf*inf failed for %s!\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
          exit (1);
        }
    }

  mpfr_clears (tab[0], tab[1], tab[2], r, (mpfr_ptr) 0);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  check_simple ();
  check_special ();

  tests_end_mpfr ();

  return 0;
}
