/*
 *   Copyright (C) 2006-2010  Michael Buesch <m@bues.ch>
 *   Copyright (C) 2024 Francesco Gringoli <francesco.gringoli@unibs.it>
 *   Copyright (C) 2024 Jakob Link <jlink@seemoo.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2
 *   as published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include "main.h"
#include "list.h"
#include "util.h"
#include "args.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define noFULLDEBUG

struct bin_instruction {
	unsigned int weird_bit;
	unsigned int opcode;
	unsigned int operands[3];
};

struct statement {
	enum {
		STMT_INSN,
		STMT_LABEL,
	} type;

	union {
		struct {
			struct bin_instruction *bin;
			const char *name;
			const char *operands[5];

			int labelref_operand;
			unsigned int labeladdr;
			struct statement *labelref;

			unsigned int weird_bit;
		} insn;
		struct {
			char *name;
		} label;
	} u;

	struct list_head list;
};

struct disassembler_context {
	/* The architecture of the input file. */
	unsigned int arch;
	unsigned int subarch;

	struct bin_instruction *code;
	size_t nr_insns;

	struct list_head stmt_list;
};


FILE *infile;
FILE *outfile;
const char *infile_name;
const char *outfile_name;


#define _msg_helper(type, msg, x...)	do {		\
	fprintf(stderr, "Disassembler " type		\
		":\n  " msg "\n" ,##x);			\
					} while (0)

#define dasm_error(msg, x...)	do {		\
	_msg_helper("ERROR", msg ,##x);		\
	exit(1);				\
				} while (0)

#define dasm_int_error(msg, x...) \
	dasm_error("Internal error (bug): " msg ,##x)

#define dasm_warn(msg, x...)				\
	if (cmdargs.suppress_warnings == 0) {		\
		_msg_helper("warning", msg ,##x);	\
	}

#define asm_info(msg, x...)	\
	_msg_helper("info", msg ,##x)

#define WARN_IF_LT(ARCH, MSG)				\
	if (cmdargs.arch < ARCH) {			\
		dasm_warn(MSG);				\
	}

#define WARN_IF_GE(ARCH, MSG)				\
	if (cmdargs.arch >= ARCH) {			\
		dasm_warn(MSG);				\
	}

#define ERROR_IF_LT(ARCH, MSG)				\
	if (cmdargs.arch < ARCH) {			\
		dasm_error(MSG);			\
	}

static const char * gen_raw_code(unsigned int operand)
{
	char *ret;

	ret = xmalloc(6);
	snprintf(ret, 6, "@%X", operand);

	return ret;
}

static const char * disasm_mem_operand(unsigned int operand)
{
	char *ret;

	if (operand > 0x7fff) {
		dasm_int_error ("memory operand with address exceeding 64KB limit");
	}

	ret = xmalloc(9);
	snprintf(ret, 9, "[0x%hX]", (unsigned short) operand);

	return ret;
}

static const char * disasm_indirect_mem_operand(unsigned int operand)
{
	char *ret;
	unsigned int offset, reg;

	if (cmdargs.arch < ARCH15) {
		offset = (operand & 0x3F);
		reg = ((operand >> 6) & 0x7);
	} else if (cmdargs.arch < ARCH65) {
		offset = (operand & 0x7F);
		reg = ((operand >> 7) & 0x7);
	} else if (cmdargs.arch == ARCH65) {
		offset = (operand & 0x3F);
		reg = ((operand >> 6) & 0xf);
	} else if (cmdargs.arch >= ARCH129 && cmdargs.arch < ARCH134) {
		offset = (operand & 0x3F);
		reg = ((operand >> 6) & 0x1f);
	} else if (cmdargs.arch == ARCH134 && cmdargs.subarch == SUBARCH1) {
		offset = (operand & 0x7F);
		reg = ((operand >> 7) & 0x1f);
	} else if (cmdargs.arch == ARCH134 && cmdargs.subarch == NOSUBARCH) {
		offset = (operand & 0x3F);
		reg = ((operand >> 6) & 0x3f);
	} else {
		dasm_int_error("disasm_indirect_mem_operand invalid arch");
	}

	ret = xmalloc(15);
	snprintf(ret, 13, "[0x%02X,off%u]", offset, reg);

	return ret;
}

static const char * disasm_imm_operand(unsigned int operand)
{
	char *ret;
	unsigned int signmask;
	unsigned int mask;

	if (cmdargs.arch < ARCH15) {
		signmask = (1 << 9);
		mask = 0x3FF;
	} else if (cmdargs.arch < ARCH129) {
		signmask = (1 << 10);
		mask = 0x7FF;
	} else if (cmdargs.arch < ARCH134) {
		signmask = (1 << 11);
		mask = 0xFFF;
	} else {
		signmask = (1 << 12);
		mask = 0x1FFF;
	}

	operand &= mask;

	ret = xmalloc(7);
	if (operand & signmask)
		operand = (operand | (~mask & 0xFFFF));
	snprintf(ret, 7, "0x%X", operand);

	return ret;
}

static const char * disasm_spr_operand(unsigned int operand)
{
	char *ret;
	unsigned int mask;

	if (cmdargs.arch < ARCH15) {
		mask = 0x1FF;
	} else if (cmdargs.arch < ARCH129) {
		mask = 0x7FF;
	} else {
		mask = 0xFFF;
	}

	ret = xmalloc(8);
	snprintf(ret, 8, "spr%03X", (operand & mask));

	return ret;
}

static const char * disasm_gpr_operand(unsigned int operand)
{
	char *ret;
	unsigned int mask;

	if (cmdargs.arch < ARCH15) {
		mask = 0x3F;
	} else if (cmdargs.arch < ARCH129) {
		mask = 0x7F;
	} else {
		mask = 0xFF;
	}

	ret = xmalloc(5);
	snprintf(ret, 5, "r%u", (operand & mask));

	return ret;
}

static void disasm_raw_operand(struct statement *stmt,
			       int oper_idx,
			       int out_idx)
{
	unsigned int operand = stmt->u.insn.bin->operands[oper_idx];

	stmt->u.insn.operands[out_idx] = gen_raw_code(operand);
}

static void disasm_std_operand(struct statement *stmt,
			       int oper_idx,
			       int out_idx)
{
	unsigned int operand = stmt->u.insn.bin->operands[oper_idx];

	if (cmdargs.arch < ARCH15) {
		if (!(operand & 0x800)) {
			stmt->u.insn.operands[out_idx] = disasm_mem_operand(operand);
			return;
		} else if ((operand & 0xC00) == 0xC00) { 
			stmt->u.insn.operands[out_idx] = disasm_imm_operand(operand);
			return;
		} else if ((operand & 0xFC0) == 0xBC0) {
			stmt->u.insn.operands[out_idx] = disasm_gpr_operand(operand);
			return;
		} else if ((operand & 0xE00) == 0x800) {
			stmt->u.insn.operands[out_idx] = disasm_spr_operand(operand);
			return;
		} else if ((operand & 0xE00) == 0xA00) {
			stmt->u.insn.operands[out_idx] = disasm_indirect_mem_operand(operand);
			return;
		}
	} else if (cmdargs.arch < ARCH129) {
		if (!(operand & 0x1000)) {
			stmt->u.insn.operands[out_idx] = disasm_mem_operand(operand);
			return;
		} else if ((operand & 0x1800) == 0x1800) { 
			stmt->u.insn.operands[out_idx] = disasm_imm_operand(operand);
			return;
		} else if ((operand & 0x1F80) == 0x1780) {
			stmt->u.insn.operands[out_idx] = disasm_gpr_operand(operand);
			return;
		} else if ((operand & 0x1C00) == 0x1000) {
			stmt->u.insn.operands[out_idx] = disasm_spr_operand(operand);
			return;
		} else if ((operand & 0x1C00) == 0x1400) {
			stmt->u.insn.operands[out_idx] = disasm_indirect_mem_operand(operand);
			return;
		}
	}
	else if (cmdargs.arch < ARCH134) {
		if (!(operand & 0x2000)) {
			stmt->u.insn.operands[out_idx] = disasm_mem_operand(operand);
			return;
		} else if ((operand & 0x3000) == 0x3000) { 
			stmt->u.insn.operands[out_idx] = disasm_imm_operand(operand);
			return;
		} else if ((operand & 0x3F00) == 0x2F00) {
			stmt->u.insn.operands[out_idx] = disasm_gpr_operand(operand);
			return;
		} else if ((operand & 0x3800) == 0x2000) {
			stmt->u.insn.operands[out_idx] = disasm_spr_operand(operand);
			return;
		} else if ((operand & 0x3800) == 0x2800) {
			stmt->u.insn.operands[out_idx] = disasm_indirect_mem_operand(operand);
			return;
		}
	} else {
		if (!(operand & 0x4000)) {
			stmt->u.insn.operands[out_idx] = disasm_mem_operand(operand);
			return;
		} else if ((operand & 0x6000) == 0x6000) { 
			stmt->u.insn.operands[out_idx] = disasm_imm_operand(operand);
			return;
		} else if ((operand & 0x5F00) == 0x5F00 && cmdargs.subarch == NOSUBARCH) {
			stmt->u.insn.operands[out_idx] = disasm_gpr_operand(operand);
			return;
		} else if ((operand & 0x5F00) == 0x5E00 && cmdargs.subarch == SUBARCH1) {
			stmt->u.insn.operands[out_idx] = disasm_gpr_operand(operand);
			return;
		} else if ((operand & 0x7000) == 0x4000) {
			stmt->u.insn.operands[out_idx] = disasm_spr_operand(operand);
			return;
		} else if ((operand & 0x7000) == 0x5000) {
			stmt->u.insn.operands[out_idx] = disasm_indirect_mem_operand(operand);
			return;
		}
	

	}
	/* No luck. Disassemble to raw operand. */
	disasm_raw_operand(stmt, oper_idx, out_idx);
}

static void disasm_opcode_raw(struct disassembler_context *ctx,
			      struct statement *stmt,
			      int raw_operands)
{
	stmt->u.insn.name = gen_raw_code(stmt->u.insn.bin->opcode);
	if (raw_operands) {
		disasm_raw_operand(stmt, 0, 0);
		disasm_raw_operand(stmt, 1, 1);
		disasm_raw_operand(stmt, 2, 2);
	} else {
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
	}
}

static void disasm_constant_opcodes(struct disassembler_context *ctx,
				    struct statement *stmt,
				    int instr_number)
{
	struct bin_instruction *bin = stmt->u.insn.bin;

	switch (bin->opcode) {
	case 0x101:
		stmt->u.insn.name = "mul";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;

	case 0x102:
		WARN_IF_LT(ARCH65, "math1 instruction may not work");
		stmt->u.insn.name = "math1";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x103:
		WARN_IF_LT(ARCH65, "math2 instruction may not work");
		stmt->u.insn.name = "math2";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x105:
		WARN_IF_LT(ARCH129, "math3 instruction may not work");
		stmt->u.insn.name = "math3";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x108:
		WARN_IF_LT(ARCH129, "math4 instruction may not work");
		stmt->u.insn.name = "math4";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x109:
		WARN_IF_LT(ARCH129, "math5 instruction may not work");
		stmt->u.insn.name = "math5";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;

	case 0x180:
		WARN_IF_LT(ARCH65, "xadd instruction may not work");
		stmt->u.insn.name = "xadd";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x182:
		WARN_IF_LT(ARCH65, "xadd. instruction may not work");
		stmt->u.insn.name = "xadd.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x181:
		WARN_IF_LT(ARCH65, "xaddc instruction may not work");
		stmt->u.insn.name = "xaddc";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x183:
		WARN_IF_LT(ARCH65, "xaddc. instruction may not work");
		stmt->u.insn.name = "xaddc.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x190:
		WARN_IF_LT(ARCH65, "xsub instruction may not work");
		stmt->u.insn.name = "xsub";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x192:
		WARN_IF_LT(ARCH65, "xsub. instruction may not work");
		stmt->u.insn.name = "xsub.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x191:
		WARN_IF_LT(ARCH65, "xsubc instruction may not work");
		stmt->u.insn.name = "xsubc";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x193:
		WARN_IF_LT(ARCH65, "xsubc. instruction may not work");
		stmt->u.insn.name = "xsubc.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;

	case 0x1C0:
		WARN_IF_GE(ARCH65, "add instruction may not work");
		stmt->u.insn.name = "add";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1C2:
		WARN_IF_GE(ARCH65, "add. instruction may not work");
		stmt->u.insn.name = "add.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1C1:
		WARN_IF_GE(ARCH65, "addc instruction may not work");
		stmt->u.insn.name = "addc";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1C3:
		WARN_IF_GE(ARCH65, "addc. instruction may not work");
		stmt->u.insn.name = "addc.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1D0:
		WARN_IF_GE(ARCH65, "sub instruction may not work");
		stmt->u.insn.name = "sub";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1D2:
		WARN_IF_GE(ARCH65, "sub. instruction may not work");
		stmt->u.insn.name = "sub.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1D1:
		WARN_IF_GE(ARCH65, "subc instruction may not work");
		stmt->u.insn.name = "subc";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1D3:
		WARN_IF_GE(ARCH65, "subc. instruction may not work");
		stmt->u.insn.name = "subc.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x130:
		stmt->u.insn.name = "sra";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x160:
		stmt->u.insn.name = "or";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x140:
		stmt->u.insn.name = "and";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x170:
		stmt->u.insn.name = "xor";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x120:
		stmt->u.insn.name = "sr";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x110:
		stmt->u.insn.name = "sl";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1A0:
		stmt->u.insn.name = "rl";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;

	case 0x1A1:
		WARN_IF_LT(ARCH129, "math6 instruction may not work");
		stmt->u.insn.name = "math6";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1A2:
		WARN_IF_LT(ARCH134, "mathC instruction may not work");
		stmt->u.insn.name = "mathC";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1A4:
		WARN_IF_LT(ARCH129, "math7 instruction may not work");
		stmt->u.insn.name = "math7";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1A5:
		WARN_IF_LT(ARCH134, "mathD instruction may not work");
		stmt->u.insn.name = "mathD";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1A6:
		WARN_IF_LT(ARCH129, "math8 instruction may not work");
		stmt->u.insn.name = "math8";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;

	case 0x1B0:
		stmt->u.insn.name = "rr";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;

	case 0x1B1:
		WARN_IF_LT(ARCH129, "math9 instruction may not work");
		stmt->u.insn.name = "math9";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1B2:
		WARN_IF_LT(ARCH129, "mathA instruction may not work");
		stmt->u.insn.name = "mathA";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1B4:
		WARN_IF_LT(ARCH129, "mathB instruction may not work");
		stmt->u.insn.name = "mathB";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;

	case 0x150:
		stmt->u.insn.name = "nand";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x040:
		stmt->u.insn.name = "jand";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x040 | 0x1):
		stmt->u.insn.name = "jnand";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x050:
		stmt->u.insn.name = "js";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x050 | 0x1):
		stmt->u.insn.name = "jns";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x060:
		WARN_IF_LT(ARCH134, "jboh4 instruction may not work");
		stmt->u.insn.name = "jboh4";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x060 | 0x1):
		WARN_IF_LT(ARCH134, "jnboh4 instruction may not work");
		stmt->u.insn.name = "jnboh4";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x070:
		WARN_IF_LT(ARCH15, "jboh instruction may not work");
		stmt->u.insn.name = "jboh";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x070 | 0x1):
		WARN_IF_LT(ARCH15, "jnboh instruction may not work");
		stmt->u.insn.name = "jnboh";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x082:
		WARN_IF_LT(ARCH65, "jboh2 instruction may not work");
		stmt->u.insn.name = "jboh2";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x082 | 0x1):
		WARN_IF_LT(ARCH65, "jnboh2 instruction may not work");
		stmt->u.insn.name = "jnboh2";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x086:
		WARN_IF_LT(ARCH129, "jboh3 instruction may not work");
		stmt->u.insn.name = "jboh3";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x086 | 0x1):
		WARN_IF_LT(ARCH129, "jnboh3 instruction may not work");
		stmt->u.insn.name = "jnboh3";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x090:
		WARN_IF_LT(ARCH65, "xje instruction may not work");
		stmt->u.insn.name = "xje";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x090 | 0x1):
		WARN_IF_LT(ARCH65, "xjne instruction may not work");
		stmt->u.insn.name = "xjne";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x092:
		WARN_IF_LT(ARCH65, "xjls instruction may not work");
		stmt->u.insn.name = "xjls";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x092 | 0x1):
		WARN_IF_LT(ARCH65, "xjges instruction may not work");
		stmt->u.insn.name = "xjges";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x094:
		WARN_IF_LT(ARCH65, "xjgs instruction may not work");
		stmt->u.insn.name = "xjgs";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x094 | 0x1):
		WARN_IF_LT(ARCH65, "xjles instruction may not work");
		stmt->u.insn.name = "xjles";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x096:
		WARN_IF_LT(ARCH65, "xjdn instruction may not work");
		stmt->u.insn.name = "xjdn";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x096 | 0x1):
		WARN_IF_LT(ARCH65, "xjdpz instruction may not work");
		stmt->u.insn.name = "xjdpz";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x098:
		WARN_IF_LT(ARCH65, "xjdp instruction may not work");
		stmt->u.insn.name = "xjdp";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x098 | 0x1):
		WARN_IF_LT(ARCH65, "xjdnz instruction may not work");
		stmt->u.insn.name = "xjdnz";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x09A:
		WARN_IF_LT(ARCH65, "xjl instruction may not work");
		stmt->u.insn.name = "xjl";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x09A | 0x1):
		WARN_IF_LT(ARCH65, "xjge instruction may not work");
		stmt->u.insn.name = "xjge";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x09C:
		WARN_IF_LT(ARCH65, "xjg instruction may not work");
		stmt->u.insn.name = "xjg";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x09C | 0x1):
		WARN_IF_LT(ARCH65, "xjle instruction may not work");
		stmt->u.insn.name = "xjle";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0B0:
		WARN_IF_LT(ARCH65, "jmah instruction may not work");
		stmt->u.insn.name = "jmah";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0B0 | 0x1):
		WARN_IF_LT(ARCH65, "jnmah instruction may not work");
		stmt->u.insn.name = "jnmah";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x0B2:
		WARN_IF_LT(ARCH132, "jmah5 instruction may not work");
		stmt->u.insn.name = "jmah5";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0B2 | 0x1):
		WARN_IF_LT(ARCH132, "jnmah5 instruction may not work");
		stmt->u.insn.name = "jnmah5";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x0B6:
		WARN_IF_LT(ARCH129, "jmah2 instruction may not work");
		stmt->u.insn.name = "jmah2";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0B6 | 0x1):
		WARN_IF_LT(ARCH129, "jnmah2 instruction may not work");
		stmt->u.insn.name = "jnmah2";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x0B8:
		WARN_IF_LT(ARCH132, "jmah6 instruction may not work");
		stmt->u.insn.name = "jmah6";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0B8 | 0x1):
		WARN_IF_LT(ARCH132, "jnmah6 instruction may not work");
		stmt->u.insn.name = "jnmah6";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x0BA:
		WARN_IF_LT(ARCH129, "jmah3 instruction may not work");
		stmt->u.insn.name = "jmah3";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0BA | 0x1):
		WARN_IF_LT(ARCH129, "jnmah3 instruction may not work");
		stmt->u.insn.name = "jnmah3";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x0BC:
		WARN_IF_LT(ARCH132, "jmah4 instruction may not work");
		stmt->u.insn.name = "jmah4";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0BC | 0x1):
		WARN_IF_LT(ARCH132, "jnmah4 instruction may not work");
		stmt->u.insn.name = "jnmah4";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;

	case 0x0D0:
		WARN_IF_GE(ARCH65, "je instruction may not work");
		stmt->u.insn.name = "je";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D0 | 0x1):
		WARN_IF_GE(ARCH65, "jne instruction may not work");
		stmt->u.insn.name = "jne";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D2:
		WARN_IF_GE(ARCH65, "jls instruction may not work");
		stmt->u.insn.name = "jls";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D2 | 0x1):
		WARN_IF_GE(ARCH65, "jges instruction may not work");
		stmt->u.insn.name = "jges";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D4:
		WARN_IF_GE(ARCH65, "jgs instruction may not work");
		stmt->u.insn.name = "jgs";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D4 | 0x1):
		WARN_IF_GE(ARCH65, "jles instruction may not work");
		stmt->u.insn.name = "jles";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D6:
		WARN_IF_GE(ARCH65, "jdn instruction may not work");
		stmt->u.insn.name = "jdn";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D6 | 0x1):
		WARN_IF_GE(ARCH65, "jdpz instruction may not work");
		stmt->u.insn.name = "jdpz";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D8:
		WARN_IF_GE(ARCH65, "jdp instruction may not work");
		stmt->u.insn.name = "jdp";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D8 | 0x1):
		WARN_IF_GE(ARCH65, "jdnz instruction may not work");
		stmt->u.insn.name = "jdnz";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0DA:
		WARN_IF_GE(ARCH65, "jl instruction may not work");
		stmt->u.insn.name = "jl";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0DA | 0x1):
		WARN_IF_GE(ARCH65, "jge instruction may not work");
		stmt->u.insn.name = "jge";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0DC:
		WARN_IF_GE(ARCH65, "jg instruction may not work");
		stmt->u.insn.name = "jg";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0DC | 0x1):
		WARN_IF_GE(ARCH65, "jle instruction may not work");
		stmt->u.insn.name = "jle";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x002: {
		char *str;
		if (cmdargs.arch == ARCH5) {
			stmt->u.insn.name = "call";
			stmt->u.insn.labelref_operand = 1;
			stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
			str = xmalloc(4);
			snprintf(str, 4, "lr%u", stmt->u.insn.bin->operands[0]);
			stmt->u.insn.operands[0] = str;
			break;
		} else if (cmdargs.arch < ARCH129) {
			unsigned int mask = 0x1780;
			stmt->u.insn.name = "nap2";
			if (stmt->u.insn.bin->operands[0] != mask) {
				dasm_warn("NAP2: invalid first argument 0x%04X\n",
					  stmt->u.insn.bin->operands[0]);
			}
			if (stmt->u.insn.bin->operands[1] != mask) {
				dasm_warn("NAP2: invalid second argument 0x%04X\n",
					  stmt->u.insn.bin->operands[1]);
			}
			if (stmt->u.insn.bin->operands[2] != 0) {
				dasm_warn("NAP2: invalid third argument 0x%04X\n",
					  stmt->u.insn.bin->operands[2]);
			}
		} else if (cmdargs.arch < ARCH134) {
			unsigned mask = 0x2F00;
			stmt->u.insn.name = "nap2";
			if (stmt->u.insn.bin->operands[0] != mask) {
				dasm_warn("NAP2: invalid first argument 0x%04X\n",
					  stmt->u.insn.bin->operands[0]);
			}
			if (stmt->u.insn.bin->operands[1] != mask) {
				dasm_warn("NAP2: invalid second argument 0x%04X\n",
					  stmt->u.insn.bin->operands[1]);
			}
			if (stmt->u.insn.bin->operands[2] != 0) {
				dasm_warn("NAP2: invalid third argument 0x%04X\n",
					  stmt->u.insn.bin->operands[2]);
			}
		} else {
			unsigned mask = 0x5F00;
			if (cmdargs.subarch != NOSUBARCH)
				mask = 0x5E00;
			stmt->u.insn.name = "nap2";
			if (stmt->u.insn.bin->operands[0] != mask) {
				dasm_warn("NAP2: invalid first argument 0x%04X\n",
					  stmt->u.insn.bin->operands[0]);
			}
			if (stmt->u.insn.bin->operands[1] != mask) {
				dasm_warn("NAP2: invalid second argument 0x%04X\n",
					  stmt->u.insn.bin->operands[1]);
			}
			if (stmt->u.insn.bin->operands[2] != 0) {
				dasm_warn("NAP2: invalid third argument 0x%04X\n",
					  stmt->u.insn.bin->operands[2]);
			}
		}
		break;
	}
	case 0x003: {
		char *str;
		WARN_IF_GE(ARCH5, "ret instruction may not work");
		stmt->u.insn.name = "ret";
		str = xmalloc(4);
		snprintf(str, 4, "lr%u", stmt->u.insn.bin->operands[0]);
		stmt->u.insn.operands[0] = str;
		str = xmalloc(4);
		snprintf(str, 4, "lr%u", stmt->u.insn.bin->operands[2]);
		stmt->u.insn.operands[2] = str;
		break;
	}
	case 0x004: {
		ERROR_IF_LT(ARCH15, "calls instruction not working in <ARCH15");
		stmt->u.insn.name = "calls";
		stmt->u.insn.labelref_operand = 0;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		if (cmdargs.arch < ARCH129) {
			if (stmt->u.insn.bin->operands[0] != 0x1780 ||
			    stmt->u.insn.bin->operands[1] != 0x1780)
				dasm_error("r15-r65 calls: Invalid first or second argument %X %X",
				           stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1]);
		} else  if (cmdargs.arch < ARCH134) {
			if (stmt->u.insn.bin->operands[0] != 0x2F00 ||
			    stmt->u.insn.bin->operands[1] != 0x2F00)
				dasm_error("r129 calls: Invalid first or second argument %X %X",
					   stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1]);
		} else {
			if (cmdargs.subarch == NOSUBARCH) {
				if (stmt->u.insn.bin->operands[0] != 0x5F00 ||
				    stmt->u.insn.bin->operands[1] != 0x5F00)
					dasm_error("r134 calls: Invalid first or second argument %X %X",
						   stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1]);
			} else {
				if ((stmt->u.insn.bin->operands[0] != 0x5E00 && stmt->u.insn.bin->operands[0] != 0x5E08) ||
				    stmt->u.insn.bin->operands[1] != 0x5E00)
					dasm_error("r134 subarch calls: Invalid first or second argument %X %X at instruction %d",
						   stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1], instr_number);

				if (stmt->u.insn.bin->operands[0] == 0x5E08)
					stmt->u.insn.name = "calls2";
			}
		}
		break;
	}
	case 0x005: {
		WARN_IF_LT(ARCH15, "rets instruction may not work");
		stmt->u.insn.name = "rets";
		if (cmdargs.arch < ARCH129) {
			// workaround for >= ARCH65
			if (stmt->u.insn.bin->operands[0] == 0x1782 &&
			    stmt->u.insn.bin->operands[1] == 0x1780 &&
			    stmt->u.insn.bin->operands[2] == 0x0) {
				WARN_IF_LT(ARCH65, "rets2 instruction may not work");
				stmt->u.insn.name = "rets2";
				break;
			}
			if (stmt->u.insn.bin->operands[0] != 0x1780 ||
			    stmt->u.insn.bin->operands[1] != 0x1780 ||
			    stmt->u.insn.bin->operands[2] != 0)
				dasm_warn("rets: Invalid argument(s) for instruction %d (%04X %04X %04X)",
				instr_number, stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1], stmt->u.insn.bin->operands[2]);
		} else if (cmdargs.arch < ARCH134) {
			if (stmt->u.insn.bin->operands[0] == 0x2F02 &&
			    stmt->u.insn.bin->operands[1] == 0x2F00 &&
			    stmt->u.insn.bin->operands[2] == 0x0) {
				stmt->u.insn.name = "rets2";
				break;
			}
			if (stmt->u.insn.bin->operands[0] != 0x2F00 ||
			    stmt->u.insn.bin->operands[1] != 0x2F00 ||
			    stmt->u.insn.bin->operands[2] != 0)
				dasm_warn("arch 129 rets: Invalid argument(s) for instruction %d (%04X %04X %04X)",
					  instr_number, stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1], stmt->u.insn.bin->operands[2]);
		} else {
			if (cmdargs.subarch == NOSUBARCH) {
				if (stmt->u.insn.bin->operands[0] == 0x5F02 &&
				    stmt->u.insn.bin->operands[1] == 0x5F00 &&
				    stmt->u.insn.bin->operands[2] == 0x0) {
					stmt->u.insn.name = "rets2";
					break;
				}
				if (stmt->u.insn.bin->operands[0] != 0x5F00 ||
				    stmt->u.insn.bin->operands[1] != 0x5F00 ||
				    stmt->u.insn.bin->operands[2] != 0)
					dasm_warn("arch 134 rets: Invalid argument(s) for instruction %d (%04X %04X %04X)",
						  instr_number, stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1], stmt->u.insn.bin->operands[2]);
			} else {
				if (cmdargs.subarch == SUBARCH1 &&
				    stmt->u.insn.bin->operands[0] == 0x5E08 &&
				    stmt->u.insn.bin->operands[1] == 0x5E00 &&
				    stmt->u.insn.bin->operands[2] == 0x0) {
					stmt->u.insn.name = "rets2";
					break;
				}
				if (cmdargs.subarch == SUBARCH1 &&
				    stmt->u.insn.bin->operands[0] == 0x5E0A &&
				    stmt->u.insn.bin->operands[1] == 0x5E00 &&
				    stmt->u.insn.bin->operands[2] == 0x0) {
					stmt->u.insn.name = "rets3";
					break;
				}
				if (stmt->u.insn.bin->operands[0] != 0x5E00 ||
				    stmt->u.insn.bin->operands[1] != 0x5E00 ||
				    stmt->u.insn.bin->operands[2] != 0)
					dasm_warn("arch 134 subarch rets: Invalid argument(s) for instruction %d (%04X %04X %04X)",
						  instr_number, stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1], stmt->u.insn.bin->operands[2]);
			}
		}
	


		break;
	}
	case 0x1E0: {
		unsigned int flags, mask;

		if (cmdargs.arch < ARCH15) {
			mask = 0x3FF;
		} else if (cmdargs.arch < ARCH129) {
			mask = 0x7FF;
		} else {
			mask = 0xFFF;
		}

		flags = stmt->u.insn.bin->operands[1];
		switch (flags & mask) {
		case 0x1:
			stmt->u.insn.name = "tkiph";
			break;
		case (0x1 | 0x2):
			stmt->u.insn.name = "tkiphs";
			break;
		case 0x0:
			stmt->u.insn.name = "tkipl";
			break;
		case (0x0 | 0x2):
			stmt->u.insn.name = "tkipls";
			break;
		default:
			dasm_error("Invalid TKIP flags %X", flags);
		}
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 2, 2);
		break;
	}
	case 0x001: {
		unsigned int mask;

		stmt->u.insn.name = "nap";
		if (cmdargs.arch < ARCH15) {
			mask = 0xBC0;
		} else if (cmdargs.arch < ARCH129) {
			mask = 0x1780;
		} else {
			mask = 0x2F00;
		}
		// workaround for >= ARCH65
		if (cmdargs.arch >= ARCH65 &&
		    stmt->u.insn.bin->operands[1] == 0x000 &&
		    stmt->u.insn.bin->operands[2] == 0x000) {
			stmt->u.insn.name = "napv";
			char *str2 = xmalloc(7);
			// for retrocompatibility
			if (cmdargs.arch < ARCH134) {
				snprintf(str2, 6, "0x%03X", (stmt->u.insn.bin->operands[0]));
			} else {
				snprintf(str2, 7, "0x%04X", (stmt->u.insn.bin->operands[0]));
			}
                        stmt->u.insn.operands[0] = str2;
			break;
		}
		if (cmdargs.arch < ARCH129) {
			if (stmt->u.insn.bin->operands[0] != mask) {
				dasm_warn("NAP: invalid first argument 0x%04X\n",
					  stmt->u.insn.bin->operands[0]);
			}
			if (stmt->u.insn.bin->operands[1] != mask) {
				dasm_warn("NAP: invalid second argument 0x%04X\n",
					  stmt->u.insn.bin->operands[1]);
			}
			if (stmt->u.insn.bin->operands[2] != 0) {
				dasm_warn("NAP: invalid third argument 0x%04X\n",
					  stmt->u.insn.bin->operands[2]);
			}
		} else {
			stmt->u.insn.name = "napv2";
			char *str2;
			str2 = xmalloc(7);
			snprintf(str2, 7, "0x%04X", (stmt->u.insn.bin->operands[0]));
			stmt->u.insn.operands[0] = str2;
			str2 = xmalloc(7);
			snprintf(str2, 7, "0x%04X", (stmt->u.insn.bin->operands[1]));
			stmt->u.insn.operands[1] = str2;
			str2 = xmalloc(7);
			snprintf(str2, 7, "0x%04X", (stmt->u.insn.bin->operands[2]));
			stmt->u.insn.operands[2] = str2;
		}
		break;
	}
	case 0x000:
		disasm_opcode_raw(ctx, stmt, 1);
		break;
	default:
		disasm_opcode_raw(ctx, stmt, (cmdargs.unknown_decode == 0));
		break;
	}
}

static void disasm_opcodes(struct disassembler_context *ctx)
{
	struct bin_instruction *bin;
	size_t i;
	struct statement *stmt;
	char *str;

	for (i = 0; i < ctx->nr_insns; i++) {
		bin = &(ctx->code[i]);

		stmt = xmalloc(sizeof(struct statement));
		stmt->type = STMT_INSN;
		INIT_LIST_HEAD(&stmt->list);
		stmt->u.insn.bin = bin;
		stmt->u.insn.labelref_operand = -1; /* none */

		stmt->u.insn.weird_bit = bin->weird_bit;

		switch (bin->opcode & 0xF00) {
		case 0x200:
			stmt->u.insn.name = "srx";

			str = xmalloc(3);
			snprintf(str, 3, "%d", (bin->opcode & 0x0F0) >> 4);
			stmt->u.insn.operands[0] = str;
			str = xmalloc(3);
			snprintf(str, 3, "%d", (bin->opcode & 0x00F));
			stmt->u.insn.operands[1] = str;

			disasm_std_operand(stmt, 0, 2);
			disasm_std_operand(stmt, 1, 3);
			disasm_std_operand(stmt, 2, 4);
			break;
		case 0x300:
			stmt->u.insn.name = "orx";

			str = xmalloc(3);
			snprintf(str, 3, "%d", (bin->opcode & 0x0F0) >> 4);
			stmt->u.insn.operands[0] = str;
			str = xmalloc(3);
			snprintf(str, 3, "%d", (bin->opcode & 0x00F));
			stmt->u.insn.operands[1] = str;

			disasm_std_operand(stmt, 0, 2);
			disasm_std_operand(stmt, 1, 3);
			disasm_std_operand(stmt, 2, 4);
			break;
		case 0x400:
			stmt->u.insn.name = "jzx";

			str = xmalloc(3);
			snprintf(str, 3, "%d", (bin->opcode & 0x0F0) >> 4);
			stmt->u.insn.operands[0] = str;
			str = xmalloc(3);
			snprintf(str, 3, "%d", (bin->opcode & 0x00F));
			stmt->u.insn.operands[1] = str;

			disasm_std_operand(stmt, 0, 2);
			disasm_std_operand(stmt, 1, 3);
			stmt->u.insn.labelref_operand = 4;
			stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
			break;
		case 0x500:
			stmt->u.insn.name = "jnzx";

			str = xmalloc(3);
			snprintf(str, 3, "%d", (bin->opcode & 0x0F0) >> 4);
			stmt->u.insn.operands[0] = str;
			str = xmalloc(3);
			snprintf(str, 3, "%d", (bin->opcode & 0x00F));
			stmt->u.insn.operands[1] = str;

			disasm_std_operand(stmt, 0, 2);
			disasm_std_operand(stmt, 1, 3);
			stmt->u.insn.labelref_operand = 4;
			stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
			break;
		case 0x600:
			stmt->u.insn.name = "jnext";

			/* for r129 first operand can be 0x2000 or 0x0000 not sure
			 * about the different but for now we disassemble to two different instructions
			 */
			int extcode = bin->opcode & 0x0FF;
			str = xmalloc(6);

			if (cmdargs.arch == ARCH134) {
				if (cmdargs.subarch == NOSUBARCH) {
					int extension = 0;
					switch (stmt->u.insn.bin->operands[0]) {
					case 0x0000:
						extension = 0;
						break;
					case 0x4000:
						extension = 1;
						break;
					default:
						dasm_int_error("jnext not decodable for arch 134");
					}
					extcode = (extcode << 1) | extension;
				}
				snprintf(str, 6, "0x%03X", extcode);
				if (cmdargs.subarch == NOSUBARCH) {
					if (stmt->u.insn.bin->operands[1] != 0) {
						dasm_int_error("jnext not decodable for arch 134(2)");
					}
				} else {
					if (stmt->u.insn.bin->operands[0] != 0x5E00 ||
					    stmt->u.insn.bin->operands[1] != 0x5E00) {
						dasm_int_error("jnext not decodable for arch 134 subarch (2)");
					}
				}
			} else if (cmdargs.arch >= ARCH129) {
				if (cmdargs.arch == ARCH129 &&
				    cmdargs.subarch == SUBARCH1) {
					if (stmt->u.insn.bin->operands[0] != 0x2F00 ||
					    stmt->u.insn.bin->operands[1] != 0x2F00) {
						dasm_int_error("jnext not decodable for arch < 129");
					}
					snprintf(str, 6, "0x%03X", extcode);
				}
				else {
					int extension = 0;
					switch (stmt->u.insn.bin->operands[0]) {
					case 0x0000:
						extension = 0;
						break;
					case 0x2000:
						extension = 1;
						break;
					default:
						dasm_int_error("jnext not decodable for arch 129");
					}
					extcode = (extcode << 1) | extension;
					snprintf(str, 6, "0x%03X", extcode);
					if (stmt->u.insn.bin->operands[1] != 0) {
						dasm_int_error("jnext not decodable for arch 129(2)");
					}
				}
			} else {
				snprintf(str, 5, "0x%02X", (bin->opcode & 0x0FF));
				if (stmt->u.insn.bin->operands[0] != 0x1780 ||
				    stmt->u.insn.bin->operands[1] != 0x1780) {
					dasm_int_error("jnext not decodable for arch < 129");
				}
			}
			stmt->u.insn.operands[0] = str;

			/* We don't disassemble the first and second operand, as
			 * that always is a dummy r0 operand.
			 * disasm_std_operand(stmt, 0, 1);
			 * disasm_std_operand(stmt, 1, 2);
			 * stmt->u.insn.labelref_operand = 3;
			 */
			stmt->u.insn.labelref_operand = 1;
			stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
			break;
		case 0x700:
			stmt->u.insn.name = "jext";

			/* for r129 first operand can be 0x2000 or 0x0000 not sure
			 * about the different but for now we disassemble to two different instructions
			 */
			extcode = bin->opcode & 0x0FF;
			str = xmalloc(6);
			if (cmdargs.arch == ARCH134) {
				if (cmdargs.subarch == NOSUBARCH) {
					int extension = 0;
					switch (stmt->u.insn.bin->operands[0]) {
					case 0x0000:
						extension = 0;
						break;
					case 0x4000:
						extension = 1;
						break;
					default:
						dasm_int_error("jext not decodable for arch 134");
					}
					extcode = (extcode << 1) | extension;
				}
				snprintf(str, 6, "0x%03X", extcode);
				if (cmdargs.subarch == NOSUBARCH) {
					if (stmt->u.insn.bin->operands[1] != 0) {
						dasm_int_error("jext not decodable for arch 134(2)");
					}
				} else {
					if (stmt->u.insn.bin->operands[0] != 0x5E00 ||
					    stmt->u.insn.bin->operands[1] != 0x5E00) {
						dasm_int_error("jext not decodable for arch 134 subarch (2)");
					}
				}
			} else if (cmdargs.arch >= ARCH129) {
				if (cmdargs.arch == ARCH129 &&
				    cmdargs.subarch == SUBARCH1) {
					if (stmt->u.insn.bin->operands[0] != 0x2F00 ||
					    stmt->u.insn.bin->operands[1] != 0x2F00) {
						dasm_int_error("jnext not decodable for arch < 129");
					}
					snprintf(str, 6, "0x%03X", extcode);
				}
				else {
					int extension = 0;
					switch (stmt->u.insn.bin->operands[0]) {
					case 0x0000:
						extension = 0;
						break;
					case 0x2000:
						extension = 1;
						break;
					default:
						dasm_int_error("jext not decodable for arch 129");
					}
					extcode = (extcode << 1) | extension;
					snprintf(str, 6, "0x%03X", extcode);
					if (stmt->u.insn.bin->operands[1] != 0) {
						dasm_int_error("jext not decodable for arch 129(2)");
					}
				}
			} else {
			        snprintf(str, 5, "0x%02X", (bin->opcode & 0x0FF));
			        if (stmt->u.insn.bin->operands[0] != 0x1780 ||
			            stmt->u.insn.bin->operands[1] != 0x1780) {
					dasm_int_error("jext not decodable for arch < 129");
			        }
			}
			stmt->u.insn.operands[0] = str;

			/* We don't disassemble the first and second operand, as
			 * that always is a dummy r0 operand.
			 * disasm_std_operand(stmt, 0, 1);
			 * disasm_std_operand(stmt, 1, 2);
			 * stmt->u.insn.labelref_operand = 3;
			 */
			stmt->u.insn.labelref_operand = 1;
			stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
			break;
		default:
			disasm_constant_opcodes(ctx, stmt, i);
			break;
		}

		list_add_tail(&stmt->list, &ctx->stmt_list);
	}
}

static struct statement * get_label_at(struct disassembler_context *ctx,
				       unsigned int addr)
{
	unsigned int addrcnt = 0;
	struct statement *stmt, *ret, *prev;

	list_for_each_entry(stmt, &ctx->stmt_list, list) {
		if (stmt->type != STMT_INSN)
			continue;
		if (addrcnt == addr) {
			prev = list_entry(stmt->list.prev, struct statement, list);
			if (prev->type == STMT_LABEL)
				return prev;
			ret = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&ret->list);
			ret->type = STMT_LABEL;
			list_add(&ret->list, &prev->list);

			return ret;
		}
		addrcnt++;
	}

	return NULL;
}

static void resolve_labels(struct disassembler_context *ctx)
{
	struct statement *stmt;
	struct statement *label;
	struct statement *n;
	unsigned int labeladdr;
	unsigned int namecnt = 0;

	/* Resolve label references */
	list_for_each_entry_safe(stmt, n, &ctx->stmt_list, list) {
		if (stmt->type != STMT_INSN)
			continue;
		if (stmt->u.insn.labelref_operand < 0)
			continue; /* Doesn't have label reference operand. */
		labeladdr = stmt->u.insn.labeladdr;
		label = get_label_at(ctx, labeladdr);
		if (!label)
			dasm_error("Labeladdress %X out of bounds", labeladdr);
		stmt->u.insn.labelref = label;
	}

	/* Name the labels */
	list_for_each_entry(stmt, &ctx->stmt_list, list) {
		if (stmt->type != STMT_LABEL)
			continue;
		stmt->u.label.name = xmalloc(20);
		snprintf(stmt->u.label.name, 20, "L%u", namecnt);
		namecnt++;
	}
}

#define MAX_WEIRD_LENGTH 60

static void emit_asm(struct disassembler_context *ctx)
{
	struct statement *stmt;
	int first;
	int err;
	int length;
	unsigned int i, addr = 0;

	err = open_output_file();
	if (err)
		exit(1);

	fprintf(outfile, "%%arch %u\n", ctx->arch);
	if (ctx->subarch != NOSUBARCH)
		fprintf(outfile, "%%subarch %u\n", ctx->subarch);
	fprintf(outfile, "%%start entry\n\n");
	fprintf(outfile, "entry:\n");
	list_for_each_entry(stmt, &ctx->stmt_list, list) {
		switch (stmt->type) {
		case STMT_INSN:
			if (cmdargs.print_addresses)
				fprintf(outfile, "/* %04X */", addr);
			if (cmdargs.emit_weird && stmt->u.insn.weird_bit)
				fprintf(outfile, "{");
			fprintf(outfile, "\t%s", stmt->u.insn.name);
			first = 1;
			length = 0;
			for (i = 0; i < ARRAY_SIZE(stmt->u.insn.operands); i++) {
				if (!stmt->u.insn.operands[i] &&
				    (stmt->u.insn.labelref_operand < 0 ||
				     (unsigned int)stmt->u.insn.labelref_operand != i))
					continue;
				if (first)
					fprintf(outfile, "\t");
				if (!first)
					length += fprintf(outfile, ", ");
				first = 0;
				if (stmt->u.insn.labelref_operand >= 0 &&
				    (unsigned int)stmt->u.insn.labelref_operand == i) {
					length += fprintf(outfile, "%s",
							  stmt->u.insn.labelref->u.label.name);
				} else {
					length += fprintf(outfile, "%s",
							  stmt->u.insn.operands[i]);
				}
			}
			if (cmdargs.emit_weird && stmt->u.insn.weird_bit && first > 0) fprintf (outfile, "\t");
			if (cmdargs.emit_weird && stmt->u.insn.weird_bit && length < MAX_WEIRD_LENGTH) {
				int kk;
				for (kk = 0; kk < MAX_WEIRD_LENGTH - length; kk ++)
					fprintf (outfile, " ");
			}
			if (cmdargs.emit_weird && stmt->u.insn.weird_bit) fprintf (outfile, "}");
			fprintf(outfile, "\n");
			addr++;
			break;
		case STMT_LABEL:
			fprintf(outfile, "%s:\n", stmt->u.label.name);
			break;
		}
}

	close_output_file();
}

static int read_input(struct disassembler_context *ctx)
{
	size_t size = 0, pos = 0;
	size_t ret;
	struct bin_instruction *code = NULL;
	unsigned char tmp[sizeof(uint64_t)];
	uint64_t codeword = 0;
	struct fw_header hdr;
	int err;

	err = open_input_file();
	if (err)
		goto error;

	switch (cmdargs.informat) {
	case FMT_RAW_LE32:
	case FMT_RAW_BE32:
		/* Nothing */
		break;
	case FMT_B43:
		ret = fread(&hdr, 1, sizeof(hdr), infile);
		if (ret != sizeof(hdr)) {
			fprintf(stderr, "Corrupt input file (no b43 header found)\n");
			goto err_close;
		}
		if (hdr.type != FW_TYPE_UCODE) {
			fprintf(stderr, "Corrupt input file. Not a b43 microcode image.\n");
			goto err_close;
		}
		if (hdr.ver != FW_HDR_VER) {
			fprintf(stderr, "Invalid input file header version.\n");
			goto err_close;
		}
		break;
	}

	while (1) {
		if (pos >= size) {
			size += 512;
			code = xrealloc(code, size * sizeof(struct bin_instruction));
		}
		ret = fread(tmp, 1, sizeof(uint64_t), infile);
		if (!ret)
			break;
		if (ret != sizeof(uint64_t)) {
			fprintf(stderr, "Corrupt input file (not 8 byte aligned)\n");
			goto err_free_code;
		}

		switch (cmdargs.informat) {
		case FMT_B43:
		case FMT_RAW_BE32:
			codeword = 0;
			codeword |= ((uint64_t)tmp[0]) << 56;
			codeword |= ((uint64_t)tmp[1]) << 48;
			codeword |= ((uint64_t)tmp[2]) << 40;
			codeword |= ((uint64_t)tmp[3]) << 32;
			codeword |= ((uint64_t)tmp[4]) << 24;
			codeword |= ((uint64_t)tmp[5]) << 16;
			codeword |= ((uint64_t)tmp[6]) << 8;
			codeword |= ((uint64_t)tmp[7]);
			codeword = ((codeword & (uint64_t)0xFFFFFFFF00000000ULL) >> 32) |
				   ((codeword & (uint64_t)0x00000000FFFFFFFFULL) << 32);
			break;
		case FMT_RAW_LE32:
			codeword = 0;
			codeword |= ((uint64_t)tmp[7]) << 56;
			codeword |= ((uint64_t)tmp[6]) << 48;
			codeword |= ((uint64_t)tmp[5]) << 40;
			codeword |= ((uint64_t)tmp[4]) << 32;
			codeword |= ((uint64_t)tmp[3]) << 24;
			codeword |= ((uint64_t)tmp[2]) << 16;
			codeword |= ((uint64_t)tmp[1]) << 8;
			codeword |= ((uint64_t)tmp[0]);
			break;
		}

		code[pos].weird_bit = 0;

		if (cmdargs.arch < ARCH15) {
			if (codeword >> 48) {
				fprintf(stderr, "Instruction format error at 0x%X (upper not clear). "
					"Wrong input format or architecture?\n", (unsigned int)pos);
				goto err_free_code;
			}
			code[pos].opcode = (codeword >> 36) & 0xFFF;
			code[pos].operands[2] = codeword & 0xFFF;
			code[pos].operands[1] = (codeword >> 12) & 0xFFF;
			code[pos].operands[0] = (codeword >> 24) & 0xFFF;
		} else if (cmdargs.arch < ARCH129) {
			if (codeword >> 51) {
				fprintf(stderr, "Instruction format error at 0x%X (upper not clear). "
					"Wrong input format or architecture?\n", (unsigned int)pos);
				goto err_free_code;
			}
			code[pos].opcode = (codeword >> 39) & 0xFFF;
			code[pos].operands[2] = codeword & 0x1FFF;
			code[pos].operands[1] = (codeword >> 13) & 0x1FFF;
			code[pos].operands[0] = (codeword >> 26) & 0x1FFF;
		} else if (cmdargs.arch < ARCH134) {
			if (codeword >> 54) {
				fprintf(stderr, "Instruction format error at 0x%X (upper not clear). "
					"Wrong input format or architecture?\n", (unsigned int)pos);
				goto err_free_code;
			}
			code[pos].weird_bit = (codeword >> 53) & 0x1;
			code[pos].opcode = (codeword >> 42) & 0x7FF;
			code[pos].operands[2] = codeword & 0x3FFF;
			code[pos].operands[1] = (codeword >> 14) & 0x3FFF;
			code[pos].operands[0] = (codeword >> 28) & 0x3FFF;
		} else {
			if (codeword >> 57) {
				fprintf(stderr, "Instruction format error at 0x%X (upper not clear). "
					"Wrong input format or architecture?\n", (unsigned int)pos);
				goto err_free_code;
			}
			code[pos].weird_bit = (codeword >> 56) & 0x1;
			code[pos].opcode = (codeword >> 45) & 0x7FF;
			code[pos].operands[2] = codeword & 0x7FFF;
			code[pos].operands[1] = (codeword >> 15) & 0x7FFF;
			code[pos].operands[0] = (codeword >> 30) & 0x7FFF;
		}
#ifdef FULLDEBUG
		printf("%16llX\t", codeword);
		printf ("%02X\t%02X\t%02X\t%02X\n", code[pos].opcode, code[pos].operands[0], code[pos].operands[1], code[pos].operands[2]);
#endif // FULLDEBUG
		pos++;
	}

	ctx->code = code;
	ctx->nr_insns = pos;

	close_input_file();

	return 0;

err_free_code:
	free(code);
err_close:
	close_input_file();
error:
	return -1;
}

static void disassemble(void)
{
	struct disassembler_context ctx;
	int err;

	memset(&ctx, 0, sizeof(ctx));
	INIT_LIST_HEAD(&ctx.stmt_list);
	ctx.arch = cmdargs.arch;
	ctx.subarch = cmdargs.subarch;

	err = read_input(&ctx);
	if (err)
		exit(1);
	disasm_opcodes(&ctx);
	resolve_labels(&ctx);
	emit_asm(&ctx);
}

int main(int argc, char **argv)
{
	int err, res = 1;

	err = parse_args(argc, argv);
	if (err < 0)
		goto out;
	if (err > 0) {
		res = 0;
		goto out;
	}
	disassemble();
	res = 0;
out:
	/* Lazyman simply leaks all allocated memory. */
	return res;
}
