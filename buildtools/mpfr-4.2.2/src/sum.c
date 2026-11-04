/* Sum -- efficiently sum a list of floating-point numbers

Copyright 2014-2025 Free Software Foundation, Inc.
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

/* Note: In the prototypes, one uses
 *
 *   const mpfr_ptr *x      i.e.:  __mpfr_struct *const *x
 *
 * instead of
 *
 *   const mpfr_srcptr *x   i.e.:  const __mpfr_struct *const *x
 *
 * because here one has a double indirection and the type matching rules
 * from the C standard in such a case are stricter and they would yield
 * annoying errors for the user in practice. See:
 *
 *   Why can't I pass a char ** to a function which expects a const char **?
 *
 * in the comp.lang.c FAQ:
 *
 *   https://c-faq.com/ansi/constmismatch.html
 */

/* See the doc/sum.txt file for the algorithm and a part of its proof
(this will later go into algorithms.tex).

TODO [VL, after a discussion with James Demmel]: Compared to
  James Demmel and Yozo Hida, Fast and accurate floating-point summation
  with application to computational geometry, Numerical Algorithms,
  volume 37, number 1-4, pages 101--112, 2004.
sorting is not necessary here. It is not done because in the most common
cases (where big cancellations are rare), it would take time and be
useless. However, the lack of sorting increases the worst case complexity.
For instance, consider many inputs that cancel one another (two by two).
One would need n/2 iterations, where each iteration reads the exponent
of each input, therefore n*n/2 read operations. Using a worst-case sort
in O(n log n) could give a O(n log n) worst-case complexity. As we don't
want to slow down the most common cases, this could be done at the 3rd
iteration. But are there practical applications which would be used as
tests?

Note: see the following paper and its references:
  http://www.acsel-lab.com/arithmetic/arith21/papers/p54.pdf
  (J. Demmel and H. D. Nguyen, Fast Reproducible Floating-Point Summation)
VL: This is very different:
          In MPFR             In the paper & references
    arbitrary precision            fixed precision
     correct rounding        just reproducible rounding
    integer operations        floating-point operations
        sequential             parallel (& sequential)
*/

#ifdef MPFR_COV_CHECK
int __gmpfr_cov_sum_tmd[MPFR_RND_MAX][2][2][3][2][2] = { 0 };
#endif

/* Update minexp (V) after detecting a potential integer overflow in
   extreme cases (only a 32-bit ABI may be concerned in practice).
   Instead of an assertion failure below, we could
   1. check that the ulp of each regular input has an exponent >= MPFR_EXP_MIN
      (with an assertion failure if this is not the case);
   2. set minexp to MPFR_EXP_MIN and shift the accumulator accordingly
      (the sum will then be exact).
   However, such cases, which involve huge precisions, will probably
   never occur in practice (at least with a 64-bit ABI) and are not
   easily testable due to these huge precisions. Moreover, switching
   to a 64-bit ABI would be a better solution for such computations.
   So, let's leave this unimplemented. */
#define SAFE_SUB(V,E,SH)                        \
  do                                            \
    {                                           \
      mpfr_prec_t sh = (SH);                    \
      MPFR_ASSERTN ((E) >= MPFR_EXP_MIN + sh);  \
      V = (E) - sh;                             \
    }                                           \
  while (0)

/* Function sum_raw
 * ================
 *
 * Accumulate a new [minexp,maxexp[ block into (wp,ws). If e and err denote
 * the exponents of the computed result and of the error bound respectively,
 * while e - err is less than some given bound (due to cancellation), shift
 * the accumulator and reiterate.
 *
 * Inputs:
 *   wp: pointer to the accumulator (least significant limb first).
 *   ws: size of the accumulator (in limbs).
 *   wq: precision of the accumulator (ws * GMP_NUMB_BITS).
 *   x: array of the input numbers.
 *   n: size of this array (number of inputs, regular or not).
 *   minexp: exponent of the least significant bit of the first block.
 *   maxexp: exponent of the first block (exponent of its MSB + 1).
 *   tp: pointer to a temporary area (pre-allocated).
 *   ts: size of this temporary area.
 *   logn: ceil(log2(rn)), where rn is the number of regular inputs.
 *   prec: lower bound for e - err (as described above).
 *   ep: pointer to mpfr_exp_t (see below), or a null pointer.
 *   minexpp: pointer to mpfr_exp_t (see below), or a null pointer.
 *   maxexpp: pointer to mpfr_exp_t (see below), or a null pointer.
 *
 * Preconditions:
 *   prec >= 1
 *   wq >= logn + prec + 2
 *
 * This function returns 0 if the accumulator is 0 (which implies that
 * the exact sum for this sum_raw invocation is 0), otherwise the number
 * of cancelled bits (>= 1), defined as the number of identical bits on
 * the most significant part of the accumulator. In the latter case, it
 * also returns the following data in variables passed by reference, if
 * the pointers are not NULL:
 * - in ep: the exponent e of the computed result;
 * - in minexpp: the last value of minexp;
 * - in maxexpp: the new value of maxexp (for the next iteration after
 *   the first invocation of sum_raw in the main code).
 *
 * Notes:
 * - minexp is also the exponent of the least significant bit of the
 *   accumulator;
 * - the temporary area must be large enough to hold a shifted input
 *   block, and the value of ts is used only when the full assertions
 *   are checked (i.e. with the --enable-assert configure option), to
 *   check that a buffer overflow doesn't occur;
 * - contrary to the returned value of minexp (the value in the last
 *   iteration), the returned value of maxexp is the one for the next
 *   iteration (= maxexp2 of the last iteration).
 */
static mpfr_prec_t
sum_raw (mp_limb_t *wp, mp_size_t ws, mpfr_prec_t wq, const mpfr_ptr *x,
         unsigned long n, mpfr_exp_t minexp, mpfr_exp_t maxexp,
         mp_limb_t *tp, mp_size_t ts, int logn, mpfr_prec_t prec,
         mpfr_exp_t *ep, mpfr_exp_t *minexpp, mpfr_exp_t *maxexpp)
{
  MPFR_LOG_FUNC
    (("ws=%Pd ts=%Pd prec=%Pd", (mpfr_prec_t) ws, (mpfr_prec_t) ts, prec),
     ("", 0));

  /* The C code below requires prec >= 0 due to the use of unsigned
     integer arithmetic on it. Actually the computation makes sense
     only with prec >= 1 (otherwise one can't even know the sign of
     the result), hence the following assertion. */
  MPFR_ASSERTD (prec >= 1);

  /* Consistency check. */
  MPFR_ASSERTD (wq == (mpfr_prec_t) ws * GMP_NUMB_BITS);

  /* The following precondition together with prec >= 1 will imply:
     minexp - shiftq < maxexp2, as required by the algorithm. */
  MPFR_ASSERTD (wq >= logn + prec + 2);

  while (1)
    {
      mpfr_exp_t maxexp2 = MPFR_EXP_MIN;
      unsigned long i;

      MPFR_LOG_MSG (("sum_raw loop: "
                     "maxexp=%" MPFR_EXP_FSPEC "d "
                     "minexp=%" MPFR_EXP_FSPEC "d\n",
                     (mpfr_eexp_t) maxexp, (mpfr_eexp_t) minexp));

      MPFR_ASSERTD (maxexp > minexp);

      for (i = 0; i < n; i++)
        if (! MPFR_IS_SINGULAR (x[i]))  /* Step 1 (see sum_raw in sum.txt) */
          {
            mp_limb_t *dp, *vp;
            mp_size_t ds, vs, vds;
            mpfr_exp_t xe, vd;
            mpfr_prec_t xq;
            int tr;

            xe = MPFR_GET_EXP (x[i]);
            xq = MPFR_GET_PREC (x[i]);

            vp = MPFR_MANT (x[i]);
            vs = MPFR_PREC2LIMBS (xq);
            vd = xe - vs * GMP_NUMB_BITS - minexp;
            /* vd is the exponent of the least significant represented bit of
               x[i] (including the trailing bits, whose value is 0) minus the
               exponent of the least significant bit of the accumulator. To
               make the code simpler, we won't try to filter out the trailing
               bits of x[i]. */

            /* Steps 2, 3, 4 (see sum_raw in sum.txt) */

            if (vd < 0)
              {
                /* This covers the following cases:
                 *     [-+- accumulator ---]
                 *   [---|----- x[i] ------|--]
                 *       |   [----- x[i] --|--]
                 *       |                 |[----- x[i] -----]
                 *       |                 |    [----- x[i] -----]
                 *     maxexp           minexp
                 */

                /* Step 2 for subcase vd < 0 */

                if (xe <= minexp)
                  {
                    /* x[i] is entirely after the LSB of the accumulator,
                       so that it will be ignored at this iteration. */
                    if (xe > maxexp2)
                      {
                        maxexp2 = xe;
                        /* And since the exponent of x[i] is valid... */
                        MPFR_ASSERTD (maxexp2 >= MPFR_EMIN_MIN);
                      }
                    continue;
                  }

                /* Step 3 for subcase vd < 0 */

                /* If some significant bits of x[i] are after the LSB of the
                   accumulator, then maxexp2 will necessarily be minexp. */
                if (MPFR_LIKELY (xe - xq < minexp))
                  maxexp2 = minexp;

                /* Step 4 for subcase vd < 0 */

                /* We need to ignore the least |vd| significant bits of x[i].
                   First, let's ignore the least vds = |vd| / GMP_NUMB_BITS
                   limbs. */
                vd = - vd;
                vds = vd / GMP_NUMB_BITS;
                vs -= vds;
                MPFR_ASSERTD (vs > 0);  /* see xe <= minexp test above */
                vp += vds;
                vd -= vds * GMP_NUMB_BITS;
                MPFR_ASSERTD (vd >= 0 && vd < GMP_NUMB_BITS);

                if (xe > maxexp)
                  {
                    vs -= (xe - maxexp) / GMP_NUMB_BITS;
                    MPFR_ASSERTD (vs > 0);
                    tr = (xe - maxexp) % GMP_NUMB_BITS;
                  }
                else
                  tr = 0;

                if (vd != 0)
                  {
                    MPFR_ASSERTD (vs <= ts);
                    mpn_rshift (tp, vp, vs, vd);
                    vp = tp;
                    tr += vd;
                    if (tr >= GMP_NUMB_BITS)
                      {
                        vs--;
                        tr -= GMP_NUMB_BITS;
                      }
                    MPFR_ASSERTD (vs >= 1);
                    MPFR_ASSERTD (tr >= 0 && tr < GMP_NUMB_BITS);
                    if (tr != 0)
                      {
                        tp[vs-1] &= MPFR_LIMB_MASK (GMP_NUMB_BITS - tr);
                        tr = 0;
                      }
                    /* Truncation has now been taken into account. */
                    MPFR_ASSERTD (tr == 0);
                  }

                dp = wp;
                ds = ws;
              }
            else  /* vd >= 0 */
              {
                /* This covers the following cases:
                 *               [-+- accumulator ---]
                 *   [- x[i] -]    |                 |
                 *             [---|-- x[i] ------]  |
                 *          [------|-- x[i] ---------]
                 *                 |   [- x[i] -]    |
                 *               maxexp           minexp
                 */

                /* Steps 2 and 3 for subcase vd >= 0 */

                MPFR_ASSERTD (xe - xq >= minexp);  /* see definition of vd */

                /* Step 4 for subcase vd >= 0 */

                /* We need to ignore the least vd significant bits
                   of the accumulator. First, let's ignore the least
                   vds = vd / GMP_NUMB_BITS limbs. -> (dp,ds) */
                vds = vd / GMP_NUMB_BITS;
                ds = ws - vds;
                if (ds <= 0)
                  continue;
                dp = wp + vds;
                vd -= vds * GMP_NUMB_BITS;
                MPFR_ASSERTD (vd >= 0 && vd < GMP_NUMB_BITS);

                /* The low part of x[i] (to be determined) will have to be
                   shifted vd bits to the left if vd != 0. */

                if (xe > maxexp)
                  {
                    vs -= (xe - maxexp) / GMP_NUMB_BITS;
                    if (vs <= 0)
                      continue;
                    tr = (xe - maxexp) % GMP_NUMB_BITS;
                  }
                else
                  tr = 0;

                MPFR_ASSERTD (tr >= 0 && tr < GMP_NUMB_BITS && vs > 0);

                /* We need to consider the least significant vs limbs of x[i]
                   except the most significant tr bits. */

                if (vd != 0)
                  {
                    mp_limb_t carry;

                    MPFR_ASSERTD (vs <= ts);
                    carry = mpn_lshift (tp, vp, vs, vd);
                    tr -= vd;
                    if (tr < 0)
                      {
                        tr += GMP_NUMB_BITS;
                        MPFR_ASSERTD (vs + 1 <= ts);
                        tp[vs++] = carry;
                      }
                    MPFR_ASSERTD (tr >= 0 && tr < GMP_NUMB_BITS);
                    vp = tp;
                  }
              }  /* vd >= 0 */

            MPFR_ASSERTD (vs > 0 && vs <= ds);

            /* We can't truncate the most significant limb of the input
               (in case it hasn't been shifted to the temporary area).
               So, let's ignore it now. It will be taken into account
               via carry propagation after the addition. */
            if (tr != 0)
              vs--;

            /* Step 5 (see sum_raw in sum.txt) */

            if (MPFR_IS_POS (x[i]))
              {
                mp_limb_t carry;

                carry = vs > 0 ? mpn_add_n (dp, dp, vp, vs) : 0;
                MPFR_ASSERTD (carry <= 1);
                if (tr != 0)
                  carry += vp[vs] & MPFR_LIMB_MASK (GMP_NUMB_BITS - tr);
                if (ds > vs)
                  mpn_add_1 (dp + vs, dp + vs, ds - vs, carry);
              }
            else
              {
                mp_limb_t borrow;

                borrow = vs > 0 ? mpn_sub_n (dp, dp, vp, vs) : 0;
                MPFR_ASSERTD (borrow <= 1);
                if (tr != 0)
                  borrow += vp[vs] & MPFR_LIMB_MASK (GMP_NUMB_BITS - tr);
                if (ds > vs)
                  mpn_sub_1 (dp + vs, dp + vs, ds - vs, borrow);
              }
          }

      {
        mpfr_prec_t cancel;  /* number of cancelled bits */
        mp_size_t wi;        /* index in the accumulator */
        mp_limb_t a, b;
        int cnt;

        cancel = 0;
        wi = ws - 1;
        MPFR_ASSERTD (wi >= 0);
        a = wp[wi] >> (GMP_NUMB_BITS - 1) ? MPFR_LIMB_MAX : MPFR_LIMB_ZERO;

        while (wi >= 0)
          if ((b = wp[wi]) == a)
            {
              cancel += GMP_NUMB_BITS;
              wi--;
            }
          else
            {
              b ^= a;
              MPFR_ASSERTD (b != 0);
              count_leading_zeros (cnt, b);
              cancel += cnt;
              break;
            }

        if (wi >= 0 || a != MPFR_LIMB_ZERO)  /* accumulator != 0 */
          {
            mpfr_exp_t e;        /* exponent of the computed result */
            mpfr_exp_t err;      /* exponent of the error bound */

            MPFR_LOG_MSG (("accumulator %s 0, cancel=%Pd\n",
                           a != MPFR_LIMB_ZERO ? "<" : ">", cancel));

            MPFR_ASSERTD (cancel > 0);
            e = minexp + wq - cancel;
            MPFR_ASSERTD (e >= minexp);
            err = maxexp2 + logn;  /* OK even if maxexp2 == MPFR_EXP_MIN */

            /* The absolute value of the truncated sum is in the binade
               [2^(e-1),2^e] (closed on both ends due to two's complement).
               The error is strictly less than 2^err (and is 0 if
               maxexp2 == MPFR_EXP_MIN). */

            /* This basically tests whether err <= e - prec without
               potential integer overflow (since prec >= 0)...
               Note that the maxexp2 == MPFR_EXP_MIN test is there just for
               the potential corner case e - prec < MPFR_EXP_MIN + logn.
               Such corner cases, involving specific huge-precision numbers,
               are probably not supported in many places in MPFR, but this
               test doesn't hurt... */
            if (maxexp2 == MPFR_EXP_MIN ||
                (err <= e && SAFE_DIFF (mpfr_uexp_t, e, err) >= prec))
              {
                MPFR_LOG_MSG (("(err=%" MPFR_EXP_FSPEC "d) <= (e=%"
                               MPFR_EXP_FSPEC "d) - (prec=%Pd)\n",
                               (mpfr_eexp_t) err, (mpfr_eexp_t) e, prec));
                /* To avoid tests or copies, we consider the only two cases
                   that will occur in sum_aux. */
                MPFR_ASSERTD ((ep != NULL &&
                               minexpp != NULL &&
                               maxexpp != NULL) ||
                              (ep == NULL &&
                               minexpp == NULL &&
                               maxexpp == NULL));
                if (ep != NULL)
                  {
                    *ep = e;
                    *minexpp = minexp;
                    *maxexpp = maxexp2;
                  }
                MPFR_LOG_MSG (("return with minexp=%" MPFR_EXP_FSPEC
                               "d maxexp2=%" MPFR_EXP_FSPEC "d%s\n",
                               (mpfr_eexp_t) minexp, (mpfr_eexp_t) maxexp2,
                               maxexp2 == MPFR_EXP_MIN ?
                               " (MPFR_EXP_MIN)" : ""));
                return cancel;
              }
            else
              {
                mpfr_exp_t diffexp;
                mpfr_prec_t shiftq;
                mpfr_size_t shifts;
                int shiftc;

                MPFR_LOG_MSG (("e=%" MPFR_EXP_FSPEC "d err=%" MPFR_EXP_FSPEC
                               "d maxexp2=%" MPFR_EXP_FSPEC "d%s\n",
                               (mpfr_eexp_t) e, (mpfr_eexp_t) err,
                               (mpfr_eexp_t) maxexp2,
                               maxexp2 == MPFR_EXP_MIN ?
                               " (MPFR_EXP_MIN)" : ""));

                diffexp = err - e;
                if (diffexp < 0)
                  diffexp = 0;
                /* diffexp = max(0, err - e) */

                MPFR_LOG_MSG (("diffexp=%" MPFR_EXP_FSPEC "d\n",
                                (mpfr_eexp_t) diffexp));

                MPFR_ASSERTD (diffexp < cancel - 2);
                shiftq = cancel - 2 - (mpfr_prec_t) diffexp;
                /* equivalent to: minexp + wq - 2 - max(e,err) */
                MPFR_ASSERTD (shiftq > 0);
                shifts = shiftq / GMP_NUMB_BITS;
                shiftc = shiftq % GMP_NUMB_BITS;
                MPFR_LOG_MSG (("shiftq = %Pd = %Pd * GMP_NUMB_BITS + %d\n",
                               shiftq, (mpfr_prec_t) shifts, shiftc));
                if (MPFR_LIKELY (shiftc != 0))
                  mpn_lshift (wp + shifts, wp, ws - shifts, shiftc);
                else
                  mpn_copyd (wp + shifts, wp, ws - shifts);
                MPN_ZERO (wp, shifts);
                /* Compute minexp = minexp - shiftq safely. */
                SAFE_SUB (minexp, minexp, shiftq);
                MPFR_ASSERTD (minexp < maxexp2);
              }
          }
        else if (maxexp2 == MPFR_EXP_MIN)
          {
            MPFR_LOG_MSG (("accumulator = 0, maxexp2 = MPFR_EXP_MIN\n", 0));
            return 0;
          }
        else
          {
            MPFR_LOG_MSG (("accumulator = 0, reiterate\n", 0));
            /* Compute minexp = maxexp2 - (wq - (logn + 1)) safely. */
            SAFE_SUB (minexp, maxexp2, wq - (logn + 1));
            /* Note: the logn + 1 corresponds to cq in the main code. */
          }
      }

      maxexp = maxexp2;
    }
}

/**********************************************************************/

/* Generic case: all the inputs are finite numbers,
   with at least 3 regular numbers. */
static int
sum_aux (mpfr_ptr sum, const mpfr_ptr *x, unsigned long n, mpfr_rnd_t rnd,
         mpfr_exp_t maxexp, unsigned long rn)
{
  mp_limb_t *sump;
  mp_limb_t *tp;  /* pointer to a temporary area */
  mp_limb_t *wp;  /* pointer to the accumulator */
  mp_size_t ts;   /* size of the temporary area, in limbs */
  mp_size_t ws;   /* size of the accumulator, in limbs */
  mp_size_t zs;   /* size of the TMD accumulator, in limbs */
  mpfr_prec_t wq; /* size of the accumulator, in bits */
  int logn;       /* ceil(log2(rn)) */
  int cq;
  mpfr_prec_t sq;
  int inex;
  MPFR_TMP_DECL (marker);

  MPFR_LOG_FUNC
    (("n=%lu rnd=%d maxexp=%" MPFR_EXP_FSPEC "d rn=%lu",
      n, rnd, (mpfr_eexp_t) maxexp, rn),
     ("sum[%Pd]=%.*Rg", mpfr_get_prec (sum), mpfr_log_prec, sum));

  MPFR_ASSERTD (rn >= 3 && rn <= n);

  /* In practice, no integer overflow on the exponent. */
  MPFR_STAT_STATIC_ASSERT (MPFR_EXP_MAX - MPFR_EMAX_MAX >=
                           sizeof (unsigned long) * CHAR_BIT);

  /* Set up some variables and the accumulator. */

  sump = MPFR_MANT (sum);

  /* rn is the number of regular inputs (the singular ones will be
     ignored). Compute logn = ceil(log2(rn)). */
  logn = MPFR_INT_CEIL_LOG2 (rn);
  MPFR_ASSERTD (logn >= 2);

  MPFR_LOG_MSG (("logn=%d maxexp=%" MPFR_EXP_FSPEC "d\n",
                 logn, (mpfr_eexp_t) maxexp));

  sq = MPFR_GET_PREC (sum);
  cq = logn + 1;

  /* First determine the size of the accumulator.
   * cq + sq + logn + 2 >= logn + sq + 5, which will be used later.
   * The assertion wq - cq - sq >= 4 is another way to check that.
   */
  ws = MPFR_PREC2LIMBS (cq + sq + logn + 2);
  wq = (mpfr_prec_t) ws * GMP_NUMB_BITS;
  MPFR_ASSERTD (wq - cq - sq >= 4);

  /* TODO: timings, comparing with a larger zs. */
  zs = MPFR_PREC2LIMBS (wq - sq);

  MPFR_LOG_MSG (("cq=%d sq=%Pd logn=%d wq=%Pd\n", cq, sq, logn, wq));

  /* An input block will have up to wq - cq bits, and its shifted value
     (to be correctly aligned) may take GMP_NUMB_BITS - 1 additional bits. */
  ts = MPFR_PREC2LIMBS (wq - cq + GMP_NUMB_BITS - 1);

  MPFR_TMP_MARK (marker);

  /* Note: If the TMD does not occur, which should be the case for most
     sums, allocating zs limbs is not necessary. However, we choose to
     do this now (thus in all cases) because zs is very small, so that
     the difference on the memory footprint will not be noticeable.
     More precisely, zs is at most 2 in practice with the current code;
     we may want to increase it in order to avoid performance issues in
     some unlikely corner cases, but even in this case, it will remain
     small.
     One will have:
       [------ ts ------][------ ws ------][- zs -]
     The following would probably be better:
       [------ ts ------]  [------ ws ------]
                   [- zs -]
     i.e. where the TMD accumulator (partially or completely) takes
     some unneeded part of the temporary area in order to improve
     data locality. But
       * in low precision, data locality is regarded as ensured even
         with the actual choice;
       * in high precision, data locality for TMD resolution may not
         be that important.
  */
  tp = MPFR_TMP_LIMBS_ALLOC (ts + ws + zs);
  wp = tp + ts;

  MPN_ZERO (wp, ws);  /* zero the accumulator */

  {
    mpfr_exp_t minexp;   /* exponent of the LSB of the block for sum_raw */
    mpfr_prec_t cancel;  /* number of cancelled bits */
    mpfr_exp_t e;        /* temporary exponent of the result */
    mpfr_exp_t u;        /* temporary exponent of the ulp (quantum) */
    mp_limb_t lbit;      /* last bit (useful if even rounding) */
    mp_limb_t rbit;      /* rounding bit (corrected in halfway case) */
    int corr;            /* correction term (from -1 to 2) */
    int sd, sh;          /* shift counts */
    mp_size_t sn;        /* size of the output number */
    int tmd;             /* 0: the TMD does not occur
                            1: the TMD occurs on a machine number
                            2: the TMD occurs on a midpoint */
    int neg;             /* 0 if positive sum, 1 if negative */
    int sgn;             /* +1 if positive sum, -1 if negative */

    MPFR_LOG_MSG (("Compute an approximation with sum_raw...\n", 0));

    /* Compute minexp = maxexp - (wq - cq) safely. */
    SAFE_SUB (minexp, maxexp, wq - cq);
    MPFR_ASSERTD (wq >= logn + sq + 5);
    cancel = sum_raw (wp, ws, wq, x, n, minexp, maxexp, tp, ts,
                      logn, sq + 3, &e, &minexp, &maxexp);

    if (MPFR_UNLIKELY (cancel == 0))
      {
        /* The exact sum is zero. Since not all inputs are 0, the sum
         * is +0 except in MPFR_RNDD, as specified according to the
         * IEEE 754 rules for the addition of two numbers.
         */
        MPFR_SET_SIGN (sum, (rnd != MPFR_RNDD ?
                             MPFR_SIGN_POS : MPFR_SIGN_NEG));
        MPFR_SET_ZERO (sum);
        MPFR_TMP_FREE (marker);
        MPFR_RET (0);
      }

    /* The absolute value of the truncated sum is in the binade
       [2^(e-1),2^e] (closed on both ends due to two's complement).
       The error is strictly less than 2^(maxexp + logn) (and is 0
       if maxexp == MPFR_EXP_MIN). */

    u = e - sq;  /* e being the exponent, u is the ulp of the target */

    /* neg = 1 if negative, 0 if positive. */
    neg = wp[ws-1] >> (GMP_NUMB_BITS - 1);
    MPFR_ASSERTD (neg == 0 || neg == 1);

    sgn = neg ? -1 : 1;
    MPFR_ASSERTN (sgn == (neg ? MPFR_SIGN_NEG : MPFR_SIGN_POS));

    MPFR_LOG_MSG (("neg=%d sgn=%d cancel=%Pd"
                   " e=%" MPFR_EXP_FSPEC "d"
                   " u=%" MPFR_EXP_FSPEC "d"
                   " maxexp=%" MPFR_EXP_FSPEC "d%s\n",
                   neg, sgn, cancel, (mpfr_eexp_t) e, (mpfr_eexp_t) u,
                   (mpfr_eexp_t) maxexp,
                   maxexp == MPFR_EXP_MIN ? " (MPFR_EXP_MIN)" : ""));

    if (rnd == MPFR_RNDF)
      {
        /* Rounding the approximate value to nearest (ties don't matter) is
           sufficient. We need to get the rounding bit; the code is similar
           to a part from the generic code (here, corr = rbit). */
        if (MPFR_LIKELY (u > minexp))
          {
            mpfr_prec_t tq;
            mp_size_t wi;
            int td;

            tq = u - minexp;
            MPFR_ASSERTD (tq > 0); /* number of trailing bits */
            MPFR_LOG_MSG (("tq=%Pd\n", tq));

            wi = tq / GMP_NUMB_BITS;
            td = tq % GMP_NUMB_BITS;
            corr = td >= 1 ? ((wp[wi] >> (td - 1)) & MPFR_LIMB_ONE) :
              (MPFR_ASSERTD (wi >= 1), wp[wi-1] >> (GMP_NUMB_BITS - 1));
          }
        else
          corr = 0;
        inex = 0;  /* not meaningful, but needs to have a value */
      }
    else  /* rnd != MPFR_RNDF */
      {
        if (MPFR_LIKELY (u > minexp))
          {
            mpfr_prec_t tq;
            mp_size_t wi;
            int td;

            tq = u - minexp;
            MPFR_ASSERTD (tq > 0); /* number of trailing bits */
            MPFR_LOG_MSG (("tq=%Pd\n", tq));

            wi = tq / GMP_NUMB_BITS;

            /* Determine the rounding bit, which is represented. */
            td = tq % GMP_NUMB_BITS;
            lbit = (wp[wi] >> td) & MPFR_LIMB_ONE;
            rbit = td >= 1 ? ((wp[wi] >> (td - 1)) & MPFR_LIMB_ONE) :
              (MPFR_ASSERTD (wi >= 1), wp[wi-1] >> (GMP_NUMB_BITS - 1));
            MPFR_ASSERTD (rbit == 0 || rbit == 1);
            MPFR_LOG_MSG (("rbit=%d\n", (int) rbit));

            if (maxexp == MPFR_EXP_MIN)
              {
                /* The sum in the accumulator is exact. Determine inex:
                   inex = 0 if the final sum is exact, else 1, i.e.
                   inex = rounding bit || sticky bit. In round to nearest,
                   also determine the rounding direction: obtained from
                   the rounding bit possibly except in halfway cases.
                   Halfway cases are rounded toward -inf iff the last bit
                   of the truncated significand in two's complement is 0
                   (in precision > 1, because the parity after rounding is
                   the same in two's complement and sign + magnitude; in
                   precision 1, one checks that the rule works for both
                   positive (lbit == 1) and negative (lbit == 0) numbers,
                   rounding halfway cases away from zero). */
                if (MPFR_LIKELY (rbit == 0 || (rnd == MPFR_RNDN && lbit == 0)))
                  {
                    /* We need to determine the sticky bit, either to set inex
                       (if the rounding bit is 0) or to possibly "correct" rbit
                       (round to nearest, halfway case rounded downward) from
                       which the rounding direction will be determined. */
                    MPFR_LOG_MSG (("Determine the sticky bit...\n", 0));

                    inex = td >= 2 ? (wp[wi] & MPFR_LIMB_MASK (td - 1)) != 0
                      : td == 0 ?
                      (MPFR_ASSERTD (wi >= 1),
                       (wp[--wi] & MPFR_LIMB_MASK (GMP_NUMB_BITS - 1)) != 0)
                      : 0;

                    if (!inex)
                      {
                        while (!inex && wi > 0)
                          inex = wp[--wi] != 0;
                        if (!inex && rbit != 0)
                          {
                            /* sticky bit = 0, rounding bit = 1,
                               i.e. halfway case, which will be
                               rounded downward (see earlier if). */
                            MPFR_ASSERTD (rnd == MPFR_RNDN);
                            inex = 1;
                            rbit = 0;  /* even rounding downward */
                            MPFR_LOG_MSG (("Halfway case rounded downward;"
                                           " set inex=1 rbit=0\n", 0));
                          }
                      }
                  }
                else
                  inex = 1;
                tmd = 0;  /* We can round correctly -> no TMD. */
              }
            else  /* maxexp > MPFR_EXP_MIN */
              {
                mpfr_exp_t d;
                mp_limb_t limb, mask;
                int nbits;

                /* Since maxexp was set to either the exponent of a x[i] or
                   to minexp... */
                MPFR_ASSERTD (maxexp >= MPFR_EMIN_MIN || maxexp == minexp);

                inex = 1;  /* We do not know whether the sum is exact. */

                MPFR_ASSERTD (u <= MPFR_EMAX_MAX && u <= minexp + wq);
                d = u - (maxexp + logn);  /* representable */
                MPFR_ASSERTD (d >= 3);  /* due to prec = sq + 3 in sum_raw */

                /* Let's see whether the TMD occurs by looking at the d bits
                   following the ulp bit, or the d-1 bits after the rounding
                   bit. */

                /* First chunk after the rounding bit... It starts at:
                   (wi,td-2) if td >= 2,
                   (wi-1,td-2+GMP_NUMB_BITS) if td < 2. */
                if (td == 0)
                  {
                    MPFR_ASSERTD (wi >= 1);
                    limb = wp[--wi];
                    mask = MPFR_LIMB_MASK (GMP_NUMB_BITS - 1);
                    nbits = GMP_NUMB_BITS;
                  }
                else if (td == 1)
                  {
                    limb = wi >= 1 ? wp[--wi] : MPFR_LIMB_ZERO;
                    mask = MPFR_LIMB_MAX;
                    nbits = GMP_NUMB_BITS + 1;
                  }
                else  /* td >= 2 */
                  {
                    MPFR_ASSERTD (td >= 2);
                    limb = wp[wi];
                    mask = MPFR_LIMB_MASK (td - 1);
                    nbits = td;
                  }

                /* nbits: number of bits of the first chunk + 1
                   (the +1 is for the rounding bit). */

                if (nbits > d)
                  {
                    /* Some low significant bits must be ignored. */
                    limb >>= nbits - d;
                    mask >>= nbits - d;
                    d = 0;
                  }
                else
                  {
                    d -= nbits;
                    MPFR_ASSERTD (d >= 0);
                  }

                limb &= mask;
                tmd =
                  limb == MPFR_LIMB_ZERO ?
                    (rbit == 0 ? 1 : rnd == MPFR_RNDN ? 2 : 0) :
                  limb == mask ?
                    (limb = MPFR_LIMB_MAX,
                     rbit != 0 ? 1 : rnd == MPFR_RNDN ? 2 : 0) : 0;

                while (tmd != 0 && d != 0)
                  {
                    mp_limb_t limb2;

                    MPFR_ASSERTD (d > 0);
                    if (wi == 0)
                      {
                        /* The non-represented bits are 0's. */
                        if (limb != MPFR_LIMB_ZERO)
                          tmd = 0;
                        break;
                      }
                    MPFR_ASSERTD (wi > 0);
                    limb2 = wp[--wi];
                    if (d < GMP_NUMB_BITS)
                      {
                        int c = GMP_NUMB_BITS - d;
                        MPFR_ASSERTD (c > 0 && c < GMP_NUMB_BITS);
                        if ((limb2 >> c) != (limb >> c))
                          tmd = 0;
                        break;
                      }
                    if (limb2 != limb)
                      tmd = 0;
                    d -= GMP_NUMB_BITS;
                  }
              }
          }
        else  /* u <= minexp */
          {
            /* The exact value of the accumulator will be copied.
             * The TMD occurs if and only if there are bits still
             * not taken into account, and if it occurs, this is
             * necessarily on a machine number (-> tmd = 1).
             */
            lbit = u == minexp ? wp[0] & MPFR_LIMB_ONE : 0;
            rbit = 0;
            inex = tmd = maxexp != MPFR_EXP_MIN;
          }

        MPFR_ASSERTD (rbit == 0 || rbit == 1);

        MPFR_LOG_MSG (("tmd=%d lbit=%d rbit=%d inex=%d neg=%d\n",
                       tmd, (int) lbit, (int) rbit, inex, neg));

        /* Here, if the final sum is known to be exact, inex = 0, otherwise
         * inex = 1. We have a truncated significand, a trailing term t such
         * that 0 <= t < 1 ulp, and an error on the trailing term bounded by
         * t' in absolute value. Thus the error e on the truncated significand
         * satisfies -t' <= e < 1 ulp + t'. Thus one has 4 correction cases
         * denoted by a corr value between -1 and 2 depending on e, neg, rbit,
         * and the rounding mode:
         *   -1: equivalent to nextbelow;
         *    0: the truncated significand is not corrected;
         *    1: add 1 ulp;
         *    2: add 1 ulp, then nextabove.
         * The nextbelow and nextabove are used here since there may be a
         * change of the binade.
         */

        if (tmd == 0)  /* no TMD */
          {
            switch (rnd)
              {
              case MPFR_RNDD:
                corr = 0;
                break;
              case MPFR_RNDU:
                corr = inex;
                break;
              case MPFR_RNDZ:
                corr = inex && neg;
                break;
              case MPFR_RNDA:
                corr = inex && !neg;
                break;
              default:
                MPFR_ASSERTN (rnd == MPFR_RNDN);
                /* Note: for halfway cases (maxexp == MPFR_EXP_MIN) that are
                   rounded downward, rbit has been changed to 0 so that corr
                   is set correctly. */
                corr = rbit;
              }
            MPFR_ASSERTD (corr == 0 || corr == 1);
            if (inex &&
                corr == 0)  /* two's complement significand decreased */
              inex = -1;
          }
        else  /* tmd */
          {
            mpfr_exp_t minexp2;
            mpfr_prec_t cancel2;
            mpfr_exp_t err;  /* exponent of the error bound */
            mp_size_t zz;    /* nb of limbs to zero in the TMD accumulator */
            mp_limb_t *zp;   /* pointer to the TMD accumulator */
            mpfr_prec_t zq;  /* size of the TMD accumulator, in bits */
            int sst;         /* sign of the secondary term */

            /* TMD case. Here we use a new variable minexp2, with the same
               meaning as minexp, as we want to keep the minexp value for
               the copy to the destination. */

            MPFR_ASSERTD (maxexp > MPFR_EXP_MIN);
            MPFR_ASSERTD (tmd == 1 || tmd == 2);

            /* TMD accumulator */
            zp = wp + ws;
            zq = (mpfr_prec_t) zs * GMP_NUMB_BITS;

            err = maxexp + logn;

            MPFR_LOG_MSG (("TMD with"
                           " maxexp=%" MPFR_EXP_FSPEC "d"
                           " err=%" MPFR_EXP_FSPEC "d"
                           " zs=%Pd"
                           " zq=%Pd\n",
                           (mpfr_eexp_t) maxexp, (mpfr_eexp_t) err,
                           (mpfr_prec_t) zs, zq));

            /* The d-1 bits from u-2 to u-d (= err) are identical. */

            if (err >= minexp)
              {
                mpfr_prec_t tq;
                mp_size_t wi;
                int td;

                /* Let's keep the last 2 over the d-1 identical bits and the
                   following bits, i.e. the bits from err+1 to minexp. */
                tq = err - minexp + 2;  /* tq = number of such bits */
                MPFR_LOG_MSG (("[TMD] tq=%Pd\n", tq));
                MPFR_ASSERTD (tq >= 2);

                wi = tq / GMP_NUMB_BITS;
                td = tq % GMP_NUMB_BITS;

                /* Note: The "else" (td == 0) branch below can be executed
                   only if tq >= GMP_NUMB_BITS, which is possible only when
                   logn is large enough. Indeed, if tq > logn + some constant,
                   this means that the TMD did not occur.
                   TODO: Find an upper bound on tq, and add a corresponding
                   MPFR_ASSERTD assertion / hint. On some platforms, this
                   branch might be dead code, and such information would
                   allow the compiler to remove it.
                   It seems that this branch is never tested (r12754). */

                if (td != 0)
                  {
                    wi++;  /* number of words with represented bits */
                    td = GMP_NUMB_BITS - td;
                    zz = zs - wi;
                    MPFR_ASSERTD (zz >= 0 && zz < zs);
                    mpn_lshift (zp + zz, wp, wi, td);
                  }
                else
                  {
                    MPFR_ASSERTD (wi > 0);
                    zz = zs - wi;
                    MPFR_ASSERTD (zz >= 0 && zz < zs);
                    if (zz > 0)
                      MPN_COPY (zp + zz, wp, wi);
                  }

                /* Compute minexp2 = minexp - (zs * GMP_NUMB_BITS + td)
                   safely. */
                SAFE_SUB (minexp2, minexp, zz * GMP_NUMB_BITS + td);
                MPFR_ASSERTD (minexp2 == err + 2 - zq);
              }
            else  /* err < minexp */
              {
                /* At least one of the identical bits is not represented,
                   meaning that it is 0 and all these bits are 0's. Thus
                   the accumulator will be 0. The new minexp is determined
                   from maxexp, with cq bits reserved to avoid an overflow
                   (as in the early steps). */
                MPFR_LOG_MSG (("[TMD] err < minexp\n", 0));
                zz = zs;

                /* Compute minexp2 = maxexp - (zq - cq) safely. */
                SAFE_SUB (minexp2, maxexp, zq - cq);
                MPFR_ASSERTD (minexp2 == err + 1 - zq);
              }

            MPN_ZERO (zp, zz);

            /* We need to determine the sign sst of the secondary term.
               In sum_raw, since the truncated sum corresponding to this
               secondary term will be in [2^(e-1),2^e] and the error
               strictly less than 2^err, we can stop the iterations when
               e - err >= 1 (this bound is the 11th argument of sum_raw). */
            cancel2 = sum_raw (zp, zs, zq, x, n, minexp2, maxexp, tp, ts,
                               logn, 1, NULL, NULL, NULL);

            if (cancel2 != 0)
              sst = MPFR_LIMB_MSB (zp[zs-1]) == 0 ? 1 : -1;
            else if (tmd == 1)
              sst = 0;
            else
              {
                /* For halfway cases, let's virtually eliminate them
                   by setting a sst equivalent to a non-halfway case,
                   which depends on the last bit of the pre-rounded
                   result. */
                MPFR_ASSERTD (rnd == MPFR_RNDN && tmd == 2);
                sst = lbit != 0 ? 1 : -1;
              }

            MPFR_LOG_MSG (("[TMD] tmd=%d rbit=%d sst=%d\n",
                           tmd, (int) rbit, sst));

            /* Do not consider the corrected sst for MPFR_COV_SET */
            MPFR_COV_SET (sum_tmd[(int) rnd][tmd-1][rbit]
                          [cancel2 == 0 ? 1 : sst+1][neg][sq > MPFR_PREC_MIN]);

            inex =
              MPFR_IS_LIKE_RNDD (rnd, sgn) ? (sst ? -1 : 0) :
              MPFR_IS_LIKE_RNDU (rnd, sgn) ? (sst ?  1 : 0) :
              (MPFR_ASSERTD (rnd == MPFR_RNDN),
               tmd == 1 ? - sst : sst);

            if (tmd == 2 && sst == (rbit != 0 ? -1 : 1))
              corr = 1 - (int) rbit;
            else if (MPFR_IS_LIKE_RNDD (rnd, sgn) && sst == -1)
              corr = (int) rbit - 1;
            else if (MPFR_IS_LIKE_RNDU (rnd, sgn) && sst == +1)
              corr = (int) rbit + 1;
            else
              corr = (int) rbit;
          }  /* tmd */
      }  /* rnd != MPFR_RNDF */

    MPFR_LOG_MSG (("neg=%d corr=%d inex=%d\n", neg, corr, inex));

    /* Sign handling (-> absolute value and sign), together with
       rounding. The most common cases are corr = 0 and corr = 1
       as this is necessarily the case when the TMD did not occur. */

    MPFR_ASSERTD (corr >= -1 && corr <= 2);

    MPFR_SIGN (sum) = sgn;

    /* Let's copy/shift the bits [max(u,minexp),e) to the
       most significant part of the destination, and zero
       the least significant part (there can be one only if
       u < minexp). The trailing bits of the destination may
       contain garbage at this point. */

    sn = MPFR_PREC2LIMBS (sq);
    sd = (mpfr_prec_t) sn * GMP_NUMB_BITS - sq;
    sh = cancel % GMP_NUMB_BITS;

    MPFR_ASSERTD (sd >= 0 && sd < GMP_NUMB_BITS);

    if (MPFR_LIKELY (u > minexp))
      {
        mp_size_t wi;

        /* Recompute the initial value of wi. */
        wi = (u - minexp) / GMP_NUMB_BITS;
        if (MPFR_LIKELY (sh != 0))
          {
            mp_size_t fi;

            fi = (e - minexp) / GMP_NUMB_BITS - (sn - 1);
            MPFR_ASSERTD (fi == wi || fi == wi + 1);
            mpn_lshift (sump, wp + fi, sn, sh);
            if (fi != wi)
              sump[0] |= wp[wi] >> (GMP_NUMB_BITS - sh);
          }
        else
          {
            MPFR_ASSERTD ((mpfr_prec_t) (ws - (wi + sn)) * GMP_NUMB_BITS
                          == cancel);
            MPN_COPY (sump, wp + wi, sn);
          }
      }
    else  /* u <= minexp */
      {
        mp_size_t en;

        en = (e - minexp + (GMP_NUMB_BITS - 1)) / GMP_NUMB_BITS;
        if (MPFR_LIKELY (sh != 0))
          mpn_lshift (sump + sn - en, wp, en, sh);
        else if (MPFR_UNLIKELY (en > 0))
          MPN_COPY (sump + sn - en, wp, en);
        if (sn > en)
          MPN_ZERO (sump, sn - en);
      }

    /* Let's take the complement if the result is negative, and at
       the same time, do the rounding and zero the trailing bits.
       As this is valid only for precisions >= 2, there is special
       code for precision 1 first. */

    if (MPFR_UNLIKELY (sq == 1))  /* precision 1 */
      {
        sump[0] = MPFR_LIMB_HIGHBIT;
        e += neg ? 1 - corr : corr;
      }
    else if (neg)  /* negative result with sq > 1 */
      {
        MPFR_ASSERTD (MPFR_LIMB_MSB (sump[sn-1]) == 0);

        /* abs(x + corr) = - (x + corr) = com(x) + (1 - corr) */

        /* We want to avoid separate mpn_com (or mpn_neg) and mpn_add_1
           (or mpn_sub_1) operations, as they could yield two loops in
           some particular cases involving a long sequence of 0's in
           the low significant bits (except the least significant bit,
           which doesn't matter). */

        if (corr <= 1)
          {
            mp_limb_t corr2;

            /* Here we can just do the correction operation on the
               least significant limb, then do either a mpn_com or
               a mpn_neg on the remaining limbs, depending on the
               carry (BTW, mpn_neg is just a mpn_com with an initial
               carry propagation: after some point, mpn_neg does a
               complement). */

            corr2 = (mp_limb_t) (1 - corr) << sd;
            /* Note: If corr = -1, this can overflow to corr2 = 0.
               This case is taken into account below. */

            sump[0] = (~ (sump[0] | MPFR_LIMB_MASK (sd))) + corr2;

            if (sump[0] < corr2 || (corr2 == 0 && corr < 0))
              {
                if (sn == 1 || ! mpn_neg (sump + 1, sump + 1, sn - 1))
                  {
                    /* Note: The | is important when sump[sn-1] is not 0
                       (this can occur with sn = 1 and corr = -1). TODO:
                       Add something to make sure that this is tested. */
                    sump[sn-1] |= MPFR_LIMB_HIGHBIT;
                    e++;
                  }
              }
            else if (sn > 1)
              mpn_com (sump + 1, sump + 1, sn - 1);
          }
        else  /* corr == 2 */
          {
            mp_limb_t corr2, c;
            mp_size_t i = 1;

            /* We want to compute com(x) - 1, but GMP doesn't have an
               operation for that. The fact is that a sequence of low
               significant bits 1 is invariant. Starting at the first
               low significant bit 0, we can do the complement with
               mpn_com. */

            corr2 = MPFR_LIMB_ONE << sd;
            c = ~ (sump[0] | MPFR_LIMB_MASK (sd));
            sump[0] = c - corr2;

            if (c == 0)
              {
                while (MPFR_ASSERTD (i < sn), sump[i] == MPFR_LIMB_MAX)
                  i++;
                sump[i] = (~ sump[i]) - 1;
                i++;
              }

            if (i < sn)
              mpn_com (sump + i, sump + i, sn - i);
            else if (MPFR_UNLIKELY (MPFR_LIMB_MSB (sump[sn-1]) == 0))
              {
                /* Happens on 01111...111, whose complement is
                   10000...000, and com(x) - 1 is 01111...111. */
                sump[sn-1] |= MPFR_LIMB_HIGHBIT;
                e--;
              }
          }
      }
    else  /* positive result with sq > 1 */
      {
        MPFR_ASSERTD (MPFR_LIMB_MSB (sump[sn-1]) != 0);
        sump[0] &= ~ MPFR_LIMB_MASK (sd);

        if (corr > 0)
          {
            mp_limb_t corr2, carry_out;

            corr2 = (mp_limb_t) corr << sd;
            /* If corr == 2 && sd == GMP_NUMB_BITS - 1, this overflows
               to corr2 = 0. This case is taken into account below. */

            carry_out = corr2 != 0 ? mpn_add_1 (sump, sump, sn, corr2) :
              (MPFR_ASSERTD (sn > 1),
               mpn_add_1 (sump + 1, sump + 1, sn - 1, MPFR_LIMB_ONE));

            MPFR_ASSERTD (sump[sn-1] >> (GMP_NUMB_BITS - 1) == !carry_out);

            if (MPFR_UNLIKELY (carry_out))
              {
                /* Note: The | is important when sump[sn-1] is not 0
                   (this can occur with sn = 1 and corr = 2). TODO:
                   Add something to make sure that this is tested. */
                sump[sn-1] |= MPFR_LIMB_HIGHBIT;
                e++;
              }
          }

        if (corr < 0)
          {
            mpn_sub_1 (sump, sump, sn, MPFR_LIMB_ONE << sd);

            if (MPFR_UNLIKELY (MPFR_LIMB_MSB (sump[sn-1]) == 0))
              {
                sump[sn-1] |= MPFR_LIMB_HIGHBIT;
                e--;
              }
          }
      }

    MPFR_ASSERTD (MPFR_LIMB_MSB (sump[sn-1]) != 0);
    MPFR_LOG_MSG (("Set exponent e=%" MPFR_EXP_FSPEC "d\n", (mpfr_eexp_t) e));
    /* e may be outside the current exponent range, but this will be checked
       with mpfr_check_range below. */
    MPFR_EXP (sum) = e;
  }  /* main block */

  MPFR_TMP_FREE (marker);
  return mpfr_check_range (sum, inex, rnd);
}

/**********************************************************************/

int
mpfr_sum (mpfr_ptr sum, const mpfr_ptr *x, unsigned long n, mpfr_rnd_t rnd)
{
  MPFR_LOG_FUNC
    (("n=%lu rnd=%d", n, rnd),
     ("sum[%Pd]=%.*Rg", mpfr_get_prec (sum), mpfr_log_prec, sum));

  if (MPFR_UNLIKELY (n <= 2))
    {
      if (n == 0)
        {
          MPFR_SET_ZERO (sum);
          MPFR_SET_POS (sum);
          MPFR_RET (0);
        }
      else if (n == 1)
        return mpfr_set (sum, x[0], rnd);
      else
        return mpfr_add (sum, x[0], x[1], rnd);
    }
  else
    {
      mpfr_exp_t maxexp = MPFR_EXP_MIN;  /* max(Empty) */
      unsigned long i;
      unsigned long rn = 0;  /* will be the number of regular inputs */
      /* sign of infinities and zeros (0: currently unknown) */
      int sign_inf = 0, sign_zero = 0;

      MPFR_LOG_MSG (("Check for special inputs (n = %lu >= 3)\n", n));

      for (i = 0; i < n; i++)
        {
          if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x[i])))
            {
              if (MPFR_IS_NAN (x[i]))
                {
                  /* The current value x[i] is NaN. Then the sum is NaN. */
                nan:
                  MPFR_SET_NAN (sum);
                  MPFR_RET_NAN;
                }
              else if (MPFR_IS_INF (x[i]))
                {
                  /* The current value x[i] is an infinity.
                     There are two cases:
                     1. This is the first infinity value (sign_inf == 0).
                        Then set sign_inf to its sign, and go on.
                     2. All the infinities found until now have the same
                        sign sign_inf. If this new infinity has a different
                        sign, then return NaN immediately, else go on. */
                  if (sign_inf == 0)
                    sign_inf = MPFR_SIGN (x[i]);
                  else if (MPFR_SIGN (x[i]) != sign_inf)
                    goto nan;
                }
              else if (MPFR_UNLIKELY (rn == 0))
                {
                  /* The current value x[i] is a zero. The code below matters
                     only when all values found until now are zeros, otherwise
                     it is harmless (the test rn == 0 above is just a minor
                     optimization).
                     Here we track the sign of the zero result when all inputs
                     are zeros: if all zeros have the same sign, the result
                     will have this sign, otherwise (i.e. if there is at least
                     a zero of each sign), the sign of the zero result depends
                     only on the rounding mode (note that this choice is
                     sticky when new zeros are considered). */
                  MPFR_ASSERTD (MPFR_IS_ZERO (x[i]));
                  if (sign_zero == 0)
                    sign_zero = MPFR_SIGN (x[i]);
                  else if (MPFR_SIGN (x[i]) != sign_zero)
                    sign_zero = rnd == MPFR_RNDD ? -1 : 1;
                }
            }
          else
            {
              /* The current value x[i] is a regular number. */
              mpfr_exp_t e = MPFR_GET_EXP (x[i]);
              if (e > maxexp)
                maxexp = e;  /* maximum exponent found until now */
              rn++;  /* current number of regular inputs */
            }
        }

      MPFR_LOG_MSG (("rn=%lu sign_inf=%d sign_zero=%d\n",
                     rn, sign_inf, sign_zero));

      /* At this point the result cannot be NaN (this case has already
         been filtered out). */

      if (MPFR_UNLIKELY (sign_inf != 0))
        {
          /* At least one infinity, and all of them have the same sign
             sign_inf. The sum is the infinity of this sign. */
          MPFR_SET_INF (sum);
          MPFR_SET_SIGN (sum, sign_inf);
          MPFR_RET (0);
        }

      /* At this point, all the inputs are finite numbers. */

      if (MPFR_UNLIKELY (rn == 0))
        {
          /* All the numbers were zeros (and there is at least one).
             The sum is zero with sign sign_zero. */
          MPFR_ASSERTD (sign_zero != 0);
          MPFR_SET_ZERO (sum);
          MPFR_SET_SIGN (sum, sign_zero);
          MPFR_RET (0);
        }

      /* Optimize the case where there are only two regular numbers. */
      if (MPFR_UNLIKELY (rn <= 2))
        {
          unsigned long h = ULONG_MAX;

          for (i = 0; i < n; i++)
            if (! MPFR_IS_SINGULAR (x[i]))
              {
                if (rn == 1)
                  return mpfr_set (sum, x[i], rnd);
                if (h != ULONG_MAX)
                  return mpfr_add (sum, x[h], x[i], rnd);
                h = i;
              }
          MPFR_RET_NEVER_GO_HERE();
        }

      return sum_aux (sum, x, n, rnd, maxexp, rn);
    }
}
