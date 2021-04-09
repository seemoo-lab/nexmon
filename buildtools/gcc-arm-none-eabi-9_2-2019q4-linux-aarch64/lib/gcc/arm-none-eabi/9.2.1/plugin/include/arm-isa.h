/* -*- buffer-read-only: t -*-
   Generated automatically by parsecpu.awk from arm-cpus.in.
   Do not edit.

   Copyright (C) 2011-2019 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3,
   or (at your option) any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

enum isa_feature {
  isa_nobit = 0,
  isa_bit_fp16fml,
  isa_bit_cmse,
  isa_bit_quirk_armv6kz,
  isa_bit_dotprod,
  isa_bit_crc32,
  isa_bit_xscale,
  isa_bit_vfpv2,
  isa_bit_vfpv3,
  isa_bit_vfpv4,
  isa_bit_lpae,
  isa_bit_armv7em,
  isa_bit_fp16,
  isa_bit_adiv,
  isa_bit_fp_d32,
  isa_bit_be8,
  isa_bit_fp16conv,
  isa_bit_thumb2,
  isa_bit_crypto,
  isa_bit_mp,
  isa_bit_sec,
  isa_bit_sb,
  isa_bit_predres,
  isa_bit_armv4,
  isa_bit_quirk_cm3_ldrd,
  isa_bit_smallmul,
  isa_bit_armv5t,
  isa_bit_armv6,
  isa_bit_thumb,
  isa_bit_armv7,
  isa_bit_armv8,
  isa_bit_fp_dbl,
  isa_bit_armv5te,
  isa_bit_fpv5,
  isa_bit_iwmmxt2,
  isa_bit_notm,
  isa_bit_iwmmxt,
  isa_bit_armv8_1,
  isa_bit_armv8_2,
  isa_bit_armv8_3,
  isa_bit_tdiv,
  isa_bit_armv8_4,
  isa_bit_armv8_5,
  isa_bit_neon,
  isa_bit_quirk_no_volatile_ce,
  isa_bit_armv6k,
  isa_num_bits
};

#define ISA_ARMv8r \
  isa_bit_notm, \
  isa_bit_armv5te, \
  isa_bit_tdiv, \
  isa_bit_thumb, \
  isa_bit_thumb2, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_armv7, \
  isa_bit_armv8, \
  isa_bit_be8, \
  isa_bit_lpae, \
  isa_bit_sec, \
  isa_bit_armv5t, \
  isa_bit_adiv, \
  isa_bit_mp, \
  isa_bit_armv6k

#define ISA_ARMv6z \
  isa_bit_armv5te, \
  isa_bit_notm, \
  isa_bit_armv5t, \
  isa_bit_be8, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb

#define ISA_DOTPROD \
  isa_bit_dotprod, \
  isa_bit_neon, \
  isa_bit_fp_d32, \
  isa_bit_fp_dbl

#define ISA_ALL_CRYPTO \
  isa_bit_crypto

#define ISA_ALL_SIMD \
  isa_bit_dotprod, \
  isa_bit_fp16fml, \
  isa_bit_neon, \
  isa_bit_fp_d32, \
  isa_bit_crypto

#define ISA_ALL_QUIRKS \
  isa_bit_quirk_armv6kz, \
  isa_bit_xscale, \
  isa_bit_quirk_no_volatile_ce, \
  isa_bit_quirk_cm3_ldrd

#define ISA_CRYPTO \
  isa_bit_fp_dbl, \
  isa_bit_neon, \
  isa_bit_fp_d32, \
  isa_bit_crypto

#define ISA_ARMv8m_base \
  isa_bit_armv5t, \
  isa_bit_tdiv, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_armv8, \
  isa_bit_cmse, \
  isa_bit_be8, \
  isa_bit_armv5te

#define ISA_ARMv6zk \
  isa_bit_armv5te, \
  isa_bit_armv5t, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_be8, \
  isa_bit_armv6k, \
  isa_bit_notm

#define ISA_VFPv2 \
  isa_bit_vfpv2

#define ISA_VFPv3 \
  isa_bit_vfpv2, \
  isa_bit_vfpv3

#define ISA_VFPv4 \
  isa_bit_vfpv2, \
  isa_bit_vfpv3, \
  isa_bit_vfpv4, \
  isa_bit_fp16conv

#define ISA_FP_D32 \
  isa_bit_fp_d32, \
  isa_bit_fp_dbl

#define ISA_ARMv7ve \
  isa_bit_sec, \
  isa_bit_thumb, \
  isa_bit_tdiv, \
  isa_bit_notm, \
  isa_bit_mp, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_armv7, \
  isa_bit_armv5te, \
  isa_bit_thumb2, \
  isa_bit_armv5t, \
  isa_bit_lpae, \
  isa_bit_adiv, \
  isa_bit_be8, \
  isa_bit_armv6k

#define ISA_ARMv7a \
  isa_bit_armv5te, \
  isa_bit_thumb2, \
  isa_bit_notm, \
  isa_bit_armv5t, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb, \
  isa_bit_armv7, \
  isa_bit_be8, \
  isa_bit_armv6k

#define ISA_ALL_SIMD_INTERNAL \
  isa_bit_neon, \
  isa_bit_fp_d32, \
  isa_bit_crypto

#define ISA_ARMv8_1a \
  isa_bit_armv8_1, \
  isa_bit_lpae, \
  isa_bit_crc32, \
  isa_bit_adiv, \
  isa_bit_thumb2, \
  isa_bit_armv5t, \
  isa_bit_be8, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_armv7, \
  isa_bit_mp, \
  isa_bit_armv8, \
  isa_bit_sec, \
  isa_bit_notm, \
  isa_bit_armv6k, \
  isa_bit_tdiv, \
  isa_bit_armv5te

#define ISA_ARMv7em \
  isa_bit_armv5t, \
  isa_bit_armv7em, \
  isa_bit_be8, \
  isa_bit_tdiv, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_armv7, \
  isa_bit_armv5te, \
  isa_bit_thumb2

#define ISA_FP_DBL \
  isa_bit_fp_dbl

#define ISA_ARMv8_3a \
  isa_bit_sec, \
  isa_bit_crc32, \
  isa_bit_lpae, \
  isa_bit_adiv, \
  isa_bit_thumb2, \
  isa_bit_armv5t, \
  isa_bit_armv5te, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_armv7, \
  isa_bit_armv8, \
  isa_bit_armv8_1, \
  isa_bit_armv8_2, \
  isa_bit_armv8_3, \
  isa_bit_notm, \
  isa_bit_tdiv, \
  isa_bit_mp, \
  isa_bit_armv6k, \
  isa_bit_be8

#define ISA_ARMv5t \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_notm, \
  isa_bit_armv5t

#define ISA_ARMv7m \
  isa_bit_thumb2, \
  isa_bit_armv5t, \
  isa_bit_tdiv, \
  isa_bit_armv5te, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb, \
  isa_bit_be8, \
  isa_bit_armv7

#define ISA_ARMv4 \
  isa_bit_notm, \
  isa_bit_armv4

#define ISA_ARMv6kz \
  isa_bit_armv6k, \
  isa_bit_be8, \
  isa_bit_notm, \
  isa_bit_armv5t, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_quirk_armv6kz, \
  isa_bit_armv5te

#define ISA_ARMv6 \
  isa_bit_notm, \
  isa_bit_be8, \
  isa_bit_armv5te, \
  isa_bit_armv5t, \
  isa_bit_thumb, \
  isa_bit_armv4, \
  isa_bit_armv6

#define ISA_ALL_FP \
  isa_bit_fp16, \
  isa_bit_fp_dbl, \
  isa_bit_vfpv2, \
  isa_bit_vfpv3, \
  isa_bit_vfpv4, \
  isa_bit_fpv5, \
  isa_bit_fp16conv, \
  isa_bit_neon, \
  isa_bit_fp_d32, \
  isa_bit_crypto

#define ISA_ARMv7 \
  isa_bit_be8, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_armv7, \
  isa_bit_armv5te, \
  isa_bit_thumb2, \
  isa_bit_armv5t

#define ISA_ARMv8_5a \
  isa_bit_sb, \
  isa_bit_adiv, \
  isa_bit_armv5te, \
  isa_bit_thumb2, \
  isa_bit_crc32, \
  isa_bit_armv8_1, \
  isa_bit_armv5t, \
  isa_bit_armv8_2, \
  isa_bit_armv8_3, \
  isa_bit_armv8_4, \
  isa_bit_armv8_5, \
  isa_bit_thumb, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_be8, \
  isa_bit_armv7, \
  isa_bit_armv8, \
  isa_bit_sec, \
  isa_bit_notm, \
  isa_bit_tdiv, \
  isa_bit_armv6k, \
  isa_bit_mp, \
  isa_bit_predres, \
  isa_bit_lpae

#define ISA_ARMv7r \
  isa_bit_armv6k, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_armv7, \
  isa_bit_thumb2, \
  isa_bit_armv5t, \
  isa_bit_notm, \
  isa_bit_be8, \
  isa_bit_tdiv, \
  isa_bit_armv5te

#define ISA_ALL_FPU_INTERNAL \
  isa_bit_vfpv2, \
  isa_bit_vfpv3, \
  isa_bit_vfpv4, \
  isa_bit_neon, \
  isa_bit_fp_d32, \
  isa_bit_crypto, \
  isa_bit_fp_dbl, \
  isa_bit_fpv5, \
  isa_bit_fp16conv

#define ISA_FPv5 \
  isa_bit_fpv5, \
  isa_bit_fp16conv, \
  isa_bit_vfpv2, \
  isa_bit_vfpv3, \
  isa_bit_vfpv4

#define ISA_ARMv6t2 \
  isa_bit_notm, \
  isa_bit_thumb2, \
  isa_bit_armv5t, \
  isa_bit_be8, \
  isa_bit_armv5te, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb

#define ISA_ARMv8m_main \
  isa_bit_armv8, \
  isa_bit_be8, \
  isa_bit_tdiv, \
  isa_bit_armv5te, \
  isa_bit_cmse, \
  isa_bit_thumb2, \
  isa_bit_armv5t, \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_armv6, \
  isa_bit_armv7

#define ISA_NEON \
  isa_bit_neon, \
  isa_bit_fp_d32, \
  isa_bit_fp_dbl

#define ISA_ARMv5te \
  isa_bit_notm, \
  isa_bit_thumb, \
  isa_bit_armv4, \
  isa_bit_armv5t, \
  isa_bit_armv5te

#define ISA_FP_ARMv8 \
  isa_bit_vfpv2, \
  isa_bit_vfpv3, \
  isa_bit_vfpv4, \
  isa_bit_fpv5, \
  isa_bit_fp16conv, \
  isa_bit_fp_d32, \
  isa_bit_fp_dbl

#define ISA_ARMv8a \
  isa_bit_lpae, \
  isa_bit_thumb2, \
  isa_bit_adiv, \
  isa_bit_be8, \
  isa_bit_armv5t, \
  isa_bit_sec, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb, \
  isa_bit_armv7, \
  isa_bit_armv8, \
  isa_bit_armv5te, \
  isa_bit_notm, \
  isa_bit_armv6k, \
  isa_bit_tdiv, \
  isa_bit_mp

#define ISA_ARMv8_2a \
  isa_bit_crc32, \
  isa_bit_armv5te, \
  isa_bit_be8, \
  isa_bit_notm, \
  isa_bit_sec, \
  isa_bit_thumb2, \
  isa_bit_tdiv, \
  isa_bit_armv5t, \
  isa_bit_armv8_1, \
  isa_bit_armv8_2, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb, \
  isa_bit_armv7, \
  isa_bit_armv8, \
  isa_bit_mp, \
  isa_bit_lpae, \
  isa_bit_adiv, \
  isa_bit_armv6k

#define ISA_ARMv6j \
  isa_bit_armv5t, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb, \
  isa_bit_armv5te, \
  isa_bit_be8, \
  isa_bit_notm

#define ISA_ARMv6k \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_armv5t, \
  isa_bit_armv5te, \
  isa_bit_notm, \
  isa_bit_armv6k, \
  isa_bit_be8, \
  isa_bit_thumb

#define ISA_ARMv4t \
  isa_bit_armv4, \
  isa_bit_thumb, \
  isa_bit_notm

#define ISA_ARMv6m \
  isa_bit_be8, \
  isa_bit_armv5t, \
  isa_bit_armv5te, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb

#define ISA_ARMv5tej \
  isa_bit_notm, \
  isa_bit_armv5te, \
  isa_bit_armv5t, \
  isa_bit_armv4, \
  isa_bit_thumb

#define ISA_ARMv8_4a \
  isa_bit_armv8_1, \
  isa_bit_armv8_2, \
  isa_bit_armv8_3, \
  isa_bit_armv8_4, \
  isa_bit_thumb2, \
  isa_bit_tdiv, \
  isa_bit_crc32, \
  isa_bit_notm, \
  isa_bit_armv5t, \
  isa_bit_armv4, \
  isa_bit_armv6, \
  isa_bit_thumb, \
  isa_bit_armv7, \
  isa_bit_armv8, \
  isa_bit_lpae, \
  isa_bit_adiv, \
  isa_bit_be8, \
  isa_bit_mp, \
  isa_bit_armv6k, \
  isa_bit_armv5te, \
  isa_bit_sec

