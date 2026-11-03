/* Test file for mpfr_add1sp.

Copyright 2004-2025 Free Software Foundation, Inc.
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

static void check_special (void);
static void check_random (mpfr_prec_t p);

static int
mpfr_add_cf (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t r)
{
  mpfr_clear_flags ();  /* allows better checking */
  return mpfr_add (a, b, c, r);
}

static void
check_overflow (void)
{
  mpfr_t x, y, z1, z2;
  mpfr_exp_t emin, emax;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  set_emin (-1021);
  set_emax (1024);

  mpfr_inits (x, y, z1, z2, (mpfr_ptr) 0);

  mpfr_set_str1 (x, "8.00468257869324898448e+307");
  mpfr_set_str1 (y, "7.44784712422708645156e+307");
  mpfr_add1sp (z1, x, y, MPFR_RNDN);
  mpfr_add1   (z2, x, y, MPFR_RNDN);
  if (mpfr_cmp (z1, z2))
    {
      printf ("Overflow bug in add1sp.\n");
      exit (1);
    }
  mpfr_clears (x, y, z1, z2, (mpfr_ptr) 0);

  set_emin (emin);
  set_emax (emax);
}

static void
bug20171217 (void)
{
  mpfr_t a, b, c;

  mpfr_init2 (a, 137);
  mpfr_init2 (b, 137);
  mpfr_init2 (c, 137);
  mpfr_set_str_binary (b, "0.11111111111111111111111111111111111111111111111111111111111111111111000000000000000000000000000000000000000000000000000000000000000000000E-66");
  mpfr_set_str_binary (c, "0.11111111111111111111111111111111111111111111111111111111111111111000000000000000000000000000000000000000000000000000000000000000000110000E-2");
  mpfr_add_cf (a, b, c, MPFR_RNDN);
  mpfr_set_str_binary (b, "0.10000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000000001000E-1");
  MPFR_ASSERTN(mpfr_equal_p (a, b));
  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
}

static void
bug20190903 (void)
{
  mpfr_t a, b, c, d;
  int inex;
  mpfr_flags_t flags;

  /* Bug in r13574, fixed in r13578.
     Note: to reproduce the failure, GMP_NUMB_BITS == 64 is assumed. */
  mpfr_inits2 (128, a, b, c, d, (mpfr_ptr) 0);
  mpfr_set_str_binary (b, "0.11111111111111111100000000000000000001001111111101111111110000101001111100111110110010011001111110000000101001001001110110101110E0");
  mpfr_set_str_binary (c, "0.10000001011101000010000111100111011110100000001001000010011011001000110100111111101100001101001101011101100100011000000101110111E-126");
  mpfr_add_cf (a, b, c, MPFR_RNDN);
  mpfr_set_str_binary (b, "0.11111111111111111100000000000000000001001111111101111111110000101001111100111110110010011001111110000000101001001001110110110000E0");
  MPFR_ASSERTN (mpfr_equal_p (a, b));
  mpfr_clears (a, b, c, d, (mpfr_ptr) 0);

  /* Bug in r13574, fixed in r13586. */
  /* Figure with GMP_NUMB_BITS = 4:
       b = 1111 1000
       c =      1000 0001
  */
  mpfr_inits2 (2 * GMP_NUMB_BITS, a, b, c, d, (mpfr_ptr) 0);
  mpfr_set_ui_2exp (d, 1, 3 * GMP_NUMB_BITS, MPFR_RNDN);
  mpfr_set_ui_2exp (c, 1, 2 * GMP_NUMB_BITS - 1, MPFR_RNDN);
  mpfr_sub (b, d, c, MPFR_RNDN);
  mpfr_add_ui (c, c, 1, MPFR_RNDN);
  inex = mpfr_add_cf (a, b, c, MPFR_RNDN);
  flags = __gmpfr_flags;
  MPFR_ASSERTN (mpfr_equal_p (a, d));
  MPFR_ASSERTN (inex < 0);
  MPFR_ASSERTN (flags == MPFR_FLAGS_INEXACT);
  inex = mpfr_add_cf (a, b, c, MPFR_RNDU);
  flags = __gmpfr_flags;
  mpfr_add_ui (d, d, 1, MPFR_RNDU);
  MPFR_ASSERTN (mpfr_equal_p (a, d));
  MPFR_ASSERTN (inex > 0);
  MPFR_ASSERTN (flags == MPFR_FLAGS_INEXACT);
  mpfr_clears (a, b, c, d, (mpfr_ptr) 0);
}

/* Check corner case b = 1, c = 2^(-p) for MPFR_PREC_MIN <= p <= pmax.
   With RNDN, result is 1, except for p=1, where it is 2. */
static void
test_corner_1 (mpfr_prec_t pmax)
{
  mpfr_prec_t p;

  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    {
      mpfr_t a, b, c;
      int inex;
      mpfr_init2 (a, p);
      mpfr_init2 (b, p);
      mpfr_init2 (c, p);
      mpfr_set_ui (b, 1, MPFR_RNDN);
      mpfr_set_ui_2exp (c, 1, -p, MPFR_RNDN);
      inex = mpfr_add_cf (a, b, c, MPFR_RNDN);
      if (p == 1) /* special case, since 2^(p-1) is odd */
        {
          MPFR_ASSERTN(inex > 0);
          MPFR_ASSERTN(mpfr_cmp_ui (a, 2) == 0);
        }
      else
        {
          MPFR_ASSERTN(inex < 0);
          MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);
        }
      mpfr_clear (a);
      mpfr_clear (b);
      mpfr_clear (c);
    }
}

static void
coverage (void)
{
  mpfr_t a, b, c;
  int inex;
  mpfr_exp_t emax;
  mpfr_prec_t p;

  mpfr_init (a);
  mpfr_init (b);
  mpfr_init (c);

  /* coverage test in mpfr_add1sp: case round away, where add_one_ulp
     gives a carry, and the new exponent is below emax */
  for (p = MPFR_PREC_MIN; p <= 3 * GMP_NUMB_BITS; p++)
    {
      mpfr_set_prec (a, p);
      mpfr_set_prec (b, p);
      mpfr_set_prec (c, p);
      mpfr_set_ui (b, 1, MPFR_RNDN);
      mpfr_nextbelow (b); /* b = 1 - 2^(-p) (including for p=1) */
      mpfr_set_ui_2exp (c, 1, -p-1, MPFR_RNDN);
      /* c = 2^(-p-1) thus b+c = 1 - 2^(-p-1) should be rounded to 1 */
      inex = mpfr_add_cf (a, b, c, MPFR_RNDU);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);
    }

  /* coverage test in mpfr_add1sp2: case GMP_NUMB_BITS <= d < 2*GMP_NUMB_BITS
     and a1 = 0 */
  mpfr_set_prec (a, GMP_NUMB_BITS + 2);
  mpfr_set_prec (b, GMP_NUMB_BITS + 2);
  mpfr_set_prec (c, GMP_NUMB_BITS + 2);
  mpfr_set_ui (b, 1, MPFR_RNDN);
  mpfr_nextbelow (b); /* b = 1 - 2^(-p) with p = GMP_NUMB_BITS+2 */
  mpfr_set_ui_2exp (c, 1, -GMP_NUMB_BITS-1, MPFR_RNDN);
  mpfr_nextbelow (c); /* c = 2^(1-p) - 2^(1-2p) */
  /* a = 1 + 2^(-p) - 2^(1-2p) should be rounded to 1 with RNDN */
  inex = mpfr_add_cf (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(inex < 0);
  MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);

  /* coverage test in mpfr_add1sp2: case round away, where add_one_ulp
     gives a carry, and the new exponent is below emax */
  mpfr_set_prec (a, GMP_NUMB_BITS + 1);
  mpfr_set_prec (b, GMP_NUMB_BITS + 1);
  mpfr_set_prec (c, GMP_NUMB_BITS + 1);
  mpfr_set_ui (b, 1, MPFR_RNDN);
  mpfr_nextbelow (b); /* b = 1 - 2^(-p) */
  mpfr_set_ui_2exp (c, 1, -GMP_NUMB_BITS-2, MPFR_RNDN);
  /* c = 2^(-p-1) */
  inex = mpfr_add_cf (a, b, c, MPFR_RNDU);
  MPFR_ASSERTN(inex > 0);
  MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);

  /* coverage test in mpfr_add1sp3: case GMP_NUMB_BITS <= d < 2*GMP_NUMB_BITS
     and a2 == 0 */
  mpfr_set_prec (a, 2 * GMP_NUMB_BITS + 2);
  mpfr_set_prec (b, 2 * GMP_NUMB_BITS + 2);
  mpfr_set_prec (c, 2 * GMP_NUMB_BITS + 2);
  mpfr_set_ui (b, 1, MPFR_RNDN);
  mpfr_nextbelow (b); /* b = 1 - 2^(-p) with p = 2*GMP_NUMB_BITS+2 */
  mpfr_set_ui_2exp (c, 1, -2*GMP_NUMB_BITS-1, MPFR_RNDN);
  mpfr_nextbelow (c); /* c = 2^(1-p) - 2^(1-2p) */
  /* a = 1 + 2^(-p) - 2^(1-2p) should be rounded to 1 with RNDN */
  inex = mpfr_add_cf (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(inex < 0);
  MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);

  /* coverage test in mpfr_add1sp3: case bx > emax */
  emax = mpfr_get_emax ();
  set_emax (1);
  mpfr_set_prec (a, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_prec (b, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_prec (c, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_ui_2exp (b, 1, mpfr_get_emax () - 1, MPFR_RNDN);
  mpfr_nextbelow (b);
  mpfr_mul_2ui (b, b, 1, MPFR_RNDN);
  /* now b is the largest number < +Inf */
  mpfr_div_2ui (c, b, GMP_NUMB_BITS - 1, MPFR_RNDN);
  /* we are in the case d < GMP_NUMB_BITS of mpfr_add1sp3 */
  inex = mpfr_add_cf (a, b, b, MPFR_RNDU);
  MPFR_ASSERTN(inex > 0);
  MPFR_ASSERTN(mpfr_inf_p (a) && mpfr_sgn (a) > 0);
  set_emax (emax);

  /* coverage test in mpfr_add1sp3: case round away, where add_one_ulp gives
     a carry, no overflow */
  mpfr_set_prec (a, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_prec (b, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_prec (c, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_ui (b, 1, MPFR_RNDN);
  mpfr_nextbelow (b); /* b = 1 - 2^(-p) */
  mpfr_set_ui_2exp (c, 1, -2 * GMP_NUMB_BITS - 2, MPFR_RNDN);
  /* c = 2^(-p-1) */
  inex = mpfr_add_cf (a, b, c, MPFR_RNDU);
  MPFR_ASSERTN(inex > 0);
  MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);

  /* coverage test in mpfr_add1sp3: case round away, where add_one_ulp gives
     a carry, with overflow */
  emax = mpfr_get_emax ();
  set_emax (1);
  mpfr_set_prec (a, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_prec (b, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_prec (c, 2 * GMP_NUMB_BITS + 1);
  mpfr_set_ui_2exp (b, 1, mpfr_get_emax () - 1, MPFR_RNDN);
  mpfr_nextbelow (b);
  mpfr_mul_2ui (b, b, 1, MPFR_RNDN);
  /* now b is the largest number < +Inf */
  mpfr_set_ui_2exp (c, 1, mpfr_get_emin () - 1, MPFR_RNDN);
  inex = mpfr_add_cf (a, b, c, MPFR_RNDU);
  MPFR_ASSERTN(inex > 0);
  MPFR_ASSERTN(mpfr_inf_p (a) && mpfr_sgn (a) > 0);
  set_emax (emax);

  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
}

int
main (void)
{
  mpfr_prec_t p;
  int i;

  tests_start_mpfr ();

  coverage ();
  test_corner_1 (1024);
  bug20171217 ();
  bug20190903 ();
  check_special ();
  for (p = MPFR_PREC_MIN; p < 200; p++)
    check_random (p);
  for (i = 0; i < 200; i++)
    {
      /* special precisions */
      check_random (GMP_NUMB_BITS);
      check_random (2 * GMP_NUMB_BITS);
    }
  check_overflow ();

  tests_end_mpfr ();
  return 0;
}

#define STD_ERROR                                                       \
  do                                                                    \
    {                                                                   \
      printf("ERROR: for %s and p=%lu and i=%d:\nB=",                   \
             mpfr_print_rnd_mode ((mpfr_rnd_t) r), (unsigned long) p, i); \
      mpfr_dump (b);                                                    \
      printf ("C="); mpfr_dump (c);                                     \
      printf ("add1  : "); mpfr_dump (a1);                              \
      printf ("add1sp: "); mpfr_dump (a2);                              \
      exit(1);                                                          \
    }                                                                   \
  while (0)

#define STD_ERROR2                                                      \
  do                                                                    \
    {                                                                   \
      printf("ERROR: Wrong inexact flag for %s and p=%lu and i=%d:\nB=", \
             mpfr_print_rnd_mode ((mpfr_rnd_t) r), (unsigned long) p, i); \
      mpfr_dump (b);                                                    \
      printf ("C="); mpfr_dump (c);                                     \
      printf ("A="); mpfr_dump (a1);                                    \
      printf ("Add1: %d. Add1sp: %d\n", inexact1, inexact2);            \
      exit(1);                                                          \
    }                                                                   \
 while (0)

#define SET_PREC(_p)                                    \
  do                                                    \
    {                                                   \
      p = _p;                                           \
      mpfr_set_prec(a1, _p); mpfr_set_prec(a2, _p);     \
      mpfr_set_prec(b,  _p); mpfr_set_prec(c,  _p);     \
    }                                                   \
  while (0)

static void
check_random (mpfr_prec_t p)
{
  mpfr_t a1, a2, b, bs, c, cs;
  int r;
  int i, inexact1, inexact2;

  mpfr_inits2 (p, a1, a2, b, c, (mpfr_ptr) 0);

  for (i = 0 ; i < 500 ; i++)
    {
      mpfr_urandom (b, RANDS, MPFR_RNDA);
      mpfr_urandom (c, RANDS, MPFR_RNDA);
      if (MPFR_IS_PURE_FP(b) && MPFR_IS_PURE_FP(c))
        {
          if (RAND_BOOL ())
            mpfr_neg (b, b, MPFR_RNDN);
          if (RAND_BOOL ())
            mpfr_neg (c, c, MPFR_RNDN);
          if (MPFR_GET_EXP(b) < MPFR_GET_EXP(c))
            {
              /* Exchange b and c, except the signs (actually, the sign
                 of cs doesn't matter). */
              MPFR_ALIAS (bs, c, MPFR_SIGN (b), MPFR_EXP (c));
              MPFR_ALIAS (cs, b, MPFR_SIGN (c), MPFR_EXP (b));
            }
          else
            {
              MPFR_ALIAS (bs, b, MPFR_SIGN (b), MPFR_EXP (b));
              MPFR_ALIAS (cs, c, MPFR_SIGN (c), MPFR_EXP (c));
            }
          RND_LOOP (r)
            {
              mpfr_flags_t flags1, flags2;

              if (r == MPFR_RNDF) /* inexact makes no sense, moreover
                                     mpfr_add1 and mpfr_add1sp could
                                     return different values */
                continue;

              mpfr_clear_flags ();
              inexact1 = mpfr_add1 (a1, bs, cs, (mpfr_rnd_t) r);
              flags1 = __gmpfr_flags;
              mpfr_clear_flags ();
              inexact2 = mpfr_add1sp (a2, b, c, (mpfr_rnd_t) r);
              flags2 = __gmpfr_flags;
              if (! mpfr_equal_p (a1, a2))
                STD_ERROR;
              if (inexact1 != inexact2)
                STD_ERROR2;
              MPFR_ASSERTN (flags1 == flags2);
            }
        }
    }

  mpfr_clears (a1, a2, b, c, (mpfr_ptr) 0);
}

static void
check_special (void)
{
  mpfr_t a1, a2, b, c;
  int r;
  mpfr_prec_t p;
  int i = -1, inexact1, inexact2;

  mpfr_inits (a1, a2, b, c, (mpfr_ptr) 0);

  RND_LOOP (r)
    {
      if (r == MPFR_RNDF)
        continue; /* inexact makes no sense, mpfr_add1 and mpfr_add1sp
                     could differ */
      SET_PREC(53);
      mpfr_set_str1 (b, "1@100");
      mpfr_set_str1 (c, "1@1");
      inexact1 = mpfr_add1(a1, b, c, (mpfr_rnd_t) r);
      inexact2 = mpfr_add1sp(a2, b, c, (mpfr_rnd_t) r);
      if (mpfr_cmp(a1, a2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;
      mpfr_set_str_binary (b, "1E53");
      mpfr_set_str_binary (c, "1E0");
      inexact1 = mpfr_add1(a1, b, c, (mpfr_rnd_t) r);
      inexact2 = mpfr_add1sp(a2, b, c, (mpfr_rnd_t) r);
      if (mpfr_cmp(a1, a2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;
    }

  mpfr_set_prec (c, 2);
  mpfr_set_prec (a1, 2);
  mpfr_set_prec (a2, 2);
  mpfr_set_str_binary (c, "1.0e1");
  mpfr_set_str_binary (a2, "1.1e-1");
  mpfr_set_str_binary (a1, "0.11E2");
  mpfr_add1sp (a2, c, a2, MPFR_RNDN);
  if (mpfr_cmp (a1, a2))
    {
      printf ("Regression reuse test failed!\n");
      exit (1);
    }

  mpfr_set_prec (a1, 63);
  mpfr_set_prec (b, 63);
  mpfr_set_prec (c, 63);
  mpfr_set_str_binary (b, "0.111111101010110111010100110101010110000101111011011011100011001E-3");
  mpfr_set_str_binary (c, "0.101111111101110000001100001000011000011011010001010011111100111E-4");
  mpfr_clear_inexflag ();
  mpfr_add1sp (a1, b, c, MPFR_RNDN);
  MPFR_ASSERTN (mpfr_inexflag_p ());

  mpfr_clears (a1, a2, b, c, (mpfr_ptr) 0);
}
