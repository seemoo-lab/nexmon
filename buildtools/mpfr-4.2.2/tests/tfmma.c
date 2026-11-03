/* Test file for mpfr_fmma and mpfr_fmms.

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

/* check both mpfr_fmma and mpfr_fmms */
static void
random_test (mpfr_ptr a, mpfr_ptr b, mpfr_ptr c, mpfr_ptr d, mpfr_rnd_t rnd)
{
  mpfr_t ref, res, ab, cd;
  int inex_ref, inex_res;
  mpfr_prec_t p = MPFR_PREC(a);

  mpfr_init2 (res, p);
  mpfr_init2 (ref, p);
  mpfr_init2 (ab, mpfr_get_prec (a) +  mpfr_get_prec (b));
  mpfr_init2 (cd, mpfr_get_prec (c) +  mpfr_get_prec (d));

  /* first check fmma */
  inex_res = mpfr_fmma (res, a, b, c, d, rnd);
  mpfr_mul (ab, a, b, rnd);
  mpfr_mul (cd, c, d, rnd);
  inex_ref = mpfr_add (ref, ab, cd, rnd);
  if (! SAME_SIGN (inex_res, inex_ref) ||
      mpfr_nan_p (res) || mpfr_nan_p (ref) ||
      ! mpfr_equal_p (res, ref))
    {
      printf ("mpfr_fmma failed for p=%ld rnd=%s\n",
              (long int) p, mpfr_print_rnd_mode (rnd));
      printf ("a="); mpfr_dump (a);
      printf ("b="); mpfr_dump (b);
      printf ("c="); mpfr_dump (c);
      printf ("d="); mpfr_dump (d);
      printf ("Expected\n  ");
      mpfr_dump (ref);
      printf ("  with inex = %d\n", inex_ref);
      printf ("Got\n  ");
      mpfr_dump (res);
      printf ("  with inex = %d\n", inex_res);
      exit (1);
    }

  /* then check fmms */
  inex_res = mpfr_fmms (res, a, b, c, d, rnd);
  mpfr_mul (ab, a, b, rnd);
  mpfr_mul (cd, c, d, rnd);
  inex_ref = mpfr_sub (ref, ab, cd, rnd);
  if (! SAME_SIGN (inex_res, inex_ref) ||
      mpfr_nan_p (res) || mpfr_nan_p (ref) ||
      ! mpfr_equal_p (res, ref))
    {
      printf ("mpfr_fmms failed for p=%ld rnd=%s\n",
              (long int) p, mpfr_print_rnd_mode (rnd));
      printf ("a="); mpfr_dump (a);
      printf ("b="); mpfr_dump (b);
      printf ("c="); mpfr_dump (c);
      printf ("d="); mpfr_dump (d);
      printf ("Expected\n  ");
      mpfr_dump (ref);
      printf ("  with inex = %d\n", inex_ref);
      printf ("Got\n  ");
      mpfr_dump (res);
      printf ("  with inex = %d\n", inex_res);
      exit (1);
    }

  mpfr_clear (ab);
  mpfr_clear (cd);
  mpfr_clear (res);
  mpfr_clear (ref);
}

static void
random_tests (void)
{
  mpfr_prec_t p;
  int r;
  mpfr_t a, b, c, d;

  for (p = MPFR_PREC_MIN; p <= 4096; p++)
    {
      mpfr_inits2 (p, a, b, c, d, (mpfr_ptr) 0);
      mpfr_urandomb (a, RANDS);
      mpfr_urandomb (b, RANDS);
      mpfr_urandomb (c, RANDS);
      mpfr_urandomb (d, RANDS);
      RND_LOOP_NO_RNDF (r)
        random_test (a, b, c, d, (mpfr_rnd_t) r);
      mpfr_clears (a, b, c, d, (mpfr_ptr) 0);
    }
}

static void
zero_tests (void)
{
  mpfr_exp_t emin, emax;
  mpfr_t a, b, c, d, res;
  int i, r;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();
  set_emin (MPFR_EMIN_MIN);
  set_emax (MPFR_EMAX_MAX);

  mpfr_inits2 (GMP_NUMB_BITS, a, b, c, d, (mpfr_ptr) 0);
  for (i = 0; i <= 4; i++)
    {
      switch (i)
        {
        case 0: case 1:
          mpfr_set_ui (a, i, MPFR_RNDN);
          break;
        case 2:
          mpfr_setmax (a, MPFR_EMAX_MAX);
          break;
        case 3:
          mpfr_setmin (a, MPFR_EMIN_MIN);
          break;
        case 4:
          mpfr_setmin (a, MPFR_EMIN_MIN+1);
          break;
        }
      RND_LOOP (r)
        {
          int j, inex;
          mpfr_flags_t flags;

          mpfr_set (b, a, MPFR_RNDN);
          mpfr_set (c, a, MPFR_RNDN);
          mpfr_neg (d, a, MPFR_RNDN);
          /* We also want to test cases where the precision of the
             result is twice the precision of each input, in case
             add1sp.c and/or sub1sp.c could be involved. */
          for (j = 1; j <= 2; j++)
            {
              mpfr_init2 (res, GMP_NUMB_BITS * j);
              mpfr_clear_flags ();
              inex = mpfr_fmma (res, a, b, c, d, (mpfr_rnd_t) r);
              flags = __gmpfr_flags;
              if (flags != 0 || inex != 0 || ! mpfr_zero_p (res) ||
                  (r == MPFR_RNDD ? MPFR_IS_POS (res) : MPFR_IS_NEG (res)))
                {
                  printf ("Error in zero_tests on i = %d, %s\n",
                          i, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                  printf ("Expected %c0, inex = 0\n",
                          r == MPFR_RNDD ? '-' : '+');
                  printf ("Got      ");
                  if (MPFR_IS_POS (res))
                    printf ("+");
                  mpfr_out_str (stdout, 16, 0, res, MPFR_RNDN);
                  printf (", inex = %d\n", inex);
                  printf ("Expected flags:");
                  flags_out (0);
                  printf ("Got flags:     ");
                  flags_out (flags);
                  exit (1);
                }
              mpfr_clear (res);
            } /* j */
        } /* r */
    } /* i */
  mpfr_clears (a, b, c, d, (mpfr_ptr) 0);

  set_emin (emin);
  set_emax (emax);
}

static void
max_tests (void)
{
  mpfr_exp_t emin, emax;
  mpfr_t x, y1, y2;
  int r;
  int i, inex1, inex2;
  mpfr_flags_t flags1, flags2;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();
  set_emin (MPFR_EMIN_MIN);
  set_emax (MPFR_EMAX_MAX);

  mpfr_init2 (x, GMP_NUMB_BITS);
  mpfr_setmax (x, MPFR_EMAX_MAX);
  flags1 = MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT;
  RND_LOOP (r)
    {
      /* We also want to test cases where the precision of the
         result is twice the precision of each input, in case
         add1sp.c and/or sub1sp.c could be involved. */
      for (i = 1; i <= 2; i++)
        {
          mpfr_inits2 (GMP_NUMB_BITS * i, y1, y2, (mpfr_ptr) 0);
          inex1 = mpfr_mul (y1, x, x, (mpfr_rnd_t) r);
          mpfr_clear_flags ();
          inex2 = mpfr_fmma (y2, x, x, x, x, (mpfr_rnd_t) r);
          flags2 = __gmpfr_flags;
          if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
                 mpfr_equal_p (y1, y2)))
            {
              printf ("Error in max_tests for %s\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r));
              printf ("Expected ");
              mpfr_dump (y1);
              printf ("  with inex = %d, flags =", inex1);
              flags_out (flags1);
              printf ("Got      ");
              mpfr_dump (y2);
              printf ("  with inex = %d, flags =", inex2);
              flags_out (flags2);
              exit (1);
            }
          mpfr_clears (y1, y2, (mpfr_ptr) 0);
        } /* i */
    } /* r */
  mpfr_clear (x);

  set_emin (emin);
  set_emax (emax);
}

/* a^2 - (a+k)(a-k) = k^2 where a^2 overflows but k^2 usually doesn't.
 * a^2 + cd where a^2 overflows and cd doesn't affect the overflow
 * (and cd may even underflow).
 */
static void
overflow_tests (void)
{
  mpfr_exp_t old_emax;
  int i;

  old_emax = mpfr_get_emax ();

  for (i = 0; i < 40; i++)
    {
      mpfr_exp_t emax, exp_a;
      mpfr_t a, k, c, d, z1, z2;
      mpfr_rnd_t rnd;
      mpfr_prec_t prec_a, prec_z;
      int add = i & 1, inex, inex1, inex2;
      mpfr_flags_t flags1, flags2;

      /* In most cases, we do the test with the maximum exponent. */
      emax = i % 8 != 0 ? MPFR_EMAX_MAX : 64 + RAND_BOOL ();
      set_emax (emax);
      exp_a = emax/2 + 32;

      rnd = RND_RAND_NO_RNDF ();
      prec_a = 64 + randlimb () % 100;
      prec_z = MPFR_PREC_MIN + randlimb () % 160;

      mpfr_init2 (a, prec_a);
      mpfr_urandom (a, RANDS, MPFR_RNDU);
      mpfr_set_exp (a, exp_a);

      mpfr_init2 (k, prec_a - 32);
      mpfr_urandom (k, RANDS, MPFR_RNDU);
      mpfr_set_exp (k, exp_a - 32);

      mpfr_init2 (c, prec_a + 1);
      inex = mpfr_add (c, a, k, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);

      mpfr_init2 (d, prec_a);
      inex = mpfr_sub (d, a, k, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      if (add)
        mpfr_neg (d, d, MPFR_RNDN);

      mpfr_init2 (z1, prec_z);
      mpfr_clear_flags ();
      inex1 = mpfr_sqr (z1, k, rnd);
      flags1 = __gmpfr_flags;

      mpfr_init2 (z2, prec_z);
      mpfr_clear_flags ();
      inex2 = add ?
        mpfr_fmma (z2, a, a, c, d, rnd) :
        mpfr_fmms (z2, a, a, c, d, rnd);
      flags2 = __gmpfr_flags;

      if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
             mpfr_equal_p (z1, z2)))
        {
          printf ("Error 1 in overflow_tests for %s\n",
                  mpfr_print_rnd_mode (rnd));
          printf ("Expected ");
          mpfr_dump (z1);
          printf ("  with inex = %d, flags =", inex1);
          flags_out (flags1);
          printf ("Got      ");
          mpfr_dump (z2);
          printf ("  with inex = %d, flags =", inex2);
          flags_out (flags2);
          exit (1);
        }

      /* c and d such that a^2 +/- cd ~= a^2 (overflow) */
      mpfr_urandom (c, RANDS, MPFR_RNDU);
      mpfr_set_exp (c, RAND_BOOL () ? exp_a - 2 : __gmpfr_emin);
      if (RAND_BOOL ())
        mpfr_neg (c, c, MPFR_RNDN);
      mpfr_urandom (d, RANDS, MPFR_RNDU);
      mpfr_set_exp (d, RAND_BOOL () ? exp_a - 2 : __gmpfr_emin);
      if (RAND_BOOL ())
        mpfr_neg (d, d, MPFR_RNDN);

      mpfr_clear_flags ();
      inex1 = mpfr_sqr (z1, a, rnd);
      flags1 = __gmpfr_flags;
      MPFR_ASSERTN (flags1 == (MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT));

      mpfr_clear_flags ();
      inex2 = add ?
        mpfr_fmma (z2, a, a, c, d, rnd) :
        mpfr_fmms (z2, a, a, c, d, rnd);
      flags2 = __gmpfr_flags;

      if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
             mpfr_equal_p (z1, z2)))
        {
          printf ("Error 2 in overflow_tests for %s\n",
                  mpfr_print_rnd_mode (rnd));
          printf ("Expected ");
          mpfr_dump (z1);
          printf ("  with inex = %d, flags =", inex1);
          flags_out (flags1);
          printf ("Got      ");
          mpfr_dump (z2);
          printf ("  with inex = %d, flags =", inex2);
          flags_out (flags2);
          printf ("a="); mpfr_dump (a);
          printf ("c="); mpfr_dump (c);
          printf ("d="); mpfr_dump (d);
          exit (1);
        }

      mpfr_clears (a, k, c, d, z1, z2, (mpfr_ptr) 0);
    }

  set_emax (old_emax);
}

/* (1/2)x + (1/2)x = x tested near underflow */
static void
half_plus_half (void)
{
  mpfr_exp_t emin;
  mpfr_t h, x1, x2, y;
  int neg, r, i, inex;
  mpfr_flags_t flags;

  emin = mpfr_get_emin ();
  set_emin (MPFR_EMIN_MIN);
  mpfr_inits2 (4, h, x1, (mpfr_ptr) 0);
  mpfr_init2 (x2, GMP_NUMB_BITS);
  mpfr_set_ui_2exp (h, 1, -1, MPFR_RNDN);

  for (mpfr_setmin (x1, __gmpfr_emin);
       MPFR_GET_EXP (x1) < __gmpfr_emin + 2;
       mpfr_nextabove (x1))
    {
      inex = mpfr_set (x2, x1, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      for (neg = 0; neg < 2; neg++)
        {
          RND_LOOP (r)
            {
              /* We also want to test cases where the precision of the
                 result is twice the precision of each input, in case
                 add1sp.c and/or sub1sp.c could be involved. */
              for (i = 1; i <= 2; i++)
                {
                  mpfr_init2 (y, GMP_NUMB_BITS * i);
                  mpfr_clear_flags ();
                  inex = mpfr_fmma (y, h, x2, h, x2, (mpfr_rnd_t) r);
                  flags = __gmpfr_flags;
                  if (! (flags == 0 && inex == 0 && mpfr_equal_p (y, x2)))
                    {
                      printf ("Error in half_plus_half for %s\n",
                              mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                      printf ("Expected ");
                      mpfr_dump (x2);
                      printf ("  with inex = 0, flags =");
                      flags_out (0);
                      printf ("Got      ");
                      mpfr_dump (y);
                      printf ("  with inex = %d, flags =", inex);
                      flags_out (flags);
                      exit (1);
                    }
                  mpfr_clear (y);
                }
            }
          mpfr_neg (x2, x2, MPFR_RNDN);
        }
    }

  mpfr_clears (h, x1, x2, (mpfr_ptr) 0);
  set_emin (emin);
}

/* check that result has exponent in [emin, emax]
   (see https://sympa.inria.fr/sympa/arc/mpfr/2017-04/msg00016.html)
   Overflow detection in sub1.c was incorrect (only for UBF cases);
   fixed in r11414. */
static void
bug20170405 (void)
{
  mpfr_t x, y, z;

  mpfr_inits2 (866, x, y, z, (mpfr_ptr) 0);

  mpfr_set_str_binary (x, "-0.10010101110110001111000110010100011001111011110001010100010000111110001010111110100001000000011010001000010000101110000000001100001011000110010000100111001100000101110000000001001101101101010110000110100010010111011001101101010011111000101100000010001100010000011100000000011110100010111011101011000101101011110110001010011001101110011101100001111000011000000011000010101010000101001001010000111101100001000001011110011000110010001100001101101001001010000111100101000010111001001101010011001110110001000010101001100000101010110000100100100010101011111001001100010001010110011000000001011110011000110001000100101000111010010111111110010111001101110101010010101101010100111001011100101101010111010011001000001010011001010001101000111011010010100110011001111111000011101111001010111001001011011011110101101001100011010001010110011100001101100100001001100111010100010100E768635654");
  mpfr_set_str_binary (y, "-0.11010001010111110010110101010011000010010011010011011101100100110000110101100110111010001001110101110000011101100010110100100011001101111010100011111001011100111101110101101001000101011110101101101011010100110010111111010011011100101111110011001001010101011101111100011101100001010010011000110010110101001110010001101111111001100100000101010100110011101101101010011001000110100001001100000010110010101111000110110000111011000110001000100100100101111110001111100101011100100100110111010000010110110001110010001001101000000110111000101000110101111110000110001110100010101111010110001111010111111111010011001001100110011000110010110011000101110001010001101000100010000110011101010010010011110100000111100000101100110001111010000100011111000001101111110100000011011110010100010010011111111000010110000000011010011001100110001110111111010111110000111110010110011001000010E768635576");
  /* since emax = 1073741821, x^2-y^2 should overflow */
  mpfr_fmms (z, x, x, y, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0);

  /* same test when z has a different precision */
  mpfr_set_prec (z, 867);
  mpfr_fmms (z, x, x, y, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (z) && mpfr_sgn (z) > 0);

  mpfr_set_prec (x, 564);
  mpfr_set_prec (y, 564);
  mpfr_set_prec (z, 2256);
  mpfr_set_str_binary (x, "1e-536870913");
  mpfr_set_str_binary (y, "-1e-536870913");
  mpfr_fmma (z, x, y, x, y, MPFR_RNDN);
  /* we should get -0 as result */
  MPFR_ASSERTN(mpfr_zero_p (z) && mpfr_signbit (z));

  mpfr_set_prec (x, 564);
  mpfr_set_prec (y, 564);
  mpfr_set_prec (z, 2256);
  mpfr_set_str_binary (x, "1.10010000111100110011001101111111101000111001011000110100110010000101000100010001000000111100010000101001011011111001111000110101111100101111001100001100011101100100011110000000011000010110111100111000100101010001011111010111011001110010001011101111001011001110110000010000011100010001010001011100100110111110101001001111001011101111110011101110101010110100010010111011111100010101111100011110111001011111101110101101101110100101111010000101011110100000000110111101000001100001000100010110100111010011011010110011100111010000101110010101111001011100110101100001100e-737194993");
  mpfr_set_str_binary (y, "-1.00101000100001001101011110100010110011101010011011010111100110101011111100000100101000111010111101100100110010001110011011100100110110000001011001000111101111101111110101100110111000000011000001101001010100010010001110001000011010000100111001001100101111111100010101110101001101101101111010100011011110001000010000010100011000011000010110101100000111111110111001100100100001101011111011100101110111000100101010110100010011101010110010100110100111000000100111101101101000000011110000100110100100011000010011110010001010000110100011111101101101110001110001101101010e-737194903");
  mpfr_fmma (z, x, y, x, y, MPFR_RNDN);
  /* we should get -0 as result */
  MPFR_ASSERTN(mpfr_zero_p (z) && mpfr_signbit (z));

  mpfr_set_prec (x, 2);
  mpfr_set_prec (y, 2);
  mpfr_set_prec (z, 2);
  /* (a+i*b)*(c+i*d) with:
     a=0.10E1
     b=0.10E-536870912
     c=0.10E-536870912
     d=0.10E1 */
  mpfr_set_str_binary (x, "0.10E1"); /* x = a = d */
  mpfr_set_str_binary (y, "0.10E-536870912"); /* y = b = c */
  /* real part is a*c-b*d = x*y-y*x */
  mpfr_fmms (z, x, y, y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (z) && !mpfr_signbit (z));
  /* imaginary part is a*d+b*c = x*x+y*y */
  mpfr_fmma (z, x, x, y, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (z, 1) == 0);
  mpfr_fmma (z, y, y, x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (z, 1) == 0);

  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

static void
underflow_tests (void)
{
  mpfr_t x, y, z;
  mpfr_prec_t p;
  mpfr_exp_t emin;

  emin = mpfr_get_emin ();
  set_emin (-17);
  for (p = MPFR_PREC_MIN; p <= 1024; p++)
    {
      mpfr_inits2 (p, x, y, (mpfr_ptr) 0);
      mpfr_init2 (z, p + 1);
      mpfr_set_str_binary (x, "1e-10");
      mpfr_set_str_binary (y, "1e-11");
      mpfr_clear_underflow ();
      mpfr_fmms (z, x, x, y, y, MPFR_RNDN);
      /* the exact result is 2^-20-2^-22, and should underflow */
      MPFR_ASSERTN(mpfr_underflow_p ());
      MPFR_ASSERTN(mpfr_zero_p (z));
      MPFR_ASSERTN(mpfr_signbit (z) == 0);
      mpfr_clears (x, y, z, (mpfr_ptr) 0);
    }
  set_emin (emin);
}

static void
bug20180604 (void)
{
  mpfr_t x, y, yneg, z;
  mpfr_exp_t emin;
  int ret;

  emin = mpfr_get_emin ();
  set_emin (-1073741821);
  mpfr_inits2 (564, x, y, yneg, (mpfr_ptr) 0);
  mpfr_init2 (z, 2256);
  mpfr_set_str_binary (x, "1.10010000111100110011001101111111101000111001011000110100110010000101000100010001000000111100010000101001011011111001111000110101111100101111001100001100011101100100011110000000011000010110111100111000100101010001011111010111011001110010001011101111001011001110110000010000011100010001010001011100100110111110101001001111001011101111110011101110101010110100010010111011111100010101111100011110111001011111101110101101101110100101111010000101011110100000000110111101000001100001000100010110100111010011011010110011100111010000101110010101111001011100110101100001100E-737194993");
  mpfr_set_str_binary (y, "-1.00101000100001001101011110100010110011101010011011010111100110101011111100000100101000111010111101100100110010001110011011100100110110000001011001000111101111101111110101100110111000000011000001101001010100010010001110001000011010000100111001001100101111111100010101110101001101101101111010100011011110001000010000010100011000011000010110101100000111111110111001100100100001101011111011100101110111000100101010110100010011101010110010100110100111000000100111101101101000000011110000100110100100011000010011110010001010000110100011111101101101110001110001101101010E-737194903");

  mpfr_clear_underflow ();
  ret = mpfr_fmms (z, x, x, y, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_underflow_p ());
  MPFR_ASSERTN(mpfr_zero_p (z));
  MPFR_ASSERTN(mpfr_signbit (z) == 1);
  MPFR_ASSERTN(ret > 0);

  mpfr_clear_underflow ();
  ret = mpfr_fmms (z, y, y, x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_underflow_p ());
  MPFR_ASSERTN(mpfr_zero_p (z));
  MPFR_ASSERTN(mpfr_signbit (z) == 0);
  MPFR_ASSERTN(ret < 0);

  mpfr_neg (yneg, y, MPFR_RNDN);
  mpfr_clear_underflow ();
  ret = mpfr_fmms (z, x, x, y, yneg, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_underflow_p ());
  MPFR_ASSERTN(mpfr_zero_p (z));
  MPFR_ASSERTN(mpfr_signbit (z) == 0);
  MPFR_ASSERTN(ret < 0);

  mpfr_clear_underflow ();
  ret = mpfr_fmms (z, y, yneg, x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_underflow_p ());
  MPFR_ASSERTN(mpfr_zero_p (z));
  MPFR_ASSERTN(mpfr_signbit (z) == 1);
  MPFR_ASSERTN(ret > 0);

  mpfr_clears (x, y, yneg, z, (mpfr_ptr) 0);
  set_emin (emin);
}

/* this bug was discovered from mpc_mul */
static void
bug20200206 (void)
{
  mpfr_exp_t emin = mpfr_get_emin ();
  mpfr_t xre, xim, yre, yim, zre;

  set_emin (-1073);
  mpfr_inits2 (53, xre, xim, yre, yim, zre, (mpfr_ptr) 0);
  mpfr_set_str (xre, "-0x8.294611b331c8p-904", 16, MPFR_RNDN);
  mpfr_set_str (xim, "-0x1.859278c2992fap-676", 16, MPFR_RNDN);
  mpfr_set_str (yre, "0x9.ac54802a95f8p-820", 16, MPFR_RNDN);
  mpfr_set_str (yim, "0x3.17e59e7612aap-412", 16, MPFR_RNDN);
  mpfr_fmms (zre, xre, yre, xim, yim, MPFR_RNDN);
  if (mpfr_regular_p (zre) && mpfr_get_exp (zre) < -1073)
    {
      printf ("Error, mpfr_fmms returns an out-of-range exponent:\n");
      mpfr_dump (zre);
      exit (1);
    }
  mpfr_clears (xre, xim, yre, yim, zre, (mpfr_ptr) 0);
  set_emin (emin);
}

/* check for integer overflow (see bug fixed in r13798) */
static void
extreme_underflow (void)
{
  mpfr_t x, y, z;
  mpfr_exp_t emin = mpfr_get_emin ();

  set_emin (MPFR_EMIN_MIN);
  mpfr_inits2 (64, x, y, z, (mpfr_ptr) 0);
  mpfr_set_ui_2exp (x, 1, MPFR_EMIN_MIN - 1, MPFR_RNDN);
  mpfr_set (y, x, MPFR_RNDN);
  mpfr_nextabove (x);
  mpfr_clear_flags ();
  mpfr_fmms (z, x, x, y, y, MPFR_RNDN);
  MPFR_ASSERTN (__gmpfr_flags == (MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT));
  MPFR_ASSERTN (MPFR_IS_ZERO (z) && MPFR_IS_POS (z));
  mpfr_clears (x, y, z, (mpfr_ptr) 0);
  set_emin (emin);
}

/* Test double-rounding cases in mpfr_set_1_2, which is called when
   all the precisions are the same. With set.c before r13347, this
   triggers errors for neg=0. */
static void
double_rounding (void)
{
  int p;

  for (p = 4; p < 4 * GMP_NUMB_BITS; p++)
    {
      mpfr_t a, b, c, d, z1, z2;
      int neg;

      mpfr_inits2 (p, a, b, c, d, z1, z2, (mpfr_ptr) 0);
      /* Choose a, b, c, d such that a*b = 2^p and c*d = 1 + epsilon */
      mpfr_set_ui (a, 2, MPFR_RNDN);
      mpfr_set_ui_2exp (b, 1, p - 1, MPFR_RNDN);
      mpfr_set_ui (c, 1, MPFR_RNDN);
      mpfr_nextabove (c);
      /* c = 1 + ulp(1) = 1 + 2 * ulp(1/2) */

      for (neg = 0; neg <= 1; neg++)
        {
          int inex1, inex2;

          /* Set d = 1 - (1 + neg) * ulp(1/2), thus
           * c*d = 1 + (1 - neg) * ulp(1/2) - 2 * (1 + neg) * ulp(1/2)^2,
           * so that a*b + c*d rounds to 2^p + 1 and epsilon has the
           * same sign as (-1)^neg.
           */
          mpfr_set_ui (d, 1, MPFR_RNDN);
          mpfr_nextbelow (d);
          mpfr_set_ui_2exp (z1, 1, p, MPFR_RNDN);
          if (neg)
            {
              mpfr_nextbelow (d);
              inex1 = -1;
            }
          else
            {
              mpfr_nextabove (z1);
              inex1 = 1;
            }

          inex2 = mpfr_fmma (z2, a, b, c, d, MPFR_RNDN);
          if (!(mpfr_equal_p (z1, z2) && SAME_SIGN (inex1, inex2)))
            {
              printf ("Error in double_rounding for precision %d, neg=%d\n",
                      p, neg);
              printf ("Expected ");
              mpfr_dump (z1);
              printf ("with inex = %d\n", inex1);
              printf ("Got      ");
              mpfr_dump (z2);
              printf ("with inex = %d\n", inex2);
              exit (1);
            }
        }

      mpfr_clears (a, b, c, d, z1, z2, (mpfr_ptr) 0);
    }
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  bug20200206 ();
  bug20180604 ();
  underflow_tests ();
  random_tests ();
  zero_tests ();
  max_tests ();
  overflow_tests ();
  half_plus_half ();
  bug20170405 ();
  double_rounding ();
  extreme_underflow ();

  tests_end_mpfr ();
  return 0;
}
