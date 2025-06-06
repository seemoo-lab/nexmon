/* mpfr_cmp_str -- compare a floating-point number with a string.

Copyright 2004-2016 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

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
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */

#include "mpfr-test.h"

int
mpfr_cmp_str (mpfr_srcptr x, const char *s, int base, mpfr_rnd_t rnd)
{
  mpfr_t y;
  int res;

  MPFR_ASSERTN (!MPFR_IS_NAN (x));
  mpfr_init2 (y, MPFR_PREC(x));
  mpfr_set_str (y, s, base, rnd);
  res = mpfr_cmp (x,y);
  mpfr_clear (y);
  return res;
}
