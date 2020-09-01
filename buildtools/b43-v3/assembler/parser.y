%{

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
#include "initvals.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

extern char *yytext;
extern void yyerror(char *);
extern int yyparse(void);
extern int yylex(void);

static struct operand * store_oper_sanity(struct operand *oper);
static void assembler_assertion_failed(void);

/* The current .section */
extern int section;
/* Pointer to the current initvals section data structure. */
extern struct initvals_sect *cur_initvals_sect;

%}

%token SECTION_TEXT SECTION_IVALS

%token ASM_ARCH ASM_START ASM_ASSERT SPR GPR OFFR LR COMMA SEMICOLON BRACK_OPEN BRACK_CLOSE PAREN_OPEN PAREN_CLOSE HEXNUM DECNUM ARCH_NEWWORLD ARCH_OLDWORLD LABEL IDENT LABELREF

%token EQUAL NOT_EQUAL LOGICAL_OR LOGICAL_AND PLUS MINUS MULTIPLY DIVIDE BITW_OR BITW_AND BITW_XOR BITW_NOT LEFTSHIFT RIGHTSHIFT

%token OP_MUL OP_ADD OP_ADDSC OP_ADDC OP_ADDSCC OP_XADD OP_XADDSC OP_XADDC OP_XADDSCC OP_SUB OP_SUBSC OP_SUBC OP_SUBSCC OP_XSUB OP_XSUBSC OP_XSUBC OP_XSUBSCC OP_SRA OP_OR OP_AND OP_XOR OP_SR OP_SRX OP_SRXH OP_SL OP_RL OP_RR OP_NAND OP_ORX OP_ORXH OP_MOV OP_JMP OP_JAND OP_JNAND OP_JS OP_JNS OP_JE OP_JNE OP_JLS OP_JGES OP_JGS OP_JLES OP_JL OP_JGE OP_JG OP_JLE OP_JZX OP_JZXH OP_JNZX OP_JNZXH OP_JEXT OP_JNEXT OP_JDN OP_JDPZ OP_JDP OP_JDNZ OP_JBOH OP_JNBOH OP_JBOH2 OP_JNBOH2 OP_XJE OP_XJNE OP_XJLS OP_XJGES OP_XJGS OP_XJLES OP_XJDN OP_XJDPZ OP_XJDP OP_XJDNZ OP_XJL OP_XJGE OP_XJG OP_XJLE OP_JMAH OP_JNMAH OP_CALL OP_CALLS OP_RET OP_RETS OP_RETS2 OP_TKIPH OP_TKIPHS OP_TKIPL OP_TKIPLS OP_NAP OP_NAP2 OP_NAPV RAW_CODE

%token IVAL_MMIO16 IVAL_MMIO32 IVAL_PHY IVAL_RADIO IVAL_SHM16 IVAL_SHM32 IVAL_TRAM

%start line

%%

line	: line_terminator {
		/* empty */
	  }
	| line statement line_terminator {
		struct statement *s = $2;
		if (s) {
			if (section != SECTION_TEXT)
				yyerror("Microcode text instruction in non .text section");
			memcpy(&s->info, &cur_lineinfo, sizeof(struct lineinfo));
			list_add_tail(&s->list, &infile.sl);
		}
	  }
	| line section_switch line_terminator {
	  }
	| line ivals_write line_terminator {
		struct initval_op *io = $2;
		if (section != SECTION_IVALS)
			yyerror("InitVals write in non .initvals section");
		memcpy(&io->info, &cur_lineinfo, sizeof(struct lineinfo));
		INIT_LIST_HEAD(&io->list);
		list_add_tail(&io->list, &cur_initvals_sect->ops);
	  }
	;

/* Allow terminating lines with the ";" char */
line_terminator : /* Nothing */
		| SEMICOLON line_terminator
		;

section_switch	: SECTION_TEXT {
			section = SECTION_TEXT;
		  }
		| SECTION_IVALS PAREN_OPEN identifier PAREN_CLOSE {
			const char *sectname = $3;
			struct initvals_sect *s;
			cur_initvals_sect = NULL;
			/* Search if there already is a section by that name. */
			list_for_each_entry(s, &infile.ivals, list) {
				if (strcmp(sectname, s->name) == 0)
					cur_initvals_sect = s;
			}
			if (!cur_initvals_sect) {
				/* Not found, create a new one. */
				s = xmalloc(sizeof(struct initvals_sect));
				s->name = sectname;
				INIT_LIST_HEAD(&s->ops);
				INIT_LIST_HEAD(&s->list);
				list_add_tail(&s->list, &infile.ivals);
				cur_initvals_sect = s;
			}
			section = SECTION_IVALS;
		  }
		;

ivals_write	: IVAL_MMIO16 imm_value COMMA imm_value {
			struct initval_op *iop = xmalloc(sizeof(struct initval_op));
			iop->type = IVAL_W_MMIO16;
			iop->args[0] = (unsigned int)(unsigned long)$2;
			iop->args[1] = (unsigned int)(unsigned long)$4;
			$$ = iop;
		  }
		| IVAL_MMIO32 imm_value COMMA imm_value {
			struct initval_op *iop = xmalloc(sizeof(struct initval_op));
			iop->type = IVAL_W_MMIO32;
			iop->args[0] = (unsigned int)(unsigned long)$2;
			iop->args[1] = (unsigned int)(unsigned long)$4;
			$$ = iop;
		  }
		| IVAL_PHY imm_value COMMA imm_value {
			struct initval_op *iop = xmalloc(sizeof(struct initval_op));
			iop->type = IVAL_W_PHY;
			iop->args[0] = (unsigned int)(unsigned long)$2;
			iop->args[1] = (unsigned int)(unsigned long)$4;
			$$ = iop;
		  }
		| IVAL_RADIO imm_value COMMA imm_value {
			struct initval_op *iop = xmalloc(sizeof(struct initval_op));
			iop->type = IVAL_W_RADIO;
			iop->args[0] = (unsigned int)(unsigned long)$2;
			iop->args[1] = (unsigned int)(unsigned long)$4;
			$$ = iop;
		  }
		| IVAL_SHM16 imm_value COMMA imm_value COMMA imm_value {
			struct initval_op *iop = xmalloc(sizeof(struct initval_op));
			iop->type = IVAL_W_SHM16;
			iop->args[0] = (unsigned int)(unsigned long)$2;
			iop->args[1] = (unsigned int)(unsigned long)$4;
			iop->args[2] = (unsigned int)(unsigned long)$6;
			$$ = iop;
		  }
		| IVAL_SHM32 imm_value COMMA imm_value COMMA imm_value {
			struct initval_op *iop = xmalloc(sizeof(struct initval_op));
			iop->type = IVAL_W_SHM32;
			iop->args[0] = (unsigned int)(unsigned long)$2;
			iop->args[1] = (unsigned int)(unsigned long)$4;
			iop->args[2] = (unsigned int)(unsigned long)$6;
			$$ = iop;
		  }
		| IVAL_TRAM imm_value COMMA imm_value {
			struct initval_op *iop = xmalloc(sizeof(struct initval_op));
			iop->type = IVAL_W_TRAM;
			iop->args[0] = (unsigned int)(unsigned long)$2;
			iop->args[1] = (unsigned int)(unsigned long)$4;
			$$ = iop;
		  }
		;

statement	: asmdir {
			struct asmdir *ad = $1;
			if (ad) {
				struct statement *s = xmalloc(sizeof(struct statement));
				INIT_LIST_HEAD(&s->list);
				s->type = STMT_ASMDIR;
				s->u.asmdir = $1;
				$$ = s;
			} else
				$$ = NULL;
		  }
		| label {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_LABEL;
			s->u.label = $1;
			$$ = s;
		  }
		| insn_mul {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_add {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_addsc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_addc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_addscc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xadd {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xaddsc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xaddc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xaddscc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_sub {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_subsc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_subc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_subscc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xsub {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xsubsc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xsubc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xsubscc {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_sra {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_or {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_and {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xor {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_sr {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_srx {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_srxh {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_sl {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_rl {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_rr {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_nand {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_orx {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_orxh {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		}
		| insn_mov {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jmp {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jand {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jnand {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_js {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jns {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_je {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jne {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jls {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jges {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jgs {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jles {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jdn {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jdpz {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jdp {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jdnz {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jboh {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jnboh {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jboh2 {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jnboh2 {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jmah {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jnmah {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xje {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjne {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjls {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjges {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjgs {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjles {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjdn {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjdpz {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjdp {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjdnz {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjl {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjge {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjg {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_xjle {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jl {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jge {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jg {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jle {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jzx {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jzxh {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jnzx {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jnzxh {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jext {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_jnext {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_call {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_calls {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_ret {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_rets {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_rets2 {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_tkiph {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_tkiphs {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_tkipl {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_tkipls {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_nap {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_nap2 {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_napv {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		| insn_raw {
			struct statement *s = xmalloc(sizeof(struct statement));
			INIT_LIST_HEAD(&s->list);
			s->type = STMT_INSN;
			s->u.insn = $1;
			$$ = s;
		  }
		;

/* ASM directives */
asmdir		: ASM_ARCH hexnum_decnum {
			struct asmdir *ad = xmalloc(sizeof(struct asmdir));
			ad->type = ADIR_ARCH;
			ad->u.arch = (unsigned int)(unsigned long)$2;
			$$ = ad;
		  }
		| ASM_START identifier {
			struct asmdir *ad = xmalloc(sizeof(struct asmdir));
			struct label *label = xmalloc(sizeof(struct label));
			label->name = $2;
			label->direction = LABELREF_ABSOLUTE;
			ad->type = ADIR_START;
			ad->u.start = label;
			$$ = ad;
		  }
		| asm_assert {
			$$ = NULL;
		  }
		;

asm_assert	: ASM_ASSERT assertion {
			unsigned int ok = (unsigned int)(unsigned long)$2;
			if (!ok)
				assembler_assertion_failed();
			$$ = NULL;
		  }
		;

assertion	: PAREN_OPEN assert_expr PAREN_CLOSE {
			$$ = $2;
		  }
		| PAREN_OPEN assertion LOGICAL_OR assertion PAREN_CLOSE {
			unsigned int a = (unsigned int)(unsigned long)$2;
			unsigned int b = (unsigned int)(unsigned long)$4;
			unsigned int result = (a || b);
			$$ = (void *)(unsigned long)result;
		  }
		| PAREN_OPEN assertion LOGICAL_AND assertion PAREN_CLOSE {
			unsigned int a = (unsigned int)(unsigned long)$2;
			unsigned int b = (unsigned int)(unsigned long)$4;
			unsigned int result = (a && b);
			$$ = (void *)(unsigned long)result;
		  }
		;

assert_expr	: imm_value EQUAL imm_value {
			unsigned int a = (unsigned int)(unsigned long)$1;
			unsigned int b = (unsigned int)(unsigned long)$3;
			unsigned int result = (a == b);
			$$ = (void *)(unsigned long)result;
		  }
		| imm_value NOT_EQUAL imm_value {
			unsigned int a = (unsigned int)(unsigned long)$1;
			unsigned int b = (unsigned int)(unsigned long)$3;
			unsigned int result = (a != b);
			$$ = (void *)(unsigned long)result;
		  }
		;

label		: LABEL {
			struct label *label = xmalloc(sizeof(struct label));
			char *l;
			l = xstrdup(yytext);
			l[strlen(l) - 1] = '\0';
			label->name = l;
			$$ = label;
		  }
		;

/* multiply */
insn_mul	: OP_MUL operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_MUL;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* add */
insn_add	: OP_ADD operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_ADD;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* add. */
insn_addsc	: OP_ADDSC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_ADDSC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* addc */
insn_addc	: OP_ADDC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_ADDC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* addc. */
insn_addscc	: OP_ADDSCC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_ADDSCC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* xadd */
insn_xadd	: OP_XADD operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XADD;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* xadd. */
insn_xaddsc	: OP_XADDSC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XADDSC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* xaddc */
insn_xaddc	: OP_XADDC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XADDC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* xaddc. */
insn_xaddscc	: OP_XADDSCC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XADDSCC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* sub */
insn_sub	: OP_SUB operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SUB;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* sub. */
insn_subsc	: OP_SUBSC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SUBSC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* subc */
insn_subc	: OP_SUBC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SUBC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* subc. */
insn_subscc	: OP_SUBSCC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SUBSCC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* xsub */
insn_xsub	: OP_XSUB operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XSUB;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* xsub. */
insn_xsubsc	: OP_XSUBSC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XSUBSC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* xsubc */
insn_xsubc	: OP_XSUBC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XSUBC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

/* xsubc. */
insn_xsubscc	: OP_XSUBSCC operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XSUBSCC;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_sra	: OP_SRA operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SRA;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_or		: OP_OR operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_OR;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_and	: OP_AND operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_AND;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xor	: OP_XOR operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XOR;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_sr		: OP_SR operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SR;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_srx	: OP_SRX extended_operlist {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SRX;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_srxh	: OP_SRXH operlist_2_human {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SRXH;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_sl		: OP_SL operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_SL;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_rl		: OP_RL operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_RL;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_rr		: OP_RR operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_RR;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_nand	: OP_NAND operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_NAND;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_orx	: OP_ORX extended_operlist {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_ORX;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_orxh	: OP_ORXH operlist_3_human {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_ORXH;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_mov	: OP_MOV operlist_2 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_MOV;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jmp	: OP_JMP labelref {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			ol->oper[0] = $2;
			insn->op = OP_JMP;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_jand	: OP_JAND operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JAND;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jnand	: OP_JNAND operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNAND;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_js		: OP_JS operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JS;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jns	: OP_JNS operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNS;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_je		: OP_JE operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JE;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jne	: OP_JNE operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNE;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jls	: OP_JLS operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JLS;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jges	: OP_JGES operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JGES;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jgs	: OP_JGS operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JGS;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jles	: OP_JLES operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JLES;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jl		: OP_JL operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JL;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jge	: OP_JGE operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JGE;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jg		: OP_JG operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JG;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jle	: OP_JLE operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JLE;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jzx	: OP_JZX extended_operlist {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JZX;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jzxh	: OP_JZXH operlist_2_jump_human {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JZXH;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jnzx	: OP_JNZX extended_operlist {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNZX;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jnzxh	: OP_JNZXH operlist_2_jump_human {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNZXH;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jdn	: OP_JDN operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JDN;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jdpz	: OP_JDPZ operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JDPZ;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jdp	: OP_JDP operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JDP;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jdnz	: OP_JDNZ operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JDNZ;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jboh	: OP_JBOH operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JBOH;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jnboh	: OP_JNBOH operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNBOH;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jboh2	: OP_JBOH2 operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JBOH2;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jnboh2	: OP_JNBOH2 operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNBOH2;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jmah	: OP_JMAH operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JMAH;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jnmah	: OP_JNMAH operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNMAH;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xje	: OP_XJE operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJE;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjne	: OP_XJNE operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJNE;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjls	: OP_XJLS operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJLS;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjges	: OP_XJGES operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJGES;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjgs	: OP_XJGS operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJGS;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjles 	: OP_XJLES operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJLES;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjdn	: OP_XJDN operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJDN;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjdpz	: OP_XJDPZ operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJDPZ;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjdp	: OP_XJDP operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJDP;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjdnz	: OP_XJDNZ operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJDNZ;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjl	: OP_XJL operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJL;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjge	: OP_XJGE operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJGE;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjg	: OP_XJG operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJG;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_xjle	: OP_XJLE operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_XJLE;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jext	: OP_JEXT external_jump_operands {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JEXT;
			insn->operands = $2;
			$$ = insn;
		  }
		;

insn_jnext	: OP_JNEXT external_jump_operands {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = OP_JNEXT;
			insn->operands = $2;
			$$ = insn;
		  }
		;

linkreg		: LR regnr {
			$$ = $2;
		  }
		;

insn_call	: OP_CALL linkreg COMMA labelref {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *oper_lr = xmalloc(sizeof(struct operand));
			struct operand *oper_zero = xmalloc(sizeof(struct operand));
			oper_zero->type = OPER_RAW;
			oper_zero->u.raw = 0;
			oper_lr->type = OPER_RAW;
			oper_lr->u.raw = (unsigned long)$2;
			ol->oper[0] = oper_lr;
			ol->oper[1] = oper_zero;
			ol->oper[2] = $4;
			insn->op = OP_CALL;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_calls	:  OP_CALLS labelref {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *oper_r0 = xmalloc(sizeof(struct operand));
			struct registr *r0 = xmalloc(sizeof(struct registr));
			r0->type = GPR;
			r0->nr = 0;
			oper_r0->type = OPER_REG;
			oper_r0->u.reg = r0;
			ol->oper[0] = oper_r0;
			ol->oper[1] = oper_r0;
			ol->oper[2] = $2;
			insn->op = OP_CALLS;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_ret	: OP_RET linkreg COMMA linkreg {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *oper_lr0 = xmalloc(sizeof(struct operand));
			struct operand *oper_lr1 = xmalloc(sizeof(struct operand));
			struct operand *oper_zero = xmalloc(sizeof(struct operand));
			oper_zero->type = OPER_RAW;
			oper_zero->u.raw = 0;
			oper_lr0->type = OPER_RAW;
			oper_lr0->u.raw = (unsigned long)$2;
			oper_lr1->type = OPER_RAW;
			oper_lr1->u.raw = (unsigned long)$4;
			ol->oper[0] = oper_lr0;
			ol->oper[1] = oper_zero;
			ol->oper[2] = oper_lr1;
			insn->op = OP_RET;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_rets	: OP_RETS {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *oper_r0 = xmalloc(sizeof(struct operand));
			struct operand *oper_zero = xmalloc(sizeof(struct operand));
			struct registr *r0 = xmalloc(sizeof(struct registr));
			oper_zero->type = OPER_RAW;
			oper_zero->u.raw = 0;
			r0->type = GPR;
			r0->nr = 0;
			oper_r0->type = OPER_REG;
			oper_r0->u.reg = r0;
			ol->oper[0] = oper_r0;
			ol->oper[1] = oper_r0;
			ol->oper[2] = oper_zero;
			insn->op = OP_RETS;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_rets2	: OP_RETS2 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *oper_r0 = xmalloc(sizeof(struct operand));
			struct operand *oper_r1 = xmalloc(sizeof(struct operand));
			struct operand *oper_zero = xmalloc(sizeof(struct operand));
			struct registr *r0 = xmalloc(sizeof(struct registr));
			struct registr *r2 = xmalloc(sizeof(struct registr));
			oper_zero->type = OPER_RAW;
			oper_zero->u.raw = 0;
			r0->type = GPR;
			r0->nr = 0;
			r2->type = GPR;
			r2->nr = 2;
			oper_r0->type = OPER_REG;
			oper_r0->u.reg = r2;
			oper_r1->type = OPER_REG;
			oper_r1->u.reg = r0;
			ol->oper[0] = oper_r0;
			ol->oper[1] = oper_r1;
			ol->oper[2] = oper_zero;
			insn->op = OP_RETS2;
			insn->operands = ol;
			$$ = insn;
		  }
		;
insn_tkiph	: OP_TKIPH operlist_2 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = $2;
			struct operand *flags = xmalloc(sizeof(struct operand));
			struct immediate *imm = xmalloc(sizeof(struct immediate));
			imm->imm = 0x1;
			flags->type = OPER_IMM;
			flags->u.imm = imm;
			ol->oper[2] = ol->oper[1];
			ol->oper[1] = flags;
			insn->op = OP_TKIPH;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_tkiphs	: OP_TKIPHS operlist_2 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = $2;
			struct operand *flags = xmalloc(sizeof(struct operand));
			struct immediate *imm = xmalloc(sizeof(struct immediate));
			imm->imm = 0x1 | 0x2;
			flags->type = OPER_IMM;
			flags->u.imm = imm;
			ol->oper[2] = ol->oper[1];
			ol->oper[1] = flags;
			insn->op = OP_TKIPH;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_tkipl	: OP_TKIPL operlist_2 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = $2;
			struct operand *flags = xmalloc(sizeof(struct operand));
			struct immediate *imm = xmalloc(sizeof(struct immediate));
			imm->imm = 0x0;
			flags->type = OPER_IMM;
			flags->u.imm = imm;
			ol->oper[2] = ol->oper[1];
			ol->oper[1] = flags;
			insn->op = OP_TKIPH;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_tkipls	: OP_TKIPLS operlist_2 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = $2;
			struct operand *flags = xmalloc(sizeof(struct operand));
			struct immediate *imm = xmalloc(sizeof(struct immediate));
			imm->imm = 0x0 | 0x2;
			flags->type = OPER_IMM;
			flags->u.imm = imm;
			ol->oper[2] = ol->oper[1];
			ol->oper[1] = flags;
			insn->op = OP_TKIPH;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_nap	: OP_NAP {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *regop = xmalloc(sizeof(struct operand));
			struct operand *zeroop = xmalloc(sizeof(struct operand));
			struct registr *r0 = xmalloc(sizeof(struct registr));
			r0->type = GPR;
			r0->nr = 0;
			regop->type = OPER_REG;
			regop->u.reg = r0;
			zeroop->type = OPER_RAW;
			zeroop->u.raw = 0x000;
			ol->oper[0] = regop;
			ol->oper[1] = regop;
			ol->oper[2] = zeroop;
			insn->op = OP_NAP;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_nap2	: OP_NAP2 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *regop = xmalloc(sizeof(struct operand));
			struct operand *zeroop = xmalloc(sizeof(struct operand));
			struct registr *r0 = xmalloc(sizeof(struct registr));
			r0->type = GPR;
			r0->nr = 0;
			regop->type = OPER_REG;
			regop->u.reg = r0;
			zeroop->type = OPER_RAW;
			zeroop->u.raw = 0x000;
			ol->oper[0] = regop;
			ol->oper[1] = regop;
			ol->oper[2] = zeroop;
			insn->op = OP_NAP2;
			insn->operands = ol;
			$$ = insn;
		  }
		;

insn_napv	: OP_NAPV hexnum_decnum {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *nzeroop = xmalloc(sizeof(struct operand));
			struct operand *zeroop = xmalloc(sizeof(struct operand));
			zeroop->type = OPER_RAW;
			zeroop->u.raw = 0x0;
			nzeroop->type = OPER_RAW;
			nzeroop->u.raw = (unsigned long)$2;
			ol->oper[0] = nzeroop;
			ol->oper[1] = zeroop;
			ol->oper[2] = zeroop;
			insn->op = OP_NAPV;
			insn->operands = ol;
			$$ = insn;
                        // struct operand *oper = xmalloc(sizeof(struct operand));
                        // oper->type = OPER_MEM;
                        // oper->u.mem = $1;
                        // $$ = oper;
		  }
		;

insn_raw	: raw_code operlist_3 {
			struct instruction *insn = xmalloc(sizeof(struct instruction));
			insn->op = RAW_CODE;
			insn->operands = $2;
			insn->opcode = (unsigned long)$1;
			$$ = insn;
		  }
		;

raw_code	: RAW_CODE {
			yytext++; /* skip @ */
			$$ = (void *)(unsigned long)strtoul(yytext, NULL, 16);
		  }
		;

extended_operlist : imm_value COMMA imm_value COMMA operand COMMA operand COMMA operand {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *mask_oper = xmalloc(sizeof(struct operand));
			struct operand *shift_oper = xmalloc(sizeof(struct operand));
			mask_oper->type = OPER_RAW;
			mask_oper->u.raw = (unsigned long)$1;
			shift_oper->type = OPER_RAW;
			shift_oper->u.raw = (unsigned long)$3;
			ol->oper[0] = mask_oper;
			ol->oper[1] = shift_oper;
			ol->oper[2] = $5;
			ol->oper[3] = $7;
			ol->oper[4] = store_oper_sanity($9);
			$$ = ol;
		  }
		;

external_jump_operands : imm COMMA labelref {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand *cond = xmalloc(sizeof(struct operand));
			cond->type = OPER_IMM;
			cond->u.imm = $1;
			ol->oper[0] = cond;
			ol->oper[1] = $3;
			$$ = ol;
		  }
		;

operlist_2	: operand COMMA operand {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			ol->oper[0] = $1;
			ol->oper[1] = store_oper_sanity($3);
			$$ = ol;
		  }
		;

operlist_3	: operand COMMA operand COMMA operand {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			ol->oper[0] = $1;
			ol->oper[1] = $3;
			ol->oper[2] = store_oper_sanity($5);
			$$ = ol;
		  }
		;

operlist_2_jump_human	: operand_shift_operand_mask COMMA operand {
			struct operlist *ol = $1;
			ol->oper[3] = store_oper_sanity($3);
			$$ = ol;
		  }
		;

operlist_2_human	: operand_shift_mask2 COMMA operand {
			struct operlist *ol = $1;
			ol->oper[3] = store_oper_sanity($3);
			$$ = ol;
		  }
		;

/* all the following could be implemented:
   Y => implemented N => raise error
	N 0xabcdefgh
	N 0xabcdefgh & BITMASK
	Y REG12 & BITMASK
	Y (REG12 << 16) & BITMASK
	Y (REG31 << 16 | REG12) & BITMASK
	N (REG12 << 16| 0xabcd) & BITMASK
	N (0xabcd0000 | REG12) & BITMASK
*/

operand_shift_operand_mask	: imm {
			yyerror("Human expression not yet implemented");
		  }
		| operandh BITW_AND imm {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			ol->oper[0] = $1;
			ol->oper[1] = xmalloc(sizeof(struct operand));
			ol->oper[1]->type = OPER_IMM;
			ol->oper[1]->u.imm = xmalloc(sizeof(struct immediate));
			ol->oper[1]->u.imm->imm = 0;
			ol->oper[2] = xmalloc(sizeof(struct operand));
			ol->oper[2]->type = OPER_IMM;
			ol->oper[2]->u.imm = $3;
			$$ = ol;
		  }
		| PAREN_OPEN operandh LEFTSHIFT imm PAREN_CLOSE BITW_AND imm {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct immediate *imm = $4;
			if(imm->imm != 16)
				yyerror("Only 16 bit shift allowed here");
			ol->oper[0] = xmalloc(sizeof(struct operand));
			ol->oper[0]->type = OPER_IMM;
			ol->oper[0]->u.imm = xmalloc(sizeof(struct immediate));
			ol->oper[0]->u.imm->imm = 0;
			ol->oper[1] = $2;
			ol->oper[2] = xmalloc(sizeof(struct operand));
			ol->oper[2]->type = OPER_IMM;
			ol->oper[2]->u.imm = $7;
			$$ = ol;
		  }
		| PAREN_OPEN PAREN_OPEN operandh LEFTSHIFT imm PAREN_CLOSE BITW_OR operandh PAREN_CLOSE BITW_AND imm {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct immediate *imm = $5;
			if(imm->imm != 16)
				yyerror("Only 16 bit shift allowed here");
			ol->oper[0] = $8;
			ol->oper[1] = $3;
			ol->oper[2] = xmalloc(sizeof(struct operand));
			ol->oper[2]->type = OPER_IMM;
			ol->oper[2]->u.imm = $11;
			$$ = ol;
		  }
		| PAREN_OPEN PAREN_OPEN operandh LEFTSHIFT imm PAREN_CLOSE BITW_OR imm PAREN_CLOSE BITW_AND imm {
			yyerror("Human expression not yet implemented");
		  }
		| PAREN_OPEN operandh BITW_OR imm PAREN_CLOSE BITW_AND imm {
			yyerror("Human expression not yet implemented");
		  }
		;

/* only the simplest two are implemented:
	(xxx >> S) & mask
	xxx & mask
*/

operand_shift_mask2	: imm {
			yyerror("Human expression not yet implemented");
		  }
		| operandh BITW_AND imm {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			ol->oper[0] = $1;
			ol->oper[1] = xmalloc(sizeof(struct operand));
			ol->oper[1]->type = OPER_IMM;
			ol->oper[1]->u.imm = xmalloc(sizeof(struct immediate));
			ol->oper[1]->u.imm->imm = 0;
			ol->oper[2] = xmalloc(sizeof(struct operand));
			ol->oper[2]->type = OPER_IMM;
			ol->oper[2]->u.imm = $3;
			$$ = ol;
		  }
		| PAREN_OPEN operandh RIGHTSHIFT imm PAREN_CLOSE BITW_AND imm {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct immediate *shift_imm = $4;
			ol->oper[0] = $2;
			ol->oper[1] = xmalloc(sizeof(struct operand));
			ol->oper[1]->type = OPER_IMM;
			ol->oper[1]->u.imm = xmalloc(sizeof(struct immediate));
			ol->oper[1]->u.imm->imm = 0;
			ol->oper[2] = xmalloc(sizeof(struct operand));
			ol->oper[2]->type = OPER_IMM;
			ol->oper[2]->u.imm = $7;
			ol->oper[2]->u.imm->imm <<= shift_imm->imm;
			free(shift_imm);
			$$ = ol;
		  }
		;

operlist_3_human	: operand_shift_mask COMMA operand_shift_mask COMMA operand {
			struct operlist *ol = xmalloc(sizeof(struct operlist));
			struct operand_shift_mask *xxx = $1;
			struct operand *shift_xxx = xmalloc(sizeof(struct operand));
			struct operand *mask_xxx = xmalloc(sizeof(struct operand));
			struct operand_shift_mask *yyy = $3;
			struct operand *shift_yyy = xmalloc(sizeof(struct operand));
			struct operand *mask_yyy = xmalloc(sizeof(struct operand));
			shift_xxx->type = OPER_RAW;
			shift_xxx->u.raw = (unsigned long) xxx->shift;
			mask_xxx->type = OPER_RAW;
			mask_xxx->u.raw = (unsigned long) xxx->mask;

			shift_yyy->type = OPER_RAW;
			shift_yyy->u.raw = (unsigned long) yyy->shift;
			mask_yyy->type = OPER_RAW;
			mask_yyy->u.raw = (unsigned long) yyy->mask;

			ol->oper[0] = xxx->op;
			ol->oper[1] = shift_xxx; 
			ol->oper[2] = mask_xxx;
			ol->oper[3] = yyy->op;
			ol->oper[4] = shift_yyy;
			ol->oper[5] = mask_yyy;
			ol->oper[6] = store_oper_sanity($5);
			free(xxx);
			free(yyy);
			$$ = ol;
		  }
		;

/* duplicate some complex_imm rules to avoid parentheses
	The following are implemented:
		0x143 || 0x143 & 0x12 || 0x143 & ~0x12
		REG12 || REG12 & 0x12 || REG12 & ~0x12
		(REG12 << 0x12) || (REG12 << 0x12) & 0x12 || (REG12 << 0x12) & ~0x12
*/
operand_shift_mask	: imm {
			struct operand_shift_mask *oper = xmalloc(sizeof(struct operand_shift_mask));
			struct operand *oper_imm = xmalloc(sizeof(struct operand));
			oper_imm->type = OPER_IMM;
			oper_imm->u.imm = $1;
			oper->op = oper_imm;
			oper->mask = 0xFFFF;
			oper->shift = 0;
			$$ = oper;
		  }
		| operand_wwo_shift {
			struct operand_shift_mask *oper = $1;
			oper->mask = 0xFFFF;
			$$ = oper;
		  }
		| operand_wwo_shift BITW_AND imm {
			struct operand_shift_mask *oper = $1;
			struct immediate *mask_imm = $3;
			oper->mask = mask_imm->imm;
			free(mask_imm);
			$$ = oper;
		  }
		;

operand_wwo_shift	: operandh {
			struct operand_shift_mask *oper = xmalloc(sizeof(struct operand_shift_mask));
			oper->op = $1;
			oper->shift = 0;
			$$ = oper;
		  }
		| PAREN_OPEN operandh LEFTSHIFT imm PAREN_CLOSE {
			struct operand_shift_mask *oper = xmalloc(sizeof(struct operand_shift_mask));
			struct immediate *shift_imm = $4;
			oper->op = $2;
			oper->shift = shift_imm->imm;
			free(shift_imm);
			$$ = oper;
		  }
		;

operandh	: reg {
			struct operand *oper = xmalloc(sizeof(struct operand));
			oper->type = OPER_REG;
			oper->u.reg = $1;
			$$ = oper;
		  }
		| mem {
			struct operand *oper = xmalloc(sizeof(struct operand));
			oper->type = OPER_MEM;
			oper->u.mem = $1;
			$$ = oper;
		  }
		;

operand		: reg {
			struct operand *oper = xmalloc(sizeof(struct operand));
			oper->type = OPER_REG;
			oper->u.reg = $1;
			$$ = oper;
		  }
		| mem {
			struct operand *oper = xmalloc(sizeof(struct operand));
			oper->type = OPER_MEM;
			oper->u.mem = $1;
			$$ = oper;
		  }
		| raw_code {
			struct operand *oper = xmalloc(sizeof(struct operand));
			oper->type = OPER_RAW;
			oper->u.raw = (unsigned long)$1;
			$$ = oper;
		  }
		| imm {
			struct operand *oper = xmalloc(sizeof(struct operand));
			oper->type = OPER_IMM;
			oper->u.imm = $1;
			$$ = oper;
		  }
		| labelref {
			$$ = $1;
		  }
		;

reg		: GPR regnr {
			struct registr *reg = xmalloc(sizeof(struct registr));
			reg->type = GPR;
			reg->nr = (unsigned long)$2;
			$$ = reg;
		  }
		| SPR {
			struct registr *reg = xmalloc(sizeof(struct registr));
			reg->type = SPR;
			yytext += 3; /* skip "spr" */
			reg->nr = strtoul(yytext, NULL, 16);
			$$ = reg;
		  }
		| OFFR regnr {
			struct registr *reg = xmalloc(sizeof(struct registr));
			reg->type = OFFR;
			reg->nr = (unsigned long)$2;
			$$ = reg;
		  }
		;

mem		: BRACK_OPEN imm BRACK_CLOSE {
			struct memory *mem = xmalloc(sizeof(struct memory));
			struct immediate *offset_imm = $2;
			mem->type = MEM_DIRECT;
			mem->offset = offset_imm->imm;
			free(offset_imm);
			$$ = mem;
		  }
		| BRACK_OPEN imm COMMA OFFR regnr BRACK_CLOSE {
			struct memory *mem = xmalloc(sizeof(struct memory));
			struct immediate *offset_imm = $2;
			mem->type = MEM_INDIRECT;
			mem->offset = offset_imm->imm;
			free(offset_imm);
			mem->offr_nr = (unsigned long)$5;
			$$ = mem;
		  }
		;

imm		: imm_value {
			struct immediate *imm = xmalloc(sizeof(struct immediate));
			imm->imm = (unsigned long)$1;
			$$ = imm;
		  }
		;

imm_value	: hexnum_decnum {
			$$ = $1;
		  }
		| hexnum_decnum imm_oper imm_value {
			unsigned long a = (unsigned long)$1;
			unsigned long b = (unsigned long)$3;
			unsigned long operation = (unsigned long)$2;
			unsigned long res = 31337;
			switch (operation) {
			case PLUS:
				res = a + b;
				break;
			case MINUS:
				res = a - b;
				break;
			case MULTIPLY:
				res = a * b;
				break;
			case DIVIDE:
				res = a / b;
				break;
			case BITW_OR:
				res = a | b;
				break;
			case BITW_AND:
				res = a & b;
				break;
			case BITW_XOR:
				res = a ^ b;
				break;
			case LEFTSHIFT:
				res = a << b;
				break;
			case RIGHTSHIFT:
				res = a >> b;
				break;
			default:
				yyerror("Internal parser BUG. imm oper unknown");
			}
			$$ = (void *)res;
		  }
		| PAREN_OPEN imm_value PAREN_CLOSE {
			$$ = $2;
		  }
		| PAREN_OPEN imm_value PAREN_CLOSE imm_oper imm_value {
			unsigned long a = (unsigned long)$2;
			unsigned long b = (unsigned long)$5;
			unsigned long operation = (unsigned long)$4;
			unsigned long res = 31337;
			switch (operation) {
			case PLUS:
				res = a + b;
				break;
			case MINUS:
				res = a - b;
				break;
			case MULTIPLY:
				res = a * b;
				break;
			case DIVIDE:
				res = a / b;
				break;
			case BITW_OR:
				res = a | b;
				break;
			case BITW_AND:
				res = a & b;
				break;
			case BITW_XOR:
				res = a ^ b;
				break;
			case LEFTSHIFT:
				res = a << b;
				break;
			case RIGHTSHIFT:
				res = a >> b;
				break;
			default:
				yyerror("Internal parser BUG. complex_imm oper unknown");
			}
			$$ = (void *)res;
		  }
		| BITW_NOT imm_value {
			unsigned long n = (unsigned long)$2;
			n = ~n;
			$$ = (void *)n;
		  }
		| asm_assert {
			// Inline assertion. Always return zero
			$$ = (void *)(unsigned long)(unsigned int)0;
		  }
		;

imm_oper : PLUS {
			$$ = (void *)(unsigned long)PLUS;
		  }
		| MINUS {
			$$ = (void *)(unsigned long)MINUS;
		  }
		| MULTIPLY {
			$$ = (void *)(unsigned long)MULTIPLY;
		  }
		| DIVIDE {
			$$ = (void *)(unsigned long)DIVIDE;
		  }
		| BITW_OR {
			$$ = (void *)(unsigned long)BITW_OR;
		  }
		| BITW_AND {
			$$ = (void *)(unsigned long)BITW_AND;
		  }
		| BITW_XOR {
			$$ = (void *)(unsigned long)BITW_XOR;
		  }
		| LEFTSHIFT {
			$$ = (void *)(unsigned long)LEFTSHIFT;
		  }
		| RIGHTSHIFT {
			$$ = (void *)(unsigned long)RIGHTSHIFT;
		  }
		;

hexnum		: HEXNUM {
			while (yytext[0] != 'x') {
				if (yytext[0] == '\0')
					yyerror("Internal HEXNUM parser error");
				yytext++;
			}
			yytext++;
			$$ = (void *)(unsigned long)strtoul(yytext, NULL, 16);
		  }
		;

decnum		: DECNUM {
			$$ = (void *)(unsigned long)strtol(yytext, NULL, 10);
		  }
		;

hexnum_decnum	: hexnum {
			$$ = $1;
		  }
		| decnum {
			$$ = $1;
		  }
		;

labelref	: identifier {
			struct operand *oper = xmalloc(sizeof(struct operand));
			struct label *label = xmalloc(sizeof(struct label));
			label->name = $1;
			label->direction = LABELREF_ABSOLUTE;
			oper->type = OPER_LABEL;
			oper->u.label = label;
			$$ = oper;
		  }
		| identifier MINUS {
			struct operand *oper = xmalloc(sizeof(struct operand));
			struct label *label = xmalloc(sizeof(struct label));
			label->name = $1;
			label->direction = LABELREF_RELATIVE_BACK;
			oper->type = OPER_LABEL;
			oper->u.label = label;
			$$ = oper;
		  }
		| identifier PLUS {
			struct operand *oper = xmalloc(sizeof(struct operand));
			struct label *label = xmalloc(sizeof(struct label));
			label->name = $1;
			label->direction = LABELREF_RELATIVE_FORWARD;
			oper->type = OPER_LABEL;
			oper->u.label = label;
			$$ = oper;
		  }
		;

regnr		: DECNUM {
			$$ = (void *)(unsigned long)strtoul(yytext, NULL, 10);
		  }
		;

identifier	: IDENT {
			$$ = xstrdup(yytext);
		  }
		;

%%

int section = SECTION_TEXT; /* default to .text section */
struct initvals_sect *cur_initvals_sect;

void yyerror(char *str)
{
	unsigned int i;

	fprintf(stderr,
		"Parser ERROR (file \"%s\", line %u, col %u):\n",
		cur_lineinfo.file,
		cur_lineinfo.lineno,
		cur_lineinfo.column);
	fprintf(stderr, "%s\n", cur_lineinfo.linecopy);
	for (i = 0; i < cur_lineinfo.column - 1; i++)
		fprintf(stderr, " ");
	fprintf(stderr, "^\n");
	fprintf(stderr, "%s\n", str);
	exit(1);
}

static struct operand * store_oper_sanity(struct operand *oper)
{
	if (oper->type == OPER_IMM &&
	    oper->u.imm->imm != 0) {
		yyerror("Only 0x000 Immediate is allowed for "
			"Output operands");
	}
	return oper;
}

static void assembler_assertion_failed(void)
{
	yyerror("Assembler %assert failed");
}
