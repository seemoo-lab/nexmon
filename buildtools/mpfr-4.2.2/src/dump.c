/* mpfr_dump, mpfr_fdump -- dump a float (for the tests and debugging purpose)

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

/* mpfr_dump is mainly for debugging purpose. It outputs a MPFR number
 * in some unspecified format, then a newline character. This function
 * is part of the API, but the output format may change without breaking
 * the ABI.
 *
 * Since this is for debugging, it can detect invalid data (or UBF), and
 * in such a case, it outputs information between exclamation marks:
 *   - 'N': the number is not normalized (MSB = 0);
 *   - 'T': there are non-zero trailing bits (output in square brackets);
 *   - 'U': this is an UBF number (internal use only);
 *   - '<': the exponent is < the current emin;
 *   - '>': the exponent is > the current emax.
 *
 * The format may depend on MPFR_* environment variables. For instance,
 * in the future, some environment variables could specify prefix and
 * suffix strings to colorize parts of the output that correspond to
 * invalid data.
 */

void
mpfr_fdump (FILE *stream, mpfr_srcptr x)
{
  if (MPFR_IS_NEG (x))
    fprintf (stream, "-");

  if (MPFR_IS_NAN (x))
    fprintf (stream, "@NaN@");
  else if (MPFR_IS_INF (x))
    fprintf (stream, "@Inf@");
  else if (MPFR_IS_ZERO (x))
    fprintf (stream, "0");
  else
    {
      mp_limb_t *mx;
      mpfr_prec_t px;
      mp_size_t n;
      char invalid[4];
      int first = 1, i = 0;

      mx = MPFR_MANT (x);
      px = MPFR_PREC (x);

      fprintf (stream, "0.");
      for (n = (px - 1) / GMP_NUMB_BITS; n >= 0; n--)
        {
          mp_limb_t wd, t;

          wd = mx[n];
          if (first)
            {
              if (! MPFR_LIMB_MSB (wd))
                invalid[i++] = 'N';
              first = 0;
            }
          for (t = MPFR_LIMB_HIGHBIT; t != 0; t >>= 1)
            {
              putc ((wd & t) == 0 ? '0' : '1', stream);
              if (--px == 0)
                {
                  MPFR_ASSERTD (n == 0);
                  if (t != 0 && (wd & (t - 1)) != 0)
                    {
                      putc ('[', stream);
                      invalid[i++] = 'T';
                    }
                  else
                    break; /* the trailing bits (all 0's) are not output */
                }
            }
        }
      if (px < 0)  /* there are non-zero trailing bits */
        putc (']', stream);

      if (MPFR_IS_UBF (x))
        {
#ifndef MPFR_USE_MINI_GMP
          gmp_fprintf (stream, "E%Zd", MPFR_ZEXP (x));
#else /* mini-gmp has no gmp_fprintf */
          fprintf (stream, "E");
          mpz_out_str (stream, 10, MPFR_ZEXP (x));
#endif
          invalid[i++] = 'U';
        }
      else
        {
          mpfr_exp_t e = MPFR_EXP (x);

          fprintf (stream, "E%" MPFR_EXP_FSPEC "d", (mpfr_eexp_t) e);
          if (e < __gmpfr_emin)
            invalid[i++] = '<';
          else if (e > __gmpfr_emax)
            invalid[i++] = '>';
        }

      if (i != 0)
        {
          invalid[i] = '\0';
          fprintf (stream, "!!!%s!!!", invalid);
        }
    }
  putc ('\n', stream);
}

void
mpfr_dump (mpfr_srcptr x)
{
  mpfr_fdump (stdout, x);
}
