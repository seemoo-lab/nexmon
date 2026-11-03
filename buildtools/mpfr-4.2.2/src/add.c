/* mpfr_add -- add two floating-point numbers

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

#include "mpfr-impl.h"

MPFR_HOT_FUNCTION_ATTR int
mpfr_add (mpfr_ptr a, mpfr_srcptr b, mpfr_srcptr c, mpfr_rnd_t rnd_mode)
{
  MPFR_LOG_FUNC
    (("b[%Pd]=%.*Rg c[%Pd]=%.*Rg rnd=%d",
      mpfr_get_prec (b), mpfr_log_prec, b,
      mpfr_get_prec (c), mpfr_log_prec, c, rnd_mode),
     ("a[%Pd]=%.*Rg", mpfr_get_prec (a), mpfr_log_prec, a));

  if (MPFR_ARE_SINGULAR_OR_UBF (b, c))
    {
      if (MPFR_IS_NAN(b) || MPFR_IS_NAN(c))
        {
          MPFR_SET_NAN(a);
          MPFR_RET_NAN;
        }
      /* neither b nor c is NaN here */
      else if (MPFR_IS_INF(b))
        {
          if (!MPFR_IS_INF(c) || MPFR_SIGN(b) == MPFR_SIGN(c))
            {
              MPFR_SET_INF(a);
              MPFR_SET_SAME_SIGN(a, b);
              MPFR_RET(0); /* exact */
            }
          else
            {
              MPFR_SET_NAN(a);
              MPFR_RET_NAN;
            }
        }
      else if (MPFR_IS_INF(c))
          {
            MPFR_SET_INF(a);
            MPFR_SET_SAME_SIGN(a, c);
            MPFR_RET(0); /* exact */
          }
      /* now both b and c are finite numbers */
      else if (MPFR_IS_ZERO(b))
        {
          if (MPFR_IS_ZERO(c))
            {
              /* for round away, we take the same convention for 0 + 0
                 as for round to zero or to nearest: it always gives +0,
                 except (-0) + (-0) = -0. */
              MPFR_SET_SIGN(a,
                            (rnd_mode != MPFR_RNDD ?
                             (MPFR_IS_NEG(b) && MPFR_IS_NEG(c) ?
                              MPFR_SIGN_NEG : MPFR_SIGN_POS) :
                             (MPFR_IS_POS(b) && MPFR_IS_POS(c) ?
                              MPFR_SIGN_POS : MPFR_SIGN_NEG)));
              MPFR_SET_ZERO(a);
              MPFR_RET(0); /* 0 + 0 is exact */
            }
          return mpfr_set (a, c, rnd_mode);
        }
      else if (MPFR_IS_ZERO(c))
        {
          return mpfr_set (a, b, rnd_mode);
        }
      else
        {
          MPFR_ASSERTD (MPFR_IS_PURE_UBF (b));
          MPFR_ASSERTD (MPFR_IS_PURE_UBF (c));
          /* mpfr_sub1sp and mpfr_add1sp are not intended to support UBF,
             for which optimization is less important. */
          if (MPFR_SIGN(b) != MPFR_SIGN(c))
            return mpfr_sub1 (a, b, c, rnd_mode);
          else if (MPFR_UBF_EXP_LESS_P (b, c))
            return mpfr_add1 (a, c, b, rnd_mode);
          else
            return mpfr_add1 (a, b, c, rnd_mode);
        }
    }

  MPFR_ASSERTD (MPFR_IS_PURE_FP (b));
  MPFR_ASSERTD (MPFR_IS_PURE_FP (c));

  if (MPFR_UNLIKELY(MPFR_SIGN(b) != MPFR_SIGN(c)))
    { /* signs differ, it is a subtraction */
      if (MPFR_LIKELY(MPFR_PREC(a) == MPFR_PREC(b)
                      && MPFR_PREC(b) == MPFR_PREC(c)))
        return mpfr_sub1sp(a, b, c, rnd_mode);
      else
        return mpfr_sub1(a, b, c, rnd_mode);
    }
  else
    { /* signs are equal, it's an addition */
      if (MPFR_LIKELY(MPFR_PREC(a) == MPFR_PREC(b)
                      && MPFR_PREC(b) == MPFR_PREC(c)))
        return mpfr_add1sp(a, b, c, rnd_mode);
      else
        if (MPFR_GET_EXP(b) < MPFR_GET_EXP(c))
          return mpfr_add1(a, c, b, rnd_mode);
        else
          return mpfr_add1(a, b, c, rnd_mode);
    }
}
