/* sm4-avx2-amd64.h  -  Shared AVX2 cipher mode code for SM4 cipher
 *
 * Copyright (C) 2020, 2022-2023 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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

#ifndef GCRY_SM4_AVX2_AMD64_H
#define GCRY_SM4_AVX2_AMD64_H

SECTION_RODATA
.align 32

ELF(.type FUNC_NAME(cipher_mode_consts),@object)
FUNC_NAME(cipher_mode_consts):

/* CTR byte addition constants */
.align 32
.Lbige_addb_0_1:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
.Lbige_addb_2_3:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3
.Lbige_addb_4_5:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5
.Lbige_addb_6_7:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7
.Lbige_addb_8_9:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9
.Lbige_addb_10_11:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11
.Lbige_addb_12_13:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13
.Lbige_addb_14_15:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 14
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15

.text

.align 16
.globl FUNC_NAME(ctr_enc)
ELF(.type   FUNC_NAME(ctr_enc),@function;)
FUNC_NAME(ctr_enc):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (16 blocks)
	 *	%rdx: src (16 blocks)
	 *	%rcx: iv (big endian, 128bit)
	 */
	CFI_STARTPROC();

	cmpb $(0x100 - 16), 15(%rcx);
	jbe .Lctr_byteadd;

	movq 8(%rcx), %rax;
	bswapq %rax;

	vbroadcasti128 .Lbswap128_mask rRIP, RTMP3;
	vpcmpeqd RNOT, RNOT, RNOT;
	vpsrldq $14, RNOT, RNOT;   /* ab: -1:0 ; cd: -1:0 */
	vpaddw RNOT, RNOT, RTMP2; /* ab: -2:0 ; cd: -2:0 */

	/* load IV and byteswap */
	vmovdqu (%rcx), RTMP4x;
	vpshufb RTMP3x, RTMP4x, RTMP4x;
	vmovdqa RTMP4x, RTMP0x;
	vpsubw RNOTx, RTMP4x, RTMP4x;
	vinserti128 $1, RTMP4x, RTMP0, RTMP0;
	vpshufb RTMP3, RTMP0, RA0; /* +1 ; +0 */

	/* construct IVs */
	vpsubw RTMP2, RTMP0, RTMP0; /* +3 ; +2 */
	vpshufb RTMP3, RTMP0, RA1;
	vpsubw RTMP2, RTMP0, RTMP0; /* +5 ; +4 */
	vpshufb RTMP3, RTMP0, RA2;
	vpsubw RTMP2, RTMP0, RTMP0; /* +7 ; +6 */
	vpshufb RTMP3, RTMP0, RA3;
	vpsubw RTMP2, RTMP0, RTMP0; /* +9 ; +8 */
	vpshufb RTMP3, RTMP0, RB0;
	vpsubw RTMP2, RTMP0, RTMP0; /* +11 ; +10 */
	vpshufb RTMP3, RTMP0, RB1;
	vpsubw RTMP2, RTMP0, RTMP0; /* +13 ; +12 */
	vpshufb RTMP3, RTMP0, RB2;
	vpsubw RTMP2, RTMP0, RTMP0; /* +15 ; +14 */
	vpshufb RTMP3, RTMP0, RB3;

.align 8
.Lload_ctr_done:
	/* Update counter */
	addb $16, 15(%rcx);
	adcb $0, 14(%rcx);

	call SM4_CRYPT_BLK16;

	vpxor (0 * 32)(%rdx), RA0, RA0;
	vpxor (1 * 32)(%rdx), RA1, RA1;
	vpxor (2 * 32)(%rdx), RA2, RA2;
	vpxor (3 * 32)(%rdx), RA3, RA3;
	vpxor (4 * 32)(%rdx), RB0, RB0;
	vpxor (5 * 32)(%rdx), RB1, RB1;
	vpxor (6 * 32)(%rdx), RB2, RB2;
	vpxor (7 * 32)(%rdx), RB3, RB3;

	vmovdqu RA0, (0 * 32)(%rsi);
	vmovdqu RA1, (1 * 32)(%rsi);
	vmovdqu RA2, (2 * 32)(%rsi);
	vmovdqu RA3, (3 * 32)(%rsi);
	vmovdqu RB0, (4 * 32)(%rsi);
	vmovdqu RB1, (5 * 32)(%rsi);
	vmovdqu RB2, (6 * 32)(%rsi);
	vmovdqu RB3, (7 * 32)(%rsi);

	vzeroall;

	ret_spec_stop;

.align 8
.Lctr_byteadd:
	vbroadcasti128 (%rcx), RB3;
	vpaddb .Lbige_addb_0_1 rRIP, RB3, RA0;
	vpaddb .Lbige_addb_2_3 rRIP, RB3, RA1;
	vpaddb .Lbige_addb_4_5 rRIP, RB3, RA2;
	vpaddb .Lbige_addb_6_7 rRIP, RB3, RA3;
	vpaddb .Lbige_addb_8_9 rRIP, RB3, RB0;
	vpaddb .Lbige_addb_10_11 rRIP, RB3, RB1;
	vpaddb .Lbige_addb_12_13 rRIP, RB3, RB2;
	vpaddb .Lbige_addb_14_15 rRIP, RB3, RB3;

	jmp .Lload_ctr_done;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(ctr_enc),.-FUNC_NAME(ctr_enc);)

.align 16
.globl FUNC_NAME(cbc_dec)
ELF(.type   FUNC_NAME(cbc_dec),@function;)
FUNC_NAME(cbc_dec):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (16 blocks)
	 *	%rdx: src (16 blocks)
	 *	%rcx: iv
	 */
	CFI_STARTPROC();

	vmovdqu (0 * 32)(%rdx), RA0;
	vmovdqu (1 * 32)(%rdx), RA1;
	vmovdqu (2 * 32)(%rdx), RA2;
	vmovdqu (3 * 32)(%rdx), RA3;
	vmovdqu (4 * 32)(%rdx), RB0;
	vmovdqu (5 * 32)(%rdx), RB1;
	vmovdqu (6 * 32)(%rdx), RB2;
	vmovdqu (7 * 32)(%rdx), RB3;

	call SM4_CRYPT_BLK16;

	vmovdqu (%rcx), RNOTx;
	vinserti128 $1, (%rdx), RNOT, RNOT;
	vpxor RNOT, RA0, RA0;
	vpxor (0 * 32 + 16)(%rdx), RA1, RA1;
	vpxor (1 * 32 + 16)(%rdx), RA2, RA2;
	vpxor (2 * 32 + 16)(%rdx), RA3, RA3;
	vpxor (3 * 32 + 16)(%rdx), RB0, RB0;
	vpxor (4 * 32 + 16)(%rdx), RB1, RB1;
	vpxor (5 * 32 + 16)(%rdx), RB2, RB2;
	vpxor (6 * 32 + 16)(%rdx), RB3, RB3;
	vmovdqu (7 * 32 + 16)(%rdx), RNOTx;
	vmovdqu RNOTx, (%rcx); /* store new IV */

	vmovdqu RA0, (0 * 32)(%rsi);
	vmovdqu RA1, (1 * 32)(%rsi);
	vmovdqu RA2, (2 * 32)(%rsi);
	vmovdqu RA3, (3 * 32)(%rsi);
	vmovdqu RB0, (4 * 32)(%rsi);
	vmovdqu RB1, (5 * 32)(%rsi);
	vmovdqu RB2, (6 * 32)(%rsi);
	vmovdqu RB3, (7 * 32)(%rsi);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(cbc_dec),.-FUNC_NAME(cbc_dec);)

.align 16
.globl FUNC_NAME(cfb_dec)
ELF(.type   FUNC_NAME(cfb_dec),@function;)
FUNC_NAME(cfb_dec):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (16 blocks)
	 *	%rdx: src (16 blocks)
	 *	%rcx: iv
	 */
	CFI_STARTPROC();

	/* Load input */
	vmovdqu (%rcx), RNOTx;
	vinserti128 $1, (%rdx), RNOT, RA0;
	vmovdqu (0 * 32 + 16)(%rdx), RA1;
	vmovdqu (1 * 32 + 16)(%rdx), RA2;
	vmovdqu (2 * 32 + 16)(%rdx), RA3;
	vmovdqu (3 * 32 + 16)(%rdx), RB0;
	vmovdqu (4 * 32 + 16)(%rdx), RB1;
	vmovdqu (5 * 32 + 16)(%rdx), RB2;
	vmovdqu (6 * 32 + 16)(%rdx), RB3;

	/* Update IV */
	vmovdqu (7 * 32 + 16)(%rdx), RNOTx;
	vmovdqu RNOTx, (%rcx);

	call SM4_CRYPT_BLK16;

	vpxor (0 * 32)(%rdx), RA0, RA0;
	vpxor (1 * 32)(%rdx), RA1, RA1;
	vpxor (2 * 32)(%rdx), RA2, RA2;
	vpxor (3 * 32)(%rdx), RA3, RA3;
	vpxor (4 * 32)(%rdx), RB0, RB0;
	vpxor (5 * 32)(%rdx), RB1, RB1;
	vpxor (6 * 32)(%rdx), RB2, RB2;
	vpxor (7 * 32)(%rdx), RB3, RB3;

	vmovdqu RA0, (0 * 32)(%rsi);
	vmovdqu RA1, (1 * 32)(%rsi);
	vmovdqu RA2, (2 * 32)(%rsi);
	vmovdqu RA3, (3 * 32)(%rsi);
	vmovdqu RB0, (4 * 32)(%rsi);
	vmovdqu RB1, (5 * 32)(%rsi);
	vmovdqu RB2, (6 * 32)(%rsi);
	vmovdqu RB3, (7 * 32)(%rsi);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(cfb_dec),.-FUNC_NAME(cfb_dec);)

.align 16
.globl FUNC_NAME(ocb_enc)
ELF(.type FUNC_NAME(ocb_enc),@function;)

FUNC_NAME(ocb_enc):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (16 blocks)
	 *	%rdx: src (16 blocks)
	 *	%rcx: offset
	 *	%r8 : checksum
	 *	%r9 : L pointers (void *L[16])
	 */
	CFI_STARTPROC();

	subq $(4 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(4 * 8);

	movq %r10, (0 * 8)(%rsp);
	movq %r11, (1 * 8)(%rsp);
	movq %r12, (2 * 8)(%rsp);
	movq %r13, (3 * 8)(%rsp);
	CFI_REL_OFFSET(%r10, 0 * 8);
	CFI_REL_OFFSET(%r11, 1 * 8);
	CFI_REL_OFFSET(%r12, 2 * 8);
	CFI_REL_OFFSET(%r13, 3 * 8);

	vmovdqu (%rcx), RTMP0x;
	vmovdqu (%r8), RTMP1x;

	/* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
	/* Checksum_i = Checksum_{i-1} xor P_i  */
	/* C_i = Offset_i xor ENCIPHER(K, P_i xor Offset_i)  */

#define OCB_INPUT(n, l0reg, l1reg, yreg) \
	  vmovdqu (n * 32)(%rdx), yreg; \
	  vpxor (l0reg), RTMP0x, RNOTx; \
	  vpxor (l1reg), RNOTx, RTMP0x; \
	  vinserti128 $1, RTMP0x, RNOT, RNOT; \
	  vpxor yreg, RTMP1, RTMP1; \
	  vpxor yreg, RNOT, yreg; \
	  vmovdqu RNOT, (n * 32)(%rsi);

	movq (0 * 8)(%r9), %r10;
	movq (1 * 8)(%r9), %r11;
	movq (2 * 8)(%r9), %r12;
	movq (3 * 8)(%r9), %r13;
	OCB_INPUT(0, %r10, %r11, RA0);
	OCB_INPUT(1, %r12, %r13, RA1);
	movq (4 * 8)(%r9), %r10;
	movq (5 * 8)(%r9), %r11;
	movq (6 * 8)(%r9), %r12;
	movq (7 * 8)(%r9), %r13;
	OCB_INPUT(2, %r10, %r11, RA2);
	OCB_INPUT(3, %r12, %r13, RA3);
	movq (8 * 8)(%r9), %r10;
	movq (9 * 8)(%r9), %r11;
	movq (10 * 8)(%r9), %r12;
	movq (11 * 8)(%r9), %r13;
	OCB_INPUT(4, %r10, %r11, RB0);
	OCB_INPUT(5, %r12, %r13, RB1);
	movq (12 * 8)(%r9), %r10;
	movq (13 * 8)(%r9), %r11;
	movq (14 * 8)(%r9), %r12;
	movq (15 * 8)(%r9), %r13;
	OCB_INPUT(6, %r10, %r11, RB2);
	OCB_INPUT(7, %r12, %r13, RB3);
#undef OCB_INPUT

	vextracti128 $1, RTMP1, RNOTx;
	vmovdqu RTMP0x, (%rcx);
	vpxor RNOTx, RTMP1x, RTMP1x;
	vmovdqu RTMP1x, (%r8);

	movq (0 * 8)(%rsp), %r10;
	movq (1 * 8)(%rsp), %r11;
	movq (2 * 8)(%rsp), %r12;
	movq (3 * 8)(%rsp), %r13;
	CFI_RESTORE(%r10);
	CFI_RESTORE(%r11);
	CFI_RESTORE(%r12);
	CFI_RESTORE(%r13);

	call SM4_CRYPT_BLK16;

	addq $(4 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(-4 * 8);

	vpxor (0 * 32)(%rsi), RA0, RA0;
	vpxor (1 * 32)(%rsi), RA1, RA1;
	vpxor (2 * 32)(%rsi), RA2, RA2;
	vpxor (3 * 32)(%rsi), RA3, RA3;
	vpxor (4 * 32)(%rsi), RB0, RB0;
	vpxor (5 * 32)(%rsi), RB1, RB1;
	vpxor (6 * 32)(%rsi), RB2, RB2;
	vpxor (7 * 32)(%rsi), RB3, RB3;

	vmovdqu RA0, (0 * 32)(%rsi);
	vmovdqu RA1, (1 * 32)(%rsi);
	vmovdqu RA2, (2 * 32)(%rsi);
	vmovdqu RA3, (3 * 32)(%rsi);
	vmovdqu RB0, (4 * 32)(%rsi);
	vmovdqu RB1, (5 * 32)(%rsi);
	vmovdqu RB2, (6 * 32)(%rsi);
	vmovdqu RB3, (7 * 32)(%rsi);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(ocb_enc),.-FUNC_NAME(ocb_enc);)

.align 16
.globl FUNC_NAME(ocb_dec)
ELF(.type FUNC_NAME(ocb_dec),@function;)

FUNC_NAME(ocb_dec):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (16 blocks)
	 *	%rdx: src (16 blocks)
	 *	%rcx: offset
	 *	%r8 : checksum
	 *	%r9 : L pointers (void *L[16])
	 */
	CFI_STARTPROC();

	subq $(4 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(4 * 8);

	movq %r10, (0 * 8)(%rsp);
	movq %r11, (1 * 8)(%rsp);
	movq %r12, (2 * 8)(%rsp);
	movq %r13, (3 * 8)(%rsp);
	CFI_REL_OFFSET(%r10, 0 * 8);
	CFI_REL_OFFSET(%r11, 1 * 8);
	CFI_REL_OFFSET(%r12, 2 * 8);
	CFI_REL_OFFSET(%r13, 3 * 8);

	vmovdqu (%rcx), RTMP0x;

	/* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
	/* C_i = Offset_i xor ENCIPHER(K, P_i xor Offset_i)  */

#define OCB_INPUT(n, l0reg, l1reg, yreg) \
	  vmovdqu (n * 32)(%rdx), yreg; \
	  vpxor (l0reg), RTMP0x, RNOTx; \
	  vpxor (l1reg), RNOTx, RTMP0x; \
	  vinserti128 $1, RTMP0x, RNOT, RNOT; \
	  vpxor yreg, RNOT, yreg; \
	  vmovdqu RNOT, (n * 32)(%rsi);

	movq (0 * 8)(%r9), %r10;
	movq (1 * 8)(%r9), %r11;
	movq (2 * 8)(%r9), %r12;
	movq (3 * 8)(%r9), %r13;
	OCB_INPUT(0, %r10, %r11, RA0);
	OCB_INPUT(1, %r12, %r13, RA1);
	movq (4 * 8)(%r9), %r10;
	movq (5 * 8)(%r9), %r11;
	movq (6 * 8)(%r9), %r12;
	movq (7 * 8)(%r9), %r13;
	OCB_INPUT(2, %r10, %r11, RA2);
	OCB_INPUT(3, %r12, %r13, RA3);
	movq (8 * 8)(%r9), %r10;
	movq (9 * 8)(%r9), %r11;
	movq (10 * 8)(%r9), %r12;
	movq (11 * 8)(%r9), %r13;
	OCB_INPUT(4, %r10, %r11, RB0);
	OCB_INPUT(5, %r12, %r13, RB1);
	movq (12 * 8)(%r9), %r10;
	movq (13 * 8)(%r9), %r11;
	movq (14 * 8)(%r9), %r12;
	movq (15 * 8)(%r9), %r13;
	OCB_INPUT(6, %r10, %r11, RB2);
	OCB_INPUT(7, %r12, %r13, RB3);
#undef OCB_INPUT

	vmovdqu RTMP0x, (%rcx);

	movq (0 * 8)(%rsp), %r10;
	movq (1 * 8)(%rsp), %r11;
	movq (2 * 8)(%rsp), %r12;
	movq (3 * 8)(%rsp), %r13;
	CFI_RESTORE(%r10);
	CFI_RESTORE(%r11);
	CFI_RESTORE(%r12);
	CFI_RESTORE(%r13);

	call SM4_CRYPT_BLK16;

	addq $(4 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(-4 * 8);

	vmovdqu (%r8), RTMP1x;

	vpxor (0 * 32)(%rsi), RA0, RA0;
	vpxor (1 * 32)(%rsi), RA1, RA1;
	vpxor (2 * 32)(%rsi), RA2, RA2;
	vpxor (3 * 32)(%rsi), RA3, RA3;
	vpxor (4 * 32)(%rsi), RB0, RB0;
	vpxor (5 * 32)(%rsi), RB1, RB1;
	vpxor (6 * 32)(%rsi), RB2, RB2;
	vpxor (7 * 32)(%rsi), RB3, RB3;

	/* Checksum_i = Checksum_{i-1} xor P_i  */

	vmovdqu RA0, (0 * 32)(%rsi);
	vpxor RA0, RTMP1, RTMP1;
	vmovdqu RA1, (1 * 32)(%rsi);
	vpxor RA1, RTMP1, RTMP1;
	vmovdqu RA2, (2 * 32)(%rsi);
	vpxor RA2, RTMP1, RTMP1;
	vmovdqu RA3, (3 * 32)(%rsi);
	vpxor RA3, RTMP1, RTMP1;
	vmovdqu RB0, (4 * 32)(%rsi);
	vpxor RB0, RTMP1, RTMP1;
	vmovdqu RB1, (5 * 32)(%rsi);
	vpxor RB1, RTMP1, RTMP1;
	vmovdqu RB2, (6 * 32)(%rsi);
	vpxor RB2, RTMP1, RTMP1;
	vmovdqu RB3, (7 * 32)(%rsi);
	vpxor RB3, RTMP1, RTMP1;

	vextracti128 $1, RTMP1, RNOTx;
	vpxor RNOTx, RTMP1x, RTMP1x;
	vmovdqu RTMP1x, (%r8);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(ocb_dec),.-FUNC_NAME(ocb_dec);)

.align 16
.globl FUNC_NAME(ocb_auth)
ELF(.type FUNC_NAME(ocb_auth),@function;)

FUNC_NAME(ocb_auth):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: abuf (16 blocks)
	 *	%rdx: offset
	 *	%rcx: checksum
	 *	%r8 : L pointers (void *L[16])
	 */
	CFI_STARTPROC();

	subq $(4 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(4 * 8);

	movq %r10, (0 * 8)(%rsp);
	movq %r11, (1 * 8)(%rsp);
	movq %r12, (2 * 8)(%rsp);
	movq %r13, (3 * 8)(%rsp);
	CFI_REL_OFFSET(%r10, 0 * 8);
	CFI_REL_OFFSET(%r11, 1 * 8);
	CFI_REL_OFFSET(%r12, 2 * 8);
	CFI_REL_OFFSET(%r13, 3 * 8);

	vmovdqu (%rdx), RTMP0x;

	/* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
	/* Sum_i = Sum_{i-1} xor ENCIPHER(K, A_i xor Offset_i)  */

#define OCB_INPUT(n, l0reg, l1reg, yreg) \
	  vmovdqu (n * 32)(%rsi), yreg; \
	  vpxor (l0reg), RTMP0x, RNOTx; \
	  vpxor (l1reg), RNOTx, RTMP0x; \
	  vinserti128 $1, RTMP0x, RNOT, RNOT; \
	  vpxor yreg, RNOT, yreg;

	movq (0 * 8)(%r8), %r10;
	movq (1 * 8)(%r8), %r11;
	movq (2 * 8)(%r8), %r12;
	movq (3 * 8)(%r8), %r13;
	OCB_INPUT(0, %r10, %r11, RA0);
	OCB_INPUT(1, %r12, %r13, RA1);
	movq (4 * 8)(%r8), %r10;
	movq (5 * 8)(%r8), %r11;
	movq (6 * 8)(%r8), %r12;
	movq (7 * 8)(%r8), %r13;
	OCB_INPUT(2, %r10, %r11, RA2);
	OCB_INPUT(3, %r12, %r13, RA3);
	movq (8 * 8)(%r8), %r10;
	movq (9 * 8)(%r8), %r11;
	movq (10 * 8)(%r8), %r12;
	movq (11 * 8)(%r8), %r13;
	OCB_INPUT(4, %r10, %r11, RB0);
	OCB_INPUT(5, %r12, %r13, RB1);
	movq (12 * 8)(%r8), %r10;
	movq (13 * 8)(%r8), %r11;
	movq (14 * 8)(%r8), %r12;
	movq (15 * 8)(%r8), %r13;
	OCB_INPUT(6, %r10, %r11, RB2);
	OCB_INPUT(7, %r12, %r13, RB3);
#undef OCB_INPUT

	vmovdqu RTMP0x, (%rdx);

	movq (0 * 8)(%rsp), %r10;
	movq (1 * 8)(%rsp), %r11;
	movq (2 * 8)(%rsp), %r12;
	movq (3 * 8)(%rsp), %r13;
	CFI_RESTORE(%r10);
	CFI_RESTORE(%r11);
	CFI_RESTORE(%r12);
	CFI_RESTORE(%r13);

	call SM4_CRYPT_BLK16;

	addq $(4 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(-4 * 8);

	vpxor RA0, RB0, RA0;
	vpxor RA1, RB1, RA1;
	vpxor RA2, RB2, RA2;
	vpxor RA3, RB3, RA3;

	vpxor RA1, RA0, RA0;
	vpxor RA3, RA2, RA2;

	vpxor RA2, RA0, RTMP1;

	vextracti128 $1, RTMP1, RNOTx;
	vpxor (%rcx), RTMP1x, RTMP1x;
	vpxor RNOTx, RTMP1x, RTMP1x;
	vmovdqu RTMP1x, (%rcx);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(ocb_auth),.-FUNC_NAME(ocb_auth);)

#endif /* GCRY_SM4_AVX2_AMD64_H */
