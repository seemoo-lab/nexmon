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
#include <ieee80211_radiotap.h> // Radiotap header relateds

extern void *inject_frame(struct wlc_info *wlc, struct sk_buff *p);

struct inject_frame {
    unsigned short len;
    unsigned char pad;
    unsigned char type;
    char data[];
};

int 
wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if)
{
    int ret = IOCTL_ERROR;

    switch (cmd) {
        case NEX_GET_CAPABILITIES:
            if (len == 4) {
                memcpy(arg, &capabilities, 4);
                ret = IOCTL_SUCCESS;
            }
            break;

        case NEX_WRITE_TO_CONSOLE:
            if (len > 0) {
                arg[len-1] = 0;
                printf("ioctl: %s\n", arg);
                ret = IOCTL_SUCCESS; 
            }
            break;

        case NEX_GET_VERSION_STRING:
            {
                int strlen = 0;
                for ( strlen = 0; version[strlen]; ++strlen );
                if (len >= strlen) {
                    memcpy(arg, version, strlen);
                    ret = IOCTL_SUCCESS;
                }
            }
            break;
			
		case NEX_INJECT_FRAME:
            {
                sk_buff *p;
                int bytes_used = 0;
                struct inject_frame *frm = (struct inject_frame *) arg;

                while ((frm->len > 0) && (bytes_used + frm->len <= len)) {
                    // add a dummy radiotap header if frame does not contain one
                    if (frm->type == 0) {
                        p = pkt_buf_get_skb(wlc->osh, frm->len + 202 + 8 - 4);
                        skb_pull(p, 202);
                        struct ieee80211_radiotap_header *radiotap = 
                            (struct ieee80211_radiotap_header *) p->data;
                        
                        memset(radiotap, 0, sizeof(struct ieee80211_radiotap_header));
                        
                        radiotap->it_len = 8;
                        
                        skb_pull(p, 8);
                        memcpy(p->data, frm->data, frm->len - 4);
                        skb_push(p, 8);
                    } else {
                        p = pkt_buf_get_skb(wlc->osh, frm->len + 202 - 4);
                        skb_pull(p, 202);

                        memcpy(p->data, frm->data, frm->len - 4);
                    }

                    inject_frame(wlc, p);

                    bytes_used += frm->len;

                    frm = (struct inject_frame *) (arg + bytes_used);
                }

                ret = IOCTL_SUCCESS;
            }
            break;

        default:
            ret = wlc_ioctl(wlc, cmd, arg, len, wlc_if);
    }

    return ret;
}

__attribute__((at(0x2061B0, "", CHIP_VER_BCM43455, FW_VER_7_45_77_0_23_8_2017)))
GenericPatch4(wlc_ioctl_hook, wlc_ioctl_hook + 1);
