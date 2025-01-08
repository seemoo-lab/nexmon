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

#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection

char
sendframe(struct wlc_info *wlc, struct sk_buff *p, unsigned int fifo, unsigned int rate)
{
    char ret;
    p->scb = wlc->band->hwrs_scb;
    if (wlc->band->bandtype == WLC_BAND_5G && rate < RATES_RATE_6M) {
        rate = RATES_RATE_6M;
    }

    if (wlc->hw->up) {
        ret = wlc_sendctl(wlc, p, wlc->active_queue, wlc->band->hwrs_scb, fifo, rate, 0);
    } else {
        ret = wlc_sendctl(wlc, p, wlc->active_queue, wlc->band->hwrs_scb, fifo, rate, 1);
        printf("ERR: wlc down\n");
    }
    return ret;
}
/*
 Based on jlinktu's patch for bcm43455c0
https://github.com/seemoo-lab/nexmon/issues/335#issuecomment-738928287
*/
__attribute__((naked))
void
check_scb(void)
{
     asm(
        "ldr r5, [r2,#0x2c]\n"
        "cmp r5, #0\n"          // check if pkt->scb is null
        "bne nonnull\n"
        "add lr,lr,0x14e\n"     // if null adapt lr to jump out of pkt dequeue loop
        "b return\n"
        "nonnull:\n"
        "ldr r3,[r5,#0xc]\n"    // get scb->cfg (crashed the chip when scb was null)
        "return:\n"
        "push {lr}\n"
        "pop {pc}\n"
    );
}

__attribute__((at(0x33734, "", CHIP_VER_BCM43436b0, FW_VER_9_88_0_0)))
__attribute__((naked))
void
patch_null_pointer_scb(void)
{
    asm(
        "bl.w check_scb\n"  // branch to null pointer check instead of accessing possibly invalid cfg
    );

}
