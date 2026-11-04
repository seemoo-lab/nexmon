/* mpfr_cbrt -- cube root function.

Copyright 2002-2025 Free Software Foundation, Inc.
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

/* The computation of y = x^(1/3) is done as follows.

   Let n = PREC(y), or PREC(y) + 1 if the rounding mode is MPFR_RNDN.
   We seek to compute an integer cube root in precision n and the
   associated inexact bit (non-zero iff the remainder is non-zero).

   Let us write x, possibly truncated, under the form sign * m * 2^(3*e)
   where m is an integer such that 2^(3n-3) <= m < 2^(3n), i.e. m has
   between 3n-2 and 3n bits.

   Let s be the integer cube root of m, i.e. the maximum integer such that
   m = s^3 + t with t >= 0. Thus 2^(n-1) <= s < 2^n, i.e. s has n bits.

   Then |x|^(1/3) = s * 2^e or (s+1) * 2^e depending on the rounding mode,
   the sign, and whether s is "inexact" (i.e. t > 0 or the truncation of x
   was not equal to x).

   Note: The truncation of x was allowed because any breakpoint has n bits
   and its cube has at most 3n bits. Thus the truncation of x cannot yield
   a cube root below RNDZ(x^(1/3)) in precision n. [TODO: add details.]
*/

int
mpfr_cbrt (mpfr_ptr y, mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  mpz_t m;
  mpfr_exp_t e, d, sh;
  mpfr_prec_t n, size_m;
  int inexact, inexact2, negative, r;
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_LOG_FUNC (
    ("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (x), mpfr_log_prec, x, rnd_mode),
    ("y[%Pd]=%.*Rg inexact=%d", mpfr_get_prec (y), mpfr_log_prec, y,
     inexact));

  /* special values */
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    {
      if (MPFR_IS_NAN (x))
        {
          MPFR_SET_NAN (y);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_INF (x))
        {
          MPFR_SET_INF (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0);
        }
      /* case 0: cbrt(+/- 0) = +/- 0 */
      else /* x is necessarily 0 */
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (x));
          MPFR_SET_ZERO (y);
          MPFR_SET_SAME_SIGN (y, x);
          MPFR_RET (0);
        }
    }

  /* General case */
  MPFR_SAVE_EXPO_MARK (expo);
  mpz_init (m);

  e = mpfr_get_z_2exp (m, x);                /* x = m * 2^e */
  if ((negative = MPFR_IS_NEG(x)))
    mpz_neg (m, m);
  r = e % 3;
  if (r < 0)
    r += 3;
  MPFR_ASSERTD (r >= 0 && r < 3 && (e - r) % 3 == 0);

  /* x = (m*2^r) * 2^(e-r) = (m*2^r) * 2^(3*q) */

  MPFR_LOG_MSG (("e=%" MPFR_EXP_FSPEC "d r=%d\n", (mpfr_eexp_t) e, r));

  MPFR_MPZ_SIZEINBASE2 (size_m, m);
  n = MPFR_PREC (y) + (rnd_mode == MPFR_RNDN);

  /* We will need to multiply m by 2^(r'), truncated if r' < 0, and
     subtract r' from e, so that m has between 3n-2 and 3n bits and
     e becomes a multiple of 3.
     Since r = e % 3, we write r' = 3 * sh + r.
     We want 3 * n - 2 <= size_m + 3 * sh + r <= 3 * n.
     Let d = 3 * n - size_m - r. Thus we want 0 <= d - 3 * sh <= 2,
     i.e. sh = floor(d/3). */
  d = 3 * (mpfr_exp_t) n - (mpfr_exp_t) size_m - r;
  sh = d >= 0 ? d / 3 : - ((2 - d) / 3);  /* floor(d/3) */
  r += 3 * sh;  /* denoted r' above */

  e -= r;
  MPFR_ASSERTD (e % 3 == 0);
  e /= 3;

  inexact = 0;

  if (r > 0)
    {
      mpz_mul_2exp (m, m, r);
    }
  else if (r < 0)
    {
      r = -r;
      inexact = mpz_scan1 (m, 0) < r;
      mpz_fdiv_q_2exp (m, m, r);
    }

  /* we reuse the variable m to store the cube root, since it is not needed
     any more: we just need to know if the root is exact */
  inexact = ! mpz_root (m, m, 3) || inexact;

#if MPFR_WANT_ASSERT > 0
  {
    mpfr_prec_t tmp;

    MPFR_MPZ_SIZEINBASE2 (tmp, m);
    MPFR_ASSERTN (tmp == n);
  }
#endif

  if (inexact)
    {
      if (negative)
        rnd_mode = MPFR_INVERT_RND (rnd_mode);
      if (rnd_mode == MPFR_RNDU || rnd_mode == MPFR_RNDA
          || (rnd_mode == MPFR_RNDN && mpz_tstbit (m, 0)))
        {
          inexact = 1;
          mpz_add_ui (m, m, 1);
        }
      else
        inexact = -1;
    }

  /* either inexact is not zero, and the conversion is exact, i.e. inexact
     is not changed; or inexact=0, and inexact is set only when
     rnd_mode=MPFR_RNDN and bit (n+1) from m is 1 */
  inexact2 = mpfr_set_z (y, m, MPFR_RNDN);
  MPFR_ASSERTD (inexact == 0 || inexact2 == 0);
  inexact += inexact2;
  MPFR_SET_EXP (y, MPFR_GET_EXP (y) + e);

  if (negative)
    {
      MPFR_CHANGE_SIGN (y);
      inexact = -inexact;
    }

  mpz_clear (m);
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (y, inexact, rnd_mode);
}
