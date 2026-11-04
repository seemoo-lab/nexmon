/* mpfr_get_decimal128 -- convert a multiple precision floating-point number
                          to an IEEE 754-2008 decimal128 float

See https://gcc.gnu.org/legacy-ml/gcc/2006-06/msg00691.html,
https://gcc.gnu.org/onlinedocs/gcc/Decimal-Float.html,
and TR 24732 <https://www.open-std.org/jtc1/sc22/wg14/www/projects#24732>.

Copyright 2006-2025 Free Software Foundation, Inc.
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

/* Warning! Do not use any conversion between binary and decimal types,
 * otherwise GCC will generate from 2 to 3 MB of code (depending on the
 * GCC version) in the MPFR shared library when the _Decimal128 format
 * is BID (e.g. on x86).
 *   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96173
 *   https://gforge.inria.fr/tracker/index.php?func=detail&aid=21849&group_id=136&atid=619
 *
 * FIXME: Try to save even more space in the MPFR library by avoiding
 * _Decimal128 operations entirely. These operations now appear only in
 * string_to_Decimal128(). In the case where the _Decimal128 format is
 * recognized as BID, this function should be reimplemented directly by
 * using the specification of the encoding of this format, as already
 * done for _Decimal64 (see string_to_Decimal64 in get_d64.c).
 * Or use strtod128 when available, making sure that the string is
 * locale-independent? (Should one optionally use libdfp for that?)
 */

#include "mpfr-impl.h"
#include "ieee_floats.h"

#define ISDIGIT(c) ('0' <= c && c <= '9')

#ifdef MPFR_WANT_DECIMAL_FLOATS

#ifndef DEC128_MAX
# define DEC128_MAX 9.999999999999999999999999999999999E6144dl
#endif

/* construct a decimal128 NaN */
static _Decimal128
get_decimal128_nan (void)
{
  return 0.dl / 0.dl;
}

/* construct the decimal128 Inf with given sign */
static _Decimal128
get_decimal128_inf (int negative)
{
  return negative ? - 1.dl / 0.dl : 1.dl / 0.dl;
}

/* construct the decimal128 zero with given sign */
static _Decimal128
get_decimal128_zero (int negative)
{
  return negative ? - 0.dl : 0.dl;
}

/* construct the decimal128 smallest non-zero with given sign:
   it is 10^emin * 10^(1-p). Since emax = 6144, emin = 1-emax = -6143,
   and p = 34, we get 10^(-6176) */
static _Decimal128
get_decimal128_min (int negative)
{
  return negative ? - 1E-6176dl : 1E-6176dl;
}

/* construct the decimal128 largest finite number with given sign */
static _Decimal128
get_decimal128_max (int negative)
{
  return negative ? - DEC128_MAX : DEC128_MAX;
}

/* one-to-one conversion:
   s is a decimal string representing a number x = m * 10^e which must be
   exactly representable in the decimal128 format, i.e.
   (a) the mantissa m has at most 34 decimal digits
   (b1) -6143 <= e <= 6144 with m integer multiple of 10^(-33), |m| < 10
   (b2) or -6176 <= e <= 6111 with m integer, |m| < 10^34.
   Assumes s is neither NaN nor +Inf nor -Inf.
   s = [-][0-9]+E[-][0-9]+

   The decimal128 format (cf table 3.6 of IEEE 754-2008) has the following
   parameters:
   * k = 128 (number of bits of storage)
   * p = 34 (precision in digits)
   * emax = 6144
   * bias = E-q = 6176
   * sign bit has 1 bit
   * w+5 = 17 bits (combination field width)
   * t = 110 bits (trailing significand width)
   We have k = 1 + 5 + w + t = 128.
*/
static _Decimal128
string_to_Decimal128 (char *s) /* portable version */
{
  long int exp = 0;
  char m[35];
  long n = 0; /* mantissa length */
  char *endptr[1];
  _Decimal128 x = 0;
  int sign = 0;

  /* read sign */
  if (*s == '-')
    {
      sign = 1;
      s ++;
    }
  /* read mantissa */
  while (ISDIGIT (*s))
    m[n++] = *s++;

  /* as constructed in mpfr_get_decimal128, s cannot have any '.' separator */

  /* we will consider an integer mantissa m*10^exp */
  MPFR_ASSERTN(n <= 34);
  /* s always has an exponent separator 'E' */
  MPFR_ASSERTN(*s == 'E');
  exp = strtol (s + 1, endptr, 10);
  MPFR_ASSERTN(**endptr == '\0');
  MPFR_ASSERTN(-6176 <= exp && exp <= (long) (6145 - n));
  while (n < 34)
    {
      m[n++] = '0';
      exp --;
    }
  /* now n=34 and -6176 <= exp <= 6111, cf (b2) */
  m[n] = '\0';

  /* the number to convert is m[] * 10^exp where the mantissa is a 34-digit
     integer */

  /* compute biased exponent */
  exp += 6176;

  MPFR_ASSERTN(exp >= -33);
  if (exp < 0)
    {
      int i;
      n = -exp;
      /* check the last n digits of the mantissa are zero */
      for (i = 1; i <= n; i++)
        MPFR_ASSERTN(m[34 - n] == '0');
      /* shift the first (34-n) digits to the right */
      for (i = 34 - n - 1; i >= 0; i--)
        m[i + n] = m[i];
      /* zero the first n digits */
      for (i = 0; i < n; i ++)
        m[i] = '0';
      exp = 0;
    }

  /* the number to convert is m[] * 10^(exp-6176) */
  exp -= 6176;

  for (n = 0; n < 34; n++)
    x = (_Decimal128) 10 * x + (_Decimal128) (m[n] - '0');

  /* multiply by 10^exp */
  if (exp > 0)
    {
      _Decimal128 ten = 10;
      _Decimal128 ten2 = ten * ten;
      _Decimal128 ten4 = ten2 * ten2;
      _Decimal128 ten8 = ten4 * ten4;
      _Decimal128 ten16 = ten8 * ten8;
      _Decimal128 ten32 = ten16 * ten16;
      _Decimal128 ten64 = ten32 * ten32;
      _Decimal128 ten128 = ten64 * ten64;
      _Decimal128 ten256 = ten128 * ten128;
      _Decimal128 ten512 = ten256 * ten256;
      _Decimal128 ten1024 = ten512 * ten512;
      _Decimal128 ten2048 = ten1024 * ten1024;
      _Decimal128 ten4096 = ten2048 * ten2048;

      if (exp >= 4096)
        {
          x *= ten4096;
          exp -= 4096;
        }
      if (exp >= 2048)
        {
          x *= ten2048;
          exp -= 2048;
        }
      if (exp >= 1024)
        {
          x *= ten1024;
          exp -= 1024;
        }
      if (exp >= 512)
        {
          x *= ten512;
          exp -= 512;
        }
      if (exp >= 256)
        {
          x *= ten256;
          exp -= 256;
        }
      if (exp >= 128)
        {
          x *= ten128;
          exp -= 128;
        }
      if (exp >= 64)
        {
          x *= ten64;
          exp -= 64;
        }
      if (exp >= 32)
        {
          x *= ten32;
          exp -= 32;
        }
      if (exp >= 16)
        {
          x *= ten16;
          exp -= 16;
        }
      if (exp >= 8)
        {
          x *= ten8;
          exp -= 8;
        }
      if (exp >= 4)
        {
          x *= ten4;
          exp -= 4;
        }
      if (exp >= 2)
        {
          x *= ten2;
          exp -= 2;
        }
      if (exp >= 1)
        {
          x *= ten;
          exp -= 1;
        }
    }
  else if (exp < 0)
    {
      _Decimal128 ten = 10;
      _Decimal128 ten2 = ten * ten;
      _Decimal128 ten4 = ten2 * ten2;
      _Decimal128 ten8 = ten4 * ten4;
      _Decimal128 ten16 = ten8 * ten8;
      _Decimal128 ten32 = ten16 * ten16;
      _Decimal128 ten64 = ten32 * ten32;
      _Decimal128 ten128 = ten64 * ten64;
      _Decimal128 ten256 = ten128 * ten128;
      _Decimal128 ten512 = ten256 * ten256;
      _Decimal128 ten1024 = ten512 * ten512;
      _Decimal128 ten2048 = ten1024 * ten1024;
      _Decimal128 ten4096 = ten2048 * ten2048;

      if (exp <= -4096)
        {
          x /= ten4096;
          exp += 4096;
        }
      if (exp <= -2048)
        {
          x /= ten2048;
          exp += 2048;
        }
      if (exp <= -1024)
        {
          x /= ten1024;
          exp += 1024;
        }
      if (exp <= -512)
        {
          x /= ten512;
          exp += 512;
        }
      if (exp <= -256)
        {
          x /= ten256;
          exp += 256;
        }
      if (exp <= -128)
        {
          x /= ten128;
          exp += 128;
        }
      if (exp <= -64)
        {
          x /= ten64;
          exp += 64;
        }
      if (exp <= -32)
        {
          x /= ten32;
          exp += 32;
        }
      if (exp <= -16)
        {
          x /= ten16;
          exp += 16;
        }
      if (exp <= -8)
        {
          x /= ten8;
          exp += 8;
        }
      if (exp <= -4)
        {
          x /= ten4;
          exp += 4;
        }
      if (exp <= -2)
        {
          x /= ten2;
          exp += 2;
        }
      if (exp <= -1)
        {
          x /= ten;
          exp += 1;
        }
    }

  if (sign)
    x = -x;

  return x;
}

_Decimal128
mpfr_get_decimal128 (mpfr_srcptr src, mpfr_rnd_t rnd_mode)
{
  int negative;
  mpfr_exp_t e;

  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (src)))
    {
      if (MPFR_IS_NAN (src))
        {
          /* we don't propagate the sign bit */
          return get_decimal128_nan ();
        }

      negative = MPFR_IS_NEG (src);

      if (MPFR_IS_INF (src))
        return get_decimal128_inf (negative);

      MPFR_ASSERTD (MPFR_IS_ZERO(src));
      return get_decimal128_zero (negative);
    }

  e = MPFR_GET_EXP (src);
  negative = MPFR_IS_NEG (src);

  MPFR_UPDATE2_RND_MODE (rnd_mode, MPFR_SIGN (src));

  /* now rnd_mode is RNDN, RNDF, RNDA or RNDZ */

  /* the smallest decimal128 number is 10^(-6176),
     with 2^(-20517) < 10^(-6176) < 2^(-20516) */
  if (MPFR_UNLIKELY (e < -20517)) /* src <= 2^(-20518) < 1/2*10^(-6176) */
    {
      if (rnd_mode != MPFR_RNDA)
        return get_decimal128_zero (negative);
      else /* RNDA: return the smallest non-zero number */
        return get_decimal128_min (negative);
    }
  /* the largest decimal128 number is just below 10^6145 < 2^20414 */
  else if (MPFR_UNLIKELY (e > 20414)) /* then src >= 2^20414 */
    {
      if (rnd_mode == MPFR_RNDZ)
        return get_decimal128_max (negative);
      else /* RNDN, RNDA, RNDF: round away */
        return get_decimal128_inf (negative);
    }
  else
    {
      /* we need to store the sign (1 character), the significand (at most 34
         characters), the exponent part (at most 6 characters for "E-6176"),
         and the terminating null character, thus we need at least 42
         characters in s. */
      char s[42];
      mpfr_get_str (s, &e, 10, 34, src, rnd_mode);
      /* the smallest normal number is 1.000...000E-6143,
         which corresponds to s=[0.]1000...000 and e=-6142 */
      if (e < -6142)
        {
          /* the smallest subnormal number is 0.000...001E-6143 = 1E-6176,
             which corresponds to s=[0.]1000...000 and e=-6175 */
          if (e < -6175)
            {
              if (rnd_mode == MPFR_RNDN && e == -6176)
                {
                  /* If 0.5E-6176 < |src| < 1E-6176 (smallest subnormal),
                     src should round to +/- 1E-6176 in MPFR_RNDN. */
                  mpfr_get_str (s, &e, 10, 1, src, MPFR_RNDA);
                  return e == -6176 && s[negative] <= '5' ?
                    get_decimal128_zero (negative) :
                    get_decimal128_min (negative);
                }
              if (rnd_mode == MPFR_RNDZ || rnd_mode == MPFR_RNDN)
                return get_decimal128_zero (negative);
              else /* RNDA or RNDF: return the smallest non-zero number */
                return get_decimal128_min (negative);
            }
          else
            {
              mpfr_exp_t e2;
              long digits = 34 - (-6142 - e);
              /* if e = -6175 then 34 - (-6142 - e) = 1 */
              mpfr_get_str (s, &e2, 10, digits, src, rnd_mode);
              /* Warning: we can have e2 = e + 1 here, when rounding to
                 nearest or away from zero. */
              s[negative + digits] = 'E';
              sprintf (s + negative + digits + 1, "%ld",
                       (long int)e2 - digits);
              return string_to_Decimal128 (s);
            }
        }
      /* the largest number is 9.999...999E+6144,
         which corresponds to s=[0.]9999...999 and e=6145 */
      else if (e > 6145)
        {
          if (rnd_mode == MPFR_RNDZ)
            return get_decimal128_max (negative);
          else /* RNDN, RNDA, RNDF: round away */
            return get_decimal128_inf (negative);
        }
      else /* -6142 <= e <= 6145 */
        {
          s[34 + negative] = 'E';
          sprintf (s + 35 + negative, "%ld", (long int) e - 34);
          return string_to_Decimal128 (s);
        }
    }
}

#endif /* MPFR_WANT_DECIMAL_FLOATS */
