/* Test file for mpfr_get_decimal64 and mpfr_set_decimal64.

Copyright 2006-2025 Free Software Foundation, Inc.
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

/* Needed due to the test on MPFR_WANT_DECIMAL_FLOATS */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef MPFR_WANT_DECIMAL_FLOATS

#include "mpfr-test.h"

#ifndef DEC64_MAX
# define DEC64_MAX 9.999999999999999E384dd
#endif

#if _MPFR_IEEE_FLOATS
static void
print_decimal64 (_Decimal64 d)
{
  union mpfr_ieee_double_extract x;
  union ieee_double_decimal64 y;
  unsigned int Gh, i;

  y.d64 = d;
  x.d = y.d;
  Gh = x.s.exp >> 6;
  printf ("|%d%d%d%d%d%d", x.s.sig, Gh >> 4, (Gh >> 3) & 1,
          (Gh >> 2) & 1, (Gh >> 1) & 1, Gh & 1);
  printf ("%d%d%d%d%d%d", (x.s.exp >> 5) & 1, (x.s.exp >> 4) & 1,
          (x.s.exp >> 3) & 1, (x.s.exp >> 2) & 1, (x.s.exp >> 1) & 1,
          x.s.exp & 1);
  for (i = 20; i > 0; i--)
    printf ("%d", (x.s.manh >> (i - 1)) & 1);
  for (i = 32; i > 0; i--)
    printf ("%d", (x.s.manl >> (i - 1)) & 1);
  printf ("|\n");
}
#else
/* Portable version, assuming long double has at least 55 bits.
   Note: __STDC_WANT_IEC_60559_DFP_EXT__ or __STDC_WANT_DEC_FP__
   might allow to use printf("%.15De\n", d) */
static void
print_decimal64 (_Decimal64 d)
{
  printf ("%.15Le\n", (long double) d);
}
#endif /* _MPFR_IEEE_FLOATS */

#define PRINT_ERR_MISC(V)                                   \
  do                                                        \
    {                                                       \
      printf ("Error in check_misc for %s.\n", V);          \
      printf ("  mpfr_get_decimal64() returned: ");         \
      print_decimal64 (d);                                  \
      printf ("  mpfr_set_decimal64() set x to: ");         \
      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);           \
      printf (" approx.\n    = ");                          \
      mpfr_dump (x);                                        \
      exit (1);                                             \
    }                                                       \
 while (0)

static void
check_misc (void)
{
  mpfr_t  x, y;
  _Decimal64 d;

  mpfr_init2 (x, 123);
  mpfr_init2 (y, 123);

#if !defined(MPFR_ERRDIVZERO)
  mpfr_set_nan (x);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  MPFR_ASSERTN (mpfr_nan_p (x));

  mpfr_set_inf (x, 1);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (! mpfr_inf_p (x) || MPFR_IS_NEG (x))
    PRINT_ERR_MISC ("+Inf");

  mpfr_set_inf (x, -1);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (! mpfr_inf_p (x) || MPFR_IS_POS (x))
    PRINT_ERR_MISC ("-Inf");
#endif

  mpfr_set_ui (x, 0, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (MPFR_NOTZERO (x) || MPFR_IS_NEG (x))
    PRINT_ERR_MISC ("+0");

  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_neg (x, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (MPFR_NOTZERO (x) || MPFR_IS_POS (x))
    PRINT_ERR_MISC ("-0");

  mpfr_set_ui (x, 1, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (mpfr_cmp_ui (x, 1) != 0)
    PRINT_ERR_MISC ("+1");

  mpfr_set_si (x, -1, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (mpfr_cmp_si (x, -1) != 0)
    PRINT_ERR_MISC ("-1");

  mpfr_set_ui (x, 2, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (mpfr_cmp_ui (x, 2) != 0)
    PRINT_ERR_MISC ("2");

  mpfr_set_ui (x, 99, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (mpfr_cmp_ui (x, 99) != 0)
    PRINT_ERR_MISC ("99");

  mpfr_set_str (x, "9999999999999999", 10, MPFR_RNDZ);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDZ);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("9999999999999999");

  /* smallest normal number */
  mpfr_set_str (x, "1E-383", 10, MPFR_RNDU);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDU);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("1E-383");

  /* smallest subnormal number */
  mpfr_set_str (x, "1E-398", 10, MPFR_RNDU);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDU);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("1E-398");

  /* exercise case e < -1323, i.e., x < 0.5*2^(-1323) */
  mpfr_set_ui_2exp (x, 1, -1324, MPFR_RNDN);
  mpfr_nextbelow (x);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  /* d should equal +0 */
  mpfr_set_decimal64 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (x) && mpfr_signbit (x) == 0);
  /* check RNDA */
  mpfr_set_ui_2exp (x, 1, -1324, MPFR_RNDN);
  mpfr_nextbelow (x);
  d = mpfr_get_decimal64 (x, MPFR_RNDA);
  /* d should equal 1E-398 */
  mpfr_set_decimal64 (x, d, MPFR_RNDN);
  mpfr_set_str (y, "1E-398", 10, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (x, y));
  /* check negative number */
  mpfr_set_ui_2exp (x, 1, -1324, MPFR_RNDN);
  mpfr_nextbelow (x);
  mpfr_neg (x, x, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  /* d should equal -0 */
  mpfr_set_decimal64 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (x) && mpfr_signbit (x) == 1);

  /* exercise case e10 < -397 */
  mpfr_set_ui_2exp (x, 1, -1323, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (x) && mpfr_signbit (x) == 0);
  mpfr_set_ui_2exp (x, 1, -1323, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDU);
  mpfr_set_str (y, "1E-398", 10, MPFR_RNDN);
  mpfr_set_decimal64 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (x, y));
  mpfr_set_ui_2exp (x, 1, -1323, MPFR_RNDN);
  /* 2^(-1323) = 5.46154776930125e-399 thus should be rounded to 1E-398 */
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  mpfr_set_str (y, "1E-398", 10, MPFR_RNDN);
  mpfr_set_decimal64 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (x, y));

  /* subnormal number with exponent change when we round back
     from 16 digits to 1 digit */
  mpfr_set_str (x, "9.9E-398", 10, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDU); /* should be 1E-397 */
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal64 (x, d, MPFR_RNDD);
  mpfr_set_str (y, "1E-397", 10, MPFR_RNDN);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("9.9E-398");

  /* largest number */
  mpfr_set_str (x, "9.999999999999999E384", 10, MPFR_RNDZ);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDU);
  if (d == DEC64_MAX)
    {
      mpfr_set_ui (x, 0, MPFR_RNDZ);
      mpfr_set_decimal64 (x, d, MPFR_RNDZ);
      if (! mpfr_equal_p (x, y))
        PRINT_ERR_MISC ("DEC64_MAX");
    }
  else
    {
      printf ("Error in check_misc for DEC64_MAX.\n");
      printf ("  mpfr_get_decimal64() returned: ");
      print_decimal64 (d);
      exit (1);
    }

  mpfr_set_str (x, "-9.999999999999999E384", 10, MPFR_RNDZ);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDA);
  if (d == -DEC64_MAX)
    {
      mpfr_set_ui (x, 0, MPFR_RNDZ);
      mpfr_set_decimal64 (x, d, MPFR_RNDZ);
      if (! mpfr_equal_p (x, y))
        PRINT_ERR_MISC ("-DEC64_MAX");
    }
  else
    {
      printf ("Error in check_misc for -DEC64_MAX.\n");
      printf ("  mpfr_get_decimal64() returned: ");
      print_decimal64 (d);
      exit (1);
    }

  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);

  /* largest number */
  mpfr_set_str (x, "9.999999999999999E384", 10, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDZ);
  mpfr_set_decimal64 (y, d, MPFR_RNDU);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("DEC64_MAX (2)");

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
check_random (void)
{
  mpfr_t  x, y;
  _Decimal64 d;
  int i;

  mpfr_init2 (x, 49);
  mpfr_init2 (y, 49);

  for (i = 0; i < 100000; i++)
    {
      mpfr_urandomb (x, RANDS); /* 0 <= x < 1 */
      /* the normal decimal64 range contains [2^(-1272), 2^1278] */
      mpfr_mul_2si (x, x, (i % 2550) - 1272, MPFR_RNDN);
      if (mpfr_get_exp (x) <= -1272)
        mpfr_mul_2ui (x, x, -1271 - mpfr_get_exp (x), MPFR_RNDN);
      d = mpfr_get_decimal64 (x, MPFR_RNDN);
      mpfr_set_decimal64 (y, d, MPFR_RNDN);
      if (! mpfr_equal_p (x, y))
        {
          printf ("Error:\n");
          printf ("x="); mpfr_dump (x);
          printf ("d="); print_decimal64 (d);
          printf ("y="); mpfr_dump (y);
          exit (1);
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
}

/* check with native decimal formats */
static void
check_native (void)
{
  mpfr_t x;
  _Decimal64 d;

  mpfr_init2 (x, 53);

  /* check important constants are correctly converted */
  mpfr_set_ui (x, 17, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  MPFR_ASSERTN(d == 17.dd);

  mpfr_set_ui (x, 42, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  MPFR_ASSERTN(d == 42.dd);

  mpfr_set_decimal64 (x, 17.dd, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (x, 17) == 0);

  mpfr_set_decimal64 (x, 17.0dd, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (x, 17) == 0);

  mpfr_set_decimal64 (x, 42.dd, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (x, 42) == 0);

  mpfr_set_decimal64 (x, 42.0dd, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui (x, 42) == 0);

  mpfr_clear (x);
}

static void
check_overflow (void)
{
  mpfr_t x;
  int err = 0, neg, rnd;

  mpfr_init2 (x, 96);
  for (neg = 0; neg < 2; neg++)
    RND_LOOP (rnd)
      {
        _Decimal64 d, e;
        mpfr_rnd_t r = (mpfr_rnd_t) rnd;
        int sign = neg ? -1 : 1;

        e = sign * (MPFR_IS_LIKE_RNDZ (r, neg) ? 1 : 2) * DEC64_MAX;
        /* This tests the binary exponent e > 1279 case of get_d64.c */
        mpfr_set_si_2exp (x, sign, 9999, MPFR_RNDN);
        d = mpfr_get_decimal64 (x, r);
        if (d != e)
          {
            printf ("Error 1 in check_overflow for %s, %s\n",
                    neg ? "negative" : "positive",
                    mpfr_print_rnd_mode (r));
            err = 1;
          }
        /* This tests the decimal exponent e > 385 case of get_d64.c */
        mpfr_set_si_2exp (x, sign * 31, 1274, MPFR_RNDN);
        d = mpfr_get_decimal64 (x, r);
        if (d != e)
          {
            printf ("Error 2 in check_overflow for %s, %s\n",
                    neg ? "negative" : "positive",
                    mpfr_print_rnd_mode (r));
            err = 1;
          }
        /* This tests the last else (-382 <= e <= 385) of get_d64.c */
        mpfr_set_decimal64 (x, e, MPFR_RNDA);
        d = mpfr_get_decimal64 (x, r);
        if (d != e)
          {
            printf ("Error 3 in check_overflow for %s, %s\n",
                    neg ? "negative" : "positive",
                    mpfr_print_rnd_mode (r));
            err = 1;
          }
      }
  mpfr_clear (x);
  if (err)
    exit (1);
}

static void
check_tiny (void)
{
  mpfr_t x;
  _Decimal64 d;

  /* If 0.5E-398 < |x| < 1E-398 (smallest subnormal), x should round
     to +/- 1E-398 in MPFR_RNDN. Note: the midpoint 0.5E-398 between
     0 and 1E-398 is not a representable binary number, so that there
     are no tests for it. */
  mpfr_init2 (x, 128);
  mpfr_set_str (x, "1E-398", 10, MPFR_RNDZ);
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  MPFR_ASSERTN (d == 1.0E-398dd);
  mpfr_neg (x, x, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  MPFR_ASSERTN (d == -1.0E-398dd);
  mpfr_set_str (x, "0.5E-398", 10, MPFR_RNDU);
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  MPFR_ASSERTN (d == 1.0E-398dd);
  mpfr_neg (x, x, MPFR_RNDN);
  d = mpfr_get_decimal64 (x, MPFR_RNDN);
  MPFR_ASSERTN (d == -1.0E-398dd);
  mpfr_clear (x);
}

static void
powers_of_10 (void)
{
  mpfr_t x1, x2;
  _Decimal64 d[2];
  int i, rnd;
  unsigned int neg;

  mpfr_inits2 (200, x1, x2, (mpfr_ptr) 0);
  for (i = 0, d[0] = 1, d[1] = 1; i < 150; i++, d[0] *= 10, d[1] /= 10)
    for (neg = 0; neg <= 3; neg++)
      RND_LOOP_NO_RNDF (rnd)
        {
          int inex1, inex2;
          mpfr_flags_t flags1, flags2;
          mpfr_rnd_t rx1;
          _Decimal64 dd;

          inex1 = mpfr_set_si (x1, (neg >> 1) ? -i : i, MPFR_RNDN);
          MPFR_ASSERTN (inex1 == 0);

          rx1 = (neg & 1) ?
            MPFR_INVERT_RND ((mpfr_rnd_t) rnd) : (mpfr_rnd_t) rnd;
          mpfr_clear_flags ();
          inex1 = mpfr_exp10 (x1, x1, rx1);
          flags1 = __gmpfr_flags;

          dd = d[neg >> 1];

          if (neg & 1)
            {
              MPFR_SET_NEG (x1);
              inex1 = -inex1;
              dd = -dd;
            }

          mpfr_clear_flags ();
          inex2 = mpfr_set_decimal64 (x2, dd, (mpfr_rnd_t) rnd);
          flags2 = __gmpfr_flags;

          if (!(mpfr_equal_p (x1, x2) &&
                SAME_SIGN (inex1, inex2) &&
                flags1 == flags2))
            {
              printf ("Error in powers_of_10 for i=%d, neg=%d, %s\n",
                      i, neg, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
              printf ("Expected ");
              mpfr_dump (x1);
              printf ("with inex = %d and flags =", inex1);
              flags_out (flags1);
              printf ("Got      ");
              mpfr_dump (x2);
              printf ("with inex = %d and flags =", inex2);
              flags_out (flags2);
              exit (1);
            }
        }
  mpfr_clears (x1, x2, (mpfr_ptr) 0);
}

static void
noncanonical (void)
{
  /* The code below assumes BID. It also needs _MPFR_IEEE_FLOATS
     due to the use of union mpfr_ieee_double_extract. */
#if _MPFR_IEEE_FLOATS && defined(DECIMAL_BID_FORMAT)
  /* The volatile below avoids _Decimal64 constant propagation, which is
     buggy for non-canonical encoding in various GCC versions on the x86 and
     x86_64 targets: failure with gcc (Debian 20190719-1) 10.0.0 20190718
     (experimental) [trunk revision 273586]; the MPFR test was not failing
     with previous GCC versions, but GCC versions 5 to 9 are also affected
     on the simple testcase at:
     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91226
  */
  volatile _Decimal64 d = 9999999999999999.dd;
  union mpfr_ieee_double_extract x;
  union ieee_double_decimal64 y;

  MPFR_ASSERTN (sizeof (x) == 8);
  MPFR_ASSERTN (sizeof (y) == 8);
  /* test for non-canonical encoding */
  y.d64 = d;
  memcpy (&x, &y, 8);
  /* if BID, we have sig=0, exp=1735, manh=231154, manl=1874919423 */
  if (x.s.sig == 0 && x.s.exp == 1735 && x.s.manh == 231154 &&
      x.s.manl == 1874919423)
    {
      mpfr_t z;
      mpfr_init2 (z, 54); /* 54 bits ensure z is exact, since 10^16 < 2^54 */
      x.s.manl += 1; /* then the significand equals 10^16 */
      memcpy (&y, &x, 8);
      mpfr_set_decimal64 (z, y.d64, MPFR_RNDN);
      if (MPFR_NOTZERO (z) || MPFR_IS_NEG (z))
        {
          int i;
          printf ("Error in noncanonical on");
          for (i = 0; i < 8; i++)
            printf (" %02X", ((unsigned char *)&y)[i]);
          printf ("\nExpected +0, got:\n");
          mpfr_dump (z);
          exit (1);
        }
      mpfr_clear (z);
    }
  else
    printf ("Warning! Unexpected value of x in noncanonical.\n");
#endif
}

/* generate random sequences of 8 bytes and interpret them as _Decimal64 */
static void
check_random_bytes (void)
{
  union {
    _Decimal64 d;
    unsigned char c[8];
  } x;
  int i;
  mpfr_t y;
  _Decimal64 e;

  mpfr_init2 (y, 55); /* 55 = 1 + ceil(16*log(10)/log(2)), thus ensures
                         that if a decimal64 number is converted to a 55-bit
                         value and back, we should get the same value */
  for (i = 0; i < 100000; i++)
    {
      int j;
      for (j = 0; j < 8; j++)
        x.c[j] = randlimb () & 255;
      mpfr_set_decimal64 (y, x.d, MPFR_RNDN);
      e = mpfr_get_decimal64 (y, MPFR_RNDN);
      if (!mpfr_nan_p (y))
        if (x.d != e)
          {
            printf ("check_random_bytes failed\n");
            printf ("x.d="); print_decimal64 (x.d);
            printf ("y="); mpfr_dump (y);
            printf ("e  ="); print_decimal64 (e);
            exit (1);
          }
    }
  mpfr_clear (y);
}

int
main (int argc, char *argv[])
{
  int verbose = argc > 1;

  tests_start_mpfr ();
  mpfr_test_init ();

  if (verbose)
    {
#ifdef DECIMAL_DPD_FORMAT
      printf ("Using DPD encoding\n");
#endif
#ifdef DECIMAL_BID_FORMAT
      printf ("Using BID encoding\n");
#endif
    }

#if !defined(MPFR_ERRDIVZERO)
  check_random_bytes ();
#endif
  noncanonical ();
  check_misc ();
  check_random ();
  check_native ();
#if !defined(MPFR_ERRDIVZERO)
  check_overflow ();
#endif
  check_tiny ();
  powers_of_10 ();

  tests_end_mpfr ();
  return 0;
}

#else /* MPFR_WANT_DECIMAL_FLOATS */

int
main (void)
{
  return 77;
}

#endif /* MPFR_WANT_DECIMAL_FLOATS */
