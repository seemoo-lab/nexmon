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
#include "darm-tbl.h"
const char *darm_mnemonics[] = {
    "INVLD", "ADC", "ADD", "ADDW", "ADR", "AND", "ASR", "B", "BFC", "BFI",
    "BIC", "BKPT", "BL", "BLX", "BX", "BXJ", "CBNZ", "CBZ", "CDP", "CDP2",
    "CHKA", "CLREX", "CLZ", "CMN", "CMP", "CPS", "CPY", "DBG", "DMB", "DSB",
    "ENTERX", "EOR", "HB", "HBL", "HBLP", "HBP", "ISB", "IT", "LDC", "LDC2",
    "LDM", "LDMDA", "LDMDB", "LDMIB", "LDR", "LDRB", "LDRBT", "LDRD", "LDREX",
    "LDREXB", "LDREXD", "LDREXH", "LDRH", "LDRHT", "LDRSB", "LDRSBT", "LDRSH",
    "LDRSHT", "LDRT", "LEAVEX", "LSL", "LSR", "MCR", "MCR2", "MCRR", "MCRR2",
    "MLA", "MLS", "MOV", "MOVT", "MOVW", "MRC", "MRC2", "MRRC", "MRRC2",
    "MRS", "MSR", "MUL", "MVN", "NEG", "NOP", "ORN", "ORR", "PKH", "PLD",
    "PLDW", "PLI", "POP", "PUSH", "QADD", "QADD16", "QADD8", "QASX", "QDADD",
    "QDSUB", "QSAX", "QSUB", "QSUB16", "QSUB8", "RBIT", "REV", "REV16",
    "REVSH", "RFE", "ROR", "RRX", "RSB", "RSC", "SADD16", "SADD8", "SASX",
    "SBC", "SBFX", "SDIV", "SEL", "SETEND", "SEV", "SHADD16", "SHADD8",
    "SHASX", "SHSAX", "SHSUB16", "SHSUB8", "SMC", "SMLA", "SMLABB", "SMLABT",
    "SMLAD", "SMLAL", "SMLALBB", "SMLALBT", "SMLALD", "SMLALTB", "SMLALTT",
    "SMLATB", "SMLATT", "SMLAW", "SMLSD", "SMLSLD", "SMMLA", "SMMLS", "SMMUL",
    "SMUAD", "SMUL", "SMULBB", "SMULBT", "SMULL", "SMULTB", "SMULTT", "SMULW",
    "SMUSD", "SRS", "SSAT", "SSAT16", "SSAX", "SSUB16", "SSUB8", "STC",
    "STC2", "STM", "STMDA", "STMDB", "STMIB", "STR", "STRB", "STRBT", "STRD",
    "STREX", "STREXB", "STREXD", "STREXH", "STRH", "STRHT", "STRT", "SUB",
    "SUBW", "SVC", "SWP", "SWPB", "SXTAB", "SXTAB16", "SXTAH", "SXTB",
    "SXTB16", "SXTH", "TBB", "TBH", "TEQ", "TST", "UADD16", "UADD8", "UASX",
    "UBFX", "UDF", "UDIV", "UHADD16", "UHADD8", "UHASX", "UHSAX", "UHSUB16",
    "UHSUB8", "UMAAL", "UMLAL", "UMULL", "UQADD16", "UQADD8", "UQASX",
    "UQSAX", "UQSUB16", "UQSUB8", "USAD8", "USADA8", "USAT", "USAT16", "USAX",
    "USUB16", "USUB8", "UXTAB", "UXTAB16", "UXTAH", "UXTB", "UXTB16", "UXTH",
    "VABA", "VABAL", "VABD", "VABDL", "VABS", "VACGE", "VACGT", "VACLE",
    "VACLT", "VADD", "VADDHN", "VADDL", "VADDW", "VAND", "VBIC", "VBIF",
    "VBIT", "VBSL", "VCEQ", "VCGE", "VCGT", "VCLE", "VCLS", "VCLT", "VCLZ",
    "VCMP", "VCMPE", "VCNT", "VCVT", "VCVTB", "VCVTR", "VCVTT", "VDIV",
    "VDUP", "VEOR", "VEXT", "VHADD", "VHSUB", "VLD1", "VLD2", "VLD3", "VLD4",
    "VLDM", "VLDR", "VMAX", "VMIN", "VMLA", "VMLAL", "VMLS", "VMLSL", "VMOV",
    "VMOVL", "VMOVN", "VMRS", "VMSR", "VMUL", "VMULL", "VMVN", "VNEG",
    "VNMLA", "VNMLS", "VNMUL", "VORN", "VORR", "VPADAL", "VPADD", "VPADDL",
    "VPMAX", "VPMIN", "VPOP", "VPUSH", "VQABS", "VQADD", "VQDMLAL", "VQDMLSL",
    "VQDMULH", "VQDMULL", "VQMOVN", "VQMOVUN", "VQNEG", "VQRDMULH", "VQRSHL",
    "VQRSHRN", "VQRSHRUN", "VQSHL", "VQSHLU", "VQSHRN", "VQSHRUN", "VQSUB",
    "VRADDHN", "VRECPE", "VRECPS", "VREV16", "VREV32", "VREV64", "VRHADD",
    "VRSHL", "VRSHR", "VRSHRN", "VRSQRTE", "VRSQRTS", "VRSRA", "VRSUBHN",
    "VSHL", "VSHLL", "VSHR", "VSHRN", "VSLI", "VSQRT", "VSRA", "VSRI", "VST1",
    "VST2", "VST3", "VST4", "VSTM", "VSTR", "VSUB", "VSUBHN", "VSUBL",
    "VSUBW", "VSWP", "VTBL", "VTBX", "VTRN", "VTST", "VUZP", "VZIP", "WFE",
    "WFI", "YIELD"
};

const char *darm_enctypes[] = {
    "INVLD", "ARM_ADR", "ARM_UNCOND", "ARM_MUL", "ARM_STACK0", "ARM_STACK1",
    "ARM_STACK2", "ARM_ARITH_SHIFT", "ARM_ARITH_IMM", "ARM_BITS",
    "ARM_BRNCHSC", "ARM_BRNCHMISC", "ARM_MOV_IMM", "ARM_CMP_OP",
    "ARM_CMP_IMM", "ARM_OPLESS", "ARM_DST_SRC", "ARM_LDSTREGS", "ARM_BITREV",
    "ARM_MISC", "ARM_SM", "ARM_PAS", "ARM_SAT", "ARM_SYNC", "ARM_PUSR",
    "ARM_MVCR", "ARM_UDF", "THUMB_ONLY_IMM8", "THUMB_COND_BRANCH",
    "THUMB_UNCOND_BRANCH", "THUMB_SHIFT_IMM", "THUMB_STACK", "THUMB_LDR_PC",
    "THUMB_GPI", "THUMB_BRANCH_REG", "THUMB_IT_HINTS", "THUMB_HAS_IMM8",
    "THUMB_EXTEND", "THUMB_MOD_SP_IMM", "THUMB_3REG", "THUMB_2REG_IMM",
    "THUMB_ADD_SP_IMM", "THUMB_MOV4", "THUMB_RW_MEMI", "THUMB_RW_MEMO",
    "THUMB_RW_REG", "THUMB_REV", "THUMB_SETEND", "THUMB_PUSHPOP", "THUMB_CMP",
    "THUMB_MOD_SP_REG", "THUMB_CBZ", "THUMB2_NO_REG", "THUMB2_RT_REG",
    "THUMB2_RT_RT2_REG", "THUMB2_RM_REG", "THUMB2_RD_REG", "THUMB2_RD_RM_REG",
    "THUMB2_RN_REG", "THUMB2_RN_RT_REG", "THUMB2_RN_RT_RT2_REG",
    "THUMB2_RN_RM_REG", "THUMB2_RN_RM_RT_REG", "THUMB2_RN_RD_REG",
    "THUMB2_RN_RD_RT_REG", "THUMB2_RN_RD_RT_RT2_REG", "THUMB2_RN_RD_RM_REG",
    "THUMB2_RN_RD_RM_RA_REG", "THUMB2_NO_IMM", "THUMB2_IMM12", "THUMB2_IMM8",
    "THUMB2_IMM2", "THUMB2_IMM2_IMM3", "THUMB2_IMM1_IMM3_IMM8",
    "THUMB2_NO_FLAG", "THUMB2_ROTATE_FLAG", "THUMB2_U_FLAG",
    "THUMB2_WUP_FLAG", "THUMB2_TYPE_FLAG", "THUMB2_REGLIST_FLAG",
    "THUMB2_WP_REGLIST_FLAG", "THUMB2_S_FLAG", "THUMB2_S_TYPE_FLAG"
};

const char *darm_registers[] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11",
    "r12", "SP", "LR", "PC"
};

