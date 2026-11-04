/* mpfr_div -- divide two floating-point numbers

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

/* References:
   [1] Short Division of Long Integers, David Harvey and Paul Zimmermann,
       Proceedings of the 20th Symposium on Computer Arithmetic (ARITH-20),
       July 25-27, 2011, pages 7-14.
   [2] Improved Division by Invariant Integers, Niels Möller and Torbjörn Granlund,
       IEEE Transactions on Computers, volume 60, number 2, pages 165-175, 2011.
*/

#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"

#if !defined(MPFR_GENERIC_ABI)

#if GMP_NUMB_BITS == 64

#include "invert_limb.h"

/* Given u = u1*B+u0 < v = v1*B+v0 with v normalized (high bit of v1 set),
   put in q = Q1*B+Q0 an approximation of floor(u*B^2/v), with:
   B = 2^GMP_NUMB_BITS and q <= floor(u*B^2/v) <= q + 21.
   Note: this function requires __gmpfr_invert_limb_approx (from invert_limb.h)
   which is only provided so far for 64-bit limb.
   Note: __gmpfr_invert_limb_approx can be replaced by __gmpfr_invert_limb,
   in that case the bound 21 reduces to 16. */
static void
mpfr_div2_approx (mpfr_limb_ptr Q1, mpfr_limb_ptr Q0,
                  mp_limb_t u1, mp_limb_t u0,
                  mp_limb_t v1, mp_limb_t v0)
{
  mp_limb_t inv, q1, q0, r1, r0, cy, xx, yy;

  /* first compute an approximation of q1, using a lower approximation of
     B^2/(v1+1) - B */
  if (MPFR_UNLIKELY(v1 == MPFR_LIMB_MAX))
    inv = MPFR_LIMB_ZERO;
  else
    __gmpfr_invert_limb_approx (inv, v1 + 1);
  /* now inv <= B^2/(v1+1) - B */
  umul_ppmm (q1, q0, u1, inv);
  q1 += u1;
  /* now q1 <= u1*B/(v1+1) < (u1*B+u0)*B/(v1*B+v0) */

  /* compute q1*(v1*B+v0) into r1:r0:yy and subtract from u1:u0:0 */
  umul_ppmm (r1, r0, q1, v1);
  umul_ppmm (xx, yy, q1, v0);

  ADD_LIMB (r0, xx, cy);
  r1 += cy;

  /* we ignore yy below, but first increment r0, to ensure we get a lower
     approximation of the remainder */
  r0 += yy != 0;
  r1 += r0 == 0 && yy != 0;
  r0 = u0 - r0;
  r1 = u1 - r1 - (r0 > u0);

  /* r1:r0 should be non-negative */
  MPFR_ASSERTD((r1 & MPFR_LIMB_HIGHBIT) == 0);

  /* the second quotient limb is approximated by (r1*B^2+r0*B) / v1,
     and since (B+inv)/B approximates B/v1, this is in turn approximated
     by (r1*B+r0)*(B+inv)/B = r1*B*r1*inv+r0+(r0*inv/B) */

  q0 = r0;
  q1 += r1;
  /* add floor(r0*inv/B) to q0 */
  umul_ppmm (xx, yy, r0, inv);
  ADD_LIMB (q0, xx, cy);
  q1 += cy;
  MPFR_ASSERTD (r1 <= 4);
  /* TODO: use value coverage on r1 to check that the 5 cases are tested. */
  while (r1) /* the number of loops is at most 4 */
    {
      /* add inv to q0 */
      ADD_LIMB (q0, inv, cy);
      q1 += cy;
      r1 --;
    }

  *Q1 = q1;
  *Q0 = q0;
}

#endif /* GMP_NUMB_BITS == 64 */

/* Special code for PREC(q) = PREC(u) = PREC(v) = p < GMP_NUMB_BITS */
static int
mpfr_div_1 (mpfr_ptr q, mpfr_srcptr u, mpfr_srcptr v, mpfr_rnd_t rnd_mode)
{
  mpfr_prec_t p = MPFR_GET_PREC(q);
  mpfr_limb_ptr qp = MPFR_MANT(q);
  mpfr_exp_t qx = MPFR_GET_EXP(u) - MPFR_GET_EXP(v);
  mpfr_prec_t sh = GMP_NUMB_BITS - p;
  mp_limb_t u0 = MPFR_MANT(u)[0];
  mp_limb_t v0 = MPFR_MANT(v)[0];
  mp_limb_t q0, rb, sb, mask = MPFR_LIMB_MASK(sh);
  int extra;

  if ((extra = (u0 >= v0)))
    u0 -= v0;

#if GMP_NUMB_BITS == 64 /* __gmpfr_invert_limb_approx only exists for 64-bit */
  /* First try with an approximate quotient.
     FIXME: for p<=62 we have sh-1<2 and will never be able to round correctly.
     Even for p=61 we have sh-1=2 and we can round correctly only when the two
     last bist of q0 are 01, which happens with probability 25% only. */
  {
    mp_limb_t inv;
    __gmpfr_invert_limb_approx (inv, v0);
    umul_ppmm (rb, sb, u0, inv);
  }
  rb += u0;
  q0 = rb >> extra;
  /* rb does not exceed the true quotient floor(u0*2^GMP_NUMB_BITS/v0),
     with error at most 2, which means the rational quotient q satisfies
     rb <= q < rb + 3. We can round correctly except when the last sh-1 bits
     of q0 are 000..000 or 111..111 or 111..110. */
  if (MPFR_LIKELY(((q0 + 2) & (mask >> 1)) > 2))
    {
      rb = q0 & (MPFR_LIMB_ONE << (sh - 1));
      sb = 1; /* result cannot be exact in this case */
    }
  else /* the true quotient is rb, rb+1 or rb+2 */
    {
      mp_limb_t h, l;
      q0 = rb;
      umul_ppmm (h, l, q0, v0);
      MPFR_ASSERTD(h < u0 || (h == u0 && l == MPFR_LIMB_ZERO));
      /* subtract {h,l} from {u0,0} */
      sub_ddmmss (h, l, u0, 0, h, l);
      /* the remainder {h, l} should be < v0 */
      /* This while loop is executed at most two times, but does not seem
         slower than two consecutive identical if-statements. */
      while (h || l >= v0)
        {
          q0 ++;
          h -= (l < v0);
          l -= v0;
        }
      MPFR_ASSERTD(h == 0 && l < v0);
      sb = l | (q0 & extra);
      q0 >>= extra;
      rb = q0 & (MPFR_LIMB_ONE << (sh - 1));
      sb |= q0 & (mask >> 1);
    }
#else
  udiv_qrnnd (q0, sb, u0, 0, v0);
  sb |= q0 & extra;
  q0 >>= extra;
  rb = q0 & (MPFR_LIMB_ONE << (sh - 1));
  sb |= q0 & (mask >> 1);
#endif

  qp[0] = (MPFR_LIMB_HIGHBIT | q0) & ~mask;
  qx += extra;
  MPFR_SIGN(q) = MPFR_MULT_SIGN (MPFR_SIGN (u), MPFR_SIGN (v));

  /* rounding */
  if (MPFR_UNLIKELY(qx > __gmpfr_emax))
    return mpfr_overflow (q, rnd_mode, MPFR_SIGN(q));

  /* Warning: underflow should be checked *after* rounding, thus when rounding
     away and when q > 0.111...111*2^(emin-1), or when rounding to nearest and
     q >= 0.111...111[1]*2^(emin-1), there is no underflow. */
  if (MPFR_UNLIKELY(qx < __gmpfr_emin))
    {
      /* Note: the case 0.111...111*2^(emin-1) < q < 2^(emin-1) is not possible
         here since (up to exponent) this would imply 1 - 2^(-p) < u/v < 1,
         thus v - 2^(-p)*v < u < v, and since we can assume 1/2 <= v < 1, it
         would imply v - 2^(-p) = v - ulp(v) < u < v, which has no solution. */

      /* For RNDN, mpfr_underflow always rounds away, thus for |q|<=2^(emin-2)
         we have to change to RNDZ. This corresponds to:
         (a) either qx < emin - 1
         (b) or qx = emin - 1 and qp[0] = 1000....000 and rb = sb = 0.
         Note: in case (b), it suffices to check whether sb = 0, since rb = 1
         and sb = 0 is not possible (the exact quotient would have p+1 bits,
         thus u would need at least p+1 bits). */
      if (rnd_mode == MPFR_RNDN &&
          (qx < __gmpfr_emin - 1 || (qp[0] == MPFR_LIMB_HIGHBIT && sb == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (q, rnd_mode, MPFR_SIGN(q));
    }

  MPFR_EXP (q) = qx; /* Don't use MPFR_SET_EXP since qx might be < __gmpfr_emin
                        in the cases "goto rounding" above. */
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(qx >= __gmpfr_emin);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      /* It is not possible to have rb <> 0 and sb = 0 here, since it would
         mean a n-bit by n-bit division gives an exact (n+1)-bit number.
         And since the case rb = sb = 0 was already dealt with, we cannot
         have sb = 0. Thus we cannot be in the middle of two numbers. */
      MPFR_ASSERTD(sb != 0);
      if (rb == 0)
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(q)))
    {
    truncate:
      MPFR_ASSERTD(qx >= __gmpfr_emin);
      MPFR_RET(-MPFR_SIGN(q));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      qp[0] += MPFR_LIMB_ONE << sh;
      MPFR_ASSERTD(qp[0] != 0);
      /* It is not possible to have an overflow in the addition above.
         Proof: if p is the precision of the inputs, it would mean we have two
         integers n and d with 2^(p-1) <= n, d < 2^p, such that the binary
         expansion of n/d starts with p '1', and has at least one '1' later.
         We distinguish two cases:
         (1) if n/d < 1, it would mean 1-2^(-p) < n/d < 1
         (2) if n/d >= 1, it would mean 2-2^(1-p) < n/d < 1
         In case (1), multiplying by d we get 1-d/2^p < n < d,
         which has no integer solution since d/2^p < 1.
         In case (2), multiplying by d we get 2d-2d/2^p < n < 2d:
         (2a) if d=2^(p-1), we get 2^p-1 < n < 2^p which has no solution;
              if d>=2^(p-1)+1, then 2d-2d/2^p >= 2^p+2-2 = 2^p, thus there is
              solution n < 2^p either. */
      MPFR_RET(MPFR_SIGN(q));
    }
}

/* Special code for PREC(q) = GMP_NUMB_BITS,
   with PREC(u), PREC(v) <= GMP_NUMB_BITS. */
static int
mpfr_div_1n (mpfr_ptr q, mpfr_srcptr u, mpfr_srcptr v, mpfr_rnd_t rnd_mode)
{
  mpfr_limb_ptr qp = MPFR_MANT(q);
  mpfr_exp_t qx = MPFR_GET_EXP(u) - MPFR_GET_EXP(v);
  mp_limb_t u0 = MPFR_MANT(u)[0];
  mp_limb_t v0 = MPFR_MANT(v)[0];
  mp_limb_t q0, rb, sb, l;
  int extra;

  MPFR_ASSERTD(MPFR_PREC(q) == GMP_NUMB_BITS);
  MPFR_ASSERTD(MPFR_PREC(u) <= GMP_NUMB_BITS);
  MPFR_ASSERTD(MPFR_PREC(v) <= GMP_NUMB_BITS);

  if ((extra = (u0 >= v0)))
    u0 -= v0;

#if GMP_NUMB_BITS == 64 /* __gmpfr_invert_limb_approx only exists for 64-bit */
  {
    mp_limb_t inv, h;

    /* First compute an approximate quotient. */
    __gmpfr_invert_limb_approx (inv, v0);
    umul_ppmm (rb, sb, u0, inv);
    q0 = u0 + rb;
    /* rb does not exceed the true quotient floor(u0*2^GMP_NUMB_BITS/v0),
       with error at most 2, which means the rational quotient q satisfies
       rb <= q < rb + 3, thus the true quotient is rb, rb+1 or rb+2 */
    umul_ppmm (h, l, q0, v0);
    MPFR_ASSERTD(h < u0 || (h == u0 && l == MPFR_LIMB_ZERO));
    /* subtract {h,l} from {u0,0} */
    sub_ddmmss (h, l, u0, 0, h, l);
    /* the remainder {h, l} should be < v0 */
    /* This while loop is executed at most two times, but does not seem
       slower than two consecutive identical if-statements. */
    while (h || l >= v0)
      {
        q0 ++;
        h -= (l < v0);
        l -= v0;
      }
    MPFR_ASSERTD(h == 0 && l < v0);
  }
#else
  udiv_qrnnd (q0, l, u0, 0, v0);
#endif

  /* now (u0 - extra*v0) * 2^GMP_NUMB_BITS = q0*v0 + l with 0 <= l < v0 */

  /* If extra=0, the quotient is q0, the round bit is 1 if l >= v0/2,
     and sb are the remaining bits from l.
     If extra=1, the quotient is MPFR_LIMB_HIGHBIT + (q0 >> 1), the round bit
     is the least significant bit of q0, and sb is l. */

  if (extra == 0)
    {
      qp[0] = q0;
      /* If "l + l < l", then there is a carry in l + l, thus 2*l > v0.
         Otherwise if there is no carry, we check whether 2*l >= v0. */
      rb = (l + l < l) || (l + l >= v0);
      sb = (rb) ? l + l - v0 : l;
    }
  else
    {
      qp[0] = MPFR_LIMB_HIGHBIT | (q0 >> 1);
      rb = q0 & MPFR_LIMB_ONE;
      sb = l;
      qx ++;
    }

  MPFR_SIGN(q) = MPFR_MULT_SIGN (MPFR_SIGN (u), MPFR_SIGN (v));

  /* rounding */
  if (MPFR_UNLIKELY(qx > __gmpfr_emax))
    return mpfr_overflow (q, rnd_mode, MPFR_SIGN(q));

  /* Warning: underflow should be checked *after* rounding, thus when rounding
     away and when q > 0.111...111*2^(emin-1), or when rounding to nearest and
     q >= 0.111...111[1]*2^(emin-1), there is no underflow. */
  if (MPFR_UNLIKELY(qx < __gmpfr_emin))
    {
      /* Note: the case 0.111...111*2^(emin-1) < q < 2^(emin-1) is not possible
         here since (up to exponent) this would imply 1 - 2^(-p) < u/v < 1,
         thus v - 2^(-p)*v < u < v, and since we can assume 1/2 <= v < 1, it
         would imply v - 2^(-p) = v - ulp(v) < u < v, which has no solution. */

      /* For RNDN, mpfr_underflow always rounds away, thus for |q|<=2^(emin-2)
         we have to change to RNDZ. This corresponds to:
         (a) either qx < emin - 1
         (b) or qx = emin - 1 and qp[0] = 1000....000 and rb = sb = 0.
         Note: in case (b), it suffices to check whether sb = 0, since rb = 1
         and sb = 0 is not possible (the exact quotient would have p+1 bits,
         thus u would need at least p+1 bits). */
      if (rnd_mode == MPFR_RNDN &&
          (qx < __gmpfr_emin - 1 || (qp[0] == MPFR_LIMB_HIGHBIT && sb == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (q, rnd_mode, MPFR_SIGN(q));
    }

  MPFR_EXP (q) = qx; /* Don't use MPFR_SET_EXP since qx might be < __gmpfr_emin
                        in the cases "goto rounding" above. */
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(qx >= __gmpfr_emin);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      /* It is not possible to have rb <> 0 and sb = 0 here, since it would
         mean a n-bit by n-bit division gives an exact (n+1)-bit number.
         And since the case rb = sb = 0 was already dealt with, we cannot
         have sb = 0. Thus we cannot be in the middle of two numbers. */
      MPFR_ASSERTD(sb != 0);
      if (rb == 0)
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(q)))
    {
    truncate:
      MPFR_ASSERTD(qx >= __gmpfr_emin);
      MPFR_RET(-MPFR_SIGN(q));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      qp[0] += MPFR_LIMB_ONE;
      /* there can be no overflow in the addition above,
         see the analysis of mpfr_div_1 */
      MPFR_ASSERTD(qp[0] != 0);
      MPFR_RET(MPFR_SIGN(q));
    }
}

/* Special code for GMP_NUMB_BITS < PREC(q) < 2*GMP_NUMB_BITS and
   PREC(u) = PREC(v) = PREC(q) */
static int
mpfr_div_2 (mpfr_ptr q, mpfr_srcptr u, mpfr_srcptr v, mpfr_rnd_t rnd_mode)
{
  mpfr_prec_t p = MPFR_GET_PREC(q);
  mpfr_limb_ptr qp = MPFR_MANT(q);
  mpfr_exp_t qx = MPFR_GET_EXP(u) - MPFR_GET_EXP(v);
  mpfr_prec_t sh = 2*GMP_NUMB_BITS - p;
  mp_limb_t h, rb, sb, mask = MPFR_LIMB_MASK(sh);
  mp_limb_t v1 = MPFR_MANT(v)[1], v0 = MPFR_MANT(v)[0];
  mp_limb_t q1, q0, r3, r2, r1, r0, l, t;
  int extra;

  r3 = MPFR_MANT(u)[1];
  r2 = MPFR_MANT(u)[0];
  extra = r3 > v1 || (r3 == v1 && r2 >= v0);
  if (extra)
    sub_ddmmss (r3, r2, r3, r2, v1, v0);

  MPFR_ASSERTD(r3 < v1 || (r3 == v1 && r2 < v0));

#if GMP_NUMB_BITS == 64
  mpfr_div2_approx (&q1, &q0, r3, r2, v1, v0);
  /* we know q1*B+q0 is smaller or equal to the exact quotient, with
     difference at most 21 */
  if (MPFR_LIKELY(((q0 + 21) & (mask >> 1)) > 21))
    sb = 1; /* result is not exact when we can round with an approximation */
  else
    {
      /* we know q1:q0 is a good-enough approximation, use it! */
      mp_limb_t s0, s1, s2, h, l;

      /* Since we know the difference should be at most 21*(v1:v0) after the
         subtraction below, thus at most 21*2^128, it suffices to compute the
         lower 3 limbs of (q1:q0) * (v1:v0). */
      umul_ppmm (s1, s0, q0, v0);
      umul_ppmm (s2, l, q0, v1);
      s1 += l;
      s2 += (s1 < l);
      umul_ppmm (h, l, q1, v0);
      s1 += l;
      s2 += h + (s1 < l);
      s2 += q1 * v1;
      /* Subtract s2:s1:s0 from r2:0:0, with result in s2:s1:s0. */
      s2 = r2 - s2;
      /* now negate s1:s0 */
      s0 = -s0;
      s1 = -s1 - (s0 != 0);
      /* there is a borrow in s2 when s0 and s1 are not both zero */
      s2 -= (s1 != 0 || s0 != 0);
      while (s2 > 0 || (s1 > v1) || (s1 == v1 && s0 >= v0))
        {
          /* add 1 to q1:q0 */
          q0 ++;
          q1 += (q0 == 0);
          /* subtract v1:v0 to s2:s1:s0 */
          s2 -= (s1 < v1) || (s1 == v1 && s0 < v0);
          sub_ddmmss (s1, s0, s1, s0, v1, v0);
        }
      sb = s1 | s0;
    }
  goto round_div2;
#endif

  /* now r3:r2 < v1:v0 */
  if (MPFR_UNLIKELY(r3 == v1)) /* can occur in some rare cases */
    {
      /* This can only occur in case extra=0, since otherwise we would have
         u_old >= u_new + v >= B^2/2 + B^2/2 = B^2. In this case we have
         r3 = u1 and r2 = u0, thus the remainder u*B-q1*v is
         v1*B^2+u0*B-(B-1)*(v1*B+v0) = (u0-v0+v1)*B+v0.
         Warning: in this case q1 = B-1 can be too large, for example with
         u = B^2/2 and v = B^2/2 + B - 1, then u*B-(B-1)*u = -1/2*B^2+2*B-1. */
      MPFR_ASSERTD(extra == 0);
      q1 = MPFR_LIMB_MAX;
      r1 = v0;
      t = v0 - r2; /* t > 0 since r3:r2 < v1:v0 */
      r2 = v1 - t;
      if (t > v1) /* q1 = B-1 is too large, we need q1 = B-2, which is ok
                        since u*B - q1*v >= v1*B^2-(B-2)*(v1*B+B-1) =
                        -B^2 + 2*B*v1 + 3*B - 2 >= 0 since v1>=B/2 and B>=2 */
        {
          q1 --;
          /* add v to r2:r1 */
          r1 += v0;
          r2 += v1 + (r1 < v0);
        }
    }
  else
    {
      /* divide r3:r2 by v1: requires r3 < v1 */
      udiv_qrnnd (q1, r2, r3, r2, v1);
      /* u-extra*v = q1 * v1 + r2 */

      /* now subtract q1*v0 to r2:0 */
      umul_ppmm (h, l, q1, v0);
      t = r2; /* save old value of r2 */
      r1 = -l;
      r2 -= h + (l != 0);
      /* Note: h + (l != 0) < 2^GMP_NUMB_BITS. */

      /* we have r2:r1 = oldr2:0 - q1*v0 mod 2^(2*GMP_NUMB_BITS)
         thus (u-extra*v)*B = q1 * v + r2:r1 mod 2^(2*GMP_NUMB_BITS) */

      /* this while loop should be run at most twice */
      while (r2 > t) /* borrow when subtracting h + (l != 0), q1 too large */
        {
          q1 --;
          /* add v1:v0 to r2:r1 */
          t = r2;
          r1 += v0;
          r2 += v1 + (r1 < v0);
          /* note: since 2^(GMP_NUMB_BITS-1) <= v1 + (r1 < v0)
             <= 2^GMP_NUMB_BITS, it suffices to check if r2 <= t to see
             if there was a carry or not. */
        }
    }

  /* now (u-extra*v)*B = q1 * v + r2:r1 with 0 <= r2:r1 < v */

  MPFR_ASSERTD(r2 < v1 || (r2 == v1 && r1 < v0));

  if (MPFR_UNLIKELY(r2 == v1))
    {
      q0 = MPFR_LIMB_MAX;
      /* r2:r1:0 - q0*(v1:v0) = v1:r1:0 - (B-1)*(v1:v0)
         = r1:0 - v0:0 + v1:v0 */
      r0 = v0;
      t = v0 - r1; /* t > 0 since r2:r1 < v1:v0 */
      r1 = v1 - t;
      if (t > v1)
        {
          q0 --;
          /* add v to r1:r0 */
          r0 += v0;
          r1 += v1 + (r0 < v0);
        }
    }
  else
    {
      /* divide r2:r1 by v1: requires r2 < v1 */
      udiv_qrnnd (q0, r1, r2, r1, v1);

      /* r2:r1 = q0*v1 + r1 */

      /* subtract q0*v0 to r1:0 */
      umul_ppmm (h, l, q0, v0);
      t = r1;
      r0 = -l;
      r1 -= h + (l != 0);

      /* this while loop should be run at most twice */
      while (r1 > t) /* borrow when subtracting h + (l != 0),
                        q0 was too large */
        {
          q0 --;
          /* add v1:v0 to r1:r0 */
          t = r1;
          r0 += v0;
          r1 += v1 + (r0 < v0);
          /* note: since 2^(GMP_NUMB_BITS-1) <= v1 + (r0 < v0)
             <= 2^GMP_NUMB_BITS, it suffices to check if r1 <= t to see
             if there was a carry or not. */
        }
    }

  MPFR_ASSERTD(r1 < v1 || (r1 == v1 && r0 < v0));

  /* now (u-extra*v)*B^2 = (q1:q0) * v + r1:r0 */

  sb = r1 | r0;

  /* here, q1:q0 should be an approximation of the quotient (or the exact
     quotient), and sb the sticky bit */

#if GMP_NUMB_BITS == 64
 round_div2:
#endif
  if (extra)
    {
      qx ++;
      sb |= q0 & 1;
      q0 = (q1 << (GMP_NUMB_BITS - 1)) | (q0 >> 1);
      q1 = MPFR_LIMB_HIGHBIT | (q1 >> 1);
    }
  rb = q0 & (MPFR_LIMB_ONE << (sh - 1));
  sb |= (q0 & mask) ^ rb;
  qp[1] = q1;
  qp[0] = q0 & ~mask;

  MPFR_SIGN(q) = MPFR_MULT_SIGN (MPFR_SIGN (u), MPFR_SIGN (v));

  /* rounding */
  if (qx > __gmpfr_emax)
    return mpfr_overflow (q, rnd_mode, MPFR_SIGN(q));

  /* Warning: underflow should be checked *after* rounding, thus when rounding
     away and when q > 0.111...111*2^(emin-1), or when rounding to nearest and
     q >= 0.111...111[1]*2^(emin-1), there is no underflow. */
  if (qx < __gmpfr_emin)
    {
      /* Note: the case 0.111...111*2^(emin-1) < q < 2^(emin-1) is not possible
         here since (up to exponent) this would imply 1 - 2^(-p) < u/v < 1,
         thus v - 2^(-p)*v < u < v, and since we can assume 1/2 <= v < 1, it
         would imply v - 2^(-p) = v - ulp(v) < u < v, which has no solution. */

      /* For RNDN, mpfr_underflow always rounds away, thus for |q|<=2^(emin-2)
         we have to change to RNDZ. This corresponds to:
         (a) either qx < emin - 1
         (b) or qx = emin - 1 and qp[1] = 100....000, qp[0] = 0 and rb = sb = 0.
         Note: in case (b), it suffices to check whether sb = 0, since rb = 1
         and sb = 0 is not possible (the exact quotient would have p+1 bits, thus
         u would need at least p+1 bits). */
      if (rnd_mode == MPFR_RNDN &&
          (qx < __gmpfr_emin - 1 ||
           (qp[1] == MPFR_LIMB_HIGHBIT && qp[0] == MPFR_LIMB_ZERO && sb == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (q, rnd_mode, MPFR_SIGN(q));
    }

  MPFR_EXP (q) = qx; /* Don't use MPFR_SET_EXP since qx might be < __gmpfr_emin
                        in the cases "goto rounding" above. */
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(qx >= __gmpfr_emin);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      /* See the comment in mpfr_div_1. */
      MPFR_ASSERTD(sb != 0);
      if (rb == 0)
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(q)))
    {
    truncate:
      MPFR_ASSERTD(qx >= __gmpfr_emin);
      MPFR_RET(-MPFR_SIGN(q));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      qp[0] += MPFR_LIMB_ONE << sh;
      qp[1] += qp[0] == 0;
      /* there can be no overflow in the addition above,
         see the analysis of mpfr_div_1 */
      MPFR_ASSERTD(qp[1] != 0);
      MPFR_RET(MPFR_SIGN(q));
    }
}

#endif /* !defined(MPFR_GENERIC_ABI) */

/* check if {ap, an} is zero */
static int
mpfr_mpn_cmpzero (mpfr_limb_ptr ap, mp_size_t an)
{
  MPFR_ASSERTD (an >= 0);
  while (an > 0)
    if (MPFR_LIKELY(ap[--an] != MPFR_LIMB_ZERO))
      return 1;
  return 0;
}

/* compare {ap, an} and {bp, bn} >> extra,
   aligned by the more significant limbs.
   Takes into account bp[0] for extra=1.
*/
static int
mpfr_mpn_cmp_aux (mpfr_limb_ptr ap, mp_size_t an,
                  mpfr_limb_ptr bp, mp_size_t bn, int extra)
{
  int cmp = 0;
  mp_size_t k;
  mp_limb_t bb;

  MPFR_ASSERTD (an >= 0);
  MPFR_ASSERTD (bn >= 0);
  MPFR_ASSERTD (extra == 0 || extra == 1);

  if (an >= bn)
    {
      k = an - bn;
      while (cmp == 0 && bn > 0)
        {
          bn --;
          bb = (extra) ? ((bp[bn+1] << (GMP_NUMB_BITS - 1)) | (bp[bn] >> 1))
            : bp[bn];
          cmp = (ap[k + bn] > bb) ? 1 : ((ap[k + bn] < bb) ? -1 : 0);
        }
      bb = (extra) ? bp[0] << (GMP_NUMB_BITS - 1) : MPFR_LIMB_ZERO;
      while (cmp == 0 && k > 0)
        {
          k--;
          cmp = (ap[k] > bb) ? 1 : ((ap[k] < bb) ? -1 : 0);
          bb = MPFR_LIMB_ZERO; /* ensure we consider only once bp[0] & 1 */
        }
      if (cmp == 0 && bb != MPFR_LIMB_ZERO)
        cmp = -1;
    }
  else /* an < bn */
    {
      k = bn - an;
      while (cmp == 0 && an > 0)
        {
          an --;
          bb = (extra) ? ((bp[k+an+1] << (GMP_NUMB_BITS - 1)) | (bp[k+an] >> 1))
            : bp[k+an];
          if (ap[an] > bb)
            cmp = 1;
          else if (ap[an] < bb)
            cmp = -1;
        }
      while (cmp == 0 && k > 0)
        {
          k--;
          bb = (extra) ? ((bp[k+1] << (GMP_NUMB_BITS - 1)) | (bp[k] >> 1))
            : bp[k];
          cmp = (bb != MPFR_LIMB_ZERO) ? -1 : 0;
        }
      if (cmp == 0 && extra && (bp[0] & MPFR_LIMB_ONE))
        cmp = -1;
    }
  return cmp;
}

/* {ap, n} <- {ap, n} - {bp, n} >> extra - cy, with cy = 0 or 1.
   Return borrow out.
*/
static mp_limb_t
mpfr_mpn_sub_aux (mpfr_limb_ptr ap, mpfr_limb_ptr bp, mp_size_t n,
                  mp_limb_t cy, int extra)
{
  mp_limb_t bb, rp;

  MPFR_ASSERTD (cy <= 1);
  MPFR_ASSERTD (n >= 0);

  while (n--)
    {
      bb = (extra) ? (MPFR_LIMB_LSHIFT(bp[1],GMP_NUMB_BITS-1) | (bp[0] >> 1)) : bp[0];
      rp = ap[0] - bb - cy;
      cy = (ap[0] < bb) || (cy && rp == MPFR_LIMB_MAX) ?
        MPFR_LIMB_ONE : MPFR_LIMB_ZERO;
      ap[0] = rp;
      ap ++;
      bp ++;
    }
  MPFR_ASSERTD (cy <= 1);
  return cy;
}

MPFR_HOT_FUNCTION_ATTR int
mpfr_div (mpfr_ptr q, mpfr_srcptr u, mpfr_srcptr v, mpfr_rnd_t rnd_mode)
{
  mp_size_t q0size, usize, vsize;
  mp_size_t qsize; /* number of limbs wanted for the computed quotient */
  mp_size_t qqsize;
  mp_size_t k;
  mpfr_limb_ptr q0p, qp;
  mpfr_limb_ptr up, vp;
  mpfr_limb_ptr ap;
  mpfr_limb_ptr bp;
  mp_limb_t qh;
  mp_limb_t sticky_u, sticky_v;
  mp_limb_t low_u;
  mp_limb_t sticky;
  mp_limb_t sticky3;
  mp_limb_t round_bit;
  mpfr_exp_t qexp;
  int sign_quotient;
  int extra_bit;
  int sh, sh2;
  int inex;
  int like_rndz;
  MPFR_TMP_DECL(marker);

  MPFR_LOG_FUNC (
    ("u[%Pd]=%.*Rg v[%Pd]=%.*Rg rnd=%d",
     mpfr_get_prec(u), mpfr_log_prec, u,
     mpfr_get_prec (v),mpfr_log_prec, v, rnd_mode),
    ("q[%Pd]=%.*Rg inexact=%d", mpfr_get_prec(q), mpfr_log_prec, q, inex));

  /**************************************************************************
   *                                                                        *
   *              This part of the code deals with special cases            *
   *                                                                        *
   **************************************************************************/

  if (MPFR_UNLIKELY(MPFR_ARE_SINGULAR(u,v)))
    {
      if (MPFR_IS_NAN(u) || MPFR_IS_NAN(v))
        {
          MPFR_SET_NAN(q);
          MPFR_RET_NAN;
        }
      sign_quotient = MPFR_MULT_SIGN(MPFR_SIGN(u), MPFR_SIGN(v));
      MPFR_SET_SIGN(q, sign_quotient);
      if (MPFR_IS_INF(u))
        {
          if (MPFR_IS_INF(v))
            {
              MPFR_SET_NAN(q);
              MPFR_RET_NAN;
            }
          else
            {
              MPFR_SET_INF(q);
              MPFR_RET(0);
            }
        }
      else if (MPFR_IS_INF(v))
        {
          MPFR_SET_ZERO (q);
          MPFR_RET (0);
        }
      else if (MPFR_IS_ZERO (v))
        {
          if (MPFR_IS_ZERO (u))
            {
              MPFR_SET_NAN(q);
              MPFR_RET_NAN;
            }
          else
            {
              MPFR_ASSERTD (! MPFR_IS_INF (u));
              MPFR_SET_INF(q);
              MPFR_SET_DIVBY0 ();
              MPFR_RET(0);
            }
        }
      else
        {
          MPFR_ASSERTD (MPFR_IS_ZERO (u));
          MPFR_SET_ZERO (q);
          MPFR_RET (0);
        }
    }

  /* When MPFR_GENERIC_ABI is defined, we don't use special code. */
#if !defined(MPFR_GENERIC_ABI)
  if (MPFR_GET_PREC(u) == MPFR_GET_PREC(q) &&
      MPFR_GET_PREC(v) == MPFR_GET_PREC(q))
    {
      if (MPFR_GET_PREC(q) < GMP_NUMB_BITS)
        return mpfr_div_1 (q, u, v, rnd_mode);

      if (GMP_NUMB_BITS < MPFR_GET_PREC(q) &&
          MPFR_GET_PREC(q) < 2 * GMP_NUMB_BITS)
        return mpfr_div_2 (q, u, v, rnd_mode);

      if (MPFR_GET_PREC(q) == GMP_NUMB_BITS)
        return mpfr_div_1n (q, u, v, rnd_mode);
    }
#endif /* !defined(MPFR_GENERIC_ABI) */

  usize = MPFR_LIMB_SIZE(u);
  vsize = MPFR_LIMB_SIZE(v);
  q0size = MPFR_LIMB_SIZE(q); /* number of limbs of destination */
  q0p = MPFR_MANT(q);
  up = MPFR_MANT(u);
  vp = MPFR_MANT(v);
  sticky_u = MPFR_LIMB_ZERO;
  sticky_v = MPFR_LIMB_ZERO;
  round_bit = MPFR_LIMB_ZERO;

  /**************************************************************************
   *                                                                        *
   *              End of the part concerning special values.                *
   *                                                                        *
   **************************************************************************/

  /* When the divisor has one limb and MPFR_LONG_WITHIN_LIMB is defined,
     we can use mpfr_div_ui, which should be faster, assuming there is no
     intermediate overflow or underflow.
     The divisor interpreted as an integer satisfies
     2^(GMP_NUMB_BITS-1) <= vm < 2^GMP_NUMB_BITS, thus the quotient
     satisfies 2^(EXP(u)-1-GMP_NUMB_BITS) < u/vm < 2^(EXP(u)-GMP_NUMB_BITS+1)
     and its exponent is either EXP(u)-GMP_NUMB_BITS or one more. */
#ifdef MPFR_LONG_WITHIN_LIMB
  if (vsize <= 1 && __gmpfr_emin <= MPFR_EXP(u) - GMP_NUMB_BITS
      && MPFR_EXP(u) - GMP_NUMB_BITS + 1 <= __gmpfr_emax
      && vp[0] <= ULONG_MAX)
    {
      mpfr_exp_t exp_v = MPFR_EXP(v); /* save it in case q=v */
      if (MPFR_IS_POS (v))
        inex = mpfr_div_ui (q, u, vp[0], rnd_mode);
      else
        {
          inex = -mpfr_div_ui (q, u, vp[0], MPFR_INVERT_RND(rnd_mode));
          MPFR_CHANGE_SIGN(q);
        }
      /* q did not under/overflow */
      MPFR_EXP(q) -= exp_v;
      /* The following test is needed, otherwise the next addition
         on the exponent may overflow, e.g. when dividing the
         largest finite MPFR number by the smallest positive one. */
      if (MPFR_UNLIKELY (MPFR_EXP(q) > __gmpfr_emax - GMP_NUMB_BITS))
        return mpfr_overflow (q, rnd_mode, MPFR_SIGN(q));
      MPFR_EXP(q) += GMP_NUMB_BITS;
      return mpfr_check_range (q, inex, rnd_mode);
    }
#endif

  MPFR_TMP_MARK(marker);

  /* set sign */
  sign_quotient = MPFR_MULT_SIGN(MPFR_SIGN(u), MPFR_SIGN(v));
  MPFR_SET_SIGN(q, sign_quotient);

  /* determine if an extra bit comes from the division, i.e. if the
     significand of u (as a fraction in [1/2, 1[) is larger than that
     of v */
  if (MPFR_LIKELY(up[usize - 1] != vp[vsize - 1]))
    extra_bit = (up[usize - 1] > vp[vsize - 1]) ? 1 : 0;
  else /* most significant limbs are equal, must look at further limbs */
    {
      mp_size_t l;

      k = usize - 1;
      l = vsize - 1;
      while (k != 0 && l != 0 && up[--k] == vp[--l]);
      /* now k=0 or l=0 or up[k] != vp[l] */
      if (up[k] != vp[l])
        extra_bit = (up[k] > vp[l]);
      /* now up[k] = vp[l], thus either k=0 or l=0 */
      else if (l == 0) /* no more divisor limb */
        extra_bit = 1;
      else /* k=0: no more dividend limb */
        extra_bit = mpfr_mpn_cmpzero (vp, l) == 0;
    }

  /* set exponent */
  qexp = MPFR_GET_EXP (u) - MPFR_GET_EXP (v) + extra_bit;

  /* sh is the number of zero bits in the low limb of the quotient */
  MPFR_UNSIGNED_MINUS_MODULO(sh, MPFR_PREC(q));

  like_rndz = rnd_mode == MPFR_RNDZ ||
    rnd_mode == (sign_quotient < 0 ? MPFR_RNDU : MPFR_RNDD);

  /**************************************************************************
   *                                                                        *
   *       We first try Mulders' short division (for large operands)        *
   *                                                                        *
   **************************************************************************/

  if (MPFR_UNLIKELY(q0size >= MPFR_DIV_THRESHOLD &&
                    vsize >= MPFR_DIV_THRESHOLD))
    {
      mp_size_t n = q0size + 1; /* we will perform a short (2n)/n division */
      mpfr_limb_ptr ap, bp, qp;
      mpfr_prec_t p;

      /* since Mulders' short division clobbers the dividend, we have to
         copy it */
      ap = MPFR_TMP_LIMBS_ALLOC (n + n);
      if (usize >= n + n) /* truncate the dividend */
        MPN_COPY(ap, up + usize - (n + n), n + n);
      else                /* zero-pad the dividend */
        {
          MPN_COPY(ap + (n + n) - usize, up, usize);
          MPN_ZERO(ap, (n + n) - usize);
        }

      if (vsize >= n) /* truncate the divisor */
        bp = vp + vsize - n;
      else            /* zero-pad the divisor */
        {
          bp = MPFR_TMP_LIMBS_ALLOC (n);
          MPN_COPY(bp + n - vsize, vp, vsize);
          MPN_ZERO(bp, n - vsize);
        }

      qp = MPFR_TMP_LIMBS_ALLOC (n);
      /* since n = q0size + 1, we have n >= 2 here */
      qh = mpfr_divhigh_n (qp, ap, bp, n);
      MPFR_ASSERTD (qh == 0 || qh == 1);
      /* in all cases, the error is at most (2n+2) ulps on qh*B^n+{qp,n},
         cf algorithms.tex */

      p = n * GMP_NUMB_BITS - MPFR_INT_CEIL_LOG2 (2 * n + 2);
      /* If rnd=RNDN, we need to be able to round with a directed rounding
         and one more bit. */
      if (qh == 1)
        {
          mpn_rshift (qp, qp, n, 1);
          qp[n - 1] |= MPFR_LIMB_HIGHBIT;
        }
      if (MPFR_LIKELY (mpfr_round_p (qp, n, p,
                                     MPFR_PREC(q) + (rnd_mode == MPFR_RNDN))))
        {
          /* we can round correctly whatever the rounding mode */
          MPN_COPY (q0p, qp + 1, q0size);
          q0p[0] &= ~MPFR_LIMB_MASK(sh); /* put to zero low sh bits */

          if (rnd_mode == MPFR_RNDN) /* round to nearest */
            {
              /* we know we can round, thus we are never in the even rule case:
                 if the round bit is 0, we truncate
                 if the round bit is 1, we add 1 */
              if (sh > 0)
                round_bit = (qp[1] >> (sh - 1)) & 1;
              else
                round_bit = qp[0] >> (GMP_NUMB_BITS - 1);
              /* TODO: add value coverage tests in tdiv to check that
                 we reach this part with different values of qh and
                 round_bit (4 cases). */
              if (round_bit == 0)
                {
                  inex = -1;
                  goto truncate;
                }
              else /* round_bit = 1 */
                goto add_one_ulp;
            }
          else if (! like_rndz) /* round away */
            goto add_one_ulp;
          else /* round to zero: nothing to do */
            {
              inex = -1;
              goto truncate;
            }
        }
    }

  /**************************************************************************
   *                                                                        *
   *     Mulders' short division failed: we revert to integer division      *
   *                                                                        *
   **************************************************************************/

  if (MPFR_UNLIKELY(rnd_mode == MPFR_RNDN && sh == 0))
    { /* we compute the quotient with one more limb, in order to get
         the round bit in the quotient, and the remainder only contains
         sticky bits */
      qsize = q0size + 1;
      /* need to allocate memory for the quotient */
      qp = MPFR_TMP_LIMBS_ALLOC (qsize);
    }
  else
    {
      qsize = q0size;
      qp = q0p; /* directly put the quotient in the destination */
    }
  qqsize = qsize + qsize;

  /* prepare the dividend */
  ap = MPFR_TMP_LIMBS_ALLOC (qqsize);
  if (MPFR_LIKELY(qqsize > usize)) /* use the full dividend */
    {
      k = qqsize - usize; /* k > 0 */
      MPN_ZERO(ap, k);
      if (extra_bit)
        ap[k - 1] = mpn_rshift (ap + k, up, usize, 1);
      else
        MPN_COPY(ap + k, up, usize);
    }
  else /* truncate the dividend */
    {
      k = usize - qqsize;
      if (extra_bit)
        sticky_u = mpn_rshift (ap, up + k, qqsize, 1);
      else
        MPN_COPY(ap, up + k, qqsize);
      sticky_u = sticky_u || mpfr_mpn_cmpzero (up, k);
    }
  low_u = sticky_u;

  /* now sticky_u is non-zero iff the truncated part of u is non-zero */

  /* prepare the divisor */
  if (MPFR_LIKELY(vsize >= qsize))
    {
      k = vsize - qsize;
      if (qp != vp)
        bp = vp + k; /* avoid copying the divisor */
      else /* need to copy, since mpn_divrem doesn't allow overlap
              between quotient and divisor, necessarily k = 0
              since quotient and divisor are the same mpfr variable */
        {
          bp = MPFR_TMP_LIMBS_ALLOC (qsize);
          MPN_COPY(bp, vp, vsize);
        }
      sticky_v = sticky_v || mpfr_mpn_cmpzero (vp, k);
      k = 0;
    }
  else /* vsize < qsize: small divisor case */
    {
      bp = vp;
      k = qsize - vsize;
    }

  /**************************************************************************
   *                                                                        *
   *  Here we perform the real division of {ap+k,qqsize-k} by {bp,qsize-k}  *
   *                                                                        *
   **************************************************************************/

  /* In the general case (usize > 2*qsize and vsize > qsize), we have:
       ______________________________________
      |                          |           |   u1 has 2*qsize limbs
      |             u1           |     u0    |   u0 has usize-2*qsize limbs
      |__________________________|___________|

                      ____________________
                     |           |        |      v1 has qsize limbs
                     |    v1     |    v0  |      v0 has vsize-qsize limbs
                     |___________|________|

      We divide u1 by v1, with quotient in qh + {qp, qsize} and
      remainder (denoted r below) stored in place of the low qsize limbs of u1.
  */

  /* if Mulders' short division failed, we revert to division with remainder */
  qh = mpn_divrem (qp, 0, ap + k, qqsize - k, bp, qsize - k);
  /* let u1 be the upper part of u, and v1 the upper part of v (with sticky_u
     and sticky_v representing the lower parts), then the quotient of u1 by v1
     is now in {qp, qsize}, with possible carry in qh, and the remainder in
     {ap + k, qsize - k} */
  /* warning: qh may be 1 if u1 == v1, but u < v */

  k = qsize;
  sticky_u = sticky_u || mpfr_mpn_cmpzero (ap, k);

  sticky = sticky_u | sticky_v;

  /* now sticky is non-zero iff one of the following holds:
     (a) the truncated part of u is non-zero
     (b) the truncated part of v is non-zero
     (c) the remainder from division is non-zero */

  if (MPFR_LIKELY(qsize == q0size))
    {
      sticky3 = qp[0] & MPFR_LIMB_MASK(sh); /* does nothing when sh=0 */
      sh2 = sh;
    }
  else /* qsize = q0size + 1: only happens when rnd_mode=MPFR_RNDN and sh=0 */
    {
      MPN_COPY (q0p, qp + 1, q0size);
      sticky3 = qp[0];
      sh2 = GMP_NUMB_BITS;
    }
  qp[0] ^= sticky3;
  /* sticky3 contains the truncated bits from the quotient,
     including the round bit, and 1 <= sh2 <= GMP_NUMB_BITS
     is the number of bits in sticky3 */
  inex = (sticky != MPFR_LIMB_ZERO) || (sticky3 != MPFR_LIMB_ZERO);

  /* to round, we distinguish two cases:
     (a) vsize <= qsize: we used the full divisor
     (b) vsize > qsize: the divisor was truncated
  */

  if (MPFR_LIKELY(vsize <= qsize)) /* use the full divisor */
    {
      if (MPFR_LIKELY(rnd_mode == MPFR_RNDN))
        {
          round_bit = sticky3 & (MPFR_LIMB_ONE << (sh2 - 1));
          sticky = (sticky3 ^ round_bit) | sticky_u;
        }
      else if (like_rndz || inex == 0)
        sticky = (inex == 0) ? MPFR_LIMB_ZERO : MPFR_LIMB_ONE;
      else  /* round away from zero */
        sticky = MPFR_LIMB_ONE;
      goto case_1;
    }
  else /* vsize > qsize: need to truncate the divisor */
    {
      if (inex == 0)
        goto truncate;
      else
        {
          /* We know the estimated quotient is an upper bound of the exact
             quotient (with rounding toward zero), with a difference of at
             most 2 in qp[0].
             Thus we can round except when sticky3 is 000...000 or 000...001
             for directed rounding, and 100...000 or 100...001 for rounding
             to nearest. (For rounding to nearest, we cannot determine the
             inexact flag for 000...000 or 000...001.)
          */
          mp_limb_t sticky3orig = sticky3;
          if (rnd_mode == MPFR_RNDN)
            {
              round_bit = sticky3 & (MPFR_LIMB_ONE << (sh2 - 1));
              sticky3   = sticky3 ^ round_bit;
            }
          if (sticky3 != MPFR_LIMB_ZERO && sticky3 != MPFR_LIMB_ONE)
            {
              sticky = sticky3;
              goto case_1;
            }
          else /* hard case: we have to compare q1 * v0 and r + u0,
                 where q1 * v0 has qsize + (vsize-qsize) = vsize limbs, and
                 r + u0 has qsize + (usize-2*qsize) = usize-qsize limbs */
            {
              mp_size_t l;
              mpfr_limb_ptr sp;
              int cmp_s_r;
              mp_limb_t qh2;

              sp = MPFR_TMP_LIMBS_ALLOC (vsize);
              k = vsize - qsize;
              /* sp <- {qp, qsize} * {vp, vsize-qsize} */
              qp[0] ^= sticky3orig; /* restore original quotient */
              if (qsize >= k)
                mpn_mul (sp, qp, qsize, vp, k);
              else
                mpn_mul (sp, vp, k, qp, qsize);
              if (qh)
                qh2 = mpn_add_n (sp + qsize, sp + qsize, vp, k);
              else
                qh2 = MPFR_LIMB_ZERO;
              qp[0] ^= sticky3orig; /* restore truncated quotient */

              /* compare qh2 + {sp, k + qsize} to {ap, qsize} + u0 */
              cmp_s_r = (qh2 != 0) ? 1 : mpn_cmp (sp + k, ap, qsize);
              if (cmp_s_r == 0) /* compare {sp, k} and u0 */
                {
                  cmp_s_r = (usize >= qqsize) ?
                    mpfr_mpn_cmp_aux (sp, k, up, usize - qqsize, extra_bit) :
                    mpfr_mpn_cmpzero (sp, k);
                }
              /* now cmp_s_r > 0 if {sp, vsize} > {ap, qsize} + u0
                     cmp_s_r = 0 if {sp, vsize} = {ap, qsize} + u0
                     cmp_s_r < 0 if {sp, vsize} < {ap, qsize} + u0 */
              if (cmp_s_r <= 0) /* quotient is in [q1, q1+1) */
                {
                  sticky = (cmp_s_r == 0) ? sticky3 : MPFR_LIMB_ONE;
                  goto case_1;
                }
              else /* cmp_s_r > 0, quotient is < q1: to determine if it is
                      in [q1-2,q1-1] or in [q1-1,q1], we need to subtract
                      the low part u0 of the dividend from q*v0 */
                {
                  mp_limb_t cy = MPFR_LIMB_ZERO;

                  /* subtract u0 >> extra_bit if non-zero */
                  if (qh2 != 0) /* whatever the value of {up, m + k}, it
                                   will be smaller than qh2 + {sp, k} */
                    cmp_s_r = 1;
                  else
                    {
                      if (low_u != MPFR_LIMB_ZERO)
                        {
                          mp_size_t m;
                          l = usize - qqsize; /* number of limbs in u0 */
                          m = (l > k) ? l - k : 0;
                          cy = (extra_bit) ?
                            (up[m] & MPFR_LIMB_ONE) : MPFR_LIMB_ZERO;
                          if (l >= k) /* u0 has at least as many limbs than s:
                                         first look if {up, m} is not zero,
                                         and compare {sp, k} and {up + m, k} */
                            {
                              cy = cy || mpfr_mpn_cmpzero (up, m);
                              low_u = cy;
                              cy = mpfr_mpn_sub_aux (sp, up + m, k,
                                                     cy, extra_bit);
                            }
                          else /* l < k: s has more limbs than u0 */
                            {
                              low_u = MPFR_LIMB_ZERO;
                              if (cy != MPFR_LIMB_ZERO)
                                cy = mpn_sub_1 (sp + k - l - 1, sp + k - l - 1,
                                                1, MPFR_LIMB_HIGHBIT);
                              cy = mpfr_mpn_sub_aux (sp + k - l, up, l,
                                                     cy, extra_bit);
                            }
                        }
                      MPFR_ASSERTD (cy <= 1);
                      cy = mpn_sub_1 (sp + k, sp + k, qsize, cy);
                      /* subtract r */
                      cy += mpn_sub_n (sp + k, sp + k, ap, qsize);
                      MPFR_ASSERTD (cy <= 1);
                      /* now compare {sp, ssize} to v */
                      cmp_s_r = mpn_cmp (sp, vp, vsize);
                      if (cmp_s_r == 0 && low_u != MPFR_LIMB_ZERO)
                        cmp_s_r = 1; /* since in fact we subtracted
                                        less than 1 */
                    }
                  if (cmp_s_r <= 0) /* q1-1 <= u/v < q1 */
                    {
                      if (sticky3 == MPFR_LIMB_ONE)
                        { /* q1-1 is either representable (directed rounding),
                             or the middle of two numbers (nearest) */
                          sticky = (cmp_s_r) ? MPFR_LIMB_ONE : MPFR_LIMB_ZERO;
                          goto case_1;
                        }
                      /* now necessarily sticky3=0 */
                      else if (round_bit == MPFR_LIMB_ZERO)
                        { /* round_bit=0, sticky3=0: q1-1 is exact only
                             when sh=0 */
                          inex = (cmp_s_r || sh) ? -1 : 0;
                          if (rnd_mode == MPFR_RNDN ||
                              (! like_rndz && inex != 0))
                            {
                              inex = 1;
                              goto truncate_check_qh;
                            }
                          else /* round down */
                            goto sub_one_ulp;
                        }
                      else /* sticky3=0, round_bit=1 ==> rounding to nearest */
                        {
                          inex = cmp_s_r;
                          goto truncate;
                        }
                    }
                  else /* q1-2 < u/v < q1-1 */
                    {
                      /* if rnd=MPFR_RNDN, the result is q1 when
                         q1-2 >= q1-2^(sh-1), i.e. sh >= 2,
                         otherwise (sh=1) it is q1-2 */
                      if (rnd_mode == MPFR_RNDN) /* sh > 0 */
                        {
                          /* Case sh=1: sb=0 always, and q1-rb is exactly
                             representable, like q1-rb-2.
                             rb action
                             0  subtract two ulps, inex=-1
                             1  truncate, inex=1

                             Case sh>1: one ulp is 2^(sh-1) >= 2
                             rb sb action
                             0  0  truncate, inex=1
                             0  1  truncate, inex=1
                             1  x  truncate, inex=-1
                           */
                          if (sh == 1)
                            {
                              if (round_bit == MPFR_LIMB_ZERO)
                                {
                                  inex = -1;
                                  sh = 0;
                                  goto sub_two_ulp;
                                }
                              else
                                {
                                  inex = 1;
                                  goto truncate_check_qh;
                                }
                            }
                          else /* sh > 1 */
                            {
                              inex = (round_bit == MPFR_LIMB_ZERO) ? 1 : -1;
                              goto truncate_check_qh;
                            }
                        }
                      else if (like_rndz)
                        {
                          /* the result is down(q1-2), i.e. subtract one
                             ulp if sh > 0, and two ulps if sh=0 */
                          inex = -1;
                          if (sh > 0)
                            goto sub_one_ulp;
                          else
                            goto sub_two_ulp;
                        }
                      /* if round away from zero, the result is up(q1-1),
                         which is q1 unless sh = 0, where it is q1-1 */
                      else
                        {
                          inex = 1;
                          if (sh > 0)
                            goto truncate_check_qh;
                          else /* sh = 0 */
                            goto sub_one_ulp;
                        }
                    }
                }
            }
        }
    }

 case_1: /* quotient is in [q1, q1+1),
            round_bit is the round_bit (0 for directed rounding),
            sticky the sticky bit */
  if (like_rndz || (round_bit == MPFR_LIMB_ZERO && sticky == MPFR_LIMB_ZERO))
    {
      inex = round_bit == MPFR_LIMB_ZERO && sticky == MPFR_LIMB_ZERO ? 0 : -1;
      goto truncate;
    }
  else if (rnd_mode == MPFR_RNDN) /* sticky <> 0 or round <> 0 */
    {
      if (round_bit == MPFR_LIMB_ZERO) /* necessarily sticky <> 0 */
        {
          inex = -1;
          goto truncate;
        }
      /* round_bit = 1 */
      else if (sticky != MPFR_LIMB_ZERO)
        goto add_one_ulp; /* inex=1 */
      else /* round_bit=1, sticky=0 */
        goto even_rule;
    }
  else /* round away from zero, sticky <> 0 */
    goto add_one_ulp; /* with inex=1 */

 sub_two_ulp:
  /* we cannot subtract MPFR_LIMB_MPFR_LIMB_ONE << (sh+1) since this is
     undefined for sh = GMP_NUMB_BITS */
  qh -= mpn_sub_1 (q0p, q0p, q0size, MPFR_LIMB_ONE << sh);
  /* go through */

 sub_one_ulp:
  qh -= mpn_sub_1 (q0p, q0p, q0size, MPFR_LIMB_ONE << sh);
  /* go through truncate_check_qh */

 truncate_check_qh:
  if (qh)
    {
      if (MPFR_LIKELY (qexp < MPFR_EXP_MAX))
        qexp ++;
      /* else qexp is now incorrect, but one will still get an overflow */
      q0p[q0size - 1] = MPFR_LIMB_HIGHBIT;
    }
  goto truncate;

 even_rule: /* has to set inex */
  inex = (q0p[0] & (MPFR_LIMB_ONE << sh)) ? 1 : -1;
  if (inex < 0)
    goto truncate;
  /* else go through add_one_ulp */

 add_one_ulp:
  inex = 1; /* always here */
  if (mpn_add_1 (q0p, q0p, q0size, MPFR_LIMB_ONE << sh))
    {
      if (MPFR_LIKELY (qexp < MPFR_EXP_MAX))
        qexp ++;
      /* else qexp is now incorrect, but one will still get an overflow */
      q0p[q0size - 1] = MPFR_LIMB_HIGHBIT;
    }

 truncate: /* inex already set */

  MPFR_TMP_FREE(marker);

  /* check for underflow/overflow */
  if (MPFR_UNLIKELY(qexp > __gmpfr_emax))
    return mpfr_overflow (q, rnd_mode, sign_quotient);
  else if (MPFR_UNLIKELY(qexp < __gmpfr_emin))
    {
      if (rnd_mode == MPFR_RNDN && ((qexp < __gmpfr_emin - 1) ||
                                   (inex >= 0 && mpfr_powerof2_raw (q))))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (q, rnd_mode, sign_quotient);
    }
  MPFR_SET_EXP(q, qexp);

  inex *= sign_quotient;
  MPFR_RET (inex);
}
