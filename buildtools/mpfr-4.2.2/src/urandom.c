/* mpfr_urandom (rop, state, rnd_mode) -- Generate a uniform pseudorandom
   real number between 0 and 1 (exclusive) and round it to the precision of rop
   according to the given rounding mode.

Copyright 2000-2004, 2006-2025 Free Software Foundation, Inc.
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


/* The mpfr_urandom() function is implemented in the following way,
   so that the exact number (the random value to be rounded) and the
   final status of the random generator do not depend on the current
   exponent range and on the rounding mode. However, they depend on
   the target precision: from the same state of the random generator,
   if the precision of the destination is changed, then the value may
   be completely different (and the state of the random generator is
   different too).
   1. One determines the exponent exp: 0 with probability 1/2, -1 with
      probability 1/4, -2 with probability 1/8, etc.
   2. One draws a 1-ulp interval ]a,b[ containing the exact result (the
      interval can be regarded as open since it has the same measure as
      the closed interval).
      One also draws the rounding bit. This is currently done with a
      separate call to mpfr_rand_raw(), but it should be better to draw
      the rounding bit as part of the significand; there is space for it
      since the MSB is always 1.
   3. Rounding is done. For the directed rounding modes, the rounded value
      is uniquely determined. For rounding to nearest, ]a,m[ and ]m,b[,
      where m = (a+b)/2, have the same measure, so that one gets a or b
      with equal probabilities.
*/

int
mpfr_urandom (mpfr_ptr rop, gmp_randstate_t rstate, mpfr_rnd_t rnd_mode)
{
  mpfr_limb_ptr rp;
  mpfr_prec_t nbits;
  mp_size_t nlimbs;
  mp_size_t n;
  mpfr_exp_t exp;
  mp_limb_t rbit;
  int cnt;
  int inex;
  MPFR_SAVE_EXPO_DECL (expo);

  /* We need to extend the exponent range in order to simplify
     the case where one rounds upward (we would not be able to
     use mpfr_nextabove() in the case emin = max). It could be
     partly reimplemented under a simpler form here, but it is
     better to make the code shorter and more readable. */
  MPFR_SAVE_EXPO_MARK (expo);

  rp = MPFR_MANT (rop);
  nbits = MPFR_PREC (rop);
  MPFR_SET_EXP (rop, 0);
  MPFR_SET_POS (rop);
  exp = 0;

  /* Step 1 (exponent). */
#define DRAW_BITS 8 /* we draw DRAW_BITS at a time */
  MPFR_STAT_STATIC_ASSERT (DRAW_BITS <= GMP_NUMB_BITS);
  do
    {
      /* generate DRAW_BITS in rp[0] */
      mpfr_rand_raw (rp, rstate, DRAW_BITS);
      if (MPFR_UNLIKELY (rp[0] == 0))
        cnt = DRAW_BITS;
      else
        {
          count_leading_zeros (cnt, rp[0]);
          cnt -= GMP_NUMB_BITS - DRAW_BITS;
        }
      /* Any value of exp < MPFR_EMIN_MIN - 1 are equivalent. So, we can
         avoid a theoretical integer overflow in the following way. */
      if (MPFR_LIKELY (exp >= MPFR_EMIN_MIN - 1))
        exp -= cnt;  /* no integer overflow */
    }
  while (cnt == DRAW_BITS);
  /* We do not want the random generator to depend on the ABI or on the
     exponent range. Therefore we do not use MPFR_EMIN_MIN or __gmpfr_emin
     in the stop condition. */

  /* Step 2 (significand): we need generate only nbits-1 bits, since the
     most significant bit is 1. */
  if (MPFR_UNLIKELY (nbits == 1))
    {
      rp[0] = MPFR_LIMB_HIGHBIT;
    }
  else
    {
      mpfr_rand_raw (rp, rstate, nbits - 1);
      nlimbs = MPFR_LIMB_SIZE (rop);
      n = nlimbs * GMP_NUMB_BITS - nbits;
      if (MPFR_LIKELY (n != 0)) /* this will put the low bits to zero */
        mpn_lshift (rp, rp, nlimbs, n);
      rp[nlimbs - 1] |= MPFR_LIMB_HIGHBIT;
    }

  /* Rounding bit */
  mpfr_rand_raw (&rbit, rstate, 1);
  MPFR_ASSERTD (rbit == 0 || rbit == 1);

  /* Step 3 (rounding). */
  if (rnd_mode == MPFR_RNDU || rnd_mode == MPFR_RNDA
      || (rnd_mode == MPFR_RNDN && rbit != 0))
    {
      mpfr_nextabove (rop);
      inex = +1;
    }
  else
    {
      inex = -1;
    }

  MPFR_EXP (rop) += exp; /* may be smaller than emin */
  MPFR_SAVE_EXPO_FREE (expo);
  return mpfr_check_range (rop, inex, rnd_mode);
}
