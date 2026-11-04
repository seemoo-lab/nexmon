/* tbuildopt.c -- test file for mpfr_buildopt_tls_p and
   mpfr_buildopt_decimal_p.

Copyright 2009-2025 Free Software Foundation, Inc.
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
check_tls_p (void)
{
#ifdef MPFR_USE_THREAD_SAFE
  if (!mpfr_buildopt_tls_p())
    {
      printf ("Error: mpfr_buildopt_tls_p should return true\n");
      exit (1);
    }
#else
  if (mpfr_buildopt_tls_p())
    {
      printf ("Error: mpfr_buildopt_tls_p should return false\n");
      exit (1);
    }
#endif
}

static void
check_decimal_p (void)
{
#ifdef MPFR_WANT_DECIMAL_FLOATS
  if (!mpfr_buildopt_decimal_p())
    {
      printf ("Error: mpfr_buildopt_decimal_p should return true\n");
      exit (1);
    }
#else
  if (mpfr_buildopt_decimal_p())
    {
      printf ("Error: mpfr_buildopt_decimal_p should return false\n");
      exit (1);
    }
#endif
}

static void
check_float128_p (void)
{
#ifdef MPFR_WANT_FLOAT128
  if (!mpfr_buildopt_float128_p())
    {
      printf ("Error: mpfr_buildopt_float128_p should return true\n");
      exit (1);
    }
#else
  if (mpfr_buildopt_float128_p())
    {
      printf ("Error: mpfr_buildopt_float128_p should return false\n");
      exit (1);
    }
#endif
}

static void
check_gmpinternals_p (void)
{
#if defined(MPFR_HAVE_GMP_IMPL) || defined(WANT_GMP_INTERNALS)
  if (!mpfr_buildopt_gmpinternals_p())
    {
      printf ("Error: mpfr_buildopt_gmpinternals_p should return true\n");
      exit (1);
    }
#else
  if (mpfr_buildopt_gmpinternals_p())
    {
      printf ("Error: mpfr_buildopt_gmpinternals_p should return false\n");
      exit (1);
    }
#endif
}

static void
check_sharedcache_p (void)
{
#if defined(MPFR_WANT_SHARED_CACHE)
  if (!mpfr_buildopt_sharedcache_p ())
    {
      printf ("Error: mpfr_buildopt_sharedcache_p should return true\n");
      exit (1);
    }
#else
  if (mpfr_buildopt_sharedcache_p ())
    {
      printf ("Error: mpfr_buildopt_sharedcache_p should return false\n");
      exit (1);
    }
#endif
}

int
main (void)
{
  tests_start_mpfr ();

  check_tls_p();
  check_decimal_p();
  check_float128_p();
  check_gmpinternals_p();
  check_sharedcache_p ();
  {
    const char *s = mpfr_buildopt_tune_case ();
    (void) strlen (s);
  }

  tests_end_mpfr ();
  return 0;
}
