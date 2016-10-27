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
 * Copyright (c) 2015 NexMon Team                                          *
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
 *                                                                         *
 **************************************************************************/

#include <firmware_version.h>   // definition of firmware version macros
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection

struct hndrte_debug {
	uint32	magic;
#define HNDRTE_DEBUG_MAGIC 0x47424544	/* 'DEBG' */

	uint32	version;		/* Debug struct version */
#define HNDRTE_DEBUG_VERSION 1

	uint32	fwid;			/* 4 bytes of fw info */
	char	epivers[32];

	uint32  trap_ptr;		/* trap_t data struct */
	uint32  console;		/* Console  */

	uint32	ram_base;
	uint32	ram_size;

	uint32	rom_base;
	uint32	rom_size;

	uint32  event_log_top;

};

__attribute__((at(0x94, "", CHIP_VER_BCM4330, FW_VER_5_90_100_41)))
struct hndrte_debug dbg = {
	.magic = HNDRTE_DEBUG_MAGIC,
	.version = HNDRTE_DEBUG_VERSION,
	.fwid = 0,
	.epivers = { 0 },
	.trap_ptr = 0,
	.console = 0x459cc,
	.ram_base = 0x0,
	.ram_size = 288 * 1024,
	.rom_base = 0x800000,
	.rom_size = 512 * 1024,
	.event_log_top = 0x0
};

__attribute__((at(0x8c, "", CHIP_VER_BCM4330, FW_VER_5_90_100_41)))
GenericPatch4(dbpp, 0x50504244);

__attribute__((at(0x90, "", CHIP_VER_BCM4330, FW_VER_5_90_100_41)))
GenericPatch4(dbppptr, &dbg);
