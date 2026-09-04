/* asm-common-aarch64.h  -  Common macros for AArch64 assembly
 *
 * Copyright (C) 2018 Martin Storsjö <martin@martin.st>
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GCRY_ASM_COMMON_AARCH64_H
#define GCRY_ASM_COMMON_AARCH64_H

#include <config.h>

#ifdef HAVE_GCC_ASM_ELF_DIRECTIVES
# define ELF(...) __VA_ARGS__
#else
# define ELF(...) /*_*/
#endif

#ifdef _WIN32
# define SECTION_RODATA .section .rdata
#else
# define SECTION_RODATA .section .rodata
#endif

#ifdef __APPLE__
#define GET_DATA_POINTER(reg, name) \
	adrp    reg, name@GOTPAGE ; \
	add     reg, reg, name@GOTPAGEOFF ;
#else
#define GET_DATA_POINTER(reg, name) \
	adrp    reg, name ; \
	add     reg, reg, #:lo12:name ;
#endif

#if defined(__ARM_FEATURE_BTI_DEFAULT) && __ARM_FEATURE_BTI_DEFAULT == 1
# define AARCH64_BTI_PROPERTY_FLAG (1 << 0)
# define AARCH64_HINT_BTI_C \
	hint #34
#else
# define AARCH64_BTI_PROPERTY_FLAG 0 /* No BTI */
# define AARCH64_HINT_BTI_C /*_*/
#endif

#if defined(__ARM_FEATURE_PAC_DEFAULT) && (__ARM_FEATURE_PAC_DEFAULT & 3) != 0
/* PAC enabled, signed with either A or B key. */
# define AARCH64_PAC_PROPERTY_FLAG (1 << 1)
#else
# define AARCH64_PAC_PROPERTY_FLAG 0 /* No PAC */
#endif

/* straight-line speculation mitigation */
#define SPEC_STOP dsb sy; isb

#ifdef HAVE_GCC_ASM_CFI_DIRECTIVES
/* CFI directives to emit DWARF stack unwinding information. */
# define CFI_STARTPROC()            .cfi_startproc; AARCH64_HINT_BTI_C
# define CFI_ENDPROC()              SPEC_STOP; .cfi_endproc
# define CFI_REMEMBER_STATE()       .cfi_remember_state
# define CFI_RESTORE_STATE()        .cfi_restore_state
# define CFI_ADJUST_CFA_OFFSET(off) .cfi_adjust_cfa_offset off
# define CFI_REL_OFFSET(reg,off)    .cfi_rel_offset reg, off
# define CFI_DEF_CFA_REGISTER(reg)  .cfi_def_cfa_register reg
# define CFI_REGISTER(ro,rn)        .cfi_register ro, rn
# define CFI_RESTORE(reg)           .cfi_restore reg

/* CFA expressions are used for pointing CFA and registers to
 * SP relative offsets. */
# define DW_REGNO_SP 31

/* Fixed length encoding used for integers for now. */
# define DW_SLEB128_7BIT(value) \
	0x00|((value) & 0x7f)
# define DW_SLEB128_28BIT(value) \
	0x80|((value)&0x7f), \
	0x80|(((value)>>7)&0x7f), \
	0x80|(((value)>>14)&0x7f), \
	0x00|(((value)>>21)&0x7f)

# define CFI_CFA_ON_STACK(rsp_offs,cfa_depth) \
	.cfi_escape \
	  0x0f, /* DW_CFA_def_cfa_expression */ \
	    DW_SLEB128_7BIT(11), /* length */ \
	  0x8f, /* DW_OP_breg31, rsp + constant */ \
	    DW_SLEB128_28BIT(rsp_offs), \
	  0x06, /* DW_OP_deref */ \
	  0x23, /* DW_OP_plus_constu */ \
	    DW_SLEB128_28BIT((cfa_depth)+8)

# define CFI_REG_ON_STACK(regno,rsp_offs) \
	.cfi_escape \
	  0x10, /* DW_CFA_expression */ \
	    DW_SLEB128_7BIT(regno), \
	    DW_SLEB128_7BIT(5), /* length */ \
	  0x8f, /* DW_OP_breg31, rsp + constant */ \
	    DW_SLEB128_28BIT(rsp_offs)

#else
# define CFI_STARTPROC() AARCH64_HINT_BTI_C
# define CFI_ENDPROC() SPEC_STOP
# define CFI_REMEMBER_STATE()
# define CFI_RESTORE_STATE()
# define CFI_ADJUST_CFA_OFFSET(off)
# define CFI_REL_OFFSET(reg,off)
# define CFI_DEF_CFA_REGISTER(reg)
# define CFI_REGISTER(ro,rn)
# define CFI_RESTORE(reg)

# define CFI_CFA_ON_STACK(rsp_offs,cfa_depth)
# define CFI_REG_ON_STACK(reg,rsp_offs)
#endif

/* 'ret' instruction replacement for straight-line speculation mitigation */
#define ret_spec_stop \
	ret; SPEC_STOP;

#define CLEAR_REG(reg) movi reg.16b, #0;

#define CLEAR_ALL_REGS() \
	CLEAR_REG(v0); CLEAR_REG(v1); CLEAR_REG(v2); CLEAR_REG(v3); \
	CLEAR_REG(v4); CLEAR_REG(v5); CLEAR_REG(v6); \
	/* v8-v15 are ABI callee saved. */ \
	CLEAR_REG(v16); CLEAR_REG(v17); CLEAR_REG(v18); CLEAR_REG(v19); \
	CLEAR_REG(v20); CLEAR_REG(v21); CLEAR_REG(v22); CLEAR_REG(v23); \
	CLEAR_REG(v24); CLEAR_REG(v25); CLEAR_REG(v26); CLEAR_REG(v27); \
	CLEAR_REG(v28); CLEAR_REG(v29); CLEAR_REG(v30); CLEAR_REG(v31);

#define VPUSH_ABI \
	stp d8, d9, [sp, #-16]!; \
	CFI_ADJUST_CFA_OFFSET(16); \
	stp d10, d11, [sp, #-16]!; \
	CFI_ADJUST_CFA_OFFSET(16); \
	stp d12, d13, [sp, #-16]!; \
	CFI_ADJUST_CFA_OFFSET(16); \
	stp d14, d15, [sp, #-16]!; \
	CFI_ADJUST_CFA_OFFSET(16);

#define VPOP_ABI \
	ldp d14, d15, [sp], #16; \
	CFI_ADJUST_CFA_OFFSET(-16); \
	ldp d12, d13, [sp], #16; \
	CFI_ADJUST_CFA_OFFSET(-16); \
	ldp d10, d11, [sp], #16; \
	CFI_ADJUST_CFA_OFFSET(-16); \
	ldp d8, d9, [sp], #16; \
	CFI_ADJUST_CFA_OFFSET(-16);

#if (AARCH64_BTI_PROPERTY_FLAG | AARCH64_PAC_PROPERTY_FLAG)
/* Generate PAC/BTI property for all assembly files including this header.
 *
 * libgcrypt support these extensions:
 *  - Armv8.3-A Pointer Authentication (PAC):
 *    As currently all AArch64 assembly functions are leaf functions and do
 *    not store/load link register LR, we just mark PAC as supported.
 *
 *  - Armv8.5-A Branch Target Identification (BTI):
 *    All AArch64 assembly functions get branch target instruction through
 *    CFI_STARTPROC macro.
 */
ELF(.section .note.gnu.property,"a")
ELF(.balign 8)
ELF(.long 1f - 0f)
ELF(.long 4f - 1f)
ELF(.long 5)
ELF(0:)
ELF(.byte 0x47, 0x4e, 0x55, 0) /* string "GNU" */
ELF(1:)
ELF(.balign 8)
ELF(.long 0xc0000000)
ELF(.long 3f - 2f)
ELF(2:)
ELF(.long (AARCH64_BTI_PROPERTY_FLAG | AARCH64_PAC_PROPERTY_FLAG))
ELF(3:)
ELF(.balign 8)
ELF(4:)
#endif

#endif /* GCRY_ASM_COMMON_AARCH64_H */
