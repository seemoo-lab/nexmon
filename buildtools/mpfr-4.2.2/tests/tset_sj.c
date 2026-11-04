/* Test file for
   mpfr_set_sj, mpfr_set_uj, mpfr_set_sj_2exp and mpfr_set_uj_2exp.

Copyright 2004, 2006-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_INTMAX_H
#include "mpfr-test.h"

#ifndef _MPFR_H_HAVE_INTMAX_T

int
main (void)
{
  return 77;
}

#else

#define PRINT_ERROR(str) \
  do { printf ("Error for %s\n", str); exit (1); } while (0)

static int
inexact_sign (int x)
{
  return (x < 0) ? -1 : (x > 0);
}

static void
check_set_uj (mpfr_prec_t pmin, mpfr_prec_t pmax, int N)
{
  mpfr_t x, y;
  mpfr_prec_t p;
  int inex1, inex2, n;
  mp_limb_t limb;

  mpfr_inits2 (pmax, x, y, (mpfr_ptr) 0);

  for (p = pmin ; p < pmax ; p++)
    {
      mpfr_set_prec (x, p);
      mpfr_set_prec (y, p);
      for (n = 0 ; n < N ; n++)
        {
          /* mp_limb_t may be unsigned long long */
          limb = (unsigned long) randlimb ();
          inex1 = mpfr_set_uj (x, limb, MPFR_RNDN);
          inex2 = mpfr_set_ui (y, limb, MPFR_RNDN);
          if (mpfr_cmp (x, y))
            {
              printf ("ERROR for mpfr_set_uj and j=%lu and p=%lu\n",
                      (unsigned long) limb, (unsigned long) p);
              printf ("X="); mpfr_dump (x);
              printf ("Y="); mpfr_dump (y);
              exit (1);
            }
          if (inexact_sign (inex1) != inexact_sign (inex2))
            {
              printf ("ERROR for inexact(set_uj): j=%lu p=%lu\n"
                      "Inexact1= %d Inexact2= %d\n",
                      (unsigned long) limb, (unsigned long) p, inex1, inex2);
              exit (1);
            }
        }
    }
  /* Special case */
  mpfr_set_prec (x, sizeof(uintmax_t)*CHAR_BIT);
  inex1 = mpfr_set_uj (x, UINTMAX_MAX, MPFR_RNDN);
  if (inex1 != 0 || mpfr_sgn(x) <= 0)
    PRINT_ERROR ("inexact / UINTMAX_MAX");
  inex1 = mpfr_add_ui (x, x, 1, MPFR_RNDN);
  if (inex1 != 0 || !mpfr_powerof2_raw (x)
      || MPFR_EXP (x) != sizeof(uintmax_t) * CHAR_BIT + 1)
    PRINT_ERROR ("power of 2");
  mpfr_set_uj (x, 0, MPFR_RNDN);
  if (!MPFR_IS_ZERO (x))
    PRINT_ERROR ("Setting 0");

  mpfr_clears (x, y, (mpfr_ptr) 0);
}

static void
check_set_uj_2exp (void)
{
  mpfr_t x;
  int inex;

  mpfr_init2 (x, sizeof(uintmax_t)*CHAR_BIT);

  inex = mpfr_set_uj_2exp (x, 1, 0, MPFR_RNDN);
  if (inex || mpfr_cmp_ui(x, 1))
    PRINT_ERROR ("(1U,0)");

  inex = mpfr_set_uj_2exp (x, 1024, -10, MPFR_RNDN);
  if (inex || mpfr_cmp_ui(x, 1))
    PRINT_ERROR ("(1024U,-10)");

  inex = mpfr_set_uj_2exp (x, 1024, 10, MPFR_RNDN);
  if (inex || mpfr_cmp_ui(x, 1024L * 1024L))
    PRINT_ERROR ("(1024U,+10)");

  inex = mpfr_set_uj_2exp (x, UINTMAX_MAX, 1000, MPFR_RNDN);
  inex |= mpfr_div_2ui (x, x, 1000, MPFR_RNDN);
  inex |= mpfr_add_ui (x, x, 1, MPFR_RNDN);
  if (inex || !mpfr_powerof2_raw (x)
      || MPFR_EXP (x) != sizeof(uintmax_t) * CHAR_BIT + 1)
    PRINT_ERROR ("(UINTMAX_MAX)");

  inex = mpfr_set_uj_2exp (x, UINTMAX_MAX, MPFR_EMAX_MAX-10, MPFR_RNDN);
  if (inex == 0 || !mpfr_inf_p (x))
    PRINT_ERROR ("Overflow");

  inex = mpfr_set_uj_2exp (x, UINTMAX_MAX, MPFR_EMIN_MIN-1000, MPFR_RNDN);
  if (inex == 0 || !MPFR_IS_ZERO (x))
    PRINT_ERROR ("Underflow");

  mpfr_clear (x);
}

static void
check_set_sj (void)
{
  mpfr_t x;
  int inex;

  mpfr_init2 (x, sizeof(intmax_t)*CHAR_BIT-1);

  inex = mpfr_set_sj (x, -INTMAX_MAX, MPFR_RNDN);
  inex |= mpfr_add_si (x, x, -1, MPFR_RNDN);
  if (inex || mpfr_sgn (x) >=0 || !mpfr_powerof2_raw (x)
      || MPFR_EXP (x) != sizeof(intmax_t) * CHAR_BIT)
    PRINT_ERROR ("set_sj (-INTMAX_MAX)");

  inex = mpfr_set_sj (x, 1742, MPFR_RNDN);
  if (inex || mpfr_cmp_ui (x, 1742))
    PRINT_ERROR ("set_sj (1742)");

  mpfr_clear (x);
}

static void
check_set_sj_2exp (void)
{
  mpfr_t x;
  int inex;

  mpfr_init2 (x, sizeof(intmax_t)*CHAR_BIT-1);

  inex = mpfr_set_sj_2exp (x, INTMAX_MIN, 1000, MPFR_RNDN);
  if (inex || mpfr_sgn (x) >=0 || !mpfr_powerof2_raw (x)
      || MPFR_EXP (x) != sizeof(intmax_t) * CHAR_BIT + 1000)
    PRINT_ERROR ("set_sj_2exp (INTMAX_MIN)");

  mpfr_clear (x);
}

#define REXP 1024

static void
test_2exp_extreme_aux (void)
{
  mpfr_t x1, x2, y;
  mpfr_exp_t e, ep[1 + 8 * 5], eb[] =
    { MPFR_EMIN_MIN, -REXP, REXP, MPFR_EMAX_MAX, MPFR_EXP_MAX };
  mpfr_flags_t flags1, flags2;
  int i, j, rnd, inex1, inex2;
  char s;

  ep[0] = MPFR_EXP_MIN;
  for (i = 0; i < numberof(eb); i++)
    for (j = 0; j < 8; j++)
      ep[1 + 8 * i + j] = eb[i] - j;

  mpfr_inits2 (3, x1, x2, (mpfr_ptr) 0);
  mpfr_init2 (y, 32);

  /* The cast to int is needed if numberof(ep) is of unsigned type, in
     which case the condition without the cast would be always false. */
  for (i = -2; i < (int) numberof(ep); i++)
    for (j = -31; j <= 31; j++)
      RND_LOOP_NO_RNDF (rnd)
        {
          int sign = j < 0 ? -1 : 1;
          intmax_t em;

          if (i < 0)
            {
              em = i == -2 ? INTMAX_MIN : INTMAX_MAX;
              mpfr_clear_flags ();
              inex1 = j == 0 ?
                (mpfr_set_zero (x1, +1), 0) : i == -2 ?
                mpfr_underflow (x1, rnd == MPFR_RNDN ?
                                MPFR_RNDZ : (mpfr_rnd_t) rnd, sign) :
                mpfr_overflow (x1, (mpfr_rnd_t) rnd, sign);
              flags1 = __gmpfr_flags;
            }
          else
            {
              em = ep[i];
              /* Compute the expected value, inex and flags */
              inex1 = mpfr_set_si (y, j, MPFR_RNDN);
              MPFR_ASSERTN (inex1 == 0);
              inex1 = mpfr_set (x1, y, (mpfr_rnd_t) rnd);
              /* x1 is the rounded value and inex1 the ternary value,
                 assuming that the exponent argument is 0 (this is the
                 rounded significand of the final result, assuming an
                 unbounded exponent range). The multiplication by a
                 power of 2 is exact, unless underflow/overflow occurs.
                 The tests on the exponent below avoid integer overflows
                 (ep[i] may take extreme values). */
              mpfr_clear_flags ();
              if (j == 0)
                goto zero;
              e = MPFR_GET_EXP (x1);
              if (ep[i] < __gmpfr_emin - e)  /* underflow */
                {
                  mpfr_rnd_t r =
                    (rnd == MPFR_RNDN &&
                     (ep[i] < __gmpfr_emin - MPFR_GET_EXP (y) - 1 ||
                      IS_POW2 (sign * j))) ?
                    MPFR_RNDZ : (mpfr_rnd_t) rnd;
                  inex1 = mpfr_underflow (x1, r, sign);
                  flags1 = __gmpfr_flags;
                }
              else if (ep[i] > __gmpfr_emax - e)  /* overflow */
                {
                  inex1 = mpfr_overflow (x1, (mpfr_rnd_t) rnd, sign);
                  flags1 = __gmpfr_flags;
                }
              else
                {
                  mpfr_set_exp (x1, ep[i] + e);
                zero:
                  flags1 = inex1 != 0 ? MPFR_FLAGS_INEXACT : 0;
                }
            }

          /* Test mpfr_set_sj_2exp */
          mpfr_clear_flags ();
          inex2 = mpfr_set_sj_2exp (x2, j, em, (mpfr_rnd_t) rnd);
          flags2 = __gmpfr_flags;

          if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
                 mpfr_equal_p (x1, x2)))
            {
              s = 's';
              goto err_extreme;
            }

          if (j < 0)
            continue;

          /* Test mpfr_set_uj_2exp */
          mpfr_clear_flags ();
          inex2 = mpfr_set_uj_2exp (x2, j, em, (mpfr_rnd_t) rnd);
          flags2 = __gmpfr_flags;

          if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
                 mpfr_equal_p (x1, x2)))
            {
              s = 'u';
            err_extreme:
              printf ("Error in extreme mpfr_set_%cj_2exp for i=%d j=%d %s\n",
                      s, i, j, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
              printf ("emin=%" MPFR_EXP_FSPEC "d "
                      "emax=%" MPFR_EXP_FSPEC "d\n",
                      (mpfr_eexp_t) __gmpfr_emin,
                      (mpfr_eexp_t) __gmpfr_emax);
#ifndef NPRINTF_J
              printf ("e = %jd\n", em);
#endif
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

  mpfr_clears (x1, x2, y, (mpfr_ptr) 0);
}

static void
test_2exp_extreme (void)
{
  mpfr_exp_t emin, emax;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  set_emin (MPFR_EMIN_MIN);
  set_emax (MPFR_EMAX_MAX);
  test_2exp_extreme_aux ();

  set_emin (-REXP);
  set_emax (REXP);
  test_2exp_extreme_aux ();

  set_emin (emin);
  set_emax (emax);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  check_set_uj (2, 128, 50);
  check_set_uj_2exp ();
  check_set_sj ();
  check_set_sj_2exp ();
  test_2exp_extreme ();

  tests_end_mpfr ();
  return 0;
}

#endif
