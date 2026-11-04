/* Utilities for MPFR developers, not exported.

Copyright 1999-2025 Free Software Foundation, Inc.
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

#ifndef __MPFR_CVERS_H__
#define __MPFR_CVERS_H__

/* Macros to detect STDC, ICC, GCC, and GLIBC versions */
#if defined(__STDC_VERSION__)
# define __MPFR_STDC(version) (__STDC_VERSION__ >= (version))
#else
# define __MPFR_STDC(version) 0
#endif

#if defined(_WIN32)
/* Under MS Windows (e.g. with VS2008 or VS2010), Intel's compiler doesn't
   support/enable extensions like the ones seen under GNU/Linux.
   https://sympa.inria.fr/sympa/arc/mpfr/2011-02/msg00032.html */
# define __MPFR_ICC(a,b,c) 0
#elif defined(__ICC)
# define __MPFR_ICC(a,b,c) (__ICC >= (a)*100+(b)*10+(c))
#elif defined(__INTEL_COMPILER)
# define __MPFR_ICC(a,b,c) (__INTEL_COMPILER >= (a)*100+(b)*10+(c))
#else
# define __MPFR_ICC(a,b,c) 0
#endif

#define __MPFR_MAJMIN(a,i) (((a) << 8) | (i))

/* Instead of __MPFR_MAJMIN below, we could use code like
     (__MAJ__ + (__MIN__ >= (i)) > (a))
   as in the patch at
   https://sympa.inria.fr/sympa/arc/mpfr/2022-12/msg00007.html
*/

#if defined(__GNUC__) && defined(__GNUC_MINOR__) && ! __MPFR_ICC(0,0,0)
# define __MPFR_GNUC(a,i) \
 (__MPFR_MAJMIN(__GNUC__,__GNUC_MINOR__) >= __MPFR_MAJMIN(a,i))
#else
# define __MPFR_GNUC(a,i) 0
#endif

#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
# define __MPFR_GLIBC(a,i) \
 (__MPFR_MAJMIN(__GLIBC__,__GLIBC_MINOR__) >= __MPFR_MAJMIN(a,i))
#else
# define __MPFR_GLIBC(a,i) 0
#endif

#endif
