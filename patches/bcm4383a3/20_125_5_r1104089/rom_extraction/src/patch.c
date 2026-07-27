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

/*
[ ROM ]
[ RAM ]
    [ UCODE0 ]
    [ UCODE1 ]
    [ TemplateRAM2 ]
    [ TemplateRAM1 ]
    [ TemplateRAM0 ]
    <--- gap / reclaim --->
    [ PATCH zone ]
*/
#pragma NEXMON targetregion "patch"
#include <wrapper.h>	// wrapper definitions for functions that already exist in the firmware
#include <structs.h>	// structures that are used by the code in the firmware
#include <firmware_version.h>   // definition of firmware version macros
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...
#include <capabilities.h>      // capabilities included in a nexmon patch
#include <local_wrapper.h>

extern unsigned char ucode0_compressed_bin[];
extern unsigned int ucode0_compressed_bin_len;
extern unsigned char ucode1_compressed_bin[];
extern unsigned int ucode1_compressed_bin_len;

extern unsigned char templateram0_bin[];
extern unsigned char templateram1_bin[];
extern unsigned char templateram2_bin[];

__attribute__((at(RECLAIM_3_END_PTR_0, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(reclaim_3_end_fixed_0, PATCHSTART);  // reclaim limit after init (PATCHSTART)

__attribute__((at(WLC_UCODE_WRITE_BL_HOOK_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_ucode_write_compressed_args, wlc_ucode_write_compressed_args);

__attribute__((at(UCODE0START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode0_start, ucode0_compressed_bin);
__attribute__((at(UCODE0SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode0_len, &ucode0_compressed_bin_len);

__attribute__((at(UCODE1START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode1_start, ucode1_compressed_bin);
__attribute__((at(UCODE1SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode1_len, &ucode1_compressed_bin_len);

__attribute__((at(TEMPLATERAM0START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(templateram0_ptr_patch, templateram0_bin); 

__attribute__((at(TEMPLATERAM1START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(templateram1_ptr_patch, templateram1_bin);

__attribute__((at(TEMPLATERAM2START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(templateram2_ptr_patch, templateram2_bin);


