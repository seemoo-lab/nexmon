/* tabort_defalloc1 -- Test for abort due to reaching out of memory

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

#define MPFR_NEED_LONGLONG_H
#include "mpfr-test.h"

/* Don't test with mini-gmp, which does not fully implement GMP
 * memory functions.
 *
 * Don't test if GCC's AddressSanitizer is used because it reports
 * the error before GMP can do the abort.
 */

#if !defined(__SANITIZE_ADDRESS__)

int
main (int argc, char **argv)
{
  /* Disable this test under Valgrind, which complains due to the
     large allocation size. */
  if (tests_run_within_valgrind ())
    return 77;

  tests_start_mpfr ();
  tests_expect_abort ();

  printf ("[tabort_defalloc1] Check for good handling of abort"
          " in memory function.\n");
  mpfr_allocate_func ((size_t) -1);

  tests_end_mpfr ();
  return -1; /* Should not be executed */
}

#else /* don't test with mini-gmp, which does not fully implement GMP
         memory functions */

int
main (void)
{
  return 77;
}

#endif
