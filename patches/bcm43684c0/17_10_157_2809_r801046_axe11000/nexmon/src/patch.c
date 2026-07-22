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

#include <firmware_version.h>
#include <patcher.h>
#include <structs.h>
#include <wrapper.h>

extern void wlc_ucode_write_compressed_args(struct wlc_hw_info *wlc_hw, const int ucode[], const unsigned int nbytes);
extern void wlc_ucodex_write_compressed_args(struct wlc_hw_info *wlc_hw, const int ucodex[], const unsigned int nbytes);

extern unsigned char d11ucode_mu129_compressed_bin[];
extern unsigned int d11ucode_mu129_compressed_bin_len;
extern unsigned char d11ucodex_mu129_compressed_bin[];
extern unsigned int d11ucodex_mu129_compressed_bin_len;
extern unsigned char d11ucode_mu129_1_compressed_bin[];
extern unsigned int d11ucode_mu129_1_compressed_bin_len;
extern unsigned char d11ucodex_mu129_1_compressed_bin[];
extern unsigned int d11ucodex_mu129_1_compressed_bin_len;
extern unsigned char d11ucode_btcxmu129_1_compressed_bin[];
extern unsigned int d11ucode_btcxmu129_1_compressed_bin_len;
extern unsigned char d11ucodex_btcxmu129_1_compressed_bin[];
extern unsigned int d11ucodex_btcxmu129_1_compressed_bin_len;
extern unsigned char d11ucode_mu129_2_compressed_bin[];
extern unsigned int d11ucode_mu129_2_compressed_bin_len;
extern unsigned char d11ucodex_mu129_2_compressed_bin[];
extern unsigned int d11ucodex_mu129_2_compressed_bin_len;
extern unsigned char d11ucode_btcxmu129_2_compressed_bin[];
extern unsigned int d11ucode_btcxmu129_2_compressed_bin_len;
extern unsigned char d11ucodex_btcxmu129_2_compressed_bin[];
extern unsigned int d11ucodex_btcxmu129_2_compressed_bin_len;

struct wlc_hw_info *wlc_hw_ucode = 0;

/* Reduce reclaimed area to reserve patch space */
__attribute__((at(RECLAIM_END_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
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
__attribute__((at(WLC_UCODE_WRITE_BL_HOOK_2_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_ucode_write_compressed_args_2, wlc_ucode_write_compressed_args__regs2wlc_hw);
__attribute__((at(WLC_UCODE_WRITE_BL_HOOK_3_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_ucode_write_compressed_args_3, wlc_ucode_write_compressed_args__regs2wlc_hw);
__attribute__((at(WLC_UCODE_WRITE_BL_HOOK_4_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_ucode_write_compressed_args_4, wlc_ucode_write_compressed_args__regs2wlc_hw);

/* Hook the call to wlc_ucodex_write in wlc_bmac_process_ucode_sr */
void
wlc_ucodex_write_compressed_args__regs2wlc_hw(struct d11regs **p_regs, const int ucodex[], const unsigned int nbytes) {
    wlc_ucodex_write_compressed_args(wlc_hw_ucode, ucodex, nbytes);
}
__attribute__((at(WLC_UCODEX_WRITE_BL_HOOK_ADDR, "", CHIP_VER_ALL, FW_VER_ALL)))
BLPatch(wlc_ucodex_write_compressed_args, wlc_ucodex_write_compressed_args__regs2wlc_hw);

/* Update pointers to ucodes and ucodesizes */
__attribute__((at(UCODE0START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode0_start, d11ucode_mu129_compressed_bin);
__attribute__((at(UCODE0SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode0_len, &d11ucode_mu129_compressed_bin_len);

__attribute__((at(UCODEX0START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodex0_start, d11ucodex_mu129_compressed_bin);
__attribute__((at(UCODEX0SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodex0_len, &d11ucodex_mu129_compressed_bin_len);

__attribute__((at(UCODE1START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode1_start, d11ucode_mu129_1_compressed_bin);
__attribute__((at(UCODE1SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode1_len, &d11ucode_mu129_1_compressed_bin_len);

__attribute__((at(UCODEX1START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodex1_start, d11ucodex_mu129_1_compressed_bin);
__attribute__((at(UCODEX1SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodex1_len, &d11ucodex_mu129_1_compressed_bin_len);

__attribute__((at(UCODEBT1START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodebt1_start, d11ucode_btcxmu129_1_compressed_bin);
__attribute__((at(UCODEBT1SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodebt1_len, &d11ucode_btcxmu129_1_compressed_bin_len);

__attribute__((at(UCODEXBT1START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodexbt1_start, d11ucodex_btcxmu129_1_compressed_bin);
__attribute__((at(UCODEXBT1SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodexbt1_len, &d11ucodex_btcxmu129_1_compressed_bin_len);

__attribute__((at(UCODE2START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode2_start, d11ucode_mu129_2_compressed_bin);
__attribute__((at(UCODE2SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucode2_len, &d11ucode_mu129_2_compressed_bin_len);

__attribute__((at(UCODEX2START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodex2_start, d11ucodex_mu129_2_compressed_bin);
__attribute__((at(UCODEX2SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodex2_len, &d11ucodex_mu129_2_compressed_bin_len);

__attribute__((at(UCODEBT2START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodebt2_start, d11ucode_btcxmu129_2_compressed_bin);
__attribute__((at(UCODEBT2SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodebt2_len, &d11ucode_btcxmu129_2_compressed_bin_len);

__attribute__((at(UCODEXBT2START_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodexbt2_start, d11ucodex_btcxmu129_2_compressed_bin);
__attribute__((at(UCODEXBT2SIZE_PTR, "", CHIP_VER_ALL, FW_VER_ALL)))
GenericPatch4(ucodexbt2_len, &d11ucodex_btcxmu129_2_compressed_bin_len);

/* Do not enable mmu protection by overwriting BL to ca7_execute_protect_on */
__attribute__((at(0x10240, "", CHIP_VER_BCM43684c0, FW_VER_17_10_157_2809_r801046_axe11000)))
__attribute__((naked))
void
no_ca7_execute_protect_on_patch(void)
{
    asm(
         "nop\n"
         "nop\n"
         );
}
