/* sm4-intel-avx2-amd64.S  -  Intel SM4 instructions accelerated AVX2 SM4 cipher
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
    defined(ENABLE_AVX2_SUPPORT) && defined(HAVE_GCC_INLINE_ASM_SM4)

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

#define RB0          %ymm9
#define RB1          %ymm10
#define RB2          %ymm11
#define RB3          %ymm12
#define RB0x         %xmm9
#define RB1x         %xmm10
#define RB2x         %xmm11
#define RB3x         %xmm12

#define RNOT         %ymm13
#define RNOTx        %xmm13

SECTION_RODATA
.align 32

ELF(.type _sm4_intel_avx2_consts,@object)
_sm4_intel_avx2_consts:

/* For output word byteswap */
.Lbswap128_mask:
	.byte 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

/* For input word byte-swap */
.Lbswap32_mask:
	.byte 3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12

.text

/**********************************************************************
  SM4 key schedule with Intel SM4 instructions
 **********************************************************************/

.align 16
.globl _gcry_sm4_intel_avx2_expand_key
ELF(.type   _gcry_sm4_intel_avx2_expand_key,@function;)
_gcry_sm4_intel_avx2_expand_key:
	/* input:
	 *	%rdi: 128-bit master key
	 *	%rsi: rkey_enc
	 *	%rdx: rkey_dec
	 *	%rcx: fk array
	 *	%r8: ck array
	 */
	CFI_STARTPROC();

	vmovdqu (%rdi), RTMP0x;
	vpshufb .Lbswap32_mask rRIP, RTMP0x, RTMP0x;
	vpxor (%rcx), RTMP0x, RTMP0x;

	vmovdqu (0*4*4)(%r8), RA0x;
	vmovdqu (1*4*4)(%r8), RA1x;
	vmovdqu (2*4*4)(%r8), RA2x;
	vmovdqu (3*4*4)(%r8), RA3x;
	vmovdqu (4*4*4)(%r8), RB0x;
	vmovdqu (5*4*4)(%r8), RB1x;
	vmovdqu (6*4*4)(%r8), RB2x;
	vmovdqu (7*4*4)(%r8), RB3x;

#define EXPAND_ROUND4(ck, r) \
	vsm4key4 ck, RTMP0x, RTMP0x; \
	vpshufd $0x1b, RTMP0x, RTMP1x; \
	vmovdqu RTMP0x, (4*4*(r))(%rsi); \
	vmovdqu RTMP1x, ((32-4)*4)-4*4*(r)(%rdx);

	EXPAND_ROUND4(RA0x, 0);
	EXPAND_ROUND4(RA1x, 1);
	EXPAND_ROUND4(RA2x, 2);
	EXPAND_ROUND4(RA3x, 3);
	EXPAND_ROUND4(RB0x, 4);
	EXPAND_ROUND4(RB1x, 5);
	EXPAND_ROUND4(RB2x, 6);
	EXPAND_ROUND4(RB3x, 7);

#undef EXPAND_ROUND4

	vzeroall;
	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size _gcry_sm4_intel_avx2_expand_key,.-_gcry_sm4_intel_avx2_expand_key;)

/**********************************************************************
  16-way SM4 with Intel SM4 instructions
 **********************************************************************/

.align 16
ELF(.type   __sm4_intel_crypt_blk16,@function;)
__sm4_intel_crypt_blk16:
	/* input:
	 *	%rdi: round key array, CTX
	 *	RA0, RA1, RA2, RA3, RB0, RB1, RB2, RB3: sixteen parallel
	 *						plaintext blocks
	 * output:
	 *	RA0, RA1, RA2, RA3, RB0, RB1, RB2, RB3: sixteen parallel
	 * 						ciphertext blocks
	 */
	CFI_STARTPROC();

	vbroadcasti128 .Lbswap32_mask rRIP, RTMP0;
	vpshufb RTMP0, RA0, RA0;
	vpshufb RTMP0, RA1, RA1;
	vpshufb RTMP0, RA2, RA2;
	vpshufb RTMP0, RA3, RA3;
	vpshufb RTMP0, RB0, RB0;
	vpshufb RTMP0, RB1, RB1;
	vpshufb RTMP0, RB2, RB2;
	vpshufb RTMP0, RB3, RB3;

#define ROUND4(rkoff) \
	vbroadcasti128 (rkoff)(%rdi), RTMP1; \
	vsm4rnds4 RTMP1, RA0, RA0; \
	vsm4rnds4 RTMP1, RA1, RA1; \
	vsm4rnds4 RTMP1, RA2, RA2; \
	vsm4rnds4 RTMP1, RA3, RA3; \
	vsm4rnds4 RTMP1, RB0, RB0; \
	vsm4rnds4 RTMP1, RB1, RB1; \
	vsm4rnds4 RTMP1, RB2, RB2; \
	vsm4rnds4 RTMP1, RB3, RB3;

	ROUND4(0 * 16);
	ROUND4(1 * 16);
	ROUND4(2 * 16);
	ROUND4(3 * 16);
	ROUND4(4 * 16);
	ROUND4(5 * 16);
	ROUND4(6 * 16);
	ROUND4(7 * 16);

#undef ROUND4

	vbroadcasti128 .Lbswap128_mask rRIP, RTMP0;
	vpshufb RTMP0, RA0, RA0;
	vpshufb RTMP0, RA1, RA1;
	vpshufb RTMP0, RA2, RA2;
	vpshufb RTMP0, RA3, RA3;
	vpshufb RTMP0, RB0, RB0;
	vpshufb RTMP0, RB1, RB1;
	vpshufb RTMP0, RB2, RB2;
	vpshufb RTMP0, RB3, RB3;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size __sm4_intel_crypt_blk16,.-__sm4_intel_crypt_blk16;)

/* Four rounds (one VSM4RNDS4 group) on 8, 4, 2 or 1 parallel blocks. */
#define ROUND4_BLK8(rkoff) \
	vbroadcasti128 (rkoff)(%rdi), RTMP1; \
	vsm4rnds4 RTMP1, RA0, RA0; \
	vsm4rnds4 RTMP1, RA1, RA1; \
	vsm4rnds4 RTMP1, RA2, RA2; \
	vsm4rnds4 RTMP1, RA3, RA3;

#define ROUND4_BLK4(rkoff) \
	vbroadcasti128 (rkoff)(%rdi), RTMP1; \
	vsm4rnds4 RTMP1, RA0, RA0; \
	vsm4rnds4 RTMP1, RA1, RA1;

#define ROUND4_BLK2(rkoff) \
	vbroadcasti128 (rkoff)(%rdi), RTMP1; \
	vsm4rnds4 RTMP1, RA0, RA0;

#define ROUND4_BLK1(rkoff) \
	vsm4rnds4 (rkoff)(%rdi), RA0x, RA0x;

/**********************************************************************
  8/4/2/1-way SM4 with Intel SM4 instructions
 **********************************************************************/
.align 16
.globl _gcry_sm4_intel_avx2_crypt_blk1_16
ELF(.type   _gcry_sm4_intel_avx2_crypt_blk1_16,@function;)
_gcry_sm4_intel_avx2_crypt_blk1_16:
	/* input:
	 *	%rdi: round key array, CTX
	 *	%rsi: dst (1..16 blocks)
	 *	%rdx: src (1..16 blocks)
	 *	%rcx: num blocks (1..16)
	 */
	CFI_STARTPROC();

	cmpq $16, %rcx;
	jne .Lblk1_15_intel;

	/* Sixteen blocks: use the eight-register block function. */
	vmovdqu (0 * 32)(%rdx), RA0;
	vmovdqu (1 * 32)(%rdx), RA1;
	vmovdqu (2 * 32)(%rdx), RA2;
	vmovdqu (3 * 32)(%rdx), RA3;
	vmovdqu (4 * 32)(%rdx), RB0;
	vmovdqu (5 * 32)(%rdx), RB1;
	vmovdqu (6 * 32)(%rdx), RB2;
	vmovdqu (7 * 32)(%rdx), RB3;
	call __sm4_intel_crypt_blk16;
	vmovdqu RA0, (0 * 32)(%rsi);
	vmovdqu RA1, (1 * 32)(%rsi);
	vmovdqu RA2, (2 * 32)(%rsi);
	vmovdqu RA3, (3 * 32)(%rsi);
	vmovdqu RB0, (4 * 32)(%rsi);
	vmovdqu RB1, (5 * 32)(%rsi);
	vmovdqu RB2, (6 * 32)(%rsi);
	vmovdqu RB3, (7 * 32)(%rsi);
	jmp .Lblk1_16_done_intel;

.Lblk1_15_intel:
	movq %rcx, %rax;
	vbroadcasti128 .Lbswap32_mask rRIP, RTMP0;
	vbroadcasti128 .Lbswap128_mask rRIP, RTMP2;

	/* Eight blocks (four YMM registers). */
	cmpq $8, %rax;
	jb .Lblk1_16_blk4_intel;

	vmovdqu (0 * 32)(%rdx), RA0;
	vmovdqu (1 * 32)(%rdx), RA1;
	vmovdqu (2 * 32)(%rdx), RA2;
	vmovdqu (3 * 32)(%rdx), RA3;
	leaq (8 * 16)(%rdx), %rdx;
	vpshufb RTMP0, RA0, RA0;
	vpshufb RTMP0, RA1, RA1;
	vpshufb RTMP0, RA2, RA2;
	vpshufb RTMP0, RA3, RA3;
	ROUND4_BLK8(0 * 16);
	ROUND4_BLK8(1 * 16);
	ROUND4_BLK8(2 * 16);
	ROUND4_BLK8(3 * 16);
	ROUND4_BLK8(4 * 16);
	ROUND4_BLK8(5 * 16);
	ROUND4_BLK8(6 * 16);
	ROUND4_BLK8(7 * 16);
	vpshufb RTMP2, RA0, RA0;
	vpshufb RTMP2, RA1, RA1;
	vpshufb RTMP2, RA2, RA2;
	vpshufb RTMP2, RA3, RA3;
	vmovdqu RA0, (0 * 32)(%rsi);
	vmovdqu RA1, (1 * 32)(%rsi);
	vmovdqu RA2, (2 * 32)(%rsi);
	vmovdqu RA3, (3 * 32)(%rsi);
	leaq (8 * 16)(%rsi), %rsi;

	subq $8, %rax;
	jz .Lblk1_16_done_intel;

.Lblk1_16_blk4_intel:
	/* Four blocks (two YMM registers). */
	cmpq $4, %rax;
	jb .Lblk1_16_blk2_intel;

	vmovdqu (0 * 32)(%rdx), RA0;
	vmovdqu (1 * 32)(%rdx), RA1;
	leaq (4 * 16)(%rdx), %rdx;
	vpshufb RTMP0, RA0, RA0;
	vpshufb RTMP0, RA1, RA1;
	ROUND4_BLK4(0 * 16);
	ROUND4_BLK4(1 * 16);
	ROUND4_BLK4(2 * 16);
	ROUND4_BLK4(3 * 16);
	ROUND4_BLK4(4 * 16);
	ROUND4_BLK4(5 * 16);
	ROUND4_BLK4(6 * 16);
	ROUND4_BLK4(7 * 16);
	vpshufb RTMP2, RA0, RA0;
	vpshufb RTMP2, RA1, RA1;
	vmovdqu RA0, (0 * 32)(%rsi);
	vmovdqu RA1, (1 * 32)(%rsi);
	leaq (4 * 16)(%rsi), %rsi;

	subq $4, %rax;
	jz .Lblk1_16_done_intel;

.Lblk1_16_blk2_intel:
	/* Two blocks (one YMM register). */
	cmpq $2, %rax;
	jb .Lblk1_16_blk1_intel;

	vmovdqu (0 * 32)(%rdx), RA0;
	leaq (2 * 16)(%rdx), %rdx;
	vpshufb RTMP0, RA0, RA0;
	ROUND4_BLK2(0 * 16);
	ROUND4_BLK2(1 * 16);
	ROUND4_BLK2(2 * 16);
	ROUND4_BLK2(3 * 16);
	ROUND4_BLK2(4 * 16);
	ROUND4_BLK2(5 * 16);
	ROUND4_BLK2(6 * 16);
	ROUND4_BLK2(7 * 16);
	vpshufb RTMP2, RA0, RA0;
	vmovdqu RA0, (0 * 32)(%rsi);
	leaq (2 * 16)(%rsi), %rsi;

	subq $2, %rax;
	jz .Lblk1_16_done_intel;

.Lblk1_16_blk1_intel:
	/* One block (one XMM register). */
	cmpq $1, %rax;
	jb .Lblk1_16_done_intel;

	vmovdqu (%rdx), RA0x;
	vpshufb RTMP0x, RA0x, RA0x;
	ROUND4_BLK1(0 * 16);
	ROUND4_BLK1(1 * 16);
	ROUND4_BLK1(2 * 16);
	ROUND4_BLK1(3 * 16);
	ROUND4_BLK1(4 * 16);
	ROUND4_BLK1(5 * 16);
	ROUND4_BLK1(6 * 16);
	ROUND4_BLK1(7 * 16);
	vpshufb RTMP2x, RA0x, RA0x;
	vmovdqu RA0x, (%rsi);

.Lblk1_16_done_intel:
	xorl %eax, %eax;
	vzeroall;
	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size _gcry_sm4_intel_avx2_crypt_blk1_16,.-_gcry_sm4_intel_avx2_crypt_blk1_16;)

#undef ROUND4_BLK8
#undef ROUND4_BLK4
#undef ROUND4_BLK2
#undef ROUND4_BLK1

#define FUNC_NAME(func) _gcry_sm4_intel_avx2_ ## func
#define SM4_CRYPT_BLK16 __sm4_intel_crypt_blk16
#include "sm4-avx2-amd64.h"

#endif /*defined(ENABLE_AVX2_SUPPORT) && defined(HAVE_GCC_INLINE_ASM_SM4)*/
#endif /*__x86_64*/
