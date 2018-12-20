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
#include "darm-tbl.h"
#include "thumb-tbl.h"
#include "thumb2-tbl.h"
#include "thumb2.h"

darm_instr_t thumb2_load_store_multiple(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_load_store_dual(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_data_shifted_reg(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_coproc_simd(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_modified_immediate(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_plain_immediate(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_branch_misc_ctrl(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_store_single_item(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_data_reg(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_mult_acc_diff(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_long_mult_acc(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_load_byte_hints(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_load_word(darm_t *d, uint16_t w, uint16_t w2);
darm_instr_t thumb2_load_halfword_hints(darm_t *d, uint16_t w, uint16_t w2);

darm_instr_t thumb2_decode_instruction(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op2 = (w >> 4) & 0x7f;

    switch ((w >> 11) & b11) {
    case 1:
        op2 &= 0x64;
        if(op2 == 0) {
            // load, store multiple
            return thumb2_load_store_multiple(d, w, w2);
        }
        else if(op2 == b100) {
            // load/store dual, load/store exclusive, table branch
            return thumb2_load_store_dual(d, w, w2);
        }
        else if(((op2 >> 5) & b11) == b1) {
            // dataprocessing (shifted register)
            return thumb2_data_shifted_reg(d, w, w2);
        }
        else if(((op2 >> 6) & b1) == b1) {
            // coproc, simd, fpu
            return thumb2_coproc_simd(d, w, w2);
        }
        break;

    case 2:
        op2 = (op2 & 0x20) >> 5;
        if(op2 == 0 && (w2 & 0x8000) == 0) {
            // dataprocessing (modified immediate)
            return thumb2_modified_immediate(d, w, w2);
        }
        else if(op2 == 1 && (w2 & 0x8000) == 0) {
            // dataprocessing (plain binary immediate)
            return thumb2_plain_immediate(d, w, w2);
        }
        else if((w2 & 0x8000) == 0x8000) {
            // branches and miscellaneous control
            return thumb2_branch_misc_ctrl(d, w, w2);
        }
        break;

    case 3:
        if((op2 & 0x71) == 0) {
            // store single data item
            return thumb2_store_single_item(d, w, w2);
        }
        else if((op2 & 0x71) == 0x10) {
            // Advanced SIMD element or structure load/store instructions
            // TODO: implement
            return I_INVLD;
        }
        else if((op2 & 0x70) == 0x20) {
            // data-processing (register)
            return thumb2_data_reg(d, w, w2);
        }
        else if((op2 & 0x78) == 0x30) {
            // multiply, multiply accumulate, and absolute difference
            return thumb2_mult_acc_diff(d, w, w2);
        }
        else if((op2 & 0x78) == 0x38) {
            // long multiply, long multiply accumulate, and divide
            return thumb2_long_mult_acc(d, w, w2);
        }
        else {
            switch (op2 & 0x67) {
            case 1:
                // load byte, memory hints
                return thumb2_load_byte_hints(d, w, w2);

            case 3:
                // load halfword, memory hints
                return thumb2_load_halfword_hints(d, w, w2);

            case 5:
                // load word
                return thumb2_load_word(d, w, w2);
            }
            break;
        }
    }

    // TODO handle other co-proc case
    return I_INVLD;
}

darm_instr_t thumb2_load_store_multiple(darm_t *d, uint16_t w, uint16_t w2)
{
    (void) w2;

    uint32_t op = (w >> 7) & b11;
    uint32_t L = (w >> 4) & b1;
    uint32_t W_Rn = ((w >> 1) & 0x10) | (w & b1111);

    d->instr_type = T_THUMB2_RN_REG;
    d->instr_imm_type = T_THUMB2_NO_IMM;
    d->instr_flag_type = T_THUMB2_REGLIST_FLAG;

    switch (op) {
    case 0:
    case 3:
        d->instr_type = T_THUMB2_NO_REG;
        d->instr_flag_type = T_THUMB2_NO_FLAG;
        if(L == 0) {
            return I_SRS;
        }
        else if(L == 1) {
            return I_RFE;
        }
        break;

    case 1:
        if(L == 0) {
            return I_STM;
        }

        if(W_Rn == 0x1d) {
            d->instr_type = T_THUMB2_NO_REG;
            return I_POP;
        }

        d->instr_flag_type = T_THUMB2_WP_REGLIST_FLAG;
        return I_LDM;

    case 2:
        if(L == 0) {
            if(W_Rn == 0x1d) {
                d->instr_type = T_THUMB2_NO_REG;
                return I_PUSH;
            }

            return I_STMDB;
        }

        d->instr_flag_type = T_THUMB2_WP_REGLIST_FLAG;
        return I_LDMDB;
    }

    return I_INVLD;
}

darm_instr_t thumb2_load_store_dual(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 7) & b11;
    uint32_t op2 = (w >> 4) & b11;
    uint32_t op3 = (w2 >> 4) & b1111;
    uint32_t Rn = w & b1111;

    d->instr_type = T_THUMB2_RN_RT_REG;
    d->instr_imm_type = T_THUMB2_IMM8;;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    if(op1 == 0 && op2 == 0) {
        d->instr_type = T_THUMB2_RN_RD_RT_REG;
        return I_STREX;
    }
    else if(op1 == 0 && op2 == 1) {
        return I_LDREX;
    }
    else if((op1 & 2) == 0 && op2 == 2) {
        d->instr_type = T_THUMB2_RN_RT_RT2_REG;
        d->instr_flag_type = T_THUMB2_WUP_FLAG;
        return I_STRD; // immediate
    }
    else if(((op1 & 2) == 0 && op2 == 3) ||
            ((op1 & 2) == 2 && (op2 & 1) == 1)) {
        d->instr_flag_type = T_THUMB2_WUP_FLAG;
        if(Rn == b1111) {
            d->instr_type = T_THUMB2_RT_RT2_REG;
        }
        else {
            d->instr_type = T_THUMB2_RN_RT_RT2_REG;
        }
        return I_LDRD; // literal and immediate
    }
    else if((op1 & 2) == 2 && (op2 & 1) == 0) {
        d->instr_type = T_THUMB2_RN_RT_RT2_REG;
        d->instr_flag_type = T_THUMB2_WUP_FLAG;
        return I_STRD; // immediate
    }
    else if(op1 == 1 && op2 == 0) {
        d->instr_imm_type = T_THUMB2_NO_IMM;
        d->instr_type = T_THUMB2_RN_RD_RT_REG;

        switch (op3) {
        case 4:
            return I_STREXB;

        case 5:
            return I_STREXH;

        case 7:
            d->instr_type = T_THUMB2_RN_RD_RT_RT2_REG;
            return I_STREXD;
        }
    }
    else if(op1 == 1 && op2 == 1) {
        d->instr_imm_type = T_THUMB2_NO_IMM;

        switch (op3) {
        case 0:
            d->instr_type = T_THUMB2_RN_RM_REG;
            d->instr_imm_type = T_THUMB2_NO_IMM;
            return I_TBB;

        case 1:
            d->instr_type = T_THUMB2_RN_RM_REG;
            d->instr_imm_type = T_THUMB2_NO_IMM;
            return I_TBH;

        case 4:
            return I_LDREXB;

        case 5:
            return I_LDREXH;

        case 7:
            d->instr_type = T_THUMB2_RN_RT_RT2_REG;
            return I_LDREXD;
        }
    }

    return I_INVLD;
}

darm_instr_t thumb2_move_shift(darm_t *d, uint16_t w, uint16_t w2)
{
    (void) w;

    uint32_t type = (w2>>4) & b11;
    uint32_t imm3_imm2 = ((w2>>10) & 0x1C) | ((w2>>6) & b11);

    d->instr_type = T_THUMB2_RD_RM_REG;
    d->instr_imm_type = T_THUMB2_IMM2_IMM3;
    d->instr_flag_type = T_THUMB2_S_FLAG;

    switch (type) {
    case 0:
        if(imm3_imm2 == 0) {
            d->instr_imm_type = T_THUMB2_NO_IMM;
            return I_MOV;
        }

        return I_LSL;

    case 1:
        return I_LSR;

    case 2:
        return I_ASR;

    case 3:
        if(imm3_imm2 == 0) {
            d->instr_imm_type = T_THUMB2_NO_IMM;
            return I_RRX;
        }

        return I_ROR;
    }

    return I_INVLD;
}

darm_instr_t thumb2_data_shifted_reg(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op = (w >> 5) & b1111;
    uint32_t Rn = w & b1111;
    uint32_t Rd_S = ((w2 >> 7) & 0x1e) | ((w >> 4) & 1);

    d->instr_type = T_THUMB2_RN_RD_RM_REG;
    d->instr_imm_type = T_THUMB2_IMM2_IMM3;
    d->instr_flag_type = T_THUMB2_S_TYPE_FLAG;

    switch (op) {
    case 0:
        if(Rd_S == 0x1f) {
            d->instr_type = T_THUMB2_RN_RM_REG;
            d->instr_flag_type = T_THUMB2_TYPE_FLAG;
            return I_TST;
        }

        return I_AND;

    case 1:
        return I_BIC;

    case 2:
        if(Rn == b1111) {
            return thumb2_move_shift(d, w, w2);
        }

        return I_ORR;

    case 3:
        if(Rn == b1111) {
            d->instr_type = T_THUMB2_RD_RM_REG;
            return I_MVN;
        }

        return I_ORN;

    case 4:
        if(Rd_S == 0x1f) {
            d->instr_type = T_THUMB2_RN_RM_REG;
            d->instr_flag_type = T_THUMB2_TYPE_FLAG;
            return I_TEQ;
        }

        return I_EOR;

    case 6:
        d->instr_flag_type = T_THUMB2_S_FLAG;
        return I_PKH;

    case 8:
        if(Rd_S == 0x1f) {
            d->instr_type = T_THUMB2_RN_RM_REG;
            d->instr_flag_type = T_THUMB2_TYPE_FLAG;
            return I_CMN;
        }

        return I_ADD;

    case 10:
        return I_ADC;

    case 11:
        return I_SBC;

    case 13:
        if(Rd_S == 0x1f) {
            d->instr_type = T_THUMB2_RN_RM_REG;
            d->instr_flag_type = T_THUMB2_TYPE_FLAG;
            return I_CMP;
        }

        return I_SUB;

    case 14:
        return I_RSB;
    }

    return I_INVLD;
}

darm_instr_t thumb2_modified_immediate(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op = (w >> 5) & b1111;
    uint32_t Rn = w & b1111;
    uint32_t Rd_S = ((w2 >> 7) & 0x1e) | ((w >> 4) & 1);

    d->instr_type = T_THUMB2_RN_RD_REG;
    d->instr_imm_type = T_THUMB2_IMM1_IMM3_IMM8;
    d->instr_flag_type = T_THUMB2_S_FLAG;

    switch (op) {
    case 0:
        if(Rd_S == 0x1f) {
            d->instr_type = T_THUMB2_RN_REG;
            d->instr_flag_type = T_THUMB2_NO_FLAG;
            return I_TST;
        }

        return I_AND;

    case 1:
        return I_BIC;

    case 2:
        if(Rn == b1111) {
            d->instr_type = T_THUMB2_RD_REG;
            return I_MOV;
        }

        return I_ORR;

    case 3:
        if(Rn == b1111) {
            d->instr_type = T_THUMB2_RD_REG;
            return I_MVN;
        }

        return I_ORN;

    case 4:
        if(Rd_S == 0x1f) {
            d->instr_type = T_THUMB2_RN_REG;
            d->instr_flag_type = T_THUMB2_NO_FLAG;
            return I_TEQ;
        }

        return I_EOR;

    case 8:
        if(Rd_S == 0x1f) {
            d->instr_type = T_THUMB2_RN_REG;
            d->instr_flag_type = T_THUMB2_NO_FLAG;
            return I_CMN;
        }

        return I_ADD;

    case 10:
        return I_ADC;

    case 11:
        return I_SBC;

    case 13:
        if(Rd_S == 0x1f) {
            d->instr_type = T_THUMB2_RN_REG;
            d->instr_flag_type = T_THUMB2_NO_FLAG;
            return I_CMP;
        }

        return I_SUB;

    case 14:
        return I_RSB;
    }

    return I_INVLD;
}

darm_instr_t thumb2_plain_immediate(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op = (w >> 4) & 0x1f;
    uint32_t Rn = w & b1111;

    d->instr_type = T_THUMB2_RN_RD_REG;
    d->instr_imm_type = T_THUMB2_IMM1_IMM3_IMM8;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    switch (op) {
    case 0:
        return Rn == b1111 ? I_ADR : I_ADDW;

    case 4:
        d->instr_type = T_THUMB2_RD_REG;
        return I_MOVW;

    case 10:
        return Rn == b1111 ? I_ADR : I_SUBW;

    case 12:
        d->instr_type = T_THUMB2_RD_REG;
        return I_MOVT;

    case 16:
        d->instr_imm_type = T_THUMB2_IMM2_IMM3;
        return I_SSAT;

    case 18:
        if((w2 & 0x70c0) == 0) {
            d->instr_imm_type = T_THUMB2_NO_IMM;
            return I_SSAT16;
        }

        d->instr_imm_type = T_THUMB2_IMM2_IMM3;
        return I_SSAT;

    case 20:
        d->instr_imm_type = T_THUMB2_IMM2_IMM3;
        return I_SBFX;

    case 22:
        d->instr_imm_type = T_THUMB2_IMM2_IMM3;
        if(Rn == b1111) {
            d->instr_type = T_THUMB2_RD_REG;
            return I_BFC;
        }

        return I_BFI;

    case 24:
        d->instr_imm_type = T_THUMB2_IMM2_IMM3;
        return I_USAT;

    case 26:
        if((w2 & 0x70c0) == 0) {
            d->instr_imm_type = T_THUMB2_NO_IMM;
            return I_USAT16;
        }

        d->instr_imm_type = T_THUMB2_IMM2_IMM3;
        return I_USAT;

    case 28:
        d->instr_imm_type = T_THUMB2_IMM2_IMM3;
        return I_UBFX;
    }

    return I_INVLD;
}

darm_instr_t thumb2_proc_state(darm_t *d, uint16_t w, uint16_t w2)
{
    (void) d; (void) w;

    uint32_t op1 = (w2 >> 8) & b111;
    uint32_t op2 = w2 & 0xff;

    if(op1 == 0) {
        switch (op2) {
        case 0:
            return I_NOP;

        case 1:
            return I_YIELD;

        case 2:
            return I_WFE;

        case 3:
            return I_WFI;

        case 4:
            return I_SEV;

        default:
            if(((op2 >> 4) & b1111) == b1111) {
                return I_DBG;
            }
        }
    }
    else {
        return I_CPS;
    }

    return I_INVLD;
}

darm_instr_t thumb2_misc_ctrl(darm_t *d, uint16_t w, uint16_t w2)
{
    (void) d; (void) w;
    uint32_t op = (w2 >> 4) & b111;

    switch (op) {
    case 0: case 1:
        // TODO: ENTERX, LEAVEX
        return I_NOP;

    case 2:
        return I_CLREX;

    case 4:
        return I_DSB;

    case 5:
        return I_DMB;

    case 6:
        return I_ISB;
    }

    return I_INVLD;
}

darm_instr_t thumb2_branch_misc_ctrl(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op = (w >> 4) & 0x7f;
    uint32_t op1 = (w2 >> 12) & b111;
    uint32_t imm8 = w2 & 0xff;

    d->instr_type = T_THUMB2_NO_REG;
    d->instr_imm_type = T_THUMB2_NO_IMM;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    if(op1 == 0 && op == 0xfe) {
        // TODO return I_HVC
        return I_NOP;
    }
    else if(op1 == 0 && op == 0xff) {
        return I_SMC;
    }
    else if((op1 & b101) == 1) {
        d->instr_flag_type = T_THUMB2_S_FLAG;
        return I_B;
    }
    else if(op1 == 2 && op == 0xff) {
        return I_UDF;
    }
    else if((op1 & b101) == 0) {
        if((op & 0x38) != 0x38) {
            d->instr_flag_type = T_THUMB2_S_FLAG;
            return I_B;
        }
        else if((op & 0x7e) == 0x38 && (imm8 & 0x10) == 0x10) {
            return I_MSR; // banked register
        }
        else if(op == 0x38 && (imm8 & 0x10) == 0) {
            d->instr_type = T_THUMB2_RN_REG;
            return I_MSR; // register
        }
        else if(op == 0x3a) {
            return thumb2_proc_state(d, w, w2);
        }
        else if(op == 0x3b) {
            return thumb2_misc_ctrl(d, w, w2);
        }
        else if(op == 0x3c) {
            d->instr_type = T_THUMB2_RM_REG;
            return I_BXJ;
        }
        else if(op == 0x3e && (imm8 & 0x10) == 0) {
            d->instr_type = T_THUMB2_RD_REG;
            return I_MRS;
        }
    }
    else if((op1 & b101) == b100) {
        d->instr_flag_type = T_THUMB2_S_FLAG;
        return I_BLX;
    }
    else if((op1 & b101) == b101) {
        d->instr_flag_type = T_THUMB2_S_FLAG;
        return I_BL;
    }

    return I_INVLD;
}

darm_instr_t thumb2_store_single_item(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 5) & b111;
    uint32_t op2 = (w2 >> 6) & 0x3f;

    d->instr_type = T_THUMB2_RN_RT_REG;
    d->instr_imm_type = T_THUMB2_IMM8;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    switch (op1) {
    case 0:
        if(op2 == 0) {
            d->instr_type = T_THUMB2_RN_RM_RT_REG;
            d->instr_imm_type = T_THUMB2_IMM2;
            return I_STRB; // register
        }
        else if((op2 & 0x3c) == 0x38) {
            return I_STRBT;
        }
        else if((op2 & 0x3c) == 0x30 || (op2 & 0x24) == 0x24) {
            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_STRB; // immediate 8 bit
        }
        break;

    case 1:
        if(op2 == 0) {
            d->instr_type = T_THUMB2_RN_RM_RT_REG;
            d->instr_imm_type = T_THUMB2_IMM2;
            return I_STRH; // register
        }
        else if((op2 & 0x3c) == 0x38) {
            return I_STRHT;
        }
        else if((op2 & 0x3c) == 0x30 || (op2 & 0x24) == 0x24) {
            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_STRH;  // immediate 8 bit
        }
        break;

    case 2:
        if(op2 == 0) {
            d->instr_type = T_THUMB2_RN_RM_RT_REG;
            d->instr_imm_type = T_THUMB2_IMM2;
            return I_STR; // register
        }
        else if((op2 & 0x3c) == 0x38) {
            return I_STRT;
        }
        else if((op2 & 0x3c) == 0x30 || (op2 & 0x24) == 0x24) {

            // PUSH pseudo single register if Rn == SP,
            // P and W are set, and imm == 4
            if((w & 0xf) == b1101 && (w2 & 0xfff) == 0xd04) {
                d->instr_type = T_THUMB2_RT_REG;
                d->instr_imm_type = T_THUMB2_NO_IMM;
                d->instr_flag_type = T_THUMB2_NO_FLAG;
                return I_PUSH;
            }

            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_STR; // immediate 8 bit
        }
        break;

    case 4:
        d->instr_imm_type = T_THUMB2_IMM12;
        return I_STRB; // immediate 12 bit

    case 5:
        d->instr_imm_type = T_THUMB2_IMM12;
        return I_STRH; // immediate 12 bit

    case 6:
        d->instr_imm_type = T_THUMB2_IMM12;
        return I_STR; // immediate 12 bit
    }

    return I_INVLD;
}

darm_instr_t thumb2_load_byte_hints(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 7) & b11;
    uint32_t op2 = (w2 >> 6) & 0x3f;
    uint32_t Rn = w & b1111;
    uint32_t Rt = (w2 >> 12) & b1111;

    d->instr_type = T_THUMB2_RN_RT_REG;
    d->instr_imm_type = T_THUMB2_IMM12;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    if((op1 & 2) == 0 && Rn == b1111) {
        d->instr_flag_type = T_THUMB2_U_FLAG;
        if(Rt == b1111) {
            d->instr_imm_type = T_THUMB2_NO_REG;
            return I_PLD; // literal
        }

        d->instr_type = T_THUMB2_RT_REG;
        return I_LDRB; // literal
    }
    else if((op1 & 2) == 2 && Rn == b1111) {
        d->instr_flag_type = T_THUMB2_U_FLAG;
        if(Rt == b1111) {
            d->instr_imm_type = T_THUMB2_NO_REG;
            return I_PLI; // immediate, literal
        }

        d->instr_type = T_THUMB2_RT_REG;
        return I_LDRSB; // literal
    }
    else if(op1 == 0) {
        if(op2 == 0) {
            d->instr_imm_type = T_THUMB2_IMM2;
            if(Rt == b1111) {
                d->instr_type = T_THUMB2_RN_RM_REG;
                return I_PLD; // PLD/PLDW register
            }

            d->instr_type = T_THUMB2_RN_RM_RT_REG;
            return I_LDRB; // register
        }
        else if((op2 & 0x24) == 0x24) {
            d->instr_imm_type = T_THUMB2_IMM8;
            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_LDRB; // immediate
        }
        else if((op2 & 0x3c) == 0x30) {
            d->instr_imm_type = T_THUMB2_IMM8;
            if(Rt == b1111) {
                d->instr_type = T_THUMB2_RN_REG;
                return I_PLD; // PLD/PLDW immediate
            }

            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_LDRB; // immediate
        }
        else if((op2 & 0x3c) == 0x38) {
            d->instr_imm_type = T_THUMB2_IMM8;
            return I_LDRBT;
        }
    }
    else if(op1 == 1) {
        if(Rt == b1111) {
            d->instr_type = T_THUMB2_RN_REG;
            return I_PLD; // PLD/PLDW immediate 12
        }

        return I_LDRB; // immediate 12
    }
    else if(op1 == 2) {
        if(op2 == 0) {
            d->instr_imm_type = T_THUMB2_IMM2;
            if(Rt == b1111) {
                d->instr_type = T_THUMB2_RN_RM_REG;
                return I_PLI; // PLI register
            }

            d->instr_type = T_THUMB2_RN_RM_RT_REG;
            return I_LDRSB; // register
        }
        else if((op2 & 0x24) == 0x24) {
            d->instr_imm_type = T_THUMB2_IMM8;
            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_LDRSB; // immediate 8
        }
        else if((op2 & 0x3C) == 0x30) {
            d->instr_imm_type = T_THUMB2_IMM8;
            if(Rt == b1111) {
                d->instr_type = T_THUMB2_RN_REG;
                return I_PLI; // PLI immediate/literal
            }

            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_LDRSB; // immediate 8
        }
        else if((op2 & 0x3C) == 0x38) {
            d->instr_imm_type = T_THUMB2_IMM8;
            return I_LDRSBT;
        }
    }
    else if(op1 == 3) {
        if(Rt == b1111) {
            d->instr_type = T_THUMB2_RN_REG;
            return I_PLI; // PLI literal/immediate
        }

        return I_LDRSB; // immediate 12
    }

    return I_INVLD;
}

darm_instr_t thumb2_load_halfword_hints(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 7) & b11;
    uint32_t op2 = (w2 >> 6) & 0x3f;
    uint32_t Rn = w & b1111;
    uint32_t Rt = (w2 >> 12) & b1111;

    d->instr_type = T_THUMB2_RN_RT_REG;
    d->instr_imm_type = T_THUMB2_IMM12;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    if((op1 & 2) == 0 && Rn == b1111) {
        d->instr_flag_type = T_THUMB2_U_FLAG;
        if(Rt == b1111) {
            return I_PLD; // literal
        }

        d->instr_type = T_THUMB2_RT_REG;
        return I_LDRH; // literal
    }
    else if((op1 & 2) == 2 && Rn == b1111) {
        if(Rt == b1111) {
            return I_NOP; // mem hint
        }

        d->instr_flag_type = T_THUMB2_U_FLAG;
        d->instr_type = T_THUMB2_RT_REG;
        return I_LDRSH; // literal
    }
    else if(op1 == 0) {
        if(op2 == 0) {
            d->instr_imm_type = T_THUMB2_IMM2;
            if(Rt == b1111) {
                d->instr_type = T_THUMB2_RN_RM_REG;
                return I_PLD; // PLD/PLDW register
            }

            d->instr_type = T_THUMB2_RN_RM_RT_REG;
            return I_LDRH; // register
        }
        else if((op2 & 0x24) == 0x24) {
            d->instr_imm_type = T_THUMB2_IMM8;
            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_LDRH; // immediate 8 bit
        }
        else if((op2 & 0x3c) == 0x30) {
            if(Rt == b1111) {
                d->instr_type = T_THUMB2_RN_REG;
                d->instr_imm_type = T_THUMB2_IMM8;
                return I_PLD; // PLD/PLDW immediate 8 bit
            }

            d->instr_imm_type = T_THUMB2_IMM8;
            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_LDRH; // immediate 8 bit
        }
        else if((op2 & 0x3c) == 0x38) {
            d->instr_imm_type = T_THUMB2_IMM8;
            return I_LDRHT;
        }
    }
    else if(op1 == 1) {
        if(Rt == b1111) {
            d->instr_type = T_THUMB2_RN_REG;
            return I_PLD; // PLD/PLDW immediate 12 bit
        }

        return I_LDRH; // immediate 12 bit
    }
    else if(op1 == 2) {
        if(op2 == 0) {
            if(Rt == b1111) {
                return I_NOP;
            }

            d->instr_imm_type = T_THUMB2_IMM2;
            d->instr_type = T_THUMB2_RN_RM_RT_REG;
            return I_LDRSH; // register
        }
        else if((op2 & 0x24) == 0x24) {
            d->instr_imm_type = T_THUMB2_IMM8;
            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_LDRSH; // immediate 8 bit
        }
        else if((op2 & 0x3c) == 0x30) {
            if(Rt == b1111) {
                return I_NOP;
            }

            d->instr_imm_type = T_THUMB2_IMM8;
            d->instr_flag_type = T_THUMB2_WUP_FLAG;
            return I_LDRSH; // immediate 8 bit
        }
        else if ((op2 & 0x3c) == 0x38) {
            d->instr_imm_type = T_THUMB2_IMM8;
            return I_LDRSHT;
        }
    }
    else if(op1 == 3) {
        if(Rt == b1111) {
            return I_NOP;
        }

        return I_LDRSH; // immediate 12 bit
    }

    return I_INVLD;
}

darm_instr_t thumb2_load_word(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 7) & b11;
    uint32_t op2 = (w2 >> 6) & 0x3f;
    uint32_t Rn = w & b1111;

    d->instr_type = T_THUMB2_RN_RT_REG;
    d->instr_imm_type = T_THUMB2_IMM8;
    d->instr_flag_type = T_THUMB2_NO_FLAG;
    d->Rn = Rn;

    if((op1 & 2) == 0 && Rn == b1111) {
        d->instr_type = T_THUMB2_RT_REG;
        d->instr_imm_type = T_THUMB2_IMM12;
        d->instr_flag_type = T_THUMB2_U_FLAG;
        return I_LDR; // literal
    }
    else if(op1 == 1 && Rn != b1111) {
        d->instr_imm_type = T_THUMB2_IMM12;
        return I_LDR; // immediate
    }
    else if(op1 == 0 && Rn != b1111) {
        if(op2 == 0) { d->instr_type = T_THUMB2_RN_RM_RT_REG;
            d->instr_imm_type = T_THUMB2_IMM2;
            return I_LDR; // register
        }
        else if((op2 & 0x3c) == 0x30 || (op2 & 0x24) == 0x24) {
            d->instr_flag_type = T_THUMB2_WUP_FLAG;

            // POP pseudo single register if Rn == SP,
            // U and W are set, and imm == 4
            if((w & 0xf) == b1101 && (w2 & 0xfff) == 0xb04) {
                d->instr_type = T_THUMB2_RT_REG;
                d->instr_imm_type = T_THUMB2_NO_IMM;
                d->instr_flag_type = T_THUMB2_NO_FLAG;
                return I_POP;
            }

            return I_LDR; // immediate
        }
        else if((op2 & 0x3c) == 0x38) {
            return I_LDRT;
        }
    }

    return I_INVLD;
}

darm_instr_t thumb2_parallel_signed(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 4) & b111;
    uint32_t op2 = (w2 >> 4) & b11;

    d->instr_type = T_THUMB2_RN_RD_RM_REG;
    d->instr_imm_type = T_THUMB2_NO_IMM;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    if(op2 == 0) {
        switch (op1) {
        case 0:
            return I_SADD8;

        case 1:
            return I_SADD16;

        case 2:
            return I_SASX;

        case 4:
            return I_SSUB8;

        case 5:
            return I_SSUB16;

        case 6:
            return I_SSAX;
        }
    }
    else if(op2 == 1) {
        switch (op1) {
        case 0:
            return I_QADD8;

        case 1:
            return I_QADD16;

        case 2:
            return I_QASX;

        case 4:
            return I_QSUB8;

        case 5:
            return I_QSUB16;

        case 6:
            return I_QSAX;
        }
    }
    else if(op2 == 2) {
        switch (op1) {
        case 0:
            return I_SHADD8;

        case 1:
            return I_SHADD16;

        case 2:
            return I_SHASX;

        case 4:
            return I_SHSUB8;

        case 5:
            return I_SHSUB16;

        case 6:
            return I_SHSAX;
        }
    }

    return I_INVLD;
}

darm_instr_t thumb2_parallel_unsigned(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 4) & b111;
    uint32_t op2 = (w2 >> 4) & b11;

    d->instr_type = T_THUMB2_RN_RD_RM_REG;
    d->instr_imm_type = T_THUMB2_NO_IMM;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    if(op2 == 0) {
        switch (op1) {
        case 0:
            return I_UADD8;

        case 1:
            return I_UADD16;

        case 2:
            return I_UASX;

        case 4:
            return I_USUB8;

        case 5:
            return I_USUB16;

        case 6:
            return I_USAX;
        }
    }
    else if(op2 == 1) {
        switch (op1) {
        case 0:
            return I_UQADD8;

        case 1:
            return I_UQADD16;

        case 2:
            return I_UQASX;

        case 4:
            return I_UQSUB8;

        case 5:
            return I_UQSUB16;

        case 6:
            return I_UQSAX;
        }
    }
    else if(op2 == 2) {
        switch (op1) {
        case 0:
            return I_UHADD8;

        case 1:
            return I_UHADD16;

        case 2:
            return I_UHASX;

        case 4:
            return I_UHSUB8;

        case 5:
            return I_UHSUB16;

        case 6:
            return I_UHSAX;
        }
    }

    return I_INVLD;
}

darm_instr_t thumb2_misc_op(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 4) & b11;
    uint32_t op2 = (w2 >> 4) & b11;

    if(((w2 >> 12) & b1111) != b1111) {
        return I_INVLD;
    }

    d->instr_type = T_THUMB2_RN_RD_RM_REG;
    d->instr_imm_type = T_THUMB2_NO_IMM;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    switch (op1) {
    case 0:
        switch (op2) {
        case 0:
            return I_QADD;

        case 1:
            return I_QDADD;

        case 2:
            return I_QSUB;

        case 3:
            return I_QDSUB;
        }

    case 1:
        d->instr_type = T_THUMB2_RD_RM_REG;
        switch (op2) {
        case 0:
            return I_REV;


        case 1:
            return I_REV16;


        case 2:
            return I_RBIT;


        case 3:
            return I_REVSH;
        }

    case 2:
        if(op2 == 0) {
            return I_SEL;
        }
        break;

    case 3:
        if(op2 == 0) {
            d->instr_type = T_THUMB2_RD_RM_REG;
            return I_CLZ;
        }
        break;
    }

    return I_INVLD;
}

darm_instr_t thumb2_data_reg(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 4) & b1111;
    uint32_t op2 = (w2 >> 4) & b1111;
    uint32_t Rn = w & b1111;

    d->instr_type = T_THUMB2_RN_RD_RM_REG;
    d->instr_imm_type = T_THUMB2_NO_IMM;
    d->instr_flag_type = T_THUMB2_ROTATE_FLAG;

    if(op2 == 0 && (op1 & b1000) == 0) {
        d->instr_flag_type = T_THUMB2_S_FLAG;
        switch (op1 & b1110) {
        case 0:
            return I_LSL; // register

        case 2:
            return I_LSR; // register

        case 4:
            return I_ASR; // register

        case 6:
            return I_ROR; // register
        }
    }
    else if(op1 < 8 && (op2 & b1000) == 8) {
        d->instr_flag_type = T_THUMB2_ROTATE_FLAG;
        switch(op1) {
        case 0:
            if(Rn == b1111) {
                d->instr_type = T_THUMB2_RD_RM_REG;
                return I_SXTH;
            }

            return I_SXTAH;

        case 1:
            if(Rn == b1111) {
                d->instr_type = T_THUMB2_RD_RM_REG;
                return I_UXTH;
            }

            return I_UXTAH;

        case 2:
            if(Rn == b1111) {
                d->instr_type = T_THUMB2_RD_RM_REG;
                return I_SXTB16;
            }

            return I_SXTAB16;

        case 3:
            if(Rn == b1111) {
                d->instr_type = T_THUMB2_RD_RM_REG;
                return I_UXTB16;
            }

            return I_UXTAB16;

        case 4:
            if(Rn == b1111) {
                d->instr_type = T_THUMB2_RD_RM_REG;
                return I_SXTB;
            }

            return I_SXTAB;

        case 5:
            if(Rn == b1111) {
                d->instr_type = T_THUMB2_RD_RM_REG;
                return I_UXTB;
            }

            return I_UXTAB;
        }
    }
    else if((op1 & b1000) == 8 && (op2 & b1100) == 0) {
        return thumb2_parallel_signed(d, w, w2);
    }
    else if((op1 & b1000) == 8 && (op2 & b1100) == 4) {
        return thumb2_parallel_unsigned(d, w, w2);
    }
    else if((op1 & b1100) == 8 && (op2 & b1100) == 8) {
        return thumb2_misc_op(d, w, w2);
    }

    return I_INVLD;
}

darm_instr_t thumb2_nm_decoder(darm_t *d, uint16_t w, uint16_t w2,
    darm_instr_t i1, darm_instr_t i2, darm_instr_t i3, darm_instr_t i4)
{
    (void) d; (void) w;

    uint32_t n = (w2 >> 5) & 1;
    uint32_t m = (w2 >> 4) & 1;

    if(n == 1) {
        if(m == 1) {
            return i4;
        }

        return i3;
    }
    else {
        if(m == 1) {
            return i2;
        }

        return i1;
    }

    return I_INVLD;
}

darm_instr_t thumb2_mult_acc_diff(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 4) & b111;
    uint32_t op2 = (w2 >> 4) & b11;
    uint32_t Ra = (w2 >> 12) & b1111;

    if(((w2 >> 6) & b11) != 0) {
        return I_INVLD;
    }

    d->instr_type = T_THUMB2_RN_RD_RM_RA_REG;
    d->instr_imm_type = T_THUMB2_NO_IMM;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    if(op1 == 1) {
        if(Ra == b1111) {
            return thumb2_nm_decoder(d, w, w2,
                I_SMULBB, I_SMULBT, I_SMULTB, I_SMULTT);
        }

        return thumb2_nm_decoder(d, w, w2,
            I_SMLABB, I_SMLABT, I_SMLATB, I_SMLATT);
    }

    if((op2 & 2) != 0) {
        return I_INVLD;
    }

    switch (op1) {
    case 0:
        if(op2 == 0 && Ra == b1111) {
            d->instr_type = T_THUMB2_RN_RD_RM_REG;
            return I_MUL;
        }
        else if(op2 == 0 && Ra != b1111) {
            return I_MLA;
        }
        else if(op2 == 1) {
            return I_MLS;
        }

        break;

    case 2:
        if(Ra == b1111) {
            d->instr_type = T_THUMB2_RN_RD_RM_REG;
            return I_SMUAD;
        }

        return I_SMLAD;

    case 3:
        // B or T variant indicated by M bit but not encoded
        // in the instruction name
        if(Ra == b1111) {
            d->instr_type = T_THUMB2_RN_RD_RM_REG;
            return I_SMULW;
        }

        return I_SMLAW;

    case 4:
        if(Ra == b1111) {
            d->instr_type = T_THUMB2_RN_RD_RM_REG;
            return I_SMUSD;
        }

        return I_SMLSD;

    case 5:
        if(Ra == b1111) {
            d->instr_type = T_THUMB2_RN_RD_RM_REG;
            return I_SMMUL;
        }

        return I_SMMLA;

    case 6:
        return I_SMMLS;

    case 7:
        if(op2 == 0 && Ra == b1111) {
            d->instr_type = T_THUMB2_RN_RD_RM_REG;
            return I_USAD8;
        }
        else if(op2 == 0) {
            return I_USADA8;
        }

        break;
    }

    return I_INVLD;
}

darm_instr_t thumb2_long_mult_acc(darm_t *d, uint16_t w, uint16_t w2)
{
    uint32_t op1 = (w >> 4) & b111;
    uint32_t op2 = (w2 >> 4) & b1111;

    d->instr_type = T_THUMB2_RN_RM_REG;
    d->instr_imm_type = T_THUMB2_NO_IMM;
    d->instr_flag_type = T_THUMB2_NO_FLAG;

    switch (op1) {
    case 0:
        if(op2 == 0) {
            return I_SMULL;
        }

        break;

    case 1:
        if(op2 == b1111) {
            d->instr_type = T_THUMB2_RN_RD_RM_REG;
            return I_SDIV;
        }

        break;

    case 2:
        if(op2 == 0) {
            return I_UMULL;
        }

        break;

    case 3:
        if(op2 == b1111) {
            d->instr_type = T_THUMB2_RN_RD_RM_REG;
            return I_UDIV;
        }

        break;

    case 4:
        if(op2 == 0) {
            return I_SMLAL;
        }
        else if((op2 & b1100) == b1000) {
            return thumb2_nm_decoder(d, w, w2,
                I_SMLALBB, I_SMLALBT, I_SMLALTB, I_SMLALTT);
        }
        else if((op2 & b1110) == b1100) {
            return I_SMLALD;
        }

        break;

    case 5:
        if((op2 & b1110) == b1100) {
            return I_SMLSLD;
        }

        break;

    case 6:
        if(op2 == 0) {
            return I_UMLAL;
        }
        else if(op2 == b110) {
            return I_UMAAL;
        }

        break;
    }

    return I_INVLD;
}

darm_instr_t thumb2_coproc_simd(darm_t *d, uint16_t w, uint16_t w2)
{
    (void) d; (void) w; (void) w2;

    // TODO implement
    return I_INVLD;
}
