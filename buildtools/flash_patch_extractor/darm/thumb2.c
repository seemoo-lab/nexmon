/*
Copyright (c) 2013, Jurriaan Bremer
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of the darm developer(s) nor the names of its
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "darm.h"
#include "darm-internal.h"
#include "thumb-tbl.h"
#include "thumb2-tbl.h"
#include "thumb2.h"

#define BITMSK_8 ((1 << 8) - 1)
#define ROR(val, rotate) (((val) >> (rotate)) | ((val) << (32 - (rotate))))
#define SIGN_EXTEND32(v, len) (((int32_t)(v) << (32 - len)) >> (32 - len))

void thumb2_parse_reg(darm_t *d, uint16_t w, uint16_t w2);
void thumb2_parse_imm(darm_t *d, uint16_t w, uint16_t w2);
void thumb2_parse_flag(darm_t *d, uint16_t w, uint16_t w2);
void thumb2_parse_misc(darm_t *d, uint16_t w, uint16_t w2);

// 12 -> 32 bit expansion function
// See manual for this
// We don't care about the carry for the moment (should we?)
uint32_t thumb_expand_imm(uint16_t imm12)
{
    uint32_t value;

    imm12 &= 0xfff;

    if((imm12 & 0xc00) == 0) {
        switch ((imm12 & 0x300) >> 8) {
        case 0:
            value = imm12 & 0xff;
            break;

        case 1:
            value = ((imm12 & 0xff) << 16) | (imm12 & 0xff);
            break;

        case 2:
            value = ((imm12 & 0xff) << 24) | ((imm12 & 0xff) << 8);
            break;

        case 3:
            imm12 &= 0xff;
            value = imm12 | (imm12 << 8) | (imm12 << 16) | (imm12 << 24);
            break;
        }
    }
    else {
        uint32_t unrotated = 0x80 | (imm12 & 0x7F);
        value = ROR(unrotated, (imm12 & 0xF80) >> 7);
    }

    return value;
}

void thumb2_decode_immshift(darm_t *d, uint8_t type, uint8_t imm5)
{
    switch (type) {
    case 0:
        d->shift_type = S_LSL;
        d->shift = imm5;
        break;

    case 1:
        d->shift_type = S_LSR;
        d->shift = imm5 == 0 ? 32 : imm5;
        break;

    case 2:
        d->shift_type = S_ASR;
        d->shift = imm5 == 0 ? 32 : imm5;
        break;

    case 3:
        d->shift_type = S_ROR;
        d->shift = imm5 == 0 ? 1 : imm5; // RRX! :)
        break;

    default:
        d->shift_type = S_INVLD;
        break;
    }
}

// Parse the register instruction type
void thumb2_parse_reg(darm_t *d, uint16_t w, uint16_t w2)
{
    switch (d->instr_type) {
    case T_THUMB2_NO_REG:
        break;

    case T_THUMB2_RT_REG:
        d->Rt = (w2 >> 12) & b1111;
        break;

    case T_THUMB2_RT_RT2_REG:
        d->Rt = (w2 >> 12) & b1111;
        d->Rt2 = (w2 >> 8) & b1111;
        break;

    case T_THUMB2_RM_REG:
        d->Rm = (w & b1111);
        break;

    case T_THUMB2_RD_REG:
        d->Rd = (w2 >> 8) & b1111;
        break;

    case T_THUMB2_RD_RM_REG:
        d->Rd = (w2 >> 8) & b1111;
        d->Rm = w2 & b1111;
        break;

    case T_THUMB2_RN_REG:
        d->Rn = w & b1111;
        break;

    case T_THUMB2_RN_RT_REG:
        d->Rn = w & b1111;
        d->Rt = (w2 >> 12) & b1111;
        break;

    case T_THUMB2_RN_RT_RT2_REG:
        d->Rn = w & b1111;
        d->Rt = (w2 >> 12) & b1111;
        d->Rt2 = (w2 >> 8) & b1111;
        break;

    case T_THUMB2_RN_RM_REG:
        d->Rn = w & b1111;
        d->Rm = w2 & b1111;
        break;

    case T_THUMB2_RN_RM_RT_REG:
        d->Rn = w & b1111;
        d->Rm = w2 & b1111;
        d->Rt = (w2 >> 12) & b1111;
        break;

    case T_THUMB2_RN_RD_REG:
        d->Rn = w & b1111;
        d->Rd = (w2 >> 8) & b1111;
        break;

    case T_THUMB2_RN_RD_RT_REG:
        d->Rn = (w & b1111);
        d->Rd = (w2 & b1111);
        d->Rt = (w2 >> 12) & b1111;
        break;

    case T_THUMB2_RN_RD_RT_RT2_REG:
        d->Rn = (w & b1111);
        d->Rd = (w2 & b1111);
        d->Rt = (w2 >> 12) & b1111;
        d->Rt2 = (w2 >> 8) & b1111;
        break;

    case T_THUMB2_RN_RD_RM_REG:
        d->Rn = w & b1111;
        d->Rm = w2 & b1111;
        d->Rd = (w2 >> 8) & b1111;
        break;

    case T_THUMB2_RN_RD_RM_RA_REG:
        d->Rn = w & b1111;
        d->Rm = w2 & b1111;
        d->Rd = (w2 >> 8) & b1111;
        d->Ra = (w2 >> 12) & b1111;
        break;

    default:
        break;
    }
}

// Parse the immediate instruction type
void thumb2_parse_imm(darm_t *d, uint16_t w, uint16_t w2)
{
    d->I = B_SET;

    switch (d->instr_imm_type) {
    case T_THUMB2_NO_IMM:
        d->I = B_UNSET;
        break;

    case T_THUMB2_IMM12:
        d->imm = w2 & 0xfff;
        break;

    case T_THUMB2_IMM8:
        d->imm = w2 & 0xff;
        break;

    case T_THUMB2_IMM2:
        // 2 bit immediate
        d->imm = (w2 >> 4) & b11;
        d->shift = d->imm;
        d->shift_type = S_LSL;
        break;

    case T_THUMB2_IMM2_IMM3:
        // 2 and 3 bit immediates
        // (imm3:imm2)
        d->imm = ((w2 >> 10) & b11100) | ((w2 >> 6) & b11);
        break;

    case T_THUMB2_IMM1_IMM3_IMM8:
        // 1, 3 and 8 bit immediates
        // i:imm3:imm8 -> imm12 -> imm32

        // if bits 9:8 == '10' then zero extend, otherwise thumb expand
        if((w & 0x300) == 0x200) {
            d->imm = ((w & 0x400) << 1) | ((w2 & 0x7000) >> 4) | (w2 & 0xff);
        }
        else {
            d->imm = ((w & 0x400) << 1) | ((w2 & 0x7000) >> 4) | (w2 & 0xff);
            d->imm = thumb_expand_imm(d->imm);
        }
        break;

    default:
        break;
    }
}

// Parse the flag instruction type
void thumb2_parse_flag(darm_t *d, uint16_t w, uint16_t w2)
{
    switch (d->instr_flag_type) {
    case T_THUMB2_NO_FLAG:
        break;

    case T_THUMB2_ROTATE_FLAG:
        // Rotate field
        d->rotate = (w2 >> 1) & b11000;
        break;

    case T_THUMB2_U_FLAG:
        // U flag
        d->U = (w >> 7) & 1 ? B_SET : B_UNSET;
        break;

    case T_THUMB2_WUP_FLAG:
        // W, U and P flags
        d->W = (w2 >> 8) & 1 ? B_SET : B_UNSET;
        d->U = (w2 >> 9) & 1 ? B_SET : B_UNSET;
        d->P = (w2 >> 10) & 1 ? B_SET : B_UNSET;
        break;

    case T_THUMB2_TYPE_FLAG:
        // Type field
        // This is always a T_THUMB2_IMM2_IMM3 type
        thumb2_decode_immshift(d, (w2 >> 4) & 3, d->imm);
        break;

    case T_THUMB2_REGLIST_FLAG:
        // Reglist field
        d->reglist = w2 & 0xffff;
        break;

    case T_THUMB2_WP_REGLIST_FLAG:
        // Reglist field and W, P flags
        d->reglist = w2 & 0xffff;
        d->W = (w >> 5) & 1 ? B_SET : B_UNSET;
        //d->P = (w >> 15) & 1 ? B_SET : B_UNSET;
        break;

    case T_THUMB2_S_FLAG:
        // S flag
        d->S = (w >> 4) & 1 ? B_SET : B_UNSET;
        break;

    case T_THUMB2_S_TYPE_FLAG:
        // S flag and type field
        d->S = (w >> 4) & 1 ? B_SET : B_UNSET;
        thumb2_decode_immshift(d, (w2 >> 4) & 3, d->imm);
        break;

    default:
        break;
    }
}

// Parse misc instruction cases
void thumb2_parse_misc(darm_t *d, uint16_t w, uint16_t w2)
{
    switch (d->instr) {
    case I_B:
        d->I = B_SET;
        d->S = (w >> 10) & 1 ? B_SET : B_UNSET;
        if ((w2 & 0x1000) == 0) {
            // T3
            // sign_extend(S:J2:J1:imm6:imm11:0, 32)
            d->imm =
                ((w & 0x400) << 10) |
                ((w2 & 0x800) << 8) |
                ((w2 & 0x2000) << 5) |
                ((w & 0x3F) << 12) |
                ((w2 & 0x7ff) << 1);
            d->imm = SIGN_EXTEND32(d->imm, 21);
            d->cond = (w >> 6) & b1111;
        }
        else {
            // T4
            // I1 = not(J1 xor S);
            // I2 = not(J2 xor S);
            // imm32 = sign_extend(S:I1:I2:imm10:imm11:0, 32)
            d->imm =
                ((w & 0x400) << 14) |
                (((~(w2 >> 13) ^ (w >> 10)) & 1) << 23) |
                ((~((w2 >> 11) ^ (w >> 10)) & 1) << 22) |
                ((w & 0x3FF) << 12) |
                ((w2 & 0x7FF) << 1);
            d->imm = SIGN_EXTEND32(d->imm, 25);
        }
        break;

    case I_BL: case I_BLX:
        d->I = B_SET;
        d->S = (w >> 10) & 1 ? B_SET : B_UNSET;
        if ((w2 & 0x1000) == 0) {
            // BLX
            // I1 = not(J1 xor S); I2 = not(J2 xor S); imm32 = sign_extend(S:I1:I2:imm10H:imm10L:00, 32)
            d->imm =
                ((w & 0x400) << 14) |
                ((~((w2 >> 13) ^ (w >> 10)) & 1) << 23) |
                ((~((w2 >> 11) ^ (w >> 10)) & 1) << 22) |
                ((w & 0x3FF) << 12) |
                ((w2 & 0x7FE) << 1);
            d->imm = SIGN_EXTEND32(d->imm, 25);
            d->H = (w & 1) ? B_SET : B_UNSET;
        }
        else {
            // BL
            // I1 = not(J1 xor S); I2 = not(J2 xor S); imm32 = sign_extend(S:I1:I2:imm10:imm11:0, 32)
            d->imm =
                ((w & 0x400) << 14) |
                (((~((w2 >> 13) ^ (w >> 10))) & 1) << 23) |
                ((~((w2 >> 11) ^ (w >> 10)) & 1) << 22) |
                ((w & 0x3FF) << 12) |
                ((w2 & 0x7FF) << 1);
            d->imm = SIGN_EXTEND32(d->imm, 25);
        }
        break;

    case I_BFC: case I_BFI:
        d->lsb = d->imm & 0x1f;
        d->msb = w2 & 0x1f;
        d->width = d->msb + 1 - d->lsb;
        break;

    case I_LSL: case I_LSR: case I_ASR: case I_ROR:
        if(d->I == B_SET) {
            d->shift = d->imm;
            d->shift_type = ((w2 >> 4) & b11);
        }
        break;

    case I_MOVW: case I_MOVT:
        d->imm =
            ((w & b1111) << 12) |
            ((w & 0x400) << 1) |
            ((w2 & 0x7000) >> 4) |
            (w2 & 0xff);
        break;

    // preserve PC in Rd for further use (?)
    case I_CMP: case I_CMN: case I_TEQ: case I_TST:
        d->Rd = PC;
        break;

    // option field
    case I_DBG: case I_DMB: case I_DSB: case I_ISB:
        d->option = w2 & b1111;
        break;

    // co-proc data processing
    // TODO: not implemented
    //case I_CPD: case I_CPD2:
    //break;

    // co-proc load/store memory
    case I_LDC: case I_LDC2:
    case I_STC: case I_STC2:
        d->P = (w >> 8) & 1 ? B_SET : B_UNSET;
        d->U = (w >> 7) & 1 ? B_SET : B_UNSET;
        d->D = (w >> 6) & 1 ? B_SET : B_UNSET;
        d->W = (w >> 5) & 1 ? B_SET : B_UNSET;

        // literal or immediate
        d->Rn = (w & b1111) == b1111 ? R_INVLD : (w & b1111);

        d->I = B_SET;
        d->imm = (w2 & 0xff) << 2;
        d->coproc = (w2 >> 8) & b1111;
        d->CRd = (w2 >> 12) & b1111;
        break;

    // Weird Rd offset
    case I_STREX:
        d->Rd = (w2 >> 8) & b1111;
        d->imm = (w2 & 0xff) << 2;
        break;

    // zero-extend corner case with '00' appended
    case I_LDRD: case I_LDREX: case I_STRD:
        d->imm = (w2 & 0xff) << 2;
        d->W = (w >> 5) & 1 ? B_SET : B_UNSET;
        d->U = (w >> 7) & 1 ? B_SET : B_UNSET;
        d->P = (w >> 8) & 1 ? B_SET : B_UNSET;
        break;

    // Catch some pop/push inconsistencies
    case I_POP: case I_PUSH:
        // no flags, TODO fixup
        if(w == 0xf85d || w == 0xf84d) {
            break;
        }

        // P flag
        if(w == 0xe8bd) {
            d->P = (w2 >> 15) & 1 ? B_SET : B_UNSET;
        }

        d->M = (w2 >> 14) & 1 ? B_SET : B_UNSET;
        break;

    // co-processor move
    case I_MCR: case I_MCR2:
    case I_MRC: case I_MRC2:
        d->CRm = w2 & b1111;
        d->CRn = w & b1111;
        d->coproc = (w2 >> 8) & b1111;
        d->Rt = (w2 >> 12) & b1111;
        d->opc1 = (w >> 5) & b111;
        d->opc2 = (w2 >> 5) & b111;
        break;

    // co-proc move 2 reg
    case I_MCRR: case I_MCRR2:
    case I_MRRC: case I_MRRC2:
        d->coproc = (w2 >> 8) & b1111;
        d->Rt = (w2 >> 12) & b1111;
        d->opc1 = (w2 >> 4) & b1111;
        d->CRm = (w2 & b1111);
        d->Rt2 = w & b1111;
        break;

    case I_MSR:
        d->I = B_SET;
        d->Rn = w & b1111;
        d->mask = d->imm = (w2 >> 10) & b11;
        break;

    case I_PKH:
        // S flag and immediate already set
        d->T = (w2 >> 4) & 1;
        thumb2_decode_immshift(d, (w2 >> 4) & 2, d->imm);
        break;

    case I_PLI:
        d->Rt = R_INVLD;
        d->P = B_INVLD;
        d->W = B_INVLD;
        if(d->Rn == b1111) {
            d->Rn = R_INVLD;
            d->imm = w2 & 0xfff;
            d->U = ((w >> 7) & 1) ? B_SET : B_UNSET;
        }
        break;

    case I_PLD:
        d->Rt = R_INVLD;
        d->P = B_INVLD;
        if(d->Rn == b1111) {
            d->Rn = d->Rm = R_INVLD;
            d->imm = w2 & 0xfff;
            d->shift_type = S_INVLD;
            d->shift = 0;
            d->U = (w >> 7) & 1 ? B_SET : B_UNSET;
        }
        d->W = (w & b1111) != b1111 ? ((w >> 5) & 1) : B_INVLD;
        break;

    case I_SBFX: case I_UBFX:
        d->lsb = d->imm;
        d->width = (w2 & 0x1f) + 1;
        break;

    // N, M flags
    case I_SMLABB: case I_SMLABT: case I_SMLATB: case I_SMLATT:
    case I_SMULBB: case I_SMULBT: case I_SMULTB: case I_SMULTT:
        d->N = (w2 >> 5) & 1 ? B_SET : B_UNSET;
        // fall-through

    case I_SMLAD: case I_SMLAW:
    case I_SMLSD: case I_SMUAD:
    case I_SMULW: case I_SMUSD:
        d->M = (w2 >> 4) & 1 ? B_SET : B_UNSET;
        if(d->Ra == b1111) {
            d->Ra = R_INVLD;
        }
        break;

    // N, M, Rdhi, Rdlo flags
    case I_SMLALBB: case I_SMLALBT: case I_SMLALTB: case I_SMLALTT:
        d->N = (w2 >> 5) & 1 ? B_SET : B_UNSET;
        // fall-through

    case I_SMLSLD: case I_SMLALD:
        d->M = (w2 >> 4) & 1 ? B_SET : B_UNSET;
        // fall-through

    case I_SMLAL: case I_SMULL:
    case I_UMAAL: case I_UMLAL: case I_UMULL:
        d->RdHi = (w2 >> 8) & b1111;
        d->RdLo = (w2 >> 12) & b1111;
        break;

    case I_SMMLA: case I_SMMLS: case I_SMMUL:
        d->R = (w2 >> 4) & 1 ? B_SET : B_UNSET;
        break;

    case I_SSAT: case I_USAT:
        thumb2_decode_immshift(d, (w >> 4) & 2, d->imm);
        d->sat_imm = w2 & 0x1f;
        break;

    case I_SSAT16: case I_USAT16:
        d->sat_imm = w2 & 0xf;
        break;

    case I_STM: case I_STMDB:
        d->W = (w >> 5) & 1 ? B_SET : B_UNSET;
        d->M = (w2 >> 14) & 1 ? B_SET : B_UNSET;
        break;

    case I_TBB: case I_TBH:
        d->H = (w2 >> 4) & 1 ? B_SET : B_UNSET;
        break;

    default:
        break;
    }
}

static int thumb2_disasm(darm_t *d, uint16_t w, uint16_t w2)
{
    d->instr = thumb2_decode_instruction(d, w, w2);
    if(d->instr == I_INVLD) {
        return -1;
    }

    thumb2_parse_reg(d, w, w2);
    thumb2_parse_imm(d, w, w2);
    thumb2_parse_flag(d, w, w2);
    thumb2_parse_misc(d, w, w2);
    d->instr_type = T_INVLD;
    return 0;
}

// placeholder function for printing out thumb2 instructions
// This is here until the format string problem is resolved
// TODO: This lacks a lot of functionality, for debug only, replace with better function
char *darm_thumb2_str(darm_t *d)
{
    int index=0, offset=0;
    static char stringbuf[512];

    for (int i = 0; i < THUMB2_INSTRUCTION_COUNT; i++) {
        if(d->instr == thumb2_instr_labels[i]) {
            index = i;
            break;
        }
    }

    offset += sprintf(stringbuf + offset, "%s",
        thumb2_instruction_strings[index]);

    if(d->Rd != R_INVLD) {
        offset += sprintf(stringbuf+offset, "rd%i,", d->Rd);
    }

    if(d->Rt != R_INVLD) {
        offset += sprintf(stringbuf+offset, "rt%i,", d->Rt2);
    }

    if(d->Rt2 != R_INVLD) {
        offset += sprintf(stringbuf+offset, "rt2%i,", d->Rt);
    }

    if(d->Rn != R_INVLD) {
        offset += sprintf(stringbuf+offset, "rn%i,", d->Rn);
    }

    if(d->Rm != R_INVLD) {
        offset += sprintf(stringbuf+offset, "rm%i ", d->Rm);
    }

    if(d->I == B_SET) {
        offset += sprintf(stringbuf+offset, "#0x%x", d->imm);
    }

    return stringbuf;
}

int darm_thumb2_disasm(darm_t *d, uint16_t w, uint16_t w2)
{
    darm_init(d);
    d->w = (w << 16) | w2;

    // we set all conditional flags to "execute always" by default, as most
    // thumb instructions don't feature a conditional flag
    d->cond = C_AL;

    switch (w >> 11) {
    case b11101: case b11110: case b11111:
        return thumb2_disasm(d, w, w2);

    default:
        return -1;
    }
}
