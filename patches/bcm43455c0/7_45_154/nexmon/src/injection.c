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
#include <ieee80211_radiotap.h>
#include <sendframe.h>
#include "d11.h"
#include "brcm.h"

int
inject_frame(struct wl_info *wl, sk_buff *p) {
    int rtap_len = 0;

    //needed for sending:
    struct wlc_info *wlc = wl->wlc;
    int data_rate = 0;
    //Radiotap parsing:
    struct ieee80211_radiotap_iterator iterator;
    struct ieee80211_radiotap_header *rtap_header;

    //parse radiotap header
    rtap_len = *((char *)(p->data + 2));

    rtap_header = (struct ieee80211_radiotap_header *) p->data;

    int ret = ieee80211_radiotap_iterator_init(&iterator, rtap_header, rtap_len, 0);

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

    //remove radiotap header
    skb_pull(p, rtap_len);

    //inject frame without using the queue
    sendframe(wlc, p, 1, data_rate);

    return 0;
}

int
wl_send_hook(struct hndrte_dev *src, struct hndrte_dev *dev, struct sk_buff *p)
{
    struct wl_info *wl = (struct wl_info *) dev->softc;
    struct wlc_info *wlc = wl->wlc;

    if (wlc->monitor && p != 0 && p->data != 0 && ((short *) p->data)[0] == 0) {
        return inject_frame(wl, p);
    } else {
        return wl_send(src, dev, p);
    }
}

__attribute__((at(0x39674, "", CHIP_VER_BCM43430a1, FW_VER_7_45_41_46)))
__attribute__((at(0x1FE4B0, "", CHIP_VER_BCM43455c0, FW_VER_7_45_154)))
GenericPatch4(wl_send_hook, wl_send_hook + 1);
