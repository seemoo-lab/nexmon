/* Test file for mpfr_set_si, mpfr_set_ui, mpfr_get_si and mpfr_get_ui.

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

#define PRINT_ERROR(str) \
  do { printf ("Error for %s\n", str); exit (1); } while (0)

static void
test_2exp (void)
{
  mpfr_t x;
  int res;

  mpfr_init2 (x, 32);

  mpfr_set_ui_2exp (x, 1, 0, MPFR_RNDN);
  if (mpfr_cmp_ui (x, 1) != 0)
    PRINT_ERROR ("(1U,0)");

  mpfr_set_ui_2exp (x, 1024, -10, MPFR_RNDN);
  if (mpfr_cmp_ui(x, 1) != 0)
    PRINT_ERROR ("(1024U,-10)");

  mpfr_set_ui_2exp (x, 1024, 10, MPFR_RNDN);
  if (mpfr_cmp_ui (x, 1024 * 1024) != 0)
    PRINT_ERROR ("(1024U,+10)");

  mpfr_set_si_2exp (x, -1024L * 1024L, -10, MPFR_RNDN);
  if (mpfr_cmp_si (x, -1024) != 0)
    PRINT_ERROR ("(1M,-10)");

  mpfr_set_ui_2exp (x, 0x92345678, 16, MPFR_RNDN);
  if (mpfr_cmp_str (x, "92345678@4", 16, MPFR_RNDN) != 0)
    PRINT_ERROR ("(x92345678U,+16)");

  mpfr_set_si_2exp (x, -0x1ABCDEF0, -256, MPFR_RNDN);
  if (mpfr_cmp_str (x, "-1ABCDEF0@-64", 16, MPFR_RNDN) != 0)
    PRINT_ERROR ("(-x1ABCDEF0,-256)");

  mpfr_set_prec (x, 2);
  res = mpfr_set_si_2exp (x, 7, 10, MPFR_RNDU);
  if (mpfr_cmp_ui (x, 1<<13) != 0 || res <= 0)
    PRINT_ERROR ("Prec 2 + si_2exp");

  res = mpfr_set_ui_2exp (x, 7, 10, MPFR_RNDU);
  if (mpfr_cmp_ui (x, 1<<13) != 0 || res <= 0)
    PRINT_ERROR ("Prec 2 + ui_2exp");

  mpfr_clear_flags ();
  mpfr_set_ui_2exp (x, 17, MPFR_EMAX_MAX, MPFR_RNDN);
  if (!mpfr_inf_p (x) || MPFR_IS_NEG (x))
    PRINT_ERROR ("mpfr_set_ui_2exp and overflow (bad result)");
  if (!mpfr_overflow_p ())
    PRINT_ERROR ("mpfr_set_ui_2exp and overflow (overflow flag not set)");

  mpfr_clear_flags ();
  mpfr_set_si_2exp (x, 17, MPFR_EMAX_MAX, MPFR_RNDN);
  if (!mpfr_inf_p (x) || MPFR_IS_NEG (x))
    PRINT_ERROR ("mpfr_set_si_2exp (pos) and overflow (bad result)");
  if (!mpfr_overflow_p ())
    PRINT_ERROR ("mpfr_set_si_2exp (pos) and overflow (overflow flag not set)");

  mpfr_clear_flags ();
  mpfr_set_si_2exp (x, -17, MPFR_EMAX_MAX, MPFR_RNDN);
  if (!mpfr_inf_p (x) || MPFR_IS_POS (x))
    PRINT_ERROR ("mpfr_set_si_2exp (neg) and overflow (bad result)");
  if (!mpfr_overflow_p ())
    PRINT_ERROR ("mpfr_set_si_2exp (neg) and overflow (overflow flag not set)");

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

  for (i = 0; i < numberof(ep); i++)
    for (j = -31; j <= 31; j++)
      RND_LOOP_NO_RNDF (rnd)
        {
          int sign = j < 0 ? -1 : 1;

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

          /* Test mpfr_set_si_2exp */
          mpfr_clear_flags ();
          inex2 = mpfr_set_si_2exp (x2, j, ep[i], (mpfr_rnd_t) rnd);
          flags2 = __gmpfr_flags;

          if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
                 mpfr_equal_p (x1, x2)))
            {
              s = 's';
              goto err_extreme;
            }

          if (j < 0)
            continue;

          /* Test mpfr_set_ui_2exp */
          mpfr_clear_flags ();
          inex2 = mpfr_set_ui_2exp (x2, j, ep[i], (mpfr_rnd_t) rnd);
          flags2 = __gmpfr_flags;

          if (! (flags1 == flags2 && SAME_SIGN (inex1, inex2) &&
                 mpfr_equal_p (x1, x2)))
            {
              s = 'u';
            err_extreme:
              printf ("Error in extreme mpfr_set_%ci_2exp for i=%d j=%d %s\n",
                      s, i, j, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
              printf ("emin=%" MPFR_EXP_FSPEC "d "
                      "emax=%" MPFR_EXP_FSPEC "d\n",
                      (mpfr_eexp_t) __gmpfr_emin,
                      (mpfr_eexp_t) __gmpfr_emax);
              printf ("ep[%d] = %" MPFR_EXP_FSPEC "d\n",
                      i, (mpfr_eexp_t) ep[i]);
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

static void
test_macros (void)
{
  mpfr_t x[3];
  mpfr_ptr p;
  int r;

  /* Note: the ++'s below allow one to check that the corresponding
     arguments are evaluated only once by the macros. */

  mpfr_inits (x[0], x[1], x[2], (mpfr_ptr) 0);
  p = x[0];
  r = 0;
  mpfr_set_ui (p++, 0, (mpfr_rnd_t) r++);
  if (p != x[1] || r != 1)
    {
      printf ("Error in mpfr_set_ui macro: p - x[0] = %d (expecting 1), "
              "r = %d (expecting 1)\n", (int) (p - x[0]), r);
      exit (1);
    }
  p = x[0];
  r = 0;
  mpfr_set_si (p++, 0, (mpfr_rnd_t) r++);
  if (p != x[1] || r != 1)
    {
      printf ("Error in mpfr_set_si macro: p - x[0] = %d (expecting 1), "
              "r = %d (expecting 1)\n", (int) (p - x[0]), r);
      exit (1);
    }
  mpfr_clears (x[0], x[1], x[2], (mpfr_ptr) 0);
}

static void
test_macros_keyword (void)
{
  mpfr_t x;
  unsigned long i;

  mpfr_init2 (x, 64);
#define MKN 0x1000000
#define long short
  mpfr_set_ui (x, MKN, MPFR_RNDN);
#undef long
  i = mpfr_get_ui (x, MPFR_RNDN);
  if (i != MKN)
    {
      printf ("Error in test_macros_keyword: expected 0x%lx, got 0x%lx.\n",
              (unsigned long) MKN, i);
      exit (1);
    }
  mpfr_clear (x);
}

static void
test_get_ui_smallneg (void)
{
  mpfr_t x;
  int i;

  mpfr_init2 (x, 64);

  for (i = 1; i <= 4; i++)
    {
      int r;

      mpfr_set_si_2exp (x, -i, -2, MPFR_RNDN);
      RND_LOOP (r)
        {
          long s;
          unsigned long u;

          mpfr_clear_erangeflag ();
          s = mpfr_get_si (x, r != MPFR_RNDF ? (mpfr_rnd_t) r : MPFR_RNDA);
          if (mpfr_erangeflag_p ())
            {
              printf ("ERROR for get_si + ERANGE + small negative op"
                      " for rnd = %s and x = -%d/4\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              exit (1);
            }
          u = mpfr_get_ui (x, (mpfr_rnd_t) r);
          if (u != 0)
            {
              printf ("ERROR for get_ui + ERANGE + small negative op"
                      " for rnd = %s and x = -%d/4\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              printf ("Expected 0, got %lu\n", u);
              exit (1);
            }
          if ((s == 0) ^ !mpfr_erangeflag_p ())
            {
              const char *Not = s == 0 ? "" : " not";

              printf ("ERROR for get_ui + ERANGE + small negative op"
                      " for rnd = %s and x = -%d/4\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              printf ("The rounding integer (%ld) is%s representable in "
                      "unsigned long,\nbut the erange flag is%s set.\n",
                      s, Not, Not);
              exit (1);
            }
        }
    }

  mpfr_clear (x);
}

/* Test mpfr_get_si and mpfr_get_ui, on values around some particular
 * integers (see ts[] and tu[]): x = t?[i] + j/4, where '?' is 's' or
 * 'u', and j is an integer from -8 to 8.
 */
static void get_tests (void)
{
  mpfr_exp_t emin, emax;
  mpfr_t x, z;
  long ts[5] = { LONG_MIN, LONG_MAX, -17, 0, 17 };
  unsigned long tu[3] = { 0, ULONG_MAX, 17 };
  int s, i, j, odd, ctr = 0;
  int inex;
  int r;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  /* We need the bitsize of an unsigned long + 3 bits (1 additional bit for
   * the cases >= ULONG_MAX + 1; 2 additional bits for the fractional part).
   */
  mpfr_init2 (x, sizeof (unsigned long) * CHAR_BIT + 3);

  mpfr_init2 (z, MPFR_PREC_MIN);
  mpfr_set_ui_2exp (z, 1, -2, MPFR_RNDN);  /* z = 1/4 */

  for (s = 1; s >= 0; s--)
    for (i = 0; i < (s ? 5 : 3); i++)
      {
        odd = (s ? (unsigned long) ts[i] : tu[i]) & 1;
        inex = s ?
          mpfr_set_si (x, ts[i], MPFR_RNDN) :
          mpfr_set_ui (x, tu[i], MPFR_RNDN);
        MPFR_ASSERTN (inex == 0);
        inex = mpfr_sub_ui (x, x, 2, MPFR_RNDN);
        MPFR_ASSERTN (inex == 0);
        for (j = -8; j <= 8; j++)
          {
            /* Test x = t?[i] + j/4 in each non-RNDF rounding mode... */
            RND_LOOP_NO_RNDF (r)
              {
                mpfr_flags_t ex_flags, flags;
                int e, k, overflow;

                ctr++;  /* for the check below */

                /* Let's determine k such that the rounded integer should
                   be t?[i] + k, assuming an unbounded exponent range. */
                k = (j + 8 +
                     (MPFR_IS_LIKE_RNDD (r, MPFR_SIGN (x)) ? 0 :
                      MPFR_IS_LIKE_RNDU (r, MPFR_SIGN (x)) ? 3 :
                      2)) / 4 - 2;
                if (r == MPFR_RNDN && ((unsigned int) j & 3) == 2 &&
                    ((odd + k) & 1))
                  k--;  /* even rounding */

                /* Overflow cases. Note that with the above choices:
                   _ t?[0] == minval(type)
                   _ t?[1] == maxval(type)
                */
                overflow = (i == 0 && k < 0) || (i == 1 && k > 0);

                /* Expected flags. Note that in case of overflow, only the
                   erange flag is set. Otherwise, the result is inexact iff
                   j mod 1 != 0, i.e. the last two bits are not 00. */
                ex_flags = overflow ? MPFR_FLAGS_ERANGE
                  : ((unsigned int) j & 3) != 0 ? MPFR_FLAGS_INEXACT : 0;

                mpfr_clear_flags ();

#define GET_TESTS_TEST(TYPE,TZ,F,C,FMT)                                 \
                do {                                                    \
                  TYPE a, d;                                            \
                                                                        \
                  a = TZ[i] + (overflow ? 0 : k);                       \
                  if (e)                                                \
                    {                                                   \
                      mpfr_exp_t ex;                                    \
                      ex = MPFR_GET_EXP (x);                            \
                      set_emin (ex);                                    \
                      set_emax (ex);                                    \
                    }                                                   \
                  d = F (x, (mpfr_rnd_t) r);                            \
                  flags = __gmpfr_flags;                                \
                  set_emin (emin);                                      \
                  set_emax (emax);                                      \
                  if (flags != ex_flags || a != d)                      \
                    {                                                   \
                      printf ("Error in get_tests for " #F " on %s%s\n", \
                              mpfr_print_rnd_mode ((mpfr_rnd_t) r),     \
                              e ? ", reduced exponent range" : "");     \
                      printf ("x = t" C "[%d] + (%d/4) = ", i, j);      \
                      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);       \
                      printf ("\n--> k = %d\n", k);                     \
                      printf ("Expected %l" FMT "\n", a);               \
                      printf ("Got      %l" FMT "\n", d);               \
                      printf ("Expected flags:");                       \
                      flags_out (ex_flags);                             \
                      printf ("Got flags:     ");                       \
                      flags_out (flags);                                \
                      exit (1);                                         \
                    }                                                   \
                } while (0)

                for (e = 0; e < 2; e++)
                  {
                    if (e && MPFR_IS_ZERO (x))
                      break;
                    if (s)
                      GET_TESTS_TEST (long,
                                      ts, mpfr_get_si, "s", "d");
                    else
                      GET_TESTS_TEST (unsigned long,
                                      tu, mpfr_get_ui, "u", "u");
                  }
              }
            inex = mpfr_add (x, x, z, MPFR_RNDN);
            MPFR_ASSERTN (inex == 0);
          }
      }

  /* Check that we have tested everything: 8 = 5 + 3 integers t?[i]
   * with 17 = 8 - (-8) + 1 additional terms (j/4) for each integer,
   * and each non-RNDF rounding mode.
   */
  MPFR_ASSERTN (ctr == 8 * 17 * ((int) MPFR_RND_MAX - 1));

  mpfr_clear (x);
  mpfr_clear (z);
}

/* FIXME: Comparing against mpfr_get_si/ui is not ideal, it'd be better to
   have all tests examine the bits in mpfr_t for what should come out.  */

int
main (int argc, char *argv[])
{
  mpfr_t x;
  long k, z, d, N;
  unsigned long zl, dl;
  int inex;
  int r;
  mpfr_exp_t emin, emax;
  int flag;

  tests_start_mpfr ();

  get_tests ();

  mpfr_init2 (x, 100);

  N = (argc == 1) ? 100000 : atol (argv[1]);

  for (k = 1; k <= N; k++)
    {
      z = (long) (randlimb () & LONG_MAX) + LONG_MIN / 2;
      inex = mpfr_set_si (x, z, MPFR_RNDZ);
      d = mpfr_get_si (x, MPFR_RNDZ);
      if (d != z)
        {
          printf ("Error in mpfr_set_si: expected %ld got %ld\n", z, d);
          exit (1);
        }
      if (inex)
        {
          printf ("Error in mpfr_set_si: inex value incorrect for %ld: %d\n",
                  z, inex);
          exit (1);
        }
    }

  for (k = 1; k <= N; k++)
    {
      zl = randlimb ();
      inex = mpfr_set_ui (x, zl, MPFR_RNDZ);
      dl = mpfr_get_ui (x, MPFR_RNDZ);
      if (dl != zl)
        {
          printf ("Error in mpfr_set_ui: expected %lu got %lu\n", zl, dl);
          exit (1);
        }
      if (inex)
        {
          printf ("Error in mpfr_set_ui: inex value incorrect for %lu: %d\n",
                  zl, inex);
          exit (1);
        }
    }

  mpfr_set_prec (x, 2);
  if (mpfr_set_si (x, 5, MPFR_RNDZ) >= 0)
    {
      printf ("Wrong inexact flag for x=5, rnd=MPFR_RNDZ\n");
      exit (1);
    }

  mpfr_set_prec (x, 2);
  if (mpfr_set_si (x, -5, MPFR_RNDZ) <= 0)
    {
      printf ("Wrong inexact flag for x=-5, rnd=MPFR_RNDZ\n");
      exit (1);
    }

  mpfr_set_prec (x, 3);
  inex = mpfr_set_si (x, 77617, MPFR_RNDD); /* should be 65536 */
  if (MPFR_MANT(x)[0] != MPFR_LIMB_HIGHBIT || inex >= 0)
    {
      printf ("Error in mpfr_set_si(x:3, 77617, MPFR_RNDD)\n");
      mpfr_dump (x);
      exit (1);
    }
  inex = mpfr_set_ui (x, 77617, MPFR_RNDD); /* should be 65536 */
  if (MPFR_MANT(x)[0] != MPFR_LIMB_HIGHBIT || inex >= 0)
    {
      printf ("Error in mpfr_set_ui(x:3, 77617, MPFR_RNDD)\n");
      mpfr_dump (x);
      exit (1);
    }

  mpfr_set_prec (x, 2);
  inex = mpfr_set_si (x, 33096, MPFR_RNDU);
  if (mpfr_get_si (x, MPFR_RNDZ) != 49152 || inex <= 0)
    {
      printf ("Error in mpfr_set_si, exp. 49152, got %ld, inex %d\n",
              mpfr_get_si (x, MPFR_RNDZ), inex);
      exit (1);
    }
  inex = mpfr_set_ui (x, 33096, MPFR_RNDU);
  if (mpfr_get_si (x, MPFR_RNDZ) != 49152)
    {
      printf ("Error in mpfr_set_ui, exp. 49152, got %ld, inex %d\n",
              mpfr_get_si (x, MPFR_RNDZ), inex);
      exit (1);
    }
  /* Also test the mpfr_set_ui function (instead of macro). */
  inex = (mpfr_set_ui) (x, 33096, MPFR_RNDU);
  if (mpfr_get_si (x, MPFR_RNDZ) != 49152)
    {
      printf ("Error in mpfr_set_ui function, exp. 49152, got %ld, inex %d\n",
              mpfr_get_si (x, MPFR_RNDZ), inex);
      exit (1);
    }

  RND_LOOP (r)
    {
      mpfr_set_si (x, -1, (mpfr_rnd_t) r);
      mpfr_set_ui (x, 0, (mpfr_rnd_t) r);
      if (MPFR_IS_NEG (x) || mpfr_get_ui (x, (mpfr_rnd_t) r) != 0)
        {
          printf ("mpfr_set_ui (x, 0) gives -0 for %s\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          exit (1);
        }

      mpfr_set_si (x, -1, (mpfr_rnd_t) r);
      mpfr_set_si (x, 0, (mpfr_rnd_t) r);
      if (MPFR_IS_NEG (x) || mpfr_get_si (x, (mpfr_rnd_t) r) != 0)
        {
          printf ("mpfr_set_si (x, 0) gives -0 for %s\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          exit (1);
        }
    }

  /* check potential bug in case mp_limb_t is unsigned */
  emax = mpfr_get_emax ();
  set_emax (0);
  mpfr_set_si (x, -1, MPFR_RNDN);
  if (mpfr_sgn (x) >= 0)
    {
      printf ("mpfr_set_si (x, -1) fails\n");
      exit (1);
    }
  set_emax (emax);

  emax = mpfr_get_emax ();
  set_emax (5);
  mpfr_set_prec (x, 2);
  mpfr_set_si (x, -31, MPFR_RNDN);
  if (mpfr_sgn (x) >= 0)
    {
      printf ("mpfr_set_si (x, -31) fails\n");
      exit (1);
    }
  set_emax (emax);

  /* test for get_ui */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_get_ui (x, MPFR_RNDN) == 0);
  mpfr_set_ui (x, ULONG_MAX, MPFR_RNDU);
  mpfr_nextabove (x);
  mpfr_get_ui (x, MPFR_RNDU);

  /* another test for get_ui */
  mpfr_set_prec (x, 10);
  mpfr_set_str_binary (x, "10.101");
  dl = mpfr_get_ui (x, MPFR_RNDN);
  MPFR_ASSERTN (dl == 3);

  mpfr_set_str_binary (x, "-1.0");
  mpfr_get_ui (x, MPFR_RNDN);

  mpfr_set_str_binary (x, "0.1");
  dl = mpfr_get_ui (x, MPFR_RNDN);
  MPFR_ASSERTN (dl == 0);
  dl = mpfr_get_ui (x, MPFR_RNDZ);
  MPFR_ASSERTN (dl == 0);
  dl = mpfr_get_ui (x, MPFR_RNDD);
  MPFR_ASSERTN (dl == 0);
  dl = mpfr_get_ui (x, MPFR_RNDU);
  MPFR_ASSERTN (dl == 1);

  /* coverage tests */
  mpfr_set_prec (x, 2);
  mpfr_set_si (x, -7, MPFR_RNDD);
  MPFR_ASSERTN(mpfr_cmp_si (x, -8) == 0);
  mpfr_set_prec (x, 2);
  mpfr_set_ui (x, 7, MPFR_RNDU);
  MPFR_ASSERTN(mpfr_cmp_ui (x, 8) == 0);
  emax = mpfr_get_emax ();
  set_emax (3);
  mpfr_set_ui (x, 7, MPFR_RNDU);
  MPFR_ASSERTN(mpfr_inf_p (x) && mpfr_sgn (x) > 0);
  set_emax (1);
  MPFR_ASSERTN( mpfr_set_ui (x, 7, MPFR_RNDU) );
  MPFR_ASSERTN(mpfr_inf_p (x) && mpfr_sgn (x) > 0);
  set_emax (emax);
  mpfr_set_ui_2exp (x, 17, -50, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_get_ui (x, MPFR_RNDD) == 0);
  MPFR_ASSERTN (mpfr_get_si (x, MPFR_RNDD) == 0);

  /* Test for ERANGE flag + correct behavior if overflow */
  mpfr_set_prec (x, 256);
  mpfr_set_ui (x, ULONG_MAX, MPFR_RNDN);
  mpfr_clear_erangeflag ();
  dl = mpfr_get_ui (x, MPFR_RNDN);
  if (dl != ULONG_MAX || mpfr_erangeflag_p ())
    {
      printf ("ERROR for get_ui + ERANGE + ULONG_MAX (1)\n");
      exit (1);
    }
  mpfr_add_ui (x, x, 1, MPFR_RNDN);
  dl = mpfr_get_ui (x, MPFR_RNDN);
  if (dl != ULONG_MAX || !mpfr_erangeflag_p ())
    {
      printf ("ERROR for get_ui + ERANGE + ULONG_MAX (2)\n");
      exit (1);
    }
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_clear_erangeflag ();
  dl = mpfr_get_ui (x, MPFR_RNDN);
  if (dl != 0 || !mpfr_erangeflag_p ())
    {
      printf ("ERROR for get_ui + ERANGE + -1 \n");
      exit (1);
    }
  mpfr_set_si (x, LONG_MAX, MPFR_RNDN);
  mpfr_clear_erangeflag ();
  d = mpfr_get_si (x, MPFR_RNDN);
  if (d != LONG_MAX || mpfr_erangeflag_p ())
    {
      printf ("ERROR for get_si + ERANGE + LONG_MAX (1): %ld\n", d);
      exit (1);
    }
  mpfr_add_ui (x, x, 1, MPFR_RNDN);
  d = mpfr_get_si (x, MPFR_RNDN);
  if (d != LONG_MAX || !mpfr_erangeflag_p ())
    {
      printf ("ERROR for get_si + ERANGE + LONG_MAX (2)\n");
      exit (1);
    }
  mpfr_set_si (x, LONG_MIN, MPFR_RNDN);
  mpfr_clear_erangeflag ();
  d = mpfr_get_si (x, MPFR_RNDN);
  if (d != LONG_MIN || mpfr_erangeflag_p ())
    {
      printf ("ERROR for get_si + ERANGE + LONG_MIN (1)\n");
      exit (1);
    }
  mpfr_sub_ui (x, x, 1, MPFR_RNDN);
  d = mpfr_get_si (x, MPFR_RNDN);
  if (d != LONG_MIN || !mpfr_erangeflag_p ())
    {
      printf ("ERROR for get_si + ERANGE + LONG_MIN (2)\n");
      exit (1);
    }

  mpfr_set_nan (x);
  mpfr_clear_flags ();
  d = mpfr_get_ui (x, MPFR_RNDN);
  if (d != 0 || __gmpfr_flags != MPFR_FLAGS_ERANGE)
    {
      printf ("ERROR for get_ui + NaN\n");
      exit (1);
    }
  mpfr_clear_erangeflag ();
  d = mpfr_get_si (x, MPFR_RNDN);
  if (d != 0 || __gmpfr_flags != MPFR_FLAGS_ERANGE)
    {
      printf ("ERROR for get_si + NaN\n");
      exit (1);
    }

  emin = mpfr_get_emin ();
  mpfr_set_prec (x, 2);

  set_emin (4);
  mpfr_clear_flags ();
  mpfr_set_ui (x, 7, MPFR_RNDU);
  flag = mpfr_underflow_p ();
  set_emin (emin);
  if (mpfr_cmp_ui (x, 8) != 0)
    {
      printf ("Error for mpfr_set_ui (x, 7, MPFR_RNDU), prec = 2, emin = 4\n");
      exit (1);
    }
  if (flag)
    {
      printf ("mpfr_set_ui (x, 7, MPFR_RNDU) should not underflow "
              "with prec = 2, emin = 4\n");
      exit (1);
    }

  set_emin (4);
  mpfr_clear_flags ();
  mpfr_set_si (x, -7, MPFR_RNDD);
  flag = mpfr_underflow_p ();
  set_emin (emin);
  if (mpfr_cmp_si (x, -8) != 0)
    {
      printf ("Error for mpfr_set_si (x, -7, MPFR_RNDD), prec = 2, emin = 4\n");
      exit (1);
    }
  if (flag)
    {
      printf ("mpfr_set_si (x, -7, MPFR_RNDD) should not underflow "
              "with prec = 2, emin = 4\n");
      exit (1);
    }

  mpfr_clear (x);

  test_2exp ();
  test_2exp_extreme ();
  test_macros ();
  test_macros_keyword ();
  test_get_ui_smallneg ();
  tests_end_mpfr ();
  return 0;
}
