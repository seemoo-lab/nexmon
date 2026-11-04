/* Test mpfr_get_ld_2exp.

Copyright 2006-2025 Free Software Foundation, Inc.
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

#include <float.h>

#include "mpfr-test.h"

/* Check that hardware rounding doesn't make mpfr_get_ld_2exp return a value
   outside its defined range. */
static void
check_round (void)
{
  static const unsigned long data[] = {1, 32, 53, 54, 63, 64, 65, 127, 128, 256, 512 };
  mpfr_t  f;
  long double  got;
  long    got_exp;
  int     i, rnd_mode, neg;

  mpfr_init2 (f, 1024L);

  RND_LOOP (rnd_mode)
    {
      for (i = 0; i < (int) numberof (data); i++)
        {
          mpfr_set_ui (f, 1L, MPFR_RNDZ);
          mpfr_mul_2ui (f, f, data[i], MPFR_RNDZ);
          mpfr_sub_ui (f, f, 1L, MPFR_RNDZ);

          for (neg = 0; neg <= 1; neg++)
            {
              got = mpfr_get_ld_2exp (&got_exp, f, (mpfr_rnd_t) rnd_mode);

              if (neg == 0
                  ? (got < 0.5 || got >= 1.0)
                  : (got <= -1.0 || got > -0.5))
                {
                  printf  ("mpfr_get_ld_2exp wrong on 2**%lu-1\n", data[i]);
                  printf  ("result out of range, expect 0.5 <= got < 1.0\n");
                  printf  ("   rnd_mode = %d\n", rnd_mode);
                  printf  ("   data[i]  = %lu\n", data[i]);
                  printf  ("   f    ");
                  mpfr_out_str (stdout, 2, 0, f, MPFR_RNDN);
                  printf  ("\n");
                  ld_trace ("   got  ", got);
                  printf  ("   got exp  %ld\n", got_exp);
                  exit(1);
                }

              mpfr_neg (f, f, MPFR_RNDZ);
            }
        }
    }

  mpfr_clear (f);
}


static void
check_inf_nan (void)
{
#if !defined(MPFR_ERRDIVZERO)
  mpfr_t  x;
  double  d;
  long    exp;

  mpfr_init2 (x, 123);

  mpfr_set_inf (x, 1);
  d = (double) mpfr_get_ld_2exp (&exp, x, MPFR_RNDZ);
  MPFR_ASSERTN (d > 0);
  MPFR_ASSERTN (DOUBLE_ISINF (d));

  mpfr_set_inf (x, -1);
  d = (double) mpfr_get_ld_2exp (&exp, x, MPFR_RNDZ);
  MPFR_ASSERTN (d < 0);
  MPFR_ASSERTN (DOUBLE_ISINF (d));

  mpfr_set_nan (x);
  d = (double) mpfr_get_ld_2exp (&exp, x, MPFR_RNDZ);
  MPFR_ASSERTN (DOUBLE_ISNAN (d));

  mpfr_clear (x);
#endif
}

static void
bug20090520 (void)
{
  mpfr_t x;
  long double d, e;
  int i;

  mpfr_init (x);
  mpfr_set_ui (x, 1, MPFR_RNDN);
  d = 1.0;
  mpfr_div_2ui (x, x, 16383, MPFR_RNDN);
  for (i = 0; i < 16383; i++)
    d *= 0.5;
  e = mpfr_get_ld (x, MPFR_RNDN);
  if (e != d)
    {
      printf ("mpfr_get_ld(1e-16383) failed\n");
      printf ("expected %.20Le\n", d);
      printf ("got      %.20Le\n", e);
      exit (1);
    }
  mpfr_clear (x);
}

static void
bug20180904 (void)
{
#if defined(HAVE_LDOUBLE_IEEE_EXT_LITTLE) || \
    defined(HAVE_LDOUBLE_IEEE_EXT_BIG)
  mpfr_t x;
  long double d = 5.450797408381041489264061250159e-4937L;
  long double e;

  mpfr_init2 (x, 64);
  mpfr_set_str_binary (x, "0.1000100000000000000000000000000000000000000000000000000000000000E-16397");
  e = mpfr_get_ld (x, MPFR_RNDN);
  if (e != d)
    {
      printf ("Error in bug20180904:\n");
      printf ("expected %.30Le\n", d);
      printf ("got      %.30Le\n", e);
      exit (1);
    }
  mpfr_clear (x);
#endif
}

int
main (void)
{
  tests_start_mpfr ();
  mpfr_test_init ();

  bug20180904 ();
  bug20090520 ();

  check_round ();
  check_inf_nan ();

  tests_end_mpfr ();
  return 0;
}
