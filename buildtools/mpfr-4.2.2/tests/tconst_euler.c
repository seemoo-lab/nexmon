/* Test file for mpfr_const_euler.

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

/* Wrapper for tgeneric */
static int
my_const_euler (mpfr_ptr x, mpfr_srcptr y, mpfr_rnd_t r)
{
  return mpfr_const_euler (x, r);
}

#define RAND_FUNCTION(x) mpfr_set_ui ((x), 0, MPFR_RNDN)
#define TEST_FUNCTION my_const_euler
#include "tgeneric.c"

static void
exercise_Ziv (void)
{
  mpfr_t x, y;
  int inex;

  mpfr_init2 (x, 177);
  mpfr_init2 (y, 177);
  inex = mpfr_const_euler (x, MPFR_RNDN);
  mpfr_set_str_binary (y, "0.10010011110001000110011111100011011111011011000011000111101001001101000110111110001111111000000100000001010100101100101101010110101000011100111011001100001110101111011001011101");
  MPFR_ASSERTN(mpfr_equal_p (x, y));
  MPFR_ASSERTN(inex > 0);
  mpfr_clear (x);
  mpfr_clear (y);
}

int
main (int argc, char *argv[])
{
  mpfr_t gamma, y, z, t;
  unsigned int err, prec, yprec, p0 = 2, p1 = 200;
  int rnd;

  tests_start_mpfr ();

  prec = (argc < 2) ? 53 : atoi(argv[1]);

  if (argc > 1)
    {
      mpfr_init2 (gamma, prec);
      mpfr_const_euler (gamma, MPFR_RNDN);
      printf("gamma="); mpfr_out_str (stdout, 10, 0, gamma, MPFR_RNDD);
      puts ("");
      mpfr_clear (gamma);
      return 0;
    }

  exercise_Ziv ();

  mpfr_init (y);
  mpfr_init (z);
  mpfr_init (t);

  mpfr_set_prec (y, 32);
  mpfr_set_prec (z, 32);
  (mpfr_const_euler) (y, MPFR_RNDN);
  mpfr_set_str_binary (z, "0.10010011110001000110011111100011");
  if (mpfr_cmp (y, z))
    {
      printf ("Error for prec=32\n");
      exit (1);
    }

  for (prec = p0; prec <= p1; prec++)
    {
      mpfr_set_prec (z, prec);
      mpfr_set_prec (t, prec);
      yprec = prec + 10;

      RND_LOOP_NO_RNDF (rnd)
        {
          mpfr_set_prec (y, yprec);
          mpfr_const_euler (y, (mpfr_rnd_t) rnd);
          err = (rnd == MPFR_RNDN) ? yprec + 1 : yprec;
          /* Note: for rnd = RNDF, rnd1 = RNDF is equivalent to rnd1 = RNDN
             in mpfr_can_round, thus rnd2 = RNDF reduces to rnd2 = RNDN in that
             case, we are duplicating the test for rnd = RNDN. */
          if (mpfr_can_round (y, err, (mpfr_rnd_t) rnd, (mpfr_rnd_t) rnd, prec))
            {
              mpfr_set (t, y, (mpfr_rnd_t) rnd);
              mpfr_const_euler (z, (mpfr_rnd_t) rnd);
              if (mpfr_cmp (t, z))
                {
                  printf ("results differ for prec=%u rnd_mode=%s\n", prec,
                          mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                  printf ("   got      ");
                  mpfr_out_str (stdout, 2, prec, z, MPFR_RNDN);
                  puts ("");
                  printf ("   expected ");
                  mpfr_out_str (stdout, 2, prec, t, MPFR_RNDN);
                  puts ("");
                  printf ("   approximation was ");
                  mpfr_dump (y);
                  exit (1);
                }
            }
        }
    }

  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (t);

  test_generic (MPFR_PREC_MIN, 200, 1);

  tests_end_mpfr ();
  return 0;
}
