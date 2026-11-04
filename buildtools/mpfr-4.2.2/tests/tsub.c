/* Test file for mpfr_sub.

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

static int
test_sub (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
#ifdef CHECK_EXTERNAL
  int res;
  int ok = rnd_mode == MPFR_RNDN && mpfr_number_p (b) && mpfr_number_p (c);

  if (ok)
    {
      mpfr_print_raw (b);
      printf (" ");
      mpfr_print_raw (c);
    }
  res = mpfr_sub (a, b, c, rnd_mode);
  if (ok)
    {
      printf (" ");
      mpfr_print_raw (a);
      printf ("\n");
    }
  return res;
#else  /* reuse test */
  int inex;

  inex = mpfr_sub (a, b, c, rnd_mode);

  if (a != b && a != c && ! MPFR_IS_NAN (a))
    {
      mpfr_t t;
      int reuse_b, reuse_c, inex_r;

      reuse_b = MPFR_PREC (a) == MPFR_PREC (b);
      reuse_c = MPFR_PREC (a) == MPFR_PREC (c);

      if (reuse_b || reuse_c)
        mpfr_init2 (t, MPFR_PREC (a));

      if (reuse_b)
        {
          mpfr_set (t, b, MPFR_RNDN);
          inex_r = mpfr_sub (t, t, c, rnd_mode);
          if (!(mpfr_equal_p (t, a) && SAME_SIGN (inex_r, inex)))
            {
              printf ("reuse of b error in b - c in %s for\n",
                      mpfr_print_rnd_mode (rnd_mode));
              printf ("b = ");
              mpfr_dump (b);
              printf ("c = ");
              mpfr_dump (c);
              printf ("Expected "); mpfr_dump (a);
              printf ("  with inex = %d\n", inex);
              printf ("Got      "); mpfr_dump (t);
              printf ("  with inex = %d\n", inex_r);
              exit (1);
            }
        }

      if (reuse_c)
        {
          mpfr_set (t, c, MPFR_RNDN);
          inex_r = mpfr_sub (t, b, t, rnd_mode);
          if (!(mpfr_equal_p (t, a) && SAME_SIGN (inex_r, inex)))
            {
              printf ("reuse of c error in b - c in %s for\n",
                      mpfr_print_rnd_mode (rnd_mode));
              printf ("b = ");
              mpfr_dump (b);
              printf ("c = ");
              mpfr_dump (c);
              printf ("Expected "); mpfr_dump (a);
              printf ("  with inex = %d\n", inex);
              printf ("Got      "); mpfr_dump (t);
              printf ("  with inex = %d\n", inex_r);
              exit (1);
            }
        }

      if (reuse_b || reuse_c)
        mpfr_clear (t);
    }

  return inex;
#endif
}

static void
check_diverse (void)
{
  mpfr_t x, y, z;
  int inexact;

  mpfr_init (x);
  mpfr_init (y);
  mpfr_init (z);

  /* check corner case cancel=0, but add_exp=1 */
  mpfr_set_prec (x, 2);
  mpfr_set_prec (y, 4);
  mpfr_set_prec (z, 2);
  mpfr_setmax (y, __gmpfr_emax);
  mpfr_set_str_binary (z, "0.1E-10"); /* tiny */
  test_sub (x, y, z, MPFR_RNDN); /* should round to 2^emax, i.e. overflow */
  if (!mpfr_inf_p (x) || mpfr_sgn (x) < 0)
    {
      printf ("Error in mpfr_sub(a,b,c,RNDN) for b=maxfloat, prec(a)<prec(b), c tiny\n");
      exit (1);
    }

  /* other coverage test */
  mpfr_set_prec (x, 2);
  mpfr_set_prec (y, 2);
  mpfr_set_prec (z, 2);
  mpfr_set_ui (y, 1, MPFR_RNDN);
  mpfr_set_si (z, -2, MPFR_RNDN);
  test_sub (x, y, z, MPFR_RNDD);
  if (mpfr_cmp_ui (x, 3))
    {
      printf ("Error in mpfr_sub(1,-2,RNDD)\n");
      exit (1);
    }

  /* yet another coverage test */
  mpfr_set_prec (x, 2);
  mpfr_set_prec (y, 3);
  mpfr_set_prec (z, 1);
  mpfr_set_ui_2exp (y, 1, mpfr_get_emax (), MPFR_RNDZ);
  /* y = (1 - 2^(-3))*2^emax */
  mpfr_set_ui_2exp (z, 1, mpfr_get_emax () - 4, MPFR_RNDZ);
  /* z = 2^(emax - 4) */
  /* y - z = (1 - 2^(-3) - 2^(-4))*2^emax > (1-2^(-2))*2^emax */
  inexact = mpfr_sub (x, y, z, MPFR_RNDU);
  MPFR_ASSERTN(inexact > 0);
  MPFR_ASSERTN(mpfr_inf_p (x) && mpfr_sgn (x) > 0);

  mpfr_set_prec (x, 288);
  mpfr_set_prec (y, 288);
  mpfr_set_prec (z, 288);
  mpfr_set_str_binary (y, "0.111000110011000001000111101010111011110011101001101111111110000011100101000001001010110010101010011001010100000001110011110001010101101010001011101110100100001011110100110000101101100011010001001011011010101010000010001101001000110010010111111011110001111101001000101101001100101100101000E80");
  mpfr_set_str_binary (z, "0.100001111111101001011010001100110010100111001110000110011101001011010100001000000100111011010110110010000000000010101101011000010000110001110010100001100101011100100100001011000100011110000001010101000100011101001000010111100000111000111011001000100100011000100000010010111000000100100111E-258");
  inexact = test_sub (x, y, z, MPFR_RNDN);
  if (inexact <= 0)
    {
      printf ("Wrong inexact flag for prec=288\n");
      exit (1);
    }

  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 63);
  mpfr_set_prec (z, 63);
  mpfr_set_str_binary (x, "0.101101111011011100100100100111E31");
  mpfr_set_str_binary (y, "0.111110010010100100110101101010001001100101110001000101110111111E-1");
  test_sub (z, x, y, MPFR_RNDN);
  mpfr_set_str_binary (y, "0.1011011110110111001001001001101100000110110101101100101001011E31");
  if (mpfr_cmp (z, y))
    {
      printf ("Error in mpfr_sub (5)\n");
      printf ("expected "); mpfr_dump (y);
      printf ("got      "); mpfr_dump (z);
      exit (1);
    }

  mpfr_set_prec (y, 63);
  mpfr_set_prec (z, 63);
  mpfr_set_str_binary (y, "0.1011011110110111001001001001101100000110110101101100101001011E31");
  mpfr_sub_ui (z, y, 1541116494, MPFR_RNDN);
  mpfr_set_str_binary (y, "-0.11111001001010010011010110101E-1");
  if (mpfr_cmp (z, y))
    {
      printf ("Error in mpfr_sub (7)\n");
      printf ("expected "); mpfr_dump (y);
      printf ("got      "); mpfr_dump (z);
      exit (1);
    }

  mpfr_set_prec (y, 63);
  mpfr_set_prec (z, 63);
  mpfr_set_str_binary (y, "0.1011011110110111001001001001101100000110110101101100101001011E31");
  mpfr_sub_ui (z, y, 1541116494, MPFR_RNDN);
  mpfr_set_str_binary (y, "-0.11111001001010010011010110101E-1");
  if (mpfr_cmp (z, y))
    {
      printf ("Error in mpfr_sub (6)\n");
      printf ("expected "); mpfr_dump (y);
      printf ("got      "); mpfr_dump (z);
      exit (1);
    }

  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 32);
  mpfr_set_str_binary (x, "0.10110111101001110100100101111000E0");
  mpfr_set_str_binary (y, "0.10001100100101000100110111000100E0");
  if ((inexact = test_sub (x, x, y, MPFR_RNDN)))
    {
      printf ("Wrong inexact flag (2): got %d instead of 0\n", inexact);
      exit (1);
    }

  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 32);
  mpfr_set_str_binary (x, "0.11111000110111011000100111011010E0");
  mpfr_set_str_binary (y, "0.10011111101111000100001000000000E-3");
  if ((inexact = test_sub (x, x, y, MPFR_RNDN)))
    {
      printf ("Wrong inexact flag (1): got %d instead of 0\n", inexact);
      exit (1);
    }

  mpfr_set_prec (x, 33);
  mpfr_set_prec (y, 33);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_set_str_binary (y, "-0.1E-32");
  mpfr_add (x, x, y, MPFR_RNDN);
  mpfr_set_str_binary (y, "0.111111111111111111111111111111111E0");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_sub (1 - 1E-33) with prec=33\n");
      printf ("Expected "); mpfr_dump (y);
      printf ("got      "); mpfr_dump (x);
      exit (1);
    }

  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 33);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_set_str_binary (y, "-0.1E-32");
  mpfr_add (x, x, y, MPFR_RNDN);
  if (mpfr_cmp_ui (x, 1))
    {
      printf ("Error in mpfr_sub (1 - 1E-33) with prec=32\n");
      printf ("Expected 1.0, got "); mpfr_dump (x);
      exit (1);
    }

  mpfr_set_prec (x, 65);
  mpfr_set_prec (y, 65);
  mpfr_set_prec (z, 64);
  mpfr_set_str_binary (x, "1.1110111011110001110111011111111111101000011001011100101100101101");
  mpfr_set_str_binary (y, "0.1110111011110001110111011111111111101000011001011100101100101100");
  test_sub (z, x, y, MPFR_RNDZ);
  if (mpfr_cmp_ui (z, 1))
    {
      printf ("Error in mpfr_sub (1)\n");
      exit (1);
    }
  test_sub (z, x, y, MPFR_RNDU);
  mpfr_set_str_binary (x, "1.000000000000000000000000000000000000000000000000000000000000001");
  if (mpfr_cmp (z, x))
    {
      printf ("Error in mpfr_sub (2)\n");
      printf ("Expected "); mpfr_dump (x);
      printf ("Got      "); mpfr_dump (z);
      exit (1);
    }
  mpfr_set_str_binary (x, "1.1110111011110001110111011111111111101000011001011100101100101101");
  test_sub (z, x, y, MPFR_RNDN);
  if (mpfr_cmp_ui (z, 1))
    {
      printf ("Error in mpfr_sub (3)\n");
      exit (1);
    }
  inexact = test_sub (z, x, y, MPFR_RNDA);
  mpfr_set_str_binary (x, "1.000000000000000000000000000000000000000000000000000000000000001");
  if (mpfr_cmp (z, x) || inexact <= 0)
    {
      printf ("Error in mpfr_sub (4)\n");
      exit (1);
    }
  mpfr_set_prec (x, 66);
  mpfr_set_str_binary (x, "1.11101110111100011101110111111111111010000110010111001011001010111");
  test_sub (z, x, y, MPFR_RNDN);
  if (mpfr_cmp_ui (z, 1))
    {
      printf ("Error in mpfr_sub (5)\n");
      exit (1);
    }

  /* check in-place operations */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  test_sub (x, x, x, MPFR_RNDN);
  if (mpfr_cmp_ui(x, 0))
    {
      printf ("Error for mpfr_sub (x, x, x, MPFR_RNDN) with x=1.0\n");
      exit (1);
    }

  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);
  mpfr_set_prec (z, 53);
  mpfr_set_str1 (x, "1.229318102e+09");
  mpfr_set_str1 (y, "2.32221184180698677665e+05");
  test_sub (z, x, y, MPFR_RNDN);
  if (mpfr_cmp_str1 (z, "1229085880.815819263458251953125"))
    {
      printf ("Error in mpfr_sub (1.22e9 - 2.32e5)\n");
      printf ("expected 1229085880.815819263458251953125, got ");
      mpfr_out_str(stdout, 10, 0, z, MPFR_RNDN);
      putchar('\n');
      exit (1);
    }

  mpfr_set_prec (x, 112);
  mpfr_set_prec (y, 98);
  mpfr_set_prec (z, 54);
  mpfr_set_str_binary (x, "0.11111100100000000011000011100000101101010001000111E-401");
  mpfr_set_str_binary (y, "0.10110000100100000101101100011111111011101000111000101E-464");
  test_sub (z, x, y, MPFR_RNDN);
  if (mpfr_cmp (z, x)) {
    printf ("mpfr_sub(z, x, y) failed for prec(x)=112, prec(y)=98\n");
    printf ("expected "); mpfr_dump (x);
    printf ("got      "); mpfr_dump (z);
    exit (1);
  }

  mpfr_set_prec (x, 33);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_div_2ui (x, x, 32, MPFR_RNDN);
  mpfr_sub_ui (x, x, 1, MPFR_RNDN);

  mpfr_set_prec (x, 5);
  mpfr_set_prec (y, 5);
  mpfr_set_str_binary (x, "1e-12");
  mpfr_set_ui (y, 1, MPFR_RNDN);
  test_sub (x, y, x, MPFR_RNDD);
  mpfr_set_str_binary (y, "0.11111");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_sub (x, y, x, MPFR_RNDD) for x=2^(-12), y=1\n");
      exit (1);
    }

  mpfr_set_prec (x, 24);
  mpfr_set_prec (y, 24);
  mpfr_set_str_binary (x, "-0.100010000000000000000000E19");
  mpfr_set_str_binary (y, "0.100000000000000000000100E15");
  mpfr_add (x, x, y, MPFR_RNDD);
  mpfr_set_str_binary (y, "-0.1E19");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_add (2)\n");
      exit (1);
    }

  mpfr_set_prec (x, 2);
  mpfr_set_prec (y, 10);
  mpfr_set_prec (z, 10);
  mpfr_set_ui (y, 0, MPFR_RNDN);
  mpfr_set_str_binary (z, "0.10001");
  if (test_sub (x, y, z, MPFR_RNDN) <= 0)
    {
      printf ("Wrong inexact flag in x=mpfr_sub(0,z) for prec(z)>prec(x)\n");
      exit (1);
    }
  if (test_sub (x, z, y, MPFR_RNDN) >= 0)
    {
      printf ("Wrong inexact flag in x=mpfr_sub(z,0) for prec(z)>prec(x)\n");
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

static void
bug_ddefour(void)
{
    mpfr_t ex, ex1, ex2, ex3, tot, tot1;

    mpfr_init2(ex, 53);
    mpfr_init2(ex1, 53);
    mpfr_init2(ex2, 53);
    mpfr_init2(ex3, 53);
    mpfr_init2(tot, 150);
    mpfr_init2(tot1, 150);

    mpfr_set_ui( ex, 1, MPFR_RNDN);
    mpfr_mul_2ui( ex, ex, 906, MPFR_RNDN);
    mpfr_log( tot, ex, MPFR_RNDN);
    mpfr_set( ex1, tot, MPFR_RNDN); /* ex1 = high(tot) */
    test_sub( ex2, tot, ex1, MPFR_RNDN); /* ex2 = high(tot - ex1) */
    test_sub( tot1, tot, ex1, MPFR_RNDN); /* tot1 = tot - ex1 */
    mpfr_set( ex3, tot1, MPFR_RNDN); /* ex3 = high(tot - ex1) */

    if (mpfr_cmp(ex2, ex3))
      {
        printf ("Error in ddefour test.\n");
        printf ("ex2="); mpfr_dump (ex2);
        printf ("ex3="); mpfr_dump (ex3);
        exit (1);
      }

    mpfr_clear (ex);
    mpfr_clear (ex1);
    mpfr_clear (ex2);
    mpfr_clear (ex3);
    mpfr_clear (tot);
    mpfr_clear (tot1);
}

/* if u = o(x-y), v = o(u-x), w = o(v+y), then x-y = u-w */
static void
check_two_sum (mpfr_prec_t p)
{
  mpfr_t x, y, u, v, w;
  mpfr_rnd_t rnd;
  int inexact;

  mpfr_init2 (x, p);
  mpfr_init2 (y, p);
  mpfr_init2 (u, p);
  mpfr_init2 (v, p);
  mpfr_init2 (w, p);
  mpfr_urandomb (x, RANDS);
  mpfr_urandomb (y, RANDS);
  if (mpfr_cmpabs (x, y) < 0)
    mpfr_swap (x, y);
  rnd = MPFR_RNDN;
  inexact = test_sub (u, x, y, rnd);
  test_sub (v, u, x, rnd);
  mpfr_add (w, v, y, rnd);
  /* as u = (x-y) - w, we should have inexact and w of opposite signs */
  if (((inexact == 0) && mpfr_cmp_ui (w, 0)) ||
      ((inexact > 0) && (mpfr_cmp_ui (w, 0) <= 0)) ||
      ((inexact < 0) && (mpfr_cmp_ui (w, 0) >= 0)))
    {
      printf ("Wrong inexact flag for prec=%u, rnd=%s\n", (unsigned)p,
               mpfr_print_rnd_mode (rnd));
      printf ("x="); mpfr_dump (x);
      printf ("y="); mpfr_dump (y);
      printf ("u="); mpfr_dump (u);
      printf ("v="); mpfr_dump (v);
      printf ("w="); mpfr_dump (w);
      printf ("inexact = %d\n", inexact);
      exit (1);
    }
  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (u);
  mpfr_clear (v);
  mpfr_clear (w);
}

#define MAX_PREC 200

static void
check_inexact (void)
{
  mpfr_t x, y, z, u;
  mpfr_prec_t px, py, pu, pz;
  int inexact, cmp;
  mpfr_rnd_t rnd;

  mpfr_init (x);
  mpfr_init (y);
  mpfr_init (z);
  mpfr_init (u);

  mpfr_set_prec (x, 2);
  mpfr_set_ui (x, 6, MPFR_RNDN);
  mpfr_div_2ui (x, x, 4, MPFR_RNDN); /* x = 6/16 */
  mpfr_set_prec (y, 2);
  mpfr_set_si (y, -1, MPFR_RNDN);
  mpfr_div_2ui (y, y, 4, MPFR_RNDN); /* y = -1/16 */
  inexact = test_sub (y, y, x, MPFR_RNDN); /* y = round(-7/16) = -1/2 */
  if (inexact >= 0)
    {
      printf ("Error: wrong inexact flag for -1/16 - (6/16)\n");
      exit (1);
    }

  for (px=2; px<MAX_PREC; px++)
    {
      mpfr_set_prec (x, px);
      do
        {
          mpfr_urandomb (x, RANDS);
        }
      while (mpfr_cmp_ui (x, 0) == 0);
      for (pu=2; pu<MAX_PREC; pu++)
        {
          mpfr_set_prec (u, pu);
          do
            {
              mpfr_urandomb (u, RANDS);
            }
          while (mpfr_cmp_ui (u, 0) == 0);
          {
              py = 2 + (randlimb () % (MAX_PREC - 2));
              mpfr_set_prec (y, py);
              /* warning: MPFR_EXP is undefined for 0 */
              pz =  (mpfr_cmpabs (x, u) >= 0) ? MPFR_EXP(x) - MPFR_EXP(u)
                : MPFR_EXP(u) - MPFR_EXP(x);
              pz = pz + MAX(MPFR_PREC(x), MPFR_PREC(u));
              mpfr_set_prec (z, pz);
              rnd = RND_RAND_NO_RNDF ();
              if (test_sub (z, x, u, rnd))
                {
                  printf ("z <- x - u should be exact\n");
                  exit (1);
                }
                {
                  rnd = RND_RAND_NO_RNDF ();
                  inexact = test_sub (y, x, u, rnd);
                  cmp = mpfr_cmp (y, z);
                  if (((inexact == 0) && (cmp != 0)) ||
                      ((inexact > 0) && (cmp <= 0)) ||
                      ((inexact < 0) && (cmp >= 0)))
                    {
                      printf ("Wrong inexact flag for rnd=%s\n",
                              mpfr_print_rnd_mode(rnd));
                      printf ("expected %d, got %d\n", cmp, inexact);
                      printf ("x="); mpfr_dump (x);
                      printf ("u="); mpfr_dump (u);
                      printf ("y=  "); mpfr_dump (y);
                      printf ("x-u="); mpfr_dump (z);
                      exit (1);
                    }
                }
            }
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (u);
}

/* Bug found by Jakub Jelinek
 * https://bugzilla.redhat.com/show_bug.cgi?id=643657
 * https://gforge.inria.fr/tracker/index.php?func=detail&aid=11301
 * The consequence can be either an assertion failure (i = 2 in the
 * testcase below, in debug mode) or an incorrectly rounded value.
 */
static void
bug20101017 (void)
{
  mpfr_t a, b, c;
  int inex;
  int i;

  mpfr_init2 (a, GMP_NUMB_BITS * 2);
  mpfr_init2 (b, GMP_NUMB_BITS);
  mpfr_init2 (c, GMP_NUMB_BITS);

  /* a = 2^(2N) + k.2^(2N-1) + 2^N and b = 1
     with N = GMP_NUMB_BITS and k = 0 or 1.
     c = a - b should round to the same value as a. */

  for (i = 2; i <= 3; i++)
    {
      mpfr_set_ui_2exp (a, i, GMP_NUMB_BITS - 1, MPFR_RNDN);
      mpfr_add_ui (a, a, 1, MPFR_RNDN);
      mpfr_mul_2ui (a, a, GMP_NUMB_BITS, MPFR_RNDN);
      mpfr_set_ui (b, 1, MPFR_RNDN);
      inex = mpfr_sub (c, a, b, MPFR_RNDN);
      mpfr_set (b, a, MPFR_RNDN);
      if (! mpfr_equal_p (c, b))
        {
          printf ("Error in bug20101017 for i = %d.\n", i);
          printf ("Expected ");
          mpfr_out_str (stdout, 16, 0, b, MPFR_RNDN);
          putchar ('\n');
          printf ("Got      ");
          mpfr_out_str (stdout, 16, 0, c, MPFR_RNDN);
          putchar ('\n');
          exit (1);
        }
      if (inex >= 0)
        {
          printf ("Error in bug20101017 for i = %d: bad inex value.\n", i);
          printf ("Expected negative, got %d.\n", inex);
          exit (1);
        }
    }

  mpfr_set_prec (a, 64);
  mpfr_set_prec (b, 129);
  mpfr_set_prec (c, 2);
  mpfr_set_str_binary (b, "0.100000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000001E65");
  mpfr_set_str_binary (c, "0.10E1");
  inex = mpfr_sub (a, b, c, MPFR_RNDN);
  if (mpfr_cmp_ui_2exp (a, 1, 64) != 0 || inex >= 0)
    {
      printf ("Error in mpfr_sub for b-c for b=2^64+1+2^(-64), c=1\n");
      printf ("Expected result 2^64 with inex < 0\n");
      printf ("Got "); mpfr_dump (a);
      printf ("with inex=%d\n", inex);
      exit (1);
    }

  mpfr_clears (a, b, c, (mpfr_ptr) 0);
}

/* hard test of rounding */
static void
check_rounding (void)
{
  mpfr_t a, b, c, res;
  mpfr_prec_t p;
  long k, l;
  int i;

#define MAXKL (2 * GMP_NUMB_BITS)
  for (p = MPFR_PREC_MIN; p <= GMP_NUMB_BITS; p++)
    {
      mpfr_init2 (a, p);
      mpfr_init2 (res, p);
      mpfr_init2 (b, p + 1 + MAXKL);
      mpfr_init2 (c, MPFR_PREC_MIN);

      /* b = 2^p + 1 + 2^(-k), c = 2^(-l) */
      for (k = 0; k <= MAXKL; k++)
        for (l = 0; l <= MAXKL; l++)
          {
            mpfr_set_ui_2exp (b, 1, p, MPFR_RNDN);
            mpfr_add_ui (b, b, 1, MPFR_RNDN);
            mpfr_mul_2ui (b, b, k, MPFR_RNDN);
            mpfr_add_ui (b, b, 1, MPFR_RNDN);
            mpfr_div_2ui (b, b, k, MPFR_RNDN);
            mpfr_set_ui_2exp (c, 1, -l, MPFR_RNDN);
            i = mpfr_sub (a, b, c, MPFR_RNDN);
            /* b - c = 2^p + 1 + 2^(-k) - 2^(-l), should be rounded to
               2^p for l <= k, and 2^p+2 for l < k, except when p=1 and
               k=l, in which case b - c = 3, and the round-away rule implies
               a = 4 = 2^p+2 = 2^(p+1) */
            if (l < k || (l == k && p > 1))
              {
                if (mpfr_cmp_ui_2exp (a, 1, p) != 0)
                  {
                    printf ("Wrong result in check_rounding\n");
                    printf ("p=%lu k=%ld l=%ld\n", (unsigned long) p, k, l);
                    printf ("b="); mpfr_dump (b);
                    printf ("c="); mpfr_dump (c);
                    printf ("Expected 2^%lu\n", (unsigned long) p);
                    printf ("Got      "); mpfr_dump (a);
                    exit (1);
                  }
                if (i >= 0)
                  {
                    printf ("Wrong ternary value in check_rounding\n");
                    printf ("p=%lu k=%ld l=%ld\n", (unsigned long) p, k, l);
                    printf ("b="); mpfr_dump (b);
                    printf ("c="); mpfr_dump (c);
                    printf ("a="); mpfr_dump (a);
                    printf ("Expected < 0, got %d\n", i);
                    exit (1);
                  }
              }
            else /* l < k  or (l = k and p = 1) */
              {
                mpfr_set_ui_2exp (res, 1, p, MPFR_RNDN);
                mpfr_add_ui (res, res, 2, MPFR_RNDN);
                if (mpfr_cmp (a, res) != 0)
                  {
                    printf ("Wrong result in check_rounding\n");
                    printf ("b="); mpfr_dump (b);
                    printf ("c="); mpfr_dump (c);
                    printf ("Expected "); mpfr_dump (res);
                    printf ("Got      "); mpfr_dump (a);
                    exit (1);
                  }
                if (i <= 0)
                  {
                    printf ("Wrong ternary value in check_rounding\n");
                    printf ("b="); mpfr_dump (b);
                    printf ("c="); mpfr_dump (c);
                    printf ("Expected > 0, got %d\n", i);
                    exit (1);
                  }
              }
          }

      mpfr_clear (a);
      mpfr_clear (res);
      mpfr_clear (b);
      mpfr_clear (c);
    }
}

/* Check a = b - c, where the significand of b has all 1's, c is small
   compared to b, and PREC(a) = PREC(b) - 1. Thus b is a midpoint for
   the precision of the result a. The test is done with the extended
   exponent range and with some reduced exponent range. Two choices
   are made for the exponent of b: the maximum exponent - 1 (similar
   to some normal case) and the maximum exponent (overflow case or
   near overflow case, depending on the rounding mode).
   This test is useful to trigger a bug in r10382: Since c is small,
   the computation in sub1.c was done by first rounding b in the
   precision of a, then correcting the result if b was a breakpoint
   for this precision (exactly representable number for the directed
   rounding modes, or midpoint for the round-to-nearest mode). The
   problem was that for a midpoint in the round-to-nearest mode, the
   rounding of b gave a spurious overflow; not only the overflow flag
   was incorrect, but the result could not be corrected, since due to
   this overflow, the "even rounding" information was lost.
   In the case of reduced exponent range, an additional test is done
   for consistency checks: the subtraction is done in the extended
   exponent range (no overflow), then the result is converted to the
   initial exponent range with mpfr_check_range. */
static void
check_max_almosteven (void)
{
  mpfr_exp_t old_emin, old_emax;
  mpfr_exp_t emin[2] = { MPFR_EMIN_MIN, -1000 };
  mpfr_exp_t emax[2] = { MPFR_EMAX_MAX, 1000 };
  int i;

  old_emin = mpfr_get_emin ();
  old_emax = mpfr_get_emax ();

  for (i = 0; i < 2; i++)
    {
      mpfr_t a1, a2, b, c;
      mpfr_prec_t p;
      int neg, j, rnd;

      set_emin (emin[i]);
      set_emax (emax[i]);

      p = MPFR_PREC_MIN + randlimb () % 70;
      mpfr_init2 (a1, p);
      mpfr_init2 (a2, p);
      mpfr_init2 (b, p+1);
      mpfr_init2 (c, MPFR_PREC_MIN);

      mpfr_setmax (b, 0);
      mpfr_set_ui (c, 1, MPFR_RNDN);

      for (neg = 0; neg < 2; neg++)
        {
          for (j = 1; j >= 0; j--)
            {
              mpfr_set_exp (b, __gmpfr_emax - j);
              RND_LOOP_NO_RNDF (rnd)
                {
                  mpfr_flags_t flags1, flags2;
                  int inex1, inex2;

                  /* Expected result. */
                  flags1 = MPFR_FLAGS_INEXACT;
                  if (rnd == MPFR_RNDN || MPFR_IS_LIKE_RNDZ (rnd, neg))
                    {
                      inex1 = neg ? 1 : -1;
                      mpfr_setmax (a1, __gmpfr_emax - j);
                    }
                  else
                    {
                      inex1 = neg ? -1 : 1;
                      if (j == 0)
                        {
                          flags1 |= MPFR_FLAGS_OVERFLOW;
                          mpfr_set_inf (a1, 1);
                        }
                      else
                        {
                          mpfr_setmin (a1, __gmpfr_emax);
                        }
                    }
                  MPFR_SET_SIGN (a1, neg ? -1 : 1);

                  /* Computed result. */
                  mpfr_clear_flags ();
                  inex2 = mpfr_sub (a2, b, c, (mpfr_rnd_t) rnd);
                  flags2 = __gmpfr_flags;

                  if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
                         mpfr_equal_p (a1, a2)))
                    {
                      printf ("Error 1 in check_max_almosteven for %s,"
                              " i = %d, j = %d, neg = %d\n",
                              mpfr_print_rnd_mode ((mpfr_rnd_t) rnd),
                              i, j, neg);
                      printf ("     b = ");
                      mpfr_dump (b);
                      printf ("Expected ");
                      mpfr_dump (a1);
                      printf ("  with inex = %d, flags =", inex1);
                      flags_out (flags1);
                      printf ("Got      ");
                      mpfr_dump (a2);
                      printf ("  with inex = %d, flags =", inex2);
                      flags_out (flags2);
                      exit (1);
                    }

                  if (i == 0)
                    break;

                  /* Additional test for the reduced exponent range. */
                  mpfr_clear_flags ();
                  set_emin (MPFR_EMIN_MIN);
                  set_emax (MPFR_EMAX_MAX);
                  inex2 = mpfr_sub (a2, b, c, (mpfr_rnd_t) rnd);
                  set_emin (emin[i]);
                  set_emax (emax[i]);
                  inex2 = mpfr_check_range (a2, inex2, (mpfr_rnd_t) rnd);
                  flags2 = __gmpfr_flags;

                  if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
                         mpfr_equal_p (a1, a2)))
                    {
                      printf ("Error 2 in check_max_almosteven for %s,"
                              " i = %d, j = %d, neg = %d\n",
                              mpfr_print_rnd_mode ((mpfr_rnd_t) rnd),
                              i, j, neg);
                      printf ("     b = ");
                      mpfr_dump (b);
                      printf ("Expected ");
                      mpfr_dump (a1);
                      printf ("  with inex = %d, flags =", inex1);
                      flags_out (flags1);
                      printf ("Got      ");
                      mpfr_dump (a2);
                      printf ("  with inex = %d, flags =", inex2);
                      flags_out (flags2);
                      exit (1);
                    }
                }
            }  /* j */

          mpfr_neg (b, b, MPFR_RNDN);
          mpfr_neg (c, c, MPFR_RNDN);
        }  /* neg */

      mpfr_clears (a1, a2, b, c, (mpfr_ptr) 0);
    }  /* i */

  set_emin (old_emin);
  set_emax (old_emax);
}

static void
test_rndf (void)
{
  mpfr_t a, b, c, d;

  mpfr_init2 (a, 7);
  mpfr_init2 (b, 7);
  mpfr_init2 (c, 7);
  mpfr_init2 (d, 7);
  mpfr_set_str_binary (b, "-1.000000e-7");
  mpfr_set_str_binary (c, "-1.000000");
  mpfr_sub (a, b, c, MPFR_RNDF);
  MPFR_ASSERTN(MPFR_IS_NORMALIZED(a));
  mpfr_sub (d, b, c, MPFR_RNDD);
  if (!mpfr_equal_p (a, d))
    {
      mpfr_sub (d, b, c, MPFR_RNDU);
      if (!mpfr_equal_p (a, d))
        {
          printf ("Error: mpfr_sub(a,b,c,RNDF) does not match RNDD/RNDU\n");
          printf ("b="); mpfr_dump (b);
          printf ("c="); mpfr_dump (c);
          printf ("a="); mpfr_dump (a);
          exit (1);
        }
    }
  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
  mpfr_clear (d);
}

static void
testall_rndf (mpfr_prec_t pmax)
{
  mpfr_t a, b, c, d;
  mpfr_prec_t pa, pb, pc;
  mpfr_exp_t eb;

  for (pa = MPFR_PREC_MIN; pa <= pmax; pa++)
    {
      mpfr_init2 (a, pa);
      mpfr_init2 (d, pa);
      for (pb = MPFR_PREC_MIN; pb <= pmax; pb++)
        {
          mpfr_init2 (b, pb);
          for (eb = 0; eb <= pmax + 3; eb ++)
            {
              mpfr_set_ui_2exp (b, 1, eb, MPFR_RNDN);
              while (mpfr_cmp_ui_2exp (b, 1, eb + 1) < 0)
                {
                  for (pc = MPFR_PREC_MIN; pc <= pmax; pc++)
                    {
                      mpfr_init2 (c, pc);
                      mpfr_set_ui (c, 1, MPFR_RNDN);
                      while (mpfr_cmp_ui (c, 2) < 0)
                        {
                          mpfr_sub (a, b, c, MPFR_RNDF);
                          mpfr_sub (d, b, c, MPFR_RNDD);
                          if (!mpfr_equal_p (a, d))
                            {
                              mpfr_sub (d, b, c, MPFR_RNDU);
                              if (!mpfr_equal_p (a, d))
                                {
                                  printf ("Error: mpfr_sub(a,b,c,RNDF) does not "
                                          "match RNDD/RNDU\n");
                                  printf ("b="); mpfr_dump (b);
                                  printf ("c="); mpfr_dump (c);
                                  printf ("a="); mpfr_dump (a);
                                  exit (1);
                                }
                            }
                          mpfr_nextabove (c);
                        }
                      mpfr_clear (c);
                    }
                  mpfr_nextabove (b);
                }
            }
          mpfr_clear (b);
        }
      mpfr_clear (a);
      mpfr_clear (d);
    }
}

static void
test_rndf_exact (mp_size_t pmax)
{
  mpfr_t a, b, c, d;
  mpfr_prec_t pa, pb, pc;
  mpfr_exp_t eb;

  for (pa = MPFR_PREC_MIN; pa <= pmax; pa++)
    {
      /* only check pa mod GMP_NUMB_BITS = -2, -1, 0, 1, 2 */
      if ((pa + 2) % GMP_NUMB_BITS > 4)
        continue;
      mpfr_init2 (a, pa);
      mpfr_init2 (d, pa);
      for (pb = MPFR_PREC_MIN; pb <= pmax; pb++)
        {
          if ((pb + 2) % GMP_NUMB_BITS > 4)
            continue;
          mpfr_init2 (b, pb);
          for (eb = 0; eb <= pmax + 3; eb ++)
            {
              mpfr_urandomb (b, RANDS);
              mpfr_mul_2ui (b, b, eb, MPFR_RNDN);
              for (pc = MPFR_PREC_MIN; pc <= pmax; pc++)
                {
                  if ((pc + 2) % GMP_NUMB_BITS > 4)
                    continue;
                  mpfr_init2 (c, pc);
                  mpfr_urandomb (c, RANDS);
                  mpfr_sub (a, b, c, MPFR_RNDF);
                  mpfr_sub (d, b, c, MPFR_RNDD);
                  if (!mpfr_equal_p (a, d))
                    {
                      mpfr_sub (d, b, c, MPFR_RNDU);
                      if (!mpfr_equal_p (a, d))
                        {
                          printf ("Error: mpfr_sub(a,b,c,RNDF) does not "
                                  "match RNDD/RNDU\n");
                          printf ("b="); mpfr_dump (b);
                          printf ("c="); mpfr_dump (c);
                          printf ("a="); mpfr_dump (a);
                          exit (1);
                        }
                    }

                  /* now make the low bits from c match those from b */
                  mpfr_add (c, b, d, MPFR_RNDN);
                  mpfr_sub (a, b, c, MPFR_RNDF);
                  mpfr_sub (d, b, c, MPFR_RNDD);
                  if (!mpfr_equal_p (a, d))
                    {
                      mpfr_sub (d, b, c, MPFR_RNDU);
                      if (!mpfr_equal_p (a, d))
                        {
                          printf ("Error: mpfr_sub(a,b,c,RNDF) does not "
                                  "match RNDD/RNDU\n");
                          printf ("b="); mpfr_dump (b);
                          printf ("c="); mpfr_dump (c);
                          printf ("a="); mpfr_dump (a);
                          exit (1);
                        }
                    }

                  mpfr_clear (c);
                }
            }
          mpfr_clear (b);
        }
      mpfr_clear (a);
      mpfr_clear (d);
    }
}

/* Bug in the case 2 <= d < p in generic code mpfr_sub1sp() introduced
 * in r12242. Before this change, the special case that is failing was
 * handled by the MPFR_UNLIKELY(ap[n-1] == MPFR_LIMB_HIGHBIT) in the
 * "truncate:" code.
 */
static void
bug20180215 (void)
{
  mpfr_t x, y, z1, z2;
  mpfr_rnd_t r[] = { MPFR_RNDN, MPFR_RNDU, MPFR_RNDA };
  int i, p;

  for (p = 3; p <= 3 + 4 * GMP_NUMB_BITS; p++)
    {
      mpfr_inits2 (p, x, y, z1, z2, (mpfr_ptr) 0);
      mpfr_set_ui_2exp (x, 1, p - 1, MPFR_RNDN);
      mpfr_nextabove (x);
      mpfr_set_ui_2exp (y, 3, -1, MPFR_RNDN);
      mpfr_set (z1, x, MPFR_RNDN);
      mpfr_nextbelow (z1);
      mpfr_nextbelow (z1);
      for (i = 0; i < numberof (r); i++)
        {
          test_sub (z2, x, y, r[i]);
          if (! mpfr_equal_p (z1, z2))
            {
              printf ("Error in bug20180215 in precision %d, %s\n",
                      p, mpfr_print_rnd_mode (r[i]));
              printf ("expected "); mpfr_dump (z1);
              printf ("got      "); mpfr_dump (z2);
              exit (1);
            }
        }
      mpfr_clears (x, y, z1, z2, (mpfr_ptr) 0);
    }
}

static void
bug20180216 (void)
{
  mpfr_t x, y, z1, z2;
  int r, p, d, inex;

  for (p = 3; p <= 3 + 4 * GMP_NUMB_BITS; p++)
    {
      mpfr_inits2 (p, x, y, z1, z2, (mpfr_ptr) 0);
      for (d = 1; d <= p-2; d++)
        {
          inex = mpfr_set_ui_2exp (z1, 1, d, MPFR_RNDN);  /* z1 = 2^d */
          MPFR_ASSERTN (inex == 0);
          inex = mpfr_add_ui (x, z1, 1, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
          mpfr_nextabove (x);  /* x = 2^d + 1 + epsilon */
          inex = mpfr_sub (y, x, z1, MPFR_RNDN);  /* y = 1 + epsilon */
          MPFR_ASSERTN (inex == 0);
          inex = mpfr_add (z2, y, z1, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
          MPFR_ASSERTN (mpfr_equal_p (z2, x));  /* consistency check */
          RND_LOOP (r)
            {
              inex = test_sub (z2, x, y, (mpfr_rnd_t) r);
              if (! mpfr_equal_p (z1, z2) || inex != 0)
                {
                  printf ("Error in bug20180216 with p=%d, d=%d, %s\n",
                          p, d, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                  printf ("expected "); mpfr_dump (z1);
                  printf ("  with inex = 0\n");
                  printf ("got      "); mpfr_dump (z2);
                  printf ("  with inex = %d\n", inex);
                  exit (1);
                }
            }
        }
      mpfr_clears (x, y, z1, z2, (mpfr_ptr) 0);
    }
}

/* Fails with r12281: "reuse of c error in b - c in MPFR_RNDN".
 *
 * If the fix in r10697 (2016-07-29) is reverted, this test also fails
 * (there were no non-regression tests for this bug until this one);
 * note that if --enable-assert=full is used, the error message is:
 * "sub1 & sub1sp return different values for MPFR_RNDN".
 */
static void
bug20180217 (void)
{
  mpfr_t x, y, z1, z2;
  int r, p, d, i, inex1, inex2;

  for (p = 3; p <= 3 + 4 * GMP_NUMB_BITS; p++)
    {
      mpfr_inits2 (p, x, y, z1, z2, (mpfr_ptr) 0);
      for (d = p; d <= p+4; d++)
        {
          mpfr_set_ui (x, 1, MPFR_RNDN);
          mpfr_set_ui_2exp (y, 1, -d, MPFR_RNDN);
          for (i = 0; i < 3; i++)
            {
              RND_LOOP_NO_RNDF (r)
                {
                  mpfr_set (z1, x, MPFR_RNDN);
                  if (d == p)
                    {
                      mpfr_nextbelow (z1);
                      if (i == 0)
                        inex1 = 0;
                      else if (r == MPFR_RNDD || r == MPFR_RNDZ ||
                               (r == MPFR_RNDN && i > 1))
                        {
                          mpfr_nextbelow (z1);
                          inex1 = -1;
                        }
                      else
                        inex1 = 1;
                    }
                  else if (r == MPFR_RNDD || r == MPFR_RNDZ ||
                           (r == MPFR_RNDN && d == p+1 && i > 0))
                    {
                      mpfr_nextbelow (z1);
                      inex1 = -1;
                    }
                  else
                    inex1 = 1;
                  inex2 = test_sub (z2, x, y, (mpfr_rnd_t) r);
                  if (!(mpfr_equal_p (z1, z2) && SAME_SIGN (inex1, inex2)))
                    {
                      printf ("Error in bug20180217 with "
                              "p=%d, d=%d, i=%d, %s\n", p, d, i,
                              mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                      printf ("x = ");
                      mpfr_dump (x);
                      printf ("y = ");
                      mpfr_dump (y);
                      printf ("Expected "); mpfr_dump (z1);
                      printf ("  with inex = %d\n", inex1);
                      printf ("Got      "); mpfr_dump (z2);
                      printf ("  with inex = %d\n", inex2);
                      exit (1);
                    }
                }
              if (i == 0)
                mpfr_nextabove (y);
              else
                {
                  if (p < 6)
                    break;
                  mpfr_nextbelow (y);
                  mpfr_mul_ui (y, y, 25, MPFR_RNDD);
                  mpfr_div_2ui (y, y, 4, MPFR_RNDN);
                }
            }
        }
      mpfr_clears (x, y, z1, z2, (mpfr_ptr) 0);
    }
}

/* Tests on UBF.
 *
 * Note: mpfr_sub1sp will never be used as it does not support UBF.
 * Thus there is no need to generate tests for both mpfr_sub1 and
 * mpfr_sub1sp.
 *
 * Note that mpfr_sub1 has a special branch "c small", where the second
 * argument c is sufficiently smaller than the ulp of the first argument
 * and the ulp of the result: MAX (aq, bq) + 2 <= diff_exp.
 * Tests should be done for both the main branch and this special branch
 * when this makes sense.
 */
#define REXP 1024

static void test_ubf_aux (void)
{
  mpfr_ubf_t x[11];
  mpfr_ptr p[11];
  int ex[11];
  mpfr_t ee, y, z, w;
  int i, j, k, neg, inexact, rnd;
  const int kn = 2;
  mpfr_exp_t e[] =
    { MPFR_EXP_MIN, MPFR_EMIN_MIN, -REXP, 0,
      REXP, MPFR_EMAX_MAX, MPFR_EXP_MAX };

  mpfr_init2 (ee, sizeof (mpfr_exp_t) * CHAR_BIT);
  mpfr_inits2 (64, y, z, (mpfr_ptr) 0);
  mpfr_init2 (w, 2);

  for (i = 0; i < 11; i++)
    p[i] = (mpfr_ptr) x[i];

  /* exact zero result, with small and large exponents */
  for (i = 0; i < 2; i++)
    {
      mpfr_init2 (p[i], 5 + (randlimb () % 128));
      mpfr_set_ui (p[i], 17, MPFR_RNDN);
      mpz_init (MPFR_ZEXP (p[i]));
      MPFR_SET_UBF (p[i]);
    }
  for (j = 0; j < numberof (e); j++)
    {
      inexact = mpfr_set_exp_t (ee, e[j], MPFR_RNDN);
      MPFR_ASSERTN (inexact == 0);
      inexact = mpfr_get_z (MPFR_ZEXP (p[0]), ee, MPFR_RNDN);
      MPFR_ASSERTN (inexact == 0);
      mpz_sub_ui (MPFR_ZEXP (p[0]), MPFR_ZEXP (p[0]), kn);

      for (k = -kn; k <= kn; k++)
        {
          /* exponent: e[j] + k, with |k| <= kn */
          mpz_set (MPFR_ZEXP (p[1]), MPFR_ZEXP (p[0]));

          for (neg = 0; neg <= 1; neg++)
            {
              RND_LOOP (rnd)
                {
                  /* Note: x[0] and x[1] are equal MPFR numbers, but do not
                     test mpfr_sub with arg2 == arg3 as pointers in order to
                     skip potentially optimized mpfr_sub code. */
                  inexact = mpfr_sub (z, p[0], p[1], (mpfr_rnd_t) rnd);
                  if (inexact != 0 || MPFR_NOTZERO (z) ||
                      (rnd != MPFR_RNDD ? MPFR_IS_NEG (z) : MPFR_IS_POS (z)))
                    {
                      printf ("Error 1 in test_ubf for exact zero result: "
                              "j=%d k=%d neg=%d, rnd=%s\nGot ", j, k, neg,
                              mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                      mpfr_dump (z);
                      printf ("inexact = %d\n", inexact);
                      exit (1);
                    }
                }

              for (i = 0; i < 2; i++)
                MPFR_CHANGE_SIGN (p[i]);
            }

          mpz_add_ui (MPFR_ZEXP (p[0]), MPFR_ZEXP (p[0]), 1);
        }
    }
  for (i = 0; i < 2; i++)
    {
      MPFR_UBF_CLEAR_EXP (p[i]);
      mpfr_clear (p[i]);
    }

  /* Up to a given exponent (for the result) and sign, test:
   *   (t + .11010) - (t + .00001) = .11001
   *   (t + 8) - (t + 111.00111)   = .11001
   * where t = 0 or a power of 2, e.g. 2^200. Test various exponents
   * (including those near the underflow/overflow boundaries) so that
   * the subtraction yields a normal number, an overflow or an underflow.
   * In MPFR_RNDA, also test with a 2-bit precision target, as this
   * yields an exponent change.
   *
   * Also test the "MAX (aq, bq) + 2 <= diff_exp" branch of sub1.c with
   * .1 - epsilon (possible decrease of the exponent) and .111 - epsilon
   * in precision 2 (possible increase of the exponent). The first test
   * triggers a possible decrease of the exponent (see bug fixed in r13806).
   * The second test triggers a possible increase of the exponent (see the
   * "exp_a != MPFR_EXP_MAX" test to avoid an integer overflow).
   */
  for (i = 0; i < 8; i++)
    {
      static int v[4] = { 26, 1, 256, 231 };

      mpfr_init2 (p[i], i < 4 ? 8 + (randlimb () % 128) : 256);
      if (i < 4)
        {
          inexact = mpfr_set_si_2exp (p[i], v[i], -5, MPFR_RNDN);
          MPFR_ASSERTN (inexact == 0);
        }
      else
        {
          inexact = mpfr_set_si_2exp (p[i], 1, 200, MPFR_RNDN);
          MPFR_ASSERTN (inexact == 0);
          inexact = mpfr_add (p[i], p[i], p[i-4], MPFR_RNDN);
          MPFR_ASSERTN (inexact == 0);
        }
      ex[i] = mpfr_get_exp (p[i]) + 5;
      MPFR_ASSERTN (ex[i] >= 0);
    }
  mpfr_inits2 (3, p[8], p[9], p[10], (mpfr_ptr) 0);
  inexact = mpfr_set_si_2exp (p[8], 1, 0, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  ex[8] = 5;
  inexact = mpfr_set_si_2exp (p[9], 1, 0, MPFR_RNDN);  /* will be epsilon */
  MPFR_ASSERTN (inexact == 0);
  ex[9] = 0;
  inexact = mpfr_set_si_2exp (p[10], 7, 0, MPFR_RNDN);
  MPFR_ASSERTN (inexact == 0);
  ex[10] = 5;

  for (i = 0; i < 11; i++)
    {
      mpz_init (MPFR_ZEXP (p[i]));
      MPFR_SET_UBF (p[i]);
    }

  for (j = 0; j < numberof (e); j++)
    {
      inexact = mpfr_set_exp_t (ee, e[j], MPFR_RNDN);
      MPFR_ASSERTN (inexact == 0);
      inexact = mpfr_get_z (MPFR_ZEXP (p[0]), ee, MPFR_RNDN);
      MPFR_ASSERTN (inexact == 0);
      for (i = 1; i < 11; i++)
        mpz_set (MPFR_ZEXP (p[i]), MPFR_ZEXP (p[0]));
      for (i = 0; i < 11; i++)
        {
          mpz_add_ui (MPFR_ZEXP (p[i]), MPFR_ZEXP (p[i]), ex[i]);
          mpz_sub_ui (MPFR_ZEXP (p[i]), MPFR_ZEXP (p[i]), 5 + kn);
        }
      mpz_sub_ui (MPFR_ZEXP (p[9]), MPFR_ZEXP (p[9]), 256);
      for (k = -kn; k <= kn; k++)
        {
          for (neg = 0; neg <= 1; neg++)
            {
              int sign = neg ? -1 : 1;

              RND_LOOP (rnd)
                for (i = 0; i <= 10; i += 2)
                  {
                    mpfr_exp_t e0;
                    mpfr_flags_t flags, flags_y;
                    int inex_y;

                    if (i >= 8)
                      {
                        int d;

                        e0 = MPFR_UBF_GET_EXP (p[i]);
                        if (e0 < MPFR_EXP_MIN + 3)
                          e0 += 3;

                        if (rnd == MPFR_RNDN)
                          d = i == 8 ? (e0 == __gmpfr_emin - 1 ? 3 : 4) : 6;
                        else if (MPFR_IS_LIKE_RNDZ (rnd, neg))
                          d = i == 8 ? 3 : 6;
                        else
                          d = i == 8 ? 4 : 8;

                        mpfr_clear_flags ();
                        inex_y = mpfr_set_si_2exp (w, sign * d, e0 - 3,
                                                   (mpfr_rnd_t) rnd);
                        flags_y = __gmpfr_flags | MPFR_FLAGS_INEXACT;
                        if (inex_y == 0)
                          inex_y = rnd == MPFR_RNDN ?
                            sign * (i == 8 ? 1 : -1) :
                            MPFR_IS_LIKE_RNDD ((mpfr_rnd_t) rnd, sign) ?
                            -1 : 1;
                        mpfr_set (y, w, MPFR_RNDN);

                        mpfr_clear_flags ();
                        inexact = mpfr_sub (w, p[i], p[9], (mpfr_rnd_t) rnd);
                        flags = __gmpfr_flags;

                        /* For MPFR_RNDF, only do a basic test. */
                        MPFR_ASSERTN (mpfr_check (w));
                        if (rnd == MPFR_RNDF)
                          continue;

                        goto testw;
                      }

                    mpfr_clear_flags ();
                    inexact = mpfr_sub (z, p[i], p[i+1], (mpfr_rnd_t) rnd);
                    flags = __gmpfr_flags;

                    /* For MPFR_RNDF, only do a basic test. */
                    MPFR_ASSERTN (mpfr_check (z));
                    if (rnd == MPFR_RNDF)
                      continue;

                    e0 = MPFR_UBF_GET_EXP (p[0]);

                    if (e0 < __gmpfr_emin)
                      {
                        mpfr_rnd_t r =
                          rnd == MPFR_RNDN && e0 < __gmpfr_emin - 1 ?
                          MPFR_RNDZ : (mpfr_rnd_t) rnd;
                        flags_y = MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT;
                        inex_y = mpfr_underflow (y, r, sign);
                      }
                    else if (e0 > __gmpfr_emax)
                      {
                        flags_y = MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT;
                        inex_y = mpfr_overflow (y, (mpfr_rnd_t) rnd, sign);
                      }
                    else
                      {
                        mpfr_set_si_2exp (y, sign * 25, e0 - 5, MPFR_RNDN);
                        flags_y = 0;
                        inex_y = 0;
                      }

                    if (flags != flags_y ||
                        ! SAME_SIGN (inexact, inex_y) ||
                        ! mpfr_equal_p (y, z))
                      {
                        printf ("Error 2 in test_ubf with "
                                "j=%d k=%d neg=%d i=%d rnd=%s\n",
                                j, k, neg, i,
                                mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                        printf ("emin=%" MPFR_EXP_FSPEC "d "
                                "emax=%" MPFR_EXP_FSPEC "d\n",
                                (mpfr_eexp_t) __gmpfr_emin,
                                (mpfr_eexp_t) __gmpfr_emax);
                        printf ("b = ");
                        mpfr_dump (p[i]);
                        printf ("c = ");
                        mpfr_dump (p[i+1]);
                        printf ("Expected ");
                        mpfr_dump (y);
                        printf ("with inex = %d and flags =", inex_y);
                        flags_out (flags_y);
                        printf ("Got      ");
                        mpfr_dump (z);
                        printf ("with inex = %d and flags =", inexact);
                        flags_out (flags);
                        exit (1);
                      }

                    /* Do the following 2-bit precision test only in RNDA. */
                    if (rnd != MPFR_RNDA)
                      continue;

                    mpfr_clear_flags ();
                    inexact = mpfr_sub (w, p[i], p[i+1], MPFR_RNDA);
                    flags = __gmpfr_flags;
                    if (e0 < MPFR_EXP_MAX)
                      e0++;

                    if (e0 < __gmpfr_emin)
                      {
                        flags_y = MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT;
                        inex_y = mpfr_underflow (y, MPFR_RNDA, sign);
                      }
                    else if (e0 > __gmpfr_emax)
                      {
                        flags_y = MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT;
                        inex_y = mpfr_overflow (y, MPFR_RNDA, sign);
                      }
                    else
                      {
                        mpfr_set_si_2exp (y, sign, e0 - 1, MPFR_RNDN);
                        flags_y = MPFR_FLAGS_INEXACT;
                        inex_y = sign;
                      }

                  testw:
                    if (flags != flags_y ||
                        ! SAME_SIGN (inexact, inex_y) ||
                        ! mpfr_equal_p (y, w))
                      {
                        printf ("Error 3 in test_ubf with "
                                "j=%d k=%d neg=%d i=%d rnd=%s\n",
                                j, k, neg, i,
                                mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                        printf ("emin=%" MPFR_EXP_FSPEC "d "
                                "emax=%" MPFR_EXP_FSPEC "d\n",
                                (mpfr_eexp_t) __gmpfr_emin,
                                (mpfr_eexp_t) __gmpfr_emax);
                        printf ("b = ");
                        mpfr_dump (p[i]);
                        printf ("c = ");
                        mpfr_dump (p[i <= 8 ? i+1 : 9]);
                        printf ("Expected ");
                        /* Set y to a 2-bit precision just for the output.
                           Since we exit, this will have no other effect. */
                        mpfr_prec_round (y, 2, MPFR_RNDA);
                        mpfr_dump (y);
                        printf ("with inex = %d and flags =", inex_y);
                        flags_out (flags_y);
                        printf ("Got      ");
                        mpfr_dump (w);
                        printf ("with inex = %d and flags =", inexact);
                        flags_out (flags);
                        exit (1);
                      }
                  }

              for (i = 0; i < 11; i++)
                MPFR_CHANGE_SIGN (p[i]);
            }

          for (i = 0; i < 11; i++)
            mpz_add_ui (MPFR_ZEXP (p[i]), MPFR_ZEXP (p[i]), 1);
        }
    }
  for (i = 0; i < 11; i++)
    {
      MPFR_UBF_CLEAR_EXP (p[i]);
      mpfr_clear (p[i]);
    }

  mpfr_clears (ee, y, z, w, (mpfr_ptr) 0);
}

/* Run the tests on UBF with the maximum exponent range and with a
   reduced exponent range. */
static void test_ubf (void)
{
  mpfr_exp_t emin, emax;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  set_emin (MPFR_EMIN_MIN);
  set_emax (MPFR_EMAX_MAX);
  test_ubf_aux ();

  set_emin (-REXP);
  set_emax (REXP);
  test_ubf_aux ();

  set_emin (emin);
  set_emax (emax);
}

#define TEST_FUNCTION test_sub
#define TWO_ARGS
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), randlimb () % 100, RANDS)
#include "tgeneric.c"

int
main (void)
{
  mpfr_prec_t p;
  unsigned int i;

  tests_start_mpfr ();

  test_rndf ();
  testall_rndf (7);
  test_rndf_exact (200);
  bug20101017 ();
  check_rounding ();
  check_diverse ();
  check_inexact ();
  check_max_almosteven ();
  bug_ddefour ();
  bug20180215 ();
  bug20180216 ();
  bug20180217 ();
  for (p=2; p<200; p++)
    for (i=0; i<50; i++)
      check_two_sum (p);
  test_generic (MPFR_PREC_MIN, 800, 100);
  test_ubf ();

  tests_end_mpfr ();
  return 0;
}
