/* Test file for mpfr_set_float128 and mpfr_get_float128.

Copyright 2012-2025 Free Software Foundation, Inc.
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

/* Needed due to the test on MPFR_WANT_FLOAT128 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef MPFR_WANT_FLOAT128

#include "mpfr-test.h"
#include "ieee_floats.h"

static void
check_special (void)
{
  mpfr_float128 f;
  mpfr_t x;

  mpfr_init2 (x, 113);

#if !defined(MPFR_ERRDIVZERO)
  /* check NaN */
  f = MPFR_DBL_NAN;
  mpfr_set_float128 (x, f, MPFR_RNDN);
  if (! mpfr_nan_p (x))
    {
      printf ("Error in mpfr_set_float128(x, NaN)\n");
      printf ("got ");
      mpfr_dump (x);
      exit (1);
    }
  f = mpfr_get_float128 (x, MPFR_RNDN);
  if (! DOUBLE_ISNAN (f))
    {
      printf ("Error in mpfr_get_float128(NaN)\n");
      printf ("got %f\n", (double) f);
      exit (1);
    }

  /* check +Inf */
  f = MPFR_DBL_INFP;
  mpfr_set_float128 (x, f, MPFR_RNDN);
  if (! mpfr_inf_p (x) || MPFR_IS_NEG (x))
    {
      printf ("Error in mpfr_set_float128(x, +Inf)\n");
      printf ("got ");
      mpfr_dump (x);
      exit (1);
    }
  f = mpfr_get_float128 (x, MPFR_RNDN);
  if (f != MPFR_DBL_INFP)
    {
      printf ("Error in mpfr_get_float128(+Inf)\n");
      printf ("got %f\n", (double) f);
      exit (1);
    }

  /* check -Inf */
  f = MPFR_DBL_INFM;
  mpfr_set_float128 (x, f, MPFR_RNDN);
  if (! mpfr_inf_p (x) || MPFR_IS_POS (x))
    {
      printf ("Error in mpfr_set_float128(x, -Inf)\n");
      printf ("got ");
      mpfr_dump (x);
      exit (1);
    }
  f = mpfr_get_float128 (x, MPFR_RNDN);
  if (f != MPFR_DBL_INFM)
    {
      printf ("Error in mpfr_get_float128(-Inf)\n");
      printf ("got %f\n", (double) f);
      exit (1);
    }
#endif

  /* check +0 */
  f = 0.0;
  mpfr_set_float128 (x, f, MPFR_RNDN);
  if (! mpfr_zero_p (x) || MPFR_IS_NEG (x))
    {
      printf ("Error in mpfr_set_float128(x, +0)\n");
      printf ("got ");
      mpfr_dump (x);
      exit (1);
    }
  f = mpfr_get_float128 (x, MPFR_RNDN);
  if (f != 0.0)  /* the sign is not checked */
    {
      printf ("Error in mpfr_get_float128(+0.0)\n");
      printf ("got %f\n", (double) f);
      exit (1);
    }
#if !defined(MPFR_ERRDIVZERO) && defined(HAVE_SIGNEDZ)
  if (1 / f != MPFR_DBL_INFP)  /* check the sign */
    {
      printf ("Error in mpfr_get_float128(+0.0)\n");
      printf ("got %f\n", (double) f);
      exit (1);
    }
#endif

  /* check -0 */
  f = -0.0;
  mpfr_set_float128 (x, f, MPFR_RNDN);
  if (! mpfr_zero_p (x))
    {
      printf ("Error in mpfr_set_float128(x, -0)\n");
      printf ("got ");
      mpfr_dump (x);
      exit (1);
    }
#if defined(HAVE_SIGNEDZ)
  if (MPFR_IS_POS (x))
    {
      printf ("Error in mpfr_set_float128(x, -0)\n");
      printf ("got ");
      mpfr_dump (x);
      exit (1);
    }
#endif
  f = mpfr_get_float128 (x, MPFR_RNDN);
  if (f != -0.0)  /* the sign is not checked */
    {
      printf ("Error in mpfr_get_float128(-0.0)\n");
      printf ("got %f\n", (double) f);
      exit (1);
    }
#if !defined(MPFR_ERRDIVZERO) && defined(HAVE_SIGNEDZ)
  if (1 / f != MPFR_DBL_INFM)  /* check the sign */
    {
      printf ("Error in mpfr_get_float128(-0.0)\n");
      printf ("got %f\n", (double) f);
      exit (1);
    }
#endif

  mpfr_clear (x);
}

static void
check_large (void)
{
  mpfr_exp_t emin, emax;
  mpfr_float128 f, e;
  int i;
  mpfr_t x, y;
  int r;
  int red;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  mpfr_init2 (x, 113);
  mpfr_init2 (y, 113);

  /* check with the largest float128 number 2^16384*(1-2^(-113)) */
  for (f = 1.0, i = 0; i < 113; i++)
    f = f + f;
  f = f - (mpfr_float128) 1.0;
  mpfr_set_ui (y, 1, MPFR_RNDN);
  mpfr_mul_2ui (y, y, 113, MPFR_RNDN);
  mpfr_sub_ui (y, y, 1, MPFR_RNDN);
  for (i = 113; i < 16384; i++)
    {
      RND_LOOP (r)
        {
          mpfr_set_float128 (x, f, (mpfr_rnd_t) r);
          if (! mpfr_equal_p (x, y))
            {
              printf ("mpfr_set_float128 failed for 2^%d*(1-2^(-113)) rnd=%s\n",
                      i, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
              printf ("got ");
              mpfr_dump (x);
              exit (1);
            }
          for (red = 0; red < 2; red++)
            {
              if (red)
                {
                  mpfr_exp_t ex;

                  if (MPFR_IS_SINGULAR (x))
                    break;
                  ex = MPFR_GET_EXP (x);
                  set_emin (ex);
                  set_emax (ex);
                }
              e =  mpfr_get_float128 (x, (mpfr_rnd_t) r);
              set_emin (emin);
              set_emax (emax);
              if (e != f)
                {
                  printf ("mpfr_get_float128 failed for 2^%d*(1-2^(-113))"
                          " rnd=%s%s\n",
                          i, mpfr_print_rnd_mode ((mpfr_rnd_t) r),
                          red ? ", reduced exponent range" : "");
                  exit (1);
                }
            }
        }

      /* check with opposite number */
      f = -f;
      mpfr_neg (y, y, MPFR_RNDN);
      RND_LOOP (r)
        {
          mpfr_set_float128 (x, f, (mpfr_rnd_t) r);
          if (! mpfr_equal_p (x, y))
            {
              printf ("mpfr_set_float128 failed for -2^%d*(1-2^(-113)) rnd=%s\n",
                      i, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
              printf ("got ");
              mpfr_dump (x);
              exit (1);
            }
          e =  mpfr_get_float128 (x, (mpfr_rnd_t) r);
          if (e != f)
            {
              printf ("mpfr_get_float128 failed for -2^%d*(1-2^(-113)) rnd=%s\n",
                      i, mpfr_print_rnd_mode ((mpfr_rnd_t) r));
              exit (1);
            }
        }

      f = -f;
      mpfr_neg (y, y, MPFR_RNDN);
      f = f + f;
      mpfr_add (y, y, y, MPFR_RNDN);
    }

  mpfr_clear (x);
  mpfr_clear (y);
}

static void
check_small (void)
{
  int t[5] = { 1, 2, 17, 111, 112 };
  mpfr_exp_t emin;
  mpfr_float128 e, f;
  int i, j, neg, inex, r;
  mpfr_t w, x, y, z;

  emin = mpfr_get_emin ();

  mpfr_inits2 (113, w, x, y, z, (mpfr_ptr) 0);

  f = 1.0;
  mpfr_set_ui (y, 1, MPFR_RNDN);
  for (i = 0; i > -16500; i--)
    {
      for (j = 0; j < 5; j++)
        {
          mpfr_div_2ui (z, y, t[j], MPFR_RNDN);
          inex = mpfr_add (z, z, y, MPFR_RNDN);
          MPFR_ASSERTN (inex == 0);
          /* z = y (1 + 2^(-t[j]) */
          for (neg = 0; neg < 2; neg++)
            {
              RND_LOOP_NO_RNDF (r)
                {
                  if (j == 0 && f != 0)
                    {
                      /* This test does not depend on j. */
                      mpfr_set_float128 (x, f, (mpfr_rnd_t) r);
                      if (! mpfr_equal_p (x, y))
                        {
                          printf ("mpfr_set_float128 failed for "
                                  "%c2^(%d) rnd=%s\n", neg ? '-' : '+', i,
                                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                          printf ("got ");
                          mpfr_dump (x);
                          exit (1);
                        }
                      e =  mpfr_get_float128 (x, (mpfr_rnd_t) r);
                      if (e != f)
                        {
                          printf ("mpfr_get_float128 failed for "
                                  "%c2^(%d) rnd=%s\n", neg ? '-' : '+', i,
                                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                          exit (1);
                        }
                    }
                  if (i < -16378)
                    {
                      /* Subnormals or close to subnormals...
                         Here we mainly test mpfr_get_float128. */
                      e =  mpfr_get_float128 (z, (mpfr_rnd_t) r);
                      mpfr_set_float128 (x, e, MPFR_RNDN); /* exact */
                      inex = mpfr_set (w, z, MPFR_RNDN);
                      MPFR_ASSERTN (inex == 0);
                      set_emin (-16493);
                      inex = mpfr_check_range (w, 0, (mpfr_rnd_t) r);
                      mpfr_subnormalize (w, inex, (mpfr_rnd_t) r);
                      set_emin (emin);
                      if (! mpfr_equal_p (x, w))
                        {
                          printf ("mpfr_get_float128 failed for "
                                  "%c(2^(%d))(1+2^(-%d)) rnd=%s\n",
                                  neg ? '-' : '+', i, t[j],
                                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                          printf ("expected ");
                          mpfr_dump (w);
                          printf ("got      ");
                          mpfr_dump (x);
                          exit (1);
                        }
                    }
                }
              f = -f;
              mpfr_neg  (y, y, MPFR_RNDN);
              mpfr_neg  (z, z, MPFR_RNDN);
            }
        }
      f =  0.5 * f;
      mpfr_div_2ui (y, y, 1, MPFR_RNDN);
    }

  mpfr_clears (w, x, y, z, (mpfr_ptr) 0);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  check_special ();

  check_large ();

  check_small ();

  tests_end_mpfr ();

  return 0;
}

#else /* MPFR_WANT_FLOAT128 */

/* dummy main to say this test is ignored */
int
main (void)
{
  return 77;
}

#endif /* MPFR_WANT_FLOAT128 */
