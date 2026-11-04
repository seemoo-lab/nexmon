/* mpfr_print_rnd_mode -- convert a given rounding mode to a string

Copyright 1999, 2001-2004, 2006-2025 Free Software Foundation, Inc.
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

/* WARNING! When adding a new rounding mode, do not forget to update the
   description of this mpfr_print_rnd_mode function in the manual. */

const char *
mpfr_print_rnd_mode (mpfr_rnd_t rnd_mode)
{
  /* If we forget to update this function after a new rounding mode
     is added, this will be detected by the following assertion. */
  MPFR_STAT_STATIC_ASSERT (MPFR_RND_MAX == MPFR_RNDF + 1);
  switch (rnd_mode)
    {
    case MPFR_RNDD:
      return "MPFR_RNDD";
    case MPFR_RNDU:
      return "MPFR_RNDU";
    case MPFR_RNDN:
      return "MPFR_RNDN";
    case MPFR_RNDZ:
      return "MPFR_RNDZ";
    case MPFR_RNDA:
      return "MPFR_RNDA";
    case MPFR_RNDF:
      return "MPFR_RNDF";
    default:
      return (const char*) 0;
    }
}
