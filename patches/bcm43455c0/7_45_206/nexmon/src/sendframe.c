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
 * Copyright (c) 2020 NexMon Team                                          *
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

/* AC firmware prepends a 124-byte D11 TX header plus PHY/control data. */
#define TXOFF 202

static struct sk_buff *
pkt_with_txoff(struct wlc_info *wlc, struct sk_buff *p)
{
    struct sk_buff *n;
    int orig_len = p->len;

    n = pkt_buf_get_skb(wlc->osh, orig_len + TXOFF);
    if (!n) {
        pkt_buf_free_skb(wlc->osh, p, 0);
        return 0;
    }

    skb_pull(n, TXOFF);
    memcpy(n->data, p->data, orig_len);
    n->len = orig_len;
    n->flags = p->flags;
    n->pkttag_flags = p->pkttag_flags;
    pkt_buf_free_skb(wlc->osh, p, 0);
    return n;
}

/*
 * wlc_send_q (0x1AAB66) walks pkt->scb at offset 0x28 and immediately
 * dereferences it. Monitor-mode TX (ours, or firmware-generated) can
 * land on that path with scb == NULL and take a data abort at 0x1AABB0.
 * If the packet has no scb, free it and continue the FIFO drain.
 *
 * 0x1AABB4 is the instruction after the original ldr r3, [r7, #0xe8].
 * 0x1AAD16 is the top of the FIFO loop. ip is caller-saved and unused
 * across that resume point.
 */
__attribute__((naked))
void
wlc_send_q_scb_check(void)
{
    asm volatile(
        "ldr r6, [r1, #40]\n\t"
        "cbz r6, 1f\n\t"
        "ldr r7, [r6, #16]\n\t"
        "ldr.w r3, [r7, #232]\n\t"
        "movw ip, #0xABB5\n\t"
        "movt ip, #0x001A\n\t"
        "bx ip\n\t"
        "1:\n\t"
        "push {lr}\n\t"
        "mov r0, sl\n\t"
        "ldr r1, [sp, #12]\n\t"
        "movs r2, #0\n\t"
        "bl pkt_buf_free_skb\n\t"
        "pop {lr}\n\t"
        "movw ip, #0xAD17\n\t"
        "movt ip, #0x001A\n\t"
        "bx ip\n\t"
    );
}

__attribute__((at(0x1AABAC, "", CHIP_VER_BCM43455c0, FW_VER_7_45_206)))
BPatch(wlc_send_q_scb_check, wlc_send_q_scb_check);

void
sendframe(struct wlc_info *wlc, struct sk_buff *p, unsigned int fifo, unsigned int rate)
{
    void *scb;
    struct wlc_txh_info txh = {0};
    short txh_off[2] = {0};

    p = pkt_with_txoff(wlc, p);
    if (!p)
        return;

    scb = wlc->band->hwrs_scb;
    if (!scb) {
        printf("ERR: no scb found, discarding packet!\n");
        pkt_buf_free_skb(wlc->osh, p, 0);
        return;
    }

    if (!wlc->hw->up) {
        printf("ERR: wlc down\n");
        pkt_buf_free_skb(wlc->osh, p, 0);
        return;
    }

    if (rate == 0)
        rate = (wlc->band->bandtype == WLC_BAND_5G) ? RATES_RATE_6M : RATES_RATE_2M;
    else if (wlc->band->bandtype == WLC_BAND_5G && rate < RATES_RATE_6M)
        rate = RATES_RATE_6M;

    if (!fifo)
        fifo = 1;

    /*
     * Raw monitor frames carry no Broadcom TX header. Build it synchronously
     * and submit straight to the D11 FIFO - the same path the working
     * BCM43430a1 port takes. wlc_sendctl() instead parks the packet on
     * wlc_send_q(), which data-aborts on a NULL pkt->scb (0x1AABB0) even
     * after we stamp hwrs_scb.
     */
    wlc_enable_mac(wlc);
    wlc_d11hdrs(wlc, p, scb, 0, 0, 1, fifo, 0, 0, rate, txh_off);
    p->scb = scb;
    wlc_get_txh_info(wlc, p, &txh);
    wlc_txfifo(wlc, fifo, p, &txh, 1, 1);
}
