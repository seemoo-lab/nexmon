/* RISC-V vector permutation AES for Libgcrypt
 * Copyright (C) 2025 Jussi Kivilinna <jussi.kivilinna@iki.fi>
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 * The code is based on the public domain library libvpaes version 0.5
 * available at http://crypto.stanford.edu/vpaes/ and which carries
 * this notice:
 *
 *     libvpaes: constant-time SSSE3 AES encryption and decryption.
 *     version 0.5
 *
 *     By Mike Hamburg, Stanford University, 2009.  Public domain.
 *     I wrote essentially all of this code.  I did not write the test
 *     vectors; they are the NIST known answer tests.  I hereby release all
 *     the code and documentation here that I wrote into the public domain.
 *
 *     This is an implementation of AES following my paper,
 *       "Accelerating AES with Vector Permute Instructions"
 *       CHES 2009; http://shiftleft.org/papers/vector_aes/
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for memcmp() */

#include "types.h"  /* for byte and u32 typedefs */
#include "g10lib.h"
#include "cipher.h"
#include "bufhelp.h"
#include "rijndael-internal.h"
#include "cipher-internal.h"


#ifdef USE_VP_RISCV


/**********************************************************************
  AT&T x86 asm to intrinsics conversion macros (RISC-V)
 **********************************************************************/

#include <riscv_vector.h>
#include "simd-common-riscv.h"

/*
 * SIMD128
 */

typedef vuint8m1_t __m128i;

#define cast_m128i_to_s8(a)     (__riscv_vreinterpret_v_u8m1_i8m1(a))
#define cast_m128i_to_u32(a)    (__riscv_vreinterpret_v_u8m1_u32m1(a))
#define cast_m128i_to_u64(a)    (__riscv_vreinterpret_v_u8m1_u64m1(a))
#define cast_m128i_to_s64(a)    (__riscv_vreinterpret_v_u64m1_i64m1(cast_m128i_to_u64(a)))

#define cast_s8_to_m128i(a)     (__riscv_vreinterpret_v_i8m1_u8m1(a))
#define cast_u32_to_m128i(a)    (__riscv_vreinterpret_v_u32m1_u8m1(a))
#define cast_u64_to_m128i(a)    (__riscv_vreinterpret_v_u64m1_u8m1(a))
#define cast_s64_to_m128i(a)    (cast_u64_to_m128i(__riscv_vreinterpret_v_i64m1_u64m1(a)))

#define pand128(a, o)           (o = __riscv_vand_vv_u8m1((o), (a), 16))
#define pandn128(a, o)          (o = __riscv_vand_vv_u8m1(__riscv_vnot_v_u8m1((o), 16), (a), 16))
#define pxor128(a, o)           (o = __riscv_vxor_vv_u8m1((o), (a), 16))
#define paddb128(a, o)          (o = __riscv_vadd_vv_u8m1((o), (a), 16))
#define paddd128(a, o)          (o = cast_u32_to_m128i(__riscv_vadd_vv_u32m1( \
							cast_m128i_to_u32(o), \
							cast_m128i_to_u32(a), 4)))
#define paddq128(a, o)          (o = cast_u64_to_m128i(__riscv_vadd_vv_u64m1( \
							cast_m128i_to_u64(o), \
							cast_m128i_to_u64(a), 2)))

#define psrld128(s, o)          (o = cast_u32_to_m128i(__riscv_vsrl_vx_u32m1(cast_m128i_to_u32(o), (s), 4))
#define psraq128(s, o)          (o = cast_s64_to_m128i(__riscv_vsra_vx_i64m1(cast_m128i_to_s64(o), (s), 2)))
#define psrldq128(s, o)         (o = __riscv_vslidedown_vx_u8m1((o), (s), 16))
#define pslldq128(s, o)         ({ vuint8m1_t __tmp = __riscv_vmv_v_x_u8m1(0, 16); \
				   o = __riscv_vslideup_vx_u8m1(__tmp, (o), (s), 16); })
#define psrl_byte_128(s, o)     (o = __riscv_vsrl_vx_u8m1((o), (s), 16))

#define pshufb128(m8, o)        (o = __riscv_vrgather_vv_u8m1((o), (m8), 16))
#define pshufd128(m32, a, o)    ({ static const __m128i_const __tmp1 = PSHUFD_MASK_TO_PSHUFB_MASK(m32); \
				   __m128i __tmp2; \
				   movdqa128(a, o); \
				   movdqa128_memld(&__tmp1, __tmp2); \
				   pshufb128(__tmp2, o); })

#define pshufd128_0x93(a, o)    pshufd128(0x93, a, o)
#define pshufd128_0xFF(a, o)    (o = cast_u32_to_m128i(__riscv_vrgather_vx_u32m1(cast_m128i_to_u32(a), 3, 4)))
#define pshufd128_0xFE(a, o)    pshufd128(0xFE, a, o)
#define pshufd128_0x4E(a, o)    pshufd128(0x4E, a, o)

#define palignr128(s, a, o)     (o = __riscv_vslideup_vx_u8m1(__riscv_vslidedown_vx_u8m1((a), (s), 16), (o), 16 - (s), 16))

#define movdqa128(a, o)         (o = (a))

#define movdqa128_memld(a, o)   (o = __riscv_vle8_v_u8m1((const void *)(a), 16))

#define pand128_amemld(m, o)    pand128(__riscv_vle8_v_u8m1((const void *)(m), 16), (o))
#define pxor128_amemld(m, o)    pxor128(__riscv_vle8_v_u8m1((const void *)(m), 16), (o))
#define paddq128_amemld(m, o)   paddq128(__riscv_vle8_v_u8m1((const void *)(m), 16), (o))
#define paddd128_amemld(m, o)   paddd128(__riscv_vle8_v_u8m1((const void *)(m), 16), (o))
#define pshufb128_amemld(m, o)  pshufb128(__riscv_vle8_v_u8m1((const void *)(m), 16), (o))

/* Following operations may have unaligned memory input */
#define movdqu128_memld(a, o)   (o = __riscv_vle8_v_u8m1((const void *)(a), 16))

/* Following operations may have unaligned memory output */
#define movdqu128_memst(a, o)   (__riscv_vse8_v_u8m1((void *)(o), (a), 16))

/*
 * SIMD256
 */

#define PSHUFD256_MASK_TO_PSHUFB256_MASK(m32) { \
	M128I_BYTE(((((m32) >> 0) & 0x03) * 4) + 0, \
		   ((((m32) >> 0) & 0x03) * 4) + 1, \
		   ((((m32) >> 0) & 0x03) * 4) + 2, \
		   ((((m32) >> 0) & 0x03) * 4) + 3, \
		   ((((m32) >> 2) & 0x03) * 4) + 0, \
		   ((((m32) >> 2) & 0x03) * 4) + 1, \
		   ((((m32) >> 2) & 0x03) * 4) + 2, \
		   ((((m32) >> 2) & 0x03) * 4) + 3, \
		   ((((m32) >> 4) & 0x03) * 4) + 0, \
		   ((((m32) >> 4) & 0x03) * 4) + 1, \
		   ((((m32) >> 4) & 0x03) * 4) + 2, \
		   ((((m32) >> 4) & 0x03) * 4) + 3, \
		   ((((m32) >> 6) & 0x03) * 4) + 0, \
		   ((((m32) >> 6) & 0x03) * 4) + 1, \
		   ((((m32) >> 6) & 0x03) * 4) + 2, \
		   ((((m32) >> 6) & 0x03) * 4) + 3), \
	M128I_BYTE(((((m32) >> 0) & 0x03) * 4) + 1 + 16, \
		   ((((m32) >> 0) & 0x03) * 4) + 1 + 16, \
		   ((((m32) >> 0) & 0x03) * 4) + 2 + 16, \
		   ((((m32) >> 0) & 0x03) * 4) + 3 + 16, \
		   ((((m32) >> 2) & 0x03) * 4) + 0 + 16, \
		   ((((m32) >> 2) & 0x03) * 4) + 1 + 16, \
		   ((((m32) >> 2) & 0x03) * 4) + 2 + 16, \
		   ((((m32) >> 2) & 0x03) * 4) + 3 + 16, \
		   ((((m32) >> 4) & 0x03) * 4) + 0 + 16, \
		   ((((m32) >> 4) & 0x03) * 4) + 1 + 16, \
		   ((((m32) >> 4) & 0x03) * 4) + 2 + 16, \
		   ((((m32) >> 4) & 0x03) * 4) + 3 + 16, \
		   ((((m32) >> 6) & 0x03) * 4) + 0 + 16, \
		   ((((m32) >> 6) & 0x03) * 4) + 1 + 16, \
		   ((((m32) >> 6) & 0x03) * 4) + 2 + 16, \
		   ((((m32) >> 6) & 0x03) * 4) + 3 + 16) }

typedef vuint8m1_t __m256i;

#define HAVE_SIMD256 1

#define check_simd256_support() (__riscv_vsetvl_e8m1(32) == 32)

#define cast_m256i_to_s8(a)     cast_m128i_to_s8(a)
#define cast_m256i_to_u32(a)    cast_m128i_to_u32(a)
#define cast_m256i_to_u64(a)    cast_m128i_to_u64(a)
#define cast_m256i_to_s64(a)    cast_m128i_to_s64(a)

#define cast_s8_to_m256i(a)     (__riscv_vreinterpret_v_i8m1_u8m1(a))
#define cast_u32_to_m256i(a)    (__riscv_vreinterpret_v_u32m1_u8m1(a))
#define cast_u64_to_m256i(a)    (__riscv_vreinterpret_v_u64m1_u8m1(a))
#define cast_s64_to_m256i(a)    (cast_u64_to_m128i(__riscv_vreinterpret_v_i64m1_u64m1(a)))

#define pand256(a, o)           (o = __riscv_vand_vv_u8m1((o), (a), 32))
#define pandn256(a, o)          (o = __riscv_vand_vv_u8m1(__riscv_vnot_v_u8m1((o), 32), (a), 32))
#define pxor256(a, o)           (o = __riscv_vxor_vv_u8m1((o), (a), 32))
#define paddb256(a, o)          (o = __riscv_vadd_vv_u8m1((o), (a), 32))
#define paddd256(a, o)          (o = cast_u32_to_m256i(__riscv_vadd_vv_u32m1( \
							cast_m256i_to_u32(o), \
							cast_m256i_to_u32(a), 8)))
#define paddq256(a, o)          (o = cast_u64_to_m256i(__riscv_vadd_vv_u64m1( \
							cast_m256i_to_u64(o), \
							cast_m256i_to_u64(a), 4)))

#define psrld256(s, o)          (o = cast_u32_to_m256i(__riscv_vsrl_vx_u32m1(cast_m256i_to_u32(o), (s), 8))
#define psraq256(s, o)          (o = cast_s64_to_m256i(__riscv_vsra_vx_i64m1(cast_m256i_to_s64(o), (s), 4)))
#define psrl_byte_256(s, o)     (o = __riscv_vsrl_vx_u8m1((o), (s), 32))

/* Note: these are not PSHUFB equavalent as full 256-bit vector is used as
 * 32 byte table. 256-bit PSHUFB on x86 handles 128-bit lanes separately as
 * 128-bit 16 byte tables. */

/* tab32 variant: indexes have values 0..31. Used when 'm8' is constant and
 * variable data is in 'o'. */
#define pshufb256_tab32(m8, o)  (o = __riscv_vrgather_vv_u8m1((o), (m8), 32))

/* tab16 variant: indexes have values 0..16 and only low 128-bit of 'o' is
 * used. Used when 'o' is constant and variable data is in 'm8'. */
#define pshufb256_tab16(m8, o)  (o = __riscv_vrgather_vv_u8m1((o), (m8), 32))

/* Load 16 byte mask for 'pshufb256_tab32' usage as if 256-bit PSHUFB was to be
 * used as on x86 (two separate 128-bit lanes). */
#define load_tab32_mask(m, o)   ({ __m128i __tmp_lo128; \
				   __m128i __tmp_hi128; \
				   movdqu128_memld(m, __tmp_lo128); \
				   __tmp_hi128 = __riscv_vadd_vx_u8m1(__tmp_lo128, 16, 16); \
				   o = __riscv_vslideup_vx_u8m1(__tmp_lo128, __tmp_hi128, 16, 32); })

#define broadcast128_256(a, o)  (o = __riscv_vslideup_vx_u8m1((a), (a), 16, 32))

/* Load 16 byte table for 'pshufb256_tab16' usage. On x86 this would splat
 * 128-bit table from memory to both 128-bit lanes of 256-bit register.
 * On RISC-V this just loads memory to lower 128-bits. */
#define load_tab16_table(m, o)  movdqu128_memld(m, o)

#define pshufd256(m32, a, o)    ({ static const __m128i_const __tmp1 = PSHUFD_MASK_TO_PSHUFB_MASK(m32); \
				   __m256i __tmp2; \
				   movdqa256(a, o); \
				   load_tab32_mask(&__tmp1, __tmp2); \
				   pshufb256_tab32(__tmp2, o); })

#define pshufd256_0x93(a, o)    pshufd256(0x93, a, o)

#define insert256_hi128(x, o)   (o = __riscv_vslideup_vx_u8m1((o), (x), 16, 32))
#define extract256_hi128(y, o)  (o = __riscv_vslidedown_vx_u8m1((y), 16, 32))

#define movdqa256(a, o)         (o = (a))

#define movdqa128_256(a, o)     (o = __riscv_vslideup_vx_u8m1((a), __riscv_vmv_v_x_u8m1(0, 16), 16, 32))
#define movdqa256_128(a, o)     (o = (a))

#define movdqa256_memld(a, o)   (o = __riscv_vle8_v_u8m1((const void *)(a), 32))

#define pand256_amemld(m, o)    pand128(__riscv_vle8_v_u8m1((const void *)(m), 32), (o))
#define pxor256_amemld(m, o)    pxor128(__riscv_vle8_v_u8m1((const void *)(m), 32), (o))
#define paddq256_amemld(m, o)   paddq128(__riscv_vle8_v_u8m1((const void *)(m), 32), (o))
#define paddd256_amemld(m, o)   paddd128(__riscv_vle8_v_u8m1((const void *)(m), 32), (o))
#define pshufb256_amemld(m, o)  pshufb128(__riscv_vle8_v_u8m1((const void *)(m), 32), (o))
#define broadcast128_256_amemld(m, o) \
				broadcast128_256(__riscv_vle8_v_u8m1((const void *)(m), 32), (o))

/* Following operations may have unaligned memory input */
#define movdqu256_memld(a, o)   (o = __riscv_vle8_v_u8m1((const void *)(a), 32))

/* Following operations may have unaligned memory output */
#define movdqu256_memst(a, o)   (__riscv_vse8_v_u8m1((void *)(o), (a), 32))


#ifdef HAVE_GCC_ATTRIBUTE_OPTIMIZE
# define FUNC_ATTR_OPT __attribute__((optimize("-O2")))
#else
# define FUNC_ATTR_OPT
#endif

#define SIMD128_OPT_ATTR FUNC_ATTR_OPT

#define FUNC_ENCRYPT _gcry_aes_vp_riscv_encrypt
#define FUNC_DECRYPT _gcry_aes_vp_riscv_decrypt
#define FUNC_CFB_ENC _gcry_aes_vp_riscv_cfb_enc
#define FUNC_CFB_DEC _gcry_aes_vp_riscv_cfb_dec
#define FUNC_CBC_ENC _gcry_aes_vp_riscv_cbc_enc
#define FUNC_CBC_DEC _gcry_aes_vp_riscv_cbc_dec
#define FUNC_CTR_ENC _gcry_aes_vp_riscv_ctr_enc
#define FUNC_CTR32LE_ENC _gcry_aes_vp_riscv_ctr32le_enc
#define FUNC_OCB_CRYPT _gcry_aes_vp_riscv_ocb_crypt
#define FUNC_OCB_AUTH _gcry_aes_vp_riscv_ocb_auth
#define FUNC_ECB_CRYPT _gcry_aes_vp_riscv_ecb_crypt
#define FUNC_XTS_CRYPT _gcry_aes_vp_riscv_xts_crypt
#define FUNC_SETKEY _gcry_aes_vp_riscv_do_setkey
#define FUNC_PREPARE_DEC _gcry_aes_vp_riscv_prepare_decryption

#include "rijndael-vp-simd128.h"

int
_gcry_aes_vp_riscv_setup_acceleration(RIJNDAEL_context *ctx)
{
  (void)ctx;
  return (__riscv_vsetvl_e8m1(16) == 16);
}

#endif /* USE_VP_RISCV */
