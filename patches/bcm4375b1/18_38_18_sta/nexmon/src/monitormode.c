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
#include <local_wrapper.h>

#define RXS_SHORT_MASK      0x01
#define RXS_PBPRES          (1 << 2)
#define RXSS_PBPRES         (1 << 3)
#define PLCP_LEN            6

struct d11rxhdr {
    uint16 RxFrameSize;     // 0x00
    uint8 dma_flags;        // 0x02
    uint8 fifo;             // 0x03
    uint16 PhyRxStatus_0;   // 0x04
    uint16 PhyRxStatus_1;   // 0x06
    uint16 PhyRxStatus_2;   // 0x08
    uint16 PhyRxStatus_3;   // 0x0A
    uint16 PhyRxStatus_4;   // 0x0C
    uint16 PhyRxStatus_5;   // 0x0E
    uint16 RxStatus1;       // 0x10
    uint16 RxStatus2;       // 0x12
    uint16 RxTSFTime;       // 0x14
    uint16 RxChan;          // 0x16
    uint16 RxFrameSize_0;   // 0x18 somehow this seems to be the RxChan
    uint16 HdrConvSt;       // 0x1A
    uint16 AvbRxTimeL;      // 0x1C
    uint16 AvbRxTimeH;      // 0x1E somehow this seems to fit
    uint16 MuRate;          // 0x20
    uint16 errflags;        // 0x22
} __attribute__((packed));


struct wlc_d11rxhdr {
    uint32 tsf_l;
    int8 rssi;
    int8 rssi_qdb;
    uint8 pad[2];
    int8 rxpwr[4];
    struct d11rxhdr rxhdr;
} __attribute__((packed));

struct myradiotaphdr {
    struct ieee80211_radiotap_header rtap;
    struct tsf tsf;
    unsigned short chan_freq;
    unsigned short chan_flags;
    char dbm_antsignal;
    char dbm_antnoise;
    uint16_t PAD;
    struct xchan xchan;
} __attribute__((packed));

struct wlc_monitor_info *
wlc_monitor_attach(struct wlc_info *wlc) {
    struct wlc_monitor_info *mon_info = (struct wlc_monitor_info *) osl_mallocz(32);
    if (!mon_info) return 0;
    mon_info->buf_32bytes = osl_mallocz(32);
    if (!mon_info->buf_32bytes) return 0;
    mon_info->buf_12bytes = (struct wlc_monitor_info *) osl_mallocz(12);
    if (!mon_info->buf_12bytes) return 0;

    mon_info->wlc = wlc;
    mon_info->promisc_bits = 0;
    mon_info->timer_active = 0;
    mon_info->mon_cal_timer = 0; // should be set to a timer
    mon_info->chanspec = 0;

    return mon_info;
}

void *
wlc_okc_attach_hook(struct wlc_info *wlc) {
    struct wlc_monitor_info *mon_info = wlc_monitor_attach(wlc);
    if (mon_info) {
        wlc->mon_info = mon_info;
        wlc->pub->_mon = 1;
    }

    return wlc_okc_attach(wlc);
}

void
wlc_tunables_init_hook(struct tunables *tunables, void *cmn, int a3, int a4) {
    wlc_tunables_init(tunables, cmn, a3, a4);
//    tunables->copycount = 2;
}

int
path_to_path_to_indirect_call_of_phy_ac_rssi_compute_hook(void *rssi_related, struct wlc_d11rxhdr *wrxh, void *a3) {
    struct sk_buff *p;
    struct wlc_info *wlc;

    __asm("mov %0, r4" : "=r" (p));
    __asm("mov %0, r5" : "=r" (wlc));

    path_to_path_to_indirect_call_of_phy_ac_rssi_compute(rssi_related, wrxh, a3);

    if (wlc->monitor == 2) {
        struct osl_info *osh = wlc->osh;
        struct wl_info *wl = wlc->wl;
        struct d11rxhdr *rxh = &wrxh->rxhdr;
        uint32_t isamsdu = (rxh->PhyRxStatus_0 & 1) != 0;
        int len = p->len;

        if (!isamsdu)
            pktfrag_trim_tailbytes(osh, p, 4, 1);

        int additional_padding = 0;

        if (p->len == 6) {
            skb_pull(p, 6);

            p->frag_used_len += 4;
            additional_padding = 4;
        } else if (p->len == 8) {
            skb_pull(p, 8);

            p->frag_used_len += 4;
            additional_padding = 2;
        } else {
            additional_padding = 6;
        }

        skb_push(p, sizeof(struct myradiotaphdr));

        struct myradiotaphdr *myrtap = (struct myradiotaphdr *) p->data;
        memset(myrtap, 0, sizeof(struct myradiotaphdr));

        myrtap->rtap.it_version = 0;
        myrtap->rtap.it_pad = len;
        myrtap->rtap.it_len = sizeof(struct myradiotaphdr) + additional_padding;

        myrtap->rtap.it_present = 
          (1<<IEEE80211_RADIOTAP_TSFT)
        | (1<<IEEE80211_RADIOTAP_CHANNEL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTSIGNAL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTNOISE)
        | (1<<IEEE80211_RADIOTAP_XCHANNEL);

        uint16_t chanspec = rxh->RxFrameSize_0;
        if (CHSPEC_IS2G(chanspec)) {
            myrtap->chan_flags = IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_DYN;
            myrtap->chan_freq = wf_channel2mhz(wf_chspec_ctlchan(chanspec), WF_CHAN_FACTOR_2_4_G);
        } else {
            myrtap->chan_flags = IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM;
            myrtap->chan_freq = wf_channel2mhz(wf_chspec_ctlchan(chanspec), WF_CHAN_FACTOR_5_G);
        }

        wlc_recover_tsf64(wlc, wrxh, &myrtap->tsf.tsf_h, &myrtap->tsf.tsf_l);
        myrtap->tsf.tsf_h = 0;

        myrtap->dbm_antsignal = wrxh->rssi;
        myrtap->dbm_antnoise = phy_noise_avg(wlc->pi);

        myrtap->xchan.xchannel_flags = myrtap->chan_flags;
        if (CHSPEC_IS40(chanspec)) {
            if (CHSPEC_SB_UPPER(chanspec)) {
                myrtap->xchan.xchannel_flags |= IEEE80211_CHAN_HT40D;
            } else {
                myrtap->xchan.xchannel_flags |= IEEE80211_CHAN_HT40U;
            }
        } else {
            myrtap->xchan.xchannel_flags |= IEEE80211_CHAN_HT20;
        }

        myrtap->xchan.xchannel_freq = myrtap->chan_freq;
        myrtap->xchan.xchannel_channel = wf_chspec_ctlchan(chanspec);
        myrtap->xchan.xchannel_maxpower = (17*2);

        wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p);
    }

    return wlc->monitor;
}

__attribute__((at(0x2170DA, "", CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta)))
BLPatch(path_to_path_to_indirect_call_of_phy_ac_rssi_compute_hook, path_to_path_to_indirect_call_of_phy_ac_rssi_compute_hook);

__attribute__((naked))
void
exit_wlc_recv(uint32_t monitor)
{
    asm(
        "cmp r0, #2\n"              // path_to_path_to_indirect_call_of_phy_ac_rssi_compute_hook return value of wlc->monitor in r0
        "beq 0x21726C\n"            // if wlc->monitor == 2 then quit wlc_recv without further processing p
        "ldrb r2, [r6,#0xE]\n"      // overwritten instruction
        "lsls r1, r2, #0x1F\n"      // overwritten instruction
        "b 0x2170E4\n"              // branch to address after overwritten instructions
    );
}

__attribute__((at(0x2170E0, "", CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta)))
BPatch(exit_wlc_recv, exit_wlc_recv);

__attribute__((at(0x24983E, "", CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta)))
BLPatch(wlc_okc_attach_hook, wlc_okc_attach_hook);

__attribute__((at(0x24BBB4, "", CHIP_VER_BCM4375b1, FW_VER_18_38_18_sta)))
BLPatch(wlc_tunables_init_hook, wlc_tunables_init_hook);
