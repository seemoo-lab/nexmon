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
 * Copyright (c) 2024 NexMon Team                                          *
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

#ifndef STRUCTS_COMMON_H
#define STRUCTS_COMMON_H

#include <types.h>

/* lbuf version with BCMHWA, HWA_PKT_MACRO, and BCMPKTIDMAP */
struct sk_buff {
    uint32 pkttag[(32 + 3) / 4];
    char *head;
    char *end;
    char *data;
    uint16 len;
    uint16 cfp_flowid;
    union {
        uint32 u32;
        struct {
            uint16 pktid;
            uint8 refcnt;
            uint8 poolid;
        };
    } mem;
    uint32 flags;
    union {
        uint32 reset;
        struct {
            union {
                uint16 dmapad;
                uint16 rxcpl_id;
                uint16 dma_index;
            };
            union {
                uint8 dataOff;
                uint8 hwa_rxOff;
            };
            uint8 ifidx;
        };
    };
    union {
        struct {
            uint16 nextid;
            uint16 linkid;
        };
        void * freelist;
    };
} __attribute__((packed));

typedef struct sk_buff sk_buff;
typedef struct sk_buff lbuf;

struct osl_ext_timer {
    uint32  tx_timer_id;                                    /* 0x000 */
    char    *tx_timer_name;                                 /* 0x004 */
    uint32  tx_timer_internal_remaining_ticks;              /* 0x008 */
    uint32  tx_timer_internal_re_initialize_ticks;          /* 0x00c */
    void    (*tx_timer_internal_timeout_function)(uint32);  /* 0x010 */
    uint32  tx_timer_internal_timeout_param;                /* 0x014 */
    void    *tx_timer_internal_active_next;                 /* 0x018 */
    void    *tx_timer_internal_active_previous;             /* 0x01c */
    void    **tx_timer_internal_list_head;                  /* 0x020 */
    void    *tx_timer_created_next;                         /* 0x024 */
    void    *tx_timer_created_previous;                     /* 0x028 */
} __attribute__((packed));

struct hndrte_timer
{
    struct osl_ext_timer timer;                             /* 0x000 */
    uint32  *context;                                       /* 0x02c */
    void    *data;                                          /* 0x030 */
    void    (*mainfn)(struct hndrte_timer *timer);          /* 0x034 */
    void    (*auxfn)(void *ctx);                            /* 0x038 */
    uint32  usec;                                           /* 0x03c */
    uint8   periodic;                                       /* 0x040 */
    uint8   deleted;                                        /* 0x041 */
    uint8   PAD;                                            /* 0x042 */
    uint8   PAD;                                            /* 0x043 */
    void    *dpc;                                           /* 0x044 */
    void    *cpuutil;                                       /* 0x048 */
} __attribute__((packed));
typedef struct hndrte_timer hnd_timer;

struct hndrte_dev {
    char name[16];
    uint32 devid;
    uint32 flags;
    union {
        struct hnd_dev_ops *ops;
        struct hnd_dev_ops *funcs;
    };
    void *softc;
    struct hndrte_dev *next;
    struct hndrte_dev *chained;
    struct hndrte_dev_stats *stats;
    void *commondata;
    void *pdev;
    void *priv;
};
typedef struct hndrte_dev hnd_dev;

struct hnd_dev_ops {
    void *(*probe)(struct hndrte_dev *dev, void *regs, uint bus, uint16 device, uint coreid, uint unit);
    int (*open)(struct hndrte_dev *dev);
    int (*close)(struct hndrte_dev *dev);
    int (*xmit)(struct hndrte_dev *src, struct hndrte_dev *dev, struct sk_buff *lbuf);
    int (*xmit_ctl)(struct hndrte_dev *src, struct hndrte_dev *dev, struct sk_buff *lbuf);
    int (*recv)(struct hndrte_dev *src, struct hndrte_dev *dev, void *pkt);
    int (*ioctl)(struct hndrte_dev *dev, uint32 cmd, void *buffer, int len, int *used, int *needed, int set);
    void (*txflowcontrol) (struct hndrte_dev *dev, bool state, int prio);
    void (*poll)(struct hndrte_dev *dev);
    void (*wowldown) (struct hndrte_dev *src);
    int32 (*flowring_link_update)(struct hndrte_dev *dev, uint16 flowid, uint8 op, uint8 *sa, uint8 *da, uint8 tid, uint8 *mode);
    int (*cfp_flow_link)(struct hndrte_dev *dev, uint16 ringid, uint8 tid, uint8* da, uint8 op, uint8** tcb_state, uint16* cfp_flowid);
};

struct d11regs {
    uint32 PAD;                /* 0x000 */
    uint32 PAD;                /* 0x004 */
    uint32 PAD;                /* 0x008 */
    uint32 PAD;                /* 0x00c */
    uint32 PAD;                /* 0x010 */
    uint32 PAD;                /* 0x014 */
    uint32 PAD;                /* 0x018 */
    uint32 PAD;                /* 0x01c */
    uint32 PAD;                /* 0x020 */
    uint32 PAD;                /* 0x024 */
    uint32 PAD;                /* 0x028 */
    uint32 PAD;                /* 0x02c */
    uint32 PAD;                /* 0x030 */
    uint32 PAD;                /* 0x034 */
    uint32 PAD;                /* 0x038 */
    uint32 PAD;                /* 0x03c */
    uint32 PAD;                /* 0x040 */
    uint32 PAD;                /* 0x044 */
    uint32 PAD;                /* 0x048 */
    uint32 PAD;                /* 0x04c */
    uint32 PAD;                /* 0x050 */
    uint32 PAD;                /* 0x054 */
    uint32 PAD;                /* 0x058 */
    uint32 PAD;                /* 0x05c */
    uint32 PAD;                /* 0x060 */
    uint32 PAD;                /* 0x064 */
    uint32 PAD;                /* 0x068 */
    uint32 PAD;                /* 0x06c */
    uint32 PAD;                /* 0x070 */
    uint32 PAD;                /* 0x074 */
    uint32 PAD;                /* 0x078 */
    uint32 PAD;                /* 0x07c */
    uint32 PAD;                /* 0x080 */
    uint32 PAD;                /* 0x084 */
    uint32 PAD;                /* 0x088 */
    uint32 PAD;                /* 0x08c */
    uint32 PAD;                /* 0x090 */
    uint32 PAD;                /* 0x094 */
    uint32 PAD;                /* 0x098 */
    uint32 PAD;                /* 0x09c */
    uint32 PAD;                /* 0x0a0 */
    uint32 PAD;                /* 0x0a4 */
    uint32 PAD;                /* 0x0a8 */
    uint32 PAD;                /* 0x0ac */
    uint32 PAD;                /* 0x0b0 */
    uint32 PAD;                /* 0x0b4 */
    uint32 PAD;                /* 0x0b8 */
    uint32 PAD;                /* 0x0bc */
    uint32 PAD;                /* 0x0c0 */
    uint32 PAD;                /* 0x0c4 */
    uint32 PAD;                /* 0x0c8 */
    uint32 PAD;                /* 0x0cc */
    uint32 PAD;                /* 0x0d0 */
    uint32 PAD;                /* 0x0d4 */
    uint32 PAD;                /* 0x0d8 */
    uint32 PAD;                /* 0x0dc */
    uint32 PAD;                /* 0x0e0 */
    uint32 PAD;                /* 0x0e4 */
    uint32 PAD;                /* 0x0e8 */
    uint32 PAD;                /* 0x0ec */
    uint32 PAD;                /* 0x0f0 */
    uint32 PAD;                /* 0x0f4 */
    uint32 PAD;                /* 0x0f8 */
    uint32 PAD;                /* 0x0fc */
    uint32 PAD;                /* 0x100 */
    uint32 PAD;                /* 0x104 */
    uint32 PAD;                /* 0x108 */
    uint32 PAD;                /* 0x10c */
    uint32 PAD;                /* 0x110 */
    uint32 PAD;                /* 0x114 */
    uint32 PAD;                /* 0x118 */
    uint32 PAD;                /* 0x11c */
    uint32 PAD;                /* 0x120 */
    uint32 PAD;                /* 0x124 */
    uint32 PAD;                /* 0x128 */
    uint32 PAD;                /* 0x12c */
    uint32 PAD;                /* 0x130 */
    uint32 PAD;                /* 0x134 */
    uint32 PAD;                /* 0x138 */
    uint32 PAD;                /* 0x13c */
    uint32 PAD;                /* 0x140 */
    uint32 PAD;                /* 0x144 */
    uint32 PAD;                /* 0x148 */
    uint32 PAD;                /* 0x14c */
    uint32 PAD;                /* 0x150 */
    uint32 PAD;                /* 0x154 */
    uint32 PAD;                /* 0x158 */
    uint32 PAD;                /* 0x15c */
    uint32 objaddr;            /* 0x160 */
    uint32 objdata;            /* 0x164 */
    /* ... */
} __attribute__((packed));

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

#endif /*STRUCTS_COMMON_H */
