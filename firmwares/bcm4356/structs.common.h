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

#include <types.h>
#include <bcmcdc.h>

// TODO: We need to define the structs for the bcm4356 here

typedef void (*to_fun_t)(void *arg);

typedef struct _ctimeout {
    struct _ctimeout *next;
    uint32 ms;
    to_fun_t fun;
    void *arg;
    bool expired;
} ctimeout_t;

struct hndrte_timer
{
    uint32  *context;       /* first field so address of context is timer struct ptr */
    void    *data;
    void    (*mainfn)(struct hndrte_timer *);
    void    (*auxfn)(void *context);
    ctimeout_t t;
    int interval;
    int set;
    int periodic;
    bool    _freedone;
} __attribute__((packed));

/*== maccontrol register ==*/
#define MCTL_GMODE      (1U << 31)
#define MCTL_DISCARD_PMQ    (1 << 30)
#define MCTL_WAKE       (1 << 26)
#define MCTL_HPS        (1 << 25)
#define MCTL_PROMISC        (1 << 24)
#define MCTL_KEEPBADFCS     (1 << 23)
#define MCTL_KEEPCONTROL    (1 << 22)
#define MCTL_PHYLOCK        (1 << 21)
#define MCTL_BCNS_PROMISC   (1 << 20)
#define MCTL_LOCK_RADIO     (1 << 19)
#define MCTL_AP         (1 << 18)
#define MCTL_INFRA      (1 << 17)
#define MCTL_BIGEND     (1 << 16)
#define MCTL_GPOUT_SEL_MASK (3 << 14)
#define MCTL_GPOUT_SEL_SHIFT    14
#define MCTL_EN_PSMDBG      (1 << 13)
#define MCTL_IHR_EN     (1 << 10)
#define MCTL_SHM_UPPER      (1 <<  9)
#define MCTL_SHM_EN     (1 <<  8)
#define MCTL_PSM_JMP_0      (1 <<  2)
#define MCTL_PSM_RUN        (1 <<  1)
#define MCTL_EN_MAC     (1 <<  0)

struct ethernet_header {
    uint8 dst[6];
    uint8 src[6];
    uint16 type;
} __attribute__((packed));

struct ipv6_header {
    uint32 version_traffic_class_flow_label;
    uint16 payload_length;
    uint8 next_header;
    uint8 hop_limit;
    uint8 src_ip[16];
    uint8 dst_ip[16];
} __attribute__((packed));

struct ip_header {
    uint8 version_ihl;
    uint8 dscp_ecn;
    uint16 total_length;
    uint16 identification;
    uint16 flags_fragment_offset;
    uint8 ttl;
    uint8 protocol;
    uint16 header_checksum;
    union {
        uint32 integer;
        uint8 array[4];
    } src_ip;
    union {
        uint32 integer;
        uint8 array[4];
    } dst_ip;
} __attribute__((packed));

struct udp_header {
    uint16 src_port;
    uint16 dst_port;
    union {
        uint16 length;          /* UDP: length of UDP header and payload */
        uint16 checksum_coverage;   /* UDPLITE: checksum_coverage */
    } len_chk_cov;
    uint16 checksum;
} __attribute__((packed));

struct ethernet_ip_udp_header {
    struct ethernet_header ethernet;
    struct ip_header ip;
    struct udp_header udp;
} __attribute__((packed));

struct ethernet_ipv6_udp_header {
    struct ethernet_header ethernet;
    struct ipv6_header ipv6;
    struct udp_header udp;
    uint8 payload[1];
} __attribute__((packed));

struct nexmon_header {
    uint32 hooked_fct;
    uint32 args[3];
    uint8 payload[1];
} __attribute__((packed));

