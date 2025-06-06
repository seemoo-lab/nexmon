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
#include <bcmwifi_channels.h>
#include <monitormode.h>        // defitionons such as MONITOR_...

#include <ieee80211_radiotap.h>

#define PLCP_HDR_LEN 6

static void
wl_sendup_multiif(struct wl_info *wl, struct sk_buff *p)
{
    if (wl->wlc->wlcif_list->wlif) {
        wl->wlc->wlcif_list->wlif->dev->chained->funcs->xmit(wl->wlc->wlcif_list->wlif->dev, wl->wlc->wlcif_list->wlif->dev->chained, p);
    } else {
        wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p);
    }
}

static int
channel2freq(struct wl_info *wl, unsigned int channel)
{
    int freq = 0;
    void *ci = 0;

    wlc_phy_chan2freq_acphy(wl->wlc->band->pi, channel, &freq, &ci);

    return freq;
}

static void
wl_monitor_radiotap(struct wl_info *wl, struct wl_rxsts *sts, struct sk_buff *p) {
    skb_pull(p, PLCP_HDR_LEN);
    void *head = (void *) (((uint32_t) p->data & 0xFFE00000) + p->head_off);

    if (p->data - head < sizeof(struct nexmon_radiotap_header))
    {
        printf("%s: no space for header\n", __FUNCTION__);
        return;
    }

    struct nexmon_radiotap_header *frame = (struct nexmon_radiotap_header *) skb_push(p, sizeof(struct nexmon_radiotap_header));

    memset(frame, 0, sizeof(struct nexmon_radiotap_header));

    frame->header.it_version = 0;
    frame->header.it_pad = 0;
    frame->header.it_len = sizeof(struct nexmon_radiotap_header);
    frame->header.it_present =
          (1<<IEEE80211_RADIOTAP_TSFT)
        | (1<<IEEE80211_RADIOTAP_FLAGS)
        | (1<<IEEE80211_RADIOTAP_RATE)
        | (1<<IEEE80211_RADIOTAP_CHANNEL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTSIGNAL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTNOISE);
    frame->tsf.tsf_l = sts->mactime;
    frame->tsf.tsf_h = 0;
    frame->flags = IEEE80211_RADIOTAP_F_FCS;
    frame->chan_freq = channel2freq(wl, CHSPEC_CHANNEL(sts->chanspec));

    if (frame->chan_freq > 3000)
        frame->chan_flags |= IEEE80211_CHAN_5GHZ;
    else
        frame->chan_flags |= IEEE80211_CHAN_2GHZ;

    if (sts->encoding == WL_RXS_ENCODING_OFDM)
        frame->chan_flags |= IEEE80211_CHAN_OFDM;
    if (sts->encoding == WL_RXS_ENCODING_DSSS_CCK)
        frame->chan_flags |= IEEE80211_CHAN_CCK;

    frame->data_rate = sts->datarate;

    frame->dbm_antsignal = sts->signal;
    frame->dbm_antnoise = sts->noise;

    p->flags |= 0x80u;
    wl_sendup_multiif(wl, p);
}

void
wl_monitor_hook(struct wl_info *wl, struct wl_rxsts *sts, struct sk_buff *p) {
    unsigned char monitor = wl->wlc->monitor & 0xFF;

    if (monitor & MONITOR_STS) {
        void *head = (void *) (((uint32_t) p->data & 0xFFE00000) + p->head_off);

        if (p->data - head < sizeof(struct wl_rxsts))
        {
            printf("%s: no space for header\n", __FUNCTION__);
            return;
        }

        skb_push(p, sizeof(struct wl_rxsts));
        memcpy(p->data, sts, sizeof(struct wl_rxsts));
        p->flags |= 0x80u;
        wl_sendup_multiif(wl, p);
    }

    if (monitor & MONITOR_RADIOTAP) {
        wl_monitor_radiotap(wl, sts, p);
    }

    if (monitor & MONITOR_IEEE80211) {
        skb_pull(p, PLCP_HDR_LEN);
        p->flags |= 0x80u;
        wl_sendup_multiif(wl, p);
    }

    if (monitor & MONITOR_LOG_ONLY) {
        printf("frame received\n");
    }

    if (monitor & MONITOR_DROP_FRM) {
        ;
    }

    if (monitor & MONITOR_IPV4_UDP) {
        printf("MONITOR over udp is not supported!\n");
    }
}

// Hook the call to wl_monitor in wlc_monitor
__attribute__((at(0x1A6C98, "", CHIP_VER_BCM43455c0, FW_VER_7_45_206)))
BLPatch(wl_monitor_hook, wl_monitor_hook);

// when the hnd_pkt_dup function is called in wlc_monitor_amsdu, we want to reserve enough space to prepend our header
sk_buff *
hnd_pkt_dup_hook(struct osl_info *osh, sk_buff *p)
{
    sk_buff *pnew;

    pnew = lb_alloc(p->len + sizeof(struct nexmon_radiotap_header), 0);

    if ( pnew ) {
        skb_pull(pnew, sizeof(struct nexmon_radiotap_header));
        memcpy(pnew->data, p->data, p->len);
        osh->pktalloced++;
    }

    return pnew;
}

__attribute__((at(0x25AF2, "flashpatch", CHIP_VER_BCM43455c0, FW_VER_ALL)))
BLPatch(hnd_pkt_dup_hook, hnd_pkt_dup_hook);

//Temporary fix to ignore A-MSDU frames
__attribute__((at(0x1B2A46, "", CHIP_VER_BCM43455c0, FW_VER_7_45_206)))
BPatch(wlc_monitor_amsdu_patch, 0x1B2A62);
