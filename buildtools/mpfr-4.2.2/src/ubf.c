/* Functions to work with unbounded floats (limited low-level interface).

Copyright 2016-2025 Free Software Foundation, Inc.
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

/* Note: In MPFR math functions, even if UBF code is not called first,
   we may still need to handle special values NaN and infinities here.
   Indeed, for MAXR * MAXR + (-inf), even though this is a special case,
   the computation will also generate an overflow due to MAXR * MAXR,
   so that UBF code will be called anyway (except if special cases are
   detected and handled separately, but for polynomial, this should not
   be needed). */

/* Get the exponent of a regular MPFR number or UBF as a mpz_t, which is
   initialized by this function. The flags are not changed. */
static void
mpfr_init_get_zexp (mpz_ptr ez, mpfr_srcptr x)
{
  mpz_init (ez);

  if (MPFR_IS_UBF (x))
    mpz_set (ez, MPFR_ZEXP (x));
  else
    {
      mpfr_exp_t ex = MPFR_GET_EXP (x);

#if _MPFR_EXP_FORMAT <= 3
      /* mpfr_exp_t fits in a long */
      mpz_set_si (ez, ex);
#else
      mp_limb_t e_limb[MPFR_EXP_LIMB_SIZE];
      mpfr_t e;
      int inex;
      MPFR_SAVE_EXPO_DECL (expo);

      MPFR_TMP_INIT1 (e_limb, e, sizeof (mpfr_exp_t) * CHAR_BIT);
      MPFR_SAVE_EXPO_MARK (expo);
      MPFR_DBGRES (inex = mpfr_set_exp_t (e, ex, MPFR_RNDN));
      MPFR_ASSERTD (inex == 0);
      MPFR_DBGRES (inex = mpfr_get_z (ez, e, MPFR_RNDN));
      MPFR_ASSERTD (inex == 0);
      MPFR_SAVE_EXPO_FREE (expo);
#endif
    }
}

/* Exact product. The number a is assumed to have enough allocated memory,
   where the trailing bits are regarded as being part of the input numbers
   (no reallocation is attempted and no check is performed as MPFR_TMP_INIT
   could have been used). The arguments b and c may actually be UBF numbers
   (mpfr_srcptr can be seen a bit like void *, but is stronger).
   This function does not change the flags, except in case of NaN. */
void
mpfr_ubf_mul_exact (mpfr_ubf_ptr a, mpfr_srcptr b, mpfr_srcptr c)
{
  MPFR_LOG_FUNC
    (("b[%Pd]=%.*Rg c[%Pd]=%.*Rg",
      mpfr_get_prec (b), mpfr_log_prec, b,
      mpfr_get_prec (c), mpfr_log_prec, c),
     ("a[%Pd]=%.*Rg",
      mpfr_get_prec ((mpfr_ptr) a), mpfr_log_prec, a));

  MPFR_ASSERTD ((mpfr_ptr) a != b);
  MPFR_ASSERTD ((mpfr_ptr) a != c);
  MPFR_SIGN (a) = MPFR_MULT_SIGN (MPFR_SIGN (b), MPFR_SIGN (c));

  if (MPFR_ARE_SINGULAR (b, c))
    {
      if (MPFR_IS_NAN (b) || MPFR_IS_NAN (c))
        MPFR_SET_NAN (a);
      else if (MPFR_IS_INF (b))
        {
          if (MPFR_NOTZERO (c))
            MPFR_SET_INF (a);
          else
            MPFR_SET_NAN (a);
        }
      else if (MPFR_IS_INF (c))
        {
          if (!MPFR_IS_ZERO (b))
            MPFR_SET_INF (a);
          else
            MPFR_SET_NAN (a);
        }
      else
        {
          MPFR_ASSERTD (MPFR_IS_ZERO(b) || MPFR_IS_ZERO(c));
          MPFR_SET_ZERO (a);
        }
    }
  else
    {
      mpfr_exp_t e;
      mp_size_t bn, cn;
      mpfr_limb_ptr ap;
      mp_limb_t u, v;
      int m;

      /* Note about the code below: For the choice of the precision of
       * the result a, one could choose PREC(b) + PREC(c), instead of
       * taking whole limbs into account, but in most cases where one
       * would gain one limb, one would need to copy the significand
       * instead of a no-op (see the mul.c code).
       * But in the case MPFR_LIMB_MSB (u) == 0, if the result fits in
       * an-1 limbs, one could actually do
       *   mpn_rshift (ap, ap, k, GMP_NUMB_BITS - 1)
       * instead of
       *   mpn_lshift (ap, ap, k, 1)
       * to gain one limb (and reduce the precision), replacing a shift
       * by another one. Would this be interesting?
       */

      bn = MPFR_LIMB_SIZE (b);
      cn = MPFR_LIMB_SIZE (c);

      ap = MPFR_MANT (a);

      if (bn == 1 && cn == 1)
        {
          umul_ppmm (ap[1], ap[0], MPFR_MANT(b)[0], MPFR_MANT(c)[0]);
          if (ap[1] & MPFR_LIMB_HIGHBIT)
            m = 0;
          else
            {
              ap[1] = (ap[1] << 1) | (ap[0] >> (GMP_NUMB_BITS - 1));
              ap[0] = ap[0] << 1;
              m = 1;
            }
        }
      else
        {
          if (b == c)
            {
              mpn_sqr (ap, MPFR_MANT (b), bn);
              u = ap[2 * bn - 1];
            }
          else
            u = (bn >= cn) ?
              mpn_mul (ap, MPFR_MANT (b), bn, MPFR_MANT (c), cn) :
              mpn_mul (ap, MPFR_MANT (c), cn, MPFR_MANT (b), bn);
          if (MPFR_LIMB_MSB (u) == 0)
            {
              m = 1;
              MPFR_DBGRES (v = mpn_lshift (ap, ap, bn + cn, 1));
              MPFR_ASSERTD (v == 0);
            }
          else
            m = 0;
        }

      if (! MPFR_IS_UBF (b) && ! MPFR_IS_UBF (c) &&
          (e = MPFR_GET_EXP (b) + MPFR_GET_EXP (c) - m,
           MPFR_EXP_IN_RANGE (e)))
        {
          MPFR_SET_EXP (a, e);
        }
      else
        {
          mpz_t be, ce;

          mpz_init (MPFR_ZEXP (a));

          /* This may involve copies of mpz_t, but exponents should not be
             very large integers anyway. */
          mpfr_init_get_zexp (be, b);
          mpfr_init_get_zexp (ce, c);
          mpz_add (MPFR_ZEXP (a), be, ce);
          mpz_clear (be);
          mpz_clear (ce);
          mpz_sub_ui (MPFR_ZEXP (a), MPFR_ZEXP (a), m);
          MPFR_SET_UBF (a);
        }
    }
}

/* Compare the exponents of two numbers, which can be either MPFR numbers
   or UBF numbers. If both numbers can be MPFR numbers, it is better to
   use the MPFR_UBF_EXP_LESS_P wrapper macro, which is optimized for this
   common case. */
int
mpfr_ubf_exp_less_p (mpfr_srcptr x, mpfr_srcptr y)
{
  mpz_t xe, ye;
  int c;

  mpfr_init_get_zexp (xe, x);
  mpfr_init_get_zexp (ye, y);
  c = mpz_cmp (xe, ye) < 0;
  mpz_clear (xe);
  mpz_clear (ye);
  return c;
}

/* Convert an mpz_t to an mpfr_exp_t, saturated to
   the interval [MPFR_EXP_MIN,MPFR_EXP_MAX]. */
mpfr_exp_t
mpfr_ubf_zexp2exp (mpz_ptr ez)
{
  mp_size_t n;
  mpfr_eexp_t e;
  mpfr_t d;
  int inex;
  MPFR_SAVE_EXPO_DECL (expo);

  n = ABSIZ (ez); /* limb size of ez */
  if (n == 0)
    return 0;

  MPFR_SAVE_EXPO_MARK (expo);
  mpfr_init2 (d, n * GMP_NUMB_BITS);
  MPFR_DBGRES (inex = mpfr_set_z (d, ez, MPFR_RNDN));
  MPFR_ASSERTD (inex == 0);
  e = mpfr_get_exp_t (d, MPFR_RNDZ);
  mpfr_clear (d);
  MPFR_SAVE_EXPO_FREE (expo);
  if (MPFR_UNLIKELY (e < MPFR_EXP_MIN))
    return MPFR_EXP_MIN;
  if (MPFR_UNLIKELY (e > MPFR_EXP_MAX))
    return MPFR_EXP_MAX;
  return e;
}

/* Return the difference of the exponents of x and y, saturated to
   the interval [MPFR_EXP_MIN,MPFR_EXP_MAX]. */
mpfr_exp_t
mpfr_ubf_diff_exp (mpfr_srcptr x, mpfr_srcptr y)
{
  mpz_t xe, ye;
  mpfr_exp_t e;

  mpfr_init_get_zexp (xe, x);
  mpfr_init_get_zexp (ye, y);
  mpz_sub (xe, xe, ye);
  mpz_clear (ye);
  e = mpfr_ubf_zexp2exp (xe);
  mpz_clear (xe);
  return e;
}
