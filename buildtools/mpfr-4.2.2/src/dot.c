/* mpfr_dot -- dot product of two array of numbers

Copyright 2018-2025 Free Software Foundation, Inc.
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

#include "mpfr-impl.h"

/* FIXME: handle intermediate overflows and underflows. */

/* res <- a[0]*b[0] + ... + a[n-1]*b[n-1] */
int
mpfr_dot (mpfr_ptr res, const mpfr_ptr *a, const mpfr_ptr *b,
          unsigned long n, mpfr_rnd_t rnd)
{
  mpfr_t *c;
  size_t i;
  int inex;
  mpfr_ptr *tab;

  if (MPFR_UNLIKELY (n == 0))
    {
      MPFR_SET_ZERO (res);
      MPFR_SET_POS (res);
      MPFR_RET (0);
    }

  c = (mpfr_t *) mpfr_allocate_func (n * sizeof (mpfr_t));
  tab = (mpfr_ptr *) mpfr_allocate_func (n * sizeof (mpfr_ptr));
  for (i = 0; i < n; i++)
    {
      mpfr_init2 (c[i], mpfr_get_prec (a[i]) + mpfr_get_prec (b[i]));
      inex = mpfr_mul (c[i], a[i], b[i], MPFR_RNDZ); /* exact except... */
      MPFR_ASSERTN (inex == 0); /* failure in case of overflow/underflow */
      tab[i] = c[i];
    }
  inex = mpfr_sum (res, tab, n, rnd);
  for (i = 0; i < n; i++)
    mpfr_clear (c[i]);
  mpfr_free_func (c, n * sizeof (mpfr_t));
  mpfr_free_func (tab, n * sizeof (mpfr_ptr));
  return inex;
}
