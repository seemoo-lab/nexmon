/* mpfr_pow_si -- power function x^y with y a signed int

Copyright 2001-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H  /* for MPFR_INT_CEIL_LOG2 */
#include "mpfr-impl.h"

/* The computation of y = pow_si/sj(x,n) is done by
 *    y = pow_ui/uj(x,n)       if n >= 0
 *    y = 1 / pow_ui/uj(x,-n)  if n < 0
 */

#ifndef POW_S
#define POW_S mpfr_pow_si
#define POW_U mpfr_pow_ui
#define SET_S mpfr_set_si
#define SET_S_2EXP mpfr_set_si_2exp
#define NBITS_UTYPE mpfr_nbits_ulong
#define TYPE long int
#define UTYPE unsigned long
#define FSPEC "l"
#endif

int
POW_S (mpfr_ptr y, mpfr_srcptr x, TYPE n, mpfr_rnd_t rnd)
{
  MPFR_LOG_FUNC
    (("x[%Pd]=%.*Rg n=%" FSPEC "d rnd=%d",
      mpfr_get_prec (x), mpfr_log_prec, x, n, rnd),
     ("y[%Pd]=%.*Rg", mpfr_get_prec (y), mpfr_log_prec, y));

  if (n >= 0)
    return POW_U (y, x, n, rnd);
  else
    {
      if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
        {
          if (MPFR_IS_NAN (x))
            {
              MPFR_SET_NAN (y);
              MPFR_RET_NAN;
            }
          else
            {
              int positive = MPFR_IS_POS (x) || ((UTYPE) n & 1) == 0;
              if (MPFR_IS_INF (x))
                MPFR_SET_ZERO (y);
              else /* x is zero */
                {
                  MPFR_ASSERTD (MPFR_IS_ZERO (x));
                  MPFR_SET_INF (y);
                  MPFR_SET_DIVBY0 ();
                }
              if (positive)
                MPFR_SET_POS (y);
              else
                MPFR_SET_NEG (y);
              MPFR_RET (0);
            }
        }

      /* detect exact powers: x^(-n) is exact iff x is a power of 2 */
      if (mpfr_cmp_si_2exp (x, MPFR_SIGN(x), MPFR_EXP(x) - 1) == 0)
        {
          mpfr_exp_t expx = MPFR_EXP (x) - 1, expy;
          MPFR_ASSERTD (n < 0);
          /* Warning: n * expx may overflow!
           *
           * Some systems (apparently alpha-freebsd) abort with
           * LONG_MIN / 1, and LONG_MIN / -1 is undefined.
           * https://bugs.freebsd.org/bugzilla/show_bug.cgi?id=72024
           *
           * Proof of the overflow checking. The expressions below are
           * assumed to be on the rational numbers, but the word "overflow"
           * still has its own meaning in the C context. / still denotes
           * the integer (truncated) division, and // denotes the exact
           * division.
           * - First, (__gmpfr_emin - 1) / n and (__gmpfr_emax - 1) / n
           *   cannot overflow due to the constraints on the exponents of
           *   MPFR numbers.
           * - If n = -1, then n * expx = - expx, which is representable
           *   because of the constraints on the exponents of MPFR numbers.
           * - If expx = 0, then n * expx = 0, which is representable.
           * - If n < -1 and expx > 0:
           *   + If expx > (__gmpfr_emin - 1) / n, then
           *           expx >= (__gmpfr_emin - 1) / n + 1
           *                > (__gmpfr_emin - 1) // n,
           *     and
           *           n * expx < __gmpfr_emin - 1,
           *     i.e.
           *           n * expx <= __gmpfr_emin - 2.
           *     This corresponds to an underflow, with a null result in
           *     the rounding-to-nearest mode.
           *   + If expx <= (__gmpfr_emin - 1) / n, then n * expx cannot
           *     overflow since 0 < expx <= (__gmpfr_emin - 1) / n and
           *           0 > n * expx >= n * ((__gmpfr_emin - 1) / n)
           *                        >= __gmpfr_emin - 1.
           * - If n < -1 and expx < 0:
           *   + If expx < (__gmpfr_emax - 1) / n, then
           *           expx <= (__gmpfr_emax - 1) / n - 1
           *                < (__gmpfr_emax - 1) // n,
           *     and
           *           n * expx > __gmpfr_emax - 1,
           *     i.e.
           *           n * expx >= __gmpfr_emax.
           *     This corresponds to an overflow (2^(n * expx) has an
           *     exponent > __gmpfr_emax).
           *   + If expx >= (__gmpfr_emax - 1) / n, then n * expx cannot
           *     overflow since 0 > expx >= (__gmpfr_emax - 1) / n and
           *           0 < n * expx <= n * ((__gmpfr_emax - 1) / n)
           *                        <= __gmpfr_emax - 1.
           * Note: one could use expx bounds based on MPFR_EXP_MIN and
           * MPFR_EXP_MAX instead of __gmpfr_emin and __gmpfr_emax. The
           * current bounds do not lead to noticeably slower code and
           * allow us to avoid a bug in Sun's compiler for Solaris/x86
           * (when optimizations are enabled); known affected versions:
           *   cc: Sun C 5.8 2005/10/13
           *   cc: Sun C 5.8 Patch 121016-02 2006/03/31
           *   cc: Sun C 5.8 Patch 121016-04 2006/10/18
           */
          expy =
            n != -1 && expx > 0 && expx > (__gmpfr_emin - 1) / n ?
            MPFR_EMIN_MIN - 2 /* Underflow */ :
            n != -1 && expx < 0 && expx < (__gmpfr_emax - 1) / n ?
            MPFR_EMAX_MAX /* Overflow */ : n * expx;
          return SET_S_2EXP (y, n % 2 ? MPFR_INT_SIGN (x) : 1, expy, rnd);
        }

      /* General case */
      {
        /* Declaration of the intermediary variable */
        mpfr_t t;
        /* Declaration of the size variable */
        mpfr_prec_t Ny;                              /* target precision */
        mpfr_prec_t Nt;                              /* working precision */
        mpfr_rnd_t rnd1;
        int size_n;
        int inexact;
        UTYPE abs_n;
        MPFR_SAVE_EXPO_DECL (expo);
        MPFR_ZIV_DECL (loop);

        abs_n = - (UTYPE) n;
        size_n = NBITS_UTYPE (abs_n);

        /* initial working precision */
        Ny = MPFR_PREC (y);
        Nt = Ny + size_n + 3 + MPFR_INT_CEIL_LOG2 (Ny);

        MPFR_SAVE_EXPO_MARK (expo);

        /* initialize of intermediary   variable */
        mpfr_init2 (t, Nt);

        /* We will compute rnd(rnd1(1/x) ^ |n|), where rnd1 is the rounding
           toward sign(x), to avoid spurious overflow or underflow, as in
           mpfr_pow_z. */
        rnd1 = MPFR_EXP (x) < 1 ? MPFR_RNDZ :
          (MPFR_IS_POS (x) ? MPFR_RNDU : MPFR_RNDD);

        /* The following ensures that 1/x cannot underflow.
           Since |x| < 2^emax, |1/x| > 2^(-emax) >= 2^emin. */
        MPFR_STAT_STATIC_ASSERT (MPFR_EMIN_MIN + MPFR_EMAX_MAX <= 0);

        MPFR_ZIV_INIT (loop, Nt);
        for (;;)
          {
            MPFR_BLOCK_DECL (flags);

            /* TODO: Compute POW_U before the division (instead of after)
               in order to reduce the error in the intermediate result?
               POW_U, whose condition number is |n|, which may be large,
               would be called on an exact value. This may be important
               in very small precisions.
               In this case, if x^|n| underflows, then |x^n| > 2^emax
               (real overflow, and we can return the result); and if
               x^|n| overflows, then the result underflows or is very
               close to the underflow threshold, so that we should use
               mpfr_pow_general (as already done for MPFR_RNDN), which
               can handle such a case.
               So the advantage of computing POW_U before the division
               is that the code would be slightly faster is the general
               case, but it could be noticeably slower in very uncommon
               cases (and only with the extended exponent range). */

            /* compute (1/x)^|n| */
            MPFR_BLOCK (flags, mpfr_ui_div (t, 1, x, rnd1));
            MPFR_ASSERTD (! MPFR_UNDERFLOW (flags));
            /* t = (1/x)*(1+theta) where |theta| <= 2^(-Nt) */
            if (MPFR_UNLIKELY (MPFR_OVERFLOW (flags)))
              goto overflow;
            MPFR_BLOCK (flags, POW_U (t, t, abs_n, rnd));
            /* t = (1/x)^|n|*(1+theta')^(|n|+1) where |theta'| <= 2^(-Nt).
               If (|n|+1)*2^(-Nt) <= 1/2, which is satisfied as soon as
               Nt >= bits(n)+2, then we can use Lemma \ref{lemma_graillat}
               from algorithms.tex, which yields x^n*(1+theta) with
               |theta| <= 2(|n|+1)*2^(-Nt), thus the error is bounded by
               2(|n|+1) ulps <= 2^(bits(n)+2) ulps. */
            if (MPFR_UNLIKELY (MPFR_OVERFLOW (flags)))
              {
              overflow:
                MPFR_ZIV_FREE (loop);
                mpfr_clear (t);
                MPFR_SAVE_EXPO_FREE (expo);
                MPFR_LOG_MSG (("overflow\n", 0));
                return mpfr_overflow (y, rnd, abs_n & 1 ?
                                      MPFR_SIGN (x) : MPFR_SIGN_POS);
              }
            if (MPFR_UNLIKELY (MPFR_UNDERFLOW (flags)))
              {
                MPFR_ZIV_FREE (loop);
                mpfr_clear (t);
                MPFR_LOG_MSG (("underflow\n", 0));
                if (rnd == MPFR_RNDN)
                  {
                    mpfr_t y2, nn;

                    /* We cannot decide now whether the result should be
                       rounded toward zero or away from zero. So, like
                       in mpfr_pow_pos_z, let's use the general case of
                       mpfr_pow in precision 2. */
                    MPFR_ASSERTD (mpfr_cmp_si_2exp (x, MPFR_SIGN (x),
                                                    MPFR_EXP (x) - 1) != 0);
                    mpfr_init2 (y2, 2);
                    mpfr_init2 (nn, sizeof (TYPE) * CHAR_BIT);
                    inexact = SET_S (nn, n, MPFR_RNDN);
                    MPFR_ASSERTN (inexact == 0);
                    inexact = mpfr_pow_general (y2, x, nn, rnd, 1,
                                                (mpfr_save_expo_t *) NULL);
                    mpfr_clear (nn);
                    mpfr_set (y, y2, MPFR_RNDN);
                    mpfr_clear (y2);
                    MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, MPFR_FLAGS_UNDERFLOW);
                    goto end;
                  }
                else
                  {
                    MPFR_SAVE_EXPO_FREE (expo);
                    return mpfr_underflow (y, rnd, abs_n & 1 ?
                                           MPFR_SIGN (x) : MPFR_SIGN_POS);
                  }
              }
            /* error estimate -- see pow function in algorithms.ps */
            if (MPFR_LIKELY (MPFR_CAN_ROUND (t, Nt - size_n - 2, Ny, rnd)))
              break;

            /* actualization of the precision */
            MPFR_ZIV_NEXT (loop, Nt);
            mpfr_set_prec (t, Nt);
          }
        MPFR_ZIV_FREE (loop);

        inexact = mpfr_set (y, t, rnd);
        mpfr_clear (t);

      end:
        MPFR_SAVE_EXPO_FREE (expo);
        return mpfr_check_range (y, inexact, rnd);
      }
    }
}
