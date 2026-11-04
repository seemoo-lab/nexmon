/* mpfr_round_raw_generic -- Generic rounding function

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

#ifndef flag
# error "ERROR: flag must be defined (0 / 1)"
#endif
#ifndef use_inexp
# error "ERROR: use_enexp must be defined (0 / 1)"
#endif
#ifndef mpfr_round_raw_generic
# error "ERROR: mpfr_round_raw_generic must be defined"
#endif

/*
 * If flag = 0, puts in y the value of xp (with precision xprec and
 * sign 1 if negative=0, -1 otherwise) rounded to precision yprec and
 * direction rnd_mode. Supposes x is not zero nor NaN nor +/- Infinity
 * (i.e. *xp != 0). In that case, the return value is a possible carry
 * (0 or 1) that may happen during the rounding, in which case the result
 * is a power of two.
 *
 * If inexp != NULL, put in *inexp the inexact flag of the rounding (0, 1, -1).
 * In case of even rounding when rnd = MPFR_RNDN, put MPFR_EVEN_INEX (2) or
 * -MPFR_EVEN_INEX (-2) in *inexp.
 *
 * If flag = 1, just returns whether one should add 1 or not for rounding.
 *
 * Note: yprec may be < MPFR_PREC_MIN; in particular, it may be equal
 * to 1. In this case, the even rounding is done away from 0, which is
 * a natural generalization. Indeed, a number with 1-bit precision can
 * be seen as a subnormal number with more precision.
 *
 * MPFR_RNDNA is now supported, but needs to be tested [TODO] and is
 * still not part of the API. In particular, the MPFR_RNDNA value (-1)
 * may change in the future without notice.
 */

#if !(flag == 0 || flag == 1)
#error "flag must be 0 or 1"
#endif

int
mpfr_round_raw_generic(
#if flag == 0
                       mp_limb_t *yp,
#endif
                       const mp_limb_t *xp, mpfr_prec_t xprec,
                       int neg, mpfr_prec_t yprec, mpfr_rnd_t rnd_mode
#if use_inexp != 0
                       , int *inexp
#endif
                       )
{
  mp_size_t xsize, nw;
  mp_limb_t himask, lomask, sb;
  int rw, new_use_inexp;
#if flag == 0
  int carry;
#endif

#if use_inexp != 0
  MPFR_ASSERTD (inexp != ((int*) 0));
#endif
  MPFR_ASSERTD (neg == 0 || neg == 1);
#if flag == 1
  /* rnd_mode = RNDF is only possible for flag = 0. */
  MPFR_ASSERTD (rnd_mode != MPFR_RNDF);
#endif

  if (rnd_mode == MPFR_RNDF)
    {
#if use_inexp != 0
      *inexp = 0;  /* make sure it has a valid value */
#endif
      rnd_mode = MPFR_RNDZ;  /* faster */
      new_use_inexp = 0;
    }
  else
    {
      if (flag && !use_inexp &&
          (xprec <= yprec || MPFR_IS_LIKE_RNDZ (rnd_mode, neg)))
        return 0;
      new_use_inexp = use_inexp;
    }

  xsize = MPFR_PREC2LIMBS (xprec);
  nw = yprec / GMP_NUMB_BITS;
  rw = yprec & (GMP_NUMB_BITS - 1);

  if (MPFR_UNLIKELY(xprec <= yprec))
    { /* No rounding is necessary. */
      /* if yp=xp, maybe an overlap: mpn_copyd is OK when src <= dst */
      if (MPFR_LIKELY(rw))
        nw++;
      MPFR_ASSERTD(nw >= 1);
      MPFR_ASSERTD(nw >= xsize);
#if use_inexp != 0
      *inexp = 0;
#endif
#if flag == 0
      mpn_copyd (yp + (nw - xsize), xp, xsize);
      MPN_ZERO(yp, nw - xsize);
#endif
      return 0;
    }

  if (new_use_inexp || !MPFR_IS_LIKE_RNDZ(rnd_mode, neg))
    {
      mp_size_t k = xsize - nw - 1;

      if (MPFR_LIKELY(rw))
        {
          nw++;
          lomask = MPFR_LIMB_MASK (GMP_NUMB_BITS - rw);
          himask = ~lomask;
        }
      else
        {
          lomask = MPFR_LIMB_MAX;
          himask = MPFR_LIMB_MAX;
        }
      MPFR_ASSERTD(k >= 0);
      sb = xp[k] & lomask;  /* First non-significant bits */
      /* Rounding to nearest? */
      if (rnd_mode == MPFR_RNDN || rnd_mode == MPFR_RNDNA)
        {
          /* Rounding to nearest */
          mp_limb_t rbmask = MPFR_LIMB_ONE << (GMP_NUMB_BITS - 1 - rw);

          if ((sb & rbmask) == 0) /* rounding bit = 0 ? */
            goto rnd_RNDZ; /* yes, behave like rounding toward zero */
          /* Rounding to nearest with rounding bit = 1 */
          if (MPFR_UNLIKELY (rnd_mode == MPFR_RNDNA))
            goto away_addone_ulp; /* like rounding away from zero */
          sb &= ~rbmask; /* first bits after the rounding bit */
          while (MPFR_UNLIKELY(sb == 0) && k > 0)
            sb = xp[--k];
          if (MPFR_UNLIKELY(sb == 0)) /* Even rounding. */
            {
              /* sb == 0 && rnd_mode == MPFR_RNDN */
              sb = xp[xsize - nw] & (himask ^ (himask << 1));
              if (sb == 0)
                {
#if use_inexp != 0
                  *inexp = 2 * MPFR_EVEN_INEX * neg - MPFR_EVEN_INEX;
#endif
                  /* ((neg!=0)^(sb!=0)) ? MPFR_EVEN_INEX : -MPFR_EVEN_INEX */
                  /* since neg = 0 or 1 and sb = 0 */
#if flag == 0
                  mpn_copyi (yp, xp + xsize - nw, nw);
                  yp[0] &= himask;
#endif
                  return 0; /* sb != 0 && rnd_mode != MPFR_RNDZ */
                }
              else
                {
                away_addone_ulp:
                  /* sb != 0 && rnd_mode == MPFR_RNDN */
#if use_inexp != 0
                  *inexp = MPFR_EVEN_INEX - 2 * MPFR_EVEN_INEX * neg;
#endif
                  /* ((neg!=0)^(sb!=0)) ? MPFR_EVEN_INEX : -MPFR_EVEN_INEX */
                  /* since neg = 0 or 1 and sb != 0 */
                  goto rnd_RNDN_add_one_ulp;
                }
            }
          else /* sb != 0 && rnd_mode == MPFR_RNDN */
            {
#if use_inexp != 0
              *inexp = 1 - 2 * neg; /* neg == 0 ? 1 : -1 */
#endif
            rnd_RNDN_add_one_ulp:
#if flag == 1
              return 1; /* sb != 0 && rnd_mode != MPFR_RNDZ */
#else
              carry = mpn_add_1 (yp, xp + xsize - nw, nw,
                                 rw ?
                                 MPFR_LIMB_ONE << (GMP_NUMB_BITS - rw)
                                 : MPFR_LIMB_ONE);
              yp[0] &= himask;
              return carry;
#endif
            }
        }
      /* Rounding toward zero? */
      else if (MPFR_IS_LIKE_RNDZ(rnd_mode, neg))
        {
          /* rnd_mode == MPFR_RNDZ */
        rnd_RNDZ:
          while (MPFR_UNLIKELY (sb == 0) && k > 0)
            sb = xp[--k];
#if use_inexp != 0
          /* rnd_mode == MPFR_RNDZ and neg = 0 or 1 */
          /* ((neg != 0) ^ (rnd_mode != MPFR_RNDZ)) ? 1 : -1 */
          *inexp = MPFR_UNLIKELY (sb == 0) ? 0 : 2 * neg - 1;
#endif
#if flag == 0
          mpn_copyi (yp, xp + xsize - nw, nw);
          yp[0] &= himask;
#endif
          return 0; /* sb != 0 && rnd_mode != MPFR_RNDZ */
        }
      else
        {
          /* Rounding away from zero */
          while (MPFR_UNLIKELY (sb == 0) && k > 0)
            sb = xp[--k];
          if (MPFR_UNLIKELY (sb == 0))
            {
              /* sb = 0 && rnd_mode != MPFR_RNDZ */
#if use_inexp != 0
              /* ((neg != 0) ^ (rnd_mode != MPFR_RNDZ)) ? 1 : -1 */
              *inexp = 0;
#endif
#if flag == 0
              mpn_copyi (yp, xp + xsize - nw, nw);
              yp[0] &= himask;
#endif
              return 0;
            }
          else
            {
              /* sb != 0 && rnd_mode != MPFR_RNDZ */
#if use_inexp != 0
              *inexp = 1 - 2 * neg; /* neg == 0 ? 1 : -1 */
#endif
#if flag == 1
              return 1;
#else
              carry = mpn_add_1(yp, xp + xsize - nw, nw,
                                rw ? MPFR_LIMB_ONE << (GMP_NUMB_BITS - rw)
                                : MPFR_LIMB_ONE);
              yp[0] &= himask;
              return carry;
#endif
            }
        }
    }
  else
    {
      /* Rounding toward zero / no inexact flag */
#if flag == 0
      if (MPFR_LIKELY(rw))
        {
          nw++;
          himask = ~MPFR_LIMB_MASK (GMP_NUMB_BITS - rw);
        }
      else
        himask = MPFR_LIMB_MAX;
      mpn_copyi (yp, xp + xsize - nw, nw);
      yp[0] &= himask;
#endif
      return 0;
    }
}

#undef flag
#undef use_inexp
#undef mpfr_round_raw_generic
