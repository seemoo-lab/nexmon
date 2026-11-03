/* mpfr_sub1sp -- internal function to perform a "real" subtraction
   All the op must have the same precision

Copyright 2003-2025 Free Software Foundation, Inc.
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

/* define MPFR_FULLSUB to use alternate code in mpfr_sub1sp2 and mpfr_sub1sp2n
   (see comments in mpfr_sub1sp2) */
/* #define MPFR_FULLSUB */

#if MPFR_WANT_ASSERT >= 2
/* Check the result of mpfr_sub1sp with mpfr_sub1.

   Note: mpfr_sub1sp itself has two algorithms: one always valid and one
   faster for small precisions (up to 3 limbs). The latter one is disabled
   if MPFR_GENERIC_ABI is defined. When MPFR_WANT_ASSERT >= 2, it could be
   interesting to compare the results of these different algorithms. For
   the time being, this is currently done by running the same code on the
   same data with and without MPFR_GENERIC_ABI defined, where we have the
   following comparisons in small precisions:
     mpfr_sub1sp slow <-> mpfr_sub1 when MPFR_GENERIC_ABI is defined;
     mpfr_sub1sp fast <-> mpfr_sub1 when MPFR_GENERIC_ABI is not defined.
   By transitivity, the absence of failures implies that the 3 results are
   the same.
*/

int mpfr_sub1sp_ref (mpfr_ptr, mpfr_srcptr, mpfr_srcptr, mpfr_rnd_t);
int mpfr_sub1sp (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mpfr_t tmpa, tmpb, tmpc;
  mpfr_flags_t old_flags, flags, flags2;
  int inexb, inexc, inexact, inexact2;

  if (rnd_mode == MPFR_RNDF)
    return mpfr_sub1sp_ref (a, b, c, rnd_mode);

  old_flags = __gmpfr_flags;

  mpfr_init2 (tmpa, MPFR_PREC (a));
  mpfr_init2 (tmpb, MPFR_PREC (b));
  mpfr_init2 (tmpc, MPFR_PREC (c));

  inexb = mpfr_set (tmpb, b, MPFR_RNDN);
  MPFR_ASSERTN (inexb == 0);

  inexc = mpfr_set (tmpc, c, MPFR_RNDN);
  MPFR_ASSERTN (inexc == 0);

  MPFR_ASSERTN (__gmpfr_flags == old_flags);

  inexact2 = mpfr_sub1 (tmpa, tmpb, tmpc, rnd_mode);
  flags2 = __gmpfr_flags;

  __gmpfr_flags = old_flags;
  inexact = mpfr_sub1sp_ref (a, b, c, rnd_mode);
  flags = __gmpfr_flags;

  /* Convert the ternary values to (-1,0,1). */
  inexact2 = VSIGN (inexact2);
  inexact = VSIGN (inexact);

  if (! mpfr_equal_p (tmpa, a) || inexact != inexact2 || flags != flags2)
    {
      fprintf (stderr, "sub1 & sub1sp return different values for %s\n"
               "Prec_a = %lu, Prec_b = %lu, Prec_c = %lu\nB = ",
               mpfr_print_rnd_mode (rnd_mode),
               (unsigned long) MPFR_PREC (a),
               (unsigned long) MPFR_PREC (b),
               (unsigned long) MPFR_PREC (c));
      mpfr_fdump (stderr, tmpb);
      fprintf (stderr, "C = ");
      mpfr_fdump (stderr, tmpc);
      fprintf (stderr, "sub1  : ");
      mpfr_fdump (stderr, tmpa);
      fprintf (stderr, "sub1sp: ");
      mpfr_fdump (stderr, a);
      fprintf (stderr, "sub1  : ternary = %2d, flags =", inexact2);
      flags_fout (stderr, flags2);
      fprintf (stderr, "sub1sp: ternary = %2d, flags =", inexact);
      flags_fout (stderr, flags);
      MPFR_ASSERTN (0);
    }
  mpfr_clears (tmpa, tmpb, tmpc, (mpfr_ptr) 0);
  return inexact;
}
# define mpfr_sub1sp mpfr_sub1sp_ref
#endif  /* MPFR_WANT_ASSERT >= 2 */

#if !defined(MPFR_GENERIC_ABI)

/* the sub1sp1_extracted.c is not ready yet */

#if 0 && defined(MPFR_WANT_PROVEN_CODE) && GMP_NUMB_BITS == 64 && \
  UINT_MAX == 0xffffffff && MPFR_PREC_BITS == 64 && \
  _MPFR_PREC_FORMAT == 3 && _MPFR_EXP_FORMAT == _MPFR_PREC_FORMAT

/* The code assumes that mp_limb_t has 64 bits exactly, unsigned int
   has 32 bits exactly, mpfr_prec_t and mpfr_exp_t are of type long,
   which has 64 bits exactly. */

#include "sub1sp1_extracted.c"

#else

/* special code for p < GMP_NUMB_BITS */
static int
mpfr_sub1sp1 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode,
              mpfr_prec_t p)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mpfr_prec_t cnt, INITIALIZED(sh);
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t a0;
  mp_limb_t mask;
  mpfr_uexp_t d;

  MPFR_ASSERTD(p < GMP_NUMB_BITS);

  if (bx == cx)
    {
      if (MPFR_UNLIKELY(bp[0] == cp[0])) /* result is zero */
        {
          if (rnd_mode == MPFR_RNDD)
            MPFR_SET_NEG(a);
          else
            MPFR_SET_POS(a);
          MPFR_SET_ZERO(a);
          MPFR_RET (0);
        }
      else if (cp[0] > bp[0]) /* borrow: |c| > |b| */
        {
          a0 = cp[0] - bp[0];
          MPFR_SET_OPPOSITE_SIGN (a, b);
        }
      else /* bp[0] > cp[0] */
        {
          a0 = bp[0] - cp[0];
          MPFR_SET_SAME_SIGN (a, b);
        }

      /* now a0 != 0 */
      MPFR_ASSERTD(a0 != 0);
      count_leading_zeros (cnt, a0);
      ap[0] = a0 << cnt;
      bx -= cnt;
      rb = sb = 0;
      /* Note: sh is not initialized, but will not be used in this case. */
    }
  else
    {
      if (bx < cx)  /* swap b and c */
        {
          mpfr_exp_t tx;
          mp_limb_t *tp;
          tx = bx; bx = cx; cx = tx;
          tp = bp; bp = cp; cp = tp;
          MPFR_SET_OPPOSITE_SIGN (a, b);
        }
      else
        {
          MPFR_SET_SAME_SIGN (a, b);
        }
      MPFR_ASSERTD (bx > cx);
      d = (mpfr_uexp_t) bx - cx;
      sh = GMP_NUMB_BITS - p;
      mask = MPFR_LIMB_MASK(sh);
      if (d < GMP_NUMB_BITS)
        {
          sb = - (cp[0] << (GMP_NUMB_BITS - d)); /* neglected part of -c */
          /* Note that "a0 = bp[0] - (cp[0] >> d) - (sb != 0);" is faster
             on some other machines and has no immediate dependencies for
             the first subtraction. In the future, make sure that the code
             is recognized as a *single* subtraction with borrow and/or use
             a builtin when available (currently provided by Clang, but not
             by GCC); create a new macro for that. See the TODO later.
             Note also that with Clang, the constant 0 for the first
             subtraction instead of a variable breaks the optimization:
             https://llvm.org/bugs/show_bug.cgi?id=31754 */
          a0 = bp[0] - (sb != 0) - (cp[0] >> d);
          /* a0 cannot be zero here since:
             a) if d >= 2, then a0 >= 2^(w-1) - (2^(w-2)-1) with
                w = GMP_NUMB_BITS, thus a0 - 1 >= 2^(w-2),
             b) if d = 1, then since p < GMP_NUMB_BITS we have sb=0.
          */
          MPFR_ASSERTD(a0 > 0);
          count_leading_zeros (cnt, a0);
          if (cnt)
            a0 = (a0 << cnt) | (sb >> (GMP_NUMB_BITS - cnt));
          sb <<= cnt;
          bx -= cnt;
          /* sh > 0 since p < GMP_NUMB_BITS */
          MPFR_ASSERTD(sh > 0);
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else /* d >= GMP_NUMB_BITS */
        {
          if (bp[0] > MPFR_LIMB_HIGHBIT)
            {
              /* We compute b - ulp(b), and the remainder ulp(b) - c satisfies:
                 1/2 ulp(b) < ulp(b) - c < ulp(b), thus rb = sb = 1. */
              ap[0] = bp[0] - (MPFR_LIMB_ONE << sh);
              rb = 1;
            }
          else
            {
              /* Warning: since we have an exponent decrease, when
                 p = GMP_NUMB_BITS - 1 and d = GMP_NUMB_BITS, the round bit
                 corresponds to the upper bit of -c. In that case rb = 0 and
                 sb = 1, except when c0 = MPFR_LIMB_HIGHBIT where rb = 1 and
                 sb = 0. */
              rb = sh > 1 || d > GMP_NUMB_BITS || cp[0] == MPFR_LIMB_HIGHBIT;
              /* sb=1 below is incorrect when p = GMP_NUMB_BITS - 1,
                 d = GMP_NUMB_BITS and c0 = MPFR_LIMB_HIGHBIT, but in
                 that case the even rule wound round up too. */
              ap[0] = ~mask;
              bx --;
              /* Warning: if d = GMP_NUMB_BITS and c0 = 1000...000, then
                 b0 - c0 = |0111...111|1000...000|, which after the shift
                 becomes |111...111|000...000| thus if p = GMP_NUMB_BITS-1
                 we have rb = 1 but sb = 0. However, in this case the round
                 even rule will round up, which is what we get with sb = 1:
                 the final result will be correct, while sb is incorrect. */
            }
          sb = 1;
        }
    }

  /* now perform rounding */

  /* Warning: MPFR considers underflow *after* rounding with an unbounded
     exponent range. However, since b and c have same precision p, they are
     multiples of 2^(emin-p), likewise for b-c. Thus if bx < emin, the
     subtraction (with an unbounded exponent range) is exact, so that bx is
     also the exponent after rounding with an unbounded exponent range. */
  if (MPFR_UNLIKELY(bx < __gmpfr_emin))
    {
      /* For RNDN, mpfr_underflow always rounds away, thus for |a| <= 2^(emin-2)
         we have to change to RNDZ. This corresponds to:
         (a) either bx < emin - 1
         (b) or bx = emin - 1 and ap[0] = 1000....000 (in this case necessarily
             rb = sb = 0 since b-c is multiple of 2^(emin-p) */
      if (rnd_mode == MPFR_RNDN &&
          (bx < __gmpfr_emin - 1 || ap[0] == MPFR_LIMB_HIGHBIT))
        {
          MPFR_ASSERTD(rb == 0 && sb == 0);
          rnd_mode = MPFR_RNDZ;
        }
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN(a));
    }

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET (0);
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      if (MPFR_UNLIKELY(ap[0] == 0))
        {
          ap[0] = MPFR_LIMB_HIGHBIT;
          /* Note: bx+1 cannot exceed __gmpfr_emax, since |a| <= |b|, thus
             bx+1 is at most equal to the original exponent of b. */
          MPFR_ASSERTD(bx + 1 <= __gmpfr_emax);
          MPFR_SET_EXP (a, bx + 1);
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

#endif /* MPFR_WANT_PROVEN_CODE */

/* special code for p = GMP_NUMB_BITS */
static int
mpfr_sub1sp1n (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mpfr_prec_t cnt;
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t a0;
  mpfr_uexp_t d;

  MPFR_ASSERTD(MPFR_PREC(a) == GMP_NUMB_BITS);
  MPFR_ASSERTD(MPFR_PREC(b) == GMP_NUMB_BITS);
  MPFR_ASSERTD(MPFR_PREC(c) == GMP_NUMB_BITS);

  if (bx == cx)
    {
      a0 = bp[0] - cp[0];
      if (a0 == 0) /* result is zero */
        {
          if (rnd_mode == MPFR_RNDD)
            MPFR_SET_NEG(a);
          else
            MPFR_SET_POS(a);
          MPFR_SET_ZERO(a);
          MPFR_RET (0);
        }
      else if (a0 > bp[0]) /* borrow: |c| > |b| */
        {
          MPFR_SET_OPPOSITE_SIGN (a, b);
          a0 = -a0;
        }
      else /* bp[0] > cp[0] */
        MPFR_SET_SAME_SIGN (a, b);

      /* now a0 != 0 */
      MPFR_ASSERTD(a0 != 0);
      count_leading_zeros (cnt, a0);
      ap[0] = a0 << cnt;
      bx -= cnt;
      rb = sb = 0;
    }
  else
    {
      if (bx < cx)  /* swap b and c */
        {
          mpfr_exp_t tx;
          mp_limb_t *tp;
          tx = bx; bx = cx; cx = tx;
          tp = bp; bp = cp; cp = tp;
          MPFR_SET_OPPOSITE_SIGN (a, b);
        }
      else
        {
          MPFR_SET_SAME_SIGN (a, b);
        }
      MPFR_ASSERTD (bx > cx);
      d = (mpfr_uexp_t) bx - cx;
      if (d < GMP_NUMB_BITS)
        {
          sb = - (cp[0] << (GMP_NUMB_BITS - d)); /* neglected part of -c */
          a0 = bp[0] - (sb != 0) - (cp[0] >> d);
          /* a0 can only be zero when d=1, b0 = B/2, and c0 = B-1, where
             B = 2^GMP_NUMB_BITS, thus b0 - c0/2 = 1/2 */
          if (a0 == MPFR_LIMB_ZERO)
            {
              bx -= GMP_NUMB_BITS;
              ap[0] = MPFR_LIMB_HIGHBIT;
              rb = sb = 0;
            }
          else
            {
              count_leading_zeros (cnt, a0);
              if (cnt)
                a0 = (a0 << cnt) | (sb >> (GMP_NUMB_BITS - cnt));
              sb <<= cnt;
              bx -= cnt;
              rb = sb & MPFR_LIMB_HIGHBIT;
              sb &= ~MPFR_LIMB_HIGHBIT;
              ap[0] = a0;
            }
        }
      else /* d >= GMP_NUMB_BITS */
        {
          /* We compute b - ulp(b) */
          if (bp[0] > MPFR_LIMB_HIGHBIT)
            {
              /* If d = GMP_NUMB_BITS, rb = 0 and sb = 1,
                 unless c0 = MPFR_LIMB_HIGHBIT in which case rb = 1 and sb = 0.
                 If d > GMP_NUMB_BITS, rb = sb = 1. */
              rb = d > GMP_NUMB_BITS || cp[0] == MPFR_LIMB_HIGHBIT;
              sb = d > GMP_NUMB_BITS || cp[0] != MPFR_LIMB_HIGHBIT;
              ap[0] = bp[0] - MPFR_LIMB_ONE;
            }
          else
            {
              /* Warning: in this case a0 is shifted by one!
                 If d=GMP_NUMB_BITS:
                   (a) if c0 = MPFR_LIMB_HIGHBIT, a0 = 111...111, rb = sb = 0
                   (b) otherwise, a0 = 111...110, rb = -c0 >= 01000...000,
                       sb = (-c0) << 2
                 If d=GMP_NUMB_BITS+1: a0 = 111...111
                   (c) if c0 = MPFR_LIMB_HIGHBIT, rb = 1 and sb = 0
                   (d) otherwise rb = 0 and sb = 1
                 If d > GMP_NUMB_BITS+1:
                   (e) a0 = 111...111, rb = sb = 1
              */
              bx --;
              if (d == GMP_NUMB_BITS && cp[0] > MPFR_LIMB_HIGHBIT)
                { /* case (b) */
                  rb = MPFR_LIMB(-cp[0]) >= (MPFR_LIMB_HIGHBIT >> 1);
                  sb = MPFR_LIMB(-cp[0]) << 2;
                  ap[0] = -(MPFR_LIMB_ONE << 1);
                }
              else /* cases (a), (c), (d) and (e) */
                {
                  /* rb=1 in case (e) and case (c) */
                  rb = d > GMP_NUMB_BITS + 1
                    || (d == GMP_NUMB_BITS + 1 && cp[0] == MPFR_LIMB_HIGHBIT);
                  /* sb = 1 in case (d) and (e) */
                  sb = d > GMP_NUMB_BITS + 1
                    || (d == GMP_NUMB_BITS + 1 && cp[0] > MPFR_LIMB_HIGHBIT);
                  /* Warning: only set ap[0] last, otherwise in case ap=cp,
                     the above comparisons involving cp[0] would be wrong */
                  ap[0] = -MPFR_LIMB_ONE;
                }
            }
        }
    }

  /* now perform rounding */

  /* Warning: MPFR considers underflow *after* rounding with an unbounded
     exponent range. However, since b and c have same precision p, they are
     multiples of 2^(emin-p), likewise for b-c. Thus if bx < emin, the
     subtraction (with an unbounded exponent range) is exact, so that bx is
     also the exponent after rounding with an unbounded exponent range. */
  if (MPFR_UNLIKELY(bx < __gmpfr_emin))
    {
      /* For RNDN, mpfr_underflow always rounds away, thus for |a| <= 2^(emin-2)
         we have to change to RNDZ. This corresponds to:
         (a) either bx < emin - 1
         (b) or bx = emin - 1 and ap[0] = 1000....000 (in this case necessarily
             rb = sb = 0 since b-c is multiple of 2^(emin-p) */
      if (rnd_mode == MPFR_RNDN &&
          (bx < __gmpfr_emin - 1 || ap[0] == MPFR_LIMB_HIGHBIT))
        {
          MPFR_ASSERTD(rb == 0 && sb == 0);
          rnd_mode = MPFR_RNDZ;
        }
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN(a));
    }

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET (0);
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & MPFR_LIMB_ONE) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE;
      if (MPFR_UNLIKELY(ap[0] == 0))
        {
          ap[0] = MPFR_LIMB_HIGHBIT;
          /* Note: bx+1 cannot exceed __gmpfr_emax, since |a| <= |b|, thus
             bx+1 is at most equal to the original exponent of b. */
          MPFR_ASSERTD(bx + 1 <= __gmpfr_emax);
          MPFR_SET_EXP (a, bx + 1);
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

/* special code for GMP_NUMB_BITS < p < 2*GMP_NUMB_BITS */
static int
mpfr_sub1sp2 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode,
              mpfr_prec_t p)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mpfr_prec_t cnt, INITIALIZED(sh);
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t mask, a0, a1;
  mpfr_uexp_t d;

  MPFR_ASSERTD(GMP_NUMB_BITS < p && p < 2 * GMP_NUMB_BITS);

  if (bx == cx) /* subtraction is exact in this case */
    {
      /* first compute a0: if the compiler is smart enough, it will use the generated
         borrow to get for free the term (bp[0] < cp[0]) */
      a0 = bp[0] - cp[0];
      a1 = bp[1] - cp[1] - (bp[0] < cp[0]);
      if (a1 == 0 && a0 == 0) /* result is zero */
        {
          if (rnd_mode == MPFR_RNDD)
            MPFR_SET_NEG(a);
          else
            MPFR_SET_POS(a);
          MPFR_SET_ZERO(a);
          MPFR_RET (0);
        }
      else if (a1 >= bp[1]) /* borrow: |c| > |b| */
        {
          MPFR_SET_OPPOSITE_SIGN (a, b);
          /* a = b-c mod 2^(2*GMP_NUMB_BITS) */
          a0 = -a0;
          a1 = -a1 - (a0 != 0);
        }
      else /* bp[0] > cp[0] */
        MPFR_SET_SAME_SIGN (a, b);

      if (a1 == 0)
        {
          a1 = a0;
          a0 = 0;
          bx -= GMP_NUMB_BITS;
        }

      /* now a1 != 0 */
      MPFR_ASSERTD(a1 != 0);
      count_leading_zeros (cnt, a1);
      if (cnt)
        {
          ap[1] = (a1 << cnt) | (a0 >> (GMP_NUMB_BITS - cnt));
          ap[0] = a0 << cnt;
          bx -= cnt;
        }
      else
        {
          ap[1] = a1;
          ap[0] = a0;
        }
      rb = sb = 0;
      /* Note: sh is not initialized, but will not be used in this case. */
    }
  else
    {
      mp_limb_t t;

      if (bx < cx)  /* swap b and c */
        {
          mpfr_exp_t tx;
          mp_limb_t *tp;
          tx = bx; bx = cx; cx = tx;
          tp = bp; bp = cp; cp = tp;
          MPFR_SET_OPPOSITE_SIGN (a, b);
        }
      else
        {
          MPFR_SET_SAME_SIGN (a, b);
        }
      MPFR_ASSERTD (bx > cx);
      d = (mpfr_uexp_t) bx - cx;
      sh =  2 * GMP_NUMB_BITS - p;
      mask = MPFR_LIMB_MASK(sh);
      if (d < GMP_NUMB_BITS)
        {
          t = (cp[1] << (GMP_NUMB_BITS - d)) | (cp[0] >> d);
          /* TODO: Change the code to generate a full subtraction with borrow,
             avoiding the test on sb and the corresponding correction. Note
             that Clang has builtins:
               https://clang.llvm.org/docs/LanguageExtensions.html#multiprecision-arithmetic-builtins
             but the generated code may not be good:
               https://llvm.org/bugs/show_bug.cgi?id=20748
             With the current source code, Clang generates on x86_64:
               1. sub %rsi,%rbx for the first subtraction in a1;
               2. sub %rdi,%rax for the subtraction in a0;
               3. sbb $0x0,%rbx for the second subtraction in a1, i.e. just
                  subtracting the borrow out from (2).
             So, Clang recognizes the borrow, but doesn't merge (1) and (3).
             Bug: https://llvm.org/bugs/show_bug.cgi?id=25858
             For GCC: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=79173
          */
#ifndef MPFR_FULLSUB
          a0 = bp[0] - t;
          a1 = bp[1] - (cp[1] >> d) - (bp[0] < t);
          sb = cp[0] << (GMP_NUMB_BITS - d); /* neglected part of c */
          if (sb)
            {
              a1 -= (a0 == 0);
              a0 --;
              /* a = a1,a0 cannot become zero here, since:
                 a) if d >= 2, then a1 >= 2^(w-1) - (2^(w-2)-1) with
                    w = GMP_NUMB_BITS, thus a1 - 1 >= 2^(w-2),
                 b) if d = 1, then since p < 2*GMP_NUMB_BITS we have sb=0. */
              MPFR_ASSERTD(a1 > 0 || a0 > 0);
              sb = -sb; /* 2^GMP_NUMB_BITS - sb */
            }
#else
          sb = - (cp[0] << (GMP_NUMB_BITS - d));
          a0 = bp[0] - t - (sb != 0);
          a1 = bp[1] - (cp[1] >> d) - (bp[0] < t || (bp[0] == t && sb != 0));
#endif
          if (a1 == 0)
            {
              /* this implies d=1, which in turn implies sb=0 */
              MPFR_ASSERTD(sb == 0);
              a1 = a0;
              a0 = 0; /* should be a0 = sb */
              /* since sb=0 already, no need to set it to 0 */
              bx -= GMP_NUMB_BITS;
            }
          /* now a1 != 0 */
          MPFR_ASSERTD(a1 != 0);
          count_leading_zeros (cnt, a1);
          if (cnt)
            {
              ap[1] = (a1 << cnt) | (a0 >> (GMP_NUMB_BITS - cnt));
              a0 = (a0 << cnt) | (sb >> (GMP_NUMB_BITS - cnt));
              sb <<= cnt;
              bx -= cnt;
            }
          else
            ap[1] = a1;
          /* sh > 0 since p < 2*GMP_NUMB_BITS */
          MPFR_ASSERTD(sh > 0);
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else if (d < 2 * GMP_NUMB_BITS)
        {  /* GMP_NUMB_BITS <= d < 2*GMP_NUMB_BITS */
          /* warning: the most significant bit of sb might become the least
             significant bit of a0 below */
          sb = (d == GMP_NUMB_BITS) ? cp[0]
            : (cp[1] << (2*GMP_NUMB_BITS - d)) | (cp[0] != 0);
          t = (cp[1] >> (d - GMP_NUMB_BITS)) + (sb != 0);
          /* warning: t might overflow to 0 if d=GMP_NUMB_BITS and sb <> 0 */
          a0 = bp[0] - t;
          a1 = bp[1] - (bp[0] < t) - (t == 0 && sb != 0);
          sb = -sb;
          /* since bp[1] has its most significant bit set, we can have an
             exponent decrease of at most one */
          if (a1 < MPFR_LIMB_HIGHBIT)
            {
              ap[1] = (a1 << 1) | (a0 >> (GMP_NUMB_BITS - 1));
              a0 = (a0 << 1) | (sb >> (GMP_NUMB_BITS - 1));
              sb <<= 1;
              bx --;
            }
          else
            ap[1] = a1;
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else /* d >= 2*GMP_NUMB_BITS */
        {
          /* We compute b - ulp(b), and the remainder ulp(b) - c satisfies:
             1/2 ulp(b) < ulp(b) - c < ulp(b), thus rb = sb = 1, unless we
             had an exponent decrease. */
          t = MPFR_LIMB_ONE << sh;
          a0 = bp[0] - t;
          a1 = bp[1] - (bp[0] < t);
          if (a1 < MPFR_LIMB_HIGHBIT)
            {
              /* necessarily we had b = 1000...000 */
              /* Warning: since we have an exponent decrease, when
                 p = 2*GMP_NUMB_BITS - 1 and d = 2*GMP_NUMB_BITS, the round bit
                 corresponds to the upper bit of -c. In that case rb = 0 and
                 sb = 1, except when c = 1000...000 where rb = 1 and sb = 0. */
              rb = sh > 1 || d > 2 * GMP_NUMB_BITS
                || (cp[1] == MPFR_LIMB_HIGHBIT && cp[0] == MPFR_LIMB_ZERO);
              /* sb=1 below is incorrect when p = 2*GMP_NUMB_BITS - 1,
                 d = 2*GMP_NUMB_BITS and c = 1000...000, but in
                 that case the even rule wound round up too. */
              ap[0] = ~mask;
              ap[1] = MPFR_LIMB_MAX;
              bx --;
            }
          else
            {
              ap[0] = a0;
              ap[1] = a1;
              rb = 1;
            }
          sb = 1;
        }
    }

  /* now perform rounding */

  /* Warning: MPFR considers underflow *after* rounding with an unbounded
     exponent range. However, since b and c have same precision p, they are
     multiples of 2^(emin-p), likewise for b-c. Thus if bx < emin, the
     subtraction (with an unbounded exponent range) is exact, so that bx is
     also the exponent after rounding with an unbounded exponent range. */
  if (MPFR_UNLIKELY(bx < __gmpfr_emin))
    {
      /* for RNDN, mpfr_underflow always rounds away, thus for |a|<=2^(emin-2)
         we have to change to RNDZ */
      if (rnd_mode == MPFR_RNDN &&
          (bx < __gmpfr_emin - 1 ||
           (ap[1] == MPFR_LIMB_HIGHBIT && ap[0] == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN(a));
    }

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET (0);
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      ap[1] += (ap[0] == 0);
      if (MPFR_UNLIKELY(ap[1] == 0))
        {
          ap[1] = MPFR_LIMB_HIGHBIT;
          /* Note: bx+1 cannot exceed __gmpfr_emax, since |a| <= |b|, thus
             bx+1 is at most equal to the original exponent of b. */
          MPFR_ASSERTD(bx + 1 <= __gmpfr_emax);
          MPFR_SET_EXP (a, bx + 1);
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

/* special code for p = 2*GMP_NUMB_BITS */
static int
mpfr_sub1sp2n (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mpfr_prec_t cnt;
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t a0, a1;
  mpfr_uexp_t d;

/* this function is inspired by mpfr_sub1sp2 (for the operations of the
   2-limb arrays) and by mpfr_sub1sp1n (for the different cases to handle) */

  if (bx == cx) /* subtraction is exact in this case */
    {
      a0 = bp[0] - cp[0];
      a1 = bp[1] - cp[1] - (bp[0] < cp[0]);
      if (a1 == 0 && a0 == 0) /* result is zero */
        {
          if (rnd_mode == MPFR_RNDD)
            MPFR_SET_NEG(a);
          else
            MPFR_SET_POS(a);
          MPFR_SET_ZERO(a);
          MPFR_RET (0);
        }
      /* since B/2 <= bp[1], cp[1] < B with B=2^GMP_NUMB_BITS,
         if no borrow we have 0 <=  bp[1] - cp[1] - x < B/2
         where x = (bp[0] < cp[0]) is 0 or 1, thus a1 < B/2 <= bp[1] */
      else if (a1 >= bp[1]) /* borrow: |c| > |b| */
        {
          MPFR_SET_OPPOSITE_SIGN (a, b);
          /* negate [a1,a0] */
          a0 = -a0;
          a1 = -a1 - (a0 != 0);
        }
      else /* no borrow */
        MPFR_SET_SAME_SIGN (a, b);

      /* now [a1,a0] is the absolute value of b - c,
         maybe not normalized */
      if (a1 == 0)
        {
          a1 = a0;
          a0 = 0;
          bx -= GMP_NUMB_BITS;
        }

      /* now a1 != 0 */
      MPFR_ASSERTD(a1 != 0);
      count_leading_zeros (cnt, a1);
      if (cnt)
        {
          /* shift [a1,a0] left by cnt bit and store in result */
          ap[1] = (a1 << cnt) | (a0 >> (GMP_NUMB_BITS - cnt));
          ap[0] = a0 << cnt;
          bx -= cnt;
        }
      else
        {
          ap[1] = a1;
          ap[0] = a0;
        }
      rb = sb = 0; /* the subtraction is exact */
    }
  else
    {
      mp_limb_t t;

      if (bx < cx)  /* swap b and c */
        {
          mpfr_exp_t tx;
          mp_limb_t *tp;
          tx = bx; bx = cx; cx = tx;
          tp = bp; bp = cp; cp = tp;
          MPFR_SET_OPPOSITE_SIGN (a, b);
        }
      else
        {
          MPFR_SET_SAME_SIGN (a, b);
        }
      MPFR_ASSERTD (bx > cx);
      d = (mpfr_uexp_t) bx - cx;
      if (d < GMP_NUMB_BITS)
        {
          t = (cp[1] << (GMP_NUMB_BITS - d)) | (cp[0] >> d);
          /* t is the part that should be subtracted to bp[0]:
             |      a1       |      a0       |
             |     bp[1]     |     bp[0]     |
             |    cp[1]>>d   |      t        |     sb     | */
#ifndef MPFR_FULLSUB
          a0 = bp[0] - t;
          a1 = bp[1] - (cp[1] >> d) - (bp[0] < t);
          sb = cp[0] << (GMP_NUMB_BITS - d); /* neglected part of c */
          /* now negate sb and subtract borrow to a0 if sb <> 0 */
          if (sb)
            {
              a1 -= (a0 == 0);
              a0 --;
              /* a = a1,a0 can only be zero when d=1, b = 0.1000...000*2^bx,
                 and c = 0.111...111*2^(bx-1). In that case (where we have
                 sb = MPFR_LIMB_HIGHBIT below), the subtraction is exact, the
                 result is b/2^(2*GMP_NUMB_BITS). This case is dealt below. */
              sb = -sb;
            }
#else
          sb = - (cp[0] << (GMP_NUMB_BITS - d));
          a0 = bp[0] - t - (sb != 0);
          a1 = bp[1] - (cp[1] >> d) - (bp[0] < t || (bp[0] == t && sb != 0));
#endif
          /* now the result is formed of [a1,a0,sb], which might not be
             normalized */
          if (a1 == MPFR_LIMB_ZERO)
            {
              /* this implies d=1 */
              MPFR_ASSERTD(d == 1);
              a1 = a0;
              a0 = sb;
              sb = MPFR_LIMB_ZERO;
              bx -= GMP_NUMB_BITS;
            }
          if (a1 == MPFR_LIMB_ZERO) /* case a = a1,a0 = 0 mentioned above */
            {
              MPFR_ASSERTD(a0 == MPFR_LIMB_HIGHBIT); /* was sb above */
              a1 = a0;
              a0 = sb;
              bx -= GMP_NUMB_BITS;
              sb = MPFR_LIMB_ZERO;
            }
          else
            {
              count_leading_zeros (cnt, a1);
              if (cnt)
                {
                  /* shift [a1,a0,sb] left by cnt bits and adjust exponent */
                  a1 = (a1 << cnt) | (a0 >> (GMP_NUMB_BITS - cnt));
                  a0 = (a0 << cnt) | (sb >> (GMP_NUMB_BITS - cnt));
                  sb <<= cnt;
                  bx -= cnt;
                }
            }
          rb = sb & MPFR_LIMB_HIGHBIT;
          sb = sb & ~MPFR_LIMB_HIGHBIT;
          ap[1] = a1;
          ap[0] = a0;
        }
      else if (d < 2 * GMP_NUMB_BITS)
        {  /* GMP_NUMB_BITS <= d < 2*GMP_NUMB_BITS:
              compute t, the part to be subtracted to bp[0],
              and sb, the neglected part of c:
             |      a1       |      a0       |
             |     bp[1]     |     bp[0]     |
                             |      t        |     sb     | */
          /* warning: we should not ignore the low bits from cp[0]
             in case d > GMP_NUMB_BITS */
          sb = (d == GMP_NUMB_BITS) ? cp[0]
            : (cp[1] << (2*GMP_NUMB_BITS - d))
              | (cp[0] >> (d - GMP_NUMB_BITS))
              | ((cp[0] << (2*GMP_NUMB_BITS - d)) != 0);
          t = (cp[1] >> (d - GMP_NUMB_BITS)) + (sb != 0);
          /* Warning: t might overflow to 0 if d=GMP_NUMB_BITS, sb <> 0,
             and cp[1] = 111...111 */
          a0 = bp[0] - t;
          a1 = bp[1] - (bp[0] < t) - (t == 0 && sb != 0);
          sb = -sb;
          /* now the result is [a1,a0,sb]. Since bp[1] has its most significant
             bit set, we can have an exponent decrease of at most one */
          if (a1 < MPFR_LIMB_HIGHBIT)
            {
              /* shift [a1,a0] left by 1 bit */
              a1 = (a1 << 1) | (a0 >> (GMP_NUMB_BITS - 1));
              MPFR_ASSERTD(a1 >= MPFR_LIMB_HIGHBIT);
              a0 = (a0 << 1) | (sb >> (GMP_NUMB_BITS - 1));
              sb <<= 1;
              bx --;
            }
          ap[1] = a1;
          ap[0] = a0;
          rb = sb & MPFR_LIMB_HIGHBIT;
          sb = sb & ~MPFR_LIMB_HIGHBIT;
        }
      else
        { /* d >= 2*GMP_NUMB_BITS:
             |      a1       |      a0       |
             |     bp[1]     |     bp[0]     |
                                                |   cp[1]   |   cp[0]   | */
          /* we mimic the case d >= GMP_NUMB_BITS of mpfr_sub1sp1n */
          int tst = cp[1] == MPFR_LIMB_HIGHBIT && cp[0] == MPFR_LIMB_ZERO;
          /* if d = 2 * GMP_NUMB_BITS and tst=1, c = 1/2*ulp(b) */
          if (bp[1] > MPFR_LIMB_HIGHBIT || bp[0] > MPFR_LIMB_ZERO)
            {
              /* no borrow in b - ulp(b) */
              rb = d > 2 * GMP_NUMB_BITS || tst;
              sb = d > 2 * GMP_NUMB_BITS || !tst;
              ap[1] = bp[1] - (bp[0] == MPFR_LIMB_ZERO);
              ap[0] = bp[0] - MPFR_LIMB_ONE;
            }
          else
            {
              /* b = 1000...000, thus subtracting c yields an exponent shift */
              bx --;
              if (d == 2 * GMP_NUMB_BITS && !tst) /* c > 1/2*ulp(b) */
                {
                  t = -cp[1] - (cp[0] > MPFR_LIMB_ZERO);
                  /* the rounding bit is the 2nd most significant bit of t
                     (where the most significant bit of t is necessarily 0),
                     and the sticky bit is formed by the remaining bits of t,
                     and those from -cp[0] */
                  rb = t >= (MPFR_LIMB_HIGHBIT >> 1);
                  sb = (t << 2) | cp[0];
                  ap[1] = MPFR_LIMB_MAX;
                  ap[0] = -(MPFR_LIMB_ONE << 1);
                }
              else /* c <= 1/2*ulp(b) */
                {
                  rb = d > 2 * GMP_NUMB_BITS + 1
                    || (d == 2 * GMP_NUMB_BITS + 1 && tst);
                  sb = d > 2 * GMP_NUMB_BITS + 1
                    || (d == 2 * GMP_NUMB_BITS + 1 && !tst);
                  ap[1] = -MPFR_LIMB_ONE;
                  ap[0] = -MPFR_LIMB_ONE;
                }
            }
        }
    }

  /* now perform rounding */

  /* Warning: MPFR considers underflow *after* rounding with an unbounded
     exponent range. However, since b and c have same precision p, they are
     multiples of 2^(emin-p), likewise for b-c. Thus if bx < emin, the
     subtraction (with an unbounded exponent range) is exact, so that bx is
     also the exponent after rounding with an unbounded exponent range. */
  if (MPFR_UNLIKELY(bx < __gmpfr_emin))
    {
      /* for RNDN, mpfr_underflow always rounds away, thus for |a|<=2^(emin-2)
         we have to change to RNDZ */
      if (rnd_mode == MPFR_RNDN &&
          (bx < __gmpfr_emin - 1 ||
           (ap[1] == MPFR_LIMB_HIGHBIT && ap[0] == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN(a));
    }

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET (0);
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & MPFR_LIMB_ONE) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE;
      ap[1] += (ap[0] == 0);
      if (MPFR_UNLIKELY(ap[1] == 0))
        {
          ap[1] = MPFR_LIMB_HIGHBIT;
          /* Note: bx+1 cannot exceed __gmpfr_emax, since |a| <= |b|, thus
             bx+1 is at most equal to the original exponent of b. */
          MPFR_ASSERTD(bx + 1 <= __gmpfr_emax);
          MPFR_SET_EXP (a, bx + 1);
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

/* special code for 2*GMP_NUMB_BITS < p < 3*GMP_NUMB_BITS */
static int
mpfr_sub1sp3 (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode,
              mpfr_prec_t p)
{
  mpfr_exp_t bx = MPFR_GET_EXP (b);
  mpfr_exp_t cx = MPFR_GET_EXP (c);
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mpfr_prec_t cnt, INITIALIZED(sh);
  mp_limb_t rb; /* round bit */
  mp_limb_t sb; /* sticky bit */
  mp_limb_t mask, a0, a1, a2;
  mpfr_uexp_t d;

  MPFR_ASSERTD(2 * GMP_NUMB_BITS < p && p < 3 * GMP_NUMB_BITS);

  if (bx == cx) /* subtraction is exact in this case */
    {
      a0 = bp[0] - cp[0];
      a1 = bp[1] - cp[1] - (bp[0] < cp[0]);
      /* a borrow is generated for a when either bp[1] < cp[1],
         or bp[1] = cp[1] and bp[0] < cp[0] */
      a2 = bp[2] - cp[2]
        - (bp[1] < cp[1] || (bp[1] == cp[1] && bp[0] < cp[0]));
      if (a2 == 0 && a1 == 0 && a0 == 0) /* result is zero */
        {
          if (rnd_mode == MPFR_RNDD)
            MPFR_SET_NEG(a);
          else
            MPFR_SET_POS(a);
          MPFR_SET_ZERO(a);
          MPFR_RET (0);
        }
      else if (a2 >= bp[2]) /* borrow: |c| > |b| */
        {
          MPFR_SET_OPPOSITE_SIGN (a, b);
          /* a = b-c mod 2^(3*GMP_NUMB_BITS) */
          a0 = -a0;
          a1 = -a1 - (a0 != 0);
          a2 = -a2 - (a0 != 0 || a1 != 0);
        }
      else /* bp[0] > cp[0] */
        MPFR_SET_SAME_SIGN (a, b);

      if (a2 == 0)
        {
          a2 = a1;
          a1 = a0;
          a0 = 0;
          bx -= GMP_NUMB_BITS;
          if (a2 == 0)
            {
              a2 = a1;
              a1 = 0;
              bx -= GMP_NUMB_BITS;
            }
        }

      /* now a2 != 0 */
      MPFR_ASSERTD(a2 != 0);
      count_leading_zeros (cnt, a2);
      if (cnt)
        {
          ap[2] = (a2 << cnt) | (a1 >> (GMP_NUMB_BITS - cnt));
          ap[1] = (a1 << cnt) | (a0 >> (GMP_NUMB_BITS - cnt));
          ap[0] = a0 << cnt;
          bx -= cnt;
        }
      else
        {
          ap[2] = a2;
          ap[1] = a1;
          ap[0] = a0;
        }
      rb = sb = 0;
      /* Note: sh is not initialized, but will not be used in this case. */
    }
  else
    {
      if (bx < cx)  /* swap b and c */
        {
          mpfr_exp_t tx;
          mp_limb_t *tp;
          tx = bx; bx = cx; cx = tx;
          tp = bp; bp = cp; cp = tp;
          MPFR_SET_OPPOSITE_SIGN (a, b);
        }
      else
        {
          MPFR_SET_SAME_SIGN (a, b);
        }
      MPFR_ASSERTD (bx > cx);
      d = (mpfr_uexp_t) bx - cx;
      sh =  3 * GMP_NUMB_BITS - p;
      mask = MPFR_LIMB_MASK(sh);
      if (d < GMP_NUMB_BITS)
        {
          mp_limb_t cy;
          /* warning: we must have the most significant bit of sb correct
             since it might become the round bit below */
          sb = cp[0] << (GMP_NUMB_BITS - d); /* neglected part of c */
          a0 = bp[0] - ((cp[1] << (GMP_NUMB_BITS - d)) | (cp[0] >> d));
          a1 = bp[1] - ((cp[2] << (GMP_NUMB_BITS - d)) | (cp[1] >> d))
            - (a0 > bp[0]);
          cy = a1 > bp[1] || (a1 == bp[1] && a0 > bp[0]); /* borrow in a1 */
          a2 = bp[2] - (cp[2] >> d) - cy;
          /* if sb is non-zero, subtract 1 from a2, a1, a0 since we want a
             non-negative neglected part */
          if (sb)
            {
              a2 -= (a1 == 0 && a0 == 0);
              a1 -= (a0 == 0);
              a0 --;
              /* a = a2,a1,a0 cannot become zero here, since:
                 a) if d >= 2, then a2 >= 2^(w-1) - (2^(w-2)-1) with
                    w = GMP_NUMB_BITS, thus a2 - 1 >= 2^(w-2),
                 b) if d = 1, then since p < 3*GMP_NUMB_BITS we have sb=0. */
              MPFR_ASSERTD(a2 > 0 || a1 > 0 || a0 > 0);
              sb = -sb; /* 2^GMP_NUMB_BITS - sb */
            }
          if (a2 == 0)
            {
              /* this implies d=1, which in turn implies sb=0 */
              MPFR_ASSERTD(sb == 0);
              a2 = a1;
              a1 = a0;
              a0 = 0; /* should be a0 = sb */
              /* since sb=0 already, no need to set it to 0 */
              bx -= GMP_NUMB_BITS;
              if (a2 == 0)
                {
                  a2 = a1;
                  a1 = 0; /* should be a1 = a0 */
                  bx -= GMP_NUMB_BITS;
                }
            }
          /* now a1 != 0 */
          MPFR_ASSERTD(a2 != 0);
          count_leading_zeros (cnt, a2);
          if (cnt)
            {
              ap[2] = (a2 << cnt) | (a1 >> (GMP_NUMB_BITS - cnt));
              ap[1] = (a1 << cnt) | (a0 >> (GMP_NUMB_BITS - cnt));
              a0 = (a0 << cnt) | (sb >> (GMP_NUMB_BITS - cnt));
              sb <<= cnt;
              bx -= cnt;
            }
          else
            {
              ap[2] = a2;
              ap[1] = a1;
            }
          /* sh > 0 since p < 2*GMP_NUMB_BITS */
          MPFR_ASSERTD(sh > 0);
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else if (d < 2 * GMP_NUMB_BITS)
        {
          mp_limb_t c0shifted;
          /* warning: we must have the most significant bit of sb correct
             since it might become the round bit below */
          sb = (d == GMP_NUMB_BITS) ? cp[0]
            : (cp[1] << (2*GMP_NUMB_BITS - d)) | (cp[0] != 0);
          c0shifted = (d == GMP_NUMB_BITS) ? cp[1]
            : (cp[2] << (2*GMP_NUMB_BITS-d)) | (cp[1] >> (d - GMP_NUMB_BITS));
          a0 = bp[0] - c0shifted;
          /* TODO: add a non-regression test for cp[2] == MPFR_LIMB_MAX,
             d == GMP_NUMB_BITS and a0 > bp[0]. */
          a1 = bp[1] - (cp[2] >> (d - GMP_NUMB_BITS)) - (a0 > bp[0]);
          a2 = bp[2] - (a1 > bp[1] || (a1 == bp[1] && a0 > bp[0]));
          /* if sb is non-zero, subtract 1 from a2, a1, a0 since we want a
             non-negative neglected part */
          if (sb)
            {
              a2 -= (a1 == 0 && a0 == 0);
              a1 -= (a0 == 0);
              a0 --;
              /* a = a2,a1,a0 cannot become zero here, since:
                 a) if d >= 2, then a2 >= 2^(w-1) - (2^(w-2)-1) with
                    w = GMP_NUMB_BITS, thus a2 - 1 >= 2^(w-2),
                 b) if d = 1, then since p < 3*GMP_NUMB_BITS we have sb=0. */
              MPFR_ASSERTD(a2 > 0 || a1 > 0 || a0 > 0);
              sb = -sb; /* 2^GMP_NUMB_BITS - sb */
            }
          /* since bp[2] has its most significant bit set, we can have an
             exponent decrease of at most one */
          if (a2 < MPFR_LIMB_HIGHBIT)
            {
              ap[2] = (a2 << 1) | (a1 >> (GMP_NUMB_BITS - 1));
              ap[1] = (a1 << 1) | (a0 >> (GMP_NUMB_BITS - 1));
              a0 = (a0 << 1) | (sb >> (GMP_NUMB_BITS - 1));
              sb <<= 1;
              bx --;
            }
          else
            {
              ap[2] = a2;
              ap[1] = a1;
            }
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else if (d < 3 * GMP_NUMB_BITS) /* 2*GMP_NUMB_BITS<=d<3*GMP_NUMB_BITS */
        {
          MPFR_ASSERTD (2*GMP_NUMB_BITS <= d && d < 3*GMP_NUMB_BITS);
          /* warning: we must have the most significant bit of sb correct
             since it might become the round bit below */
          if (d == 2 * GMP_NUMB_BITS)
            sb = cp[1] | (cp[0] != 0);
          else
            sb = cp[2] << (3*GMP_NUMB_BITS - d) | (cp[1] != 0) | (cp[0] != 0);
          sb = -sb;
          /* TODO: add a non-regression test for cp[2] == MPFR_LIMB_MAX,
             d == 2*GMP_NUMB_BITS and sb != 0. */
          a0 = bp[0] - (cp[2] >> (d - 2*GMP_NUMB_BITS)) - (sb != 0);
          a1 = bp[1] - (a0 > bp[0] || (a0 == bp[0] && sb != 0));
          a2 = bp[2] - (a1 > bp[1]);
          if (a2 < MPFR_LIMB_HIGHBIT)
            {
              ap[2] = (a2 << 1) | (a1 >> (GMP_NUMB_BITS - 1));
              ap[1] = (a1 << 1) | (a0 >> (GMP_NUMB_BITS - 1));
              a0 = (a0 << 1) | (sb >> (GMP_NUMB_BITS - 1));
              sb <<= 1;
              bx --;
            }
          else
            {
              ap[2] = a2;
              ap[1] = a1;
            }
          rb = a0 & (MPFR_LIMB_ONE << (sh - 1));
          sb |= (a0 & mask) ^ rb;
          ap[0] = a0 & ~mask;
        }
      else /* d >= 3*GMP_NUMB_BITS */
        {
          /* We compute b - ulp(b), and the remainder ulp(b) - c satisfies:
             1/2 ulp(b) < ulp(b) - c < ulp(b), thus rb = sb = 1. */
          mp_limb_t t = MPFR_LIMB_ONE << sh;
          a0 = bp[0] - t;
          a1 = bp[1] - (bp[0] < t);
          a2 = bp[2] - (a1 > bp[1]);
          if (a2 < MPFR_LIMB_HIGHBIT)
            {
              /* necessarily we had b = 1000...000 */
              /* Warning: since we have an exponent decrease, when
                 p = 3*GMP_NUMB_BITS - 1 and d = 3*GMP_NUMB_BITS, the round bit
                 corresponds to the upper bit of -c. In that case rb = 0 and
                 sb = 1, except when c = 1000...000 where rb = 1 and sb = 0. */
              rb = sh > 1 || d > 3 * GMP_NUMB_BITS
                || (cp[2] == MPFR_LIMB_HIGHBIT && cp[1] == MPFR_LIMB_ZERO &&
                    cp[0] == MPFR_LIMB_ZERO);
              /* sb=1 below is incorrect when p = 2*GMP_NUMB_BITS - 1,
                 d = 2*GMP_NUMB_BITS and c = 1000...000, but in
                 that case the even rule wound round up too. */
              ap[0] = ~mask;
              ap[1] = MPFR_LIMB_MAX;
              ap[2] = MPFR_LIMB_MAX;
              bx --;
            }
          else
            {
              ap[0] = a0;
              ap[1] = a1;
              ap[2] = a2;
              rb = 1;
            }
          sb = 1;
        }
    }

  /* now perform rounding */

  /* Warning: MPFR considers underflow *after* rounding with an unbounded
     exponent range. However, since b and c have same precision p, they are
     multiples of 2^(emin-p), likewise for b-c. Thus if bx < emin, the
     subtraction (with an unbounded exponent range) is exact, so that bx is
     also the exponent after rounding with an unbounded exponent range. */
  if (MPFR_UNLIKELY(bx < __gmpfr_emin))
    {
      /* for RNDN, mpfr_underflow always rounds away, thus for |a|<=2^(emin-2)
         we have to change to RNDZ */
      if (rnd_mode == MPFR_RNDN &&
          (bx < __gmpfr_emin - 1 ||
           (ap[2] == MPFR_LIMB_HIGHBIT && ap[1] == 0 && ap[0] == 0)))
        rnd_mode = MPFR_RNDZ;
      return mpfr_underflow (a, rnd_mode, MPFR_SIGN(a));
    }

  MPFR_SET_EXP (a, bx);
  if ((rb == 0 && sb == 0) || rnd_mode == MPFR_RNDF)
    MPFR_RET (0);
  else if (rnd_mode == MPFR_RNDN)
    {
      if (rb == 0 || (sb == 0 && (ap[0] & (MPFR_LIMB_ONE << sh)) == 0))
        goto truncate;
      else
        goto add_one_ulp;
    }
  else if (MPFR_IS_LIKE_RNDZ(rnd_mode, MPFR_IS_NEG(a)))
    {
    truncate:
      MPFR_RET(-MPFR_SIGN(a));
    }
  else /* round away from zero */
    {
    add_one_ulp:
      ap[0] += MPFR_LIMB_ONE << sh;
      ap[1] += (ap[0] == 0);
      ap[2] += (ap[1] == 0 && ap[0] == 0);
      if (MPFR_UNLIKELY(ap[2] == 0))
        {
          ap[2] = MPFR_LIMB_HIGHBIT;
          /* Note: bx+1 cannot exceed __gmpfr_emax, since |a| <= |b|, thus
             bx+1 is at most equal to the original exponent of b. */
          MPFR_ASSERTD(bx + 1 <= __gmpfr_emax);
          MPFR_SET_EXP (a, bx + 1);
        }
      MPFR_RET(MPFR_SIGN(a));
    }
}

#endif /* !defined(MPFR_GENERIC_ABI) */

/* Rounding Sub */

/*
   compute sgn(b)*(|b| - |c|) if |b|>|c| else -sgn(b)*(|c| -|b|)
   Returns 0 iff result is exact,
   a negative value when the result is less than the exact value,
   a positive value otherwise.
*/

/* A0...Ap-1
 *          Cp Cp+1 ....
 *             <- C'p+1 ->
 * Cp = -1 if calculated from c mantissa
 * Cp = 0  if 0 from a or c
 * Cp = 1  if calculated from a.
 * C'p+1 = First bit not null or 0 if there isn't one
 *
 * Can't have Cp=-1 and C'p+1=1*/

/* RND = MPFR_RNDZ:
 *  + if Cp=0 and C'p+1=0,1,  Truncate.
 *  + if Cp=0 and C'p+1=-1,   SubOneUlp
 *  + if Cp=-1,               SubOneUlp
 *  + if Cp=1,                AddOneUlp
 * RND = MPFR_RNDA (Away)
 *  + if Cp=0 and C'p+1=0,-1, Truncate
 *  + if Cp=0 and C'p+1=1,    AddOneUlp
 *  + if Cp=1,                AddOneUlp
 *  + if Cp=-1,               Truncate
 * RND = MPFR_RNDN
 *  + if Cp=0,                Truncate
 *  + if Cp=1 and C'p+1=1,    AddOneUlp
 *  + if Cp=1 and C'p+1=-1,   Truncate
 *  + if Cp=1 and C'p+1=0,    Truncate if Ap-1=0, AddOneUlp else
 *  + if Cp=-1 and C'p+1=-1,  SubOneUlp
 *  + if Cp=-1 and C'p+1=0,   Truncate if Ap-1=0, SubOneUlp else
 *
 * If AddOneUlp:
 *   If carry, then it is 11111111111 + 1 = 10000000000000
 *      ap[n-1]=MPFR_HIGHT_BIT
 * If SubOneUlp:
 *   If we lose one bit, it is 1000000000 - 1 = 0111111111111
 *      Then shift, and put as last bit x which is calculated
 *              according Cp, Cp-1 and rnd_mode.
 * If Truncate,
 *    If it is a power of 2,
 *       we may have to suboneulp in some special cases.
 *
 * To simplify, we don't use Cp = 1.
 *
 */

MPFR_HOT_FUNCTION_ATTR int
mpfr_sub1sp (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  mpfr_exp_t bx, cx;
  mpfr_uexp_t d;
  mpfr_prec_t p, sh, cnt;
  mp_size_t n, k;
  mp_limb_t *ap = MPFR_MANT(a);
  mp_limb_t *bp = MPFR_MANT(b);
  mp_limb_t *cp = MPFR_MANT(c);
  mp_limb_t limb;
  int inexact;
  mp_limb_t rb, sb; /* round and sticky bits. They are interpreted as
                       negative, i.e., if rb <> 0, then we should subtract 1
                       at the round bit position, and of sb <> 0, we should
                       subtract something below the round bit position. */
  mp_limb_t rbb = MPFR_LIMB_MAX, sbb = MPFR_LIMB_MAX; /* rbb is the next bit
    after the round bit, and sbb the corresponding sticky bit.
    gcc claims that they might be used uninitialized. We fill them with invalid
    values, which should produce a failure if so. See README.dev file. */
  int pow2;

  MPFR_TMP_DECL(marker);

  MPFR_ASSERTD(MPFR_PREC(a) == MPFR_PREC(b) && MPFR_PREC(b) == MPFR_PREC(c));
  MPFR_ASSERTD(MPFR_IS_PURE_FP(b));
  MPFR_ASSERTD(MPFR_IS_PURE_FP(c));

  /* Read prec and num of limbs */
  p = MPFR_GET_PREC (b);

#if !defined(MPFR_GENERIC_ABI)
  /* special case for p < GMP_NUMB_BITS */
  if (p < GMP_NUMB_BITS)
    return mpfr_sub1sp1 (a, b, c, rnd_mode, p);

  /* special case for GMP_NUMB_BITS < p < 2*GMP_NUMB_BITS */
  if (GMP_NUMB_BITS < p && p < 2 * GMP_NUMB_BITS)
    return mpfr_sub1sp2 (a, b, c, rnd_mode, p);

  /* special case for p = GMP_NUMB_BITS: we put it *after* mpfr_sub1sp2,
     in order not to slow down mpfr_sub1sp2, which should be more frequent */
  if (p == GMP_NUMB_BITS)
    return mpfr_sub1sp1n (a, b, c, rnd_mode);

  /* special case for 2*GMP_NUMB_BITS < p < 3*GMP_NUMB_BITS */
  if (2 * GMP_NUMB_BITS < p && p < 3 * GMP_NUMB_BITS)
    return mpfr_sub1sp3 (a, b, c, rnd_mode, p);

  if (p == 2 * GMP_NUMB_BITS)
    return mpfr_sub1sp2n (a, b, c, rnd_mode);
#endif

  n = MPFR_PREC2LIMBS (p);
  /* Fast cmp of |b| and |c| */
  bx = MPFR_GET_EXP (b);
  cx = MPFR_GET_EXP (c);

  MPFR_TMP_MARK(marker);

  k = n - 1;

  if (bx == cx)
    {
      /* Check mantissa since exponents are equal */
      while (k >= 0 && MPFR_UNLIKELY(bp[k] == cp[k]))
        k--;
      /* now k = - 1 if b == c, otherwise k is the largest integer < n such
         that bp[k] <> cp[k] */
      if (k < 0)
        /* b == c ! */
        {
          /* Return exact number 0 */
          if (rnd_mode == MPFR_RNDD)
            MPFR_SET_NEG(a);
          else
            MPFR_SET_POS(a);
          MPFR_SET_ZERO(a);
          MPFR_RET(0);
        }
      else if (bp[k] > cp[k])
        goto BGreater;
      else
        {
          MPFR_ASSERTD(bp[k] < cp[k]);
          goto CGreater;
        }
    }
  else if (bx < cx)
    {
      /* Swap b and c and set sign */
      mpfr_srcptr t;
      mpfr_exp_t tx;
      mp_limb_t *tp;

      tx = bx; bx = cx; cx = tx;
    CGreater:
      MPFR_SET_OPPOSITE_SIGN(a,b);
      t  = b;  b  = c;  c  = t;
      tp = bp; bp = cp; cp = tp;
    }
  else
    {
      /* |b| > |c| */
    BGreater:
      MPFR_SET_SAME_SIGN(a,b);
    }

  /* Now |b| > |c| */
  MPFR_ASSERTD(bx >= cx);
  d = (mpfr_uexp_t) bx - cx;
  /* printf ("New with diff=%lu\n", (unsigned long) d); */

  /* FIXME: The goto's below are too complex (long backward) and make
     the code unreadable. */

  if (d == 0)
    {
      /* <-- b -->
         <-- c --> : exact sub */
          mpn_sub_n (ap, bp, cp, n);
          /* Normalize */
        ExactNormalize:
          limb = ap[n-1];
          if (MPFR_LIKELY (limb != 0))
            {
              /* First limb is not zero. */
              count_leading_zeros (cnt, limb);
              /* Warning: cnt can be 0 when we come from the case SubD1Lose
                 with goto ExactNormalize */
              if (MPFR_LIKELY(cnt))
                {
                  mpn_lshift (ap, ap, n, cnt); /* Normalize number */
                  bx -= cnt; /* Update final expo */
                }
              /* Last limb should be OK */
              MPFR_ASSERTD(!(ap[0] & MPFR_LIMB_MASK((unsigned int) (-p)
                                                    % GMP_NUMB_BITS)));
            }
          else
            {
              /* First limb is zero: this can only occur for n >= 2 */
              mp_size_t len;
              /* Find the first limb not equal to zero. It necessarily exists
                 since |b| > |c|. We know that bp[k] > cp[k] and all upper
                 limbs are equal. */
              while (ap[k] == 0)
                k--;
              limb = ap[k];
              /* ap[k] is the non-zero limb of largest index, thus we have
                 to consider the k+1 least significant limbs */
              MPFR_ASSERTD(limb != 0);
              count_leading_zeros(cnt, limb);
              k++;
              len = n - k; /* Number of most significant zero limbs */
              MPFR_ASSERTD(k > 0);
              if (cnt)
                mpn_lshift (ap + len, ap, k, cnt); /* Normalize the High Limb*/
              else
                /* Must use copyd since src and dst may overlap & dst>=src */
                mpn_copyd (ap + len, ap, k);
              MPN_ZERO(ap, len); /* Zeroing the last limbs */
              bx -= cnt + len * GMP_NUMB_BITS; /* update exponent */
              /* ap[len] should have its low bits zero: it is bp[0]-cp[0] */
              MPFR_ASSERTD(!(ap[len] & MPFR_LIMB_MASK((unsigned int) (-p)
                                                      % GMP_NUMB_BITS)));
            }
          /* Check exponent underflow (no overflow can happen) */
          if (MPFR_UNLIKELY(bx < __gmpfr_emin))
            {
              MPFR_TMP_FREE(marker);
              /* since b and c have same sign, exponent and precision, the
                 subtraction is exact */
              /* printf("(D==0 Underflow)\n"); */
              /* for MPFR_RNDN, mpfr_underflow always rounds away from zero,
                 thus for |a| <= 2^(emin-2) we change to RNDZ. */
              if (rnd_mode == MPFR_RNDN &&
                  (bx < __gmpfr_emin - 1 || mpfr_powerof2_raw (a)))
                rnd_mode = MPFR_RNDZ;
              return mpfr_underflow (a, rnd_mode, MPFR_SIGN(a));
            }
          MPFR_SET_EXP (a, bx);
          /* No rounding is necessary since the result is exact */
          MPFR_ASSERTD(ap[n-1] & MPFR_LIMB_HIGHBIT);
          MPFR_TMP_FREE(marker);
          return 0;
    }
  else if (d == 1)
        {
          /* | <-- b -->
             |  <-- c --> */
          mp_limb_t c0, mask;
          MPFR_UNSIGNED_MINUS_MODULO(sh, p);
          /* If we lose at least one bit, compute 2*b-c (Exact)
           * else compute b-c/2 */
          limb = bp[k] - cp[k]/2;
          /* Let W = 2^GMP_NUMB_BITS:
             we have |b|-|c| >= limb*W^k - (2*W^k-1)/2 >= limb*W^k - W^k + 1/2
             thus if limb > W/2, |b|-|c| >= 1/2*W^n.
             Moreover if trunc(|c|) represents the first p-1 bits of |c|,
             minus the last significant bit called c0 below (in fact c0 is that
             bit shifted by sh bits), then we have
             |b|-trunc(|c|) >= 1/2*W^n+1, thus the two mpn_sub_n calls
             below necessarily yield a > 1/2*W^n. */
          if (limb > MPFR_LIMB_HIGHBIT) /* case limb > W/2 */
            {
              mp_limb_t *tp;
              /* The exponent cannot decrease: compute b-c/2 */
              /* Shift c in the allocated temporary block */
            SubD1NoLose:
              c0 = cp[0] & (MPFR_LIMB_ONE << sh);
              mask = ~MPFR_LIMB_MASK(sh);
              tp = MPFR_TMP_LIMBS_ALLOC (n);
              /* FIXME: it might be faster to have one function shifting c by 1
                 to the right and adding with b to a, which would read c once
                 only, and avoid a temporary allocation. */
              mpn_rshift (tp, cp, n, 1);
              tp[0] &= mask; /* Zero last bit of c if set */
              mpn_sub_n (ap, bp, tp, n);
              MPFR_SET_EXP(a, bx); /* No exponent overflow! */
              MPFR_ASSERTD(ap[n-1] & MPFR_LIMB_HIGHBIT);
              if (MPFR_LIKELY(c0 == 0))
                {
                  /* Result is exact: no need of rounding! */
                  MPFR_TMP_FREE(marker);
                  return 0;
                }
              /* c0 is non-zero, thus we have to subtract 1/2*ulp(a),
                 however, we know (see analysis above) that this cannot
                 make the exponent decrease */
              MPFR_ASSERTD( !(ap[0] & ~mask) );    /* Check last bits */
              /* No normalize is needed */

              /* Rounding is necessary since c0 is non-zero */
              /* we have to subtract 1 at the round bit position,
                 and 0 for the lower bits */
              rb = 1; rbb = sbb = 0;
            }
          else if (MPFR_LIKELY(limb < MPFR_LIMB_HIGHBIT))
            {
              mp_limb_t *tp;
              /* |b| - |c| <= (W/2-1)*W^k + W^k-1 = 1/2*W^n - 1 */
              /* The exponent decreases by one. */
            SubD1Lose:
              /* Compute 2*b-c (Exact) */
#if defined(WANT_GMP_INTERNALS) && defined(HAVE___GMPN_RSBLSH1_N)
              /* {ap, n} = 2*{bp, n} - {cp, n} */
              /* mpn_rsblsh1_n -- rp[] = (vp[] << 1) - up[] */
              __gmpn_rsblsh1_n (ap, cp, bp, n);
#else
              tp = MPFR_TMP_LIMBS_ALLOC (n);
              /* Shift b in the allocated temporary block */
              mpn_lshift (tp, bp, n, 1);
              mpn_sub_n (ap, tp, cp, n);
#endif
              bx--;
              MPFR_ASSERTD(k == n-1);
              goto ExactNormalize;
            }
          else /* limb = MPFR_LIMB_HIGHBIT */
            {
              /* Case: limb = 100000000000 */
              /* Check while b[l] == c'[l] (C' is C shifted by 1) */
              /* If b[l]<c'[l] => We lose at least one bit */
              /* If b[l]>c'[l] => We don't lose any bit */
              /* If l==-1 => We don't lose any bit
                 AND the result is 100000000000 0000000000 00000000000 */
              mp_size_t l = n - 1;
              mp_limb_t cl_shifted;
              do
                {
                  /* the first loop will compare b[n-2] and c'[n-2] */
                  cl_shifted = cp[l] << (GMP_NUMB_BITS - 1);
                  if (--l < 0)
                    break;
                  cl_shifted += cp[l] >> 1;
                }
              while (bp[l] == cl_shifted);
              if (MPFR_UNLIKELY(l < 0))
                {
                  if (MPFR_UNLIKELY(cl_shifted))
                    {
                      /* Since cl_shifted is what should be subtracted
                         from ap[-1], if non-zero then necessarily the
                         precision is a multiple of GMP_NUMB_BITS, and we lose
                         one bit, thus the (exact) result is a power of 2
                         minus 1. */
                      memset (ap, -1, n * MPFR_BYTES_PER_MP_LIMB);
                      MPFR_SET_EXP (a, bx - 1);
                      /* No underflow is possible since cx = bx-1 is a valid
                         exponent. */
                    }
                  else
                    {
                      /* cl_shifted=0: result is a power of 2. */
                      MPN_ZERO (ap, n - 1);
                      ap[n-1] = MPFR_LIMB_HIGHBIT;
                      MPFR_SET_EXP (a, bx); /* No exponent overflow! */
                    }
                  /* No Normalize is needed */
                  /* No Rounding is needed */
                  MPFR_TMP_FREE (marker);
                  return 0;
                }
              /* cl_shifted is the shifted value c'[l] */
              else if (bp[l] > cl_shifted)
                goto SubD1NoLose; /* |b|-|c| >= 1/2*W^n */
              else
                {
                  /* we cannot have bp[l] = cl_shifted since the only way we
                     can exit the while loop above is when bp[l] <> cl_shifted
                     or l < 0, and the case l < 0 was already treated above. */
                  MPFR_ASSERTD(bp[l] < cl_shifted);
                  goto SubD1Lose; /* |b|-|c| <= 1/2*W^n-1 and is exact */
                }
            }
        }
  else if (MPFR_UNLIKELY(d >= p)) /* the difference of exponents is larger
                                     than the precision of all operands, thus
                                     the result is either b or b - 1 ulp,
                                     with a possible exact result when
                                     d = p, b = 2^e and c = 1/2 ulp(b) */
    {
      MPFR_UNSIGNED_MINUS_MODULO(sh, p);
      /* We can't set A before since we use cp for rounding... */
      /* Perform rounding: check if a=b or a=b-ulp(b) */
      if (MPFR_UNLIKELY(d == p))
        {
          /* since c is normalized, we need to subtract 1/2 ulp(b) */
          rb = 1;
          /* rbb is the bit of weight 1/4 ulp(b) in c. We assume a limb has
             at least 2 bits. If the precision is 1, we read in the unused
             bits, which should be zero, and this is what we want. */
          rbb = cp[n-1] & (MPFR_LIMB_HIGHBIT >> 1);

          /* We need also sbb */
          sbb = cp[n-1] & MPFR_LIMB_MASK(GMP_NUMB_BITS - 2);
          for (k = n-1; sbb == 0 && k > 0;)
            sbb = cp[--k];
        }
      else
        {
          rb = 0;
          if (d == p + 1)
            {
              rbb = 1;
              sbb = cp[n-1] & MPFR_LIMB_MASK(GMP_NUMB_BITS - 1);
              for (k = n-1; sbb == 0 && k > 0;)
                sbb = cp[--k];
            }
          else
            {
              rbb = 0;
              sbb = 1; /* since C is non-zero */
            }
        }
      /* Copy mantissa B in A */
      MPN_COPY(ap, bp, n);
    }
  else /* case 2 <= d < p */
    {
      mpfr_uexp_t dm;
      mp_size_t m;
      mp_limb_t mask, *tp;

      MPFR_UNSIGNED_MINUS_MODULO(sh, p);
      tp = MPFR_TMP_LIMBS_ALLOC (n);

      /* Shift c in temporary allocated place */
      dm = d % GMP_NUMB_BITS;
      m = d / GMP_NUMB_BITS;
      if (MPFR_UNLIKELY(dm == 0))
        {
          /* dm = 0 and m > 0: Just copy */
          MPFR_ASSERTD(m != 0);
          MPN_COPY(tp, cp + m, n - m);
          MPN_ZERO(tp + n - m, m);
        }
      else if (MPFR_LIKELY(m == 0))
        {
          /* dm >=2 and m == 0: just shift */
          MPFR_ASSERTD(dm >= 2);
          mpn_rshift (tp, cp, n, dm);
        }
      else
        {
          /* dm > 0 and m > 0: shift and zero  */
          mpn_rshift (tp, cp + m, n - m, dm);
          MPN_ZERO (tp + n - m, m);
        }
      /* FIXME: Instead of doing "cp = tp;", keep using tp to avoid
         confusion? Thus in the block below, we don't need
         "mp_limb_t *cp = MPFR_MANT(c);". In short, cp should always
         be MPFR_MANT(c) defined earlier, possibly after the swap. */
      cp = tp;

      /* mpfr_print_mant_binary("Before", MPFR_MANT(c), p); */
      /* mpfr_print_mant_binary("B=    ", MPFR_MANT(b), p); */
      /* mpfr_print_mant_binary("After ", cp, p); */

      /* Compute rb=Cp and sb=C'p+1 */
      {
        /* Compute rb and rbb from C */
        mp_limb_t *cp = MPFR_MANT(c);
        /* The round bit is bit p-d in C, assuming the most significant bit
           of C is bit 0 */
        mpfr_prec_t  x = p - d;
        mp_size_t   kx = n - 1 - (x / GMP_NUMB_BITS);
        mpfr_prec_t sx = GMP_NUMB_BITS - 1 - (x % GMP_NUMB_BITS);
        /* the round bit is in cp[kx], at position sx */
        MPFR_ASSERTD(p >= d);
        rb = cp[kx] & (MPFR_LIMB_ONE << sx);

        /* Now compute rbb: since d >= 2 it always exists in C */
        if (sx == 0) /* rbb is in the next limb */
          {
            kx --;
            sx = GMP_NUMB_BITS - 1;
          }
        else
          sx --; /* rb and rbb are in the same limb */
        rbb = cp[kx] & (MPFR_LIMB_ONE << sx);

        /* Now look at the remaining low bits of C to determine sbb */
        sbb = cp[kx] & MPFR_LIMB_MASK(sx);
        while (sbb == 0 && kx > 0)
          sbb = cp[--kx];
      }
      /* printf("sh=%lu Cp=%d C'p+1=%d\n", sh, rb!=0, sb!=0); */

      /* Clean shifted C' */
      mask = ~MPFR_LIMB_MASK (sh);
      cp[0] &= mask;

      /* Subtract the mantissa c from b in a */
      mpn_sub_n (ap, bp, cp, n);
      /* mpfr_print_mant_binary("Sub=  ", ap, p); */

      /* Normalize: we lose at most one bit */
      if (MPFR_UNLIKELY(MPFR_LIMB_MSB(ap[n-1]) == 0))
        {
          /* High bit is not set and we have to fix it! */
          /* Ap >= 010000xxx001 */
          mpn_lshift (ap, ap, n, 1);
          /* Ap >= 100000xxx010 */
          if (MPFR_UNLIKELY(rb != 0)) /* Check if Cp = -1 */
            /* Since Cp == -1, we have to subtract one more */
            {
              mpn_sub_1 (ap, ap, n, MPFR_LIMB_ONE << sh);
              MPFR_ASSERTD(MPFR_LIMB_MSB(ap[n-1]) != 0);
            }
          /* Ap >= 10000xxx001 */
          /* Final exponent -1 since we have shifted the mantissa */
          bx--;
          /* Update rb and sb */
          rb  = rbb;
          rbb = sbb;
          /* We don't have anymore a valid Cp+1!
             But since Ap >= 100000xxx001, the final sub can't unnormalize!*/
        }
      MPFR_ASSERTD( !(ap[0] & ~mask) );
    }

 rounding:
  /* at this point 'a' contains b - high(c), normalized,
     and we have to subtract rb * 1/2 ulp(a), rbb * 1/4 ulp(a),
     and sbb * 1/8 ulp(a), interpreting rb/rbb/sbb as 1 if non-zero. */

  sb = rbb | sbb;

  if (rb == 0 && sb == 0)
    {
      inexact = 0;
      goto end_of_sub;
    }

  pow2 = mpfr_powerof2_raw (a);
  if (pow2 && rb != 0) /* subtract 1 ulp */
    {
      mpn_sub_1 (ap, ap, n, MPFR_LIMB_ONE << sh);
      ap[n-1] |= MPFR_LIMB_HIGHBIT;
      bx--;
      rb = rbb;
      rbb = sbb;
      sbb = 0;
      /* Note: for p=1 this case can only happen with d=1, but then we will
         have rb=sb=0 at the next round. */
      goto rounding;
    }

  /* now if a is a power of two, necessary rb = 0,
     which means the exact result is always in (pred(a), a),
     and the bounds cannot be attained */

  if (rnd_mode == MPFR_RNDF)
    inexact = 0;
  else if (rnd_mode == MPFR_RNDN)
    {
      if (pow2)
        {
          MPFR_ASSERTD(rb == 0);
          /* since we have at the end of the binade, we have in fact rb = rbb
             and sb = sbb */
          rb = rbb;
          sb = sbb;
        }
      /* Warning: for p=1, the significand is always odd: the "even" rule
         rounds to the value with largest magnitude, thus we have to check
         that case separately */
      if (rb == 0 ||
          (rb != 0 && sb == 0 &&
           ((ap[0] & (MPFR_LIMB_ONE << sh)) == 0 || p == 1)))
        inexact = 1; /* round to a and return 1 */
      else /* round to pred(a) and return -1 */
        {
        subtract:
          mpn_sub_1 (ap, ap, n, MPFR_LIMB_ONE << sh);
          if (pow2) /* deal with cancellation */
            {
              ap[n-1] |= MPFR_LIMB_HIGHBIT;
              bx--;
            }
          inexact = -1;
        }
    }
  else /* directed rounding */
    {
      MPFR_UPDATE_RND_MODE(rnd_mode, MPFR_IS_NEG(a));
      if (rnd_mode == MPFR_RNDZ)
        goto subtract;
      else
        inexact = 1;
    }

 end_of_sub:
  /* Update Exponent */
  /* bx >= emin. Proof:
      If d==0      : Exact case. This is never called.
      if 1 < d < p : bx=MPFR_EXP(b) or MPFR_EXP(b)-1 > MPFR_EXP(c) > emin
      if d == 1    : bx=MPFR_EXP(b). If we could lose any bits, the exact
                     normalization is called.
      if d >=  p   : bx=MPFR_EXP(b) >= MPFR_EXP(c) + p > emin
     After SubOneUlp, we could have one bit less.
      if 1 < d < p : bx >= MPFR_EXP(b)-2 >= MPFR_EXP(c) > emin
      if d == 1    : bx >= MPFR_EXP(b)-1 = MPFR_EXP(c) > emin.
      if d >=  p   : bx >= MPFR_EXP(b)-1 > emin since p>=2.
  */
  MPFR_ASSERTD( bx >= __gmpfr_emin);
  MPFR_SET_EXP (a, bx);

  MPFR_TMP_FREE(marker);
  MPFR_RET (inexact * MPFR_INT_SIGN (a));
}
