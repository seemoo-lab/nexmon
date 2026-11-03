/* talloc -- test file concerning memory allocation

Copyright 2015-2025 Free Software Foundation, Inc.
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

#include "mpfr-test.h"

/* We enable the test only when MPFR_ALLOCA_MAX is not greater than the
   default value: we do not want to make the test fail if the user has
   redefined MPFR_ALLOCA_MAX to a very large value. */

/* Needed with --with-gmp-build */
#ifndef MPFR_ALLOCA_MAX
# define MPFR_ALLOCA_MAX 16384
#endif

/* ISO C99 is needed by MPFR_DECL_INIT. */
#if __MPFR_STDC (199901L) && MPFR_ALLOCA_MAX <= 16384

int
main (void)
{
  MPFR_DECL_INIT (x, 4 + MPFR_ALLOCA_MAX * CHAR_BIT);
  MPFR_DECL_INIT (y, 8 + MPFR_ALLOCA_MAX * CHAR_BIT);

  tests_start_mpfr ();

  /* We do not want to use mpfr_add1sp because if MPFR_WANT_ASSERT >= 2,
     mpfr_init is used. The following test is based on the fact that
     MPFR_TMP_LIMBS_ALLOC is used in mpfr_add1 before any other form
     of memory allocation. In r9454, this crashes because the memory
     allocation function used by this macro (under the condition that
     the size is > MPFR_ALLOCA_MAX) isn't defined yet. This bug comes
     from r8813.
     WARNING! Do not add MPFR calls before this test. Otherwise the
     mentioned problem may no longer be actually tested, making this
     test useless. For other allocation tests, it is better to use a
     different test file. */
  mpfr_set_ui (x, 1, MPFR_RNDN);
  mpfr_set_ui (y, 2, MPFR_RNDN);
  mpfr_add (x, x, y, MPFR_RNDN);

  tests_end_mpfr ();
  return 0;
}

#else

int
main (void)
{
  return 77;
}

#endif
