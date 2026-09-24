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
 * Copyright (c) 2024 NexMon Team                                          *
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
#include <structs.h>            // structures that are used by the code in the firmware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware

extern void wlc_ucode_write_compressed_args(struct wlc_hw_info *wlc_hw, const int ucode[], const unsigned int nbytes);
extern void wlc_ucodex_write_compressed_args(struct wlc_hw_info *wlc_hw, const int ucodex[], const unsigned int nbytes);

extern unsigned char ucode0_compressed_bin[];
extern unsigned int ucode0_compressed_bin_len;
extern unsigned char ucode1_compressed_bin[];
extern unsigned int ucode1_compressed_bin_len;
extern unsigned char ucode2_compressed_bin[];
extern unsigned int ucode2_compressed_bin_len;
extern unsigned char ucode3_compressed_bin[];
extern unsigned int ucode3_compressed_bin_len;

struct wlc_hw_info *wlc_hw_ucode = 0;

/* Reduce reclaimed area to reserve patch space */
__attribute__((at(RECLAIM_3_END_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(reclaim_end, PATCHSTART);

/* 
 * Hook call to wlc_bmac_process_ucode_sr to fetch wlc_hw pointer
 * Required as struct d11regs** is first parameter to wlc_ucode[x]_write
 * but struct wlc_hw* needed by wlc_ucode[x]_write_compressed_args 
 */
int
wlc_bmac_process_ucode_sr_hook(void *wlc_hw) {
    wlc_hw_ucode = wlc_hw;
    return wlc_bmac_process_ucode_sr(wlc_hw);
}
__attribute__((at(WLC_BMAC_PROCESS_UCODE_SR_BL_HOOK_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_bmac_process_ucode_sr_hook, wlc_bmac_process_ucode_sr_hook);

/* Hook the calls to wlc_ucode_write in wlc_bmac_process_ucode_sr */
void
wlc_ucode_write_compressed_args__regs2wlc_hw(struct d11regs **p_regs, const int ucode[], const unsigned int nbytes) {
    wlc_ucode_write_compressed_args(wlc_hw_ucode, ucode, nbytes);
}
__attribute__((at(WLC_UCODE_WRITE_BL_HOOK_0_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_ucode_write_compressed_args_0, wlc_ucode_write_compressed_args__regs2wlc_hw);
__attribute__((at(WLC_UCODE_WRITE_BL_HOOK_1_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_ucode_write_compressed_args_1, wlc_ucode_write_compressed_args__regs2wlc_hw);

/* Hook the call to wlc_ucodex_write in wlc_bmac_process_ucode_sr */
void
wlc_ucodex_write_compressed_args__regs2wlc_hw(struct d11regs **p_regs, const int ucodex[], const unsigned int nbytes) {
    wlc_ucodex_write_compressed_args(wlc_hw_ucode, ucodex, nbytes);
}
__attribute__((at(WLC_UCODEX_WRITE_BL_HOOK_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_ucodex_write_compressed_args, wlc_ucodex_write_compressed_args__regs2wlc_hw);

/* Update pointers to ucodes and ucodesizes */
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
__attribute__((at(UCODE3START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode3_start, ucode3_compressed_bin);
__attribute__((at(UCODE3SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode3_len, &ucode3_compressed_bin_len);

/* Do not enable mmu protection by overwriting BL to hnd_mmu_enable_protection */
__attribute__((at(0xED20, "", CHIP_VER_BCM6715b0, FW_VER_17_10_188_6401_r808804_ax82uv2)))
__attribute__((naked))
void
no_mmu_protection_patch(void)
{
    asm(
         "nop\n"
         "nop\n"
         );
}
