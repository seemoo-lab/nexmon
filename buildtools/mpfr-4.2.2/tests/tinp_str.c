/* Test file for mpfr_inp_str.

Copyright 2004, 2006-2025 Free Software Foundation, Inc.
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

#include <ctype.h>

#include "mpfr-test.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

int
main (int argc, char *argv[])
{
  mpfr_t x;
  mpfr_t y;
  FILE *f;
  int i, n;

  tests_start_mpfr ();

  mpfr_init (x);
  mpfr_init (y);

  mpfr_set_prec (x, 15);
  f = src_fopen ("inp_str.dat", "r");
  if (f == NULL)
    {
      printf ("Error, can't open inp_str.dat\n");
      exit (1);
    }
  i = mpfr_inp_str (x, f, 10, MPFR_RNDN);
  if (i != 5 || mpfr_cmp_si0 (x, -1700))
    {
      printf ("Error in reading 1st line from file inp_str.dat (%d)\n", i);
      mpfr_dump (x);
      exit (1);
    }
  i = mpfr_inp_str (x, f, 10, MPFR_RNDN);
  if (i != 10 || mpfr_cmp_ui0 (x, 31415))
    {
      printf ("Error in reading 2nd line from file inp_str.dat (%d)\n", i);
      mpfr_dump (x);
      exit (1);
    }
  i = mpfr_inp_str (x, f, 10, MPFR_RNDN);
  if (i != 111 || mpfr_cmp_ui0 (x, 31416))
    {
      printf ("Error in reading 3rd line from file inp_str.dat (%d)\n", i);
      mpfr_dump (x);
      exit (1);
    }
  i = mpfr_inp_str (x, f, 10, MPFR_RNDN);
  if (i != 0)
    {
      printf ("Error in reading 4th line from file inp_str.dat (%d)\n", i);
      mpfr_dump (x);
      exit (1);
    }

  mpfr_set_prec (x, 53);
  mpfr_set_prec (y, 53);
  mpfr_set_str (y, "1.0010010100001110100101001110011010111011100001110010e226",
                2, MPFR_RNDN);
  for (n = 2; n < 63; n++)
    {
      i = mpfr_inp_str (x, f, n, MPFR_RNDN);
      if (i == 0 || !mpfr_equal_p (x, y))
        {
          printf ("Error in reading %dth line from file inp_str.dat (%d)\n",
                  n+3, i);
          mpfr_dump (x);
          exit (1);
        }
      mpfr_set_ui (x, 0, MPFR_RNDN);
    }

  /* This last test assumes that isspace(0) is false.
     If it isn't, set the locale to "C" in order to ensure that;
     but we check isspace(0) again, just in case. */
  if (! isspace (0)
#if defined(HAVE_LOCALE_H) && defined(HAVE_SETLOCALE)
      || (setlocale (LC_ALL, "C"), ! isspace (0))
#endif
      )
    {
      i = mpfr_inp_str (x, f, 10, MPFR_RNDN);
      if (i != 0)
        {
          printf ("Error in the '\\0' test (%d)\n", i);
          exit (1);
        }
      i = mpfr_inp_str (x, f, 10, MPFR_RNDN);
      if (i != 3 || mpfr_cmp_ui0 (x, 17))
        {
          printf ("Error following the '\\0' test (%d)\n", i);
          mpfr_dump (x);
          exit (1);
        }
    }

  fclose (f);

  mpfr_clear (x);
  mpfr_clear (y);

  tests_end_mpfr ();
  return 0;
}
