/* Test file for mpfr_random_deviate

Copyright 2011-2025 Free Software Foundation, Inc.
Contributed by Charles Karney <charles@karney.com>, SRI International.

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

#include "random_deviate.h"

#define W 32                    /* Must match value in random_deviate.c */

/* set random deviate rop from op */
static void
mpfr_random_deviate_set (mpfr_random_deviate_t rop, mpfr_random_deviate_t op)
{
  rop->e = op->e;
  rop->h = op->h;
  mpz_set (rop->f, op->f);
}

/* set random deviate to fract * 2^-expt.  expt must be a multiple
 * of W and cannot be 0.  fract must be in [0,2^W) */
static void
mpfr_random_deviate_ldexp (mpfr_random_deviate_t rop,
                           unsigned long fract, unsigned long expt)
{
  rop->h = (expt > W ? 0ul : fract);
  mpz_set_ui (rop->f, expt > W ? fract : 0ul);
  rop->e = expt;
}

/* Test mpfr_random_deviate_less.  With two initially equal deviates this
 * should return true half the time.  In order to execute additional code
 * paths, the two deviates are repeatedly set equal and the test repeated (with
 * now a longer fraction and with the test now triggering the sampling of an
 * additional chunk. */
static void
test_compare (long nbtests, int verbose)
{
  mpfr_random_deviate_t u, v;
  int k, i, t1, t2;
  long count;

  mpfr_random_deviate_init (u);
  mpfr_random_deviate_init (v);

  count = 0;
  for (k = 0; k < nbtests; ++k)
    {
      mpfr_random_deviate_reset (u);
      mpfr_random_deviate_reset (v);
      for (i = 0; i < 10; ++i)
        {
          t1 = mpfr_random_deviate_less (u, v, RANDS);
          t2 = mpfr_random_deviate_less (u, v, RANDS);
          if (t1 != t2)
            {
              printf ("Error: mpfr_random_deviate_less() inconsistent.\n");
              exit (1);
            }
          if (t1)
            ++count;
          /* Force the test to sample an additional chunk */
          mpfr_random_deviate_set (u, v);
        }
      t1 = mpfr_random_deviate_less (u, u, RANDS);
      if (t1)
        {
          printf ("Error: mpfr_random_deviate_less() gives u < u.\n");
          exit (1);
        }
      t1 = mpfr_random_deviate_tstbit (u, 0, RANDS);
      if (t1)
        {
          printf ("Error: mpfr_random_deviate_tstbit() says 1 bit is on.\n");
          exit (1);
        }
    }
  mpfr_random_deviate_clear (v);
  mpfr_random_deviate_clear (u);
  if (verbose)
    printf ("Fraction of true random_deviate_less = %.4f"
            " (should be about 0.5)\n",
            count / (double) (10 * nbtests));
}

/* A random_deviate should consistently return the same value at a given
 * precision, even if intervening operations have caused the fraction to be
 * extended. */
static void
test_value_consistency (long nbtests)
{
  mpfr_t a1, a2, a3, b1, b2, b3;
  mpfr_random_deviate_t u;
  int inexa1, inexa2, inexa3, inexb1, inexb2, inexb3;
  mpfr_prec_t prec1, prec2, prec3;
  mpfr_rnd_t rnd;
  long i;
  unsigned n;
  int neg;

  /* Pick prec{1,2,3} random in [2,101] */
  prec1 = 2 + gmp_urandomm_ui (RANDS, 100);
  prec2 = 2 + gmp_urandomm_ui (RANDS, 100);
  prec3 = 2 + gmp_urandomm_ui (RANDS, 100);

  rnd = MPFR_RNDN;
  mpfr_random_deviate_init (u);
  mpfr_init2 (a1, prec1);
  mpfr_init2 (b1, prec1);
  mpfr_init2 (a2, prec2);
  mpfr_init2 (b2, prec2);
  mpfr_init2 (a3, prec3);
  mpfr_init2 (b3, prec3);

  for (i = 0; i < nbtests; ++i)
    {
      mpfr_random_deviate_reset (u);
      n = gmp_urandomb_ui (RANDS, 4);
      neg = gmp_urandomb_ui (RANDS, 1);
      inexa1 = mpfr_random_deviate_value (neg, n, u, a1, RANDS, rnd);
      inexa2 = mpfr_random_deviate_value (neg, n, u, a2, RANDS, rnd);
      inexa3 = mpfr_random_deviate_value (neg, n, u, a3, RANDS, rnd);
      inexb1 = mpfr_random_deviate_value (neg, n, u, b1, RANDS, rnd);
      inexb2 = mpfr_random_deviate_value (neg, n, u, b2, RANDS, rnd);
      inexb3 = mpfr_random_deviate_value (neg, n, u, b3, RANDS, rnd);
      /* Of course a1, a2, and a3 should all be nearly equal.  But more
       * crucially (and easier to test), we need a1 == b1, etc.  (This is not a
       * trivial issue because the detailed mpfr operations giving b1 will be
       * different than for a1, if, e.g., prec2 > prec1. */
      if ( !( inexa1 == inexb1 && mpfr_equal_p (a1, b1) &&
              inexa2 == inexb2 && mpfr_equal_p (a2, b2) &&
              inexa3 == inexb3 && mpfr_equal_p (a3, b3) ) )
        {
          printf ("Error: random_deviate values are inconsistent.\n");
          exit (1);
        }
    }
  mpfr_random_deviate_clear (u);
  mpfr_clears (a1, a2, a3, b1, b2, b3, (mpfr_ptr) 0);
}

/* Check that the values from random_deviate with different rounding modes are
 * consistent. */
static void
test_value_round (long nbtests, mpfr_prec_t prec)
{
  mpfr_t xn, xd, xu, xz, xa, t;
  mpfr_random_deviate_t u;
  int inexn, inexd, inexu, inexz, inexa, inext;
  long i;
  unsigned n;
  int neg, s;

  mpfr_random_deviate_init (u);
  mpfr_inits2 (prec, xn, xd, xu, xz, xa, t, (mpfr_ptr) 0);

  for (i = 0; i < nbtests; ++i)
    {
      mpfr_random_deviate_reset (u);
      n = gmp_urandomb_ui (RANDS, 4);
      neg = gmp_urandomb_ui (RANDS, 1);
      s = neg ? -1 : 1;
      inexn = mpfr_random_deviate_value (neg, n, u, xn, RANDS, MPFR_RNDN);
      inexd = mpfr_random_deviate_value (neg, n, u, xd, RANDS, MPFR_RNDD);
      inexu = mpfr_random_deviate_value (neg, n, u, xu, RANDS, MPFR_RNDU);
      inexz = mpfr_random_deviate_value (neg, n, u, xz, RANDS, MPFR_RNDZ);
      inexa = mpfr_random_deviate_value (neg, n, u, xa, RANDS, MPFR_RNDA);
      inext = mpfr_set (t, xn, MPFR_RNDN);
      /* Check inexact values */
      if ( !( inexn != 0 && inext == 0 &&
              inexd     < 0 && inexu     > 0 &&
              inexz * s < 0 && inexa * s > 0 ) )
        {
          printf ("n %d t %d d %d u %d z %d a %d s %d\n",
                  inexn, inext, inexd, inexu, inexz, inexa, s);
          printf ("Error: random_deviate has wrong values for inexact.\n");
          exit (1);
        }
      if (inexn < 0)
        mpfr_nextabove (t);
      else
        mpfr_nextbelow (t);
      /* Check that x{d,u,z,a} == xn is the inexact flags match, else
       * x{d,u,z,a} == t */
      if ( !( mpfr_equal_p(xd, SAME_SIGN(inexn, inexd) ? xn : t) &&
              mpfr_equal_p(xu, SAME_SIGN(inexn, inexu) ? xn : t) &&
              mpfr_equal_p(xz, SAME_SIGN(inexn, inexz) ? xn : t) &&
              mpfr_equal_p(xa, SAME_SIGN(inexn, inexa) ? xn : t) ) )
        {
          printf ("n %d t %d d %d u %d z %d a %d s %d\n",
                  inexn, inext, inexd, inexu, inexz, inexa, s);
          printf ("n %.4f t %.4f d %.4f u %.4f z %.4f a %.4f\n",
                  mpfr_get_d (xn, MPFR_RNDN), mpfr_get_d (t, MPFR_RNDN),
                  mpfr_get_d (xd, MPFR_RNDN), mpfr_get_d (xu, MPFR_RNDN),
                  mpfr_get_d (xz, MPFR_RNDN), mpfr_get_d (xa, MPFR_RNDN));
          printf ("Error: random_deviate rounds inconsistently.\n");
          exit (1);
        }
    }
  mpfr_random_deviate_clear (u);
  mpfr_clears (xn, xd, xu, xz, xa, t, (mpfr_ptr) 0);
}

/* Test mpfr_random_deviate_value.  Check for the leading bit in the number in
 * various positions. */
static void
test_value (long nbtests, mpfr_prec_t prec, mpfr_rnd_t rnd,
            int verbose)
{
  mpfr_t x;
  mpfr_random_deviate_t u;
  int inexact, exact;
  int i, k, b, neg;
  unsigned long e, f, n;
  long count, sum;

  mpfr_random_deviate_init (u);
  mpfr_init2 (x, prec);

  count = 0; sum = 0;
  exact = 0;

  for (k = 0; k < nbtests; ++k)
    {
      for (i = 0; i < 32; ++i)
        {
          b = gmp_urandomm_ui (RANDS, 32) + 1; /* bits to sample in integer */
          n = gmp_urandomb_ui (RANDS, b);
          neg = gmp_urandomb_ui (RANDS, 1);
          inexact = mpfr_random_deviate_value (neg, n, u, x, RANDS, rnd);
          if (!inexact)
            exact = 1;
          if (inexact > 0)
            ++count;
          ++sum;
        }
      for (i = 0; i < 32; ++i)
        {
          b = gmp_urandomm_ui (RANDS, W) + 1; /* bits to sample in fraction */
          f = gmp_urandomb_ui (RANDS, b);
          e = W * (gmp_urandomm_ui (RANDS, 3) + 1);
          mpfr_random_deviate_ldexp (u, f, e);
          neg = gmp_urandomb_ui (RANDS, 1);
          inexact = mpfr_random_deviate_value (neg, 0, u, x, RANDS, rnd);
          if (!inexact)
            exact = 1;
          if (inexact > 0)
            ++count;
          ++sum;
        }
      if (exact)
        {
          printf ("Error: random_deviate() returns a zero ternary value.\n");
          exit (1);
        }
      mpfr_random_deviate_reset (u);
    }
  mpfr_random_deviate_clear (u);
  mpfr_clear (x);

  if (verbose)
    {
      printf ("Fraction of inexact > 0 = %.4f", count / (double) (sum));
      if (rnd == MPFR_RNDD)
        printf (" should be exactly 0\n");
      else if (rnd ==  MPFR_RNDU)
        printf (" should be exactly 1\n");
      else
        printf (" should be about 0.5\n");
    }
}

int
main (int argc, char *argv[])
{
  long nbtests;
  int verbose;
  long a;

  tests_start_mpfr ();

  verbose = 0;
  nbtests = 10;
  if (argc > 1)
    {
      a = atol (argv[1]);
      verbose = 1;
      if (a != 0)
        nbtests = a;
    }

  test_compare (nbtests, verbose);
  test_value_consistency (nbtests);
  test_value_round (nbtests, 2);
  test_value_round (nbtests, 64);
  test_value (nbtests,  2, MPFR_RNDD, verbose);
  test_value (nbtests,  5, MPFR_RNDU, verbose);
  test_value (nbtests, 24, MPFR_RNDN, verbose);
  test_value (nbtests, 53, MPFR_RNDZ, verbose);
  test_value (nbtests, 64, MPFR_RNDA, verbose);

  tests_end_mpfr ();
  return 0;
}
