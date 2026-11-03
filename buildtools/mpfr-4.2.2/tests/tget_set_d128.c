/* Test file for mpfr_set_decimal128 and mpfr_get_decimal128.

Copyright 2018-2025 Free Software Foundation, Inc.
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

#ifndef DEC128_MAX
# define DEC128_MAX 9.999999999999999999999999999999999E6144dl
#endif

static void
print_decimal128 (_Decimal128 d)
{
  if (DOUBLE_ISNAN (d))
    printf ("NaN");
  else if (d > DEC128_MAX)
    printf ("Inf");
  else if (d < -DEC128_MAX)
    printf ("-Inf");
  else if (d == 0)
    {
      printf ("%.1f", (double) d);
    }
  else /* regular number */
    {
      long e = 0;
      while (d < 1.dl)
        {
          d *= 10.dl;
          e --;
        }
      /* now d >= 1 */
      while (d > 10.dl)
        {
          d /= 10.dl;
          e ++;
        }
      /* now 1 <= d < 10 */
      printf ("%.33LfE%ld", (long double) d, e);
    }
  printf ("\n");
}

#define PRINT_ERR_MISC(V)                                   \
  do                                                        \
    {                                                       \
      printf ("Error in check_misc for %s.\n", V);          \
      printf ("  mpfr_get_decimal128() returned: ");        \
      print_decimal128 (d);                                 \
      printf ("  mpfr_set_decimal128() set x to: ");        \
      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);           \
      printf (" approx.\n    = ");                          \
      mpfr_dump (x);                                        \
      exit (1);                                             \
    }                                                       \
 while (0)

static void
test_set (void)
{
  long v[] = { 1, -1, 2147483647, -2147483647 };
  mpfr_t x;
  mpfr_flags_t flags;
  int i, inex;

  mpfr_init2 (x, 53);
  for (i = 0; i < numberof (v); i++)
    {
      mpfr_clear_flags ();
      inex = mpfr_set_decimal128 (x, (_Decimal128) v[i], MPFR_RNDN);
      flags = __gmpfr_flags;
      if (mpfr_cmp_si (x, v[i]) != 0 || inex != 0 || flags != 0)
        {
          printf ("Error in test_set for i=%d\n", i);
          printf ("Expected %ld\n    with inex = 0 and flags =", v[i]);
          flags_out (0);
          printf ("Got      ");
          mpfr_dump (x);
          printf ("    with inex = %d and flags =", inex);
          flags_out (flags);
          exit (1);
        }
    }
  mpfr_clear (x);
}

static void
powers_of_10 (void)
{
  mpfr_t x1, x2;
  _Decimal128 d[2];
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
          _Decimal128 dd;

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
          inex2 = mpfr_set_decimal128 (x2, dd, (mpfr_rnd_t) rnd);
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
check_misc (void)
{
  mpfr_t  x, y;
  _Decimal128 d;

  mpfr_init2 (x, 123);
  mpfr_init2 (y, 123);

#if !defined(MPFR_ERRDIVZERO)
  mpfr_set_nan (x);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  MPFR_ASSERTN (mpfr_nan_p (x));

  mpfr_set_inf (x, 1);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (! mpfr_inf_p (x) || MPFR_IS_NEG (x))
    PRINT_ERR_MISC ("+Inf");

  mpfr_set_inf (x, -1);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (! mpfr_inf_p (x) || MPFR_IS_POS (x))
    PRINT_ERR_MISC ("-Inf");
#endif

  mpfr_set_ui (x, 0, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (MPFR_NOTZERO (x) || MPFR_IS_NEG (x))
    PRINT_ERR_MISC ("+0");

  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_neg (x, x, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 1, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (MPFR_NOTZERO (x) || MPFR_IS_POS (x))
    PRINT_ERR_MISC ("-0");

  mpfr_set_ui (x, 1, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (mpfr_cmp_ui (x, 1) != 0)
    PRINT_ERR_MISC ("+1");

  mpfr_set_si (x, -1, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (mpfr_cmp_si (x, -1) != 0)
    PRINT_ERR_MISC ("-1");

  mpfr_set_ui (x, 2, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (mpfr_cmp_ui (x, 2) != 0)
    PRINT_ERR_MISC ("2");

  mpfr_set_ui (x, 99, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (mpfr_cmp_ui (x, 99) != 0)
    PRINT_ERR_MISC ("99");

  mpfr_set_str (x, "9999999999999999999999999999999999", 10, MPFR_RNDZ);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDZ);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("9999999999999999999999999999999999");

  /* smallest normal number */
  mpfr_set_str (x, "1E-6143", 10, MPFR_RNDU);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDU);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("1E-6143");

  /* smallest subnormal number */
  mpfr_set_str (x, "1E-6176", 10, MPFR_RNDU);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDU);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("1E-6176");

  /* exercise case e < -20517, i.e., x < 0.5*2^(-20517) */
  mpfr_set_ui_2exp (x, 1, -20518, MPFR_RNDN);
  mpfr_nextbelow (x);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  /* d should equal +0 */
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (x) && mpfr_signbit (x) == 0);
  /* check RNDA */
  mpfr_set_ui_2exp (x, 1, -20518, MPFR_RNDN);
  mpfr_nextbelow (x);
  d = mpfr_get_decimal128 (x, MPFR_RNDA);
  /* d should equal 1E-6176 */
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  mpfr_set_str (y, "1E-6176", 10, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (x, y));
  /* check negative number */
  mpfr_set_ui_2exp (x, 1, -20518, MPFR_RNDN);
  mpfr_nextbelow (x);
  mpfr_neg (x, x, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  /* d should equal -0 */
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (x) && mpfr_signbit (x) == 1);

  /* exercise case e10 < -6175 */
  mpfr_set_ui_2exp (x, 1, -20517, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (x) && mpfr_signbit (x) == 0);
  mpfr_set_ui_2exp (x, 1, -20517, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDU);
  mpfr_set_str (y, "1E-6176", 10, MPFR_RNDN);
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (x, y));
  mpfr_set_ui_2exp (x, 1, -20517, MPFR_RNDN);
  /* 2^(-20517) = 5.85570193228610e-6177 thus should be rounded to 1E-6176 */
  d = mpfr_get_decimal128 (x, MPFR_RNDN);
  mpfr_set_str (y, "1E-6176", 10, MPFR_RNDN);
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (x, y));

  /* subnormal number with exponent change when we round back
     from 34 digits to 1 digit */
  mpfr_set_str (x, "9.9E-6176", 10, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDU); /* should be 1E-6175 */
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDU);
  mpfr_set_str (y, "1E-6175", 10, MPFR_RNDN);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("9.9E-6176");

  /* largest number */
  mpfr_set_str (x, "9.999999999999999999999999999999999E6144", 10, MPFR_RNDZ);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDU);
  if (d == DEC128_MAX)
    {
      mpfr_set_ui (x, 0, MPFR_RNDZ);
      mpfr_set_decimal128 (x, d, MPFR_RNDZ);
      if (! mpfr_equal_p (x, y))
        PRINT_ERR_MISC ("DEC128_MAX");
    }
  else
    {
      printf ("Error in check_misc for DEC128_MAX.\n");
      printf ("  mpfr_get_decimal128() returned: ");
      print_decimal128 (d);
      exit (1);
    }

  mpfr_set_str (x, "-9.999999999999999999999999999999999E6144", 10, MPFR_RNDZ);
  mpfr_set (y, x, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDA);
  if (d == -DEC128_MAX)
    {
      mpfr_set_ui (x, 0, MPFR_RNDZ);
      mpfr_set_decimal128 (x, d, MPFR_RNDZ);
      if (! mpfr_equal_p (x, y))
        PRINT_ERR_MISC ("-DEC128_MAX");
    }
  else
    {
      printf ("Error in check_misc for -DEC128_MAX.\n");
      printf ("  mpfr_get_decimal128() returned: ");
      print_decimal128 (d);
      exit (1);
    }

  /* exercise |x| > DEC128_MAX */
  mpfr_set_str (x, "10E6144", 10, MPFR_RNDU);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  MPFR_ASSERTN(d == DEC128_MAX);
  mpfr_set_str (x, "-10E6144", 10, MPFR_RNDU);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  MPFR_ASSERTN(d == -DEC128_MAX);

  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);

  /* largest number */
  mpfr_set_str (x, "9.999999999999999999999999999999999E6144", 10, MPFR_RNDZ);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  mpfr_set_decimal128 (y, d, MPFR_RNDU);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("DEC128_MAX (2)");

  /* since 1+ceil(109*log(2)/log(10)) = 34, the 109-bit value x, when
     converted to a 34-digit decimal d, gives back x when converted back to
     binary */
  mpfr_set_prec (x, 109);
  mpfr_set_prec (y, 109);
  mpfr_set_str (x, "1E1793", 10, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDN);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  mpfr_set_str (y, "1E1793", 10, MPFR_RNDN);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("1E1793");

  mpfr_set_str (x, "2E4095", 10, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDN);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  mpfr_set_str (y, "2E4095", 10, MPFR_RNDN);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("2E4095");

  mpfr_set_str (x, "2E-4096", 10, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDN);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  mpfr_set_str (y, "2E-4096", 10, MPFR_RNDN);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("2E-4096");

  mpfr_set_str (x, "2E-6110", 10, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDN);
  mpfr_set_ui (x, 0, MPFR_RNDZ);
  mpfr_set_decimal128 (x, d, MPFR_RNDN);
  mpfr_set_str (y, "2E-6110", 10, MPFR_RNDN);
  if (! mpfr_equal_p (x, y))
    PRINT_ERR_MISC ("2E-6110");

  /* case where EXP(x) > 20414, thus outside the decimal128 range */
  mpfr_set_ui_2exp (x, 1, 20414, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  MPFR_ASSERTN(d == DEC128_MAX);
  d = mpfr_get_decimal128 (x, MPFR_RNDA);
  MPFR_ASSERTN(d > DEC128_MAX); /* implies d = +Inf */
  mpfr_set_si_2exp (x, -1, 20414, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDZ);
  MPFR_ASSERTN(d == -DEC128_MAX);
  d = mpfr_get_decimal128 (x, MPFR_RNDA);
  MPFR_ASSERTN(d < -DEC128_MAX); /* implies d = -Inf */

  /* case where EXP(x) = 20414, at the limit of the decimal128 range */
  mpfr_set_ui_2exp (x, 3, 20412, MPFR_RNDN); /* 3*2^20412 > 9.999...E6144 */
  d = mpfr_get_decimal128 (x, MPFR_RNDN);
  MPFR_ASSERTN(d > DEC128_MAX); /* implies d = +Inf */
  mpfr_set_si_2exp (x, -3, 20412, MPFR_RNDN);
  d = mpfr_get_decimal128 (x, MPFR_RNDN);
  MPFR_ASSERTN(d < -DEC128_MAX); /* implies d = -Inf */

  {
    unsigned long i;
    for (i = 1; i < 1000; i++)
      {
        mpfr_set_ui_2exp (x, i, 20403, MPFR_RNDN);
        d = mpfr_get_decimal128 (x, MPFR_RNDN);
        mpfr_set_decimal128 (x, d, MPFR_RNDN);
        MPFR_ASSERTN(mpfr_cmp_ui_2exp (x, i, 20403) == 0);
      }
  }

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
noncanonical (void)
{
  /* The code below assumes BID. */
#if HAVE_DECIMAL128_IEEE && defined(DECIMAL_BID_FORMAT)
  union ieee_decimal128 x;

  MPFR_ASSERTN (sizeof (x) == 16);
  /* produce a non-canonical decimal128 with Gh >= 24 */
  x.d128 = 1;
  /* if BID, we have sig=0, comb=49408, t0=t1=t2=0, t3=1 */
  if (x.s.sig == 0 && x.s.comb == 49408 && x.s.t0 == 0 && x.s.t1 == 0 &&
      x.s.t2 == 0 && x.s.t3 == 1)
    {
      /* The volatile below avoids _Decimal128 constant propagation, which is
         buggy for non-canonical encoding in various GCC versions on the x86
         and x86_64 targets: failure in the second test below ("Error 2")
         with gcc (Debian 20190820-1) 10.0.0 20190820 (experimental)
         [trunk revision 274744]. The MPFR test was not failing with previous
         GCC versions, not even with gcc (Debian 20190719-1) 10.0.0 20190718
         (experimental) [trunk revision 273586] (contrary to the similar test
         in tget_set_d64.c). More information at:
         https://gcc.gnu.org/bugzilla/show_bug.cgi?id=91226
      */
      volatile _Decimal128 d = 9999999999999999999999999999999999.dl;
      mpfr_t y;

      x.s.comb = 98560; /* force Gh >= 24 thus a non-canonical number
                           (significand >= 2^113 > 20^34-1) */
      mpfr_init2 (y, 113);
      mpfr_set_decimal128 (y, x.d128, MPFR_RNDN);
      if (MPFR_NOTZERO (y) || MPFR_IS_NEG (y))
        {
          int i;
          printf ("Error 1 in noncanonical on");
          for (i = 0; i < 16; i++)
            printf (" %02X", ((unsigned char *)&x)[i]);
          printf ("\nExpected +0, got:\n");
          mpfr_dump (y);
          exit (1);
        }

      /* now construct a case Gh < 24, but where the significand exceeds
         10^34-1 */
      x.d128 = d;
      /* should give sig=0, comb=49415, t0=11529, t1=3199043520,
         t2=932023907, t3=4294967295 */
      x.s.t3 ++; /* should give 0 */
      x.s.t2 += (x.s.t3 == 0);
      /* now the significand is 10^34 */
      mpfr_set_decimal128 (y, x.d128, MPFR_RNDN);
      if (MPFR_NOTZERO (y) || MPFR_IS_NEG (y))
        {
          int i;
          printf ("Error 2 in noncanonical on");
          for (i = 0; i < 16; i++)
            printf (" %02X", ((unsigned char *)&x)[i]);
          printf ("\nExpected +0, got:\n");
          mpfr_dump (y);
          exit (1);
        }

      mpfr_clear (y);
    }
  else
    printf ("Warning! Unexpected value of x in noncanonical.\n");
#endif
}

/* generate random sequences of 16 bytes and interpret them as _Decimal128 */
static void
check_random_bytes (void)
{
  union {
    _Decimal128 d;
    unsigned char c[16];
  } x;
  int i;
  mpfr_t y;
  _Decimal128 e;

  mpfr_init2 (y, 114); /* 114 = 1 + ceil(34*log(10)/log(2)), thus ensures
                         that if a decimal128 number is converted to a 114-bit
                         value and back, we should get the same value */
  for (i = 0; i < 100000; i++)
    {
      int j;
      for (j = 0; j < 16; j++)
        x.c[j] = randlimb () & 255;
      mpfr_set_decimal128 (y, x.d, MPFR_RNDN);
      e = mpfr_get_decimal128 (y, MPFR_RNDN);
      if (!mpfr_nan_p (y))
        if (x.d != e)
          {
            printf ("check_random_bytes failed\n");
            printf ("x.d="); print_decimal128 (x.d);
            printf ("y="); mpfr_dump (y);
            printf ("e="); print_decimal128 (e);
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
  test_set ();
  powers_of_10 ();
#if !defined(MPFR_ERRDIVZERO)
  check_misc ();
#endif
  noncanonical ();

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
