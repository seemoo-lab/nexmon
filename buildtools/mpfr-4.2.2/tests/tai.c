/* Test file for mpfr_ai.

Copyright 2010-2025 Free Software Foundation, Inc.
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

#define TEST_FUNCTION mpfr_ai
#define TEST_RANDOM_EMIN -5
#define TEST_RANDOM_EMAX 5
#define REDUCE_EMAX 7 /* this is to avoid that test_generic() calls mpfr_ai
                         with too large inputs. FIXME: remove this once
                         mpfr_ai can handle large inputs */
#include "tgeneric.c"

static void
check_large (void)
{
  mpfr_t x, y, z;

  mpfr_init2 (x, 38);
  mpfr_init2 (y, 110);
  mpfr_init2 (z, 110);
  mpfr_set_str_binary (x, "-1E8");
  mpfr_ai (y, x, MPFR_RNDN);
  mpfr_set_str_binary (z, "-10001110100001011111110001100011101100011100010000110100100101011111011100000101110101010010000000101110011111E-112");
  if (mpfr_equal_p (y, z) == 0)
    {
      printf ("Error in mpfr_ai for x=-2^8\n");
      exit (1);
    }
#if 0 /* disabled since mpfr_ai does not currently handle large arguments */
  mpfr_set_str_binary (x, "-1E26");
  mpfr_ai (y, x, MPFR_RNDN);
  mpfr_set_str_binary (z, "-110001111100000011001010010101001101001011001011101011001010100100001110001101101101000010000011001000001011E-118");
  if (mpfr_equal_p (y, z) == 0)
    {
      printf ("Error in mpfr_ai for x=-2^26\n");
      exit (1);
    }
  mpfr_set_str_binary (x, "-0.11111111111111111111111111111111111111E1073741823");
  mpfr_ai (y, x, MPFR_RNDN);
  /* FIXME: compute the correctly rounded value we should get for Ai(x),
     and check we get this value */
#endif
  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

static void
check_zero (void)
{
  mpfr_t x, y, r;

  mpfr_init2 (x, 53);
  mpfr_init2 (y, 53);
  mpfr_init2 (r, 53);

  mpfr_set_str_binary (r, "10110101110001100011110010110001001110001010110111E-51");

  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_ai (y, x, MPFR_RNDN);
  if (mpfr_equal_p (y, r) == 0)
    {
      printf ("Error in mpfr_ai for x=0\n");
      printf ("Expected "); mpfr_dump (r);
      printf ("Got      "); mpfr_dump (y);
      exit (1);
    }
  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (r);
}

static void
bug20180107 (void)
{
  mpfr_t x, y, z;
  mpfr_exp_t emin;
  int inex;
  mpfr_flags_t flags;

  mpfr_init2 (x, 152);
  mpfr_init2 (y, 11);
  mpfr_init2 (z, 11);
  mpfr_set_str_binary (x, "0.11010101100111000111001001010110101001100001011110101111000010100111011101011110000100111011101100100100001010000110100011001000111010010001110000011100E5");
  emin = mpfr_get_emin ();
  set_emin (-134);
  mpfr_clear_flags ();
  inex = mpfr_ai (y, x, MPFR_RNDA);
  flags = __gmpfr_flags;
  /* result should be 0.10011100000E-135 with unlimited exponent range,
     and thus should be rounded to 0.1E-134 */
  mpfr_set_str_binary (z, "0.1E-134");
  MPFR_ASSERTN (mpfr_equal_p (y, z));
  MPFR_ASSERTN (inex > 0);
  MPFR_ASSERTN (flags == (MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT));

  mpfr_set_prec (x, 2);
  mpfr_set_str_binary (x, "0.11E7");
  mpfr_set_prec (y, 2);
  mpfr_clear_flags ();
  inex = mpfr_ai (y, x, MPFR_RNDA);
  flags = __gmpfr_flags;
  /* result should be 1.0E-908 with unlimited exponent range,
     and thus should be rounded to 0.1E-134 */
  mpfr_set_str_binary (z, "0.1E-134");
  MPFR_ASSERTN (mpfr_equal_p (y, z));
  MPFR_ASSERTN (inex > 0);
  MPFR_ASSERTN (flags == (MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT));

  set_emin (emin);
  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

/* exercise mpfr_ai near m*2^e, for precision p */
static void
test_near_m2e (long m, mpfr_exp_t e, mpfr_prec_t pmax)
{
  mpfr_t x, xx, y, yy;
  mpfr_prec_t p;
  int inex;

  mpfr_clear_flags ();

  /* first determine the smallest precision for which m*2^e is exact */
  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    {
      mpfr_init2 (x, p);
      inex = mpfr_set_si_2exp (x, m, e, MPFR_RNDN);
      mpfr_clear (x);
      if (inex == 0)
        break;
    }

  mpfr_init2 (x, p);
  inex = mpfr_set_si_2exp (x, m, e, MPFR_RNDN);
  MPFR_ASSERTN(inex == 0);

  for (; p <= pmax; p++)
    {
      mpfr_init2 (y, p);
      mpfr_init2 (xx, p);
      mpfr_init2 (yy, p);
      mpfr_prec_round (x, p, MPFR_RNDN);
      mpfr_ai (y, x, MPFR_RNDN);
      while (1)
        {
          mpfr_set (xx, x, MPFR_RNDN);
          mpfr_nextbelow (xx);
          mpfr_ai (yy, xx, MPFR_RNDN);
          if (mpfr_cmpabs (yy, y) >= 0)
            break;
          else
            {
              mpfr_set (x, xx, MPFR_RNDN);
              mpfr_set (y, yy, MPFR_RNDN);
            }
        }
      while (1)
        {
          mpfr_set (xx, x, MPFR_RNDN);
          mpfr_nextabove (xx);
          mpfr_ai (yy, xx, MPFR_RNDN);
          if (mpfr_cmpabs (yy, y) >= 0)
            break;
          else
            {
              mpfr_set (x, xx, MPFR_RNDN);
              mpfr_set (y, yy, MPFR_RNDN);
            }
        }
      mpfr_clear (y);
      mpfr_clear (xx);
      mpfr_clear (yy);
    }

  mpfr_clear (x);

  /* Since some tests don't really check that the result is not NaN... */
  MPFR_ASSERTN (! mpfr_nanflag_p ());
}

/* example provided by Sylvain Chevillard, which exercises the case
   wprec < err + 1, and thus correct_bits = 0, in src/ai.c */
static void
coverage (void)
{
  mpfr_t x, y;
  int inex;

  mpfr_init2 (x, 800);
  mpfr_init2 (y, 20);
  mpfr_set_str (x, "-2.3381074104597670384891972524467354406385401456723878524838544372136680027002836477821640417313293202847600938532659527752254668583598667448688987168197275409731526749911127480659996456283534915503672", 10, MPFR_RNDN);
  inex = mpfr_ai (y, x, MPFR_RNDN);
  MPFR_ASSERTN(inex < 0);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (y, 593131, -682) == 0);

  mpfr_clear (x);
  mpfr_clear (y);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  coverage ();
  test_near_m2e (-5, -1, 100); /* exercise near -2.5 */
  test_near_m2e (-4, 0, 100); /* exercise near -4 */
  test_near_m2e (-11, -1, 100); /* exercise near -5.5 */
  test_near_m2e (-27, -2, 100); /* exercise near -6.75 */
  test_near_m2e (-31, -2, 100); /* exercise near -7.75 */
  test_near_m2e (-15, -1, 100); /* exercise near -7.5 */
  bug20180107 ();
  check_large ();
  check_zero ();

  test_generic (MPFR_PREC_MIN, 100, 5);

  tests_end_mpfr ();
  return 0;
}
