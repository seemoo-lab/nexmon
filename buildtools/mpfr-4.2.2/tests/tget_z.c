/* Test file for mpz_set_fr / mpfr_get_z / mpfr_get_z_2exp.

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

#include "mpfr-test.h"

static void
check_diff (void)
{
  int inex;
  mpfr_t x;
  mpz_t  z;
  mpfr_exp_t emin;

  mpz_init   (z);
  mpfr_init2 (x, 2);

  mpfr_set_ui (x, 2047, MPFR_RNDU);
  mpz_set_fr (z, x, MPFR_RNDN);
  if (mpz_cmp_ui (z, 2048) != 0)
    {
      printf ("get_z RU 2048 failed\n");
      exit (1);
    }

  mpfr_set_prec (x, 6);
  mpfr_set_str (x, "17.5", 10, MPFR_RNDN);
  inex = mpfr_get_z (z, x, MPFR_RNDN);
  if (inex <= 0 || mpz_cmp_ui (z, 18) != 0)
    {
      printf ("get_z RN 17.5 failed\n");
      exit (1);
    }

  /* save default emin */
  emin = mpfr_get_emin ();;

  set_emin (17);
  mpfr_set_ui (x, 0, MPFR_RNDN);
  inex = mpfr_get_z (z, x, MPFR_RNDN);
  if (inex != 0 || mpz_cmp_ui (z, 0) != 0)
    {
      printf ("get_z 0 failed\n");
      exit (1);
    }

  /* restore default emin */
  set_emin (emin);

  mpfr_clear (x);
  mpz_clear  (z);
}

static void
check_one (mpz_ptr z)
{
  mpfr_exp_t emin, emax;
  int    inex;
  int    sh, neg;
  mpfr_t f;
  mpz_t  got, ex, t;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  mpfr_init2 (f, MAX (mpz_sizeinbase (z, 2), MPFR_PREC_MIN));
  mpz_init (got);
  mpz_init (ex);
  mpz_init (t);

  for (sh = -2*GMP_NUMB_BITS ; sh < 2*GMP_NUMB_BITS ; sh++)
    {
      inex = mpfr_set_z (f, z, MPFR_RNDN);  /* exact */
      MPFR_ASSERTN (inex == 0);

      inex = sh < 0 ?
        mpfr_div_2ui (f, f, -sh, MPFR_RNDN) :
        mpfr_mul_2ui (f, f, sh, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);

      for (neg = 0; neg <= 1; neg++)
        {
          int rnd;

          /* Test (-1)^neg * z * 2^sh */

          RND_LOOP_NO_RNDF (rnd)
            {
              int ex_inex, same;
              int d, fi, e;
              mpfr_flags_t flags[3] = { 0, MPFR_FLAGS_ALL ^ MPFR_FLAGS_ERANGE,
                                        MPFR_FLAGS_ALL }, ex_flags, gt_flags;

              if (neg)
                mpz_neg (ex, z);
              else
                mpz_set (ex, z);

              if (sh < 0)
                switch (rnd)
                  {
                  case MPFR_RNDN:
                    mpz_set_si (t, neg ? -1 : 1);
                    mpz_mul_2exp (t, t, -sh - 1);
                    mpz_add (ex, ex, t);
                    /* d = mpz_divisible_2exp_p (ex, -sh); */
                    d = mpz_scan1 (ex, 0) >= -sh;
                    mpz_tdiv_q_2exp (ex, ex, -sh);
                    if (d && mpz_tstbit (ex, 0) != 0)  /* even rounding */
                      {
                        if (neg)
                          mpz_add_ui (ex, ex, 1);
                        else
                          mpz_sub_ui (ex, ex, 1);
                      }
                    break;
                  case MPFR_RNDZ:
                    mpz_tdiv_q_2exp (ex, ex, -sh);
                    break;
                  case MPFR_RNDU:
                    mpz_cdiv_q_2exp (ex, ex, -sh);
                    break;
                  case MPFR_RNDD:
                    mpz_fdiv_q_2exp (ex, ex, -sh);
                    break;
                  case MPFR_RNDA:
                    if (neg)
                      mpz_fdiv_q_2exp (ex, ex, -sh);
                    else
                      mpz_cdiv_q_2exp (ex, ex, -sh);
                    break;
                  default:
                    MPFR_ASSERTN (0);
                  }
              else
                mpz_mul_2exp (ex, ex, sh);

              ex_inex = - mpfr_cmp_z (f, ex);
              ex_inex = VSIGN (ex_inex);

              for (fi = 0; fi < numberof (flags); fi++)
                for (e = 0; e < 2; e++)
                  {
                    if (e)
                      {
                        mpfr_exp_t ef;

                        if (MPFR_IS_ZERO (f))
                          break;
                        ef = MPFR_GET_EXP (f);
                        set_emin (ef);
                        set_emax (ef);
                      }
                    ex_flags = __gmpfr_flags = flags[fi];
                    if (ex_inex != 0)
                      ex_flags |= MPFR_FLAGS_INEXACT;
                    inex = mpfr_get_z (got, f, (mpfr_rnd_t) rnd);
                    inex = VSIGN (inex);
                    gt_flags = __gmpfr_flags;
                    set_emin (emin);
                    set_emax (emax);
                    same = SAME_SIGN (inex, ex_inex);

                    if (mpz_cmp (got, ex) != 0 ||
                        !same || gt_flags != ex_flags)
                      {
                        printf ("Error in check_one for sh=%d, fi=%d, %s%s\n",
                                sh, fi,
                                mpfr_print_rnd_mode ((mpfr_rnd_t) rnd),
                                e ? ", reduced exponent range" : "");
                        printf ("     f = "); mpfr_dump (f);
                        printf ("expected "); mpz_out_str (stdout, 10, ex);
                        printf ("\n     got "); mpz_out_str (stdout, 10, got);
                        printf ("\nExpected inex ~ %d, got %d (%s)\n",
                                ex_inex, inex, same ? "OK" : "wrong");
                        printf ("Flags:\n");
                        printf ("      in"); flags_out (flags[fi]);
                        printf ("expected"); flags_out (ex_flags);
                        printf ("     got"); flags_out (gt_flags);
                        exit (1);
                      }
                  }
            }

          mpfr_neg (f, f, MPFR_RNDN);
        }
    }

  mpfr_clear (f);
  mpz_clear (got);
  mpz_clear (ex);
  mpz_clear (t);
}

static void
check (void)
{
  mpz_t  z;

  mpz_init (z);

  mpz_set_ui (z, 0L);
  check_one (z);

  mpz_set_si (z, 17L);
  check_one (z);

  mpz_set_si (z, 123L);
  check_one (z);

  mpz_rrandomb (z, RANDS, 2*GMP_NUMB_BITS);
  check_one (z);

  mpz_rrandomb (z, RANDS, 5*GMP_NUMB_BITS);
  check_one (z);

  mpz_clear (z);
}

static void
special (void)
{
  int inex;
  mpfr_t x;
  mpz_t z;
  int i, fi;
  int rnd;
  mpfr_exp_t e;
  mpfr_flags_t flags[3] = { 0, MPFR_FLAGS_ALL ^ MPFR_FLAGS_ERANGE,
                            MPFR_FLAGS_ALL }, ex_flags, gt_flags;

  mpfr_init2 (x, 2);
  mpz_init (z);

  RND_LOOP (rnd)
    for (i = -1; i <= 1; i++)
      for (fi = 0; fi < numberof (flags); fi++)
        {
          ex_flags = flags[fi] | MPFR_FLAGS_ERANGE;
          if (i != 0)
            mpfr_set_nan (x);
          else
            mpfr_set_inf (x, i);
          __gmpfr_flags = flags[fi];
          inex = mpfr_get_z (z, x, (mpfr_rnd_t) rnd);
          gt_flags = __gmpfr_flags;
          if (gt_flags != ex_flags || inex != 0 || mpz_cmp_ui (z, 0) != 0)
            {
              printf ("special() failed on mpfr_get_z"
                      " for %s, i = %d, fi = %d\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), i, fi);
              printf ("Expected z = 0, inex = 0,");
              flags_out (ex_flags);
              printf ("Got      z = ");
              mpz_out_str (stdout, 10, z);
              printf (", inex = %d,", inex);
              flags_out (gt_flags);
              exit (1);
            }
          __gmpfr_flags = flags[fi];
          e = mpfr_get_z_2exp (z, x);
          gt_flags = __gmpfr_flags;
          if (gt_flags != ex_flags || e != __gmpfr_emin ||
              mpz_cmp_ui (z, 0) != 0)
            {
              printf ("special() failed on mpfr_get_z_2exp"
                      " for %s, i = %d, fi = %d\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), i, fi);
              printf ("Expected z = 0, e = %" MPFR_EXP_FSPEC "d,",
                      (mpfr_eexp_t) __gmpfr_emin);
              flags_out (ex_flags);
              printf ("Got      z = ");
              mpz_out_str (stdout, 10, z);
              printf (", e = %" MPFR_EXP_FSPEC "d,", (mpfr_eexp_t) e);
              flags_out (gt_flags);
              exit (1);
            }
        }

  mpfr_clear (x);
  mpz_clear (z);
}

int
main (void)
{
  tests_start_mpfr ();

  check ();
  check_diff ();
  special ();

  tests_end_mpfr ();
  return 0;
}
