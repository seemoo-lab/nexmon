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

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection
#include <capabilities.h>		// capabilities included in a nexmon patch

int capabilities = NEX_CAP_MONITOR_MODE | NEX_CAP_MONITOR_MODE_RADIOTAP;

// Normally the former space of the flash patching config will be freed and added to the
// heap. We intend to place our "patch" memory region there, so that we can store our
// patch code, hence, we nop the call to the function that adds the fp config space to the heap
__attribute__((at(0x18AA58, "", CHIP_VER_BCM4358, FW_VER_7_112_200_17)))
GenericPatch4(nop_freeing_fp_config, 0x00000000);

// Hook the call to wlc_ucode_write in wlc_ucode_download
__attribute__((at(0x1F485C, "", CHIP_VER_BCM4358, FW_VER_7_112_200_17)))
BLPatch(wlc_ucode_write_compressed, wlc_ucode_write_compressed);

// Patch the "wl%d: Broadcom BCM%04x 802.11 Wireless Controller %s\n" string
__attribute__((at(0x201551, "", CHIP_VER_BCM4358, FW_VER_7_112_200_17)))
StringPatch(version_string, "nexmon (" __DATE__ " " __TIME__ ")\n");
