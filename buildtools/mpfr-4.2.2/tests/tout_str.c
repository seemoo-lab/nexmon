/* Test file for mpfr_out_str.

Copyright 1999, 2001-2025 Free Software Foundation, Inc.
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

/* FIXME: The output is not tested (thus coverage data are meaningless). */

#include <float.h>

#include "mpfr-test.h"

FILE *fout;

#define check(d,r,b) check4(d,r,b,53)

static void
check4 (double d, mpfr_rnd_t rnd, int base, int prec)
{
  mpfr_t x;

  mpfr_init2 (x, prec);
  mpfr_set_d (x, d, rnd);
  fprintf (fout, "%1.19e base %d %s:\n ", d, base, mpfr_print_rnd_mode (rnd));
  mpfr_out_str (fout, base, (base == 2) ? prec : 0, x, rnd);
  fputc ('\n', fout);
  mpfr_clear (x);
}

static void
special (void)
{
  mpfr_t x;
  unsigned int n;

  mpfr_init (x);

  mpfr_set_nan (x);
  n = mpfr_out_str (fout, 10, 0, x, MPFR_RNDN);
  fputc ('\n', fout);
  if (n != 5)
    {
      printf ("Error: mpfr_out_str (file, 10, 0, NaN, MPFR_RNDN) wrote %u "
              "characters instead of 5.\n", n);
      exit (1);
    }

  mpfr_set_inf (x, 1);
  n = mpfr_out_str (fout, 10, 0, x, MPFR_RNDN);
  fputc ('\n', fout);
  if (n != 5)
    {
      printf ("Error: mpfr_out_str (file, 10, 0, +Inf, MPFR_RNDN) wrote %u "
               "characters instead of 5.\n", n);
      exit (1);
    }

  mpfr_set_inf (x, -1);
  n = mpfr_out_str (fout, 10, 0, x, MPFR_RNDN);
  fputc ('\n', fout);
  if (n != 6)
    {
      printf ("Error: mpfr_out_str (file, 10, 0, -Inf, MPFR_RNDN) wrote %u "
               "characters instead of 6.\n", n);
      exit (1);
    }

  mpfr_set_ui (x, 0, MPFR_RNDN);
  n = mpfr_out_str (fout, 10, 0, x, MPFR_RNDN);
  fputc ('\n', fout);
  if (n != 1)
    {
      printf ("Error: mpfr_out_str (file, 10, 0, +0, MPFR_RNDN) wrote %u "
               "characters instead of 1.\n", n);
      exit (1);
    }

  mpfr_neg (x, x, MPFR_RNDN);
  n = mpfr_out_str (fout, 10, 0, x, MPFR_RNDN);
  fputc ('\n', fout);
  if (n != 2)
    {
      printf ("Error: mpfr_out_str (file, 10, 0, -0, MPFR_RNDN) wrote %u "
               "characters instead of 2.\n", n);
      exit (1);
    }

  mpfr_clear (x);
}

int
main (int argc, char *argv[])
{
  const char *fname = "tout_str_out.txt";
  int i, N = 10000;

  tests_start_mpfr ();

  /* with no argument: prints to a temporary file,
     tout_str N: prints N tests to stdout */
  if (argc == 1)
    {
      fout = fopen (fname, "w");
      if (fout == NULL)
        {
          perror (NULL);
          fprintf (stderr, "Failed to open \"%s\" for writing\n", fname);
          exit (1);
        }
    }
  else
    {
      fout = stdout;
      N = atoi (argv[1]);
    }

  special ();

  check (-1.37247529013405550000e+15, MPFR_RNDN, 7);
  check (-1.5674376729569697500e+15, MPFR_RNDN, 19);
  check (-5.71262771772792640000e-79, MPFR_RNDU, 16);
  check (DBL_NEG_ZERO, MPFR_RNDU, 7);
  check (-4.5306392613572974756e-308, MPFR_RNDN, 8);
  check (-6.7265890111403371523e-165, MPFR_RNDN, 4);
  check (-1.3242553591261807653e+156, MPFR_RNDN, 16);
  check (-6.606499965302424244461355e233, MPFR_RNDN, 10);
  check4 (1.0, MPFR_RNDN, 10, 120);
  check (1.0, MPFR_RNDU, 10);
  check (4.059650008e-83, MPFR_RNDN, 10);
  check (-7.4, MPFR_RNDN, 10);
  check (0.997, MPFR_RNDN, 10);
  check (-4.53063926135729747564e-308, MPFR_RNDN, 10);
  check (2.14478198760196000000e+16, MPFR_RNDN, 10);
  check (7.02293374921793516813e-84, MPFR_RNDN, 10);

  /* random tests */
  for (i = 0; i < N; i++)
    {
      double d;
      mpfr_rnd_t rnd;
      int b;

      d = DBL_RAND ();
      rnd = RND_RAND ();
      do
        b = (randlimb () % (62 + 36 + 1)) - 36;
      while (b > -2 && b < 2);
      check (d, rnd, b);
    }

  if (fout != stdout)
    {
      if (fclose (fout) != 0)
        {
          perror (NULL);
          fprintf (stderr, "Failed to close \"%s\"\n", fname);
          exit (1);
        }
      remove (fname);
    }

  tests_end_mpfr ();
  return 0;
}
