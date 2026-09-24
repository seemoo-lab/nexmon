/* mpfr_urandom (rop, state, rnd_mode) -- Generate a uniform pseudorandom
   real number between 0 and 1 (exclusive) and round it to the precision of rop
   according to the given rounding mode.

Copyright 2000-2004, 2006-2016 Free Software Foundation, Inc.
Contributed by the AriC and Caramba projects, INRIA.

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
along with the GNU MPFR Library; see the file COPYING.LESSER.  If not, see
http://www.gnu.org/licenses/ or write to the Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA. */


#define MPFR_NEED_LONGLONG_H
#include "mpfr-impl.h"


/* generate one random bit */
static int
random_rounding_bit (gmp_randstate_t rstate)
{
  mp_limb_t r;

  mpfr_rand_raw (&r, rstate, 1);
  return r & MPFR_LIMB_ONE;
}


int
mpfr_urandom (mpfr_ptr rop, gmp_randstate_t rstate, mpfr_rnd_t rnd_mode)
{
  mpfr_limb_ptr rp;
  mpfr_prec_t nbits;
  mp_size_t nlimbs;
  mp_size_t n;
  mpfr_exp_t exp;
  mpfr_exp_t emin;
  int cnt;
  int inex;

  rp = MPFR_MANT (rop);
  nbits = MPFR_PREC (rop);
  nlimbs = MPFR_LIMB_SIZE (rop);
  MPFR_SET_POS (rop);
  exp = 0;
  emin = mpfr_get_emin ();
  if (MPFR_UNLIKELY (emin > 0))
    {
      if (rnd_mode == MPFR_RNDU || rnd_mode == MPFR_RNDA
          || (emin == 1 && rnd_mode == MPFR_RNDN
              && random_rounding_bit (rstate)))
        {
          mpfr_set_ui_2exp (rop, 1, emin - 1, rnd_mode);
          return +1;
        }
      else
        {
          MPFR_SET_ZERO (rop);
          return -1;
        }
    }

  /* Exponent */
#define DRAW_BITS 8 /* we draw DRAW_BITS at a time */
  cnt = DRAW_BITS;
  MPFR_ASSERTN(DRAW_BITS <= GMP_NUMB_BITS);
  while (cnt == DRAW_BITS)
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

      if (MPFR_UNLIKELY (exp < emin + cnt))
        {
          /* To get here, we have been drawing more than -emin zeros
             in a row, then return 0 or the smallest representable
             positive number.

             The rounding to nearest mode is subtle:
             If exp - cnt == emin - 1, the rounding bit is set, except
             if cnt == DRAW_BITS in which case the rounding bit is
             outside rp[0] and must be generated. */
          if (rnd_mode == MPFR_RNDU || rnd_mode == MPFR_RNDA
              || (rnd_mode == MPFR_RNDN && cnt == exp - emin - 1
                  && (cnt != DRAW_BITS || random_rounding_bit (rstate))))
            {
              mpfr_set_ui_2exp (rop, 1, emin - 1, rnd_mode);
              return +1;
            }
          else
            {
              MPFR_SET_ZERO (rop);
              return -1;
            }
        }
      exp -= cnt;
    }
  MPFR_EXP (rop) = exp; /* Warning: may be outside the current
                           exponent range */


  /* Significand: we need generate only nbits-1 bits, since the most
     significant is 1 */
  mpfr_rand_raw (rp, rstate, nbits - 1);
  n = nlimbs * GMP_NUMB_BITS - nbits;
  if (MPFR_LIKELY (n != 0)) /* this will put the low bits to zero */
    mpn_lshift (rp, rp, nlimbs, n);

  /* Set the msb to 1 since it was fixed by the exponent choice */
  rp[nlimbs - 1] |= MPFR_LIMB_HIGHBIT;

  /* Rounding */
  if (rnd_mode == MPFR_RNDU || rnd_mode == MPFR_RNDA
      || (rnd_mode == MPFR_RNDN && random_rounding_bit (rstate)))
    {
      /* Take care of the exponent range: it may have been reduced */
      if (exp < emin)
        mpfr_set_ui_2exp (rop, 1, emin - 1, rnd_mode);
      else if (exp > mpfr_get_emax ())
        mpfr_set_inf (rop, +1); /* overflow, flag set by mpfr_check_range */
      else
        mpfr_nextabove (rop);
      inex = +1;
    }
  else
    inex = -1;

  return mpfr_check_range (rop, inex, rnd_mode);
}
