/* mpfr_sqrt -- square root of a floating-point number

Copyright 1999-2025 Free Software Foundation, Inc.
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

#if !defined(MPFR_GENERIC_ABI) && GMP_NUMB_BITS == 64

#include "invsqrt_limb.h"

/* Put in rp[1]*2^64+rp[0] an approximation of floor(sqrt(2^128*n)),
   with 2^126 <= n := np[1]*2^64 + np[0] < 2^128. We have:
   {rp, 2} - 4 <= floor(sqrt(2^128*n)) <= {rp, 2} + 26. */
static void
mpfr_sqrt2_approx (mpfr_limb_ptr rp, mpfr_limb_srcptr np)
{
  mp_limb_t x, r1, r0, h, l;

  __gmpfr_sqrt_limb (r1, h, l, x, np[1]);

  /* now r1 = floor(sqrt(2^64*n1)) and h:l = 2^64*n1 - r1^2 with h:l <= 2*r1,
     thus h <= 1, and x is an approximation of 2^96/sqrt(np[1])-2^64 */

  l += np[0];
  h += (l < np[0]);

  /* now 2^64*n1 + n0 - r1^2 = 2^64*h + l with h <= 2 */

  /* divide by 2 */
  l = (h << 63) | (l >> 1);
  h = h >> 1;

  /* now h <= 1 */

  /* now add (2^64+x) * (h*2^64+l) / 2^64 to [r1*2^64, 0] */

  umul_hi (r0, x, l); /* x * l */
  r0 += l;
  r1 += h + (r0 < l); /* now we have added 2^64 * (h*2^64+l) */
  if (h)
    {
      r0 += x;
      r1 += (r0 < x); /* add x */
    }

  MPFR_ASSERTD(r1 & MPFR_LIMB_HIGHBIT);

  rp[0] = r0;
  rp[1] = r1;
}

/* Special code for prec(r) = prec(u) < GMP_NUMB_BITS. We cannot have
   prec(u) = GMP_NUMB_BITS here, since when the exponent of u is odd,
   we need to shift u by one bit to the right without losing any bit.
   Assumes GMP_NUMB_BITS = 64. */
static int
mpfr_sqrt1 (mpfr_ptr r, mpfr_srcptr u, mpfr_rnd_t rnd_mode)
{
  mpfr_prec_t p = MPFR_GET_PREC(r);
  mpfr_prec_t exp_u = MPFR_EXP(u), exp_r, sh = GMP_NUMB_BITS - p;
  mp_limb_t u0, r0, rb, sb, mask = MPFR_LIMB_MASK(sh);
  mpfr_limb_ptr rp = MPFR_MANT(r);

  MPFR_STAT_STATIC_ASSERT (GMP_NUMB_BITS == 64);

  /* first make the exponent even */
  u0 = MPFR_MANT(u)[0];
  if (((unsigned int) exp_u & 1) != 0)
    {
      u0 >>= 1;
      exp_u ++;
    }
  MPFR_ASSERTD (((unsigned int) exp_u & 1) == 0);
  exp_r = exp_u / 2;

  /* then compute an approximation of the integer square root of
     u0*2^GMP_NUMB_BITS */
  __gmpfr_sqrt_limb_approx (r0, u0);

  sb = 1; /* when we can round correctly with the approximation, the sticky bit
             is non-zero */

  /* the exact square root is in [r0, r0 + 7] */
  if (MPFR_UNLIKELY(((r0 + 7) & (mask >> 1)) <= 7))
    {
      /* We should ensure r0 has its most significant bit set.
         Since r0 <= sqrt(2^64*u0) <= r0 + 7, as soon as sqrt(2^64*u0)>=2^63+7,
         which happens for u0>=2^62+8, then r0 >= 2^63.
         It thus remains to check that for 2^62 <= u0 <= 2^62+7,
         __gmpfr_sqrt_limb_approx (r0, u0) gives r0 >= 2^63, which is indeed
         the case:
         u0=4611686018427387904 r0=9223372036854775808
         u0=4611686018427387905 r0=9223372036854775808
         u0=4611686018427387906 r0=9223372036854775809
         u0=4611686018427387907 r0=9223372036854775810
         u0=4611686018427387908 r0=9223372036854775811
         u0=4611686018427387909 r0=9223372036854775812
         u0=4611686018427387910 r0=9223372036854775813
         u0=4611686018427387911 r0=9223372036854775814 */
      MPFR_ASSERTD(r0 >= MPFR_LIMB_HIGHBIT);
      umul_ppmm (rb, sb, r0, r0);
      sub_ddmmss (rb, sb, u0, 0, rb, sb);
      /* for the exact square root, we should have 0 <= rb:sb <= 2*r0 */
      while (!(rb == 0 || (rb == 1 && sb <= 2 * r0)))
        {
          /* subtract 2*r0+1 from rb:sb: subtract r0 before incrementing r0,
             then r0 after (which is r0+1) */
          rb -= (sb < r0);
          sb -= r0;
          r0 ++;
          rb -= (sb < r0);
          sb -= r0;
        }
      /* now we should have rb*2^64 + sb <= 2*r0 */
      MPFR_ASSERTD(rb == 0 || (rb == 1 && sb <= 2 * r0));
      sb = rb | sb;
    }

  rb = r0 & (MPFR_LIMB_ONE << (sh - 1));
  sb |= (r0 & mask) ^ rb;
  rp[0] = r0 & ~mask;

  /* rounding: sb = 0 implies rb = 0, since (rb,sb)=(1,0) is not possible */
  MPFR_ASSERTD (rb == 0 || sb != 0);

  /* Note: if 1 and 2 are in [emin,emax], no overflow nor underflow
     is possible */
  if (MPFR_UNLIKELY (exp_r > __gmpfr_emax))
    return mpfr_overflow (r, rnd_mode, 1);

  /* See comments in mpfr_div_1 */
  if (MPFR_UNLIKELY (exp_r < __gmpfr_emin))
    {
      if (rnd_mode == MPFR_RNDN)
        {
          /* If (1-2^(-p-1))*2^(emin-1) <= sqrt(u) < 2^(emin-1),
             then sqrt(u) would be rounded to 2^(emin-1) with unbounded
             exponent range, and there would be no underflow.
             But this case cannot happen if u has precision p.
             Indeed, we would have:
             (1-2^(-p-1))^2*2^(2*emin-2) <= u < 2^(2*emin-2), i.e.,
             (1-2^(-p)+2^(-2p-2))*2^(2*emin-2) <= u < 2^(2*emin-2),
             and there is no p-bit number in that interval. */
          /* If the result is <= 0.5^2^(emin-1), we should round to 0. */
          if (exp_r < __gmpfr_emin - 1 ||
              (rp[0] == MPFR_LIMB_HIGHBIT && sb == 0))
            rnd_mode = MPFR_RNDZ;
        }
      else if (MPFR_IS_LIKE_RNDA(rnd_mode, 0))
        {
          if (exp_r == __gmpfr_emin - 1 &&
              rp[0] == ~mask &&
              (rb | sb) != 0)
            goto rounding; /* no underflow */
        }
      return mpfr_underflow (r, rnd_mode, 1);
    }

 rounding:
  MPFR_EXP (r) = exp_r;
  if (sb == 0 /* implies rb = 0 */ || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD (rb == 0 || rnd_mode == MPFR_RNDF);
      MPFR_ASSERTD(exp_r >= __gmpfr_emin);
      MPFR_ASSERTD(exp_r <= __gmpfr_emax);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      /* since sb <> 0, only rb is needed to decide how to round, and the exact
         middle is not possible */
      if (rb == 0)
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, 0))
    {
    truncate:
      MPFR_ASSERTD(exp_r >= __gmpfr_emin);
      MPFR_ASSERTD(exp_r <= __gmpfr_emax);
      MPFR_RET(-1);
    }
  else /* round away from zero */
    {
    add_one_ulp:
      rp[0] += MPFR_LIMB_ONE << sh;
      if (rp[0] == 0)
        {
          rp[0] = MPFR_LIMB_HIGHBIT;
          if (MPFR_UNLIKELY(exp_r + 1 > __gmpfr_emax))
            return mpfr_overflow (r, rnd_mode, 1);
          MPFR_ASSERTD(exp_r + 1 <= __gmpfr_emax);
          MPFR_ASSERTD(exp_r + 1 >= __gmpfr_emin);
          MPFR_SET_EXP (r, exp_r + 1);
        }
      MPFR_RET(1);
    }
}

/* Special code for prec(r) = prec(u) = GMP_NUMB_BITS. */
static int
mpfr_sqrt1n (mpfr_ptr r, mpfr_srcptr u, mpfr_rnd_t rnd_mode)
{
  mpfr_prec_t exp_u = MPFR_EXP(u), exp_r;
  mp_limb_t u0, r0, rb, sb, low;
  mpfr_limb_ptr rp = MPFR_MANT(r);

  MPFR_STAT_STATIC_ASSERT (GMP_NUMB_BITS == 64);
  MPFR_ASSERTD(MPFR_PREC(r) == GMP_NUMB_BITS);
  MPFR_ASSERTD(MPFR_PREC(u) <= GMP_NUMB_BITS);

  /* first make the exponent even */
  u0 = MPFR_MANT(u)[0];
  if (((unsigned int) exp_u & 1) != 0)
    {
      low = u0 << (GMP_NUMB_BITS - 1);
      u0 >>= 1;
      exp_u ++;
    }
  else
    low = 0; /* low part of u0 */
  MPFR_ASSERTD (((unsigned int) exp_u & 1) == 0);
  exp_r = exp_u / 2;

  /* then compute an approximation of the integer square root of
     u0*2^GMP_NUMB_BITS */
  __gmpfr_sqrt_limb_approx (r0, u0);

  /* the exact square root is in [r0, r0 + 7] */

  /* As shown in mpfr_sqrt1 above, r0 has its most significant bit set */
  MPFR_ASSERTD(r0 >= MPFR_LIMB_HIGHBIT);

  umul_ppmm (rb, sb, r0, r0);
  sub_ddmmss (rb, sb, u0, low, rb, sb);
  /* for the exact square root, we should have 0 <= rb:sb <= 2*r0 */
  while (!(rb == 0 || (rb == 1 && sb <= 2 * r0)))
    {
      /* subtract 2*r0+1 from rb:sb: subtract r0 before incrementing r0,
         then r0 after (which is r0+1) */
      rb -= (sb < r0);
      sb -= r0;
      r0 ++;
      rb -= (sb < r0);
      sb -= r0;
    }
  /* now we have u0*2^64+low = r0^2 + rb*2^64+sb, with rb*2^64+sb <= 2*r0 */
  MPFR_ASSERTD(rb == 0 || (rb == 1 && sb <= 2 * r0));

  /* We can't have the middle case u0*2^64 = (r0 + 1/2)^2 since
     (r0 + 1/2)^2 is not an integer.
     We thus rb = 1 whenever u0*2^64 > (r0 + 1/2)^2, thus rb*2^64 + sb > r0
     and the sticky bit is always 1, unless we had rb = sb = 0. */

  rb = rb || (sb > r0);
  sb = rb | sb;
  rp[0] = r0;

  /* rounding */

  /* Note: if 1 and 2 are in [emin,emax], no overflow nor underflow
     is possible */
  if (MPFR_UNLIKELY (exp_r > __gmpfr_emax))
    return mpfr_overflow (r, rnd_mode, 1);

  /* See comments in mpfr_div_1 */
  if (MPFR_UNLIKELY (exp_r < __gmpfr_emin))
    {
      if (rnd_mode == MPFR_RNDN)
        {
          /* the case rp[0] = 111...111 and rb = 1 cannot happen, since it
             would imply u0 >= (2^64-1/2)^2/2^64 thus u0 >= 2^64 */
          if (exp_r < __gmpfr_emin - 1 ||
              (rp[0] == MPFR_LIMB_HIGHBIT && sb == 0))
            rnd_mode = MPFR_RNDZ;
        }
      else if (MPFR_IS_LIKE_RNDA(rnd_mode, 0))
        {
          if (exp_r == __gmpfr_emin - 1 &&
              rp[0] == MPFR_LIMB_MAX &&
              (rb | sb) != 0)
            goto rounding; /* no underflow */
        }
      return mpfr_underflow (r, rnd_mode, 1);
    }

  /* sb = 0 can only occur when the square root is exact, i.e., rb = 0 */

 rounding:
  MPFR_EXP (r) = exp_r;
  if (sb == 0 /* implies rb = 0 */ || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(exp_r >= __gmpfr_emin);
      MPFR_ASSERTD(exp_r <= __gmpfr_emax);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      /* we can't have sb = 0, thus rb is enough */
      if (rb == 0)
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, 0))
    {
    truncate:
      MPFR_ASSERTD(exp_r >= __gmpfr_emin);
      MPFR_ASSERTD(exp_r <= __gmpfr_emax);
      MPFR_RET(-1);
    }
  else /* round away from zero */
    {
    add_one_ulp:
      rp[0] += MPFR_LIMB_ONE;
      if (rp[0] == 0)
        {
          rp[0] = MPFR_LIMB_HIGHBIT;
          if (MPFR_UNLIKELY(exp_r + 1 > __gmpfr_emax))
            return mpfr_overflow (r, rnd_mode, 1);
          MPFR_ASSERTD(exp_r + 1 <= __gmpfr_emax);
          MPFR_ASSERTD(exp_r + 1 >= __gmpfr_emin);
          MPFR_SET_EXP (r, exp_r + 1);
        }
      MPFR_RET(1);
    }
}

/* Special code for GMP_NUMB_BITS < prec(r) = prec(u) < 2*GMP_NUMB_BITS.
   Assumes GMP_NUMB_BITS=64. */
static int
mpfr_sqrt2 (mpfr_ptr r, mpfr_srcptr u, mpfr_rnd_t rnd_mode)
{
  mpfr_prec_t p = MPFR_GET_PREC(r);
  mpfr_limb_ptr up = MPFR_MANT(u), rp = MPFR_MANT(r);
  mp_limb_t np[4], rb, sb, mask;
  mpfr_prec_t exp_u = MPFR_EXP(u), exp_r, sh = 2 * GMP_NUMB_BITS - p;

  MPFR_STAT_STATIC_ASSERT (GMP_NUMB_BITS == 64);

  if (((unsigned int) exp_u & 1) != 0)
    {
      np[3] = up[1] >> 1;
      np[2] = (up[1] << (GMP_NUMB_BITS - 1)) | (up[0] >> 1);
      np[1] = up[0] << (GMP_NUMB_BITS - 1);
      exp_u ++;
    }
  else
    {
      np[3] = up[1];
      np[2] = up[0];
      np[1] = 0;
    }
  exp_r = exp_u / 2;

  mask = MPFR_LIMB_MASK(sh);

  mpfr_sqrt2_approx (rp, np + 2);
  /* with n = np[3]*2^64+np[2], we have:
     {rp, 2} - 4 <= floor(sqrt(2^128*n)) <= {rp, 2} + 26, thus we can round
     correctly except when the number formed by the last sh-1 bits
     of rp[0] is in the range [-26, 4]. */
  if (MPFR_LIKELY(((rp[0] + 26) & (mask >> 1)) > 30))
    sb = 1;
  else
    {
      mp_limb_t tp[4], h, l;

      np[0] = 0;
      mpn_sqr (tp, rp, 2);
      /* since we know s - 26 <= r <= s + 4 and 0 <= n^2 - s <= 2*s, we have
         -8*s-16 <= n - r^2 <= 54*s - 676, thus it suffices to compute
         n - r^2 modulo 2^192 */
      mpn_sub_n (tp, np, tp, 3);
      /* invariant: h:l = 2 * {rp, 2}, with upper bit implicit */
      h = (rp[1] << 1) | (rp[0] >> (GMP_NUMB_BITS - 1));
      l = rp[0] << 1;
      while ((mp_limb_signed_t) tp[2] < 0) /* approximation was too large */
        {
          /* subtract 1 to {rp, 2}, thus 2 to h:l */
          h -= (l <= MPFR_LIMB_ONE);
          l -= 2;
          /* add (1:h:l)+1 to {tp,3} */
          tp[0] += l + 1;
          tp[1] += h + (tp[0] < l);
          /* necessarily rp[1] has its most significant bit set */
          tp[2] += MPFR_LIMB_ONE + (tp[1] < h || (tp[1] == h && tp[0] < l));
        }
      /* now tp[2] >= 0 */
      /* now we want {tp, 4} <= 2 * {rp, 2}, which implies tp[2] <= 1 */
      while (tp[2] > 1 || (tp[2] == 1 && tp[1] > h) ||
             (tp[2] == 1 && tp[1] == h && tp[0] > l))
        {
          /* subtract (1:h:l)+1 from {tp,3} */
          tp[2] -= MPFR_LIMB_ONE + (tp[1] < h || (tp[1] == h && tp[0] <= l));
          tp[1] -= h + (tp[0] <= l);
          tp[0] -= l + 1;
          /* add 2 to  h:l */
          l += 2;
          h += (l <= MPFR_LIMB_ONE);
        }
      /* restore {rp, 2} from h:l */
      rp[1] = MPFR_LIMB_HIGHBIT | (h >> 1);
      rp[0] = (h << (GMP_NUMB_BITS - 1)) | (l >> 1);
      sb = tp[2] | tp[0] | tp[1];
    }

  rb = rp[0] & (MPFR_LIMB_ONE << (sh - 1));
  sb |= (rp[0] & mask) ^ rb;
  rp[0] = rp[0] & ~mask;

  /* rounding */
  if (MPFR_UNLIKELY (exp_r > __gmpfr_emax))
    return mpfr_overflow (r, rnd_mode, 1);

  /* See comments in mpfr_div_1 */
  if (MPFR_UNLIKELY (exp_r < __gmpfr_emin))
    {
      if (rnd_mode == MPFR_RNDN)
        {
          if (exp_r < __gmpfr_emin - 1 || (rp[1] == MPFR_LIMB_HIGHBIT &&
                                           rp[0] == MPFR_LIMB_ZERO && sb == 0))
            rnd_mode = MPFR_RNDZ;
        }
      else if (MPFR_IS_LIKE_RNDA(rnd_mode, 0))
        {
          if (exp_r == __gmpfr_emin - 1 && (rp[1] == MPFR_LIMB_MAX &&
                                            rp[0] == ~mask) && (rb | sb))
            goto rounding; /* no underflow */
        }
      return mpfr_underflow (r, rnd_mode, 1);
    }

 rounding:
  MPFR_EXP (r) = exp_r;
  if (sb == 0 /* implies rb = 0 */ || rnd_mode == MPFR_RNDF)
    {
      MPFR_ASSERTD(exp_r >= __gmpfr_emin);
      MPFR_ASSERTD(exp_r <= __gmpfr_emax);
      MPFR_RET (0);
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      /* since sb <> 0 now, only rb is needed */
      if (rb == 0)
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, 0))
    {
    truncate:
      MPFR_ASSERTD(exp_r >= __gmpfr_emin);
      MPFR_ASSERTD(exp_r <= __gmpfr_emax);
      MPFR_RET(-1);
    }
  else /* round away from zero */
    {
    add_one_ulp:
      rp[0] += MPFR_LIMB_ONE << sh;
      rp[1] += rp[0] == 0;
      if (rp[1] == 0)
        {
          rp[1] = MPFR_LIMB_HIGHBIT;
          if (MPFR_UNLIKELY(exp_r + 1 > __gmpfr_emax))
            return mpfr_overflow (r, rnd_mode, 1);
          MPFR_ASSERTD(exp_r + 1 <= __gmpfr_emax);
          MPFR_ASSERTD(exp_r + 1 >= __gmpfr_emin);
          MPFR_SET_EXP (r, exp_r + 1);
        }
      MPFR_RET(1);
    }
}

#endif /* !defined(MPFR_GENERIC_ABI) && GMP_NUMB_BITS == 64 */

int
mpfr_sqrt (mpfr_ptr r, mpfr_srcptr u, mpfr_rnd_t rnd_mode)
{
  mp_size_t rsize; /* number of limbs of r (plus 1 if exact limb multiple) */
  mp_size_t rrsize;
  mp_size_t usize; /* number of limbs of u */
  mp_size_t tsize; /* number of limbs of the sqrtrem remainder */
  mp_size_t k;
  mp_size_t l;
  mpfr_limb_ptr rp, rp0;
  mpfr_limb_ptr up;
  mpfr_limb_ptr sp;
  mp_limb_t sticky0; /* truncated part of input */
  mp_limb_t sticky1; /* truncated part of rp[0] */
  mp_limb_t sticky;
  int odd_exp;
  int sh; /* number of extra bits in rp[0] */
  int inexact; /* return ternary flag */
  mpfr_exp_t expr;
  mpfr_prec_t rq = MPFR_GET_PREC (r);
  MPFR_TMP_DECL(marker);

  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg rnd=%d", mpfr_get_prec (u), mpfr_log_prec, u, rnd_mode),
     ("y[%Pd]=%.*Rg inexact=%d",
      mpfr_get_prec (r), mpfr_log_prec, r, inexact));

  if (MPFR_UNLIKELY(MPFR_IS_SINGULAR(u)))
    {
      if (MPFR_IS_NAN(u))
        {
          MPFR_SET_NAN(r);
          MPFR_RET_NAN;
        }
      else if (MPFR_IS_ZERO(u))
        {
          /* 0+ or 0- */
          MPFR_SET_SAME_SIGN(r, u);
          MPFR_SET_ZERO(r);
          MPFR_RET(0); /* zero is exact */
        }
      else
        {
          MPFR_ASSERTD(MPFR_IS_INF(u));
          /* sqrt(-Inf) = NAN */
          if (MPFR_IS_NEG(u))
            {
              MPFR_SET_NAN(r);
              MPFR_RET_NAN;
            }
          MPFR_SET_POS(r);
          MPFR_SET_INF(r);
          MPFR_RET(0);
        }
    }
  if (MPFR_UNLIKELY(MPFR_IS_NEG(u)))
    {
      MPFR_SET_NAN(r);
      MPFR_RET_NAN;
    }
  MPFR_SET_POS(r);

#if !defined(MPFR_GENERIC_ABI) && GMP_NUMB_BITS == 64
  {
    mpfr_prec_t uq = MPFR_GET_PREC (u);

    if (rq == uq)
      {
        if (rq < GMP_NUMB_BITS)
          return mpfr_sqrt1 (r, u, rnd_mode);

        if (GMP_NUMB_BITS < rq && rq < 2*GMP_NUMB_BITS)
          return mpfr_sqrt2 (r, u, rnd_mode);

        if (rq == GMP_NUMB_BITS)
          return mpfr_sqrt1n (r, u, rnd_mode);
      }
  }
#endif

  MPFR_TMP_MARK (marker);
  MPFR_UNSIGNED_MINUS_MODULO (sh, rq);
  if (sh == 0 && rnd_mode == MPFR_RNDN)
    sh = GMP_NUMB_BITS; /* ugly case */
  rsize = MPFR_LIMB_SIZE(r) + (sh == GMP_NUMB_BITS);
  /* rsize is the number of limbs of r + 1 if exact limb multiple and rounding
     to nearest, this is the number of wanted limbs for the square root */
  rrsize = rsize + rsize;
  usize = MPFR_LIMB_SIZE(u); /* number of limbs of u */
  rp0 = MPFR_MANT(r);
  rp = (sh < GMP_NUMB_BITS) ? rp0 : MPFR_TMP_LIMBS_ALLOC (rsize);
  up = MPFR_MANT(u);
  sticky0 = MPFR_LIMB_ZERO; /* truncated part of input */
  sticky1 = MPFR_LIMB_ZERO; /* truncated part of rp[0] */
  odd_exp = (unsigned int) MPFR_GET_EXP (u) & 1;
  inexact = -1; /* return ternary flag */

  sp = MPFR_TMP_LIMBS_ALLOC (rrsize);

  /* copy the most significant limbs of u to {sp, rrsize} */
  if (MPFR_LIKELY(usize <= rrsize)) /* in case r and u have the same precision,
                                       we have indeed rrsize = 2 * usize */
    {
      k = rrsize - usize;
      if (MPFR_LIKELY(k))
        MPN_ZERO (sp, k);
      if (odd_exp)
        {
          if (MPFR_LIKELY(k))
            sp[k - 1] = mpn_rshift (sp + k, up, usize, 1);
          else
            sticky0 = mpn_rshift (sp, up, usize, 1);
        }
      else
        MPN_COPY (sp + rrsize - usize, up, usize);
    }
  else /* usize > rrsize: truncate the input */
    {
      k = usize - rrsize;
      if (odd_exp)
        sticky0 = mpn_rshift (sp, up + k, rrsize, 1);
      else
        MPN_COPY (sp, up + k, rrsize);
      l = k;
      while (sticky0 == MPFR_LIMB_ZERO && l != 0)
        sticky0 = up[--l];
    }

  /* sticky0 is non-zero iff the truncated part of the input is non-zero */

  tsize = mpn_sqrtrem (rp, NULL, sp, rrsize);

  /* a return value of zero in mpn_sqrtrem indicates a perfect square */
  sticky = sticky0 || tsize != 0;

  /* truncate low bits of rp[0] */
  sticky1 = rp[0] & ((sh < GMP_NUMB_BITS) ? MPFR_LIMB_MASK(sh)
                     : MPFR_LIMB_MAX);
  rp[0] -= sticky1;

  sticky = sticky || sticky1;

  expr = (MPFR_GET_EXP(u) + odd_exp) / 2;  /* exact */

  if (rnd_mode == MPFR_RNDZ || rnd_mode == MPFR_RNDD ||
      sticky == MPFR_LIMB_ZERO)
    {
      inexact = (sticky == MPFR_LIMB_ZERO) ? 0 : -1;
      goto truncate;
    }
  else if (rnd_mode == MPFR_RNDN)
    {
      /* if sh < GMP_NUMB_BITS, the round bit is bit (sh-1) of sticky1
                  and the sticky bit is formed by the low sh-1 bits from
                  sticky1, together with the sqrtrem remainder and sticky0. */
      if (sh < GMP_NUMB_BITS)
        {
          if (sticky1 & (MPFR_LIMB_ONE << (sh - 1)))
            { /* round bit is set */
              if (sticky1 == (MPFR_LIMB_ONE << (sh - 1)) && tsize == 0
                  && sticky0 == 0)
                goto even_rule;
              else
                goto add_one_ulp;
            }
          else /* round bit is zero */
            goto truncate; /* with the default inexact=-1 */
        }
      else /* sh = GMP_NUMB_BITS: the round bit is the most significant bit
              of rp[0], and the remaining GMP_NUMB_BITS-1 bits contribute to
              the sticky bit */
        {
          if (sticky1 & MPFR_LIMB_HIGHBIT)
            { /* round bit is set */
              if (sticky1 == MPFR_LIMB_HIGHBIT && tsize == 0 && sticky0 == 0)
                goto even_rule;
              else
                goto add_one_ulp;
            }
          else /* round bit is zero */
            goto truncate; /* with the default inexact=-1 */
        }
    }
  else /* rnd_mode=GMP_RDNU, necessarily sticky <> 0, thus add 1 ulp */
    goto add_one_ulp;

 even_rule: /* has to set inexact */
  if (sh < GMP_NUMB_BITS)
    inexact = (rp[0] & (MPFR_LIMB_ONE << sh)) ? 1 : -1;
  else
    inexact = (rp[1] & MPFR_LIMB_ONE) ? 1 : -1;
  if (inexact == -1)
    goto truncate;
  /* else go through add_one_ulp */

 add_one_ulp:
  inexact = 1; /* always here */
  if (sh == GMP_NUMB_BITS)
    {
      rp ++;
      rsize --;
      sh = 0;
    }
  /* now rsize = MPFR_LIMB_SIZE(r) */
  if (mpn_add_1 (rp0, rp, rsize, MPFR_LIMB_ONE << sh))
    {
      expr ++;
      rp0[rsize - 1] = MPFR_LIMB_HIGHBIT;
    }
  goto end;

 truncate: /* inexact = 0 or -1 */
  if (sh == GMP_NUMB_BITS)
    MPN_COPY (rp0, rp + 1, rsize - 1);

 end:
  /* Do not use MPFR_SET_EXP because the range has not been checked yet. */
  MPFR_ASSERTN (expr >= MPFR_EMIN_MIN && expr <= MPFR_EMAX_MAX);
  MPFR_EXP (r) = expr;
  MPFR_TMP_FREE(marker);

  return mpfr_check_range (r, inexact, rnd_mode);
}
