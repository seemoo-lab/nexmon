/* Test file for mpfr_sin.

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
test_sin (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode)
{
  int res;
  int ok = rnd_mode == MPFR_RNDN && mpfr_number_p (b) && mpfr_get_prec (a)>=53;
  if (ok)
    {
      mpfr_print_raw (b);
    }
  res = mpfr_sin (a, b, rnd_mode);
  if (ok)
    {
      printf (" ");
      mpfr_print_raw (a);
      printf ("\n");
    }
  return res;
}
#else
#define test_sin mpfr_sin
#endif

static void
check53 (const char *xs, const char *sin_xs, mpfr_rnd_t rnd_mode)
{
  mpfr_t xx, s;

  mpfr_init2 (xx, 53);
  mpfr_init2 (s, 53);
  mpfr_set_str1 (xx, xs); /* should be exact */
  test_sin (s, xx, rnd_mode);
  if (mpfr_cmp_str1 (s, sin_xs))
    {
      printf ("mpfr_sin failed for x=%s, rnd=%s\n",
              xs, mpfr_print_rnd_mode (rnd_mode));
      printf ("mpfr_sin gives sin(x)=");
      mpfr_out_str (stdout, 10, 0, s, MPFR_RNDN);
      printf (", expected %s\n", sin_xs);
      exit (1);
    }
  mpfr_clear (xx);
  mpfr_clear (s);
}

static void
check53b (const char *xs, const char *sin_xs, mpfr_rnd_t rnd_mode)
{
  mpfr_t xx, s;

  mpfr_init2 (xx, 53);
  mpfr_init2 (s, 53);
  mpfr_set_str (xx, xs, 2, MPFR_RNDN); /* should be exact */
  test_sin (s, xx, rnd_mode);
  if (mpfr_cmp_str (s, sin_xs, 2, MPFR_RNDN))
    {
      printf ("mpfr_sin failed in rounding mode %s for\n     x = %s\n",
              mpfr_print_rnd_mode (rnd_mode), xs);
      printf ("     got ");
      mpfr_out_str (stdout, 2, 0, s, MPFR_RNDN);
      printf ("\nexpected %s\n", sin_xs);
      exit (1);
    }
  mpfr_clear (xx);
  mpfr_clear (s);
}

static void
test_sign (void)
{
  mpfr_t pid, piu, x, y;
  int p, k;

  mpfr_init2 (pid, 4096);
  mpfr_const_pi (pid, MPFR_RNDD);
  mpfr_init2 (piu, 4096);
  mpfr_const_pi (piu, MPFR_RNDU);
  mpfr_init (x);
  mpfr_init2 (y, 2);
  for (p = 8; p <= 128; p++)
    for (k = 2; k <= 6; k += 2)
      {
        mpfr_set_prec (x, p);
        mpfr_mul_ui (x, pid, k, MPFR_RNDD);
        test_sin (y, x, MPFR_RNDN);
        if (MPFR_IS_POS (y))
          {
            printf ("Error in test_sign for sin(%dpi-epsilon), prec = %d"
                    " for argument.\nResult should have been negative.\n",
                    k, p);
            exit (1);
          }
        mpfr_mul_ui (x, piu, k, MPFR_RNDU);
        test_sin (y, x, MPFR_RNDN);
        if (MPFR_IS_NEG (y))
          {
            printf ("Error in test_sign for sin(%dpi+epsilon), prec = %d"
                    " for argument.\nResult should have been positive.\n",
                    k, p);
            exit (1);
          }
      }

  /* worst case on 53 bits */
  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);
  mpfr_set_str (x, "6134899525417045", 10, MPFR_RNDN);
  test_sin (y, x, MPFR_RNDN);
  mpfr_set_str_binary (x, "11011010111101011110111100010101010101110000000001011E-106");
  MPFR_ASSERTN(mpfr_cmp (x, y) == 0);

  /* Bug on Special cases */
  mpfr_set_str_binary (x, "0.100011011010111101E-32");
  test_sin (y, x, MPFR_RNDN);
  if (mpfr_cmp_str (y, "0.10001101101011110100000000000000000000000000000000000E-32", 2, MPFR_RNDN))
    {
      printf("sin special 97 error:\nx=");
      mpfr_dump (x); printf("y=");
      mpfr_dump (y);
      exit (1);
    }

  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);
  mpfr_set_str_binary (x, "1.1001001000011111101101010100010001000010110100010011");
  mpfr_set_str_binary (y, "1.1111111111111111111111111111111111111111111111111111e-1");
  test_sin (x, x, MPFR_RNDZ);
  MPFR_ASSERTN(mpfr_cmp (x, y) == 0);

  mpfr_clear (pid);
  mpfr_clear (piu);
  mpfr_clear (x);
  mpfr_clear (y);
}

static void
check_nans (void)
{
  mpfr_t  x, y;

  mpfr_init2 (x, 123L);
  mpfr_init2 (y, 123L);

  mpfr_set_nan (x);
  test_sin (y, x, MPFR_RNDN);
  if (! mpfr_nan_p (y))
    {
      printf ("Error: sin(NaN) != NaN\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  test_sin (y, x, MPFR_RNDN);
  if (! mpfr_nan_p (y))
    {
      printf ("Error: sin(Inf) != NaN\n");
      exit (1);
    }

  mpfr_set_inf (x, -1);
  test_sin (y, x, MPFR_RNDN);
  if (! mpfr_nan_p (y))
    {
      printf ("Error: sin(-Inf) != NaN\n");
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
}

#define TEST_FUNCTION test_sin
#ifndef MPFR_USE_MINI_GMP
#define REDUCE_EMAX 262143 /* otherwise arg. reduction is too expensive */
#else
#define REDUCE_EMAX 16383  /* reduce further since mini-gmp works in O(n^2) */
#endif
#include "tgeneric.c"

const char xs[] = "0.111011111110110000111000001100000111110E-1";

static void
check_regression (void)
{
  mpfr_t x, y;
  mpfr_prec_t p;
  int i;

  p = strlen (xs) - 2 - 3;
  mpfr_inits2 (p, x, y, (mpfr_ptr) 0);

  mpfr_set_str (x, xs, 2, MPFR_RNDN);
  i = mpfr_sin (y, x, MPFR_RNDN);
  if (i >= 0
      || mpfr_cmp_str (y, "0.111001110011110011110001010110011101110E-1",
                       2, MPFR_RNDN))
    {
      printf ("Regression test failed (1) i=%d\ny=", i);
      mpfr_dump (y);
      exit (1);
    }
  mpfr_clears (x, y, (mpfr_ptr) 0);
}

/* Test provided by Christopher Creutzig, 2007-05-21. */
static void
check_tiny (void)
{
  mpfr_t x, y;

  mpfr_init2 (x, 53);
  mpfr_init2 (y, 53);

  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_set_exp (x, mpfr_get_emin ());
  mpfr_sin (y, x, MPFR_RNDD);
  if (mpfr_cmp (x, y) < 0)
    {
      printf ("Error in check_tiny: got sin(x) > x for x = 2^(emin-1)\n");
      exit (1);
    }

  mpfr_sin (y, x, MPFR_RNDU);
  mpfr_mul_2ui (y, y, 1, MPFR_RNDU);
  if (mpfr_cmp (x, y) > 0)
    {
      printf ("Error in check_tiny: got sin(x) < x/2 for x = 2^(emin-1)\n");
      exit (1);
    }

  mpfr_neg (x, x, MPFR_RNDN);
  mpfr_sin (y, x, MPFR_RNDU);
  if (mpfr_cmp (x, y) > 0)
    {
      printf ("Error in check_tiny: got sin(x) < x for x = -2^(emin-1)\n");
      exit (1);
    }

  mpfr_sin (y, x, MPFR_RNDD);
  mpfr_mul_2ui (y, y, 1, MPFR_RNDD);
  if (mpfr_cmp (x, y) < 0)
    {
      printf ("Error in check_tiny: got sin(x) > x/2 for x = -2^(emin-1)\n");
      exit (1);
    }

  mpfr_clear (y);
  mpfr_clear (x);
}

static void
check_binary128 (void)
{
  mpfr_t x, y, z;

  mpfr_init2 (x, 113);
  mpfr_init2 (y, 113);
  mpfr_init2 (z, 113);

  /* number closest to a odd multiple of pi/2 in the binary128 format:
     8794873135033829349702184924722639 * 2^1852 */
  mpfr_set_str (x, "1b19ee7c329d7d951906d1e11b5cfp1852", 16, MPFR_RNDN);
  mpfr_cos (y, x, MPFR_RNDN);
  mpfr_set_str (z, "1.ad1a2037cd7820f748483f5d39c3p-124", 16, MPFR_RNDN);
  if (! mpfr_equal_p (y, z))
    {
      printf ("Error in check_binary128 (cos x)\n");
      printf ("expected "); mpfr_dump (z);
      printf ("got      "); mpfr_dump (y);
      exit (1);
    }
  mpfr_sin (y, x, MPFR_RNDN);
  mpfr_set_ui (z, 1, MPFR_RNDN);
  if (! mpfr_equal_p (y, z))
    {
      printf ("Error in check_binary128 (sin x)\n");
      printf ("expected "); mpfr_dump (z);
      printf ("got      "); mpfr_dump (y);
      exit (1);
    }

  /* now multiply x by 2, so that it is near an even multiple of pi/2 */
  mpfr_mul_2ui (x, x, 1, MPFR_RNDN);
  mpfr_cos (y, x, MPFR_RNDN);
  mpfr_set_si (z, -1, MPFR_RNDN);
  if (! mpfr_equal_p (y, z))
    {
      printf ("Error in check_binary128 (cos 2x)\n");
      printf ("expected "); mpfr_dump (z);
      printf ("got      "); mpfr_dump (y);
      exit (1);
    }
  mpfr_sin (y, x, MPFR_RNDN);
  mpfr_set_str (z, "3.5a34406f9af041ee90907eba7386p-124", 16, MPFR_RNDN);
  if (! mpfr_equal_p (y, z))
    {
      printf ("Error in check_binary128 (sin 2x)\n");
      printf ("expected "); mpfr_dump (z);
      printf ("got      "); mpfr_dump (y);
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

/* check Ziv's loop with precision 212 bits */
static void
check_212 (void)
{
  mpfr_t x, y, z;

  mpfr_init2 (x, 212);
  mpfr_init2 (y, 212);
  mpfr_init2 (z, 212);

  mpfr_set_str (x, "f.c34b10aa02f796d435a3db0146b4e8a0b2850422f778af06be66p+0", 16, MPFR_RNDN);
  mpfr_sin (y, x, MPFR_RNDN);
  mpfr_set_str (z, "-e.0c2d5c189f8a0d185d7036b87b90f3040f4f2aa0f46f901bad44p-8", 16, MPFR_RNDN);
  if (! mpfr_equal_p (y, z))
    {
      printf ("Error in check_212\n");
      printf ("expected "); mpfr_dump (z);
      printf ("got      "); mpfr_dump (y);
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

int
main (int argc, char *argv[])
{
  mpfr_t x, c, s, c2, s2;

  tests_start_mpfr ();

  check_regression ();
  check_nans ();

  /* worst case from PhD thesis of Vincent Lefe`vre: x=8980155785351021/2^54 */
  check53 ("4.984987858808754279e-1", "4.781075595393330379e-1", MPFR_RNDN);
  check53 ("4.984987858808754279e-1", "4.781075595393329824e-1", MPFR_RNDD);
  check53 ("4.984987858808754279e-1", "4.781075595393329824e-1", MPFR_RNDZ);
  check53 ("4.984987858808754279e-1", "4.781075595393330379e-1", MPFR_RNDU);
  check53 ("1.00031274099908640274",  "8.416399183372403892e-1", MPFR_RNDN);
  check53 ("1.00229256850978698523",  "8.427074524447979442e-1", MPFR_RNDZ);
  check53 ("1.00288304857059840103",  "8.430252033025980029e-1", MPFR_RNDZ);
  check53 ("1.00591265847407274059",  "8.446508805292128885e-1", MPFR_RNDN);

  /* Other worst cases showing a bug introduced on 2005-01-29 in rev 3248 */
  check53b ("1.0111001111010111010111111000010011010001110001111011e-21",
            "1.0111001111010111010111111000010011010001101001110001e-21",
            MPFR_RNDU);
  check53b ("1.1011101111111010000001010111000010000111100100101101",
            "1.1111100100101100001111100000110011110011010001010101e-1",
            MPFR_RNDU);

  mpfr_init2 (x, 2);

  mpfr_set_str (x, "0.5", 10, MPFR_RNDN);
  test_sin (x, x, MPFR_RNDD);
  if (mpfr_cmp_ui_2exp (x, 3, -3)) /* x != 0.375 = 3/8 */
    {
      printf ("mpfr_sin(0.5, MPFR_RNDD) failed with precision=2\n");
      exit (1);
    }

  /* bug found by Kevin Ryde */
  mpfr_const_pi (x, MPFR_RNDN);
  mpfr_mul_ui (x, x, 3L, MPFR_RNDN);
  mpfr_div_ui (x, x, 2L, MPFR_RNDN);
  test_sin (x, x, MPFR_RNDN);
  if (mpfr_cmp_ui (x, 0) >= 0)
    {
      printf ("Error: wrong sign for sin(3*Pi/2)\n");
      exit (1);
    }

  /* Can fail on an assert */
  mpfr_set_prec (x, 53);
  mpfr_set_str (x, "77291789194529019661184401408", 10, MPFR_RNDN);
  mpfr_init2 (c, 4); mpfr_init2 (s, 42);
  mpfr_init2 (c2, 4); mpfr_init2 (s2, 42);

  test_sin (s, x, MPFR_RNDN);
  mpfr_cos (c, x, MPFR_RNDN);
  mpfr_sin_cos (s2, c2, x, MPFR_RNDN);
  if (mpfr_cmp (c2, c))
    {
      printf("cos differs for x=77291789194529019661184401408");
      exit (1);
    }
  if (mpfr_cmp (s2, s))
    {
      printf("sin differs for x=77291789194529019661184401408");
      exit (1);
    }

  mpfr_set_str_binary (x, "1.1001001000011111101101010100010001000010110100010011");
  test_sin (x, x, MPFR_RNDZ);
  if (mpfr_cmp_str (x, "1.1111111111111111111111111111111111111111111111111111e-1", 2, MPFR_RNDN))
    {
      printf ("Error for x= 1.1001001000011111101101010100010001000010110100010011\nGot ");
      mpfr_dump (x);
      exit (1);
    }

  mpfr_set_prec (s, 9);
  mpfr_set_prec (x, 190);
  mpfr_const_pi (x, MPFR_RNDN);
  mpfr_sin (s, x, MPFR_RNDZ);
  if (mpfr_cmp_str (s, "0.100000101e-196", 2, MPFR_RNDN))
    {
      printf ("Error for x ~= pi\n");
      mpfr_dump (s);
      exit (1);
    }

  mpfr_clear (s2);
  mpfr_clear (c2);
  mpfr_clear (s);
  mpfr_clear (c);
  mpfr_clear (x);

  test_generic (MPFR_PREC_MIN, 100, 15);
  test_generic (MPFR_SINCOS_THRESHOLD-1, MPFR_SINCOS_THRESHOLD+1, 2);
  test_sign ();
  check_tiny ();
  check_binary128 ();
  check_212 ();

  data_check ("data/sin", mpfr_sin, "mpfr_sin");
  bad_cases (mpfr_sin, mpfr_asin, "mpfr_sin", 256, -40, 0, 4, 128, 800, 50);

  tests_end_mpfr ();
  return 0;
}
