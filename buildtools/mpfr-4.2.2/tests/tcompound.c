/* Test file for mpfr_compound_si.

Copyright 2021-2025 Free Software Foundation, Inc.
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

#define TEST_FUNCTION mpfr_compound_si
#define INTEGER_TYPE long
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), 1, RANDS)
#define test_generic_ui test_generic_si
#include "tgeneric_ui.c"

/* Special cases from IEEE 754-2019 */
static void
check_ieee754 (void)
{
  mpfr_t x, y;
  long i;
  long t[] = { 0, 1, 2, 3, 17, LONG_MAX-1, LONG_MAX };
  int j;
  mpfr_prec_t prec = 2; /* we need at least 2 so that 3/4 is exact */

  mpfr_init2 (x, prec);
  mpfr_init2 (y, prec);

  /* compound(x,n) = NaN for x < -1, and set invalid exception */
  for (i = 0; i < numberof(t); i++)
    for (j = 0; j < 2; j++)
      {
        const char *s;

        mpfr_clear_nanflag ();
        if (j == 0)
          {
            mpfr_set_si (x, -2, MPFR_RNDN);
            s = "-2";
          }
        else
          {
            mpfr_set_inf (x, -1);
            s = "-Inf";
          }
        mpfr_compound_si (y, x, t[i], MPFR_RNDN);
        if (!mpfr_nan_p (y))
          {
            printf ("Error, compound(%s,%ld) should give NaN\n", s, t[i]);
            printf ("got "); mpfr_dump (y);
            exit (1);
          }
        if (!mpfr_nanflag_p ())
          {
            printf ("Error, compound(%s,%ld) should raise invalid flag\n",
                    s, t[i]);
            exit (1);
          }
      }

  /* compound(x,0) = 1 for x >= -1 or x = NaN */
  for (i = -2; i <= 2; i++)
    {
      if (i == -2)
        mpfr_set_nan (x);
      else if (i == 2)
        mpfr_set_inf (x, 1);
      else
        mpfr_set_si (x, i, MPFR_RNDN);
      mpfr_compound_si (y, x, 0, MPFR_RNDN);
      if (mpfr_cmp_ui (y, 1) != 0)
        {
          printf ("Error, compound(x,0) should give 1 on\nx = ");
          mpfr_dump (x);
          printf ("got "); mpfr_dump (y);
          exit (1);
        }
    }

  /* compound(-1,n) = +Inf for n < 0, and raise divide-by-zero flag */
  mpfr_clear_divby0 ();
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_compound_si (y, x, -1, MPFR_RNDN);
  if (!mpfr_inf_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error, compound(-1,-1) should give +Inf\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }
  if (!mpfr_divby0_p ())
    {
      printf ("Error, compound(-1,-1) should raise divide-by-zero flag\n");
      exit (1);
    }

  /* compound(-1,n) = +0 for n > 0 */
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_compound_si (y, x, 1, MPFR_RNDN);
  if (!mpfr_zero_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error, compound(-1,1) should give +0\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }

  /* compound(+/-0,n) = 1 */
  for (i = -1; i <= 1; i++)
    {
      mpfr_set_zero (x, -1);
      mpfr_compound_si (y, x, i, MPFR_RNDN);
      if (mpfr_cmp_ui (y, 1) != 0)
        {
          printf ("Error1, compound(x,%ld) should give 1\non x = ", i);
          mpfr_dump (x);
          printf ("got "); mpfr_dump (y);
          exit (1);
        }
      mpfr_set_zero (x, +1);
      mpfr_compound_si (y, x, i, MPFR_RNDN);
      if (mpfr_cmp_ui (y, 1) != 0)
        {
          printf ("Error, compound(x,%ld) should give 1\non x = ", i);
          mpfr_dump (x);
          printf ("got "); mpfr_dump (y);
          exit (1);
        }
    }

  /* compound(+Inf,n) = +Inf for n > 0 */
  mpfr_set_inf (x, 1);
  mpfr_compound_si (y, x, 1, MPFR_RNDN);
  if (!mpfr_inf_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error, compound(+Inf,1) should give +Inf\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }

  /* compound(+Inf,n) = +0 for n < 0 */
  mpfr_set_inf (x, 1);
  mpfr_compound_si (y, x, -1, MPFR_RNDN);
  if (!mpfr_zero_p (y) || MPFR_SIGN(y) < 0)
    {
      printf ("Error, compound(+Inf,-1) should give +0\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }

  /* compound(NaN,n) = NaN for n <> 0 */
  mpfr_set_nan (x);
  mpfr_compound_si (y, x, -1, MPFR_RNDN);
  if (!mpfr_nan_p (y))
    {
      printf ("Error, compound(NaN,-1) should give NaN\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }
  mpfr_compound_si (y, x, +1, MPFR_RNDN);
  if (!mpfr_nan_p (y))
    {
      printf ("Error, compound(NaN,+1) should give NaN\n");
      printf ("got "); mpfr_dump (y);
      exit (1);
    }

  /* hard-coded test: x is the 32-bit nearest approximation of 17/42 */
  mpfr_set_prec (x, 32);
  mpfr_set_prec (y, 32);
  mpfr_set_ui_2exp (x, 3476878287UL, -33, MPFR_RNDN);
  mpfr_compound_si (y, x, 12, MPFR_RNDN);
  mpfr_set_ui_2exp (x, 1981447393UL, -25, MPFR_RNDN);
  if (!mpfr_equal_p (y, x))
    {
      printf ("Error for compound(3476878287/2^33,12)\n");
      printf ("expected "); mpfr_dump (x);
      printf ("got      "); mpfr_dump (y);
      exit (1);
    }

  /* test for negative n */
  i = -1;
  while (1)
    {
      /* i has the form -(2^k-1) */
      mpfr_set_si_2exp (x, -1, -1, MPFR_RNDN); /* x = -0.5 */
      mpfr_compound_si (y, x, i, MPFR_RNDN);
      mpfr_set_ui_2exp (x, 1, -i, MPFR_RNDN);
      if (!mpfr_equal_p (y, x))
        {
          printf ("Error for compound(-0.5,%ld)\n", i);
          printf ("expected "); mpfr_dump (x);
          printf ("got      "); mpfr_dump (y);
          exit (1);
        }
      if (i == -2147483647) /* largest possible value on 32-bit machine */
        break;
      i = 2 * i - 1;
    }

  /* The "#if" makes sure that 64-bit constants are supported, avoiding
     a compilation failure. The "if" makes sure that the constant is
     representable in a long (this would not be the case with 32-bit
     unsigned long and 64-bit limb). */
#if GMP_NUMB_BITS >= 64 || MPFR_PREC_BITS >= 64
  if (4994322635099777669 <= LONG_MAX)
    {
      i = -4994322635099777669;
      mpfr_set_ui (x, 1, MPFR_RNDN);
      mpfr_compound_si (y, x, i, MPFR_RNDN);
      mpfr_set_si (x, 1, MPFR_RNDN);
      mpfr_mul_2si (x, x, i, MPFR_RNDN);
      if (!mpfr_equal_p (y, x))
        {
          printf ("Error for compound(1,%ld)\n", i);
          printf ("expected "); mpfr_dump (x);
          printf ("got      "); mpfr_dump (y);
          exit (1);
        }
    }
#endif

  mpfr_clear (x);
  mpfr_clear (y);
}

/* Failure with mpfr_compound_si from 2021-02-15 due to
   incorrect underflow detection. */
static void
bug_20230206 (void)
{
  if (MPFR_PREC_MIN == 1)
    {
      mpfr_t x, y1, y2;
      int inex1, inex2;
      mpfr_flags_t flags1, flags2;
#if MPFR_PREC_BITS >= 64
      mpfr_exp_t emin;
#endif

      mpfr_inits2 (1, x, y1, y2, (mpfr_ptr) 0);
      mpfr_set_ui_2exp (x, 1, -1, MPFR_RNDN);  /* x = 1/2 */

      /* This first test is useful mainly for a 32-bit mpfr_exp_t type
         (no failure with a 64-bit mpfr_exp_t type since the underflow
         threshold in the extended exponent range is much lower). */

      mpfr_set_ui_2exp (y1, 1, -1072124363, MPFR_RNDN);
      inex1 = -1;
      flags1 = MPFR_FLAGS_INEXACT;
      mpfr_clear_flags ();
      /* -1832808704 ~= -2^30 / log2(3/2) */
      inex2 = mpfr_compound_si (y2, x, -1832808704, MPFR_RNDN);
      flags2 = __gmpfr_flags;
      if (!(mpfr_equal_p (y1, y2) &&
            SAME_SIGN (inex1, inex2) &&
            flags1 == flags2))
        {
          printf ("Error in bug_20230206 (1):\n");
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

      /* This second test is for a 64-bit mpfr_exp_t type
         (it is disabled with a 32-bit mpfr_exp_t type). */

      /* The "#if" makes sure that 64-bit constants are supported, avoiding
         a compilation failure. The "if" makes sure that the constant is
         representable in a long (this would not be the case with 32-bit
         unsigned long and 64-bit limb). It also ensures that mpfr_exp_t
         has at least 64 bits. */
#if MPFR_PREC_BITS >= 64
      emin = mpfr_get_emin ();
      set_emin (MPFR_EMIN_MIN);
      mpfr_set_ui_2exp (y1, 1, -4611686018427366846, MPFR_RNDN);
      inex1 = 1;
      flags1 = MPFR_FLAGS_INEXACT;
      mpfr_clear_flags ();
      /* -7883729320669216768 ~= -2^62 / log2(3/2) */
      inex2 = mpfr_compound_si (y2, x, -7883729320669216768, MPFR_RNDN);
      flags2 = __gmpfr_flags;
      if (!(mpfr_equal_p (y1, y2) &&
            SAME_SIGN (inex1, inex2) &&
            flags1 == flags2))
        {
          printf ("Error in bug_20230206 (2):\n");
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
      set_emin (emin);
#endif

      mpfr_clears (x, y1, y2, (mpfr_ptr) 0);
    }
}

/* Reported by Patrick Pelissier on 2023-02-11 for the master branch
   (tgeneric_ui.c with GMP_CHECK_RANDOMIZE=1412991715).
   On a 32-bit host, one gets Inf (overflow) instead of 0.1E1071805703.
*/
static void
bug_20230211 (void)
{
  mpfr_t x, y1, y2;
  int inex1, inex2;
  mpfr_flags_t flags1, flags2;

  mpfr_inits2 (1, x, y1, y2, (mpfr_ptr) 0);
  mpfr_set_ui_2exp (x, 1, -1, MPFR_RNDN);  /* x = 1/2 */
  mpfr_set_ui_2exp (y1, 1, 1071805702, MPFR_RNDN);
  inex1 = 1;
  flags1 = MPFR_FLAGS_INEXACT;
  mpfr_clear_flags ();
  inex2 = mpfr_compound_si (y2, x, 1832263949, MPFR_RNDN);
  flags2 = __gmpfr_flags;
  if (!(mpfr_equal_p (y1, y2) &&
        SAME_SIGN (inex1, inex2) &&
        flags1 == flags2))
    {
      printf ("Error in bug_20230211:\n");
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
  mpfr_clears (x, y1, y2, (mpfr_ptr) 0);
}

/* Integer overflow with compound.c d04caeae04c6a83276916c4fbac1fe9b0cec3c8b
   (2023-02-23) or 952fb0f5cc2df1fffde3eb54c462fdae5f123ea6 in the 4.2 branch
   on "n * (kx - 1) + 1". Note: if the only effect is just a random value,
   this probably doesn't affect the result (one might enter the "if" while
   one shouldn't, but the real check is done inside the "if"). This test
   fails if -fsanitize=undefined -fno-sanitize-recover is used or if the
   processor emits a signal in case of integer overflow.
   This test has been made obsolete by the "kx < ex" condition
   in 2cb3123891dd46fe0258d4aec7f8655b8ec69aaf (master branch)
   or f5cb40571bc3d1559f05b230cf4ffecaf0952852 (4.2 branch). */
static void
bug_20230517 (void)
{
  mpfr_exp_t old_emax;
  mpfr_t x;

  old_emax = mpfr_get_emax ();
  set_emax (MPFR_EMAX_MAX);

  mpfr_init2 (x, 123456);
  mpfr_set_ui (x, 65536, MPFR_RNDN);
  mpfr_nextabove (x);
  mpfr_compound_si (x, x, LONG_MAX >> 16, MPFR_RNDN);
  mpfr_clear (x);

  set_emax (old_emax);
}

/* Inverse function on non-special cases...
   One has x = (1+y)^n with y > -1 and x > 0. Thus y = x^(1/n) - 1.
   The inverse function is useful
     - to build and check hard-to-round cases (see bad_cases() in tests.c);
     - to test the behavior close to the overflow and underflow thresholds.
   The case x = 0 actually needs to be handled as it may occur with
   bad_cases() due to rounding.
*/
static int
inv_compound (mpfr_ptr y, mpfr_srcptr x, long n, mpfr_rnd_t rnd_mode)
{
  mpfr_t t;
  int inexact;
  mpfr_prec_t precy, prect;
  MPFR_ZIV_DECL (loop);
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_ASSERTN (n != 0);

  if (MPFR_UNLIKELY (MPFR_IS_ZERO (x)))
    {
      if (n > 0)
        return mpfr_set_si (y, -1, rnd_mode);
      else
        {
          MPFR_SET_INF (y);
          MPFR_SET_POS (y);
          MPFR_RET (0);
        }
    }

  MPFR_SAVE_EXPO_MARK (expo);

  if (mpfr_equal_p (x, __gmpfr_one))
    {
      MPFR_SAVE_EXPO_FREE (expo);
      mpfr_set_zero (y, 1);
      MPFR_RET (0);
    }

  precy = MPFR_GET_PREC (y);
  prect = precy + 20;
  mpfr_init2 (t, prect);

  MPFR_ZIV_INIT (loop, prect);
  for (;;)
    {
      mpfr_exp_t expt1, expt2, err;
      unsigned int inext;

      if (mpfr_rootn_si (t, x, n, MPFR_RNDN) == 0)
        {
          /* With a huge t, this case would yield inext != 0 and a
             MPFR_CAN_ROUND failure until a huge precision is reached
             (as the result is very close to an exact point). Fortunately,
             since t is exact, we can obtain the correctly rounded result
             by doing the second operation to the target precision directly.
          */
          inexact = mpfr_sub_ui (y, t, 1, rnd_mode);
          goto end;
        }
      expt1 = MPFR_GET_EXP (t);
      /* |error| <= 2^(expt1-prect-1) */
      inext = mpfr_sub_ui (t, t, 1, MPFR_RNDN);
      if (MPFR_UNLIKELY (MPFR_IS_ZERO (t)))
        goto cont;  /* cannot round yet */
      expt2 = MPFR_GET_EXP (t);
      err = 1;
      if (expt2 < expt1)
        err += expt1 - expt2;
      /* |error(rootn)| <= 2^(err+expt2-prect-2)
         and if mpfr_sub_ui is inexact:
         |error| <= 2^(err+expt2-prect-2) + 2^(expt2-prect-1)
                 <= (2^(err-1) + 1) * 2^(expt2-prect-1)
                 <= 2^((err+1)+expt2-prect-2) */
      if (inext)
        err++;
      /* |error| <= 2^(err+expt2-prect-2) */
      if (MPFR_CAN_ROUND (t, prect + 2 - err, precy, rnd_mode))
        break;

    cont:
      MPFR_ZIV_NEXT (loop, prect);
      mpfr_set_prec (t, prect);
    }

  inexact = mpfr_set (y, t, rnd_mode);

 end:
  MPFR_ZIV_FREE (loop);
  mpfr_clear (t);
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}

#define DEFN(N)                                                         \
  static int mpfr_compound##N (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t r) \
  { return mpfr_compound_si (y, x, N, r); }                             \
  static int inv_compound##N (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t r)  \
  { return inv_compound (y, x, N, r); }

DEFN(2)
DEFN(3)
DEFN(4)
DEFN(5)
DEFN(17)
DEFN(120)

#define TEST_FUNCTION mpfr_compound2
#define test_generic test_generic_compound2
#include "tgeneric.c"

#define TEST_FUNCTION mpfr_compound3
#define test_generic test_generic_compound3
#include "tgeneric.c"

#define TEST_FUNCTION mpfr_compound4
#define test_generic test_generic_compound4
#include "tgeneric.c"

#define TEST_FUNCTION mpfr_compound5
#define test_generic test_generic_compound5
#include "tgeneric.c"

#define TEST_FUNCTION mpfr_compound17
#define test_generic test_generic_compound17
#include "tgeneric.c"

#define TEST_FUNCTION mpfr_compound120
#define test_generic test_generic_compound120
#include "tgeneric.c"

int
main (void)
{
  tests_start_mpfr ();

  check_ieee754 ();
  bug_20230206 ();
  bug_20230211 ();
  bug_20230517 ();

  test_generic_si (MPFR_PREC_MIN, 100, 100);

  test_generic_compound2 (MPFR_PREC_MIN, 100, 100);
  test_generic_compound3 (MPFR_PREC_MIN, 100, 100);
  test_generic_compound4 (MPFR_PREC_MIN, 100, 100);
  test_generic_compound5 (MPFR_PREC_MIN, 100, 100);
  test_generic_compound17 (MPFR_PREC_MIN, 100, 100);
  test_generic_compound120 (MPFR_PREC_MIN, 100, 100);

  /* Note: For small n, we need a psup high enough to avoid too many
     "f exact while f^(-1) inexact" occurrences in bad_cases(). */
  bad_cases (mpfr_compound2, inv_compound2, "mpfr_compound2",
             0, -256, 255, 4, 128, 240, 40);
  bad_cases (mpfr_compound3, inv_compound3, "mpfr_compound3",
             0, -256, 255, 4, 128, 120, 40);
  bad_cases (mpfr_compound4, inv_compound4, "mpfr_compound4",
             0, -256, 255, 4, 128, 80, 40);
  bad_cases (mpfr_compound5, inv_compound5, "mpfr_compound5",
             0, -256, 255, 4, 128, 80, 40);
  bad_cases (mpfr_compound17, inv_compound17, "mpfr_compound17",
             0, -256, 255, 4, 128, 80, 40);
  bad_cases (mpfr_compound120, inv_compound120, "mpfr_compound120",
             0, -256, 255, 4, 128, 80, 40);

  tests_end_mpfr ();
  return 0;
}
