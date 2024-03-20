/*
 *   Copyright (C) 2006-2010  Michael Buesch <m@bues.ch>
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


struct bin_instruction {
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

#define dasm_warn(msg, x...)	\
	_msg_helper("warning", msg ,##x)

#define asm_info(msg, x...)	\
	_msg_helper("info", msg ,##x)

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

	ret = xmalloc(9);
	snprintf(ret, 9, "[0x%X]", operand);

	return ret;
}

static const char * disasm_indirect_mem_operand(unsigned int operand)
{
	char *ret;
	unsigned int offset, reg;

	switch (cmdargs.arch) {
	case 5:
		offset = (operand & 0x3F);
		reg = ((operand >> 6) & 0x7);
		break;
	case 15:
		offset = (operand & 0x3F);
		reg = ((operand >> 6) & 0xf);
		
		break;
	default:
		dasm_int_error("disasm_indirect_mem_operand invalid arch");
	}
	ret = xmalloc(13);
	snprintf(ret, 13, "[0x%02X,off%u]", offset, reg);

	return ret;
}

static const char * disasm_imm_operand(unsigned int operand)
{
	char *ret;
	unsigned int signmask;
	unsigned int mask;

	switch (cmdargs.arch) {
	case 5:
		signmask = (1 << 9);
		mask = 0x3FF;
		break;
	case 15:
		signmask = (1 << 10);
		mask = 0x7FF;
		break;
	default:
		dasm_int_error("disasm_imm_operand invalid arch");
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

	switch (cmdargs.arch) {
	case 5:
		mask = 0x1FF;
		break;
	case 15:
		mask = 0x7FF;
		break;
	default:
		dasm_int_error("disasm_spr_operand invalid arch");
	}

	ret = xmalloc(8);
	snprintf(ret, 8, "spr%03x", (operand & mask));

	return ret;
}

static const char * disasm_gpr_operand(unsigned int operand)
{
	char *ret;
	unsigned int mask;

	switch (cmdargs.arch) {
	case 5:
		mask = 0x3F;
		break;
	case 15:
		mask = 0x7F;
		break;
	default:
		dasm_int_error("disasm_gpr_operand invalid arch");
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

	switch (cmdargs.arch) {
	case 5:
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
		break;
	case 15:
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
		break;
	default:
		dasm_int_error("disasm_std_operand invalid arch");
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
/*
	case 0x102:
		stmt->u.insn.name = "orxX";

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
*/
	
/*
	case 0x102:
		stmt->u.insn.name = "merd";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x103:
		stmt->u.insn.name = "cazz";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
*/
	case 0x180:
		stmt->u.insn.name = "xadd";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x182:
		stmt->u.insn.name = "xadd.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x181:
		stmt->u.insn.name = "xaddc";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x183:
		stmt->u.insn.name = "xaddc.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x190:
		stmt->u.insn.name = "xsub";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x192:
		stmt->u.insn.name = "xsub.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x191:
		stmt->u.insn.name = "xsubc";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x193:
		stmt->u.insn.name = "xsubc.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1C0:
		stmt->u.insn.name = "add";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1C2:
		stmt->u.insn.name = "add.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1C1:
		stmt->u.insn.name = "addc";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1C3:
		stmt->u.insn.name = "addc.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1D0:
		stmt->u.insn.name = "sub";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1D2:
		stmt->u.insn.name = "sub.";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1D1:
		stmt->u.insn.name = "subc";
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		disasm_std_operand(stmt, 2, 2);
		break;
	case 0x1D3:
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
	case 0x1B0:
		stmt->u.insn.name = "rr";
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
	case 0x070:
		stmt->u.insn.name = "jboh";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x070 | 0x1):
		stmt->u.insn.name = "jnboh";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x082:
		stmt->u.insn.name = "jboh2";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x082 | 0x1):
		stmt->u.insn.name = "jnboh2";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x090:
		stmt->u.insn.name = "xje";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x090 | 0x1):
		stmt->u.insn.name = "xjne";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x092:
		stmt->u.insn.name = "xjls";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x092 | 0x1):
		stmt->u.insn.name = "xjges";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x094:
		stmt->u.insn.name = "xjgs";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x094 | 0x1):
		stmt->u.insn.name = "xjles";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x096:
		stmt->u.insn.name = "xjdn";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x096 | 0x1):
		stmt->u.insn.name = "xjdpz";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x098:
		stmt->u.insn.name = "xjdp";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x098 | 0x1):
		stmt->u.insn.name = "xjdnz";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x09A:
		stmt->u.insn.name = "xjl";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x09A | 0x1):
		stmt->u.insn.name = "xjge";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x09C:
		stmt->u.insn.name = "xjg";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x09C | 0x1):
		stmt->u.insn.name = "xjle";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0B0:
		stmt->u.insn.name = "jmah";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0B0 | 0x1):
		stmt->u.insn.name = "jnmah";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D0:
		stmt->u.insn.name = "je";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D0 | 0x1):
		stmt->u.insn.name = "jne";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D2:
		stmt->u.insn.name = "jls";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D2 | 0x1):
		stmt->u.insn.name = "jges";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D4:
		stmt->u.insn.name = "jgs";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D4 | 0x1):
		stmt->u.insn.name = "jles";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D6:
		stmt->u.insn.name = "jdn";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D6 | 0x1):
		stmt->u.insn.name = "jdpz";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0D8:
		stmt->u.insn.name = "jdp";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0D8 | 0x1):
		stmt->u.insn.name = "jdnz";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0DA:
		stmt->u.insn.name = "jl";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0DA | 0x1):
		stmt->u.insn.name = "jge";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x0DC:
		stmt->u.insn.name = "jg";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case (0x0DC | 0x1):
		stmt->u.insn.name = "jle";
		stmt->u.insn.labelref_operand = 2;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		disasm_std_operand(stmt, 0, 0);
		disasm_std_operand(stmt, 1, 1);
		break;
	case 0x002: {
		char *str;
		unsigned int mask;

		switch (cmdargs.arch) {
		case 5:
			stmt->u.insn.name = "call";
			stmt->u.insn.labelref_operand = 1;
			stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
			str = xmalloc(4);
			snprintf(str, 4, "lr%u", stmt->u.insn.bin->operands[0]);
			stmt->u.insn.operands[0] = str;
			break;
		case 15:
			stmt->u.insn.name = "nap2";
			mask = 0x1780;
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
			break;
		}
		break;
	}
	case 0x003: {
		char *str;

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
		if (cmdargs.arch != 15) {
			dasm_error("arch 15 'calls' instruction found in arch %d binary",
				   cmdargs.arch);
		}
		stmt->u.insn.name = "calls";
		stmt->u.insn.labelref_operand = 0;
		stmt->u.insn.labeladdr = stmt->u.insn.bin->operands[2];
		if (stmt->u.insn.bin->operands[0] != 0x1780 ||
		    stmt->u.insn.bin->operands[1] != 0x1780)
			dasm_warn("r15 calls: Invalid first or second argument");
		break;
	}
	case 0x005: {
		if (cmdargs.arch != 15) {
			dasm_error("arch 15 'rets' instruction found in arch %d binary",
				   cmdargs.arch);
		}
		stmt->u.insn.name = "rets";
		// workaround for new operand (mumimo)
		if (stmt->u.insn.bin->operands[0] == 0x1782 &&
		    stmt->u.insn.bin->operands[1] == 0x1780 &&
		    stmt->u.insn.bin->operands[2] == 0x0) {
			stmt->u.insn.name = "rets2";
			break;
		}
		if (stmt->u.insn.bin->operands[0] != 0x1780 ||
		    stmt->u.insn.bin->operands[1] != 0x1780 ||
		    stmt->u.insn.bin->operands[2] != 0)
			dasm_warn("arch 15 rets: Invalid argument(s) for instruction %d (%04X %04X %04X)",
				  instr_number, stmt->u.insn.bin->operands[0], stmt->u.insn.bin->operands[1], stmt->u.insn.bin->operands[2]);
		break;
	}
	case 0x1E0: {
		unsigned int flags, mask;

		switch (cmdargs.arch) {
		case 5:
			mask = 0x3FF;
			break;
		case 15:
			mask = 0x7FF;
			break;
		default:
			dasm_int_error("TKIP invalid arch");
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
		switch (cmdargs.arch) {
		case 5:
			mask = 0xBC0;
			break;
		case 15:
			mask = 0x1780;
			break;
		default:
			dasm_int_error("NAP invalid arch");
		}
		// workaround for mumimo phy
		if (cmdargs.arch == 15 &&
		    stmt->u.insn.bin->operands[1] == 0x000 &&
		    stmt->u.insn.bin->operands[2] == 0x000) {
			stmt->u.insn.name = "napv";
                        char *str2 = xmalloc(6);
                        snprintf(str2, 6, "0x%03X", (stmt->u.insn.bin->operands[0]));
                        stmt->u.insn.operands[0] = str2;

			//stmt->u.insn.labelref_operand = 1;
			//disasm_std_operand(stmt, 0, 0);
			//                 disasm_raw_operand(stmt, 0, 0);
			break;
		}
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

			str = xmalloc(5);
			snprintf(str, 5, "0x%02X", (bin->opcode & 0x0FF));
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

			str = xmalloc(5);
			snprintf(str, 5, "0x%02X", (bin->opcode & 0x0FF));
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

static void emit_asm(struct disassembler_context *ctx)
{
	struct statement *stmt;
	int first;
	int err;
	unsigned int i, addr = 0;

	err = open_output_file();
	if (err)
		exit(1);

	fprintf(outfile, "%%arch %u\n", ctx->arch);
	fprintf(outfile, "%%start entry\n\n");
	fprintf(outfile, "entry:\n");
	list_for_each_entry(stmt, &ctx->stmt_list, list) {
		switch (stmt->type) {
		case STMT_INSN:
			if (cmdargs.print_addresses)
				fprintf(outfile, "/* %04X */", addr);
			fprintf(outfile, "\t%s", stmt->u.insn.name);
			first = 1;
			for (i = 0; i < ARRAY_SIZE(stmt->u.insn.operands); i++) {
				if (!stmt->u.insn.operands[i] &&
				    (stmt->u.insn.labelref_operand < 0 ||
				     (unsigned int)stmt->u.insn.labelref_operand != i))
					continue;
				if (first)
					fprintf(outfile, "\t");
				if (!first)
					fprintf(outfile, ", ");
				first = 0;
				if (stmt->u.insn.labelref_operand >= 0 &&
				    (unsigned int)stmt->u.insn.labelref_operand == i) {
					fprintf(outfile, "%s",
						stmt->u.insn.labelref->u.label.name);
				} else {
					fprintf(outfile, "%s",
						stmt->u.insn.operands[i]);
				}
			}
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

		switch (cmdargs.arch) {
		case 5:
			if (codeword >> 48) {
				fprintf(stderr, "Instruction format error at 0x%X (upper not clear). "
					"Wrong input format or architecture?\n", (unsigned int)pos);
				goto err_free_code;
			}
			code[pos].opcode = (codeword >> 36) & 0xFFF;
			code[pos].operands[2] = codeword & 0xFFF;
			code[pos].operands[1] = (codeword >> 12) & 0xFFF;
			code[pos].operands[0] = (codeword >> 24) & 0xFFF;
			break;
		case 15:
			if (codeword >> 51) {
				fprintf(stderr, "Instruction format error at 0x%X (upper not clear). "
					"Wrong input format or architecture?\n", (unsigned int)pos);
				goto err_free_code;
			}
			code[pos].opcode = (codeword >> 39) & 0xFFF;
			code[pos].operands[2] = codeword & 0x1FFF;
			code[pos].operands[1] = (codeword >> 13) & 0x1FFF;
			code[pos].operands[0] = (codeword >> 26) & 0x1FFF;
			break;
		default:
			fprintf(stderr, "Internal error: read_input unknown arch %u\n",
				cmdargs.arch);
			goto err_free_code;
		}

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
