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
#include "bcm43438.h"
#include <ieee80211_radiotap.h>
#include "d11.h"
#include "brcm.h"

struct brcmf_proto_bcdc_header {
	unsigned char flags;
	unsigned char priority;
	unsigned char flags2;
	unsigned char data_offset;
};

struct bdc_radiotap_header {
    struct brcmf_proto_bcdc_header bdc;
    struct nexmon_radiotap_header radiotap;
} __attribute__((packed));

void
wl_monitor_hook(struct wl_info *wl, struct wl_rxsts *sts, struct sk_buff *p) {
    struct sk_buff *p_new = pkt_buf_get_skb(OSL_INFO_ADDR, p->len + sizeof(struct bdc_radiotap_header));
    struct bdc_radiotap_header *frame = (struct bdc_radiotap_header *) p_new->data;

    struct tsf tsf;
	wlc_bmac_read_tsf(wl->wlc_hw, &tsf.tsf_l, &tsf.tsf_h);

	memset(p_new->data, 0, sizeof(struct bdc_radiotap_header));

    frame->bdc.flags = 0x20;
    frame->bdc.priority = 0;
    frame->bdc.flags2 = 0;
    frame->bdc.data_offset = 0;

    frame->radiotap.header.it_version = 0;
    frame->radiotap.header.it_pad = 0;
    frame->radiotap.header.it_len = sizeof(struct nexmon_radiotap_header);
    frame->radiotap.header.it_present = 
          (1<<IEEE80211_RADIOTAP_TSFT) 
        | (1<<IEEE80211_RADIOTAP_FLAGS)
        | (1<<IEEE80211_RADIOTAP_CHANNEL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTSIGNAL);
    frame->radiotap.tsf.tsf_l = tsf.tsf_l;
    frame->radiotap.tsf.tsf_h = tsf.tsf_h;
    frame->radiotap.flags = IEEE80211_RADIOTAP_F_FCS;
    frame->radiotap.chan_freq = wlc_phy_channel2freq(CHSPEC_CHANNEL(sts->chanspec));
    frame->radiotap.chan_flags = 0;
    frame->radiotap.dbm_antsignal = sts->rssi;
	
	memcpy(p_new->data + sizeof(struct bdc_radiotap_header), p->data + 6, p->len - 6);

	p_new->len -= 6;
	dngl_sendpkt(SDIO_INFO_ADDR, p_new, 2);
}

__attribute__((at(0x81F620, "flashpatch")))
BLPatch(flash_patch_179, wl_monitor_hook);
