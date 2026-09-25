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
    n->scb = p->scb;
    pkt_buf_free_skb(wlc->osh, p, 0);
    return n;
}

int dbg_scb_check_null_count = 0;
int dbg_scb_check_ok_count = 0;
unsigned int dbg_sf_p_scb = 0;
unsigned int dbg_mctl_before = 0;
unsigned int dbg_mctl_after = 0;

/*
 * wlc_send_q (0x1AB5AC) is byte-identical in relative layout to 7.45.206's
 * 0x1AAB66. It walks pkt->scb at offset 0x28 and data-aborts at 0x1AB5F6
 * when scb is NULL. If the packet has no scb, free it and continue the
 * FIFO drain. Resume 0x1AB5FA (after ldr r3,[r7,#0xe8]); loop 0x1AB75C.
 */
__attribute__((naked))
void
wlc_send_q_scb_check(void)
{
    asm volatile(
        "ldr r6, [r1, #40]\n\t"
        "cbz r6, 1f\n\t"
        "ldr.w r2, =dbg_scb_check_ok_count\n\t"
        "ldr r3, [r2]\n\t"
        "adds r3, #1\n\t"
        "str r3, [r2]\n\t"
        "ldr r7, [r6, #16]\n\t"
        "ldr.w r3, [r7, #232]\n\t"
        "movw ip, #0xB5FB\n\t"
        "movt ip, #0x001A\n\t"
        "bx ip\n\t"
        "1:\n\t"
        "ldr.w r2, =dbg_scb_check_null_count\n\t"
        "ldr r3, [r2]\n\t"
        "adds r3, #1\n\t"
        "str r3, [r2]\n\t"
        "push {lr}\n\t"
        "mov r0, sl\n\t"
        "ldr r1, [sp, #12]\n\t"
        "movs r2, #0\n\t"
        "bl pkt_buf_free_skb\n\t"
        "pop {lr}\n\t"
        "movw ip, #0xB75D\n\t"
        "movt ip, #0x001A\n\t"
        "bx ip\n\t"
        ".ltorg\n\t"
    );
}

__attribute__((at(0x1AB5F2, "", CHIP_VER_BCM43455c0, FW_VER_7_45_265_28bca26_CY)))
BPatch(wlc_send_q_scb_check, wlc_send_q_scb_check);

int dbg_sf_calls = 0;
int dbg_sf_txoff_null = 0;
int dbg_sf_no_scb = 0;
int dbg_sf_wlc_down = 0;
int dbg_sf_txfifo_calls = 0;
int dbg_sendctl_ret = -999;
unsigned int dbg_sf_scb_ptr = 0;
int dbg_sf_rate = 0;
int dbg_sf_band = 0;
int dbg_sf_hw_up = 0;
int dbg_sf_wlc_clk = 0;
unsigned int dbg_wlc_ptr = 0;
unsigned int dbg_wlc_10 = 0;
unsigned int dbg_wlc_14 = 0;
unsigned int dbg_di_ptr = 0;
unsigned int dbg_fn_ptr = 0;
unsigned int dbg_p_len = 0;
unsigned int dbg_txh_p28 = 0;
unsigned int dbg_txh_dump[16] = {0};

void
sendframe(struct wlc_info *wlc, struct sk_buff *p, unsigned int fifo, unsigned int rate)
{
    void *scb;

    printf("sendframe called: wlc=%p p=%p fifo=%u rate=%u\n", wlc, p, fifo, rate);
    dbg_sf_calls++;

    /* Guarantee headroom for the D11 TX header wlc_sendctl->wlc_d11hdrs
     * pushes; the raw injected skb has none. */
    p = pkt_with_txoff(wlc, p);
    if (!p) {
        dbg_sf_txoff_null++;
        return;
    }

    scb = wlc->band->hwrs_scb;
    dbg_sf_scb_ptr = (unsigned int) scb;
    if (!scb) {
        dbg_sf_no_scb++;
        printf("ERR: no scb found, discarding packet!\n");
        pkt_buf_free_skb(wlc->osh, p, 0);
        return;
    }

    dbg_sf_hw_up = wlc->hw->up;
    dbg_sf_band = wlc->band->bandtype;
    dbg_sf_wlc_clk = ((unsigned char *) wlc)[25];

    if (rate == 0)
        rate = (wlc->band->bandtype == WLC_BAND_5G) ? RATES_RATE_6M : RATES_RATE_2M;
    else if (wlc->band->bandtype == WLC_BAND_5G && rate < RATES_RATE_6M)
        rate = RATES_RATE_6M;

    dbg_sf_rate = rate;

    /* AC_BE. Fifo 4 (BCMC) is only drained at DTIM in AP mode. */
    if (!fifo)
        fifo = 1;

    dbg_wlc_ptr = (unsigned int) wlc;
    if (wlc->hw && fifo < 6 && wlc->hw->di[fifo])
        dbg_di_ptr = (unsigned int) wlc->hw->di[fifo];
    dbg_p_len = p->len;
    memcpy(dbg_txh_dump, p->data, 64);

    p->scb = scb;
    dbg_sf_p_scb = (unsigned int) p->scb;

    /*
     * Proven D11AC injection path (BCM4358/BCM4356): wlc_sendctl builds the
     * AC-native d11actxh internally (correct ratespec encoding from the raw
     * 500kbps-unit rate) and drives the full TX engine, unlike the direct
     * wlc_txfifo path which is a D11N idiom the AC ucode silently drops.
     * wlc_sendctl is ROM at 0x31CE8 for this chip.
     */
    if (wlc && wlc->hw) {
        *(int *)((char *)wlc->hw + 0x144) = 0;
        *(int *)((char *)wlc->hw + 0xf0) = 1;
    }
    if (wlc->regs) dbg_mctl_before = wlc->regs->maccontrol;
    wlc_enable_mac(wlc);
    if (wlc->regs) dbg_mctl_after = wlc->regs->maccontrol;
    printf("sendframe: wlc=%p p=%p fifo=%u rate=%u len=%u scb=%p\n", wlc, p, fifo, rate, p->len, scb);
    if (wlc->hw->up) {
        dbg_sendctl_ret = wlc_sendctl(wlc, p, wlc->active_queue, scb, fifo, rate, 0);
        printf("wlc_sendctl ret=%d (up)\n", dbg_sendctl_ret);
    } else {
        dbg_sf_wlc_down++;
        dbg_sendctl_ret = wlc_sendctl(wlc, p, wlc->active_queue, scb, fifo, rate, 1);
        printf("wlc_sendctl ret=%d (down)\n", dbg_sendctl_ret);
        printf("ERR: wlc down\n");
    }
    dbg_sf_txfifo_calls++;
}
