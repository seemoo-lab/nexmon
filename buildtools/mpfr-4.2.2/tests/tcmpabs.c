/* Test file for mpfr_cmpabs and mpfr_cmpabs_ui.

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

#define PRINT_ERROR(s) do { printf ("Error: %s\n", s); exit (1); } while (0)

static int
cmpabs (mpfr_srcptr x, mpfr_srcptr y)
{
  unsigned int i;
  int r[4];
  mpfr_flags_t f1, f2, flags[2] = { 0, MPFR_FLAGS_ALL };
  mpfr_exp_t emin, emax;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  for (i = 0; i < 4; i++)
    {
      if (i & 2)
        {
          mpfr_exp_t ex = MPFR_IS_SINGULAR (x) ? emax : MPFR_GET_EXP (x);
          mpfr_exp_t ey = MPFR_IS_SINGULAR (y) ? emax : MPFR_GET_EXP (y);
          set_emin (ex < ey ? ex : ey);
          set_emax (ex < ey ? ey : ex);
        }

      __gmpfr_flags = f1 = flags[i % 2];
      r[i] = mpfr_cmpabs (x, y);
      f2 = __gmpfr_flags;
      if (MPFR_IS_NAN (x) || MPFR_IS_NAN (y))
        f1 |= MPFR_FLAGS_ERANGE;

      if (i & 2)
        {
          set_emin (emin);
          set_emax (emax);
        }

      if (f1 != f2)
        {
          printf ("Flags error in mpfr_cmpabs for i = %u\n  x = ", i);
          mpfr_dump (x);
          printf ("  y = ");
          mpfr_dump (y);
          printf ("Expected flags = ");
          flags_out (f1);
          printf ("Obtained flags = ");
          flags_out (f2);
          exit (1);
        }

      if (i > 0)
        MPFR_ASSERTN (r[i] == r[0]);
    }

  return r[0];
}

static void
test_cmpabs (void)
{
  mpfr_t xx, yy;

  mpfr_init2 (xx, 2);
  mpfr_init2 (yy, 2);

  mpfr_clear_erangeflag ();
  MPFR_SET_NAN (xx);
  MPFR_SET_NAN (yy);
  if (cmpabs (xx, yy) != 0)
    PRINT_ERROR ("mpfr_cmpabs (NAN,NAN) returns non-zero");
  if (!mpfr_erangeflag_p ())
    PRINT_ERROR ("mpfr_cmpabs (NAN,NAN) doesn't set erange flag");

  mpfr_set_si (yy, -1, MPFR_RNDN);
  if (cmpabs (xx, yy) != 0)
    PRINT_ERROR ("mpfr_cmpabs (NAN,-1) returns non-zero");
  if (cmpabs (yy, xx) != 0)
    PRINT_ERROR ("mpfr_cmpabs (-1,NAN) returns non-zero");

  mpfr_set_str_binary (xx, "0.10E0");
  mpfr_set_str_binary (yy, "-0.10E0");
  if (cmpabs (xx, yy) != 0)
    PRINT_ERROR ("mpfr_cmpabs (xx, yy) returns non-zero for prec=2");

  mpfr_set_prec (xx, 65);
  mpfr_set_prec (yy, 65);
  mpfr_set_str_binary (xx, "-0.10011010101000110101010000000011001001001110001011101011111011101E623");
  mpfr_set_str_binary (yy, "0.10011010101000110101010000000011001001001110001011101011111011100E623");
  if (cmpabs (xx, yy) <= 0)
    PRINT_ERROR ("Error (1) in mpfr_cmpabs");

  mpfr_set_str_binary (xx, "-0.10100010001110110111000010001000010011111101000100011101000011100");
  mpfr_set_str_binary (yy, "-0.10100010001110110111000010001000010011111101000100011101000011011");
  if (cmpabs (xx, yy) <= 0)
    PRINT_ERROR ("Error (2) in mpfr_cmpabs");

  mpfr_set_prec (xx, 160);
  mpfr_set_prec (yy, 160);
  mpfr_set_str_binary (xx, "0.1E1");
  mpfr_set_str_binary (yy, "-0.1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111100000110001110100");
  if (cmpabs (xx, yy) <= 0)
    PRINT_ERROR ("Error (3) in mpfr_cmpabs");

  mpfr_set_prec(xx, 53);
  mpfr_set_prec(yy, 200);
  mpfr_set_ui (xx, 1, (mpfr_rnd_t) 0);
  mpfr_set_ui (yy, 1, (mpfr_rnd_t) 0);
  if (cmpabs (xx, yy) != 0)
    PRINT_ERROR ("Error in mpfr_cmpabs: 1.0 != 1.0");

  mpfr_set_prec (yy, 31);
  mpfr_set_str (xx, "-1.0000000002", 10, (mpfr_rnd_t) 0);
  mpfr_set_ui (yy, 1, (mpfr_rnd_t) 0);
  if (cmpabs (xx, yy) <= 0)
    PRINT_ERROR ("Error in mpfr_cmpabs: not 1.0000000002 > 1.0");
  mpfr_set_prec(yy, 53);

  mpfr_set_ui(xx, 0, MPFR_RNDN);
  mpfr_set_str (yy, "-0.1", 10, MPFR_RNDN);
  if (cmpabs (xx, yy) >= 0)
    PRINT_ERROR ("Error in mpfr_cmpabs(0.0, 0.1)");

  mpfr_set_inf (xx, -1);
  mpfr_set_str (yy, "23489745.0329", 10, MPFR_RNDN);
  if (cmpabs (xx, yy) <= 0)
    PRINT_ERROR ("Error in mpfr_cmpabs(-Inf, 23489745.0329)");

  mpfr_set_inf (xx, 1);
  mpfr_set_inf (yy, -1);
  if (cmpabs (xx, yy) != 0)
    PRINT_ERROR ("Error in mpfr_cmpabs(Inf, -Inf)");

  mpfr_set_inf (yy, -1);
  mpfr_set_str (xx, "2346.09234", 10, MPFR_RNDN);
  if (cmpabs (xx, yy) >= 0)
    PRINT_ERROR ("Error in mpfr_cmpabs(-Inf, 2346.09234)");

  mpfr_set_prec (xx, 2);
  mpfr_set_prec (yy, 128);
  mpfr_set_str_binary (xx, "0.1E10");
  mpfr_set_str_binary (yy,
                       "0.100000000000000000000000000000000000000000000000"
                       "00000000000000000000000000000000000000000000001E10");
  if (cmpabs (xx, yy) >= 0)
    PRINT_ERROR ("Error in mpfr_cmpabs(10.235, 2346.09234)");
  mpfr_swap (xx, yy);
  if (cmpabs (xx, yy) <= 0)
    PRINT_ERROR ("Error in mpfr_cmpabs(2346.09234, 10.235)");
  mpfr_swap (xx, yy);

  mpfr_clear (xx);
  mpfr_clear (yy);
}

static int
cmpabs_ui (mpfr_srcptr x, unsigned long u)
{
  unsigned int i;
  int r[4];
  mpfr_flags_t f1, f2, flags[2] = { 0, MPFR_FLAGS_ALL };
  mpfr_exp_t emin, emax;

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  for (i = 0; i < 4; i++)
    {
      if (i & 2)
        {
          mpfr_exp_t e = MPFR_IS_SINGULAR (x) ? emax : MPFR_GET_EXP (x);
          set_emin (e);
          set_emax (e);
        }

      __gmpfr_flags = f1 = flags[i % 2];
      r[i] = mpfr_cmpabs_ui (x, u);
      f2 = __gmpfr_flags;
      if (MPFR_IS_NAN (x))
        f1 |= MPFR_FLAGS_ERANGE;

      if (i & 2)
        {
          set_emin (emin);
          set_emax (emax);
        }

      if (f1 != f2)
        {
          printf ("Flags error in mpfr_cmpabs_ui for i = %u\n  x = ", i);
          mpfr_dump (x);
          printf ("  u = %lu\n", u);
          printf ("Expected flags = ");
          flags_out (f1);
          printf ("Obtained flags = ");
          flags_out (f2);
          exit (1);
        }

      if (i > 0)
        MPFR_ASSERTN (r[i] == r[0]);
    }

  return r[0];
}

static void
test_cmpabs_ui (void)
{
  mpfr_t x;

  mpfr_init2 (x, 64); /* should be enough so that ULONG_MAX fits */
  /* check NaN */
  mpfr_set_nan (x);
  if (cmpabs_ui (x, 17) != 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (NaN, 17)");
  /* check -Inf */
  mpfr_set_inf (x, -1);
  if (cmpabs_ui (x, 17) <= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (-Inf, 17)");
  /* check +Inf */
  mpfr_set_inf (x, +1);
  if (cmpabs_ui (x, 17) <= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (+Inf, 17)");
  /* check -1 */
  mpfr_set_si (x, -1, MPFR_RNDN);
  if (cmpabs_ui (x, 0) <= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (-1, 0)");
  if (cmpabs_ui (x, 17) >= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (-1, 17)");
  /* check -0 */
  mpfr_set_zero (x, -1);
  if (cmpabs_ui (x, 0) != 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (-0, 0)");
  if (cmpabs_ui (x, 17) >= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (-0, 17)");
  /* check +0 */
  mpfr_set_zero (x, +1);
  if (cmpabs_ui (x, 0) != 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (+0, 0)");
  if (cmpabs_ui (x, 17) >= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (+0, 17)");
  /* check 1 */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  if (cmpabs_ui (x, 0) <= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (1, 0)");
  if (cmpabs_ui (x, 1) != 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (1, 1)");
  if (cmpabs_ui (x, 17) >= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (1, 17)");
  /* check ULONG_MAX */
  mpfr_set_ui (x, ULONG_MAX, MPFR_RNDN);
  if (cmpabs_ui (x, 0) <= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (ULONG_MAX, 0)");
  if (cmpabs_ui (x, 1) <= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (ULONG_MAX, 1)");
  if (cmpabs_ui (x, 17) <= 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (ULONG_MAX, 17)");
  if (cmpabs_ui (x, ULONG_MAX) != 0)
    PRINT_ERROR ("mpfr_cmpabs_ui (ULONG_MAX, ULONG_MAX)");
  mpfr_clear (x);
}

int
main (void)
{
  tests_start_mpfr ();

  test_cmpabs ();
  test_cmpabs_ui ();

  tests_end_mpfr ();
  return 0;
}
