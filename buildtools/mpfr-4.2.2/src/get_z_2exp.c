/* mpfr_get_z_2exp -- get a multiple-precision integer and an exponent
                      from a floating-point number

Copyright 2000-2025 Free Software Foundation, Inc.
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

/* puts the significand of f into z, and returns 'exp' such that f = z * 2^exp
 *
 * 0 doesn't have an exponent, therefore the returned exponent in this case
 * isn't really important. We choose to return __gmpfr_emin because
 *   1) it is in the exponent range [__gmpfr_emin,__gmpfr_emax],
 *   2) the smaller a number is (in absolute value), the smaller its
 *      exponent is. In other words, the f -> exp function is monotonous
 *      on non-negative numbers. --> This is WRONG since the returned
 *      exponent is not necessarily in the exponent range!
 * Note that this is different from the C function frexp().
 *
 * For NaN and infinities, we choose to set z = 0 (neutral value).
 * The exponent doesn't really matter, so let's keep __gmpfr_emin
 * for consistency. The erange flag is set.
 */

/* MPFR_LARGE_EXP can be defined when mpfr_exp_t is guaranteed to have
   at least 64 bits (in a portable way). */
#if GMP_NUMB_BITS >= 64
/* Now, we know that the constant below is supported by the compiler. */
# if _MPFR_EXP_FORMAT >= 3 && LONG_MAX >= 9223372036854775807
#  define MPFR_LARGE_EXP 1
# endif
#endif

mpfr_exp_t
mpfr_get_z_2exp (mpz_ptr z, mpfr_srcptr f)
{
  mp_size_t fn;
  int sh;

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (f)))
    {
      if (MPFR_UNLIKELY (MPFR_NOTZERO (f)))
        MPFR_SET_ERANGEFLAG ();
      mpz_set_ui (z, 0);
      return __gmpfr_emin;
    }

  fn = MPFR_LIMB_SIZE(f);

  /* FIXME: temporary assert for security. Too large values should
     probably be handled like infinities. */
  MPFR_ASSERTN (fn <= INT_MAX);  /* due to SIZ(z) being an int */

  /* check whether allocated space for z is enough */
  mpz_realloc2 (z, (mp_bitcnt_t) fn * GMP_NUMB_BITS);

  MPFR_UNSIGNED_MINUS_MODULO (sh, MPFR_PREC (f));
  if (MPFR_LIKELY (sh))
    mpn_rshift (PTR (z), MPFR_MANT (f), fn, sh);
  else
    MPN_COPY (PTR (z), MPFR_MANT (f), fn);

  SIZ(z) = MPFR_IS_NEG (f) ? -fn : fn;

#ifndef MPFR_LARGE_EXP
  /* If mpfr_exp_t has 64 bits, then MPFR_GET_EXP(f) >= MPFR_EMIN_MIN = 1-2^62
     and MPFR_EXP_MIN <= 1-2^63, thus the following implies PREC(f) > 2^62,
     which is impossible due to memory constraints. */
  if (MPFR_UNLIKELY ((mpfr_uexp_t) MPFR_GET_EXP (f) - MPFR_EXP_MIN
                     < (mpfr_uexp_t) MPFR_PREC (f)))
    {
      /* The exponent isn't representable in an mpfr_exp_t. */
      MPFR_SET_ERANGEFLAG ();
      return MPFR_EXP_MIN;
    }
#endif

  return MPFR_GET_EXP (f) - MPFR_PREC (f);
}
