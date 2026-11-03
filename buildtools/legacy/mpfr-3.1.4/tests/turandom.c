/* Test file for mpfr_urandom

Copyright 1999-2004, 2006-2016 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

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
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include <stdio.h>
#include <stdlib.h>

#include "mpfr-test.h"

static void
test_urandom (long nbtests, mpfr_prec_t prec, mpfr_rnd_t rnd, long bit_index,
              int verbose)
{
  mpfr_t x;
  int *tab, size_tab, k, sh, xn;
  double d, av = 0, var = 0, chi2 = 0, th;
  mpfr_exp_t emin;
  mp_size_t limb_index = 0;
  mp_limb_t limb_mask = 0;
  long count = 0;
  int i;
  int inex = 1;

  size_tab = (nbtests >= 1000 ? nbtests / 50 : 20);
  tab = (int *) calloc (size_tab, sizeof(int));
  if (tab == NULL)
    {
      fprintf (stderr, "trandom: can't allocate memory in test_urandom\n");
      exit (1);
    }

  mpfr_init2 (x, prec);
  xn = 1 + (prec - 1) / mp_bits_per_limb;
  sh = xn * mp_bits_per_limb - prec;
  if (bit_index >= 0 && bit_index < prec)
    {
      /* compute the limb index and limb mask to fetch the bit #bit_index */
      limb_index = (prec - bit_index) / mp_bits_per_limb;
      i = 1 + bit_index - (bit_index / mp_bits_per_limb) * mp_bits_per_limb;
      limb_mask = MPFR_LIMB_ONE << (mp_bits_per_limb - i);
    }

  for (k = 0; k < nbtests; k++)
    {
      i = mpfr_urandom (x, RANDS, rnd);
      inex = (i != 0) && inex;
      /* check that lower bits are zero */
      if (MPFR_MANT(x)[0] & MPFR_LIMB_MASK(sh) && !MPFR_IS_ZERO (x))
        {
          printf ("Error: mpfr_urandom() returns invalid numbers:\n");
          mpfr_print_binary (x); puts ("");
          exit (1);
        }
      /* check that the value is in [0,1] */
      if (mpfr_cmp_ui (x, 0) < 0 || mpfr_cmp_ui (x, 1) > 0)
        {
          printf ("Error: mpfr_urandom() returns number outside [0, 1]:\n");
          mpfr_print_binary (x); puts ("");
          exit (1);
        }

      d = mpfr_get_d1 (x); av += d; var += d*d;
      i = (int)(size_tab * d);
      if (d == 1.0) i --;
      tab[i]++;

      if (limb_mask && (MPFR_MANT (x)[limb_index] & limb_mask))
        count ++;
    }

  if (inex == 0)
    {
      /* one call in the loop pretended to return an exact number! */
      printf ("Error: mpfr_urandom() returns a zero ternary value.\n");
      exit (1);
    }

  /* coverage test */
  emin = mpfr_get_emin ();
  for (k = 0; k < 5; k++)
    {
      set_emin (k+1);
      inex = mpfr_urandom (x, RANDS, rnd);
      if ((   (rnd == MPFR_RNDZ || rnd == MPFR_RNDD)
              && (!MPFR_IS_ZERO (x) || inex != -1))
          || ((rnd == MPFR_RNDU || rnd == MPFR_RNDA)
              && (mpfr_cmp_ui (x, 1 << k) != 0 || inex != +1))
          || (rnd == MPFR_RNDN
              && (k > 0 || mpfr_cmp_ui (x, 1 << k) != 0 || inex != +1)
              && (!MPFR_IS_ZERO (x) || inex != -1)))
        {
          printf ("Error: mpfr_urandom() do not handle correctly a restricted"
                  " exponent range.\nrounding mode: %s\nternary value: %d\n"
                  "random value: ", mpfr_print_rnd_mode (rnd), inex);
          mpfr_print_binary (x); puts ("");
          exit (1);
        }
    }
  set_emin (emin);

  mpfr_clear (x);
  if (!verbose)
    {
      free(tab);
      return;
    }

  av /= nbtests;
  var = (var / nbtests) - av * av;

  th = (double)nbtests / size_tab;
  printf ("Average = %.5f\nVariance = %.5f\n", av, var);
  printf ("Repartition for urandom with rounding mode %s. "
          "Each integer should be close to %d.\n",
         mpfr_print_rnd_mode (rnd), (int)th);

  for (k = 0; k < size_tab; k++)
    {
      chi2 += (tab[k] - th) * (tab[k] - th) / th;
      printf("%d ", tab[k]);
      if (((k+1) & 7) == 0)
        printf("\n");
    }

  printf("\nChi2 statistics value (with %d degrees of freedom) : %.5f\n",
         size_tab - 1, chi2);

  if (limb_mask)
    printf ("Bit #%ld is set %ld/%ld = %.1f %% of time\n",
            bit_index, count, nbtests, count * 100.0 / nbtests);

  puts ("");

  free(tab);
  return;
}

/* problem reported by Carl Witty */
static void
bug20100914 (void)
{
  mpfr_t x;
  gmp_randstate_t s;

#if __MPFR_GMP(4,2,0)
# define C1 "0.8488312"
# define C2 "0.8156509"
#else
# define C1 "0.6485367"
# define C2 "0.9362717"
#endif

  gmp_randinit_default (s);
  gmp_randseed_ui (s, 42);
  mpfr_init2 (x, 17);
  mpfr_urandom (x, s, MPFR_RNDN);
  if (mpfr_cmp_str1 (x, C1) != 0)
    {
      printf ("Error in bug20100914, expected " C1 ", got ");
      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  mpfr_urandom (x, s, MPFR_RNDN);
  if (mpfr_cmp_str1 (x, C2) != 0)
    {
      printf ("Error in bug20100914, expected " C2 ", got ");
      mpfr_out_str (stdout, 10, 0, x, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  mpfr_clear (x);
  gmp_randclear (s);
}

int
main (int argc, char *argv[])
{
  long nbtests;
  mpfr_prec_t prec;
  int verbose = 0;
  int rnd;
  long bit_index;

  tests_start_mpfr ();

  if (argc > 1)
    verbose = 1;

  nbtests = 10000;
  if (argc > 1)
    {
      long a = atol(argv[1]);
      if (a != 0)
        nbtests = a;
    }

  if (argc <= 2)
    prec = 1000;
  else
    prec = atol(argv[2]);

  if (argc <= 3)
    bit_index = -1;
  else
    {
      bit_index = atol(argv[3]);
      if (bit_index >= prec)
        {
          printf ("Warning. Cannot compute the bit frequency: the given bit "
                  "index (= %ld) is not less than the precision (= %ld).\n",
                  bit_index, (long) prec);
          bit_index = -1;
        }
    }

  RND_LOOP(rnd)
    {
      test_urandom (nbtests, prec, (mpfr_rnd_t) rnd, bit_index, verbose);

      if (argc == 1)  /* check also small precision */
        {
          test_urandom (nbtests, 2, (mpfr_rnd_t) rnd, -1, 0);
        }
    }

  bug20100914 ();

  tests_end_mpfr ();
  return 0;
}
