/* __gmpfr_ceil_exp2 - returns y >= 2^d

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

/* returns y >= 2^d, assuming that d <= 1024
   for d integer, returns exactly 2^d
*/
double
__gmpfr_ceil_exp2 (double d)
{
  long exp;
#if _MPFR_IEEE_FLOATS
  union mpfr_ieee_double_extract x;
#else
  struct {double d;} x;
#endif

  MPFR_ASSERTN(d <= 1024.0);
  exp = (long) d;
  if (d != (double) exp)
    exp++;
  /* now exp = ceil(d) */
  x.d = 1.0;
#if _MPFR_IEEE_FLOATS
  x.s.exp = exp <= -1022 ? 1 : 1023 + exp;
#else
  if (exp >= 0)
    {
      while (exp != 0)
        {
          x.d *= 2.0;
          exp--;
        }
    }
  else
    {
      while (exp != 0)
        {
          x.d *= (1.0 / 2.0);
          exp++;
        }
    }
#endif /* _MPFR_IEEE_FLOATS */
  return x.d;
}
