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
 * Copyright (c) 2026 NexMon Team                                          *
 * Copyright (c) 2026 Jakob Link <jlink@seemoo.de>                         *
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
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...

extern unsigned char ucode0_compressed_bin[];
extern unsigned int ucode0_compressed_bin_len;
extern unsigned char ucode1_compressed_bin[];
extern unsigned int ucode1_compressed_bin_len;
extern unsigned char ucode2_compressed_bin[];
extern unsigned int ucode2_compressed_bin_len;
extern unsigned char templateram0_bin[];
extern unsigned char templateram1_bin[];
extern unsigned char templateram2_bin[];
extern unsigned char templateram3_bin[];
extern unsigned char rom_data_dbg[];
extern unsigned char dbg[];

__attribute__((at(RECLAIM_3_END_PTR_0, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(reclaim_3_end_0, PATCHSTART);
__attribute__((at(RECLAIM_3_END_PTR_1, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(reclaim_3_end_1, PATCHSTART);
__attribute__((at(RECLAIM_3_END_PTR_2, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(reclaim_3_end_2, PATCHSTART);

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
__attribute__((at(UCODE2START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode2_start, ucode2_compressed_bin);
__attribute__((at(UCODE2SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode2_len, &ucode2_compressed_bin_len);

__attribute__((at(TEMPLATERAM0START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(templateram0_bin, templateram0_bin);
__attribute__((at(TEMPLATERAM1START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(templateram1_bin, templateram1_bin);
__attribute__((at(TEMPLATERAM2START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(templateram2_bin, templateram2_bin);
__attribute__((at(TEMPLATERAM3START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(templateram3_bin, templateram3_bin);

__attribute__((at(FP_ROM_DATA_DBG_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(rom_data_dbg, rom_data_dbg);
__attribute__((at(FP_DBG_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(dbg, dbg);

// Do not enable mmu protection by overwriting BL to hnd_mmu_enable_protection
__attribute__((at(MMU_PROTECT_ON_BL_ADDR, "", CHIP_VER_BCM4390b1, FW_VER_ALL)))
__attribute__((naked))
void
no_mmu_protection_patch(void)
{
    asm(
         "nop\n"
         "nop\n"
         );
}

/*
__attribute__((at(0x28BE90, "flashpatch", CHIP_VER_BCM4390b1, FW_VER_ALL)))
__attribute__((naked))
void
flash_patch_test(void)
{
    asm(
        ".byte 0x38, 0x2e, 0x38, 0x78\n"
//        ".byte 0x00, 0x45, 0x4c, 0x3a\n" // originally "EL: ..." in console
        ".byte 0x00, 0x46, 0x50, 0x3a\n" // now "FP: ..."
        ".byte 0x20, 0x25, 0x78, 0x20\n"
        ".byte 0x25, 0x78, 0x0a, 0x00\n"
        );
}
*/
