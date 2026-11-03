/* Chi-squared test for mpfr_erandom

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

/* Return Phi(x) = 1 - exp(-x), the cumulative probability function for the
 * exponential distribution.  We only take differences of this function so the
 * offset doesn't matter; here Phi(0) = 0. */
static void
exponential_cumulative (mpfr_ptr z, mpfr_ptr x, mpfr_rnd_t rnd)
{
  mpfr_neg (z, x, rnd);
  mpfr_expm1 (z, z, rnd);
  mpfr_neg (z, z, rnd);
}

/* Given nu and chisqp, compute probability that chisq > chisqp.  This uses,
 * A&S 26.4.16,
 *
 * Q(nu,chisqp) =
 *     erfc( (3/2)*sqrt(nu) * ( cbrt(chisqp/nu) - 1 + 2/(9*nu) ) ) / 2
 *
 * which is valid for nu > 30.  This is the basis for the formula in Knuth,
 * TAOCP, Vol 2, 3.3.1, Table 1.  It more accurate than the similar formula,
 * DLMF 8.11.10. */
static void
chisq_prob (mpfr_ptr q, long nu, mpfr_ptr chisqp)
{
  mpfr_t t;
  mpfr_rnd_t rnd;

  rnd = MPFR_RNDN;  /* This uses an approx formula.  Might as well use RNDN. */
  mpfr_init2 (t, mpfr_get_prec (q));

  mpfr_div_si (q, chisqp, nu, rnd); /* chisqp/nu */
  mpfr_cbrt (q, q, rnd);            /* (chisqp/nu)^(1/3) */
  mpfr_sub_ui (q, q, 1, rnd);       /* (chisqp/nu)^(1/3) - 1 */
  mpfr_set_ui (t, 2, rnd);
  mpfr_div_si (t, t, 9*nu, rnd); /* 2/(9*nu) */
  mpfr_add (q, q, t, rnd);       /* (chisqp/nu)^(1/3) - 1 + 2/(9*nu) */
  mpfr_sqrt_ui (t, nu, rnd);     /* sqrt(nu) */
  mpfr_mul_d (t, t, 1.5, rnd);   /* (3/2)*sqrt(nu) */
  mpfr_mul (q, q, t, rnd);       /* arg to erfc */
  mpfr_erfc (q, q, rnd);         /* erfc(...) */
  mpfr_div_ui (q, q, 2, rnd);    /* erfc(...)/2 */

  mpfr_clear (t);
}

/* The continuous chi-squared test on with a set of bins of equal width.
 *
 * A single precision is picked for sampling and the chi-squared calculation.
 * This should picked high enough so that binning in test doesn't need to be
 * accurately aligned with possible values of the deviates.  Also we need the
 * precision big enough that chi-squared calculation itself is reliable.
 *
 * There's no particular benefit is testing with at very higher precisions;
 * because of the way terandom samples, this just adds additional barely
 * significant random bits to the deviates.  So this chi-squared test with
 * continuous equal width bins isn't a good tool for finding problems here.
 *
 * The testing of low precision exponential deviates is done by
 * test_erandom_chisq_disc. */
static double
test_erandom_chisq_cont (long num, mpfr_prec_t prec, int nu,
                         double xmin, double xmax, int verbose)
{
  mpfr_t x, a, b, dx, z, pa, pb, ps, t;
  long *counts;
  int i, inexact;
  long k;
  mpfr_rnd_t rnd, rndd;
  double Q, chisq;

  rnd = MPFR_RNDN;              /* For chi-squared calculation */
  rndd = MPFR_RNDD;             /* For sampling and figuring the bins */
  mpfr_inits2 (prec, x, a, b, dx, z, pa, pb, ps, t, (mpfr_ptr) 0);

  counts = (long *) tests_allocate ((nu + 1) * sizeof (long));
  for (i = 0; i <= nu; i++)
    counts[i] = 0;

  /* a and b are bounds of nu equally spaced bins.  Set dx = (b-a)/nu */
  mpfr_set_d (a, xmin, rnd);
  mpfr_set_d (b, xmax, rnd);

  mpfr_sub (dx, b, a, rnd);
  mpfr_div_si (dx, dx, nu, rnd);

  for (k = 0; k < num; ++k)
    {
      inexact = mpfr_erandom (x, RANDS, rndd);
      if (inexact == 0)
        {
          /* one call in the loop pretended to return an exact number! */
          printf ("Error: mpfr_erandom() returns a zero ternary value.\n");
          exit (1);
        }
      if (mpfr_signbit (x))
        {
          printf ("Error: mpfr_erandom() returns a negative deviate.\n");
          exit (1);
        }
      mpfr_sub (x, x, a, rndd);
      mpfr_div (x, x, dx, rndd);
      i = mpfr_get_si (x, rndd);
      ++counts[i >= 0 && i < nu ? i : nu];
    }

  mpfr_set (x, a, rnd);
  exponential_cumulative (pa, x, rnd);
  mpfr_add_ui (ps, pa, 1, rnd);
  mpfr_set_zero (t, 1);
  for (i = 0; i <= nu; ++i)
    {
      if (i < nu)
        {
          mpfr_add (x, x, dx, rnd);
          exponential_cumulative (pb, x, rnd);
          mpfr_sub (pa, pb, pa, rnd); /* prob for this bin */
        }
      else
        mpfr_sub (pa, ps, pa, rnd); /* prob for last bin, i = nu */

      /* Compute z = counts[i] - num * p; t += z * z / (num * p) */
      mpfr_mul_ui (pa, pa, num, rnd);
      mpfr_ui_sub (z, counts[i], pa, rnd);
      mpfr_sqr (z, z, rnd);
      mpfr_div (z, z, pa, rnd);
      mpfr_add (t, t, z, rnd);
      mpfr_swap (pa, pb);       /* i.e., pa = pb */
    }

  chisq = mpfr_get_d (t, rnd);
  chisq_prob (t, nu, t);
  Q = mpfr_get_d (t, rnd);
  if (verbose)
    {
      printf ("num = %ld, equal bins in [%.2f, %.2f], nu = %d: chisq = %.2f\n",
              num, xmin, xmax, nu, chisq);
      if (Q < 0.05)
        printf ("    WARNING: probability (less than 5%%) = %.2e\n", Q);
    }

  tests_free (counts, (nu + 1) * sizeof (long));
  mpfr_clears (x, a, b, dx, z, pa, pb, ps, t, (mpfr_ptr) 0);
  return Q;
}

/* Return a sequential number for a positive low-precision x.  x is altered by
 * this function.  low precision means prec = 2, 3, or 4.  High values of
 * precision will result in integer overflow. */
static long
sequential (mpfr_ptr x)
{
  long expt, prec;

  prec = mpfr_get_prec (x);
  expt =  mpfr_get_exp (x);
  mpfr_mul_2si (x, x, prec - expt, MPFR_RNDN);

  return expt * (1 << (prec - 1)) + mpfr_get_si (x, MPFR_RNDN);
}

/* The chi-squared test on low precision exponential deviates.  wprec is the
 * working precision for the chi-squared calculation.  prec is the precision
 * for the sampling; choose this in [2,5].  The bins consist of all the
 * possible deviate values in the range [xmin, xmax] coupled with the value of
 * inexact.  Thus with prec = 2, the bins are
 *   ...
 *   (7/16, 1/2)  x = 1/2, inexact = +1
 *   (1/2 , 5/8)  x = 1/2, inexact = -1
 *   (5/8 , 3/4)  x = 3/4, inexact = +1
 *   (3/4 , 7/8)  x = 3/4, inexact = -1
 *   (7/8 , 1  )  x = 1  , inexact = +1
 *   (1   , 5/4)  x = 1  , inexact = -1
 *   (5/4 , 3/2)  x = 3/2, inexact = +1
 *   (3/2 , 7/4)  x = 3/2, inexact = -1
 *   ...
 * In addition, two bins are allocated for [0,xmin) and (xmax,inf).
 *
 * The sampling is with MPFR_RNDN.  This is the rounding mode which elicits the
 * most information.  trandom_deviate includes checks on the consistency of the
 * results extracted from a random_deviate with other rounding modes.  */
static double
test_erandom_chisq_disc (long num, mpfr_prec_t wprec, int prec,
                         double xmin, double xmax, int verbose)
{
  mpfr_t x, v, pa, pb, z, t;
  mpfr_rnd_t rnd;
  int i, inexact, nu;
  long *counts;
  long k, seqmin, seqmax, seq;
  double Q, chisq;

  rnd = MPFR_RNDN;
  mpfr_init2 (x, prec);
  mpfr_init2 (v, prec+1);
  mpfr_inits2 (wprec, pa, pb, z, t, (mpfr_ptr) 0);

  mpfr_set_d (x, xmin, rnd);
  xmin = mpfr_get_d (x, rnd);
  mpfr_set (v, x, rnd);
  seqmin = sequential (x);
  mpfr_set_d (x, xmax, rnd);
  xmax = mpfr_get_d (x, rnd);
  seqmax = sequential (x);

  /* Two bins for each sequential number (for inexact = +/- 1), plus 1 for u <
   * umin and 1 for u > umax, minus 1 for degrees of freedom */
  nu = 2 * (seqmax - seqmin + 1) + 2 - 1;
  counts = (long *) tests_allocate ((nu + 1) * sizeof (long));
  for (i = 0; i <= nu; i++)
    counts[i] = 0;

  for (k = 0; k < num; ++k)
    {
      inexact = mpfr_erandom (x, RANDS, rnd);
      if (mpfr_signbit (x))
        {
          printf ("Error: mpfr_erandom() returns a negative deviate.\n");
          exit (1);
        }
      /* Don't call sequential with small args to avoid undefined behavior with
       * zero and possibility of overflow. */
      seq = mpfr_greaterequal_p (x, v) ? sequential (x) : seqmin - 1;
      ++counts[seq < seqmin ? 0 :
               seq <= seqmax ? 2 * (seq - seqmin) + 1 + (inexact > 0 ? 0 : 1) :
               nu];
    }

  mpfr_set_zero (v, 1);
  exponential_cumulative (pa, v, rnd);
  /* Cycle through all the bin boundaries using mpfr_nextabove at precision
   * prec + 1 starting at mpfr_nextbelow (xmin) */
  mpfr_set_d (x, xmin, rnd);
  mpfr_set (v, x, rnd);
  mpfr_nextbelow (v);
  mpfr_nextbelow (v);
  mpfr_set_zero (t, 1);
  for (i = 0; i <= nu; ++i)
    {
      if (i < nu)
        mpfr_nextabove (v);
      else
        mpfr_set_inf (v, 1);
      exponential_cumulative (pb, v, rnd);
      mpfr_sub (pa, pb, pa, rnd);

      /* Compute z = counts[i] - num * p; t += z * z / (num * p). */
      mpfr_mul_ui (pa, pa, num, rnd);
      mpfr_ui_sub (z, counts[i], pa, rnd);
      mpfr_sqr (z, z, rnd);
      mpfr_div (z, z, pa, rnd);
      mpfr_add (t, t, z, rnd);
      mpfr_swap (pa, pb);       /* i.e., pa = pb */
    }

  chisq = mpfr_get_d (t, rnd);
  chisq_prob (t, nu, t);
  Q = mpfr_get_d (t, rnd);
  if (verbose)
    {
      printf ("num = %ld, discrete (prec = %d) bins in [%.6f, %.2f], "
              "nu = %d: chisq = %.2f\n", num, prec, xmin, xmax, nu, chisq);
      if (Q < 0.05)
        printf ("    WARNING: probability (less than 5%%) = %.2e\n", Q);
    }

  tests_free (counts, (nu + 1) * sizeof (long));
  mpfr_clears (x, v, pa, pb, z, t, (mpfr_ptr) 0);
  return Q;
}

static void
run_chisq (double (*f)(long, mpfr_prec_t, int, double, double, int),
           long num, mpfr_prec_t prec, int bin,
           double xmin, double xmax, int verbose)
{
  double Q, Qcum, Qbad, Qthresh;
  int i;

  Qcum = 1;
  Qbad = 1.e-9;
  Qthresh = 0.01;
  for (i = 0; i < 3; ++i)
    {
      Q = (*f)(num, prec, bin, xmin, xmax, verbose);
      Qcum *= Q;
      if (Q > Qthresh)
        return;
      else if (Q < Qbad)
        {
          printf ("Error: mpfr_erandom chi-squared failure "
                  "(prob = %.2e < %.2e)\n", Q, Qbad);
          exit (1);
        }
      num *= 10;
      Qthresh /= 10;
    }
  if (Qcum < Qbad)              /* Presumably this is true */
    {
      printf ("Error: mpfr_erandom combined chi-squared failure "
              "(prob = %.2e)\n", Qcum);
      exit (1);
    }
}

int
main (int argc, char *argv[])
{
  long nbtests;
  int verbose;

  tests_start_mpfr ();

  verbose = 0;
  nbtests = 100000;
  if (argc > 1)
    {
      long a = atol (argv[1]);
      verbose = 1;
      if (a != 0)
        nbtests = a;
    }

  run_chisq (test_erandom_chisq_cont, nbtests, 64, 60, 0, 7, verbose);
  run_chisq (test_erandom_chisq_disc, nbtests, 64, 2, 0.002, 6, verbose);
  run_chisq (test_erandom_chisq_disc, nbtests, 64, 3, 0.02, 7, verbose);
  run_chisq (test_erandom_chisq_disc, nbtests, 64, 4, 0.04, 8, verbose);

  tests_end_mpfr ();
  return 0;
}
