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

struct tx_task {
    struct wlc_info *wlc;
    struct sk_buff *p;
    unsigned int fifo;
    unsigned int rate;
    int txrepetitions;
    int txperiodicity;
};

void
sendframe(struct wlc_info *wlc, struct sk_buff *p, unsigned int fifo, unsigned int rate)
{
    if (wlc->band->bandtype == WLC_BAND_5G && rate < RATES_RATE_6M) {
        rate = RATES_RATE_6M;
    }

    if (wlc->hw->up) {
        if (p->flags & 0x80) { // WLF_TXHDR = 0x80
            if (wlc_prec_enq(wlc, wlc->active_queue + 4, p, 5)) {
                wlc_send_q(wlc, wlc->active_queue);
            } else {
                pkt_buf_free_skb(wlc->osh, p, 0);
            }
        } else {
            wlc_sendctl(wlc, p, wlc->active_queue, wlc->band->hwrs_scb, fifo, rate, 0);
        }
    } else {
        pkt_buf_free_skb(wlc->osh, p, 0);
        printf("ERR: wlc down\n");
    }
}

static void
sendframe_copy(struct tx_task *task)
{
    // first, we create a copy copy of the frame that should be transmitted
    struct sk_buff *p_copy = pkt_buf_get_skb(task->wlc->osh, task->p->len + 202);
    skb_pull(p_copy, 202);
    memcpy(p_copy->data, task->p->data, task->p->len);
    p_copy->flags = task->p->flags;
    p_copy->scb = task->p->scb;

    sendframe(task->wlc, p_copy, task->fifo, task->rate);

    if (task->txrepetitions > 0) {
        task->txrepetitions--;
    }
}

static void
sendframe_timer_handler(struct hndrte_timer *t)
{
    struct tx_task *task = (struct tx_task *) t->data;

    if (task->txrepetitions == 0) {
        // there must have been a mistake, just delete the frame task and timer
        pkt_buf_free_skb(task->wlc->osh, task->p, 0);
        goto free_timer_and_task;
    } else if (task->txrepetitions == 1) {
        // transmit the last frame
        sendframe(task->wlc, task->p, task->fifo, task->rate);
free_timer_and_task:
        hndrte_del_timer(t);
        hndrte_free_timer(t);
        free(task);
    } else {
        sendframe_copy(task);
    }
}

static void
sendframe_repeatedly(struct tx_task *task)
{
    struct hndrte_timer *t;

    sendframe_copy(task);
    if (task->txrepetitions == 0)
        return;

    t = hndrte_init_timer(sendframe_repeatedly, task, sendframe_timer_handler, 0);

    if (!t) {
        free(task);
        return;
    }

    if (!hndrte_add_timer(t, task->txperiodicity, 1)) {
        hndrte_free_timer(t);
        free(task);

        printf("ERR: could not add timer");
    }
}

/**
 *  Is scheduled to transmit a frame after a delay
 */
static void
sendframe_task_handler(struct hndrte_timer *t)
{
    struct tx_task *task = (struct tx_task *) t->data;

    if (task->txrepetitions != 0 && task->txperiodicity > 0) {
        sendframe_repeatedly(task);
    } else {
        sendframe(task->wlc, task->p, task->fifo, task->rate);
        free(task);
    }
}

void
sendframe_with_timer(struct wlc_info *wlc, struct sk_buff *p, unsigned int fifo, unsigned int rate, int txdelay, int txrepetitions, int txperiodicity)
{
    struct tx_task *task = 0;

    // if we need to send the frame with a delay or repeatedly, we create a task
    if (txdelay > 0 || (txrepetitions != 0 && txperiodicity > 0)) {
        task = (struct tx_task *) malloc(sizeof(struct tx_task), 0);
        memset(task, 0, sizeof(struct tx_task)); // will be freed after finishing the task
        task->wlc = wlc;
        task->p = p;
        task->fifo = fifo;
        task->rate = rate;
        task->txrepetitions = txrepetitions;
        task->txperiodicity = txperiodicity;
    }

    if (txdelay > 0) {
        hndrte_schedule_work(sendframe_with_timer, task, sendframe_task_handler, txdelay);
    } else if (txrepetitions != 0 && txperiodicity > 0) {
        sendframe_repeatedly(task);
    } else {
        sendframe(wlc, p, fifo, rate);
    }
}
