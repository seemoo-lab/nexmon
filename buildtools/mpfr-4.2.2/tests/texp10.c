/* Test file for mpfr_exp10.

Copyright 2007-2025 Free Software Foundation, Inc.
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

#define TEST_FUNCTION mpfr_exp10
#define TEST_RANDOM_EMIN -36
#define TEST_RANDOM_EMAX 36
#include "tgeneric.c"

static void
special_overflow (void)
{
  mpfr_t x, y;
  int inex;
  mpfr_exp_t emin, emax;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  set_emin (-125);
  set_emax (128);

  mpfr_init2 (x, 24);
  mpfr_init2 (y, 24);

  mpfr_set_str_binary (x, "0.101100100000000000110100E15");
  inex = mpfr_exp10 (y, x, MPFR_RNDN);
  if (!mpfr_inf_p (y) || inex <= 0)
    {
      printf ("Overflow error.\n");
      mpfr_dump (y);
      printf ("inex = %d\n", inex);
      exit (1);
    }

  mpfr_clear (y);
  mpfr_clear (x);
  set_emin (emin);
  set_emax (emax);
}

static void
emax_m_eps (void)
{
  if (mpfr_get_emax () <= LONG_MAX)
    {
      mpfr_t x, y;
      int inex, ov;

      mpfr_init2 (x, sizeof(mpfr_exp_t) * CHAR_BIT * 4);
      mpfr_init2 (y, 8);
      mpfr_set_si (x, mpfr_get_emax (), MPFR_RNDN);

      mpfr_clear_flags ();
      inex = mpfr_exp10 (y, x, MPFR_RNDN);
      ov = mpfr_overflow_p ();
      if (!ov || !mpfr_inf_p (y) || inex <= 0)
        {
          printf ("Overflow error for x = emax, MPFR_RNDN.\n");
          mpfr_dump (y);
          printf ("inex = %d, %soverflow\n", inex, ov ? "" : "no ");
          exit (1);
        }

      mpfr_clear (x);
      mpfr_clear (y);
    }
}

static void
exp_range (void)
{
  mpfr_t x;
  mpfr_exp_t emin;

  emin = mpfr_get_emin ();
  set_emin (3);
  mpfr_init2 (x, 16);
  mpfr_set_ui (x, 4, MPFR_RNDN);
  mpfr_exp10 (x, x, MPFR_RNDN);
  set_emin (emin);
  if (mpfr_nan_p (x) || mpfr_cmp_ui (x, 10000) != 0)
    {
      printf ("Error in mpfr_exp10 for x = 4, with emin = 3\n");
      printf ("Expected 10000, got ");
      mpfr_out_str (stdout, 2, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  mpfr_clear (x);
}

static void
overfl_exp10_0 (void)
{
  mpfr_t x, y;
  int emax, i, inex, rnd, err = 0;
  mpfr_exp_t old_emax;

  old_emax = mpfr_get_emax ();

  mpfr_init2 (x, 8);
  mpfr_init2 (y, 8);

  for (emax = -1; emax <= 0; emax++)
    {
      mpfr_set_ui_2exp (y, 1, emax, MPFR_RNDN);
      mpfr_nextbelow (y);
      set_emax (emax);  /* 1 is not representable. */
      /* and if emax < 0, 1 - eps is not representable either. */
      for (i = -1; i <= 1; i++)
        RND_LOOP (rnd)
          {
            mpfr_set_si_2exp (x, i, -512 * ABS (i), MPFR_RNDN);
            mpfr_clear_flags ();
            inex = mpfr_exp10 (x, x, (mpfr_rnd_t) rnd);
            if ((i >= 0 || emax < 0 || rnd == MPFR_RNDN || rnd == MPFR_RNDU) &&
                ! mpfr_overflow_p ())
              {
                printf ("Error in overfl_exp10_0 (i = %d, rnd = %s):\n"
                        "  The overflow flag is not set.\n",
                        i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                err = 1;
              }
            if (rnd == MPFR_RNDZ || rnd == MPFR_RNDD)
              {
                if (inex >= 0)
                  {
                    printf ("Error in overfl_exp10_0 (i = %d, rnd = %s):\n"
                            "  The inexact value must be negative.\n",
                            i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                    err = 1;
                  }
                if (! mpfr_equal_p (x, y))
                  {
                    printf ("Error in overfl_exp10_0 (i = %d, rnd = %s):\n"
                            "  Got        ", i,
                            mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                    mpfr_dump (x);
                    printf ("  instead of 0.11111111E%d.\n", emax);
                    err = 1;
                  }
              }
            else if (rnd != MPFR_RNDF)
              {
                if (inex <= 0)
                  {
                    printf ("Error in overfl_exp10_0 (i = %d, rnd = %s):\n"
                            "  The inexact value must be positive.\n",
                            i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                    err = 1;
                  }
                if (! (mpfr_inf_p (x) && MPFR_IS_POS (x)))
                  {
                    printf ("Error in overfl_exp10_0 (i = %d, rnd = %s):\n"
                            "  Got        ", i,
                            mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                    mpfr_dump (x);
                    printf ("  instead of +Inf.\n");
                    err = 1;
                  }
              }
          }
      set_emax (old_emax);
    }

  if (err)
    exit (1);
  mpfr_clear (x);
  mpfr_clear (y);
}

/* Bug in mpfr_pow_general found by ofuf_thresholds (on 2023-02-13 for
   a 32-bit exponent, changed on 2023-03-06 for a 64-bit exponent too),
   fixed in commit b62966df913f73f08b3c5252e1d0c702bc20442f.
   With a 32-bit exponent, failure for i=0.
     expected 0.1111E1073741823
     got      @Inf@
     expected flags = inexact (8)
     got flags      = overflow inexact (10)
   With a 64-bit exponent, failure for i=1.
     expected 0.11111111111111111111111E4611686018427387903
     got      @Inf@
     expected flags = inexact (8)
     got flags      = overflow inexact (10)
   Note: ofuf_thresholds was added to the master branch, but for the
   time being, there are issues with these tests.
*/
static void
bug20230213 (void)
{
  const char *s[2] = {
    "0x1.34413504b3ccdbd5dd8p+28",
    "0x1.34413509f79fef2c4e0dd14a7ae0ecfbacdbp+60"
  };
  mpfr_t x1, x2, y1, y2;
  mpfr_prec_t px[2] = { 74, 147 };
  mpfr_prec_t py[2] = { 4, 23 };
  mpfr_exp_t old_emax, emax;
  mpfr_flags_t flags1, flags2;
  int i;

  old_emax = mpfr_get_emax ();

  for (i = 0; i < 2; i++)
    {
      if (i != 0)
        set_emax (MPFR_EMAX_MAX);

      emax = mpfr_get_emax ();

      mpfr_inits2 (px[i], x1, x2, (mpfr_ptr) 0);
      mpfr_inits2 (py[i], y1, y2, (mpfr_ptr) 0);

      mpfr_setmax (y1, emax);
      mpfr_log10 (x1, y1, MPFR_RNDD);
      mpfr_set_str (x2, s[i], 0, MPFR_RNDN);
      /* For i == 0, emax == 2^30, so that the value can be checked.
         For i != 0, check the value for the case emax == 2^62.
         The "0UL" ensures that the shifts are valid. */
      if (i == 0 || (((0UL + MPFR_EMAX_MAX) >> 31) >> 30) == 1)
        {
          /* printf ("Checking x1 for i=%d\n", i); */
          MPFR_ASSERTN (mpfr_equal_p (x1, x2));
        }

      /* Let MAXF be the maximum finite value (y1 above).
         Since x1 < log10(MAXF), one should have exp10(x1) < MAXF, and
         therefore, y2 = RU(exp10(x1)) <= RU(MAXF) = MAXF (no overflow). */
      flags1 = MPFR_FLAGS_INEXACT;
      mpfr_clear_flags ();
      mpfr_exp10 (y2, x1, MPFR_RNDU);
      flags2 = __gmpfr_flags;

      if (! (mpfr_lessequal_p (y2, y1) && flags2 == flags1))
        {
          printf ("Error in bug20230213 for i=%d\n", i);
          printf ("emax = %" MPFR_EXP_FSPEC "d\n", (mpfr_eexp_t) emax);
          printf ("expected "); mpfr_dump (y1);
          printf ("got      "); mpfr_dump (y2);
          printf ("expected flags =");
          flags_out (flags1);
          printf ("got flags      =");
          flags_out (flags2);
          exit (1);
        }

      mpfr_clears (x1, x2, y1, y2, (mpfr_ptr) 0);
    }

  set_emax (old_emax);
}

/* Bug in mpfr_pow_general in precision 1 in the particular case of
   rounding to nearest, z * 2^k = 2^(emin - 2) and real result larger
   than this value; fixed in ff5012b61d5e5fee5156c57b8aa8fc1739c2a771
   (which is simplified in 4f5de980be290687ac1409aa02873e9e0dd1a030);
   initially found by ofuf_thresholds (though the test was incorrect).
   With a 32-bit exponent, failure for i=0.
   With a 64-bit exponent, failure for i=1.
   The result was correct, but the underflow flag was missing.
   Note: ofuf_thresholds was added to the master branch, but for the
   time being, there are issues with these tests.
*/
static void
bug20230427 (void)
{
  const char *s[2] = {
    "-0.1001101000100000100110101000011E29",
    "-0.100110100010000010011010100001001111101111001111111101111001101E61"
  };
  mpfr_t x, y, z, t1, t2;
  mpfr_exp_t old_emin;
  mpfr_flags_t flags, ex_flags;
  int i, inex;

  old_emin = mpfr_get_emin ();

  mpfr_init2 (x, 63);
  mpfr_inits2 (1, y, z, (mpfr_ptr) 0);
  mpfr_inits2 (128, t1, t2, (mpfr_ptr) 0);

  for (i = 0; i < 2; i++)
    {
      if (i == 0)
        {
          /* Basic check: the default emin should be -2^30 (exactly). */
          if (mpfr_get_emin () != -1073741823)
            abort ();
        }
      else
        {
          /* This test assumes that MPFR_EMIN_MIN = -2^62 (exactly).
             The "0UL" ensures that the shifts are valid. */
          if ((((0UL - MPFR_EMIN_MIN) >> 31) >> 30) != 1)
            break;

          set_emin (MPFR_EMIN_MIN);
        }

      mpfr_set_str_binary (x, s[i]);

      /* We will test 10^x rounded to nearest in precision 1.
         Check that 2^(emin - 2) < 10^x < (3/2) * 2^(emin - 2).
         This is approximate, but by outputting the values, one can check
         that one is not too close to the boundaries:
           emin - 2              = -4611686018427387905
           log2(10^x)           ~= -4611686018427387904.598
           emin - 2 + log2(3/2) ~= -4611686018427387904.415
         Thus the result should be the smallest positive number 2^(emin - 1)
         because 10^x is closer to this number than to 0, the midpoint being
         2^(emin - 2). And there should be an underflow in precision 1 because
         the result rounded to nearest in an unbounded exponent range should
         have been 2^(emin - 2), the midpoint being (3/2) * 2^(emin - 2).
      */
      mpfr_set_ui (t1, 10, MPFR_RNDN);
      mpfr_log2 (t2, t1, MPFR_RNDN);
      mpfr_mul (t1, t2, x, MPFR_RNDN);
      inex = mpfr_set_exp_t (t2, mpfr_get_emin () - 2, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      MPFR_ASSERTN (mpfr_greater_p (t1, t2));  /* log2(10^x) > emin - 2 */
      inex = mpfr_sub (t1, t1, t2, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      mpfr_set_ui (t2, 3, MPFR_RNDN);
      mpfr_log2 (t2, t2, MPFR_RNDN);
      mpfr_sub_ui (t2, t2, 1, MPFR_RNDN);  /* log2(3/2) */
      MPFR_ASSERTN (mpfr_less_p (t1, t2));

      mpfr_clear_flags ();
      mpfr_exp10 (y, x, MPFR_RNDN);
      flags = __gmpfr_flags;
      ex_flags = MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT;

      mpfr_setmin (z, mpfr_get_emin ());  /* z = 0.1@emin */
      if (! (mpfr_equal_p (y, z) && flags == ex_flags))
        {
          printf ("Error in bug20230427 for i=%d\n", i);
          printf ("expected "); mpfr_dump (z);
          printf ("got      "); mpfr_dump (y);
          printf ("emin =       %" MPFR_EXP_FSPEC "d\n",
                  (mpfr_eexp_t) mpfr_get_emin ());
          printf ("expected flags =");
          flags_out (ex_flags);
          printf ("got flags      =");
          flags_out (flags);
          exit (1);
        }
    }

  mpfr_clears (x, y, z, t1, t2, (mpfr_ptr) 0);
  set_emin (old_emin);
}

int
main (int argc, char *argv[])
{
  mpfr_t x, y;
  mpfr_exp_t emin, emax;
  int inex, ov;

  tests_start_mpfr ();

  bug20230213 ();
  bug20230427 ();

  special_overflow ();
  emax_m_eps ();
  exp_range ();

  mpfr_init (x);
  mpfr_init (y);

  mpfr_set_ui (x, 4, MPFR_RNDN);
  mpfr_exp10 (y, x, MPFR_RNDN);
  if (mpfr_cmp_ui (y, 10000) != 0)
    {
      printf ("Error for 10^4, MPFR_RNDN\n");
      exit (1);
    }
  mpfr_exp10 (y, x, MPFR_RNDD);
  if (mpfr_cmp_ui (y, 10000) != 0)
    {
      printf ("Error for 10^4, MPFR_RNDD\n");
      exit (1);
    }
  mpfr_exp10 (y, x, MPFR_RNDU);
  if (mpfr_cmp_ui (y, 10000) != 0)
    {
      printf ("Error for 10^4, MPFR_RNDU\n");
      exit (1);
    }

  mpfr_set_prec (x, 10);
  mpfr_set_prec (y, 10);
  /* save emin */
  emin = mpfr_get_emin ();
  set_emin (-11);
  mpfr_set_si (x, -4, MPFR_RNDN);
  mpfr_exp10 (y, x, MPFR_RNDN);
  if (MPFR_NOTZERO (y) || MPFR_IS_NEG (y))
    {
      printf ("Error for emin = -11, x = -4, RNDN\n");
      printf ("Expected +0\n");
      printf ("Got      "); mpfr_dump (y);
      exit (1);
    }
  /* restore emin */
  set_emin (emin);

  /* save emax */
  emax = mpfr_get_emax ();
  set_emax (13);
  mpfr_set_ui (x, 4, MPFR_RNDN);
  mpfr_exp10 (y, x, MPFR_RNDN);
  if (!mpfr_inf_p (y) || mpfr_sgn (y) < 0)
    {
      printf ("Error for emax = 13, x = 4, RNDN\n");
      printf ("Expected +inf\n");
      printf ("Got      "); mpfr_dump (y);
      exit (1);
    }
  /* restore emax */
  set_emax (emax);

  MPFR_SET_INF (x);
  MPFR_SET_POS (x);
  mpfr_exp10 (y, x, MPFR_RNDN);
  if (!MPFR_IS_INF (y))
    {
      printf ("evaluation of function in INF does not return INF\n");
      exit (1);
    }

  MPFR_CHANGE_SIGN (x);
  mpfr_exp10 (y, x, MPFR_RNDN);
  if (!MPFR_IS_ZERO (y))
    {
      printf ("evaluation of function in -INF does not return 0\n");
      exit (1);
    }

  MPFR_SET_NAN (x);
  mpfr_exp10 (y, x, MPFR_RNDN);
  if (!MPFR_IS_NAN (y))
    {
      printf ("evaluation of function in NaN does not return NaN\n");
      exit (1);
    }

  if ((mpfr_uexp_t) 8 << 31 != 0 ||
      mpfr_get_emax () <= (mpfr_uexp_t) 100000 * 100000)
    {
      /* emax <= 10000000000 */
      mpfr_set_prec (x, 40);
      mpfr_set_prec (y, 40);
      mpfr_set_str (x, "3010299957", 10, MPFR_RNDN);
      mpfr_clear_flags ();
      inex = mpfr_exp10 (y, x, MPFR_RNDN);
      ov = mpfr_overflow_p ();
      if (!(MPFR_IS_INF (y) && MPFR_IS_POS (y) && ov))
        {
          printf ("Overflow error for x = 3010299957, MPFR_RNDN.\n");
          mpfr_dump (y);
          printf ("inex = %d, %soverflow\n", inex, ov ? "" : "no ");
          exit (1);
        }
    }

  test_generic (MPFR_PREC_MIN, 100, 100);

  mpfr_clear (x);
  mpfr_clear (y);

  overfl_exp10_0 ();

  data_check ("data/exp10", mpfr_exp10, "mpfr_exp10");
  bad_cases (mpfr_exp10, mpfr_log10, "mpfr_exp10",
             0, -256, 255, 4, 128, 800, 50);

  tests_end_mpfr ();
  return 0;
}
