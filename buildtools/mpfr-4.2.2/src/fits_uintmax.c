/* mpfr_fits_uintmax_p -- test whether an mpfr fits an uintmax_t.

Copyright 2004, 2006-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_INTMAX_H
#include "mpfr-impl.h"

/* Note: though mpfr-impl.h is included in fits_u.h, we also include it
   above so that it gets included even when _MPFR_H_HAVE_INTMAX_T is not
   defined; this is necessary to avoid an empty translation unit, which
   is forbidden by ISO C. Without this, a failing test can be reproduced
   by creating an invalid stdint.h somewhere in the default include path
   and by compiling MPFR with "gcc -ansi -pedantic-errors". */

#ifdef _MPFR_H_HAVE_INTMAX_T

#define FUNCTION   mpfr_fits_uintmax_p
#define MAXIMUM    UINTMAX_MAX
#define TYPE       uintmax_t

#include "fits_u.h"

#endif
