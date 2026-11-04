/* sm4-intel-avx512-amd64.S  -  Intel SM4 instructions AVX512 SM4 cipher
 *
 * Copyright (C) 2026 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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
 */

#include <config.h>

#ifdef __x86_64
#if (defined(HAVE_COMPATIBLE_GCC_AMD64_PLATFORM_AS) || \
     defined(HAVE_COMPATIBLE_GCC_WIN64_PLATFORM_AS)) && \
    defined(ENABLE_AVX512_SUPPORT) && defined(HAVE_GCC_INLINE_ASM_SM4_AVX512)

#include "asm-common-amd64.h"

/* vector registers */
#define RA0          %ymm0
#define RA1          %ymm1
#define RA2          %ymm2
#define RA3          %ymm3
#define RA0x         %xmm0
#define RA1x         %xmm1
#define RA2x         %xmm2
#define RA3x         %xmm3
#define RA0z         %zmm0
#define RA1z         %zmm1
#define RA2z         %zmm2
#define RA3z         %zmm3

#define RTMP0        %ymm4
#define RTMP1        %ymm5
#define RTMP2        %ymm6
#define RTMP3        %ymm7
#define RTMP4        %ymm8
#define RTMP0x       %xmm4
#define RTMP1x       %xmm5
#define RTMP2x       %xmm6
#define RTMP3x       %xmm7
#define RTMP4x       %xmm8
#define RTMP0z       %zmm4
#define RTMP1z       %zmm5
#define RTMP2z       %zmm6
#define RTMP3z       %zmm7
#define RTMP4z       %zmm8

#define RB0          %ymm9
#define RB1          %ymm10
#define RB2          %ymm11
#define RB3          %ymm12
#define RB0x         %xmm9
#define RB1x         %xmm10
#define RB2x         %xmm11
#define RB3x         %xmm12
#define RB0z         %zmm9
#define RB1z         %zmm10
#define RB2z         %zmm11
#define RB3z         %zmm12

#define RX0          %ymm13
#define RX1          %ymm14
#define RX0x         %xmm13
#define RX1x         %xmm14
#define RX0z         %zmm13
#define RX1z         %zmm14

#define RNOT         %ymm15
#define RNOTx        %xmm15
#define RNOTz        %zmm15

SECTION_RODATA
.align 32

ELF(.type _sm4_intel_avx512_consts,@object)
_sm4_intel_avx512_consts:

/* For output word byte-swap */
.Lbswap128_mask:
	.byte 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

/* For input word byte-swap */
.Lbswap32_mask:
	.byte 3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12

.text

/**********************************************************************
  32-way SM4 with Intel SM4 instructions
 **********************************************************************/

.align 16
ELF(.type   __sm4_intel_crypt_blk32,@function;)
__sm4_intel_crypt_blk32:
	/* input:
	 *	%rdi: round key array, CTX
	 *	RA0z, RA1z, RA2z, RA3z, RB0z, RB1z, RB2z, RB3z: 32 parallel
	 *						plaintext blocks
	 * output:
	 *	RA0z, RA1z, RA2z, RA3z, RB0z, RB1z, RB2z, RB3z: 32 parallel
	 *						ciphertext blocks
	 */
	CFI_STARTPROC();

	vbroadcasti32x4 .Lbswap32_mask rRIP, RTMP0z;
	vpshufb RTMP0z, RA0z, RA0z;
	vpshufb RTMP0z, RA1z, RA1z;
	vpshufb RTMP0z, RA2z, RA2z;
	vpshufb RTMP0z, RA3z, RA3z;
	vpshufb RTMP0z, RB0z, RB0z;
	vpshufb RTMP0z, RB1z, RB1z;
	vpshufb RTMP0z, RB2z, RB2z;
	vpshufb RTMP0z, RB3z, RB3z;

#define ROUND4(rkoff) \
	vbroadcasti32x4 (rkoff)(%rdi), RTMP1z; \
	vsm4rnds4 RTMP1z, RA0z, RA0z; \
	vsm4rnds4 RTMP1z, RA1z, RA1z; \
	vsm4rnds4 RTMP1z, RA2z, RA2z; \
	vsm4rnds4 RTMP1z, RA3z, RA3z; \
	vsm4rnds4 RTMP1z, RB0z, RB0z; \
	vsm4rnds4 RTMP1z, RB1z, RB1z; \
	vsm4rnds4 RTMP1z, RB2z, RB2z; \
	vsm4rnds4 RTMP1z, RB3z, RB3z;

	ROUND4(0 * 16);
	ROUND4(1 * 16);
	ROUND4(2 * 16);
	ROUND4(3 * 16);
	ROUND4(4 * 16);
	ROUND4(5 * 16);
	ROUND4(6 * 16);
	ROUND4(7 * 16);

#undef ROUND4

	vbroadcasti32x4 .Lbswap128_mask rRIP, RTMP0z;
	vpshufb RTMP0z, RA0z, RA0z;
	vpshufb RTMP0z, RA1z, RA1z;
	vpshufb RTMP0z, RA2z, RA2z;
	vpshufb RTMP0z, RA3z, RA3z;
	vpshufb RTMP0z, RB0z, RB0z;
	vpshufb RTMP0z, RB1z, RB1z;
	vpshufb RTMP0z, RB2z, RB2z;
	vpshufb RTMP0z, RB3z, RB3z;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size __sm4_intel_crypt_blk32,.-__sm4_intel_crypt_blk32;)

#define FUNC_NAME(func) _gcry_sm4_intel_avx512_ ## func
#define SM4_CRYPT_BLK32 __sm4_intel_crypt_blk32
#include "sm4-avx512-amd64.h"

#endif /*ENABLE_AVX512_SUPPORT && HAVE_GCC_INLINE_ASM_SM4_AVX512*/
#endif /*__x86_64*/
