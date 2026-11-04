/* mpfr_round_nearest_away -- round to nearest away

Copyright 2012-2025 Free Software Foundation, Inc.
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

/* Note: this doesn't work for 2^(emin-2). Currently, the best that can be
   done is to extend the exponent range internally, and do not support the
   case emin = MPFR_EMIN_MIN from the caller. */

/* In order to work, we'll save the context within the mantissa of 'rop'.
   For that, we need to do some low level stuff like for init2/clear to create
   a mantissa of bigger size, which contains the extra context.
   To add a new field of type T in the context, add its type in
   mpfr_size_limb_extended_t (if it is not already present)
   and add a new value for the enum mpfr_index_extended_t before MANTISSA. */
typedef union {
  mp_size_t    si;
  mp_limb_t    li;
  mpfr_exp_t   ex;
  mpfr_prec_t  pr;
  mpfr_sign_t  sg;
  mpfr_flags_t fl;
  mpfr_limb_ptr pi;
} mpfr_size_limb_extended_t;
typedef enum {
  ALLOC_SIZE = 0,
  OLD_MANTISSA,
  OLD_EXPONENT,
  OLD_SIGN,
  OLD_PREC,
  OLD_FLAGS,
  OLD_EXP_MIN,
  OLD_EXP_MAX,
  MANTISSA
} mpfr_index_extended_t ;

#define MPFR_MALLOC_EXTENDED_SIZE(s) \
  (MANTISSA * sizeof(mpfr_size_limb_extended_t) + \
   MPFR_BYTES_PER_MP_LIMB * (size_t) (s))

/* This function is called before the applied function
   and prepares rop to give it one more bit of precision
   and to save its old value within it. */
void
mpfr_round_nearest_away_begin (mpfr_ptr rop)
{
  mpfr_t tmp;
  mp_size_t xsize;
  mpfr_size_limb_extended_t *ext;
  mpfr_prec_t p;
  int inexact;
  MPFR_SAVE_EXPO_DECL (expo);

  /* when emin is the smallest possible value, we cannot
     determine the correct round-to-nearest-away rounding for
     0.25*2^emin, which gets rounded to 0 with nearest-even,
     instead of 0.5^2^emin. */
  MPFR_ASSERTN(__gmpfr_emin > MPFR_EMIN_MIN);

  p = MPFR_PREC (rop) + 1;
  MPFR_SAVE_EXPO_MARK (expo);

  /* We can't use mpfr_init2 since we need to store an additional context
     within the mantissa. */
  MPFR_ASSERTN(p <= MPFR_PREC_MAX);
  /* Allocate the context within the needed mantissa. */
  xsize = MPFR_PREC2LIMBS (p);
  ext   = (mpfr_size_limb_extended_t *)
    mpfr_allocate_func (MPFR_MALLOC_EXTENDED_SIZE(xsize));

  /* Save the context first. */
  ext[ALLOC_SIZE].si   = xsize;
  ext[OLD_MANTISSA].pi = MPFR_MANT(rop);
  ext[OLD_EXPONENT].ex = MPFR_EXP(rop);
  ext[OLD_SIGN].sg     = MPFR_SIGN(rop);
  ext[OLD_PREC].pr     = MPFR_PREC(rop);
  ext[OLD_FLAGS].fl    = expo.saved_flags;
  ext[OLD_EXP_MIN].ex  = expo.saved_emin;
  ext[OLD_EXP_MAX].ex  = expo.saved_emax;

  /* Create tmp as a proper NAN. */
  MPFR_PREC(tmp) = p;                           /* Set prec */
  MPFR_SET_POS(tmp);                            /* Set a sign */
  MPFR_MANT(tmp) =  (mp_limb_t*)(ext+MANTISSA); /* Set Mantissa ptr */
  MPFR_SET_NAN(tmp);                            /* initializes to NaN */

  /* Note: This full initialization to NaN may be unnecessary because of
     the mpfr_set just below, but this is cleaner in case internals would
     change in the future (for instance, some fields of tmp could be read
     before being set, and reading an uninitialized value is UB here). */

  /* Copy rop into tmp now (rop may be used as input later). */
  MPFR_DBGRES (inexact = mpfr_set(tmp, rop, MPFR_RNDN));
  MPFR_ASSERTD(inexact == 0); /* Shall be exact since prec(tmp) > prec(rop) */

  /* Overwrite rop with tmp. */
  rop[0] = tmp[0];

  /* The new rop now contains a copy of the old rop, with one extra bit of
     precision while keeping the old rop "hidden" within its mantissa. */

  return;
}

/* This function is called after the applied function.
   rop is the one prepared in the function above,
   and contains the result of the applied function.
   This function restores rop like it was before the
   calls to mpfr_round_nearest_away_begin while
   copying it back the result of the applied function
   and performing additional roundings. */
int
mpfr_round_nearest_away_end (mpfr_ptr rop, int inex)
{
  mpfr_t    tmp;
  mp_size_t xsize;
  mpfr_size_limb_extended_t *ext;
  mpfr_prec_t n;
  MPFR_SAVE_EXPO_DECL (expo);

  /* Get back the hidden context.
     Note: The cast to void * prevents the compiler from emitting a warning
     (or error), such as:
       cast increases required alignment of target type
     with the -Wcast-align GCC option. Correct alignment is a consequence
     of the code that built rop.
  */
  ext = ((mpfr_size_limb_extended_t *) (void *) MPFR_MANT(rop)) - MANTISSA;

  /* Create tmp with the result of the function. */
  tmp[0] = rop[0];

  /* Revert rop back to what it was before calling
     mpfr_round_neareset_away_begin. */
  MPFR_PREC(rop) = ext[OLD_PREC].pr;
  MPFR_SIGN(rop) = ext[OLD_SIGN].sg;
  MPFR_EXP(rop)  = ext[OLD_EXPONENT].ex;
  MPFR_MANT(rop) = ext[OLD_MANTISSA].pi;
  MPFR_ASSERTD(MPFR_PREC(tmp) == MPFR_PREC(rop)+1);

  /* Restore the saved context. */
  expo.saved_flags = ext[OLD_FLAGS].fl;
  expo.saved_emin  = ext[OLD_EXP_MIN].ex;
  expo.saved_emax  = ext[OLD_EXP_MAX].ex;
  xsize            = ext[ALLOC_SIZE].si;

  /* Perform RNDNA. */
  n = MPFR_PREC(rop);
  if (MPFR_UNLIKELY (MPFR_IS_SINGULAR (tmp)))
    mpfr_set (rop, tmp, MPFR_RNDN); /* inex unchanged */
  else
    {
      int lastbit, sh;

      MPFR_UNSIGNED_MINUS_MODULO(sh, n + 1);
      lastbit = (MPFR_MANT(tmp)[0] >> sh) & 1;

      if (lastbit == 0)
        mpfr_set (rop, tmp, MPFR_RNDN); /* exact, inex unchanged */
      else if (inex == 0)  /* midpoint: round away from zero */
        inex = mpfr_set (rop, tmp, MPFR_RNDA);
      else  /* lastbit == 1, inex != 0: double rounding */
        inex = mpfr_set (rop, tmp, (inex > 0) ? MPFR_RNDD : MPFR_RNDU);
    }

  MPFR_SAVE_EXPO_UPDATE_FLAGS (expo, __gmpfr_flags);
  MPFR_SAVE_EXPO_FREE (expo);

  /* special treatment for the case rop = +/- 2^(emin-2), which should be
     rounded to +/- 2^(emin-1). We do as if it was rounded to zero, thus the
     mpfr_check_range() call will round it to +/- 2^(emin-1). */
  if (inex == 0 && mpfr_cmp_si_2exp (rop, (mpfr_sgn (rop) > 0) ? 1 : -1,
                                     __gmpfr_emin - 2) == 0)
    inex = -mpfr_sgn (rop);

  /* Free tmp (cannot call mpfr_clear): free the associated context. */
  mpfr_free_func(ext, MPFR_MALLOC_EXTENDED_SIZE(xsize));

  return mpfr_check_range (rop, inex, MPFR_RNDN);
}
