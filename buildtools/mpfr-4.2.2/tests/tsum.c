/* tsum -- test file for the list summation function

Copyright 2004-2025 Free Software Foundation, Inc.
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

#ifndef MPFR_NCANCEL
#define MPFR_NCANCEL 10
#endif

#include <time.h>

/* return the cpu time in milliseconds */
static int
cputime (void)
{
  return clock () / (CLOCKS_PER_SEC / 1000);
}

static mpfr_prec_t
get_prec_max (mpfr_t *t, int n)
{
  mpfr_exp_t e, min, max;
  int i;

  min = MPFR_EMAX_MAX;
  max = MPFR_EMIN_MIN;
  for (i = 0; i < n; i++)
    if (MPFR_IS_PURE_FP (t[i]))
      {
        e = MPFR_GET_EXP (t[i]);
        if (e > max)
          max = e;
        e -= MPFR_GET_PREC (t[i]);
        if (e < min)
          min = e;
      }

  return min > max ? MPFR_PREC_MIN /* no pure FP values */
    : max - min + __gmpfr_ceil_log2 (n);
}

static void
get_exact_sum (mpfr_ptr sum, mpfr_t *tab, int n)
{
  int i;

  mpfr_set_prec (sum, get_prec_max (tab, n));
  mpfr_set_ui (sum, 0, MPFR_RNDN);
  for (i = 0; i < n; i++)
    if (mpfr_add (sum, sum, tab[i], MPFR_RNDN))
      {
        printf ("FIXME: get_exact_sum is buggy.\n");
        exit (1);
      }
}

static void
generic_tests (void)
{
  mpfr_t exact_sum, sum1, sum2;
  mpfr_t *t;
  mpfr_ptr *p;
  mpfr_prec_t precmax = 444;
  int i, m, nmax = 500;
  int rnd_mode;

  t = (mpfr_t *) tests_allocate (nmax * sizeof(mpfr_t));
  p = (mpfr_ptr *) tests_allocate (nmax * sizeof(mpfr_ptr));
  for (i = 0; i < nmax; i++)
    {
      mpfr_init2 (t[i], precmax);
      p[i] = t[i];
    }
  mpfr_inits2 (precmax, exact_sum, sum1, sum2, (mpfr_ptr) 0);

  for (m = 0; m < 20000; m++)
    {
      int non_uniform, n;
      mpfr_prec_t prec;

      non_uniform = randlimb () % 10;
      n = (randlimb () % nmax) + 1;
      prec = MPFR_PREC_MIN + (randlimb () % (precmax - MPFR_PREC_MIN + 1));
      mpfr_set_prec (sum1, prec);
      mpfr_set_prec (sum2, prec);

      for (i = 0; i < n; i++)
        {
          mpfr_set_prec (t[i], MPFR_PREC_MIN +
                         (randlimb () % (precmax - MPFR_PREC_MIN + 1)));
          mpfr_urandomb (t[i], RANDS);
          if (m % 8 != 0 && (m % 8 == 1 || RAND_BOOL ()))
            mpfr_neg (t[i], t[i], MPFR_RNDN);
          if (non_uniform && MPFR_NOTZERO (t[i]))
            mpfr_set_exp (t[i], randlimb () % 1000);
          /* putchar ("-0+"[SIGN (mpfr_sgn (t[i])) + 1]); */
        }
      /* putchar ('\n'); */
      get_exact_sum (exact_sum, t, n);
      RND_LOOP (rnd_mode)
        if (rnd_mode == MPFR_RNDF)
          {
            int inex;

            inex = mpfr_set (sum1, exact_sum, MPFR_RNDD);
            mpfr_sum (sum2, p, n, MPFR_RNDF);
            if (! mpfr_equal_p (sum1, sum2) &&
                (inex == 0 ||
                 (mpfr_nextabove (sum1), ! mpfr_equal_p (sum1, sum2))))
              {
                printf ("generic_tests failed on m = %d, MPFR_RNDF\n", m);
                printf ("Exact sum = ");
                mpfr_dump (exact_sum);
                printf ("Got         ");
                mpfr_dump (sum2);
                exit (1);
              }
          }
        else
          {
            int inex1, inex2;

            inex1 = mpfr_set (sum1, exact_sum, (mpfr_rnd_t) rnd_mode);
            inex2 = mpfr_sum (sum2, p, n, (mpfr_rnd_t) rnd_mode);
            if (!(mpfr_equal_p (sum1, sum2) && SAME_SIGN (inex1, inex2)))
              {
                printf ("generic_tests failed on m = %d, %s\n", m,
                        mpfr_print_rnd_mode ((mpfr_rnd_t) rnd_mode));
                printf ("Expected ");
                mpfr_dump (sum1);
                printf ("with inex = %d\n", inex1);
                printf ("Got      ");
                mpfr_dump (sum2);
                printf ("with inex = %d\n", inex2);
                exit (1);
              }
          }
    }

  for (i = 0; i < nmax; i++)
    mpfr_clear (t[i]);
  mpfr_clears (exact_sum, sum1, sum2, (mpfr_ptr) 0);
  tests_free (t, nmax * sizeof(mpfr_t));
  tests_free (p, nmax * sizeof(mpfr_ptr));
}

/* glibc free() error or segmentation fault when configured
 * with GMP 6.0.0 built with "--disable-alloca ABI=32".
 * GCC's address sanitizer shows a heap-buffer-overflow.
 * Fixed in r9369 (before the merge into the trunk). The problem was due
 * to the fact that mpn functions do not accept a zero size argument, and
 * since mpn_add_1 is here a macro in GMP, there's no assertion even when
 * GMP was built with assertion checking (--enable-assert).
 */
static void
check_simple (void)
{
  mpfr_t tab[3], r;
  mpfr_ptr tabp[3];
  int i;

  mpfr_init2 (r, 16);
  for (i = 0; i < 3; i++)
    {
      mpfr_init2 (tab[i], 16);
      mpfr_set_ui (tab[i], 1, MPFR_RNDN);
      tabp[i] = tab[i];
    }

  i = mpfr_sum (r, tabp, 3, MPFR_RNDN);
  if (mpfr_cmp_ui (r, 3) || i != 0)
    {
      printf ("Error in check_simple\n");
      exit (1);
    }

  mpfr_clears (tab[0], tab[1], tab[2], r, (mpfr_ptr) 0);
}

static void
check_special (void)
{
  mpfr_t tab[3], r;
  mpfr_ptr tabp[3];
  int i;

  mpfr_inits2 (53, tab[0], tab[1], tab[2], r, (mpfr_ptr) 0);
  tabp[0] = tab[0];
  tabp[1] = tab[1];
  tabp[2] = tab[2];

  i = mpfr_sum (r, tabp, 0, MPFR_RNDN);
  if (!MPFR_IS_ZERO (r) || !MPFR_IS_POS (r) || i != 0)
    {
      printf ("Special case n==0 failed!\n");
      exit (1);
    }

  mpfr_set_ui (tab[0], 42, MPFR_RNDN);
  i = mpfr_sum (r, tabp, 1, MPFR_RNDN);
  if (mpfr_cmp_ui (r, 42) || i != 0)
    {
      printf ("Special case n==1 failed!\n");
      exit (1);
    }

  mpfr_set_ui (tab[1], 17, MPFR_RNDN);
  MPFR_SET_NAN (tab[2]);
  i = mpfr_sum (r, tabp, 3, MPFR_RNDN);
  if (!MPFR_IS_NAN (r) || i != 0)
    {
      printf ("Special case NAN failed!\n");
      exit (1);
    }

  MPFR_SET_INF (tab[2]);
  MPFR_SET_POS (tab[2]);
  i = mpfr_sum (r, tabp, 3, MPFR_RNDN);
  if (!MPFR_IS_INF (r) || !MPFR_IS_POS (r) || i != 0)
    {
      printf ("Special case +INF failed!\n");
      exit (1);
    }

  MPFR_SET_INF (tab[2]);
  MPFR_SET_NEG (tab[2]);
  i = mpfr_sum (r, tabp, 3, MPFR_RNDN);
  if (!MPFR_IS_INF (r) || !MPFR_IS_NEG (r) || i != 0)
    {
      printf ("Special case -INF failed!\n");
      exit (1);
    }

  MPFR_SET_ZERO (tab[1]);
  i = mpfr_sum (r, tabp, 2, MPFR_RNDN);
  if (mpfr_cmp_ui (r, 42) || i != 0)
    {
      printf ("Special case 42+0 failed!\n");
      exit (1);
    }

  MPFR_SET_NAN (tab[0]);
  i = mpfr_sum (r, tabp, 3, MPFR_RNDN);
  if (!MPFR_IS_NAN (r) || i != 0)
    {
      printf ("Special case NAN+0+-INF failed!\n");
      exit (1);
    }

  mpfr_set_inf (tab[0], 1);
  mpfr_set_ui  (tab[1], 59, MPFR_RNDN);
  mpfr_set_inf (tab[2], -1);
  i = mpfr_sum (r, tabp, 3, MPFR_RNDN);
  if (!MPFR_IS_NAN (r) || i != 0)
    {
      printf ("Special case +INF + 59 +-INF failed!\n");
      exit (1);
    }

  mpfr_clears (tab[0], tab[1], tab[2], r, (mpfr_ptr) 0);
}

#define NC 7
#define NS 6

static void
check_more_special (void)
{
  const char *str[NC] = { "NaN", "+Inf", "-Inf", "+0", "-0", "+1", "-1" };
  int i, r, k[NS];
  mpfr_t c[NC], s[NS], sum;
  mpfr_ptr p[NS];
  int inex;

  for (i = 0; i < NC; i++)
    {
      int ret;
      mpfr_init2 (c[i], 8);
      ret = mpfr_set_str (c[i], str[i], 0, MPFR_RNDN);
      MPFR_ASSERTN (ret == 0);
    }
  for (i = 0; i < NS; i++)
    mpfr_init2 (s[i], 8);
  mpfr_init2 (sum, 8);

  RND_LOOP(r)
    {
      i = 0;
      while (1)
        {
          while (i < NS)
            {
              p[i] = c[0];
              mpfr_set_nan (s[i]);
              k[i++] = 0;
            }
          inex = mpfr_sum (sum, p, NS, (mpfr_rnd_t) r);
          if (! SAME_VAL (sum, s[NS-1]) || inex != 0)
            {
              printf ("Error in check_more_special on %s",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r));
              for (i = 0; i < NS; i++)
                printf (" %d", k[i]);
              printf (" with\n");
              for (i = 0; i < NS; i++)
                {
                  printf ("  p[%d] = %s = ", i, str[k[i]]);
                  mpfr_dump (p[i]);
                }
              printf ("Expected ");
              mpfr_dump (s[NS-1]);
              printf ("with inex = 0\n");
              printf ("Got      ");
              mpfr_dump (sum);
              printf ("with inex = %d\n", inex);
              exit (1);
            }
          while (k[--i] == NC-1)
            if (i == 0)
              goto next_rnd;
          p[i] = c[++k[i]];
          if (i == 0)
            mpfr_set (s[i], p[i], (mpfr_rnd_t) r);
          else
            mpfr_add (s[i], s[i-1], p[i], (mpfr_rnd_t) r);
          i++;
        }
    next_rnd: ;
    }

  for (i = 0; i < NC; i++)
    mpfr_clear (c[i]);
  for (i = 0; i < NS; i++)
    mpfr_clear (s[i]);
  mpfr_clear (sum);
}

/* i * 2^(46+h) + j * 2^(45+h) + k * 2^(44+h) + f * 2^(-2),
   with -1 <= i, j, k <= 1, i != 0, -3 <= f <= 3, and
   * prec set up so that ulp(exact sum) = 2^0, then
   * prec set up so that ulp(exact sum) = 2^(44+h) when possible,
     i.e. when prec >= MPFR_PREC_MIN.
   ------
   Some explanations:
   ulp(exact sum) = 2^q means EXP(exact sum) - prec = q where prec is
   the precision of the output. Thus ulp(exact sum) = 2^0 is achieved
   by setting prec = EXP(s3), where s3 is the exact sum (computed with
   mpfr_add's and sufficient precision). Then ulp(exact sum) = 2^(44+h)
   is achieved by subtracting 44+h from prec. The loop on prec does
   this. Since EXP(s3) <= 47+h, prec <= 3 at the second iteration,
   thus there will be at most 2 iterations. Whether a second iteration
   is done or not depends on EXP(s3), i.e. the values of the parameters,
   and the value of MPFR_PREC_MIN. */
static void
check1 (int h)
{
  mpfr_t sum1, sum2, s1, s2, s3, t[4];
  mpfr_ptr p[4];
  int i, j, k, f, prec, r, inex1, inex2;

  mpfr_init2 (sum1, 47 + h);
  mpfr_init2 (sum2, 47 + h);
  mpfr_init2 (s1, 3);
  mpfr_init2 (s2, 3);
  mpfr_init2 (s3, 49 + h);
  for (i = 0; i < 4; i++)
    {
      mpfr_init2 (t[i], 2);
      p[i] = t[i];
    }

  for (i = -1; i <= 1; i += 2)
    {
      mpfr_set_si_2exp (t[0], i, 46 + h, MPFR_RNDN);
      for (j = -1; j <= 1; j++)
        {
          mpfr_set_si_2exp (t[1], j, 45 + h, MPFR_RNDN);
          inex1 = mpfr_add (s1, t[0], t[1], MPFR_RNDN);
          MPFR_ASSERTN (inex1 == 0);
          for (k = -1; k <= 1; k++)
            {
              mpfr_set_si_2exp (t[2], k, 44 + h, MPFR_RNDN);
              inex1 = mpfr_add (s2, s1, t[2], MPFR_RNDN);
              MPFR_ASSERTN (inex1 == 0);
              for (f = -3; f <= 3; f++)
                {
                  mpfr_set_si_2exp (t[3], f, -2, MPFR_RNDN);
                  inex1 = mpfr_add (s3, s2, t[3], MPFR_RNDN);
                  MPFR_ASSERTN (inex1 == 0);
                  for (prec = mpfr_get_exp (s3);
                       prec >= MPFR_PREC_MIN;
                       prec -= 44 + h)
                    {
                      mpfr_set_prec (sum1, prec);
                      mpfr_set_prec (sum2, prec);
                      RND_LOOP_NO_RNDF (r)
                        {
                          inex1 = mpfr_set (sum1, s3, (mpfr_rnd_t) r);
                          inex2 = mpfr_sum (sum2, p, 4, (mpfr_rnd_t) r);
                          MPFR_ASSERTN (mpfr_check (sum1));
                          MPFR_ASSERTN (mpfr_check (sum2));
                          if (!(mpfr_equal_p (sum1, sum2) &&
                                SAME_SIGN (inex1, inex2)))
                            {
                              printf ("Error in check1 on %s, prec = %d, "
                                      "i = %d, j = %d, k = %d, f = %d, "
                                      "h = %d\n",
                                      mpfr_print_rnd_mode ((mpfr_rnd_t) r),
                                      prec, i, j, k, f, h);
                              printf ("Expected ");
                              mpfr_dump (sum1);
                              printf ("with inex = %d\n", inex1);
                              printf ("Got      ");
                              mpfr_dump (sum2);
                              printf ("with inex = %d\n", inex2);
                              exit (1);
                            }
                        }
                    }
                }
            }
        }
    }

  for (i = 0; i < 4; i++)
    mpfr_clear (t[i]);
  mpfr_clears (sum1, sum2, s1, s2, s3, (mpfr_ptr) 0);
}

/* With N = 2 * GMP_NUMB_BITS:
   i * 2^N + j + k * 2^(-1) + f1 * 2^(-N) + f2 * 2^(-N),
   with i = -1 or 1, j = 0 or i, -1 <= k <= 1, -1 <= f1 <= 1, -1 <= f2 <= 1
   ulp(exact sum) = 2^0. */
static void
check2 (void)
{
  mpfr_t sum1, sum2, s1, s2, s3, s4, t[5];
  mpfr_ptr p[5];
  int i, j, k, f1, f2, prec, r, inex1, inex2;

#define N (2 * GMP_NUMB_BITS)

  mpfr_init2 (sum1, N+1);
  mpfr_init2 (sum2, N+1);
  mpfr_init2 (s1, N+1);
  mpfr_init2 (s2, N+2);
  mpfr_init2 (s3, 2*N+1);
  mpfr_init2 (s4, 2*N+1);
  for (i = 0; i < 5; i++)
    {
      mpfr_init2 (t[i], 2);
      p[i] = t[i];
    }

  for (i = -1; i <= 1; i += 2)
    {
      mpfr_set_si_2exp (t[0], i, N, MPFR_RNDN);
      for (j = 0; j != 2*i; j += i)
        {
          mpfr_set_si (t[1], j, MPFR_RNDN);
          inex1 = mpfr_add (s1, t[0], t[1], MPFR_RNDN);
          MPFR_ASSERTN (inex1 == 0);
          for (k = -1; k <= 1; k++)
            {
              mpfr_set_si_2exp (t[2], k, -1, MPFR_RNDN);
              inex1 = mpfr_add (s2, s1, t[2], MPFR_RNDN);
              MPFR_ASSERTN (inex1 == 0);
              for (f1 = -1; f1 <= 1; f1++)
                {
                  mpfr_set_si_2exp (t[3], f1, -N, MPFR_RNDN);
                  inex1 = mpfr_add (s3, s2, t[3], MPFR_RNDN);
                  MPFR_ASSERTN (inex1 == 0);
                  for (f2 = -1; f2 <= 1; f2++)
                    {
                      mpfr_set_si_2exp (t[4], f2, -N, MPFR_RNDN);
                      inex1 = mpfr_add (s4, s3, t[4], MPFR_RNDN);
                      MPFR_ASSERTN (inex1 == 0);
                      prec = mpfr_get_exp (s4);
                      mpfr_set_prec (sum1, prec);
                      mpfr_set_prec (sum2, prec);
                      RND_LOOP_NO_RNDF (r)
                        {
                          inex1 = mpfr_set (sum1, s4, (mpfr_rnd_t) r);
                          inex2 = mpfr_sum (sum2, p, 5, (mpfr_rnd_t) r);
                          MPFR_ASSERTN (mpfr_check (sum1));
                          MPFR_ASSERTN (mpfr_check (sum2));
                          if (!(mpfr_equal_p (sum1, sum2) &&
                                SAME_SIGN (inex1, inex2)))
                            {
                              printf ("Error in check2 on %s, prec = %d, "
                                      "i = %d, j = %d, k = %d, f1 = %d, "
                                      "f2 = %d\n",
                                      mpfr_print_rnd_mode ((mpfr_rnd_t) r),
                                      prec, i, j, k, f1, f2);
                              printf ("Expected ");
                              mpfr_dump (sum1);
                              printf ("with inex = %d\n", inex1);
                              printf ("Got      ");
                              mpfr_dump (sum2);
                              printf ("with inex = %d\n", inex2);
                              exit (1);
                            }
                        }
                    }
                }
            }
        }
    }

  for (i = 0; i < 5; i++)
    mpfr_clear (t[i]);
  mpfr_clears (sum1, sum2, s1, s2, s3, s4, (mpfr_ptr) 0);
}

/* t[i] = (2^17 - 1) * 2^(17*(i-8)) for 0 <= i <= 16.
 * t[17] = 2^(17*9+1) * j for -4 <= j <= 4.
 * t[18] = 2^(-1) * k for -1 <= k <= 1.
 * t[19] = 2^(-17*8) * m for -3 <= m <= 3.
 * prec = MPFR_PREC_MIN and 17*9+4
 */
static void
check3 (void)
{
  mpfr_t sum1, sum2, s1, s2, s3, s4, t[20];
  mpfr_ptr p[20];
  mpfr_flags_t flags1, flags2;
  int i, s, j, k, m, q, r, inex1, inex2;
  int prec[2] = { MPFR_PREC_MIN, 17*9+4 };

  mpfr_init2 (s1, 17*17);
  mpfr_init2 (s2, 17*17+4);
  mpfr_init2 (s3, 17*17+4);
  mpfr_init2 (s4, 17*17+5);
  mpfr_set_ui (s1, 0, MPFR_RNDN);
  for (i = 0; i < 20; i++)
    {
      mpfr_init2 (t[i], 20);
      p[i] = t[i];
      if (i < 17)
        {
          mpfr_set_ui_2exp (t[i], 0x1ffff, 17*(i-8), MPFR_RNDN);
          inex1 = mpfr_add (s1, s1, t[i], MPFR_RNDN);
          MPFR_ASSERTN (inex1 == 0);
        }
    }

  for (s = 1; s >= -1; s -= 2)
    {
      for (j = -4; j <= 4; j++)
        {
          mpfr_set_si_2exp (t[17], j, 17*9+1, MPFR_RNDN);
          inex1 = mpfr_add (s2, s1, t[17], MPFR_RNDN);
          MPFR_ASSERTN (inex1 == 0);
          for (k = -1; k <= 1; k++)
            {
              mpfr_set_si_2exp (t[18], k, -1, MPFR_RNDN);
              inex1 = mpfr_add (s3, s2, t[18], MPFR_RNDN);
              MPFR_ASSERTN (inex1 == 0);
              for (m = -3; m <= 3; m++)
                {
                  mpfr_set_si_2exp (t[19], m, -17*8, MPFR_RNDN);
                  inex1 = mpfr_add (s4, s3, t[19], MPFR_RNDN);
                  MPFR_ASSERTN (inex1 == 0);
                  for (q = 0; q < 2; q++)
                    {
                      mpfr_inits2 (prec[q], sum1, sum2, (mpfr_ptr) 0);
                      RND_LOOP_NO_RNDF (r)
                        {
                          mpfr_clear_flags ();
                          inex1 = mpfr_set (sum1, s4, (mpfr_rnd_t) r);
                          flags1 = __gmpfr_flags;
                          mpfr_clear_flags ();
                          inex2 = mpfr_sum (sum2, p, 20, (mpfr_rnd_t) r);
                          flags2 = __gmpfr_flags;
                          MPFR_ASSERTN (mpfr_check (sum1));
                          MPFR_ASSERTN (mpfr_check (sum2));
                          if (!(mpfr_equal_p (sum1, sum2) &&
                                SAME_SIGN (inex1, inex2) &&
                                flags1 == flags2))
                            {
                              printf ("Error in check3 on %s, "
                                      "s = %d, j = %d, k = %d, m = %d\n",
                                      mpfr_print_rnd_mode ((mpfr_rnd_t) r),
                                      s, j, k, m);
                              printf ("Expected ");
                              mpfr_dump (sum1);
                              printf ("with inex = %d and flags =", inex1);
                              flags_out (flags1);
                              printf ("Got      ");
                              mpfr_dump (sum2);
                              printf ("with inex = %d and flags =", inex2);
                              flags_out (flags2);
                              exit (1);
                            }
                        }
                      mpfr_clears (sum1, sum2, (mpfr_ptr) 0);
                    }  /* q */
                }  /* m */
            }  /* k */
        }  /* j */
      for (i = 0; i < 17; i++)
        mpfr_neg (t[i], t[i], MPFR_RNDN);
      mpfr_neg (s1, s1, MPFR_RNDN);
    }  /* s */

  for (i = 0; i < 20; i++)
    mpfr_clear (t[i]);
  mpfr_clears (s1, s2, s3, s4, (mpfr_ptr) 0);
}

/* Test of s * (q * 2^(n-1) - 2^k) + h + i * 2^(-2) + j * 2^(-2)
 * with h = -1 or 1, -1 <= i odd <= j <= 3, 2 <= q <= 3, s = -1 or 1,
 * prec n-k.
 * On a 64-bit machine:
 * MPFR_RNDN, tmd=2, rbit=0, sst=0, negative is checked with the inputs
 *   -3*2^58, 2^5, -1, 2^(-2), 3*2^(-2)
 * MPFR_RNDN, tmd=2, rbit=0, sst=1, negative is checked with the inputs
 *   -3*2^58, 2^5, -1, 3*2^(-2), 3*2^(-2)
 *
 * Note: This test detects an error in a result when "sq + 3" is replaced
 * by "sq + 2" (11th argument of the first sum_raw invocation) and the
 * corresponding assertion d >= 3 is removed, confirming that one cannot
 * decrease this proved error bound.
 */
static void
check4 (void)
{
  mpfr_t sum1, sum2, s1, s2, s3, s4, t[5];
  mpfr_ptr p[5];
  int h, i, j, k, n, q, r, s, prec, inex1, inex2;

  mpfr_inits2 (257, sum1, sum2, s1, s2, s3, s4, (mpfr_ptr) 0);
  for (i = 0; i < 5; i++)
    {
      mpfr_init2 (t[i], 2);
      p[i] = t[i];
    }

  /* No GNU style for the many nested loops... */
  for (k = 1; k <= 64; k++) {
    mpfr_set_si_2exp (t[0], -1, k, MPFR_RNDN);
    for (n = k + MPFR_PREC_MIN; n <= k + 65; n++) {
      prec = n - k;
      mpfr_set_prec (sum1, prec);
      mpfr_set_prec (sum2, prec);
      for (q = 2; q <= 3; q++) {
        mpfr_set_si_2exp (t[1], q, n - 1, MPFR_RNDN);
        inex1 = mpfr_add (s1, t[0], t[1], MPFR_RNDN);
        MPFR_ASSERTN (inex1 == 0);
        for (s = -1; s <= 1; s += 2) {
          mpfr_neg (t[0], t[0], MPFR_RNDN);
          mpfr_neg (t[1], t[1], MPFR_RNDN);
          mpfr_neg (s1, s1, MPFR_RNDN);
          for (h = -1; h <= 1; h += 2) {
            mpfr_set_si (t[2], h, MPFR_RNDN);
            inex1 = mpfr_add (s2, s1, t[2], MPFR_RNDN);
            MPFR_ASSERTN (inex1 == 0);
            for (i = -1; i <= 3; i += 2) {
              mpfr_set_si_2exp (t[3], i, -2, MPFR_RNDN);
              inex1 = mpfr_add (s3, s2, t[3], MPFR_RNDN);
              MPFR_ASSERTN (inex1 == 0);
              for (j = i; j <= 3; j++) {
                mpfr_set_si_2exp (t[4], j, -2, MPFR_RNDN);
                inex1 = mpfr_add (s4, s3, t[4], MPFR_RNDN);
                MPFR_ASSERTN (inex1 == 0);
                RND_LOOP_NO_RNDF (r) {
                  inex1 = mpfr_set (sum1, s4, (mpfr_rnd_t) r);
                  inex2 = mpfr_sum (sum2, p, 5, (mpfr_rnd_t) r);
                  MPFR_ASSERTN (mpfr_check (sum1));
                  MPFR_ASSERTN (mpfr_check (sum2));
                  if (!(mpfr_equal_p (sum1, sum2) &&
                        SAME_SIGN (inex1, inex2)))
                    {
                      printf ("Error in check4 on %s, "
                              "k = %d, n = %d (prec %d), "
                              "q = %d, s = %d, h = %d, i = %d, j = %d\n",
                              mpfr_print_rnd_mode ((mpfr_rnd_t) r),
                              k, n, prec, q, s, h, i, j);
                      printf ("Expected ");
                      mpfr_dump (sum1);
                      printf ("with inex = %d\n", inex1);
                      printf ("Got      ");
                      mpfr_dump (sum2);
                      printf ("with inex = %d\n", inex2);
                      exit (1);
                    }
                }
              }
            }
          }
        }
      }
    }
  }

  for (i = 0; i < 5; i++)
    mpfr_clear (t[i]);
  mpfr_clears (sum1, sum2, s1, s2, s3, s4, (mpfr_ptr) 0);
}

/* bug reported by Joseph S. Myers on 2013-10-27
   https://sympa.inria.fr/sympa/arc/mpfr/2013-10/msg00015.html */
static void
bug20131027 (void)
{
  mpfr_t sum, t[4];
  mpfr_ptr p[4];
  const char *s[4] = {
    "0x1p1000",
    "-0x0.fffffffffffff80000000000000001p1000",
    "-0x1p947",
    "0x1p880"
  };
  int i, r;

  mpfr_init2 (sum, 53);

  for (i = 0; i < 4; i++)
    {
      mpfr_init2 (t[i], i == 0 ? 53 : 1000);
      mpfr_set_str (t[i], s[i], 0, MPFR_RNDN);
      p[i] = t[i];
    }

  RND_LOOP(r)
    {
      int expected_sign = (mpfr_rnd_t) r == MPFR_RNDD ? -1 : 1;
      int inex;

      inex = mpfr_sum (sum, p, 4, (mpfr_rnd_t) r);

      if (MPFR_NOTZERO (sum) || MPFR_SIGN (sum) != expected_sign || inex != 0)
        {
          printf ("mpfr_sum incorrect in bug20131027 for %s:\n"
                  "expected %c0 with inex = 0, got ",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r),
                  expected_sign > 0 ? '+' : '-');
          mpfr_dump (sum);
          printf ("with inex = %d\n", inex);
          exit (1);
        }
    }

  for (i = 0; i < 4; i++)
    mpfr_clear (t[i]);
  mpfr_clear (sum);
}

/* Occurs in branches/new-sum/src/sum.c@9344 on a 64-bit machine. */
static void
bug20150327 (void)
{
  mpfr_t sum1, sum2, t[3];
  mpfr_ptr p[3];
  const char *s[3] = {
    "0.10000111110101000010101011100001",
    "1E-100",
    "0.1E95"
  };
  int i, r;

  mpfr_inits2 (58, sum1, sum2, (mpfr_ptr) 0);

  for (i = 0; i < 3; i++)
    {
      mpfr_init2 (t[i], 64);
      mpfr_set_str (t[i], s[i], 2, MPFR_RNDN);
      p[i] = t[i];
    }

  RND_LOOP_NO_RNDF (r)
    {
      int inex1, inex2;

      mpfr_set (sum1, t[2], MPFR_RNDN);
      inex1 = -1;
      if (MPFR_IS_LIKE_RNDU ((mpfr_rnd_t) r, 1))
        {
          mpfr_nextabove (sum1);
          inex1 = 1;
        }

      inex2 = mpfr_sum (sum2, p, 3, (mpfr_rnd_t) r);

      if (!(mpfr_equal_p (sum1, sum2) && SAME_SIGN (inex1, inex2)))
        {
          printf ("mpfr_sum incorrect in bug20150327 for %s:\n",
                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          printf ("Expected ");
          mpfr_dump (sum1);
          printf ("with inex = %d\n", inex1);
          printf ("Got      ");
          mpfr_dump (sum2);
          printf ("with inex = %d\n", inex2);
          exit (1);
        }
    }

  for (i = 0; i < 3; i++)
    mpfr_clear (t[i]);
  mpfr_clears (sum1, sum2, (mpfr_ptr) 0);
}

/* TODO: A test with more inputs (but can't be compared to mpfr_add). */
static void
check_extreme (void)
{
  mpfr_t u, v, w, x, y;
  mpfr_ptr t[2];
  int i, inex1, inex2, r;

  t[0] = u;
  t[1] = v;

  mpfr_inits2 (32, u, v, w, x, y, (mpfr_ptr) 0);
  mpfr_setmin (u, mpfr_get_emax ());
  mpfr_setmax (v, mpfr_get_emin ());
  mpfr_setmin (w, mpfr_get_emax () - 40);
  RND_LOOP_NO_RNDF (r)
    for (i = 0; i < 2; i++)
      {
        mpfr_set_prec (x, 64);
        inex1 = mpfr_add (x, u, w, MPFR_RNDN);
        MPFR_ASSERTN (inex1 == 0);
        inex1 = mpfr_prec_round (x, 32, (mpfr_rnd_t) r);
        inex2 = mpfr_sum (y, t, 2, (mpfr_rnd_t) r);
        if (!(mpfr_equal_p (x, y) && SAME_SIGN (inex1, inex2)))
          {
            printf ("Error in check_extreme (%s, i = %d)\n",
                    mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
            printf ("Expected ");
            mpfr_dump (x);
            printf ("with inex = %d\n", inex1);
            printf ("Got      ");
            mpfr_dump (y);
            printf ("with inex = %d\n", inex2);
            exit (1);
          }
        mpfr_neg (v, v, MPFR_RNDN);
        mpfr_neg (w, w, MPFR_RNDN);
      }
  mpfr_clears (u, v, w, x, y, (mpfr_ptr) 0);
}

/* Generic random tests with cancellations.
 *
 * In summary, we do 4000 tests of the following form:
 * 1. We set the first MPFR_NCANCEL members of an array to random values,
 *    with a random exponent taken in 4 ranges, depending on the value of
 *    i % 4 (see code below).
 * 2. For each of the next MPFR_NCANCEL iterations:
 *    A. we randomly permute some terms of the array (to make sure that a
 *       particular order doesn't have an influence on the result);
 *    B. we compute the sum in a random rounding mode;
 *    C. if this sum is zero, we end the current test (there is no longer
 *       anything interesting to test);
 *    D. we check that this sum is below some bound (chosen as infinite
 *       for the first iteration of (2), i.e. this test will be useful
 *       only for the next iterations, after cancellations);
 *    E. we put the opposite of this sum in the array, the goal being to
 *       introduce a chain of cancellations;
 *    F. we compute the bound for the next iteration, derived from (E).
 * 3. We do another iteration like (2), but with reusing a random element
 *    of the array. This last test allows one to check the support of
 *    reused arguments. Before this support (r10467), it triggers an
 *    assertion failure with (almost?) all seeds, and if assertions are
 *    not checked, tsum fails in most cases but not all.
 */
static void
cancel (void)
{
  mpfr_t x[2 * MPFR_NCANCEL], bound;
  mpfr_ptr px[2 * MPFR_NCANCEL];
  int i, j, k, n;

  mpfr_init2 (bound, 2);

  /* With 4000 tests, tsum will fail in most cases without support of
     reused arguments (before r10467). */
  for (i = 0; i < 4000; i++)
    {
      mpfr_set_inf (bound, 1);
      for (n = 0; n <= numberof (x); n++)
        {
          mpfr_prec_t p;
          mpfr_rnd_t rnd;

          if (n < numberof (x))
            {
              px[n] = x[n];
              p = MPFR_PREC_MIN + (randlimb () % 256);
              mpfr_init2 (x[n], p);
              k = n;
            }
          else
            {
              /* Reuse of a random member of the array. */
              k = randlimb () % n;
            }

          if (n < MPFR_NCANCEL)
            {
              mpfr_exp_t e;

              MPFR_ASSERTN (n < numberof (x));
              e = (i & 1) ? 0 : mpfr_get_emin ();
              tests_default_random (x[n], 256, e,
                                    ((i & 2) ? e + 2000 : mpfr_get_emax ()),
                                    0);
            }
          else
            {
              /* random permutation with n random transpositions */
              for (j = 0; j < n; j++)
                {
                  int k1, k2;

                  k1 = randlimb () % (n-1);
                  k2 = randlimb () % (n-1);
                  mpfr_swap (x[k1], x[k2]);
                }

              rnd = RND_RAND ();

#if MPFR_DEBUG
              printf ("mpfr_sum cancellation test\n");
              for (j = 0; j < n; j++)
                {
                  printf ("  x%d[%3ld] = ", j, mpfr_get_prec(x[j]));
                  mpfr_out_str (stdout, 16, 0, x[j], MPFR_RNDN);
                  printf ("\n");
                }
              printf ("  rnd = %s, output prec = %ld\n",
                      mpfr_print_rnd_mode (rnd), mpfr_get_prec (x[n]));
#endif

              mpfr_sum (x[k], px, n, rnd);

              if (mpfr_zero_p (x[k]))
                {
                  if (k == n)
                    n++;
                  break;
                }

              if (mpfr_cmpabs (x[k], bound) > 0)
                {
                  printf ("Error in cancel on i = %d, n = %d\n", i, n);
                  printf ("Expected bound: ");
                  mpfr_dump (bound);
                  printf ("x[%d]: ", k);
                  mpfr_dump (x[k]);
                  exit (1);
                }

              if (k != n)
                break;

              /* For the bound, use MPFR_RNDU due to possible underflow.
                 It would be nice to add some specific underflow checks,
                 though there are already ones in check_underflow(). */
              mpfr_set_ui_2exp (bound, 1,
                                mpfr_get_exp (x[n]) - p - (rnd == MPFR_RNDN),
                                MPFR_RNDU);
              /* The next sum will be <= bound in absolute value
                 (the equality can be obtained in all rounding modes
                 since the sum will be rounded). */

              mpfr_neg (x[n], x[n], MPFR_RNDN);
            }
        }

      while (--n >= 0)
        mpfr_clear (x[n]);
    }

  mpfr_clear (bound);
}

#ifndef NOVFL
# define NOVFL 30
#endif

static void
check_overflow (void)
{
  mpfr_t sum1, sum2, x, y;
  mpfr_ptr t[2 * NOVFL];
  mpfr_exp_t emin, emax;
  int i, r;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();
  set_emin (MPFR_EMIN_MIN);
  set_emax (MPFR_EMAX_MAX);

  mpfr_inits2 (32, sum1, sum2, x, y, (mpfr_ptr) 0);
  mpfr_setmax (x, mpfr_get_emax ());
  mpfr_neg (y, x, MPFR_RNDN);

  for (i = 0; i < 2 * NOVFL; i++)
    t[i] = i < NOVFL ? x : y;

  /* Two kinds of test:
   *   i = 1: overflow.
   *   i = 2: intermediate overflow (exact sum is 0).
   */
  for (i = 1; i <= 2; i++)
    RND_LOOP(r)
      {
        int inex1, inex2;

        inex1 = mpfr_add (sum1, x, i == 1 ? x : y, (mpfr_rnd_t) r);
        inex2 = mpfr_sum (sum2, t, i * NOVFL, (mpfr_rnd_t) r);
        MPFR_ASSERTN (mpfr_check (sum1));
        MPFR_ASSERTN (mpfr_check (sum2));
        if (!(mpfr_equal_p (sum1, sum2) && SAME_SIGN (inex1, inex2)))
          {
            printf ("Error in check_overflow on %s, i = %d\n",
                    mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
            printf ("Expected ");
            mpfr_dump (sum1);
            printf ("with inex = %d\n", inex1);
            printf ("Got      ");
            mpfr_dump (sum2);
            printf ("with inex = %d\n", inex2);
            exit (1);
          }
      }

  mpfr_clears (sum1, sum2, x, y, (mpfr_ptr) 0);

  set_emin (emin);
  set_emax (emax);
}

#ifndef NUNFL
# define NUNFL 9
#endif

static void
check_underflow (void)
{
  mpfr_t sum1, sum2, t[NUNFL];
  mpfr_ptr p[NUNFL];
  mpfr_prec_t precmax = 444;
  mpfr_exp_t emin, emax;
  unsigned int ex_flags, flags;
  int c, i;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();
  set_emin (MPFR_EMIN_MIN);
  set_emax (MPFR_EMAX_MAX);

  ex_flags = MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT;

  mpfr_init2 (sum1, MPFR_PREC_MIN);
  mpfr_init2 (sum2, precmax);

  for (i = 0; i < NUNFL; i++)
    {
      mpfr_init2 (t[i], precmax);
      p[i] = t[i];
    }

  for (c = 0; c < 8; c++)
    {
      mpfr_prec_t fprec;
      int n, neg, r;

      fprec = MPFR_PREC_MIN + (randlimb () % (precmax - MPFR_PREC_MIN + 1));
      n = 3 + (randlimb () % (NUNFL - 2));
      MPFR_ASSERTN (n <= NUNFL);

      mpfr_set_prec (sum2, RAND_BOOL () ? MPFR_PREC_MIN : precmax);
      mpfr_set_prec (t[0], fprec + 64);
      mpfr_set_zero (t[0], 1);

      for (i = 1; i < n; i++)
        {
          int inex;

          mpfr_set_prec (t[i], MPFR_PREC_MIN +
                         (randlimb () % (fprec - MPFR_PREC_MIN + 1)));
          do
            mpfr_urandomb (t[i], RANDS);
          while (MPFR_IS_ZERO (t[i]));
          mpfr_set_exp (t[i], MPFR_EMIN_MIN);
          inex = mpfr_sub (t[0], t[0], t[i], MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
        }

      neg = RAND_BOOL ();
      if (neg)
        mpfr_nextbelow (t[0]);
      else
        mpfr_nextabove (t[0]);

      RND_LOOP(r)
        {
          int inex1, inex2;

          mpfr_set_zero (sum1, 1);
          if (neg)
            mpfr_nextbelow (sum1);
          else
            mpfr_nextabove (sum1);
          inex1 = mpfr_div_2ui (sum1, sum1, 2, (mpfr_rnd_t) r);

          mpfr_clear_flags ();
          inex2 = mpfr_sum (sum2, p, n, (mpfr_rnd_t) r);
          flags = __gmpfr_flags;

          MPFR_ASSERTN (mpfr_check (sum1));
          MPFR_ASSERTN (mpfr_check (sum2));

          if (flags != ex_flags)
            {
              printf ("Bad flags in check_underflow on %s, c = %d\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r), c);
              printf ("Expected flags:");
              flags_out (ex_flags);
              printf ("Got flags:     ");
              flags_out (flags);
              printf ("sum = ");
              mpfr_dump (sum2);
              exit (1);
            }

          if (!(mpfr_equal_p (sum1, sum2) && SAME_SIGN (inex1, inex2)))
            {
              printf ("Error in check_underflow on %s, c = %d\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r), c);
              printf ("Expected ");
              mpfr_dump (sum1);
              printf ("with inex = %d\n", inex1);
              printf ("Got      ");
              mpfr_dump (sum2);
              printf ("with inex = %d\n", inex2);
              exit (1);
            }
        }
    }

  for (i = 0; i < NUNFL; i++)
    mpfr_clear (t[i]);
  mpfr_clears (sum1, sum2, (mpfr_ptr) 0);

  set_emin (emin);
  set_emax (emax);
}

static void
check_coverage (void)
{
#ifdef MPFR_COV_CHECK
  int r, i, j, k, p, q;
  int err = 0;

  RND_LOOP_NO_RNDF (r)
    for (i = 0; i < 1 + ((mpfr_rnd_t) r == MPFR_RNDN); i++)
      for (j = 0; j < 2; j++)
        for (k = 0; k < 3; k++)
          for (p = 0; p < 2; p++)
            for (q = 0; q < 2; q++)
              if (!__gmpfr_cov_sum_tmd[r][i][j][k][p][q])
                {
                  printf ("TMD not tested on %s, tmd=%d, rbit=%d, sst=%d,"
                          " %s, sq %s MPFR_PREC_MIN\n",
                          mpfr_print_rnd_mode ((mpfr_rnd_t) r), i+1, j, k-1,
                          p ? "pos" : "neg", q ? ">" : "==");
                  err = 1;
                }

  if (err)
    exit (1);
#endif
}

static int
mpfr_sum_naive (mpfr_ptr s, mpfr_t *x, int n, mpfr_rnd_t rnd)
{
  int ret, i;
  switch (n)
    {
    case 0:
      return mpfr_set_ui (s, 0, rnd);
    case 1:
      return mpfr_set (s, x[0], rnd);
    default:
      ret = mpfr_add (s, x[0], x[1], rnd);
      for (i = 2; i < n; i++)
        ret = mpfr_add (s, s, x[i], rnd);
      return ret;
    }
}

/* check adding n random numbers, iterated k times */
static void
check_random (int n, int k, mpfr_prec_t prec, mpfr_rnd_t rnd)
{
  mpfr_t *x, s, ref_s;
  mpfr_ptr *y;
  int i, st, ret = 0, ref_ret = 0;
  gmp_randstate_t state;

  gmp_randinit_default (state);
  mpfr_init2 (s, prec);
  mpfr_init2 (ref_s, prec);
  x = (mpfr_t *) tests_allocate (n * sizeof (mpfr_t));
  y = (mpfr_ptr *) tests_allocate (n * sizeof (mpfr_ptr));
  for (i = 0; i < n; i++)
    {
      y[i] = x[i];
      mpfr_init2 (x[i], prec);
      mpfr_urandom (x[i], state, rnd);
    }

  st = cputime ();
  for (i = 0; i < k; i++)
    ref_ret = mpfr_sum_naive (ref_s, x, n, rnd);
  printf ("mpfr_sum_naive took %dms\n", cputime () - st);

  st = cputime ();
  for (i = 0; i < k; i++)
    ret = mpfr_sum (s, y, n, rnd);
  printf ("mpfr_sum took %dms\n", cputime () - st);

  if (n <= 2)
    {
      MPFR_ASSERTN (mpfr_cmp (ref_s, s) == 0);
      MPFR_ASSERTN (ref_ret == ret);
    }

  for (i = 0; i < n; i++)
    mpfr_clear (x[i]);
  tests_free (x, n * sizeof (mpfr_t));
  tests_free (y, n * sizeof (mpfr_ptr));
  mpfr_clear (s);
  mpfr_clear (ref_s);
  gmp_randclear (state);
}

/* This bug appears when porting sum.c for MPFR 3.1.4 (0.11E826 is returned),
   but does not appear in the trunk. It was due to buggy MPFR_IS_LIKE_RNDD
   and MPFR_IS_LIKE_RNDU macros. The fix was done in r9295 in the trunk and
   it has been merged in r10234 in the 3.1 branch. Note: the bug would have
   been caught by generic_tests anyway, but a simple testcase is easier for
   checking with log messages (MPFR_LOG_ALL=1). */
static void
bug20160315 (void)
{
  mpfr_t r, t[4];
  mpfr_ptr p[4];
  const char *s[4] = { "0.10E20", "0", "0.11E382", "0.10E826" };
  int i;

  mpfr_init2 (r, 2);
  for (i = 0; i < 4; i++)
    {
      mpfr_init2 (t[i], 2);
      mpfr_set_str_binary (t[i], s[i]);
      p[i] = t[i];
    }
  mpfr_sum (r, p, 4, MPFR_RNDN);
  if (! mpfr_equal_p (r, t[3]))
    {
      printf ("Error in bug20160315.\n");
      printf ("Expected ");
      mpfr_dump (t[3]);
      printf ("Got      ");
      mpfr_dump (r);
      exit (1);
    }
  for (i = 0; i < 4; i++)
    mpfr_clear (t[i]);
  mpfr_clear (r);
}

int
main (int argc, char *argv[])
{
  int h;

  if (argc == 5)
    {
      check_random (atoi (argv[1]), atoi (argv[2]), atoi (argv[3]),
                    (mpfr_rnd_t) atoi (argv[4]));
      return 0;
    }

  tests_start_mpfr ();

  if (argc != 1)
    {
      fprintf (stderr, "Usage: tsum\n       tsum n k prec rnd\n");
      exit (1);
    }

  check_simple ();
  check_special ();
  check_more_special ();
  for (h = 0; h <= 64; h++)
    check1 (h);
  check2 ();
  check3 ();
  check4 ();
  bug20131027 ();
  bug20150327 ();
  bug20160315 ();
  generic_tests ();
  check_extreme ();
  cancel ();
  check_overflow ();
  check_underflow ();

  check_coverage ();
  tests_end_mpfr ();
  return 0;
}
