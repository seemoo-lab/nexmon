/* Test file for mpfr_div_ui.

Copyright 1999-2025 Free Software Foundation, Inc.
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

#include <float.h>

#include "mpfr-test.h"

static void
check (const char *ds, unsigned long u, mpfr_rnd_t rnd, const char *es)
{
  mpfr_t x, y;

  mpfr_init2 (x, 53);
  mpfr_init2 (y, 53);
  mpfr_set_str1 (x, ds);
  mpfr_div_ui (y, x, u, rnd);
  if (mpfr_cmp_str1 (y, es))
    {
      printf ("mpfr_div_ui failed for x=%s, u=%lu, rnd=%s\n", ds, u,
              mpfr_print_rnd_mode (rnd));
      printf ("expected result is %s, got", es);
      mpfr_out_str(stdout, 10, 0, y, MPFR_RNDN);
      exit (1);
    }
  mpfr_clear (x);
  mpfr_clear (y);
}

static void
special (void)
{
  mpfr_t x, y;
  unsigned xprec, yprec;

  mpfr_init (x);
  mpfr_init (y);

  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 32);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_div_ui (y, x, 3, MPFR_RNDN);

  mpfr_set_prec (x, 100);
  mpfr_set_prec (y, 100);
  mpfr_urandomb (x, RANDS);
  mpfr_div_ui (y, x, 123456, MPFR_RNDN);
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_div_ui (y, x, 123456789, MPFR_RNDN);
  if (mpfr_cmp_ui (y, 0))
    {
      printf ("mpfr_div_ui gives non-zero for 0/ui\n");
      exit (1);
    }

  /* bug found by Norbert Mueller, 21 Aug 2001 */
  mpfr_set_prec (x, 110);
  mpfr_set_prec (y, 60);
  mpfr_set_str_binary (x, "0.110101110011111110011111001110011001110111000000111110001000111011000011E-44");
  mpfr_div_ui (y, x, 17, MPFR_RNDN);
  mpfr_set_str_binary (x, "0.11001010100101100011101110000001100001010110101001010011011E-48");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in x/17 for x=1/16!\n");
      printf ("Expected ");
      mpfr_out_str (stdout, 2, 0, x, MPFR_RNDN);
      printf ("\nGot      ");
      mpfr_out_str (stdout, 2, 0, y, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  /* corner case */
  mpfr_set_prec (x, 2 * mp_bits_per_limb);
  mpfr_set_prec (y, 2);
  mpfr_set_ui (x, 4, MPFR_RNDN);
  mpfr_nextabove (x);
  mpfr_div_ui (y, x, 2, MPFR_RNDN); /* exactly in the middle */
  MPFR_ASSERTN(mpfr_cmp_ui (y, 2) == 0);
  (mpfr_div_ui) (y, x, 2, MPFR_RNDN); /* exactly in the middle */
  MPFR_ASSERTN(mpfr_cmp_ui (y, 2) == 0);

  mpfr_set_prec (x, 3 * mp_bits_per_limb);
  mpfr_set_prec (y, 2);
  mpfr_set_ui (x, 2, MPFR_RNDN);
  mpfr_nextabove (x);
  mpfr_div_ui (y, x, 2, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0);
  (mpfr_div_ui) (y, x, 2, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (y, 1) == 0);

  mpfr_set_prec (x, 3 * mp_bits_per_limb);
  mpfr_set_prec (y, 2);
  mpfr_set_si (x, -4, MPFR_RNDN);
  mpfr_nextbelow (x);
  mpfr_div_ui (y, x, 2, MPFR_RNDD);
  MPFR_ASSERTN(mpfr_cmp_si (y, -3) == 0);
  (mpfr_div_ui) (y, x, 2, MPFR_RNDD);
  MPFR_ASSERTN(mpfr_cmp_si (y, -3) == 0);

  for (xprec = 53; xprec <= 128; xprec++)
    {
      mpfr_set_prec (x, xprec);
      mpfr_set_str_binary (x, "0.1100100100001111110011111000000011011100001100110111E2");
      for (yprec = 53; yprec <= 128; yprec++)
        {
          mpfr_set_prec (y, yprec);
          mpfr_div_ui (y, x, 1, MPFR_RNDN);
          if (mpfr_cmp(x,y))
            {
              printf ("division by 1.0 fails for xprec=%u, yprec=%u\n", xprec, yprec);
              printf ("expected "); mpfr_dump (x);
              printf ("got      "); mpfr_dump (y);
              exit (1);
            }
        }
    }

  /* Bug reported by Mark Dickinson, 6 Nov 2007 */
  mpfr_set_si (x, 0, MPFR_RNDN);
  mpfr_set_si (y, -1, MPFR_RNDN);
  mpfr_div_ui (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(MPFR_IS_ZERO(y) && MPFR_IS_POS(y));
  (mpfr_div_ui) (y, x, 4, MPFR_RNDN);
  MPFR_ASSERTN(MPFR_IS_ZERO(y) && MPFR_IS_POS(y));

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
check_inexact (void)
{
  mpfr_t x, y, z;
  mpfr_prec_t px, py;
  int inexact, cmp;
  unsigned long int u;
  int rnd;

  mpfr_init (x);
  mpfr_init (y);
  mpfr_init (z);

  for (px=2; px<300; px++)
    {
      mpfr_set_prec (x, px);
      mpfr_urandomb (x, RANDS);
      do
        {
          u = randlimb ();
        }
      while (u == 0);
      for (py=2; py<300; py++)
        {
          mpfr_set_prec (y, py);
          mpfr_set_prec (z, py + mp_bits_per_limb);
          /* The following test fails with MPFR_RNDF ("Wrong ternary value")
             when building with CFLAGS="-Wall -Werror -std=c90 -pedantic
             -Wno-error=overlength-strings -Wno-error=format" so that
             MPFR_LONG_WITHIN_LIMB is not defined (the implementation
             is not the same in this case). But the ternary value is not
             specified for MPFR_RNDF. Thus use RND_LOOP_NO_RNDF. */
          RND_LOOP_NO_RNDF (rnd)
            {
              inexact = mpfr_div_ui (y, x, u, (mpfr_rnd_t) rnd);
              if (mpfr_mul_ui (z, y, u, (mpfr_rnd_t) rnd))
                {
                  printf ("z <- y * u should be exact for u=%lu\n", u);
                  printf ("y="); mpfr_dump (y);
                  printf ("z="); mpfr_dump (z);
                  exit (1);
                }
              cmp = mpfr_cmp (z, x);
              if (((inexact == 0) && (cmp != 0)) ||
                  ((inexact > 0) && (cmp <= 0)) ||
                  ((inexact < 0) && (cmp >= 0)))
                {
                  printf ("Wrong ternary value for u=%lu, rnd=%s\n", u,
                          mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                  printf ("x="); mpfr_dump (x);
                  printf ("y="); mpfr_dump (y);
                  exit (1);
                }
            }
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}

/* Note that the preprocessor test and the if test in the function are
   complementary. If the size of an unsigned long is a power of 2 and
   this type has no padding bits, the test on ULONG_MAX and the if test
   are equivalent. The preprocessor test can avoid compiler warnings
   about the large shifts. */
#if GMP_NUMB_BITS == 64 && ULONG_MAX > 4294967295
/* With r11140, on a 64-bit machine with GMP_CHECK_RANDOMIZE=1484406128:
   Consistency error for i = 2577
*/
static void
test_20170105 (void)
{
  mpfr_t x,z, t;

  if (sizeof (unsigned long) * CHAR_BIT >= 64)
    {
      mpfr_init2 (x, 138);
      mpfr_init2 (z, 128);
      mpfr_init2 (t, 128);
      mpfr_set_str_binary (x, "100110111001001000101111010010011101111110111111110001110100000001110111010100111010100011101010110000010100000011100100110101101011000000");
      /* up to exponents, x/y is exactly
         367625553447399614694201910705139062483, which has 129 bits,
         thus we are in the round-to-nearest-even case, and since the
         penultimate bit of x/y is 1, we should round upwards */
      mpfr_set_str_binary (t, "10001010010010010000110110010110111111111100011011101010000000000110101000010001011110011011010000111010000000001100101101101010E-53");
      mpfr_div_ui (z, x, 36UL << 58, MPFR_RNDN);
      MPFR_ASSERTN(mpfr_equal_p (z, t));

      mpfr_set_prec (x, 189);
      mpfr_set_prec (z, 185);
      mpfr_set_prec (t, 185);
      mpfr_set_str_binary (x, "100001010000111100011110111010000011110000000110100010001010101011110001110000110111101000100100001101010011000111110100011111110110011011101000000000001010010010111011001100111111111101001");
      mpfr_set_str_binary (t, "10011000000100010100011111100100110101101110001011100101010101011010011010010110010000100111001010000101111011111111001011011010101111101100000000000000101111000100001110101001001001000E-60");
      mpfr_div_ui (z, x, 7UL << 61, MPFR_RNDN);
      MPFR_ASSERTN(mpfr_equal_p (z, t));

      mpfr_clears (x, z, t, (mpfr_ptr) 0);
    }
}
#endif

static void
bug20180126 (void)
{
  mpfr_t w, x, y, z, t;
  unsigned long u, v;
  int i, k, m, n, p, pmax, q, r;
  int inex;

  /* Let m = n * q + r, with 0 <= r < v.
     (2^m-1) / (2^n-1) = 2^r * (2^(n*q)-1) / (2^n-1) + (2^r-1) / (2^n-1)
       = sum(i=0,q-1,2^(r+n*i)) + sum(i=1,inf,(2^r-1)*2^(-n*i))
  */
  n = 1;
  for (u = 1; u != ULONG_MAX; u = (u << 1) + 1)
    n++;
  pmax = 6 * n;
  mpfr_init2 (t, n);
  for (m = n; m < 4 * n; m++)
    {
      q = m / n;
      r = m % n;
      mpfr_init2 (w, pmax + n + 1);
      mpfr_set_zero (w, 1);
      for (i = 0; i < q; i++)
        {
          inex = mpfr_set_ui_2exp (t, 1, r + n * i, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
          inex = mpfr_add (w, w, t, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
        }
      v = (1UL << r) - 1;
      for (i = 1; n * (q - 1 + i) <= MPFR_PREC (w); i++)
        {
          inex = mpfr_set_ui_2exp (t, v, - n * i, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
          mpfr_add (w, w, t, MPFR_RNDN);
        }
      for (p = pmax; p >= MPFR_PREC_MIN; p--)
        {
          mpfr_inits2 (p, y, z, (mpfr_ptr) 0);
          mpfr_set (z, w, MPFR_RNDN);  /* the sticky bit is not 0 */
          mpfr_init2 (x, m);
          inex = mpfr_set_ui_2exp (x, 1, m, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
          inex = mpfr_sub_ui (x, x, 1, MPFR_RNDN);  /* x = 2^m-1 */
          MPFR_ASSERTN (inex == 0);
          for (k = 0; k < 2; k++)
            {
              if (k)
                {
                  inex = mpfr_prec_round (x, 6 * n, MPFR_RNDN);
                  MPFR_ASSERTN (inex == 0);
                }
              inex = mpfr_div_ui (y, x, u, MPFR_RNDN);
              if (! mpfr_equal_p (y, z))
                {
                  printf ("Error in bug20180126 for (2^%d-1)/(2^%d-1)"
                          " with px=%d py=%d\n", m, n,
                          (int) MPFR_PREC (x), p);
                  printf ("Expected ");
                  mpfr_dump (z);
                  printf ("Got      ");
                  mpfr_dump (y);
                  exit (1);
                }
            }
          mpfr_clears (x, y, z, (mpfr_ptr) 0);
        }
      mpfr_clear (w);
    }
  mpfr_clear (t);

  /* This test expects that a limb fits in an unsigned long.
     One failing case from function bug20180126() in tdiv.c,
     for GMP_NUMB_BITS == 64. */
#if ULONG_MAX > 4294967295
  if (GMP_NUMB_BITS == 64 && MPFR_LIMB_MAX <= ULONG_MAX)
    {
      mpfr_init2 (x, 133);
      mpfr_init2 (y, 64);
      mpfr_set_ui (x, 1, MPFR_RNDN);
      mpfr_nextbelow (x); /* 1 - 2^(-133) = (2^133-1)/2^133 */
      u = MPFR_LIMB_MAX;  /* 2^64 - 1 */
      inex = mpfr_div_ui (y, x, u, MPFR_RNDN);
      /* 2^133*x/u = (2^133-1)/(2^64-1) = (2^64+1)*2^5 + 31/(2^64-1)
         and should be rounded to 2^69+2^6, thus x/u should be rounded
         to 2^(-133)*(2^69+2^6). */
      MPFR_ASSERTN (inex > 0);
      mpfr_nextbelow (y);
      MPFR_ASSERTN (mpfr_cmp_ui_2exp (y, 1, -64) == 0);

      mpfr_set_prec (x, 49);
      mpfr_set_str_binary (x, "0.1000000000000000000111111111111111111111100000000E0");
      /* x = 281476050452224/2^49 */
      /* let X = 2^256*x = q*u+r, then q has 192 bits, and
         r = 8222597979955926678 > u/2 thus we should round to (q+1)/2^256 */
      mpfr_set_prec (y, 192);
      /* The cast below avoid spurious warnings from GCC with a 32-bit ABI. */
      u = (mp_limb_t) 10865468317030705979U;
      inex = mpfr_div_ui (y, x, u, MPFR_RNDN);
      mpfr_init2 (z, 192);
      mpfr_set_str_binary (z, "0.110110010100111111000100101011011110010101010010001101100110101111001010100011010111010011100001101000110100011101001010000001010000001001011100000100000110101111110100100101011000000110011111E-64");
      MPFR_ASSERTN (inex > 0);
      MPFR_ASSERTN (mpfr_equal_p (y, z));
      mpfr_clear (x);
      mpfr_clear (y);
      mpfr_clear (z);
    }
#endif
}

/* check corner cases where the round bit is located in the upper bit of r */
static void
corner_cases (int n)
{
  mpfr_t x, y, t;
  unsigned long u, v;
  int i, xn;

  if (MPFR_LIMB_MAX <= ULONG_MAX)
    {
      /* We need xn > yn + 1, thus we take xn = 3 and yn = 1.
         Also take xn = 4 to 6 to cover more code. */
      for (xn = 3; xn < 6; xn++)
        {
          mpfr_init2 (x, xn * GMP_NUMB_BITS);
          mpfr_init2 (y, GMP_NUMB_BITS);
          mpfr_init2 (t, 2 * GMP_NUMB_BITS);
          for (i = 0; i < n; i++)
            {
              do
                u = randlimb ();
              while (u == 0);
              do
                v = randlimb ();
              while (v <= MPFR_LIMB_HIGHBIT);
              mpfr_set_ui (t, v, MPFR_RNDN);
              mpfr_sub_d (t, t, 0.5, MPFR_RNDN);
              /* t = v-1/2 */
              mpfr_mul_ui (x, t, u, MPFR_RNDN);

              /* when x = (v-1/2)*u, x/u should give v-1/2, which should
                 round to either v (if v is even) or v-1 (if v is odd) */
              mpfr_div_ui (y, x, u, MPFR_RNDN);
              MPFR_ASSERTN(mpfr_cmp_ui (y, v - (v & 1)) == 0);

              /* when x = (v-1/2)*u - epsilon, x/u should round to v-1 */
              mpfr_nextbelow (x);
              mpfr_div_ui (y, x, u, MPFR_RNDN);
              MPFR_ASSERTN(mpfr_cmp_ui (y, v - 1) == 0);

              /* when x = (v-1/2)*u + epsilon, x/u should round to v */
              mpfr_nextabove (x);
              mpfr_nextabove (x);
              mpfr_div_ui (y, x, u, MPFR_RNDN);
              MPFR_ASSERTN(mpfr_cmp_ui (y, v) == 0);
            }
          mpfr_clear (x);
          mpfr_clear (y);
          mpfr_clear (t);
        }
    }
}

static void
midpoint_exact (void)
{
  mpfr_t x, y1, y2;
  unsigned long j;
  int i, kx, ky, px, pxmin, py, pymin, r;
  int inex1, inex2;

  pymin = 1;
  for (i = 3; i < 32; i += 2)
    {
      if ((i & (i-2)) == 1)
        pymin++;
      for (j = 1; j != 0; j++)
        {
          if (j == 31)
            j = ULONG_MAX;
          /* Test of (i*j) / j with various precisions. The target precisions
             include: large, length(i), and length(i)-1; the latter case
             corresponds to a midpoint. */
          mpfr_init2 (x, 5 + sizeof(long) * CHAR_BIT);
          inex1 = mpfr_set_ui (x, j, MPFR_RNDN);
          MPFR_ASSERTN (inex1 == 0);
          inex1 = mpfr_mul_ui (x, x, i, MPFR_RNDN);
          MPFR_ASSERTN (inex1 == 0);
          /* x = (i*j) */
          pxmin = mpfr_min_prec (x);
          if (pxmin < MPFR_PREC_MIN)
            pxmin = MPFR_PREC_MIN;
          for (kx = 0; kx < 8; kx++)
            {
              px = pxmin;
              if (kx != 0)
                px += randlimb () % (4 * GMP_NUMB_BITS);
              inex1 = mpfr_prec_round (x, px, MPFR_RNDN);
              MPFR_ASSERTN (inex1 == 0);
              for (ky = 0; ky < 8; ky++)
                {
                  py = pymin;
                  if (ky == 0)
                    py--;
                  else if (ky > 1)
                    py += randlimb () % (4 * GMP_NUMB_BITS);
                  if (py < MPFR_PREC_MIN)
                    break;
                  mpfr_inits2 (py, y1, y2, (mpfr_ptr) 0);
                  RND_LOOP_NO_RNDF (r)
                    {
                      inex1 = mpfr_set_ui (y1, i, (mpfr_rnd_t) r);
                      inex2 = mpfr_div_ui (y2, x, j, (mpfr_rnd_t) r);
                      if (! mpfr_equal_p (y1, y2) ||
                          ! SAME_SIGN (inex1, inex2))
                        {
                          printf ("Error in midpoint_exact for "
                                  "i=%d j=%lu px=%d py=%d %s\n", i, j, px, py,
                                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                          printf ("Expected ");
                          mpfr_dump (y1);
                          printf ("with inex = %d\n", inex1);
                          printf ("Got      ");
                          mpfr_dump (y2);
                          printf ("with inex = %d\n", inex2);
                          exit (1);
                        }
                    }
                  mpfr_clears (y1, y2, (mpfr_ptr) 0);
                }
            }
          mpfr_clear (x);
        }
    }
}

static void
check_coverage (void)
{
#ifdef MPFR_COV_CHECK
  int i, j;
  int err = 0;

  if (MPFR_LIMB_MAX <= ULONG_MAX)
    {
      for (i = 0; i < numberof (__gmpfr_cov_div_ui_sb); i++)
        for (j = 0; j < 2; j++)
          if (!__gmpfr_cov_div_ui_sb[i][j])
            {
              printf ("mpfr_div_ui not tested on case %d, sb=%d\n", i, j);
              err = 1;
            }

      if (err)
        exit (1);
    }
  else /* e.g. mips64 with the n32 ABI */
    printf ("Warning! Value coverage disabled (mp_limb_t > unsigned long).\n");
#endif
}

#define TEST_FUNCTION mpfr_div_ui
#define ULONG_ARG2
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), 1, RANDS)
#include "tgeneric.c"

int
main (int argc, char **argv)
{
  mpfr_t x;

  tests_start_mpfr ();

  corner_cases (100);

  bug20180126 ();

  special ();

  check_inexact ();

  check("1.0", 3, MPFR_RNDN, "3.3333333333333331483e-1");
  check("1.0", 3, MPFR_RNDZ, "3.3333333333333331483e-1");
  check("1.0", 3, MPFR_RNDU, "3.3333333333333337034e-1");
  check("1.0", 3, MPFR_RNDD, "3.3333333333333331483e-1");
  check("1.0", 2116118, MPFR_RNDN, "4.7256343927890600483e-7");
  check("1.098612288668109782", 5, MPFR_RNDN, "0.21972245773362195087");
#if GMP_NUMB_BITS == 64 && ULONG_MAX > 4294967295
  test_20170105 ();
#endif

  mpfr_init2 (x, 53);
  mpfr_set_ui (x, 3, MPFR_RNDD);
  mpfr_log (x, x, MPFR_RNDD);
  mpfr_div_ui (x, x, 5, MPFR_RNDD);
  if (mpfr_cmp_str1 (x, "0.21972245773362189536"))
    {
      printf ("Error in mpfr_div_ui for x=ln(3), u=5\n");
      exit (1);
    }
  mpfr_clear (x);

  test_generic (MPFR_PREC_MIN, 200, 100);
  midpoint_exact ();

  check_coverage ();
  tests_end_mpfr ();
  return 0;
}
