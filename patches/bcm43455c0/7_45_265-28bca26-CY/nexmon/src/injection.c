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

#include <firmware_version.h>
#include <wrapper.h>	// wrapper definitions for functions that already exist in the firmware
#include <structs.h>	// structures that are used by the code in the firmware
#include <patcher.h>
#include <helper.h>
#include <ieee80211_radiotap.h>
#include <sendframe.h>

/*
 * Read the radiotap it_len field: a little-endian u16 at offset 2 of the
 * header (it_version, it_pad, it_len[2], it_present[4]).
 *
 * Upstream reads this as *((char *)(p->data + 2)) - a *signed* single byte.
 * That is wrong twice over: it takes only the low half of the u16, so any
 * header of 256 bytes or more loses its high byte, and it sign-extends, so
 * any header from 128..255 bytes comes out negative. The bad value is then
 * handed to ieee80211_radiotap_iterator_init() and, worse, to skb_pull(),
 * which walks p->data off the front of the buffer and leaves p->len
 * underflowed. The corrupted skb reaches wlc_sendctl() and the dongle takes
 * a data abort inside the TX path.
 */
static inline int
rtap_len_of(struct sk_buff *p)
{
    unsigned char *d = (unsigned char *) p->data;

    return d[2] | (d[3] << 8);
}

int
inject_frame(struct wl_info *wl, sk_buff *p, int rtap_len) {
    //needed for sending:
    struct wlc_info *wlc = wl->wlc;
    int data_rate = 0;
    //Radiotap parsing:
    struct ieee80211_radiotap_iterator iterator;
    struct ieee80211_radiotap_header *rtap_header;

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

    /*
     * ((short *) p->data)[0] == 0 only says the first two bytes are zero,
     * which is true of a radiotap header (it_version 0, it_pad 0) but also of
     * any other frame that happens to start 00 00. Validate the length field
     * before committing to the radiotap path: it has to be at least a bare
     * radiotap header and cannot claim more bytes than the skb actually holds.
     * Anything else goes out the normal TX path instead of being pulled apart
     * as a radiotap frame - which is both correct for a non-injected frame and
     * keeps a malformed header from reaching skb_pull()/wlc_sendctl().
     */
    if (wlc->monitor && p != 0 && p->data != 0 &&
        p->len >= sizeof(struct ieee80211_radiotap_header) &&
        ((short *) p->data)[0] == 0) {
        int rtap_len = rtap_len_of(p);

        if (rtap_len >= (int) sizeof(struct ieee80211_radiotap_header) &&
            rtap_len <= (int) p->len) {
            return inject_frame(wl, p, rtap_len);
        }
    }

    return wl_send(src, dev, p);
}

__attribute__((at(0x200E20, "", CHIP_VER_BCM43455c0, FW_VER_7_45_265_28bca26_CY)))
GenericPatch4(wl_send_hook, wl_send_hook + 1);
