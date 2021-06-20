/* Test file for mpfr_can_round and mpfr_round_p.

Copyright 1999, 2001-2016 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

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
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <stdio.h>
#include <stdlib.h>

#include "mpfr-test.h"

#define MAX_LIMB_SIZE 100

static void
check_round_p (void)
{
  mp_limb_t buf[MAX_LIMB_SIZE];
  mp_size_t n, i;
  mpfr_prec_t p;
  mpfr_exp_t err;
  int r1, r2;

  for (n = 2 ; n <= MAX_LIMB_SIZE ; n++)
    {
      /* avoid mpn_random which leaks memory */
      for (i = 0; i < n; i++)
        buf[i] = randlimb ();
      /* force the number to be normalized */
      buf[n - 1] |= MPFR_LIMB_HIGHBIT;
      p = randlimb() % ((n-1) * GMP_NUMB_BITS) + MPFR_PREC_MIN;
      err = p + randlimb () % GMP_NUMB_BITS;
      r1 = mpfr_round_p (buf, n, err, p);
      r2 = mpfr_can_round_raw (buf, n, MPFR_SIGN_POS, err,
                               MPFR_RNDN, MPFR_RNDZ, p);
      if (r1 != r2)
        {
          printf ("mpfr_round_p(%d) != mpfr_can_round(%d)!\n"
                  "bn = %ld, err0 = %ld, prec = %lu\nbp = ",
                  r1, r2, n, (long) err, (unsigned long) p);
          gmp_printf ("%NX\n", buf, n);
          exit (1);
        }
    }
}

/* check x=2^i with precision px, error at most 1, and target precision prec */
static void
test_pow2 (mpfr_exp_t i, mpfr_prec_t px, mpfr_rnd_t r1, mpfr_rnd_t r2,
           mpfr_prec_t prec)
{
  mpfr_t x;
  int b, expected_b, b2;

  mpfr_init2 (x, px);
  mpfr_set_ui_2exp (x, 1, i, MPFR_RNDN);
  b = !!mpfr_can_round (x, i+1, r1, r2, prec);
  /* Note: If mpfr_can_round succeeds for both
     (r1,r2) = (MPFR_RNDD,MPFR_RNDN) and
     (r1,r2) = (MPFR_RNDU,MPFR_RNDN), then it should succeed for
     (r1,r2) = (MPFR_RNDN,MPFR_RNDN). So, the condition on prec below
     for r1 = MPFR_RNDN should be the most restrictive between those
     for r1 = any directed rounding mode.
     For r1 like MPFR_RNDA, the unrounded, unknown number may be anyone
     in [2^i-1,i]. As both 2^i-1 and 2^i fit on i bits, one cannot round
     in any precision >= i bits, hence the condition prec < i; prec = i-1
     will work here for r2 = MPFR_RNDN thanks to the even-rounding rule
     (and also with rounding ties away from zero). */
  expected_b =
    MPFR_IS_LIKE_RNDD (r1, MPFR_SIGN_POS) ?
    (MPFR_IS_LIKE_RNDU (r2, MPFR_SIGN_POS) ? 0 : prec <= i) :
    MPFR_IS_LIKE_RNDU (r1, MPFR_SIGN_POS) ?
    (MPFR_IS_LIKE_RNDD (r2, MPFR_SIGN_POS) ? 0 : prec < i) :
    (r2 != MPFR_RNDN ? 0 : prec < i);
  /* We only require mpfr_can_round to return 1 when we can really
     round, it is allowed to return 0 in some rare boundary cases,
     for example when x = 2^k and the error is 0.25 ulp.
     Note: if this changes in the future, the test could be improved by
     removing the "&& expected_b == 0" below. */
  if (b != expected_b && expected_b == 0)
    {
      printf ("Error for x=2^%d, px=%lu, err=%d, r1=%s, r2=%s, prec=%d\n",
              (int) i, (unsigned long) px, (int) i + 1,
              mpfr_print_rnd_mode (r1), mpfr_print_rnd_mode (r2), (int) prec);
      printf ("Expected %d, got %d\n", expected_b, b);
      exit (1);
    }

  if (r1 == MPFR_RNDN && r2 == MPFR_RNDZ)
    {
      /* Similar test to the one done in src/round_p.c
         for MPFR_WANT_ASSERT >= 2. */
      b2 = !!mpfr_round_p (MPFR_MANT(x), MPFR_LIMB_SIZE(x), i+1, prec);
      if (b2 != b)
        {
          printf ("Error for x=2^%d, px=%lu, err=%d, prec=%d\n",
                  (int) i, (unsigned long) px, (int) i + 1, (int) prec);
          printf ("mpfr_can_round gave %d, mpfr_round_p gave %d\n", b, b2);
          exit (1);
        }
    }

  mpfr_clear (x);
}

int
main (void)
{
  mpfr_t x;
  mpfr_prec_t i, j, k;
  int r1, r2;
  int n;

  tests_start_mpfr ();

  /* checks that rounds to nearest sets the last
     bit to zero in case of equal distance */
  mpfr_init2 (x, 59);
  mpfr_set_str_binary (x, "-0.10010001010111000011110010111010111110000000111101100111111E663");
  if (mpfr_can_round (x, 54, MPFR_RNDZ, MPFR_RNDZ, 53) != 0)
    {
      printf ("Error (1) in mpfr_can_round\n");
      exit (1);
    }

  mpfr_set_str_binary (x, "-Inf");
  if (mpfr_can_round (x, 2000, MPFR_RNDZ, MPFR_RNDZ, 2000) != 0)
    {
      printf ("Error (2) in mpfr_can_round\n");
      exit (1);
    }

  mpfr_set_prec (x, 64);
  mpfr_set_str_binary (x, "0.1011001000011110000110000110001111101011000010001110011000000000");
  if (mpfr_can_round (x, 65, MPFR_RNDN, MPFR_RNDN, 54))
    {
      printf ("Error (3) in mpfr_can_round\n");
      exit (1);
    }

  mpfr_set_prec (x, 137);
  mpfr_set_str_binary (x, "-0.10111001101001010110011000110100111010011101101010010100101100001110000100111111011101010110001010111100100101110111100001000010000000000E-97");
  if (mpfr_can_round (x, 132, MPFR_RNDU, MPFR_RNDZ, 128) == 0)
    {
      printf ("Error (4) in mpfr_can_round\n");
      exit (1);
    }

  /* in the following, we can round but cannot determine the inexact flag */
  mpfr_set_prec (x, 86);
  mpfr_set_str_binary (x, "-0.11100100010011001111011010100111101010011000000000000000000000000000000000000000000000E-80");
  if (mpfr_can_round (x, 81, MPFR_RNDU, MPFR_RNDZ, 44) == 0)
    {
      printf ("Error (5) in mpfr_can_round\n");
      exit (1);
    }

  mpfr_set_prec (x, 62);
  mpfr_set_str (x, "0.ff4ca619c76ba69", 16, MPFR_RNDZ);
  for (i = 30; i < 99; i++)
    for (j = 30; j < 99; j++)
      for (r1 = 0; r1 < MPFR_RND_MAX; r1++)
        for (r2 = 0; r2 < MPFR_RND_MAX; r2++)
          {
            /* test for assertions */
            mpfr_can_round (x, i, (mpfr_rnd_t) r1, (mpfr_rnd_t) r2, j);
          }

  test_pow2 (32, 32, MPFR_RNDN, MPFR_RNDN, 32);
  test_pow2 (174, 174, MPFR_RNDN, MPFR_RNDN, 174);
  test_pow2 (174, 174, MPFR_RNDU, MPFR_RNDN, 174);
  test_pow2 (176, 129, MPFR_RNDU, MPFR_RNDU, 174);
  test_pow2 (176, 2, MPFR_RNDZ, MPFR_RNDZ, 174);
  test_pow2 (176, 2, MPFR_RNDU, MPFR_RNDU, 176);

  /* Tests for x = 2^i (E(x) = i+1) with error at most 1 = 2^0. */
  for (n = 0; n < 100; n++)
    {
      i = (randlimb() % 200) + 4;
      for (j = i - 2; j < i + 2; j++)
        for (r1 = 0; r1 < MPFR_RND_MAX; r1++)
          for (r2 = 0; r2 < MPFR_RND_MAX; r2++)
            for (k = MPFR_PREC_MIN; k <= i + 2; k++)
              test_pow2 (i, k, (mpfr_rnd_t) r1, (mpfr_rnd_t) r2, j);
    }

  mpfr_clear (x);

  check_round_p ();

  tests_end_mpfr ();
  return 0;
}
