/* Declarations of random_deviate routines for mpfr_erandom and mpfr_nrandom.

Copyright 2013-2025 Free Software Foundation, Inc.
Contributed by Charles Karney <charles@karney.com>, SRI International.

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

#if !defined(MPFR_RANDOM_DEVIATE_H)
#define MPFR_RANDOM_DEVIATE_H 1

#include "mpfr-impl.h"

/* This should be an unsigned type with a width of at least 32 and capable of
 * representing at least 2*MPFR_PREC_MAX.  This is used to count the bits in
 * the fraction of a mpfr_random_deviate_t.  See the checks made on this type
 * in random_deviate_generate. */
typedef unsigned long mpfr_random_size_t;

typedef struct {
  mpfr_random_size_t e;       /* total number of bits in the fraction */
  unsigned long h;            /* the high W bits of the fraction */
  mpz_t f;                    /* the rest of the fraction */
} __mpfr_random_deviate_struct;

typedef __mpfr_random_deviate_struct mpfr_random_deviate_t[1];
typedef __mpfr_random_deviate_struct *mpfr_random_deviate_ptr;

#if defined(__cplusplus)
extern "C" {
#endif

/* allocate and set to (0,1) */
__MPFR_DECLSPEC void
  mpfr_random_deviate_init (mpfr_random_deviate_ptr);

/* reset to (0,1) */
__MPFR_DECLSPEC void
  mpfr_random_deviate_reset (mpfr_random_deviate_ptr);

/* deallocate */
__MPFR_DECLSPEC void
  mpfr_random_deviate_clear (mpfr_random_deviate_ptr);

/* swap two random deviates */
__MPFR_DECLSPEC void
  mpfr_random_deviate_swap (mpfr_random_deviate_ptr,
                            mpfr_random_deviate_ptr);

/* return kth bit of fraction, representing 2^-k */
__MPFR_DECLSPEC int
  mpfr_random_deviate_tstbit (mpfr_random_deviate_ptr,
                              mpfr_random_size_t,
                              gmp_randstate_t);

/* compare two random deviates, x < y */
__MPFR_DECLSPEC int
  mpfr_random_deviate_less (mpfr_random_deviate_ptr,
                            mpfr_random_deviate_ptr,
                            gmp_randstate_t);

/* set mpfr_t z = (neg ? -1 : 1) * (n + x) */
__MPFR_DECLSPEC int
  mpfr_random_deviate_value (int, unsigned long,
                             mpfr_random_deviate_ptr, mpfr_ptr,
                             gmp_randstate_t, mpfr_rnd_t);

#if defined(__cplusplus)
}
#endif

#endif
