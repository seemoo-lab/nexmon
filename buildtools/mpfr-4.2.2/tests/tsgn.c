/* tsgn -- Test for the sign of a floating point number.

Copyright 2003, 2006-2025 Free Software Foundation, Inc.
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

#define MPFR_TESTS_TSGN 1
#include "mpfr-test.h"

static void
mpfr_sgn_test (mpfr_srcptr x, int res)
{
  mpfr_exp_t old_emin, old_emax, e;
  mpfr_flags_t flags;
  int i, v;

  old_emin = mpfr_get_emin ();
  old_emax = mpfr_get_emax ();

  for (i = 0; i <= 1; i++)
    {
      if (i != 0)
        {
          e = MPFR_IS_SINGULAR (x) ? MPFR_EMIN_MIN : mpfr_get_exp (x);
          set_emin (e);
          set_emax (e);
        }

      for (flags = 0; flags <= MPFR_FLAGS_ALL; flags++)
        {
          __gmpfr_flags = flags;
          v = (mpfr_sgn) (x);
          MPFR_ASSERTN (__gmpfr_flags == flags);
          if (VSIGN (v) != res)
            {
              printf ("mpfr_sgn function error (got %d) for ", v);
              mpfr_dump (x);
              exit (1);
            }
          v = mpfr_sgn (x);
          MPFR_ASSERTN (__gmpfr_flags == flags);
          if (VSIGN (v) != res)
            {
              printf ("mpfr_sgn macro error (got %d) for ", v);
              mpfr_dump (x);
              exit (1);
            }
        }
    }

  set_emin (old_emin);
  set_emax (old_emax);
}

static void
check_special (void)
{
  mpfr_t x;
  int i;

  mpfr_init (x);

  for (i = 1; i >= -1; i -= 2)
    {
      mpfr_set_zero (x, i);
      mpfr_sgn_test (x, 0);
      mpfr_set_inf (x, i);
      mpfr_sgn_test (x, i);
      mpfr_set_si (x, i, MPFR_RNDN);
      mpfr_sgn_test (x, i);
    }

  MPFR_SET_NAN (x);

  mpfr_clear_flags ();
  if ((mpfr_sgn) (x) != 0 || !mpfr_erangeflag_p ())
    {
      printf ("Sgn error for NaN.\n");
      exit (1);
    }

  mpfr_clear_flags ();
  if (mpfr_sgn (x) != 0 || !mpfr_erangeflag_p ())
    {
      printf ("Sgn error for NaN.\n");
      exit (1);
    }

  mpfr_clear (x);
}

static void
check_sgn(void)
{
  mpfr_t x;
  int i, s1, s2;

  mpfr_init(x);
  for(i = 0 ; i < 100 ; i++)
    {
      mpfr_urandomb (x, RANDS);
      if (i&1)
        {
          MPFR_SET_POS(x);
          s2 = 1;
        }
      else
        {
          MPFR_SET_NEG(x);
          s2 = -1;
        }
      s1 = mpfr_sgn(x);
      if (s1 < -1 || s1 > 1)
        {
          printf("Error for sgn: out of range.\n");
          goto lexit;
        }
      else if (MPFR_IS_NAN(x) || MPFR_IS_ZERO(x))
        {
          if (s1 != 0)
            {
              printf("Error for sgn: Nan or Zero should return 0.\n");
              goto lexit;
            }
        }
      else if (s1 != s2)
        {
          printf("Error for sgn. Return %d instead of %d.\n", s1, s2);
          goto lexit;
        }
    }
  mpfr_clear(x);
  return;

 lexit:
  mpfr_clear(x);
  exit(1);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  check_special ();
  check_sgn ();

  tests_end_mpfr ();
  return 0;
}
