/* MPFR internal header related to intmax_t.

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

/* If you need something defined here, do not include this header file
   directly, but define the MPFR_NEED_INTMAX_H macro before including
   "mpfr-impl.h" (or "mpfr-test.h" in the tests). This will ensure that
   this header file "mpfr-intmax.h" is included in a consistent way,
   thus avoiding the various cases that could otherwise be obtained on
   different platforms and compilation options. Note that in particular,
   it needs <limits.h> (always included first in "mpfr-impl.h") if
   <inttypes.h> or <stdint.h> does not exist or does not work. It also
   needs "config.h" if used (HAVE_CONFIG_H defined), but "config.h" is
   also included first in "mpfr-impl.h". */

#ifndef MPFR_NEED_INTMAX_H
# error "Never include mpfr-intmax.h directly; define MPFR_NEED_INTMAX_H instead."
#endif

#ifndef __MPFR_INTMAX_H__
#define __MPFR_INTMAX_H__

/* The ISO C99 standard specifies that in C++ implementations the
 * INTMAX_MAX, ... macros should only be defined if explicitly requested.
 */
#if defined __cplusplus
# define __STDC_LIMIT_MACROS
# define __STDC_CONSTANT_MACROS
#endif

/* The definition of MPFR_USE_INTMAX_T is needed on systems for which
 * the current (non-standard) macro tests in mpfr.h is not sufficient.
 * This will force the support of intmax_t/uintmax_t if <inttypes.h>
 * and/or <stdint.h> are available. This also avoids a failure in the
 * tests (replace the macro tests in mpfr.h by just
 *   #if defined (MPFR_USE_INTMAX_T)
 * to simulate such a system and reproduce the problem).
 * Note: if this makes the build fail on some systems (because these
 * headers are broken), we will need a configure test to undefine
 * HAVE_INTTYPES_H and HAVE_STDINT_H in such a case.
 */

#if HAVE_INTTYPES_H
# include <inttypes.h>
# define MPFR_USE_INTMAX_T
#endif

#if HAVE_STDINT_H
# include <stdint.h>
# define MPFR_USE_INTMAX_T
#endif

/* Largest integer type available and fully working for the MPFR build
   (may be smaller than intmax_t / uintmax_t if NPRINTF_J is defined). */
#if defined(MPFR_USE_INTMAX_T) && !defined(NPRINTF_J)
typedef uintmax_t mpfr_uintmax_t;
typedef intmax_t mpfr_intmax_t;
# define MPFR_UINTMAX_MAX UINTMAX_MAX
# define MPFR_INTMAX_MAX INTMAX_MAX
# define MPFR_INTMAX_MIN INTMAX_MIN
# define MPFR_INTMAX_FSPEC "j"
#elif defined(HAVE_LONG_LONG) && !defined(NPRINTF_LL)
typedef unsigned long long mpfr_uintmax_t;
typedef long long mpfr_intmax_t;
#if defined(ULLONG_MAX)
/* standard */
# define MPFR_UINTMAX_MAX ULLONG_MAX
# define MPFR_INTMAX_MAX LLONG_MAX
# define MPFR_INTMAX_MIN LLONG_MIN
#elif defined(ULONGLONG_MAX)
/* Silicon Graphics IRIX 6.5 with native /usr/bin/cc */
# define MPFR_UINTMAX_MAX ULONGLONG_MAX
# define MPFR_INTMAX_MAX LONGLONG_MAX
# define MPFR_INTMAX_MIN LONGLONG_MIN
#else
/* We do not know any platform in this case (but this case would be a bug).
   Let's give an error. A workaround should be possible, but it should be
   tested first. */
# error "Neither ULLONG_MAX nor ULONGLONG_MAX is defined."
#endif
# define MPFR_INTMAX_FSPEC "ll"
#else
typedef unsigned long mpfr_uintmax_t;
typedef long mpfr_intmax_t;
# define MPFR_UINTMAX_MAX ULONG_MAX
# define MPFR_INTMAX_MAX LONG_MAX
# define MPFR_INTMAX_MIN LONG_MIN
# define MPFR_INTMAX_FSPEC "l"
#endif

#endif
