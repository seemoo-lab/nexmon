/* mpfr_set -- copy of a floating-point number

Copyright 1999, 2001-2025 Free Software Foundation, Inc.
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

/* set a to abs(b) * signb: a=b when signb = SIGN(b), a=abs(b) when signb=1 */
MPFR_HOT_FUNCTION_ATTR int
mpfr_set4 (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode, int signb)
{
  /* Sign is ALWAYS copied */
  MPFR_SET_SIGN (a, signb);

  /* Exponent is also always copied since if the number is singular,
     the exponent field determined the number.
     Can't use MPFR_SET_EXP since the exponent may be singular */
  MPFR_EXP (a) = MPFR_EXP (b);

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (b)))
    {
      /* MPFR_SET_NAN, MPFR_SET_ZERO and MPFR_SET_INF are useless
         since MPFR_EXP (a) = MPFR_EXP (b) does the job */
      if (MPFR_IS_NAN (b))
        MPFR_RET_NAN;
      else
        MPFR_RET (0);
    }
  else if (MPFR_PREC (b) == MPFR_PREC (a))
    {
      /* Same precision and b is not singular:
       * just copy the mantissa, and set the exponent and the sign
       * The result is exact. */
      MPN_COPY (MPFR_MANT (a), MPFR_MANT (b), MPFR_LIMB_SIZE (b));
      MPFR_RET (0);
    }
  else
    {
      int inex;

      /* Else Round B inside a */
      MPFR_RNDRAW (inex, a, MPFR_MANT (b), MPFR_PREC (b), rnd_mode, signb,
                   if (MPFR_UNLIKELY (++ MPFR_EXP (a) > __gmpfr_emax))
                     return mpfr_overflow (a, rnd_mode, signb)
                   );
      MPFR_RET (inex);
    }
}

/* Set a to b  */
#undef mpfr_set
int
mpfr_set (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode)
{
  /* Contrary to other mpfr_set4 based functions (mpfr_abs, mpfr_neg, etc.),
     do not detect the case a == b as there is no interest to call mpfr_set
     in this case, so that it is very unlikely that the user calls it
     with a == b (this is the reverse of what is assumed for the other
     functions). */
  return mpfr_set4 (a, b, rnd_mode, MPFR_SIGN (b));
}

/* Set a to |b| */
#undef mpfr_abs
int
mpfr_abs (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode)
{
  if (MPFR_UNLIKELY (a != b))
    return mpfr_set4 (a, b, rnd_mode, MPFR_SIGN_POS);
  else
    {
      MPFR_SET_POS (a);
      if (MPFR_UNLIKELY (MPFR_IS_NAN (b)))
        MPFR_RET_NAN;
      else
        MPFR_RET (0);
    }
}

/* Round (u, inex) into s with rounding mode rnd_mode, where inex is
   the ternary value associated with u, which has been obtained using
   the *same* rounding mode rnd_mode.
   Assumes PREC(u) = 2*PREC(s).
   The generic algorithm is the following:
   1. inex2 = mpfr_set (s, u, rnd_mode);
   2. if (inex2 != 0) return inex2; else return inex;
      except in the double-rounding case: in MPFR_RNDN, when u is in the
      middle of two consecutive PREC(s)-bit numbers, if inex and inex2
      are both > 0 (resp. both < 0), we correct s to nextbelow(s) (resp.
      nextabove(s)), and return the opposite of inex.
   Note: this function can be called with rnd_mode == MPFR_RNDF, in
   which case, the rounding direction and the returned ternary value
   are unspecified. */
int
mpfr_set_1_2 (mpfr_ptr s, mpfr_srcptr u, mpfr_rnd_t rnd_mode, int inex)
{
  mpfr_prec_t p = MPFR_PREC(s);
  mpfr_prec_t sh = GMP_NUMB_BITS - p;
  mp_limb_t rb, sb;
  mp_limb_t *sp = MPFR_MANT(s);
  mp_limb_t *up = MPFR_MANT(u);
  mp_limb_t mask;
  int inex2;

  if (MPFR_UNLIKELY(MPFR_IS_SINGULAR(u)))
    {
      mpfr_set (s, u, rnd_mode);
      return inex;
    }

  MPFR_ASSERTD(MPFR_PREC(u) == 2 * p);

  if (p < GMP_NUMB_BITS)
    {
      mask = MPFR_LIMB_MASK(sh);

      if (MPFR_PREC(u) <= GMP_NUMB_BITS)
        {
          mp_limb_t u0 = up[0];

          /* it suffices to round (u0, inex) */
          rb = u0 & (MPFR_LIMB_ONE << (sh - 1));
          sb = (u0 & mask) ^ rb;
          sp[0] = u0 & ~mask;
        }
      else
        {
          mp_limb_t u1 = up[1];

          /* we need to round (u1, u0, inex) */
          mask = MPFR_LIMB_MASK(sh);
          rb = u1 & (MPFR_LIMB_ONE << (sh - 1));
          sb = ((u1 & mask) ^ rb) | up[0];
          sp[0] = u1 & ~mask;
        }

      inex2 = inex * MPFR_SIGN(u);
      MPFR_SIGN(s) = MPFR_SIGN(u);
      MPFR_EXP(s) = MPFR_EXP(u);

      /* in case inex2 > 0, the value of u is rounded away,
         thus we need to subtract something from (u0, rb, sb):
         (a) if sb is not zero, since the subtracted value is < 1, we can leave
         sb as it is;
         (b) if rb <> 0 and sb = 0: change to rb = 0 and sb = 1
         (c) if rb = sb = 0: change to rb = 1 and sb = 1, and subtract 1 */
      if (inex2 > 0)
        {
          if (rb && sb == 0)
            {
              rb = 0;
              sb = 1;
            }
        }
      else /* inex2 <= 0 */
        sb |= inex;

      /* now rb, sb are the round and sticky bits, together with the value of
         sp[0], except possibly in the case rb = sb = 0 and inex2 > 0 */
      if (rb == 0 && sb == 0)
        {
          if (inex2 <= 0)
            MPFR_RET(0);
          else /* inex2 > 0 can only occur for RNDN and RNDA:
                  RNDN: return sp[0] and inex
                  RNDA: return sp[0] and inex */
            MPFR_RET(inex);
        }
      else if (rnd_mode == MPFR_RNDN)
        {
          if (rb == 0 || (sb == 0 && (sp[0] & (MPFR_LIMB_ONE << sh)) == 0))
            goto truncate;
          else
            goto add_one_ulp;
        }
      else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(s)))
        {
        truncate:
          MPFR_RET(-MPFR_SIGN(s));
        }
      else /* round away from zero */
        {
        add_one_ulp:
          sp[0] += MPFR_LIMB_ONE << sh;
          if (MPFR_UNLIKELY(sp[0] == 0))
            {
              sp[0] = MPFR_LIMB_HIGHBIT;
              if (MPFR_EXP(s) + 1 <= __gmpfr_emax)
                MPFR_SET_EXP (s, MPFR_EXP(s) + 1);
              else /* overflow */
                return mpfr_overflow (s, rnd_mode, MPFR_SIGN(s));
            }
          MPFR_RET(MPFR_SIGN(s));
        }
    }

  /* general case PREC(s) >= GMP_NUMB_BITS */
  inex2 = mpfr_set (s, u, rnd_mode);
  /* Check the double-rounding case, i.e. with u = middle of two
     consecutive PREC(s)-bit numbers, which is equivalent to u being
     exactly representable on PREC(s) + 1 bits but not on PREC(s) bits.
     Moreover, since PREC(u) = 2*PREC(s), u and s cannot be identical
     (as pointers), thus u was not changed. */
  if (rnd_mode == MPFR_RNDN && inex * inex2 > 0 &&
      mpfr_min_prec (u) == p + 1)
    {
      if (inex > 0)
        mpfr_nextbelow (s);
      else
        mpfr_nextabove (s);
      return -inex;
    }
  return inex2 != 0 ? inex2 : inex;
}
