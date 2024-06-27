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
//#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
//#include <rates.h>              // rates used to build the ratespec for frame injection
//#include <bcmwifi_channels.h>
//#include <ieee80211_radiotap.h> // radiotap header related
//#include <vendor_radiotap.h>    // vendor specific radiotap extensions
//#include <sendframe.h>          // sendframe functionality
//#include <securitycookie.h>     // related to securtiy cookie

struct ethernet_ip_udp_header ethernet_ipv4_udp_header = {
  .ethernet = {
    .dst = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    .src = { 'N', 'E', 'X', 'M', 'O', 'N' },
    .type = 0x0008
  },
  .ip = {
    .version_ihl = 0x45,
    .dscp_ecn = 0x00,
    .total_length = 0x0000,
    .identification = 0x0100,
    .flags_fragment_offset = 0x0000,
    .ttl = 0x01, 
    .protocol = 0x11,
    .header_checksum = 0x0000,
    .src_ip.array = { 10, 10, 10, 10 },
    .dst_ip.array = { 255, 255, 255, 255 }
  },
  .udp = {
    .src_port = HTONS(5500),
    .dst_port = HTONS(5500),
    .len_chk_cov.length = 0x0000,
    .checksum = 0x0000
  }
};

/**
 * Calculates the IPv4 header checksum given the total IPv4 packet length.
 *
 * This checksum is specific to the packet format above. This is not a full
 * implementation of the checksum algorithm. Instead, as much as possible is
 * precalculated to reduce the amount of computation needed. This calculation
 * is accurate for total lengths up to 42457.
 */
static inline uint16_t
calc_checksum(uint16_t total_len)
{
    return ~(23078 + total_len);
}

void
prepend_ethernet_ipv4_udp_header(struct sk_buff *p)
{
    ethernet_ipv4_udp_header.ip.total_length = htons(p->len + sizeof(struct ip_header) + sizeof(struct udp_header));
    ethernet_ipv4_udp_header.ip.header_checksum = htons(calc_checksum(p->len + sizeof(struct ip_header) + sizeof(struct udp_header)));
    ethernet_ipv4_udp_header.udp.len_chk_cov.length = htons(p->len + sizeof(struct udp_header));

   	skb_push(p, sizeof(ethernet_ipv4_udp_header));
    memcpy(p->data, &ethernet_ipv4_udp_header, sizeof(ethernet_ipv4_udp_header));
}

void
udpvnprintf(struct wl_info *wl, unsigned int n, const char *format, va_list args)
{
    int len = 0;
    struct sk_buff *p = pkt_buf_get_skb(wl->wlc->osh, sizeof(struct ethernet_ip_udp_header) + n);

    if (p != 0) {
        skb_pull(p, sizeof(struct ethernet_ip_udp_header));

        len = vsnprintf(p->data, n, format, args);

        p->len = len;

        prepend_ethernet_ipv4_udp_header(p);

        wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p);
    }
}

void
udpnprintf(struct wl_info *wl, unsigned int n, const char *format, ...)
{
    va_list args;
    int len = 0;
    struct sk_buff *p = pkt_buf_get_skb(wl->wlc->osh, sizeof(struct ethernet_ip_udp_header) + n);

    if (p != 0) {
        skb_pull(p, sizeof(struct ethernet_ip_udp_header));

        va_start(args, format);
        len = vsnprintf(p->data, n, format, args);
        va_end(args);

        p->len = len;

        prepend_ethernet_ipv4_udp_header(p);

        wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p);
    }
}

void
udpprintf(struct wl_info *wl, const char *format, ...)
{
    va_list args;
    int len = 0;
    struct sk_buff *p = pkt_buf_get_skb(wl->wlc->osh, sizeof(struct ethernet_ip_udp_header) + 1000);

    if (p != 0) {
        skb_pull(p, sizeof(struct ethernet_ip_udp_header));

        va_start(args, format);
        len = vsnprintf(p->data, 1000, format, args);
        va_end(args);

        p->len = len;

        prepend_ethernet_ipv4_udp_header(p);

        wl->dev->chained->funcs->xmit(wl->dev, wl->dev->chained, p);
    }
}
