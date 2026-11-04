/* Test file for mpfr_get_exp and mpfr_set_exp.

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

int
main (int argc, char *argv[])
{
  mpfr_t x;
  int ret;
  mpfr_exp_t emin, emax, e;
  int i = 0;

  tests_start_mpfr ();

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  mpfr_init2 (x, 53);

  mpfr_set_ui (x, 1, MPFR_RNDN);
  ret = mpfr_set_exp (x, 2);
  MPFR_ASSERTN (ret == 0 && mpfr_cmp_ui (x, 2) == 0);
  e = mpfr_get_exp (x);
  MPFR_ASSERTN (e == 2);
  e = (mpfr_get_exp) (x);
  MPFR_ASSERTN (e == 2);

  ret = mpfr_set_exp (x, emax);
  e = mpfr_get_exp (x);
  MPFR_ASSERTN (e == emax);
  e = (mpfr_get_exp) (x);
  MPFR_ASSERTN (e == emax);

  ret = mpfr_set_exp (x, emin);
  e = mpfr_get_exp (x);
  MPFR_ASSERTN (e == emin);
  e = (mpfr_get_exp) (x);
  MPFR_ASSERTN (e == emin);

  set_emin (-1);

  e = mpfr_get_exp (x);
  MPFR_ASSERTN (e == emin);
  e = (mpfr_get_exp) (x);
  MPFR_ASSERTN (e == emin);

#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
  e = mpfr_get_exp ((i++, VOIDP_CAST(x)));
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
  MPFR_ASSERTN (e == emin);
  MPFR_ASSERTN (i == 1);

  ret = mpfr_set_exp (x, -1);
  MPFR_ASSERTN (ret == 0 && mpfr_cmp_ui_2exp (x, 1, -2) == 0);

  set_emax (1);
  ret = mpfr_set_exp (x, 1);
  MPFR_ASSERTN (ret == 0 && mpfr_cmp_ui (x, 1) == 0);

  ret = mpfr_set_exp (x, -2);
  MPFR_ASSERTN (ret != 0 && mpfr_cmp_ui (x, 1) == 0);

  ret = mpfr_set_exp (x, 2);
  MPFR_ASSERTN (ret != 0 && mpfr_cmp_ui (x, 1) == 0);

  mpfr_clear (x);

  set_emin (emin);
  set_emax (emax);

  tests_end_mpfr ();
  return 0;
}
