/* mpfr_set_sj -- set a MPFR number from a huge machine signed integer

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

#define MPFR_NEED_INTMAX_H
#include "mpfr-impl.h"

#ifdef _MPFR_H_HAVE_INTMAX_T

int
mpfr_set_sj (mpfr_ptr x, intmax_t j, mpfr_rnd_t rnd)
{
  return mpfr_set_sj_2exp (x, j, 0, rnd);
}

int
mpfr_set_sj_2exp (mpfr_ptr x, intmax_t j, intmax_t e, mpfr_rnd_t rnd)
{
  if (j >= 0)
    return mpfr_set_uj_2exp (x, j, e, rnd);
  else
    {
      int inex;
      inex = mpfr_set_uj_2exp (x, - (uintmax_t) j, e, MPFR_INVERT_RND (rnd));
      MPFR_CHANGE_SIGN (x);
      return -inex;
    }
}

#endif
