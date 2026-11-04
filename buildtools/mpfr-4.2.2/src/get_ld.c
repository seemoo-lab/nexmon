/* mpfr_get_ld, mpfr_get_ld_2exp -- convert a multiple precision floating-point
                                    number to a machine long double

Copyright 2002-2025 Free Software Foundation, Inc.
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

#include <float.h> /* needed so that MPFR_LDBL_MANT_DIG is correctly defined */

#include "mpfr-impl.h"

#if defined(HAVE_LDOUBLE_IS_DOUBLE)

/* special code when "long double" is the same format as "double" */
long double
mpfr_get_ld (mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  return (long double) mpfr_get_d (x, rnd_mode);
}

#elif defined(HAVE_LDOUBLE_IEEE_EXT_LITTLE)

/* Note: The code will return a result with a 64-bit precision, even
   if the rounding precision is only 53 bits like on FreeBSD and
   NetBSD 6- (or with GCC's -mpc64 option to simulate this on other
   platforms). This is consistent with how strtold behaves in these
   cases, for instance. */

/* special code for IEEE 754 little-endian extended format */
long double
mpfr_get_ld (mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  mpfr_long_double_t ld;
  mpfr_t tmp;
  int inex;
  MPFR_SAVE_EXPO_DECL (expo);

  MPFR_SAVE_EXPO_MARK (expo);

  mpfr_init2 (tmp, MPFR_LDBL_MANT_DIG);
  inex = mpfr_set (tmp, x, rnd_mode);

  mpfr_set_emin (-16381-63); /* emin=-16444, see below */
  mpfr_set_emax (16384);
  mpfr_subnormalize (tmp, mpfr_check_range (tmp, inex, rnd_mode), rnd_mode);
  mpfr_prec_round (tmp, 64, MPFR_RNDZ); /* exact */
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (tmp)))
    ld.ld = (long double) mpfr_get_d (tmp, rnd_mode);
  else
    {
      mp_limb_t *tmpmant;
      mpfr_exp_t e, denorm;

      tmpmant = MPFR_MANT (tmp);
      e = MPFR_GET_EXP (tmp);
      /* The smallest positive normal number is 2^(-16382), which is
         0.5*2^(-16381) in MPFR, thus any exponent <= -16382 corresponds to a
         subnormal number. The smallest positive subnormal number is 2^(-16445)
         which is 0.5*2^(-16444) in MPFR thus 0 <= denorm <= 63. */
      denorm = MPFR_UNLIKELY (e <= -16382) ? - e - 16382 + 1 : 0;
      MPFR_ASSERTD (0 <= denorm && denorm < 64);
#if GMP_NUMB_BITS >= 64
      ld.s.manl = (tmpmant[0] >> denorm);
      ld.s.manh = (tmpmant[0] >> denorm) >> 32;
#elif GMP_NUMB_BITS == 32
      if (MPFR_LIKELY (denorm == 0))
        {
          ld.s.manl = tmpmant[0];
          ld.s.manh = tmpmant[1];
        }
      else if (denorm < 32)
        {
          ld.s.manl = (tmpmant[0] >> denorm) | (tmpmant[1] << (32 - denorm));
          ld.s.manh = tmpmant[1] >> denorm;
        }
      else /* 32 <= denorm < 64 */
        {
          ld.s.manl = tmpmant[1] >> (denorm - 32);
          ld.s.manh = 0;
        }
#elif GMP_NUMB_BITS == 16
      if (MPFR_LIKELY (denorm == 0))
        {
          /* manl = tmpmant[1] | tmpmant[0]
             manh = tmpmant[3] | tmpmant[2] */
          ld.s.manl = tmpmant[0] | ((unsigned long) tmpmant[1] << 16);
          ld.s.manh = tmpmant[2] | ((unsigned long) tmpmant[3] << 16);
        }
      else if (denorm < 16)
        {
          /* manl = low(mant[2],denorm) | mant[1] | high(mant[0],16-denorm)
             manh = mant[3] | high(mant[2],16-denorm) */
          ld.s.manl = (tmpmant[0] >> denorm)
            | ((unsigned long) tmpmant[1] << (16 - denorm))
            | ((unsigned long) tmpmant[2] << (32 - denorm));
          ld.s.manh = (tmpmant[2] >> denorm)
            | ((unsigned long) tmpmant[3] << (16 - denorm));
        }
      else if (denorm == 16)
        {
          /* manl = tmpmant[2] | tmpmant[1]
             manh = 0000000000 | tmpmant[3] */
          ld.s.manl = tmpmant[1] | ((unsigned long) tmpmant[2] << 16);
          ld.s.manh = tmpmant[3];
        }
      else if (denorm < 32)
        {
          /* manl = low(mant[3],denorm-16) | mant[2] | high(mant[1],32-denorm)
             manh = high(mant[3],32-denorm) */
          ld.s.manl = (tmpmant[1] >> (denorm - 16))
            | ((unsigned long) tmpmant[2] << (32 - denorm))
            | ((unsigned long) tmpmant[3] << (48 - denorm));
          ld.s.manh = tmpmant[3] >> (denorm - 16);
        }
      else if (denorm == 32)
        {
          /* manl = tmpmant[3] | tmpmant[2]
             manh = 0 */
          ld.s.manl = tmpmant[2] | ((unsigned long) tmpmant[3] << 16);
          ld.s.manh = 0;
        }
      else if (denorm < 48)
        {
          /* manl = zero(denorm-32) | tmpmant[3] | high(tmpmant[2],48-denorm)
             manh = 0 */
          ld.s.manl = (tmpmant[2] >> (denorm - 32))
            | ((unsigned long) tmpmant[3] << (48 - denorm));
          ld.s.manh = 0;
        }
      else /* 48 <= denorm < 64 */
        {
          /* we assume a right shift of 0 is identity */
          ld.s.manl = tmpmant[3] >> (denorm - 48);
          ld.s.manh = 0;
        }
#elif GMP_NUMB_BITS == 8
      {
        unsigned long long mant = 0;
        int i;
        for (i = 0; i < 8; i++)
          mant |= (unsigned long long) tmpmant[i] << (8*i);
        mant >>= denorm;
        ld.s.manl = mant;
        ld.s.manh = mant >> 32;
      }
#else
# error "GMP_NUMB_BITS must be 16, 32 or >= 64"
      /* Other values have never been supported anyway. */
#endif
      if (MPFR_LIKELY (denorm == 0))
        {
          ld.s.exph = (e + 0x3FFE) >> 8;
          ld.s.expl = (e + 0x3FFE);
        }
      else
        ld.s.exph = ld.s.expl = 0;
      ld.s.sign = MPFR_IS_NEG (x);
    }

  mpfr_clear (tmp);
  MPFR_SAVE_EXPO_FREE (expo);
  return ld.ld;
}

#else

/* generic code */
long double
mpfr_get_ld (mpfr_srcptr x, mpfr_rnd_t rnd_mode)
{
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (x)))
    return (long double) mpfr_get_d (x, rnd_mode);
  else /* now x is a normal non-zero number */
    {
      long double r; /* result */
      double s; /* part of result */
      MPFR_SAVE_EXPO_DECL (expo);

      MPFR_SAVE_EXPO_MARK (expo);

#if defined(HAVE_LDOUBLE_MAYBE_DOUBLE_DOUBLE)
      if (MPFR_LDBL_MANT_DIG == 106)
        {
          /* Assume double-double format (as found with the PowerPC ABI).
             The generic code below isn't used because numbers with
             precision > 106 would not be supported. */
          s = mpfr_get_d (x, MPFR_RNDN); /* high part of x */
          /* Let's first consider special cases separately. The test for
             infinity is really needed to avoid a NaN result. The test
             for NaN is mainly for optimization. The test for 0 is useful
             to get the correct sign (assuming mpfr_get_d supports signed
             zeros on the implementation). */
          if (s == 0 || DOUBLE_ISNAN (s) || DOUBLE_ISINF (s))
            {
              /* we don't propagate the sign bit of NaN */
              r = (long double) s;
            }
          else
            {
              mpfr_t y, z;

              mpfr_init2 (y, mpfr_get_prec (x));
              mpfr_init2 (z, IEEE_DBL_MANT_DIG); /* keep the precision small */
              mpfr_set_d (z, s, MPFR_RNDN);  /* exact */
              mpfr_sub (y, x, z, MPFR_RNDN); /* exact */
              /* Add the second part of y (in the correct rounding mode). */
              r = (long double) s + (long double) mpfr_get_d (y, rnd_mode);
              mpfr_clear (z);
              mpfr_clear (y);
            }
        }
      else
#endif
        {
          long double m;
          mpfr_exp_t sh; /* exponent shift -> x/2^sh is in the double range */
          mpfr_t y, z;
          int sign;

          /* First round x to the target long double precision, so that
             all subsequent operations are exact (this avoids double rounding
             problems). However, if the format contains numbers that have
             more precision, MPFR won't be able to generate such numbers. */
          mpfr_init2 (y, MPFR_LDBL_MANT_DIG);
          mpfr_init2 (z, MPFR_LDBL_MANT_DIG);
          /* Note about the precision of z: even though IEEE_DBL_MANT_DIG is
             sufficient, z has been set to the same precision as y so that
             the mpfr_sub below calls mpfr_sub1sp, which is faster than the
             generic subtraction, even in this particular case (from tests
             done by Patrick Pelissier on a 64-bit Core2 Duo against r7285).
             But here there is an important cancellation in the subtraction.
             TODO: get more information about what has been tested. */

          mpfr_set (y, x, rnd_mode);
          sh = MPFR_GET_EXP (y);
          sign = MPFR_SIGN (y);
          MPFR_SET_EXP (y, 0);
          MPFR_SET_POS (y);

          r = 0.0;
          do
            {
              s = mpfr_get_d (y, MPFR_RNDN); /* high part of y */
              r += (long double) s;
              mpfr_set_d (z, s, MPFR_RNDN);  /* exact */
              mpfr_sub (y, y, z, MPFR_RNDN); /* exact */
            }
          while (!MPFR_IS_ZERO (y));

          mpfr_clear (z);
          mpfr_clear (y);

          /* we now have to multiply back by 2^sh */
          MPFR_ASSERTD (r > 0);
          if (sh != 0)
            {
              /* An overflow may occur (example: 0.5*2^1024) */
              while (r < 1.0)
                {
                  r += r;
                  sh--;
                }

              if (sh > 0)
                m = 2.0;
              else
                {
                  m = 0.5;
                  sh = -sh;
                }

              for (;;)
                {
                  if (sh % 2)
                    r = r * m;
                  sh >>= 1;
                  if (sh == 0)
                    break;
                  m = m * m;
                }
            }
          if (sign < 0)
            r = -r;
        }
      MPFR_SAVE_EXPO_FREE (expo);
      return r;
    }
}

#endif

/* contributed by Damien Stehle */
long double
mpfr_get_ld_2exp (long *expptr, mpfr_srcptr src, mpfr_rnd_t rnd_mode)
{
  long double ret;
  mpfr_exp_t exp;
  mpfr_t tmp;

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (src)))
    return (long double) mpfr_get_d_2exp (expptr, src, rnd_mode);

  MPFR_ALIAS (tmp, src, MPFR_SIGN (src), 0);
  ret = mpfr_get_ld (tmp, rnd_mode);

  exp = MPFR_GET_EXP (src);

  /* rounding can give 1.0, adjust back to 0.5 <= abs(ret) < 1.0 */
  if (ret == 1.0)
    {
      ret = 0.5;
      exp ++;
    }
  else if (ret ==  -1.0)
    {
      ret = -0.5;
      exp ++;
    }

  MPFR_ASSERTN ((ret >= 0.5 && ret < 1.0)
                || (ret <= -0.5 && ret > -1.0));
  MPFR_ASSERTN (exp >= LONG_MIN && exp <= LONG_MAX);

  *expptr = exp;
  return ret;
}
