/* Test file for mpfr_sub1sp.

Copyright 2003-2025 Free Software Foundation, Inc.
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
static void check_underflow (mpfr_prec_t p);
static void check_corner (mpfr_prec_t p);

static void
bug20170109 (void)
{
  mpfr_t a, b, c;

  mpfr_init2 (a, 111);
  mpfr_init2 (b, 111);
  mpfr_init2 (c, 111);
  mpfr_set_str_binary (b, "0.110010010000111111011010101000100010000101101000110000100011010011000100110001100110001010001011100000001101110E1");
  mpfr_set_str_binary (c, "0.111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111E-63");
  mpfr_sub (a, b, c, MPFR_RNDN);
  mpfr_set_str_binary (b, "0.110010010000111111011010101000100010000101101000110000100011001111000100110001100110001010001011100000001101110E1");
  MPFR_ASSERTN(mpfr_equal_p (a, b));
  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
}

/* check mpfr_sub1sp1 when:
   (1) p = GMP_NUMB_BITS-1, d = GMP_NUMB_BITS and bp[0] = MPFR_LIMB_HIGHBIT
   (2) p = 2*GMP_NUMB_BITS-1, d = 2*GMP_NUMB_BITS and b = 1000...000
   (3) p = 3*GMP_NUMB_BITS-1, d = 3*GMP_NUMB_BITS and b = 1000...000
*/
static void
test20170208 (void)
{
  mpfr_t a, b, c;
  int inex;

  mpfr_inits2 (GMP_NUMB_BITS - 1, a, b, c, (mpfr_ptr) 0);

  /* test (1) */
  mpfr_set_ui_2exp (b, 1, GMP_NUMB_BITS, MPFR_RNDN);
  mpfr_set_ui_2exp (c, 1, 0, MPFR_RNDN);
  inex = mpfr_sub (a, b, c, MPFR_RNDN);
  /* b-c = 2^GMP_NUMB_BITS-1 which has GMP_NUMB_BITS bits, thus we should
     round to 2^GMP_NUMB_BITS (even rule) */
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, GMP_NUMB_BITS) == 0);
  MPFR_ASSERTN(inex > 0);
  inex = mpfr_sub1sp (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, GMP_NUMB_BITS) == 0);
  MPFR_ASSERTN(inex > 0);

  mpfr_set_ui_2exp (c, 2, 0, MPFR_RNDN);
  mpfr_nextbelow (c);
  /* now c = 2 - 2^(1-GMP_NUMB_BITS) */
  inex = mpfr_sub (a, b, c, MPFR_RNDN);
  /* b-c = 2^GMP_NUMB_BITS-2+2^(1-GMP_NUMB_BITS), which should
     round to 2^GMP_NUMB_BITS-2. We check by directly inspecting the bit
     field of a, since mpfr_cmp_ui might not work if unsigned long is shorter
     than mp_limb_t, and we don't want to use mpfr_add_ui or mpfr_sub_ui
     to construct the expected result. */
  MPFR_ASSERTN(MPFR_MANT(a)[0] == (mp_limb_t) -2);
  MPFR_ASSERTN(MPFR_EXP(a) == GMP_NUMB_BITS);
  MPFR_ASSERTN(inex < 0);
  inex = mpfr_sub1sp (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(MPFR_MANT(a)[0] == (mp_limb_t) -2);
  MPFR_ASSERTN(MPFR_EXP(a) == GMP_NUMB_BITS);
  MPFR_ASSERTN(inex < 0);

  /* test (2) */
  mpfr_set_prec (a, 2 * GMP_NUMB_BITS - 1);
  mpfr_set_prec (b, 2 * GMP_NUMB_BITS - 1);
  mpfr_set_prec (c, 2 * GMP_NUMB_BITS - 1);
  mpfr_set_ui_2exp (b, 1, 2 * GMP_NUMB_BITS, MPFR_RNDN);
  mpfr_set_ui_2exp (c, 1, 0, MPFR_RNDN);
  inex = mpfr_sub (a, b, c, MPFR_RNDN);
  /* b-c = 2^(2*GMP_NUMB_BITS)-1 which has 2*GMP_NUMB_BITS bits, thus we should
     round to 2^(2*GMP_NUMB_BITS) (even rule) */
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, 2 * GMP_NUMB_BITS) == 0);
  MPFR_ASSERTN(inex > 0);
  inex = mpfr_sub1sp (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, 2 * GMP_NUMB_BITS) == 0);
  MPFR_ASSERTN(inex > 0);

  mpfr_set_ui_2exp (c, 2, 0, MPFR_RNDN);
  mpfr_nextbelow (c);
  /* now c = 2 - 2^(1-2*GMP_NUMB_BITS) */
  inex = mpfr_sub (a, b, c, MPFR_RNDN);
  /* b-c = 2^(2*GMP_NUMB_BITS)-2+2^(1-2*GMP_NUMB_BITS), which should
     round to 2^(2*GMP_NUMB_BITS)-2. We check by directly inspecting the bit
     field of a, since mpfr_cmp_ui might not work if unsigned long is shorter
     than mp_limb_t, and we don't want to use mpfr_add_ui or mpfr_sub_ui
     to construct the expected result. */
  MPFR_ASSERTN(MPFR_MANT(a)[1] == (mp_limb_t) -1);
  MPFR_ASSERTN(MPFR_MANT(a)[0] == (mp_limb_t) -2);
  MPFR_ASSERTN(MPFR_EXP(a) == 2 * GMP_NUMB_BITS);
  MPFR_ASSERTN(inex < 0);
  inex = mpfr_sub1sp (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(MPFR_MANT(a)[1] == (mp_limb_t) -1);
  MPFR_ASSERTN(MPFR_MANT(a)[0] == (mp_limb_t) -2);
  MPFR_ASSERTN(MPFR_EXP(a) == 2 * GMP_NUMB_BITS);
  MPFR_ASSERTN(inex < 0);

  /* test (3) */
  mpfr_set_prec (a, 3 * GMP_NUMB_BITS - 1);
  mpfr_set_prec (b, 3 * GMP_NUMB_BITS - 1);
  mpfr_set_prec (c, 3 * GMP_NUMB_BITS - 1);
  mpfr_set_ui_2exp (b, 1, 3 * GMP_NUMB_BITS, MPFR_RNDN);
  mpfr_set_ui_2exp (c, 1, 0, MPFR_RNDN);
  inex = mpfr_sub (a, b, c, MPFR_RNDN);
  /* b-c = 2^(3*GMP_NUMB_BITS)-1 which has 3*GMP_NUMB_BITS bits, thus we should
     round to 3^(2*GMP_NUMB_BITS) (even rule) */
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, 3 * GMP_NUMB_BITS) == 0);
  MPFR_ASSERTN(inex > 0);
  inex = mpfr_sub1sp (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (a, 1, 3 * GMP_NUMB_BITS) == 0);
  MPFR_ASSERTN(inex > 0);

  mpfr_set_ui_2exp (c, 2, 0, MPFR_RNDN);
  mpfr_nextbelow (c);
  /* now c = 2 - 2^(1-3*GMP_NUMB_BITS) */
  inex = mpfr_sub (a, b, c, MPFR_RNDN);
  /* b-c = 2^(3*GMP_NUMB_BITS)-2+2^(1-3*GMP_NUMB_BITS), which should
     round to 2^(3*GMP_NUMB_BITS)-2. We check by directly inspecting the bit
     field of a, since mpfr_cmp_ui might not work if unsigned long is shorter
     than mp_limb_t, and we don't want to use mpfr_add_ui or mpfr_sub_ui
     to construct the expected result. */
  MPFR_ASSERTN(MPFR_MANT(a)[2] == (mp_limb_t) -1);
  MPFR_ASSERTN(MPFR_MANT(a)[1] == (mp_limb_t) -1);
  MPFR_ASSERTN(MPFR_MANT(a)[0] == (mp_limb_t) -2);
  MPFR_ASSERTN(MPFR_EXP(a) == 3 * GMP_NUMB_BITS);
  MPFR_ASSERTN(inex < 0);
  inex = mpfr_sub1sp (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(MPFR_MANT(a)[2] == (mp_limb_t) -1);
  MPFR_ASSERTN(MPFR_MANT(a)[1] == (mp_limb_t) -1);
  MPFR_ASSERTN(MPFR_MANT(a)[0] == (mp_limb_t) -2);
  MPFR_ASSERTN(MPFR_EXP(a) == 3 * GMP_NUMB_BITS);
  MPFR_ASSERTN(inex < 0);

  mpfr_clears (a, b, c, (mpfr_ptr) 0);
}

static void
compare_sub_sub1sp (void)
{
  mpfr_t a, b, c, a_ref;
  mpfr_prec_t p;
  unsigned long d;
  int i, inex_ref, inex;
  int r;

  for (p = 1; p <= 3*GMP_NUMB_BITS; p++)
    {
      mpfr_inits2 (p, a, b, c, a_ref, (mpfr_ptr) 0);
      for (d = 0; d <= p + 2; d++)
        {
          /* EXP(b) - EXP(c) = d */
          for (i = 0; i < 4; i++)
            {
              /* for i even, b is the smallest number, for b odd the largest */
              mpfr_set_ui_2exp (b, 1, d, MPFR_RNDN);
              if (i & 1)
                {
                  mpfr_mul_2ui (b, b, 1, MPFR_RNDN);
                  mpfr_nextbelow (b);
                }
              mpfr_set_ui_2exp (c, 1, 0, MPFR_RNDN);
              if (i & 2)
                {
                  mpfr_mul_2ui (c, c, 1, MPFR_RNDN);
                  mpfr_nextbelow (c);
                }
              RND_LOOP_NO_RNDF (r)
                {
                  /* increase the precision of b to ensure sub1sp is not used */
                  mpfr_prec_round (b, p + 1, MPFR_RNDN);
                  inex_ref = mpfr_sub (a_ref, b, c, (mpfr_rnd_t) r);
                  inex = mpfr_prec_round (b, p, MPFR_RNDN);
                  MPFR_ASSERTN(inex == 0);
                  inex = mpfr_sub1sp (a, b, c, (mpfr_rnd_t) r);
                  if (inex != inex_ref)
                    {
                      printf ("mpfr_sub and mpfr_sub1sp differ for r=%s\n",
                              mpfr_print_rnd_mode ((mpfr_rnd_t) r));
                      printf ("b="); mpfr_dump (b);
                      printf ("c="); mpfr_dump (c);
                      printf ("expected inex=%d and ", inex_ref);
                      mpfr_dump (a_ref);
                      printf ("got      inex=%d and ", inex);
                      mpfr_dump (a);
                      exit (1);
                    }
                  MPFR_ASSERTN(mpfr_equal_p (a, a_ref));
                }
            }
        }
      mpfr_clears (a, b, c, a_ref, (mpfr_ptr) 0);
    }
}

static void
bug20171213 (void)
{
  mpfr_t a, b, c;

  mpfr_init2 (a, 127);
  mpfr_init2 (b, 127);
  mpfr_init2 (c, 127);
  mpfr_set_str_binary (b, "0.1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000E1");
  mpfr_set_str_binary (c, "0.1000011010111101100101100110101111111001011001010000110000000000000000000000000000000000000000000000000000000000000000000000000E-74");
  mpfr_sub (a, b, c, MPFR_RNDN);
  mpfr_set_str_binary (b, "0.1111111111111111111111111111111111111111111111111111111111111111111111111101111001010000100110100110010100000001101001101011110E0");
  MPFR_ASSERTN(mpfr_equal_p (a, b));
  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
}

/* generic test for bug20171213:
   b = 1.0 with precision p
   c = 0.1xxx110...0E-e with precision p, with e >= 1, such that the part 1xxx1 has
       exactly p+1-e bits, thus b-c = 0.111..01... is exact on p+1 bits.
   Due to the round-to-even rule, b-c should be rounded to 0.111..0.
*/
static void
bug20171213_gen (mpfr_prec_t pmax)
{
  mpfr_prec_t p;
  mpfr_exp_t e;
  mpfr_t a, b, c, d;

  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    {
      for (e = 1; e < p; e++)
        {
          mpfr_init2 (a, p);
          mpfr_init2 (b, p);
          mpfr_init2 (c, p);
          mpfr_init2 (d, p);
          mpfr_set_ui (b, 1, MPFR_RNDN);
          mpfr_set_ui_2exp (c, 1, p + 1 - e, MPFR_RNDN); /* c = 2^(p + 1 - e) */
          mpfr_sub_ui (c, c, 1, MPFR_RNDN); /* c = 2^(p + 1 - e) - 1 */
          mpfr_div_2ui (c, c, p + 1, MPFR_RNDN); /* c = 2^(-e) - 2^(-p-1) */
          /* the exact difference is 1 - 2^(-e) + 2^(-p-1) */
          mpfr_sub (a, b, c, MPFR_RNDN);
          /* check that a = 1 - 2^(-e) */
          mpfr_set_ui_2exp (d, 1, e, MPFR_RNDN); /* b = 2^e */
          mpfr_sub_ui (d, d, 1, MPFR_RNDN);      /* b = 2^e - 1 */
          mpfr_div_2ui (d, d, e, MPFR_RNDN);    /* b = 1 - 2^(-e) */
          if (! mpfr_equal_p (a, d))
            {
              printf ("bug20171213_gen failed for p=%ld, e=%ld\n",
                      (long) p, (long) e);
              printf ("b="); mpfr_dump (b);
              printf ("c="); mpfr_dump (c);
              printf ("got      a="); mpfr_dump (a);
              printf ("expected d="); mpfr_dump (d);
              exit (1);
            }
          mpfr_clear (a);
          mpfr_clear (b);
          mpfr_clear (c);
          mpfr_clear (d);
        }
    }
}

static void
coverage (void)
{
  mpfr_t a, b, c, d, u;
  int inex;

  /* coverage test in mpfr_sub1sp: case d=1, limb > MPFR_LIMB_HIGHBIT, RNDF
     and also RNDZ */
  mpfr_init2 (a, 3 * GMP_NUMB_BITS);
  mpfr_init2 (b, 3 * GMP_NUMB_BITS);
  mpfr_init2 (c, 3 * GMP_NUMB_BITS);
  mpfr_init2 (d, 3 * GMP_NUMB_BITS);
  mpfr_init2 (u, 3 * GMP_NUMB_BITS);
  mpfr_set_ui (b, 1, MPFR_RNDN);
  mpfr_nextbelow (b); /* b = 1 - 2^(-p) */
  mpfr_set_prec (c, GMP_NUMB_BITS);
  mpfr_set_ui_2exp (c, 1, -1, MPFR_RNDN);
  mpfr_nextbelow (c);
  mpfr_nextbelow (c); /* c = 1/2 - 2*2^(-GMP_NUMB_BITS-1) */
  mpfr_prec_round (c, 3 * GMP_NUMB_BITS, MPFR_RNDN);
  mpfr_nextbelow (c); /* c = 1/2 - 2*2^(-GMP_NUMB_BITS-1) - 2^(-p-1) */
  /* b-c = c */
  mpfr_sub (a, b, c, MPFR_RNDF);
  mpfr_sub (d, b, c, MPFR_RNDD);
  mpfr_sub (u, b, c, MPFR_RNDU);
  /* check a = d or u */
  MPFR_ASSERTN(mpfr_equal_p (a, d) || mpfr_equal_p (a, u));

  /* coverage test in mpfr_sub1sp: case d=p, RNDN, sb = 0, significand of b
     is even but b<>2^e, (case 1e) */
  mpfr_set_prec (a, 3 * GMP_NUMB_BITS);
  mpfr_set_prec (b, 3 * GMP_NUMB_BITS);
  mpfr_set_prec (c, 3 * GMP_NUMB_BITS);
  mpfr_set_ui (b, 1, MPFR_RNDN);
  mpfr_nextabove (b);
  mpfr_nextabove (b);
  mpfr_set_ui_2exp (c, 1, -3 * GMP_NUMB_BITS, MPFR_RNDN);
  inex = mpfr_sub (a, b, c, MPFR_RNDN);
  MPFR_ASSERTN(inex > 0);
  MPFR_ASSERTN(mpfr_equal_p (a, b));

  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
  mpfr_clear (d);
  mpfr_clear (u);
}

/* bug in mpfr_sub1sp1n, made generic */
static void
bug20180217 (mpfr_prec_t pmax)
{
  mpfr_t a, b, c;
  int inex;
  mpfr_prec_t p;

  for (p = MPFR_PREC_MIN; p <= pmax; p++)
    {
      mpfr_init2 (a, p);
      mpfr_init2 (b, p);
      mpfr_init2 (c, p);
      mpfr_set_ui (b, 1, MPFR_RNDN); /* b = 1 */
      mpfr_set_ui_2exp (c, 1, -p-1, MPFR_RNDN); /* c = 2^(-p-1) */
      /* a - b = 1 - 2^(-p-1) and should be rounded to 1 (case 2f of
         mpfr_sub1sp) */
      inex = mpfr_sub (a, b, c, MPFR_RNDN);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);
      /* check also when a=b */
      mpfr_set_ui (a, 1, MPFR_RNDN);
      inex = mpfr_sub (a, a, c, MPFR_RNDN);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);
      /* and when a=c */
      mpfr_set_ui (b, 1, MPFR_RNDN); /* b = 1 */
      mpfr_set_ui_2exp (a, 1, -p-1, MPFR_RNDN);
      inex = mpfr_sub (a, b, a, MPFR_RNDN);
      MPFR_ASSERTN(inex > 0);
      MPFR_ASSERTN(mpfr_cmp_ui (a, 1) == 0);
      mpfr_clear (a);
      mpfr_clear (b);
      mpfr_clear (c);
    }
}

/* bug in revision 12985 with tlog and GMP_CHECK_RANDOMIZE=1534111552615050
   (introduced in revision 12242, does not affect the 4.0 branch) */
static void
bug20180813 (void)
{
  mpfr_t a, b, c;

  mpfr_init2 (a, 194);
  mpfr_init2 (b, 194);
  mpfr_init2 (c, 194);
  mpfr_set_str_binary (b, "0.10000111101000100000010000100010110111011100110100000101100111000010101000110110010101011101101011110110001000111001000010110010111010010100011011010100001010001110000101000010101110100110001000E7");
  mpfr_set_str_binary (c, "0.10000000000000000100001111010001000000100001000101101110111001101000001011001110000101010001101100101010111011010111101100010001110010000101100101110100101000110110101000010100011100001010000101E24");
  mpfr_sub (a, b, c, MPFR_RNDN);
  mpfr_set_str_binary (b, "-0.11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111E23");
  MPFR_ASSERTN(mpfr_equal_p (a, b));
  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
}

/* bug in revision 13599 with tatan and GMP_CHECK_RANDOMIZE=1567609230659336:
   the values are equal, but the ternary value differs between sub1 and sub1sp
   (bug introduced with mpfr_sub1sp2n, does not affect the 4.0 branch) */
static void
bug20190904 (void)
{
  mpfr_t a, b, c;
  int ret;

  mpfr_init2 (a, 128);
  mpfr_init2 (b, 128);
  mpfr_init2 (c, 128);
  mpfr_set_str_binary (b, "0.11001001000011111101101010100010001000010110100011000010001101001100010011000110011000101000101110000000110111000001110011010001E1");
  mpfr_set_str_binary (c, "0.10010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010010000000000000000000000E-102");
  ret = mpfr_sub (a, b, c, MPFR_RNDN);
  mpfr_set_str_binary (b, "0.11001001000011111101101010100010001000010110100011000010001101001100010011000110011000101000101101111111101111000001110011010001E1");
  MPFR_ASSERTN(mpfr_equal_p (a, b));
  MPFR_ASSERTN(ret > 0);
  mpfr_clear (a);
  mpfr_clear (b);
  mpfr_clear (c);
}

int
main (void)
{
  mpfr_prec_t p;

  tests_start_mpfr ();

  bug20190904 ();
  bug20180813 ();
  bug20180217 (1024);
  coverage ();
  compare_sub_sub1sp ();
  test20170208 ();
  bug20170109 ();
  bug20171213 ();
  bug20171213_gen (256);
  check_special ();
  for (p = MPFR_PREC_MIN ; p < 200 ; p++)
    {
      check_underflow (p);
      check_random (p);
      check_corner (p);
    }

  tests_end_mpfr ();
  return 0;
}

#define STD_ERROR                                                       \
  do                                                                    \
    {                                                                   \
      printf("ERROR: for %s and p=%lu and i=%d:\nY=",                   \
             mpfr_print_rnd_mode ((mpfr_rnd_t) r), (unsigned long) p, i); \
      mpfr_dump (y);                                                    \
      printf ("Z="); mpfr_dump (z);                                     \
      printf ("Expected: "); mpfr_dump (x2);                            \
      printf ("Got :     "); mpfr_dump (x);                             \
      abort();                                                          \
    }                                                                   \
 while (0)

#define STD_ERROR2                                                      \
  do                                                                    \
    {                                                                   \
      printf("ERROR: for %s and p=%lu and i=%d:\nY=",                   \
             mpfr_print_rnd_mode ((mpfr_rnd_t) r), (unsigned long) p, i); \
      mpfr_dump (y);                                                    \
      printf ("Z="); mpfr_dump (z);                                     \
      printf ("Expected: "); mpfr_dump (x2);                            \
      printf ("Got :     "); mpfr_dump (x);                             \
      printf ("Wrong inexact flag. Expected %d. Got %d\n",              \
              inexact1, inexact2);                                      \
      exit(1);                                                          \
    }                                                                   \
 while (0)

static void
check_random (mpfr_prec_t p)
{
  mpfr_t x,y,z,x2;
  int r;
  int i, inexact1, inexact2;

  mpfr_inits2 (p, x, y, z, x2, (mpfr_ptr) 0);

  for (i = 0 ; i < 500 ; i++)
    {
      mpfr_urandomb (y, RANDS);
      mpfr_urandomb (z, RANDS);
      if (MPFR_IS_PURE_FP(y) && MPFR_IS_PURE_FP(z))
        RND_LOOP (r)
          {
            if (r == MPFR_RNDF)
              continue; /* mpfr_sub1 and mpfr_sub1sp could differ,
                           and inexact makes no sense */
            inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
            inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
            if (mpfr_cmp(x, x2))
              STD_ERROR;
            if (inexact1 != inexact2)
              STD_ERROR2;
          }
    }

  mpfr_clears (x, y, z, x2, (mpfr_ptr) 0);
}

static void
check_special (void)
{
  mpfr_t x,y,z,x2;
  int r;
  mpfr_prec_t p;
  int i = -1, inexact1, inexact2;
  mpfr_exp_t es;

  mpfr_inits (x, y, z, x2, (mpfr_ptr) 0);

  RND_LOOP (r)
    {
      if (r == MPFR_RNDF)
        continue; /* comparison between sub1 and sub1sp makes no sense here */

      p = 53;
      mpfr_set_prec(x, 53);
      mpfr_set_prec(x2, 53);
      mpfr_set_prec(y, 53);
      mpfr_set_prec(z, 53);

      mpfr_set_str_binary (y,
       "0.10110111101101110010010010011011000001101101011011001E31");

      mpfr_sub1sp (x, y, y, (mpfr_rnd_t) r);
      if (mpfr_cmp_ui(x, 0))
        {
          printf("Error for x-x with p=%lu. Expected 0. Got:",
                 (unsigned long) p);
          mpfr_dump (x);
          exit(1);
        }

      mpfr_set(z, y, (mpfr_rnd_t) r);
      mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp_ui(x, 0))
        {
          printf("Error for x-y with y=x and p=%lu. Expected 0. Got:",
                 (unsigned long) p);
          mpfr_dump (x);
          exit(1);
        }
      /* diff = 0 */
      mpfr_set_str_binary (y,
       "0.10110111101101110010010010011011001001101101011011001E31");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      /* Diff = 1 */
      mpfr_set_str_binary (y,
       "0.10110111101101110010010010011011000001101101011011001E30");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      /* Diff = 2 */
      mpfr_set_str_binary (y,
       "0.10110111101101110010010010011011000101101101011011001E32");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      /* Diff = 32 */
      mpfr_set_str_binary (y,
       "0.10110111101101110010010010011011000001101101011011001E63");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      /* Diff = 52 */
      mpfr_set_str_binary (y,
       "0.10110111101101110010010010011011010001101101011011001E83");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      /* Diff = 53 */
      mpfr_set_str_binary (y,
       "0.10110111101101110010010010011111000001101101011011001E31");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      /* Diff > 200 */
      mpfr_set_str_binary (y,
       "0.10110111101101110010010010011011000001101101011011001E331");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
       "0.10000000000000000000000000000000000000000000000000000E31");
      mpfr_set_str_binary (z,
       "0.11111111111111111111111111111111111111111111111111111E30");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
       "0.10000000000000000000000000000000000000000000000000000E31");
      mpfr_set_str_binary (z,
       "0.11111111111111111111111111111111111111111111111111111E29");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
       "0.10000000000000000000000000000000000000000000000000000E52");
      mpfr_set_str_binary (z,
       "0.10000000000010000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
        "0.11100000000000000000000000000000000000000000000000000E53");
      mpfr_set_str_binary (z,
        "0.10000000000000000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(z, y, z, (mpfr_rnd_t) r);
      mpfr_set(x, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
       "0.10000000000000000000000000000000000000000000000000000E53");
      mpfr_set_str_binary (z,
       "0.10100000000000000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
        "0.10000000000000000000000000000000000000000000000000000E54");
      mpfr_set_str_binary (z,
        "0.10100000000000000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      p = 63;
      mpfr_set_prec(x, p);
      mpfr_set_prec(x2, p);
      mpfr_set_prec(y, p);
      mpfr_set_prec(z, p);
      mpfr_set_str_binary (y,
      "0.100000000000000000000000000000000000000000000000000000000000000E62");
      mpfr_set_str_binary (z,
      "0.110000000000000000000000000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      p = 64;
      mpfr_set_prec(x, 64);
      mpfr_set_prec(x2, 64);
      mpfr_set_prec(y, 64);
      mpfr_set_prec(z, 64);

      mpfr_set_str_binary (y,
      "0.1100000000000000000000000000000000000000000000000000000000000000E31");
      mpfr_set_str_binary (z,
      "0.1111111111111111111111111110000000000000000000000000011111111111E29");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
      "0.1000000000000000000000000000000000000000000000000000000000000000E63");
      mpfr_set_str_binary (z,
      "0.1011000000000000000000000000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
      "0.1000000000000000000000000000000000000000000000000000000000000000E63");
      mpfr_set_str_binary (z,
      "0.1110000000000000000000000000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
        "0.10000000000000000000000000000000000000000000000000000000000000E63");
      mpfr_set_str_binary (z,
        "0.10000000000000000000000000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
      "0.1000000000000000000000000000000000000000000000000000000000000000E64");
      mpfr_set_str_binary (z,
      "0.1010000000000000000000000000000000000000000000000000000000000000E00");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      MPFR_SET_NAN(x);
      MPFR_SET_NAN(x2);
      mpfr_set_str_binary (y,
      "0.1000000000000000000000000000000000000000000000000000000000000000"
                          "E-1073741823");
      mpfr_set_str_binary (z,
      "0.1100000000000000000000000000000000000000000000000000000000000000"
                          "E-1073741823");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      p = 9;
      mpfr_set_prec(x, p);
      mpfr_set_prec(x2, p);
      mpfr_set_prec(y, p);
      mpfr_set_prec(z, p);

      mpfr_set_str_binary (y, "0.100000000E1");
      mpfr_set_str_binary (z, "0.100000000E-8");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      p = 34;
      mpfr_set_prec(x, p);
      mpfr_set_prec(x2, p);
      mpfr_set_prec(y, p);
      mpfr_set_prec(z, p);

      mpfr_set_str_binary (y, "-0.1011110000111100010111011100110100E-18");
      mpfr_set_str_binary (z, "0.1000101010110011010101011110000000E-14");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      p = 124;
      mpfr_set_prec(x, p);
      mpfr_set_prec(x2, p);
      mpfr_set_prec(y, p);
      mpfr_set_prec(z, p);

      mpfr_set_str_binary (y,
"0.1000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000E1");
      mpfr_set_str_binary (z,
"0.1011111000100111000011001000011101010101101100101010101001000001110100001101110110001110111010000011101001100010111110001100E-31");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      p = 288;
      mpfr_set_prec(x, p);
      mpfr_set_prec(x2, p);
      mpfr_set_prec(y, p);
      mpfr_set_prec(z, p);

      mpfr_set_str_binary (y,
     "0.111000110011000001000111101010111011110011101001101111111110000011100101000001001010110010101010011001010100000001110011110001010101101010001011101110100100001011110100110000101101100011010001001011011010101010000010001101001000110010010111111011110001111101001000101101001100101100101000E80");
      mpfr_set_str_binary (z,
     "-0.100001111111101001011010001100110010100111001110000110011101001011010100001000000100111011010110110010000000000010101101011000010000110001110010100001100101011100100100001011000100011110000001010101000100011101001000010111100000111000111011001000100100011000100000010010111000000100100111E-258");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      p = 85;
      mpfr_set_prec(x, p);
      mpfr_set_prec(x2, p);
      mpfr_set_prec(y, p);
      mpfr_set_prec(z, p);

      mpfr_set_str_binary (y,
"0.1111101110100110110110100010101011101001100010100011110110110010010011101100101111100E-4");
      mpfr_set_str_binary (z,
"0.1111101110100110110110100010101001001000011000111000011101100101110100001110101010110E-4");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      p = 64;
      mpfr_set_prec(x, p); mpfr_set_prec(x2, p);
      mpfr_set_prec(y, p); mpfr_set_prec(z, p);

      mpfr_set_str_binary (y,
                          "0.11000000000000000000000000000000"
                          "00000000000000000000000000000000E1");
      mpfr_set_str_binary (z,
                          "0.10000000000000000000000000000000"
                          "00000000000000000000000000000001E0");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
                          "0.11000000000000000000000000000000"
                          "000000000000000000000000000001E1");
      mpfr_set_str_binary (z,
                          "0.10000000000000000000000000000000"
                          "00000000000000000000000000000001E0");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      es = mpfr_get_emin ();
      set_emin (-1024);

      mpfr_set_str_binary (y,
                          "0.10000000000000000000000000000000"
                          "000000000000000000000000000000E-1023");
      mpfr_set_str_binary (z,
                          "0.10000000000000000000000000000000"
                          "00000000000000000000000000000001E-1023");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      mpfr_set_str_binary (y,
                           "0.10000000000000000000000000000000"
                           "000000000000000000000000000000E-1023");
      mpfr_set_str_binary (z,
                           "0.1000000000000000000000000000000"
                           "000000000000000000000000000000E-1023");
      inexact1 = mpfr_sub1(x2, y, z, (mpfr_rnd_t) r);
      inexact2 = mpfr_sub1sp(x, y, z, (mpfr_rnd_t) r);
      if (mpfr_cmp(x, x2))
        STD_ERROR;
      if (inexact1 != inexact2)
        STD_ERROR2;

      set_emin (es);
    }

  mpfr_clears (x, y, z, x2, (mpfr_ptr) 0);
}

static void
check_underflow (mpfr_prec_t p)
{
  mpfr_t x, y, z;
  int inexact;

  mpfr_inits2 (p, x, y, z, (mpfr_ptr) 0);

  if (p >= 2) /* we need p >= 2 so that 3 is exact */
    {
      mpfr_set_ui_2exp (y, 4, mpfr_get_emin () - 2, MPFR_RNDN);
      mpfr_set_ui_2exp (z, 3, mpfr_get_emin () - 2, MPFR_RNDN);
      inexact = mpfr_sub (x, y, z, MPFR_RNDN);
      if (inexact >= 0 || (mpfr_cmp_ui (x, 0) != 0))
        {
          printf ("4*2^(emin-2) - 3*2^(emin-2) with RNDN failed for p=%ld\n",
                  (long) p);
          printf ("Expected inexact < 0 with x=0\n");
          printf ("Got inexact=%d with x=", inexact);
          mpfr_dump (x);
          exit (1);
        }
    }

  if (p >= 3) /* we need p >= 3 so that 5 is exact */
    {
      mpfr_set_ui_2exp (y, 5, mpfr_get_emin () - 2, MPFR_RNDN);
      mpfr_set_ui_2exp (z, 4, mpfr_get_emin () - 2, MPFR_RNDN);
      inexact = mpfr_sub (x, y, z, MPFR_RNDN);
      if (inexact >= 0 || (mpfr_cmp_ui (x, 0) != 0))
        {
          printf ("5*2^(emin-2) - 4*2^(emin-2) with RNDN failed for p=%ld\n",
                  (long) p);
          printf ("Expected inexact < 0 with x=0\n");
          printf ("Got inexact=%d with x=", inexact);
          mpfr_dump (x);
          exit (1);
        }
    }

  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

/* check corner cases of mpfr_sub1sp in case d = 1 and limb = MPFR_LIMB_HIGHBIT */
static void
check_corner (mpfr_prec_t p)
{
  mpfr_t x, y, z;
  mpfr_exp_t e;
  int inex, odd;

  if (p < 4) /* ensures that the initial value of z is > 1 below */
    return;

  mpfr_inits2 (p, x, y, z, (mpfr_ptr) 0);
  mpfr_const_pi (y, MPFR_RNDN);
  mpfr_set_ui (z, 2, MPFR_RNDN);
  inex = mpfr_sub (z, y, z, MPFR_RNDN); /* z is near pi-2, thus y-z is near 2 */
  MPFR_ASSERTN(inex == 0);
  for (e = 0; e < p; e++)
    {
      /* add 2^(-e) to z */
      mpfr_mul_2ui (z, z, e, MPFR_RNDN);
      inex = mpfr_add_ui (z, z, 1, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);
      mpfr_div_2ui (z, z, e, MPFR_RNDN);

      /* compute x = y - z which should be exact, near 2-2^(-e) */
      inex = mpfr_sub (x, y, z, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);
      MPFR_ASSERTN(mpfr_get_exp (x) == 1);

      /* restore initial z */
      mpfr_mul_2ui (z, z, e, MPFR_RNDN);
      inex = mpfr_sub_ui (z, z, 1, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);
      mpfr_div_2ui (z, z, e, MPFR_RNDN);

      /* subtract 2^(-e) to z */
      mpfr_mul_2ui (z, z, e, MPFR_RNDN);
      inex = mpfr_sub_ui (z, z, 1, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);
      mpfr_div_2ui (z, z, e, MPFR_RNDN);

      /* ensure last significant bit of z is 0 so that y-z is exact */
      odd = mpfr_min_prec (z) == p;
      if (odd) /* add one ulp to z */
        mpfr_nextabove (z);

      /* compute x = y - z which should be exact, near 2+2^(-e) */
      inex = mpfr_sub (x, y, z, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);
      MPFR_ASSERTN(mpfr_get_exp (x) == 2);

      /* restore initial z */
      if (odd)
        mpfr_nextbelow (z);
      mpfr_mul_2ui (z, z, e, MPFR_RNDN);
      inex = mpfr_add_ui (z, z, 1, MPFR_RNDN);
      MPFR_ASSERTN(inex == 0);
      mpfr_div_2ui (z, z, e, MPFR_RNDN);
    }
  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}
