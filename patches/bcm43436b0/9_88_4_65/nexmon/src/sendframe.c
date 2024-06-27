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
#include <nexioctls.h>          // ioctls added in the nexmon patch
#include <capabilities.h>       // capabilities included in a nexmon patch

char
sendframe(struct wlc_info *wlc, struct sk_buff *p, unsigned int fifo, unsigned int rate)
{
    char ret;
    /*
    printf("wlc->hw->up:%02X\n", wlc->hw->up);
    printf("p:%p\n", p);
    printf("p->len:%d\n", p->len);
    printf("wlc:%p\n", wlc);
    printf("wlc->active_queue:%p\n", wlc->active_queue);
    printf("wlc->band:%p\n", wlc->band);
    printf("wlc->band->bandtype:%d wlc->band->bandunit:%d wlc->band->phytype:%d wlc->band->phyrev:%d \n", wlc->band->bandtype, wlc->band->bandunit, wlc->band->phytype, wlc->band->phyrev);
    printf("wlc->band->hwrs_scb:%p\n", wlc->band->hwrs_scb);
    */

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
    //printf("Sendctl returned:%d\n\n", ret);
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
        "cmp r5, #0\n"             // check if pkt->scb is null
        "bne nonnull\n"
        "add lr,lr,0x14c\n"        // if null adapt lr to jump out of pkt dequeue loop
        "b return\n"
        "nonnull:\n"
        "ldr.w r3,[r5,#0xc]\n"    // get scb->cfg (crashed the chip when scb was null)
        "return:\n"
        "push {lr}\n"
        "pop {pc}\n"
    );
}

__attribute__((at(0x3489e, "", CHIP_VER_BCM43436b0, FW_VER_9_88_4_65)))
__attribute__((naked))
void
patch_null_pointer_scb(void)
{
    asm(
        "bl check_scb\n"    // branch to null pointer check instead of accessing possibly invalid cfg
    );

}
