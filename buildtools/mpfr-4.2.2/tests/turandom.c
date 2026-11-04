/* Test file for mpfr_urandom

Copyright 1999-2004, 2006-2025 Free Software Foundation, Inc.
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

static void
test_urandom (long nbtests, mpfr_prec_t prec, mpfr_rnd_t rnd, long bit_index,
              int verbose)
{
  mpfr_t x;
  int *tab, size_tab, k, sh, xn;
  double d, av = 0, var = 0, chi2 = 0, th;
  mpfr_exp_t emin;
  mp_size_t limb_index = 0;
  mp_limb_t limb_mask = 0;
  long count = 0;
  int i;
  int inex = 1;
  mpfr_flags_t ex_flags, flags;

  size_tab = (nbtests >= 1000 ? nbtests / 50 : 20);
  tab = (int *) tests_allocate (size_tab * sizeof (int));
  for (k = 0; k < size_tab; k++)
    tab[k] = 0;

  mpfr_init2 (x, prec);
  xn = 1 + (prec - 1) / mp_bits_per_limb;
  sh = xn * mp_bits_per_limb - prec;
  if (bit_index >= 0 && bit_index < prec)
    {
      /* compute the limb index and limb mask to fetch the bit #bit_index */
      limb_index = (prec - bit_index) / mp_bits_per_limb;
      i = 1 + bit_index - (bit_index / mp_bits_per_limb) * mp_bits_per_limb;
      limb_mask = MPFR_LIMB_ONE << (mp_bits_per_limb - i);
    }

  for (k = 0; k < nbtests; k++)
    {
      mpfr_clear_flags ();
      ex_flags = MPFR_FLAGS_INEXACT;
      i = mpfr_urandom (x, RANDS, rnd);
      flags = __gmpfr_flags;
      inex = (i != 0) && inex;
      /* check that lower bits are zero */
      if (MPFR_MANT(x)[0] & MPFR_LIMB_MASK(sh) && !MPFR_IS_ZERO (x))
        {
          printf ("Error: mpfr_urandom() returns invalid numbers:\n");
          mpfr_dump (x);
          exit (1);
        }
      /* check that the value is in [0,1] */
      if (mpfr_cmp_ui (x, 0) < 0 || mpfr_cmp_ui (x, 1) > 0)
        {
          printf ("Error: mpfr_urandom() returns number outside [0, 1]:\n");
          mpfr_dump (x);
          exit (1);
        }
      /* check the flags (an underflow is theoretically possible, but
         impossible in practice due to the huge exponent range) */
      if (flags != ex_flags)
        {
          printf ("Error: mpfr_urandom() returns incorrect flags.\n");
          printf ("Expected ");
          flags_out (ex_flags);
          printf ("Got      ");
          flags_out (flags);
          exit (1);
        }

      d = mpfr_get_d1 (x);
      av += d;
      var += d*d;
      i = (int) (size_tab * d);
      if (d == 1.0)
        i--;
      MPFR_ASSERTN (i < size_tab);
      tab[i]++;

      if (limb_mask && (MPFR_MANT (x)[limb_index] & limb_mask))
        count ++;
    }

  if (inex == 0)
    {
      /* one call in the loop pretended to return an exact number! */
      printf ("Error: mpfr_urandom() returns a zero ternary value.\n");
      exit (1);
    }

  /* coverage test */
  emin = mpfr_get_emin ();
  for (k = 0; k < 5; k++)
    {
      set_emin (k+1);
      ex_flags = MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT;
      for (i = 0; i < 5; i++)
        {
          mpfr_clear_flags ();
          inex = mpfr_urandom (x, RANDS, rnd);
          flags = __gmpfr_flags;
          if (k > 0 && flags != ex_flags)
            {
              printf ("Error: mpfr_urandom() returns incorrect flags"
                      " for emin = %d (i = %d).\n", k+1, i);
              printf ("Expected ");
              flags_out (ex_flags);
              printf ("Got      ");
              flags_out (flags);
              exit (1);
            }
          if ((   (rnd == MPFR_RNDZ || rnd == MPFR_RNDD)
                  && (!MPFR_IS_ZERO (x) || inex != -1))
              || ((rnd == MPFR_RNDU || rnd == MPFR_RNDA)
                  && (mpfr_cmp_ui (x, 1 << k) != 0 || inex != +1))
              || (rnd == MPFR_RNDN
                  && (k > 0 || mpfr_cmp_ui (x, 1 << k) != 0 || inex != +1)
                  && (!MPFR_IS_ZERO (x) || inex != -1)))
            {
              printf ("Error: mpfr_urandom() does not handle correctly"
                      " a restricted exponent range.\nemin = %d\n"
                      "rounding mode: %s\nternary value: %d\nrandom value: ",
                      k+1, mpfr_print_rnd_mode (rnd), inex);
              mpfr_dump (x);
              exit (1);
            }
        }
    }
  set_emin (emin);

  mpfr_clear (x);

  if (verbose)
    {
      av /= nbtests;
      var = (var / nbtests) - av * av;

      th = (double)nbtests / size_tab;
      printf ("Average = %.5f\nVariance = %.5f\n", av, var);
      printf ("Repartition for urandom with rounding mode %s. "
              "Each integer should be close to %d.\n",
              mpfr_print_rnd_mode (rnd), (int) th);

      for (k = 0; k < size_tab; k++)
        {
          chi2 += (tab[k] - th) * (tab[k] - th) / th;
          printf("%d ", tab[k]);
          if (((unsigned int) (k+1) & 7) == 0)
            printf("\n");
        }

      printf("\nChi2 statistics value (with %d degrees of freedom) : %.5f\n",
             size_tab - 1, chi2);

      if (limb_mask)
        printf ("Bit #%ld is set %ld/%ld = %.1f %% of time\n",
                bit_index, count, nbtests, count * 100.0 / nbtests);

      puts ("");
    }

  tests_free (tab, size_tab * sizeof (int));
  return;
}

static void
underflow_tests (void)
{
  mpfr_t x;
  mpfr_exp_t emin;
  int i, k;
  int rnd;

  emin = mpfr_get_emin ();
  mpfr_init2 (x, 4);

  for (i = 2; i >= -4; i--)
    RND_LOOP (rnd)
      for (k = 0; k < 100; k++)
        {
          mpfr_flags_t ex_flags, flags;
          int inex;

          if (i >= 2)
            {
              /* Always underflow when emin >= 2, i.e. when the minimum
                 representable positive number is >= 2. */
              ex_flags = MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT;
            }
          else
            {
#ifndef MPFR_USE_MINI_GMP
              gmp_randstate_t s;

              /* Since the unrounded random number does not depend on
                 the current exponent range, we can detect underflow
                 in a range larger than the one that will be tested. */
              gmp_randinit_set (s, mpfr_rands);
              mpfr_clear_flags ();
              mpfr_urandom (x, s, (mpfr_rnd_t) rnd);
              gmp_randclear (s);
              ex_flags = MPFR_FLAGS_INEXACT;
              if (MPFR_IS_ZERO (x) || mpfr_get_exp (x) < i)
                ex_flags |= MPFR_FLAGS_UNDERFLOW;
#else
              /* Do not test the flags. */
              ex_flags = 0;
#endif
            }

          set_emin (i);
          mpfr_clear_flags ();
          inex = mpfr_urandom (x, mpfr_rands, (mpfr_rnd_t) rnd);
          flags = __gmpfr_flags;
          MPFR_ASSERTN (mpfr_inexflag_p ());
          set_emin (emin);

          if (MPFR_IS_NEG (x))
            {
              printf ("Error in underflow_tests: got a negative sign"
                      " for i=%d rnd=%s k=%d.\n",
                      i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), k);
              exit (1);
            }

          if (MPFR_IS_ZERO (x))
            {
              if (rnd == MPFR_RNDU || rnd == MPFR_RNDA)
                {
                  printf ("Error in underflow_tests: the value cannot"
                          " be 0 for i=%d rnd=%s k=%d.\n",
                          i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), k);
                  exit (1);
                }
            }

          if (inex == 0 || (MPFR_IS_ZERO (x) && inex > 0))
            {
              printf ("Error in underflow_tests: incorrect inex (%d)"
                      " for i=%d rnd=%s k=%d.\n", inex,
                      i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), k);
              exit (1);
            }

          if (ex_flags != 0 && flags != ex_flags)
            {
              printf ("Error in underflow_tests: incorrect flags"
                      " for i=%d rnd=%s k=%d.\n",
                      i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), k);
              printf ("Expected ");
              flags_out (ex_flags);
              printf ("Got      ");
              flags_out (flags);
              exit (1);
            }
        }

  mpfr_clear (x);
}

static void
test_underflow (int verbose)
{
  mpfr_t x;
  mpfr_exp_t emin = mpfr_get_emin ();
  long i, exp[6] = {0, 0, 0, 0, 0, 0};

  mpfr_init2 (x, 2);
  set_emin (-3);
#define N 1000000
  for (i = 0; i < N; i++)
    {
      mpfr_urandom (x, RANDS, MPFR_RNDN);
      if (mpfr_zero_p (x))
        exp[5] ++;
      else
        /* exp=1 is possible if the generated number is 0.111111... */
        exp[1-mpfr_get_exp(x)] ++;
    }
  if (verbose)
    printf ("exp=1:%.3f(%.3f) 0:%.3f(%.3f) -1:%.3f(%.3f) -2:%.3f(%.3f) "
            "-3:%.3f(%.3f) zero:%.3f(%.3f)\n",
            100.0 * (double) exp[0] / (double) N, 12.5,
            100.0 * (double) exp[1] / (double) N, 43.75,
            100.0 * (double) exp[2] / (double) N, 21.875,
            100.0 * (double) exp[3] / (double) N, 10.9375,
            100.0 * (double) exp[4] / (double) N, 7.8125,
            100.0 * (double) exp[5] / (double) N, 3.125);
  mpfr_clear (x);
  set_emin (emin);
#undef N
}

static void
overflow_tests (void)
{
  mpfr_t x;
  mpfr_exp_t emax;
  int i, k;
  int inex;
  int rnd;
  mpfr_flags_t ex_flags, flags;

  emax = mpfr_get_emax ();
  mpfr_init2 (x, 4);
  ex_flags = MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT; /* if overflow */
  for (i = -4; i <= 0; i++)
    {
      set_emax (i);
      RND_LOOP (rnd)
        for (k = 0; k < 100; k++)
          {
            mpfr_clear_flags ();
            inex = mpfr_urandom (x, mpfr_rands, (mpfr_rnd_t) rnd);
            flags = __gmpfr_flags;
            MPFR_ASSERTN (mpfr_inexflag_p ());
            if (MPFR_IS_NEG (x))
              {
                printf ("Error in overflow_tests: got a negative sign"
                        " for i=%d rnd=%s k=%d.\n",
                        i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), k);
                exit (1);
              }
            if (MPFR_IS_INF (x))
              {
                if (rnd == MPFR_RNDD || rnd == MPFR_RNDZ)
                  {
                    printf ("Error in overflow_tests: the value cannot"
                            " be +inf for i=%d rnd=%s k=%d.\n",
                            i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), k);
                    exit (1);
                  }
                if (flags != ex_flags)
                  {
                    printf ("Error in overflow_tests: incorrect flags"
                            " for i=%d rnd=%s k=%d.\n",
                            i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), k);
                    printf ("Expected ");
                    flags_out (ex_flags);
                    printf ("Got      ");
                    flags_out (flags);
                    exit (1);
                  }
              }
            if (inex == 0 || (MPFR_IS_INF (x) && inex < 0))
              {
                printf ("Error in overflow_tests: incorrect inex (%d)"
                        " for i=%d rnd=%s k=%d.\n", inex,
                        i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), k);
                exit (1);
              }
          }
    }
  mpfr_clear (x);
  set_emax (emax);
}

#ifndef MPFR_USE_MINI_GMP

/* Problem reported by Carl Witty. This test assumes the random generator
   used by GMP is deterministic (for a given seed). We need to distinguish
   two cases since the random generator changed in GMP 4.2.0. */
static void
bug20100914 (void)
{
  mpfr_t x;
  gmp_randstate_t s;

#if __MPFR_GMP(4,2,0)
# define C1 "0.8488312"
# define C2 "0.8156509"
#else
# define C1 "0.6485367"
# define C2 "0.9362717"
#endif

  gmp_randinit_default (s);
  gmp_randseed_ui (s, 42);
  mpfr_init2 (x, 17);
  mpfr_urandom (x, s, MPFR_RNDN);
  if (mpfr_cmp_str1 (x, C1) != 0)
    {
      printf ("Error in bug20100914, expected " C1 ", got ");
      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  mpfr_urandom (x, s, MPFR_RNDN);
  if (mpfr_cmp_str1 (x, C2) != 0)
    {
      printf ("Error in bug20100914, expected " C2 ", got ");
      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  mpfr_clear (x);
  gmp_randclear (s);
}

/* non-regression test for bug reported by Trevor Spiteri
   https://sympa.inria.fr/sympa/arc/mpfr/2017-01/msg00020.html */
static void
bug20170123 (void)
{
#if __MPFR_GMP(4,2,0)
  mpfr_t x;
  mpfr_exp_t emin;
  gmp_randstate_t s;

  emin = mpfr_get_emin ();
  set_emin (-7);
  mpfr_init2 (x, 53);
  gmp_randinit_default (s);
  gmp_randseed_ui (s, 398);
  mpfr_urandom (x, s, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (x, 1, -8) == 0);
  mpfr_clear (x);
  gmp_randclear (s);
  set_emin (emin);
#endif
}

/* Reproducibility test with several rounding modes and exponent ranges. */
static void
reprod_rnd_exp (void)
{
  int i;

  for (i = 0; i < 10; i++)
    {
      gmp_randstate_t s1;
      mpfr_prec_t prec;
      mpfr_t x1, x2, y;
      mp_limb_t v;
      int r;

      prec = MPFR_PREC_MIN + (randlimb () % 200);
      mpfr_inits2 (prec, x1, x2, y, (mpfr_ptr) 0);

      gmp_randinit_set (s1, mpfr_rands);
      mpfr_urandom (x1, mpfr_rands, MPFR_RNDZ);
      mpfr_rand_raw (&v, mpfr_rands, GMP_NUMB_BITS);
      mpfr_set (x2, x1, MPFR_RNDN);
      mpfr_nextabove (x2);
      /* The real number is between x1 and x2. */

      RND_LOOP (r)
        {
          gmp_randstate_t s2;
          mpfr_rnd_t rr = (mpfr_rnd_t) r;
          mp_limb_t w;
          mpfr_ptr t[2];
          int k, nk = 0;

          gmp_randinit_set (s2, s1);
          mpfr_urandom (y, s2, rr);
          mpfr_rand_raw (&w, s2, GMP_NUMB_BITS);
          if (w != v)
            {
              printf ("Error in reprod_rnd_exp for i=%d rnd=%s: different "
                      "PRNG state\n", i, mpfr_print_rnd_mode (rr));
              exit (1);
            }

          if (! MPFR_IS_LIKE_RNDA (rr, 0))
            t[nk++] = x1;
          if (! MPFR_IS_LIKE_RNDZ (rr, 0))
            t[nk++] = x2;
          MPFR_ASSERTN (nk == 1 || nk == 2);

          if (!(mpfr_equal_p (y, t[0]) || (nk > 1 && mpfr_equal_p (y, t[1]))))
            {
              printf ("Error in reprod_rnd_exp for i=%d rnd=%s:\n",
                      i, mpfr_print_rnd_mode (rr));
              printf ("Expected%s\n", nk > 1 ? " one of" : "");
              for (k = 0; k < nk; k++)
                {
                  printf ("  ");
                  mpfr_dump (t[k]);
                }
              printf ("Got\n  ");
              mpfr_dump (y);
              exit (1);
            }

          gmp_randclear (s2);
        }

      mpfr_clears (x1, x2, y, (mpfr_ptr) 0);
      gmp_randclear (s1);
    }
}

/* Reproducibility test: check that the behavior does not depend on
   the platform ABI or MPFR version (new, incompatible MPFR versions
   may introduce changes, in which case the hardcoded values should
   depend on MPFR_VERSION).
   It is not necessary to test with different rounding modes and
   exponent ranges as this has already been done in reprod_rnd_exp.
   We do not need to check the status of the PRNG after mpfr_urandom
   since this is done implicitly by comparing the next value, except
   for the last itaration.
*/
static void
reprod_abi (void)
{
#if __MPFR_GMP(4,2,0)
#define N 6
  /* Run this program with the MPFR_REPROD_ABI_OUTPUT environment variable
     set to get the array of strings. */
  const char *t[5 * N] = {
    "1.0@-1",
    "3.0@-1",
    "7.0@-1",
    "9.0@-1",
    "c.0@-1",
    "4.385434c0@-1",
    "1.9a018734@-1",
    "8.26547780@-1",
    "a.fd334198@-1",
    "9.aa11d5f00@-1",
    "d.aa9a32fd0a801ac0@-1",
    "c.eb47074368ec6340@-1",
    "9.7dbe2ced88ae4c30@-1",
    "d.03218ea6704a42c0@-1",
    "7.1530156aac800d980@-1",
    "e.270121b1d74aea9029ccc740@-1",
    "5.614fc2d9ca3917107609e2e0@-1",
    "5.15417c51af272232181d6a40@-1",
    "f.dfb431dd6533c004b6d3c590@-1",
    "4.345f96fd2929d41eb278a4f40@-1",
    "a.804590c6449cd8c83bae31f31f7a4100@-1",
    "a.0a2b318d3c99911a45e4cf33847d3680@-1",
    "2.89f6127c19092d7a1808b1842b296400@-1",
    "2.1db4fc00348ca1531983fe4bd4cdf6d2@-1",
    "5.2d90f11ed710425ebe549a95decbb6540@-1",
    "8.ca35d1302cf369e03c2a58bf2ce5cff8307f0bc0@-1",
    "3.a22bae632c32f2a7a67a1fa78a93f5e84f9caa40@-1",
    "f.370a36febed972dbb47f2503f7e08a651edbf120@-1",
    "d.0764d7a38c206eeba6ffe8cf39d777194f5c9200@-1",
    "a.1a312f0bb16db20c4783c6438725ed5d6dff6af80@-1"
  };
  gmp_randstate_t s;
  int generate, i;

  /* We must hardcode the seed to be able to compare with hardcoded values. */
  gmp_randinit_default (s);
  gmp_randseed_ui (s, 17);

  generate = getenv ("MPFR_REPROD_ABI_OUTPUT") != NULL;

  for (i = 0; i < 5 * N; i++)
    {
      mpfr_prec_t prec;
      mpfr_t x;

      prec = i < 5 ? MPFR_PREC_MIN + i : (i / 5) * 32 + (i % 5) - 2;
      mpfr_init2 (x, prec);
      mpfr_urandom (x, s, MPFR_RNDN);
      if (generate)
        {
          printf ("    \"");
          mpfr_out_str (stdout, 16, 0, x, MPFR_RNDZ);
          printf (i < 5 * N - 1 ? "\",\n" : "\"\n");
        }
      else if (mpfr_cmp_str (x, t[i], 16, MPFR_RNDN) != 0)
        {
          printf ("Error in reprod_abi for i=%d\n", i);
          printf ("Expected %s\n", t[i]);
          printf ("Got      ");
          mpfr_out_str (stdout, 16, 0, x, MPFR_RNDZ);
          printf ("\n");
          exit (1);
        }
      mpfr_clear (x);
    }

  gmp_randclear (s);
#endif
}

#endif

int
main (int argc, char *argv[])
{
  long nbtests;
  mpfr_prec_t prec;
  int verbose = 0;
  int rnd;
  long bit_index;

  tests_start_mpfr ();

  if (argc > 1)
    verbose = 1;

  nbtests = 10000;
  if (argc > 1)
    {
      long a = atol(argv[1]);
      if (a != 0)
        nbtests = a;
    }

  if (argc <= 2)
    prec = 1000;
  else
    prec = atol(argv[2]);

  if (argc <= 3)
    bit_index = -1;
  else
    {
      bit_index = atol(argv[3]);
      if (bit_index >= prec)
        {
          printf ("Warning. Cannot compute the bit frequency: the given bit "
                  "index (= %ld) is not less than the precision (= %ld).\n",
                  bit_index, (long) prec);
          bit_index = -1;
        }
    }

  RND_LOOP(rnd)
    {
      test_urandom (nbtests, prec, (mpfr_rnd_t) rnd, bit_index, verbose);

      if (argc == 1)  /* check also small precision */
        {
          test_urandom (nbtests, MPFR_PREC_MIN, (mpfr_rnd_t) rnd, -1, 0);
        }
    }

  underflow_tests ();
  overflow_tests ();

#ifndef MPFR_USE_MINI_GMP
  /* Since these tests assume a deterministic random generator, and
     this is not implemented in mini-gmp, we omit it with mini-gmp. */
  bug20100914 ();
  bug20170123 ();
  reprod_rnd_exp ();
  reprod_abi ();
#endif

  test_underflow (verbose);

  tests_end_mpfr ();
  return 0;
}
