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

#ifndef __DARM__
#define __DARM__

#include "armv7-tbl.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#define B_UNSET 0
#define B_SET   1
#define B_INVLD 2

typedef enum _darm_reg_t {
    r0 = 0, r1 = 1, r2 = 2, r3 = 3, r4 = 4, r5 = 5, r6 = 6, r7 = 7, r8 = 8,
    r9 = 9, r10 = 10, r11 = 11, r12 = 12, r13 = 13, r14 = 14, r15 = 15,

    FP = 11, IP = 12, SP = 13, LR = 14, PC = 15,

    cr0 = 0, cr1 = 1, cr2 = 2, cr3 = 3, cr4 = 4, cr5 = 5, cr6 = 6, cr7 = 7,
    cr8 = 8, cr9 = 9, cr10 = 10, cr11 = 11, cr12 = 12, cr13 = 13, cr14 = 14,
    cr15 = 15,

    R_INVLD = -1
} darm_reg_t;

typedef enum _darm_cond_t {
    C_EQ = 0, C_NE = 1, C_CS = 2, C_CC = 3, C_MI = 4,
    C_PL = 5, C_VS = 6, C_VC = 7, C_HI = 8, C_LS = 9,
    C_GE = 10, C_LT = 11, C_GT = 12, C_LE = 13, C_AL = 14,

    C_HS = C_CS, C_LO = C_CC,
    C_UNCOND = 15,

    C_INVLD = -1
} darm_cond_t;

typedef enum _darm_shift_type_t {
    S_LSL = 0, S_LSR = 1, S_ASR = 2, S_ROR = 3,

    S_INVLD = -1,
} darm_shift_type_t;

typedef enum _darm_option_t {
    O_SY    = 15, // b1111
    O_ST    = 14, // b1110
    O_ISH   = 11, // b1011
    O_ISHST = 10, // b1010
    O_NSH   = 7,  // b0111
    O_NSHST = 6,  // b0110
    O_OSH   = 3,  // b0011
    O_OSHST = 2,  // b0010

    O_INVLD = -1,
} darm_option_t;

typedef struct _darm_t {
    // the original encoded instruction
    uint32_t        w;

    // the instruction label
    darm_instr_t    instr;
    darm_enctype_t  instr_type;
    darm_enctype_t  instr_imm_type;  // thumb2 immediate type
    darm_enctype_t  instr_flag_type; // thumb2 flag type

    // conditional flags, if any
    darm_cond_t     cond;

    // if set, swap only one byte, otherwise swap four bytes
    uint32_t        B;

    // does this instruction update the conditional flags?
    uint32_t        S;

    // endian specifier for the SETEND instruction
    uint32_t        E;

    // whether halfwords should be swapped before various signed
    // multiplication operations
    uint32_t        M;

    // specifies, together with the M flag, which half of the source
    // operand is used to multiply
    uint32_t        N;

    // option operand for the DMB, DSB and ISB instructions
    darm_option_t   option;

    // to add or to subtract the immediate, this is used for instructions
    // which take a relative offset to a pointer or to the program counter
    uint32_t        U;

    // the bit for the unconditional BLX instruction which allows one to
    // branch with link to a 2-byte aligned thumb2 instruction
    uint32_t        H;

    // specifies whether this instruction uses pre-indexed addressing or
    // post-indexed addressing
    uint32_t        P;

    // specifies whether signed multiplication results should be rounded
    // or not
    uint32_t        R;

    // the PKH instruction has two variants, namely, PKHBT and PKHTB, the
    // tbform is represented by T, i.e., if T = 1 then the instruction is
    // PKHTB, otherwise it's PKHBT
    uint32_t        T;

    // write-back bit
    uint32_t        W;

    // flag which specifies whether an immediate has been set
    uint32_t        I;

    // rotation value
    uint32_t        rotate;

    // register operands
    darm_reg_t      Rd; // destination
    darm_reg_t      Rn; // first operand
    darm_reg_t      Rm; // second operand
    darm_reg_t      Ra; // accumulate operand
    darm_reg_t      Rt; // transferred operand
    darm_reg_t      Rt2; // second transferred operand

    // for instructions which produce a 64bit output we have to specify a
    // high and a low 32bits destination register
    darm_reg_t      RdHi; // high 32bits destination
    darm_reg_t      RdLo; // low 32bits destination

    // immediate operand
    uint32_t        imm;
    uint32_t        sat_imm;

    // register shift info
    darm_shift_type_t shift_type;
    darm_reg_t      Rs;
    uint32_t        shift;

    // certain instructions operate on bits, they specify the lowest or highest
    // significant bit to be used, as well as the width, the amount of bits
    // that are affected
    uint32_t        lsb;
    uint32_t        msb;
    uint32_t        width;

    // bitmask of registers affected by the STM/LDM/PUSH/POP instruction
    uint16_t        reglist;

    // special registers and values for the MRC/MCR/etc instructions
    uint8_t         coproc;
    uint8_t         opc1;
    uint8_t         opc2;
    darm_reg_t      CRd;
    darm_reg_t      CRn;
    darm_reg_t      CRm;
    uint32_t        D;

    // condition and mask for the IT instruction
    darm_cond_t     firstcond;
    uint8_t         mask;
} darm_t;

typedef struct _darm_str_t {
    // the full mnemonic, including extensions, flags, etc.
    char mnemonic[12];

    // a representation of each argument in a separate string
    char arg[6][32];

    // representation of shifting, if present
    char shift[12];

    // the entire instruction
    char total[64];
} darm_str_t;

// reset a darm object, this function is internally called right before using
// any of the disassemble routines, hence a user is normally not required to
// call this function beforehand
void darm_init(darm_t *d);

// disassemble an armv7 instruction
int darm_armv7_disasm(darm_t *d, uint32_t w);

// disassemble a thumb instruction
int darm_thumb_disasm(darm_t *d, uint16_t w);

// disassemble a thumb2 instruction
int darm_thumb2_disasm(darm_t *d, uint16_t w, uint16_t w2);

//
// Disassembles an instruction - determines instruction set
// (ARMv7 or Thumb/Thumb2) based on the address and determines Thumb or
// Thumb2 mode based on the instruction itself.
//
// Takes two 16 bit words as input, the first representing the lower 16 bits
// and the second 16 bit word representing the upper 16 bits of the possibly
// full 32 bits.
//
// Returns 0 on failure, 1 for Thumb, 2 for Thumb2, and 2 for ARMv7. In other
// words, the function returns the amount of 16 bit words that were used to
// disassemble this instruction.
//
// Note that, in order to instruct the disassembler to disassemble a Thumb or
// Thumb2 instruction, the address has to have the least significant bit set.
// That is, given a 4-byte aligned addr, addr is disassembled as 32bit ARMv7
// instruction, addr+1 is disassembled as Thumb or Thumb2 instruction, and
// addr+3 is also disassembled as Thumb/Thumb2. Furthermore, addr+2 is
// disassembled as ARMv7, but do not rely on this being defined behavior in
// the ARM CPU.
//
int darm_disasm(darm_t *d, uint16_t w, uint16_t w2, uint32_t addr);

int darm_immshift_decode(const darm_t *d, const char **type,
    uint32_t *immediate);

const char *darm_mnemonic_name(darm_instr_t instr);
const char *darm_enctype_name(darm_enctype_t enctype);
const char *darm_register_name(darm_reg_t reg);
const char *darm_shift_type_name(darm_shift_type_t shifttype);

// postfix for each condition, e.g., EQ, NE
const char *darm_condition_name(darm_cond_t cond, int omit_always_execute);

// meaning if this condition is used for regular instructions
const char *darm_condition_meaning_int(darm_cond_t cond);

// meaning if this condition is used for floating point instructions
const char *darm_condition_meaning_fp(darm_cond_t cond);

// look up a condition code, e.g., "EQ" => C_EQ
darm_cond_t darm_condition_index(const char *condition_code);

int darm_reglist(uint16_t reglist, char *out);
void darm_dump(const darm_t *d);

int darm_str(const darm_t *d, darm_str_t *str);
int darm_str2(const darm_t *d, darm_str_t *str, int lowercase);

#endif
