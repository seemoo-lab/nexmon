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
 * Copyright (c) 2023 NexMon Team                                          *
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
 * Classic (bcm43430a1) frame-injection TX path, ported to bcm43439a0.
 * Ported from patches/bcm43430a1/7_45_41_46/nexmon/src/sendframe.c.
 *
 * Symbols used here are all already resolved for the 43439 in
 * patches/common/wrapper.c:
 *   wlc_d11hdrs       @ 0x8929FC   build the d11 TX descriptor
 *   wlc_get_txh_info  @ 0x82B8FC   read the tx header info into a scratch blob
 *   wlc_txfifo        @ 0x834194   enqueue into TX DMA FIFO 1
 *   pkt_buf_free_skb  @ 0x80C144   free on the error path
 *
 * The two struct fields touched here (wlc->band->hwrs_scb at band+0x018 and
 * wlc->hw->up at hw+0x006) were resolved against the 43439 ROM/RAM dump; see
 * structs.common.h.
 */

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...
#include <rates.h>              // rates used to build the ratespec for frame injection
#include <nexioctls.h>          // ioctls added in the nexmon patch
#include <capabilities.h>       // capabilities included in a nexmon patch

void
sendframe(struct wlc_info *wlc, struct sk_buff *p, unsigned int fifo, unsigned int rate)
{
    int short_preamble = 0;
    struct wlc_txh_info txh = {0};

    // hwrs_scb is the hardware-rate-set scb the firmware uses as the TX scb for
    // self-originated frames; hw->up gates TX on the MAC being up.  Both offsets
    // are from the ROM/RAM dump (see structs.common.h).
    if (wlc->band->hwrs_scb) {
        if (wlc->hw->up) {
            wlc_d11hdrs(wlc, p, wlc->band->hwrs_scb, short_preamble, 0, 1, 1, 0, 0, rate);
            p->scb = wlc->band->hwrs_scb;
            wlc_get_txh_info(wlc, p, &txh);
            wlc_txfifo(wlc, fifo, p, &txh, 1, 1);
        } else {
            // MAC is down: nothing will transmit. Free the skb instead of
            // leaking it; the ioctl trigger fires at 1 Hz, so a leak here
            // exhausts the packet pool and wedges the chip.
            printf("ERR: wlc down\n");
            pkt_buf_free_skb(wlc->osh, p, 0);
        }
    } else {
        printf("ERR: no scb found, discarding packet!\n");
        pkt_buf_free_skb(wlc->osh, p, 0);
    }
}
