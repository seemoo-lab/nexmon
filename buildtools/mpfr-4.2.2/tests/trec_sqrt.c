/* Test file for mpfr_rec_sqrt.

Copyright 2008-2025 Free Software Foundation, Inc.
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

#include <time.h>
#include "mpfr-test.h"

#define TEST_FUNCTION mpfr_rec_sqrt
#define TEST_RANDOM_POS 8 /* 8/512 = 1/64 of the tested numbers are negative */
#include "tgeneric.c"

static void
special (void)
{
  mpfr_t x, y;
  int inex;

  mpfr_init (x);
  mpfr_init (y);

  /* rec_sqrt(NaN) = NaN */
  mpfr_set_nan (x);
  inex = mpfr_rec_sqrt (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (x) && inex == 0);

  /* rec_sqrt(+Inf) = +0 */
  mpfr_set_inf (x, 1);
  inex = mpfr_rec_sqrt (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_zero_p (x) && MPFR_IS_POS(x) && inex == 0);

  /* rec_sqrt(-Inf) = NaN */
  mpfr_set_inf (x, -1);
  inex = mpfr_rec_sqrt (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (x) && inex == 0);

  /* rec_sqrt(+0) = +Inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  inex = mpfr_rec_sqrt (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (x) && MPFR_IS_POS(x) && inex == 0);

  /* rec_sqrt(-0) = +Inf */
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_neg (x, x, MPFR_RNDN);
  inex = mpfr_rec_sqrt (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_inf_p (x) && MPFR_IS_POS(x) && inex == 0);

  /* rec_sqrt(-1) = NaN */
  mpfr_set_si (x, -1, MPFR_RNDN);
  inex = mpfr_rec_sqrt (x, x, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_nan_p (x) && inex == 0);

  /* rec_sqrt(1) = 1 */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  inex = mpfr_rec_sqrt (x, x, MPFR_RNDN);
  MPFR_ASSERTN((mpfr_cmp_ui (x, 1) == 0) && (inex == 0));

  mpfr_set_prec (x, 23);
  mpfr_set_prec (y, 33);
  mpfr_set_str_binary (x, "1.0001110110101001010100e-1");
  inex = mpfr_rec_sqrt (y, x, MPFR_RNDU);
  mpfr_set_prec (x, 33);
  mpfr_set_str_binary (x, "1.01010110101110100100100101011");
  MPFR_ASSERTN (inex > 0 && mpfr_cmp (x, y) == 0);

  mpfr_clear (x);
  mpfr_clear (y);
}

/* Worst case incorrectly rounded in r5573, found with the bad_cases test */
static void
bad_case1 (void)
{
  mpfr_t x, y, z;

  mpfr_init2 (x, 72);
  mpfr_inits2 (6, y, z, (mpfr_ptr) 0);
  mpfr_set_str (x, "1.08310518720928b30e@-120", 16, MPFR_RNDN);
  mpfr_set_str (z, "f.8@59", 16, MPFR_RNDN);
  /* z = rec_sqrt(x) rounded on 6 bits toward 0, the exact value
     being ~= f.bffffffffffffffffa11@59. */
  mpfr_rec_sqrt (y, x, MPFR_RNDZ);
  if (mpfr_cmp0 (y, z) != 0)
    {
      printf ("Error in bad_case1\nexpected ");
      mpfr_out_str (stdout, 16, 0, z, MPFR_RNDN);
      printf ("\ngot      ");
      mpfr_out_str (stdout, 16, 0, y, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

static int
pm2 (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  return mpfr_pow_si (y, x, -2, rnd_mode);
}

/* exercises corner cases with inputs around 1 or 2 */
static void
bad_case2 (void)
{
  mpfr_t r, u;
  mpfr_prec_t pr, pu;
  int rnd;

  for (pr = MPFR_PREC_MIN; pr <= 192; pr++)
    for (pu = MPFR_PREC_MIN; pu <= 192; pu++)
      {
        mpfr_init2 (r, pr);
        mpfr_init2 (u, pu);

        mpfr_set_ui (u, 1, MPFR_RNDN);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_nextbelow (u);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_nextbelow (u);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_set_ui (u, 1, MPFR_RNDN);
        mpfr_nextabove (u);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_nextabove (u);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_set_ui (u, 2, MPFR_RNDN);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_nextbelow (u);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_nextbelow (u);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_set_ui (u, 2, MPFR_RNDN);
        mpfr_nextabove (u);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_nextabove (u);
        RND_LOOP (rnd)
          mpfr_rec_sqrt (r, u, (mpfr_rnd_t) rnd);

        mpfr_clear (r);
        mpfr_clear (u);
      }
}

/* Before commits 270f4df6b3a49caae1cf564dcdc1c55b1c5989eb (master) and
   934dd8842b4bdeb919a73123203bc8ce56db38d1 (4.2 branch) on 2023-04-17,
   this was giving a stack overflow in mpfr_rec_sqrt due to a Ziv loop
   where the working precision was increased additively instead of the
   standard Ziv loop using the MPFR_ZIV_* macros.
*/
static void
bad_case3 (void)
{
  mpfr_t x, y;
  int inex;

  mpfr_init2 (x, 123456);
  mpfr_init2 (y, 4);
  mpfr_set_ui (y, 9, MPFR_RNDN);
  mpfr_ui_div (x, 1, y, MPFR_RNDN);
  inex = mpfr_rec_sqrt (y, x, MPFR_RNDN);
  /* Let's also check the result, though this is not the real purpose
     of this test (a stack overflow just makes the program crash).
     1/9 = 0.111000111000111000111000111000111000...E-3 and since the
     precision 123456 is divisible by 6, x > 1/9. Thus 1/sqrt(x) < 3. */
  if (mpfr_cmp_ui0 (y, 3) != 0 || inex <= 0)
    {
      printf ("Error in bad_case3: expected 3 with inex > 0, got ");
      mpfr_out_str (stdout, 10, 0, y, MPFR_RNDN);
      printf (" with inex=%d\n", inex);
      exit (1);
    }
  mpfr_clear (x);
  mpfr_clear (y);
}

/* timing test for n limbs (so that we can compare with GMP speed -s n) */
static void
test (unsigned long n)
{
  mpfr_prec_t p = n * GMP_NUMB_BITS;
  mpfr_t x, y, z;
  gmp_randstate_t state;
  double t;

  gmp_randinit_default (state);
  mpfr_init2 (x, p);
  mpfr_init2 (y, p);
  mpfr_init2 (z, p);
  mpfr_urandom (x, state, MPFR_RNDN);
  mpfr_urandom (y, state, MPFR_RNDN);

  /* multiplication */
  t = clock ();
  mpfr_mul (z, x, y, MPFR_RNDN);
  t = clock () - t;
  printf ("mpfr_mul:      %.3gs\n", t / (double) CLOCKS_PER_SEC);

  /* squaring */
  t = clock ();
  mpfr_sqr (z, x, MPFR_RNDN);
  t = clock () - t;
  printf ("mpfr_sqr:      %.3gs\n", t / (double) CLOCKS_PER_SEC);

  /* square root */
  t = clock ();
  mpfr_sqrt (z, x, MPFR_RNDN);
  t = clock () - t;
  printf ("mpfr_sqrt:     %.3gs\n", t / (double) CLOCKS_PER_SEC);

  /* inverse square root */
  t = clock ();
  mpfr_rec_sqrt (z, x, MPFR_RNDN);
  t = clock () - t;
  printf ("mpfr_rec_sqrt: %.3gs\n", t / (double) CLOCKS_PER_SEC);

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  gmp_randclear (state);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  if (argc == 2) /* trec_sqrt n */
    {
      unsigned long n = strtoul (argv[1], NULL, 10);
      test (n);
      goto end;
    }

  special ();
  bad_case1 ();
  bad_case2 ();
  bad_case3 ();
  test_generic (MPFR_PREC_MIN, 300, 15);

  data_check ("data/rec_sqrt", mpfr_rec_sqrt, "mpfr_rec_sqrt");
  bad_cases (mpfr_rec_sqrt, pm2, "mpfr_rec_sqrt", 0, -256, 255, 4, 128,
             800, 50);
  bad_cases (mpfr_rec_sqrt, pm2, "mpfr_rec_sqrt", 0, -256, 255, 9999, 9999,
             120000, 1);

 end:
  tests_end_mpfr ();
  return 0;
}
