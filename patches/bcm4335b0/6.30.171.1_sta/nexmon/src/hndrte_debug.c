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
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
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

__attribute__((at(0x180100, "", CHIP_VER_BCM4335b0, FW_VER_6_30_171_1_sta)))
struct hndrte_debug dbg = {
	.magic = HNDRTE_DEBUG_MAGIC,
	.version = HNDRTE_DEBUG_VERSION,
	.fwid = 0,
	.epivers = { 0 },
	.trap_ptr = 0,
	.console = 0x1FB7D0,
	.ram_base = 0x180000,
	.ram_size = 768 * 1024,
	.rom_base = 0,
	.rom_size = 640 * 1024,
	.event_log_top = 0x0
};

__attribute__((at(0x180878, "", CHIP_VER_BCM4335b0, FW_VER_6_30_171_1_sta)))
GenericPatch4(dbpp, 0x50504244);

__attribute__((at(0x18087C, "", CHIP_VER_BCM4335b0, FW_VER_6_30_171_1_sta)))
GenericPatch4(dbppptr, &dbg);
