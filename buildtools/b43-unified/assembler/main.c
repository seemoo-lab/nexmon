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
#include "parser.h"
#include "args.h"
#include "initvals.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

arch_t arch_supported [] = {ARCH5, ARCH15, ARCH65, ARCH129, ARCH134, ARCHINVALID};
                        
subarch_valid_t subarch_supported [] = {
        {ARCH129, SUBARCH1 },
        {ARCH134, SUBARCH1 },   
        {ARCHINVALID, SUBARCHINVALID},
};                              

extern int yyparse(void);
extern int yydebug;

struct file infile;
const char *infile_name;
const char *outfile_name;

struct out_operand {
	enum {
		OUTOPER_NORMAL,
		OUTOPER_LABELREF,
	} type;

	union {
		unsigned int operand; /* For NORMAL */
		struct label *label; /* For LABELREF */
	} u;
};

struct code_output {
	enum {
		OUT_INSN,
		OUT_LABEL,
	} type;

	/* Set to true, if this is a jump instruction.
	 * This is only used when assembling RET to check
	 * whether the previous instruction was a jump or not. */
	bool is_jump_insn;

	unsigned int opcode;
	struct out_operand operands[3];

	/* The absolute address of this instruction.
	 * Only used in resolve_labels(). */
	unsigned int address;

	const char *labelname; /* only for OUT_LABEL */
	/* Set to 1, if this is the %start instruction. */
	int is_start_insn;

	struct list_head list;

	unsigned int weird;
};

struct assembler_context {
	/* The architecture version (802.11 core revision) */
	unsigned int arch;
	unsigned int subarch;

	struct label *start_label;

	/* Tracking stuff */
	struct statement *cur_stmt;

	struct list_head output;
};


#define for_each_statement(ctx, s)			\
	list_for_each_entry(s, &infile.sl, list) {	\
		ctx->cur_stmt = s;

#define for_each_statement_end(ctx, s)			\
	} do { ctx->cur_stmt = NULL; } while (0)

#define _msg_helper(type, stmt, msg, x...)	do {		\
	fprintf(stderr, "Assembler " type);			\
	if (stmt) {						\
		fprintf(stderr, " (file \"%s\", line %u)",	\
			stmt->info.file,			\
			stmt->info.lineno);			\
	}							\
	fprintf(stderr, ":\n  " msg "\n" ,##x);			\
						} while (0)

#define asm_error(ctx, msg, x...)	do {			\
	_msg_helper("ERROR", (ctx)->cur_stmt, msg ,##x);	\
	exit(1);						\
					} while (0)

#define asm_warn(ctx, msg, x...)					\
	if (cmdargs.suppress_warnings == 0) {				\
		_msg_helper("warning", (ctx)->cur_stmt, msg ,##x);	\
	}

#define asm_info(ctx, msg, x...)	\
	_msg_helper("info", (ctx)->cur_stmt, msg ,##x)

#define WARN_IF_LT(ARCH, MSG)				\
        if (ctx->arch < ARCH) {				\
                asm_warn(ctx, MSG);			\
        }

#define WARN_IF_GE(ARCH, MSG)				\
        if (ctx->arch >= ARCH) {			\
                asm_warn(ctx, MSG);			\
        }

#define ERROR_IF_LT(ARCH, MSG)				\
        if (ctx->arch < ARCH) {				\
                asm_error(ctx, MSG);			\
        }

static void eval_directives(struct assembler_context *ctx)
{
	struct statement *s;
	struct asmdir *ad;
	struct label *l;
	int have_start_label = 0;
	int have_arch = 0;
	int have_subarch = 0;
	unsigned int arch_fallback = 0;

	for_each_statement(ctx, s) {
		if (s->type == STMT_ASMDIR) {
			ad = s->u.asmdir;
			switch (ad->type) {
			case ADIR_ARCH:
				if (have_arch)
					asm_error(ctx, "Multiple %%arch definitions");
				ctx->arch = ad->u.arch;
				if (ctx->arch > ARCH5 && ctx->arch < ARCH15)
					arch_fallback = ARCH5;
				if (ctx->arch > ARCH15 && ctx->arch < ARCH65)
					arch_fallback = ARCH15;
				if (ctx->arch > ARCH65 && ctx->arch < ARCH129)
					arch_fallback = ARCH65;
				if (ctx->arch > ARCH129 && ctx->arch < ARCH134)
					arch_fallback = ARCH129;
				if (ctx->arch > ARCH134)
					arch_fallback = ARCH134;
				if (arch_fallback) {
					asm_warn(ctx, "Using %%arch %d is incorrect. "
						 "The wireless core revision %d uses the "
						 "firmware architecture %d. So use %%arch %d",
						 ctx->arch, ctx->arch, arch_fallback, arch_fallback);
					ctx->arch = arch_fallback;
				}
				int kk;
				int supported_arch_number = sizeof (arch_supported) / sizeof (arch_t);
				for (kk = 0; arch_supported [kk] != ARCHINVALID; kk ++)
				{
					if (ctx->arch == arch_supported [kk])
						break;
				}
				if (arch_supported [kk] == ARCHINVALID) {
					fprintf (stderr, "Architecture version %u unsupported",
						 ctx->arch);
				        fprintf(stderr, "Only these archs are supported: ");
				        for (kk = 0; kk < supported_arch_number - 1; kk ++) {
				                fprintf(stderr, "%d ", arch_supported [kk]);
				        }
					fprintf(stderr, "\n");
					asm_error (ctx, "Error");
				}
				have_arch = 1;
				break;
			case ADIR_SUBARCH:
				if (have_subarch)
					asm_error(ctx, "Multiple %%subarch definitions");
				if (!have_arch)
					asm_error(ctx, "Cannot define subarch with no arch defined");
				ctx->subarch = ad->u.subarch;
				for (kk = 0; subarch_supported [kk].arch != ARCHINVALID; kk ++) {
					if (ctx->arch == subarch_supported [kk].arch &&
					    ctx->subarch == subarch_supported [kk].subarch)
						break;
				}
				if (subarch_supported [kk].arch == ARCHINVALID) {
					fprintf(stderr, "Unsupported subarchitecture, ");
					fprintf(stderr, "only these subarchs are supported:\n");
					for (kk = 0; subarch_supported [kk].arch != ARCHINVALID; kk ++) {
						fprintf(stderr, "\tarch %d subarch %d\n",
							subarch_supported [kk].arch,
							subarch_supported [kk].subarch);
					}
					asm_error (ctx, "Error");
				}
				have_subarch = 1;
				break;
			case ADIR_START:
				if (have_start_label)
					asm_error(ctx, "Multiple %%start definitions");
				ctx->start_label = ad->u.start;
				have_start_label = 1;
				break;
			default:
				asm_error(ctx, "Unknown ASM directive");
			}
		}
	} for_each_statement_end(ctx, s);

	if (!have_arch)
		asm_error(ctx, "No %%arch defined");
	if (!have_start_label)
		asm_info(ctx, "Using start address 0");
}

static bool is_possible_imm(unsigned int imm)
{
	unsigned int mask;

	/* Immediates are only possible up to 16bit (wordsize). */
	mask = ~0;
	mask <<= 16;
	if (imm & (1 << 15)) {
		if ((imm & mask) != mask &&
		    (imm & mask) != 0)
			return 0;
	} else {
		if ((imm & mask) != 0)
			return 0;
	}

	return 1;
}

static unsigned int immediate_nr_bits(struct assembler_context *ctx)
{
	if (ctx->arch < ARCH15)
		return 10; /* 10 bits */
	else if (ctx->arch < ARCH129)
		return 11; /* 11 bits */
	else if (ctx->arch < ARCH134)
		return 12; /* 12 bits */
        else if (ctx->arch == ARCH134)
		return 13; /* 13 bits */
	asm_error(ctx, "Internal error: immediate_nr_bits unknown arch\n");
}

static bool is_valid_imm(struct assembler_context *ctx,
			 unsigned int imm)
{
	unsigned int mask;
	unsigned int immediate_size;

	/* This function checks if the immediate value is representable
	 * as a native immediate operand.
	 *
	 * For v5 architecture the immediate can be 10bit long.
	 * For v15 and v65 architecture the immediate can be 11bit long.
	 * For v129-v132 architecture the immediate can be 12bit long.
	 * For v134 architecture the immediate can be 13bit long.
	 *
	 * The value is sign-extended, so we allow values
	 * of 0xFFFA, for example.
	 */

	if (!is_possible_imm(imm))
		return 0;
	imm &= 0xFFFF;

	immediate_size = immediate_nr_bits(ctx);

	/* First create a mask with all possible bits for
	 * an immediate value unset. */
	mask = (~0u << immediate_size) & 0xFFFF;
	/* Is the sign bit of the immediate set? */
	if (imm & (1 << (immediate_size - 1))) {
		/* Yes, so all bits above that must also
		 * be set, otherwise we can't represent this
		 * value in an operand. */
		if ((imm & mask) != mask)
			return 0;
	} else {
		/* All bits above the immediate's size must
		 * be unset. */
		if (imm & mask)
			return 0;
	}

	return 1;
}

/* This checks if the value is nonzero and a power of two. */
static bool is_power_of_two(unsigned int value)
{
	return (value && ((value & (value - 1)) == 0));
}

/* This checks if all bits set in the mask are contiguous.
 * Zero is also considered a contiguous mask. */
static bool is_contiguous_bitmask(unsigned int mask)
{
	unsigned int low_zeros_mask;
	bool is_contiguous;

	if (mask == 0)
		return 1;
	/* Turn the lowest zeros of the mask into a bitmask.
	 * Example:  0b00011000 -> 0b00000111 */
	low_zeros_mask = (mask - 1) & ~mask;
	/* Adding the low_zeros_mask to the original mask
	 * basically is a bitwise OR operation.
	 * If the original mask was contiguous, we end up with a
	 * contiguous bitmask from bit 0 to the highest bit
	 * set in the original mask. Adding 1 will result in a single
	 * bit set, which is a power of two. */
	is_contiguous = is_power_of_two(mask + low_zeros_mask + 1);

	return is_contiguous;
}

static unsigned int generate_imm_operand(struct assembler_context *ctx,
					 const struct immediate *imm)
{
	unsigned int val, tmp;
	unsigned int mask;

	val = 0xC00;
	if (ctx->arch >= ARCH15 && ctx->arch < ARCH129)
		val <<= 1;
	else if (ctx->arch == ARCH129)
		val <<= 2;
	else if (ctx->arch == ARCH134)
		val <<= 3;
	tmp = imm->imm;

	if (!is_valid_imm(ctx, tmp)) {
		asm_warn(ctx, "IMMEDIATE 0x%X (%d) too long "
			      "(> %u bits + sign). Did you intend to "
			      "use implicit sign extension?",
			 tmp, (int)tmp, immediate_nr_bits(ctx) - 1);
	}

	if (ctx->arch < ARCH15)
		tmp &= 0x3FF;
	else if (ctx->arch < ARCH129)
		tmp &= 0x7FF;
	else if (ctx->arch < ARCH134)
		tmp &= 0xFFF;
	else
		tmp &= 0x1FFF;	// not sure
	val |= tmp;

	return val;
}

static unsigned int generate_reg_operand(struct assembler_context *ctx,
					 const struct registr *reg)
{
	unsigned int val = 0;

	switch (reg->type) {
	case GPR:
		val |= 0xBC0;
		if (ctx->arch >= ARCH15 && ctx->arch < ARCH129)
			val <<= 1;
		else if (ctx->arch == ARCH129)
			val <<= 2;
		else if (ctx->arch == ARCH134 && ctx->subarch == NOSUBARCH)
			val = 0x5F00;
		else if (ctx->arch == ARCH134 && ctx->subarch == SUBARCH1)
			val = 0x5E00;
           
		if (ctx->arch < ARCH65) {
			if (reg->nr & ~0x3F)
				asm_error(ctx, "GPR-nr too big for this arch");
		}
		else if (ctx->arch < ARCH134) {
			if (reg->nr & ~0x7F)
				asm_error(ctx, "GPR-nr too big for this arch)");
		} else {
			if (reg->nr & ~0x7F)
				asm_error(ctx, "GPR-nr too big for this arch)");
		}
		val |= reg->nr;
		break;

	case SPR:
		val |= 0x800;
		if (ctx->arch >= ARCH15 && ctx->arch < ARCH129)
			val <<= 1;
		else if (ctx->arch < ARCH134)
			val <<= 2;
		else if (ctx->arch == ARCH134)
			val <<= 3;
		if (ctx->arch < ARCH15) {
			if (reg->nr & ~0x1FF)
				asm_error(ctx, "SPR-nr too big for this arch");
		}
		else if (ctx->arch < ARCH129) {
			if (reg->nr & ~0x3FF)
				asm_error(ctx, "SPR-nr too big for this arch");
		}
		else if (ctx->arch < ARCH134) {
			if (reg->nr & ~0x7FF)
				asm_error(ctx, "SPR-nr too big for this arch");
		} else {
			if (reg->nr & ~0xFFF)
				asm_error(ctx, "SPR-nr too big for this arch");
		}
		val |= reg->nr;
		break;
	case OFFR:
		asm_error (ctx, "This code should not be executed, it was left for debugging");
/*
		val |= 0x860;
		if (ctx->arch >= ARCH15 && ctx->arch < ARCH129)
			val <<= 1;
		else if (ctx->arch == ARCH129)
			val <<= 2;
		if (ctx->arch < ARCH65) {
			if (reg->nr & ~0x7)
				asm_error(ctx, "OFFR-nr too big");
		}
		else {
			if (reg->nr & ~0xF)
				asm_error(ctx, "OFFR-nr too big (arch15 and arch129)");
		}
		val |= reg->nr;
*/
		break;
	default:
		asm_error(ctx, "generate_reg_operand() regtype");
	}

	return val;
}

static unsigned int generate_mem_operand(struct assembler_context *ctx,
					 const struct memory *mem)
{
	unsigned int val = 0, off, reg, off_mask, reg_shift;

	switch (mem->type) {
	case MEM_DIRECT:
		off = mem->offset;
		if (ctx->arch < ARCH15) {
			if (off & ~0x7FF) {
				asm_error(ctx, "DIRECT memoffset 0x%X too long (> 11 bits)", off);
				// off &= 0x7FF;
			}
		} else if (ctx->arch < ARCH129) {
			if (off & ~0xFFF) {
				asm_error(ctx, "DIRECT memoffset 0x%X too long (> 12 bits)", off);
				// off &= 0xFFF;
			}
		} else if (ctx->arch < ARCH134) {
			if (off & ~0x1FFF) {
				asm_error(ctx, "DIRECT memoffset 0x%X too long (> 13 bits)", off);
				// off &= 0x1FFF;
			}
		} else {
			if (ctx->subarch == 1 &&
			    (off & ~0x3FFF)) // for pixel 8
			{
				asm_error(ctx, "DIRECT memoffset 0x%X too long (> 14 bits)", off);
				// off &= 0x1FFF;
			}
			else if (ctx->subarch == 0 &&
				 (off & ~0x1FFF)) {
				asm_error(ctx, "DIRECT memoffset 0x%X too long (> 13 bits)", off);
				// off &= 0x1FFF;
			}
		}
		val |= off;
		break;
	case MEM_INDIRECT:
		if (ctx->arch < ARCH15) {
			val = 0xA00;
			off_mask = 0x3F;
			reg_shift = 6;
		} else if (ctx->arch < ARCH65) {
			val = 0x1400;
			off_mask = 0x7F;
			reg_shift = 7;
		} else if (ctx->arch < ARCH129) {
			val = 0x1400;
			off_mask = 0x3F;
			reg_shift = 6;
		} else if (ctx->arch < ARCH134) {
			val = 0x2800;
			off_mask = 0x3f;
			reg_shift = 6;
		} else if (ctx->arch == ARCH134) {
			val = 0x5000;
			off_mask = 0x3f;
			reg_shift = 6;
			if (ctx->subarch == 0) { // for asus be
				off_mask = 0x3f;
				reg_shift = 6;
			}
			else if (ctx->subarch == 1) { // for pixel 8
				off_mask = 0x7f;
				reg_shift = 7;
			}
			else {
				asm_error (ctx, "UNKNOWN subarch\n");
			}
		} else {
			asm_error (ctx, "INTERNAL ERROR\n");
		}

		off = mem->offset;
		reg = mem->offr_nr;
		if (off & ~off_mask) {
			asm_warn(ctx, "INDIRECT memoffset 0x%X too long (> %u bits)",
				 off, reg_shift);
			off &= off_mask;
		}
		if(ctx->arch == ARCH5) {
			if (reg > 6) {
				// Assembler bug. The parser shouldn't pass this value.
				asm_error(ctx, "OFFR-nr too big");
			}
			if (reg == 6) {
				asm_warn(ctx, "Using offset register 6. This register is broken "
					 "on certain devices. Use off0 to off5 only.");
			}
		} else if (ctx->arch < ARCH129) {
			if (reg > 15) {
				// Assembler bug. The parser shouldn't pass this value.
				asm_error(ctx, "OFFR-nr too big");
			}
		} else if (ctx->arch == ARCH129) {
			if (reg > 23) {
				// Assembler bug. The parser shouldn't pass this value.
				asm_error(ctx, "OFFR-nr too big");
			}
		} else {
			if (ctx->subarch == 1 && reg >= 28) { // for pixel8
				// Assembler bug. The parser shouldn't pass this value.
				asm_error(ctx, "OFFR-nr too big");
			}
			else if (ctx->subarch == 0 && reg >= 36) { // for asus be
				// Assembler bug. The parser shouldn't pass this value.
				asm_error(ctx, "OFFR-nr too big");
			}
		}
		val |= off;
		val |= (reg << reg_shift);
		break;
	default:
		asm_error(ctx, "generate_mem_operand() memtype");
	}

	return val;
}

static void generate_operand(struct assembler_context *ctx,
			     const struct operand *oper,
			     struct out_operand *out)
{
	out->type = OUTOPER_NORMAL;

	switch (oper->type) {
	case OPER_IMM:
		out->u.operand = generate_imm_operand(ctx, oper->u.imm);
		break;
	case OPER_REG:
		out->u.operand = generate_reg_operand(ctx, oper->u.reg);
		break;
	case OPER_MEM:
		out->u.operand = generate_mem_operand(ctx, oper->u.mem);
		break;
	case OPER_LABEL:
		out->type = OUTOPER_LABELREF;
		out->u.label = oper->u.label;
		break;
	case OPER_ADDR:
		out->u.operand = oper->u.addr->addr;
		break;
	case OPER_RAW:
		out->u.operand = oper->u.raw;
		break;
	default:
		asm_error(ctx, "generate_operand() operstate");
	}
}

static struct code_output * do_assemble_insn(struct assembler_context *ctx,
					     struct instruction *insn,
					     unsigned int opcode)
{
	unsigned int i;
	struct operlist *ol;
	int nr_oper = 0;
	uint64_t code = 0;
	struct code_output *out;
	struct label *labelref = NULL;
	struct operand *oper;
	int have_spr_operand = 0;
	int have_mem_operand = 0;

	out = xmalloc(sizeof(*out));
	INIT_LIST_HEAD(&out->list);
	out->opcode = opcode;

	out->weird = insn->weird;

	ol = insn->operands;
	if (ARRAY_SIZE(out->operands) > ARRAY_SIZE(ol->oper))
		asm_error(ctx, "Internal operand array confusion");

	for (i = 0; i < ARRAY_SIZE(out->operands); i++) {
		oper = ol->oper[i];
		if (!oper)
			continue;

		/* If this is an INPUT operand (first or second), we must
		 * make sure that not both are accessing SPR or MEMORY.
		 * The device only supports one SPR or MEMORY operand in
		 * the input operands. */
		if ((i == 0) || (i == 1)) {
			if ((oper->type == OPER_REG) &&
			    (oper->u.reg->type == SPR)) {
				if (have_spr_operand)
					asm_error(ctx, "Multiple SPR input operands in one instruction");
				have_spr_operand = 1;
			}
			if (oper->type == OPER_MEM) {
				//if (have_mem_operand)
				//	asm_error(ctx, "Multiple MEMORY input operands in on instruction");
				have_mem_operand = 1;
			}
		}

		generate_operand(ctx, oper, &out->operands[i]);
		nr_oper++;
	}
	if (nr_oper != 3)
		asm_error(ctx, "Internal error: nr_oper at "
			       "lowlevel do_assemble_insn");

	list_add_tail(&out->list, &ctx->output);

	return out;
}

static void do_assemble_ret(struct assembler_context *ctx,
			    struct instruction *insn,
			    unsigned int opcode)
{
	struct code_output *out;

	/* Get the previous instruction and check whether it
	 * is a jump instruction. */
	list_for_each_entry_reverse(out, &ctx->output, list) {
		/* Search the last insn. */
		if (out->type == OUT_INSN) {
			if (out->is_jump_insn) {
				asm_warn(ctx, "RET instruction directly after "
					 "jump instruction. The hardware won't like this.");
			}
			break;
		}
	}
	do_assemble_insn(ctx, insn, opcode);
}

static unsigned int merge_ext_into_opcode_human(struct assembler_context *ctx,
						unsigned int opbase,
						struct instruction *insn)
{
	struct operlist *ol;
	unsigned int opcode;

	unsigned long masks[256];
	unsigned long m, s, j = 0;
	unsigned long imm, shift, mask, mask2;
	for(m = 0; m < 16; m++) {
		mask = (1 << (m+1)) - 1;
		for(s = 0; s < 16; s++) {
			mask2 = (mask << s) | (mask >> (16 - s));
			mask2 = mask2 & 0xFFFF;
			masks[j] = mask2;
			j++;
		}
	}

	opcode = opbase;
	ol = insn->operands;

	// both operands can't be immediate
	if(ol->oper[0]->type == OPER_IMM && ol->oper[3]->type == OPER_IMM)
	     asm_error(ctx, "can't encode if first two operands are immediate");

	// second operand is immediate, special case
	if(ol->oper[3]->type == OPER_IMM) {
	     shift = ol->oper[1]->u.raw;
	     mask = ol->oper[2]->u.raw;

	     if(shift > 15)
		  asm_error(ctx, "invalid shift");

	     for(j = 0; j < 256; j++)
		  if(masks[j] == mask)
			break;

	     if(j == 256)
		  asm_error(ctx, "can't build valid mask");

	     m = j / 16; s = j % 16;

	     if(s != shift)
		  asm_error(ctx, "shift error");

	     // check if something is left after possible right shift
	     // and mask
	     if( (0xFFFF >> (16 - s)) & mask )
		  asm_error(ctx, "instruction might lead to unexpected result");

	     // check if second operand value will not change
	     if( (ol->oper[3]->u.imm->imm & ~mask) != ol->oper[3]->u.imm->imm )
		  asm_error(ctx, "instruction might lead to unexpected result(2)");

	} else {
	     for(j = 0; j < 256; j++)
		  if(masks[j] == ((~ol->oper[5]->u.raw) & 0xFFFF))
		       break;
	     if(j == 256)
		  asm_error(ctx, "can't build valid mask");
	     m = j / 16; s = j % 16;
	     mask = (1 << (m + 1)) - 1;
	     mask = (mask << s) | (mask >> (16 - s));
	     if(mask != ((~ol->oper[5]->u.raw) & 0xFFFF))
		  asm_error(ctx, "can't match mask");

	     if(ol->oper[4]->u.raw != 0)
		  asm_error(ctx, "can't use shift here");

	     shift = ol->oper[1]->u.raw;
	     mask = ol->oper[2]->u.raw;

	     if(ol->oper[0]->type == OPER_IMM) {
		  imm = ((ol->oper[0]->u.imm->imm << shift) & mask) & 0xFFFF;
		  ol->oper[0]->u.imm->imm = imm;
		  imm = ((imm >> s) | (imm << (16-s))) & 0xFFFF;
		  if(imm & ~0x3FF ||
		     ol->oper[0]->u.imm->imm != (((imm << s) | (imm >> (16-s))) & 0xFFFF))
		       asm_error(ctx, "immediate value can't be encoded");
		  ol->oper[0]->u.imm->imm = imm;
	     } else {
		  if(shift > 15)
		       asm_error(ctx, "invalid shift");
		  if(mask != ((~ol->oper[5]->u.raw) & 0xFFFF))
		       asm_error(ctx, "unmatched masks");
	     }
	}

	opcode |= (m << 4);
	opcode |= s;

	ol->oper[1] = ol->oper[3];
	ol->oper[2] = ol->oper[6];

	return opcode;
}

static unsigned int merge_ext_into_opcode_human2(struct assembler_context *ctx,
						 unsigned int opbase,
						 struct instruction *insn)
{
	struct operlist *ol;
	unsigned long masks[256];
	unsigned long m, s, j = 0;
	unsigned long mask;
	unsigned int opcode = opbase;
	for(m = 0; m < 16; m++) {
		mask = (1 << (m+1)) - 1;
		for(s = 0; s < 16; s++) {
			masks[j] = (mask << s) & 0xFFFFFFFF;
			j++;
		}
	}

	ol = insn->operands;
	for(j = 0; j < 256; j++)
		if(ol->oper[2]->u.imm->imm == masks[j])
			break;
	if(j == 256)
		asm_error(ctx, "can't build valid mask");
	m = j / 16; s = j % 16;
	mask = (1 << (m + 1)) - 1;
	mask = (mask << s) & 0xFFFFFFFF;
	if(mask != ol->oper[2]->u.imm->imm)
		asm_error(ctx, "can't match mask");
	opcode |= (m << 4);
	opcode |= s;

	ol->oper[2] = ol->oper[3];
	return opcode;
}

static unsigned int merge_ext_into_opcode(struct assembler_context *ctx,
					  unsigned int opbase,
					  struct instruction *insn)
{
	struct operlist *ol;
	unsigned int opcode;
	unsigned int mask, shift;

	ol = insn->operands;
	opcode = opbase;
	mask = ol->oper[0]->u.raw;
	if (mask & ~0xF)
		asm_error(ctx, "opcode MASK extension too big (> 0xF)");
	shift = ol->oper[1]->u.raw;
	if (shift & ~0xF)
		asm_error(ctx, "opcode SHIFT extension too big (> 0xF)");
	opcode |= (mask << 4);
	opcode |= shift;
	ol->oper[0] = ol->oper[2];
	ol->oper[1] = ol->oper[3];
	ol->oper[2] = ol->oper[4];

	return opcode;
}

static void forge_operands_r0r1 (struct assembler_context *ctx,
			         struct instruction *insn,
			         int fake1_val, int fake2_val)
{
	struct operlist *ol = insn->operands;
	struct operand *op2 = ol->oper[2];
	memset(ol->oper, 0, sizeof(ol->oper));

	/* on arch 134 subarch1 both operands must be 0x5E00 */
	struct operand *fake1;
	fake1 = xmalloc(sizeof(*fake1));
	fake1->type = OPER_RAW;
	fake1->u.raw = fake1_val;

	struct operand *fake2;
	fake2 = xmalloc(sizeof(*fake2));
	fake2->type = OPER_RAW;
	fake2->u.raw = fake2_val;

	ol->oper[0] = fake1;
	ol->oper[1] = fake2;
	ol->oper[2] = op2;
}

static unsigned int merge_external_jmp_into_opcode(struct assembler_context *ctx,
						   unsigned int opbase,
						   struct instruction *insn)
{
	struct operand *fake;
	struct operand *extension;
	struct registr *fake_reg;
	struct memory *fake_mem;
	struct operand *target;
	struct operlist *ol;
	unsigned int cond;
	unsigned int opcode;

	ol = insn->operands;
	opcode = opbase;
	cond = ol->oper[0]->u.imm->imm;
	if ((ctx->arch < ARCH129 ||
	     (ctx->arch == ARCH129 && ctx->subarch == SUBARCH1)) &&
	    cond & ~0xFF)
		asm_error(ctx, "External jump condition value too big for this arch");
	if (ctx->arch < ARCH134 && cond & ~0x1FF)
		asm_error(ctx, "External jump condition value too big for this arch");
	if (ctx->arch == ARCH134 && cond & ~0x1FF)
		asm_error(ctx, "External jump condition value too big for ARCH134");

	fake = xmalloc(sizeof(*fake));

	if (ctx->arch < ARCH129) {
		opcode |= cond;
		target = ol->oper[1];
		memset(ol->oper, 0, sizeof(ol->oper));


		/* This instruction for <129 has two fake r0 operands
		 * at position 0 and 1. */
		fake_reg = xmalloc(sizeof(*fake_reg));
		fake->type = OPER_REG;
		fake->u.reg = fake_reg;
		fake_reg->type = GPR;
		fake_reg->nr = 0;
		ol->oper[0] = fake;
		ol->oper[1] = fake;
		ol->oper[2] = target;
	}
	/* For 129 subarch 1 encoding is different than 129 nosubarch */
	else if (ctx->arch == ARCH129 && ctx->subarch == SUBARCH1) {
		opcode |= cond;
		target = ol->oper[1];
		memset(ol->oper, 0, sizeof(ol->oper));

		/* This instruction for 129subarch1 has two fake raw operands
		 * equal to 0x2F00 */
		fake_mem = xmalloc(sizeof(*fake_mem));
		fake->type = OPER_RAW;
		fake->u.raw = 0x2F00;
		ol->oper[0] = fake;
		ol->oper[1] = fake;
		ol->oper[2] = target;
	}
	/* For 129 & 134 (nosubarch) first operand encodes the LSB, second must be all zeros */
	else if (ctx->arch < ARCH134 ||
		 (ctx->arch == ARCH134 && ctx->subarch == 0)) {
		int extension_bit = cond & 1;
		cond = cond >> 1;
		opcode |= cond;
		target = ol->oper[1];
		memset(ol->oper, 0, sizeof(ol->oper));

		extension = xmalloc(sizeof(*extension));

		/* Depending on extension we should either encode 0 or 0x2000
		 * For the former we can use memory, for the latter spr */
		if (extension_bit == 0) {
			struct memory *extension_mem;
			extension_mem = xmalloc(sizeof(*extension_mem));
			extension_mem->type = MEM_DIRECT;
			extension_mem->offset = 0;
			extension->type = OPER_MEM;
			extension->u.mem = extension_mem;
		} else {
			struct registr *extension_reg;
			extension_reg = xmalloc(sizeof(*extension_reg));
			extension_reg->type = SPR;
			extension_reg->nr = 0;
			extension->type = OPER_REG;
			extension->u.reg = extension_reg;
		}

		/* Second operand always zero (use memory) */
		fake_mem = xmalloc(sizeof(*fake_mem));
		fake->type = OPER_MEM;
		fake->u.mem = fake_mem;
		fake_mem->type = MEM_DIRECT;
		fake_mem->offset = 0;

		ol->oper[0] = extension;
		ol->oper[1] = fake;
		ol->oper[2] = target;
	} 
	/* For 134 subarch1 both operands must be 0x5E00 */
	else if (ctx->arch == ARCH134 && ctx->subarch != 0) {
		opcode |= cond;
		target = ol->oper[1];
		memset(ol->oper, 0, sizeof(ol->oper));

		/* This instruction for 134subarch1 has two fake raw operands
		 * equal to 0x5E00 */
		fake_mem = xmalloc(sizeof(*fake_mem));
		fake->type = OPER_RAW;
		fake->u.raw = 0x5E00;
		ol->oper[0] = fake;
		ol->oper[1] = fake;
		ol->oper[2] = target;
	}
	else {
		asm_error(ctx, "Error enconding extension");
	}

	return opcode;
}

static void assemble_instruction(struct assembler_context *ctx,
				 struct instruction *insn);

static void emulate_mov_insn(struct assembler_context *ctx,
			     struct instruction *insn)
{
	struct instruction em_insn;
	struct operlist em_ol;
	struct operand em_op_shift;
	struct operand em_op_mask;
	struct operand em_op_x;
	struct operand em_op_y;
	struct immediate em_imm_x;
	struct immediate em_imm_y;

	struct operand *in, *out;
	unsigned int tmp;

	/* This is a pseudo-OP. We emulate it by OR or ORX */

	in = insn->operands->oper[0];
	out = insn->operands->oper[1];

	em_insn.weird = insn->weird;
	em_insn.op = OP_OR;
	em_ol.oper[0] = in;
	em_imm_x.imm = 0;
	em_op_x.type = OPER_IMM;
	em_op_x.u.imm = &em_imm_x;
	em_ol.oper[1] = &em_op_x;
	em_ol.oper[2] = out;

	if (in->type == OPER_IMM) {
		tmp = in->u.imm->imm;
		if (!is_possible_imm(tmp))
			asm_error(ctx, "MOV operand 0x%X > 16bit", tmp);
		if (cmdargs.movenc == ENC_MOV_XOR ||
		    !is_valid_imm(ctx, tmp)) {
			/* Always use xor or immediate is too big for plain OR */
			em_insn.op = OP_ORX;

			em_op_mask.type = OPER_RAW;
			em_op_mask.u.raw = 0x7;
			em_op_shift.type = OPER_RAW;
			em_op_shift.u.raw = 0x8;

			em_imm_x.imm = (tmp & 0xFF00) >> 8;
			em_op_x.type = OPER_IMM;
			em_op_x.u.imm = &em_imm_x;

			em_imm_y.imm = (tmp & 0x00FF);
			em_op_y.type = OPER_IMM;
			em_op_y.u.imm = &em_imm_y;

			em_ol.oper[0] = &em_op_mask;
			em_ol.oper[1] = &em_op_shift;
			em_ol.oper[2] = &em_op_x;
			em_ol.oper[3] = &em_op_y;
			em_ol.oper[4] = out;
		}
	}

	em_insn.operands = &em_ol;
	assemble_instruction(ctx, &em_insn); /* recurse */
}

static void emulate_jmp_insn(struct assembler_context *ctx,
			     struct instruction *insn)
{
	struct instruction em_insn;
	struct operlist em_ol;
	struct immediate em_condition;
	struct operand em_cond_op;

	/* This is a pseudo-OP. We emulate it with
	 * JEXT 0x7F for < 129, or JEXT 0xBF for >= 129, target
	 * EXCEPTION
	 * 129 subarch 1 -> 0x5F
	 */

	em_insn.weird = insn->weird;
	em_insn.op = OP_JEXT;
	if ((ctx->arch == ARCH129 && ctx->subarch != NOSUBARCH) ||
	    (ctx->arch == ARCH134 && ctx->subarch != NOSUBARCH))
		em_condition.imm = 0x5F;
	else
		em_condition.imm = (ctx->arch < ARCH129 ? 0x7F : 0xBF); /* Ext cond: Always true */
	em_cond_op.type = OPER_IMM;
	em_cond_op.u.imm = &em_condition;
	em_ol.oper[0] = &em_cond_op;
	em_ol.oper[1] = insn->operands->oper[0]; /* Target */
	em_insn.operands = &em_ol;

	assemble_instruction(ctx, &em_insn); /* recurse */
}

static void emulate_jand_insn(struct assembler_context *ctx,
			      struct instruction *insn,
			      int inverted)
{
	struct code_output *out;
	struct instruction em_insn;
	struct operlist em_ol;
	struct operand em_op_shift;
	struct operand em_op_mask;
	struct operand em_op_y;
	struct immediate em_imm;

	struct operand *oper0, *oper1, *oper2;
	struct operand *imm_oper = NULL;
	unsigned int tmp;
	int first_bit, last_bit;

	em_insn.weird = insn->weird;

	oper0 = insn->operands->oper[0];
	oper1 = insn->operands->oper[1];
	oper2 = insn->operands->oper[2];

	if (oper0->type == OPER_IMM)
		imm_oper = oper0;
	if (oper1->type == OPER_IMM)
		imm_oper = oper1;
	if (oper0->type == OPER_IMM && oper1->type == OPER_IMM)
		imm_oper = NULL;

	if (imm_oper) {
		/* We have a single immediate operand.
		 * Check if it's representable by a normal JAND insn.
		 */
		tmp = imm_oper->u.imm->imm;
		if (!is_valid_imm(ctx, tmp)) {
			/* Nope, this must be emulated by JZX/JNZX */
			if (!is_contiguous_bitmask(tmp)) {
				asm_error(ctx, "Long bitmask 0x%X is not contiguous",
					  tmp);
			}

			first_bit = ffs(tmp);
			last_bit = ffs(~(tmp >> (first_bit - 1))) - 1 + first_bit - 1;

			if (inverted)
				em_insn.op = OP_JZX;
			else
				em_insn.op = OP_JNZX;
			em_op_shift.type = OPER_RAW;
			em_op_shift.u.raw = first_bit - 1;
			em_op_mask.type = OPER_RAW;
			em_op_mask.u.raw = last_bit - first_bit;

			em_imm.imm = 0;
			em_op_y.type = OPER_IMM;
			em_op_y.u.imm = &em_imm;

			em_ol.oper[0] = &em_op_mask;
			em_ol.oper[1] = &em_op_shift;
			if (oper0->type != OPER_IMM)
				em_ol.oper[2] = oper0;
			else
				em_ol.oper[2] = oper1;
			em_ol.oper[3] = &em_op_y;
			em_ol.oper[4] = oper2;

			em_insn.operands = &em_ol;

			assemble_instruction(ctx, &em_insn); /* recurse */
			return;
		}
	}

	/* Do a normal JAND/JNAND instruction */
	if (inverted)
		out = do_assemble_insn(ctx, insn, 0x040 | 0x1);
	else
		out = do_assemble_insn(ctx, insn, 0x040);
	out->is_jump_insn = 1;
}

static void assemble_instruction(struct assembler_context *ctx,
				 struct instruction *insn)
{
	struct code_output *out;
	unsigned int opcode;

	switch (insn->op) {
	case OP_MUL:
		do_assemble_insn(ctx, insn, 0x101);
		break;

	case OP_MATH1:
		WARN_IF_LT(ARCH65, "math1 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x102);
		break;
	case OP_MATH2:
		WARN_IF_LT(ARCH65, "math2 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x103);
		break;
	case OP_MATH3:
		WARN_IF_LT(ARCH129, "math3 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x105);
		break;
	case OP_MATH4:
		WARN_IF_LT(ARCH129, "math4 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x108);
		break;
	case OP_MATH5:
		WARN_IF_LT(ARCH129, "math5 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x109);
		break;
	case OP_MATH6:
		WARN_IF_LT(ARCH129, "math6 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1A1);
		break;
	case OP_MATH7:
		WARN_IF_LT(ARCH129, "math7 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1A4);
		break;
	case OP_MATH8:
		WARN_IF_LT(ARCH129, "math8 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1A6);
		break;
	case OP_MATH9:
		WARN_IF_LT(ARCH129, "math9 instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1B1);
		break;
	case OP_MATHA:
		WARN_IF_LT(ARCH129, "mathA instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1B2);
		break;
	case OP_MATHB:
		WARN_IF_LT(ARCH129, "mathB instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1B4);
		break;

	case OP_MATHC:
		WARN_IF_LT(ARCH134, "mathC instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1A2);
		break;
	case OP_MATHD:
		WARN_IF_LT(ARCH134, "mathD instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1A5);
		break;

	case OP_ADD:
		WARN_IF_GE(ARCH65, "add instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1C0);
		break;
	case OP_ADDSC:
		WARN_IF_GE(ARCH65, "add. instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1C2);
		break;
	case OP_ADDC:
		WARN_IF_GE(ARCH65, "addc instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1C1);
		break;
	case OP_ADDSCC:
		WARN_IF_GE(ARCH65, "addc. instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1C3);
		break;

	case OP_XADD:
		WARN_IF_LT(ARCH65, "xadd instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x180);
		break;
	case OP_XADDSC:
		WARN_IF_LT(ARCH65, "xadd. instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x182);
		break;
	case OP_XADDC:
		WARN_IF_LT(ARCH65, "xaddc instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x181);
		break;
	case OP_XADDSCC:
		WARN_IF_LT(ARCH65, "xaddc. instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x183);
		break;

	case OP_SUB:
		WARN_IF_GE(ARCH65, "sub instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1D0);
		break;
	case OP_SUBSC:
		WARN_IF_GE(ARCH65, "sub. instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1D2);
		break;
	case OP_SUBC:
		WARN_IF_GE(ARCH65, "subc instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1D1);
		break;
	case OP_SUBSCC:
		WARN_IF_GE(ARCH65, "subc. instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x1D3);
		break;

	case OP_XSUB:
		WARN_IF_LT(ARCH65, "xsub instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x190);
		break;
	case OP_XSUBSC:
		WARN_IF_LT(ARCH65, "xsub. instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x192);
		break;
	case OP_XSUBC:
		WARN_IF_LT(ARCH65, "xsubc instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x191);
		break;
	case OP_XSUBSCC:
		WARN_IF_LT(ARCH65, "xsubc. instruction could not work on this arch");
		do_assemble_insn(ctx, insn, 0x193);
		break;

	case OP_SRA:
		do_assemble_insn(ctx, insn, 0x130);
		break;
	case OP_OR:
		do_assemble_insn(ctx, insn, 0x160);
		break;
	case OP_AND:
		do_assemble_insn(ctx, insn, 0x140);
		break;
	case OP_XOR:
		do_assemble_insn(ctx, insn, 0x170);
		break;
	case OP_SR:
		do_assemble_insn(ctx, insn, 0x120);
		break;
	case OP_SRX:
		opcode = merge_ext_into_opcode(ctx, 0x200, insn);
		do_assemble_insn(ctx, insn, opcode);
		break;
	case OP_SRXH:
		opcode = merge_ext_into_opcode_human2(ctx, 0x200, insn);
		out = do_assemble_insn(ctx, insn, opcode);
		out->is_jump_insn = 1;
		break;
	case OP_SL:
		do_assemble_insn(ctx, insn, 0x110);
		break;
	case OP_RL:
		do_assemble_insn(ctx, insn, 0x1A0);
		break;
	case OP_RR:
		do_assemble_insn(ctx, insn, 0x1B0);
		break;
	case OP_NAND:
		do_assemble_insn(ctx, insn, 0x150);
		break;
	case OP_ORX:
		opcode = merge_ext_into_opcode(ctx, 0x300, insn);
		do_assemble_insn(ctx, insn, opcode);
		break;
	case OP_ORXH:
		opcode = merge_ext_into_opcode_human(ctx, 0x300, insn);
		do_assemble_insn(ctx, insn, opcode);
		break;
	case OP_MOV:
		emulate_mov_insn(ctx, insn);
		return;
	case OP_JMP:
		emulate_jmp_insn(ctx, insn);
		return;
	case OP_JAND:
		emulate_jand_insn(ctx, insn, 0);
		return;
	case OP_JNAND:
		emulate_jand_insn(ctx, insn, 1);
		return;

	case OP_JS:
		out = do_assemble_insn(ctx, insn, 0x050);
		out->is_jump_insn = 1;
		break;
	case OP_JNS:
		out = do_assemble_insn(ctx, insn, 0x050 | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JE:
		WARN_IF_GE(ARCH65, "je instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D0);
		out->is_jump_insn = 1;
		break;
	case OP_JNE:
		WARN_IF_GE(ARCH65, "jne instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D0 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JLS:
		WARN_IF_GE(ARCH65, "jls instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D2);
		out->is_jump_insn = 1;
		break;
	case OP_JGES:
		WARN_IF_GE(ARCH65, "jges instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D2 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JGS:
		WARN_IF_GE(ARCH65, "jgs instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D4);
		out->is_jump_insn = 1;
		break;
	case OP_JLES:
		WARN_IF_GE(ARCH65, "jles instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D4 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JL:
		WARN_IF_GE(ARCH65, "jl instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0DA);
		out->is_jump_insn = 1;
		break;
	case OP_JGE:
		WARN_IF_GE(ARCH65, "jge instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0DA | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JG:
		WARN_IF_GE(ARCH65, "jg instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0DC);
		break;
	case OP_JLE:
		WARN_IF_GE(ARCH65, "jle instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0DC | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JDN:
		WARN_IF_GE(ARCH65, "jdn instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D6);
		out->is_jump_insn = 1;
		break;
	case OP_JDPZ:
		WARN_IF_GE(ARCH65, "jdpz instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D6 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JDP:
		WARN_IF_GE(ARCH65, "jdp instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D8);
		out->is_jump_insn = 1;
		break;
	case OP_JDNZ:
		WARN_IF_GE(ARCH65, "jdnz instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0D8 | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JBOH:
		WARN_IF_LT(ARCH15, "jboh instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x070);
		out->is_jump_insn = 1;
		break;
	case OP_JNBOH:
		WARN_IF_LT(ARCH15, "jnboh instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x070 | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JBOH2:
		WARN_IF_LT(ARCH65, "jboh2 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x082);
		out->is_jump_insn = 1;
		break;
	case OP_JNBOH2:
		WARN_IF_LT(ARCH65, "jnboh2 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x082 | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JBOH3:
		WARN_IF_LT(ARCH129, "jboh3 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x086);
		out->is_jump_insn = 1;
		break;
	case OP_JNBOH3:
		WARN_IF_LT(ARCH129, "jnboh3 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x086 | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JBOH4:
		WARN_IF_LT(ARCH134, "jboh4 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x060);
		out->is_jump_insn = 1;
		break;
	case OP_JNBOH4:
		WARN_IF_LT(ARCH134, "jnboh4 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x060 | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JMAH:
		WARN_IF_LT(ARCH65, "jmah instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0B0);
		out->is_jump_insn = 1;
		break;
	case OP_JNMAH:
		WARN_IF_LT(ARCH65, "jnmah instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0B0 | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JMAH2:
		WARN_IF_LT(ARCH129, "jmah2 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0B6);
		out->is_jump_insn = 1;
		break;
	case OP_JNMAH2:
		WARN_IF_LT(ARCH129, "jnmah2 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0B6 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JMAH3:
		WARN_IF_LT(ARCH129, "jmah3 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0BA);
		out->is_jump_insn = 1;
		break;
	case OP_JNMAH3:
		WARN_IF_LT(ARCH129, "jnmah3 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0BA | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JMAH4:
		WARN_IF_LT(ARCH129, "jmah4 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0BC);
		out->is_jump_insn = 1;
		break;
	case OP_JNMAH4:
		WARN_IF_LT(ARCH129, "jnmah4 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0BC | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JMAH5:
		WARN_IF_LT(ARCH129, "jmah5 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0B2);
		out->is_jump_insn = 1;
		break;
	case OP_JNMAH5:
		WARN_IF_LT(ARCH129, "jnmah5 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0B2 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_JMAH6:
		WARN_IF_LT(ARCH129, "jmah6 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0B8);
		out->is_jump_insn = 1;
		break;
	case OP_JNMAH6:
		WARN_IF_LT(ARCH129, "jnmah6 instrunction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x0B8 | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_XJE:
		WARN_IF_LT(ARCH65, "xje instruction could not work on this arch");
		//if (ctx->arch == ARCH134)
		//	out = do_assemble_insn(ctx, insn, 0x0190);
		//else
		out = do_assemble_insn(ctx, insn, 0x090);
		out->is_jump_insn = 1;
		break;
	case OP_XJNE:
		WARN_IF_LT(ARCH65, "xjne instruction could not work on this arch");
		//if (ctx->arch == ARCH134)
		//	out = do_assemble_insn(ctx, insn, 0x0190 | 0x1);
		//else
		out = do_assemble_insn(ctx, insn, 0x090 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_XJLS:
		WARN_IF_LT(ARCH65, "xjls instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x092);
		out->is_jump_insn = 1;
		break;
	case OP_XJGES:
		WARN_IF_LT(ARCH65, "xjges instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x092 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_XJGS:
		WARN_IF_LT(ARCH65, "xjgs instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x094);
		out->is_jump_insn = 1;
		break;
	case OP_XJLES:
		WARN_IF_LT(ARCH65, "xjles instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x094 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_XJDN:
		WARN_IF_LT(ARCH65, "xjdn instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x096);
		out->is_jump_insn = 1;
		break;
	case OP_XJDPZ:
		WARN_IF_LT(ARCH65, "xjdpz instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x096 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_XJDP:
		WARN_IF_LT(ARCH65, "xjdp instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x098);
		out->is_jump_insn = 1;
		break;
	case OP_XJDNZ:
		WARN_IF_LT(ARCH65, "xjdnz instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x098 | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_XJL:
		WARN_IF_LT(ARCH65, "xjl instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x09A);
		out->is_jump_insn = 1;
		break;
	case OP_XJGE:
		WARN_IF_LT(ARCH65, "xjge instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x09A | 0x1);
		out->is_jump_insn = 1;
		break;
	case OP_XJG:
		WARN_IF_LT(ARCH65, "xjg instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x09C);
		out->is_jump_insn = 1;
		break;
	case OP_XJLE:
		WARN_IF_LT(ARCH65, "xjle instruction could not work on this arch");
		out = do_assemble_insn(ctx, insn, 0x09C | 0x1);
		out->is_jump_insn = 1;
		break;

	case OP_JZX:
		opcode = merge_ext_into_opcode(ctx, 0x400, insn);
		out = do_assemble_insn(ctx, insn, opcode);
		out->is_jump_insn = 1;
		break;
	case OP_JZXH:
		opcode = merge_ext_into_opcode_human2(ctx, 0x400, insn);
		out = do_assemble_insn(ctx, insn, opcode);
		out->is_jump_insn = 1;
		break;
	case OP_JNZX:
		opcode = merge_ext_into_opcode(ctx, 0x500, insn);
		out = do_assemble_insn(ctx, insn, opcode);
		out->is_jump_insn = 1;
		break;
	case OP_JNZXH:
		opcode = merge_ext_into_opcode_human2(ctx, 0x500, insn);
		out = do_assemble_insn(ctx, insn, opcode);
		out->is_jump_insn = 1;
		break;
	case OP_JEXT:
		opcode = merge_external_jmp_into_opcode(ctx, 0x700, insn);
		out = do_assemble_insn(ctx, insn, opcode);
		out->is_jump_insn = 1;
		break;
	case OP_JNEXT:
		opcode = merge_external_jmp_into_opcode(ctx, 0x600, insn);
		out = do_assemble_insn(ctx, insn, opcode);
		out->is_jump_insn = 1;
		break;

	case OP_CALL:
		if (ctx->arch >= ARCH15)
			asm_error(ctx, "'call' instruction is only supported on arch 5");
		do_assemble_insn(ctx, insn, 0x002);
		break;
	case OP_CALLS:
		if (ctx->arch < ARCH15)
			asm_error(ctx, "'calls' instruction is not supported on arch 5");
		if (ctx->arch == ARCH134 && ctx->subarch == 1)
			forge_operands_r0r1(ctx, insn, 0x5E00, 0x5E00);
		do_assemble_insn(ctx, insn, 0x004);
		break;
	case OP_CALLS2:
		if (ctx->arch < ARCH134 || (ctx->arch == ARCH134 && ctx->subarch == 0))
			asm_error(ctx, "'calls2' instruction is supported only on arch 134 subarch1");
		forge_operands_r0r1(ctx, insn, 0x5E08, 0x5E00);
		do_assemble_insn(ctx, insn, 0x004);
		break;
	case OP_RET:
		if (ctx->arch >= ARCH15)
			asm_error(ctx, "'ret' instruction is only supported on arch 5");
		do_assemble_ret(ctx, insn, 0x003);
		break;
	case OP_RETS:
		if (ctx->arch < ARCH15)
			asm_error(ctx, "'rets' instruction is not supported on arch 5");
		if (ctx->arch == ARCH134 && ctx->subarch == 1)
			forge_operands_r0r1(ctx, insn, 0x5E00, 0x5E00);
		do_assemble_insn(ctx, insn, 0x005);
		break;
	case OP_RETS2:
		if (ctx->arch < ARCH65)
			asm_error(ctx, "'rets2' instruction is only supported on arch >=65");
		if (ctx->arch == ARCH134 && ctx->subarch == 1)
			forge_operands_r0r1(ctx, insn, 0x5E08, 0x5E00);
		do_assemble_insn(ctx, insn, 0x005);
		break;

	case OP_RETS3:
		if (ctx->arch < ARCH134 || ctx->subarch == 0)
			asm_error(ctx, "'rets3' instruction is only supported on arch134 subarch1");
		forge_operands_r0r1(ctx, insn, 0x5E0A, 0x5E00);
		do_assemble_insn(ctx, insn, 0x005);
		break;

	case OP_TKIPH:
	case OP_TKIPHS:
	case OP_TKIPL:
	case OP_TKIPLS:
		do_assemble_insn(ctx, insn, 0x1E0);
		break;
	case OP_NAP:
		do_assemble_insn(ctx, insn, 0x001);
		break;
	case OP_NAP2:
		WARN_IF_LT(ARCH15, "NAP2 not supported on arch 5");
		if (ctx->arch == ARCH134 && ctx->subarch == 1)
			forge_operands_r0r1(ctx, insn, 0x5E00, 0x5E00);
		do_assemble_insn(ctx, insn, 0x002);
		break;
	case OP_NAPV:
		WARN_IF_LT(ARCH65, "napv instruction supported only onarch >=65");
		do_assemble_insn(ctx, insn, 0x001);
		break;
	case OP_NAPV2:
		WARN_IF_LT(ARCH129, "napv2 instruction supported only on arch >=129");
		do_assemble_insn(ctx, insn, 0x001);
		break;
	case RAW_CODE:
		do_assemble_insn(ctx, insn, insn->opcode);
		break;
	default:
		asm_error(ctx, "Unknown op");
	}
}

static void assemble_instructions(struct assembler_context *ctx)
{
	struct statement *s;
	struct instruction *insn;
	struct code_output *out;

	if (ctx->start_label) {
		/* Generate a jump instruction at offset 0 to
		 * jump to the code start.
		 */
		struct instruction sjmp;
		struct operlist ol;
		struct operand oper;

		oper.type = OPER_LABEL;
		oper.u.label = ctx->start_label;
		ol.oper[0] = &oper;
		sjmp.op = OP_JMP;
		sjmp.operands = &ol;

		assemble_instruction(ctx, &sjmp);
		out = list_entry(ctx->output.next, struct code_output, list);
		out->is_start_insn = 1;
	}

	for_each_statement(ctx, s) {
		switch (s->type) {
		case STMT_INSN:
			ctx->cur_stmt = s;
			insn = s->u.insn;
			assemble_instruction(ctx, insn);
			break;
		case STMT_LABEL:
			out = xmalloc(sizeof(*out));
			INIT_LIST_HEAD(&out->list);
			out->type = OUT_LABEL;
			out->labelname = s->u.label->name;

			list_add_tail(&out->list, &ctx->output);
			break;
		case STMT_ASMDIR:
			break;
		}
	} for_each_statement_end(ctx, s);
}

/* Resolve a label reference to the address it points to. */
static int get_labeladdress(struct assembler_context *ctx,
			    struct code_output *this_insn,
			    struct label *labelref)
{
	struct code_output *c;
	bool found = 0;
	int address = -1;

	switch (labelref->direction) {
	case LABELREF_ABSOLUTE:
		list_for_each_entry(c, &ctx->output, list) {
			if (c->type != OUT_LABEL)
				continue;
			if (strcmp(c->labelname, labelref->name) != 0)
				continue;
			if (found) {
				asm_error(ctx, "Ambiguous label reference \"%s\"",
					  labelref->name);
			}
			found = 1;
			address = c->address;
		}
		break;
	case LABELREF_RELATIVE_BACK:
		for (c = list_entry(this_insn->list.prev, typeof(*c), list);
		     &c->list != &ctx->output;
		     c = list_entry(c->list.prev, typeof(*c), list)) {
			if (c->type != OUT_LABEL)
				continue;
			if (strcmp(c->labelname, labelref->name) == 0) {
				/* Found */
				address = c->address;
				break;
			}
		}
		break;
	case LABELREF_RELATIVE_FORWARD:
		for (c = list_entry(this_insn->list.next, typeof(*c), list);
		     &c->list != &ctx->output;
		     c = list_entry(c->list.next, typeof(*c), list)) {
			if (c->type != OUT_LABEL)
				continue;
			if (strcmp(c->labelname, labelref->name) == 0) {
				/* Found */
				address = c->address;
				break;
			}
		}
		break;
	}

	return address;
}

static void resolve_labels(struct assembler_context *ctx)
{
	struct code_output *c;
	int addr;
	unsigned int i;
	unsigned int current_address;

	/* Calculate the absolute addresses for each instruction. */
recalculate_addresses:
	current_address = 0;
	list_for_each_entry(c, &ctx->output, list) {
		switch (c->type) {
		case OUT_INSN:
			c->address = current_address;
			current_address++;
			break;
		case OUT_LABEL:
			c->address = current_address;
			break;
		}
	}

	/* Resolve the symbolic label references. */
	list_for_each_entry(c, &ctx->output, list) {
		switch (c->type) {
		case OUT_INSN:
			if (c->is_start_insn) {
				/* If the first %start-jump jumps to 001, we can
				 * optimize it away, as it's unneeded.
				 */
				i = 2;
				if (c->operands[i].type != OUTOPER_LABELREF)
					asm_error(ctx, "Internal error, %%start insn oper 2 not labelref");
				if (c->operands[i].u.label->direction != LABELREF_ABSOLUTE)
					asm_error(ctx, "%%start label reference not absolute");
				addr = get_labeladdress(ctx, c, c->operands[i].u.label);
				if (addr < 0)
					goto does_not_exist;
				if (addr == 1) {
					list_del(&c->list); /* Kill it */
					goto recalculate_addresses;
				}
			}

			for (i = 0; i < ARRAY_SIZE(c->operands); i++) {
				if (c->operands[i].type != OUTOPER_LABELREF)
					continue;
				addr = get_labeladdress(ctx, c, c->operands[i].u.label);
				if (addr < 0)
					goto does_not_exist;
				c->operands[i].u.operand = addr;
				if (i != 2) {
					/* Is not a jump target.
					 * Make it be an immediate */
					if (ctx->arch == ARCH5)
						c->operands[i].u.operand |= 0xC00;
					else if (ctx->arch < ARCH129)
						c->operands[i].u.operand |= 0xC00 << 1;
					else if (ctx->arch < ARCH134)
						c->operands[i].u.operand |= 0xC00 << 2;
					else if (ctx->arch == ARCH134)
						c->operands[i].u.operand |= 0xC00 << 3;
					else
						asm_error(ctx, "Internal error: label res imm");
				}
			}
			break;
		case OUT_LABEL:
			break;
		}
	}

	return;
does_not_exist:
	asm_error(ctx, "Label \"%s\" does not exist",
		  c->operands[i].u.label->name);
}

static void emit_code(struct assembler_context *ctx)
{
	FILE *fd;
	const char *fn;
	struct code_output *c;
	uint64_t code;
	unsigned char outbuf[8];
	unsigned int insn_count = 0, insn_count_limit;
	struct fw_header hdr;

	fn = outfile_name;
	fd = fopen(fn, "w+");
	if (!fd) {
		fprintf(stderr, "Could not open microcode output file \"%s\"\n", fn);
		exit(1);
	}
	if (IS_VERBOSE_DEBUG)
		printf("\nCode:\n");

	list_for_each_entry(c, &ctx->output, list) {
		switch (c->type) {
		case OUT_INSN:
			insn_count++;
			break;
		default:
			break;
		}
	}

	switch (cmdargs.outformat) {
	case FMT_RAW_LE32:
	case FMT_RAW_BE32:
		/* Nothing */
		break;
	case FMT_B43:
		memset(&hdr, 0, sizeof(hdr));
		hdr.type = FW_TYPE_UCODE;
		hdr.ver = FW_HDR_VER;
		hdr.size = cpu_to_be32(8 * insn_count);
		if (fwrite(&hdr, sizeof(hdr), 1, fd) != 1) {
			fprintf(stderr, "Could not write microcode outfile\n");
			exit(1);
		}
		break;
	}

	if (ctx->arch == ARCH5) {
		insn_count_limit = NUM_INSN_LIMIT_R5;
	} else if (ctx->arch <= ARCH134) {
		insn_count_limit = ~0; //FIXME limit currently unknown.
	} else {
		asm_error(ctx, "Internal error: emit_code unknown arch\n");
	}
	if (insn_count > insn_count_limit)
		asm_warn(ctx, "Generating more than %u instructions. This "
			      "will overflow the device microcode memory.",
			 insn_count_limit);

	list_for_each_entry(c, &ctx->output, list) {
		switch (c->type) {
		case OUT_INSN:
			if (IS_VERBOSE_DEBUG) {
				printf("%03X %04X,%04X,%04X\n",
					c->opcode,
					c->operands[0].u.operand,
					c->operands[1].u.operand,
					c->operands[2].u.operand);
			}

			if (c->weird && ctx->arch < ARCH129)
				asm_error (ctx, "Weird encoding is not supported on this architecture\n");
			if (c->weird && (ctx->arch == ARCH129 && ctx->subarch == SUBARCH1))
				asm_error (ctx, "Weird encoding is not supported on this architecture\n");

			if (ctx->arch == ARCH5) {
				code = 0;
				code |= ((uint64_t)c->operands[2].u.operand);
				code |= ((uint64_t)c->operands[1].u.operand) << 12;
				code |= ((uint64_t)c->operands[0].u.operand) << 24;
				code |= ((uint64_t)c->opcode) << 36;
			} else if (ctx->arch < ARCH129) {
				code = 0;
				code |= ((uint64_t)c->operands[2].u.operand);
				code |= ((uint64_t)c->operands[1].u.operand) << 13;
				code |= ((uint64_t)c->operands[0].u.operand) << 26;
				code |= ((uint64_t)c->opcode) << 39;
			} else if (ctx->arch < ARCH134) {
				if (c->weird != 0)
					code = 0x0020000000000000;
				else
					code = 0;
				code |= ((uint64_t)c->operands[2].u.operand);
				code |= ((uint64_t)c->operands[1].u.operand) << 14;
				code |= ((uint64_t)c->operands[0].u.operand) << 28;
				code |= ((uint64_t)c->opcode) << 42;
			} else {
				if (c->weird != 0)
					code = 0x0100000000000000;
				else
					code = 0;
				code |= ((uint64_t)c->operands[2].u.operand);
				code |= ((uint64_t)c->operands[1].u.operand) << 15;
				code |= ((uint64_t)c->operands[0].u.operand) << 30;
				code |= ((uint64_t)c->opcode) << 45;
			}

			switch (cmdargs.outformat) {
			case FMT_B43:
			case FMT_RAW_BE32:
				code = ((code & (uint64_t)0xFFFFFFFF00000000ULL) >> 32) |
				       ((code & (uint64_t)0x00000000FFFFFFFFULL) << 32);
				outbuf[0] = (code & (uint64_t)0xFF00000000000000ULL) >> 56;
				outbuf[1] = (code & (uint64_t)0x00FF000000000000ULL) >> 48;
				outbuf[2] = (code & (uint64_t)0x0000FF0000000000ULL) >> 40;
				outbuf[3] = (code & (uint64_t)0x000000FF00000000ULL) >> 32;
				outbuf[4] = (code & (uint64_t)0x00000000FF000000ULL) >> 24;
				outbuf[5] = (code & (uint64_t)0x0000000000FF0000ULL) >> 16;
				outbuf[6] = (code & (uint64_t)0x000000000000FF00ULL) >> 8;
				outbuf[7] = (code & (uint64_t)0x00000000000000FFULL) >> 0;
				break;
			case FMT_RAW_LE32:
				outbuf[7] = (code & (uint64_t)0xFF00000000000000ULL) >> 56;
				outbuf[6] = (code & (uint64_t)0x00FF000000000000ULL) >> 48;
				outbuf[5] = (code & (uint64_t)0x0000FF0000000000ULL) >> 40;
				outbuf[4] = (code & (uint64_t)0x000000FF00000000ULL) >> 32;
				outbuf[3] = (code & (uint64_t)0x00000000FF000000ULL) >> 24;
				outbuf[2] = (code & (uint64_t)0x0000000000FF0000ULL) >> 16;
				outbuf[1] = (code & (uint64_t)0x000000000000FF00ULL) >> 8;
				outbuf[0] = (code & (uint64_t)0x00000000000000FFULL) >> 0;
				break;
			}

			if (fwrite(&outbuf, ARRAY_SIZE(outbuf), 1, fd) != 1) {
				fprintf(stderr, "Could not write microcode outfile\n");
				exit(1);
			}
			break;
		case OUT_LABEL:
			break;
		}
	}

	if (cmdargs.print_sizes) {
		printf("%s:  text = %u instructions (%u bytes)\n",
		       fn, insn_count,
		       (unsigned int)(insn_count * sizeof(uint64_t)));
	}

	fclose(fd);
}

static void assemble(void)
{
	struct assembler_context ctx;

	memset(&ctx, 0, sizeof(ctx));
	INIT_LIST_HEAD(&ctx.output);

	eval_directives(&ctx);
	assemble_instructions(&ctx);
	resolve_labels(&ctx);
	emit_code(&ctx);
}

static void initialize(void)
{
	INIT_LIST_HEAD(&infile.sl);
	INIT_LIST_HEAD(&infile.ivals);
#ifdef YYDEBUG
	if (IS_INSANE_DEBUG)
		yydebug = 1;
	else
		yydebug = 0;
#endif /* YYDEBUG */
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
	err = open_input_file();
	if (err)
		goto out;
	initialize();
	yyparse();
	assemble();
	assemble_initvals();
	close_input_file();
	res = 0;
out:
	/* Lazyman simply leaks all allocated memory. */
	return res;
}
