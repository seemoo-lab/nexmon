/* mpfr_set_q -- set a floating-point number from a multiple-precision rational

Copyright 2000-2002, 2004-2025 Free Software Foundation, Inc.
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
#include "mpfr-impl.h"

#ifndef MPFR_USE_MINI_GMP
/*
 * Set f to z, choosing the smallest precision for f
 * so that z = f*(2^BPML)*zs*2^(RetVal)
 */
static int
set_z (mpfr_ptr f, mpz_srcptr z, mp_size_t *zs)
{
  mp_limb_t *p;
  mp_size_t s;
  int c;
  mpfr_prec_t pf;

  MPFR_ASSERTD (mpz_sgn (z) != 0);

  /* Remove useless ending 0 */
  for (p = PTR (z), s = *zs = ABSIZ (z) ; *p == 0; p++, s--)
    MPFR_ASSERTD (s >= 0);

  /* Get working precision */
  count_leading_zeros (c, p[s-1]);
  pf = s * GMP_NUMB_BITS - c;
  MPFR_ASSERTD (pf >= 1);
  mpfr_init2 (f, pf >= MPFR_PREC_MIN ? pf : MPFR_PREC_MIN);

  /* Copy Mantissa */
  if (MPFR_LIKELY (c))
    mpn_lshift (MPFR_MANT (f), p, s, c);
  else
    MPN_COPY (MPFR_MANT (f), p, s);

  MPFR_SET_SIGN (f, mpz_sgn (z));
  MPFR_SET_EXP (f, 0);

  return -c;
}

/* set f to the rational q */
int
mpfr_set_q (mpfr_ptr f, mpq_srcptr q, mpfr_rnd_t rnd)
{
  mpz_srcptr num, den;
  mpfr_t n, d;
  int inexact;
  int cn, cd;
  long shift;
  mp_size_t sn, sd;
  MPFR_SAVE_EXPO_DECL (expo);

  num = mpq_numref (q);
  den = mpq_denref (q);
  /* NAN and INF for mpq are not really documented, but could be found */
  if (MPFR_UNLIKELY (mpz_sgn (num) == 0))
    {
      if (MPFR_UNLIKELY (mpz_sgn (den) == 0))
        {
          MPFR_SET_NAN (f);
          MPFR_RET_NAN;
        }
      else
        {
          MPFR_SET_ZERO (f);
          MPFR_SET_POS (f);
          MPFR_RET (0);
        }
    }
  if (MPFR_UNLIKELY (mpz_sgn (den) == 0))
    {
      MPFR_SET_INF (f);
      MPFR_SET_SIGN (f, mpz_sgn (num));
      MPFR_RET (0);
    }

  MPFR_SAVE_EXPO_MARK (expo);

  cn = set_z (n, num, &sn);
  cd = set_z (d, den, &sd);

  /* sn is the number of limbs of the numerator, sd that of the denominator */

  sn -= sd;
#if GMP_NUMB_BITS <= 32 /* overflow/underflow cannot happen on 64-bit
                           processors, where MPFR_EMAX_MAX is 2^62 - 1, due to
                           memory limits */
  /* If sn >= 0, the quotient has at most sn limbs, thus is larger or equal to
     2^((sn-1)*GMP_NUMB_BITS), thus its exponent >= (sn-1)*GMP_NUMB_BITS)+1.
     (sn-1)*GMP_NUMB_BITS)+1 > emax yields (sn-1)*GMP_NUMB_BITS) >= emax,
     i.e., sn-1 >= floor(emax/GMP_NUMB_BITS). */
  if (MPFR_UNLIKELY (sn > MPFR_EMAX_MAX / GMP_NUMB_BITS))
    {
      MPFR_SAVE_EXPO_FREE (expo);
      inexact = mpfr_overflow (f, rnd, MPFR_SIGN (f));
      goto end;
    }
  /* If sn < 0, the inverse quotient is >= 2^((-sn-1)*GMP_NUMB_BITS),
     thus the quotient is <= 2^((sn+1)*GMP_NUMB_BITS), and thus its
     exponent is <= (sn+1)*GMP_NUMB_BITS+1.
     (sn+1)*GMP_NUMB_BITS+1 < emin yields (sn+1)*GMP_NUMB_BITS+2 <= emin,
     i.e., sn+1 <= floor((emin-2)/GMP_NUMB_BITS). */
  if (MPFR_UNLIKELY (sn <= (MPFR_EMIN_MIN - 2) / GMP_NUMB_BITS - 1))
    {
      MPFR_SAVE_EXPO_FREE (expo);
      if (rnd == MPFR_RNDN)
        rnd = MPFR_RNDZ;
      inexact = mpfr_underflow (f, rnd, MPFR_SIGN (f));
      goto end;
    }
#endif

  inexact = mpfr_div (f, n, d, rnd);
  shift = GMP_NUMB_BITS*sn+cn-cd;
  MPFR_ASSERTD (shift == GMP_NUMB_BITS*sn+cn-cd);
  cd = mpfr_mul_2si (f, f, shift, rnd);
  MPFR_SAVE_EXPO_FREE (expo);
  /* we can have cd <> 0 only in case of underflow or overflow, but since we
     are still in extended exponent range, this cannot happen on 64-bit (see
     above) */
#if GMP_NUMB_BITS <= 32
  if (MPFR_UNLIKELY (cd != 0))
    inexact = cd;
  else
    inexact = mpfr_check_range (f, inexact, rnd);
 end:
#else
  MPFR_ASSERTD(cd == 0);
  inexact = mpfr_check_range (f, inexact, rnd);
#endif
  mpfr_clear (d);
  mpfr_clear (n);
  MPFR_RET (inexact);
}
#endif
