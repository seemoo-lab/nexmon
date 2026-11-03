/* Test file for mpfr_rootn_si.

Copyright 2022-2025 Free Software Foundation, Inc.
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

#define DEFN(N)                                                         \
  static int root##N (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd)        \
  { return mpfr_rootn_si (y, x, N, rnd); }                              \
  static int pow##N (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd)         \
  { return mpfr_pow_si (y, x, N, rnd); }                                \
  static int rootm##N (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd)       \
  { return mpfr_rootn_si (y, x, -N, rnd); }                             \
  static int powm##N (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd)        \
  { return mpfr_pow_si (y, x, -N, rnd); }

DEFN(2)
DEFN(3)
DEFN(4)
DEFN(5)
DEFN(17)
DEFN(120)

static void
special (void)
{
  mpfr_t x, y;
  int i, inex, sx;
  int n[] = { -123456, -12345, -123, -12, -5, -4, -3, -2, -1, 0,
    1, 2, 3, 4, 5, 12, 123, 12345, 123456 };

  mpfr_inits2 (123, x, y, (mpfr_ptr) 0);

  /* rootn(NaN) = NaN */
  mpfr_set_nan (x);
  for (i = 0; i < numberof (n); i++)
    {
      mpfr_clear_flags ();
      inex = mpfr_rootn_si (y, x, n[i], MPFR_RNDN);
      if (! MPFR_IS_NAN (y))
        {
          printf ("Error: rootn(NaN,%d) <> NaN\n", n[i]);
          exit (1);
        }
      MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);
      MPFR_ASSERTN (inex == 0);
    }

  /* rootn(+Inf) = +0, NaN or +Inf for sign(n) = -1, 0, 1 respectively */
  mpfr_set_inf (x, 1);
  for (i = 0; i < numberof (n); i++)
    {
      mpfr_clear_flags ();
      inex = mpfr_rootn_si (y, x, n[i], MPFR_RNDN);
      if (n[i] < 0)
        {
          if (MPFR_NOTZERO (y) || MPFR_IS_NEG (y))
            {
              printf ("Error: rootn(+Inf,%d) <> +0\n", n[i]);
              exit (1);
            }
        }
      else if (n[i] > 0)
        {
          if (! MPFR_IS_INF (y) || MPFR_IS_NEG (y))
            {
              printf ("Error: rootn(+Inf,%d) <> +Inf\n", n[i]);
              exit (1);
            }
        }
      else if (! MPFR_IS_NAN (y))
        {
          printf ("Error: rootn(+Inf,0) <> NaN\n");
          exit (1);
        }
      MPFR_ASSERTN (__gmpfr_flags == (n[i] == 0 ? MPFR_FLAGS_NAN : 0));
      MPFR_ASSERTN (inex == 0);
    }

  /* rootn(-Inf) = -0 (resp. -Inf) for sign(n) = -1 (resp. 1) and odd n,
     NaN for even n */
  mpfr_set_inf (x, -1);
  for (i = 0; i < numberof (n); i++)
    {
      mpfr_clear_flags ();
      inex = mpfr_rootn_si (y, x, n[i], MPFR_RNDN);
      if (n[i] % 2 == 0)
        {
          if (! MPFR_IS_NAN (y))
            {
              printf ("Error: rootn(-Inf,%d) <> NaN\n", n[i]);
              exit (1);
            }
          MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);
        }
      else
        {
          if (n[i] < 0)
            {
              if (MPFR_NOTZERO (y) || MPFR_IS_POS (y))
                {
                  printf ("Error: rootn(-Inf,%d) <> -0\n", n[i]);
                  exit (1);
                }
            }
          else
            {
              if (! MPFR_IS_INF (y) || MPFR_IS_POS (y))
                {
                  printf ("Error: rootn(-Inf,%d) <> -Inf\n", n[i]);
                  exit (1);
                }
            }
          MPFR_ASSERTN (__gmpfr_flags == 0);
        }
      MPFR_ASSERTN (inex == 0);
    }

  /* rootn(+/- 0) */
  for (i = 0; i < numberof (n); i++)
    for (sx = -1; sx <= 1; sx += 2)
      {
        mpfr_set_zero (x, sx);
        mpfr_clear_flags ();
        inex = mpfr_rootn_si (y, x, n[i], MPFR_RNDN);
        if (sx > 0 || n[i] % 2 == 0 ? MPFR_IS_NEG (y) : MPFR_IS_POS (y))
          {
            printf ("Error: rootn(%c0,%d) has a wrong sign\n",
                    sx > 0 ? '+' : '-', n[i]);
            exit (1);
          }
        if (n[i] < 0)
          {
            if (! MPFR_IS_INF (y))
              {
                printf ("Error: rootn(%c0,%d) is not an infinity\n",
                        sx > 0 ? '+' : '-', n[i]);
                exit (1);
              }
            MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);
          }
        else if (n[i] > 0)
          {
            if (MPFR_NOTZERO (y))
              {
                printf ("Error: rootn(%c0,%d) is not a zero\n",
                        sx > 0 ? '+' : '-', n[i]);
                exit (1);
              }
            MPFR_ASSERTN (__gmpfr_flags == 0);
          }
        else
          {
            if (! MPFR_IS_NAN (y))
              {
                printf ("Error: rootn(%c0,0) <> NaN\n", sx > 0 ? '+' : '-');
                exit (1);
              }
            MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);
          }
        MPFR_ASSERTN (inex == 0);
      }

  /* TODO: complete the tests. */

  mpfr_clears (x, y, (mpfr_ptr) 0);
}

#define TEST_FUNCTION mpfr_rootn_si
#define INTEGER_TYPE long
#define INT_RAND_FUNCTION() \
  (randlimb () % 16 == 0 ? randlong () : (long) (randlimb () % 31) - 15)
#include "tgeneric_ui.c"

int
main (void)
{
  tests_start_mpfr ();

  special ();

  /* The sign of the random value y (used to generate a potential bad case)
     is negative with a probability 256/512 = 1/2 for odd n, and never
     negative (probability 0/512) for even n (if y is negative, then
     (y^(2k))^(1/(2k)) is different from y, so that this would yield
     an error). */
  bad_cases (root2, pow2, "rootn[2]", 0, -256, 255, 4, 128, 80, 40);
  bad_cases (root3, pow3, "rootn[3]", 256, -256, 255, 4, 128, 200, 40);
  bad_cases (root4, pow4, "rootn[4]", 0, -256, 255, 4, 128, 320, 40);
  bad_cases (root5, pow5, "rootn[5]", 256, -256, 255, 4, 128, 440, 40);
  bad_cases (root17, pow17, "rootn[17]", 256, -256, 255, 4, 128, 800, 40);
  bad_cases (root120, pow120, "rootn[120]", 0, -256, 255, 4, 128, 800, 40);

  /* Ditto. */
  bad_cases (rootm2, powm2, "rootn[-2]", 0, -256, 255, 4, 128, 80, 40);
  bad_cases (rootm3, powm3, "rootn[-3]", 256, -256, 255, 4, 128, 200, 40);
  bad_cases (rootm4, powm4, "rootn[-4]", 0, -256, 255, 4, 128, 320, 40);
  bad_cases (rootm5, powm5, "rootn[-5]", 256, -256, 255, 4, 128, 440, 40);
  bad_cases (rootm17, powm17, "rootn[-17]", 256, -256, 255, 4, 128, 800, 40);
  bad_cases (rootm120, powm120, "rootn[-120]", 0, -256, 255, 4, 128, 800, 40);

  test_generic_ui (MPFR_PREC_MIN, 200, 30);

  tests_end_mpfr ();
  return 0;
}
