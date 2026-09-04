/* sm4-avx512-amd64.h  -  Shared AVX512 32-block cipher mode code for SM4
 *
 * Copyright (C) 2022-2023 Jussi Kivilinna <jussi.kivilinna@iki.fi>
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

#ifndef GCRY_SM4_AVX512_AMD64_H
#define GCRY_SM4_AVX512_AMD64_H

SECTION_RODATA
.align 64

ELF(.type FUNC_NAME(cipher_mode_consts),@object)
FUNC_NAME(cipher_mode_consts):

/* CTR mode addition constants */

.Lcounter0123_lo:
	.quad 0, 0
	.quad 1, 0
	.quad 2, 0
	.quad 3, 0

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
.Lbige_addb_16:
	.byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16

.Lcounter2222_lo:
	.quad 2, 0
.Lcounter4444_lo:
	.quad 4, 0
.Lcounter8888_lo:
	.quad 8, 0
.Lcounter16161616_lo:
	.quad 16, 0

.text

.align 16
.globl FUNC_NAME(crypt_blk32)
ELF(.type   FUNC_NAME(crypt_blk32),@function;)
FUNC_NAME(crypt_blk32):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (32 blocks)
	 *	%rdx: src (32 blocks)
	 */
	CFI_STARTPROC();
	spec_stop_avx512;

	/* Load input */
	vmovdqu32 (0 * 64)(%rdx), RA0z;
	vmovdqu32 (1 * 64)(%rdx), RA1z;
	vmovdqu32 (2 * 64)(%rdx), RA2z;
	vmovdqu32 (3 * 64)(%rdx), RA3z;
	vmovdqu32 (4 * 64)(%rdx), RB0z;
	vmovdqu32 (5 * 64)(%rdx), RB1z;
	vmovdqu32 (6 * 64)(%rdx), RB2z;
	vmovdqu32 (7 * 64)(%rdx), RB3z;

	call SM4_CRYPT_BLK32;

	vmovdqu32 RA0z, (0 * 64)(%rsi);
	vmovdqu32 RA1z, (1 * 64)(%rsi);
	vmovdqu32 RA2z, (2 * 64)(%rsi);
	vmovdqu32 RA3z, (3 * 64)(%rsi);
	vmovdqu32 RB0z, (4 * 64)(%rsi);
	vmovdqu32 RB1z, (5 * 64)(%rsi);
	vmovdqu32 RB2z, (6 * 64)(%rsi);
	vmovdqu32 RB3z, (7 * 64)(%rsi);

	xorl %eax, %eax;
	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(crypt_blk32),.-FUNC_NAME(crypt_blk32);)

.align 16
.globl FUNC_NAME(ctr_enc_blk32)
ELF(.type   FUNC_NAME(ctr_enc_blk32),@function;)
FUNC_NAME(ctr_enc_blk32):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (32 blocks)
	 *	%rdx: src (32 blocks)
	 *	%rcx: iv (big endian, 128bit)
	 */
	CFI_STARTPROC();
	spec_stop_avx512;

	cmpb $(0x100 - 32), 15(%rcx);
	jbe .Lctr_byteadd32;

	vbroadcasti64x2 .Lbswap128_mask rRIP, RTMP0z;
	vmovdqa32 .Lcounter0123_lo rRIP, RTMP1z;
	vbroadcasti64x2 .Lcounter4444_lo rRIP, RTMP2z;
	vbroadcasti64x2 .Lcounter8888_lo rRIP, RTMP3z;
	vbroadcasti64x2 .Lcounter16161616_lo rRIP, RTMP4z;

	/* load IV and byteswap */
	vbroadcasti64x2 (%rcx), RB3z;
	vpshufb RTMP0z, RB3z, RB3z;

	/* construct IVs */
	vpaddw RTMP1z, RB3z, RA0z; /* +0:+1:+2:+3 */
	vpaddw RTMP2z, RA0z, RA1z; /* +4:+5:+6:+7 */
	vpaddw RTMP3z, RA0z, RA2z; /* +8:+9:+10:+11 */
	vpaddw RTMP3z, RA1z, RA3z; /* +12:+13:+14:+15 */
	vpaddw RTMP4z, RA0z, RB0z; /* +16... */
	vpaddw RTMP4z, RA1z, RB1z; /* +20... */
	vpaddw RTMP4z, RA2z, RB2z; /* +24... */
	vpaddw RTMP4z, RA3z, RB3z; /* +28... */

	/* Byte-swap IVs. */
	vpshufb RTMP0z, RA0z, RA0z;
	vpshufb RTMP0z, RA1z, RA1z;
	vpshufb RTMP0z, RA2z, RA2z;
	vpshufb RTMP0z, RA3z, RA3z;
	vpshufb RTMP0z, RB0z, RB0z;
	vpshufb RTMP0z, RB1z, RB1z;
	vpshufb RTMP0z, RB2z, RB2z;
	vpshufb RTMP0z, RB3z, RB3z;

.align 16
.Lload_ctr_done32:
	/* Update counter */
	addb $32, 15(%rcx);
	adcb $0, 14(%rcx);

	call SM4_CRYPT_BLK32;

	vpxord (0 * 64)(%rdx), RA0z, RA0z;
	vpxord (1 * 64)(%rdx), RA1z, RA1z;
	vpxord (2 * 64)(%rdx), RA2z, RA2z;
	vpxord (3 * 64)(%rdx), RA3z, RA3z;
	vpxord (4 * 64)(%rdx), RB0z, RB0z;
	vpxord (5 * 64)(%rdx), RB1z, RB1z;
	vpxord (6 * 64)(%rdx), RB2z, RB2z;
	vpxord (7 * 64)(%rdx), RB3z, RB3z;

	vmovdqu32 RA0z, (0 * 64)(%rsi);
	vmovdqu32 RA1z, (1 * 64)(%rsi);
	vmovdqu32 RA2z, (2 * 64)(%rsi);
	vmovdqu32 RA3z, (3 * 64)(%rsi);
	vmovdqu32 RB0z, (4 * 64)(%rsi);
	vmovdqu32 RB1z, (5 * 64)(%rsi);
	vmovdqu32 RB2z, (6 * 64)(%rsi);
	vmovdqu32 RB3z, (7 * 64)(%rsi);

	vzeroall;

	ret_spec_stop;

.align 16
.Lctr_byteadd32:
	vbroadcasti64x2 (%rcx), RA3z;
	vbroadcasti64x2 .Lbige_addb_16 rRIP, RB3z;
	vpaddb RB3z, RA3z, RB3z;
	vpaddb .Lbige_addb_0_1 rRIP, RA3z, RA0z;
	vpaddb .Lbige_addb_4_5 rRIP, RA3z, RA1z;
	vpaddb .Lbige_addb_8_9 rRIP, RA3z, RA2z;
	vpaddb .Lbige_addb_12_13 rRIP, RA3z, RA3z;
	vpaddb .Lbige_addb_0_1 rRIP, RB3z, RB0z;
	vpaddb .Lbige_addb_4_5 rRIP, RB3z, RB1z;
	vpaddb .Lbige_addb_8_9 rRIP, RB3z, RB2z;
	vpaddb .Lbige_addb_12_13 rRIP, RB3z, RB3z;

	jmp .Lload_ctr_done32;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(ctr_enc_blk32),.-FUNC_NAME(ctr_enc_blk32);)

.align 16
.globl FUNC_NAME(cbc_dec_blk32)
ELF(.type   FUNC_NAME(cbc_dec_blk32),@function;)
FUNC_NAME(cbc_dec_blk32):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (32 blocks)
	 *	%rdx: src (32 blocks)
	 *	%rcx: iv
	 */
	CFI_STARTPROC();
	spec_stop_avx512;

	vmovdqu32 (0 * 64)(%rdx), RA0z;
	vmovdqu32 (1 * 64)(%rdx), RA1z;
	vmovdqu32 (2 * 64)(%rdx), RA2z;
	vmovdqu32 (3 * 64)(%rdx), RA3z;
	vmovdqu32 (4 * 64)(%rdx), RB0z;
	vmovdqu32 (5 * 64)(%rdx), RB1z;
	vmovdqu32 (6 * 64)(%rdx), RB2z;
	vmovdqu32 (7 * 64)(%rdx), RB3z;

	call SM4_CRYPT_BLK32;

	vmovdqu (%rcx), RNOTx;
	vinserti64x2 $1, (0 * 16)(%rdx), RNOT, RNOT;
	vinserti64x4 $1, (1 * 16)(%rdx), RNOTz, RNOTz;
	vpxord RNOTz, RA0z, RA0z;
	vpxord (0 * 64 + 48)(%rdx), RA1z, RA1z;
	vpxord (1 * 64 + 48)(%rdx), RA2z, RA2z;
	vpxord (2 * 64 + 48)(%rdx), RA3z, RA3z;
	vpxord (3 * 64 + 48)(%rdx), RB0z, RB0z;
	vpxord (4 * 64 + 48)(%rdx), RB1z, RB1z;
	vpxord (5 * 64 + 48)(%rdx), RB2z, RB2z;
	vpxord (6 * 64 + 48)(%rdx), RB3z, RB3z;
	vmovdqu (7 * 64 + 48)(%rdx), RNOTx;
	vmovdqu RNOTx, (%rcx); /* store new IV */

	vmovdqu32 RA0z, (0 * 64)(%rsi);
	vmovdqu32 RA1z, (1 * 64)(%rsi);
	vmovdqu32 RA2z, (2 * 64)(%rsi);
	vmovdqu32 RA3z, (3 * 64)(%rsi);
	vmovdqu32 RB0z, (4 * 64)(%rsi);
	vmovdqu32 RB1z, (5 * 64)(%rsi);
	vmovdqu32 RB2z, (6 * 64)(%rsi);
	vmovdqu32 RB3z, (7 * 64)(%rsi);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(cbc_dec_blk32),.-FUNC_NAME(cbc_dec_blk32);)

.align 16
.globl FUNC_NAME(cfb_dec_blk32)
ELF(.type   FUNC_NAME(cfb_dec_blk32),@function;)
FUNC_NAME(cfb_dec_blk32):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (32 blocks)
	 *	%rdx: src (32 blocks)
	 *	%rcx: iv
	 */
	CFI_STARTPROC();
	spec_stop_avx512;

	/* Load input */
	vmovdqu (%rcx), RA0x;
	vinserti64x2 $1, (%rdx), RA0, RA0;
	vinserti64x4 $1, 16(%rdx), RA0z, RA0z;
	vmovdqu32 (0 * 64 + 48)(%rdx), RA1z;
	vmovdqu32 (1 * 64 + 48)(%rdx), RA2z;
	vmovdqu32 (2 * 64 + 48)(%rdx), RA3z;
	vmovdqu32 (3 * 64 + 48)(%rdx), RB0z;
	vmovdqu32 (4 * 64 + 48)(%rdx), RB1z;
	vmovdqu32 (5 * 64 + 48)(%rdx), RB2z;
	vmovdqu32 (6 * 64 + 48)(%rdx), RB3z;

	/* Update IV */
	vmovdqu (7 * 64 + 48)(%rdx), RNOTx;
	vmovdqu RNOTx, (%rcx);

	call SM4_CRYPT_BLK32;

	vpxord (0 * 64)(%rdx), RA0z, RA0z;
	vpxord (1 * 64)(%rdx), RA1z, RA1z;
	vpxord (2 * 64)(%rdx), RA2z, RA2z;
	vpxord (3 * 64)(%rdx), RA3z, RA3z;
	vpxord (4 * 64)(%rdx), RB0z, RB0z;
	vpxord (5 * 64)(%rdx), RB1z, RB1z;
	vpxord (6 * 64)(%rdx), RB2z, RB2z;
	vpxord (7 * 64)(%rdx), RB3z, RB3z;

	vmovdqu32 RA0z, (0 * 64)(%rsi);
	vmovdqu32 RA1z, (1 * 64)(%rsi);
	vmovdqu32 RA2z, (2 * 64)(%rsi);
	vmovdqu32 RA3z, (3 * 64)(%rsi);
	vmovdqu32 RB0z, (4 * 64)(%rsi);
	vmovdqu32 RB1z, (5 * 64)(%rsi);
	vmovdqu32 RB2z, (6 * 64)(%rsi);
	vmovdqu32 RB3z, (7 * 64)(%rsi);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(cfb_dec_blk32),.-FUNC_NAME(cfb_dec_blk32);)

.align 16
.globl FUNC_NAME(ocb_enc_blk32)
ELF(.type FUNC_NAME(ocb_enc_blk32),@function;)
FUNC_NAME(ocb_enc_blk32):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (32 blocks)
	 *	%rdx: src (32 blocks)
	 *	%rcx: offset
	 *	%r8 : checksum
	 *	%r9 : L pointers (void *L[32])
	 */
	CFI_STARTPROC();
	spec_stop_avx512;

	subq $(5 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(5 * 8);

	movq %r12, (0 * 8)(%rsp);
	movq %r13, (1 * 8)(%rsp);
	movq %r14, (2 * 8)(%rsp);
	movq %r15, (3 * 8)(%rsp);
	movq %rbx, (4 * 8)(%rsp);
	CFI_REL_OFFSET(%r12, 0 * 8);
	CFI_REL_OFFSET(%r13, 1 * 8);
	CFI_REL_OFFSET(%r14, 2 * 8);
	CFI_REL_OFFSET(%r15, 3 * 8);
	CFI_REL_OFFSET(%rbx, 4 * 8);

	vmovdqu (%rcx), RTMP0x;

	/* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
	/* Checksum_i = Checksum_{i-1} xor P_i  */
	/* C_i = Offset_i xor ENCIPHER(K, P_i xor Offset_i)  */

#define OCB_INPUT(n, l0reg, l1reg, l2reg, l3reg, zreg, zplain) \
	  vmovdqu32 (n * 64)(%rdx), zplain; \
	  vpxor (l0reg), RTMP0x, RNOTx; \
	  vpxor (l1reg), RNOTx, RTMP0x; \
	  vinserti64x2 $1, RTMP0x, RNOT, RNOT; \
	  vpxor (l2reg), RTMP0x, RTMP0x; \
	  vinserti64x2 $2, RTMP0x, RNOTz, RNOTz; \
	  vpxor (l3reg), RTMP0x, RTMP0x; \
	  vinserti64x2 $3, RTMP0x, RNOTz, RNOTz; \
	  vpxord zplain, RNOTz, zreg; \
	  vmovdqu32 RNOTz, (n * 64)(%rsi);

#define OCB_LOAD_PTRS(n) \
	  movq ((n * 4 * 8) + (0 * 8))(%r9), %r10; \
	  movq ((n * 4 * 8) + (1 * 8))(%r9), %r11; \
	  movq ((n * 4 * 8) + (2 * 8))(%r9), %r12; \
	  movq ((n * 4 * 8) + (3 * 8))(%r9), %r13; \
	  movq ((n * 4 * 8) + (4 * 8))(%r9), %r14; \
	  movq ((n * 4 * 8) + (5 * 8))(%r9), %r15; \
	  movq ((n * 4 * 8) + (6 * 8))(%r9), %rax; \
	  movq ((n * 4 * 8) + (7 * 8))(%r9), %rbx;

	OCB_LOAD_PTRS(0);
	OCB_INPUT(0, %r10, %r11, %r12, %r13, RA0z, RTMP1z);
	OCB_INPUT(1, %r14, %r15, %rax, %rbx, RA1z, RTMP2z);
	OCB_LOAD_PTRS(2);
	OCB_INPUT(2, %r10, %r11, %r12, %r13, RA2z, RTMP3z);
	vpternlogd $0x96, RTMP1z, RTMP2z, RTMP3z;
	OCB_INPUT(3, %r14, %r15, %rax, %rbx, RA3z, RTMP4z);
	OCB_LOAD_PTRS(4);
	OCB_INPUT(4, %r10, %r11, %r12, %r13, RB0z, RX0z);
	OCB_INPUT(5, %r14, %r15, %rax, %rbx, RB1z, RX1z);
	vpternlogd $0x96, RTMP4z, RX0z, RX1z;
	OCB_LOAD_PTRS(6);
	OCB_INPUT(6, %r10, %r11, %r12, %r13, RB2z, RTMP4z);
	OCB_INPUT(7, %r14, %r15, %rax, %rbx, RB3z, RX0z);
#undef OCB_LOAD_PTRS
#undef OCB_INPUT

	vpternlogd $0x96, RTMP3z, RTMP4z, RX0z;
	vpxord RX1z, RX0z, RNOTz;
	vextracti64x4 $1, RNOTz, RTMP1;
	vpxor RTMP1, RNOT, RNOT;
	vextracti128 $1, RNOT, RTMP1x;
	vpternlogd $0x96, (%r8), RTMP1x, RNOTx;

	movq (0 * 8)(%rsp), %r12;
	movq (1 * 8)(%rsp), %r13;
	movq (2 * 8)(%rsp), %r14;
	movq (3 * 8)(%rsp), %r15;
	movq (4 * 8)(%rsp), %rbx;
	CFI_RESTORE(%r12);
	CFI_RESTORE(%r13);
	CFI_RESTORE(%r14);
	CFI_RESTORE(%r15);
	CFI_RESTORE(%rbx);

	vmovdqu RTMP0x, (%rcx);
	vmovdqu RNOTx, (%r8);

	call SM4_CRYPT_BLK32;

	addq $(5 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(-5 * 8);

	vpxord (0 * 64)(%rsi), RA0z, RA0z;
	vpxord (1 * 64)(%rsi), RA1z, RA1z;
	vpxord (2 * 64)(%rsi), RA2z, RA2z;
	vpxord (3 * 64)(%rsi), RA3z, RA3z;
	vpxord (4 * 64)(%rsi), RB0z, RB0z;
	vpxord (5 * 64)(%rsi), RB1z, RB1z;
	vpxord (6 * 64)(%rsi), RB2z, RB2z;
	vpxord (7 * 64)(%rsi), RB3z, RB3z;

	vmovdqu32 RA0z, (0 * 64)(%rsi);
	vmovdqu32 RA1z, (1 * 64)(%rsi);
	vmovdqu32 RA2z, (2 * 64)(%rsi);
	vmovdqu32 RA3z, (3 * 64)(%rsi);
	vmovdqu32 RB0z, (4 * 64)(%rsi);
	vmovdqu32 RB1z, (5 * 64)(%rsi);
	vmovdqu32 RB2z, (6 * 64)(%rsi);
	vmovdqu32 RB3z, (7 * 64)(%rsi);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(ocb_enc_blk32),.-FUNC_NAME(ocb_enc_blk32);)

.align 16
.globl FUNC_NAME(ocb_dec_blk32)
ELF(.type FUNC_NAME(ocb_dec_blk32),@function;)
FUNC_NAME(ocb_dec_blk32):
	/* input:
	 *	%rdi: ctx, CTX
	 *	%rsi: dst (32 blocks)
	 *	%rdx: src (32 blocks)
	 *	%rcx: offset
	 *	%r8 : checksum
	 *	%r9 : L pointers (void *L[32])
	 */
	CFI_STARTPROC();
	spec_stop_avx512;

	subq $(5 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(5 * 8);

	movq %r12, (0 * 8)(%rsp);
	movq %r13, (1 * 8)(%rsp);
	movq %r14, (2 * 8)(%rsp);
	movq %r15, (3 * 8)(%rsp);
	movq %rbx, (4 * 8)(%rsp);
	CFI_REL_OFFSET(%r12, 0 * 8);
	CFI_REL_OFFSET(%r13, 1 * 8);
	CFI_REL_OFFSET(%r14, 2 * 8);
	CFI_REL_OFFSET(%r15, 3 * 8);
	CFI_REL_OFFSET(%rbx, 4 * 8);

	vmovdqu (%rcx), RTMP0x;

	/* Offset_i = Offset_{i-1} xor L_{ntz(i)} */
	/* C_i = Offset_i xor DECIPHER(K, P_i xor Offset_i)  */

#define OCB_INPUT(n, l0reg, l1reg, l2reg, l3reg, zreg) \
	  vmovdqu32 (n * 64)(%rdx), RTMP1z; \
	  vpxor (l0reg), RTMP0x, RNOTx; \
	  vpxor (l1reg), RNOTx, RTMP0x; \
	  vinserti64x2 $1, RTMP0x, RNOT, RNOT; \
	  vpxor (l2reg), RTMP0x, RTMP0x; \
	  vinserti64x2 $2, RTMP0x, RNOTz, RNOTz; \
	  vpxor (l3reg), RTMP0x, RTMP0x; \
	  vinserti64x2 $3, RTMP0x, RNOTz, RNOTz; \
	  vpxord RTMP1z, RNOTz, zreg; \
	  vmovdqu32 RNOTz, (n * 64)(%rsi);

#define OCB_LOAD_PTRS(n) \
	  movq ((n * 4 * 8) + (0 * 8))(%r9), %r10; \
	  movq ((n * 4 * 8) + (1 * 8))(%r9), %r11; \
	  movq ((n * 4 * 8) + (2 * 8))(%r9), %r12; \
	  movq ((n * 4 * 8) + (3 * 8))(%r9), %r13; \
	  movq ((n * 4 * 8) + (4 * 8))(%r9), %r14; \
	  movq ((n * 4 * 8) + (5 * 8))(%r9), %r15; \
	  movq ((n * 4 * 8) + (6 * 8))(%r9), %rax; \
	  movq ((n * 4 * 8) + (7 * 8))(%r9), %rbx;

	OCB_LOAD_PTRS(0);
	OCB_INPUT(0, %r10, %r11, %r12, %r13, RA0z);
	OCB_INPUT(1, %r14, %r15, %rax, %rbx, RA1z);
	OCB_LOAD_PTRS(2);
	OCB_INPUT(2, %r10, %r11, %r12, %r13, RA2z);
	OCB_INPUT(3, %r14, %r15, %rax, %rbx, RA3z);
	OCB_LOAD_PTRS(4);
	OCB_INPUT(4, %r10, %r11, %r12, %r13, RB0z);
	OCB_INPUT(5, %r14, %r15, %rax, %rbx, RB1z);
	OCB_LOAD_PTRS(6);
	OCB_INPUT(6, %r10, %r11, %r12, %r13, RB2z);
	OCB_INPUT(7, %r14, %r15, %rax, %rbx, RB3z);
#undef OCB_LOAD_PTRS
#undef OCB_INPUT

	movq (0 * 8)(%rsp), %r12;
	movq (1 * 8)(%rsp), %r13;
	movq (2 * 8)(%rsp), %r14;
	movq (3 * 8)(%rsp), %r15;
	movq (4 * 8)(%rsp), %rbx;
	CFI_RESTORE(%r12);
	CFI_RESTORE(%r13);
	CFI_RESTORE(%r14);
	CFI_RESTORE(%r15);
	CFI_RESTORE(%rbx);

	vmovdqu RTMP0x, (%rcx);

	call SM4_CRYPT_BLK32;

	addq $(5 * 8), %rsp;
	CFI_ADJUST_CFA_OFFSET(-5 * 8);

	vpxord (0 * 64)(%rsi), RA0z, RA0z;
	vpxord (1 * 64)(%rsi), RA1z, RA1z;
	vpxord (2 * 64)(%rsi), RA2z, RA2z;
	vpxord (3 * 64)(%rsi), RA3z, RA3z;
	vpxord (4 * 64)(%rsi), RB0z, RB0z;
	vpxord (5 * 64)(%rsi), RB1z, RB1z;
	vpxord (6 * 64)(%rsi), RB2z, RB2z;
	vpxord (7 * 64)(%rsi), RB3z, RB3z;

	vmovdqu32 RA0z, (0 * 64)(%rsi);
	vmovdqu32 RA1z, (1 * 64)(%rsi);
	vmovdqu32 RA2z, (2 * 64)(%rsi);
	vmovdqu32 RA3z, (3 * 64)(%rsi);
	vmovdqu32 RB0z, (4 * 64)(%rsi);
	vmovdqu32 RB1z, (5 * 64)(%rsi);
	vmovdqu32 RB2z, (6 * 64)(%rsi);
	vmovdqu32 RB3z, (7 * 64)(%rsi);

	/* Checksum_i = Checksum_{i-1} xor C_i  */
	vpternlogd $0x96, RA0z, RA1z, RA2z;
	vpternlogd $0x96, RA3z, RB0z, RB1z;
	vpternlogd $0x96, RB2z, RB3z, RA2z;
	vpxord RA2z, RB1z, RTMP1z;

	vextracti64x4 $1, RTMP1z, RNOT;
	vpxor RNOT, RTMP1, RTMP1;
	vextracti128 $1, RTMP1, RNOTx;
	vpternlogd $0x96, (%r8), RNOTx, RTMP1x;
	vmovdqu RTMP1x, (%r8);

	vzeroall;

	ret_spec_stop;
	CFI_ENDPROC();
ELF(.size FUNC_NAME(ocb_dec_blk32),.-FUNC_NAME(ocb_dec_blk32);)

#endif /* GCRY_SM4_AVX512_AMD64_H */
