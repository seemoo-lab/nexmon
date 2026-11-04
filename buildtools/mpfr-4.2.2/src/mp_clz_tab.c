/* __clz_tab -- support for longlong.h

   THE CONTENTS OF THIS FILE ARE FOR INTERNAL USE AND MAY CHANGE
   INCOMPATIBLY OR DISAPPEAR IN A FUTURE GNU MPFR RELEASE.

Copyright 1991, 1993-1994, 1996-1997, 2000-2001, 2004, 2006-2025 Free Software Foundation, Inc.

This file is part of the GNU MPFR Library.
It has been copied and adapted from the GNU MP Library.

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
#include "mpfr-impl.h"

/* Warning: up from version 5.1.0, __clz_tab has 129 elements. */
#if defined(COUNT_LEADING_ZEROS_NEED_CLZ_TAB) && !defined(MPFR_HAVE_GMP_IMPL)
const
unsigned char __clz_tab[129] =
{
  1,2,3,3,4,4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  9
};
#endif
