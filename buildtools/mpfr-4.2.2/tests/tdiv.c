/* Test file for mpfr_div (and some mpfr_div_ui, etc. tests).

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

#include "mpfr-test.h"

static void
check_equal (mpfr_srcptr a, mpfr_srcptr a2, const char *s,
             mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t r)
{
  if (SAME_VAL (a, a2))
    return;
  if (r == MPFR_RNDF) /* RNDF might return different values */
    return;
  printf ("Error in %s\n", mpfr_print_rnd_mode (r));
  printf ("b  = ");
  mpfr_dump (b);
  printf ("c  = ");
  mpfr_dump (c);
  printf ("mpfr_div    result: ");
  mpfr_dump (a);
  printf ("%s result: ", s);
  mpfr_dump (a2);
  exit (1);
}

static int
mpfr_all_div (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t r)
{
  mpfr_t a2;
  unsigned int oldflags, newflags;
  int inex, inex2;

  oldflags = __gmpfr_flags;
  inex = mpfr_div (a, b, c, r);

  /* this test makes no sense for RNDF, since it compares the ternary value
     and the flags */
  if (a == b || a == c || r == MPFR_RNDF)
    return inex;

  newflags = __gmpfr_flags;

  mpfr_init2 (a2, MPFR_PREC (a));

  if (mpfr_integer_p (b) && ! (MPFR_IS_ZERO (b) && MPFR_IS_NEG (b)))
    {
      /* b is an integer, but not -0 (-0 is rejected as
         it becomes +0 when converted to an integer). */
      if (mpfr_fits_ulong_p (b, MPFR_RNDA))
        {
          __gmpfr_flags = oldflags;
          inex2 = mpfr_ui_div (a2, mpfr_get_ui (b, MPFR_RNDN), c, r);
          if (!SAME_SIGN (inex2, inex))
            {
              printf ("Error for ternary value (rnd=%s), mpfr_div %d, mpfr_ui_div %d\n",
                      mpfr_print_rnd_mode (r), inex, inex2);
              exit (1);
            }
          if (__gmpfr_flags != newflags)
            {
              printf ("Error for flags, mpfr_div %d, mpfr_ui_div %d\n",
                      newflags, __gmpfr_flags);
              exit (1);
            }
          check_equal (a, a2, "mpfr_ui_div", b, c, r);
        }
      if (mpfr_fits_slong_p (b, MPFR_RNDA))
        {
          __gmpfr_flags = oldflags;
          inex2 = mpfr_si_div (a2, mpfr_get_si (b, MPFR_RNDN), c, r);
          MPFR_ASSERTN (SAME_SIGN (inex2, inex));
          MPFR_ASSERTN (__gmpfr_flags == newflags);
          check_equal (a, a2, "mpfr_si_div", b, c, r);
        }
    }

  if (mpfr_integer_p (c) && ! (MPFR_IS_ZERO (c) && MPFR_IS_NEG (c)))
    {
      /* c is an integer, but not -0 (-0 is rejected as
         it becomes +0 when converted to an integer). */
      if (mpfr_fits_ulong_p (c, MPFR_RNDA))
        {
          __gmpfr_flags = oldflags;
          inex2 = mpfr_div_ui (a2, b, mpfr_get_ui (c, MPFR_RNDN), r);
          MPFR_ASSERTN (SAME_SIGN (inex2, inex));
          MPFR_ASSERTN (__gmpfr_flags == newflags);
          check_equal (a, a2, "mpfr_div_ui", b, c, r);
        }
      if (mpfr_fits_slong_p (c, MPFR_RNDA))
        {
          __gmpfr_flags = oldflags;
          inex2 = mpfr_div_si (a2, b, mpfr_get_si (c, MPFR_RNDN), r);
          MPFR_ASSERTN (SAME_SIGN (inex2, inex));
          MPFR_ASSERTN (__gmpfr_flags == newflags);
          check_equal (a, a2, "mpfr_div_si", b, c, r);
        }
    }

  mpfr_clear (a2);

  return inex;
}

#ifdef CHECK_EXTERNAL
static int
test_div (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  int res;
  int ok = rnd_mode == MPFR_RNDN && mpfr_number_p (b) && mpfr_number_p (c);
  if (ok)
    {
      mpfr_print_raw (b);
      printf (" ");
      mpfr_print_raw (c);
    }
  res = mpfr_all_div (a, b, c, rnd_mode);
  if (ok)
    {
      printf (" ");
      mpfr_print_raw (a);
      printf ("\n");
    }
  return res;
}
#else
#define test_div mpfr_all_div
#endif

#define check53(n, d, rnd, res) check4(n, d, rnd, 53, res)

/* return 0 iff a and b are of the same sign */
static int
inex_cmp (int a, int b)
{
  if (a > 0)
    return (b > 0) ? 0 : 1;
  else if (a == 0)
    return (b == 0) ? 0 : 1;
  else
    return (b < 0) ? 0 : 1;
}

static void
check4 (const char *Ns, const char *Ds, mpfr_rnd_t rnd_mode, int p,
        const char *Qs)
{
  mpfr_t q, n, d;

  mpfr_inits2 (p, q, n, d, (mpfr_ptr) 0);
  mpfr_set_str1 (n, Ns);
  mpfr_set_str1 (d, Ds);
  test_div(q, n, d, rnd_mode);
  if (mpfr_cmp_str (q, Qs, ((p==53) ? 10 : 2), MPFR_RNDN) )
    {
      printf ("mpfr_div failed for n=%s, d=%s, p=%d, rnd_mode=%s\n",
              Ns, Ds, p, mpfr_print_rnd_mode (rnd_mode));
      printf ("got      ");
      mpfr_dump (q);
      mpfr_set_str (q, Qs, ((p==53) ? 10 : 2), MPFR_RNDN);
      printf ("expected ");
      mpfr_dump (q);
      exit (1);
    }
  mpfr_clears (q, n, d, (mpfr_ptr) 0);
}

static void
check24 (const char *Ns, const char *Ds, mpfr_rnd_t rnd_mode, const char *Qs)
{
  mpfr_t q, n, d;

  mpfr_inits2 (24, q, n, d, (mpfr_ptr) 0);

  mpfr_set_str1 (n, Ns);
  mpfr_set_str1 (d, Ds);
  test_div(q, n, d, rnd_mode);
  if (mpfr_cmp_str1 (q, Qs) )
    {
      printf ("mpfr_div failed for n=%s, d=%s, prec=24, rnd_mode=%s\n",
             Ns, Ds, mpfr_print_rnd_mode(rnd_mode));
      printf ("expected quotient is %s, got ", Qs);
      mpfr_out_str(stdout,10,0,q, MPFR_RNDN); putchar('\n');
      exit (1);
    }
  mpfr_clears (q, n, d, (mpfr_ptr) 0);
}

/* the following examples come from the paper "Number-theoretic Test
   Generation for Directed Rounding" from Michael Parks, Table 2 */
static void
check_float(void)
{
  check24("70368760954880.0", "8388609.0", MPFR_RNDN, "8.388609e6");
  check24("140737479966720.0", "16777213.0", MPFR_RNDN, "8.388609e6");
  check24("70368777732096.0", "8388611.0", MPFR_RNDN, "8.388609e6");
  check24("105553133043712.0", "12582911.0", MPFR_RNDN, "8.38861e6");
  /* the exponent for the following example was forgotten in
     the Arith'14 version of Parks' paper */
  check24 ("12582913.0", "12582910.0", MPFR_RNDN, "1.000000238");
  check24 ("105553124655104.0", "12582910.0", MPFR_RNDN, "8388610.0");
  check24("140737479966720.0", "8388609.0", MPFR_RNDN, "1.6777213e7");
  check24("70368777732096.0", "8388609.0", MPFR_RNDN, "8.388611e6");
  check24("105553133043712.0", "8388610.0", MPFR_RNDN, "1.2582911e7");
  check24("105553124655104.0", "8388610.0", MPFR_RNDN, "1.258291e7");

  check24("70368760954880.0", "8388609.0", MPFR_RNDZ, "8.388608e6");
  check24("140737479966720.0", "16777213.0", MPFR_RNDZ, "8.388609e6");
  check24("70368777732096.0", "8388611.0", MPFR_RNDZ, "8.388608e6");
  check24("105553133043712.0", "12582911.0", MPFR_RNDZ, "8.38861e6");
  check24("12582913.0", "12582910.0", MPFR_RNDZ, "1.000000238");
  check24 ("105553124655104.0", "12582910.0", MPFR_RNDZ, "8388610.0");
  check24("140737479966720.0", "8388609.0", MPFR_RNDZ, "1.6777213e7");
  check24("70368777732096.0", "8388609.0", MPFR_RNDZ, "8.38861e6");
  check24("105553133043712.0", "8388610.0", MPFR_RNDZ, "1.2582911e7");
  check24("105553124655104.0", "8388610.0", MPFR_RNDZ, "1.258291e7");

  check24("70368760954880.0", "8388609.0", MPFR_RNDU, "8.388609e6");
  check24("140737479966720.0", "16777213.0", MPFR_RNDU, "8.38861e6");
  check24("70368777732096.0", "8388611.0", MPFR_RNDU, "8.388609e6");
  check24("105553133043712.0", "12582911.0", MPFR_RNDU, "8.388611e6");
  check24("12582913.0", "12582910.0", MPFR_RNDU, "1.000000357");
  check24 ("105553124655104.0", "12582910.0", MPFR_RNDU, "8388611.0");
  check24("140737479966720.0", "8388609.0", MPFR_RNDU, "1.6777214e7");
  check24("70368777732096.0", "8388609.0", MPFR_RNDU, "8.388611e6");
  check24("105553133043712.0", "8388610.0", MPFR_RNDU, "1.2582912e7");
  check24("105553124655104.0", "8388610.0", MPFR_RNDU, "1.2582911e7");

  check24("70368760954880.0", "8388609.0", MPFR_RNDD, "8.388608e6");
  check24("140737479966720.0", "16777213.0", MPFR_RNDD, "8.388609e6");
  check24("70368777732096.0", "8388611.0", MPFR_RNDD, "8.388608e6");
  check24("105553133043712.0", "12582911.0", MPFR_RNDD, "8.38861e6");
  check24("12582913.0", "12582910.0", MPFR_RNDD, "1.000000238");
  check24 ("105553124655104.0", "12582910.0", MPFR_RNDD, "8388610.0");
  check24("140737479966720.0", "8388609.0", MPFR_RNDD, "1.6777213e7");
  check24("70368777732096.0", "8388609.0", MPFR_RNDD, "8.38861e6");
  check24("105553133043712.0", "8388610.0", MPFR_RNDD, "1.2582911e7");
  check24("105553124655104.0", "8388610.0", MPFR_RNDD, "1.258291e7");

  check24("70368760954880.0", "8388609.0", MPFR_RNDA, "8.388609e6");
}

static void
check_double(void)
{
  check53("0.0", "1.0", MPFR_RNDZ, "0.0");
  check53("-7.4988969224688591e63", "4.8816866450288732e306", MPFR_RNDD,
          "-1.5361282826510687291e-243");
  check53("-1.33225773037748601769e+199", "3.63449540676937123913e+79",
          MPFR_RNDZ, "-3.6655920045905428978e119");
  check53("9.89438396044940256501e-134", "5.93472984109987421717e-67",MPFR_RNDU,
          "1.6672003992376663654e-67");
  check53("9.89438396044940256501e-134", "5.93472984109987421717e-67",MPFR_RNDA,
          "1.6672003992376663654e-67");
  check53("9.89438396044940256501e-134", "-5.93472984109987421717e-67",
          MPFR_RNDU, "-1.6672003992376663654e-67");
  check53("-4.53063926135729747564e-308", "7.02293374921793516813e-84",
          MPFR_RNDD, "-6.4512060388748850857e-225");
  check53("6.25089225176473806123e-01","-2.35527154824420243364e-230",
          MPFR_RNDD, "-2.6540006635008291192e229");
  check53("6.25089225176473806123e-01","-2.35527154824420243364e-230",
          MPFR_RNDA, "-2.6540006635008291192e229");
  check53("6.52308934689126e15", "-1.62063546601505417497e273", MPFR_RNDN,
          "-4.0250194961676020848e-258");
  check53("1.04636807108079349236e-189", "3.72295730823253012954e-292",
          MPFR_RNDZ, "2.810583051186143125e102");
  /* problems found by Kevin under HP-PA */
  check53 ("2.861044553323177e-136", "-1.1120354257068143e+45", MPFR_RNDZ,
           "-2.5727998292003016e-181");
  check53 ("-4.0559157245809205e-127", "-1.1237723844524865e+77", MPFR_RNDN,
           "3.6091968273068081e-204");
  check53 ("-1.8177943561493235e-93", "-8.51233984260364e-104", MPFR_RNDU,
           "2.1354814184595821e+10");
}

static void
check_64(void)
{
  mpfr_t x,y,z;

  mpfr_inits2 (64, x, y, z, (mpfr_ptr) 0);

  mpfr_set_str_binary(x, "1.00100100110110101001010010101111000001011100100101010000000000E54");
  mpfr_set_str_binary(y, "1.00000000000000000000000000000000000000000000000000000000000000E584");
  test_div(z, x, y, MPFR_RNDU);
  if (mpfr_cmp_str (z, "0.1001001001101101010010100101011110000010111001001010100000000000E-529", 2, MPFR_RNDN))
    {
      printf ("Error for tdiv for MPFR_RNDU and p=64\nx=");
      mpfr_dump (x);
      printf ("y=");
      mpfr_dump (y);
      printf ("got      ");
      mpfr_dump (z);
      printf ("expected 0.1001001001101101010010100101011110000010111001001010100000000000E-529\n");
      exit (1);
    }

  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

static void
check_convergence (void)
{
  mpfr_t x, y; int i, j;

  mpfr_init2(x, 130);
  mpfr_set_str_binary(x, "0.1011111101011010101000001010011111101000011100011101010011111011000011001010000000111100100111110011001010110100100001001000111001E6944");
  mpfr_init2(y, 130);
  mpfr_set_ui(y, 5, MPFR_RNDN);
  test_div(x, x, y, MPFR_RNDD); /* exact division */

  mpfr_set_prec(x, 64);
  mpfr_set_prec(y, 64);
  mpfr_set_str_binary(x, "0.10010010011011010100101001010111100000101110010010101E55");
  mpfr_set_str_binary(y, "0.1E585");
  test_div(x, x, y, MPFR_RNDN);
  mpfr_set_str_binary(y, "0.10010010011011010100101001010111100000101110010010101E-529");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_div for prec=64, rnd=MPFR_RNDN\n");
      printf ("got        "); mpfr_dump (x);
      printf ("instead of "); mpfr_dump (y);
      exit(1);
    }

  for (i=32; i<=64; i+=32)
    {
      mpfr_set_prec(x, i);
      mpfr_set_prec(y, i);
      mpfr_set_ui(x, 1, MPFR_RNDN);
      RND_LOOP(j)
        {
          mpfr_set_ui (y, 1, MPFR_RNDN);
          test_div (y, x, y, (mpfr_rnd_t) j);
          if (mpfr_cmp_ui (y, 1))
            {
              printf ("mpfr_div failed for x=1.0, y=1.0, prec=%d rnd=%s\n",
                      i, mpfr_print_rnd_mode ((mpfr_rnd_t) j));
              printf ("got "); mpfr_dump (y);
              exit (1);
            }
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
}

#define KMAX 10000

/* given y = o(x/u), x, u, find the inexact flag by
   multiplying y by u */
static int
get_inexact (mpfr_ptr y, mpfr_ptr x, mpfr_ptr u)
{
  mpfr_t xx;
  int inex;
  mpfr_init2 (xx, mpfr_get_prec (y) + mpfr_get_prec (u));
  mpfr_mul (xx, y, u, MPFR_RNDN); /* exact */
  inex = mpfr_cmp (xx, x);
  mpfr_clear (xx);
  return inex;
}

static void
check_hard (void)
{
  mpfr_t u, v, q, q2;
  mpfr_prec_t precu, precv, precq;
  int rnd;
  int inex, inex2, i, j;

  mpfr_init (q);
  mpfr_init (q2);
  mpfr_init (u);
  mpfr_init (v);

  for (precq = MPFR_PREC_MIN; precq <= 64; precq ++)
    {
      mpfr_set_prec (q, precq);
      mpfr_set_prec (q2, precq + 1);
      for (j = 0; j < 2; j++)
        {
          if (j == 0)
            {
              do
                {
                  mpfr_urandomb (q2, RANDS);
                }
              while (mpfr_cmp_ui (q2, 0) == 0);
            }
          else /* use q2=1 */
            mpfr_set_ui (q2, 1, MPFR_RNDN);
      for (precv = precq; precv <= 10 * precq; precv += precq)
        {
          mpfr_set_prec (v, precv);
          do
            {
              mpfr_urandomb (v, RANDS);
            }
          while (mpfr_cmp_ui (v, 0) == 0);
          for (precu = precq; precu <= 10 * precq; precu += precq)
            {
              mpfr_set_prec (u, precu);
              mpfr_mul (u, v, q2, MPFR_RNDN);
              mpfr_nextbelow (u);
              for (i = 0; i <= 2; i++)
                {
                  RND_LOOP_NO_RNDF (rnd)
                    {
                      inex = test_div (q, u, v, (mpfr_rnd_t) rnd);
                      inex2 = get_inexact (q, u, v);
                      if (inex_cmp (inex, inex2))
                        {
                          printf ("Wrong inexact flag for rnd=%s: expected %d, got %d\n",
                                  mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), inex2, inex);
                          printf ("u=  "); mpfr_dump (u);
                          printf ("v=  "); mpfr_dump (v);
                          printf ("q=  "); mpfr_dump (q);
                          mpfr_set_prec (q2, precq + precv);
                          mpfr_mul (q2, q, v, MPFR_RNDN);
                          printf ("q*v="); mpfr_dump (q2);
                          exit (1);
                        }
                    }
                  mpfr_nextabove (u);
                }
            }
        }
        }
    }

  mpfr_clear (q);
  mpfr_clear (q2);
  mpfr_clear (u);
  mpfr_clear (v);
}

static void
check_lowr (void)
{
  mpfr_t x, y, z, z2, z3, tmp;
  int k, c, c2;


  mpfr_init2 (x, 1000);
  mpfr_init2 (y, 100);
  mpfr_init2 (tmp, 850);
  mpfr_init2 (z, 10);
  mpfr_init2 (z2, 10);
  mpfr_init2 (z3, 50);

  for (k = 1; k < KMAX; k++)
    {
      do
        {
          mpfr_urandomb (z, RANDS);
        }
      while (mpfr_cmp_ui (z, 0) == 0);
      do
        {
          mpfr_urandomb (tmp, RANDS);
        }
      while (mpfr_cmp_ui (tmp, 0) == 0);
      mpfr_mul (x, z, tmp, MPFR_RNDN); /* exact */
      c = test_div (z2, x, tmp, MPFR_RNDN);

      if (c || mpfr_cmp (z2, z))
        {
          printf ("Error in mpfr_div rnd=MPFR_RNDN\n");
          printf ("got        "); mpfr_dump (z2);
          printf ("instead of "); mpfr_dump (z);
          printf ("inex flag = %d, expected 0\n", c);
          exit (1);
        }
    }

  /* x has still precision 1000, z precision 10, and tmp prec 850 */
  mpfr_set_prec (z2, 9);
  for (k = 1; k < KMAX; k++)
    {
      mpfr_urandomb (z, RANDS);
      do
        {
          mpfr_urandomb (tmp, RANDS);
        }
      while (mpfr_cmp_ui (tmp, 0) == 0);
      mpfr_mul (x, z, tmp, MPFR_RNDN); /* exact */
      c = test_div (z2, x, tmp, MPFR_RNDN);
      /* since z2 has one less bit that z, either the division is exact
         if z is representable on 9 bits, or we have an even round case */

      c2 = get_inexact (z2, x, tmp);
      if ((mpfr_cmp (z2, z) == 0 && c) || inex_cmp (c, c2))
        {
          printf ("Error in mpfr_div rnd=MPFR_RNDN\n");
          printf ("got        "); mpfr_dump (z2);
          printf ("instead of "); mpfr_dump (z);
          printf ("inex flag = %d, expected %d\n", c, c2);
          exit (1);
        }
      else if (c == 2)
        {
          mpfr_nexttoinf (z);
          if (mpfr_cmp(z2, z))
            {
              printf ("Error in mpfr_div [even rnd?] rnd=MPFR_RNDN\n");
              printf ("Dividing ");
              printf ("got        "); mpfr_dump (z2);
              printf ("instead of "); mpfr_dump (z);
              printf ("inex flag = %d\n", 1);
              exit (1);
            }
        }
      else if (c == -2)
        {
          mpfr_nexttozero (z);
          if (mpfr_cmp(z2, z))
            {
              printf ("Error in mpfr_div [even rnd?] rnd=MPFR_RNDN\n");
              printf ("Dividing ");
              printf ("got        "); mpfr_dump (z2);
              printf ("instead of "); mpfr_dump (z);
              printf ("inex flag = %d\n", 1);
              exit (1);
            }
        }
    }

  mpfr_set_prec(x, 1000);
  mpfr_set_prec(y, 100);
  mpfr_set_prec(tmp, 850);
  mpfr_set_prec(z, 10);
  mpfr_set_prec(z2, 10);

  /* almost exact divisions */
  for (k = 1; k < KMAX; k++)
    {
      do
        {
          mpfr_urandomb (z, RANDS);
        }
      while (mpfr_cmp_ui (z, 0) == 0);
      do
        {
          mpfr_urandomb (tmp, RANDS);
        }
      while (mpfr_cmp_ui (tmp, 0) == 0);
      mpfr_mul(x, z, tmp, MPFR_RNDN);
      mpfr_set(y, tmp, MPFR_RNDD);
      mpfr_nexttoinf (x);

      c = test_div(z2, x, y, MPFR_RNDD);
      test_div(z3, x, y, MPFR_RNDD);
      mpfr_set(z, z3, MPFR_RNDD);

      if (c != -1 || mpfr_cmp(z2, z))
        {
          printf ("Error in mpfr_div rnd=MPFR_RNDD\n");
          printf ("got        "); mpfr_dump (z2);
          printf ("instead of "); mpfr_dump (z);
          printf ("inex flag = %d\n", c);
          exit (1);
        }

      mpfr_set (y, tmp, MPFR_RNDU);
      test_div (z3, x, y, MPFR_RNDU);
      mpfr_set (z, z3, MPFR_RNDU);
      c = test_div (z2, x, y, MPFR_RNDU);
      if (c != 1 || mpfr_cmp (z2, z))
        {
          printf ("Error in mpfr_div rnd=MPFR_RNDU\n");
          printf ("u="); mpfr_dump (x);
          printf ("v="); mpfr_dump (y);
          printf ("got        "); mpfr_dump (z2);
          printf ("instead of "); mpfr_dump (z);
          printf ("inex flag = %d\n", c);
          exit (1);
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (z2);
  mpfr_clear (z3);
  mpfr_clear (tmp);
}

#define MAX_PREC 128

static void
check_inexact (void)
{
  mpfr_t x, y, z, u;
  mpfr_prec_t px, py, pu;
  int inexact, cmp;
  mpfr_rnd_t rnd;

  mpfr_init (x);
  mpfr_init (y);
  mpfr_init (z);
  mpfr_init (u);

  mpfr_set_prec (x, 28);
  mpfr_set_prec (y, 28);
  mpfr_set_prec (z, 1023);
  mpfr_set_str_binary (x, "0.1000001001101101111100010011E0");
  mpfr_set_str (z, "48284762641021308813686974720835219181653367326353400027913400579340343320519877153813133510034402932651132854764198688352364361009429039801248971901380781746767119334993621199563870113045276395603170432175354501451429471578325545278975153148347684600400321033502982713296919861760382863826626093689036010394", 10, MPFR_RNDN);
  mpfr_div (x, x, z, MPFR_RNDN);
  mpfr_set_str_binary (y, "0.1111001011001101001001111100E-1023");
  if (mpfr_cmp (x, y))
    {
      printf ("Error in mpfr_div for prec=28, RNDN\n");
      printf ("Expected "); mpfr_dump (y);
      printf ("Got      "); mpfr_dump (x);
      exit (1);
    }

  mpfr_set_prec (x, 53);
  mpfr_set_str_binary (x, "0.11101100110010100011011000000100001111011111110010101E0");
  mpfr_set_prec (u, 127);
  mpfr_set_str_binary (u, "0.1000001100110110110101110110101101111000110000001111111110000000011111001010110100110010111111111101000001011011101011101101000E-2");
  mpfr_set_prec (y, 95);
  inexact = test_div (y, x, u, MPFR_RNDN);
  if (inexact != (cmp = get_inexact (y, x, u)))
    {
      printf ("Wrong inexact flag (0): expected %d, got %d\n", cmp, inexact);
      printf ("x="); mpfr_out_str (stdout, 10, 99, x, MPFR_RNDN); printf ("\n");
      printf ("u="); mpfr_out_str (stdout, 10, 99, u, MPFR_RNDN); printf ("\n");
      printf ("y="); mpfr_out_str (stdout, 10, 99, y, MPFR_RNDN); printf ("\n");
      exit (1);
    }

  mpfr_set_prec (x, 33);
  mpfr_set_str_binary (x, "0.101111100011011101010011101100001E0");
  mpfr_set_prec (u, 2);
  mpfr_set_str_binary (u, "0.1E0");
  mpfr_set_prec (y, 28);
  inexact = test_div (y, x, u, MPFR_RNDN);
  if (inexact >= 0)
    {
      printf ("Wrong inexact flag (1): expected -1, got %d\n",
              inexact);
      exit (1);
    }

  mpfr_set_prec (x, 129);
  mpfr_set_str_binary (x, "0.111110101111001100000101011100101100110011011101010001000110110101100101000010000001110110100001101010001010100010001111001101010E-2");
  mpfr_set_prec (u, 15);
  mpfr_set_str_binary (u, "0.101101000001100E-1");
  mpfr_set_prec (y, 92);
  inexact = test_div (y, x, u, MPFR_RNDN);
  if (inexact <= 0)
    {
      printf ("Wrong inexact flag for rnd=MPFR_RNDN(1): expected 1, got %d\n",
              inexact);
      mpfr_dump (x);
      mpfr_dump (u);
      mpfr_dump (y);
      exit (1);
    }

  for (px=2; px<MAX_PREC; px++)
    {
      mpfr_set_prec (x, px);
      mpfr_urandomb (x, RANDS);
      for (pu=2; pu<=MAX_PREC; pu++)
        {
          mpfr_set_prec (u, pu);
          do { mpfr_urandomb (u, RANDS); } while (mpfr_cmp_ui (u, 0) == 0);
            {
              py = MPFR_PREC_MIN + (randlimb () % (MAX_PREC - MPFR_PREC_MIN));
              mpfr_set_prec (y, py);
              mpfr_set_prec (z, py + pu);
                {
                  /* inexact is undefined for RNDF */
                  rnd = RND_RAND_NO_RNDF ();
                  inexact = test_div (y, x, u, rnd);
                  if (mpfr_mul (z, y, u, rnd))
                    {
                      printf ("z <- y * u should be exact\n");
                      exit (1);
                    }
                  cmp = mpfr_cmp (z, x);
                  if (((inexact == 0) && (cmp != 0)) ||
                      ((inexact > 0) && (cmp <= 0)) ||
                      ((inexact < 0) && (cmp >= 0)))
                    {
                      printf ("Wrong inexact flag for rnd=%s\n",
                              mpfr_print_rnd_mode(rnd));
                      printf ("expected %d, got %d\n", cmp, inexact);
                      printf ("x="); mpfr_dump (x);
                      printf ("u="); mpfr_dump (u);
                      printf ("y="); mpfr_dump (y);
                      printf ("y*u="); mpfr_dump (z);
                      exit (1);
                    }
                }
            }
        }
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (u);
}

static void
check_special (void)
{
  mpfr_t  a, d, q;
  mpfr_exp_t emax, emin;
  int i;

  mpfr_init2 (a, 100L);
  mpfr_init2 (d, 100L);
  mpfr_init2 (q, 100L);

  /* 1/nan == nan */
  mpfr_set_ui (a, 1L, MPFR_RNDN);
  MPFR_SET_NAN (d);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);

  /* nan/1 == nan */
  MPFR_SET_NAN (a);
  mpfr_set_ui (d, 1L, MPFR_RNDN);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);

  /* +inf/1 == +inf */
  MPFR_SET_INF (a);
  MPFR_SET_POS (a);
  mpfr_set_ui (d, 1L, MPFR_RNDN);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q));
  MPFR_ASSERTN (mpfr_sgn (q) > 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* +inf/-1 == -inf */
  MPFR_SET_INF (a);
  MPFR_SET_POS (a);
  mpfr_set_si (d, -1, MPFR_RNDN);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q));
  MPFR_ASSERTN (mpfr_sgn (q) < 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* -inf/1 == -inf */
  MPFR_SET_INF (a);
  MPFR_SET_NEG (a);
  mpfr_set_ui (d, 1L, MPFR_RNDN);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q));
  MPFR_ASSERTN (mpfr_sgn (q) < 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* -inf/-1 == +inf */
  MPFR_SET_INF (a);
  MPFR_SET_NEG (a);
  mpfr_set_si (d, -1, MPFR_RNDN);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q));
  MPFR_ASSERTN (mpfr_sgn (q) > 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* 1/+inf == +0 */
  mpfr_set_ui (a, 1L, MPFR_RNDN);
  MPFR_SET_INF (d);
  MPFR_SET_POS (d);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (MPFR_IS_ZERO (q));
  MPFR_ASSERTN (MPFR_IS_POS (q));
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* 1/-inf == -0 */
  mpfr_set_ui (a, 1L, MPFR_RNDN);
  MPFR_SET_INF (d);
  MPFR_SET_NEG (d);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (MPFR_IS_ZERO (q));
  MPFR_ASSERTN (MPFR_IS_NEG (q));
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* -1/+inf == -0 */
  mpfr_set_si (a, -1, MPFR_RNDN);
  MPFR_SET_INF (d);
  MPFR_SET_POS (d);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (MPFR_IS_ZERO (q));
  MPFR_ASSERTN (MPFR_IS_NEG (q));
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* -1/-inf == +0 */
  mpfr_set_si (a, -1, MPFR_RNDN);
  MPFR_SET_INF (d);
  MPFR_SET_NEG (d);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (MPFR_IS_ZERO (q));
  MPFR_ASSERTN (MPFR_IS_POS (q));
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* 0/0 == nan */
  mpfr_set_ui (a, 0L, MPFR_RNDN);
  mpfr_set_ui (d, 0L, MPFR_RNDN);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);

  /* +inf/+inf == nan */
  MPFR_SET_INF (a);
  MPFR_SET_POS (a);
  MPFR_SET_INF (d);
  MPFR_SET_POS (d);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_NAN);

  /* 1/+0 = +inf */
  mpfr_set_ui (a, 1, MPFR_RNDZ);
  mpfr_set_ui (d, 0, MPFR_RNDZ);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) > 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);

  /* 1/-0 = -inf */
  mpfr_set_ui (a, 1, MPFR_RNDZ);
  mpfr_set_ui (d, 0, MPFR_RNDZ);
  mpfr_neg (d, d, MPFR_RNDZ);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) < 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);

  /* -1/+0 = -inf */
  mpfr_set_si (a, -1, MPFR_RNDZ);
  mpfr_set_ui (d, 0, MPFR_RNDZ);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) < 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);

  /* -1/-0 = +inf */
  mpfr_set_si (a, -1, MPFR_RNDZ);
  mpfr_set_ui (d, 0, MPFR_RNDZ);
  mpfr_neg (d, d, MPFR_RNDZ);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) > 0);
  MPFR_ASSERTN (__gmpfr_flags == MPFR_FLAGS_DIVBY0);

  /* +inf/+0 = +inf */
  MPFR_SET_INF (a);
  MPFR_SET_POS (a);
  mpfr_set_ui (d, 0, MPFR_RNDZ);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) > 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* +inf/-0 = -inf */
  MPFR_SET_INF (a);
  MPFR_SET_POS (a);
  mpfr_set_ui (d, 0, MPFR_RNDZ);
  mpfr_neg (d, d, MPFR_RNDZ);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) < 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* -inf/+0 = -inf */
  MPFR_SET_INF (a);
  MPFR_SET_NEG (a);
  mpfr_set_ui (d, 0, MPFR_RNDZ);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) < 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* -inf/-0 = +inf */
  MPFR_SET_INF (a);
  MPFR_SET_NEG (a);
  mpfr_set_ui (d, 0, MPFR_RNDZ);
  mpfr_neg (d, d, MPFR_RNDZ);
  mpfr_clear_flags ();
  MPFR_ASSERTN (test_div (q, a, d, MPFR_RNDZ) == 0); /* exact */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) > 0);
  MPFR_ASSERTN (__gmpfr_flags == 0);

  /* check overflow */
  emax = mpfr_get_emax ();
  set_emax (1);
  mpfr_set_ui (a, 1, MPFR_RNDZ);
  mpfr_set_ui (d, 1, MPFR_RNDZ);
  mpfr_div_2ui (d, d, 1, MPFR_RNDZ);
  mpfr_clear_flags ();
  test_div (q, a, d, MPFR_RNDU); /* 1 / 0.5 = 2 -> overflow */
  MPFR_ASSERTN (mpfr_inf_p (q) && mpfr_sgn (q) > 0);
  MPFR_ASSERTN (__gmpfr_flags == (MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT));
  set_emax (emax);

  /* check underflow */
  emin = mpfr_get_emin ();
  set_emin (-1);
  mpfr_set_ui (a, 1, MPFR_RNDZ);
  mpfr_div_2ui (a, a, 2, MPFR_RNDZ);
  mpfr_set_prec (d, mpfr_get_prec (q) + 8);
  for (i = -1; i <= 1; i++)
    {
      int sign;

      /* Test 2^(-2) / (+/- (2 + eps)), with eps < 0, eps = 0, eps > 0.
         -> underflow.
         With div.c r5513, this test fails for eps > 0 in MPFR_RNDN. */
      mpfr_set_ui (d, 2, MPFR_RNDZ);
      if (i < 0)
        mpfr_nextbelow (d);
      if (i > 0)
        mpfr_nextabove (d);
      for (sign = 0; sign <= 1; sign++)
        {
          mpfr_clear_flags ();
          test_div (q, a, d, MPFR_RNDZ); /* result = 0 */
          MPFR_ASSERTN (__gmpfr_flags ==
                        (MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT));
          MPFR_ASSERTN (sign ? MPFR_IS_NEG (q) : MPFR_IS_POS (q));
          MPFR_ASSERTN (MPFR_IS_ZERO (q));
          mpfr_clear_flags ();
          test_div (q, a, d, MPFR_RNDN); /* result = 0 iff eps >= 0 */
          MPFR_ASSERTN (__gmpfr_flags ==
                        (MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT));
          MPFR_ASSERTN (sign ? MPFR_IS_NEG (q) : MPFR_IS_POS (q));
          if (i < 0)
            mpfr_nexttozero (q);
          MPFR_ASSERTN (MPFR_IS_ZERO (q));
          mpfr_neg (d, d, MPFR_RNDN);
        }
    }
  set_emin (emin);

  mpfr_clear (a);
  mpfr_clear (d);
  mpfr_clear (q);
}

static void
consistency (void)
{
  mpfr_t x, y, z1, z2;
  int i;

  mpfr_inits (x, y, z1, z2, (mpfr_ptr) 0);

  for (i = 0; i < 10000; i++)
    {
      mpfr_rnd_t rnd;
      mpfr_prec_t px, py, pz, p;
      int inex1, inex2;

      /* inex is undefined for RNDF */
      rnd = RND_RAND_NO_RNDF ();
      px = (randlimb () % 256) + 2;
      py = (randlimb () % 128) + 2;
      pz = (randlimb () % 256) + 2;
      mpfr_set_prec (x, px);
      mpfr_set_prec (y, py);
      mpfr_set_prec (z1, pz);
      mpfr_set_prec (z2, pz);
      mpfr_urandomb (x, RANDS);
      do
        mpfr_urandomb (y, RANDS);
      while (mpfr_zero_p (y));
      inex1 = mpfr_div (z1, x, y, rnd);
      MPFR_ASSERTN (!MPFR_IS_NAN (z1));
      p = MAX (MAX (px, py), pz);
      if (mpfr_prec_round (x, p, MPFR_RNDN) != 0 ||
          mpfr_prec_round (y, p, MPFR_RNDN) != 0)
        {
          printf ("mpfr_prec_round error for i = %d\n", i);
          exit (1);
        }
      inex2 = mpfr_div (z2, x, y, rnd);
      MPFR_ASSERTN (!MPFR_IS_NAN (z2));
      if (inex1 != inex2 || mpfr_cmp (z1, z2) != 0)
        {
          printf ("Consistency error for i = %d, rnd = %s\n", i,
                  mpfr_print_rnd_mode (rnd));
          printf ("inex1=%d inex2=%d\n", inex1, inex2);
          printf ("z1="); mpfr_dump (z1);
          printf ("z2="); mpfr_dump (z2);
          exit (1);
        }
    }

  mpfr_clears (x, y, z1, z2, (mpfr_ptr) 0);
}

/* Reported by Carl Witty on 2007-06-03 */
static void
test_20070603 (void)
{
  mpfr_t n, d, q, c;

  mpfr_init2 (n, 128);
  mpfr_init2 (d, 128);
  mpfr_init2 (q, 31);
  mpfr_init2 (c, 31);

  mpfr_set_str (n, "10384593717069655257060992206846485", 10, MPFR_RNDN);
  mpfr_set_str (d, "10384593717069655257060992206847132", 10, MPFR_RNDN);
  mpfr_div (q, n, d, MPFR_RNDU);

  mpfr_set_ui (c, 1, MPFR_RNDN);
  if (mpfr_cmp (q, c) != 0)
    {
      printf ("Error in test_20070603\nGot        ");
      mpfr_dump (q);
      printf ("instead of ");
      mpfr_dump (c);
      exit (1);
    }

  /* same for 64-bit machines */
  mpfr_set_prec (n, 256);
  mpfr_set_prec (d, 256);
  mpfr_set_prec (q, 63);
  mpfr_set_str (n, "822752278660603021077484591278675252491367930877209729029898240", 10, MPFR_RNDN);
  mpfr_set_str (d, "822752278660603021077484591278675252491367930877212507873738752", 10, MPFR_RNDN);
  mpfr_div (q, n, d, MPFR_RNDU);
  if (mpfr_cmp (q, c) != 0)
    {
      printf ("Error in test_20070603\nGot        ");
      mpfr_dump (q);
      printf ("instead of ");
      mpfr_dump (c);
      exit (1);
    }

  mpfr_clear (n);
  mpfr_clear (d);
  mpfr_clear (q);
  mpfr_clear (c);
}

/* Bug found while adding tests for mpfr_cot */
static void
test_20070628 (void)
{
  mpfr_exp_t old_emax;
  mpfr_t x, y;
  int inex, err = 0;

  old_emax = mpfr_get_emax ();
  set_emax (256);

  mpfr_inits2 (53, x, y, (mpfr_ptr) 0);
  mpfr_set_si (x, -1, MPFR_RNDN);
  mpfr_set_si_2exp (y, 1, -256, MPFR_RNDN);
  mpfr_clear_flags ();
  inex = mpfr_div (x, x, y, MPFR_RNDD);
  if (MPFR_IS_POS (x) || ! mpfr_inf_p (x))
    {
      printf ("Error in test_20070628: expected -Inf, got\n");
      mpfr_dump (x);
      err++;
    }
  if (inex >= 0)
    {
      printf ("Error in test_20070628: expected inex < 0, got %d\n", inex);
      err++;
    }
  if (! mpfr_overflow_p ())
    {
      printf ("Error in test_20070628: overflow flag is not set\n");
      err++;
    }
  mpfr_clears (x, y, (mpfr_ptr) 0);
  set_emax (old_emax);
}

/* Bug in mpfr_divhigh_n_basecase when all limbs of q (except the most
   significant one) are B-1 where B=2^GMP_NUMB_BITS. Since we truncate
   the divisor at each step, it might happen at some point that
   (np[n-1],np[n-2]) > (d1,d0), and not only the equality.
   Reported by Ricky Farr
   <https://sympa.inria.fr/sympa/arc/mpfr/2015-10/msg00023.html>
   To get a failure, a MPFR_DIVHIGH_TAB entry below the MPFR_DIV_THRESHOLD
   limit must have a value 0. With most mparam.h files, this cannot occur. To
   make the bug appear, one can configure MPFR with -DMPFR_TUNE_COVERAGE. */
static void
test_20151023 (void)
{
  mpfr_prec_t p;
  mpfr_t n, d, q, q0;
  int inex, i;

  for (p = GMP_NUMB_BITS; p <= 2000; p++)
    {
      mpfr_init2 (n, 2*p);
      mpfr_init2 (d, p);
      mpfr_init2 (q, p);
      mpfr_init2 (q0, GMP_NUMB_BITS);

      /* generate a random divisor of p bits */
      do
        mpfr_urandomb (d, RANDS);
      while (mpfr_zero_p (d));
      /* generate a random non-zero quotient of GMP_NUMB_BITS bits */
      do
        mpfr_urandomb (q0, RANDS);
      while (mpfr_zero_p (q0));
      /* zero-pad the quotient to p bits */
      inex = mpfr_prec_round (q0, p, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);

      for (i = 0; i < 3; i++)
        {
          /* i=0: try with the original quotient xxx000...000
             i=1: try with the original quotient minus one ulp
             i=2: try with the original quotient plus one ulp */
          if (i == 1)
            mpfr_nextbelow (q0);
          else if (i == 2)
            {
              mpfr_nextabove (q0);
              mpfr_nextabove (q0);
            }

          inex = mpfr_mul (n, d, q0, MPFR_RNDN);
          MPFR_ASSERTN(inex == 0);
          mpfr_nextabove (n);
          mpfr_div (q, n, d, MPFR_RNDN);
          if (! mpfr_equal_p (q, q0))
            {
              printf ("Error in test_20151023 for p=%ld, rnd=RNDN, i=%d\n",
                      (long) p, i);
              printf ("n="); mpfr_dump (n);
              printf ("d="); mpfr_dump (d);
              printf ("expected q0="); mpfr_dump (q0);
              printf ("got       q="); mpfr_dump (q);
              exit (1);
            }

          inex = mpfr_mul (n, d, q0, MPFR_RNDN);
          MPFR_ASSERTN(inex == 0);
          mpfr_nextbelow (n);
          mpfr_div (q, n, d, MPFR_RNDN);
          MPFR_ASSERTN(mpfr_cmp (q, q0) == 0);
        }

      mpfr_clear (n);
      mpfr_clear (d);
      mpfr_clear (q);
      mpfr_clear (q0);
    }
}

/* test a random division of p+extra bits divided by p+extra bits,
   with quotient of p bits only, where the p+extra bit approximation
   of the quotient is very near a rounding frontier. */
static void
test_bad_aux (mpfr_prec_t p, mpfr_prec_t extra)
{
  mpfr_t u, v, w, q0, q;

  mpfr_init2 (u, p + extra);
  mpfr_init2 (v, p + extra);
  mpfr_init2 (w, p + extra);
  mpfr_init2 (q0, p);
  mpfr_init2 (q, p);
  do mpfr_urandomb (q0, RANDS); while (mpfr_zero_p (q0));
  do mpfr_urandomb (v, RANDS); while (mpfr_zero_p (v));

  mpfr_set (w, q0, MPFR_RNDN); /* exact */
  mpfr_nextabove (w); /* now w > q0 */
  mpfr_mul (u, v, w, MPFR_RNDU); /* thus u > v*q0 */
  mpfr_div (q, u, v, MPFR_RNDU); /* should have q > q0 */
  MPFR_ASSERTN (mpfr_cmp (q, q0) > 0);
  mpfr_div (q, u, v, MPFR_RNDZ); /* should have q = q0 */
  MPFR_ASSERTN (mpfr_cmp (q, q0) == 0);

  mpfr_set (w, q0, MPFR_RNDN); /* exact */
  mpfr_nextbelow (w); /* now w < q0 */
  mpfr_mul (u, v, w, MPFR_RNDZ); /* thus u < v*q0 */
  mpfr_div (q, u, v, MPFR_RNDZ); /* should have q < q0 */
  MPFR_ASSERTN (mpfr_cmp (q, q0) < 0);
  mpfr_div (q, u, v, MPFR_RNDU); /* should have q = q0 */
  MPFR_ASSERTN (mpfr_cmp (q, q0) == 0);

  mpfr_clear (u);
  mpfr_clear (v);
  mpfr_clear (w);
  mpfr_clear (q0);
  mpfr_clear (q);
}

static void
test_bad (void)
{
  mpfr_prec_t p, extra;

  for (p = MPFR_PREC_MIN; p <= 1024; p += 17)
    for (extra = 2; extra <= 64; extra++)
      test_bad_aux (p, extra);
}

#define TEST_FUNCTION test_div
#define TWO_ARGS
#define RAND_FUNCTION(x) mpfr_random2(x, MPFR_LIMB_SIZE (x), randlimb () % 100, RANDS)
#include "tgeneric.c"

static void
test_extreme (void)
{
  mpfr_t x, y, z;
  mpfr_exp_t emin, emax;
  mpfr_prec_t p[4] = { 8, 32, 64, 256 };
  int xi, yi, zi, j, r;
  unsigned int flags, ex_flags;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  set_emin (MPFR_EMIN_MIN);
  set_emax (MPFR_EMAX_MAX);

  for (xi = 0; xi < 4; xi++)
    {
      mpfr_init2 (x, p[xi]);
      mpfr_setmax (x, MPFR_EMAX_MAX);
      MPFR_ASSERTN (mpfr_check (x));
      for (yi = 0; yi < 4; yi++)
        {
          mpfr_init2 (y, p[yi]);
          mpfr_setmin (y, MPFR_EMIN_MIN);
          for (j = 0; j < 2; j++)
            {
              MPFR_ASSERTN (mpfr_check (y));
              for (zi = 0; zi < 4; zi++)
                {
                  mpfr_init2 (z, p[zi]);
                  RND_LOOP (r)
                    {
                      mpfr_clear_flags ();
                      mpfr_div (z, x, y, (mpfr_rnd_t) r);
                      flags = __gmpfr_flags;
                      MPFR_ASSERTN (mpfr_check (z));
                      ex_flags = MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT;
                      if (flags != ex_flags)
                        {
                          printf ("Bad flags in test_extreme on z = a/b"
                                  " with %s and\n",
                                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                          printf ("a = ");
                          mpfr_dump (x);
                          printf ("b = ");
                          mpfr_dump (y);
                          printf ("Expected flags:");
                          flags_out (ex_flags);
                          printf ("Got flags:     ");
                          flags_out (flags);
                          printf ("z = ");
                          mpfr_dump (z);
                          exit (1);
                        }
                      mpfr_clear_flags ();
                      mpfr_div (z, y, x, (mpfr_rnd_t) r);
                      flags = __gmpfr_flags;
                      MPFR_ASSERTN (mpfr_check (z));
                      ex_flags = MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT;
                      if (flags != ex_flags)
                        {
                          printf ("Bad flags in test_extreme on z = a/b"
                                  " with %s and\n",
                                  mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                          printf ("a = ");
                          mpfr_dump (y);
                          printf ("b = ");
                          mpfr_dump (x);
                          printf ("Expected flags:");
                          flags_out (ex_flags);
                          printf ("Got flags:     ");
                          flags_out (flags);
                          printf ("z = ");
                          mpfr_dump (z);
                          exit (1);
                        }
                    }
                  mpfr_clear (z);
                }  /* zi */
              mpfr_nextabove (y);
            }  /* j */
          mpfr_clear (y);
        }  /* yi */
      mpfr_clear (x);
    }  /* xi */

  set_emin (emin);
  set_emax (emax);
}

static void
testall_rndf (mpfr_prec_t pmax)
{
  mpfr_t a, b, c, d;
  mpfr_prec_t pa, pb, pc;

  for (pa = MPFR_PREC_MIN; pa <= pmax; pa++)
    {
      mpfr_init2 (a, pa);
      mpfr_init2 (d, pa);
      for (pb = MPFR_PREC_MIN; pb <= pmax; pb++)
        {
          mpfr_init2 (b, pb);
          mpfr_set_ui (b, 1, MPFR_RNDN);
          while (mpfr_cmp_ui (b, 2) < 0)
            {
              for (pc = MPFR_PREC_MIN; pc <= pmax; pc++)
                {
                  mpfr_init2 (c, pc);
                  mpfr_set_ui (c, 1, MPFR_RNDN);
                  while (mpfr_cmp_ui (c, 2) < 0)
                    {
                      mpfr_div (a, b, c, MPFR_RNDF);
                      mpfr_div (d, b, c, MPFR_RNDD);
                      if (!mpfr_equal_p (a, d))
                        {
                          mpfr_div (d, b, c, MPFR_RNDU);
                          if (!mpfr_equal_p (a, d))
                            {
                              printf ("Error: mpfr_div(a,b,c,RNDF) does not "
                                      "match RNDD/RNDU\n");
                              printf ("b="); mpfr_dump (b);
                              printf ("c="); mpfr_dump (c);
                              printf ("a="); mpfr_dump (a);
                              exit (1);
                            }
                        }
                      mpfr_nextabove (c);
                    }
                  mpfr_clear (c);
                }
              mpfr_nextabove (b);
            }
          mpfr_clear (b);
        }
      mpfr_clear (a);
      mpfr_clear (d);
    }
}

static void
test_mpfr_divsp2 (void)
{
  mpfr_t u, v, q;

  /* test to exercise r2 = v1 in mpfr_divsp2 */
  mpfr_init2 (u, 128);
  mpfr_init2 (v, 128);
  mpfr_init2 (q, 83);

  mpfr_set_str (u, "286677858044426991425771529092412636160", 10, MPFR_RNDN);
  mpfr_set_str (v, "241810647971575979588130185988987264768", 10, MPFR_RNDN);
  mpfr_div (q, u, v, MPFR_RNDN);
  mpfr_set_str (u, "5732952910203749289426944", 10, MPFR_RNDN);
  mpfr_div_2ui (u, u, 82, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (q, u));

  mpfr_clear (u);
  mpfr_clear (v);
  mpfr_clear (q);
}

/* Assertion failure in r10769 with --enable-assert --enable-gmp-internals
   (same failure in tatan on a similar example). */
static void
test_20160831 (void)
{
  mpfr_t u, v, q;

  mpfr_inits2 (124, u, v, q, (mpfr_ptr) 0);

  mpfr_set_ui (u, 1, MPFR_RNDN);
  mpfr_set_str (v, "0x40000000000000005", 16, MPFR_RNDN);
  mpfr_div (q, u, v, MPFR_RNDN);
  mpfr_set_str (u, "0xfffffffffffffffecp-134", 16, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_equal_p (q, u));

  mpfr_set_prec (u, 128);
  mpfr_set_prec (v, 128);
  mpfr_set_str (u, "186127091671619245460026015088243485690", 10, MPFR_RNDN);
  mpfr_set_str (v, "205987256581218236405412302590110119580", 10, MPFR_RNDN);
  mpfr_div (q, u, v, MPFR_RNDN);
  mpfr_set_str (u, "19217137613667309953639458782352244736", 10, MPFR_RNDN);
  mpfr_div_2ui (u, u, 124, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_equal_p (q, u));

  mpfr_clears (u, v, q, (mpfr_ptr) 0);
}

/* With r11138, on a 64-bit machine:
   div.c:128: MPFR assertion failed: qx >= __gmpfr_emin
*/
static void
test_20170104 (void)
{
  mpfr_t u, v, q;
  mpfr_exp_t emin;

  emin = mpfr_get_emin ();
  set_emin (-42);

  mpfr_init2 (u, 12);
  mpfr_init2 (v, 12);
  mpfr_init2 (q, 11);
  mpfr_set_str_binary (u, "0.111111111110E-29");
  mpfr_set_str_binary (v, "0.111111111111E14");
  mpfr_div (q, u, v, MPFR_RNDN);
  mpfr_clears (u, v, q, (mpfr_ptr) 0);

  set_emin (emin);
}

/* With r11140, on a 64-bit machine with GMP_CHECK_RANDOMIZE=1484406128:
   Consistency error for i = 2577
*/
static void
test_20170105 (void)
{
  mpfr_t x, y, z, t;

  mpfr_init2 (x, 138);
  mpfr_init2 (y, 6);
  mpfr_init2 (z, 128);
  mpfr_init2 (t, 128);
  mpfr_set_str_binary (x, "0.100110111001001000101111010010011101111110111111110001110100000001110111010100111010100011101010110000010100000011100100110101101011000000E-6");
  mpfr_set_str_binary (y, "0.100100E-2");
  /* up to exponents, x/y is exactly 367625553447399614694201910705139062483,
     which has 129 bits, thus we are in the round-to-nearest-even case, and since
     the penultimate bit of x/y is 1, we should round upwards */
  mpfr_set_str_binary (t, "0.10001010010010010000110110010110111111111100011011101010000000000110101000010001011110011011010000111010000000001100101101101010E-3");
  mpfr_div (z, x, y, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_equal_p (z, t));
  mpfr_clears (x, y, z, t, (mpfr_ptr) 0);
}

/* The real cause of the mpfr_ttanh failure from the non-regression test
   added in tests/ttanh.c@11993 was actually due to a bug in mpfr_div, as
   this can be seen by comparing the logs of the 3.1 branch and the trunk
   r11992 with MPFR_LOG_ALL=1 MPFR_LOG_PREC=50 on this particular test
   (this was noticed because adding this test to the 3.1 branch did not
   yield a failure like in the trunk, though the mpfr_ttanh code did not
   change until r11993). This was the bug actually fixed in r12002.
*/
static void
test_20171219 (void)
{
  mpfr_t x, y, z, t;

  mpfr_inits2 (126, x, y, z, t, (mpfr_ptr) 0);
  mpfr_set_str_binary (x, "0.111000000000000111100000000000011110000000000001111000000000000111100000000000011110000000000001111000000000000111100000000000E1");
  /* x = 36347266450315671364380109803814927 / 2^114 */
  mpfr_add_ui (y, x, 2, MPFR_RNDN);
  /* y = 77885641318594292392624080437575695 / 2^114 */
  mpfr_div (z, x, y, MPFR_RNDN);
  mpfr_set_ui_2exp (t, 3823, -13, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_equal_p (z, t));
  mpfr_clears (x, y, z, t, (mpfr_ptr) 0);
}

#if !defined(MPFR_GENERIC_ABI) && GMP_NUMB_BITS == 64
/* exercise mpfr_div2_approx */
static void
test_mpfr_div2_approx (unsigned long n)
{
  mpfr_t x, y, z;

  mpfr_init2 (x, 113);
  mpfr_init2 (y, 113);
  mpfr_init2 (z, 113);
  while (n--)
    {
      mpfr_urandomb (x, RANDS);
      mpfr_urandomb (y, RANDS);
      mpfr_div (z, x, y, MPFR_RNDN);
    }
  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
}
#endif

/* bug found in ttan with GMP_CHECK_RANDOMIZE=1514257254 */
static void
bug20171218 (void)
{
  mpfr_t s, c;
  mpfr_init2 (s, 124);
  mpfr_init2 (c, 124);
  mpfr_set_str_binary (s, "-0.1110000111100001111000011110000111100001111000011110000111100001111000011110000111100001111000011110000111100001111000011110E0");
  mpfr_set_str_binary (c, "0.1111000011110000111100001111000011110000111100001111000011110000111100001111000011110000111100001111000011110000111100001111E-1");
  mpfr_div (c, s, c, MPFR_RNDN);
  mpfr_set_str_binary (s, "-1.111000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
  MPFR_ASSERTN(mpfr_equal_p (c, s));
  mpfr_clear (s);
  mpfr_clear (c);
}

/* Extended test based on a bug found with flint-arb test suite with a
   32-bit ABI: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=888459
   Division of the form: (1 - 2^(-pa)) / (1 - 2^(-pb)).
   The result is compared to the one obtained by increasing the precision of
   the divisor (without changing its value, so that both results should be
   equal). For all of these tests, a failure may occur in r12126 only with
   pb=GMP_NUMB_BITS and MPFR_RNDN (and some particular values of pa and pc).
   This bug was introduced by r9086, where mpfr_div uses mpfr_div_ui when
   the divisor has only one limb.
*/
static void
bug20180126 (void)
{
  mpfr_t a, b1, b2, c1, c2;
  int pa, i, j, pc, sa, sb, r, inex1, inex2;

  for (pa = 100; pa < 800; pa += 11)
    for (i = 1; i <= 4; i++)
      for (j = -2; j <= 2; j++)
        {
          int pb = GMP_NUMB_BITS * i + j;

          if (pb > pa)
            continue;

          mpfr_inits2 (pa, a, b1, (mpfr_ptr) 0);
          mpfr_inits2 (pb, b2, (mpfr_ptr) 0);

          mpfr_set_ui (a, 1, MPFR_RNDN);
          mpfr_nextbelow (a);                   /* 1 - 2^(-pa) */
          mpfr_set_ui (b2, 1, MPFR_RNDN);
          mpfr_nextbelow (b2);                  /* 1 - 2^(-pb) */
          inex1 = mpfr_set (b1, b2, MPFR_RNDN);
          MPFR_ASSERTN (inex1 == 0);

          for (pc = 32; pc <= 320; pc += 32)
            {
              mpfr_inits2 (pc, c1, c2, (mpfr_ptr) 0);

              for (sa = 0; sa < 2; sa++)
                {
                  for (sb = 0; sb < 2; sb++)
                    {
                      RND_LOOP_NO_RNDF (r)
                        {
                          MPFR_ASSERTN (mpfr_equal_p (b1, b2));
                          inex1 = mpfr_div (c1, a, b1, (mpfr_rnd_t) r);
                          inex2 = mpfr_div (c2, a, b2, (mpfr_rnd_t) r);

                          if (! mpfr_equal_p (c1, c2) ||
                              ! SAME_SIGN (inex1, inex2))
                            {
                              printf ("Error in bug20180126 for "
                                      "pa=%d pb=%d pc=%d sa=%d sb=%d %s\n",
                                      pa, pb, pc, sa, sb,
                                      mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                              printf ("inex1 = %d, c1 = ", inex1);
                              mpfr_dump (c1);
                              printf ("inex2 = %d, c2 = ", inex2);
                              mpfr_dump (c2);
                              exit (1);
                            }
                        }

                      mpfr_neg (b1, b1, MPFR_RNDN);
                      mpfr_neg (b2, b2, MPFR_RNDN);
                    }  /* sb */

                  mpfr_neg (a, a, MPFR_RNDN);
                }  /* sa */

              mpfr_clears (c1, c2, (mpfr_ptr) 0);
            }  /* pc */

          mpfr_clears (a, b1, b2, (mpfr_ptr) 0);
        }  /* j */
}

static void
coverage (mpfr_prec_t pmax)
{
  mpfr_prec_t p;

  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    {
      int inex;
      mpfr_t q, u, v;

      mpfr_init2 (q, p);
      mpfr_init2 (u, p);
      mpfr_init2 (v, p);

      /* exercise case qx < emin */
      mpfr_set_ui_2exp (u, 1, mpfr_get_emin (), MPFR_RNDN);
      mpfr_set_ui (v, 4, MPFR_RNDN);

      mpfr_clear_flags ();
      /* u/v = 2^(emin-2), should be rounded to +0 for RNDN */
      inex = mpfr_div (q, u, v, MPFR_RNDN);
      MPFR_ASSERTN(inex < 0);
      MPFR_ASSERTN(mpfr_zero_p (q) && mpfr_signbit (q) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

      mpfr_clear_flags ();
      /* u/v = 2^(emin-2), should be rounded to 2^(emin-1) for RNDU */
      inex = mpfr_div (q, u, v, MPFR_RNDU);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui_2exp (q, 1, mpfr_get_emin () - 1) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

      mpfr_clear_flags ();
      /* u/v = 2^(emin-2), should be rounded to +0 for RNDZ */
      inex = mpfr_div (q, u, v, MPFR_RNDZ);
      MPFR_ASSERTN(inex < 0);
      MPFR_ASSERTN(mpfr_zero_p (q) && mpfr_signbit (q) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

      if (p == 1)
        goto end_of_loop;

      mpfr_set_ui_2exp (u, 1, mpfr_get_emin (), MPFR_RNDN);
      mpfr_nextbelow (u); /* u = (1-2^(-p))*2^emin */
      mpfr_set_ui (v, 2, MPFR_RNDN);

      mpfr_clear_flags ();
      /* u/v = (1-2^(-p))*2^(emin-1), will round to 2^(emin-1) for RNDN */
      inex = mpfr_div (q, u, v, MPFR_RNDN);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui_2exp (q, 1, mpfr_get_emin () - 1) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

      mpfr_clear_flags ();
      /* u/v should round to 2^(emin-1) for RNDU */
      inex = mpfr_div (q, u, v, MPFR_RNDU);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui_2exp (q, 1, mpfr_get_emin () - 1) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

      mpfr_clear_flags ();
      /* u/v should round to +0 for RNDZ */
      inex = mpfr_div (q, u, v, MPFR_RNDZ);
      MPFR_ASSERTN(inex < 0);
      MPFR_ASSERTN(mpfr_zero_p (q) && mpfr_signbit (q) == 0);
      MPFR_ASSERTN(mpfr_underflow_p ());

    end_of_loop:
      mpfr_clear (q);
      mpfr_clear (u);
      mpfr_clear (v);
    }
}

/* coverage for case usize >= n + n in Mulders' algorithm */
static void
coverage2 (void)
{
  mpfr_prec_t p;
  mpfr_t q, u, v, t, w;
  int inex, inex2;

  p = MPFR_DIV_THRESHOLD * GMP_NUMB_BITS;
  mpfr_init2 (q, p);
  mpfr_init2 (u, 2 * p + 3 * GMP_NUMB_BITS);
  mpfr_init2 (v, p);
  do mpfr_urandomb (u, RANDS); while (mpfr_zero_p (u));
  do mpfr_urandomb (v, RANDS); while (mpfr_zero_p (v));
  inex = mpfr_div (q, u, v, MPFR_RNDN);
  mpfr_init2 (t, mpfr_get_prec (u));
  mpfr_init2 (w, mpfr_get_prec (u));
  inex2 = mpfr_mul (t, q, v, MPFR_RNDN);
  MPFR_ASSERTN(inex2 == 0);
  if (inex == 0) /* check q*v = u */
    MPFR_ASSERTN(mpfr_equal_p (u, t));
  else
    {
      if (inex > 0)
        mpfr_nextbelow (q);
      else
        mpfr_nextabove (q);
      inex2 = mpfr_mul (w, q, v, MPFR_RNDN);
      MPFR_ASSERTN(inex2 == 0);
      inex2 = mpfr_sub (t, t, u, MPFR_RNDN);
      MPFR_ASSERTN(inex2 == 0);
      inex2 = mpfr_sub (w, w, u, MPFR_RNDN);
      MPFR_ASSERTN(inex2 == 0);
      MPFR_ASSERTN(mpfr_cmpabs (t, w) <= 0);
      if (mpfr_cmpabs (t, w) == 0) /* even rule: significand of q should now
                                      be odd */
        MPFR_ASSERTN(mpfr_min_prec (q) == mpfr_get_prec (q));
    }

  mpfr_clear (q);
  mpfr_clear (u);
  mpfr_clear (v);
  mpfr_clear (t);
  mpfr_clear (w);
}

int
main (int argc, char *argv[])
{
  tests_start_mpfr ();

  coverage (1024);
  coverage2 ();
  bug20180126 ();
  bug20171218 ();
  testall_rndf (9);
  test_20170105 ();
  check_inexact ();
  check_hard ();
  check_special ();
  check_lowr ();
  check_float (); /* checks single precision */
  check_double ();
  check_convergence ();
  check_64 ();

  check4("4.0","4.503599627370496e15", MPFR_RNDZ, 62,
   "0.10000000000000000000000000000000000000000000000000000000000000E-49");
  check4("1.0","2.10263340267725788209e+187", MPFR_RNDU, 65,
   "0.11010011111001101011111001100111110100000001101001111100111000000E-622");
  check4("2.44394909079968374564e-150", "2.10263340267725788209e+187",MPFR_RNDU,
         65,
  "0.11010011111001101011111001100111110100000001101001111100111000000E-1119");

  consistency ();
  test_20070603 ();
  test_20070628 ();
  test_20151023 ();
  test_20160831 ();
  test_20170104 ();
  test_20171219 ();
  test_generic (MPFR_PREC_MIN, 800, 50);
  test_bad ();
  test_extreme ();
  test_mpfr_divsp2 ();
#if !defined(MPFR_GENERIC_ABI) && GMP_NUMB_BITS == 64
  test_mpfr_div2_approx (1000000);
#endif

  tests_end_mpfr ();
  return 0;
}
