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
#include <ieee80211_radiotap.h> // radiotap header related
#include <vendor_radiotap.h>    // vendor specific radiotap extensions
#include <sendframe.h>          // sendframe functionality
#include <securitycookie.h>     // related to securtiy cookie

#define NEXUDP_IOCTL            0
#define NEXUDP_INJECT_RADIOTAP  1

extern int wlc_ioctl_hook(struct wlc_info *wlc, int cmd, char *arg, int len, void *wlc_if);
extern void prepend_ethernet_ipv4_udp_header(struct sk_buff *p);

struct nexudp_header {
    char nex[3];
    char type;
    int securitycookie;
} __attribute__((packed));

struct nexudp_ioctl_header {
    struct nexudp_header nexudphdr;
    unsigned int cmd;
    unsigned int set;
    char payload[1];
} __attribute__((packed));

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
    int txdelay = 0;
    int txrepetitions = 0;
    int txperiodicity = 0;

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
                    txdelay = get_unaligned_le32(iterator.this_arg);
                    break;

                case RADIOTAP_NEX_TXREPETITIONS:
                    txrepetitions = get_unaligned_le32(iterator.this_arg);
                    txperiodicity = get_unaligned_le32(iterator.this_arg + 4);
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

    wlc_d11hdrs_ext(wlc, p, wlc->band->hwrs_scb, 0, 0, 1, 1, 0, 0, data_rate, 0);
    p->scb = wlc->band->hwrs_scb;
    
    // 124 bytes is d11txh length
    // 4 bytes are added in wlc_d11hdrs_ext
    // 16 bytes just for fun
    //hexdump("d11txh", p->data, 124 + 4 + 16);

    sendframe_with_timer(wlc, p, 1, data_rate, txdelay, txrepetitions, txperiodicity);

    return 0;
}

void *
handle_sdio_xmit_request_hook(void *sdio_hw, struct sk_buff *p)
{
    struct wl_info *wl = *(*((struct wl_info ***) sdio_hw + 15) + 6);
    struct wlc_info *wlc = wl->wlc;
    struct ethernet_ip_udp_header *ethfrm = (struct ethernet_ip_udp_header *) (p != 0) ? (p->data + 4) : 0;
    struct nexudp_ioctl_header *nexioctlhdr = (struct nexudp_ioctl_header *) (((void *) ethfrm) + sizeof(struct ethernet_ip_udp_header));
    struct nexudp_header *nexudphdr = &nexioctlhdr->nexudphdr;

    // Check if destination MAC address starts with ff:ff:ff:ff, port equals 5500, and first three bytes equal NEX
    if (p != 0 && p->data != 0 
            && !memcmp(&ethfrm->ethernet.dst, "\xff\xff\xff\xff\xff\xff", 6) 
            && ethfrm->udp.dst_port == htons(5500) 
            && !memcmp(&nexudphdr->nex, "NEX", 3)) {

        if (!check_securitycookie(nexudphdr->securitycookie)) {
            printf("ERR: incorrect or unset security cookie.\n");
            pkt_buf_free_skb(wlc->osh, p, 0);
            return 0;
        }

        // remove bdc, ethernet, ip, udp and nexudp headers
        skb_pull(p, sizeof(struct bdc_ethernet_ip_udp_header) + sizeof(struct nexudp_header));

        switch(nexudphdr->type) {
            case NEXUDP_IOCTL:
                wlc_ioctl_hook(wlc, nexioctlhdr->cmd, nexioctlhdr->payload, p->len - sizeof(nexioctlhdr->cmd) - sizeof(nexioctlhdr->set), 0);

                // prepare to send back an answer tunneled over udp
                prepend_ethernet_ipv4_udp_header(p);

                // send back the answer
                wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p);
                return 0;
                break;

            case NEXUDP_INJECT_RADIOTAP:
                return inject_frame(wlc, p);
                break;

            default:
                hexdump("test", p->data, p->len);
                pkt_buf_free_skb(wlc->osh, p, 0);
                return 0;
        }
    } else if (wlc->monitor && p != 0 && p->data != 0 && ((short *) p->data)[2] == 0) {
        // remove bdc header
        skb_pull(p, 4);

        // check if in monitor mode and if first two bytes in the frame correspond to a radiotap header, if true, inject frame
        return inject_frame(wlc, p);
    } else {
        // otherwise, handle frame normally
        return handle_sdio_xmit_request_ram(sdio_hw, p);
    }
}

// Hook the call to handle_sdio_xmit_request_hook in sdio_header_parsing_from_sk_buff
__attribute__((at(0x185C5E, "", CHIP_VER_BCM4335b0, FW_VER_6_30_171_1_sta)))
BPatch(handle_sdio_xmit_request_hook, handle_sdio_xmit_request_hook);

// Replace the entry in the function pointer table by handle_sdio_xmit_request_hook
__attribute__((at(0x180DF8, "", CHIP_VER_BCM4335b0, FW_VER_ALL)))
GenericPatch4(handle_sdio_xmit_request_hook, handle_sdio_xmit_request_hook + 1);
