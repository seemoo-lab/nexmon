/* MPFR internal constant FP numbers

Copyright 2005-2025 Free Software Foundation, Inc.
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

static const mp_limb_t __gmpfr_limb1[1] = {MPFR_LIMB_HIGHBIT};
const mpfr_t __gmpfr_one = {{2, MPFR_SIGN_POS, 1, (mp_limb_t*)__gmpfr_limb1}};
const mpfr_t __gmpfr_two = {{2, MPFR_SIGN_POS, 2, (mp_limb_t*)__gmpfr_limb1}};
const mpfr_t __gmpfr_four ={{2, MPFR_SIGN_POS, 3, (mp_limb_t*)__gmpfr_limb1}};
const mpfr_t __gmpfr_mone ={{2, MPFR_SIGN_NEG, 1, (mp_limb_t*)__gmpfr_limb1}};

#if 0
#elif GMP_NUMB_BITS == 8
static const mp_limb_t mpfr_l2_dd__tab[] = { 0xab, 0x79, 0xcf, 0xd1, 0xf7, 0x17, 0x72, 0xb1 };
#elif GMP_NUMB_BITS == 16
static const mp_limb_t mpfr_l2_dd__tab[] = { 0x79ab, 0xd1cf, 0x17f7, 0xb172 };
#elif GMP_NUMB_BITS == 32
static const mp_limb_t mpfr_l2_dd__tab[] = { 0xd1cf79ab, 0xb17217f7};
#elif GMP_NUMB_BITS == 64
static const mp_limb_t mpfr_l2_dd__tab[] = { 0xb17217f7d1cf79ab };
#elif GMP_NUMB_BITS == 96
static const mp_limb_t mpfr_l2_dd__tab[] = { 0xb17217f7d1cf79ab00000000 };
#elif GMP_NUMB_BITS == 128
static const mp_limb_t mpfr_l2_dd__tab[] = { 0xb17217f7d1cf79ab0000000000000000 };
#elif GMP_NUMB_BITS == 256
static const mp_limb_t mpfr_l2_dd__tab[] = { 0xb17217f7d1cf79ab000000000000000000000000000000000000000000000000 };
#endif

const mpfr_t __gmpfr_const_log2_RNDD =
  {{64, MPFR_SIGN_POS, 0, (mp_limb_t*)mpfr_l2_dd__tab}};

#if 0
#elif GMP_NUMB_BITS == 8
static const mp_limb_t mpfr_l2_du__tab[] = { 0xac, 0x79, 0xcf, 0xd1, 0xf7, 0x17, 0x72, 0xb1 };
#elif GMP_NUMB_BITS == 16
static const mp_limb_t mpfr_l2_du__tab[] = { 0x79ac, 0xd1cf, 0x17f7, 0xb172 };
#elif GMP_NUMB_BITS == 32
static const mp_limb_t mpfr_l2_du__tab[] = { 0xd1cf79ac, 0xb17217f7};
#elif GMP_NUMB_BITS == 64
static const mp_limb_t mpfr_l2_du__tab[] = { 0xb17217f7d1cf79ac };
#elif GMP_NUMB_BITS == 96
static const mp_limb_t mpfr_l2_du__tab[] = { 0xb17217f7d1cf79ac00000000 };
#elif GMP_NUMB_BITS == 128
static const mp_limb_t mpfr_l2_du__tab[] = { 0xb17217f7d1cf79ac0000000000000000 };
#elif GMP_NUMB_BITS == 256
static const mp_limb_t mpfr_l2_du__tab[] = { 0xb17217f7d1cf79ac000000000000000000000000000000000000000000000000 };
#endif

const mpfr_t __gmpfr_const_log2_RNDU =
  {{64, MPFR_SIGN_POS, 0, (mp_limb_t*)mpfr_l2_du__tab}};
