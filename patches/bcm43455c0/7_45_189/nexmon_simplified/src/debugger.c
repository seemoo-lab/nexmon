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

/**
 *  Replaces the SP and LR registers on the stack with those of the system mode.
 *  As we decided to stay in abort mode to handle breakpoints and watchpoints, we need to 
 *  change to system mode to fetch the correct sp and lr registers to store the correct state
 *  on the stack. The required state is the one before the prefetch/data abort exception.
 */
// function implemented in debugger_base.c
extern struct trace * fix_sp_lr(struct trace *trace);
extern void set_abort_stack_pointer(void);

/**
 *  Saves one-hot encoded which breakpoint was hit
 */
unsigned char breakpoint_hit = 0;

/**
 *  Saves one-hot encoded whether we should perform single stepping
 */
unsigned char single_stepping = 0;

/**
 *  Breakpoint step counter, saves how many steps we performed after triggering the breakpoint
 */

unsigned int breakpoint_cnt0 = 0;
unsigned int breakpoint_cnt1 = 0;
unsigned int breakpoint_cnt2 = 0;
unsigned int breakpoint_cnt3 = 0;

/**
 *  Saves one-hot encoded which watchpoint was hit
 */
unsigned char watchpoint_hit = 0;

/**
 *  Watchpoint hit counter, saves how often a watchpoint was hit
 */
unsigned char watchpoint_hit_counter = 0;

/**
 *  Watchpoint hit limit, defines how often a watchpoint should trigger
 */
unsigned char watchpoint_hit_limit = 20;

/**
 *  Watchpoint handler function
 */
void __attribute__((optimize("O0")))
handle_data_abort_exception(struct trace *trace)
{
    fix_sp_lr(trace);

    // TODO
    // Currently I do not know how to find out, which watchpoint was triggered, so here I always handle watchpoint 0
    watchpoint_hit |= DBGWP0;

    // Disable the watchpoint so that the instruction that triggered the watchpoint can be executed without triggering the watchpoint again.
    dbg_disable_watchpoint(0);
    dbg_disable_watchpoint(1);
    dbg_disable_watchpoint(2);
    dbg_disable_watchpoint(3);

    printf("   WP hit pc=%08x\n", trace->pc);

    // Set breakpoint 3 to reset the watchpoint after executing the instruction that triggered the watchpoint.
    dbg_set_breakpoint_for_addr_mismatch(3, trace->PC);
}

/**
 *  Breakpoint handler function
 */
void __attribute__((optimize("O0")))
handle_pref_abort_exception(struct trace *trace)
{
    fix_sp_lr(trace);

    // check whether breakpoint 0 is enabled
    if(dbg_is_breakpoint_enabled(0)) {
        if (dbg_triggers_on_breakpoint_address(0, trace->pc)) {
            // to continue executed on the instruction where breakpoint 0 triggered, we set the breakpoint type 
            // to address mismatch to trigger on any instruction except the breakpoint address
            dbg_set_breakpoint_type_to_instr_addr_mismatch(0);

            if (!strncmp((char *) trace->r1, "sdpcmd_dpc", 10)) {
                printf("   BP0 step %d: pc=%08x *r1=%s\n", breakpoint_cnt0, trace->pc, trace->r1);
                // activate single-stepping for this breakpoint
                single_stepping |= DBGBP0;
            }

            // to know which breakpoint mismatch was triggerd on a next breakpoint hit, we set a bit in the breakpoint_hit variable
            breakpoint_hit |= DBGBP0;
        } else if (breakpoint_hit & DBGBP0) {
            // check whether single-stepping should be activated for this breakpoint
            if (single_stepping & DBGBP0) {
                // we reset the breakpoint for address mismatch for single-stepping
                dbg_set_breakpoint_for_addr_mismatch(0, trace->pc);

                // increase the breakpoint step counter
                breakpoint_cnt0++;

                printf("   BP0 step %d: pc=%08x\n", breakpoint_cnt0, trace->pc);

                // after 4 steps, we want to stop single-stepping
                if ((breakpoint_cnt0 == 4)) {
                    // we can now reset the breakpoint to trigger on printf again (in case we do not want to disable it)
                    dbg_set_breakpoint_for_addr_match(0, 0x126f0);
                    
                    // we reset the breakpoint counter back to zero
                    breakpoint_cnt0 = 0;

                    // we disable the breakpoint
                    //dbg_disable_breakpoint(0);

                    // we unset the bit in the breakpoint_hit variable for breakpoint 0
                    breakpoint_hit &= ~DBGBP0;

                    // we disable single-stepping for this breakpoint
                    single_stepping &= ~DBGBP0;

                    printf("   BP0 single-stepping done\n");
                }    
            } else {
                // we reset the breakpoint for address matching
                dbg_set_breakpoint_type_to_instr_addr_match(0);
                
                // we reset the breakpoint counter back to zero
                breakpoint_cnt0 = 0;

                // we unset the bit in the breakpoint_hit variable for breakpoint 0
                breakpoint_hit &= ~DBGBP0;
            }
        }
    }

    // check whether breakpoint 3 is enabled
    if (dbg_is_breakpoint_enabled(3)) {
        // Used to reset watchpoint
        if (watchpoint_hit & DBGWP0) {
            dbg_disable_breakpoint(3);

            // we unset the bit in the watchpoint_hit variable for watchpoint 0
            watchpoint_hit &= ~DBGWP0;

            // reenable watchpoint 0 only if the hit counter is below our limit
            if (++watchpoint_hit_counter < watchpoint_hit_limit) {
                dbg_enable_watchpoint(0);
            } else {
                printf("WP0 not reset (%d)\n", watchpoint_hit_counter);
            }
        }
    }
}

void
set_debug_registers(void)
{
    set_abort_stack_pointer();
//    dbg_unlock_debug_registers();
//    dbg_disable_breakpoint(0);
//    dbg_disable_breakpoint(1);
//    dbg_disable_breakpoint(2);
//    dbg_disable_breakpoint(3);
//    dbg_enable_monitor_mode_debugging();
    
    // Programm Breakpoint to match the instruction we want to hit
//    dbg_set_breakpoint_for_addr_match(0, 0x126f0); // trigger on printf

    // Program watchpoint to match the address we want to monitor
    //dbg_set_watchpoint_for_addr_match(0, 0x1D3A59); // trigger on "TCAM: %d used: %d exceed:%d" string
//    dbg_set_watchpoint_for_addr_match(0, 0x1FC2A4); // trigger on "%s: Broadcom SDPCMD CDC driver" string
}
