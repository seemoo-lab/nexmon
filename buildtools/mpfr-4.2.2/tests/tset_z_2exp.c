/* Test file for mpfr_set_z_2exp.

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

/* generate a random exponent in [__gmpfr_emin, __gmpfr_emax-1] */
static mpfr_exp_t
randexp (void)
{
  mpfr_uexp_t e;

  if (MPFR_EXP_MAX <= MPFR_LIMB_MAX >> 1)
    {
      /* mpfr_uexp_t fits in a limb: we can generate the whole range
         [emin, emax] directly. */
      e = randlimb ();
    }
  else
    {
      mpfr_uexp_t emax = (mpfr_uexp_t) -1;

      e = 0;
      while (emax != 0)
        {
          /* Since mp_limb_t < mpfr_uexp_t, the shift counts are valid.
             Use GMP_NUMB_BITS - 1 instead of GMP_NUMB_BITS to avoid a
             bug in GCC. */
          e = (e << (GMP_NUMB_BITS - 1)) + (randlimb () >> 1);
          emax >>= GMP_NUMB_BITS - 1;
        }
    }
  return (mpfr_exp_t) (e % (__gmpfr_emax - __gmpfr_emin)) + __gmpfr_emin;
}

static void
check0 (void)
{
  mpz_t y;
  mpfr_t x;
  int inexact, r;
  mpfr_exp_t e;

  /* Check for +0 */
  mpfr_init (x);
  mpz_init (y);
  mpz_set_si (y, 0);
  RND_LOOP (r)
    {
      e = randexp ();
      inexact = mpfr_set_z_2exp (x, y, e, (mpfr_rnd_t) r);
      if (!MPFR_IS_ZERO(x) || !MPFR_IS_POS(x) || inexact)
        {
          printf ("mpfr_set_z_2exp(x,0,e) failed for e=");
          if (e < LONG_MIN)
            printf ("(<LONG_MIN)");
          else if (e > LONG_MAX)
            printf ("(>LONG_MAX)");
          else
            printf ("%ld", (long) e);
          printf (", rnd=%s\n", mpfr_print_rnd_mode ((mpfr_rnd_t) r));
          exit (1);
        }
    }

  /* coverage test for huge exponent */
  mpz_setbit (y, GMP_NUMB_BITS);
  mpfr_clear_flags ();
  inexact = mpfr_set_z_2exp (x, y, mpfr_get_emax_max(), MPFR_RNDN);
  MPFR_ASSERTN(inexact > 0);
  MPFR_ASSERTN(mpfr_inf_p (x) && mpfr_sgn (x) > 0);
  MPFR_ASSERTN(mpfr_overflow_p ());
  mpfr_clear(x);
  mpz_clear(y);
}

/* FIXME: It'd be better to examine the actual data in an mpfr_t to see that
   it's as expected.  Comparing mpfr_set_z with mpfr_cmp or against
   mpfr_get_si is a rather indirect test of a low level routine.  */

static void
check (long i, mpfr_rnd_t rnd, int reduced)
{
  mpfr_t f1, f2, f3;
  mpz_t z;
  mpfr_exp_t e, old_emin, old_emax;
  int inex;
  mpfr_flags_t flags;

  old_emin = mpfr_get_emin ();
  old_emax = mpfr_get_emax ();

  /* using CHAR_BIT * sizeof(long) bits of precision ensures that
     mpfr_set_z_2exp is exact below */
  mpfr_inits2 (CHAR_BIT * sizeof(long), f1, f2, f3, (mpfr_ptr) 0);
  mpz_init (z);
  mpz_set_ui (z, i);
  /* the following loop ensures that no overflow occurs */
  do
    e = randexp ();
  while (e > mpfr_get_emax () - CHAR_BIT * sizeof(long));

  mpfr_clear_flags ();
  inex = mpfr_set_z_2exp (f1, z, e, rnd);
  flags = __gmpfr_flags;

  if (inex != 0 || flags != 0 ||
      (mpfr_div_2si (f2, f1, e, rnd), mpfr_get_si (f2, MPFR_RNDZ) != i))
    {
      printf ("Error in mpfr_set_z_2exp for i=%ld e=%" MPFR_EXP_FSPEC
              "d rnd_mode=%d\n", i, (mpfr_eexp_t) e, rnd);
      mpfr_set_si_2exp (f2, i, e, MPFR_RNDN);
      printf ("expected "); mpfr_dump (f2);
      printf ("with inex = %d and flags =", 0);
      flags_out (0);
      printf ("got      "); mpfr_dump (f1);
      printf ("with inex = %d and flags =", inex);
      flags_out (flags);
      exit (1);
    }

  if (reduced)
    {
      mpfr_exp_t ef, emin, emax;
      int inex2, inex3;
      mpfr_flags_t flags2, flags3;

      ef = i == 0 ? 0 : mpfr_get_exp (f1);
      for (emin = ef - 2; emin <= ef + 2; emin++)
        for (emax = emin; emax <= ef + 2; emax++)
          {
            inex3 = mpfr_set (f3, f1, rnd);
            MPFR_ASSERTN (inex3 == 0);
            set_emin (emin);
            set_emax (emax);
            mpfr_clear_flags ();
            inex2 = mpfr_set_z_2exp (f2, z, e, rnd);
            flags2 = __gmpfr_flags;
            mpfr_clear_flags ();
            inex3 = mpfr_check_range (f3, 0, rnd);
            flags3 = __gmpfr_flags;
            if (!(mpfr_equal_p (f2, f3) &&
                  SAME_SIGN (inex2, inex3) &&
                  flags2 == flags3))
              {
                printf ("Error in mpfr_set_z_2exp for i=%ld e=%"
                        MPFR_EXP_FSPEC "d rnd_mode=%d\nand emin=%"
                        MPFR_EXP_FSPEC "d emax=%" MPFR_EXP_FSPEC
                        "d\n", i, (mpfr_eexp_t) e, rnd,
                        (mpfr_eexp_t) emin, (mpfr_eexp_t) emax);
                printf ("expected "); mpfr_dump (f3);
                printf ("with inex = %d and flags =", inex3);
                flags_out (flags3);
                printf ("got      "); mpfr_dump (f2);
                printf ("with inex = %d and flags =", inex2);
                flags_out (flags2);
                exit (1);
              }
          }
      set_emin (old_emin);
      set_emax (old_emax);
    }

  mpfr_clears (f1, f2, f3, (mpfr_ptr) 0);
  mpz_clear (z);
}

static void
check_huge (void)
{
  if (getenv ("MPFR_CHECK_LARGEMEM") != NULL)
    {
      mpfr_t x;
      mpz_t z;
      long e;

      /* Increase tests_memory_limit to the maximum in order to avoid
         an obvious failure due to insufficient memory. */
      tests_memory_limit = (size_t) -1;  /* no memory limit */

      mpfr_init2 (x, 32);

      /* In r14140, with a 32-bit ABI (GCC's -m32):
         - With UBsan (-fsanitize=undefined -fno-sanitize-recover),
           this fails with:
             set_z_2exp.c:71:26: runtime error: signed integer overflow:
             67108864 * 32 cannot be represented in type 'long int'
         - With -D_MPFR_EXP_FORMAT=4, this fails with:
             Expected 0.10001000000000000000000000000000E5
             Got      0
      */
      mpz_init_set_ui (z, 17);
      e = 0x7ffffff0;
      mpz_mul_2exp (z, z, e);
      mpz_add_ui (z, z, 1);
      mpfr_set_z_2exp (x, z, -e, MPFR_RNDN);
      if (mpfr_cmp_ui0 (x, 17) != 0)
        {
          printf ("Error 1 in check_huge\n");
          printf ("Expected 0.10001000000000000000000000000000E5\n");
          printf ("Got      ");
          mpfr_dump (x);
          exit (1);
        }
      mpz_clear (z);

      mpz_init_set_ui (z, 17);
      mpz_mul_2exp (z, z, 0xffffffb0);
      mpz_add_ui (z, z, 1);
      mpfr_set_z_2exp (x, z, -1, MPFR_RNDN);
      if (! MPFR_IS_INF (x) || MPFR_IS_NEG (x))
        {
          printf ("Error 2 in check_huge\n");
          printf ("Expected @Inf@\n");
          printf ("Got      ");
          mpfr_dump (x);
          exit (1);
        }
      mpz_clear (z);

      mpfr_clear (x);
    }
}

int
main (int argc, char *argv[])
{
  long j;

  tests_start_mpfr ();

  check (0, MPFR_RNDN, 0);
  for (j = 0; j < 200000; j++)
    check (randlimb () & LONG_MAX, RND_RAND (), j < 200);
  check0 ();

  check_huge ();

  tests_end_mpfr ();

  return 0;
}
