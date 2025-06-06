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
#include <ieee80211_radiotap.h>
#include <bcmwifi_channels.h>

extern void prepend_ethernet_ipv4_udp_header(struct sk_buff *p);

#define FRAMEOFFSET 56

void
wlc_recv_hook(struct wlc_info *wlc, struct sk_buff *p) {
    struct osl_info *osh = wlc->osh;
    uint32_t monitor = wlc->monitor;

    if (monitor > 0) {
        struct sk_buff *p_new = pkt_buf_get_skb(osh, p->len - FRAMEOFFSET);
        struct wl_info *wl = wlc->wl;

        if (p_new != 0) {
            memcpy(p_new->data, p->data + FRAMEOFFSET, p->len - FRAMEOFFSET);
            wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p_new);
        }
    }

    wlc_recv(wlc, p);
}

__attribute__((at(0x1BB6FE, "", CHIP_VER_BCM4361b0, FW_VER_13_38_55_1_sta)))
BLPatch(wlc_recv_hook, wlc_recv_hook);
