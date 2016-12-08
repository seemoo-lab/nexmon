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
#include <bcmwifi_channels.h>
#include <ieee80211_radiotap.h>
#include <sendframe.h>          // sendframe functionality

void *
inject_frame(struct wlc_info *wlc, struct sk_buff *p)
{
    int rtap_len = 0;
    int data_rate = 0;

    // Radiotap parsing:
    struct ieee80211_radiotap_iterator iterator;
    struct ieee80211_radiotap_header *rtap_header;

    // remove bdc header
    skb_pull(p, 4);

    // parse radiotap header
    rtap_len = *((char *)(p->data + 2));
    rtap_header = (struct ieee80211_radiotap_header *) p->data;
    
    int ret = ieee80211_radiotap_iterator_init(&iterator, rtap_header, rtap_len);
    
    if(ret) {
        pkt_buf_free_skb(wlc->osh, p, 0);
        printf("rtap_init error\n");
        return 0;
    }

    while(!ret) {
        ret = ieee80211_radiotap_iterator_next(&iterator);
        if(ret) {
            continue;
        }
        switch(iterator.this_arg_index) {
            case IEEE80211_RADIOTAP_RATE:
                data_rate = (*iterator.this_arg);
                break;
            case IEEE80211_RADIOTAP_CHANNEL:
                //printf("Channel (freq): %d\n", iterator.this_arg[0] | (iterator.this_arg[1] << 8) );
                break;
            default:
                //printf("default: %d\n", iterator.this_arg_index);
                break;
        }
    }

    // remove radiotap header
    skb_pull(p, rtap_len);

    sendframe(wlc, p, 1, data_rate);

    return 0;
}

void *
handle_sdio_xmit_request_hook(void *sdio_hw, struct sk_buff *p)
{
    struct wl_info *wl = *(*((struct wl_info ***) sdio_hw + 15) + 6);
    struct wlc_info *wlc = wl->wlc;

    if (wlc->monitor && p != 0 && p->data != 0 && ((short *) p->data)[2] == 0) {
        // check if in monitor mode and if first two bytes in the frame correspond to a radiotap header, if true, inject frame
        return inject_frame(wlc, p);
    } else {
        // otherwise, handle frame normally
        return handle_sdio_xmit_request_ram(sdio_hw, p);
    }
}

// Hook the call to handle_sdio_xmit_request_hook in sdio_header_parsing_from_sk_buff
__attribute__((at(0x182AAA, "", CHIP_VER_BCM4339, FW_VER_ALL)))
BPatch(handle_sdio_xmit_request_hook, handle_sdio_xmit_request_hook);

// Replace the entry in the function pointer table by handle_sdio_xmit_request_hook
__attribute__((at(0x180BCC, "", CHIP_VER_BCM4339, FW_VER_ALL)))
GenericPatch4(handle_sdio_xmit_request_hook, handle_sdio_xmit_request_hook + 1);
