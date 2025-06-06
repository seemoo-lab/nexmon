/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * Warning:                                                                *
 *                                                                         *
 * Our software may damage your hardware and may void your hardware’s      *
 * warranty! You use our tools at your own risk and responsibility!        *
 *                                                                         *
 * License:                                                                *
 * Copyright (c) 2017 NexMon Team                                          *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining   *
 * a copy of this software and associated documentation files (the         *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute copies of the Software, and to permit persons to whom the    *
 * Software is furnished to do so, subject to the following conditions:    *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * Any use of the Software which results in an academic publication or     *
 * other publication which includes a bibliography must include a citation *
 * to the author's publication "M. Schulz, D. Wegemer and M. Hollick.      *
 * NexMon: A Cookbook for Firmware Modifications on Smartphones to Enable  *
 * Monitor Mode.".                                                         *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 *                                                                         *                                                       *
 **************************************************************************/

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <debug.h>              // contains macros to access the debug hardware

// functions implemented in debugger.c
extern void handle_pref_abort_exception(struct trace *trace);
extern void set_debug_registers(void);

/**
 *  Replaces the SP and LR registers on the stack with those of the system mode.
 *  As we decided to stay in abort mode to handle breakpoints and watchpoints, we need to 
 *  change to system mode to fetch the correct sp and lr registers to store the correct state
 *  on the stack. The required state is the one before the prefetch/data abort exception.
 */
struct trace * __attribute__((optimize("O0")))
fix_sp_lr(struct trace *trace)
{
    register unsigned int sp_sys asm("r1");
    register unsigned int lr_sys asm("r2");

    dbg_disable_monitor_mode_debugging();
    dbg_change_processor_mode(DBG_PROCESSOR_MODE_SYS);
    asm("mov %[result], sp" : [result] "=r" (sp_sys));
    asm("mov %[result], lr" : [result] "=r" (lr_sys));
    dbg_change_processor_mode(DBG_PROCESSOR_MODE_ABT);
    dbg_enable_monitor_mode_debugging();

    trace->lr = lr_sys;
    trace->sp = sp_sys;

    return trace;
}

// 1 KiBi for stack in abort mode
uint32 stack_abt[256] = { 0x54424153 };

/**
 *  Sets the stack pointer of the abort mode to be at the end of the current stack area, a part that is likely not used during regular program execution
 */
void __attribute__((optimize("O0")))
set_abort_stack_pointer(void)
{
    register unsigned int sp_abt asm("r0") = (uint32) &stack_abt[255];

    dbg_change_processor_mode(DBG_PROCESSOR_MODE_ABT);
    asm("mov sp, %[value]" : : [value] "r" (sp_abt));
    dbg_change_processor_mode(DBG_PROCESSOR_MODE_SYS);
}

/**
 *  is called before the wlc_ucode_download function to print our hello world message
 */
//__attribute__((at(0x180020, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
__attribute__((naked)) void
c_main_hook(void)
{
    asm(
        "push {r0-r3,lr}\n"                         // Push the registers that could be modified by a call to a C function
        "bl set_debug_registers\n"                  // Call a C function
        "pop {r0-r3,lr}\n"                          // Pop the registers that were saved before
        "b c_main\n"                                // Call the hooked function
        );
}

__attribute__((at(0x199E10, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
GenericPatch4(c_main_hook, c_main_hook + 1);

/**
 *  Exception handler that is triggered on reset/startup/powerup 
 */
/*
__attribute__((naked)) void
tr_reset_hook(void)
{
    asm(
        "mrs r0, cpsr\n"
        "mov r1, #0x1f\n"
        "bic r0, r0, r1\n"
        "mov r1, #0xdf\n"
        "orr r0, r0, r1\n"
        "msr cpsr_fc, r0\n"
        "b setup\n"
        );
}
*/

/**
 *  Exception handler that is triggered by watchpoint or illegal address access 
 */
__attribute__((at(0x199B7A, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
__attribute__((naked)) void
tr_data_abort_hook(void)
{
    asm(
        "sub lr, lr, #8\n"                                  // we directly subtract 4 from the link register (original code overwrites the stack pointer first)
        "srsdb sp!, #0x17\n"                                // stores return state (LR and SPSR) to the stack of the abort mode (0x17) (original codes switches to system mode (0x1F))
        "push {r0}\n"
        "push {lr}\n"                                       // here we do not get the expected link register from the system mode, but the link register from the abort mode
        "sub sp, sp, #24\n"
        "push {r0-r7}\n"
        "eor r0, r0, r0\n"
        "add r0, r0, #4\n"
        "b handle_exceptions\n"
        );
}

/**
 *  Exception handler that is triggered by breakpoint 
 */
__attribute__((at(0x199B58, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
__attribute__((naked)) void
tr_pref_abort_hook(void)
{
    asm(
        "sub lr, lr, #4\n"                                  // we directly subtract 4 from the link register (original code overwrites the stack pointer first)
        "srsdb sp!, #0x17\n"                                // stores return state (LR and SPSR) to the stack of the abort mode (0x17) (original codes switches to system mode (0x1F))
        "push {r0}\n"
        "push {lr}\n"                                       // here we do not get the expected link register from the system mode, but the link register from the abort mode
        "sub sp, sp, #24\n"
        "push {r0-r7}\n"
        "eor r0, r0, r0\n"
        "add r0, r0, #3\n"
        "b handle_exceptions\n"
        );
}

__attribute__((naked)) void
choose_exception_handler(void)
{
    asm(
            "cmp r0, #3\n"
            "beq label_pref_abort\n"
            "cmp r0, #4\n"
            "beq label_data_abort\n"
        "label_continue_exception:\n"
            "cmp r0, #6\n"                                  // check if fast interrupt
            "bne label_other_exception\n"                   // if interrupt was not a fast interrupt
            "cmp r1, #64\n"                                 // if FIQ was enabled, enable it again
            "beq label_other_exception\n"
            "cpsie f\n"
        "label_other_exception:\n"
            "mov r0, sp\n"
            "push {lr}\n"
            "pop {lr}\n"
            "b dump_stack_print_dbg_stuff_intr_handler\n"
        "label_pref_abort:\n"
            "mov r0, sp\n"
            "push {lr}\n"
            "pop {lr}\n"
            "b handle_pref_abort_exception\n"
        "label_data_abort:\n"
            "mrc p15, 0, r0, c5, c0, 0\n"                   // read DFSR
            "and r0, r0, #0x1F\n"                           // select FS bits from DFSR
            "cmp r0, #2\n"                                  // compare FS to "debug event"
            "bne label_continue_exception\n"                // if data abort not caused by a "debug event", go to regular exception handler
            "mov r0, sp\n"
            "b handle_data_abort_exception\n"
        );
}

/**
 *
 */
__attribute__((at(0x199C02, "", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
__attribute__((naked)) void
handle_exceptions(void)
{
    asm(
        "mov r4, sp\n"
        "add r4, r4, #64\n"
        "ldmia r4!, {r1,r3}\n"
        "mrs r2, cpsr\n"
        "push {r0-r3}\n"
        "sub r4, r4, #12\n"
        "str r1, [r4]\n"
        "and r1, r3, #64\n"             // r3 is CPSR_SYS, bit 6 is the FIQ mask bit
        "mov r7, sp\n"
        "add r7, r7, #88\n"
        "mov r6, r12\n"
        "mov r5, r11\n"
        "mov r4, r10\n"
        "mov r3, r9\n"
        "mov r2, r8\n"
        "add sp, sp, #72\n"
        "push {r2-r7}\n"
        /*
         * Now the stack looks like this:
         * SPSR -> CPSR_SYS
         * LR_ABT -> PC_SYS
         * PC_SYS
         * LR_ABT call fix_sp_lr to change this to LR_SYS
         * SP_ABT call fix_sp_lr to change this to SP_SYS
         * R12
         * R11
         * R10
         * R9
         * R8
         * R7
         * R6
         * R5
         * R4
         * R3
         * R2
         * R1
         * R0
         * CPSR_SYS
         * CPSR_ABT
         * PC_SYS
         * Exception ID
         */
        "sub sp, sp, #48\n"
        "bl choose_exception_handler\n"
        "cpsid if\n"
        "add sp, sp, #48\n"
        "pop {r0-r6}\n"
        "mov r8, r0\n"
        "mov r9, r1\n"
        "mov r10, r2\n"
        "mov r11, r3\n"
        "mov r12, r4\n"
        "mov lr, r6\n"
        "sub sp, sp, #60\n"
        "pop {r0-r7}\n"
        "add sp, sp, #32\n"
        "rfefd sp!\n"
        );
}

// The following patch, that is necessary to access debug registers in an interrupt handler breaks normal wifi operation
// write nops over the si_update_chipcontrol_shm(sii, 5, 16, 0) call in si_setup_cores
//__attribute__((at(0x16692, "flashpatch", CHIP_VER_BCM43455c0, FW_VER_7_45_189)))
//GenericPatch4(activate_debugger, 0xBF00BF00);

