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
 * Warning:                                                                *
 *                                                                         *
 * Our software may damage your hardware and may void your hardware’s      *
 * warranty! You use our tools at your own risk and responsibility!        *
 *                                                                         *
 * License:                                                                *
 * Copyright (c) 2015 NexMon Team                                          *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining   *
 * a copy of this software and associated documentation files (the         *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute copies of the Software, and to permit persons to whom the    *
 * Software is furnished to do so, subject to the following conditions:    *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * Any use of the Software which results in an academic publication or     *
 * other publication which includes a bibliography must include a citation *
 * to the author's publication "M. Schulz, D. Wegemer and M. Hollick.      *
 * NexMon: A Cookbook for Firmware Modifications on Smartphones to Enable  *
 * Monitor Mode.".                                                         *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
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

#define RADIOTAP_MCS
#define RADIOTAP_VENDOR
#include <ieee80211_radiotap.h>

// plcp length in bytes
#define PLCP_LEN 6

int
channel2freq(struct wl_info *wl, unsigned int channel)
{
    int freq = 0;
    void *ci = 0;

    wlc_phy_chan2freq_acphy(wl->wlc->band->pi, channel, &freq, &ci);

    return freq;
}

void
wl_monitor_hook(struct wl_info *wl, struct wl_rxsts *sts, struct sk_buff *p)
{
    struct osl_info *osh = wl->wlc->osh;
    unsigned int p_len_new = p->len + sizeof(struct nexmon_radiotap_header);
    struct sk_buff *p_new;

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

    struct nexmon_radiotap_header *frame = (struct nexmon_radiotap_header *) p_new->data;

    memset(p_new->data, 0, sizeof(struct nexmon_radiotap_header));

    frame->header.it_version = 0;
    frame->header.it_pad = 0;
    frame->header.it_len = sizeof(struct nexmon_radiotap_header) + PLCP_LEN;
    frame->header.it_present = 
          (1<<IEEE80211_RADIOTAP_TSFT) 
        | (1<<IEEE80211_RADIOTAP_FLAGS)
        | (1<<IEEE80211_RADIOTAP_RATE)
        | (1<<IEEE80211_RADIOTAP_CHANNEL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTSIGNAL)
        | (1<<IEEE80211_RADIOTAP_DBM_ANTNOISE)
        | (1<<IEEE80211_RADIOTAP_MCS)
        | (1<<IEEE80211_RADIOTAP_VENDOR_NAMESPACE);
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

    if (sts->encoding == WL_RXS_ENCODING_HT) {
        frame->mcs[0] = 
              IEEE80211_RADIOTAP_MCS_HAVE_BW
            | IEEE80211_RADIOTAP_MCS_HAVE_MCS
            | IEEE80211_RADIOTAP_MCS_HAVE_GI
            | IEEE80211_RADIOTAP_MCS_HAVE_FMT
            | IEEE80211_RADIOTAP_MCS_HAVE_FEC
            | IEEE80211_RADIOTAP_MCS_HAVE_STBC;
        switch(sts->htflags) {
            case WL_RXS_HTF_40:
                frame->mcs[1] |= IEEE80211_RADIOTAP_MCS_BW_40;
                break;
            case WL_RXS_HTF_20L:
                frame->mcs[1] |= IEEE80211_RADIOTAP_MCS_BW_20L;
                break;
            case WL_RXS_HTF_20U:
                frame->mcs[1] |= IEEE80211_RADIOTAP_MCS_BW_20U;
                break;
            case WL_RXS_HTF_SGI:
                frame->mcs[1] |= IEEE80211_RADIOTAP_MCS_SGI;
                break;
            case WL_RXS_HTF_STBC_MASK:
                frame->mcs[1] |= ((sts->htflags & WL_RXS_HTF_STBC_MASK) >> WL_RXS_HTF_STBC_SHIFT) << IEEE80211_RADIOTAP_MCS_STBC_SHIFT;
                break;
            case WL_RXS_HTF_LDPC:
                frame->mcs[1] |= IEEE80211_RADIOTAP_MCS_FEC_LDPC;
                break;
        }
        frame->mcs[2] = sts->mcs;
    }

    frame->vendor_oui[0] = 'N';
    frame->vendor_oui[1] = 'E';
    frame->vendor_oui[2] = 'X';
    frame->vendor_sub_namespace = 0;
    frame->vendor_skip_length = PLCP_LEN;


    memcpy(p_new->data + sizeof(struct nexmon_radiotap_header), p->data, p->len);

    wl_sendup(wl, 0, p_new);
}

// Hook the call to wl_monitor in wlc_monitor
__attribute__((at(0x18DA30, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_40_r581243)))
__attribute__((at(0x18DB20, "", CHIP_VER_BCM4339, FW_VER_6_37_32_RC23_34_43_r639704)))
__attribute__((at(0x1f0a6, "flashpatch", CHIP_VER_BCM4358, FW_VER_7_112_200_17)))
BLPatch(wl_monitor_hook, wl_monitor_hook);
