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
#include <ieee80211_radiotap.h> // radiotap header related
#include <vendor_radiotap.h>    // vendor specific radiotap extensions

char
sendframe(struct wlc_info *wlc, struct sk_buff *p, unsigned int fifo, unsigned int rate);

inline uint32_t
get_unaligned_le32(void *p) {
    return ((uint8 *) p)[0] | ((uint8 *) p)[1] << 8 | ((uint8 *) p)[2] << 16 | ((uint8 *) p)[3] << 24;
}

void *
inject_frame(struct wlc_info *wlc, struct sk_buff *p)
{
    int rtap_len = 0;
    int data_rate = 0;
    unsigned char use_ratespec = 0;
    //int txdelay = 0;
    //int txrepetitions = 0;
    //int txperiodicity = 0;

    // Radiotap parsing:
    struct ieee80211_radiotap_iterator iterator;
    struct ieee80211_radiotap_header *rtap_header;

    // parse radiotap header
    rtap_len = *((char *)(p->data + 2));
    rtap_header = (struct ieee80211_radiotap_header *) p->data;
    
    int ret = ieee80211_radiotap_iterator_init(&iterator, rtap_header, rtap_len, &rtap_vendor_namespaces);
    
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

        if (iterator.current_namespace == &rtap_vendor_namespaces.ns[0]) {
            switch(iterator.this_arg_index) {
                case RADIOTAP_NEX_TXDELAY:
                    //txdelay = get_unaligned_le32(iterator.this_arg);
                    break;

                case RADIOTAP_NEX_TXREPETITIONS:
                    //txrepetitions = get_unaligned_le32(iterator.this_arg);
                    //txperiodicity = get_unaligned_le32(iterator.this_arg + 4);
                    break;

                case RADIOTAP_NEX_RATESPEC:
                    data_rate = get_unaligned_le32(iterator.this_arg);
                    use_ratespec = 1; // this will override the rate of the regular radiotap header
                    break;

                default:
                    printf("unknows vendor field %d\n", iterator.this_arg_index);
            }

        } else if (iterator.current_namespace == &radiotap_ns) {
            switch(iterator.this_arg_index) {
                case IEEE80211_RADIOTAP_RATE:
                    if (!use_ratespec) {
                        data_rate = (*iterator.this_arg);
                    }
                    break;

                case IEEE80211_RADIOTAP_CHANNEL:
                    //printf("Channel (freq): %d\n", iterator.this_arg[0] | (iterator.this_arg[1] << 8) );
                    break;

                default:
                    //printf("default: %d\n", iterator.this_arg_index);
                    break;
            }
        }
    }

    // remove radiotap header
    skb_pull(p, rtap_len);

    sendframe(wlc, p, 1, data_rate);

    return 0;
}
