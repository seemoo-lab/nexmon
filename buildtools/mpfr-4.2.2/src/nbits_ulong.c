/* mpfr_nbits_ulong -- number of significant bits in an unsigned long
   mpfr_nbits_uj    -- number of significant bits in an uintmax_t

Copyright 2018-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H  /* for count_leading_zeros */
#define MPFR_NEED_INTMAX_H
#include "mpfr-impl.h"

/* count the number of significant bits of n, i.e.,
   nbits(unsigned long) - count_leading_zeros (n) */
int
mpfr_nbits_ulong (unsigned long n)
{
  int cnt;

  MPFR_ASSERTD (n > 0);

#ifdef MPFR_LONG_WITHIN_LIMB

  count_leading_zeros (cnt, (mp_limb_t) n);
  cnt = GMP_NUMB_BITS - cnt;

#else

  cnt = 0;

  while (n >= 0x10000)
    {
      n >>= 16;
      cnt += 16;
    }

  MPFR_ASSERTD (n <= 0xffff);

  if (n >= 0x100)
    {
      n >>= 8;
      cnt += 8;
    }

  MPFR_ASSERTD (n <= 0xff);

  if (n >= 0x10)
    {
      n >>= 4;
      cnt += 4;
    }

  MPFR_ASSERTD (n <= 0xf);

  if (n >= 4)
    {
      n >>= 2;
      cnt += 2;
    }

  MPFR_ASSERTD (n <= 3);

  /* now n = 1, 2, or 3 */
  cnt += 1 + (n >= 2);

#endif

  MPFR_ASSERTD (cnt >= 0);
  return cnt;
}

#ifdef _MPFR_H_HAVE_INTMAX_T
/* count the number of significant bits of n, i.e.,
   nbits(uintmax_t) - count_leading_zeros (n) */
int
mpfr_nbits_uj (uintmax_t n)
{
  int cnt;

  MPFR_ASSERTD (n > 0);

  cnt = 0;

  while (n >= 0x10000)
    {
      n >>= 16;
      cnt += 16;
    }

  MPFR_ASSERTD (n <= 0xffff);

  if (n >= 0x100)
    {
      n >>= 8;
      cnt += 8;
    }

  MPFR_ASSERTD (n <= 0xff);

  if (n >= 0x10)
    {
      n >>= 4;
      cnt += 4;
    }

  MPFR_ASSERTD (n <= 0xf);

  if (n >= 4)
    {
      n >>= 2;
      cnt += 2;
    }

  MPFR_ASSERTD (n <= 3);

  /* now n = 1, 2, or 3 */
  cnt += 1 + (n >= 2);

  MPFR_ASSERTD (cnt >= 0);
  return cnt;
}
#endif
