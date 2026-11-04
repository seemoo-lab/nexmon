/* __gmpfr_floor_log2 - returns floor(log(d)/log(2))

Copyright 1999-2004, 2006-2025 Free Software Foundation, Inc.
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

/* returns floor(log2(d)) */
long
__gmpfr_floor_log2 (double d)
{
  long exp;
#if _MPFR_IEEE_FLOATS
  union mpfr_ieee_double_extract x;

  x.d = d;
  /* The cast below is useless in theory, but let us not depend on the
     integer promotion rules (for instance, tcc is currently wrong). */
  exp = (long) x.s.exp - 1023;
  MPFR_ASSERTN (exp < 1023);  /* fail on infinities */
  return exp;
#else /* _MPFR_IEEE_FLOATS */
  double m;

  MPFR_ASSERTD (d >= 0);
  if (d >= 1.0)
    {
      exp = -1;
      for (m = 1.0; m <= d; m *= 2.0)
        exp++;
    }
  else
    {
      exp = 0;
      for (m = 1.0; m > d; m *= 0.5)
        exp--;
    }
  return exp;
#endif /* _MPFR_IEEE_FLOATS */
}
