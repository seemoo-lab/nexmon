/* mpfr_pow_uj -- compute the power of a floating-point by a uintmax_t

Copyright 2021-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H
#define MPFR_NEED_INTMAX_H
#include "mpfr-impl.h"

#ifdef _MPFR_H_HAVE_INTMAX_T

#define POW_U mpfr_pow_uj
#define MPZ_SET_U mpfr_mpz_set_uj
#define UTYPE uintmax_t
#define FSPEC "j"

#define ULONG_BITS (sizeof (unsigned long) * CHAR_BIT)

/* z <- n, assuming uintmax_t is at most twice as wide as unsigned long
   and assume no padding bits.
   FIXME: code should be shared with mpfr_set_uj_2exp to avoid such
   limitations (and this could be more efficient).
*/
static void
mpfr_mpz_set_uj (mpz_t z, uintmax_t n)
{
  if ((unsigned long) n == n)
    mpz_set_ui (z, n);
  else
    {
      uintmax_t h = (n >> (ULONG_BITS - 1)) >> 1;
      /* equivalent to n >> ULONG_BITS, as a workaround to GCC bug 4210
         "should not warn in dead code". */

      MPFR_ASSERTN ((unsigned long) h == h);

      mpz_set_ui (z, (unsigned long) h);
      mpz_mul_2exp (z, z, ULONG_BITS);
      mpz_add_ui (z, z, (unsigned long) n);
    }
}

#include "pow_ui.c"

#endif
