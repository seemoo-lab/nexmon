/* mpfr_add1 -- internal function to perform a "real" addition

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

#include "mpfr-impl.h"

/* compute sign(b) * (|b| + |c|), assuming that b and c
   are not NaN, Inf, nor zero. Assumes EXP(b) >= EXP(c).
*/
MPFR_HOT_FUNCTION_ATTR int
mpfr_add1 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mp_limb_t *ap, *bp, *cp;
  mpfr_prec_t aq, bq, cq, aq2;
  mp_size_t an, bn, cn;
  mpfr_exp_t difw, exp, diff_exp;
  int sh, rb, fb, inex;
  MPFR_TMP_DECL(marker);

  MPFR_ASSERTD (MPFR_IS_PURE_UBF (b));
  MPFR_ASSERTD (MPFR_IS_PURE_UBF (c));
  MPFR_ASSERTD (! MPFR_UBF_EXP_LESS_P (b, c));

  if (MPFR_UNLIKELY (MPFR_IS_UBF (b)))
    {
      exp = MPFR_UBF_GET_EXP (b);
      if (exp > __gmpfr_emax)
        return mpfr_overflow (a, rnd_mode, MPFR_SIGN (b));;
    }
  else
    exp = MPFR_GET_EXP (b);

  MPFR_ASSERTD (exp <= __gmpfr_emax);

  MPFR_TMP_MARK(marker);

  aq = MPFR_GET_PREC (a);
  bq = MPFR_GET_PREC (b);
  cq = MPFR_GET_PREC (c);

  an = MPFR_PREC2LIMBS (aq); /* number of limbs of a */
  aq2 = (mpfr_prec_t) an * GMP_NUMB_BITS;
  sh = aq2 - aq;                  /* non-significant bits in low limb */

  bn = MPFR_PREC2LIMBS (bq); /* number of limbs of b */
  cn = MPFR_PREC2LIMBS (cq); /* number of limbs of c */

  ap = MPFR_MANT(a);
  bp = MPFR_MANT(b);
  cp = MPFR_MANT(c);

  if (MPFR_UNLIKELY(ap == bp))
    {
      bp = MPFR_TMP_LIMBS_ALLOC (bn);
      MPN_COPY (bp, ap, bn);
      if (ap == cp)
        { cp = bp; }
    }
  else if (ap == cp)
    {
      cp = MPFR_TMP_LIMBS_ALLOC (cn);
      MPN_COPY(cp, ap, cn);
    }

  MPFR_SET_SAME_SIGN(a, b);
  MPFR_UPDATE2_RND_MODE (rnd_mode, MPFR_SIGN (b));
  /* now rnd_mode is either MPFR_RNDN, MPFR_RNDZ, MPFR_RNDA or MPFR_RNDF. */
  if (MPFR_UNLIKELY (MPFR_IS_UBF (c)))
    {
      MPFR_STAT_STATIC_ASSERT (MPFR_EXP_MAX > MPFR_PREC_MAX);
      diff_exp = mpfr_ubf_diff_exp (b, c);
    }
  else
    diff_exp = exp - MPFR_GET_EXP (c);

  MPFR_ASSERTD (diff_exp >= 0);

  /*
   * 1. Compute the significant part A', the non-significant bits of A
   * are taken into account.
   *
   * 2. Perform the rounding. At each iteration, we remember:
   *     _ r = rounding bit
   *     _ f = following bits (same value)
   * where the result has the form: [number A]rfff...fff + a remaining
   * value in the interval [0,2) ulp. We consider the most significant
   * bits of the remaining value to update the result; a possible carry
   * is immediately taken into account and A is updated accordingly. As
   * soon as the bits f don't have the same value, A can be rounded.
   * Variables:
   *     _ rb = rounding bit (0 or 1).
   *     _ fb = following bits (0 or 1), then sticky bit.
   * If fb == 0, the only thing that can change is the sticky bit.
   */

  rb = fb = -1; /* means: not initialized */

  if (MPFR_UNLIKELY (MPFR_UEXP (aq2) <= diff_exp))
    { /* c does not overlap with a' */
      if (MPFR_UNLIKELY(an > bn))
        { /* a has more limbs than b */
          /* copy b to the most significant limbs of a */
          MPN_COPY(ap + (an - bn), bp, bn);
          /* zero the least significant limbs of a */
          MPN_ZERO(ap, an - bn);
        }
      else /* an <= bn */
        {
          /* copy the most significant limbs of b to a */
          MPN_COPY(ap, bp + (bn - an), an);
        }
    }
  else /* aq2 > diff_exp */
    { /* c overlaps with a' */
      mp_limb_t *a2p;
      mp_limb_t cc;
      mpfr_prec_t dif;
      mp_size_t difn, k;
      int shift;

      /* copy c (shifted) into a */

      dif = aq2 - diff_exp;
      /* dif is the number of bits of c which overlap with a' */

      difn = MPFR_PREC2LIMBS (dif);
      /* only the highest difn limbs from c have to be considered */
      if (MPFR_UNLIKELY(difn > cn))
        {
          /* c doesn't have enough limbs; take into account the virtual
             zero limbs now by zeroing the least significant limbs of a' */
          MPFR_ASSERTD(difn - cn <= an);
          MPN_ZERO(ap, difn - cn);
          difn = cn;
        }
      k = diff_exp / GMP_NUMB_BITS;

      /* zero the most significant k limbs of a */
      a2p = ap + (an - k);
      MPN_ZERO(a2p, k);

      shift = diff_exp % GMP_NUMB_BITS;

      if (MPFR_LIKELY(shift))
        {
          MPFR_ASSERTD(a2p - difn >= ap);
          cc = mpn_rshift(a2p - difn, cp + (cn - difn), difn, shift);
          if (MPFR_UNLIKELY(a2p - difn > ap))
            *(a2p - difn - 1) = cc;
        }
      else
        MPN_COPY(a2p - difn, cp + (cn - difn), difn);

      /* add b to a */
      cc = an > bn
        ? mpn_add_n(ap + (an - bn), ap + (an - bn), bp, bn)
        : mpn_add_n(ap, ap, bp + (bn - an), an);

      if (MPFR_UNLIKELY(cc)) /* carry */
        {
          if (MPFR_UNLIKELY(exp == __gmpfr_emax))
            {
              inex = mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
              goto end_of_add;
            }
          exp++;
          rb = (ap[0] >> sh) & 1; /* LSB(a) --> rounding bit after the shift */
          if (MPFR_LIKELY(sh))
            {
              mp_limb_t mask, bb;

              mask = MPFR_LIMB_MASK (sh);
              bb = ap[0] & mask;
              ap[0] &= MPFR_LIMB_LSHIFT (~mask, 1);
              if (bb == 0)
                fb = 0;
              else if (bb == mask)
                fb = 1;
            }
          mpn_rshift(ap, ap, an, 1);
          ap[an-1] += MPFR_LIMB_HIGHBIT;
          if (sh && fb < 0)
            goto rounding;
        } /* cc */
    } /* aq2 > diff_exp */

  /* zero the non-significant bits of a */
  if (MPFR_LIKELY(rb < 0 && sh))
    {
      mp_limb_t mask, bb;

      mask = MPFR_LIMB_MASK (sh);
      bb = ap[0] & mask;
      ap[0] &= ~mask;
      rb = bb >> (sh - 1);
      if (MPFR_LIKELY(sh > 1))
        {
          mask >>= 1;
          bb &= mask;
          if (bb == 0)
            fb = 0;
          else if (bb == mask)
            fb = 1;
          else
            goto rounding;
        }
    }

  /* Determine rounding and sticky bits (and possible carry).
     In faithful rounding, we may stop two bits after ulp(a):
     the approximation is regarded as the number formed by a,
     the rounding bit rb and an additional bit fb; and the
     corresponding error is < 1/2 ulp of the unrounded result. */

  difw = (mpfr_exp_t) an - (mpfr_exp_t) (diff_exp / GMP_NUMB_BITS);
  /* difw is the number of limbs from b (regarded as having an infinite
     precision) that have already been combined with c; -n if the next
     n limbs from b won't be combined with c. */

  if (MPFR_UNLIKELY(bn > an))
    { /* there are still limbs from b that haven't been taken into account */
      mp_size_t bk;

      if (fb == 0 && difw <= 0)
        {
          fb = 1; /* c hasn't been taken into account ==> sticky bit != 0 */
          goto rounding;
        }

      bk = bn - an; /* index of lowest considered limb from b, > 0 */
      while (difw < 0)
        { /* ulp(next limb from b) > msb(c) */
          mp_limb_t bb;

          bb = bp[--bk];

          MPFR_ASSERTD(fb != 0);
          if (fb > 0)
            {
              /* Note: Here, we can round to nearest, but the loop may still
                 be necessary to determine whether there is a carry from c,
                 which will have an effect on the ternary value. However, in
                 faithful rounding, we do not have to determine the ternary
                 value, so that we can end the loop here. */
              if (bb != MPFR_LIMB_MAX || rnd_mode == MPFR_RNDF)
                goto rounding;
            }
          else /* fb not initialized yet */
            {
              if (rb < 0) /* rb not initialized yet */
                {
                  rb = bb >> (GMP_NUMB_BITS - 1);
                  bb |= MPFR_LIMB_HIGHBIT;
                }
              fb = 1;
              if (bb != MPFR_LIMB_MAX)
                goto rounding;
            }

          if (bk == 0)
            { /* b has entirely been read */
              fb = 1; /* c hasn't been taken into account
                         ==> sticky bit != 0 */
              goto rounding;
            }

          difw++;
        } /* while */
      MPFR_ASSERTD(bk > 0 && difw >= 0);

      if (difw <= cn)
        {
          mp_size_t ck;
          mp_limb_t cprev;
          int difs;

          ck = cn - difw;
          difs = diff_exp % GMP_NUMB_BITS;

          if (difs == 0 && ck == 0)
            goto c_read;

          cprev = ck == cn ? 0 : cp[ck];

          if (fb < 0)
            {
              mp_limb_t bb, cc;

              if (difs)
                {
                  cc = cprev << (GMP_NUMB_BITS - difs);
                  if (--ck >= 0)
                    {
                      cprev = cp[ck];
                      cc += cprev >> difs;
                    }
                }
              else
                cc = cp[--ck];

              bb = bp[--bk] + cc;

              if (bb < cc /* carry */
                  && (rb < 0 || (rb ^= 1) == 0)
                  && mpn_add_1(ap, ap, an, MPFR_LIMB_ONE << sh))
                {
                  if (exp == __gmpfr_emax)
                    {
                      inex = mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
                      goto end_of_add;
                    }
                  exp++;
                  ap[an-1] = MPFR_LIMB_HIGHBIT;
                  rb = 0;
                }

              if (rb < 0) /* rb not initialized yet */
                {
                  rb = bb >> (GMP_NUMB_BITS - 1);
                  bb <<= 1;
                  bb |= bb >> (GMP_NUMB_BITS - 1);
                }

              fb = bb != 0;
              if (fb && bb != MPFR_LIMB_MAX)
                goto rounding;
            } /* fb < 0 */

          /* At least two bits after ulp(a) have been read, which is
             sufficient for faithful rounding, as we do not need to
             determine on which side of a breakpoint the result is. */
          if (rnd_mode == MPFR_RNDF)
            goto rounding;

          while (bk > 0)
            {
              mp_limb_t bb, cc;

              if (difs)
                {
                  if (ck < 0)
                    goto c_read;
                  cc = cprev << (GMP_NUMB_BITS - difs);
                  if (--ck >= 0)
                    {
                      cprev = cp[ck];
                      cc += cprev >> difs;
                    }
                }
              else
                {
                  if (ck == 0)
                    goto c_read;
                  cc = cp[--ck];
                }

              bb = bp[--bk] + cc;
              if (bb < cc) /* carry */
                {
                  fb ^= 1;
                  if (fb)
                    goto rounding;
                  rb ^= 1;
                  if (rb == 0 && mpn_add_1 (ap, ap, an, MPFR_LIMB_ONE << sh))
                    {
                      if (MPFR_UNLIKELY(exp == __gmpfr_emax))
                        {
                          inex = mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
                          goto end_of_add;
                        }
                      exp++;
                      ap[an-1] = MPFR_LIMB_HIGHBIT;
                    }
                } /* bb < cc */

              if (!fb && bb != 0)
                {
                  fb = 1;
                  goto rounding;
                }
              if (fb && bb != MPFR_LIMB_MAX)
                goto rounding;
            } /* while */

          /* b has entirely been read */

          if (fb || ck < 0)
            goto rounding;
          if (difs && MPFR_LIMB_LSHIFT(cprev, GMP_NUMB_BITS - difs) != 0)
            {
              fb = 1;
              goto rounding;
            }
          while (ck)
            {
              if (cp[--ck])
                {
                  fb = 1;
                  goto rounding;
                }
            } /* while */
        } /* difw <= cn */
      else
        { /* c has entirely been read */
        c_read:
          if (fb < 0) /* fb not initialized yet */
            {
              mp_limb_t bb;

              MPFR_ASSERTD(bk > 0);
              bb = bp[--bk];
              if (rb < 0) /* rb not initialized yet */
                {
                  rb = bb >> (GMP_NUMB_BITS - 1);
                  bb &= ~MPFR_LIMB_HIGHBIT;
                }
              fb = bb != 0;
            } /* fb < 0 */
          if (fb || rnd_mode == MPFR_RNDF)
            goto rounding;
          while (bk)
            {
              if (bp[--bk])
                {
                  fb = 1;
                  goto rounding;
                }
            } /* while */
        } /* difw > cn */
    } /* bn > an */
  else if (fb != 1) /* if fb == 1, the sticky bit is 1 (no possible carry) */
    { /* b has entirely been read */
      if (difw > cn)
        { /* c has entirely been read */
          if (rb < 0)
            rb = 0;
          fb = 0;
        }
      else if (diff_exp > MPFR_UEXP (aq2))
        { /* b is followed by at least a zero bit, then by c */
          if (rb < 0)
            rb = 0;
          fb = 1;
        }
      else
        {
          mp_size_t ck;
          int difs;

          MPFR_ASSERTD(difw >= 0 && cn >= difw);
          ck = cn - difw;
          difs = diff_exp % GMP_NUMB_BITS;

          if (difs == 0 && ck == 0)
            { /* c has entirely been read */
              if (rb < 0)
                rb = 0;
              fb = 0;
            }
          else
            {
              mp_limb_t cc;

              cc = difs ? (MPFR_ASSERTD(ck < cn),
                           cp[ck] << (GMP_NUMB_BITS - difs)) : cp[--ck];
              if (rb < 0)
                {
                  rb = cc >> (GMP_NUMB_BITS - 1);
                  cc &= ~MPFR_LIMB_HIGHBIT;
                }
              if (cc == 0 && rnd_mode == MPFR_RNDF)
                {
                  fb = 0;
                  goto rounding;
                }
              while (cc == 0)
                {
                  if (ck == 0)
                    {
                      fb = 0;
                      goto rounding;
                    }
                  cc = cp[--ck];
                } /* while */
              fb = 1;
            }
        }
    } /* fb != 1 */

 rounding:
  /* rnd_mode should be one of MPFR_RNDN, MPFR_RNDF, MPFR_RNDZ or MPFR_RNDA */
  if (MPFR_LIKELY(rnd_mode == MPFR_RNDN || rnd_mode == MPFR_RNDF))
    {
      if (fb == 0)
        {
          if (rb == 0)
            {
              inex = 0;
              goto set_exponent;
            }
          /* round to even */
          if (ap[0] & (MPFR_LIMB_ONE << sh))
            goto rndn_away;
          else
            goto rndn_zero;
        }
      if (rb == 0)
        {
        rndn_zero:
          inex = MPFR_IS_NEG(a) ? 1 : -1;
          goto set_exponent;
        }
      else
        {
        rndn_away:
          inex = MPFR_IS_POS(a) ? 1 : -1;
          goto add_one_ulp;
        }
    }
  else if (rnd_mode == MPFR_RNDZ)
    {
      inex = rb || fb ? (MPFR_IS_NEG(a) ? 1 : -1) : 0;
      goto set_exponent;
    }
  else
    {
      MPFR_ASSERTN (rnd_mode == MPFR_RNDA);
      inex = rb || fb ? (MPFR_IS_POS(a) ? 1 : -1) : 0;
      if (inex)
        goto add_one_ulp;
      else
        goto set_exponent;
    }

 add_one_ulp: /* add one unit in last place to a */
  if (MPFR_UNLIKELY(mpn_add_1 (ap, ap, an, MPFR_LIMB_ONE << sh)))
    {
      if (MPFR_UNLIKELY(exp == __gmpfr_emax))
        {
          inex = mpfr_overflow (a, rnd_mode, MPFR_SIGN(a));
          goto end_of_add;
        }
      exp++;
      ap[an-1] = MPFR_LIMB_HIGHBIT;
    }

 set_exponent:
  if (MPFR_UNLIKELY (exp < __gmpfr_emin))  /* possible if b and c are UBF's */
    {
      if (rnd_mode == MPFR_RNDN &&
          (exp < __gmpfr_emin - 1 ||
           (inex >= 0 && mpfr_powerof2_raw (a))))
        rnd_mode = MPFR_RNDZ;
      inex = mpfr_underflow (a, rnd_mode, MPFR_SIGN(a));
      goto end_of_add;
    }
  MPFR_SET_EXP (a, exp);

 end_of_add:
  MPFR_TMP_FREE(marker);
  MPFR_RET (inex);
}
