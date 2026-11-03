/* Test file for mpfr_fma.

Copyright 2001-2025 Free Software Foundation, Inc.
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

/* When a * b is exact, the FMA is equivalent to the separate operations. */
static void
test_exact (void)
{
  const char *val[] =
    { "@NaN@", "-@Inf@", "-2", "-1", "-0", "0", "1", "2", "@Inf@" };
  int sv = numberof (val);
  int i, j, k;
  int rnd;
  mpfr_t a, b, c, r1, r2;

  mpfr_inits2 (8, a, b, c, r1, r2, (mpfr_ptr) 0);

  for (i = 0; i < sv; i++)
    for (j = 0; j < sv; j++)
      for (k = 0; k < sv; k++)
        RND_LOOP (rnd)
          {
            if (mpfr_set_str (a, val[i], 10, MPFR_RNDN) ||
                mpfr_set_str (b, val[j], 10, MPFR_RNDN) ||
                mpfr_set_str (c, val[k], 10, MPFR_RNDN) ||
                mpfr_mul (r1, a, b, (mpfr_rnd_t) rnd) ||
                mpfr_add (r1, r1, c, (mpfr_rnd_t) rnd))
              {
                if (rnd == MPFR_RNDF)
                  continue;
                printf ("test_exact internal error for (%d,%d,%d,%d,%s)\n",
                        i, j, k, rnd, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                exit (1);
              }
            if (mpfr_fma (r2, a, b, c, (mpfr_rnd_t) rnd))
              {
                printf ("test_exact(%d,%d,%d,%d): mpfr_fma should be exact\n",
                        i, j, k, rnd);
                exit (1);
              }
            if (MPFR_IS_NAN (r1))
              {
                if (MPFR_IS_NAN (r2))
                  continue;
                printf ("test_exact(%d,%d,%d,%d): mpfr_fma should be NaN\n",
                        i, j, k, rnd);
                exit (1);
              }
            if (! mpfr_equal_p (r1, r2) || MPFR_SIGN (r1) != MPFR_SIGN (r2))
              {
                printf ("test_exact(%d,%d,%d,%d):\nexpected ", i, j, k, rnd);
                mpfr_out_str (stdout, 10, 0, r1, MPFR_RNDN);
                printf ("\n     got ");
                mpfr_out_str (stdout, 10, 0, r2, MPFR_RNDN);
                printf ("\n");
                exit (1);
              }
          }

  mpfr_clears (a, b, c, r1, r2, (mpfr_ptr) 0);
}

static void
test_overflow1 (void)
{
  mpfr_t x, y, z, r;
  int inex;

  mpfr_inits2 (8, x, y, z, r, (mpfr_ptr) 0);
  MPFR_SET_POS (x);
  mpfr_setmax (x, mpfr_get_emax ());  /* x = 2^emax - ulp */
  mpfr_set_ui (y, 2, MPFR_RNDN);       /* y = 2 */
  mpfr_neg (z, x, MPFR_RNDN);          /* z = -x = -(2^emax - ulp) */
  mpfr_clear_flags ();
  /* The intermediate multiplication x * y overflows, but x * y + z = x
     is representable. */
  inex = mpfr_fma (r, x, y, z, MPFR_RNDN);
  if (inex || ! mpfr_equal_p (r, x))
    {
      printf ("Error in test_overflow1\nexpected ");
      mpfr_out_str (stdout, 2, 0, x, MPFR_RNDN);
      printf (" with inex = 0\n     got ");
      mpfr_out_str (stdout, 2, 0, r, MPFR_RNDN);
      printf (" with inex = %d\n", inex);
      exit (1);
    }
  if (mpfr_overflow_p ())
    {
      printf ("Error in test_overflow1: overflow flag set\n");
      exit (1);
    }
  mpfr_clears (x, y, z, r, (mpfr_ptr) 0);
}

static void
test_overflow2 (void)
{
  mpfr_t x, y, z, r;
  int i, inex, rnd, err = 0;

  mpfr_inits2 (8, x, y, z, r, (mpfr_ptr) 0);

  MPFR_SET_POS (x);
  mpfr_setmin (x, mpfr_get_emax ());  /* x = 0.1@emax */
  mpfr_set_si (y, -2, MPFR_RNDN);      /* y = -2 */
  /* The intermediate multiplication x * y will overflow. */

  for (i = -9; i <= 9; i++)
    RND_LOOP_NO_RNDF (rnd)
      {
        int inf, overflow;

        inf = rnd == MPFR_RNDN || rnd == MPFR_RNDD || rnd == MPFR_RNDA;
        overflow = inf || i <= 0;

        inex = mpfr_set_si_2exp (z, i, mpfr_get_emin (), MPFR_RNDN);
        MPFR_ASSERTN (inex == 0);

        mpfr_clear_flags ();
        /* One has: x * y = -1@emax exactly (but not representable). */
        inex = mpfr_fma (r, x, y, z, (mpfr_rnd_t) rnd);
        if (overflow ^ (mpfr_overflow_p () != 0))
          {
            printf ("Error in test_overflow2 (i = %d, %s): wrong overflow"
                    " flag (should be %d)\n", i,
                    mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), overflow);
            err = 1;
          }
        if (mpfr_nanflag_p ())
          {
            printf ("Error in test_overflow2 (i = %d, %s): NaN flag should"
                    " not be set\n", i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
            err = 1;
          }
        if (mpfr_nan_p (r))
          {
            printf ("Error in test_overflow2 (i = %d, %s): got NaN\n",
                    i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
            err = 1;
          }
        else if (MPFR_IS_POS (r))
          {
            printf ("Error in test_overflow2 (i = %d, %s): wrong sign "
                    "(+ instead of -)\n", i,
                    mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
            err = 1;
          }
        else if (inf && ! mpfr_inf_p (r))
          {
            printf ("Error in test_overflow2 (i = %d, %s): expected -Inf,"
                    " got\n", i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
            mpfr_dump (r);
            err = 1;
          }
        else if (!inf && (mpfr_inf_p (r) ||
                          (mpfr_nextbelow (r), ! mpfr_inf_p (r))))
          {
            printf ("Error in test_overflow2 (i = %d, %s): expected -MAX,"
                    " got\n", i, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
            mpfr_dump (r);
            err = 1;
          }
        if (inf ? inex >= 0 : inex <= 0)
          {
            printf ("Error in test_overflow2 (i = %d, %s): wrong inexact"
                    " flag (got %d)\n", i,
                    mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), inex);
            err = 1;
          }

      }

  if (err)
    exit (1);
  mpfr_clears (x, y, z, r, (mpfr_ptr) 0);
}

static void
test_overflow3 (void)
{
  mpfr_t x, y, z, r;
  int inex;
  mpfr_prec_t p = 8;
  mpfr_flags_t ex_flags = MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT, flags;
  int i, j, k;
  unsigned int neg;

  mpfr_inits2 (p, x, y, z, (mpfr_ptr) 0);
  for (i = 0; i < 2; i++)
    {
      mpfr_init2 (r, 2 * p + i);
      mpfr_set_ui_2exp (x, 1, mpfr_get_emax () - 1, MPFR_RNDN);
      mpfr_set_ui (y, 2, MPFR_RNDN);       /* y = 2 */
      for (j = 1; j <= 2; j++)
        for (k = 0; k <= 1; k++)
          {
            mpfr_set_si_2exp (z, -1, mpfr_get_emax () - mpfr_get_prec (r) - j,
                              MPFR_RNDN);
            if (k)
              mpfr_nextabove (z);
            for (neg = 0; neg <= 3; neg++)
              {
                mpfr_clear_flags ();
                /* (The following applies for neg = 0 or 2, all the signs
                   need to be reversed for neg = 1 or 3.)
                   We have x*y = 2^emax and
                   z = - 2^(emax-2p-i-j) * (1-k*2^(-p)), thus
                   x*y+z = 2^emax - 2^(emax-2p-i-j) + k*2^(emax-3p-i-j)
                   should overflow. Indeed it is >= the midpoint of
                   2^emax - 2^(emax-2p-i) and 2^emax, the midpoint
                   being obtained for j = 1 and k = 0. */
                inex = mpfr_fma (r, x, y, z, MPFR_RNDN);
                flags = __gmpfr_flags;
                if (! mpfr_inf_p (r) || flags != ex_flags ||
                    ((neg & 1) == 0 ?
                     (inex <= 0 || MPFR_IS_NEG (r)) :
                     (inex >= 0 || MPFR_IS_POS (r))))
                  {
                    printf ("Error in test_overflow3 for "
                            "i=%d j=%d k=%d neg=%u\n", i, j, k, neg);
                    printf ("Expected %c@Inf@\n  with inex %c 0 and flags:",
                            (neg & 1) == 0 ? '+' : '-',
                            (neg & 1) == 0 ? '>' : '<');
                    flags_out (ex_flags);
                    printf ("Got      ");
                    mpfr_dump (r);
                    printf ("  with inex = %d and flags:", inex);
                    flags_out (flags);
                    exit (1);
                  }
                if (neg == 0 || neg == 2)
                  mpfr_neg (x, x, MPFR_RNDN);
                if (neg == 1 || neg == 3)
                  mpfr_neg (y, y, MPFR_RNDN);
                mpfr_neg (z, z, MPFR_RNDN);
              }  /* neg */
          }  /* k */
      mpfr_clear (r);
    }  /* i */
  mpfr_clears (x, y, z, (mpfr_ptr) 0);
}

static void
test_overflow4 (void)
{
  mpfr_t x, y, z, r1, r2;
  mpfr_exp_t emax, e;
  mpfr_prec_t px;
  mpfr_flags_t flags1, flags2;
  int inex1, inex2;
  int ei, i, j;
  int below;
  unsigned int neg;

  emax = mpfr_get_emax ();

  mpfr_init2 (y, MPFR_PREC_MIN);
  mpfr_set_ui (y, 2, MPFR_RNDN);  /* y = 2 */

  mpfr_init2 (z, 8);

  for (px = 17; px < 256; px *= 2)
    {
      mpfr_init2 (x, px);
      mpfr_inits2 (px - 8, r1, r2, (mpfr_ptr) 0);
      for (ei = 0; ei <= 1; ei++)
        {
          e = ei ? emax : 0;
          mpfr_set_ui_2exp (x, 1, e - 1, MPFR_RNDN);
          mpfr_nextabove (x);  /* x = 2^(e - 1) + 2^(e - px) */
          /* x*y = 2^e + 2^(e - px + 1), which internally overflows
             when e = emax. */
          for (i = -4; i <= 4; i++)
            for (j = 2; j <= 3; j++)
              {
                mpfr_set_si_2exp (z, -j, e - px + i, MPFR_RNDN);
                /* If |z| <= 2^(e - px + 1), then x*y + z >= 2^e and
                   RZ(x*y + z) = 2^e with an unbounded exponent range.
                   If |z| > 2^(e - px + 1), then RZ(x*y + z) is the
                   predecessor of 2^e (since |z| < ulp(r)/2); this
                   occurs when i > 0 and when i = 0 and j > 2 */
                mpfr_set_ui_2exp (r1, 1, e - 1, MPFR_RNDN);
                below = i > 0 || (i == 0 && j > 2);
                if (below)
                  mpfr_nextbelow (r1);
                mpfr_clear_flags ();
                inex1 = mpfr_mul_2ui (r1, r1, 1, MPFR_RNDZ);
                if (below || e < emax)
                  {
                    inex1 = i == 0 && j == 2 ? 0 : -1;
                    flags1 = inex1 ? MPFR_FLAGS_INEXACT : 0;
                  }
                else
                  {
                    MPFR_ASSERTN (inex1 < 0);
                    flags1 = MPFR_FLAGS_INEXACT | MPFR_FLAGS_OVERFLOW;
                    MPFR_ASSERTN (flags1 == __gmpfr_flags);
                  }
                for (neg = 0; neg <= 3; neg++)
                  {
                    mpfr_clear_flags ();
                    inex2 = mpfr_fma (r2, x, y, z, MPFR_RNDZ);
                    flags2 = __gmpfr_flags;
                    if (! (mpfr_equal_p (r1, r2) &&
                           SAME_SIGN (inex1, inex2) &&
                           flags1 == flags2))
                      {
                        printf ("Error in test_overflow4 for "
                                "px=%d ei=%d i=%d j=%d neg=%u\n",
                                (int) px, ei, i, j, neg);
                        printf ("Expected ");
                        mpfr_dump (r1);
                        printf ("with inex = %d and flags:", inex1);
                        flags_out (flags1);
                        printf ("Got      ");
                        mpfr_dump (r2);
                        printf ("with inex = %d and flags:", inex2);
                        flags_out (flags2);
                        exit (1);
                      }
                    if (neg == 0 || neg == 2)
                      mpfr_neg (x, x, MPFR_RNDN);
                    if (neg == 1 || neg == 3)
                      mpfr_neg (y, y, MPFR_RNDN);
                    mpfr_neg (z, z, MPFR_RNDN);
                    mpfr_neg (r1, r1, MPFR_RNDN);
                    inex1 = - inex1;
                  }
              }
        }
      mpfr_clears (x, r1, r2, (mpfr_ptr) 0);
    }

  mpfr_clears (y, z, (mpfr_ptr) 0);
}

static void
test_overflow5 (void)
{
  mpfr_t x, y, z, r1, r2;
  mpfr_exp_t emax;
  int inex1, inex2;
  int i, rnd;
  unsigned int neg, negr;

  emax = mpfr_get_emax ();

  mpfr_init2 (x, 123);
  mpfr_init2 (y, 45);
  mpfr_init2 (z, 67);
  mpfr_inits2 (89, r1, r2, (mpfr_ptr) 0);

  mpfr_set_ui_2exp (x, 1, emax - 1, MPFR_RNDN);

  for (i = 3; i <= 17; i++)
    {
      mpfr_set_ui (y, i, MPFR_RNDN);
      mpfr_set_ui_2exp (z, 1, emax - 1, MPFR_RNDN);
      for (neg = 0; neg < 8; neg++)
        {
          mpfr_setsign (x, x, neg & 1, MPFR_RNDN);
          mpfr_setsign (y, y, neg & 2, MPFR_RNDN);
          mpfr_setsign (z, z, neg & 4, MPFR_RNDN);

          /* |x*y + z| = (i +/- 1) * 2^(emax - 1) >= 2^emax (overflow)
             and x*y + z has the same sign as x*y. */
          negr = (neg ^ (neg >> 1)) & 1;

          RND_LOOP (rnd)
            {
              mpfr_set_inf (r1, 1);
              if (MPFR_IS_LIKE_RNDZ ((mpfr_rnd_t) rnd, negr))
                {
                  mpfr_nextbelow (r1);
                  inex1 = -1;
                }
              else
                inex1 = 1;

              if (negr)
                {
                  mpfr_neg (r1, r1, MPFR_RNDN);
                  inex1 = - inex1;
                }

              mpfr_clear_flags ();
              inex2 = mpfr_fma (r2, x, y, z, (mpfr_rnd_t) rnd);
              MPFR_ASSERTN (__gmpfr_flags ==
                            (MPFR_FLAGS_INEXACT | MPFR_FLAGS_OVERFLOW));

              if (! (mpfr_equal_p (r1, r2) && SAME_SIGN (inex1, inex2)))
                {
                  printf ("Error in test_overflow5 for i=%d neg=%u %s\n",
                          i, neg, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                  printf ("Expected ");
                  mpfr_dump (r1);
                  printf ("with inex = %d\n", inex1);
                  printf ("Got      ");
                  mpfr_dump (r2);
                  printf ("with inex = %d\n", inex2);
                  exit (1);
                }
            }  /* rnd */
        }  /* neg */
    }  /* i */

  mpfr_clears (x, y, z, r1, r2, (mpfr_ptr) 0);
}

static void
test_underflow1 (void)
{
  mpfr_t x, y, z, r;
  int inex, signy, signz, rnd, err = 0;

  mpfr_inits2 (8, x, y, z, r, (mpfr_ptr) 0);

  MPFR_SET_POS (x);
  mpfr_setmin (x, mpfr_get_emin ());  /* x = 0.1@emin */

  for (signy = -1; signy <= 1; signy += 2)
    {
      mpfr_set_si_2exp (y, signy, -1, MPFR_RNDN);  /* |y| = 1/2 */
      for (signz = -3; signz <= 3; signz += 2)
        {
          RND_LOOP (rnd)
            {
              mpfr_set_si (z, signz, MPFR_RNDN);
              if (ABS (signz) != 1)
                mpfr_setmax (z, mpfr_get_emax ());
              /* |z| = 1 or 2^emax - ulp */
              mpfr_clear_flags ();
              inex = mpfr_fma (r, x, y, z, (mpfr_rnd_t) rnd);
#define STRTU1 "Error in test_underflow1 (signy = %d, signz = %d, %s)\n  "
              if (mpfr_nanflag_p ())
                {
                  printf (STRTU1 "NaN flag is set\n", signy, signz,
                          mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                  err = 1;
                }
              if (signy < 0 && MPFR_IS_LIKE_RNDD(rnd, signz))
                mpfr_nextbelow (z);
              if (signy > 0 && MPFR_IS_LIKE_RNDU(rnd, signz))
                mpfr_nextabove (z);
              if ((mpfr_overflow_p () != 0) ^ (mpfr_inf_p (z) != 0))
                {
                  printf (STRTU1 "wrong overflow flag\n", signy, signz,
                          mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                  err = 1;
                }
              if (mpfr_underflow_p ())
                {
                  printf (STRTU1 "underflow flag is set\n", signy, signz,
                          mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                  err = 1;
                }
              if (! mpfr_equal_p (r, z))
                {
                  printf (STRTU1 "got        ", signy, signz,
                          mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                  mpfr_dump (r);
                  printf ("  instead of ");
                  mpfr_dump (z);
                  err = 1;
                }
              if (inex >= 0 && (rnd == MPFR_RNDD ||
                                (rnd == MPFR_RNDZ && signz > 0) ||
                                (rnd == MPFR_RNDN && signy > 0)))
                {
                  printf (STRTU1 "ternary value = %d instead of < 0\n",
                          signy, signz, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd),
                          inex);
                  err = 1;
                }
              if (inex <= 0 && (rnd == MPFR_RNDU ||
                                (rnd == MPFR_RNDZ && signz < 0) ||
                                (rnd == MPFR_RNDN && signy < 0)))
                {
                  printf (STRTU1 "ternary value = %d instead of > 0\n",
                          signy, signz, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd),
                          inex);
                  err = 1;
                }
            }
        }
    }

  if (err)
    exit (1);
  mpfr_clears (x, y, z, r, (mpfr_ptr) 0);
}

static void
test_underflow2 (void)
{
  mpfr_t x, y, z, r1, r2;
  int e, b, i, prec = 32, pz, inex;
  unsigned int neg;

  mpfr_init2 (x, MPFR_PREC_MIN);
  mpfr_inits2 (prec, y, z, r1, r2, (mpfr_ptr) 0);

  mpfr_set_si_2exp (x, 1, mpfr_get_emin () - 1, MPFR_RNDN);
  /* x = 2^(emin-1) */

  for (e = -1; e <= prec + 2; e++)
    {
      mpfr_set (z, x, MPFR_RNDN);
      /* z = x = 2^(emin+e) */
      for (b = 0; b <= 1; b++)
        {
          for (pz = prec - 4 * (b == 0); pz <= prec + 4; pz++)
            {
              inex = mpfr_prec_round (z, pz, MPFR_RNDZ);
              MPFR_ASSERTN (inex == 0);
              for (i = 15; i <= 17; i++)
                {
                  mpfr_flags_t flags1, flags2;
                  int inex1, inex2;

                  mpfr_set_si_2exp (y, i, -4 - prec, MPFR_RNDN);

                  /*      <--- r --->
                   *  z = 1.000...00b   with b = 0 or 1
                   * xy =            01111  (i = 15)
                   *   or            10000  (i = 16)
                   *   or            10001  (i = 17)
                   *
                   * x, y, and z will be modified to test the different sign
                   * combinations. In the case b = 0 (i.e. |z| is a power of
                   * 2) and x*y has a different sign from z, then y will be
                   * divided by 2, so that i = 16 corresponds to a midpoint.
                   */

                  for (neg = 0; neg < 8; neg++)
                    {
                      int xyneg, prev_binade;

                      mpfr_setsign (x, x, neg & 1, MPFR_RNDN);
                      mpfr_setsign (y, y, neg & 2, MPFR_RNDN);
                      mpfr_setsign (z, z, neg & 4, MPFR_RNDN);

                      xyneg = (neg ^ (neg >> 1)) & 1;  /* true iff x*y < 0 */

                      /* If a change of binade occurs by adding x*y to z
                         exactly, then take into account the fact that the
                         midpoint has a different exponent. */
                      prev_binade = b == 0 && (xyneg ^ MPFR_IS_NEG (z));
                      if (prev_binade)
                        mpfr_div_2ui (y, y, 1, MPFR_RNDN);

                      mpfr_set (r1, z, MPFR_RNDN);
                      flags1 = MPFR_FLAGS_INEXACT;

                      if (e == -1 && i == 17 && b == 0 &&
                          (xyneg ^ (neg >> 2)) != 0)
                        {
                          /* Special underflow case. */
                          flags1 |= MPFR_FLAGS_UNDERFLOW;
                          inex1 = xyneg ? 1 : -1;
                        }
                      else if (i == 15 || (i == 16 && b == 0))
                        {
                          /* round toward z */
                          inex1 = xyneg ? 1 : -1;
                        }
                      else if (xyneg)
                        {
                          /* round away from z, with x*y < 0 */
                          mpfr_nextbelow (r1);
                          inex1 = -1;
                        }
                      else
                        {
                          /* round away from z, with x*y > 0 */
                          mpfr_nextabove (r1);
                          inex1 = 1;
                        }

                      mpfr_clear_flags ();
                      inex2 = mpfr_fma (r2, x, y, z, MPFR_RNDN);
                      flags2 = __gmpfr_flags;

                      if (! (mpfr_equal_p (r1, r2) &&
                             SAME_SIGN (inex1, inex2) &&
                             flags1 == flags2))
                        {
                          printf ("Error in test_underflow2 for "
                                  "e=%d b=%d pz=%d i=%d neg=%u\n",
                                  e, b, pz, i, neg);
                          printf ("Expected ");
                          mpfr_dump (r1);
                          printf ("with inex = %d and flags:", inex1);
                          flags_out (flags1);
                          printf ("Got      ");
                          mpfr_dump (r2);
                          printf ("with inex = %d and flags:", inex2);
                          flags_out (flags2);
                          exit (1);
                        }

                      /* Restore y. */
                      if (prev_binade)
                        mpfr_mul_2ui (y, y, 1, MPFR_RNDN);
                    }  /* neg */
                }  /* i */
            }  /* pz */

          inex = mpfr_prec_round (z, prec, MPFR_RNDZ);
          MPFR_ASSERTN (inex == 0);
          MPFR_SET_POS (z);
          mpfr_nextabove (z);
        }  /* b */
      mpfr_mul_2ui (x, x, 1, MPFR_RNDN);
    }  /* e */

  mpfr_clears (x, y, z, r1, r2, (mpfr_ptr) 0);
}

static void
test_underflow3 (int n)
{
  mpfr_t x, y, z, t1, t2;
  int sign, k, s, rnd, inex1, inex2;
  mpfr_exp_t e;
  mpfr_flags_t flags1, flags2;

  mpfr_inits2 (4, x, z, t1, t2, (mpfr_ptr) 0);
  mpfr_init2 (y, 6);

  e = mpfr_get_emin () - 1;

  for (sign = 1; sign >= -1; sign -= 2)
    for (k = 1; k <= 7; k++)
      for (s = -1; s <= 1; s++)
        {
          mpfr_set_si_2exp (x, sign, e, MPFR_RNDN);
          mpfr_set_si_2exp (y, 8*k+s, -6, MPFR_RNDN);
          mpfr_neg (z, x, MPFR_RNDN);
          /* x = sign * 2^(emin-1)
             y = (8 * k + s) * 2^(-6) = k / 8 + s * 2^(-6)
             z = -x = -sign * 2^(emin-1)
             FMA(x,y,z) = sign * ((k-8) * 2^(emin-4) + s * 2^(emin-7)) exactly.
             Note: The purpose of the s * 2^(emin-7) term is to yield
             double rounding when scaling for k = 4, s != 0, MPFR_RNDN. */

          RND_LOOP_NO_RNDF (rnd)
            {
              mpfr_clear_flags ();
              inex1 = mpfr_set_si_2exp (t1, sign * (8*k+s-64), e-6,
                                        (mpfr_rnd_t) rnd);
              flags1 = __gmpfr_flags;

              mpfr_clear_flags ();
              inex2 = mpfr_fma (t2, x, y, z, (mpfr_rnd_t) rnd);
              flags2 = __gmpfr_flags;

              if (! (mpfr_equal_p (t1, t2) &&
                     SAME_SIGN (inex1, inex2) &&
                     flags1 == flags2))
                {
                  printf ("Error in test_underflow3, n = %d, sign = %d,"
                          " k = %d, s = %d, %s\n", n, sign, k, s,
                          mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                  printf ("Expected ");
                  mpfr_dump (t1);
                  printf ("  with inex ~ %d, flags =", inex1);
                  flags_out (flags1);
                  printf ("Got      ");
                  mpfr_dump (t2);
                  printf ("  with inex ~ %d, flags =", inex2);
                  flags_out (flags2);
                  exit (1);
                }
            }
        }

  mpfr_clears (x, y, z, t1, t2, (mpfr_ptr) 0);
}

/* Test s = x*y + z with PREC(z) > PREC(s) + 1, x*y underflows, where
   z + x*y and z + sign(x*y) * 2^(emin-1) do not give the same result.
     x = 2^emin
     y = 2^(-8)
     z = 2^emin * (2^PREC(s) + k - 2^(-1))
   with k = 3 for MPFR_RNDN and k = 2 for the directed rounding modes.
   Also test the opposite versions with neg != 0.
*/
static void
test_underflow4 (void)
{
  mpfr_t x, y, z, s1, s2;
  mpfr_prec_t ps = 32;
  int inex, rnd;

  mpfr_inits2 (MPFR_PREC_MIN, x, y, (mpfr_ptr) 0);
  mpfr_inits2 (ps, s1, s2, (mpfr_ptr) 0);
  mpfr_init2 (z, ps + 2);

  inex = mpfr_set_si_2exp (x, 1, mpfr_get_emin (), MPFR_RNDN);
  MPFR_ASSERTN (inex == 0);
  inex = mpfr_set_si_2exp (y, 1, -8, MPFR_RNDN);
  MPFR_ASSERTN (inex == 0);

  RND_LOOP_NO_RNDF (rnd)
    {
      mpfr_flags_t flags1, flags2;
      int inex1, inex2;
      unsigned int neg;

      inex = mpfr_set_si_2exp (z, 1 << 1, ps, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      inex = mpfr_sub_ui (z, z, 1, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      inex = mpfr_div_2ui (z, z, 1, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      inex = mpfr_add_ui (z, z, rnd == MPFR_RNDN ? 3 : 2, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      inex = mpfr_mul (z, z, x, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);

      for (neg = 0; neg <= 3; neg++)
        {
          inex1 = mpfr_set (s1, z, (mpfr_rnd_t) rnd);
          flags1 = MPFR_FLAGS_INEXACT;

          mpfr_clear_flags ();
          inex2 = mpfr_fma (s2, x, y, z, (mpfr_rnd_t) rnd);
          flags2 = __gmpfr_flags;

          if (! (mpfr_equal_p (s1, s2) &&
                 SAME_SIGN (inex1, inex2) &&
                 flags1 == flags2))
            {
              printf ("Error in test_underflow4 for neg=%u %s\n",
                      neg, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
              printf ("Expected ");
              mpfr_dump (s1);
              printf ("  with inex ~ %d, flags =", inex1);
              flags_out (flags1);
              printf ("Got      ");
              mpfr_dump (s2);
              printf ("  with inex ~ %d, flags =", inex2);
              flags_out (flags2);
              exit (1);
            }

          if (neg == 0 || neg == 2)
            mpfr_neg (x, x, MPFR_RNDN);
          if (neg == 1 || neg == 3)
            mpfr_neg (y, y, MPFR_RNDN);
          mpfr_neg (z, z, MPFR_RNDN);
        }
    }

  mpfr_clears (x, y, z, s1, s2, (mpfr_ptr) 0);
}

/* Test s = x*y + z on:
     x = +/- 2^emin
     y = +/- 2^(-3)
     z = +/- 2^(emin + PREC(s)) and MPFR numbers close to this value.
   with PREC(z) from PREC(s) - 2 to PREC(s) + 8.
*/
static void
test_underflow5 (void)
{
  mpfr_t w, x, y, z, s1, s2, t;
  mpfr_exp_t emin;
  mpfr_prec_t ps = 32;
  int inex, i, j, rnd;
  unsigned int neg;

  mpfr_inits2 (MPFR_PREC_MIN, w, x, y, (mpfr_ptr) 0);
  mpfr_inits2 (ps, s1, s2, (mpfr_ptr) 0);
  mpfr_init2 (t, ps + 9);

  emin = mpfr_get_emin ();

  inex = mpfr_set_si_2exp (w, 1, emin, MPFR_RNDN);  /* w = 2^emin */
  MPFR_ASSERTN (inex == 0);
  inex = mpfr_set_si (x, 1, MPFR_RNDN);
  MPFR_ASSERTN (inex == 0);
  inex = mpfr_set_si_2exp (y, 1, -3, MPFR_RNDN);
  MPFR_ASSERTN (inex == 0);

  for (i = -2; i <= 8; i++)
    {
      mpfr_init2 (z, ps + i);
      inex = mpfr_set_si_2exp (z, 1, ps, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);

      for (j = 1; j <= 32; j++)
        mpfr_nextbelow (z);

      for (j = -32; j <= 32; j++)
        {
          for (neg = 0; neg < 8; neg++)
            {
              mpfr_setsign (x, x, neg & 1, MPFR_RNDN);
              mpfr_setsign (y, y, neg & 2, MPFR_RNDN);
              mpfr_setsign (z, z, neg & 4, MPFR_RNDN);

              inex = mpfr_fma (t, x, y, z, MPFR_RNDN);
              MPFR_ASSERTN (inex == 0);

              inex = mpfr_mul (x, x, w, MPFR_RNDN);
              MPFR_ASSERTN (inex == 0);
              inex = mpfr_mul (z, z, w, MPFR_RNDN);
              MPFR_ASSERTN (inex == 0);

              RND_LOOP_NO_RNDF (rnd)
                {
                  mpfr_flags_t flags1, flags2;
                  int inex1, inex2;

                  mpfr_clear_flags ();
                  inex1 = mpfr_mul (s1, w, t, (mpfr_rnd_t) rnd);
                  flags1 = __gmpfr_flags;

                  mpfr_clear_flags ();
                  inex2 = mpfr_fma (s2, x, y, z, (mpfr_rnd_t) rnd);
                  flags2 = __gmpfr_flags;

                  if (! (mpfr_equal_p (s1, s2) &&
                         SAME_SIGN (inex1, inex2) &&
                         flags1 == flags2))
                    {
                      printf ("Error in test_underflow5 on "
                              "i=%d j=%d neg=%u %s\n", i, j, neg,
                              mpfr_print_rnd_mode ((mpfr_rnd_t) rnd));
                      printf ("Expected ");
                      mpfr_dump (s1);
                      printf ("  with inex ~ %d, flags =", inex1);
                      flags_out (flags1);
                      printf ("Got      ");
                      mpfr_dump (s2);
                      printf ("  with inex ~ %d, flags =", inex2);
                      flags_out (flags2);
                      exit (1);
                    }
                }  /* rnd */

              inex = mpfr_div (x, x, w, MPFR_RNDN);
              MPFR_ASSERTN (inex == 0);
              inex = mpfr_div (z, z, w, MPFR_RNDN);
              MPFR_ASSERTN (inex == 0);
            }  /* neg */

          MPFR_SET_POS (z);  /* restore the value before the loop on neg */
          mpfr_nextabove (z);
        }  /* j */

      mpfr_clear (z);
    }  /* i */

  mpfr_clears (w, x, y, s1, s2, t, (mpfr_ptr) 0);
}

static void
bug20101018 (void)
{
  mpfr_t x, y, z, t, u;
  int i;

  mpfr_init2 (x, 64);
  mpfr_init2 (y, 64);
  mpfr_init2 (z, 64);
  mpfr_init2 (t, 64);
  mpfr_init2 (u, 64);

  mpfr_set_str (x, "0xf.fffffffffffffffp-14766", 16, MPFR_RNDN);
  mpfr_set_str (y, "-0xf.fffffffffffffffp+317", 16, MPFR_RNDN);
  mpfr_set_str (z, "0x8.3ffffffffffe3ffp-14443", 16, MPFR_RNDN);
  mpfr_set_str (t, "0x8.7ffffffffffc7ffp-14444", 16, MPFR_RNDN);
  i = mpfr_fma (u, x, y, z, MPFR_RNDN);
  if (! mpfr_equal_p (u, t))
    {
      printf ("Wrong result in bug20101018 (a)\n");
      printf ("Expected ");
      mpfr_out_str (stdout, 16, 0, t, MPFR_RNDN);
      printf ("\nGot      ");
      mpfr_out_str (stdout, 16, 0, u, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  if (i <= 0)
    {
      printf ("Wrong ternary value in bug20101018 (a)\n");
      printf ("Expected > 0\n");
      printf ("Got      %d\n", i);
      exit (1);
    }

  mpfr_set_str (x, "-0xf.fffffffffffffffp-11420", 16, MPFR_RNDN);
  mpfr_set_str (y, "0xf.fffffffffffffffp+9863", 16, MPFR_RNDN);
  mpfr_set_str (z, "0x8.fffff80ffffffffp-1551", 16, MPFR_RNDN);
  mpfr_set_str (t, "0x9.fffff01ffffffffp-1552", 16, MPFR_RNDN);
  i = mpfr_fma (u, x, y, z, MPFR_RNDN);
  if (! mpfr_equal_p (u, t))
    {
      printf ("Wrong result in bug20101018 (b)\n");
      printf ("Expected ");
      mpfr_out_str (stdout, 16, 0, t, MPFR_RNDN);
      printf ("\nGot      ");
      mpfr_out_str (stdout, 16, 0, u, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  if (i <= 0)
    {
      printf ("Wrong ternary value in bug20101018 (b)\n");
      printf ("Expected > 0\n");
      printf ("Got      %d\n", i);
      exit (1);
    }

  mpfr_set_str (x, "0xf.fffffffffffffffp-2125", 16, MPFR_RNDN);
  mpfr_set_str (y, "-0xf.fffffffffffffffp-6000", 16, MPFR_RNDN);
  mpfr_set_str (z, "0x8p-8119", 16, MPFR_RNDN);
  mpfr_set_str (t, "0x8.000000000000001p-8120", 16, MPFR_RNDN);
  i = mpfr_fma (u, x, y, z, MPFR_RNDN);
  if (! mpfr_equal_p (u, t))
    {
      printf ("Wrong result in bug20101018 (c)\n");
      printf ("Expected ");
      mpfr_out_str (stdout, 16, 0, t, MPFR_RNDN);
      printf ("\nGot      ");
      mpfr_out_str (stdout, 16, 0, u, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  if (i <= 0)
    {
      printf ("Wrong ternary value in bug20101018 (c)\n");
      printf ("Expected > 0\n");
      printf ("Got      %d\n", i);
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (t);
  mpfr_clear (u);
}

/* bug found with GMP_CHECK_RANDOMIZE=1514407719 */
static void
bug20171219 (void)
{
  mpfr_t x, y, z, t, u;
  int i;

  mpfr_init2 (x, 60);
  mpfr_init2 (y, 60);
  mpfr_init2 (z, 60);
  mpfr_init2 (t, 68);
  mpfr_init2 (u, 68);

  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_set_ui (y, 1, MPFR_RNDN);
  mpfr_set_ui (z, 1, MPFR_RNDN);
  mpfr_set_ui (t, 2, MPFR_RNDN);
  i = mpfr_fma (u, x, y, z, MPFR_RNDA);
  if (! mpfr_equal_p (u, t))
    {
      printf ("Wrong result in bug20171219 (a)\n");
      printf ("Expected ");
      mpfr_out_str (stdout, 16, 0, t, MPFR_RNDN);
      printf ("\nGot      ");
      mpfr_out_str (stdout, 16, 0, u, MPFR_RNDN);
      printf ("\n");
      exit (1);
    }
  if (i != 0)
    {
      printf ("Wrong ternary value in bug20171219\n");
      printf ("Expected 0\n");
      printf ("Got      %d\n", i);
      exit (1);
    }

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (t);
  mpfr_clear (u);
}

/* coverage test for mpfr_set_1_2, case prec < GMP_NUMB_BITS,
   inex > 0, rb <> 0, sb = 0 */
static void
coverage (void)
{
  mpfr_t x, y, z, s;
  int inex;
  mpfr_exp_t emax;

  mpfr_inits2 (GMP_NUMB_BITS - 1, x, y, z, s, (mpfr_ptr) 0);

  /* set x to 2^(GMP_NUMB_BITS/2) + 1, y to 2^(GMP_NUMB_BITS/2) - 1 */
  MPFR_ASSERTN((GMP_NUMB_BITS % 2) == 0);
  mpfr_set_ui_2exp (x, 1, GMP_NUMB_BITS / 2, MPFR_RNDN);
  mpfr_sub_ui (y, x, 1, MPFR_RNDN);
  mpfr_add_ui (x, x, 1, MPFR_RNDN);
  /* we have x*y = 2^GMP_NUMB_BITS - 1, thus has exponent GMP_NUMB_BITS */
  /* set z to 2^(1-GMP_NUMB_BITS), with exponent 2-GMP_NUMB_BITS */
  mpfr_set_ui_2exp (z, 1, 1 - GMP_NUMB_BITS, MPFR_RNDN);
  inex = mpfr_fms (s, x, y, z, MPFR_RNDN);
  /* s should be 2^GMP_NUMB_BITS-2 */
  MPFR_ASSERTN(inex < 0);
  inex = mpfr_add_ui (s, s, 2, MPFR_RNDN);
  MPFR_ASSERTN(mpfr_cmp_ui_2exp (s, 1, GMP_NUMB_BITS) == 0);

  /* same example, but with overflow */
  emax = mpfr_get_emax ();
  set_emax (GMP_NUMB_BITS + 1);
  mpfr_set_ui_2exp (z, 1, mpfr_get_emax () - 1, MPFR_RNDZ);
  mpfr_clear_flags ();
  inex = mpfr_fma (s, x, y, z, MPFR_RNDN);
  /* x*y = 2^GMP_NUMB_BITS - 1, z = 2^GMP_NUMB_BITS, thus
     x*y+z = 2^(GMP_NUMB_BITS+1) - 1 should round to 2^(GMP_NUMB_BITS+1),
     thus give an overflow */
  MPFR_ASSERTN(inex > 0);
  MPFR_ASSERTN(mpfr_inf_p (s) && mpfr_sgn (s) > 0);
  MPFR_ASSERTN(mpfr_overflow_p ());
  set_emax (emax);

  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (s);
}

int
main (int argc, char *argv[])
{
  mpfr_t x, y, z, s;
  mpfr_exp_t emin, emax;
  int i;

  tests_start_mpfr ();

  coverage ();

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  bug20171219 ();
  bug20101018 ();

  mpfr_init (x);
  mpfr_init (s);
  mpfr_init (y);
  mpfr_init (z);

  /* check special cases */
  mpfr_set_prec (x, 2);
  mpfr_set_prec (y, 2);
  mpfr_set_prec (z, 2);
  mpfr_set_prec (s, 2);
  mpfr_set_str (x, "-0.75", 10, MPFR_RNDN);
  mpfr_set_str (y, "0.5", 10, MPFR_RNDN);
  mpfr_set_str (z, "0.375", 10, MPFR_RNDN);
  mpfr_fma (s, x, y, z, MPFR_RNDU); /* result is 0 */
  if (mpfr_cmp_ui (s, 0))
    {
      printf ("Error: -0.75 * 0.5 + 0.375 should be equal to 0 for prec=2\n");
      printf ("got instead ");
      mpfr_dump (s);
      exit (1);
    }

  mpfr_set_prec (x, 27);
  mpfr_set_prec (y, 27);
  mpfr_set_prec (z, 27);
  mpfr_set_prec (s, 27);
  mpfr_set_str_binary (x, "1.11111111111111111111111111e-1");
  mpfr_set (y, x, MPFR_RNDN);
  mpfr_set_str_binary (z, "-1.00011110100011001011001001e-1");
  if (mpfr_fma (s, x, y, z, MPFR_RNDN) >= 0)
    {
      printf ("Wrong inexact flag for x=y=1-2^(-27)\n");
      exit (1);
    }

  mpfr_set_nan (x);
  mpfr_urandomb (y, RANDS);
  mpfr_urandomb (z, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_nan_p (s))
    {
      printf ("evaluation of function in x=NAN does not return NAN\n");
      exit (1);
    }

  mpfr_set_nan (y);
  mpfr_urandomb (x, RANDS);
  mpfr_urandomb (z, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_nan_p(s))
    {
      printf ("evaluation of function in y=NAN does not return NAN\n");
      exit (1);
    }

  mpfr_set_nan (z);
  mpfr_urandomb (y, RANDS);
  mpfr_urandomb (x, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_nan_p (s))
    {
      printf ("evaluation of function in z=NAN does not return NAN\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  mpfr_set_inf (y, 1);
  mpfr_set_inf (z, 1);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_inf_p (s) || mpfr_sgn (s) < 0)
    {
      printf ("Error for (+inf) * (+inf) + (+inf)\n");
      exit (1);
    }

  mpfr_set_inf (x, -1);
  mpfr_set_inf (y, -1);
  mpfr_set_inf (z, 1);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_inf_p (s) || mpfr_sgn (s) < 0)
    {
      printf ("Error for (-inf) * (-inf) + (+inf)\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  mpfr_set_inf (y, -1);
  mpfr_set_inf (z, -1);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_inf_p (s) || mpfr_sgn (s) > 0)
    {
      printf ("Error for (+inf) * (-inf) + (-inf)\n");
      exit (1);
    }

  mpfr_set_inf (x, -1);
  mpfr_set_inf (y, 1);
  mpfr_set_inf (z, -1);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_inf_p (s) || mpfr_sgn (s) > 0)
    {
      printf ("Error for (-inf) * (+inf) + (-inf)\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  mpfr_set_ui (y, 0, MPFR_RNDN);
  mpfr_urandomb (z, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_nan_p (s))
    {
      printf ("evaluation of function in x=INF y=0  does not return NAN\n");
      exit (1);
    }

  mpfr_set_inf (y, 1);
  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_urandomb (z, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_nan_p (s))
    {
      printf ("evaluation of function in x=0 y=INF does not return NAN\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  mpfr_urandomb (y, RANDS); /* always positive */
  mpfr_set_inf (z, -1);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_nan_p (s))
    {
      printf ("evaluation of function in x=INF y>0 z=-INF does not return NAN\n");
      exit (1);
    }

  mpfr_set_inf (y, 1);
  mpfr_urandomb (x, RANDS);
  mpfr_set_inf (z, -1);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_nan_p (s))
    {
      printf ("evaluation of function in x>0 y=INF z=-INF does not return NAN\n");
      exit (1);
    }

  mpfr_set_inf (x, 1);
  do mpfr_urandomb (y, RANDS); while (MPFR_IS_ZERO(y));
  mpfr_urandomb (z, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_inf_p (s) || mpfr_sgn (s) < 0)
    {
      printf ("evaluation of function in x=INF does not return INF\n");
      exit (1);
    }

  mpfr_set_inf (y, 1);
  do mpfr_urandomb (x, RANDS); while (MPFR_IS_ZERO(x));
  mpfr_urandomb (z, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_inf_p (s) || mpfr_sgn (s) < 0)
    {
      printf ("evaluation of function at y=INF does not return INF\n");
      exit (1);
    }

  mpfr_set_inf (z, 1);
  mpfr_urandomb (x, RANDS);
  mpfr_urandomb (y, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (!mpfr_inf_p (s) || mpfr_sgn (s) < 0)
    {
      printf ("evaluation of function in z=INF does not return INF\n");
      exit (1);
    }

  mpfr_set_ui (x, 0, MPFR_RNDN);
  mpfr_urandomb (y, RANDS);
  mpfr_urandomb (z, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (! mpfr_equal_p (s, z))
    {
      printf ("evaluation of function in x=0 does not return z\n");
      exit (1);
    }

  mpfr_set_ui (y, 0, MPFR_RNDN);
  mpfr_urandomb (x, RANDS);
  mpfr_urandomb (z, RANDS);
  mpfr_fma (s, x, y, z, MPFR_RNDN);
  if (! mpfr_equal_p (s, z))
    {
      printf ("evaluation of function in y=0 does not return z\n");
      exit (1);
    }

  {
    mpfr_prec_t prec;
    mpfr_t t, slong;
    mpfr_rnd_t rnd;
    int inexact, compare;
    unsigned int n;

    mpfr_prec_t p0 = 2, p1 = 200;
    unsigned int N = 200;

    mpfr_init (t);
    mpfr_init (slong);

    /* generic test */
    for (prec = p0; prec <= p1; prec++)
      {
        mpfr_set_prec (x, prec);
        mpfr_set_prec (y, prec);
        mpfr_set_prec (z, prec);
        mpfr_set_prec (s, prec);
        mpfr_set_prec (t, prec);

        for (n = 0; n < N; n++)
          {
            mpfr_urandomb (x, RANDS);
            mpfr_urandomb (y, RANDS);
            mpfr_urandomb (z, RANDS);

            if (RAND_BOOL ())
              mpfr_neg (x, x, MPFR_RNDN);
            if (RAND_BOOL ())
              mpfr_neg (y, y, MPFR_RNDN);
            if (RAND_BOOL ())
              mpfr_neg (z, z, MPFR_RNDN);

            rnd = RND_RAND_NO_RNDF ();
            mpfr_set_prec (slong, 2 * prec);
            if (mpfr_mul (slong, x, y, rnd))
              {
                printf ("x*y should be exact\n");
                exit (1);
              }
            compare = mpfr_add (t, slong, z, rnd);
            inexact = mpfr_fma (s, x, y, z, rnd);
            if (! mpfr_equal_p (s, t))
              {
                printf ("results differ for\n");
                printf ("  x=");
                mpfr_dump (x);
                printf ("  y=");
                mpfr_dump (y);
                printf ("  z=");
                mpfr_dump (z);
                printf ("  with prec=%u rnd_mode=%s\n", (unsigned int) prec,
                        mpfr_print_rnd_mode (rnd));
                printf ("got      ");
                mpfr_dump (s);
                printf ("expected ");
                mpfr_dump (t);
                printf ("approx   ");
                mpfr_dump (slong);
                exit (1);
              }
            if (! SAME_SIGN (inexact, compare))
              {
                printf ("Wrong inexact flag for rnd=%s: expected %d, got %d\n",
                        mpfr_print_rnd_mode (rnd), compare, inexact);
                printf (" x="); mpfr_dump (x);
                printf (" y="); mpfr_dump (y);
                printf (" z="); mpfr_dump (z);
                printf (" s="); mpfr_dump (s);
                exit (1);
              }
          }
      }
    mpfr_clear (t);
    mpfr_clear (slong);

  }
  mpfr_clear (x);
  mpfr_clear (y);
  mpfr_clear (z);
  mpfr_clear (s);

  test_exact ();

  for (i = 0; i <= 2; i++)
    {
      if (i == 0)
        {
          /* corresponds to the generic code + mpfr_check_range */
          set_emin (-1024);
          set_emax (1024);
        }
      else if (i == 1)
        {
          set_emin (MPFR_EMIN_MIN);
          set_emax (MPFR_EMAX_MAX);
        }
      else
        {
          MPFR_ASSERTN (i == 2);
          if (emin == MPFR_EMIN_MIN && emax == MPFR_EMAX_MAX)
            break;
          set_emin (emin);
          set_emax (emax);
        }

      test_overflow1 ();
      test_overflow2 ();
      test_overflow3 ();
      test_overflow4 ();
      test_overflow5 ();
      test_underflow1 ();
      test_underflow2 ();
      test_underflow3 (i);
      test_underflow4 ();
      test_underflow5 ();
    }

  tests_end_mpfr ();
  return 0;
}
