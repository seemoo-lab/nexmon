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
#include <sendframe.h>          // sendframe functionality
#include <version.h>            // version information
//#include <bcmpcie.h>
#include <argprintf.h>          // allows to execute argprintf to print into the arg buffer
#include <local_wrapper.h>
#include <ieee80211_radiotap.h>
#include <udptunnel.h>
#include <bcmwifi_channels.h>

#define TXOFF 204

extern char version[];
extern char date[];
extern char time[];

struct inject_frame {
    struct {
    unsigned short len;
    unsigned char pad;
    unsigned char type;
    } hdr;
    char data[];
};

void *inject_frame(struct wlc_info *wlc, struct sk_buff *p);

uint8_t eapol1[] = {
  0x08, 0x02, 0x3a, 0x01, 0x82, 0x3e, 0x7a, 0x30, 0x6c, 0xa8, 0x64, 0x66,
  0xb3, 0xce, 0x22, 0xa3, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa3, 0x00, 0x00,
  0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e, 0x02, 0x03, 0x00, 0x5f,
  0x02, 0x00, 0x8a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x3b, 0x75, 0xe7, 0x7e, 0x43, 0x52, 0x48, 0xa5, 0x8d, 0x20, 0x70,
  0xb0, 0x22, 0xc5, 0x49, 0xeb, 0xf3, 0x45, 0x74, 0x78, 0xef, 0x38, 0x3a,
  0xf0, 0x11, 0x64, 0x2e, 0xa6, 0xd7, 0xa9, 0xbe, 0xcc, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t eapol2[] = {
  0x08, 0x01, 0x3a, 0x01, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa3, 0x82, 0x3e,
  0x7a, 0x30, 0x6c, 0xa8, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa3, 0x30, 0x00,
  0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e, 0x01, 0x03, 0x00, 0x75,
  0x02, 0x01, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0xe4, 0xf9, 0x79, 0x6c, 0x38, 0x84, 0x66, 0x36, 0xa8, 0x8e, 0x78,
  0x87, 0xb1, 0x0d, 0x19, 0xc1, 0xca, 0x2a, 0x50, 0xa0, 0x28, 0xb5, 0x8d,
  0x88, 0x37, 0x1c, 0x1f, 0xe5, 0x82, 0x84, 0x36, 0x11, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x3a, 0x55, 0x20, 0x5f, 0x00, 0x4a,
  0xcd, 0x61, 0x44, 0x23, 0x55, 0x5f, 0x81, 0xbf, 0x3e, 0x00, 0x16, 0x30,
  0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac,
  0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x80, 0x00
};

uint8_t eapol3[] = {
  0x08, 0x02, 0x3a, 0x01, 0x82, 0x3e, 0x7a, 0x30, 0x6c, 0xa8, 0x64, 0x66,
  0xb3, 0xce, 0x22, 0xa3, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa3, 0x10, 0x00,
  0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e, 0x02, 0x03, 0x00, 0x97,
  0x02, 0x13, 0xca, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x3b, 0x75, 0xe7, 0x7e, 0x43, 0x52, 0x48, 0xa5, 0x8d, 0x20, 0x70,
  0xb0, 0x22, 0xc5, 0x49, 0xeb, 0xf3, 0x45, 0x74, 0x78, 0xef, 0x38, 0x3a,
  0xf0, 0x11, 0x64, 0x2e, 0xa6, 0xd7, 0xa9, 0xbe, 0xcc, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x95, 0xba, 0x3c, 0xcb, 0xac, 0x58, 0x4a,
  0x25, 0xa6, 0xa1, 0xc8, 0x03, 0xa7, 0x39, 0xdf, 0x0f, 0x00, 0x38, 0xc5,
  0x7e, 0xae, 0xd5, 0xe3, 0x6e, 0xa1, 0x8a, 0x90, 0x3e, 0xc2, 0x7c, 0x6c,
  0xb4, 0xe9, 0xa7, 0x2b, 0x9e, 0xb2, 0x7d, 0xf8, 0x55, 0x0a, 0xb4, 0x89,
  0x2b, 0x38, 0x1e, 0xc1, 0x81, 0x0d, 0x96, 0x6c, 0x60, 0xe7, 0x8d, 0x84,
  0x80, 0x9e, 0xad, 0xc8, 0xe0, 0xe9, 0xec, 0x88, 0x14, 0xba, 0x69, 0x9d,
  0xb7, 0x3b, 0x0b, 0x10, 0xc8, 0x74, 0xc9
};

uint8_t eapol4[] = {
  0x08, 0x01, 0x3a, 0x01, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa3, 0x82, 0x3e,
  0x7a, 0x30, 0x6c, 0xa8, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa3, 0x40, 0x00,
  0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e, 0x01, 0x03, 0x00, 0x5f,
  0x02, 0x03, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0xb4, 0x07, 0x2f, 0xfc, 0x75, 0xfe,
  0x32, 0xa1, 0xda, 0xad, 0x68, 0x9c, 0xf5, 0xe3, 0xa6, 0x00, 0x00
};

uint8_t auth1[] = {
  0xb0, 0x00, 0x3c, 0x00, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa4, 0x62, 0x18,
  0xa0, 0x2e, 0x53, 0x0e, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa4, 0x40, 0x8d,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xdd, 0x09, 0x00, 0x10, 0x18, 0x02,
  0x00, 0x00, 0x10, 0x00, 0x00
};

uint8_t auth2[] = {
  0xb0, 0x00, 0x3c, 0x00, 0x62, 0x18, 0xa0, 0x2e, 0x53, 0x0e, 0x64, 0x66,
  0xb3, 0xce, 0x22, 0xa4, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa4, 0x80, 0x04,
  0x00, 0x00, 0x02, 0x00, 0x00, 0x00
};

uint8_t assoc1[] = {
  0x00, 0x00, 0x3c, 0x00, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa4, 0x62, 0x18,
  0xa0, 0x2e, 0x53, 0x0e, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa4, 0x50, 0x8d,
  0x11, 0x00, 0x0a, 0x00, 0x00, 0x04, 0x54, 0x53, 0x54, 0x35, 0x01, 0x08,
  0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c, 0x21, 0x02, 0x0a, 0x11,
  0x24, 0x0a, 0x24, 0x04, 0x34, 0x04, 0x64, 0x0c, 0x95, 0x04, 0xa5, 0x01,
  0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f,
  0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x80, 0x00, 0x3b, 0x15,
  0x73, 0x70, 0x73, 0x74, 0x75, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82,
  0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x51, 0x53, 0x54, 0x2d, 0x1a, 0x6f,
  0x00, 0x1b, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x7f, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00,
  0x20, 0xdd, 0x09, 0x00, 0x10, 0x18, 0x02, 0x00, 0x00, 0x10, 0x00, 0x00,
  0xdd, 0x07, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x01, 0x00
};

uint8_t assoc2[] = {
  0x10, 0x00, 0x3c, 0x00, 0x62, 0x18, 0xa0, 0x2e, 0x53, 0x0e, 0x64, 0x66,
  0xb3, 0xce, 0x22, 0xa4, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa4, 0x90, 0x04,
  0x11, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x01, 0x08, 0x8c, 0x12, 0x98, 0x24,
  0xb0, 0x48, 0x60, 0x6c, 0x2d, 0x1a, 0xed, 0x01, 0x1b, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3d, 0x16, 0x28, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x08, 0x04, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xdd, 0x18, 0x00, 0x50, 0xf2, 0x02,
  0x01, 0x01, 0x80, 0x00, 0x03, 0xa4, 0x00, 0x00, 0x27, 0xa4, 0x00, 0x00,
  0x42, 0x43, 0x5e, 0x00, 0x62, 0x32, 0x2f, 0x00
};

uint8_t eapol1qos[] ={
  0x88, 0x02, 0x3c, 0x00, 0x62, 0x18, 0xa0, 0x2e, 0x53, 0x0e, 0x64, 0x66,
  0xb3, 0xce, 0x22, 0xa4, 0x64, 0x66, 0xb3, 0xce, 0x22, 0xa4, 0x00, 0x00,
  0x07, 0x00, 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8e, 0x02, 0x03,
  0x00, 0x5f, 0x02, 0x00, 0x8a, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0xfb, 0x69, 0x71, 0x3f, 0xf4, 0xd1, 0x4f, 0x2b, 0x62,
  0xe5, 0xfa, 0x54, 0x79, 0x31, 0xfb, 0x71, 0x9a, 0x41, 0x5b, 0x5a, 0xc9,
  0x1a, 0xc9, 0x90, 0x6d, 0x74, 0xe2, 0xa9, 0x71, 0xef, 0x9b, 0x03, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00
};

__attribute__((naked))
int
wlc_ioctl_orig(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    asm(
        "push {r0-r2,r4-r9,lr}\n"
        "ldr r9, [sp,#40]\n"
        "b wlc_ioctl_plus8\n"
    );
}

typedef struct {
    char        *buf;
    uint32_t    buf_size;
    uint32_t    idx;
} hndrte_log_t;

extern hndrte_log_t active_log;

int
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    void *lr;
    __asm("mov %0, lr" : "=r" (lr));

    int ret = IOCTL_ERROR;
    argprintf_init(arg, len);

    switch (cmd) {
        case NEX_INJECT_FRAME:
            {
                sk_buff *p;
                int bytes_used = 0;
                struct inject_frame *frm = (struct inject_frame *) arg;

                while ((frm->hdr.len > 0) && (bytes_used + frm->hdr.len <= len)) {
                    // add a dummy radiotap header if frame does not contain one
                    if (frm->hdr.type == 0) {
                        p = pkt_buf_get_skb(wlc->osh, frm->hdr.len + TXOFF + sizeof(struct ieee80211_radiotap_header) - sizeof(frm->hdr));
                        if (p == 0) {
                            break;
                        }
                        skb_pull(p, TXOFF);
                        struct ieee80211_radiotap_header *radiotap = 
                            (struct ieee80211_radiotap_header *) p->data;
                        
                        memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
                        
                        radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
                        
                        skb_pull(p, sizeof(struct ieee80211_radiotap_header));
                        memcpy(p->data, frm->data, frm->hdr.len - sizeof(frm->hdr));
                        skb_push(p, sizeof(struct ieee80211_radiotap_header));
                    } else {
                        p = pkt_buf_get_skb(wlc->osh, frm->hdr.len + TXOFF - sizeof(frm->hdr));
                        if (p == 0) {
                            break;
                        }
                        skb_pull(p, TXOFF);

                        memcpy(p->data, frm->data, frm->hdr.len - sizeof(frm->hdr));
                    }

                    inject_frame(wlc, p);

                    bytes_used += frm->hdr.len;

                    frm = (struct inject_frame *) (arg + bytes_used);
                }

                ret = IOCTL_SUCCESS;
            }
            break;

        case WLC_GET_MONITOR:
        {
            struct wlc_info *wlc2 = (struct wlc_info*) get_other_wlc(wlc);
            uint32_t mon = 0;
            wlc_ioctl_orig(wlc2, WLC_GET_MONITOR, (char *) &mon, sizeof(mon), wlc_if);
            wlc_ioctl_orig(wlc, WLC_GET_MONITOR, arg, len, wlc_if);
            
            *(uint32_t *) arg |= mon;

            ret = IOCTL_SUCCESS;
            break;
        }

        case WLC_SET_MONITOR:
        case 0x613:
        {
            unsigned short chanspec = get_chanspec(wlc);
            struct wlc_info *wlc_for_chanspec = (struct wlc_info *) find_wlc_for_chanspec(wlc, 0, chanspec, 0, 0);
            struct wlc_info *wlc_other = (struct wlc_info*) get_other_wlc(wlc_for_chanspec);

            wlc_other->pub->tunables->copycount = 17;
            path_to_wl_set_monitor(wlc_other, 0);
            
            if (*(uint32_t *) arg == 0) {
                wlc_for_chanspec->pub->tunables->copycount = 17;
            } else {
                wlc_for_chanspec->pub->tunables->copycount = 2;
            }
            path_to_wl_set_monitor(wlc_for_chanspec, *(uint32_t *) arg);

            ret = IOCTL_SUCCESS;

            break;
        }

        case WLC_SET_VAR:
        {
            if (!strncmp(arg, "chanspec", 8)) {
                ret = wlc_ioctl_orig(wlc, WLC_SET_VAR, arg, len, wlc_if);
            } else {
                ret = wlc_ioctl_orig(wlc, WLC_SET_VAR, arg, len, wlc_if);
            }
            break;
        }

        case 0x600:
        {
            argprintf("%s\n%s\n%s\n", version, date, time);

            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x609: // return console
        {
            if (len > active_log.buf_size) len = active_log.buf_size;
            if (active_log.idx >= len) {
                memcpy(arg, &active_log.buf[active_log.idx - len], len);
            } else {
                memcpy(&arg[len - active_log.idx], active_log.buf, active_log.idx);
                memcpy(arg, &active_log.buf[active_log.buf_size - (len - active_log.idx)], len - active_log.idx);
            }
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x621:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(eapol1) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, eapol1, sizeof(eapol1));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x622:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(eapol2) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, eapol2, sizeof(eapol2));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x623:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(eapol3) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, eapol3, sizeof(eapol3));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x624:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(eapol4) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, eapol4, sizeof(eapol4));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x625:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(auth1) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, auth1, sizeof(auth1));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x626:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(auth2) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, auth2, sizeof(auth2));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x627:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(assoc1) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, assoc1, sizeof(assoc1));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x628:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(assoc2) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, assoc2, sizeof(assoc2));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        case 0x629:
        {
            struct sk_buff *p;
            p = pkt_buf_get_skb(wlc->osh, sizeof(eapol1qos) + TXOFF + sizeof(struct ieee80211_radiotap_header));
            if (p == 0) {
                break;
            }
            skb_pull(p, TXOFF);
            struct ieee80211_radiotap_header *radiotap = 
                (struct ieee80211_radiotap_header *) p->data;
            
            memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
            
            radiotap->it_len = sizeof(struct ieee80211_radiotap_header);
            
            skb_pull(p, sizeof(struct ieee80211_radiotap_header));
            memcpy(p->data, eapol1qos, sizeof(eapol1qos));
            skb_push(p, sizeof(struct ieee80211_radiotap_header));

            inject_frame(wlc, p);
            ret = IOCTL_SUCCESS;
            break;
        }

        default:
            ret = wlc_ioctl_orig(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((naked, at(0x000C4160, "flashpatch", CHIP_VER_ALL, FW_VER_ALL)))
void
B_wlc_ioctl_hook(void)
{
// HINT: the flashpatches on the BCM4375 are always 8 byte long and also aligned
// on an 8 byte boundary, hence, we need to add the bytes that will be overwritten
// by the flashpatch
    asm(
        "b wlc_ioctl_hook\n"
        "ldr r9, [sp,#40]\n"
    );
}
