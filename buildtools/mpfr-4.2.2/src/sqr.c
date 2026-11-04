/* mpfr_sqr -- Floating-point square

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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

#if !defined(MPFR_GENERIC_ABI) && (GMP_NUMB_BITS == 32 || GMP_NUMB_BITS == 64)

/* Special code for prec(a) < GMP_NUMB_BITS and prec(b) <= GMP_NUMB_BITS.
   Note: this function was copied from mpfr_mul_1 in file mul.c, thus any
   change here should be done also in mpfr_mul_1.
   Although this function works as soon as prec(a) < GMP_NUMB_BITS and
   prec(b) <= GMP_NUMB_BITS, we use it for prec(a)=prec(b) < GMP_NUMB_BITS. */
static int
mpfr_sqr_1 (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode, mpfr_prec_t p)
{
  mp_limb_t a0;
  mpfr_limb_ptr ap = MPFR_MANT(a);
  mp_limb_t b0 = MPFR_MANT(b)[0];
  mpfr_exp_t ax;
  mpfr_prec_t sh = GMP_NUMB_BITS - p;
  mp_limb_t rb, sb, mask = MPFR_LIMB_MASK(sh);

  /* When prec(b) <= GMP_NUMB_BITS / 2, we could replace umul_ppmm
     by a limb multiplication as follows, but we assume umul_ppmm is as fast
     as a limb multiplication on modern processors:
      a0 = (b0 >> (GMP_NUMB_BITS / 2)) * (b0 >> (GMP_NUMB_BITS / 2));
      sb = 0;
  */
  ax = MPFR_GET_EXP(b) * 2;
  umul_ppmm (a0, sb, b0, b0);
  if (a0 < MPFR_LIMB_HIGHBIT)
    {
      ax --;
      a0 = (a0 << 1) | (sb >> (GMP_NUMB_BITS - 1));
      sb <<= 1;
    }
  rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
  sb |= (a0 & mask) ^ rb;
  ap[0] = a0 & ~mask;

  MPFR_SIGN(a) = MPFR_SIGN_POS;

  /* rounding */
  if (MPFR_UNLIKELY(ax > __gmpfr_emax))
    return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);

  /* Warning: underflow should be checked *after* rounding, thus when rounding
     away and when a > 0.111...111*2^(emin-1), or when rounding to nearest and
     a >= 0.111...111[1]*2^(emin-1), there is no underflow. */
  if (MPFR_UNLIKELY(ax < __gmpfr_emin))
    {
      /* Note: for emin=2*k+1, a >= 0.111...111*2^(emin-1) is not possible,
         i.e., a >= (1 - 2^(-p))*2^(2k), since we need a = b^2 with EXP(b)=k,
         and the largest such b is (1 - 2^(-p))*2^k satisfies
         b^2 < (1 - 2^(-p))*2^(2k).
         For emin=2*k, it is only possible for some values of p: it is not
         possible for p=53, because the largest significand is 6369051672525772
         but its square has only 52 leading ones. For p=24 it is possible,
         with b = 11863283, whose square has 24 leading ones. */
      if (ax == __gmpfr_emin - 1 && ap[0] == ~mask &&
          ((rnd_mode == MPFR_RNDN && rb) ||
           (MPFR_IS_LIKE_RNDA (rnd_mode, 0) && (rb | sb))))
        goto rounding; /* no underflow */
      /* For RNDN, mpfr_underflow always rounds away, thus for |a| <= 2^(emin-2)
         we have to change to RNDZ. This corresponds to:
         (a) either ax < emin - 1
         (b) or ax = emin - 1 and ap[0] = 1000....000 and rb = sb = 0 */
      if (rnd_mode == MPFR_RNDN &&
          (ax < __gmpfr_emin - 1 ||
           (ap[0] == MPFR_LIMB_HIGHBIT && (rb | sb) == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN_POS);
    }

 rounding:
  MPFR_EXP (a) = ax; /* Don't use MPFR_SET_EXP since ax might be < __gmpfr_emin
                        in the cases "goto rounding" above. */
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(ax >= __gmpfr_emin);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ (rnd_mode, 0))
    {
    truncate:
      MPFR_ASSERTD(ax >= __gmpfr_emin);
      MPFR_RET(-MPFR_SIGN_POS);
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      if (ap[0] == 0)
        {
          ap[0] = MPFR_LIMB_HIGHBIT;
          if (MPFR_UNLIKELY(ax + 1 > __gmpfr_emax))
            return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);
          MPFR_ASSERTD(ax + 1 <= __gmpfr_emax);
          MPFR_ASSERTD(ax + 1 >= __gmpfr_emin);
          MPFR_SET_EXP (a, ax + 1);
        }
      MPFR_RET(MPFR_SIGN_POS);
    }
}

/* special code for PREC(a) = GMP_NUMB_BITS */
static int
mpfr_sqr_1n (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode)
{
  mp_limb_t a0;
  mpfr_limb_ptr ap = MPFR_MANT(a);
  mp_limb_t b0 = MPFR_MANT(b)[0];
  mpfr_exp_t ax;
  mp_limb_t rb, sb;

  ax = MPFR_GET_EXP(b) * 2;
  umul_ppmm (a0, sb, b0, b0);
  if (a0 < MPFR_LIMB_HIGHBIT)
    {
      ax --;
      a0 = (a0 << 1) | (sb >> (GMP_NUMB_BITS - 1));
      sb <<= 1;
    }
  rb = sb & MPFR_LIMB_HIGHBIT;
  sb = sb & ~MPFR_LIMB_HIGHBIT;
  ap[0] = a0;

  MPFR_SIGN(a) = MPFR_SIGN_POS;

  /* rounding */
  if (MPFR_UNLIKELY(ax > __gmpfr_emax))
    return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);

  /* Warning: underflow should be checked *after* rounding, thus when rounding
     away and when a > 0.111...111*2^(emin-1), or when rounding to nearest and
     a >= 0.111...111[1]*2^(emin-1), there is no underflow. */
  if (MPFR_UNLIKELY(ax < __gmpfr_emin))
    {
      /* As seen in mpfr_mul_1, we cannot have a0 = 111...111 here if there
         was not an exponent decrease (ax--) above.
         In the case of an exponent decrease:
         - For GMP_NUMB_BITS=32, a0 = 111...111 is not possible since the
           largest b0 such that b0^2 < 2^(2*32-1) is b0=3037000499, but
           its square has only 30 leading ones.
         - For GMP_NUMB_BITS=64, a0 = 111...111 is possible: the largest b0
           is 13043817825332782212, and its square has 64 leading ones; but
           since the next bit is rb=0, for RNDN, we always have an underflow.
         For the test below, note that a is positive.
      */
      if (ax == __gmpfr_emin - 1 && ap[0] == MPFR_LIMB_MAX &&
          MPFR_IS_LIKE_RNDA (rnd_mode, 0))
        goto rounding; /* no underflow */
      /* For RNDN, mpfr_underflow always rounds away, thus for |a| <= 2^(emin-2)
         we have to change to RNDZ. This corresponds to:
         (a) either ax < emin - 1
         (b) or ax = emin - 1 and ap[0] = 1000....000 and rb = sb = 0 */
      if (rnd_mode == MPFR_RNDN &&
          (ax < __gmpfr_emin - 1 ||
           (ap[0] == MPFR_LIMB_HIGHBIT && (rb | sb) == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN_POS);
    }

 rounding:
  MPFR_EXP (a) = ax; /* Don't use MPFR_SET_EXP since ax might be < __gmpfr_emin
                        in the cases "goto rounding" above. */
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(ax >= __gmpfr_emin);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & MPFR_LIMB_ONE) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ (rnd_mode, 0))
    {
    truncate:
      MPFR_ASSERTD(ax >= __gmpfr_emin);
      MPFR_RET(-MPFR_SIGN_POS);
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE;
      if (ap[0] == 0)
        {
          ap[0] = MPFR_LIMB_HIGHBIT;
          if (MPFR_UNLIKELY(ax + 1 > __gmpfr_emax))
            return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);
          MPFR_ASSERTD(ax + 1 <= __gmpfr_emax);
          MPFR_ASSERTD(ax + 1 >= __gmpfr_emin);
          MPFR_SET_EXP (a, ax + 1);
        }
      MPFR_RET(MPFR_SIGN_POS);
    }
}

/* Special code for GMP_NUMB_BITS < prec(a) < 2*GMP_NUMB_BITS and
   GMP_NUMB_BITS < prec(b) <= 2*GMP_NUMB_BITS.
   Note: this function was copied and optimized from mpfr_mul_2 in file mul.c,
   thus any change here should be done also in mpfr_mul_2, if applicable. */
static int
mpfr_sqr_2 (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode, mpfr_prec_t p)
{
  mp_limb_t h, l, u, v;
  mpfr_limb_ptr ap = MPFR_MANT(a);
  mpfr_exp_t ax = 2 * MPFR_GET_EXP(b);
  mpfr_prec_t sh = 2 * GMP_NUMB_BITS - p;
  mp_limb_t rb, sb, sb2, mask = MPFR_LIMB_MASK(sh);
  mp_limb_t *bp = MPFR_MANT(b);

  /* we store the 4-limb product in h=ap[1], l=ap[0], sb=ap[-1], sb2=ap[-2] */
  umul_ppmm (h, l, bp[1], bp[1]);
  umul_ppmm (u, v, bp[1], bp[0]);
  l += u << 1;
  h += (l < (u << 1)) + (u >> (GMP_NUMB_BITS - 1));

  /* now the full square is {h, l, 2*v + high(b0*c0), low(b0*c0)},
     where the lower part contributes to less than 3 ulps to {h, l} */

  /* If h has its most significant bit set and the low sh-1 bits of l are not
     000...000 nor 111...111 nor 111...110, then we can round correctly;
     if h has zero as most significant bit, we have to shift left h and l,
     thus if the low sh-2 bits are not 000...000 nor 111...111 nor 111...110,
     then we can round correctly. To avoid an extra test we consider the latter
     case (if we can round, we can also round in the former case).
     For sh <= 3, we have mask <= 7, thus (mask>>2) <= 1, and the approximation
     cannot be enough. */
  if (MPFR_LIKELY(((l + 2) & (mask >> 2)) > 2))
    sb = sb2 = 1; /* result cannot be exact in that case */
  else
    {
      mp_limb_t carry1, carry2;

      umul_ppmm (sb, sb2, bp[0], bp[0]);
      /* the full product is {h, l, sb + v + w, sb2} */
      ADD_LIMB (sb, v, carry1);
      ADD_LIMB (l, carry1, carry2);
      h += carry2;
      ADD_LIMB (sb, v, carry1);
      ADD_LIMB (l, carry1, carry2);
      h += carry2;
    }
  if (h < MPFR_LIMB_HIGHBIT)
    {
      ax --;
      h = (h << 1) | (l >> (GMP_NUMB_BITS - 1));
      l = (l << 1) | (sb >> (GMP_NUMB_BITS - 1));
      sb <<= 1;
      /* no need to shift sb2 since we only want to know if it is zero or not */
    }
  ap[1] = h;
  rb = l & (MPFR_LIMB_ONE << (sh - 1));
  sb |= ((l & mask) ^ rb) | sb2;
  ap[0] = l & ~mask;

  MPFR_SIGN(a) = MPFR_SIGN_POS;

  /* rounding */
  if (MPFR_UNLIKELY(ax > __gmpfr_emax))
    return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);

  /* Warning: underflow should be checked *after* rounding, thus when rounding
     away and when a > 0.111...111*2^(emin-1), or when rounding to nearest and
     a >= 0.111...111[1]*2^(emin-1), there is no underflow. */
  if (MPFR_UNLIKELY(ax < __gmpfr_emin))
    {
      /* Note: like for mpfr_sqr_1, the case
         0.111...111*2^(emin-1) < a < 2^(emin-1) is not possible when emin is
         odd, since (modulo a shift) this would imply 1-2^(-p) < a = b^2 < 1,
         and this is not possible with 1-2^(-p) <= b < 1.
         For emin even, it is possible for some values of p, for example for
         p=69 with b=417402170410649030795*2^k. */
      if (ax == __gmpfr_emin - 1 &&
          ap[1] == MPFR_LIMB_MAX &&
          ap[0] == ~mask &&
          ((rnd_mode == MPFR_RNDN && rb) ||
           (MPFR_IS_LIKE_RNDA (rnd_mode, 0) && (rb | sb))))
        goto rounding; /* no underflow */
      /* for RNDN, mpfr_underflow always rounds away, thus for
         |a| <= 2^(emin-2) we have to change to RNDZ */
      if (rnd_mode == MPFR_RNDN &&
          (ax < __gmpfr_emin - 1 ||
           (ap[1] == MPFR_LIMB_HIGHBIT && ap[0] == 0 && (rb | sb) == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN_POS);
    }

 rounding:
  MPFR_EXP (a) = ax; /* Don't use MPFR_SET_EXP since ax might be < __gmpfr_emin
                        in the cases "goto rounding" above. */
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(ax >= __gmpfr_emin);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ (rnd_mode, 0))
    {
    truncate:
      MPFR_ASSERTD(ax >= __gmpfr_emin);
      MPFR_RET(-MPFR_SIGN_POS);
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      ap[1] += (ap[0] == 0);
      if (ap[1] == 0)
        {
          ap[1] = MPFR_LIMB_HIGHBIT;
          if (MPFR_UNLIKELY(ax + 1 > __gmpfr_emax))
            return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);
          MPFR_ASSERTD(ax + 1 <= __gmpfr_emax);
          MPFR_ASSERTD(ax + 1 >= __gmpfr_emin);
          MPFR_SET_EXP (a, ax + 1);
        }
      MPFR_RET(MPFR_SIGN_POS);
    }
}

/* Special code for 2*GMP_NUMB_BITS < prec(a) < 3*GMP_NUMB_BITS and
   2*GMP_NUMB_BITS < prec(b) <= 3*GMP_NUMB_BITS. */
static int
mpfr_sqr_3 (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode, mpfr_prec_t p)
{
  mp_limb_t a0, a1, a2, h, l;
  mpfr_limb_ptr ap = MPFR_MANT(a);
  mpfr_exp_t ax = 2 * MPFR_GET_EXP(b);
  mpfr_prec_t sh = 3 * GMP_NUMB_BITS - p;
  mp_limb_t rb, sb, sb2, mask = MPFR_LIMB_MASK(sh);
  mp_limb_t *bp = MPFR_MANT(b);

  /* we store the upper 3-limb product in a2, a1, a0:
     b2^2, 2*b2*b1, 2*b2*b0+b1^2 */

  /* first compute b2*b1 and b2*b0, which will be shifted by 1 */
  umul_ppmm (a1, a0, bp[2], bp[1]);
  umul_ppmm (h, l, bp[2], bp[0]);
  a0 += h;
  a1 += (a0 < h);
  /* now a1, a0 contains b2*b1 + floor(b2*b0/B): there can be no overflow
     since b2*b1*B + b2*b0 <= b2*(b1*B+b0) <= b2*(B^2-1) < B^3 */

  /* multiply a2, a1, a0 by 2 */
  a2 = a1 >> (GMP_NUMB_BITS - 1);
  a1 = (a1 << 1) | (a0 >> (GMP_NUMB_BITS - 1));
  a0 = (a0 << 1);

  /* add b2^2 */
  umul_ppmm (h, l, bp[2], bp[2]);
  a1 += l;
  a2 += h + (a1 < l);

  /* add b1^2 */
  umul_ppmm (h, l, bp[1], bp[1]);
  a0 += h;
  a1 += (a0 < h);
  a2 += (a1 == 0 && a0 < h);

  /* Now the approximate product {a2, a1, a0} has an error of less than
     5 ulps (3 ulps for the ignored low limbs of 2*b2*b0+b1^2,
     plus 2 ulps for the ignored 2*b1*b0 (plus b0^2).
     Since we might shift by 1 bit, we make sure the low sh-2 bits of a0
     are not 0, -1, -2, -3 or -4. */

  if (MPFR_LIKELY(((a0 + 4) & (mask >> 2)) > 4))
    sb = sb2 = 1; /* result cannot be exact in that case */
  else
    {
      mp_limb_t t[6];
      mpn_sqr (t, bp, 3);
      a2 = t[5];
      a1 = t[4];
      a0 = t[3];
      sb = t[2];
      sb2 = t[1] | t[0];
    }
  if (a2 < MPFR_LIMB_HIGHBIT)
    {
      ax --;
      a2 = (a2 << 1) | (a1 >> (GMP_NUMB_BITS - 1));
      a1 = (a1 << 1) | (a0 >> (GMP_NUMB_BITS - 1));
      a0 = (a0 << 1) | (sb >> (GMP_NUMB_BITS - 1));
      sb <<= 1;
      /* no need to shift sb2: we only need to know if it is zero or not */
    }
  ap[2] = a2;
  ap[1] = a1;
  rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
  sb |= ((a0 & mask) ^ rb) | sb2;
  ap[0] = a0 & ~mask;

  MPFR_SIGN(a) = MPFR_SIGN_POS;

  /* rounding */
  if (MPFR_UNLIKELY(ax > __gmpfr_emax))
    return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);

  /* Warning: underflow should be checked *after* rounding, thus when rounding
     away and when a > 0.111...111*2^(emin-1), or when rounding to nearest and
     a >= 0.111...111[1]*2^(emin-1), there is no underflow. */
  if (MPFR_UNLIKELY(ax < __gmpfr_emin))
    {
      if (ax == __gmpfr_emin - 1 &&
          ap[2] == MPFR_LIMB_MAX &&
          ap[1] == MPFR_LIMB_MAX &&
          ap[0] == ~mask &&
          ((rnd_mode == MPFR_RNDN && rb) ||
           (MPFR_IS_LIKE_RNDA (rnd_mode, 0) && (rb | sb))))
        goto rounding; /* no underflow */
      /* for RNDN, mpfr_underflow always rounds away, thus for |a| <= 2^(emin-2)
         we have to change to RNDZ */
      if (rnd_mode == MPFR_RNDN &&
          (ax < __gmpfr_emin - 1 ||
           (ap[2] == MPFR_LIMB_HIGHBIT && ap[1] == 0 && ap[0] == 0
            && (rb | sb) == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN_POS);
    }

 rounding:
  MPFR_EXP (a) = ax; /* Don't use MPFR_SET_EXP since ax might be < __gmpfr_emin
                        in the cases "goto rounding" above. */
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(ax >= __gmpfr_emin);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ (rnd_mode, 0))
    {
    truncate:
      MPFR_ASSERTD(ax >= __gmpfr_emin);
      MPFR_RET(-MPFR_SIGN_POS);
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      ap[1] += (ap[0] == 0);
      ap[2] += (ap[1] == 0) && (ap[0] == 0);
      if (ap[2] == 0)
        {
          ap[2] = MPFR_LIMB_HIGHBIT;
          if (MPFR_UNLIKELY(ax + 1 > __gmpfr_emax))
            return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);
          MPFR_ASSERTD(ax + 1 <= __gmpfr_emax);
          MPFR_ASSERTD(ax + 1 >= __gmpfr_emin);
          MPFR_SET_EXP (a, ax + 1);
        }
      MPFR_RET(MPFR_SIGN_POS);
    }
}

#endif /* !defined(MPFR_GENERIC_ABI) && ... */

/* Note: mpfr_sqr will call mpfr_mul if bn > MPFR_SQR_THRESHOLD,
   in order to use Mulders' mulhigh, which is handled only here
   to avoid partial code duplication. There is some overhead due
   to the additional tests, but slowdown should not be noticeable
   as this code is not executed in very small precisions. */

int
mpfr_sqr (mpfr_ptr a, mpfr_srcptr b, mpfr_rnd_t rnd_mode)
{
  int cc, inexact;
  mpfr_exp_t ax;
  mp_limb_t *tmp;
  mp_limb_t b1;
  mpfr_prec_t aq, bq;
  mp_size_t bn, tn;
  MPFR_TMP_DECL(marker);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (b), mpfr_log_prec, b, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec (a), mpfr_log_prec, a, inexact));

  /* deal with special cases */
  if (MPFR_UNLIKELY(MPFR_IS_SINGULAR(b)))
    {
      if (MPFR_IS_NAN(b))
        {
          MPFR_SET_NAN(a);
          MPFR_RET_NAN;
        }
      MPFR_SET_POS (a);
      if (MPFR_IS_INF(b))
        MPFR_SET_INF(a);
      else
        ( MPFR_ASSERTD(MPFR_IS_ZERO(b)), MPFR_SET_ZERO(a) );
      MPFR_RET(0);
    }
  aq = MPFR_GET_PREC(a);
  bq = MPFR_GET_PREC(b);

#if !defined(MPFR_GENERIC_ABI) && (GMP_NUMB_BITS == 32 || GMP_NUMB_BITS == 64)
  if (aq == bq)
    {
      if (aq < GMP_NUMB_BITS)
        return mpfr_sqr_1 (a, b, rnd_mode, aq);

      if (GMP_NUMB_BITS < aq && aq < 2 * GMP_NUMB_BITS)
        return mpfr_sqr_2 (a, b, rnd_mode, aq);

      if (aq == GMP_NUMB_BITS)
        return mpfr_sqr_1n (a, b, rnd_mode);

      if (2 * GMP_NUMB_BITS < aq && aq < 3 * GMP_NUMB_BITS)
        return mpfr_sqr_3 (a, b, rnd_mode, aq);
    }
#endif

  ax = 2 * MPFR_GET_EXP (b);
  MPFR_ASSERTN (2 * (mpfr_uprec_t) bq <= MPFR_PREC_MAX);

  bn = MPFR_LIMB_SIZE (b); /* number of limbs of b */
  tn = MPFR_PREC2LIMBS (2 * bq); /* number of limbs of square,
                                    2*bn or 2*bn-1 */

  if (MPFR_UNLIKELY(bn > MPFR_SQR_THRESHOLD))
    /* the following line should not be replaced by mpfr_sqr,
       otherwise we'll get an infinite loop! */
    return mpfr_mul (a, b, b, rnd_mode);

  MPFR_TMP_MARK(marker);
  tmp = MPFR_TMP_LIMBS_ALLOC (2 * bn);

  /* Multiplies the mantissa in temporary allocated space */
  mpn_sqr (tmp, MPFR_MANT(b), bn);
  b1 = tmp[2 * bn - 1];

  /* now tmp[0]..tmp[2*bn-1] contains the product of both mantissa,
     with tmp[2*bn-1]>=2^(GMP_NUMB_BITS-2) */
  b1 >>= GMP_NUMB_BITS - 1; /* msb from the product */

  /* if the mantissas of b and c are uniformly distributed in ]1/2, 1],
     then their product is in ]1/4, 1/2] with probability 2*ln(2)-1 ~ 0.386
     and in [1/2, 1] with probability 2-2*ln(2) ~ 0.614 */
  tmp += 2 * bn - tn; /* +0 or +1 */
  if (MPFR_UNLIKELY(b1 == 0))
    mpn_lshift (tmp, tmp, tn, 1); /* tn <= k, so no stack corruption */

  cc = mpfr_round_raw (MPFR_MANT (a), tmp, 2 * bq, 0, aq, rnd_mode, &inexact);
  /* cc = 1 ==> result is a power of two */
  if (MPFR_UNLIKELY(cc))
    MPFR_MANT(a)[MPFR_LIMB_SIZE(a)-1] = MPFR_LIMB_HIGHBIT;

  MPFR_TMP_FREE(marker);
  {
    mpfr_exp_t ax2 = ax + ((int) b1 - 1 + cc);
    if (MPFR_UNLIKELY( ax2 > __gmpfr_emax))
      return mpfr_overflow (a, rnd_mode, MPFR_SIGN_POS);
    if (MPFR_UNLIKELY( ax2 < __gmpfr_emin))
      {
        /* In the rounding to the nearest mode, if the exponent of the exact
           result (i.e. before rounding, i.e. without taking cc into account)
           is < __gmpfr_emin - 1 or the exact result is a power of 2 (i.e. if
           both arguments are powers of 2), then round to zero. */
        if (rnd_mode == MPFR_RNDN &&
            (ax + (mpfr_exp_t) b1 < __gmpfr_emin || mpfr_powerof2_raw (b)))
          rnd_mode = MPFR_RNDZ;
        return mpfr_underflow (a, rnd_mode, MPFR_SIGN_POS);
      }
    MPFR_SET_EXP (a, ax2);
    MPFR_SET_POS (a);
  }
  MPFR_RET (inexact);
}
