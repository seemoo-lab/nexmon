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

#include <firmware_version.h>
#include <wrapper.h>	// wrapper definitions for functions that already exist in the firmware
#include <structs.h>	// structures that are used by the code in the firmware
#include <patcher.h>
#include <helper.h>
#include "d11.h"
#include "brcm.h"

//#define RADIOTAP_MCS
#include <ieee80211_radiotap.h>

#define MONITOR_DISABLED  0
#define MONITOR_IEEE80211 1
#define MONITOR_RADIOTAP  2
#define MONITOR_LOG_ONLY  3
#define MONITOR_DROP_FRM  4
#define MONITOR_IPV4_UDP  5

void
wl_monitor_radiotap(struct wl_info *wl, struct wl_rxsts *sts, struct sk_buff *p) {
    struct sk_buff *p_new = pkt_buf_get_skb(wl->wlc->osh, p->len + sizeof(struct nexmon_radiotap_header));
    struct nexmon_radiotap_header *frame = (struct nexmon_radiotap_header *) p_new->data;
    struct tsf tsf;
    wlc_bmac_read_tsf(wl->wlc_hw, &tsf.tsf_l, &tsf.tsf_h);

    frame->header.it_version = 0;
    frame->header.it_pad = 0;
    frame->header.it_len = sizeof(struct nexmon_radiotap_header);
    frame->header.it_present =
          (1<<IEEE80211_RADIOTAP_TSFT)
        | (1<<IEEE80211_RADIOTAP_FLAGS)
        | (1<<IEEE80211_RADIOTAP_CHANNEL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTSIGNAL);
    frame->tsf.tsf_l = tsf.tsf_l;
    frame->tsf.tsf_h = tsf.tsf_h;
    frame->flags = IEEE80211_RADIOTAP_F_FCS;
    frame->chan_freq = wlc_phy_channel2freq(CHSPEC_CHANNEL(sts->chanspec));
    frame->chan_flags = 0;
    frame->dbm_antsignal = sts->rssi;

	memcpy(p_new->data + sizeof(struct nexmon_radiotap_header), p->data + 6, p->len - 6);

	p_new->len -= 6;
	wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p_new);
}

void
wl_monitor_hook(struct wl_info *wl, struct wl_rxsts *sts, struct sk_buff *p) {
    switch(wl->wlc->monitor & 0xFF) {
        case MONITOR_RADIOTAP:
                wl_monitor_radiotap(wl, sts, p);
            break;

        case MONITOR_IEEE80211:
                wl_monitor(wl, sts, p);
            break;

        case MONITOR_LOG_ONLY:
                printf("frame received\n");
            break;

        case MONITOR_DROP_FRM:
            break;

        case MONITOR_IPV4_UDP:
                printf("%s: udp tunneling not implemented\n");
                // not implemented yet
            break;
    }
}

__attribute__((at(0x81F620, "flashpatch")))
BLPatch(flash_patch_179, wl_monitor_hook);
