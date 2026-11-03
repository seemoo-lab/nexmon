/* mpz_t pool

Copyright 2014-2025 Free Software Foundation, Inc.
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

#define MPFR_POOL_DONT_REDEFINE
#include "mpfr-impl.h"

#ifndef MPFR_POOL_MAX_SIZE
# define MPFR_POOL_MAX_SIZE 32 /* maximal size (in limbs) for each entry */
#endif

/* If the number of entries of the mpz_t pool is not zero */
#if MPFR_POOL_NENTRIES

/* Index in the stack table of mpz_t and stack table of mpz_t */
static MPFR_THREAD_ATTR int n_alloc = 0;
static MPFR_THREAD_ATTR __mpz_struct mpz_tab[MPFR_POOL_NENTRIES];

MPFR_HOT_FUNCTION_ATTR void
mpfr_mpz_init (mpz_ptr z)
{
  if (MPFR_LIKELY (n_alloc > 0))
    {
      /* Get a mpz_t from the MPFR stack of previously used mpz_t.
         It reduces memory pressure, and it allows to reuse
         a mpz_t that should be sufficiently big. */
      MPFR_ASSERTD (n_alloc <= numberof (mpz_tab));
      memcpy (z, &mpz_tab[--n_alloc], sizeof (mpz_t));
      SIZ(z) = 0;
    }
  else
    {
      /* Call the real GMP function */
      mpz_init (z);
    }
}

MPFR_HOT_FUNCTION_ATTR void
mpfr_mpz_init2 (mpz_ptr z, mp_bitcnt_t n)
{
  /* The condition on n is used below as the argument n will be ignored if
     the mpz_t is obtained from the MPFR stack of previously used mpz_t.
     Said otherwise, it z is expected to have a large size at the end, then
     it is better to allocate this size directly than to get a mpz_t of
     small size, with possibly several realloc's on it. But if n satisfies
     the condition and is larger than the stacked mpz_t, this may still
     yield useless realloc's. This is not ideal. We might consider to use
     mpz_init2 with the maximum size in mpfr_mpz_init to solve this issue. */
  if (MPFR_LIKELY (n_alloc > 0 && n <= MPFR_POOL_MAX_SIZE * GMP_NUMB_BITS))
    {
      /* Get a mpz_t from the MPFR stack of previously used mpz_t.
         It reduces memory pressure, and it allows to reuse
         a mpz_t that should be sufficiently big. */
      MPFR_ASSERTD (n_alloc <= numberof (mpz_tab));
      memcpy (z, &mpz_tab[--n_alloc], sizeof (mpz_t));
      SIZ(z) = 0;
    }
  else
    {
      /* Call the real GMP function */
      mpz_init2 (z, n);
    }
}


MPFR_HOT_FUNCTION_ATTR void
mpfr_mpz_clear (mpz_ptr z)
{
  /* We only put objects with at most MPFR_POOL_MAX_SIZE in the mpz_t pool,
     to avoid it takes too much memory (and anyway the speedup is mainly
     for small precision). */
  if (MPFR_LIKELY (n_alloc < numberof (mpz_tab) &&
                   ALLOC (z) <= MPFR_POOL_MAX_SIZE))
    {
      /* Push back the mpz_t inside the stack of the used mpz_t */
      MPFR_ASSERTD (n_alloc >= 0);
      memcpy (&mpz_tab[n_alloc++], z, sizeof (mpz_t));
    }
  else
    {
      /* Call the real GMP function */
      mpz_clear (z);
    }
}

#endif

void
mpfr_free_pool (void)
{
#if MPFR_POOL_NENTRIES
  int i;

  MPFR_ASSERTD (n_alloc >= 0 && n_alloc <= numberof (mpz_tab));
  for (i = 0; i < n_alloc; i++)
    mpz_clear (&mpz_tab[i]);
  n_alloc = 0;
#endif
}
