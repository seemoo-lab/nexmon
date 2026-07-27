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
 * Copyright (c) 2025 NexMon Team                                          *
 * Copyright (c) 2025 Manuel Calviño                                       *
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
 
// Enhanced protection patch 
// Declares a WRX segment on to our patch zone

#pragma NEXMON targetregion "patch"
#include <wrapper.h>	// wrapper definitions for functions that already exist in the firmware
#include <structs.h>	// structures that are used by the code in the firmware
#include <firmware_version.h>   // definition of firmware version macros
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...
#include <capabilities.h>      // capabilities included in a nexmon patch
#include <local_wrapper.h>

#define TEXT_START 0x6E0000;
#define TEXT_END 0x786D44;
#define AI_SCAN 0x79FC50;

__attribute__((at(0x7BC738, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(protection_patch_rwx, 0x7BC56B); 

void hnd_image_info_hook(uint32_t *img_info) {
    img_info[0] = TEXT_START; 
    img_info[1] = TEXT_END;
    img_info[2] = TEXT_END;
    img_info[3] = 0x798FD8;
    img_info[4] = 0x799738;
    img_info[5] = 0x79B790; 
    img_info[6] = 0x79B790; 
    img_info[7] = AI_SCAN;
    img_info[24] = 0x772AA8;
    img_info[25] = 0x79875F;
    img_info[22] = 0x6E2000;
    img_info[26] = PATCHEND;
    img_info[23] = ((PATCHEND-1) & 0xFFFFFC00) - 1;
    img_info[10] = AI_SCAN;
    img_info[11] = 0x7E2658;
    img_info[12] = 0x7E2658;
    img_info[13] = RECLAIM_3_END_PTR_0;
    img_info[14] = 0x6E00FC;
    img_info[15] = 0x6E0100;
    img_info[16] = TEXT_START;
    img_info[17] = TEXT_START;
    img_info[18] = AI_SCAN;
    img_info[19] = AI_SCAN;
    img_info[20] = 0x6E03F8;
    img_info[21] = 0x6E2000;*/
}
__attribute__((at(0x7BC6AA, "", CHIP_VER_BCM4383a3, FW_VER_20_125_5_r1104089)))
BLPatch(hnd_image_info_hook, hnd_image_info_hook);