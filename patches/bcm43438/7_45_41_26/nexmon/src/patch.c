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

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>
#include <wrapper.h>	// wrapper definitions for functions that already exist in the firmware
#include <structs.h>	// structures that are used by the code in the firmware
#include <patcher.h>
#include <helper.h>
#include <capabilities.h>      // capabilities included in a nexmon patch

int capabilities = NEX_CAP_MONITOR_MODE | NEX_CAP_MONITOR_MODE_RADIOTAP | NEX_CAP_FRAME_INJECTION;

void *
wlc_recvdata_hook(void *wlc, void *osh, void *rxh, void *p) {
    return pkt_buf_free_skb(osh, p, 0);
}

__attribute__((at(0x1210C, "", CHIP_VER_BCM43438, FW_VER_ALL)))
BPatch(wlc_recvdata_hook, wlc_recvdata_hook);

// Hook the call to wlc_ucode_write in wlc_ucode_download
//__attribute__((at(0x44ED0, "", CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327)))
//BLPatch(wlc_ucode_write_compressed, wlc_ucode_write_compressed);

// Update the ucode length to become the length of the extracted ucode before compression
//__attribute__((at(0x4E9BC, "", CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327)))
//GenericPatch4(ucode_length, 0xCC28);

//__attribute__((at(0x2654, "", CHIP_VER_BCM43438, FW_VER_7_45_41_26_r640327)))
//GenericPatch4(hndrte_reclaim_0_end, 0x592a4);
