/* Test file for mpfr_set.

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

static int error;

#define PRINT_ERROR_IF(condition, text)         \
  do {                                          \
    if (condition)                              \
      {                                         \
        printf ("%s", text);                    \
        error = 1;                              \
      }                                         \
  } while (0)


/* Maybe better create its own test file? */
static void
check_neg_special (void)
{
  mpfr_t x, y;
  int inexact;
  int s1, s2, s3;

  mpfr_inits2 (53, x, y, (mpfr_ptr) 0);

  MPFR_SET_NAN (x);
  s1 = mpfr_signbit (x) != 0;

  mpfr_clear_nanflag ();
  inexact = mpfr_neg (y, x, MPFR_RNDN);
  s2 = mpfr_signbit (y) != 0;
  PRINT_ERROR_IF (!mpfr_nanflag_p (),
                  "ERROR: neg (NaN) doesn't set Nan flag (1).\n");
  PRINT_ERROR_IF (!mpfr_nan_p (y) || inexact != 0,
                  "ERROR: neg (NaN) failed to set variable to NaN (1).\n");
  PRINT_ERROR_IF (s1 == s2,
                  "ERROR: neg (NaN) doesn't correctly flip sign bit (1).\n");

  mpfr_clear_nanflag ();
  inexact = mpfr_neg (x, x, MPFR_RNDN);
  s2 = mpfr_signbit (x) != 0;
  PRINT_ERROR_IF (!mpfr_nanflag_p (),
                  "ERROR: neg (NaN) doesn't set Nan flag (2).\n");
  PRINT_ERROR_IF (!mpfr_nan_p (x) || inexact != 0,
                  "ERROR: neg (NaN) failed to set variable to NaN (2).\n");
  /* check following "bug" is fixed:
     https://sympa.inria.fr/sympa/arc/mpfr/2017-11/msg00003.html */
  PRINT_ERROR_IF (s1 == s2,
                  "ERROR: neg (NaN) doesn't correctly flip sign bit (2).\n");

  mpfr_clear_nanflag ();
  inexact = mpfr_neg (y, x, MPFR_RNDN);
  s3 = mpfr_signbit (y) != 0;
  PRINT_ERROR_IF (!mpfr_nanflag_p (),
                  "ERROR: neg (NaN) doesn't set Nan flag (3).\n");
  PRINT_ERROR_IF (!mpfr_nan_p (y) || inexact != 0,
                  "ERROR: neg (NaN) failed to set variable to NaN (3).\n");
  PRINT_ERROR_IF (s2 == s3,
                  "ERROR: neg (NaN) doesn't correctly flip sign bit (3).\n");

  mpfr_clear_nanflag ();
  inexact = mpfr_neg (x, x, MPFR_RNDN);
  s3 = mpfr_signbit (x) != 0;
  PRINT_ERROR_IF (!mpfr_nanflag_p (),
                  "ERROR: neg (NaN) doesn't set Nan flag (4).\n");
  PRINT_ERROR_IF (!mpfr_nan_p (x) || inexact != 0,
                  "ERROR: neg (NaN) failed to set variable to NaN (4).\n");
  PRINT_ERROR_IF (s2 == s3,
                  "ERROR: neg (NaN) doesn't correctly flip sign bit (4).\n");

  mpfr_clears (x, y, (mpfr_ptr) 0);
}

static void
check_special (void)
{
  mpfr_t x, y;
  int inexact;
  int s1, s2;

  mpfr_inits2 (53, x, y, (mpfr_ptr) 0);

  mpfr_set_inf (x, 1);
  PRINT_ERROR_IF (!mpfr_inf_p (x) || mpfr_sgn (x) < 0,
                  "ERROR: mpfr_set_inf failed to set variable to +inf [1].\n");
  mpfr_set_inf (x, INT_MAX);
  PRINT_ERROR_IF (!mpfr_inf_p (x) || mpfr_sgn (x) < 0,
                  "ERROR: mpfr_set_inf failed to set variable to +inf [2].\n");
  mpfr_set_inf (x, 0);
  PRINT_ERROR_IF (!mpfr_inf_p (x) || mpfr_sgn (x) < 0,
                  "ERROR: mpfr_set_inf failed to set variable to +inf [3].\n");
  inexact = mpfr_set (y, x, MPFR_RNDN);
  PRINT_ERROR_IF (!mpfr_inf_p (y) || mpfr_sgn (y) < 0 || inexact != 0,
                  "ERROR: mpfr_set failed to set variable to +infinity.\n");

  inexact = mpfr_set_ui (y, 0, MPFR_RNDN);
  PRINT_ERROR_IF (MPFR_NOTZERO (y) || MPFR_IS_NEG (y) || inexact != 0,
                  "ERROR: mpfr_set_ui failed to set variable to +0.\n");

  mpfr_set_inf (x, -1);
  PRINT_ERROR_IF (!mpfr_inf_p (x) || mpfr_sgn (x) > 0,
                  "ERROR: mpfr_set_inf failed to set variable to -inf [1].\n");
  mpfr_set_inf (x, INT_MIN);
  PRINT_ERROR_IF (!mpfr_inf_p (x) || mpfr_sgn (x) > 0,
                  "ERROR: mpfr_set_inf failed to set variable to -inf [2].\n");
  inexact = mpfr_set (y, x, MPFR_RNDN);
  PRINT_ERROR_IF (!mpfr_inf_p (y) || mpfr_sgn (y) > 0 || inexact != 0,
                  "ERROR: mpfr_set failed to set variable to -infinity.\n");

  mpfr_set_zero (x, 1);
  PRINT_ERROR_IF (MPFR_NOTZERO (x) || MPFR_IS_NEG (x),
                  "ERROR: mpfr_set_zero failed to set variable to +0 [1].\n");
  mpfr_set_zero (x, INT_MAX);
  PRINT_ERROR_IF (MPFR_NOTZERO (x) || MPFR_IS_NEG (x),
                  "ERROR: mpfr_set_zero failed to set variable to +0 [2].\n");
  mpfr_set_zero (x, 0);
  PRINT_ERROR_IF (MPFR_NOTZERO (x) || MPFR_IS_NEG (x),
                  "ERROR: mpfr_set_zero failed to set variable to +0 [3].\n");
  inexact = mpfr_set (y, x, MPFR_RNDN);
  PRINT_ERROR_IF (MPFR_NOTZERO (y) || MPFR_IS_NEG (y) || inexact != 0,
                  "ERROR: mpfr_set failed to set variable to +0.\n");

  mpfr_set_zero (x, -1);
  PRINT_ERROR_IF (MPFR_NOTZERO (x) || MPFR_IS_POS (x),
                  "ERROR: mpfr_set_zero failed to set variable to -0 [1].\n");
  mpfr_set_zero (x, INT_MIN);
  PRINT_ERROR_IF (MPFR_NOTZERO (x) || MPFR_IS_POS (x),
                  "ERROR: mpfr_set_zero failed to set variable to -0 [2].\n");
  inexact = mpfr_set (y, x, MPFR_RNDN);
  PRINT_ERROR_IF (MPFR_NOTZERO (y) || MPFR_IS_POS (y) || inexact != 0,
                  "ERROR: mpfr_set failed to set variable to -0.\n");

  /* NaN tests */

  mpfr_set_nan (x);
  PRINT_ERROR_IF (!mpfr_nan_p (x),
                  "ERROR: mpfr_set_nan failed to set variable to NaN.\n");
  s1 = mpfr_signbit (x) != 0;

  mpfr_clear_nanflag ();
  inexact = mpfr_set (y, x, MPFR_RNDN);
  s2 = mpfr_signbit (y) != 0;
  PRINT_ERROR_IF (!mpfr_nanflag_p (),
                  "ERROR: mpfr_set doesn't set Nan flag (1).\n");
  PRINT_ERROR_IF (!mpfr_nan_p (y) || inexact != 0,
                  "ERROR: mpfr_set failed to set variable to NaN (1).\n");
  PRINT_ERROR_IF (s1 != s2,
                  "ERROR: mpfr_set doesn't preserve the sign bit (1).\n");

  mpfr_clear_nanflag ();
  inexact = mpfr_set (x, x, MPFR_RNDN);
  s2 = mpfr_signbit (x) != 0;
  PRINT_ERROR_IF (!mpfr_nanflag_p (),
                  "ERROR: mpfr_set doesn't set Nan flag (2).\n");
  PRINT_ERROR_IF (!mpfr_nan_p (x) || inexact != 0,
                  "ERROR: mpfr_set failed to set variable to NaN (2).\n");
  PRINT_ERROR_IF (s1 != s2,
                  "ERROR: mpfr_set doesn't preserve the sign bit (2).\n");

  MPFR_CHANGE_SIGN (x);
  s1 = !s1;

  mpfr_clear_nanflag ();
  inexact = mpfr_set (y, x, MPFR_RNDN);
  s2 = mpfr_signbit (y) != 0;
  PRINT_ERROR_IF (!mpfr_nanflag_p (),
                  "ERROR: mpfr_set doesn't set Nan flag (3).\n");
  PRINT_ERROR_IF (!mpfr_nan_p (y) || inexact != 0,
                  "ERROR: mpfr_set failed to set variable to NaN (3).\n");
  PRINT_ERROR_IF (s1 != s2,
                  "ERROR: mpfr_set doesn't preserve the sign bit (3).\n");

  mpfr_clear_nanflag ();
  inexact = mpfr_set (x, x, MPFR_RNDN);
  s2 = mpfr_signbit (x) != 0;
  PRINT_ERROR_IF (!mpfr_nanflag_p (),
                  "ERROR: mpfr_set doesn't set Nan flag (4).\n");
  PRINT_ERROR_IF (!mpfr_nan_p (x) || inexact != 0,
                  "ERROR: mpfr_set failed to set variable to NaN (4).\n");
  PRINT_ERROR_IF (s1 != s2,
                  "ERROR: mpfr_set doesn't preserve the sign bit (4).\n");

  mpfr_clears (x, y, (mpfr_ptr) 0);
}

static void
check_ternary_value (void)
{
  int k, p, q, rnd;
  int inexact, cmp;
  mpfr_t x, y;

  mpfr_init (x);
  mpfr_init (y);
  for (p=2; p<500; p++)
    {
      mpfr_set_prec (x, p);
      mpfr_urandomb (x, RANDS);
      if (RAND_BOOL ())
        mpfr_neg (x, x, MPFR_RNDN);
      for (q=2; q<2*p; q++)
        {
          mpfr_set_prec (y, q);
          RND_LOOP (rnd)
            {
              if (rnd == MPFR_RNDF) /* the test below makes no sense */
                continue;
              for (k = 0; k < 3; k++)
                {
                  int a = 0, b = 0, c = 0;

                  switch (k)
                    {
                    case 0:
                      inexact = mpfr_set (y, x, (mpfr_rnd_t) rnd);
                      break;
                    case 1:
                      inexact = (mpfr_set) (y, x, (mpfr_rnd_t) rnd);
                      break;
                    case 2:
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
                      inexact = mpfr_set ((a++, VOIDP_CAST(y)),
                                          (b++, VOIDP_CAST(x)),
                                          (c++, (mpfr_rnd_t) rnd));
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
                      MPFR_ASSERTN (a == 1);
                      MPFR_ASSERTN (b == 1);
                      MPFR_ASSERTN (c == 1);
                      break;
                    }
                  cmp = mpfr_cmp (y, x);
                  if (((inexact == 0) && (cmp != 0)) ||
                      ((inexact > 0) && (cmp <= 0)) ||
                      ((inexact < 0) && (cmp >= 0)))
                    {
                      printf ("Wrong ternary value in mpfr_set for %s (%d):"
                              " expected %d, got %d\n",
                              mpfr_print_rnd_mode ((mpfr_rnd_t) rnd),
                              k, cmp, inexact);
                      exit (1);
                    }
                }
            }
        }
    }
  mpfr_clear (x);
  mpfr_clear (y);
}

static void
test_set_1_2 (void)
{
  mpfr_t u, v, zz, z;
  int inex;

  /* (8,16)-bit test */
  mpfr_inits2 (16, u, v, zz, (mpfr_ptr) 0);
  mpfr_init2 (z, 8);
  mpfr_set_str_binary (u, "0.1100001100011010E-1");
  mpfr_set_str_binary (v, "0.1100010101110010E0");
  /* u + v = 1.0010011011111111 */
  inex = mpfr_add (zz, u, v, MPFR_RNDN);
  MPFR_ASSERTN(inex > 0);
  mpfr_set_str_binary (u, "1.001001110000000");
  MPFR_ASSERTN(mpfr_equal_p (zz, u));
  inex = mpfr_set_1_2 (z, zz, MPFR_RNDN, inex);
  /* we should have z = 1.0010011 and inex < 0 */
  MPFR_ASSERTN(inex < 0);
  mpfr_set_str_binary (u, "1.0010011");
  MPFR_ASSERTN(mpfr_equal_p (z, u));
  mpfr_clears (u, v, zz, z, (mpfr_ptr) 0);

  /* (16,32)-bit test:
   * take for v a random 32-bit number in [1/2,1), here 2859611790/2^32
   * take for z a random 16-bit number in [1,2), less than 2*v,
   with last bit 0, here we take z = 40900/2^15
   * take u = z-v-1/2^16-1/2^32 */
  mpfr_inits2 (32, u, v, zz, (mpfr_ptr) 0);
  mpfr_init2 (z, 16);
  mpfr_set_str_binary (u, "0.10010101000101001100100101110001");
  mpfr_set_str_binary (v, "0.10101010011100100011011010001110");
  /* u + v = 1.00111111100001101111111111111111 */
  inex = mpfr_add (zz, u, v, MPFR_RNDN);
  MPFR_ASSERTN(inex > 0);
  mpfr_set_str_binary (u, "1.0011111110000111");
  MPFR_ASSERTN(mpfr_equal_p (zz, u));
  inex = mpfr_set_1_2 (z, zz, MPFR_RNDN, inex);
  /* we should have z = 1.001111111000011 and inex < 0 */
  MPFR_ASSERTN(inex < 0);
  mpfr_set_str_binary (u, "1.001111111000011");
  MPFR_ASSERTN(mpfr_equal_p (z, u));
  mpfr_clears (u, v, zz, z, (mpfr_ptr) 0);

  /* (32,64)-bit test:
   * take for v a random 64-bit number in [1/2,1),
     here v = 13687985014345662879/2^64
   * take for z a random 32-bit number in [1,2), less than 2*v,
     with last bit 0, here we take z = 2871078774/2^31
   * take u = z-v-1/2^32-1/2^64 */
  mpfr_inits2 (64, u, v, zz, (mpfr_ptr) 0);
  mpfr_init2 (z, 32);
  mpfr_set_str_binary (u, "0.10011000010011001110000100010001110010010000111001111110011");
  mpfr_set_str_binary (v, "0.1011110111110101011111011101100100110110111100011000000110011111");
  /* u + v = 1.0101011001000010010111101110101011111111111111111111111111111111 */
  inex = mpfr_add (zz, u, v, MPFR_RNDN);
  MPFR_ASSERTN(inex > 0);
  mpfr_set_str_binary (u, "1.01010110010000100101111011101011");
  MPFR_ASSERTN(mpfr_equal_p (zz, u));
  inex = mpfr_set_1_2 (z, zz, MPFR_RNDN, inex);
  /* we should have z = 1.0101011001000010010111101110101 and inex < 0 */
  MPFR_ASSERTN(inex < 0);
  mpfr_set_str_binary (u, "1.0101011001000010010111101110101");
  MPFR_ASSERTN(mpfr_equal_p (z, u));
  mpfr_clears (u, v, zz, z, (mpfr_ptr) 0);

  /* (64,128)-bit test:
   * take for v a random 128-bit number in [1/2,1),
     here v = 322263811942091240216761391118876232409/2^128
   * take for z a random 64-bit number in [1,2), less than 2*v,
     with last bit 0, here we take z = 16440347967874738276/2^63
   * take u = z-v-1/2^64-1/2^128 */
  mpfr_inits2 (128, u, v, zz, (mpfr_ptr) 0);
  mpfr_init2 (z, 64);
  mpfr_set_str_binary (u, "0.1101010111011101111100100001011111111000010011011001000101111010110101101101011011100110101001010001101011011110101101010010011");
  mpfr_set_str_binary (v, "0.11110010011100011100000010100110100010011010110010111111010011000010100100101001000110010101101011100101001000010100101011011001");
  inex = mpfr_add (zz, u, v, MPFR_RNDN);
  MPFR_ASSERTN(inex > 0);
  mpfr_set_str_binary (u, "1.1100100001001111101100101011111010000001111110100101000011000111");
  MPFR_ASSERTN(mpfr_equal_p (zz, u));
  inex = mpfr_set_1_2 (z, zz, MPFR_RNDN, inex);
  MPFR_ASSERTN(inex < 0);
  mpfr_set_str_binary (u, "1.1100100001001111101100101011111010000001111110100101000011000110");
  MPFR_ASSERTN(mpfr_equal_p (z, u));
  mpfr_clears (u, v, zz, z, (mpfr_ptr) 0);
}

#define TEST_FUNCTION mpfr_set
#include "tgeneric.c"

int
main (void)
{
  mpfr_t x, y, z, u;
  int inexact;
  mpfr_exp_t emax;

  tests_start_mpfr ();

  test_set_1_2 ();

  /* Default : no error */
  error = 0;

  /* check prototypes of mpfr_init_set_* */
  inexact = mpfr_init_set_si (x, -1, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  inexact = mpfr_init_set (y, x, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  inexact = mpfr_init_set_ui (z, 1, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  inexact = mpfr_init_set_d (u, 1.0, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);

  emax = mpfr_get_emax ();
  set_emax (0);
  mpfr_set_prec (x, 3);
  mpfr_set_str_binary (x, "0.111");
  mpfr_set_prec (y, 2);
  mpfr_set (y, x, MPFR_RNDU);
  if (!(MPFR_IS_INF (y) && MPFR_IS_POS (y)))
    {
      printf ("Error for y=x=0.111 with px=3, py=2 and emax=0\nx=");
      mpfr_dump (x);
      printf ("y=");
      mpfr_dump (y);
      exit (1);
    }

  set_emax (emax);

  mpfr_set_prec (y, 11);
  mpfr_set_str_binary (y, "0.11111111100E-8");
  mpfr_set_prec (x, 2);
  mpfr_set (x, y, MPFR_RNDN);
  mpfr_set_str_binary (y, "1.0E-8");
  if (mpfr_cmp (x, y))
    {
      printf ("Error for y=0.11111111100E-8, prec=2, rnd=MPFR_RNDN\n");
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (u);

  check_ternary_value ();
  check_special ();
  check_neg_special ();

  test_generic (MPFR_PREC_MIN, 1000, 10);

  tests_end_mpfr ();
  return error;
}
