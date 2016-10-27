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
#ifndef __DARM_TBL__
#define __DARM_TBL__
#include <stdint.h>
typedef enum _darm_enctype_t {
    // info:
    // Invalid or non-existent type
    //
    // encodings:
    // I_INVLD
    //
    // affects:
    // 
    T_INVLD,

    // info:
    // ADR Instruction, which is an optimization of ADD
    //
    // encodings:
    // ADR<c> <Rd>,<label>
    //
    // affects:
    // ADR
    T_ARM_ADR,

    // info:
    // All unconditional instructions
    //
    // encodings:
    // ins <endian_specifier>
    // ins [<Rn>,#+/-<imm12>]
    // ins [<Rn>,#<imm12>]
    // ins
    // ins #<option>
    // ins <label>
    //
    // affects:
    // BLX, CLREX, DMB, DSB, ISB, PLD, PLI, SETEND
    T_ARM_UNCOND,

    // info:
    // All multiplication instructions
    //
    // encodings:
    // ins{S}<c> <Rd>,<Rn>,<Rm>
    // ins{S}<c> <Rd>,<Rn>,<Rm>,<Ra>
    // ins{S}<c> <RdLo>,<RdHi>,<Rn>,<Rm>
    //
    // affects:
    // MLA, MLS, MUL, SMLAL, SMULL, UMAAL, UMLAL, UMULL
    T_ARM_MUL,

    // info:
    // Various STR and LDR instructions
    //
    // encodings:
    // ins<c> <Rt>,[<Rn>,#+/-<imm12>]
    // ins<c> <Rt>,[<Rn>],#+/-<imm12>
    // ins<c> <Rt>,[<Rn>],+/-<Rm>{,<shift>}
    //
    // affects:
    // LDR, LDRB, LDRBT, LDRT, POP, PUSH, STR, STRB, STRBT, STRT
    T_ARM_STACK0,

    // info:
    // Various unprivileged STR and LDR instructions
    //
    // encodings:
    // ins<c> <Rt>,[<Rn>],+/-<Rm>
    // ins<c> <Rt>,[<Rn>]{,#+/-<imm8>}
    //
    // affects:
    // LDRHT, LDRSBT, LDRSHT, STRHT
    T_ARM_STACK1,

    // info:
    // Various other STR and LDR instructions
    //
    // encodings:
    // ins<c> <Rt>,<Rt2>,[<Rn>],+/-<Rm>
    // ins<c> <Rt>,[<Rn>],+/-<Rm>
    // ins<c> <Rt>,<Rt2>,[<Rn>],#+/-<imm8>
    // ins<c> <Rt>,<Rt2>,[<Rn>,#+/-<imm8>]
    // ins<c> <Rt>,[<Rn>,#+/-<imm8>]
    //
    // affects:
    // LDRD, LDRH, LDRSB, LDRSH, STRD, STRH
    T_ARM_STACK2,

    // info:
    // Arithmetic instructions which take a shift for the second source
    //
    // encodings:
    // ins{S}<c> <Rd>,<Rn>,<Rm>{,<shift>}
    // ins{S}<c> <Rd>,<Rn>,<Rm>,<type> <Rs>
    //
    // affects:
    // ADC, ADD, AND, BIC, EOR, ORR, RSB, RSC, SBC, SUB
    T_ARM_ARITH_SHIFT,

    // info:
    // Arithmetic instructions which take an immediate as second source
    //
    // encodings:
    // ins{S}<c> <Rd>,<Rn>,#<const>
    //
    // affects:
    // ADC, ADD, AND, BIC, EOR, ORR, RSB, RSC, SBC, SUB
    T_ARM_ARITH_IMM,

    // info:
    // Bit field magic
    //
    // encodings:
    // 
    //
    // affects:
    // BFC, BFI, SBFX, UBFX
    T_ARM_BITS,

    // info:
    // Branch and System Call instructions
    //
    // encodings:
    // B(L)<c> <label>
    // SVC<c> #<imm24>
    //
    // affects:
    // B, BL, SVC
    T_ARM_BRNCHSC,

    // info:
    // Branch and Misc instructions
    //
    // encodings:
    // B(L)X(J)<c> <Rm>
    // BKPT #<imm16>
    // MSR<c> <spec_reg>,<Rn>
    //
    // affects:
    // BKPT, BLX, BX, BXJ, MSR, SMLAW, SMULW
    T_ARM_BRNCHMISC,

    // info:
    // Move immediate to a register (possibly negating it)
    //
    // encodings:
    // ins{S}<c> <Rd>,#<const>
    //
    // affects:
    // MOV, MOVT, MOVW, MVN
    T_ARM_MOV_IMM,

    // info:
    // Comparison instructions which take two operands
    //
    // encodings:
    // ins<c> <Rn>,<Rm>{,<shift>}
    // ins<c> <Rn>,<Rm>,<type> <Rs>
    //
    // affects:
    // CMN, CMP, TEQ, TST
    T_ARM_CMP_OP,

    // info:
    // Comparison instructions which take an immediate
    //
    // encodings:
    // ins<c> <Rn>,#<const>
    //
    // affects:
    // CMN, CMP, TEQ, TST
    T_ARM_CMP_IMM,

    // info:
    // Instructions which don't take any operands
    //
    // encodings:
    // ins<c>
    //
    // affects:
    // NOP, SEV, WFE, WFI, YIELD
    T_ARM_OPLESS,

    // info:
    // Manipulate and move a register to another register
    //
    // encodings:
    // ins{S}<c> <Rd>,<Rm>
    // ins{S}<c> <Rd>,<Rm>,#<imm>
    // ins{S}<c> <Rd>,<Rn>,<Rm>
    //
    // affects:
    // ASR, LDREXD, LSL, LSR, MOV, ROR, RRX, STREXD
    T_ARM_DST_SRC,

    // info:
    // Load or store multiple registers at once
    //
    // encodings:
    // ins<c> <Rn>{!},<registers>
    //
    // affects:
    // LDM, LDMDA, LDMDB, LDMIB, PUSH, STM, STMDA, STMDB, STMIB
    T_ARM_LDSTREGS,

    // info:
    // Bit reverse instructions
    //
    // encodings:
    // ins<c> <Rd>,<Rm>
    //
    // affects:
    // CLZ, RBIT, REV, REV16, REVSH
    T_ARM_BITREV,

    // info:
    // Various miscellaneous instructions
    //
    // encodings:
    // ins{S}<c> <Rd>,<Rm>,<type> <Rs>
    // ins{S}<c> <Rd>,<Rm>{,<shift>}
    // ins<c> #<imm4>
    // ins<c> #<option>
    // ins<c> <Rd>,<Rn>,<Rm>{,<type> #<imm>}
    // ins<c> <Rd>,<Rn>,<Rm>
    //
    // affects:
    // DBG, MVN, PKH, SEL, SMC
    T_ARM_MISC,

    // info:
    // Various signed multiply instructions
    //
    // encodings:
    // 
    //
    // affects:
    // SMLA, SMLAD, SMLAL, SMLALD, SMLSD, SMLSLD, SMMLA, SMMLS, SMMUL, SMUAD,
    // SMUL, SMUSD
    T_ARM_SM,

    // info:
    // Parallel signed and unsigned addition and subtraction
    //
    // encodings:
    // ins<c> <Rd>,<Rn>,<Rm>
    //
    // affects:
    // QADD16, QADD8, QASX, QSAX, QSUB16, QSUB8, SADD16, SADD8, SASX, SHADD16,
    // SHADD8, SHASX, SHSAX, SHSUB16, SHSUB8, SSAX, SSUB16, SSUB8, UADD16, UADD8,
    // UASX, UHADD16, UHADD8, UHASX, UHSAX, UHSUB16, UHSUB8, UQADD16, UQADD8,
    // UQASX, UQSAX, UQSUB16, UQSUB8, USAX, USUB16, USUB8
    T_ARM_PAS,

    // info:
    // Saturating addition and subtraction instructions
    //
    // encodings:
    // ins<c> <Rd>,<Rn>,<Rm>
    //
    // affects:
    // QADD, QDADD, QDSUB, QSUB
    T_ARM_SAT,

    // info:
    // Synchronization primitives
    //
    // encodings:
    // ins{B}<c> <Rt>,<Rt2>,[<Rn>]
    // ins<c> <Rd>,<Rt>,[<Rn>]
    // ins<c> <Rt>,<Rt2>,[<Rn>]
    // ins<c> <Rt>,[<Rn>]
    //
    // affects:
    // LDREX, LDREXB, LDREXH, STREX, STREXB, STREXH, SWP, SWPB
    T_ARM_SYNC,

    // info:
    // Packing, unpacking, saturation, and reversal instructions
    //
    // encodings:
    // ins<c> <Rd>,#<imm>,<Rn>
    // ins<c> <Rd>,#<imm>,<Rn>{,<shift>}
    // ins<c> <Rd>,<Rn>,<Rm>{,<rotation>}
    // ins<c> <Rd>,<Rm>{,<rotation>}
    //
    // affects:
    // SSAT, SSAT16, SXTAB, SXTAB16, SXTAH, SXTB, SXTB16, SXTH, USAT, USAT16,
    // UXTAB, UXTAB16, UXTAH, UXTB, UXTB16, UXTH
    T_ARM_PUSR,

    // info:
    // Move to/from Coprocessor to/from ARM core register
    //
    // encodings:
    // ins<c> <coproc>,<opc1>,<Rt>,<CRn>,<CRm>,{,<opc2>}
    //
    // affects:
    // CDP, MCR, MRC
    T_ARM_MVCR,

    // info:
    // Permanently Undefined Instruction
    //
    // encodings:
    // ins<c> #<imm>
    //
    // affects:
    // UDF
    T_ARM_UDF,

    // info:
    // Instructions which only take an 8-byte immediate
    //
    // encodings:
    // ins<c> #<imm8>
    //
    // affects:
    // BKPT, SVC, UDF
    T_THUMB_ONLY_IMM8,

    // info:
    // Conditional branch
    //
    // encodings:
    // ins<c> <label>
    //
    // affects:
    // B
    T_THUMB_COND_BRANCH,

    // info:
    // Unconditional branch
    //
    // encodings:
    // ins<c> <label>
    //
    // affects:
    // B
    T_THUMB_UNCOND_BRANCH,

    // info:
    // Shifting instructions which take an immediate
    //
    // encodings:
    // ins<c> <Rd>, <Rm>, #<imm>
    //
    // affects:
    // ASR, LSL, LSR
    T_THUMB_SHIFT_IMM,

    // info:
    // Load from and Store to the stack
    //
    // encodings:
    // ins<c> <Rt>, [SP, #<imm>]
    //
    // affects:
    // LDR, STR
    T_THUMB_STACK,

    // info:
    // Load a value relative to PC
    //
    // encodings:
    // ins<c> <Rt>, <label>
    //
    // affects:
    // LDR
    T_THUMB_LDR_PC,

    // info:
    // Various General Purpose Instructions
    //
    // encodings:
    // ins<c> <Rdn>, <Rm>
    // ins<c> <Rn>, <Rm>
    // ins<c> <Rdm>, <Rn>, <Rdm>
    // ins<c> <Rd>, <Rm>
    //
    // affects:
    // ADC, AND, ASR, BIC, CMN, CMP, EOR, LSL, LSR, MUL, MVN, ORR, ROR, RSB, SBC,
    // TST
    T_THUMB_GPI,

    // info:
    // Branch (and optionally link) to a Register
    //
    // encodings:
    // ins<c> <Rm>
    //
    // affects:
    // BLX, BX
    T_THUMB_BRANCH_REG,

    // info:
    // If-Then and Hints
    //
    // encodings:
    // ins<c>
    //
    // affects:
    // IT, NOP, SEV, WFE, WFI, YIELD
    T_THUMB_IT_HINTS,

    // info:
    // Instructions with an 8bit immediate
    //
    // encodings:
    // ins<c> <Rdn>, #<imm>
    // ins<c> <Rd>, SP, #<imm>
    // ins<c> <Rd>, <label>
    // ins<c> <Rn>, #<imm>
    // ins<c> <Rd>, #<imm>
    //
    // affects:
    // ADD, ADR, CMP, MOV, SUB
    T_THUMB_HAS_IMM8,

    // info:
    // Bit Extension instructions
    //
    // encodings:
    // ins<c> <Rd>, <Rm>
    //
    // affects:
    // SXTB, SXTH, UXTB, UXTH
    T_THUMB_EXTEND,

    // info:
    // Modifies the Stack Pointer by an Immediate
    //
    // encodings:
    // ins<c> SP, SP, #<imm>
    //
    // affects:
    // ADD, SUB
    T_THUMB_MOD_SP_IMM,

    // info:
    // Instructions with 3 registers as operands
    //
    // encodings:
    // ins<c> <Rd>, <Rn>, <Rm>
    //
    // affects:
    // ADD, SUB
    T_THUMB_3REG,

    // info:
    // Instructions with two registers and an immediate
    //
    // encodings:
    // ins<c> <Rd>, <Rn>, #<imm>
    //
    // affects:
    // ADD, SUB
    T_THUMB_2REG_IMM,

    // info:
    // Add SP with an Immediate to a register
    //
    // encodings:
    // ins<c> <Rd>, SP, #<imm>
    //
    // affects:
    // ADD
    T_THUMB_ADD_SP_IMM,

    // info:
    // Move operation with 4bit registers
    //
    // encodings:
    // ins<c> <Rd>, <Rn>
    //
    // affects:
    // MOV
    T_THUMB_MOV4,

    // info:
    // Instructions to manipulate memory
    //
    // encodings:
    // ins<c> <Rt>, [<Rn>, #<imm>]
    //
    // affects:
    // LDR, LDRB, LDRH, STR, STRB, STRH
    T_THUMB_RW_MEMI,

    // info:
    // Instructions to manipulate memory
    //
    // encodings:
    // ins<c> <Rt>, [<Rn>, <Rm>]
    //
    // affects:
    // LDR, LDRB, LDRH, LDRSB, LDRSH, STR, STRB, STRH
    T_THUMB_RW_MEMO,

    // info:
    // Load and Store register lists
    //
    // encodings:
    // ins<c> <Rn>{!}, <registers>
    //
    // affects:
    // LDM, STM
    T_THUMB_RW_REG,

    // info:
    // Various Reverse instructions
    //
    // encodings:
    // ins<c> <Rd>, <Rm>
    //
    // affects:
    // REV, REV16, REVSH
    T_THUMB_REV,

    // info:
    // Set Endian instruction
    //
    // encodings:
    // ins <endian_specifier>
    //
    // affects:
    // SETEND
    T_THUMB_SETEND,

    // info:
    // Push and Pop registers
    //
    // encodings:
    // ins<c> <registers>
    //
    // affects:
    // POP, PUSH
    T_THUMB_PUSHPOP,

    // info:
    // Comparison instruction
    //
    // encodings:
    // ins<c> <Rn>, <Rm>
    //
    // affects:
    // CMP
    T_THUMB_CMP,

    // info:
    // Add a Register to the Stack Pointer
    //
    // encodings:
    // ins<c> SP, <Rm>
    //
    // affects:
    // ADD
    T_THUMB_MOD_SP_REG,

    // info:
    // Compare and Branch on (Non)Zero
    //
    // encodings:
    // ins<c> <Rn>, <label>
    //
    // affects:
    // CBZ
    T_THUMB_CBZ,

    // info:
    // Instructions that do not operate on a register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_NO_REG,

    // info:
    // Instructions that operate on Rt register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RT_REG,

    // info:
    // Instructions that operate on Rt and Rt2 register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RT_RT2_REG,

    // info:
    // Instructions that operate on the Rm register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RM_REG,

    // info:
    // Instructions that operate on the Rd register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RD_REG,

    // info:
    // Instructions that operate on the Rd and Rm register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RD_RM_REG,

    // info:
    // Instructions that operate on the Rn register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_REG,

    // info:
    // Instructions that operate on the Rn and Rt register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RT_REG,

    // info:
    // Instructions that operate on the Rn, Rt and Rt2 register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RT_RT2_REG,

    // info:
    // Instructions that operate on the Rn and Rm register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RM_REG,

    // info:
    // Instructions that operate on the Rn, Rm and Rt register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RM_RT_REG,

    // info:
    // Instructions that operate on the Rn and Rd register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RD_REG,

    // info:
    // Instructions that operate on the Rn, Rd and Rt register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RD_RT_REG,

    // info:
    // Instructions that operate on the Rn, Rd, Rt and Rt2 register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RD_RT_RT2_REG,

    // info:
    // Instructions that operate on the Rn, Rd and Rm register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RD_RM_REG,

    // info:
    // Instructions that operate on the Rn, Rd, Rm and Ra register
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_RN_RD_RM_RA_REG,

    // info:
    // Instructions that do not operate on an immediate
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_NO_IMM,

    // info:
    // Instructions that use a 12 bit immediate
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_IMM12,

    // info:
    // Instructions that use an 8 bit immediate
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_IMM8,

    // info:
    // Instructions that use a 2 bit immediate
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_IMM2,

    // info:
    // Instructions that use a 2 and 3 bit immediate
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_IMM2_IMM3,

    // info:
    // Instructions that use a 1, 3 and 8 bit immediate
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_IMM1_IMM3_IMM8,

    // info:
    // Instructions that have no flags
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_NO_FLAG,

    // info:
    // Instructions that use the rotate flag
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_ROTATE_FLAG,

    // info:
    // Instructions that use the U flag
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_U_FLAG,

    // info:
    // Instructions that use the WUP flags
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_WUP_FLAG,

    // info:
    // Instructions that use the shift type flag
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_TYPE_FLAG,

    // info:
    // Instructions that use the register list
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_REGLIST_FLAG,

    // info:
    // Instructions that use the WP flags and register list
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_WP_REGLIST_FLAG,

    // info:
    // Instructions that use the S flag
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_S_FLAG,

    // info:
    // Instructions that use the S flag and shift type flag
    //
    // encodings:
    // 
    //
    // affects:
    // 
    T_THUMB2_S_TYPE_FLAG,
} darm_enctype_t;

typedef enum _darm_instr_t {
    I_INVLD, I_ADC, I_ADD, I_ADDW, I_ADR, I_AND, I_ASR, I_B, I_BFC, I_BFI,
    I_BIC, I_BKPT, I_BL, I_BLX, I_BX, I_BXJ, I_CBNZ, I_CBZ, I_CDP, I_CDP2,
    I_CHKA, I_CLREX, I_CLZ, I_CMN, I_CMP, I_CPS, I_CPY, I_DBG, I_DMB, I_DSB,
    I_ENTERX, I_EOR, I_HB, I_HBL, I_HBLP, I_HBP, I_ISB, I_IT, I_LDC, I_LDC2,
    I_LDM, I_LDMDA, I_LDMDB, I_LDMIB, I_LDR, I_LDRB, I_LDRBT, I_LDRD, I_LDREX,
    I_LDREXB, I_LDREXD, I_LDREXH, I_LDRH, I_LDRHT, I_LDRSB, I_LDRSBT, I_LDRSH,
    I_LDRSHT, I_LDRT, I_LEAVEX, I_LSL, I_LSR, I_MCR, I_MCR2, I_MCRR, I_MCRR2,
    I_MLA, I_MLS, I_MOV, I_MOVT, I_MOVW, I_MRC, I_MRC2, I_MRRC, I_MRRC2,
    I_MRS, I_MSR, I_MUL, I_MVN, I_NEG, I_NOP, I_ORN, I_ORR, I_PKH, I_PLD,
    I_PLDW, I_PLI, I_POP, I_PUSH, I_QADD, I_QADD16, I_QADD8, I_QASX, I_QDADD,
    I_QDSUB, I_QSAX, I_QSUB, I_QSUB16, I_QSUB8, I_RBIT, I_REV, I_REV16,
    I_REVSH, I_RFE, I_ROR, I_RRX, I_RSB, I_RSC, I_SADD16, I_SADD8, I_SASX,
    I_SBC, I_SBFX, I_SDIV, I_SEL, I_SETEND, I_SEV, I_SHADD16, I_SHADD8,
    I_SHASX, I_SHSAX, I_SHSUB16, I_SHSUB8, I_SMC, I_SMLA, I_SMLABB, I_SMLABT,
    I_SMLAD, I_SMLAL, I_SMLALBB, I_SMLALBT, I_SMLALD, I_SMLALTB, I_SMLALTT,
    I_SMLATB, I_SMLATT, I_SMLAW, I_SMLSD, I_SMLSLD, I_SMMLA, I_SMMLS, I_SMMUL,
    I_SMUAD, I_SMUL, I_SMULBB, I_SMULBT, I_SMULL, I_SMULTB, I_SMULTT, I_SMULW,
    I_SMUSD, I_SRS, I_SSAT, I_SSAT16, I_SSAX, I_SSUB16, I_SSUB8, I_STC,
    I_STC2, I_STM, I_STMDA, I_STMDB, I_STMIB, I_STR, I_STRB, I_STRBT, I_STRD,
    I_STREX, I_STREXB, I_STREXD, I_STREXH, I_STRH, I_STRHT, I_STRT, I_SUB,
    I_SUBW, I_SVC, I_SWP, I_SWPB, I_SXTAB, I_SXTAB16, I_SXTAH, I_SXTB,
    I_SXTB16, I_SXTH, I_TBB, I_TBH, I_TEQ, I_TST, I_UADD16, I_UADD8, I_UASX,
    I_UBFX, I_UDF, I_UDIV, I_UHADD16, I_UHADD8, I_UHASX, I_UHSAX, I_UHSUB16,
    I_UHSUB8, I_UMAAL, I_UMLAL, I_UMULL, I_UQADD16, I_UQADD8, I_UQASX,
    I_UQSAX, I_UQSUB16, I_UQSUB8, I_USAD8, I_USADA8, I_USAT, I_USAT16, I_USAX,
    I_USUB16, I_USUB8, I_UXTAB, I_UXTAB16, I_UXTAH, I_UXTB, I_UXTB16, I_UXTH,
    I_VABA, I_VABAL, I_VABD, I_VABDL, I_VABS, I_VACGE, I_VACGT, I_VACLE,
    I_VACLT, I_VADD, I_VADDHN, I_VADDL, I_VADDW, I_VAND, I_VBIC, I_VBIF,
    I_VBIT, I_VBSL, I_VCEQ, I_VCGE, I_VCGT, I_VCLE, I_VCLS, I_VCLT, I_VCLZ,
    I_VCMP, I_VCMPE, I_VCNT, I_VCVT, I_VCVTB, I_VCVTR, I_VCVTT, I_VDIV,
    I_VDUP, I_VEOR, I_VEXT, I_VHADD, I_VHSUB, I_VLD1, I_VLD2, I_VLD3, I_VLD4,
    I_VLDM, I_VLDR, I_VMAX, I_VMIN, I_VMLA, I_VMLAL, I_VMLS, I_VMLSL, I_VMOV,
    I_VMOVL, I_VMOVN, I_VMRS, I_VMSR, I_VMUL, I_VMULL, I_VMVN, I_VNEG,
    I_VNMLA, I_VNMLS, I_VNMUL, I_VORN, I_VORR, I_VPADAL, I_VPADD, I_VPADDL,
    I_VPMAX, I_VPMIN, I_VPOP, I_VPUSH, I_VQABS, I_VQADD, I_VQDMLAL, I_VQDMLSL,
    I_VQDMULH, I_VQDMULL, I_VQMOVN, I_VQMOVUN, I_VQNEG, I_VQRDMULH, I_VQRSHL,
    I_VQRSHRN, I_VQRSHRUN, I_VQSHL, I_VQSHLU, I_VQSHRN, I_VQSHRUN, I_VQSUB,
    I_VRADDHN, I_VRECPE, I_VRECPS, I_VREV16, I_VREV32, I_VREV64, I_VRHADD,
    I_VRSHL, I_VRSHR, I_VRSHRN, I_VRSQRTE, I_VRSQRTS, I_VRSRA, I_VRSUBHN,
    I_VSHL, I_VSHLL, I_VSHR, I_VSHRN, I_VSLI, I_VSQRT, I_VSRA, I_VSRI, I_VST1,
    I_VST2, I_VST3, I_VST4, I_VSTM, I_VSTR, I_VSUB, I_VSUBHN, I_VSUBL,
    I_VSUBW, I_VSWP, I_VTBL, I_VTBX, I_VTRN, I_VTST, I_VUZP, I_VZIP, I_WFE,
    I_WFI, I_YIELD, I_INSTRCNT
} darm_instr_t;

extern const char *darm_mnemonics[354];
extern const char *darm_enctypes[83];
extern const char *darm_registers[16];
#endif
