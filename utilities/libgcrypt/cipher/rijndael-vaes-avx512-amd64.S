/* VAES/AVX512 AMD64 accelerated AES for Libgcrypt
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

#if defined(__x86_64__)
#include <config.h>
#if (defined(HAVE_COMPATIBLE_GCC_AMD64_PLATFORM_AS) || \
     defined(HAVE_COMPATIBLE_GCC_WIN64_PLATFORM_AS)) && \
    defined(ENABLE_AESNI_SUPPORT) && defined(ENABLE_AVX2_SUPPORT) && \
    defined(ENABLE_AVX512_SUPPORT) && \
    defined(HAVE_GCC_INLINE_ASM_VAES_VPCLMUL)

#include "asm-common-amd64.h"

.text

/**********************************************************************
  helper macros
 **********************************************************************/
#define no(...) /*_*/
#define yes(...) __VA_ARGS__

#define AES_OP8(op, key, b0, b1, b2, b3, b4, b5, b6, b7) \
	op key, b0, b0; \
	op key, b1, b1; \
	op key, b2, b2; \
	op key, b3, b3; \
	op key, b4, b4; \
	op key, b5, b5; \
	op key, b6, b6; \
	op key, b7, b7;

#define VAESENC8(key, b0, b1, b2, b3, b4, b5, b6, b7) \
	AES_OP8(vaesenc, key, b0, b1, b2, b3, b4, b5, b6, b7)

#define VAESDEC8(key, b0, b1, b2, b3, b4, b5, b6, b7) \
	AES_OP8(vaesdec, key, b0, b1, b2, b3, b4, b5, b6, b7)

#define XOR8(key, b0, b1, b2, b3, b4, b5, b6, b7) \
	AES_OP8(vpxord, key, b0, b1, b2, b3, b4, b5, b6, b7)

#define AES_OP4(op, key, b0, b1, b2, b3) \
	op key, b0, b0; \
	op key, b1, b1; \
	op key, b2, b2; \
	op key, b3, b3;

#define VAESENC4(key, b0, b1, b2, b3) \
	AES_OP4(vaesenc, key, b0, b1, b2, b3)

#define VAESDEC4(key, b0, b1, b2, b3) \
	AES_OP4(vaesdec, key, b0, b1, b2, b3)

#define XOR4(key, b0, b1, b2, b3) \
	AES_OP4(vpxord, key, b0, b1, b2, b3)

/**********************************************************************
  CBC-mode decryption
 **********************************************************************/
ELF(.type _gcry_vaes_avx512_cbc_dec_amd64,@function)
.globl _gcry_vaes_avx512_cbc_dec_amd64
.align 16
_gcry_vaes_avx512_cbc_dec_amd64:
	/* input:
	 *	%rdi: round keys
	 *	%rsi: iv
	 *	%rdx: dst
	 *	%rcx: src
	 *	%r8:  nblocks
	 *	%r9:  nrounds
	 */
	CFI_STARTPROC();

	cmpq $16, %r8;
	jb .Lcbc_dec_skip_avx512;

	spec_stop_avx512;

	/* Load IV. */
	vmovdqu (%rsi), %xmm15;

	/* Load first and last key. */
	leal (, %r9d, 4), %eax;
	vbroadcasti32x4 (%rdi), %zmm30;
	vbroadcasti32x4 (%rdi, %rax, 4), %zmm31;

	/* Process 32 blocks per loop. */
.align 16
.Lcbc_dec_blk32:
	cmpq $32, %r8;
	jb .Lcbc_dec_blk16;

	leaq -32(%r8), %r8;

	/* Load input and xor first key. Update IV. */
	vmovdqu32 (0 * 16)(%rcx), %zmm0;
	vshufi32x4 $0b10010011, %zmm0, %zmm0, %zmm9;
	vmovdqu32 (4 * 16)(%rcx), %zmm1;
	vmovdqu32 (8 * 16)(%rcx), %zmm2;
	vmovdqu32 (12 * 16)(%rcx), %zmm3;
	vmovdqu32 (16 * 16)(%rcx), %zmm4;
	vmovdqu32 (20 * 16)(%rcx), %zmm5;
	vmovdqu32 (24 * 16)(%rcx), %zmm6;
	vmovdqu32 (28 * 16)(%rcx), %zmm7;
	vinserti32x4 $0, %xmm15, %zmm9, %zmm9;
	vpxord %zmm30, %zmm0, %zmm0;
	vpxord %zmm30, %zmm1, %zmm1;
	vpxord %zmm30, %zmm2, %zmm2;
	vpxord %zmm30, %zmm3, %zmm3;
	vpxord %zmm30, %zmm4, %zmm4;
	vpxord %zmm30, %zmm5, %zmm5;
	vpxord %zmm30, %zmm6, %zmm6;
	vpxord %zmm30, %zmm7, %zmm7;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm8;
	vpxord %zmm31, %zmm9, %zmm9;
	vpxord (3 * 16)(%rcx), %zmm31, %zmm10;
	vpxord (7 * 16)(%rcx), %zmm31, %zmm11;
	vpxord (11 * 16)(%rcx), %zmm31, %zmm12;
	vpxord (15 * 16)(%rcx), %zmm31, %zmm13;
	vpxord (19 * 16)(%rcx), %zmm31, %zmm14;
	vpxord (23 * 16)(%rcx), %zmm31, %zmm16;
	vpxord (27 * 16)(%rcx), %zmm31, %zmm17;
	vmovdqu (31 * 16)(%rcx), %xmm15;
	leaq (32 * 16)(%rcx), %rcx;

	/* AES rounds */
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (2 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (3 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (4 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (5 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (6 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (7 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (8 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (9 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	cmpl $12, %r9d;
	jb .Lcbc_dec_blk32_last;
	vbroadcasti32x4 (10 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (11 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	jz .Lcbc_dec_blk32_last;
	vbroadcasti32x4 (12 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (13 * 16)(%rdi), %zmm8;
	VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);

	/* Last round and output handling. */
  .align 16
  .Lcbc_dec_blk32_last:
	vaesdeclast %zmm9, %zmm0, %zmm0;
	vaesdeclast %zmm10, %zmm1, %zmm1;
	vaesdeclast %zmm11, %zmm2, %zmm2;
	vaesdeclast %zmm12, %zmm3, %zmm3;
	vaesdeclast %zmm13, %zmm4, %zmm4;
	vaesdeclast %zmm14, %zmm5, %zmm5;
	vaesdeclast %zmm16, %zmm6, %zmm6;
	vaesdeclast %zmm17, %zmm7, %zmm7;
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	vmovdqu32 %zmm4, (16 * 16)(%rdx);
	vmovdqu32 %zmm5, (20 * 16)(%rdx);
	vmovdqu32 %zmm6, (24 * 16)(%rdx);
	vmovdqu32 %zmm7, (28 * 16)(%rdx);
	leaq (32 * 16)(%rdx), %rdx;

	jmp .Lcbc_dec_blk32;

	/* Process 16 blocks per loop. */
.align 16
.Lcbc_dec_blk16:
	cmpq $16, %r8;
	jb .Lcbc_dec_tail;

	leaq -16(%r8), %r8;

	/* Load input and xor first key. Update IV. */
	vmovdqu32 (0 * 16)(%rcx), %zmm0;
	vshufi32x4 $0b10010011, %zmm0, %zmm0, %zmm9;
	vmovdqu32 (4 * 16)(%rcx), %zmm1;
	vmovdqu32 (8 * 16)(%rcx), %zmm2;
	vmovdqu32 (12 * 16)(%rcx), %zmm3;
	vinserti32x4 $0, %xmm15, %zmm9, %zmm9;
	vpxord %zmm30, %zmm0, %zmm0;
	vpxord %zmm30, %zmm1, %zmm1;
	vpxord %zmm30, %zmm2, %zmm2;
	vpxord %zmm30, %zmm3, %zmm3;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm8;
	vpxord %zmm31, %zmm9, %zmm9;
	vpxord (3 * 16)(%rcx), %zmm31, %zmm10;
	vpxord (7 * 16)(%rcx), %zmm31, %zmm11;
	vpxord (11 * 16)(%rcx), %zmm31, %zmm12;
	vmovdqu (15 * 16)(%rcx), %xmm15;
	leaq (16 * 16)(%rcx), %rcx;

	/* AES rounds */
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (2 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (3 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (4 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (5 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (6 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (7 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (8 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (9 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	cmpl $12, %r9d;
	jb .Lcbc_dec_blk16_last;
	vbroadcasti32x4 (10 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (11 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	jz .Lcbc_dec_blk16_last;
	vbroadcasti32x4 (12 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (13 * 16)(%rdi), %zmm8;
	VAESDEC4(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3);

	/* Last round and output handling. */
  .align 16
  .Lcbc_dec_blk16_last:
	vaesdeclast %zmm9, %zmm0, %zmm0;
	vaesdeclast %zmm10, %zmm1, %zmm1;
	vaesdeclast %zmm11, %zmm2, %zmm2;
	vaesdeclast %zmm12, %zmm3, %zmm3;
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	leaq (16 * 16)(%rdx), %rdx;

.align 16
.Lcbc_dec_tail:
	/* Store IV. */
	vmovdqu %xmm15, (%rsi);

	/* Clear used AVX512 registers. */
	vpxord %ymm16, %ymm16, %ymm16;
	vpxord %ymm17, %ymm17, %ymm17;
	vpxord %ymm30, %ymm30, %ymm30;
	vpxord %ymm31, %ymm31, %ymm31;
	vzeroall;

.align 16
.Lcbc_dec_skip_avx512:
	/* Handle trailing blocks with AVX2 implementation. */
	cmpq $0, %r8;
	ja _gcry_vaes_avx2_cbc_dec_amd64 AT_PLT;

	ret_spec_stop
	CFI_ENDPROC();
ELF(.size _gcry_vaes_avx512_cbc_dec_amd64,.-_gcry_vaes_avx512_cbc_dec_amd64)

/**********************************************************************
  CFB-mode decryption
 **********************************************************************/
ELF(.type _gcry_vaes_avx512_cfb_dec_amd64,@function)
.globl _gcry_vaes_avx512_cfb_dec_amd64
.align 16
_gcry_vaes_avx512_cfb_dec_amd64:
	/* input:
	 *	%rdi: round keys
	 *	%rsi: iv
	 *	%rdx: dst
	 *	%rcx: src
	 *	%r8:  nblocks
	 *	%r9:  nrounds
	 */
	CFI_STARTPROC();

	cmpq $16, %r8;
	jb .Lcfb_dec_skip_avx512;

	spec_stop_avx512;

	/* Load IV. */
	vmovdqu (%rsi), %xmm15;

	/* Load first and last key. */
	leal (, %r9d, 4), %eax;
	vbroadcasti32x4 (%rdi), %zmm30;
	vbroadcasti32x4 (%rdi, %rax, 4), %zmm31;

	/* Process 32 blocks per loop. */
.align 16
.Lcfb_dec_blk32:
	cmpq $32, %r8;
	jb .Lcfb_dec_blk16;

	leaq -32(%r8), %r8;

	/* Load input and xor first key. Update IV. */
	vmovdqu32 (0 * 16)(%rcx), %zmm9;
	vshufi32x4 $0b10010011, %zmm9, %zmm9, %zmm0;
	vmovdqu32 (3 * 16)(%rcx), %zmm1;
	vinserti32x4 $0, %xmm15, %zmm0, %zmm0;
	vmovdqu32 (7 * 16)(%rcx), %zmm2;
	vmovdqu32 (11 * 16)(%rcx), %zmm3;
	vmovdqu32 (15 * 16)(%rcx), %zmm4;
	vmovdqu32 (19 * 16)(%rcx), %zmm5;
	vmovdqu32 (23 * 16)(%rcx), %zmm6;
	vmovdqu32 (27 * 16)(%rcx), %zmm7;
	vmovdqu (31 * 16)(%rcx), %xmm15;
	vpxord %zmm30, %zmm0, %zmm0;
	vpxord %zmm30, %zmm1, %zmm1;
	vpxord %zmm30, %zmm2, %zmm2;
	vpxord %zmm30, %zmm3, %zmm3;
	vpxord %zmm30, %zmm4, %zmm4;
	vpxord %zmm30, %zmm5, %zmm5;
	vpxord %zmm30, %zmm6, %zmm6;
	vpxord %zmm30, %zmm7, %zmm7;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm8;
	vpxord %zmm31, %zmm9, %zmm9;
	vpxord (4 * 16)(%rcx), %zmm31, %zmm10;
	vpxord (8 * 16)(%rcx), %zmm31, %zmm11;
	vpxord (12 * 16)(%rcx), %zmm31, %zmm12;
	vpxord (16 * 16)(%rcx), %zmm31, %zmm13;
	vpxord (20 * 16)(%rcx), %zmm31, %zmm14;
	vpxord (24 * 16)(%rcx), %zmm31, %zmm16;
	vpxord (28 * 16)(%rcx), %zmm31, %zmm17;
	leaq (32 * 16)(%rcx), %rcx;

	/* AES rounds */
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (2 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (3 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (4 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (5 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (6 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (7 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (8 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (9 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	cmpl $12, %r9d;
	jb .Lcfb_dec_blk32_last;
	vbroadcasti32x4 (10 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (11 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	jz .Lcfb_dec_blk32_last;
	vbroadcasti32x4 (12 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (13 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);

	/* Last round and output handling. */
  .align 16
  .Lcfb_dec_blk32_last:
	vaesenclast %zmm9, %zmm0, %zmm0;
	vaesenclast %zmm10, %zmm1, %zmm1;
	vaesenclast %zmm11, %zmm2, %zmm2;
	vaesenclast %zmm12, %zmm3, %zmm3;
	vaesenclast %zmm13, %zmm4, %zmm4;
	vaesenclast %zmm14, %zmm5, %zmm5;
	vaesenclast %zmm16, %zmm6, %zmm6;
	vaesenclast %zmm17, %zmm7, %zmm7;
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	vmovdqu32 %zmm4, (16 * 16)(%rdx);
	vmovdqu32 %zmm5, (20 * 16)(%rdx);
	vmovdqu32 %zmm6, (24 * 16)(%rdx);
	vmovdqu32 %zmm7, (28 * 16)(%rdx);
	leaq (32 * 16)(%rdx), %rdx;

	jmp .Lcfb_dec_blk32;

	/* Handle trailing 16 blocks. */
.align 16
.Lcfb_dec_blk16:
	cmpq $16, %r8;
	jb .Lcfb_dec_tail;

	leaq -16(%r8), %r8;

	/* Load input and xor first key. Update IV. */
	vmovdqu32 (0 * 16)(%rcx), %zmm10;
	vshufi32x4 $0b10010011, %zmm10, %zmm10, %zmm0;
	vmovdqu32 (3 * 16)(%rcx), %zmm1;
	vinserti32x4 $0, %xmm15, %zmm0, %zmm0;
	vmovdqu32 (7 * 16)(%rcx), %zmm2;
	vmovdqu32 (11 * 16)(%rcx), %zmm3;
	vmovdqu (15 * 16)(%rcx), %xmm15;
	vpxord %zmm30, %zmm0, %zmm0;
	vpxord %zmm30, %zmm1, %zmm1;
	vpxord %zmm30, %zmm2, %zmm2;
	vpxord %zmm30, %zmm3, %zmm3;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm4;
	vpxord %zmm31, %zmm10, %zmm10;
	vpxord (4 * 16)(%rcx), %zmm31, %zmm11;
	vpxord (8 * 16)(%rcx), %zmm31, %zmm12;
	vpxord (12 * 16)(%rcx), %zmm31, %zmm13;
	leaq (16 * 16)(%rcx), %rcx;

	/* AES rounds */
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	cmpl $12, %r9d;
	jb .Lcfb_dec_blk16_last;
	vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	jz .Lcfb_dec_blk16_last;
	vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);

	/* Last round and output handling. */
  .align 16
  .Lcfb_dec_blk16_last:
	vaesenclast %zmm10, %zmm0, %zmm0;
	vaesenclast %zmm11, %zmm1, %zmm1;
	vaesenclast %zmm12, %zmm2, %zmm2;
	vaesenclast %zmm13, %zmm3, %zmm3;
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	leaq (16 * 16)(%rdx), %rdx;

.align 16
.Lcfb_dec_tail:
	/* Store IV. */
	vmovdqu %xmm15, (%rsi);

	/* Clear used AVX512 registers. */
	vpxord %ymm16, %ymm16, %ymm16;
	vpxord %ymm17, %ymm17, %ymm17;
	vpxord %ymm30, %ymm30, %ymm30;
	vpxord %ymm31, %ymm31, %ymm31;
	vzeroall;

.align 16
.Lcfb_dec_skip_avx512:
	/* Handle trailing blocks with AVX2 implementation. */
	cmpq $0, %r8;
	ja _gcry_vaes_avx2_cfb_dec_amd64 AT_PLT;

	ret_spec_stop
	CFI_ENDPROC();
ELF(.size _gcry_vaes_avx512_cfb_dec_amd64,.-_gcry_vaes_avx512_cfb_dec_amd64)

/**********************************************************************
  CTR-mode encryption
 **********************************************************************/
ELF(.type _gcry_vaes_avx512_ctr_enc_amd64,@function)
.globl _gcry_vaes_avx512_ctr_enc_amd64
.align 16
_gcry_vaes_avx512_ctr_enc_amd64:
	/* input:
	 *	%rdi: round keys
	 *	%rsi: counter
	 *	%rdx: dst
	 *	%rcx: src
	 *	%r8:  nblocks
	 *	%r9:  nrounds
	 */
	CFI_STARTPROC();

	cmpq $16, %r8;
	jb .Lctr_enc_skip_avx512;

	spec_stop_avx512;

	vmovdqa32 .Lbige_addb_0 rRIP, %zmm20;
	vmovdqa32 .Lbige_addb_4 rRIP, %zmm21;
	vmovdqa32 .Lbige_addb_8 rRIP, %zmm22;
	vmovdqa32 .Lbige_addb_12 rRIP, %zmm23;

	movzbl 15(%rsi), %r10d;
	movzbl 14(%rsi), %r11d;

	/* Load first and last key. */
	leal (, %r9d, 4), %eax;
	vbroadcasti32x4 (%rdi), %zmm30;
	vbroadcasti32x4 (%rdi, %rax, 4), %zmm31;

	cmpq $32, %r8;
	jb .Lctr_enc_blk16;

	vmovdqa32 .Lbige_addb_16 rRIP, %zmm24;
	vmovdqa32 .Lbige_addb_20 rRIP, %zmm25;
	vmovdqa32 .Lbige_addb_24 rRIP, %zmm26;
	vmovdqa32 .Lbige_addb_28 rRIP, %zmm27;

	/* Process 32 blocks per loop. */
.align 16
.Lctr_enc_blk32:
	leaq -32(%r8), %r8;

	vbroadcasti32x4 (%rsi), %zmm7;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm8;

	/* detect if carry handling is needed */
	addb $32, %r10b;
	jc .Lctr_enc_blk32_handle_carry;

  .Lctr_enc_blk32_byte_bige_add:
	/* Increment counters. */
	vpaddb %zmm20, %zmm7, %zmm0;
	vpaddb %zmm21, %zmm7, %zmm1;
	vpaddb %zmm22, %zmm7, %zmm2;
	vpaddb %zmm23, %zmm7, %zmm3;
	vpaddb %zmm24, %zmm7, %zmm4;
	vpaddb %zmm25, %zmm7, %zmm5;
	vpaddb %zmm26, %zmm7, %zmm6;
	vpaddb %zmm27, %zmm7, %zmm7;

  .Lctr_enc_blk32_rounds:
	movb %r10b, 15(%rsi);
	/* AES rounds */
	XOR8(%zmm30, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (2 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (3 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (4 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (5 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (6 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (7 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (8 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (9 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	cmpl $12, %r9d;
	jb .Lctr_enc_blk32_last;
	vbroadcasti32x4 (10 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (11 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	jz .Lctr_enc_blk32_last;
	vbroadcasti32x4 (12 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (13 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);

	/* Last round and output handling. */
  .align 16
  .Lctr_enc_blk32_last:
	vpxord (0 * 16)(%rcx), %zmm31, %zmm9; /* Xor src to last round key. */
	vpxord (4 * 16)(%rcx), %zmm31, %zmm10;
	vpxord (8 * 16)(%rcx), %zmm31, %zmm11;
	vpxord (12 * 16)(%rcx), %zmm31, %zmm12;
	vpxord (16 * 16)(%rcx), %zmm31, %zmm13;
	vpxord (20 * 16)(%rcx), %zmm31, %zmm14;
	vpxord (24 * 16)(%rcx), %zmm31, %zmm15;
	vpxord (28 * 16)(%rcx), %zmm31, %zmm8;
	leaq (32 * 16)(%rcx), %rcx;
	vaesenclast %zmm9, %zmm0, %zmm0;
	vaesenclast %zmm10, %zmm1, %zmm1;
	vaesenclast %zmm11, %zmm2, %zmm2;
	vaesenclast %zmm12, %zmm3, %zmm3;
	vaesenclast %zmm13, %zmm4, %zmm4;
	vaesenclast %zmm14, %zmm5, %zmm5;
	vaesenclast %zmm15, %zmm6, %zmm6;
	vaesenclast %zmm8, %zmm7, %zmm7;
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	vmovdqu32 %zmm4, (16 * 16)(%rdx);
	vmovdqu32 %zmm5, (20 * 16)(%rdx);
	vmovdqu32 %zmm6, (24 * 16)(%rdx);
	vmovdqu32 %zmm7, (28 * 16)(%rdx);
	leaq (32 * 16)(%rdx), %rdx;

	cmpq $32, %r8;
	jnb .Lctr_enc_blk32;

	/* Clear used AVX512 registers. */
	vpxord %ymm24, %ymm24, %ymm24;
	vpxord %ymm25, %ymm25, %ymm25;
	vpxord %ymm26, %ymm26, %ymm26;
	vpxord %ymm27, %ymm27, %ymm27;

	jmp .Lctr_enc_blk16;

  .align 16
  .Lctr_enc_blk32_handle_carry:
	leal 1(%r11d), %r11d;
	movb %r11b, 14(%rsi);
	jz .Lctr_enc_blk32_byte_bige_add;

	/* Increment counters (handle 16-bit carry). */
	vbroadcasti32x4 .Lbswap128_mask rRIP, %zmm15;
	vpmovzxbq .Lcounter0_1_2_3_lo_bq rRIP, %zmm10;
	vpshufb %zmm15, %zmm7, %zmm7; /* be => le */
	vpmovzxbq .Lcounter4_4_4_4_lo_bq rRIP, %zmm11;
	vpmovzxbq .Lcounter8_8_8_8_lo_bq rRIP, %zmm12;
	vpaddw %zmm10, %zmm7, %zmm0; /* +0:+1:+2:+3 */
	vpaddw %zmm11, %zmm0, %zmm1; /* +4:+5:+6:+7 */
	vpaddw %zmm12, %zmm0, %zmm2; /* +8:... */
	vpshufb %zmm15, %zmm0, %zmm0; /* le => be */
	vpaddw %zmm12, %zmm1, %zmm3; /* +12:... */
	vpshufb %zmm15, %zmm1, %zmm1; /* le => be */
	vpaddw %zmm12, %zmm2, %zmm4; /* +16:... */
	vpshufb %zmm15, %zmm2, %zmm2; /* le => be */
	vpaddw %zmm12, %zmm3, %zmm5; /* +20:... */
	vpshufb %zmm15, %zmm3, %zmm3; /* le => be */
	vpaddw %zmm12, %zmm4, %zmm6; /* +24:... */
	vpshufb %zmm15, %zmm4, %zmm4; /* le => be */
	vpaddw %zmm12, %zmm5, %zmm7; /* +28:... */
	vpshufb %zmm15, %zmm5, %zmm5; /* le => be */
	vpshufb %zmm15, %zmm6, %zmm6; /* le => be */
	vpshufb %zmm15, %zmm7, %zmm7; /* le => be */

	jmp .Lctr_enc_blk32_rounds;

	/* Handle trailing 16 blocks. */
.align 16
.Lctr_enc_blk16:
	cmpq $16, %r8;
	jb .Lctr_enc_tail;

	leaq -16(%r8), %r8;

	vbroadcasti32x4 (%rsi), %zmm3;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm4;

	/* detect if carry handling is needed */
	addb $16, %r10b;
	jc .Lctr_enc_blk16_handle_carry;

  .Lctr_enc_blk16_byte_bige_add:
	/* Increment counters. */
	vpaddb %zmm20, %zmm3, %zmm0;
	vpaddb %zmm21, %zmm3, %zmm1;
	vpaddb %zmm22, %zmm3, %zmm2;
	vpaddb %zmm23, %zmm3, %zmm3;

  .Lctr_enc_blk16_rounds:
	movb %r10b, 15(%rsi);
	/* AES rounds */
	XOR4(%zmm30, %zmm0, %zmm1, %zmm2, %zmm3);
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	cmpl $12, %r9d;
	jb .Lctr_enc_blk16_last;
	vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	jz .Lctr_enc_blk16_last;
	vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);

	/* Last round and output handling. */
  .align 16
  .Lctr_enc_blk16_last:
	vpxord (0 * 16)(%rcx), %zmm31, %zmm5; /* Xor src to last round key. */
	vpxord (4 * 16)(%rcx), %zmm31, %zmm6;
	vpxord (8 * 16)(%rcx), %zmm31, %zmm7;
	vpxord (12 * 16)(%rcx), %zmm31, %zmm4;
	leaq (16 * 16)(%rcx), %rcx;
	vaesenclast %zmm5, %zmm0, %zmm0;
	vaesenclast %zmm6, %zmm1, %zmm1;
	vaesenclast %zmm7, %zmm2, %zmm2;
	vaesenclast %zmm4, %zmm3, %zmm3;
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	leaq (16 * 16)(%rdx), %rdx;

	jmp .Lctr_enc_tail;

  .align 16
  .Lctr_enc_blk16_handle_carry:
	leal 1(%r11d), %r11d;
	movb %r11b, 14(%rsi);
	jz .Lctr_enc_blk16_byte_bige_add;

	/* Increment counters (handle 16-bit carry). */
	vbroadcasti32x4 .Lbswap128_mask rRIP, %zmm15;
	vpmovzxbq .Lcounter0_1_2_3_lo_bq rRIP, %zmm10;
	vpshufb %zmm15, %zmm3, %zmm3; /* be => le */
	vpmovzxbq .Lcounter4_4_4_4_lo_bq rRIP, %zmm11;
	vpmovzxbq .Lcounter8_8_8_8_lo_bq rRIP, %zmm12;
	vpaddw %zmm10, %zmm3, %zmm0; /* +0:+1:+2:+3 */
	vpaddw %zmm11, %zmm0, %zmm1; /* +4:+5:+6:+7 */
	vpaddw %zmm12, %zmm0, %zmm2; /* +8:... */
	vpshufb %zmm15, %zmm0, %zmm0; /* le => be */
	vpaddw %zmm12, %zmm1, %zmm3; /* +12:... */
	vpshufb %zmm15, %zmm1, %zmm1; /* le => be */
	vpshufb %zmm15, %zmm2, %zmm2; /* le => be */
	vpshufb %zmm15, %zmm3, %zmm3; /* le => be */

	jmp .Lctr_enc_blk16_rounds;

.align 16
.Lctr_enc_tail:
	xorl %r10d, %r10d;
	xorl %r11d, %r11d;

	/* Clear used AVX512 registers. */
	vpxord %ymm20, %ymm20, %ymm20;
	vpxord %ymm21, %ymm21, %ymm21;
	vpxord %ymm22, %ymm22, %ymm22;
	vpxord %ymm23, %ymm23, %ymm23;
	vpxord %ymm30, %ymm30, %ymm30;
	vpxord %ymm31, %ymm31, %ymm31;
	vzeroall;

.align 16
.Lctr_enc_skip_avx512:
	/* Handle trailing blocks with AVX2 implementation. */
	cmpq $0, %r8;
	ja _gcry_vaes_avx2_ctr_enc_amd64 AT_PLT;

	ret_spec_stop
	CFI_ENDPROC();
ELF(.size _gcry_vaes_avx512_ctr_enc_amd64,.-_gcry_vaes_avx512_ctr_enc_amd64)

/**********************************************************************
  Little-endian 32-bit CTR-mode encryption (GCM-SIV)
 **********************************************************************/
ELF(.type _gcry_vaes_avx512_ctr32le_enc_amd64,@function)
.globl _gcry_vaes_avx512_ctr32le_enc_amd64
.align 16
_gcry_vaes_avx512_ctr32le_enc_amd64:
	/* input:
	 *	%rdi: round keys
	 *	%rsi: counter
	 *	%rdx: dst
	 *	%rcx: src
	 *	%r8:  nblocks
	 *	%r9:  nrounds
	 */
	CFI_STARTPROC();

	cmpq $16, %r8;
	jb .Lctr32le_enc_skip_avx512;

	spec_stop_avx512;

	/* Load counter. */
	vbroadcasti32x4 (%rsi), %zmm15;

	vpmovzxbq .Lcounter0_1_2_3_lo_bq rRIP, %zmm20;
	vpmovzxbq .Lcounter4_5_6_7_lo_bq rRIP, %zmm21;
	vpmovzxbq .Lcounter8_9_10_11_lo_bq rRIP, %zmm22;
	vpmovzxbq .Lcounter12_13_14_15_lo_bq rRIP, %zmm23;

	/* Load first and last key. */
	leal (, %r9d, 4), %eax;
	vbroadcasti32x4 (%rdi), %zmm30;
	vbroadcasti32x4 (%rdi, %rax, 4), %zmm31;

	cmpq $32, %r8;
	jb .Lctr32le_enc_blk16;

	vpmovzxbq .Lcounter16_17_18_19_lo_bq rRIP, %zmm24;
	vpmovzxbq .Lcounter20_21_22_23_lo_bq rRIP, %zmm25;
	vpmovzxbq .Lcounter24_25_26_27_lo_bq rRIP, %zmm26;
	vpmovzxbq .Lcounter28_29_30_31_lo_bq rRIP, %zmm27;

	/* Process 32 blocks per loop. */
.align 16
.Lctr32le_enc_blk32:
	leaq -32(%r8), %r8;

	/* Increment counters. */
	vpmovzxbq .Lcounter32_32_32_32_lo_bq rRIP, %zmm9;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm8;
	vpaddd %zmm20, %zmm15, %zmm0;
	vpaddd %zmm21, %zmm15, %zmm1;
	vpaddd %zmm22, %zmm15, %zmm2;
	vpaddd %zmm23, %zmm15, %zmm3;
	vpaddd %zmm24, %zmm15, %zmm4;
	vpaddd %zmm25, %zmm15, %zmm5;
	vpaddd %zmm26, %zmm15, %zmm6;
	vpaddd %zmm27, %zmm15, %zmm7;

	vpaddd %zmm9, %zmm15, %zmm15;

	/* AES rounds */
	XOR8(%zmm30, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (2 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (3 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (4 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (5 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (6 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (7 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (8 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (9 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	cmpl $12, %r9d;
	jb .Lctr32le_enc_blk32_last;
	vbroadcasti32x4 (10 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (11 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	jz .Lctr32le_enc_blk32_last;
	vbroadcasti32x4 (12 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	vbroadcasti32x4 (13 * 16)(%rdi), %zmm8;
	VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);

	/* Last round and output handling. */
  .align 16
  .Lctr32le_enc_blk32_last:
	vpxord (0 * 16)(%rcx), %zmm31, %zmm9; /* Xor src to last round key. */
	vpxord (4 * 16)(%rcx), %zmm31, %zmm10;
	vpxord (8 * 16)(%rcx), %zmm31, %zmm11;
	vpxord (12 * 16)(%rcx), %zmm31, %zmm12;
	vpxord (16 * 16)(%rcx), %zmm31, %zmm13;
	vpxord (20 * 16)(%rcx), %zmm31, %zmm14;
	vpxord (24 * 16)(%rcx), %zmm31, %zmm16;
	vpxord (28 * 16)(%rcx), %zmm31, %zmm8;
	leaq (32 * 16)(%rcx), %rcx;
	vaesenclast %zmm9, %zmm0, %zmm0;
	vaesenclast %zmm10, %zmm1, %zmm1;
	vaesenclast %zmm11, %zmm2, %zmm2;
	vaesenclast %zmm12, %zmm3, %zmm3;
	vaesenclast %zmm13, %zmm4, %zmm4;
	vaesenclast %zmm14, %zmm5, %zmm5;
	vaesenclast %zmm16, %zmm6, %zmm6;
	vaesenclast %zmm8, %zmm7, %zmm7;
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	vmovdqu32 %zmm4, (16 * 16)(%rdx);
	vmovdqu32 %zmm5, (20 * 16)(%rdx);
	vmovdqu32 %zmm6, (24 * 16)(%rdx);
	vmovdqu32 %zmm7, (28 * 16)(%rdx);
	leaq (32 * 16)(%rdx), %rdx;

	cmpq $32, %r8;
	jnb .Lctr32le_enc_blk32;

	/* Clear used AVX512 registers. */
	vpxord %ymm16, %ymm16, %ymm16;
	vpxord %ymm24, %ymm24, %ymm24;
	vpxord %ymm25, %ymm25, %ymm25;
	vpxord %ymm26, %ymm26, %ymm26;
	vpxord %ymm27, %ymm27, %ymm27;

	/* Handle trailing 16 blocks. */
.align 16
.Lctr32le_enc_blk16:
	cmpq $16, %r8;
	jb .Lctr32le_enc_tail;

	leaq -16(%r8), %r8;

	/* Increment counters. */
	vpmovzxbq .Lcounter16_16_16_16_lo_bq rRIP, %zmm5;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm4;
	vpaddd %zmm20, %zmm15, %zmm0;
	vpaddd %zmm21, %zmm15, %zmm1;
	vpaddd %zmm22, %zmm15, %zmm2;
	vpaddd %zmm23, %zmm15, %zmm3;

	vpaddd %zmm5, %zmm15, %zmm15;

	/* AES rounds */
	XOR4(%zmm30, %zmm0, %zmm1, %zmm2, %zmm3);
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	cmpl $12, %r9d;
	jb .Lctr32le_enc_blk16_last;
	vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	jz .Lctr32le_enc_blk16_last;
	vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
	VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);

	/* Last round and output handling. */
  .align 16
  .Lctr32le_enc_blk16_last:
	vpxord (0 * 16)(%rcx), %zmm31, %zmm5; /* Xor src to last round key. */
	vpxord (4 * 16)(%rcx), %zmm31, %zmm6;
	vpxord (8 * 16)(%rcx), %zmm31, %zmm7;
	vpxord (12 * 16)(%rcx), %zmm31, %zmm4;
	leaq (16 * 16)(%rcx), %rcx;
	vaesenclast %zmm5, %zmm0, %zmm0;
	vaesenclast %zmm6, %zmm1, %zmm1;
	vaesenclast %zmm7, %zmm2, %zmm2;
	vaesenclast %zmm4, %zmm3, %zmm3;
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	leaq (16 * 16)(%rdx), %rdx;

.align 16
.Lctr32le_enc_tail:
	/* Store IV. */
	vmovdqu %xmm15, (%rsi);

	/* Clear used AVX512 registers. */
	vpxord %ymm20, %ymm20, %ymm20;
	vpxord %ymm21, %ymm21, %ymm21;
	vpxord %ymm22, %ymm22, %ymm22;
	vpxord %ymm23, %ymm23, %ymm23;
	vpxord %ymm30, %ymm30, %ymm30;
	vpxord %ymm31, %ymm31, %ymm31;
	vzeroall;

.align 16
.Lctr32le_enc_skip_avx512:
	/* Handle trailing blocks with AVX2 implementation. */
	cmpq $0, %r8;
	ja _gcry_vaes_avx2_ctr32le_enc_amd64 AT_PLT;

	ret_spec_stop
	CFI_ENDPROC();
ELF(.size _gcry_vaes_avx512_ctr32le_enc_amd64,.-_gcry_vaes_avx512_ctr32le_enc_amd64)

/**********************************************************************
  OCB-mode encryption/decryption/authentication
 **********************************************************************/
ELF(.type _gcry_vaes_avx512_ocb_aligned_crypt_amd64,@function)
.globl _gcry_vaes_avx512_ocb_aligned_crypt_amd64
.align 16
_gcry_vaes_avx512_ocb_aligned_crypt_amd64:
	/* input:
	 *	%rdi:     round keys
	 *	%esi:     nblk
	 *	%rdx:     dst
	 *	%rcx:     src
	 *	%r8:      nblocks
	 *	%r9:      nrounds
	 *	0(%rsp): offset
	 *	8(%rsp): checksum
	 *	16(%rsp): L-array
	 *	24(%rsp): decrypt/encrypt/auth
	 */
	CFI_STARTPROC();

	cmpq $32, %r8;
	jb .Locb_skip_avx512;

	spec_stop_avx512;

	pushq %r12;
	CFI_PUSH(%r12);
	pushq %r13;
	CFI_PUSH(%r13);
	pushq %r14;
	CFI_PUSH(%r14);
	pushq %rbx;
	CFI_PUSH(%rbx);

#define OFFSET_PTR_Q   0+5*8(%rsp)
#define CHECKSUM_PTR_Q 8+5*8(%rsp)
#define L_ARRAY_PTR_L  16+5*8(%rsp)
#define OPER_MODE_L    24+5*8(%rsp)

	movq OFFSET_PTR_Q, %r13; /* offset ptr. */
	movq L_ARRAY_PTR_L, %r14; /* L-array ptr. */
	movl OPER_MODE_L, %ebx; /* decrypt/encrypt/auth-mode. */
	movq CHECKSUM_PTR_Q, %r12; /* checksum ptr. */

	leal (, %r9d, 4), %eax;
	vmovdqu (%r13), %xmm15; /* Load offset. */
	vmovdqa (0 * 16)(%rdi), %xmm0; /* first key */
	vpxor (%rdi, %rax, 4), %xmm0, %xmm0; /* first key ^ last key */
	vpxor (0 * 16)(%rdi), %xmm15, %xmm15; /* offset ^ first key */
	vshufi32x4 $0, %zmm0, %zmm0, %zmm30;
	vpxord %ymm29, %ymm29, %ymm29;

	vshufi32x4 $0, %zmm15, %zmm15, %zmm15;

	/* Prepare L-array optimization.
	 * Since nblk is aligned to 16, offsets will have following
	 * construction:
	 *  - block1 = ntz{0} = offset ^ L[0]
	 *  - block2 = ntz{1} = offset ^ L[0] ^ L[1]
	 *  - block3 = ntz{0} = offset ^ L[1]
	 *  - block4 = ntz{2} = offset ^ L[1] ^ L[2]
	 *  => zmm20
	 *
	 *  - block5 = ntz{0} = offset ^ L[0] ^ L[1] ^ L[2]
	 *  - block6 = ntz{1} = offset ^ L[0] ^ L[2]
	 *  - block7 = ntz{0} = offset ^ L[2]
	 *  - block8 = ntz{3} = offset ^ L[2] ^ L[3]
	 *  => zmm21
	 *
	 *  - block9 = ntz{0} = offset ^ L[0] ^ L[2] ^ L[3]
	 *  - block10 = ntz{1} = offset ^ L[0] ^ L[1] ^ L[2] ^ L[3]
	 *  - block11 = ntz{0} = offset ^ L[1] ^ L[2] ^ L[3]
	 *  - block12 = ntz{2} = offset ^ L[1] ^ L[3]
	 *  => zmm22
	 *
	 *  - block13 = ntz{0} = offset ^ L[0] ^ L[1] ^ L[3]
	 *  - block14 = ntz{1} = offset ^ L[0] ^ L[3]
	 *  - block15 = ntz{0} = offset ^ L[3]
	 *  - block16 = ntz{4} = offset ^ L[3] ^ L[4]
	 *  => zmm23
	 */
	vmovdqu (0 * 16)(%r14), %xmm0; /* L[0] */
	vmovdqu (1 * 16)(%r14), %xmm1; /* L[1] */
	vmovdqu (2 * 16)(%r14), %xmm2; /* L[2] */
	vmovdqu (3 * 16)(%r14), %xmm3; /* L[3] */
	vmovdqu32 (4 * 16)(%r14), %xmm16; /* L[4] */
	vpxor %xmm0, %xmm1, %xmm4; /* L[0] ^ L[1] */
	vpxor %xmm0, %xmm2, %xmm5; /* L[0] ^ L[2] */
	vpxor %xmm0, %xmm3, %xmm6; /* L[0] ^ L[3] */
	vpxor %xmm1, %xmm2, %xmm7; /* L[1] ^ L[2] */
	vpxor %xmm1, %xmm3, %xmm8; /* L[1] ^ L[3] */
	vpxor %xmm2, %xmm3, %xmm9; /* L[2] ^ L[3] */
	vpxord %xmm16, %xmm3, %xmm17; /* L[3] ^ L[4] */
	vpxor %xmm4, %xmm2, %xmm10; /* L[0] ^ L[1] ^ L[2] */
	vpxor %xmm5, %xmm3, %xmm11; /* L[0] ^ L[2] ^ L[3] */
	vpxor %xmm7, %xmm3, %xmm12; /* L[1] ^ L[2] ^ L[3] */
	vpxor %xmm0, %xmm8, %xmm13; /* L[0] ^ L[1] ^ L[3] */
	vpxor %xmm4, %xmm9, %xmm14; /* L[0] ^ L[1] ^ L[2] ^ L[3] */
	vinserti128 $1, %xmm4, %ymm0, %ymm0;
	vinserti128 $1, %xmm7, %ymm1, %ymm1;
	vinserti32x8 $1, %ymm1, %zmm0, %zmm20;
	vinserti128 $1, %xmm5, %ymm10, %ymm10;
	vinserti128 $1, %xmm9, %ymm2, %ymm2;
	vinserti32x8 $1, %ymm2, %zmm10, %zmm21;
	vinserti128 $1, %xmm14, %ymm11, %ymm11;
	vinserti128 $1, %xmm8, %ymm12, %ymm12;
	vinserti32x8 $1, %ymm12, %zmm11, %zmm22;
	vinserti128 $1, %xmm6, %ymm13, %ymm13;
	vinserti32x4 $1, %xmm17, %ymm3, %ymm23;
	vinserti32x8 $1, %ymm23, %zmm13, %zmm23;

	/*
	 *  - block17 = ntz{0} = offset ^ L[0] ^ L[3] ^ L[4]
	 *  - block18 = ntz{1} = offset ^ L[0] ^ L[1] ^ L[3] ^ L[4]
	 *  - block19 = ntz{0} = offset ^ L[1] ^ L[3] ^ L[4]
	 *  - block20 = ntz{2} = offset ^ L[1] ^ L[2] ^ L[3] ^ L[4]
	 *  => zmm24
	 *
	 *  - block21 = ntz{0} = offset ^ L[0] ^ L[1] ^ L[2] ^ L[3] ^ L[4]
	 *  - block22 = ntz{1} = offset ^ L[0] ^ L[2] ^ L[3] ^ L[4]
	 *  - block23 = ntz{0} = offset ^ L[2] ^ L[3] ^ L[4]
	 *  - block24 = ntz{3} = offset ^ L[2] ^ L[4]
	 *  => zmm25
	 *
	 *  - block25 = ntz{0} = offset ^ L[0] ^ L[2] ^ L[4]
	 *  - block26 = ntz{1} = offset ^ L[0] ^ L[1] ^ L[2] ^ L[4]
	 *  - block27 = ntz{0} = offset ^ L[1] ^ L[2] ^ L[4]
	 *  - block28 = ntz{2} = offset ^ L[1] ^ L[4]
	 *  => zmm26
	 *
	 *  - block29 = ntz{0} = offset ^ L[0] ^ L[1] ^ L[4]
	 *  - block30 = ntz{1} = offset ^ L[0] ^ L[4]
	 *  - block31 = ntz{0} = offset ^ L[4]
	 *  - block32 = 0 (later filled with ntz{x} = offset ^ L[4] ^ L[ntz{x}])
	 *  => zmm16
	 */
	vpxord %xmm16, %xmm0, %xmm0; /* L[0] ^ L[4] */
	vpxord %xmm16, %xmm1, %xmm1; /* L[1] ^ L[4] */
	vpxord %xmm16, %xmm2, %xmm2; /* L[2] ^ L[4] */
	vpxord %xmm16, %xmm4, %xmm4; /* L[0] ^ L[1] ^ L[4] */
	vpxord %xmm16, %xmm5, %xmm5; /* L[0] ^ L[2] ^ L[4] */
	vpxord %xmm16, %xmm6, %xmm6; /* L[0] ^ L[3] ^ L[4] */
	vpxord %xmm16, %xmm7, %xmm7; /* L[1] ^ L[2] ^ L[4] */
	vpxord %xmm16, %xmm8, %xmm8; /* L[1] ^ L[3] ^ L[4] */
	vpxord %xmm16, %xmm9, %xmm9; /* L[2] ^ L[3] ^ L[4] */
	vpxord %xmm16, %xmm10, %xmm10; /* L[0] ^ L[1] ^ L[2] ^ L[4] */
	vpxord %xmm16, %xmm11, %xmm11; /* L[0] ^ L[2] ^ L[3] ^ L[4] */
	vpxord %xmm16, %xmm12, %xmm12; /* L[1] ^ L[2] ^ L[3] ^ L[4] */
	vpxord %xmm16, %xmm13, %xmm13; /* L[0] ^ L[1] ^ L[3] ^ L[4] */
	vpxord %xmm16, %xmm14, %xmm14; /* L[0] ^ L[1] ^ L[2] ^ L[3] ^ L[4] */
	vinserti128 $1, %xmm13, %ymm6, %ymm6;
	vinserti32x4 $1, %xmm12, %ymm8, %ymm24;
	vinserti32x8 $1, %ymm24, %zmm6, %zmm24;
	vinserti128 $1, %xmm11, %ymm14, %ymm14;
	vinserti32x4 $1, %xmm2, %ymm9, %ymm25;
	vinserti32x8 $1, %ymm25, %zmm14, %zmm25;
	vinserti128 $1, %xmm10, %ymm5, %ymm5;
	vinserti32x4 $1, %xmm1, %ymm7, %ymm26;
	vinserti32x8 $1, %ymm26, %zmm5, %zmm26;
	vinserti128 $1, %xmm0, %ymm4, %ymm4;
	vinserti32x8 $1, %ymm16, %zmm4, %zmm16;

	/* Aligned: Process 32 blocks per loop. */
.align 16
.Locb_aligned_blk32:
	cmpq $32, %r8;
	jb .Locb_aligned_blk16;

	leaq -32(%r8), %r8;

	leal 32(%esi), %esi;
	tzcntl %esi, %eax;
	shll $4, %eax;

	vpxord %zmm20, %zmm15, %zmm8;
	vpxord %zmm21, %zmm15, %zmm9;
	vpxord %zmm22, %zmm15, %zmm10;
	vpxord %zmm23, %zmm15, %zmm11;
	vpxord %zmm24, %zmm15, %zmm12;
	vpxord %zmm25, %zmm15, %zmm27;
	vpxord %zmm26, %zmm15, %zmm28;

	vmovdqa (4 * 16)(%r14), %xmm14;
	vpxor (%r14, %rax), %xmm14, %xmm14; /* L[4] ^ L[ntz{nblk+16}] */
	vinserti32x4 $3, %xmm14, %zmm16, %zmm14;

	vpxord %zmm14, %zmm15, %zmm14;

	cmpl $1, %ebx;
	jb .Locb_aligned_blk32_dec;
	ja .Locb_aligned_blk32_auth;
		vmovdqu32 (0 * 16)(%rcx), %zmm17;
		vpxord %zmm17, %zmm8, %zmm0;
		vmovdqu32 (4 * 16)(%rcx), %zmm18;
		vpxord %zmm18, %zmm9, %zmm1;
		vmovdqu32 (8 * 16)(%rcx), %zmm19;
		vpxord %zmm19, %zmm10, %zmm2;
		vmovdqu32 (12 * 16)(%rcx), %zmm31;
		vpxord %zmm31, %zmm11, %zmm3;

		vpternlogd $0x96, %zmm17, %zmm18, %zmm19;

		vmovdqu32 (16 * 16)(%rcx), %zmm17;
		vpxord %zmm17, %zmm12, %zmm4;
		vmovdqu32 (20 * 16)(%rcx), %zmm18;
		vpxord %zmm18, %zmm27, %zmm5;

		vpternlogd $0x96, %zmm31, %zmm17, %zmm18;

		vmovdqu32 (24 * 16)(%rcx), %zmm31;
		vpxord %zmm31, %zmm28, %zmm6;
		vmovdqu32 (28 * 16)(%rcx), %zmm17;
		vpxord %zmm17, %zmm14, %zmm7;
		leaq (32 * 16)(%rcx), %rcx;

		vpternlogd $0x96, %zmm31, %zmm17, %zmm19;
		vpternlogd $0x96, %zmm18, %zmm19, %zmm29;

		vbroadcasti32x4 (1 * 16)(%rdi), %zmm13;

		vpxord %zmm8, %zmm30, %zmm8;
		vpxord %zmm9, %zmm30, %zmm9;
		vpxord %zmm10, %zmm30, %zmm10;
		vpxord %zmm11, %zmm30, %zmm11;
		vpxord %zmm12, %zmm30, %zmm12;
		vpxord %zmm27, %zmm30, %zmm27;
		vpxord %zmm28, %zmm30, %zmm28;
		vshufi32x4 $0b11111111, %zmm14, %zmm14, %zmm15;
		vpxord %zmm14, %zmm30, %zmm14;

		/* AES rounds */
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		cmpl $12, %r9d;
		jb .Locb_aligned_blk32_enc_last;
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		jz .Locb_aligned_blk32_enc_last;
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);

		/* Last round and output handling. */
	.align 16
	.Locb_aligned_blk32_enc_last:
		vaesenclast %zmm8, %zmm0, %zmm0;
		vaesenclast %zmm9, %zmm1, %zmm1;
		vaesenclast %zmm10, %zmm2, %zmm2;
		vaesenclast %zmm11, %zmm3, %zmm3;
		vaesenclast %zmm12, %zmm4, %zmm4;
		vaesenclast %zmm27, %zmm5, %zmm5;
		vaesenclast %zmm28, %zmm6, %zmm6;
		vaesenclast %zmm14, %zmm7, %zmm7;
		vmovdqu32 %zmm0, (0 * 16)(%rdx);
		vmovdqu32 %zmm1, (4 * 16)(%rdx);
		vmovdqu32 %zmm2, (8 * 16)(%rdx);
		vmovdqu32 %zmm3, (12 * 16)(%rdx);
		vmovdqu32 %zmm4, (16 * 16)(%rdx);
		vmovdqu32 %zmm5, (20 * 16)(%rdx);
		vmovdqu32 %zmm6, (24 * 16)(%rdx);
		vmovdqu32 %zmm7, (28 * 16)(%rdx);
		leaq (32 * 16)(%rdx), %rdx;

		jmp .Locb_aligned_blk32;

	.align 16
	.Locb_aligned_blk32_auth:
		vpxord (0 * 16)(%rcx), %zmm8, %zmm0;
		vpxord (4 * 16)(%rcx), %zmm9, %zmm1;
		vpxord (8 * 16)(%rcx), %zmm10, %zmm2;
		vpxord (12 * 16)(%rcx), %zmm11, %zmm3;
		vpxord (16 * 16)(%rcx), %zmm12, %zmm4;
		vpxord (20 * 16)(%rcx), %zmm27, %zmm5;
		vpxord (24 * 16)(%rcx), %zmm28, %zmm6;
		vpxord (28 * 16)(%rcx), %zmm14, %zmm7;
		leaq (32 * 16)(%rcx), %rcx;

		vbroadcasti32x4 (1 * 16)(%rdi), %zmm13;

		vshufi32x4 $0b11111111, %zmm14, %zmm14, %zmm15;

		/* AES rounds */
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm13;
		cmpl $12, %r9d;
		jb .Locb_aligned_blk32_auth_last;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm13;
		jz .Locb_aligned_blk32_auth_last;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (14 * 16)(%rdi), %zmm13;

		/* Last round and output handling. */
	.align 16
	.Locb_aligned_blk32_auth_last:
		vaesenclast %zmm13, %zmm0, %zmm0;
		vaesenclast %zmm13, %zmm1, %zmm1;
		vaesenclast %zmm13, %zmm2, %zmm2;
		vaesenclast %zmm13, %zmm3, %zmm3;
		vaesenclast %zmm13, %zmm4, %zmm4;
		vaesenclast %zmm13, %zmm5, %zmm5;
		vaesenclast %zmm13, %zmm6, %zmm6;
		vaesenclast %zmm13, %zmm7, %zmm7;

		vpternlogd $0x96, %zmm0, %zmm1, %zmm2;
		vpternlogd $0x96, %zmm3, %zmm4, %zmm5;
		vpternlogd $0x96, %zmm6, %zmm7, %zmm29;
		vpternlogd $0x96, %zmm2, %zmm5, %zmm29;

		jmp .Locb_aligned_blk32;

	.align 16
	.Locb_aligned_blk32_dec:
		vpxord (0 * 16)(%rcx), %zmm8, %zmm0;
		vpxord (4 * 16)(%rcx), %zmm9, %zmm1;
		vpxord (8 * 16)(%rcx), %zmm10, %zmm2;
		vpxord (12 * 16)(%rcx), %zmm11, %zmm3;
		vpxord (16 * 16)(%rcx), %zmm12, %zmm4;
		vpxord (20 * 16)(%rcx), %zmm27, %zmm5;
		vpxord (24 * 16)(%rcx), %zmm28, %zmm6;
		vpxord (28 * 16)(%rcx), %zmm14, %zmm7;
		leaq (32 * 16)(%rcx), %rcx;

		vbroadcasti32x4 (1 * 16)(%rdi), %zmm13;

		vpxord %zmm8, %zmm30, %zmm8;
		vpxord %zmm9, %zmm30, %zmm9;
		vpxord %zmm10, %zmm30, %zmm10;
		vpxord %zmm11, %zmm30, %zmm11;
		vpxord %zmm12, %zmm30, %zmm12;
		vpxord %zmm27, %zmm30, %zmm27;
		vpxord %zmm28, %zmm30, %zmm28;
		vshufi32x4 $0b11111111, %zmm14, %zmm14, %zmm15;
		vpxord %zmm14, %zmm30, %zmm14;

		/* AES rounds */
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		cmpl $12, %r9d;
		jb .Locb_aligned_blk32_dec_last;
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		jz .Locb_aligned_blk32_dec_last;
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm13;
		VAESDEC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);

		/* Last round and output handling. */
	.align 16
	.Locb_aligned_blk32_dec_last:
		vaesdeclast %zmm8, %zmm0, %zmm0;
		vaesdeclast %zmm9, %zmm1, %zmm1;
		vaesdeclast %zmm10, %zmm2, %zmm2;
		vaesdeclast %zmm11, %zmm3, %zmm3;
		vaesdeclast %zmm12, %zmm4, %zmm4;
		vaesdeclast %zmm27, %zmm5, %zmm5;
		vaesdeclast %zmm28, %zmm6, %zmm6;
		vaesdeclast %zmm14, %zmm7, %zmm7;
		vmovdqu32 %zmm0, (0 * 16)(%rdx);
		vmovdqu32 %zmm1, (4 * 16)(%rdx);
		vmovdqu32 %zmm2, (8 * 16)(%rdx);
		vmovdqu32 %zmm3, (12 * 16)(%rdx);
		vmovdqu32 %zmm4, (16 * 16)(%rdx);
		vmovdqu32 %zmm5, (20 * 16)(%rdx);
		vmovdqu32 %zmm6, (24 * 16)(%rdx);
		vmovdqu32 %zmm7, (28 * 16)(%rdx);
		leaq (32 * 16)(%rdx), %rdx;

		vpternlogd $0x96, %zmm0, %zmm1, %zmm2;
		vpternlogd $0x96, %zmm3, %zmm4, %zmm5;
		vpternlogd $0x96, %zmm6, %zmm7, %zmm29;
		vpternlogd $0x96, %zmm2, %zmm5, %zmm29;

		jmp .Locb_aligned_blk32;

	/* Aligned: Process trailing 16 blocks. */
.align 16
.Locb_aligned_blk16:
	cmpq $16, %r8;
	jb .Locb_aligned_done;

	leaq -16(%r8), %r8;

	leal 16(%esi), %esi;

	vpxord %zmm20, %zmm15, %zmm8;
	vpxord %zmm21, %zmm15, %zmm9;
	vpxord %zmm22, %zmm15, %zmm10;
	vpxord %zmm23, %zmm15, %zmm14;

	cmpl $1, %ebx;
	jb .Locb_aligned_blk16_dec;
	ja .Locb_aligned_blk16_auth;
		vmovdqu32 (0 * 16)(%rcx), %zmm17;
		vpxord %zmm17, %zmm8, %zmm0;
		vmovdqu32 (4 * 16)(%rcx), %zmm18;
		vpxord %zmm18, %zmm9, %zmm1;
		vmovdqu32 (8 * 16)(%rcx), %zmm19;
		vpxord %zmm19, %zmm10, %zmm2;
		vmovdqu32 (12 * 16)(%rcx), %zmm31;
		vpxord %zmm31, %zmm14, %zmm3;
		leaq (16 * 16)(%rcx), %rcx;

		vpternlogd $0x96, %zmm17, %zmm18, %zmm19;
		vpternlogd $0x96, %zmm31, %zmm19, %zmm29;

		vbroadcasti32x4 (1 * 16)(%rdi), %zmm13;

		vpxord %zmm8, %zmm30, %zmm8;
		vpxord %zmm9, %zmm30, %zmm9;
		vpxord %zmm10, %zmm30, %zmm10;
		vshufi32x4 $0b11111111, %zmm14, %zmm14, %zmm15;
		vpxord %zmm14, %zmm30, %zmm14;

		/* AES rounds */
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		cmpl $12, %r9d;
		jb .Locb_aligned_blk16_enc_last;
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		jz .Locb_aligned_blk16_enc_last;
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm13;
		VAESENC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);

		/* Last round and output handling. */
	.align 16
	.Locb_aligned_blk16_enc_last:
		vaesenclast %zmm8, %zmm0, %zmm0;
		vaesenclast %zmm9, %zmm1, %zmm1;
		vaesenclast %zmm10, %zmm2, %zmm2;
		vaesenclast %zmm14, %zmm3, %zmm3;
		vmovdqu32 %zmm0, (0 * 16)(%rdx);
		vmovdqu32 %zmm1, (4 * 16)(%rdx);
		vmovdqu32 %zmm2, (8 * 16)(%rdx);
		vmovdqu32 %zmm3, (12 * 16)(%rdx);
		leaq (16 * 16)(%rdx), %rdx;

		jmp .Locb_aligned_done;

	.align 16
	.Locb_aligned_blk16_auth:
		vpxord (0 * 16)(%rcx), %zmm8, %zmm0;
		vpxord (4 * 16)(%rcx), %zmm9, %zmm1;
		vpxord (8 * 16)(%rcx), %zmm10, %zmm2;
		vpxord (12 * 16)(%rcx), %zmm14, %zmm3;
		leaq (16 * 16)(%rcx), %rcx;

		vbroadcasti32x4 (1 * 16)(%rdi), %zmm13;

		vshufi32x4 $0b11111111, %zmm14, %zmm14, %zmm15;

		/* AES rounds */
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm13;
		cmpl $12, %r9d;
		jb .Locb_aligned_blk16_auth_last;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm13;
		jz .Locb_aligned_blk16_auth_last;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm13;
		VAESENC8(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (14 * 16)(%rdi), %zmm13;

		/* Last round and output handling. */
	.align 16
	.Locb_aligned_blk16_auth_last:
		vaesenclast %zmm13, %zmm0, %zmm0;
		vaesenclast %zmm13, %zmm1, %zmm1;
		vaesenclast %zmm13, %zmm2, %zmm2;
		vaesenclast %zmm13, %zmm3, %zmm3;

		vpternlogd $0x96, %zmm0, %zmm1, %zmm2;
		vpternlogd $0x96, %zmm3, %zmm2, %zmm29;

		jmp .Locb_aligned_done;

	.align 16
	.Locb_aligned_blk16_dec:
		vpxord (0 * 16)(%rcx), %zmm8, %zmm0;
		vpxord (4 * 16)(%rcx), %zmm9, %zmm1;
		vpxord (8 * 16)(%rcx), %zmm10, %zmm2;
		vpxord (12 * 16)(%rcx), %zmm14, %zmm3;
		leaq (16 * 16)(%rcx), %rcx;

		vbroadcasti32x4 (1 * 16)(%rdi), %zmm13;

		vpxord %zmm8, %zmm30, %zmm8;
		vpxord %zmm9, %zmm30, %zmm9;
		vpxord %zmm10, %zmm30, %zmm10;
		vshufi32x4 $0b11111111, %zmm14, %zmm14, %zmm15;
		vpxord %zmm14, %zmm30, %zmm14;

		/* AES rounds */
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		cmpl $12, %r9d;
		jb .Locb_aligned_blk16_dec_last;
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		jz .Locb_aligned_blk16_dec_last;
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm13;
		VAESDEC4(%zmm13, %zmm0, %zmm1, %zmm2, %zmm3);

		/* Last round and output handling. */
	.align 16
	.Locb_aligned_blk16_dec_last:
		vaesdeclast %zmm8, %zmm0, %zmm0;
		vaesdeclast %zmm9, %zmm1, %zmm1;
		vaesdeclast %zmm10, %zmm2, %zmm2;
		vaesdeclast %zmm14, %zmm3, %zmm3;
		vmovdqu32 %zmm0, (0 * 16)(%rdx);
		vmovdqu32 %zmm1, (4 * 16)(%rdx);
		vmovdqu32 %zmm2, (8 * 16)(%rdx);
		vmovdqu32 %zmm3, (12 * 16)(%rdx);
		leaq (16 * 16)(%rdx), %rdx;

		vpternlogd $0x96, %zmm0, %zmm1, %zmm2;
		vpternlogd $0x96, %zmm3, %zmm2, %zmm29;

.align 16
.Locb_aligned_done:
	vpxor (0 * 16)(%rdi), %xmm15, %xmm15; /* offset ^ first key ^ first key */

	vextracti32x8 $1, %zmm29, %ymm0;
	vpxord %ymm29, %ymm0, %ymm0;
	vextracti128 $1, %ymm0, %xmm1;
	vpternlogd $0x96, (%r12), %xmm1, %xmm0;
	vmovdqu %xmm0, (%r12);

	vmovdqu %xmm15, (%r13); /* Store offset. */

	popq %rbx;
	CFI_POP(%rbx);
	popq %r14;
	CFI_POP(%r14);
	popq %r13;
	CFI_POP(%r13);
	popq %r12;
	CFI_POP(%r12);

	/* Clear used AVX512 registers. */
	vpxord %ymm16, %ymm16, %ymm16;
	vpxord %ymm17, %ymm17, %ymm17;
	vpxord %ymm18, %ymm18, %ymm18;
	vpxord %ymm19, %ymm19, %ymm19;
	vpxord %ymm20, %ymm20, %ymm20;
	vpxord %ymm21, %ymm21, %ymm21;
	vpxord %ymm22, %ymm22, %ymm22;
	vpxord %ymm23, %ymm23, %ymm23;
	vzeroall;
	vpxord %ymm24, %ymm24, %ymm24;
	vpxord %ymm25, %ymm25, %ymm25;
	vpxord %ymm26, %ymm26, %ymm26;
	vpxord %ymm27, %ymm27, %ymm27;
	vpxord %ymm28, %ymm28, %ymm28;
	vpxord %ymm29, %ymm29, %ymm29;
	vpxord %ymm30, %ymm30, %ymm30;
	vpxord %ymm31, %ymm31, %ymm31;

.align 16
.Locb_skip_avx512:
	/* Handle trailing blocks with AVX2 implementation. */
	cmpq $0, %r8;
	ja _gcry_vaes_avx2_ocb_crypt_amd64 AT_PLT;

	xorl %eax, %eax;
	ret_spec_stop

#undef STACK_REGS_POS
#undef STACK_ALLOC

	CFI_ENDPROC();
ELF(.size _gcry_vaes_avx512_ocb_aligned_crypt_amd64,
	  .-_gcry_vaes_avx512_ocb_aligned_crypt_amd64)

/**********************************************************************
  XTS-mode encryption
 **********************************************************************/
ELF(.type _gcry_vaes_avx512_xts_crypt_amd64,@function)
.globl _gcry_vaes_avx512_xts_crypt_amd64
.align 16
_gcry_vaes_avx512_xts_crypt_amd64:
	/* input:
	 *	%rdi: round keys
	 *	%rsi: tweak
	 *	%rdx: dst
	 *	%rcx: src
	 *	%r8:  nblocks
	 *	%r9:  nrounds
	 *	8(%rsp): encrypt
	 */
	CFI_STARTPROC();

	cmpq $16, %r8;
	jb .Lxts_crypt_skip_avx512;

	spec_stop_avx512;

	/* Load first and last key. */
	leal (, %r9d, 4), %eax;
	vbroadcasti32x4 (%rdi), %zmm30;
	vbroadcasti32x4 (%rdi, %rax, 4), %zmm31;

	movl 8(%rsp), %eax;

	vpmovzxbd .Lxts_gfmul_clmul_bd rRIP, %zmm20;
	vbroadcasti32x4 .Lxts_high_bit_shuf rRIP, %zmm21;

#define tweak_clmul(shift, out, tweak, hi_tweak, gfmul_clmul, tmp1, tmp2) \
	vpsrld $(32-(shift)), hi_tweak, tmp2; \
	vpsllq $(shift), tweak, out; \
	vpclmulqdq $0, gfmul_clmul, tmp2, tmp1; \
	vpunpckhqdq tmp2, tmp1, tmp1; \
	vpxord tmp1, out, out;

	/* Prepare tweak. */
	vmovdqu (%rsi), %xmm15;
	vpshufb %xmm21, %xmm15, %xmm13;
	tweak_clmul(1, %xmm11, %xmm15, %xmm13, %xmm20, %xmm0, %xmm1);
	vinserti128 $1, %xmm11, %ymm15, %ymm15; /* tweak:tweak1 */
	vpshufb %ymm21, %ymm15, %ymm13;
	tweak_clmul(2, %ymm11, %ymm15, %ymm13, %ymm20, %ymm0, %ymm1);
	vinserti32x8 $1, %ymm11, %zmm15, %zmm15; /* tweak:tweak1:tweak2:tweak3 */
	vpshufb %zmm21, %zmm15, %zmm13;

	cmpq $16, %r8;
	jb .Lxts_crypt_done;

	/* Process 16 blocks per loop. */
	leaq -16(%r8), %r8;

	vmovdqa32 %zmm15, %zmm5;
	tweak_clmul(4, %zmm6, %zmm15, %zmm13, %zmm20, %zmm0, %zmm1);
	tweak_clmul(8, %zmm7, %zmm15, %zmm13, %zmm20, %zmm0, %zmm1);
	tweak_clmul(12, %zmm8, %zmm15, %zmm13, %zmm20, %zmm0, %zmm1);
	tweak_clmul(16, %zmm15, %zmm15, %zmm13, %zmm20, %zmm0, %zmm1);
	vpshufb %zmm21, %zmm15, %zmm13;

	vmovdqu32 (0 * 16)(%rcx), %zmm0;
	vmovdqu32 (4 * 16)(%rcx), %zmm1;
	vmovdqu32 (8 * 16)(%rcx), %zmm2;
	vmovdqu32 (12 * 16)(%rcx), %zmm3;
	leaq (16 * 16)(%rcx), %rcx;
	vpternlogd $0x96, %zmm30, %zmm5, %zmm0;
	vpternlogd $0x96, %zmm30, %zmm6, %zmm1;
	vpternlogd $0x96, %zmm30, %zmm7, %zmm2;
	vpternlogd $0x96, %zmm30, %zmm8, %zmm3;

.align 16
.Lxts_crypt_blk16_loop:
	cmpq $16, %r8;
	jb .Lxts_crypt_blk16_tail;
		leaq -16(%r8), %r8;

		testl %eax, %eax;
		jz .Lxts_dec_blk16;
			/* AES rounds */
			vbroadcasti32x4 (1 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			  vmovdqa32 %zmm15, %zmm9;
			  tweak_clmul(4, %zmm10, %zmm15, %zmm13, %zmm20, %zmm12, %zmm14);
			vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			  tweak_clmul(8, %zmm11, %zmm15, %zmm13, %zmm20, %zmm12, %zmm14);
			vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			cmpl $12, %r9d;
			jb .Lxts_enc_blk16_last;
			vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			jz .Lxts_enc_blk16_last;
			vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);

			/* Last round and output handling. */
		.align 16
		.Lxts_enc_blk16_last:
			vpxord %zmm31, %zmm5, %zmm5; /* Xor tweak to last round key. */
			vpxord %zmm31, %zmm6, %zmm6;
			vpxord %zmm31, %zmm7, %zmm7;
			vpxord %zmm31, %zmm8, %zmm4;
			  tweak_clmul(12, %zmm8, %zmm15, %zmm13, %zmm20, %zmm12, %zmm14);
			vaesenclast %zmm5, %zmm0, %zmm16;
			vaesenclast %zmm6, %zmm1, %zmm17;
			vaesenclast %zmm7, %zmm2, %zmm18;
			vaesenclast %zmm4, %zmm3, %zmm19;
			  tweak_clmul(16, %zmm15, %zmm15, %zmm13, %zmm20, %zmm12, %zmm14);
			  vpshufb %zmm21, %zmm15, %zmm13;

			vmovdqu32 (0 * 16)(%rcx), %zmm0;
			vmovdqu32 (4 * 16)(%rcx), %zmm1;
			vmovdqu32 (8 * 16)(%rcx), %zmm2;
			vmovdqu32 (12 * 16)(%rcx), %zmm3;
			leaq (16 * 16)(%rcx), %rcx;

			vmovdqu32 %zmm16, (0 * 16)(%rdx);
			vmovdqu32 %zmm17, (4 * 16)(%rdx);
			vmovdqu32 %zmm18, (8 * 16)(%rdx);
			vmovdqu32 %zmm19, (12 * 16)(%rdx);
			leaq (16 * 16)(%rdx), %rdx;

			vpternlogd $0x96, %zmm30, %zmm9, %zmm0;
			vpternlogd $0x96, %zmm30, %zmm10, %zmm1;
			vpternlogd $0x96, %zmm30, %zmm11, %zmm2;
			vpternlogd $0x96, %zmm30, %zmm8, %zmm3;

			  vmovdqa32 %zmm9, %zmm5;
			  vmovdqa32 %zmm10, %zmm6;
			  vmovdqa32 %zmm11, %zmm7;

			jmp .Lxts_crypt_blk16_loop;

		.align 16
		.Lxts_dec_blk16:
			/* AES rounds */
			vbroadcasti32x4 (1 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			  vmovdqa32 %zmm15, %zmm9;
			  tweak_clmul(4, %zmm10, %zmm15, %zmm13, %zmm20, %zmm12, %zmm14);
			vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			  tweak_clmul(8, %zmm11, %zmm15, %zmm13, %zmm20, %zmm12, %zmm14);
			vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			cmpl $12, %r9d;
			jb .Lxts_dec_blk16_last;
			vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			jz .Lxts_dec_blk16_last;
			vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);

			/* Last round and output handling. */
		.align 16
		.Lxts_dec_blk16_last:
			vpxord %zmm31, %zmm5, %zmm5; /* Xor tweak to last round key. */
			vpxord %zmm31, %zmm6, %zmm6;
			vpxord %zmm31, %zmm7, %zmm7;
			vpxord %zmm31, %zmm8, %zmm4;
			  tweak_clmul(12, %zmm8, %zmm15, %zmm13, %zmm20, %zmm12, %zmm14);
			vaesdeclast %zmm5, %zmm0, %zmm16;
			vaesdeclast %zmm6, %zmm1, %zmm17;
			vaesdeclast %zmm7, %zmm2, %zmm18;
			vaesdeclast %zmm4, %zmm3, %zmm19;
			  tweak_clmul(16, %zmm15, %zmm15, %zmm13, %zmm20, %zmm12, %zmm14);
			  vpshufb %zmm21, %zmm15, %zmm13;

			vmovdqu32 (0 * 16)(%rcx), %zmm0;
			vmovdqu32 (4 * 16)(%rcx), %zmm1;
			vmovdqu32 (8 * 16)(%rcx), %zmm2;
			vmovdqu32 (12 * 16)(%rcx), %zmm3;
			leaq (16 * 16)(%rcx), %rcx;

			vmovdqu32 %zmm16, (0 * 16)(%rdx);
			vmovdqu32 %zmm17, (4 * 16)(%rdx);
			vmovdqu32 %zmm18, (8 * 16)(%rdx);
			vmovdqu32 %zmm19, (12 * 16)(%rdx);
			leaq (16 * 16)(%rdx), %rdx;

			vpternlogd $0x96, %zmm30, %zmm9, %zmm0;
			vpternlogd $0x96, %zmm30, %zmm10, %zmm1;
			vpternlogd $0x96, %zmm30, %zmm11, %zmm2;
			vpternlogd $0x96, %zmm30, %zmm8, %zmm3;

			  vmovdqa32 %zmm9, %zmm5;
			  vmovdqa32 %zmm10, %zmm6;
			  vmovdqa32 %zmm11, %zmm7;

			jmp .Lxts_crypt_blk16_loop;

	.align 16
	.Lxts_crypt_blk16_tail:
		testl %eax, %eax;
		jz .Lxts_dec_tail_blk16;
			/* AES rounds */
			vbroadcasti32x4 (1 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			cmpl $12, %r9d;
			jb .Lxts_enc_blk16_tail_last;
			vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			jz .Lxts_enc_blk16_tail_last;
			vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
			VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);

			/* Last round and output handling. */
		.align 16
		.Lxts_enc_blk16_tail_last:
			vpxord %zmm31, %zmm5, %zmm5; /* Xor tweak to last round key. */
			vpxord %zmm31, %zmm6, %zmm6;
			vpxord %zmm31, %zmm7, %zmm7;
			vpxord %zmm31, %zmm8, %zmm4;
			vaesenclast %zmm5, %zmm0, %zmm0;
			vaesenclast %zmm6, %zmm1, %zmm1;
			vaesenclast %zmm7, %zmm2, %zmm2;
			vaesenclast %zmm4, %zmm3, %zmm3;
			vmovdqu32 %zmm0, (0 * 16)(%rdx);
			vmovdqu32 %zmm1, (4 * 16)(%rdx);
			vmovdqu32 %zmm2, (8 * 16)(%rdx);
			vmovdqu32 %zmm3, (12 * 16)(%rdx);
			leaq (16 * 16)(%rdx), %rdx;

			jmp .Lxts_crypt_done;

		.align 16
		.Lxts_dec_tail_blk16:
			/* AES rounds */
			vbroadcasti32x4 (1 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			cmpl $12, %r9d;
			jb .Lxts_dec_blk16_tail_last;
			vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			jz .Lxts_dec_blk16_tail_last;
			vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
			vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
			VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);

			/* Last round and output handling. */
		.align 16
		.Lxts_dec_blk16_tail_last:
			vpxord %zmm31, %zmm5, %zmm5; /* Xor tweak to last round key. */
			vpxord %zmm31, %zmm6, %zmm6;
			vpxord %zmm31, %zmm7, %zmm7;
			vpxord %zmm31, %zmm8, %zmm4;
			vaesdeclast %zmm5, %zmm0, %zmm0;
			vaesdeclast %zmm6, %zmm1, %zmm1;
			vaesdeclast %zmm7, %zmm2, %zmm2;
			vaesdeclast %zmm4, %zmm3, %zmm3;
			vmovdqu32 %zmm0, (0 * 16)(%rdx);
			vmovdqu32 %zmm1, (4 * 16)(%rdx);
			vmovdqu32 %zmm2, (8 * 16)(%rdx);
			vmovdqu32 %zmm3, (12 * 16)(%rdx);
			leaq (16 * 16)(%rdx), %rdx;

.align 16
.Lxts_crypt_done:
	/* Store IV. */
	vmovdqu %xmm15, (%rsi);

	/* Clear used AVX512 registers. */
	vpxord %ymm16, %ymm16, %ymm16;
	vpxord %ymm17, %ymm17, %ymm17;
	vpxord %ymm18, %ymm18, %ymm18;
	vpxord %ymm19, %ymm19, %ymm19;
	vpxord %ymm20, %ymm20, %ymm20;
	vpxord %ymm21, %ymm21, %ymm21;
	vpxord %ymm30, %ymm30, %ymm30;
	vpxord %ymm31, %ymm31, %ymm31;
	vzeroall;

.align 16
.Lxts_crypt_skip_avx512:
	/* Handle trailing blocks with AVX2 implementation. */
	cmpq $0, %r8;
	ja _gcry_vaes_avx2_xts_crypt_amd64 AT_PLT;

	ret_spec_stop
	CFI_ENDPROC();
ELF(.size _gcry_vaes_avx512_xts_crypt_amd64,.-_gcry_vaes_avx512_xts_crypt_amd64)

/**********************************************************************
  ECB-mode encryption
 **********************************************************************/
ELF(.type _gcry_vaes_avx512_ecb_crypt_amd64,@function)
.globl _gcry_vaes_avx512_ecb_crypt_amd64
.align 16
_gcry_vaes_avx512_ecb_crypt_amd64:
	/* input:
	 *	%rdi: round keys
	 *	%esi: encrypt
	 *	%rdx: dst
	 *	%rcx: src
	 *	%r8:  nblocks
	 *	%r9:  nrounds
	 */
	CFI_STARTPROC();

	cmpq $16, %r8;
	jb .Lecb_crypt_skip_avx512;

	spec_stop_avx512;

	leal (, %r9d, 4), %eax;
	vbroadcasti32x4 (%rdi), %zmm14; /* first key */
	vbroadcasti32x4 (%rdi, %rax, 4), %zmm15; /* last key */

	/* Process 32 blocks per loop. */
.align 16
.Lecb_blk32:
	cmpq $32, %r8;
	jb .Lecb_blk16;

	leaq -32(%r8), %r8;

	/* Load input and xor first key. */
	vpxord (0 * 16)(%rcx), %zmm14, %zmm0;
	vpxord (4 * 16)(%rcx), %zmm14, %zmm1;
	vpxord (8 * 16)(%rcx), %zmm14, %zmm2;
	vpxord (12 * 16)(%rcx), %zmm14, %zmm3;
	vpxord (16 * 16)(%rcx), %zmm14, %zmm4;
	vpxord (20 * 16)(%rcx), %zmm14, %zmm5;
	vpxord (24 * 16)(%rcx), %zmm14, %zmm6;
	vpxord (28 * 16)(%rcx), %zmm14, %zmm7;
	leaq (32 * 16)(%rcx), %rcx;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm8;

	testl %esi, %esi;
	jz .Lecb_dec_blk32;
		/* AES rounds */
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		cmpl $12, %r9d;
		jb .Lecb_enc_blk32_last;
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		jz .Lecb_enc_blk32_last;
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm8;
		VAESENC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	  .align 16
	  .Lecb_enc_blk32_last:
		vaesenclast %zmm15, %zmm0, %zmm0;
		vaesenclast %zmm15, %zmm1, %zmm1;
		vaesenclast %zmm15, %zmm2, %zmm2;
		vaesenclast %zmm15, %zmm3, %zmm3;
		vaesenclast %zmm15, %zmm4, %zmm4;
		vaesenclast %zmm15, %zmm5, %zmm5;
		vaesenclast %zmm15, %zmm6, %zmm6;
		vaesenclast %zmm15, %zmm7, %zmm7;
		jmp .Lecb_blk32_end;

	  .align 16
	  .Lecb_dec_blk32:
		/* AES rounds */
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		cmpl $12, %r9d;
		jb .Lecb_dec_blk32_last;
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		jz .Lecb_dec_blk32_last;
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm8;
		VAESDEC8(%zmm8, %zmm0, %zmm1, %zmm2, %zmm3, %zmm4, %zmm5, %zmm6, %zmm7);
	  .align 16
	  .Lecb_dec_blk32_last:
		vaesdeclast %zmm15, %zmm0, %zmm0;
		vaesdeclast %zmm15, %zmm1, %zmm1;
		vaesdeclast %zmm15, %zmm2, %zmm2;
		vaesdeclast %zmm15, %zmm3, %zmm3;
		vaesdeclast %zmm15, %zmm4, %zmm4;
		vaesdeclast %zmm15, %zmm5, %zmm5;
		vaesdeclast %zmm15, %zmm6, %zmm6;
		vaesdeclast %zmm15, %zmm7, %zmm7;

  .align 16
  .Lecb_blk32_end:
	vmovdqu32 %zmm0, (0 * 16)(%rdx);
	vmovdqu32 %zmm1, (4 * 16)(%rdx);
	vmovdqu32 %zmm2, (8 * 16)(%rdx);
	vmovdqu32 %zmm3, (12 * 16)(%rdx);
	vmovdqu32 %zmm4, (16 * 16)(%rdx);
	vmovdqu32 %zmm5, (20 * 16)(%rdx);
	vmovdqu32 %zmm6, (24 * 16)(%rdx);
	vmovdqu32 %zmm7, (28 * 16)(%rdx);
	leaq (32 * 16)(%rdx), %rdx;

	jmp .Lecb_blk32;

	/* Handle trailing 16 blocks. */
.align 16
.Lecb_blk16:
	cmpq $16, %r8;
	jmp .Lecb_crypt_tail;

	leaq -16(%r8), %r8;

	/* Load input and xor first key. */
	vpxord (0 * 16)(%rcx), %zmm14, %zmm0;
	vpxord (4 * 16)(%rcx), %zmm14, %zmm1;
	vpxord (8 * 16)(%rcx), %zmm14, %zmm2;
	vpxord (12 * 16)(%rcx), %zmm14, %zmm3;
	leaq (16 * 16)(%rcx), %rcx;
	vbroadcasti32x4 (1 * 16)(%rdi), %zmm4;

	testl %esi, %esi;
	jz .Lecb_dec_blk16;
		/* AES rounds */
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		cmpl $12, %r9d;
		jb .Lecb_enc_blk16_last;
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		jz .Lecb_enc_blk16_last;
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
		VAESENC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	  .align 16
	  .Lecb_enc_blk16_last:
		vaesenclast %zmm15, %zmm0, %zmm0;
		vaesenclast %zmm15, %zmm1, %zmm1;
		vaesenclast %zmm15, %zmm2, %zmm2;
		vaesenclast %zmm15, %zmm3, %zmm3;
		vmovdqu32 %zmm0, (0 * 16)(%rdx);
		vmovdqu32 %zmm1, (4 * 16)(%rdx);
		vmovdqu32 %zmm2, (8 * 16)(%rdx);
		vmovdqu32 %zmm3, (12 * 16)(%rdx);
		leaq (16 * 16)(%rdx), %rdx;
		jmp .Lecb_crypt_tail;

	  .align 16
	  .Lecb_dec_blk16:
		/* AES rounds */
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (2 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (3 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (4 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (5 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (6 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (7 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (8 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (9 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		cmpl $12, %r9d;
		jb .Lecb_dec_blk16_last;
		vbroadcasti32x4 (10 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (11 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		jz .Lecb_dec_blk16_last;
		vbroadcasti32x4 (12 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
		vbroadcasti32x4 (13 * 16)(%rdi), %zmm4;
		VAESDEC4(%zmm4, %zmm0, %zmm1, %zmm2, %zmm3);
	  .align 16
	  .Lecb_dec_blk16_last:
		vaesdeclast %zmm15, %zmm0, %zmm0;
		vaesdeclast %zmm15, %zmm1, %zmm1;
		vaesdeclast %zmm15, %zmm2, %zmm2;
		vaesdeclast %zmm15, %zmm3, %zmm3;
		vmovdqu32 %zmm0, (0 * 16)(%rdx);
		vmovdqu32 %zmm1, (4 * 16)(%rdx);
		vmovdqu32 %zmm2, (8 * 16)(%rdx);
		vmovdqu32 %zmm3, (12 * 16)(%rdx);
		leaq (16 * 16)(%rdx), %rdx;

.align 16
.Lecb_crypt_tail:
	/* Clear used AVX512 registers. */
	vzeroall;

.align 16
.Lecb_crypt_skip_avx512:
	/* Handle trailing blocks with AVX2 implementation. */
	cmpq $0, %r8;
	ja _gcry_vaes_avx2_ecb_crypt_amd64 AT_PLT;

	ret_spec_stop
	CFI_ENDPROC();
ELF(.size _gcry_vaes_avx512_ecb_crypt_amd64,.-_gcry_vaes_avx512_ecb_crypt_amd64)

/**********************************************************************
  constants
 **********************************************************************/
SECTION_RODATA

.align 64
ELF(.type _gcry_vaes_avx512_consts,@object)
_gcry_vaes_avx512_consts:

.Lbige_addb_0:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.Lbige_addb_1:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
.Lbige_addb_2:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2
.Lbige_addb_3:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3
.Lbige_addb_4:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4
.Lbige_addb_5:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5
.Lbige_addb_6:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6
.Lbige_addb_7:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7
.Lbige_addb_8:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8
.Lbige_addb_9:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9
.Lbige_addb_10:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10
.Lbige_addb_11:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11
.Lbige_addb_12:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12
.Lbige_addb_13:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13
.Lbige_addb_14:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14
.Lbige_addb_15:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15
.Lbige_addb_16:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16
.Lbige_addb_17:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 17
.Lbige_addb_18:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18
.Lbige_addb_19:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 19
.Lbige_addb_20:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20
.Lbige_addb_21:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 21
.Lbige_addb_22:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22
.Lbige_addb_23:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23
.Lbige_addb_24:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24
.Lbige_addb_25:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25
.Lbige_addb_26:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 26
.Lbige_addb_27:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 27
.Lbige_addb_28:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28
.Lbige_addb_29:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 29
.Lbige_addb_30:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30
.Lbige_addb_31:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31

.Lbswap128_mask:
	.byte 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

.Lxts_high_bit_shuf:
	.byte -1, -1, -1, -1, 12, 13, 14, 15, 4, 5, 6, 7, -1, -1, -1, -1
.Lxts_gfmul_clmul_bd:
	.byte 0x00, 0x87, 0x00, 0x00
	.byte 0x00, 0x87, 0x00, 0x00
	.byte 0x00, 0x87, 0x00, 0x00
	.byte 0x00, 0x87, 0x00, 0x00

.Lcounter0_1_2_3_lo_bq:
	.byte 0, 0, 1, 0, 2, 0, 3, 0
.Lcounter4_5_6_7_lo_bq:
	.byte 4, 0, 5, 0, 6, 0, 7, 0
.Lcounter8_9_10_11_lo_bq:
	.byte 8, 0, 9, 0, 10, 0, 11, 0
.Lcounter12_13_14_15_lo_bq:
	.byte 12, 0, 13, 0, 14, 0, 15, 0
.Lcounter16_17_18_19_lo_bq:
	.byte 16, 0, 17, 0, 18, 0, 19, 0
.Lcounter20_21_22_23_lo_bq:
	.byte 20, 0, 21, 0, 22, 0, 23, 0
.Lcounter24_25_26_27_lo_bq:
	.byte 24, 0, 25, 0, 26, 0, 27, 0
.Lcounter28_29_30_31_lo_bq:
	.byte 28, 0, 29, 0, 30, 0, 31, 0
.Lcounter4_4_4_4_lo_bq:
	.byte 4, 0, 4, 0, 4, 0, 4, 0
.Lcounter8_8_8_8_lo_bq:
	.byte 8, 0, 8, 0, 8, 0, 8, 0
.Lcounter16_16_16_16_lo_bq:
	.byte 16, 0, 16, 0, 16, 0, 16, 0
.Lcounter32_32_32_32_lo_bq:
	.byte 32, 0, 32, 0, 32, 0, 32, 0

ELF(.size _gcry_vaes_avx512_consts,.-_gcry_vaes_avx512_consts)

#endif /* HAVE_GCC_INLINE_ASM_VAES */
#endif /* __x86_64__ */
