/* mpfr_free_cache... - Free cache/pool memory used by MPFR.

Copyright 2004-2025 Free Software Foundation, Inc.
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

/* These caches may be global to all threads or local to the current one. */
static void
mpfr_free_const_caches (void)
{
#ifndef MPFR_USE_LOGGING
  mpfr_clear_cache (__gmpfr_cache_const_pi);
  mpfr_clear_cache (__gmpfr_cache_const_log2);
#else
  mpfr_clear_cache (__gmpfr_normal_pi);
  mpfr_clear_cache (__gmpfr_normal_log2);
  mpfr_clear_cache (__gmpfr_logging_pi);
  mpfr_clear_cache (__gmpfr_logging_log2);
#endif
  mpfr_clear_cache (__gmpfr_cache_const_euler);
  mpfr_clear_cache (__gmpfr_cache_const_catalan);
}

/* These caches/pools are always local to a thread. */
static void
mpfr_free_local_cache (void)
{
  /* Before freeing the mpz_t pool, we need to free any cache of
     mpz_t numbers, since freeing such a cache may add entries to
     the mpz_t pool. */
  mpfr_bernoulli_freecache ();
  mpfr_free_pool ();
}

void
mpfr_free_cache (void)
{
  mpfr_free_local_cache ();
  mpfr_free_const_caches ();
}

void
mpfr_free_cache2 (mpfr_free_cache_t way)
{
  if ((unsigned int) way & MPFR_FREE_LOCAL_CACHE)
    {
      mpfr_free_local_cache ();
#if !defined(MPFR_WANT_SHARED_CACHE)
      mpfr_free_const_caches ();
#endif
    }
  if ((unsigned int) way & MPFR_FREE_GLOBAL_CACHE)
    {
#if defined(MPFR_WANT_SHARED_CACHE)
      mpfr_free_const_caches ();
#endif
    }
}

/* Function an application should call before mp_set_memory_functions().
   This is currently equivalent to freeing the caches (and pools) since
   they are allocated with GMP's current allocator. But this might change
   in the future to avoid the drawback of having to free the caches just
   because the allocators are changed: the caches could optionally be
   allocated with malloc().
   This function returns 0 in case of success, non-zero in case of error.
   Errors are currently not possible. But let's avoid a prototype change
   in the future, in case errors would be possible. */
int
mpfr_mp_memory_cleanup (void)
{
  mpfr_free_cache ();
  return 0;
}
