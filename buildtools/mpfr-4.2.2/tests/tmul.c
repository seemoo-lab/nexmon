/* Test file for mpfr_mul.

Copyright 1999, 2001-2025 Free Software Foundation, Inc.
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
test_mul (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  int res;
  int ok = rnd_mode == MPFR_RNDN && mpfr_number_p (b) && mpfr_number_p (c);
  if (ok)
    {
      mpfr_print_raw (b);
      printf (" ");
      mpfr_print_raw (c);
    }
  res = mpfr_mul (a, b, c, rnd_mode);
  if (ok)
    {
      printf (" ");
      mpfr_print_raw (a);
      printf ("\n");
    }
  return res;
}
#else
#define test_mul mpfr_mul
#endif

/* checks that xs * ys gives the expected result res */
static void
check (const char *xs, const char *ys, mpfr_rnd_t rnd_mode,
        unsigned int px, unsigned int py, unsigned int pz, const char *res)
{
  mpfr_t xx, yy, zz;

  mpfr_init2 (xx, px);
  mpfr_init2 (yy, py);
  mpfr_init2 (zz, pz);
  mpfr_set_str1 (xx, xs);
  mpfr_set_str1 (yy, ys);
  test_mul(zz, xx, yy, rnd_mode);
  if (mpfr_cmp_str1 (zz, res) )
    {
      printf ("(1) mpfr_mul failed for x=%s y=%s with rnd=%s\n",
              xs, ys, mpfr_print_rnd_mode (rnd_mode));
      printf ("correct is %s, mpfr_mul gives ", res);
      mpfr_out_str (stdout, 10, 0, zz, MPFR_RNDN);
      putchar ('\n');
      exit (1);
    }
  mpfr_clear (xx);
  mpfr_clear (yy);
  mpfr_clear (zz);
}

static void
check53 (const char *xs, const char *ys, mpfr_rnd_t rnd_mode, const char *zs)
{
  mpfr_t xx, yy, zz;

  mpfr_inits2 (53, xx, yy, zz, (mpfr_ptr) 0);
  mpfr_set_str1 (xx, xs);
  mpfr_set_str1 (yy, ys);
  test_mul (zz, xx, yy, rnd_mode);
  if (mpfr_cmp_str1 (zz, zs) )
    {
      printf ("(2) mpfr_mul failed for x=%s y=%s with rnd=%s\n",
              xs, ys, mpfr_print_rnd_mode(rnd_mode));
      printf ("correct result is %s,\n mpfr_mul gives ", zs);
      mpfr_out_str (stdout, 10, 0, zz, MPFR_RNDN);
      putchar ('\n');
      exit (1);
    }
  mpfr_clears (xx, yy, zz, (mpfr_ptr) 0);
}

/* checks that x*y gives the right result with 24 bits of precision */
static void
check24 (const char *xs, const char *ys, mpfr_rnd_t rnd_mode, const char *zs)
{
  mpfr_t xx, yy, zz;

  mpfr_inits2 (24, xx, yy, zz, (mpfr_ptr) 0);
  mpfr_set_str1 (xx, xs);
  mpfr_set_str1 (yy, ys);
  test_mul (zz, xx, yy, rnd_mode);
  if (mpfr_cmp_str1 (zz, zs) )
    {
      printf ("(3) mpfr_mul failed for x=%s y=%s with "
              "rnd=%s\n", xs, ys, mpfr_print_rnd_mode(rnd_mode));
      printf ("correct result is gives %s, mpfr_mul gives ", zs);
      mpfr_out_str (stdout, 10, 0, zz, MPFR_RNDN);
      putchar ('\n');
      exit (1);
    }
  mpfr_clears (xx, yy, zz, (mpfr_ptr) 0);
}

/* the following examples come from the paper "Number-theoretic Test
   Generation for Directed Rounding" from Michael Parks, Table 1 */
static void
check_float (void)
{
  check24("8388609.0",  "8388609.0", MPFR_RNDN, "70368760954880.0");
  check24("16777213.0", "8388609.0", MPFR_RNDN, "140737479966720.0");
  check24("8388611.0",  "8388609.0", MPFR_RNDN, "70368777732096.0");
  check24("12582911.0", "8388610.0", MPFR_RNDN, "105553133043712.0");
  check24("12582914.0", "8388610.0", MPFR_RNDN, "105553158209536.0");
  check24("13981013.0", "8388611.0", MPFR_RNDN, "117281279442944.0");
  check24("11184811.0", "8388611.0", MPFR_RNDN, "93825028587520.0");
  check24("11184810.0", "8388611.0", MPFR_RNDN, "93825020198912.0");
  check24("13981014.0", "8388611.0", MPFR_RNDN, "117281287831552.0");

  check24("8388609.0",  "8388609.0", MPFR_RNDZ, "70368760954880.0");
  check24("16777213.0", "8388609.0", MPFR_RNDZ, "140737471578112.0");
  check24("8388611.0",  "8388609.0", MPFR_RNDZ, "70368777732096.0");
  check24("12582911.0", "8388610.0", MPFR_RNDZ, "105553124655104.0");
  check24("12582914.0", "8388610.0", MPFR_RNDZ, "105553158209536.0");
  check24("13981013.0", "8388611.0", MPFR_RNDZ, "117281271054336.0");
  check24("11184811.0", "8388611.0", MPFR_RNDZ, "93825028587520.0");
  check24("11184810.0", "8388611.0", MPFR_RNDZ, "93825011810304.0");
  check24("13981014.0", "8388611.0", MPFR_RNDZ, "117281287831552.0");

  check24("8388609.0",  "8388609.0", MPFR_RNDU, "70368769343488.0");
  check24("16777213.0", "8388609.0", MPFR_RNDU, "140737479966720.0");
  check24("8388611.0",  "8388609.0", MPFR_RNDU, "70368786120704.0");
  check24("12582911.0", "8388610.0", MPFR_RNDU, "105553133043712.0");
  check24("12582914.0", "8388610.0", MPFR_RNDU, "105553166598144.0");
  check24("13981013.0", "8388611.0", MPFR_RNDU, "117281279442944.0");
  check24("11184811.0", "8388611.0", MPFR_RNDU, "93825036976128.0");
  check24("11184810.0", "8388611.0", MPFR_RNDU, "93825020198912.0");
  check24("13981014.0", "8388611.0", MPFR_RNDU, "117281296220160.0");

  check24("8388609.0",  "8388609.0", MPFR_RNDD, "70368760954880.0");
  check24("16777213.0", "8388609.0", MPFR_RNDD, "140737471578112.0");
  check24("8388611.0",  "8388609.0", MPFR_RNDD, "70368777732096.0");
  check24("12582911.0", "8388610.0", MPFR_RNDD, "105553124655104.0");
  check24("12582914.0", "8388610.0", MPFR_RNDD, "105553158209536.0");
  check24("13981013.0", "8388611.0", MPFR_RNDD, "117281271054336.0");
  check24("11184811.0", "8388611.0", MPFR_RNDD, "93825028587520.0");
  check24("11184810.0", "8388611.0", MPFR_RNDD, "93825011810304.0");
  check24("13981014.0", "8388611.0", MPFR_RNDD, "117281287831552.0");
}

/* check sign of result */
static void
check_sign (void)
{
  mpfr_t a, b;

  mpfr_init2 (a, 53);
  mpfr_init2 (b, 53);
  mpfr_set_si (a, -1, MPFR_RNDN);
  mpfr_set_ui (b, 2, MPFR_RNDN);
  test_mul(a, b, b, MPFR_RNDN);
  if (mpfr_cmp_ui (a, 4) )
    {
      printf ("2.0*2.0 gives \n");
      mpfr_out_str (stdout, 10, 0, a, MPFR_RNDN);
      putchar ('\n');
      exit (1);
    }
  mpfr_clear(a); mpfr_clear(b);
}

/* checks that the inexact return value is correct */
static void
check_exact (void)
{
  mpfr_t a, b, c, d;
  mpfr_prec_t prec;
  int i, inexact;
  mpfr_rnd_t rnd;

  mpfr_init (a);
  mpfr_init (b);
  mpfr_init (c);
  mpfr_init (d);

  mpfr_set_prec (a, 17);
  mpfr_set_prec (b, 17);
  mpfr_set_prec (c, 32);
  mpfr_set_str_binary (a, "1.1000111011000100e-1");
  mpfr_set_str_binary (b, "1.0010001111100111e-1");
  if (test_mul (c, a, b, MPFR_RNDZ))
    {
      printf ("wrong return value (1)\n");
      exit (1);
    }

  for (prec = MPFR_PREC_MIN; prec < 100; prec++)
    {
      mpfr_set_prec (a, prec);
      mpfr_set_prec (b, prec);
      /* for prec=1, ensure PREC(c) >= 1 */
      mpfr_set_prec (c, 2 * prec - 2 + (prec == 1));
      mpfr_set_prec (d, 2 * prec);
      for (i = 0; i < 1000; i++)
        {
          mpfr_urandomb (a, RANDS);
          mpfr_urandomb (b, RANDS);
          rnd = RND_RAND ();
          inexact = test_mul (c, a, b, rnd);
          if (test_mul (d, a, b, rnd)) /* should be always exact */
            {
              printf ("unexpected inexact return value\n");
              exit (1);
            }
          if ((inexact == 0) && mpfr_cmp (c, d) && rnd != MPFR_RNDF)
            {
              printf ("rnd=%s: inexact=0 but results differ\n",
                      mpfr_print_rnd_mode (rnd));
              printf ("a=");
              mpfr_out_str (stdout, 2, 0, a, rnd);
              printf ("\nb=");
              mpfr_out_str (stdout, 2, 0, b, rnd);
              printf ("\nc=");
              mpfr_out_str (stdout, 2, 0, c, rnd);
              printf ("\nd=");
              mpfr_out_str (stdout, 2, 0, d, rnd);
              printf ("\n");
              exit (1);
            }
          else if (inexact && (mpfr_cmp (c, d) == 0) && rnd != MPFR_RNDF)
            {
              printf ("inexact!=0 but results agree\n");
              printf ("prec=%u rnd=%s a=", (unsigned int) prec,
                      mpfr_print_rnd_mode (rnd));
              mpfr_out_str (stdout, 2, 0, a, rnd);
              printf ("\nb=");
              mpfr_out_str (stdout, 2, 0, b, rnd);
              printf ("\nc=");
              mpfr_out_str (stdout, 2, 0, c, rnd);
              printf ("\nd=");
              mpfr_out_str (stdout, 2, 0, d, rnd);
              printf ("\n");
              exit (1);
            }
        }
    }

  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
  mpfr_clear (d);
}

static void
check_max (void)
{
  mpfr_t xx, yy, zz;
  mpfr_exp_t emin;
  int inex;

  mpfr_init2(xx, 4);
  mpfr_init2(yy, 4);
  mpfr_init2(zz, 4);
  mpfr_set_str1 (xx, "0.68750");
  mpfr_mul_2si(xx, xx, MPFR_EMAX_DEFAULT/2, MPFR_RNDN);
  mpfr_set_str1 (yy, "0.68750");
  mpfr_mul_2si(yy, yy, MPFR_EMAX_DEFAULT - MPFR_EMAX_DEFAULT/2 + 1, MPFR_RNDN);
  mpfr_clear_flags();
  test_mul(zz, xx, yy, MPFR_RNDU);
  if (!(mpfr_overflow_p() && MPFR_IS_INF(zz)))
    {
      printf("check_max failed (should be an overflow)\n");
      exit(1);
    }

  mpfr_clear_flags();
  test_mul(zz, xx, yy, MPFR_RNDD);
  if (mpfr_overflow_p() || MPFR_IS_INF(zz))
    {
      printf("check_max failed (should NOT be an overflow)\n");
      exit(1);
    }
  mpfr_set_str1 (xx, "0.93750");
  mpfr_mul_2si(xx, xx, MPFR_EMAX_DEFAULT, MPFR_RNDN);
  if (!(MPFR_IS_FP(xx) && MPFR_IS_FP(zz)))
    {
      printf("check_max failed (internal error)\n");
      exit(1);
    }
  if (mpfr_cmp(xx, zz) != 0)
    {
      printf("check_max failed: got ");
      mpfr_out_str(stdout, 2, 0, zz, MPFR_RNDZ);
      printf(" instead of ");
      mpfr_out_str(stdout, 2, 0, xx, MPFR_RNDZ);
      printf("\n");
      exit(1);
    }

  /* check underflow */
  emin = mpfr_get_emin ();
  set_emin (0);
  mpfr_set_str_binary (xx, "0.1E0");
  mpfr_set_str_binary (yy, "0.1E0");
  test_mul (zz, xx, yy, MPFR_RNDN);
  /* exact result is 0.1E-1, which should round to 0 */
  MPFR_ASSERTN(mpfr_cmp_ui (zz, 0) == 0 && MPFR_IS_POS(zz));
  set_emin (emin);

  /* coverage test for mpfr_powerof2_raw */
  emin = mpfr_get_emin ();
  set_emin (0);
  mpfr_set_prec (xx, mp_bits_per_limb + 1);
  mpfr_set_str_binary (xx, "0.1E0");
  mpfr_nextabove (xx);
  mpfr_set_str_binary (yy, "0.1E0");
  test_mul (zz, xx, yy, MPFR_RNDN);
  /* exact result is just above 0.1E-1, which should round to minfloat */
  MPFR_ASSERTN(mpfr_cmp (zz, yy) == 0);
  set_emin (emin);

  /* coverage test for mulders.c, case n > MUL_FFT_THRESHOLD */
  mpfr_set_prec (xx, (MUL_FFT_THRESHOLD + 1) * GMP_NUMB_BITS);
  mpfr_set_prec (yy, (MUL_FFT_THRESHOLD + 1) * GMP_NUMB_BITS);
  mpfr_set_prec (zz, (MUL_FFT_THRESHOLD + 1) * GMP_NUMB_BITS);
  mpfr_set_ui (xx, 1, MPFR_RNDN);
  mpfr_nextbelow (xx);
  mpfr_set_ui (yy, 1, MPFR_RNDN);
  mpfr_nextabove (yy);
  /* xx = 1 - 2^(-p), yy = 1 + 2^(1-p), where p = PREC(x),
     thus xx * yy should be rounded to 1 */
  inex = mpfr_mul (zz, xx, yy, MPFR_RNDN);
  MPFR_ASSERTN(inex < 0);
  MPFR_ASSERTN(mpfr_cmp_ui (zz, 1) == 0);

  mpfr_clear(xx);
  mpfr_clear(yy);
  mpfr_clear(zz);
}

static void
check_min(void)
{
  mpfr_t xx, yy, zz;

  mpfr_init2(xx, 4);
  mpfr_init2(yy, 4);
  mpfr_init2(zz, 3);
  mpfr_set_str1(xx, "0.9375");
  mpfr_mul_2si(xx, xx, MPFR_EMIN_DEFAULT/2, MPFR_RNDN);
  mpfr_set_str1(yy, "0.9375");
  mpfr_mul_2si(yy, yy, MPFR_EMIN_DEFAULT - MPFR_EMIN_DEFAULT/2 - 1, MPFR_RNDN);
  test_mul(zz, xx, yy, MPFR_RNDD);
  if (MPFR_NOTZERO (zz))
    {
      printf("check_min failed: got ");
      mpfr_out_str(stdout, 2, 0, zz, MPFR_RNDZ);
      printf(" instead of 0\n");
      exit(1);
    }

  test_mul(zz, xx, yy, MPFR_RNDU);
  mpfr_set_str1 (xx, "0.5");
  mpfr_mul_2si(xx, xx, MPFR_EMIN_DEFAULT, MPFR_RNDN);
  if (mpfr_sgn(xx) <= 0)
    {
      printf("check_min failed (internal error)\n");
      exit(1);
    }
  if (mpfr_cmp(xx, zz) != 0)
    {
      printf("check_min failed: got ");
      mpfr_out_str(stdout, 2, 0, zz, MPFR_RNDZ);
      printf(" instead of ");
      mpfr_out_str(stdout, 2, 0, xx, MPFR_RNDZ);
      printf("\n");
      exit(1);
    }

  mpfr_clear(xx);
  mpfr_clear(yy);
  mpfr_clear(zz);
}

static void
check_nans (void)
{
  mpfr_t  p, x, y;

  mpfr_init2 (x, 123L);
  mpfr_init2 (y, 123L);
  mpfr_init2 (p, 123L);

  /* nan * 0 == nan */
  mpfr_set_nan (x);
  mpfr_set_ui (y, 0L, MPFR_RNDN);
  test_mul (p, x, y, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (p));

  /* 1 * nan == nan */
  mpfr_set_ui (x, 1L, MPFR_RNDN);
  mpfr_set_nan (y);
  test_mul (p, x, y, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (p));

  /* 0 * +inf == nan */
  mpfr_set_ui (x, 0L, MPFR_RNDN);
  mpfr_set_nan (y);
  test_mul (p, x, y, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_nan_p (p));

  /* +1 * +inf == +inf */
  mpfr_set_ui (x, 1L, MPFR_RNDN);
  mpfr_set_inf (y, 1);
  test_mul (p, x, y, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (p));
  MPFR_ASSERTN (mpfr_sgn (p) > 0);

  /* -1 * +inf == -inf */
  mpfr_set_si (x, -1L, MPFR_RNDN);
  mpfr_set_inf (y, 1);
  test_mul (p, x, y, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inf_p (p));
  MPFR_ASSERTN (mpfr_sgn (p) < 0);

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (p);
}

#define BUFSIZE 1552

static void
get_string (char *s, FILE *fp)
{
  int c, n = BUFSIZE;

  while ((c = getc (fp)) != '\n')
    {
      if (c == EOF)
        {
          printf ("Error in get_string: end of file\n");
          exit (1);
        }
      *(unsigned char *)s++ = c;
      if (--n == 0)
        {
          printf ("Error in get_string: buffer is too small\n");
          exit (1);
        }
    }
  *s = '\0';
}

static void
check_regression (void)
{
  mpfr_t x, y, z;
  int i;
  FILE *fp;
  char s[BUFSIZE];

  mpfr_inits2 (6177, x, y, z, (mpfr_ptr) 0);
  /* we read long strings from a file since ISO C90 does not support strings of
     length > 509 */
  fp = src_fopen ("tmul.dat", "r");
  if (fp == NULL)
    {
      fprintf (stderr, "Error, cannot open tmul.dat in srcdir\n");
      exit (1);
    }
  get_string (s, fp);
  mpfr_set_str (y, s, 16, MPFR_RNDN);
  get_string (s, fp);
  mpfr_set_str (z, s, 16, MPFR_RNDN);
  i = mpfr_mul (x, y, z, MPFR_RNDN);
  get_string (s, fp);
  if (mpfr_cmp_str (x, s, 16, MPFR_RNDN) != 0 || i != -1)
    {
      printf ("Regression test 1 failed (i=%d, expected -1)\nx=", i);
      mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN); putchar ('\n');
      exit (1);
    }
  fclose (fp);

  mpfr_set_prec (x, 606);
  mpfr_set_prec (y, 606);
  mpfr_set_prec (z, 606);

  mpfr_set_str (y, "-f.ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff92daefc3f8052ca9f58736564d9e93e62d324@-1", 16, MPFR_RNDN);
  mpfr_set_str (z, "-f.ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff92daefc3f8052ca9f58736564d9e93e62d324@-1", 16, MPFR_RNDN);
  i = mpfr_mul (x, y, z, MPFR_RNDU);
  mpfr_set_str (y, "f.ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff25b5df87f00a5953eb0e6cac9b3d27cc5a64c@-1", 16, MPFR_RNDN);
  if (mpfr_cmp (x, y) || i <= 0)
    {
      printf ("Regression test (2) failed! (i=%d - Expected 1)\n", i);
      mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN); putchar ('\n');
      exit (1);
    }

  mpfr_set_prec (x, 184);
  mpfr_set_prec (y, 92);
  mpfr_set_prec (z, 1023);

  mpfr_set_str (y, "6.9b8c8498882770d8038c3b0@-1", 16, MPFR_RNDN);
  mpfr_set_str (z, "7.44e24b986e7fb296f1e936ce749fec3504cbf0d5ba769466b1c9f1578115efd5d29b4c79271191a920a99280c714d3a657ad6e3afbab77ffce9d697e9bb9110e26d676069afcea8b69f1d1541f2365042d80a97c21dcccd8ace4f1bb58b49922003e738e6f37bb82ef653cb2e87f763974e6ae50ae54e7724c38b80653e3289@255", 16, MPFR_RNDN);
  i = mpfr_mul (x, y, z, MPFR_RNDU);
  mpfr_set_prec (y, 184);
  mpfr_set_str (y, "3.0080038f2ac5054e3e71ccbb95f76aaab2221715025a28@255",
                16, MPFR_RNDN);
  if (mpfr_cmp (x, y) || i <= 0)
    {
      printf ("Regression test (4) failed! (i=%d - expected 1)\n", i);
      printf ("Ref: 3.0080038f2ac5054e3e71ccbb95f76aaab2221715025a28@255\n"
              "Got: ");
      mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  mpfr_set_prec (x, 908);
  mpfr_set_prec (y, 908);
  mpfr_set_prec (z, 908);
  mpfr_set_str (y, "-f.fffffffffffffffffffffffffffffffffffffffffffffffffffffff"
"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
"ffffffffffffffffffffffffffffffffffffffffffffffffffffff99be91f83ec6f0ed28a3d42"
"e6e9a327230345ea6@-1", 16, MPFR_RNDN);
  mpfr_set_str (z, "-f.fffffffffffffffffffffffffffffffffffffffffffffffffffffff"
"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
"ffffffffffffffffffffffffffffffffffffffffffffffffffffff99be91f83ec6f0ed28a3d42"
                "e6e9a327230345ea6@-1", 16, MPFR_RNDN);
  i = mpfr_mul (x, y, z, MPFR_RNDU);
  mpfr_set_str (y, "f.ffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
"fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
"fffffffffffffffffffffffffffffffffffffffffffffffffffff337d23f07d8de1da5147a85c"
"dd3464e46068bd4d@-1", 16, MPFR_RNDN);
  if (mpfr_cmp (x, y) || i <= 0)
    {
      printf ("Regression test (5) failed! (i=%d - expected 1)\n", i);
      mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }


  mpfr_set_prec (x, 50);
  mpfr_set_prec (y, 40);
  mpfr_set_prec (z, 53);
  mpfr_set_str (y, "4.1ffffffff8", 16, MPFR_RNDN);
  mpfr_set_str (z, "4.2000000ffe0000@-4", 16, MPFR_RNDN);
  i = mpfr_mul (x, y, z, MPFR_RNDN);
  if (mpfr_cmp_str (x, "1.104000041d6c0@-3", 16, MPFR_RNDN) != 0
      || i <= 0)
    {
      printf ("Regression test (6) failed! (i=%d - expected 1)\nx=", i);
      mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
      printf ("\nMore prec=");
      mpfr_set_prec (x, 93);
      mpfr_mul (x, y, z, MPFR_RNDN);
      mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  mpfr_set_prec (x, 439);
  mpfr_set_prec (y, 393);
  mpfr_set_str (y, "-1.921fb54442d18469898cc51701b839a252049c1114cf98e804177d"
                "4c76273644a29410f31c6809bbdf2a33679a748636600",
                16, MPFR_RNDN);
  /* the following call to mpfr_mul with identical arguments is intentional */
  i = mpfr_mul (x, y, y, MPFR_RNDU);
  if (mpfr_cmp_str (x, "2.77a79937c8bbcb495b89b36602306b1c2159a8ff834288a19a08"
    "84094f1cda3dc426da61174c4544a173de83c2500f8bfea2e0569e3698",
                    16, MPFR_RNDN) != 0
      || i <= 0)
    {
      printf ("Regression test (7) failed! (i=%d - expected 1)\nx=", i);
      mpfr_out_str (stdout, 16, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  mpfr_set_prec (x, 1023);
  mpfr_set_prec (y, 1023);
  mpfr_set_prec (z, 511);
  mpfr_set_ui (x, 17, MPFR_RNDN);
  mpfr_set_ui (y, 42, MPFR_RNDN);
  i = mpfr_mul (z, x, y, MPFR_RNDN);
  if (mpfr_cmp_ui (z, 17*42) != 0 || i != 0)
    {
      printf ("Regression test (8) failed! (i=%d - expected 0)\nz=", i);
      mpfr_out_str (stdout, 16, 0, z, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }

  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

#define TEST_FUNCTION test_mul
#define TWO_ARGS
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), randlimb () % 100, RANDS)
#include "tgeneric.c"

/* multiplies x by 53-bit approximation of Pi */
static int
mpfr_mulpi (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t r)
{
  mpfr_t z;
  int inex;

  mpfr_init2 (z, 53);
  mpfr_set_str_binary (z, "11.001001000011111101101010100010001000010110100011");
  inex = mpfr_mul (y, x, z, r);
  mpfr_clear (z);
  return inex;
}

static void
valgrind20110503 (void)
{
  mpfr_t a, b, c;

  mpfr_init2 (a, 2);
  mpfr_init2 (b, 2005);
  mpfr_init2 (c, 2);

  mpfr_set_ui (b, 5, MPFR_RNDN);
  mpfr_nextabove (b);
  mpfr_set_ui (c, 1, MPFR_RNDN);
  mpfr_mul (a, b, c, MPFR_RNDZ);
  /* After the call to mpfr_mulhigh_n, valgrind complains:
     Conditional jump or move depends on uninitialised value(s) */

  mpfr_clears (a, b, c, (mpfr_ptr) 0);
}

static void
testall_rndf (mpfr_prec_t pmax)
{
  mpfr_t a, b, c, d;
  mpfr_prec_t pa, pb, pc;

  for (pa = MPFR_PREC_MIN; pa <= pmax; pa++)
    {
      mpfr_init2 (a, pa);
      mpfr_init2 (d, pa);
      for (pb = MPFR_PREC_MIN; pb <= pmax; pb++)
        {
          mpfr_init2 (b, pb);
          mpfr_set_ui (b, 1, MPFR_RNDN);
          while (mpfr_cmp_ui (b, 2) < 0)
            {
              for (pc = MPFR_PREC_MIN; pc <= pmax; pc++)
                {
                  mpfr_init2 (c, pc);
                  mpfr_set_ui (c, 1, MPFR_RNDN);
                  while (mpfr_cmp_ui (c, 2) < 0)
                    {
                      mpfr_mul (a, b, c, MPFR_RNDF);
                      mpfr_mul (d, b, c, MPFR_RNDD);
                      if (!mpfr_equal_p (a, d))
                        {
                          mpfr_mul (d, b, c, MPFR_RNDU);
                          if (!mpfr_equal_p (a, d))
                            {
                              printf ("Error: mpfr_mul(a,b,c,RNDF) does not "
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
          mpfr_clear (b);
        }
      mpfr_clear (a);
      mpfr_clear (d);
    }
}

/* Check underflow flag corresponds to *after* rounding.
 *
 * More precisely, we want to test mpfr_mul on inputs b and c such that
 * EXP(b*c) < emin but EXP(round(b*c, p, rnd)) = emin. Thus an underflow
 * must not be generated.
 */
static void
test_underflow (mpfr_prec_t pmax)
{
  mpfr_exp_t emin;
  mpfr_prec_t p;
  mpfr_t a0, a, b, c;
  int inex;

  mpfr_init2 (a0, MPFR_PREC_MIN);
  emin = mpfr_get_emin ();
  mpfr_setmin (a0, emin);  /* 0.5 * 2^emin */

  /* for RNDN, we want b*c < 0.5 * 2^emin but RNDN(b*c, p) = 0.5 * 2^emin,
     thus b*c >= (0.5 - 1/4 * ulp_p(0.5)) * 2^emin */
  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    {
      mpfr_init2 (a, p + 1);
      mpfr_init2 (b, p + 10);
      mpfr_init2 (c, p + 10);
      do mpfr_urandomb (b, RANDS); while (MPFR_IS_ZERO (b));
      inex = mpfr_set_ui_2exp (a, 1, -1, MPFR_RNDZ); /* a = 0.5 */
      MPFR_ASSERTN (inex == 0);
      mpfr_nextbelow (a); /* 0.5 - 1/2*ulp_{p+1}(0.5) = 0.5 - 1/4*ulp_p(0.5) */
      inex = mpfr_div (c, a, b, MPFR_RNDU);
      /* 0.5 - 1/4 * ulp_p(0.5) = a <= b*c < 0.5 */
      mpfr_mul_2si (b, b, emin / 2, MPFR_RNDZ);
      mpfr_mul_2si (c, c, (emin - 1) / 2, MPFR_RNDZ);
      /* now (0.5 - 1/4 * ulp_p(0.5)) * 2^emin <= b*c < 0.5 * 2^emin,
         thus b*c should be rounded to 0.5 * 2^emin */
      mpfr_set_prec (a, p);
      mpfr_clear_underflow ();
      mpfr_mul (a, b, c, MPFR_RNDN);
      if (mpfr_underflow_p () || ! mpfr_equal_p (a, a0))
        {
          printf ("Error, b*c incorrect or underflow flag incorrectly set"
                  " for emin=%" MPFR_EXP_FSPEC "d, rnd=%s\n",
                  (mpfr_eexp_t) emin, mpfr_print_rnd_mode (MPFR_RNDN));
          printf ("b="); mpfr_dump (b);
          printf ("c="); mpfr_dump (c);
          printf ("a="); mpfr_dump (a);
          mpfr_set_prec (a, mpfr_get_prec (b) + mpfr_get_prec (c));
          mpfr_mul_2ui (b, b, 1, MPFR_RNDN);
          inex = mpfr_mul (a, b, c, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
          printf ("Exact 2*a="); mpfr_dump (a);
          exit (1);
        }
      mpfr_clear (a);
      mpfr_clear (b);
      mpfr_clear (c);
    }

  /* for RNDU, we want b*c < 0.5*2^emin but RNDU(b*c, p) = 0.5*2^emin thus
     b*c > (0.5 - 1/2 * ulp_p(0.5)) * 2^emin */
  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    {
      mpfr_init2 (a, p);
      mpfr_init2 (b, p + 10);
      mpfr_init2 (c, p + 10);
      do mpfr_urandomb (b, RANDS); while (MPFR_IS_ZERO (b));
      inex = mpfr_set_ui_2exp (a, 1, -1, MPFR_RNDZ); /* a = 0.5 */
      MPFR_ASSERTN (inex == 0);
      mpfr_nextbelow (a); /* 0.5 - 1/2 * ulp_p(0.5) */
      inex = mpfr_div (c, a, b, MPFR_RNDU);
      /* 0.5 - 1/2 * ulp_p(0.5) <= b*c < 0.5 */
      mpfr_mul_2si (b, b, emin / 2, MPFR_RNDZ);
      mpfr_mul_2si (c, c, (emin - 1) / 2, MPFR_RNDZ);
      if (inex == 0)
        mpfr_nextabove (c); /* ensures b*c > (0.5 - 1/2 * ulp_p(0.5)) * 2^emin.
                               Warning: for p=1, 0.5 - 1/2 * ulp_p(0.5)
                               = 0.25, thus b*c > 2^(emin-2), which should
                               also be rounded up with p=1 to 0.5 * 2^emin
                               with an unbounded exponent range. */
      mpfr_clear_underflow ();
      mpfr_mul (a, b, c, MPFR_RNDU);
      if (mpfr_underflow_p () || ! mpfr_equal_p (a, a0))
        {
          printf ("Error, b*c incorrect or underflow flag incorrectly set"
                  " for emin=%" MPFR_EXP_FSPEC "d, rnd=%s\n",
                  (mpfr_eexp_t) emin, mpfr_print_rnd_mode (MPFR_RNDU));
          printf ("b="); mpfr_dump (b);
          printf ("c="); mpfr_dump (c);
          printf ("a="); mpfr_dump (a);
          mpfr_set_prec (a, mpfr_get_prec (b) + mpfr_get_prec (c));
          mpfr_mul_2ui (b, b, 1, MPFR_RNDN);
          inex = mpfr_mul (a, b, c, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
          printf ("Exact 2*a="); mpfr_dump (a);
          exit (1);
        }
      mpfr_clear (a);
      mpfr_clear (b);
      mpfr_clear (c);
    }

  mpfr_clear (a0);
}

/* checks special case where no underflow should occur */
static void
bug20161209 (void)
{
  mpfr_exp_t emin;
  mpfr_t x, y, z;

  emin = mpfr_get_emin ();
  set_emin (-1);

  /* test for mpfr_mul_1 for 64-bit limb, mpfr_mul_2 for 32-bit limb */
  mpfr_init2 (x, 53);
  mpfr_init2 (y, 53);
  mpfr_init2 (z, 53);
  mpfr_set_str_binary (x, "0.101000001E-1"); /* x = 321/2^10 */
  mpfr_set_str_binary (y, "0.110011000010100101111000011011000111011000001E-1");
  /* y = 28059810762433/2^46 */
  /* x * y = (2^53+1)/2^56 = 0.001...000[1]000..., and should round to 0.25 */
  mpfr_mul (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (z, 1, -2) == 0);

  /* test for mpfr_mul_2 for 64-bit limb */
  mpfr_set_prec (x, 65);
  mpfr_set_prec (y, 65);
  mpfr_set_prec (z, 65);
  mpfr_set_str_binary (x, "0.101101000010010110100001E-1"); /* 11806113/2^25 */
  mpfr_set_str_binary (y, "0.101101011110010101011010001111111001100001E-1");
  /* y = 3124947910241/2^43 */
  /* x * y = (2^65+1)/2^68 = 0.001...000[1]000..., and should round to 0.25 */
  mpfr_mul (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (z, 1, -2) == 0);

  /* test for the generic code */
  mpfr_set_prec (x, 54);
  mpfr_set_prec (y, 55);
  mpfr_set_prec (z, 53);
  mpfr_set_str_binary (x, "0.101000001E-1");
  mpfr_set_str_binary (y, "0.110011000010100101111000011011000111011000001E-1");
  mpfr_mul (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (z, 1, -2) == 0);

  /* another test for the generic code */
  mpfr_set_prec (x, 66);
  mpfr_set_prec (y, 67);
  mpfr_set_prec (z, 65);
  mpfr_set_str_binary (x, "0.101101000010010110100001E-1");
  mpfr_set_str_binary (y, "0.101101011110010101011010001111111001100001E-1");
  mpfr_mul (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (z, 1, -2) == 0);

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  set_emin (emin);
}

/* test for case in mpfr_mul_1() where:
   ax = __gmpfr_emin - 1
   ap[0] == ~mask
   rnd_mode = MPFR_RNDZ.
   Whatever the values of rb and sb, we should round to zero (underflow). */
static void
bug20161209a (void)
{
  mpfr_exp_t emin;
  mpfr_t x, y, z;

  emin = mpfr_get_emin ();
  set_emin (-1);

  mpfr_init2 (x, 53);
  mpfr_init2 (y, 53);
  mpfr_init2 (z, 53);

  /* case rb = sb = 0 */
  mpfr_set_str_binary (x, "0.11010010100110000110110011111E-1");
  mpfr_set_str_binary (y, "0.1001101110011000110100001");
  /* x = 441650591/2^30, y = 20394401/2^25 */
  /* x * y = (2^53-1)/2^55 = 0.00111...111[0]000..., and should round to 0 */
  mpfr_mul (z, x, y, MPFR_RNDZ);
  MPFR_ASSERTN(mpfr_zero_p (z));

  /* case rb = 1, sb = 0 */
  mpfr_set_str_binary (x, "0.111111111000000000000000000111111111E-1");
  mpfr_set_str_binary (y, "0.1000000001000000001");
  /* x = 68585259519/2^37, y = 262657/2^19 */
  /* x * y = (2^54-1)/2^56 = 0.00111...111[1]000..., and should round to 0 */
  mpfr_mul (z, x, y, MPFR_RNDZ);
  MPFR_ASSERTN(mpfr_zero_p (z));

  /* case rb = 0, sb = 1 */
  mpfr_set_str_binary (x, "0.110010011001011110001100100001000001E-1");
  mpfr_set_str_binary (y, "0.10100010100010111101");
  /* x = 541144371852^37, y = 665789/2^20 */
  /* x * y = (2^55-3)/2^57 = 0.00111...111[0]100..., and should round to 0 */
  mpfr_mul (z, x, y, MPFR_RNDZ);
  MPFR_ASSERTN(mpfr_zero_p (z));

  /* case rb = sb = 1 */
  mpfr_set_str_binary (x, "0.10100110001001001010001111110010100111E-1");
  mpfr_set_str_binary (y, "0.110001010011101001");
  /* x = 178394823847/2^39, y = 201961/2^18 */
  /* x * y = (2^55-1)/2^57 = 0.00111...111[1]100..., and should round to 0 */
  mpfr_mul (z, x, y, MPFR_RNDZ);
  MPFR_ASSERTN(mpfr_zero_p (z));

  /* similar test for mpfr_mul_2 (we only check rb = sb = 1 here) */
  mpfr_set_prec (x, 65);
  mpfr_set_prec (y, 65);
  mpfr_set_prec (z, 65);
  /* 2^67-1 = 193707721 * 761838257287 */
  mpfr_set_str_binary (x, "0.1011100010111011111011001001E-1");
  mpfr_set_str_binary (y, "0.1011000101100001000110010100010010000111");
  mpfr_mul (z, x, y, MPFR_RNDZ);
  MPFR_ASSERTN(mpfr_zero_p (z));

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  set_emin (emin);
}

/* bug for RNDF */
static void
bug20170602 (void)
{
  mpfr_t x, u, y, yd, yu;

  mpfr_init2 (x, 493);
  mpfr_init2 (u, 493);
  mpfr_init2 (y, 503);
  mpfr_init2 (yd, 503);
  mpfr_init2 (yu, 503);
  mpfr_set_str_binary (x, "0.1111100000000000001111111110000000001111111111111000000000000000000011111111111111111111111000000000000000000001111111111111111111111111111111111111111000000000011111111111111111111000000000011111111111111000000000000001110000000000000000000000000000000000000000011111111111110011111111111100000000000000011111111111111111110000000011111111111111111110011111111111110000000000001111111111111111000000000000000000000000000000000000111111111111111111111111111111011111111111111111111111111111100E44");
  mpfr_set_str_binary (u, "0.1110000000000000001111111111111111111111111111111111111000000000111111111111111111111111111111000000000000000000001111111000000000000000011111111111111111111111111111111111111111111111111111111000000000000000011111111111111000000011111111111111111110000000000000001111111111111111111111111111111111111110000000000001111111111111111111111111111111111111000000000000000000000000000000000001111111111111000000000000000000001111111111100000000000000011111111111111111111111111111111111111111111111E35");
  mpfr_mul (y, x, u, MPFR_RNDF);
  mpfr_mul (yd, x, u, MPFR_RNDD);
  mpfr_mul (yu, x, u, MPFR_RNDU);
  if (mpfr_cmp (y, yd) != 0 && mpfr_cmp (y, yu) != 0)
    {
      printf ("RNDF is neither RNDD nor RNDU\n");
      printf ("x"); mpfr_dump (x);
      printf ("u"); mpfr_dump (u);
      printf ("y(RNDF)="); mpfr_dump (y);
      printf ("y(RNDD)="); mpfr_dump (yd);
      printf ("y(RNDU)="); mpfr_dump (yu);
      exit (1);
    }
  mpfr_clear (x);
  mpfr_clear (u);
  mpfr_clear (y);
  mpfr_clear (yd);
  mpfr_clear (yu);
}

/* Test for 1 to 3 limbs. */
static void
small_prec (void)
{
  mpfr_exp_t emin, emax;
  mpfr_t x, y, z1, z2, zz;
  int xq, yq, zq;
  mpfr_rnd_t rnd;
  mpfr_flags_t flags1, flags2;
  int inex1, inex2;
  int i, j, r, s, ediff;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  /* The mpfr_mul implementation doesn't extend the exponent range,
     so that it is OK to reduce it here for the test to make sure that
     mpfr_mul_2si can be used. */
  set_emin (-1000);
  set_emax (1000);

  mpfr_inits2 (3 * GMP_NUMB_BITS, x, y, z1, z2, (mpfr_ptr) 0);
  mpfr_init2 (zz, 6 * GMP_NUMB_BITS);
  for (i = 0; i < 3; i++)
    for (j = 0; j < 10000; j++)
      {
        xq = i * GMP_NUMB_BITS + 1 + randlimb () % GMP_NUMB_BITS;
        mpfr_set_prec (x, xq);
        yq = i * GMP_NUMB_BITS + 1 + randlimb () % GMP_NUMB_BITS;
        mpfr_set_prec (y, yq);
        zq = i * GMP_NUMB_BITS + 1 + randlimb () % (GMP_NUMB_BITS-1);
        mpfr_set_prec (z1, zq);
        mpfr_set_prec (z2, zq);
        s = r = randlimb () & 0x7f;
        do mpfr_urandomb (x, RANDS); while (mpfr_zero_p (x));
        if (s & 1)
          mpfr_neg (x, x, MPFR_RNDN);
        s >>= 1;
        if (s & 1)
          {
            do mpfr_urandomb (y, RANDS); while (mpfr_zero_p (y));
          }
        else
          {
            mpfr_ui_div (y, 1, x, MPFR_RNDN);
            mpfr_set_exp (y, 0);
          }
        s >>= 1;
        if (s & 1)
          mpfr_neg (y, y, MPFR_RNDN);
        s >>= 1;
        rnd = RND_RAND_NO_RNDF ();
        inex1 = mpfr_mul (zz, x, y, MPFR_RNDN);
        MPFR_ASSERTN (inex1 == 0);
        if (s == 0)
          {
            ediff = __gmpfr_emin - MPFR_EXP (x);
            mpfr_set_exp (x, __gmpfr_emin);
          }
        else if (s == 1)
          {
            ediff = __gmpfr_emax - MPFR_EXP (x) + 1;
            mpfr_set_exp (x, __gmpfr_emax);
            mpfr_mul_2ui (y, y, 1, MPFR_RNDN);
          }
        else
          ediff = 0;
        mpfr_clear_flags ();
        inex1 = mpfr_mul_2si (z1, zz, ediff, rnd);
        flags1 = __gmpfr_flags;
        mpfr_clear_flags ();
        inex2 = mpfr_mul (z2, x, y, rnd);
        flags2 = __gmpfr_flags;
        if (!(mpfr_equal_p (z1, z2) &&
              SAME_SIGN (inex1, inex2)
              && flags1 == flags2
              ))
          {
            printf ("Error in small_prec on i = %d, j = %d\n", i, j);
            printf ("r = 0x%x, xq = %d, yq = %d, zq = %d, rnd = %s\n",
                    r, xq, yq, zq, mpfr_print_rnd_mode (rnd));
            printf ("x = ");
            mpfr_dump (x);
            printf ("y = ");
            mpfr_dump (y);
            printf ("ediff = %d\n", ediff);
            printf ("zz = ");
            mpfr_dump (zz);
            printf ("Expected ");
            mpfr_dump (z1);
            printf ("with inex = %d and flags =", inex1);
            flags_out (flags1);
            printf ("Got      ");
            mpfr_dump (z2);
            printf ("with inex = %d and flags =", inex2);
            flags_out (flags2);
            exit (1);
          }
      }
  mpfr_clears (x, y, z1, z2, zz, (mpfr_ptr) 0);

  set_emin (emin);
  set_emax (emax);
}

/* check ax < __gmpfr_emin with rnd_mode == MPFR_RNDN, rb = 0 and sb <> 0 */
static void
test_underflow2 (void)
{
  mpfr_t x, y, z;
  mpfr_exp_t emin;

  emin = mpfr_get_emin ();
  set_emin (0);

  mpfr_init2 (x, 24);
  mpfr_init2 (y, 24);
  mpfr_init2 (z, 24);

  mpfr_set_ui_2exp (x, 12913, -14, MPFR_RNDN);
  mpfr_set_ui_2exp (y, 5197,  -13, MPFR_RNDN);
  mpfr_clear_underflow ();
  /* x*y = 0.0111111111111111111111111[01] thus underflow */
  mpfr_mul (z, y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (z, 1, -1) == 0);
  MPFR_ASSERTN(mpfr_underflow_p ());

  mpfr_set_prec (x, 65);
  mpfr_set_prec (y, 65);
  mpfr_set_prec (z, 65);
  mpfr_set_str_binary (x, "0.10011101000110111011111100101001111");
  mpfr_set_str_binary (y, "0.110100001001000111000011011110011");
  mpfr_clear_underflow ();
  /* x*y = 0.011111111111111111111111111111111111111111111111111111111111111111[01] thus underflow */
  mpfr_mul (z, y, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (z, 1, -1) == 0);
  MPFR_ASSERTN(mpfr_underflow_p ());

  mpfr_clear (y);
  mpfr_clear (x);
  mpfr_clear (z);

  set_emin (emin);
}

static void
coverage (mpfr_prec_t pmax)
{
  mpfr_t a, b, c;
  mpfr_prec_t p;
  int inex;

  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    {
      mpfr_init2 (a, p);
      mpfr_init2 (b, p);
      mpfr_init2 (c, p);

      /* exercise case b*c = 2^(emin-2), which is just in the middle
         between 0 and the smallest positive number 0.5*2^emin */
      mpfr_set_ui_2exp (b, 1, mpfr_get_emin (), MPFR_RNDN);
      mpfr_set_ui_2exp (c, 1, -2, MPFR_RNDN);
      mpfr_clear_flags ();
      inex = mpfr_mul (a, b, c, MPFR_RNDN);
      MPFR_ASSERTN(inex < 0);
      MPFR_ASSERTN(mpfr_zero_p (a) && mpfr_signbit (a) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

      if (p == 1)
        goto end_of_loop;

      /* case b*c > 2^(emin-2): b = (1-2^(-p))*2^emin,
         c = 0.25*(1+2^(1-p)), thus b*c = (1+2^(-p)-2^(1-2p))*2^(emin-2)
         should be rounded to 2^(emin-1) for RNDN */
      mpfr_nextbelow (b);
      mpfr_nextabove (c);
      mpfr_clear_flags ();
      inex = mpfr_mul (a, b, c, MPFR_RNDN);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, mpfr_get_emin () - 1) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

      /* b = (1-2^(1-p))*2^emin, c = 0.25*(1+2^(1-p)),
         thus b*c = (1-2^(2-2p))*2^(emin-2) should be rounded to 0 */
      mpfr_nextbelow (b);
      mpfr_clear_flags ();
      inex = mpfr_mul (a, b, c, MPFR_RNDN);
      MPFR_ASSERTN(inex < 0);
      MPFR_ASSERTN(mpfr_zero_p (a) && mpfr_signbit (a) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

      /* special case where b*c is in [nextbelow(0.5*2^emin),0.5*2^emin[ */
      if ((p % 2) == 0)
        {
          /* the middle of the interval [nextbelow(0.5*2^emin),0.5*2^emin[
             is (1-2^(-p-1))*2^(emin-1)
             = (1-2^(-p/2))*(1+2^(-p/2))*2^(emin-1) */
          mpfr_set_si_2exp (b, -1, -p/2, MPFR_RNDN);
          mpfr_add_ui (b, b, 1, MPFR_RNDN);
          mpfr_set_si_2exp (c, 1, -p/2, MPFR_RNDN);
          mpfr_add_ui (c, c, 1, MPFR_RNDN);
          MPFR_ASSERTN(mpfr_get_emin () < 0);
          mpfr_mul_2si (b, b, (mpfr_get_emin () - 1) / 2, MPFR_RNDN);
          mpfr_mul_2si (c, c, (mpfr_get_emin () - 2) / 2, MPFR_RNDN);
          mpfr_clear_flags ();
          inex = mpfr_mul (a, b, c, MPFR_RNDN);
          MPFR_ASSERTN(inex > 0);
          MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, mpfr_get_emin () - 1) == 0);
          MPFR_ASSERTN(mpfr_underflow_p ());
          mpfr_clear_flags ();
          inex = mpfr_mul (a, b, c, MPFR_RNDU);
          MPFR_ASSERTN(inex > 0);
          MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, mpfr_get_emin () - 1) == 0);
          MPFR_ASSERTN(mpfr_underflow_p ());
          mpfr_clear_flags ();
          inex = mpfr_mul (a, b, c, MPFR_RNDD);
          MPFR_ASSERTN(inex < 0);
          MPFR_ASSERTN(mpfr_zero_p (a) && mpfr_signbit (a) == 0);
          MPFR_ASSERTN(mpfr_underflow_p ());
        }
      else /* p is odd:
              b = (1-2^(-(p+1)/2))*2^...
              c = (1+2^(-(p+1)/2))*2^... */
        {
          mpfr_set_si_2exp (b, -1, -(p+1)/2, MPFR_RNDN);
          mpfr_add_ui (b, b, 1, MPFR_RNDN);
          mpfr_set_si_2exp (c, 1, -(p+1)/2, MPFR_RNDN);
          mpfr_add_ui (c, c, 1, MPFR_RNDN);
          MPFR_ASSERTN(mpfr_get_emin () < 0);
          mpfr_mul_2si (b, b, (mpfr_get_emin () - 1) / 2, MPFR_RNDN);
          mpfr_mul_2si (c, c, (mpfr_get_emin () - 2) / 2, MPFR_RNDN);
          mpfr_clear_flags ();
          inex = mpfr_mul (a, b, c, MPFR_RNDN);
          MPFR_ASSERTN(inex > 0);
          MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, mpfr_get_emin () - 1) == 0);
          MPFR_ASSERTN(!mpfr_underflow_p ());
          mpfr_clear_flags ();
          inex = mpfr_mul (a, b, c, MPFR_RNDU);
          MPFR_ASSERTN(inex > 0);
          MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, mpfr_get_emin () - 1) == 0);
          MPFR_ASSERTN(!mpfr_underflow_p ());
          mpfr_clear_flags ();
          inex = mpfr_mul (a, b, c, MPFR_RNDD);
          MPFR_ASSERTN(inex < 0);
          MPFR_ASSERTN(mpfr_zero_p (a) && mpfr_signbit (a) == 0);
          MPFR_ASSERTN(mpfr_underflow_p ());
        }

      if (p <= 2) /* for p=2, 1+2^(-ceil((p+1)/2)) = 1 + 2^(-2) is not
                     exactly representable */
        goto end_of_loop;

      /* b = 1-2^(-ceil((p+1)/2))
         c = 1+2^(-ceil((p+1)/2))
         For p odd, b*c = 1-2^(p+1) should round to 1;
         for p even, b*c = 1-2^(p+2) should round to 1 too. */
      mpfr_set_si_2exp (b, -1, -(p+2)/2, MPFR_RNDN);
      mpfr_add_ui (b, b, 1, MPFR_RNDN);
      mpfr_set_si_2exp (c, 1, -(p+2)/2, MPFR_RNDN);
      mpfr_add_ui (c, c, 1, MPFR_RNDN);
      inex = mpfr_mul (a, b, c, MPFR_RNDN);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);
      /* For RNDU, b*c should round to 1 */
      inex = mpfr_mul (a, b, c, MPFR_RNDU);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);
      /* For RNDD, b*c should round to 1-2^(-p) */
      inex = mpfr_mul (a, b, c, MPFR_RNDD);
      MPFR_ASSERTN(inex < 0);
      mpfr_nextabove (a);
      MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);

      /* same as above, but near emax, to exercise the case where a carry
         produces an overflow */
      mpfr_set_si_2exp (b, -1, -(p+2)/2, MPFR_RNDN);
      mpfr_add_ui (b, b, 1, MPFR_RNDN);
      mpfr_mul_2si (b, b, mpfr_get_emax (), MPFR_RNDN);
      mpfr_set_si_2exp (c, 1, -(p+2)/2, MPFR_RNDN);
      mpfr_add_ui (c, c, 1, MPFR_RNDN);
      /* b*c should round to 2^emax */
      mpfr_clear_flags ();
      inex = mpfr_mul (a, b, c, MPFR_RNDN);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_inf_p (a) && mpfr_sgn (a) > 0);
      MPFR_ASSERTN(mpfr_overflow_p ());
      /* idem for RNDU */
      mpfr_clear_flags ();
      inex = mpfr_mul (a, b, c, MPFR_RNDU);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_inf_p (a) && mpfr_sgn (a) > 0);
      MPFR_ASSERTN(mpfr_overflow_p ());
      /* For RNDD, b*c should round to (1-2^(-p))*2^emax */
      mpfr_clear_flags ();
      inex = mpfr_mul (a, b, c, MPFR_RNDD);
      MPFR_ASSERTN(inex < 0);
      MPFR_ASSERTN(!mpfr_inf_p (a));
      MPFR_ASSERTN(!mpfr_overflow_p ());
      mpfr_nextabove (a);
      MPFR_ASSERTN(mpfr_inf_p (a) && mpfr_sgn (a) > 0);

    end_of_loop:
      mpfr_clear (a);
      mpfr_clear (b);
      mpfr_clear (c);
    }
}

/* check special underflow case for precision = 64 */
static void
coverage2 (void)
{
  mpfr_t a, b, c;
  int inex;
  mpfr_exp_t emin;

  emin = mpfr_get_emin (); /* save emin */
  set_emin (0);

  mpfr_init2 (a, 64);
  mpfr_init2 (b, 64);
  mpfr_init2 (c, 64);

  mpfr_set_str_binary (b, "1111110110100001011100100000100000110110001100100010011010011001E-64"); /* 18276014142440744601/2^64 */
  mpfr_set_str_binary (c, "1000000100110010000111000100010010010001000100101010111101010100E-64"); /* 9309534460545511252/2^64 */
  /* since 1/2-2^-66 < b0*c0 < 1/2, b0*c0 should be rounded to 1/2 */
  inex = mpfr_mul (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, -1) == 0);
  MPFR_ASSERTN(inex > 0);

  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);

  set_emin (emin); /* restore emin */
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  coverage (1024);
  coverage2 ();
  testall_rndf (9);
  check_nans ();
  check_exact ();
  check_float ();

  check53("6.9314718055994530941514e-1", "0.0", MPFR_RNDZ, "0.0");
  check53("0.0", "6.9314718055994530941514e-1", MPFR_RNDZ, "0.0");
  check_sign();
  check53("-4.165000000e4", "-0.00004801920768307322868063274915", MPFR_RNDN,
          "2.0");
  check53("2.71331408349172961467e-08", "-6.72658901114033715233e-165",
          MPFR_RNDZ, "-1.8251348697787782844e-172");
  check53("2.71331408349172961467e-08", "-6.72658901114033715233e-165",
          MPFR_RNDA, "-1.8251348697787786e-172");
  check53("0.31869277231188065", "0.88642843322303122", MPFR_RNDZ,
          "2.8249833483992453642e-1");
  check("8.47622108205396074254e-01", "3.24039313247872939883e-01", MPFR_RNDU,
        28, 45, 2, "0.375");
  check("8.47622108205396074254e-01", "3.24039313247872939883e-01", MPFR_RNDA,
        28, 45, 2, "0.375");
  check("2.63978122803639081440e-01", "6.8378615379333496093e-1", MPFR_RNDN,
        34, 23, 31, "0.180504585267044603");
  check("1.0", "0.11835170935876249132", MPFR_RNDU, 6, 41, 36,
        "0.1183517093595583");
  check53("67108865.0", "134217729.0", MPFR_RNDN, "9.007199456067584e15");
  check("1.37399642157394197284e-01", "2.28877275604219221350e-01", MPFR_RNDN,
        49, 15, 32, "0.0314472340833162888");
  check("4.03160720978664954828e-01", "5.854828e-1"
        /*"5.85483042917246621073e-01"*/, MPFR_RNDZ,
        51, 22, 32, "0.2360436821472831");
  check("3.90798504668055102229e-14", "9.85394674650308388664e-04", MPFR_RNDN,
        46, 22, 12, "0.385027296503914762e-16");
  check("4.58687081072827851358e-01", "2.20543551472118792844e-01", MPFR_RNDN,
        49, 3, 2, "0.09375");
  check_max();
  check_min();
  small_prec ();

  check_regression ();
  test_generic (MPFR_PREC_MIN, 500, 100);

  data_check ("data/mulpi", mpfr_mulpi, "mpfr_mulpi");

  valgrind20110503 ();
  test_underflow (128);
  bug20161209 ();
  bug20161209a ();
  bug20170602 ();
  test_underflow2 ();

  tests_end_mpfr ();
  return 0;
}
