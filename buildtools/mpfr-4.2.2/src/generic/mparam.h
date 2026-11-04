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

#ifndef MPFR_MULHIGH_TAB
# define MPFR_MULHIGH_TAB -1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,0,0,0
#endif

#ifndef MPFR_SQRHIGH_TAB
# define MPFR_SQRHIGH_TAB -1,-1,-1,-1,-1,-1,-1,-1,0,0,0,0,0,0,0,0,0
#endif

#ifndef MPFR_DIVHIGH_TAB
/* the value of divhigh_ktab[n] should be 0 or < n-1 */
# define MPFR_DIVHIGH_TAB 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
#endif

#ifndef MPFR_MUL_THRESHOLD
# define MPFR_MUL_THRESHOLD 20 /* limbs */
#endif

#ifndef MPFR_SQR_THRESHOLD
# define MPFR_SQR_THRESHOLD 20 /* limbs */
#endif

#ifndef MPFR_DIV_THRESHOLD
# define MPFR_DIV_THRESHOLD 25 /* limbs */
#endif

#ifndef MPFR_EXP_2_THRESHOLD
# define MPFR_EXP_2_THRESHOLD 100 /* bits */
#endif

#ifndef MPFR_EXP_THRESHOLD
# define MPFR_EXP_THRESHOLD 25000 /* bits */
#endif

#ifndef MPFR_SINCOS_THRESHOLD
# define MPFR_SINCOS_THRESHOLD 30000 /* bits */
#endif

#ifndef MPFR_AI_THRESHOLD1
# define MPFR_AI_THRESHOLD1 -13107 /* threshold for negative input of mpfr_ai */
#endif

#ifndef MPFR_AI_THRESHOLD2
# define MPFR_AI_THRESHOLD2 1311
#endif

#ifndef MPFR_AI_THRESHOLD3
# define MPFR_AI_THRESHOLD3 19661
#endif

