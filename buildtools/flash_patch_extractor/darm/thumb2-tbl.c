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
#include "thumb2-tbl.h"
darm_instr_t thumb2_instr_labels[] = {
    I_ADC, I_ADC, I_WFE, I_WFI, I_SEV, I_NOP, I_SUB, I_LDR, I_ROR, I_LSR,
    I_STREX, I_PUSH, I_LDRD, I_STRT, I_MOVT, I_ADD, I_BIC, I_ASR, I_REVSH,
    I_SHADD8, I_YIELD, I_USUB16, I_USAD8, I_UHASX, I_UXTB, I_ADDW, I_RRX,
    I_SUB, I_SUBW, I_MVN, I_LDREXH, I_LDR, I_LDMDB, I_B, I_UADD16, I_UHADD8,
    I_UDF, I_LDRSB, I_PLI, I_SADD8, I_BLX, I_UQADD16, I_LDRSBT, I_UHSUB8,
    I_POP, I_BFC, I_UMLAL, I_UHADD16, I_MLS, I_SHASX, I_SUB, I_PLD, I_REV16,
    I_UMAAL, I_STREXD, I_UHSUB16, I_LDRT, I_SUBW, I_SMLAD, I_LDREX, I_USUB8,
    I_CMP, I_LDREXD, I_CLZ, I_SMLSLD, I_CMN, I_B, I_SASX, I_REV, I_STRB,
    I_QDADD, I_ORR, I_SMULW, I_UHSAX, I_STM, I_TEQ, I_BXJ, I_QSUB16, I_AND,
    I_UDIV, I_PLI, I_QSUB, I_PUSH, I_SHSUB8, I_UXTB16, I_SXTAH, I_SXTB,
    I_QDSUB, I_MOVW, I_LDRD, I_EOR, I_QASX, I_ADR, I_USAX, I_LDRB, I_MLA,
    I_SMUL, I_SMLALD, I_SBFX, I_SHSUB16, I_UQASX, I_LDRH, I_SSUB8, I_STRHT,
    I_UXTAH, I_BL, I_BIC, I_SHSAX, I_RSB, I_UXTAB16, I_STRB, I_UQADD8,
    I_SSUB16, I_SXTAB16, I_SEL, I_DBG, I_SUB, I_ADD, I_STR, I_LDRH, I_LDRSH,
    I_CMN, I_QADD, I_POP, I_LDRB, I_UMULL, I_UADD8, I_SSAX, I_SMLAL, I_STR,
    I_SMMLS, I_STR, I_TBB, I_ORR, I_STRH, I_SXTAB, I_CLREX, I_TST, I_LDM,
    I_QADD8, I_LDR, I_LDRSB, I_DSB, I_SMLSD, I_SMMUL, I_LDRH, I_SHADD16,
    I_LSL, I_LDRSB, I_UXTAB, I_LDREXB, I_DMB, I_UQSUB8, I_BFI, I_SMLA, I_ADDW,
    I_ISB, I_LDR, I_MVN, I_UASX, I_QADD16, I_UBFX, I_PLD, I_LDRHT, I_ORN,
    I_LDRH, I_ADR, I_UQSUB16, I_USADA8, I_ADD, I_STREXB, I_SBC, I_LSR,
    I_STRBT, I_SMUAD, I_SXTH, I_TST, I_TEQ, I_LDRSB, I_STRB, I_LDRB, I_PLD,
    I_EOR, I_STMDB, I_SMLAW, I_SDIV, I_PLI, I_SMUSD, I_QSUB8, I_UXTH, I_AND,
    I_UQSAX, I_SADD16, I_LDRBT, I_LDRB, I_ADD, I_PLI, I_RBIT, I_CMP, I_SMULL,
    I_ASR, I_SXTB16, I_STRH, I_ORN, I_LDRSH, I_MUL, I_STRH, I_MOV, I_STREXH,
    I_RSB, I_SBC, I_PLD, I_SMMLA, I_QSAX, I_SMLAL, I_LDRSH, I_MOV, I_LDRSHT,
    I_LDRSH, I_MRS
};

const char *thumb2_instruction_strings[] = {
    "ADC", "ADC", "WFE", "WFI", "SEV", "NOP", "SUB", "LDR", "ROR", "LSR",
    "STREX", "PUSH", "LDRD", "STRT", "MOVT", "ADD", "BIC", "ASR", "REVSH",
    "SHADD8", "YIELD", "USUB16", "USAD8", "UHASX", "UXTB", "ADDW", "RRX",
    "SUB", "SUBW", "MVN", "LDREXH", "LDR", "LDMDB", "B", "UADD16", "UHADD8",
    "UDF", "LDRSB", "PLI", "SADD8", "BLX", "UQADD16", "LDRSBT", "UHSUB8",
    "POP", "BFC", "UMLAL", "UHADD16", "MLS", "SHASX", "SUB", "PLD", "REV16",
    "UMAAL", "STREXD", "UHSUB16", "LDRT", "SUBW", "SMLAD", "LDREX", "USUB8",
    "CMP", "LDREXD", "CLZ", "SMLSLD", "CMN", "B", "SASX", "REV", "STRB",
    "QDADD", "ORR", "SMULW", "UHSAX", "STM", "TEQ", "BXJ", "QSUB16", "AND",
    "UDIV", "PLI", "QSUB", "PUSH", "SHSUB8", "UXTB16", "SXTAH", "SXTB",
    "QDSUB", "MOVW", "LDRD", "EOR", "QASX", "ADR", "USAX", "LDRB", "MLA",
    "SMUL", "SMLALD", "SBFX", "SHSUB16", "UQASX", "LDRH", "SSUB8", "STRHT",
    "UXTAH", "BL", "BIC", "SHSAX", "RSB", "UXTAB16", "STRB", "UQADD8",
    "SSUB16", "SXTAB16", "SEL", "DBG", "SUB", "ADD", "STR", "LDRH", "LDRSH",
    "CMN", "QADD", "POP", "LDRB", "UMULL", "UADD8", "SSAX", "SMLAL", "STR",
    "SMMLS", "STR", "TBB", "ORR", "STRH", "SXTAB", "CLREX", "TST", "LDM",
    "QADD8", "LDR", "LDRSB", "DSB", "SMLSD", "SMMUL", "LDRH", "SHADD16",
    "LSL", "LDRSB", "UXTAB", "LDREXB", "DMB", "UQSUB8", "BFI", "SMLA", "ADDW",
    "ISB", "LDR", "MVN", "UASX", "QADD16", "UBFX", "PLD", "LDRHT", "ORN",
    "LDRH", "ADR", "UQSUB16", "USADA8", "ADD", "STREXB", "SBC", "LSR",
    "STRBT", "SMUAD", "SXTH", "TST", "TEQ", "LDRSB", "STRB", "LDRB", "PLD",
    "EOR", "STMDB", "SMLAW", "SDIV", "PLI", "SMUSD", "QSUB8", "UXTH", "AND",
    "UQSAX", "SADD16", "LDRBT", "LDRB", "ADD", "PLI", "RBIT", "CMP", "SMULL",
    "ASR", "SXTB16", "STRH", "ORN", "LDRSH", "MUL", "STRH", "MOV", "STREXH",
    "RSB", "SBC", "PLD", "SMMLA", "QSAX", "SMLAL", "LDRSH", "MOV", "LDRSHT",
    "LDRSH", "MRS"
};

