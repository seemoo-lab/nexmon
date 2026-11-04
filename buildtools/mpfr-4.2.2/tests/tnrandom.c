/* Test file for mpfr_nrandom

Copyright 2011-2025 Free Software Foundation, Inc.
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
test_special (mpfr_prec_t p)
{
  mpfr_t x;
  int inexact;

  mpfr_init2 (x, p);

  inexact = mpfr_nrandom (x, RANDS, MPFR_RNDN);
  if (inexact == 0)
    {
      printf ("Error: mpfr_nrandom() returns a zero ternary value.\n");
      exit (1);
    }

  mpfr_clear (x);
}

#define NRES 10

static const char *res[NRES] = {
  "-2.07609e2d96da78b2d6bea3ab30d4359222a82f8a35e4e4464303ad4808f57458@0",
  "1.a4650a963ab9f266ed009ee96c8788f6b88212f5f2a4d4aef65db2a9e57c44bc@-1",
  "c.0b3cda7f370a36febed972dbb47f2503f7e08a651edbf12d0303d968257841b0@-1",
  "-7.4dffd19868c4bc2ec5b6c8311e0f190e1c97b575b7fdabec897e5de2a1d802b8@-1",
  "1.40f6204ded71a4346ed17094863347b8c735e62712cc0b4d0c5402ee310d9714@0",
  "-3.09fd2a1fc234e23bfa048dbf1e7850ac6cbea2514a0f3ce011e964d9f331cbcc@-1",
  "-1.104e769aadb5fce5a7ad1c546e91b889829a76920c7cc7ac4cbd12009451ce90@0",
  "3.0a08181e342b02187463c0025f895b41ddb7076c5bf157e3b898e9248baf4ad4@-1",
  "-d.44fda7a51276b722ebc88dd016b7d9d7ea5ba682282a42cdef6948312e5dcf70@-1",
  "1.5bf69aff31bb3e6430cc263fdd45ef2c70a779984e764524bc35a9cb4a430dd0@0" };

/* If checkval is true, check the obtained results by using a fixed seed
   for reproducibility. */
static void
test_nrandom (long nbtests, mpfr_prec_t prec, mpfr_rnd_t rnd,
              int verbose, int checkval)
{
  gmp_randstate_t s;
  mpfr_t *t;
  int i, inexact;

  if (checkval)
    {
      gmp_randinit_default (s);
      gmp_randseed_ui (s, 17);
      nbtests = NRES;
    }

  t = (mpfr_t *) tests_allocate (nbtests * sizeof (mpfr_t));

  for (i = 0; i < nbtests; ++i)
    mpfr_init2 (t[i], prec);

  for (i = 0; i < nbtests; i++)
    {
      inexact = mpfr_nrandom (t[i], checkval ? s : RANDS, MPFR_RNDN);

      if (checkval && mpfr_cmp_str (t[i], res[i], 16, MPFR_RNDN) != 0)
        {
          printf ("Unexpected value in test_nrandom().\n"
                  "Expected %s\n"
                  "Got      ", res[i]);
          mpfr_out_str (stdout, 16, 0, t[i], MPFR_RNDN);
          printf ("\n");
          exit (1);
        }

      if (inexact == 0)
        {
          /* one call in the loop pretended to return an exact number! */
          printf ("Error: mpfr_nrandom() returns a zero ternary value.\n");
          exit (1);
        }
    }

#if defined(HAVE_STDARG) && !defined(MPFR_USE_MINI_GMP)
  if (verbose)
    {
      mpfr_t av, va, tmp;

      mpfr_init2 (av, prec);
      mpfr_init2 (va, prec);
      mpfr_init2 (tmp, prec);

      mpfr_set_ui (av, 0, MPFR_RNDN);
      mpfr_set_ui (va, 0, MPFR_RNDN);
      for (i = 0; i < nbtests; ++i)
        {
          mpfr_add (av, av, t[i], MPFR_RNDN);
          mpfr_sqr (tmp, t[i], MPFR_RNDN);
          mpfr_add (va, va, tmp, MPFR_RNDN);
        }
      mpfr_div_ui (av, av, nbtests, MPFR_RNDN);
      mpfr_div_ui (va, va, nbtests, MPFR_RNDN);
      mpfr_sqr (tmp, av, MPFR_RNDN);
      mpfr_sub (va, va, av, MPFR_RNDN);

      mpfr_printf ("Average = %.5Rf\nVariance = %.5Rf\n", av, va);
      mpfr_clear (av);
      mpfr_clear (va);
      mpfr_clear (tmp);
    }
#endif /* HAVE_STDARG */

  for (i = 0; i < nbtests; ++i)
    mpfr_clear (t[i]);
  tests_free (t, nbtests * sizeof (mpfr_t));
  if (checkval)
    gmp_randclear (s);
  return;
}


int
main (int argc, char *argv[])
{
  long nbtests;
  int verbose;

  tests_start_mpfr ();

  verbose = 0;
  nbtests = 10;
  if (argc > 1)
    {
      /* Number of values in argument. Note that the mpfr_clear loop above
         is in O(n^2) until the FIXME for tests_memory_find() in memory.c
         is resolved (the search in tests_memory_find() is in O(n), while
         it could be in almost constant time). */
      long a = atol (argv[1]);
      verbose = 1;
      if (a != 0)
        nbtests = a;
    }

  test_nrandom (nbtests, 420, MPFR_RNDN, verbose, 0);

#ifndef MPFR_USE_MINI_GMP
  /* The random generator in mini-gmp is not deterministic. */
  test_nrandom (0, 256, MPFR_RNDN, 0, 1);
#endif /* MPFR_USE_MINI_GMP */

  test_special (2);
  test_special (42000);

  tests_end_mpfr ();
  return 0;
}
