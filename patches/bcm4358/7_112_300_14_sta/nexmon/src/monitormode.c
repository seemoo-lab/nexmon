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

#define MONITOR_DISABLED  0
#define MONITOR_IEEE80211 1
#define MONITOR_RADIOTAP  2
#define MONITOR_LOG_ONLY  3
#define MONITOR_DROP_FRM  4
#define MONITOR_IPV4_UDP  5

static char call_original_wl_monitor = 0;

int
channel2freq(struct wl_info *wl, unsigned int channel)
{
    int freq = 0;
    void *ci = 0;

    wlc_phy_chan2freq_acphy(wl->wlc->band->pi, channel, &freq, &ci);

    return freq;
}

void
wl_monitor_radiotap(struct wl_info *wl, struct wl_rxsts *sts, struct sk_buff *p, unsigned char tunnel_over_udp)
{
    struct osl_info *osh = wl->wlc->osh;
    unsigned int p_len_new;
    struct sk_buff *p_new;

    if (tunnel_over_udp) {
        p_len_new = p->len + sizeof(struct ethernet_ip_udp_header) + 
            sizeof(struct nexmon_radiotap_header);
    } else {
        p_len_new = p->len + sizeof(struct nexmon_radiotap_header);
    }

    // We figured out that frames larger than 2032 will not arrive in user space
    if (p_len_new > 2032) {
        printf("ERR: frame too large\n");
        return;
    } else {
        p_new = pkt_buf_get_skb(osh, p_len_new);
    }

    if (!p_new) {
        printf("ERR: no free sk_buff\n");
        return;
    }

    if (tunnel_over_udp)
        skb_pull(p_new, sizeof(struct ethernet_ip_udp_header));

    struct nexmon_radiotap_header *frame = (struct nexmon_radiotap_header *) p_new->data;

    memset(p_new->data, 0, sizeof(struct nexmon_radiotap_header));

    frame->header.it_version = 0;
    frame->header.it_pad = 0;
    frame->header.it_len = sizeof(struct nexmon_radiotap_header);
    frame->header.it_present = 
          (1<<IEEE80211_RADIOTAP_TSFT) 
        | (1<<IEEE80211_RADIOTAP_FLAGS)
        | (1<<IEEE80211_RADIOTAP_CHANNEL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTSIGNAL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTNOISE);
    frame->tsf.tsf_l = sts->mactime;
    frame->tsf.tsf_h = 0;
    frame->flags = IEEE80211_RADIOTAP_F_FCS;
    frame->chan_freq = channel2freq(wl, CHSPEC_CHANNEL(sts->chanspec));
    frame->chan_flags = 0;
    frame->dbm_antsignal = sts->signal;
    frame->dbm_antnoise = sts->noise;

    memcpy(p_new->data + sizeof(struct nexmon_radiotap_header), p->data + 6, p->len - 6);
    p_new->len -= 6;

    if (tunnel_over_udp) {
        prepend_ethernet_ipv4_udp_header(p_new);
    }

    //wl_sendup(wl, 0, p_new);
    wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p_new);
}

void
wl_monitor_hook(struct wl_info *wl, struct wl_rxsts *sts, struct sk_buff *p) {
    //printf("sts=%08x\n", (unsigned int) sts);
    switch(wl->wlc->monitor & 0xFF) {
        case MONITOR_RADIOTAP:
                wl_monitor_radiotap(wl, sts, p, 0);
            break;

        case MONITOR_IEEE80211:
                call_original_wl_monitor = 1;
                wl_monitor(wl, sts, p);
                call_original_wl_monitor = 0;
            break;

        case MONITOR_LOG_ONLY:
                printf("frame received\n");
            break;

        case MONITOR_DROP_FRM:
            break;

        case MONITOR_IPV4_UDP:
                wl_monitor_radiotap(wl, sts, p, 1);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
// as wlc_monitor is located in ROM, but in this firmware version there are no
// free ROM patches left, we needed a workaround to hook the call to wl_monitor
// instead of hooking wl_monitor, we simply hook all calls to pkt_buf_get_skb 
// and check whether the link register (lr) is set to the return address within
// wl_monitor. If this is the case, we call our own wl_monitor_hook function and
// return 0 to hinder the original wl_monitor function from creating a new 
// frame. If the link register points to another address, we simply call the 
// original pkt_buf_get_skb function. As we created a HookPatch by replacing 
// the first bytes in the function with a branch instruction, we need to 
// execute the overwritten instructions first, before continuing with the
// original pkt_buf_get_skb function.

__attribute__((at(0x18CE3C, "", CHIP_VER_BCM4358, FW_VER_7_112_300_14)))
void b_pkt_buf_get_skb(void);

__attribute__((naked))
void *
pkt_buf_get_skb_orig(void *osh, unsigned int len)
{
    asm(
        // here starts the original function
        "push {r4,lr}\n"
        "mov r2, 0\n"
        "b b_pkt_buf_get_skb + 4\n"
        );
}

__attribute__((optimize("O0")))
void *
_pkt_buf_get_skb(void *osh, unsigned int len)
{
    register unsigned int lr asm("lr");
    register void *wl asm("r6");
    register void *sp asm("sp");
    register void *p asm("r5");
    void *sts = sp + 56; // add this offset to the stack pointer to find the sts struct created in wlc_monitor

    if (lr == 0x1863f && !call_original_wl_monitor) { // called from wl_monitor
        wl_monitor_hook(wl, sts, p);
        return 0;
    } else {
        return pkt_buf_get_skb_orig(osh, len);
    }
}

__attribute__((naked)) void
b_pkt_buf_get_skb(void) { asm("b _pkt_buf_get_skb\n"); }

// Hook the call to wl_monitor in wlc_monitor
//__attribute__((at(0x1f0a6, "flashpatch", CHIP_VER_BCM4358, FW_VER_ALL)))
//BLPatch(wl_monitor_hook, wl_monitor_hook);

//__attribute__((at(0x739DC, "flashpatch", CHIP_VER_BCM4358, FW_VER_ALL)))
//GenericPatch4(xxxx, 0x0);
