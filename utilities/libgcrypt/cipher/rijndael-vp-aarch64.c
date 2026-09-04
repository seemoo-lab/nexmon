/* AArch64 SIMD vector permutation AES for Libgcrypt
 * Copyright (C) 2014-2025 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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
#include "./cipher-internal.h"


#ifdef USE_VP_AARCH64


/**********************************************************************
  AT&T x86 asm to intrinsics conversion macros (ARM)
 **********************************************************************/

#include "simd-common-aarch64.h"
#include <arm_neon.h>

#define __m128i uint64x2_t

#define pand128(a, o)           (o = vandq_u64(o, a))
#define pandn128(a, o)          (o = vbicq_u64(a, o))
#define pxor128(a, o)           (o = veorq_u64(o, a))
#define paddq128(a, o)          (o = vaddq_u64(o, a))
#define paddd128(a, o)          (o = (__m128i)vaddq_u32((uint32x4_t)o, (uint32x4_t)a))
#define paddb128(a, o)          (o = (__m128i)vaddq_u8((uint8x16_t)o, (uint8x16_t)a))

#define psrld128(s, o)          (o = (__m128i)vshrq_n_u32((uint32x4_t)o, s))
#define psraq128(s, o)          (o = (__m128i)vshrq_n_s64((int64x2_t)o, s))
#define psrldq128(s, o)         ({ uint64x2_t __tmp = { 0, 0 }; \
				   o = (__m128i)vextq_u8((uint8x16_t)o, \
				                         (uint8x16_t)__tmp, (s) & 15);})
#define pslldq128(s, o)         ({ uint64x2_t __tmp = { 0, 0 }; \
                                   o = (__m128i)vextq_u8((uint8x16_t)__tmp, \
                                                         (uint8x16_t)o, (16 - (s)) & 15);})
#define psrl_byte_128(s, o)     (o = (__m128i)vshrq_n_u8((uint8x16_t)o, s))

#define pshufb128(m8, o)        (o = (__m128i)vqtbl1q_u8((uint8x16_t)o, (uint8x16_t)m8))
#define pshufd128(m32, a, o)    ({ static const __m128i __tmp1 = PSHUFD_MASK_TO_PSHUFB_MASK(m32); \
				   __m128i __tmp2; \
				   movdqa128(a, o); \
				   movdqa128_memld(&__tmp1, __tmp2); \
				   pshufb128(__tmp2, o); })
#define pshufd128_0x93(a, o)    (o = (__m128i)vextq_u8((uint8x16_t)a, (uint8x16_t)a, 12))
#define pshufd128_0xFF(a, o)    (o = (__m128i)vdupq_laneq_u32((uint32x4_t)a, 3))
#define pshufd128_0xFE(a, o)    pshufd128(0xFE, a, o)
#define pshufd128_0x4E(a, o)    (o = (__m128i)vextq_u8((uint8x16_t)a, (uint8x16_t)a, 8))

#define palignr128(s, a, o)     (o = (__m128i)vextq_u8((uint8x16_t)a, (uint8x16_t)o, s))

#define movdqa128(a, o)         (o = a)

#define movdqa128_memld(a, o)   (o = (__m128i)vld1q_u8((const uint8_t *)(a)))

#define pand128_amemld(m, o)    pand128((__m128i)vld1q_u8((const uint8_t *)(m)), o)
#define pxor128_amemld(m, o)    pxor128((__m128i)vld1q_u8((const uint8_t *)(m)), o)
#define paddq128_amemld(m, o)   paddq128((__m128i)vld1q_u8((const uint8_t *)(m)), o)
#define paddd128_amemld(m, o)   paddd128((__m128i)vld1q_u8((const uint8_t *)(m)), o)
#define pshufb128_amemld(m, o)  pshufb128((__m128i)vld1q_u8((const uint8_t *)(m)), o)

/* Following operations may have unaligned memory input */
#define movdqu128_memld(a, o)   (o = (__m128i)vld1q_u8((const uint8_t *)(a)))

/* Following operations may have unaligned memory output */
#define movdqu128_memst(a, o)   vst1q_u8((uint8_t *)(o), (uint8x16_t)a)


#ifdef HAVE_GCC_ATTRIBUTE_OPTIMIZE
# define FUNC_ATTR_OPT __attribute__((optimize("-O2")))
#else
# define FUNC_ATTR_OPT
#endif

#define SIMD128_OPT_ATTR FUNC_ATTR_OPT

#define FUNC_ENCRYPT _gcry_aes_vp_aarch64_encrypt
#define FUNC_DECRYPT _gcry_aes_vp_aarch64_decrypt
#define FUNC_CFB_ENC _gcry_aes_vp_aarch64_cfb_enc
#define FUNC_CFB_DEC _gcry_aes_vp_aarch64_cfb_dec
#define FUNC_CBC_ENC _gcry_aes_vp_aarch64_cbc_enc
#define FUNC_CBC_DEC _gcry_aes_vp_aarch64_cbc_dec
#define FUNC_CTR_ENC _gcry_aes_vp_aarch64_ctr_enc
#define FUNC_CTR32LE_ENC _gcry_aes_vp_aarch64_ctr32le_enc
#define FUNC_OCB_CRYPT _gcry_aes_vp_aarch64_ocb_crypt
#define FUNC_OCB_AUTH _gcry_aes_vp_aarch64_ocb_auth
#define FUNC_ECB_CRYPT _gcry_aes_vp_aarch64_ecb_crypt
#define FUNC_XTS_CRYPT _gcry_aes_vp_aarch64_xts_crypt
#define FUNC_SETKEY _gcry_aes_vp_aarch64_do_setkey
#define FUNC_PREPARE_DEC _gcry_aes_vp_aarch64_prepare_decryption

#include "rijndael-vp-simd128.h"

#endif /* USE_VP_AARCH64 */
