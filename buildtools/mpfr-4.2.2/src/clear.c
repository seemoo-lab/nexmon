/* mpfr_clear -- free the memory space allocated for a floating-point number

Copyright 1999-2001, 2004-2025 Free Software Foundation, Inc.
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

MPFR_HOT_FUNCTION_ATTR void
mpfr_clear (mpfr_ptr m)
{
  mpfr_free_func (MPFR_GET_REAL_PTR (m),
                      MPFR_MALLOC_SIZE (MPFR_GET_ALLOC_SIZE (m)));
  MPFR_MANT (m) = (mp_limb_t *) 0;
}
