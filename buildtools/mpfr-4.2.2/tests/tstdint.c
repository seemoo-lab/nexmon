/* Test file for multiple mpfr.h inclusion and intmax_t related functions

Copyright 2010-2025 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if HAVE_STDINT_H

#if _MPFR_EXP_FORMAT == 4
/* If mpfr_exp_t is defined as intmax_t, intmax_t must be defined before
   the inclusion of mpfr.h (this test doesn't use mpfr-impl.h). */
# include <stdint.h>
#endif

#ifdef MPFR_USE_MINI_GMP
#include "mpfr-test.h"
#endif

/* One of the goals of this test is to detect potential issues with the
 * following case in user code:
 *
 * #include <some_lib.h>
 * #include <stdint.h>
 * #define MPFR_USE_INTMAX_T
 * #include <mpfr.h>
 *
 * where some_lib.h has "#include <mpfr.h>". So, the mpfr.h header file
 * is included multiple times, a first time without <stdint.h> before,
 * and a second time with <stdint.h> support. We need to make sure that
 * the second inclusion is not a no-op due to some #include guard. This
 * was fixed in r7320.
 *
 * With mini-gmp, mpfr-impl.h is included first, but this should not
 * affect this test.
 *
 * Note: If _MPFR_EXP_FORMAT == 4 (which is never the case by default),
 * a part of the above check is not done because <stdint.h> is included
 * before the first mpfr.h inclusion (see above).
 *
 * Moreover, assuming that this test is run on a platform that has
 * <stdint.h> (most platforms do nowadays), without mini-gmp, this
 * test also allows one to detect that mpfr.h can be included without
 * any other inclusion before[*] (such as <stdio.h>). For instance,
 * it can detect any unprotected use of FILE in the mpfr.h header
 * file.
 * [*] possibly except config.h when used, which is normally not the
 *     case with a normal build. Anyway, if we decided to change that,
 *     this inclusion would not change anything as config.h would only
 *     have defines (such as HAVE_STDINT_H) currently provided as "-D"
 *     compiler arguments.
 */
#include <mpfr.h>

#include <stdint.h>

#define MPFR_USE_INTMAX_T
#include <mpfr.h>

#include "mpfr-test.h"

int
main (void)
{
  mpfr_t x;
  intmax_t j;

  tests_start_mpfr ();

  mpfr_init_set_ui (x, 1, MPFR_RNDN);
  j = mpfr_get_uj (x, MPFR_RNDN);
  mpfr_clear (x);
  if (j != 1)
    {
#ifndef NPRINTF_J
      printf ("Error: got %jd instead of 1.\n", j);
#else
      printf ("Error: did not get 1.\n");
#endif
      exit (1);
    }

  tests_end_mpfr ();
  return 0;
}

#else  /* HAVE_STDINT_H */

/* The test is disabled. */

int
main (void)
{
  return 77;
}

#endif  /* HAVE_STDINT_H */
