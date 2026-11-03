/* Various Thresholds of MPFR, not exported.  -*- mode: C -*-

Copyright 2005-2025 Free Software Foundation, Inc.

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

#ifndef __MPFR_IMPL_H__
# error "MPFR Internal not included"
#endif

/* Threshold when testing coverage */
#if defined(MPFR_TUNE_COVERAGE)
#define MPFR_TUNE_CASE "src/generic/coverage/mparam.h"
#include "generic/coverage/mparam.h"

/* Note: the different macros used here are those defined by gcc,
   for example with "gcc -mtune=native -dM -E -xc /dev/null".

   The best parameters may depend on the local machine, not just
   the architecture or even the target processor as accepted by
   -mtune. Target-related options set by GCC can be obtained with
   the "-Q --help=target --help=params" options (in addition to
   -march=... and -mtune=...).
   See https://gcc.gnu.org/pipermail/gcc-help/2021-September/140720.html */

#elif (defined (__tune_core2__) || defined (__tune_nocona__)) && defined (__x86_64) /* 64-bit Core 2 or Xeon */
#define MPFR_TUNE_CASE "src/x86_64/core2/mparam.h"
#include "x86_64/core2/mparam.h"

/* Put that before __x86_64__ since __x86_64__ is also defined on AMD 64,
   We also have to define __tune_k8__ since __amd64__ is also defined on
   Intel x86_64! Ignore Clang as it defines both of these macros even on
   Intel x86_64, and it does not seem to be possible to provide tuning
   information when compiling with Clang, so that it is better to select
   generic parameters for x86_64. */
#elif defined (__amd64__) && (defined (__tune_k8__) || defined (__tune_znver1__)) && ! defined (__clang__) /* AMD 64 */
#define MPFR_TUNE_CASE "src/amd/mparam.h"
#include "amd/mparam.h"

/* _M_X64 is for the Microsoft compiler, see
   https://docs.microsoft.com/en-us/cpp/preprocessor/predefined-macros */
#elif defined (__x86_64__) || defined (_M_X64)
/* generic parameters for x86_64 */
#define MPFR_TUNE_CASE "src/x86_64/mparam.h"
#include "x86_64/mparam.h"

#elif defined (__i386) || defined(_M_IX86)
/* we consider all other 386's here,
   including a 64-bit machine with gmp/mpfr compiled with ABI=32 */
#define MPFR_TUNE_CASE "src/x86/mparam.h"
#include "x86/mparam.h"

#elif defined (__PPC64__) /* Threshold for 64-bit PowerPC */
#define MPFR_TUNE_CASE "src/powerpc64/mparam.h"
#include "powerpc64/mparam.h"

#elif defined (__sparc_v9__) /* Threshold for 64-bit Sparc */
#define MPFR_TUNE_CASE "src/sparc64/mparam.h"
#include "sparc64/mparam.h"

#elif defined (__mips__) /* MIPS */
#define MPFR_TUNE_CASE "src/mips/mparam.h"
#include "mips/mparam.h"

#elif defined (__arm__) || defined (_M_ARM) /* ARM */
#define MPFR_TUNE_CASE "src/arm/mparam.h"
#include "arm/mparam.h"

#else
#define MPFR_TUNE_CASE "default"
#endif

/****************************************************************
 * Default values of Threshold.                                 *
 * Must be included in any case: it checks, for every constant, *
 * if it has been defined, and it sets it to a default value if *
 * it was not previously defined.                               *
 ****************************************************************/
#include "generic/mparam.h"
