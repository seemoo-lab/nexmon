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
 * Frame injection entry point for bcm43439a0.
 * Ported from patches/bcm43430a1/7_45_41_46/nexmon/src/injection.c.
 *
 * IMPORTANT divergence from the classic port:
 * the Pico W has no Linux netdev and no mon0 interface, so there is NO
 * wl_send_hook / netdev-transmit GenericPatch4 here.  inject_frame() is driven
 * purely by the NEX_INJECT_FRAME ioctl case in ioctl.c, which hands us a fully
 * formed, radiotap-prefixed skb.  We therefore take (wlc, p) directly instead
 * of (wl, p): the ioctl hook already has wlc in hand.
 *
 * Do NOT include local_wrapper.h here: the 43439 ships include/local_wrapper.h
 * but no src/local_wrapper.c, so any file that includes it fails to build.
 */

#pragma NEXMON targetregion "patch"

#include <firmware_version.h>   // definition of firmware version macros
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <patcher.h>            // macros used to create patches such as BLPatch, BPatch, ...
#include <helper.h>             // useful helper functions
#include <ieee80211_radiotap.h> // radiotap header parsing
#include <sendframe.h>          // sendframe()
#include "d11.h"
#include "brcm.h"

int
inject_frame(struct wlc_info *wlc, struct sk_buff *p)
{
    int rtap_len = 0;
    int data_rate = 0;

    struct ieee80211_radiotap_iterator iterator;
    struct ieee80211_radiotap_header *rtap_header;

    // radiotap it_len is a little-endian uint16 at offset 2 (bytes 2..3). Read
    // both bytes so we agree with the iterator; a single signed byte would
    // sign-extend and disagree for lengths > 127.
    rtap_len = ((unsigned char *)p->data)[2] | (((unsigned char *)p->data)[3] << 8);
    rtap_header = (struct ieee80211_radiotap_header *) p->data;

    // The NEX_INJECT_FRAME caller controls these header bytes. Never trust it_len
    // past the actual frame: an overstated it_len would let the iterator read
    // past the skb allocation and would underflow p->len (uint16) at skb_pull below.
    if (rtap_len < sizeof(struct ieee80211_radiotap_header) || rtap_len > p->len) {
        printf("ERR: inject rtap_len\n");
        pkt_buf_free_skb(wlc->osh, p, 0);
        return -1;
    }

    // Bound the iterator by the real frame length so it can never be told the
    // buffer is larger than it is.
    int ret = ieee80211_radiotap_iterator_init(&iterator, rtap_header, p->len, 0);

    if (ret) {
        // malformed radiotap header: free the skb so the ioctl path never leaks it
        printf("ERR: inject rtap_init\n");
        pkt_buf_free_skb(wlc->osh, p, 0);
        return -1;
    }

    while (!ret) {
        ret = ieee80211_radiotap_iterator_next(&iterator);
        if (ret) {
            continue;
        }
        switch (iterator.this_arg_index) {
            case IEEE80211_RADIOTAP_RATE:
                // legacy rate, in 500 kbps units, used as the rspec override
                data_rate = (*iterator.this_arg);
                break;
            case IEEE80211_RADIOTAP_CHANNEL:
                break;
            default:
                break;
        }
    }

    skb_pull(p, rtap_len);

    // inject straight into TX FIFO 1, bypassing the regular tx queue.
    // sendframe() consumes p (hands it to the DMA FIFO) or frees it on error.
    sendframe(wlc, p, 1, data_rate);

    return 0;
}
