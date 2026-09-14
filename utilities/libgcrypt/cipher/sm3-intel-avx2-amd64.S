/*
;;
;; Copyright (c) 2023-2024, Intel Corporation
;;
;; Redistribution and use in source and binary forms, with or without
;; modification, are permitted provided that the following conditions are met:
;;
;;     * Redistributions of source code must retain the above copyright notice,
;;       this list of conditions and the following disclaimer.
;;     * Redistributions in binary form must reproduce the above copyright
;;       notice, this list of conditions and the following disclaimer in the
;;       documentation and/or other materials provided with the distribution.
;;     * Neither the name of Intel Corporation nor the names of its contributors
;;       may be used to endorse or promote products derived from this software
;;       without specific prior written permission.
;;
;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
;; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
;; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
;; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
;; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
;; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
;; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;
*/
/*
 * From:
 *  https://github.com/intel/intel-ipsec-mb/blob/2b8d6041780fe94b749c98c08b92e7ea06cf50e2/lib/avx2_t4/sm3_ni_x1_avx2.asm
 *
 * Conversion to GAS assembly and integration to libgcrypt
 *  by Jussi Kivilinna <jussi.kivilinna@iki.fi>
 *
 * Uses the Intel SM3 instruction set extension (VSM3MSG1/VSM3MSG2/VSM3RNDS2)
 */

#ifdef __x86_64
#include <config.h>
#if (defined(HAVE_COMPATIBLE_GCC_AMD64_PLATFORM_AS) || \
     defined(HAVE_COMPATIBLE_GCC_WIN64_PLATFORM_AS)) && \
    defined(HAVE_INTEL_SYNTAX_PLATFORM_AS) && \
    defined(HAVE_GCC_INLINE_ASM_AVX2) && \
    defined(HAVE_GCC_INLINE_ASM_SM3) && defined(USE_SM3)

#include "asm-common-amd64.h"

.intel_syntax noprefix

#define arg_hash      rdi
#define arg_msg       rsi
#define arg_num_blks  rdx

SECTION_RODATA

ELF(.type _gcry_sm3_intel_avx2_consts,@object)
_gcry_sm3_intel_avx2_consts:

.align 16
.LSHUFF_MASK:
        .byte 3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12
ELF(.size _gcry_sm3_intel_avx2_consts,.-_gcry_sm3_intel_avx2_consts)

.text

/* Create 4 x 32-bit new words of message schedule W[] using SM3-NI ISA. */
#define SM3MSG(W03_00, W07_04, W11_08, W15_12, W19_16, T1, T2) \
        vpalignr W19_16, W11_08, W07_04, 3*4;  /* W19_16 = W10 W9 W8 W7 */ \
        vpsrldq  T1, W15_12, 4;                 /* T1 = 0 W15 W14 W13 */ \
        vsm3msg1 W19_16, T1, W03_00;            /* W19_16 = WTMP3..0 */ \
        vpalignr T1, W07_04, W03_00, 3*4;       /* T1 = W6 W5 W4 W3 */ \
        vpalignr T2, W15_12, W11_08, 2*4;       /* T2 = W13 W12 W11 W10 */ \
        vsm3msg2 W19_16, T1, T2;                /* W19_16 = W19 W18 W17 W16 */

/* Perform 4 rounds of SM3, consuming 4 words of message schedule W[] and
 * updating the SM3 state registers ABEF and CDGH. */
#define SM3ROUNDS4(ABEF, CDGH, W03_00, W07_04, T1, R) \
        vpunpcklqdq T1, W03_00, W07_04;         /* T1 = W5 W4 W1 W0 */ \
        vsm3rnds2   CDGH, ABEF, T1, R;          /* CDGH = updated ABEF */ \
        vpunpckhqdq T1, W03_00, W07_04;         /* T1 = W7 W6 W3 W2 */ \
        vsm3rnds2   ABEF, CDGH, T1, ((R) + 2);  /* ABEF = updated CDGH */

/*
 * unsigned int
 * _gcry_sm3_transform_intel_avx2 (void *state, const unsigned char *data,
 *                                 size_t nblks)
 */
.align 16
.globl _gcry_sm3_transform_intel_avx2
ELF(.type _gcry_sm3_transform_intel_avx2,@function)
_gcry_sm3_transform_intel_avx2:
        /* input:
         *   %rdi: state (8 x u32 hash value)
         *   %rsi: data (message blocks)
         *   %rdx: nblks
         */
        CFI_STARTPROC();

        test    arg_num_blks, arg_num_blks;
        jz      .Ldone;

        /* Load current hash value and change word order. */
        vmovdqu xmm6, [arg_hash + 0*16];        /* xmm6 = D C B A */
        vmovdqu xmm7, [arg_hash + 1*16];        /* xmm7 = H G F E */
        vpshufd xmm0, xmm6, 0x1B;               /* xmm0 = A B C D */
        vpshufd xmm1, xmm7, 0x1B;               /* xmm1 = E F G H */
        vpunpckhqdq xmm6, xmm1, xmm0;           /* xmm6 = A B E F */
        vpunpcklqdq xmm7, xmm1, xmm0;           /* xmm7 = C D G H */

        /* Pre-rotate C,D,G,H. */
        vpsrld  xmm2, xmm7, 9;
        vpslld  xmm3, xmm7, 23;
        vpxor   xmm1, xmm2, xmm3;               /* xmm1 = ROL32(CDGH, 23) */
        vpsrld  xmm4, xmm7, 19;
        vpslld  xmm5, xmm7, 13;
        vpxor   xmm0, xmm4, xmm5;               /* xmm0 = ROL32(CDGH, 13) */
        vpblendd xmm7, xmm1, xmm0, 0x3;         /* xmm7 = ROL(C,23) ROL(D,23)
                                                          ROL(G,13) ROL(H,13) */

        vmovdqa xmm12, [.LSHUFF_MASK ADD_RIP];

.align 16
.Lblock_loop:
        vmovdqa xmm10, xmm6;
        vmovdqa xmm11, xmm7;

        /* Prepare W[0..15] - read and byte-swap the message words. */
        vmovdqu xmm2, [arg_msg + 0*16];
        vmovdqu xmm3, [arg_msg + 1*16];
        vmovdqu xmm4, [arg_msg + 2*16];
        vmovdqu xmm5, [arg_msg + 3*16];
        vpshufb xmm2, xmm2, xmm12;              /* xmm2 = W03 W02 W01 W00 */
        vpshufb xmm3, xmm3, xmm12;              /* xmm3 = W07 W06 W05 W04 */
        vpshufb xmm4, xmm4, xmm12;              /* xmm4 = W11 W10 W09 W08 */
        vpshufb xmm5, xmm5, xmm12;              /* xmm5 = W15 W14 W13 W12 */

        SM3MSG(xmm2, xmm3, xmm4, xmm5, xmm8, xmm9, xmm1)        /* W19..16 */
        SM3ROUNDS4(xmm6, xmm7, xmm2, xmm3, xmm1, 0)

        vmovdqa xmm2, xmm8;
        SM3MSG(xmm3, xmm4, xmm5, xmm2, xmm8, xmm9, xmm1)        /* W23..20 */
        SM3ROUNDS4(xmm6, xmm7, xmm3, xmm4, xmm1, 4)

        vmovdqa xmm3, xmm8;
        SM3MSG(xmm4, xmm5, xmm2, xmm3, xmm8, xmm9, xmm1)        /* W27..24 */
        SM3ROUNDS4(xmm6, xmm7, xmm4, xmm5, xmm1, 8)

        vmovdqa xmm4, xmm8;
        SM3MSG(xmm5, xmm2, xmm3, xmm4, xmm8, xmm9, xmm1)        /* W31..28 */
        SM3ROUNDS4(xmm6, xmm7, xmm5, xmm2, xmm1, 12)

        vmovdqa xmm5, xmm8;
        SM3MSG(xmm2, xmm3, xmm4, xmm5, xmm8, xmm9, xmm1)        /* W35..32 */
        SM3ROUNDS4(xmm6, xmm7, xmm2, xmm3, xmm1, 16)

        vmovdqa xmm2, xmm8;
        SM3MSG(xmm3, xmm4, xmm5, xmm2, xmm8, xmm9, xmm1)        /* W39..36 */
        SM3ROUNDS4(xmm6, xmm7, xmm3, xmm4, xmm1, 20)

        vmovdqa xmm3, xmm8;
        SM3MSG(xmm4, xmm5, xmm2, xmm3, xmm8, xmm9, xmm1)        /* W43..40 */
        SM3ROUNDS4(xmm6, xmm7, xmm4, xmm5, xmm1, 24)

        vmovdqa xmm4, xmm8;
        SM3MSG(xmm5, xmm2, xmm3, xmm4, xmm8, xmm9, xmm1)        /* W47..44 */
        SM3ROUNDS4(xmm6, xmm7, xmm5, xmm2, xmm1, 28)

        vmovdqa xmm5, xmm8;
        SM3MSG(xmm2, xmm3, xmm4, xmm5, xmm8, xmm9, xmm1)        /* W51..48 */
        SM3ROUNDS4(xmm6, xmm7, xmm2, xmm3, xmm1, 32)

        vmovdqa xmm2, xmm8;
        SM3MSG(xmm3, xmm4, xmm5, xmm2, xmm8, xmm9, xmm1)        /* W55..52 */
        SM3ROUNDS4(xmm6, xmm7, xmm3, xmm4, xmm1, 36)

        vmovdqa xmm3, xmm8;
        SM3MSG(xmm4, xmm5, xmm2, xmm3, xmm8, xmm9, xmm1)        /* W59..56 */
        SM3ROUNDS4(xmm6, xmm7, xmm4, xmm5, xmm1, 40)

        vmovdqa xmm4, xmm8;
        SM3MSG(xmm5, xmm2, xmm3, xmm4, xmm8, xmm9, xmm1)        /* W63..60 */
        SM3ROUNDS4(xmm6, xmm7, xmm5, xmm2, xmm1, 44)

        vmovdqa xmm5, xmm8;
        SM3MSG(xmm2, xmm3, xmm4, xmm5, xmm8, xmm9, xmm1)        /* W67..64 */
        SM3ROUNDS4(xmm6, xmm7, xmm2, xmm3, xmm1, 48)

        vmovdqa xmm2, xmm8;
        SM3ROUNDS4(xmm6, xmm7, xmm3, xmm4, xmm1, 52)

        SM3ROUNDS4(xmm6, xmm7, xmm4, xmm5, xmm1, 56)

        SM3ROUNDS4(xmm6, xmm7, xmm5, xmm2, xmm1, 60)

        /* Add feed-forward to the chaining value and move to the next block. */
        vpxor   xmm6, xmm6, xmm10;
        vpxor   xmm7, xmm7, xmm11;
        add     arg_msg, 64;
        dec     arg_num_blks;
        jnz     .Lblock_loop;

        /* Un-rotate C,D,G,H. */
        vpslld  xmm2, xmm7, 9;
        vpsrld  xmm3, xmm7, 23;
        vpxor   xmm1, xmm2, xmm3;               /* xmm1 = ROL32(CDGH, 9) */
        vpslld  xmm4, xmm7, 19;
        vpsrld  xmm5, xmm7, 13;
        vpxor   xmm0, xmm4, xmm5;               /* xmm0 = ROL32(CDGH, 19) */
        vpblendd xmm7, xmm1, xmm0, 0x3;         /* xmm7 = ROL(C,9) ROL(D,9)
                                                          ROL(G,19) ROL(H,19) */
        vpshufd xmm0, xmm6, 0x1B;               /* xmm0 = F E B A */
        vpshufd xmm1, xmm7, 0x1B;               /* xmm1 = H G D C */
        vpunpcklqdq xmm6, xmm0, xmm1;           /* xmm6 = D C B A */
        vpunpckhqdq xmm7, xmm0, xmm1;           /* xmm7 = H G F E */
        vmovdqu [arg_hash + 0*16], xmm6;
        vmovdqu [arg_hash + 1*16], xmm7;

        vzeroall;

.Ldone:
        xor     eax, eax;
        ret_spec_stop;
        CFI_ENDPROC();
ELF(.size _gcry_sm3_transform_intel_avx2,.-_gcry_sm3_transform_intel_avx2;)

#endif
#endif
