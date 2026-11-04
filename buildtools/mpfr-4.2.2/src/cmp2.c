/* mpfr_cmp2 -- exponent shift when subtracting two numbers.

Copyright 1999-2004, 2006-2025 Free Software Foundation, Inc.
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

/* If |b| != |c|, puts the number of canceled bits when one subtracts |c|
   from |b| in *cancel. Returns the sign of the difference (-1, 0, 1).

   Assumes neither of b or c is NaN, +/- infinity, or +/- 0.

   In other terms, if |b| != |c|, mpfr_cmp2 (b, c) stores
   EXP(max(|b|,|c|)) - EXP(|b| - |c|) in *cancel.

   One necessarily has 0 <= cancel <= max(PREC(b),PREC(c)), so that this
   value is representable in a mpfr_prec_t. Note that in the code, the
   maximum intermediate value is cancel + 1, but since MPFR_PREC_MAX is
   not the maximum value of mpfr_prec_t, there is no integer overflow.
*/

int
mpfr_cmp2 (mpfr_srcptr b, mpfr_srcptr c, mpfr_prec_t *cancel)
{
  mp_limb_t *bp, *cp, bb, cc, lastc, dif;
  int high_dif;  /* manipulated like a boolean */
  mp_size_t bn, cn;
  mpfr_exp_t sdiff_exp;
  mpfr_uexp_t diff_exp;
  mpfr_prec_t res = 0;  /* will be the number of canceled bits (*cancel) */
  int sign;

  /* b=c should not happen, since cmp2 is called only from agm (with
     different variables) and from sub1 (if b=c, then sub1sp would be
     called instead). So, no need for a particular optimization here. */

  /* the cases b=0 or c=0 are also treated apart in agm and sub
     (which calls sub1) */
  MPFR_ASSERTD (MPFR_IS_PURE_UBF (b));
  MPFR_ASSERTD (MPFR_IS_PURE_UBF (c));

  sdiff_exp = MPFR_UNLIKELY (MPFR_IS_UBF (b) || MPFR_IS_UBF (c)) ?
    mpfr_ubf_diff_exp (b, c) : MPFR_GET_EXP (b) - MPFR_GET_EXP (c);

  /* The returned result is saturated to [MPFR_EXP_MIN,MPFR_EXP_MAX],
     which is the range of the mpfr_exp_t type. But under the condition
     below, since |MPFR_EXP_MIN| >= MPFR_EXP_MAX, the value of cancel
     will not be affected: by symmetry (as done in the code), assume
     |b| >= |c|; if EXP(b) - EXP(c) >= MPFR_EXP_MAX, then |c| < ulp(b),
     so that the value of cancel is 0, or 1 if |b| is a power of 2,
     whatever the exact value of EXP(b) - EXP(c). */
  MPFR_STAT_STATIC_ASSERT (MPFR_EXP_MAX > MPFR_PREC_MAX);

  if (sdiff_exp >= 0)
    {
      sign = 1;  /* assumes |b| > |c|; will be changed if not. */
      diff_exp = sdiff_exp;

      bp = MPFR_MANT(b);
      cp = MPFR_MANT(c);

      /* index of the most significant limb of b and c */
      bn = (MPFR_PREC(b) - 1) / GMP_NUMB_BITS;
      cn = (MPFR_PREC(c) - 1) / GMP_NUMB_BITS;

      /* If diff_exp != 0, i.e. diff_exp > 0, then |b| > |c|. Otherwise... */
      if (diff_exp == 0)
        {
          /* Skip the identical most significant limbs, adding GMP_NUMB_BITS
             to the number of canceled bits at each iteration. */
          while (bn >= 0 && cn >= 0 && bp[bn] == cp[cn])
            {
              bn--;
              cn--;
              res += GMP_NUMB_BITS;
            }

          if (MPFR_UNLIKELY (bn < 0))
            {
              if (MPFR_LIKELY (cn < 0)) /* |b| = |c| */
                return 0;

              /* b has been read entirely, but not c. Thus |b| <= |c|.
                 Swap (bp,bn) and (cp,cn), and take the opposite sign
                 for the symmetric case below (simulating a swap).
                 Note: cp will not be used, thus is not assigned; and
                 "cn = -1;" is necessary to enter the following "if"
                 (probably less confusing than a "goto"). */
              bp = cp;
              bn = cn;
              cn = -1;
              sign = -1;
            }

          if (MPFR_UNLIKELY (cn < 0))
            /* c discards exactly the upper part of b */
            {
              int z;

              MPFR_ASSERTD (bn >= 0);

              /* Skip null limbs of b (= non-represented null limbs of c),
                 adding GMP_NUMB_BITS to the number of canceled bits at
                 each iteration. */
              while (bp[bn] == 0)
                {
                  if (--bn < 0) /* |b| = |c| */
                    return 0;
                  res += GMP_NUMB_BITS;
                }

              count_leading_zeros (z, bp[bn]); /* bp[bn] != 0 */
              *cancel = res + z;
              return sign;
            }

          MPFR_ASSERTD (bn >= 0);
          MPFR_ASSERTD (cn >= 0);
          MPFR_ASSERTD (bp[bn] != cp[cn]);

          /* |b| != |c|. If |b| < |c|: swap (bp,bn) and (cp,cn),
             and take the opposite sign. */
          if (bp[bn] < cp[cn])
            {
              mp_limb_t *tp;
              mp_size_t tn;

              tp = bp; bp = cp; cp = tp;
              tn = bn; bn = cn; cn = tn;
              sign = -1;
            }
        }
    } /* MPFR_EXP(b) >= MPFR_EXP(c) */
  else /* MPFR_EXP(b) < MPFR_EXP(c) */
    {
      /* We necessarily have |b| < |c|. Simulate a swap by reading the
         parameters so that |(bp,bn)| > |(cp,cn)|. */

      sign = -1;
      diff_exp = - (mpfr_uexp_t) sdiff_exp;

      bp = MPFR_MANT(c);
      cp = MPFR_MANT(b);

      bn = (MPFR_PREC(c) - 1) / GMP_NUMB_BITS;
      cn = (MPFR_PREC(b) - 1) / GMP_NUMB_BITS;
    }

  /* Now we have removed the identical upper limbs of b and c
     (when diff_exp = 0), and after the possible swap, we have |b| > |c|,
     where b is represented by (bp,bn) and c is represented by (cp,cn).
     The value diff_exp = EXP(b) - EXP(c) can be regarded as the number
     of leading zeros of c, when aligned with b. */

  /* When a limb of c is read from memory, the part that is not taken
     into account for the operation with a limb bp[bn] of b will be put
     in lastc, shifted to the leftmost part (for alignment with b):
       [-------- bp[bn] --------][------- bp[bn-1] -------]
       [-- old_lastc --][-------- cp[cn] --------]
                                 [-- new_lastc --]
     Note: if diff_exp == 0, then lastc will always remain 0. */
  lastc = 0;

  /* Compute the next limb difference, which cannot be 0 (dif >= 1). */

  if (MPFR_LIKELY (diff_exp < GMP_NUMB_BITS))
    {
      cc = cp[cn] >> diff_exp;
      /* warning: a shift by GMP_NUMB_BITS is not allowed by ISO C */
      if (diff_exp != 0)
        lastc = cp[cn] << (GMP_NUMB_BITS - diff_exp);
      cn--;
    }
  else
    {
      cc = 0;
      diff_exp -= GMP_NUMB_BITS;  /* remove GMP_NUMB_BITS leading zeros */
    }

  MPFR_ASSERTD (bp[bn] >= cc);  /* no borrow out in subtraction below */
  dif = bp[bn--] - cc;
  MPFR_ASSERTD (dif >= 1);
  high_dif = 0;

  /* The current difference, here and later, is expressed under the form
     [high_dif][dif], where high_dif is 0 or 1, and dif is a limb.
     Here, since we have computed a difference of limbs (with b >= c),
     high_dif = 0. */

  /* One needs to accumulate canceled bits for the remaining case where
     b and c are close to each other due to a long borrow propagation:
       b = [common part]1000...000[low(b)]
       c = [common part]0111...111[low(c)]
     After eliminating the common part above, we have computed a difference
     of the most significant parts, which has been stored in [high_dif][dif]
     with high_dif = 0. We will loop as long as the currently computed
     difference [high_dif][dif] = 1 (it is >= 1 by construction). The
     computation of the difference will be:
        1bbb...bbb
       - ccc...ccc
     where the leading 1 before bbb...bbb corresponds to [high_dif][dif]
     at the beginning of the loop. We will exit the loop also when c has
     entirely been taken into account as cancellation is no longer possible
     in this case (it is no longer possible to cancel the leading 1).
     Note: We can enter the loop only with diff_exp = 0 (with a non-empty
     common part, partly or entirely removed) or with diff_exp = 1 (with
     an empty common part). Indeed, if diff_exp > 1, then no limbs have
     been skipped, so that bp[bn] had its MSB equal to 1 and the most two
     significant bits of cc are 0, which implies that dif > 1. */

  while (MPFR_UNLIKELY ((cn >= 0 || lastc != 0)
                        && high_dif == 0 && dif == 1))
    {
      /* Since we consider the next limb, we assume a cancellation of
         GMP_NUMB_BITS (the new exponent of the difference now being the
         one of the MSB of the next limb). But if the leading 1 remains
         1 in the difference (i.e. high_dif = 1 at the end of the loop),
         then we will need to decrease res. */
      res += GMP_NUMB_BITS;
      MPFR_ASSERTD (diff_exp <= 1);  /* see comment before the loop */
      bb = bn >= 0 ? bp[bn--] : 0;  /* next limb of b or non-represented 0 */
      if (MPFR_UNLIKELY (cn < 0))
        {
          cc = lastc;
          lastc = 0;
        }
      else if (diff_exp == 0)
        {
          cc = cp[cn--];
        }
      else
        {
          MPFR_ASSERTD (diff_exp == 1);
          MPFR_ASSERTD (lastc == 0 || lastc == MPFR_LIMB_HIGHBIT);
          cc = lastc + (cp[cn] >> 1);
          lastc = cp[cn--] << (GMP_NUMB_BITS - 1);
        }
      dif = bb - cc;
      high_dif = bb >= cc;
    }

  /* Now, c has entirely been taken into account or [high_dif][dif] > 1.
     In any case, [high_dif][dif] >= 1 by construction.
     First, we determine the currently number of canceled bits,
     corresponding to the exponent of the current difference.
     The trailing bits of c, if any, can still decrease the exponent of
     the difference when [high_dif][dif] is a power of two, but since
     [high_dif][dif] > 1 in this case, by not more than 1. */

  if (high_dif != 0) /* high_dif == 1 */
    {
      res--;  /* see comment at the beginning of the above loop */
      MPFR_ASSERTD (res >= 0);
      /* Terminate if [high_dif][dif] is not a power of two. */
      if (MPFR_LIKELY (dif != 0))
        goto end;
    }
  else /* high_dif == 0 */
    {
      int z;

      MPFR_ASSERTD (dif >= 1);  /* [high_dif][dif] >= 1 */
      count_leading_zeros (z, dif);
      res += z;
      /* Terminate if [high_dif][dif] is not a power of two. */
      if (MPFR_LIKELY (NOT_POW2 (dif)))
        goto end;
    }

  /* Now, the result will be res + (low(b) < low(c)). */

  /* If c has entirely been taken into account, it can no longer modify
     the current result. */
  if (cn < 0 && lastc == 0)
    goto end;

  for (; bn >= 0 ; bn--)
    {
      if (diff_exp >= GMP_NUMB_BITS)
        {
          diff_exp -= GMP_NUMB_BITS;
          MPFR_ASSERTD (cc == 0);
        }
      else if (MPFR_UNLIKELY (cn < 0))
        {
          cc = lastc;
          lastc = 0;
        }
      else if (diff_exp == 0)
        {
          cc = cp[cn--];
        }
      else
        {
          MPFR_ASSERTD (diff_exp >= 1 && diff_exp < GMP_NUMB_BITS);
          cc = lastc + (cp[cn] >> diff_exp);
          lastc = cp[cn--] << (GMP_NUMB_BITS - diff_exp);
        }

      if (bp[bn] != cc)
        {
          res += bp[bn] < cc;
          goto end;
        }
    }

  /* b has entirely been read. Determine whether the trailing part of c
     is non-zero. */

  if (lastc != 0)
    res++;
  else
    {
      while (cn >= 0 && cp[cn] == 0)
        cn--;
      if (cn >= 0)
        res++;
    }

 end:
  *cancel = res;
  return sign;
}
