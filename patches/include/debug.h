#ifndef DEBUG_H
#define DEBUG_H

struct trace  {
	unsigned int exception_id;
	unsigned int PC;
	unsigned int CPSR;
	unsigned int SPSR;
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int sp;
	unsigned int lr;
	unsigned int pc;
} __attribute__((packed));

/* the number of available watch- and breakpoints */
#define DBG_NUMBER_OF_WATCHPOINTS 4
#define DBG_NUMBER_OF_BREAKPOINTS 4

#define SET_DBG_VALUE(name, value) (((value) & name ## _MASK) << name ## _SHIFT)
#define GET_DBG_VALUE(name, reg) (((reg) >> name ## _SHIFT) & name ## _MASK)
#define GET_DBG_MASK(name) (name ## _MASK << name ## _SHIFT)
#define GET_DBG_INV_MASK(name) ~GET_DBG_MASK(name)
#define UPDATE_DBG_REG(reg, mask, value) (((reg) & ~(mask)) | (value))

/* The address where the memory mapped debug registers are located */
#define DBGBASE (0x18007000)

/* C11.11.20 DBGDSCR - Debug Status and Control Register */
#define DBGDSCR (*(volatile int *) (DBGBASE + 0x88))

// MASK masks the bits before shifting left or after shifting right
/* DBGDTRRX register full (read only) */
#define DBGDSCR_RXfull_SHIFT 30	
#define DBGDSCR_RXfull_MASK 1
#define DBGDSCR_RXfull_EMPTY 0
#define DBGDSCR_RXfull_FULL 1

/* DBGDTRTX register full (read only) */
#define DBGDSCR_TXfull_SHIFT 29
#define DBGDSCR_TXfull_MASK 1
#define DBGDSCR_TXfull_EMPTY 0
#define DBGDSCR_TXfull_FULL 1

/* Latched RXfull (read only) */
#define DBGDSCR_RXfull_I_SHIFT 27
#define DBGDSCR_RXfull_I_MASK 1			

/* Latched TXfull (read only) */
#define DBGDSCR_TXfull_I_SHIFT 26
#define DBGDSCR_TXfull_I_MASK 1			

/* Sticky Pipeline Advance bit (read only) */
#define DBGDSCR_PipeAdv_SHIFT 25
#define DBGDSCR_PipeAdv_MASK 1			

/* Latched Instruction Complete (read only) */
#define DBGDSCR_InstrCompl_I_SHIFT 24
#define DBGDSCR_InstrCompl_I_MASK 1
#define DBGDSCR_InstrCompl_I_NOT_COMPLETED 0
#define DBGDSCR_InstrCompl_I_COMPLETED 1

/* External DCC access mode */
#define DBGDSCR_ExtDCCmode_SHIFT 20
#define DBGDSCR_ExtDCCmode_MASK 3			
#define DBGDSCR_ExtDCCmode_NON_BLOCKING_MODE 0x0
#define DBGDSCR_ExtDCCmode_STALL_MODE 0x1
#define DBGDSCR_ExtDCCmode_FAST_MODE 0x2

/* Asynchronous Aborts Discarded */
#define DBGDSCR_ADAdiscard_SHIFT 19
#define DBGDSCR_ADAdiscard_MASK 1

/* Non-secure state status */
#define DBGDSCR_NS_SHIFT 18
#define DBGDSCR_NS_MASK 1

/* Secure PL1 Non_invasive Debug Disabled */
#define DBGDSCR_SPNIDdis_SHIFT 17
#define DBGDSCR_SPNIDdis_MASK 1
#define DBGDSCR_SPNIDdis_PERMITTED 0
#define DBGDSCR_SPNIDdis_NOT_PERMITTED 1

/* Secure PL1 Invasive Debug Disabled bit */
#define DBGDSCR_SPIDdis_SHIFT 16
#define DBGDSCR_SPIDdis_MASK 1
#define DBGDSCR_SPIDdis_PERMITTED 0
#define DBGDSCR_SPIDdis_NOT_PERMITTED 1

/* Monitor debug-mode enable */
#define DBGDSCR_MDBGen_SHIFT 15
#define DBGDSCR_MDBGen_MASK 1
#define DBGDSCR_MDBGen_DISABLED 0
#define DBGDSCR_MDBGen_ENABLED 1

/* Halting debug-mode enable */
#define DBGDSCR_HDBGen_SHIFT 14
#define DBGDSCR_HDBGen_MASK 1
#define DBGDSCR_HDBGen_DISABLED 0
#define DBGDSCR_HDBGen_ENABLED 1

/* Execute ARM instruction enable */
#define DBGDSCR_ITRen_SHIFT 13
#define DBGDSCR_ITRen_MASK 1
#define DBGDSCR_ITRen_DISABLED 0
#define DBGDSCR_ITRen_ENABLED 1

/* User mode access to Debug Communications Channel (DCC) disabled */
#define DBGDSCR_UDCCdis_SHIFT 12
#define DBGDSCR_UDCCdis_MASK 1
#define DBGDSCR_UDCCdis_ENABLED 0
#define DBGDSCR_UDCCdis_DISABLED 1

/* Interrumpts Disable */
#define DBGDSCR_INTdis_SHIFT 11
#define DBGDSCR_INTdis_MASK 1
#define DBGDSCR_INTdis_ENABLED 0
#define DBGDSCR_INTdis_DISABLED 1

/* Force Debug Acknowledge */
#define DBGDSCR_DBGack_SHIFT 10
#define DBGDSCR_DBGack_MASK 1

/* Fault status */
#define DBGDSCR_FS_SHIFT 9
#define DBGDSCR_FS_MASK 1

/* Sticky Undefined Instruction */
#define DBGDSCR_UND_I_SHIFT 8
#define DBGDSCR_UND_I_MASK 1

/* Sticky Asynchronous Abort */
#define DBGDSCR_ADABORT_I_SHIFT 7
#define DBGDSCR_ADABORT_I_MASK 1

/* Sticky Synchronous Data Abort */
#define DBGDSCR_SDABORT_I_SHIFT 6
#define DBGDSCR_SDABORT_I_MASK 1

/* Method of Debug entry */
#define DBGDSCR_MOE_SHIFT 2
#define DBGDSCR_MOE_MASK 0xF

/* Processor Restarted */
#define DBGDSCR_RESTARTED_SHIFT 1
#define DBGDSCR_RESTARTED_MASK 1

/* Processor Halted */
#define DBGDSCR_HALTED_SHIFT 0
#define DBGDSCR_HALTED_MASK 1
#define DBGDSCR_HALTED_NON_DEBUG_STATE 0
#define DBGDSCR_HALTED_DEBUG_STATE 1

/* C11.11.3 DBGBVR - Breakpoint Value Registers */
#define DBGBVR0 (*(volatile int *) (DBGBASE + 0x100))
#define DBGBVR1 (*(volatile int *) (DBGBASE + 0x104))
#define DBGBVR2 (*(volatile int *) (DBGBASE + 0x108))
#define DBGBVR3 (*(volatile int *) (DBGBASE + 0x10C))
#define DBGBVR4 (*(volatile int *) (DBGBASE + 0x110))
#define DBGBVR5 (*(volatile int *) (DBGBASE + 0x114))
#define DBGBVR6 (*(volatile int *) (DBGBASE + 0x118))
#define DBGBVR7 (*(volatile int *) (DBGBASE + 0x11C))

/* The last two bits of an instruction address needs to be set to 0 */
#define DBGBVR_ADDRMASK (0xFFFFFFFC)

/* C11.11.2 DBGBCR - Breakpoint Control Registers */
#define DBGBCR0 (*(volatile int *) (DBGBASE + 0x140))
#define DBGBCR1 (*(volatile int *) (DBGBASE + 0x144))
#define DBGBCR2 (*(volatile int *) (DBGBASE + 0x148))
#define DBGBCR3 (*(volatile int *) (DBGBASE + 0x14C))
#define DBGBCR4 (*(volatile int *) (DBGBASE + 0x150))
#define DBGBCR5 (*(volatile int *) (DBGBASE + 0x154))
#define DBGBCR6 (*(volatile int *) (DBGBASE + 0x158))
#define DBGBCR7 (*(volatile int *) (DBGBASE + 0x15C))

/* Address range mask */
#define DBGBCR_MASK_SHIFT 24
#define DBGBCR_MASK_MASK 0x1F
#define DBGBCR_MASK_NO_MASK  0
#define DBGBCR_MASK_00000007 3
#define DBGBCR_MASK_0000000F 4
#define DBGBCR_MASK_0000001F 5
#define DBGBCR_MASK_0000003F 6
#define DBGBCR_MASK_0000007F 7
#define DBGBCR_MASK_000000FF 8
#define DBGBCR_MASK_000001FF 9
#define DBGBCR_MASK_000003FF 10
#define DBGBCR_MASK_000007FF 11
#define DBGBCR_MASK_00000FFF 12
#define DBGBCR_MASK_00001FFF 13
#define DBGBCR_MASK_00003FFF 14
#define DBGBCR_MASK_00007FFF 15
#define DBGBCR_MASK_0000FFFF 16
#define DBGBCR_MASK_0001FFFF 17
#define DBGBCR_MASK_0003FFFF 18
#define DBGBCR_MASK_0007FFFF 19
#define DBGBCR_MASK_000FFFFF 20
#define DBGBCR_MASK_001FFFFF 21
#define DBGBCR_MASK_003FFFFF 22
#define DBGBCR_MASK_007FFFFF 23
#define DBGBCR_MASK_00FFFFFF 24
#define DBGBCR_MASK_01FFFFFF 25
#define DBGBCR_MASK_03FFFFFF 26
#define DBGBCR_MASK_07FFFFFF 27
#define DBGBCR_MASK_0FFFFFFF 28
#define DBGBCR_MASK_1FFFFFFF 29
#define DBGBCR_MASK_3FFFFFFF 30
#define DBGBCR_MASK_7FFFFFFF 31

/* Breakpoint type */
#define DBGBCR_BT_SHIFT 20
#define DBGBCR_BT_MASK 0xF
#define DBGBCR_BT_UNLINKED_INSTR_ADDR_MATCH 0
#define DBGBCR_BT_LINKED_INSTR_ADDR_MATCH 1
#define DBGBCR_BT_UNLINKED_CONTEXT_ID_MATCH 2
#define DBGBCR_BT_LINKED_CONTEXT_ID_MATCH 3
#define DBGBCR_BT_UNLINKED_INSTR_ADDR_MISMATCH 4
#define DBGBCR_BT_LINKED_INSTR_ADDR_MISMATCH 5

/* Linked breakpoint number */
#define DBGBCR_LBN_SHIFT 16
#define DBGBCR_LBN_MASK 0xF

/* Security state control */
#define DBGBCR_SSC_SHIFT 14
#define DBGBCR_SSC_MASK 3

/* Hyp mode control bit */
#define DBGBCR_HMC_SHIFT 13
#define DBGBCR_HMC_MASK 1

/* Byte address select */
#define DBGBCR_BAS_SHIFT 5
#define DBGBCR_BAS_MASK 0xF

#define GET_BAS_FOR_THUMB_ADDR(addr) (3 << (addr & 2))

/* Privileged mode control */
#define DBGBCR_PMC_SHIFT 1
#define DBGBCR_PMC_MASK 3

/* combined options for SSC, HMC and PMC */
/* secure and non secure modes */
#define DBGBCR_SSC_HMC_PMC__PL0_SUP_SYS						(SET_DBG_VALUE(DBGBCR_SSC, 0) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 0))
#define DBGBCR_SSC_HMC_PMC__PL1								(SET_DBG_VALUE(DBGBCR_SSC, 0) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 1))
#define DBGBCR_SSC_HMC_PMC__PL0								(SET_DBG_VALUE(DBGBCR_SSC, 0) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 2))
#define DBGBCR_SSC_HMC_PMC__SEC_ALL__NON_SEC_PL1_PL0		(SET_DBG_VALUE(DBGBCR_SSC, 0) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 3))
#define DBGBCR_SSC_HMC_PMC__SEC_PL1__NON_SEC_PL2_PL1		(SET_DBG_VALUE(DBGBCR_SSC, 0) | SET_DBG_VALUE(DBGBCR_HMC, 1) | SET_DBG_VALUE(DBGBCR_PMC, 1))
#define DBGBCR_SSC_HMC_PMC__ALL								(SET_DBG_VALUE(DBGBCR_SSC, 0) | SET_DBG_VALUE(DBGBCR_HMC, 1) | SET_DBG_VALUE(DBGBCR_PMC, 3))
/* only non-secure modes */
#define DBGBCR_SSC_HMC_PMC__NON_SEC_PL0_SUP_SYS				(SET_DBG_VALUE(DBGBCR_SSC, 1) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 0))
#define DBGBCR_SSC_HMC_PMC__NON_SEC_PL2						(SET_DBG_VALUE(DBGBCR_SSC, 3) | SET_DBG_VALUE(DBGBCR_HMC, 1) | SET_DBG_VALUE(DBGBCR_PMC, 0))
#define DBGBCR_SSC_HMC_PMC__NON_SEC_PL1						(SET_DBG_VALUE(DBGBCR_SSC, 1) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 1))
#define DBGBCR_SSC_HMC_PMC__NON_SEC_PL0						(SET_DBG_VALUE(DBGBCR_SSC, 1) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 2))
#define DBGBCR_SSC_HMC_PMC__NON_SEC_PL1_PL0					(SET_DBG_VALUE(DBGBCR_SSC, 1) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 3))
#define DBGBCR_SSC_HMC_PMC__NON_SEC_PL2_PL1					(SET_DBG_VALUE(DBGBCR_SSC, 1) | SET_DBG_VALUE(DBGBCR_HMC, 1) | SET_DBG_VALUE(DBGBCR_PMC, 1))
#define DBGBCR_SSC_HMC_PMC__NON_SEC_ALL						(SET_DBG_VALUE(DBGBCR_SSC, 1) | SET_DBG_VALUE(DBGBCR_HMC, 1) | SET_DBG_VALUE(DBGBCR_PMC, 3))
/* only secure modes */
#define DBGBCR_SSC_HMC_PMC__SEC_PL0_SUP_SYS					(SET_DBG_VALUE(DBGBCR_SSC, 2) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 0))
#define DBGBCR_SSC_HMC_PMC__SEC_PL1							(SET_DBG_VALUE(DBGBCR_SSC, 2) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 1))
#define DBGBCR_SSC_HMC_PMC__SEC_PL0							(SET_DBG_VALUE(DBGBCR_SSC, 2) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 2))
#define DBGBCR_SSC_HMC_PMC__SEC_ALL							(SET_DBG_VALUE(DBGBCR_SSC, 2) | SET_DBG_VALUE(DBGBCR_HMC, 0) | SET_DBG_VALUE(DBGBCR_PMC, 3))

#define DBGBCR_SSC_HMC_PMC_SHIFT 0
#define DBGBCR_SSC_HMC_PMC_MASK (GET_DBG_MASK(DBGBCR_SSC) | GET_DBG_MASK(DBGBCR_HMC) | GET_DBG_MASK(DBGBCR_PMC))

/* Breakpoint enable */
#define DBGBCR_E_SHIFT 0
#define DBGBCR_E_MASK 1
#define DBGBCR_E_DISABLED 0
#define DBGBCR_E_ENABLED 1

/* DBGLAR - Lock Access Register */
#define DBGLAR (*(volatile int *) (DBGBASE + 0xFB0))
#define DBGLAR_UNLOCK_CODE (0xC5ACCE55)

/* Breakpoint numbers one-hot encoded */
#define DBGBP0 (1 << 0)
#define DBGBP1 (1 << 1)
#define DBGBP2 (1 << 2)
#define DBGBP3 (1 << 3)

/* Watch numbers one-hot encoded */
#define DBGWP0 (1 << 0)
#define DBGWP1 (1 << 1)
#define DBGWP2 (1 << 2)
#define DBGWP3 (1 << 3)

/* C11.11.46 DBGWVR - Watchpoint Value Registers */
#define DBGWVR0 (*(volatile int *) (DBGBASE + 0x180))
#define DBGWVR1 (*(volatile int *) (DBGBASE + 0x184))
#define DBGWVR2 (*(volatile int *) (DBGBASE + 0x188))
#define DBGWVR3 (*(volatile int *) (DBGBASE + 0x18C))
#define DBGWVR4 (*(volatile int *) (DBGBASE + 0x190))
#define DBGWVR5 (*(volatile int *) (DBGBASE + 0x194))
#define DBGWVR6 (*(volatile int *) (DBGBASE + 0x198))
#define DBGWVR7 (*(volatile int *) (DBGBASE + 0x19C))

/* The last two bits of an address needs to be set to 0 */
#define DBGWVR_ADDRMASK (0xFFFFFFFC)

/* C11.11.44 DBGWCR - Watchpoint Control Registers */
#define DBGWCR0 (*(volatile int *) (DBGBASE + 0x1C0))
#define DBGWCR1 (*(volatile int *) (DBGBASE + 0x1C4))
#define DBGWCR2 (*(volatile int *) (DBGBASE + 0x1C8))
#define DBGWCR3 (*(volatile int *) (DBGBASE + 0x1CC))
#define DBGWCR4 (*(volatile int *) (DBGBASE + 0x1D0))
#define DBGWCR5 (*(volatile int *) (DBGBASE + 0x1D4))
#define DBGWCR6 (*(volatile int *) (DBGBASE + 0x1D8))
#define DBGWCR7 (*(volatile int *) (DBGBASE + 0x1DC))

/* Address range mask */
#define DBGWCR_MASK_SHIFT 24
#define DBGWCR_MASK_MASK 0x1F
#define DBGWCR_MASK_NO_MASK  0
#define DBGWCR_MASK_00000007 3
#define DBGWCR_MASK_0000000F 4
#define DBGWCR_MASK_0000001F 5
#define DBGWCR_MASK_0000003F 6
#define DBGWCR_MASK_0000007F 7
#define DBGWCR_MASK_000000FF 8
#define DBGWCR_MASK_000001FF 9
#define DBGWCR_MASK_000003FF 10
#define DBGWCR_MASK_000007FF 11
#define DBGWCR_MASK_00000FFF 12
#define DBGWCR_MASK_00001FFF 13
#define DBGWCR_MASK_00003FFF 14
#define DBGWCR_MASK_00007FFF 15
#define DBGWCR_MASK_0000FFFF 16
#define DBGWCR_MASK_0001FFFF 17
#define DBGWCR_MASK_0003FFFF 18
#define DBGWCR_MASK_0007FFFF 19
#define DBGWCR_MASK_000FFFFF 20
#define DBGWCR_MASK_001FFFFF 21
#define DBGWCR_MASK_003FFFFF 22
#define DBGWCR_MASK_007FFFFF 23
#define DBGWCR_MASK_00FFFFFF 24
#define DBGWCR_MASK_01FFFFFF 25
#define DBGWCR_MASK_03FFFFFF 26
#define DBGWCR_MASK_07FFFFFF 27
#define DBGWCR_MASK_0FFFFFFF 28
#define DBGWCR_MASK_1FFFFFFF 29
#define DBGWCR_MASK_3FFFFFFF 30
#define DBGWCR_MASK_7FFFFFFF 31

/* Watchpoint type */
#define DBGWCR_WT_SHIFT 20
#define DBGWCR_WT_MASK 1
#define DBGWCR_WT_UNLINKED_DATA_ADDR_MATCH 0
#define DBGWCR_WT_LINKED_DATA_ADDR_MATCH 1

/* Linked watchpoint number */
#define DBGWCR_LBN_SHIFT 16
#define DBGWCR_LBN_MASK 0xF

/* Security state control */
#define DBGWCR_SSC_SHIFT 14
#define DBGWCR_SSC_MASK 3

/* Hyp mode control bit */
#define DBGWCR_HMC_SHIFT 13
#define DBGWCR_HMC_MASK 1

/* Byte address select */
#define DBGWCR_BAS_4BIT_SHIFT 5
#define DBGWCR_BAS_4BIT_MASK 0xF
#define DBGWCR_BAS_8BIT_SHIFT 5
#define DBGWCR_BAS_8BIT_MASK 0xFF

/* Load/store access control */
#define DBGWCR_LSC_SHIFT 3
#define DBGWCR_LSC_MASK 3
#define DBGWCR_LSC_MATCH_LOAD 1
#define DBGWCR_LSC_MATCH_STORE 2
#define DBGWCR_LSC_MATCH_ALL 3

/* Privileged access control */
#define DBGWCR_PAC_SHIFT 1
#define DBGWCR_PAC_MASK 3

/* combined options for SSC, HMC and PAC */
/* secure and non secure modes */
#define DBGWCR_SSC_HMC_PAC__PL1								(SET_DBG_VALUE(DBGWCR_SSC, 0) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 1))
#define DBGWCR_SSC_HMC_PAC__PL0								(SET_DBG_VALUE(DBGWCR_SSC, 0) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 2))
#define DBGWCR_SSC_HMC_PAC__PL1_PL0							(SET_DBG_VALUE(DBGWCR_SSC, 0) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 3))
#define DBGWCR_SSC_HMC_PAC__SEC_PL1__NON_SEC_PL2_PL1		(SET_DBG_VALUE(DBGWCR_SSC, 0) | SET_DBG_VALUE(DBGWCR_HMC, 1) | SET_DBG_VALUE(DBGWCR_PAC, 1))
#define DBGWCR_SSC_HMC_PAC__ALL								(SET_DBG_VALUE(DBGWCR_SSC, 0) | SET_DBG_VALUE(DBGWCR_HMC, 1) | SET_DBG_VALUE(DBGWCR_PAC, 3))
/* only non-secure modes */
#define DBGWCR_SSC_HMC_PAC__NON_SEC_PL2						(SET_DBG_VALUE(DBGWCR_SSC, 3) | SET_DBG_VALUE(DBGWCR_HMC, 1) | SET_DBG_VALUE(DBGWCR_PAC, 0))
#define DBGWCR_SSC_HMC_PAC__NON_SEC_PL1						(SET_DBG_VALUE(DBGWCR_SSC, 1) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 1))
#define DBGWCR_SSC_HMC_PAC__NON_SEC_PL0						(SET_DBG_VALUE(DBGWCR_SSC, 1) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 2))
#define DBGWCR_SSC_HMC_PAC__NON_SEC_PL1_PL0					(SET_DBG_VALUE(DBGWCR_SSC, 1) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 3))
#define DBGWCR_SSC_HMC_PAC__NON_SEC_PL2_PL1					(SET_DBG_VALUE(DBGWCR_SSC, 1) | SET_DBG_VALUE(DBGWCR_HMC, 1) | SET_DBG_VALUE(DBGWCR_PAC, 1))
#define DBGWCR_SSC_HMC_PAC__NON_SEC_PL2_PL1_PL0				(SET_DBG_VALUE(DBGWCR_SSC, 1) | SET_DBG_VALUE(DBGWCR_HMC, 1) | SET_DBG_VALUE(DBGWCR_PAC, 3))
/* only secure modes */
#define DBGWCR_SSC_HMC_PAC__SEC_PL1							(SET_DBG_VALUE(DBGWCR_SSC, 2) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 1))
#define DBGWCR_SSC_HMC_PAC__SEC_PL0							(SET_DBG_VALUE(DBGWCR_SSC, 2) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 2))
#define DBGWCR_SSC_HMC_PAC__SEC_PL1_PL0						(SET_DBG_VALUE(DBGWCR_SSC, 2) | SET_DBG_VALUE(DBGWCR_HMC, 0) | SET_DBG_VALUE(DBGWCR_PAC, 3))

#define DBGWCR_SSC_HMC_PAC_SHIFT 0
#define DBGWCR_SSC_HMC_PAC_MASK (GET_DBG_MASK(DBGWCR_SSC) | GET_DBG_MASK(DBGWCR_HMC) | GET_DBG_MASK(DBGWCR_PAC))

/* Watchpoint enable */
#define DBGWCR_E_SHIFT 0
#define DBGWCR_E_MASK 1
#define DBGWCR_E_DISABLED 0
#define DBGWCR_E_ENABLED 1

/* Processor modes */
#define DBG_PROCESSOR_MODE_USR 0x10
#define DBG_PROCESSOR_MODE_FIQ 0x11
#define DBG_PROCESSOR_MODE_IRQ 0x12
#define DBG_PROCESSOR_MODE_SVC 0x13
#define DBG_PROCESSOR_MODE_ABT 0x17
#define DBG_PROCESSOR_MODE_UND 0x1B
#define DBG_PROCESSOR_MODE_SYS 0x1F

/* debug helper macros */
#define dbg_expand1(x) #x
#define dbg_expand(x) dbg_expand1(x)

#define dbg_change_processor_mode(mode) do { \
		asm("cps #" dbg_expand(mode) "\n"); \
	} while (0)

#define dbg_unlock_debug_registers() do { \
		DBGLAR = DBGLAR_UNLOCK_CODE; \
	} while (0)

#define dbg_enable_monitor_mode_debugging() do { \
		DBGDSCR = UPDATE_DBG_REG(DBGDSCR, GET_DBG_MASK(DBGDSCR_MDBGen), SET_DBG_VALUE(DBGDSCR_MDBGen, DBGDSCR_MDBGen_ENABLED)); \
	} while (0)

#define dbg_disable_monitor_mode_debugging() do { \
		DBGDSCR = UPDATE_DBG_REG(DBGDSCR, GET_DBG_MASK(DBGDSCR_MDBGen), SET_DBG_VALUE(DBGDSCR_MDBGen, DBGDSCR_MDBGen_DISABLED)); \
	} while (0)

#define dbg_enable_breakpoint(number) do { \
		DBGBCR ## number = UPDATE_DBG_REG(DBGBCR ## number, GET_DBG_MASK(DBGBCR_E), SET_DBG_VALUE(DBGBCR_E, DBGBCR_E_ENABLED)); \
	} while (0)

#define dbg_disable_breakpoint(number) do { \
		DBGBCR ## number = UPDATE_DBG_REG(DBGBCR ## number, GET_DBG_MASK(DBGBCR_E), SET_DBG_VALUE(DBGBCR_E, DBGBCR_E_DISABLED)); \
	} while (0)

#define dbg_set_breakpoint_type_to_instr_addr_match(number) do { \
		DBGBCR ## number = UPDATE_DBG_REG(DBGBCR ## number, GET_DBG_MASK(DBGBCR_BT), SET_DBG_VALUE(DBGBCR_BT, DBGBCR_BT_UNLINKED_INSTR_ADDR_MATCH)); \
	} while (0)

#define dbg_set_breakpoint_type_to_instr_addr_mismatch(number) do { \
		DBGBCR ## number = UPDATE_DBG_REG(DBGBCR ## number, GET_DBG_MASK(DBGBCR_BT), SET_DBG_VALUE(DBGBCR_BT, DBGBCR_BT_UNLINKED_INSTR_ADDR_MISMATCH)); \
	} while (0)

#define dbg_is_breakpoint_enabled(number) (GET_DBG_VALUE(DBGBCR_E, DBGBCR ## number))

#define dbg_is_breakpoint_type_instr_addr_match(number) (GET_DBG_VALUE(DBGBCR_BT, DBGBCR ## number) == DBGBCR_BT_UNLINKED_INSTR_ADDR_MATCH)

#define dbg_is_breakpoint_type_instr_addr_mismatch(number) (GET_DBG_VALUE(DBGBCR_BT, DBGBCR ## number) == DBGBCR_BT_UNLINKED_INSTR_ADDR_MISMATCH)

#define dbg_triggers_on_breakpoint_address(number, address) ((DBGBVR ## number == (address & DBGBVR_ADDRMASK)) && (GET_DBG_VALUE(DBGBCR_BAS, DBGBCR ## number) == GET_BAS_FOR_THUMB_ADDR(address)))

#define dbg_set_breakpoint_for_addr_match(number, address) do { \
		DBGBCR ## number = 0x0; \
		DBGBVR ## number = (address) & DBGBVR_ADDRMASK; \
		DBGBCR ## number =  \
			SET_DBG_VALUE(DBGBCR_BT, DBGBCR_BT_UNLINKED_INSTR_ADDR_MATCH) |  \
			SET_DBG_VALUE(DBGBCR_MASK, DBGBCR_MASK_NO_MASK) |  \
			SET_DBG_VALUE(DBGBCR_E, DBGBCR_E_ENABLED) | \
			SET_DBG_VALUE(DBGBCR_SSC_HMC_PMC, DBGBCR_SSC_HMC_PMC__PL0_SUP_SYS) |  \
			SET_DBG_VALUE(DBGBCR_BAS, GET_BAS_FOR_THUMB_ADDR(address)); \
	} while (0)

#define dbg_set_breakpoint_for_addr_mismatch(number, address) do { \
		DBGBCR ## number = 0x0; \
		DBGBVR ## number = (address) & DBGBVR_ADDRMASK; \
		DBGBCR ## number =  \
			SET_DBG_VALUE(DBGBCR_BT, DBGBCR_BT_UNLINKED_INSTR_ADDR_MISMATCH) |  \
			SET_DBG_VALUE(DBGBCR_MASK, DBGBCR_MASK_NO_MASK) |  \
			SET_DBG_VALUE(DBGBCR_E, DBGBCR_E_ENABLED) | \
			SET_DBG_VALUE(DBGBCR_SSC_HMC_PMC, DBGBCR_SSC_HMC_PMC__PL0_SUP_SYS) |  \
			SET_DBG_VALUE(DBGBCR_BAS, GET_BAS_FOR_THUMB_ADDR(address)); \
	} while (0)


#define dbg_enable_watchpoint(number) do { \
		DBGWCR ## number = UPDATE_DBG_REG(DBGWCR ## number, GET_DBG_MASK(DBGWCR_E), SET_DBG_VALUE(DBGWCR_E, DBGWCR_E_ENABLED)); \
	} while (0)

#define dbg_disable_watchpoint(number) do { \
		DBGWCR ## number = UPDATE_DBG_REG(DBGWCR ## number, GET_DBG_MASK(DBGWCR_E), SET_DBG_VALUE(DBGWCR_E, DBGWCR_E_DISABLED)); \
	} while (0)

#define dbg_set_watchpoint_for_addr_match(number, address) do { \
		DBGWCR ## number = 0x0; \
		DBGWVR ## number = (address) & DBGWVR_ADDRMASK; \
		DBGWCR ## number = \
			SET_DBG_VALUE(DBGWCR_WT, DBGWCR_WT_UNLINKED_DATA_ADDR_MATCH) | \
			SET_DBG_VALUE(DBGWCR_MASK, DBGWCR_MASK_NO_MASK) | \
			SET_DBG_VALUE(DBGWCR_E, DBGWCR_E_ENABLED) | \
			SET_DBG_VALUE(DBGWCR_SSC_HMC_PAC, DBGWCR_SSC_HMC_PAC__ALL) | \
			SET_DBG_VALUE(DBGWCR_LSC, DBGWCR_LSC_MATCH_ALL) | \
			SET_DBG_VALUE(DBGWCR_BAS_4BIT, 0xF); \
	} while (0)

#define dbg_set_watchpoint_for_addr_match_with_mask(number, address, mask) do { \
		DBGWCR ## number = 0x0; \
		DBGWVR ## number = (address) & DBGWVR_ADDRMASK; \
		DBGWCR ## number = \
			SET_DBG_VALUE(DBGWCR_WT, DBGWCR_WT_UNLINKED_DATA_ADDR_MATCH) | \
			SET_DBG_VALUE(DBGWCR_MASK, mask) | \
			SET_DBG_VALUE(DBGWCR_E, DBGWCR_E_ENABLED) | \
			SET_DBG_VALUE(DBGWCR_SSC_HMC_PAC, DBGWCR_SSC_HMC_PAC__ALL) | \
			SET_DBG_VALUE(DBGWCR_LSC, DBGWCR_LSC_MATCH_ALL) | \
			SET_DBG_VALUE(DBGWCR_BAS_4BIT, 0xF); \
	} while (0)


#endif /* DEBUG_H */