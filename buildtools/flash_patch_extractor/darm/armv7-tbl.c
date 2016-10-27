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
#include "armv7-tbl.h"
darm_enctype_t armv7_instr_types[] = {
    T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT,
    T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT,
    T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT,
    T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT,
    T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT, T_ARM_ARITH_SHIFT,
    T_ARM_ARITH_SHIFT, T_ARM_SM, T_ARM_CMP_OP, T_ARM_BRNCHMISC, T_ARM_CMP_OP,
    T_ARM_SM, T_ARM_CMP_OP, T_ARM_MISC, T_ARM_CMP_OP, T_ARM_ARITH_SHIFT,
    T_ARM_ARITH_SHIFT, T_ARM_DST_SRC, T_ARM_DST_SRC, T_ARM_ARITH_SHIFT,
    T_ARM_ARITH_SHIFT, T_ARM_MISC, T_ARM_MISC, T_ARM_ARITH_IMM,
    T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM,
    T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM,
    T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM,
    T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_MOV_IMM,
    T_ARM_CMP_IMM, T_ARM_OPLESS, T_ARM_CMP_IMM, T_ARM_MOV_IMM, T_ARM_CMP_IMM,
    T_INVLD, T_ARM_CMP_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_MOV_IMM,
    T_ARM_MOV_IMM, T_ARM_ARITH_IMM, T_ARM_ARITH_IMM, T_ARM_MOV_IMM,
    T_ARM_MOV_IMM, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_INVLD, T_INVLD, T_INVLD, T_ARM_PAS, T_ARM_PAS, T_ARM_PAS, T_INVLD,
    T_ARM_PAS, T_ARM_PAS, T_ARM_PAS, T_ARM_MISC, T_INVLD, T_INVLD,
    T_ARM_BITREV, T_INVLD, T_INVLD, T_INVLD, T_ARM_BITREV, T_ARM_SM, T_INVLD,
    T_INVLD, T_INVLD, T_ARM_SM, T_ARM_SM, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_ARM_BITS, T_ARM_BITS, T_ARM_BITS, T_ARM_BITS, T_ARM_BITS, T_ARM_UDF,
    T_ARM_LDSTREGS, T_ARM_LDSTREGS, T_ARM_LDSTREGS, T_ARM_LDSTREGS, T_INVLD,
    T_INVLD, T_INVLD, T_INVLD, T_ARM_LDSTREGS, T_ARM_LDSTREGS, T_ARM_LDSTREGS,
    T_ARM_LDSTREGS, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_ARM_LDSTREGS,
    T_ARM_LDSTREGS, T_ARM_LDSTREGS, T_ARM_LDSTREGS, T_INVLD, T_INVLD, T_INVLD,
    T_INVLD, T_ARM_LDSTREGS, T_ARM_LDSTREGS, T_ARM_LDSTREGS, T_ARM_LDSTREGS,
    T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD, T_INVLD,
    T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR,
    T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR,
    T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR, T_ARM_MVCR, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC,
    T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC, T_ARM_BRNCHSC
};

darm_instr_t armv7_instr_labels[] = {
    I_AND, I_AND, I_EOR, I_EOR, I_SUB, I_SUB, I_RSB, I_RSB, I_ADD, I_ADD,
    I_ADC, I_ADC, I_SBC, I_SBC, I_RSC, I_RSC, I_SMLA, I_TST, I_SMULW, I_TEQ,
    I_SMLAL, I_CMP, I_SMC, I_CMN, I_ORR, I_ORR, I_STREXD, I_RRX, I_BIC, I_BIC,
    I_MVN, I_MVN, I_AND, I_AND, I_EOR, I_EOR, I_SUB, I_SUB, I_RSB, I_RSB,
    I_ADD, I_ADD, I_ADC, I_ADC, I_SBC, I_SBC, I_RSC, I_RSC, I_MOVW, I_TST,
    I_YIELD, I_TEQ, I_MOVT, I_CMP, I_INVLD, I_CMN, I_ORR, I_ORR, I_MOV, I_MOV,
    I_BIC, I_BIC, I_MVN, I_MVN, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_SSUB8, I_QSUB8, I_SHSUB8, I_INVLD,
    I_USUB8, I_UQSUB8, I_UHSUB8, I_SEL, I_INVLD, I_INVLD, I_REV16, I_INVLD,
    I_INVLD, I_INVLD, I_REVSH, I_SMUSD, I_INVLD, I_INVLD, I_INVLD, I_SMLSLD,
    I_SMMUL, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_SBFX, I_SBFX, I_BFI, I_BFI,
    I_UBFX, I_UDF, I_STMDA, I_LDMDA, I_STMDA, I_LDMDA, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_STM, I_LDM, I_STM, I_LDM, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_STMDB, I_LDMDB, I_STMDB, I_LDMDB, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_STMIB, I_LDMIB, I_STMIB, I_LDMIB, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_B, I_B, I_B, I_B, I_B, I_B, I_B, I_B, I_B, I_B, I_B, I_B, I_B,
    I_B, I_B, I_B, I_BL, I_BL, I_BL, I_BL, I_BL, I_BL, I_BL, I_BL, I_BL, I_BL,
    I_BL, I_BL, I_BL, I_BL, I_BL, I_BL, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_MCR, I_MRC, I_MCR, I_MRC, I_MCR,
    I_MRC, I_MCR, I_MRC, I_MCR, I_MRC, I_MCR, I_MRC, I_MCR, I_MRC, I_MCR,
    I_MRC, I_SVC, I_SVC, I_SVC, I_SVC, I_SVC, I_SVC, I_SVC, I_SVC, I_SVC,
    I_SVC, I_SVC, I_SVC, I_SVC, I_SVC, I_SVC, I_SVC
};

darm_instr_t type_shift_instr_lookup[] = {
    I_LSL, I_LSL, I_LSR, I_LSR, I_ASR, I_ASR, I_ROR, I_ROR, I_LSL, I_INVLD,
    I_LSR, I_INVLD, I_ASR, I_INVLD, I_ROR, I_INVLD
};

darm_instr_t type_brnchmisc_instr_lookup[] = {
    I_MSR, I_BX, I_BXJ, I_BLX, I_INVLD, I_QSUB, I_INVLD, I_BKPT, I_SMLAW,
    I_INVLD, I_SMULW, I_INVLD, I_SMLAW, I_INVLD, I_SMULW, I_INVLD
};

darm_instr_t type_opless_instr_lookup[] = {
    I_NOP, I_YIELD, I_WFE, I_WFI, I_SEV, I_INVLD, I_INVLD, I_INVLD
};

darm_instr_t type_uncond2_instr_lookup[] = {
    I_INVLD, I_CLREX, I_INVLD, I_INVLD, I_DSB, I_DMB, I_ISB, I_INVLD
};

darm_instr_t type_mul_instr_lookup[] = {
    I_MUL, I_MLA, I_UMAAL, I_MLS, I_UMULL, I_UMLAL, I_SMULL, I_SMLAL
};

darm_instr_t type_stack0_instr_lookup[] = {
    I_STR, I_LDR, I_STRT, I_LDRT, I_STRB, I_LDRB, I_STRBT, I_LDRBT, I_STR,
    I_LDR, I_STRT, I_LDRT, I_STRB, I_LDRB, I_STRBT, I_LDRBT, I_STR, I_LDR,
    I_STR, I_LDR, I_STRB, I_LDRB, I_STRB, I_LDRB, I_STR, I_LDR, I_STR, I_LDR,
    I_STRB, I_LDRB, I_STRB, I_LDRB
};

darm_instr_t type_stack1_instr_lookup[] = {
    I_INVLD, I_INVLD, I_STRHT, I_LDRHT, I_INVLD, I_LDRSBT, I_INVLD, I_LDRSHT
};

darm_instr_t type_stack2_instr_lookup[] = {
    I_INVLD, I_INVLD, I_STRH, I_LDRH, I_LDRD, I_LDRSB, I_STRD, I_LDRSH
};

darm_instr_t type_bits_instr_lookup[] = {
    I_INVLD, I_SBFX, I_BFI, I_UBFX
};

darm_instr_t type_pas_instr_lookup[] = {
    I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_SADD16, I_SASX, I_SSAX, I_SSUB16, I_SADD8, I_INVLD, I_INVLD, I_SSUB8,
    I_QADD16, I_QASX, I_QSAX, I_QSUB16, I_QADD8, I_INVLD, I_INVLD, I_QSUB8,
    I_SHADD16, I_SHASX, I_SHSAX, I_SHSUB16, I_SHADD8, I_INVLD, I_INVLD,
    I_SHSUB8, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD, I_INVLD,
    I_INVLD, I_UADD16, I_UASX, I_USAX, I_USUB16, I_UADD8, I_INVLD, I_INVLD,
    I_USUB8, I_UQADD16, I_UQASX, I_UQSAX, I_UQSUB16, I_UQADD8, I_INVLD,
    I_INVLD, I_UQSUB8, I_UHADD16, I_UHASX, I_UHSAX, I_UHSUB16, I_UHADD8,
    I_INVLD, I_INVLD, I_UHSUB8
};

darm_instr_t type_sat_instr_lookup[] = {
    I_QADD, I_QSUB, I_QDADD, I_QDSUB
};

darm_instr_t type_sync_instr_lookup[] = {
    I_SWP, I_INVLD, I_INVLD, I_INVLD, I_SWPB, I_INVLD, I_INVLD, I_INVLD,
    I_STREX, I_LDREX, I_STREXD, I_LDREXD, I_STREXB, I_LDREXB, I_STREXH,
    I_LDREXH
};

darm_instr_t type_pusr_instr_lookup[] = {
    I_SXTAB16, I_SXTB16, I_INVLD, I_INVLD, I_SXTAB, I_SXTB, I_SXTAH, I_SXTH,
    I_UXTAB16, I_UXTB16, I_INVLD, I_INVLD, I_UXTAB, I_UXTB, I_UXTAH, I_UXTH
};

const char *armv7_format_strings[479][3] = {
    [I_ADC] = {"scdnmS", "scdni"},
    [I_ADD] = {"scdnmS", "scdni"},
    [I_ADR] = {"cdb"},
    [I_AND] = {"scdnmS", "scdni"},
    [I_ASR] = {"scdnm", "scdmS"},
    [I_BFC] = {"cdLw"},
    [I_BFI] = {"cdnLw"},
    [I_BIC] = {"scdnmS", "scdni"},
    [I_BKPT] = {"i"},
    [I_BLX] = {"b", "cm"},
    [I_BL] = {"cb"},
    [I_BXJ] = {"cm"},
    [I_BX] = {"cm"},
    [I_B] = {"cb"},
    [I_CDP2] = {"cCpINJ<opc2>"},
    [I_CDP] = {"cCpINJ<opc2>"},
    [I_CLREX] = {""},
    [I_CLZ] = {"cdm"},
    [I_CMN] = {"cnmS", "cni"},
    [I_CMP] = {"cnmS", "cni"},
    [I_DBG] = {"co"},
    [I_DMB] = {"o"},
    [I_DSB] = {"o"},
    [I_EOR] = {"scdnmS", "scdni"},
    [I_ISB] = {"o"},
    [I_LDC2] = {"{L}cCIB#+/-<imm>"},
    [I_LDC] = {"{L}cCIB#+/-<imm>"},
    [I_LDMDA] = {"cn!r"},
    [I_LDMDB] = {"cn!r"},
    [I_LDMIB] = {"cn!r"},
    [I_LDM] = {"cn!r"},
    [I_LDRBT] = {"ctBOS"},
    [I_LDRB] = {"ctBOS"},
    [I_LDRD] = {"ct2BO"},
    [I_LDREXB] = {"ctB"},
    [I_LDREXD] = {"ct2B"},
    [I_LDREXH] = {"ctB"},
    [I_LDREX] = {"ctB"},
    [I_LDRHT] = {"ctBO"},
    [I_LDRH] = {"ctBO"},
    [I_LDRSBT] = {"ctBO"},
    [I_LDRSB] = {"ctBO"},
    [I_LDRSHT] = {"ctBO"},
    [I_LDRSH] = {"ctBO"},
    [I_LDRT] = {"ctBOS"},
    [I_LDR] = {"ctBOS"},
    [I_LSL] = {"scdnm", "scdmS"},
    [I_LSR] = {"scdnm", "scdmS"},
    [I_MCR2] = {"cCptNJP"},
    [I_MCRR2] = {"cCpt2J"},
    [I_MCRR] = {"cCpt2J"},
    [I_MCR] = {"cCptNJP"},
    [I_MLA] = {"scdnma"},
    [I_MLS] = {"cdnma"},
    [I_MOVT] = {"cdi"},
    [I_MOVW] = {"cdi"},
    [I_MOV] = {"scdi", "scdm"},
    [I_MRC2] = {"cCptNJP"},
    [I_MRC] = {"cCptNJP"},
    [I_MRRC2] = {"cC<opc>t2J"},
    [I_MRRC] = {"cC<opc>t2J"},
    [I_MRS] = {"cd<spec_reg>"},
    [I_MSR] = {"c<spec_reg>n", "c<spec_reg>i"},
    [I_MUL] = {"scdnm"},
    [I_MVN] = {"scdi", "scdmS"},
    [I_NOP] = {"c"},
    [I_ORR] = {"scdnmS", "scdni"},
    [I_PKH] = {"TcdnmS"},
    [I_PLD] = {"cM"},
    [I_PLI] = {"M"},
    [I_POP] = {"cr"},
    [I_PUSH] = {"cr"},
    [I_QADD16] = {"cdnm"},
    [I_QADD8] = {"cdnm"},
    [I_QADD] = {"cdmn"},
    [I_QASX] = {"cdnm"},
    [I_QDADD] = {"cdmn"},
    [I_QDSUB] = {"cdmn"},
    [I_QSAX] = {"cdnm"},
    [I_QSUB16] = {"cdnm"},
    [I_QSUB8] = {"cdnm"},
    [I_QSUB] = {"cdmn"},
    [I_RBIT] = {"cdm"},
    [I_REV16] = {"cdm"},
    [I_REVSH] = {"cdm"},
    [I_REV] = {"cdm"},
    [I_ROR] = {"scdnm", "scdmS"},
    [I_RRX] = {"scdm"},
    [I_RSB] = {"scdnmS", "scdni"},
    [I_RSC] = {"scdnmS", "scdni"},
    [I_SADD16] = {"cdnm"},
    [I_SADD8] = {"cdnm"},
    [I_SASX] = {"cdnm"},
    [I_SBC] = {"scdnmS", "scdni"},
    [I_SBFX] = {"cdnLw"},
    [I_SEL] = {"cdnm"},
    [I_SETEND] = {"e"},
    [I_SEV] = {"c"},
    [I_SHADD16] = {"cdnm"},
    [I_SHADD8] = {"cdnm"},
    [I_SHASX] = {"cdnm"},
    [I_SHSAX] = {"cdnm"},
    [I_SHSUB16] = {"cdnm"},
    [I_SHSUB8] = {"cdnm"},
    [I_SMC] = {"ci"},
    [I_SMLAD] = {"xcdnma"},
    [I_SMLALD] = {"xclhnm"},
    [I_SMLAL] = {"Xclhnm", "sclhnm"},
    [I_SMLAW] = {"<y>cdnma"},
    [I_SMLA] = {"Xcdnma"},
    [I_SMLSD] = {"xcdnma"},
    [I_SMLSLD] = {"xclhnm"},
    [I_SMMLA] = {"Rcdnma"},
    [I_SMMLS] = {"Rcdnma"},
    [I_SMMUL] = {"Rcdnm"},
    [I_SMUAD] = {"xcdnm"},
    [I_SMULL] = {"sclhnm"},
    [I_SMULW] = {"<y>cdnm"},
    [I_SMUL] = {"Xcdnm"},
    [I_SMUSD] = {"xcdnm"},
    [I_SSAT16] = {"cd#<imm>n"},
    [I_SSAT] = {"cd#<imm>nS"},
    [I_SSAX] = {"cdnm"},
    [I_SSUB16] = {"cdnm"},
    [I_SSUB8] = {"cdnm"},
    [I_STC2] = {"{L}cCIB#+/-<imm>"},
    [I_STC] = {"{L}cCIB#+/-<imm>"},
    [I_STMDA] = {"cn!r"},
    [I_STMDB] = {"cn!r"},
    [I_STMIB] = {"cn!r"},
    [I_STM] = {"cn!r"},
    [I_STRBT] = {"ctBOS"},
    [I_STRB] = {"ctBOS"},
    [I_STRD] = {"ct2BO"},
    [I_STREXB] = {"cdtB"},
    [I_STREXD] = {"cdt2B"},
    [I_STREXH] = {"cdtB"},
    [I_STREX] = {"cdtB"},
    [I_STRHT] = {"ctBO"},
    [I_STRH] = {"ctBO"},
    [I_STRT] = {"ctBOS"},
    [I_STR] = {"ctBOS"},
    [I_SUB] = {"scdnmS", "scdni"},
    [I_SVC] = {"ci"},
    [I_SWPB] = {"ct2B"},
    [I_SWP] = {"ct2B"},
    [I_SXTAB16] = {"cdnmA"},
    [I_SXTAB] = {"cdnmA"},
    [I_SXTAH] = {"cdnmA"},
    [I_SXTB16] = {"cdmA"},
    [I_SXTB] = {"cdmA"},
    [I_SXTH] = {"cdmA"},
    [I_TEQ] = {"cnmS", "cni"},
    [I_TST] = {"cnmS", "cni"},
    [I_UADD16] = {"cdnm"},
    [I_UADD8] = {"cdnm"},
    [I_UASX] = {"cdnm"},
    [I_UBFX] = {"cdnLw"},
    [I_UDF] = {"ci"},
    [I_UHADD16] = {"cdnm"},
    [I_UHADD8] = {"cdnm"},
    [I_UHASX] = {"cdnm"},
    [I_UHSAX] = {"cdnm"},
    [I_UHSUB16] = {"cdnm"},
    [I_UHSUB8] = {"cdnm"},
    [I_UMAAL] = {"clhnm"},
    [I_UMLAL] = {"sclhnm"},
    [I_UMULL] = {"sclhnm"},
    [I_UQADD16] = {"cdnm"},
    [I_UQADD8] = {"cdnm"},
    [I_UQASX] = {"cdnm"},
    [I_UQSAX] = {"cdnm"},
    [I_UQSUB16] = {"cdnm"},
    [I_UQSUB8] = {"cdnm"},
    [I_USAD8] = {"cdnm"},
    [I_USADA8] = {"cdnma"},
    [I_USAT16] = {"cdin"},
    [I_USAT] = {"cdinS"},
    [I_USAX] = {"cdnm"},
    [I_USUB16] = {"cdnm"},
    [I_USUB8] = {"cdnm"},
    [I_UXTAB16] = {"cdnmA"},
    [I_UXTAB] = {"cdnmA"},
    [I_UXTAH] = {"cdnmA"},
    [I_UXTB16] = {"cdmA"},
    [I_UXTB] = {"cdmA"},
    [I_UXTH] = {"cdmA"},
    [I_WFE] = {"c"},
    [I_WFI] = {"c"},
    [I_YIELD] = {"c"},
};
