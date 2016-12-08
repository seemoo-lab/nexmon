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

int capabilities = NEX_CAP_MONITOR_MODE | NEX_CAP_MONITOR_MODE_RADIOTAP | NEX_CAP_FRAME_INJECTION;

// Hook the call to wlc_ucode_write in wlc_ucode_download
__attribute__((at(0x1F4F08, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243)))
__attribute__((at(0x1F4F14, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
BLPatch(wlc_ucode_write_compressed, wlc_ucode_write_compressed);

// reduce the amount of ucode memory freed to become part of the heap
__attribute__((at(0x1816E0, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
GenericPatch4(hndrte_reclaim_0_end, PATCHSTART);

extern unsigned char templateram_bin[];

// Moving template ram to another place in the ucode region
__attribute__((at(0x185544, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
GenericPatch4(templateram_bin, templateram_bin);

// Patch the "wl%d: Broadcom BCM%04x 802.11 Wireless Controller %s\n" string
__attribute__((at(0x1FD31B, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243)))
__attribute__((at(0x1FD327, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
StringPatch(version_string, "nexmon (" __DATE__ " " __TIME__ ")\n");
