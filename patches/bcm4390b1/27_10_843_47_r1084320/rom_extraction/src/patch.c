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
extern unsigned char fp_rom_data_dbg_bin[];
extern unsigned char fp_dbg_bin[];

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
GenericPatch4(fp_rom_data_dbg_bin, fp_rom_data_dbg_bin);
__attribute__((at(FP_DBG_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(fp_dbg_bin, fp_dbg_bin);

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
