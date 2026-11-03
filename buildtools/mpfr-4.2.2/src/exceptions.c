/* Exception flags and utilities. Constructors and destructors (debug).

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

#include "mpfr-impl.h"

MPFR_THREAD_VAR (mpfr_flags_t, __gmpfr_flags, 0)
MPFR_THREAD_VAR (mpfr_exp_t, __gmpfr_emin, MPFR_EMIN_DEFAULT)
MPFR_THREAD_VAR (mpfr_exp_t, __gmpfr_emax, MPFR_EMAX_DEFAULT)

#undef mpfr_get_emin

MPFR_COLD_FUNCTION_ATTR mpfr_exp_t
mpfr_get_emin (void)
{
  return __gmpfr_emin;
}

#undef mpfr_set_emin

int
mpfr_set_emin (mpfr_exp_t exponent)
{
  if (MPFR_LIKELY (exponent >= MPFR_EMIN_MIN && exponent <= MPFR_EMIN_MAX))
    {
      __gmpfr_emin = exponent;
      return 0;
    }
  else
    {
      return 1;
    }
}

MPFR_COLD_FUNCTION_ATTR mpfr_exp_t
mpfr_get_emin_min (void)
{
  return MPFR_EMIN_MIN;
}

MPFR_COLD_FUNCTION_ATTR mpfr_exp_t
mpfr_get_emin_max (void)
{
  return MPFR_EMIN_MAX;
}

#undef mpfr_get_emax

MPFR_COLD_FUNCTION_ATTR mpfr_exp_t
mpfr_get_emax (void)
{
  return __gmpfr_emax;
}

#undef mpfr_set_emax

int
mpfr_set_emax (mpfr_exp_t exponent)
{
  if (MPFR_LIKELY (exponent >= MPFR_EMAX_MIN && exponent <= MPFR_EMAX_MAX))
    {
      __gmpfr_emax = exponent;
      return 0;
    }
  else
    {
      return 1;
    }
}

MPFR_COLD_FUNCTION_ATTR mpfr_exp_t
mpfr_get_emax_min (void)
{
  return MPFR_EMAX_MIN;
}

MPFR_COLD_FUNCTION_ATTR mpfr_exp_t
mpfr_get_emax_max (void)
{
  return MPFR_EMAX_MAX;
}


#undef mpfr_flags_clear

MPFR_COLD_FUNCTION_ATTR void
mpfr_flags_clear (mpfr_flags_t mask)
{
  __gmpfr_flags &= MPFR_FLAGS_ALL ^ mask;
}

#undef mpfr_flags_set

MPFR_COLD_FUNCTION_ATTR void
mpfr_flags_set (mpfr_flags_t mask)
{
  __gmpfr_flags |= mask;
}

#undef mpfr_flags_test

MPFR_COLD_FUNCTION_ATTR mpfr_flags_t
mpfr_flags_test (mpfr_flags_t mask)
{
  return __gmpfr_flags & mask;
}

#undef mpfr_flags_save

MPFR_COLD_FUNCTION_ATTR mpfr_flags_t
mpfr_flags_save (void)
{
  return __gmpfr_flags;
}

#undef mpfr_flags_restore

MPFR_COLD_FUNCTION_ATTR void
mpfr_flags_restore (mpfr_flags_t flags, mpfr_flags_t mask)
{
  __gmpfr_flags =
    (__gmpfr_flags & (MPFR_FLAGS_ALL ^ mask)) |
    (flags & mask);
}


#undef mpfr_clear_flags

void
mpfr_clear_flags (void)
{
  __gmpfr_flags = 0;
}

#undef mpfr_clear_underflow

MPFR_COLD_FUNCTION_ATTR void
mpfr_clear_underflow (void)
{
  __gmpfr_flags &= MPFR_FLAGS_ALL ^ MPFR_FLAGS_UNDERFLOW;
}

#undef mpfr_clear_overflow

MPFR_COLD_FUNCTION_ATTR void
mpfr_clear_overflow (void)
{
  __gmpfr_flags &= MPFR_FLAGS_ALL ^ MPFR_FLAGS_OVERFLOW;
}

#undef mpfr_clear_divby0

MPFR_COLD_FUNCTION_ATTR void
mpfr_clear_divby0 (void)
{
  __gmpfr_flags &= MPFR_FLAGS_ALL ^ MPFR_FLAGS_DIVBY0;
}

#undef mpfr_clear_nanflag

MPFR_COLD_FUNCTION_ATTR void
mpfr_clear_nanflag (void)
{
  __gmpfr_flags &= MPFR_FLAGS_ALL ^ MPFR_FLAGS_NAN;
}

#undef mpfr_clear_inexflag

MPFR_COLD_FUNCTION_ATTR void
mpfr_clear_inexflag (void)
{
  __gmpfr_flags &= MPFR_FLAGS_ALL ^ MPFR_FLAGS_INEXACT;
}

#undef mpfr_clear_erangeflag

MPFR_COLD_FUNCTION_ATTR void
mpfr_clear_erangeflag (void)
{
  __gmpfr_flags &= MPFR_FLAGS_ALL ^ MPFR_FLAGS_ERANGE;
}

#undef mpfr_set_underflow

MPFR_COLD_FUNCTION_ATTR void
mpfr_set_underflow (void)
{
  __gmpfr_flags |= MPFR_FLAGS_UNDERFLOW;
}

#undef mpfr_set_overflow

MPFR_COLD_FUNCTION_ATTR void
mpfr_set_overflow (void)
{
  __gmpfr_flags |= MPFR_FLAGS_OVERFLOW;
}

#undef mpfr_set_divby0

MPFR_COLD_FUNCTION_ATTR void
mpfr_set_divby0 (void)
{
  __gmpfr_flags |= MPFR_FLAGS_DIVBY0;
}

#undef mpfr_set_nanflag

MPFR_COLD_FUNCTION_ATTR void
mpfr_set_nanflag (void)
{
  __gmpfr_flags |= MPFR_FLAGS_NAN;
}

#undef mpfr_set_inexflag

MPFR_COLD_FUNCTION_ATTR void
mpfr_set_inexflag (void)
{
  __gmpfr_flags |= MPFR_FLAGS_INEXACT;
}

#undef mpfr_set_erangeflag

MPFR_COLD_FUNCTION_ATTR void
mpfr_set_erangeflag (void)
{
  __gmpfr_flags |= MPFR_FLAGS_ERANGE;
}


#undef mpfr_check_range

/* Note: It is possible that for pure FP numbers, EXP(x) < MPFR_EMIN_MIN,
   but the caller must make sure that the difference remains small enough
   to avoid reaching the special exponent values. */
/* This function does not have logging messages. As it is also partly
   implemented as a macro, if messages are added in the future, the macro
   may need to be disabled when logging is enabled. */
int
mpfr_check_range (mpfr_ptr x, int t, mpfr_rnd_t rnd_mode)
{
  if (MPFR_LIKELY (! MPFR_IS_SINGULAR (x)))
    { /* x is a non-zero FP */
      mpfr_exp_t exp = MPFR_EXP (x);  /* Do not use MPFR_GET_EXP */

      MPFR_ASSERTD (MPFR_IS_NORMALIZED (x));
      if (MPFR_UNLIKELY (exp < __gmpfr_emin))
        {
          /* The following test is necessary because in the rounding to the
           * nearest mode, mpfr_underflow always rounds away from 0. In
           * this rounding mode, we need to round to 0 if:
           *   _ |x| < 2^(emin-2), or
           *   _ |x| = 2^(emin-2) and the absolute value of the exact
           *     result is <= 2^(emin-2).
           */
          if (rnd_mode == MPFR_RNDN &&
              (exp + 1 < __gmpfr_emin ||
               (mpfr_powerof2_raw(x) &&
                (MPFR_IS_NEG(x) ? t <= 0 : t >= 0))))
            rnd_mode = MPFR_RNDZ;
          return mpfr_underflow (x, rnd_mode, MPFR_SIGN(x));
        }
      if (MPFR_UNLIKELY (exp > __gmpfr_emax))
        return mpfr_overflow (x, rnd_mode, MPFR_SIGN(x));
    }
  else if (MPFR_UNLIKELY (t != 0 && MPFR_IS_INF (x)))
    {
      /* We need to do the following because most MPFR functions are
       * implemented in the following way:
       *   Ziv's loop:
       *   | Compute an approximation to the result and an error bound.
       *   | Possible underflow/overflow detection -> return.
       *   | If can_round, break (exit the loop).
       *   | Otherwise, increase the working precision and loop.
       *   Round the approximation in the target precision.  <== See below
       *   Restore the flags (that could have been set due to underflows
       *   or overflows during the internal computations).
       *   Execute: return mpfr_check_range (...).
       * The problem is that an overflow could be generated when rounding the
       * approximation (in general, such an overflow could not be detected
       * earlier), and the overflow flag is lost when the flags are restored.
       * This can occur only when the rounding yields an exponent change
       * and the new exponent is larger than the maximum exponent, so that
       * an infinity is necessarily obtained.
       * So, the simplest solution is to detect this overflow case here in
       * mpfr_check_range, which is easy to do since the rounded result is
       * necessarily an inexact infinity.
       */
      __gmpfr_flags |= MPFR_FLAGS_OVERFLOW;
    }
  MPFR_RET (t);  /* propagate inexact ternary value, unlike most functions */
}


#undef mpfr_underflow_p

MPFR_COLD_FUNCTION_ATTR int
mpfr_underflow_p (void)
{
  MPFR_STAT_STATIC_ASSERT (MPFR_FLAGS_UNDERFLOW <= INT_MAX);
  return __gmpfr_flags & MPFR_FLAGS_UNDERFLOW;
}

#undef mpfr_overflow_p

MPFR_COLD_FUNCTION_ATTR int
mpfr_overflow_p (void)
{
  MPFR_STAT_STATIC_ASSERT (MPFR_FLAGS_OVERFLOW <= INT_MAX);
  return __gmpfr_flags & MPFR_FLAGS_OVERFLOW;
}

#undef mpfr_divby0_p

MPFR_COLD_FUNCTION_ATTR int
mpfr_divby0_p (void)
{
  MPFR_STAT_STATIC_ASSERT (MPFR_FLAGS_DIVBY0 <= INT_MAX);
  return __gmpfr_flags & MPFR_FLAGS_DIVBY0;
}

#undef mpfr_nanflag_p

MPFR_COLD_FUNCTION_ATTR int
mpfr_nanflag_p (void)
{
  MPFR_STAT_STATIC_ASSERT (MPFR_FLAGS_NAN <= INT_MAX);
  return __gmpfr_flags & MPFR_FLAGS_NAN;
}

#undef mpfr_inexflag_p

MPFR_COLD_FUNCTION_ATTR int
mpfr_inexflag_p (void)
{
  MPFR_STAT_STATIC_ASSERT (MPFR_FLAGS_INEXACT <= INT_MAX);
  return __gmpfr_flags & MPFR_FLAGS_INEXACT;
}

#undef mpfr_erangeflag_p

MPFR_COLD_FUNCTION_ATTR int
mpfr_erangeflag_p (void)
{
  MPFR_STAT_STATIC_ASSERT (MPFR_FLAGS_ERANGE <= INT_MAX);
  return __gmpfr_flags & MPFR_FLAGS_ERANGE;
}


/* #undef mpfr_underflow */

/* Note: In the rounding to the nearest mode, mpfr_underflow
   always rounds away from 0. In this rounding mode, you must call
   mpfr_underflow with rnd_mode = MPFR_RNDZ if the exact result
   is <= 2^(emin-2) in absolute value.
   We chose the default to round away from zero instead of toward zero
   because rounding away from zero (MPFR_RNDA) wasn't supported at that
   time (r1910), so that the caller had no way to change rnd_mode to
   this mode. */

MPFR_COLD_FUNCTION_ATTR int
mpfr_underflow (mpfr_ptr x, mpfr_rnd_t rnd_mode, int sign)
{
  int inex;

  MPFR_LOG_FUNC
    (("rnd=%d sign=%d", rnd_mode, sign),
     ("x[%Pd]=%.*Rg", mpfr_get_prec (x), mpfr_log_prec, x));

  MPFR_ASSERT_SIGN (sign);

  if (MPFR_IS_LIKE_RNDZ(rnd_mode, sign < 0))
    {
      MPFR_SET_ZERO(x);
      inex = -1;
    }
  else
    {
      mpfr_setmin (x, __gmpfr_emin);
      inex = 1;
    }
  MPFR_SET_SIGN(x, sign);
  __gmpfr_flags |= MPFR_FLAGS_INEXACT | MPFR_FLAGS_UNDERFLOW;
  return sign > 0 ? inex : -inex;
}

/* #undef mpfr_overflow */

MPFR_COLD_FUNCTION_ATTR int
mpfr_overflow (mpfr_ptr x, mpfr_rnd_t rnd_mode, int sign)
{
  int inex;

  MPFR_LOG_FUNC
    (("rnd=%d sign=%d", rnd_mode, sign),
     ("x[%Pd]=%.*Rg", mpfr_get_prec (x), mpfr_log_prec, x));

  MPFR_ASSERT_SIGN (sign);

  if (MPFR_IS_LIKE_RNDZ(rnd_mode, sign < 0))
    {
      mpfr_setmax (x, __gmpfr_emax);
      inex = -1;
    }
  else
    {
      MPFR_SET_INF(x);
      inex = 1;
    }
  MPFR_SET_SIGN(x, sign);
  __gmpfr_flags |= MPFR_FLAGS_INEXACT | MPFR_FLAGS_OVERFLOW;
  return sign > 0 ? inex : -inex;
}

/**************************************************************************/

/* Code related to constructors and destructors (for debugging) should
   be put here. The reason is that such code must be in an object file
   that will be kept by the linker for symbol resolution, and symbols
   __gmpfr_emin and __gmpfr_emax from this file will be used by every
   program calling a MPFR math function (where rounding is involved). */

#if defined MPFR_DEBUG_PREDICTION

/* Print prediction statistics at the end of a program.
 *
 * Code to debug branch prediction, based on Ulrich Drepper's paper
 * "What Every Programmer Should Know About Memory":
 *   https://people.freebsd.org/~lstewart/articles/cpumemory.pdf
 */

extern long int __start_predict_data;
extern long int __stop_predict_data;
extern long int __start_predict_line;
extern const char *__start_predict_file;

static void __attribute__ ((destructor))
predprint (void)
{
  long int *s = &__start_predict_data;
  long int *e = &__stop_predict_data;
  long int *sl = &__start_predict_line;
  const char **sf = &__start_predict_file;

  while (s < e)
    {
      printf("%s:%ld: incorrect=%ld, correct=%ld%s\n",
             *sf, *sl, s[0], s[1],
             s[0] > s[1] ? " <==== WARNING" : "");
      ++sl;
      ++sf;
      s += 2;
    }
}

#endif

#if MPFR_WANT_ASSERT >= 2

/* Similar to flags_out in tests/tests.c */

void
flags_fout (FILE *stream, mpfr_flags_t flags)
{
  int none = 1;

  if (flags & MPFR_FLAGS_UNDERFLOW)
    none = 0, fprintf (stream, " underflow");
  if (flags & MPFR_FLAGS_OVERFLOW)
    none = 0, fprintf (stream, " overflow");
  if (flags & MPFR_FLAGS_NAN)
    none = 0, fprintf (stream, " nan");
  if (flags & MPFR_FLAGS_INEXACT)
    none = 0, fprintf (stream, " inexact");
  if (flags & MPFR_FLAGS_ERANGE)
    none = 0, fprintf (stream, " erange");
  if (none)
    fprintf (stream, " none");
  fprintf (stream, " (%u)\n", flags);
}

#endif
