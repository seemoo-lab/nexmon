/* mpfr_pow_sj -- power function x^y with y an intmax_t

Copyright 2021-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_LONGLONG_H
#define MPFR_NEED_INTMAX_H
#include "mpfr-impl.h"

#ifdef _MPFR_H_HAVE_INTMAX_T

#define POW_S mpfr_pow_sj
#define POW_U mpfr_pow_uj
#define SET_S mpfr_set_sj
#define SET_S_2EXP mpfr_set_sj_2exp
#define NBITS_UTYPE mpfr_nbits_uj
#define TYPE intmax_t
#define UTYPE uintmax_t
#define FSPEC "j"

#include "pow_si.c"

#endif
