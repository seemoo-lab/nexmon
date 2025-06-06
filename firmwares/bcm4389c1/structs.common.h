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
 * Copyright (c) 2023 NexMon Team                                          *
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

//struct lbuf {
struct sk_buff {
    union {                    /* 0x000 */
        uint32  u32;
        struct {
            uint16  pktid;
            uint8   refcnt;
            uint8   poolid;
        };
    } mem;
    union {                    /* 0x004 */
        uint32 offsets;
        struct {
            uint32 head_off : 21;
            uint32 end_off  : 11;
        };
    };
    uint8 *data;               /* 0x008 */
    uint16 len;                /* 0x00c */
    uint16 PAD;                /* 0x00e */
    uint32 flags;              /* 0x010 */
    union {                    /* 0x014 */
        uint32 reset;
        struct {
            union {
                uint16 dmapad;
                uint16 rxcpl_id;
                uint16 dma_index;
            };
            union {
                uint8  dataOff;
                uint8  hwa_rxOff;
            };
            uint8  ifidx;
        };
    };
    struct sk_buff *next;      /* 0x018 */
    struct sk_buff *link;      /* 0x01c */
    uint32 pkttag[7];          /* 0x020 */
} __attribute__((packed));

typedef struct sk_buff sk_buff;
typedef struct sk_buff lbuf;

struct wl_rxsts {
    uint    pkterror;
    uint    phytype;
    uint16  chanspec;
    uint16  datarate;
    uint8   mcs;
    uint8   htflags;
    uint    antenna;
    uint    pktlength;
    uint32  mactime;
    uint    sq;
    int32   signal;
    int32   noise;
    uint    preamble;
    uint    encoding;
    uint    nfrmtype;
    uint8   nss;
    uint8   coding;
    uint16  aid;
    uint8   gid;
    uint8   bw;
    uint16  vhtflags;
    uint16  bw_nonht;
    uint32  ampdu_counter;
    uint32  sig_a1;
    uint32  sig_a2;
    uint16  data1;
    uint16  data2;
    uint16  data3;
    uint16  data4;
    uint16  data5;
    uint16  data6;
    uint8   ru_channel1[4];
    uint8   ru_channel2[4];
    uint16  flag1;
    uint16  flag2;
} __attribute__((packed));

struct obj_registry {
    int count;
    void **value;
    uint8 *ref;
};


enum obj_registry_key {
    OBJR_SELF = 0x0,
    OBJR_0x001 = 0x1,
    OBJR_0x002 = 0x2,
    OBJR_0x003 = 0x3,
    OBJR_0x004 = 0x4,
    OBJR_0x005 = 0x5,
    OBJR_WLC_CMN_INFO = 0x6,
    OBJR_WLC_PUB_CMN_INFO = 0x7,
    OBJR_PHY_CMN_SROM_INFO = 0x8,
    OBJR_0x009 = 0x9,
    OBJR_0x00a = 0xA,
    OBJR_0x00b = 0xB,
    OBJR_0x00c = 0xC,
    OBJR_0x00d = 0xD,
    OBJR_0x00e = 0xE,
    OBJR_0x00f = 0xF,
    OBJR_0x010 = 0x10,
    OBJR_0x011 = 0x11,
    OBJR_0x012 = 0x12,
    OBJR_0x013 = 0x13,
    OBJR_0x014 = 0x14,
    OBJR_0x015 = 0x15,
    OBJR_0x016 = 0x16,
    OBJR_0x017 = 0x17,
    OBJR_0x018 = 0x18,
    OBJR_RSDB_CMN_INFO = 0x19,
    OBJR_0x01a = 0x1A,
    OBJR_0x01b = 0x1B,
    OBJR_0x01c = 0x1C,
    OBJR_0x01d = 0x1D,
    OBJR_0x01e = 0x1E,
    OBJR_0x01f = 0x1F,
    OBJR_0x020 = 0x20,
    OBJR_0x021 = 0x21,
    OBJR_0x022 = 0x22,
    OBJR_0x023 = 0x23,
    OBJR_0x024 = 0x24,
    OBJR_0x025 = 0x25,
    OBJR_0x026 = 0x26,
    OBJR_0x027 = 0x27,
    OBJR_0x028 = 0x28,
    OBJR_0x029 = 0x29,
    OBJR_0x02a = 0x2A,
    OBJR_0x02b = 0x2B,
    OBJR_0x02c = 0x2C,
    OBJR_0x02d = 0x2D,
    OBJR_0x02e = 0x2E,
    OBJR_0x02f = 0x2F,
    OBJR_0x030 = 0x30,
    OBJR_0x031 = 0x31,
    OBJR_0x032 = 0x32,
    OBJR_0x033 = 0x33,
    OBJR_0x034 = 0x34,
    OBJR_0x035 = 0x35,
    OBJR_0x036 = 0x36,
    OBJR_0x037 = 0x37,
    OBJR_0x038 = 0x38,
    OBJR_0x039 = 0x39,
    OBJR_0x03a = 0x3A,
    OBJR_0x03b = 0x3B,
    OBJR_0x03c = 0x3C,
    OBJR_0x03d = 0x3D,
    OBJR_0x03e = 0x3E,
    OBJR_0x03f = 0x3F,
    OBJR_0x040 = 0x40,
    OBJR_0x041 = 0x41,
    OBJR_0x042 = 0x42,
    OBJR_0x043 = 0x43,
    OBJR_0x044 = 0x44,
    OBJR_0x045 = 0x45,
    OBJR_0x046 = 0x46,
    OBJR_0x047 = 0x47,
    OBJR_0x048 = 0x48,
    OBJR_0x049 = 0x49,
    OBJR_0x04a = 0x4A,
    OBJR_0x04b = 0x4B,
    OBJR_0x04c = 0x4C,
    OBJR_0x04d = 0x4D,
    OBJR_0x04e = 0x4E,
    OBJR_0x04f = 0x4F,
    OBJR_0x050 = 0x50,
    OBJR_0x051 = 0x51,
    OBJR_0x052 = 0x52,
    OBJR_0x053 = 0x53,
    OBJR_0x054 = 0x54,
    OBJR_0x055 = 0x55,
    OBJR_0x056 = 0x56,
    OBJR_0x057 = 0x57,
    OBJR_0x058 = 0x58,
    OBJR_0x059 = 0x59,
    OBJR_0x05a = 0x5A,
    OBJR_0x05b = 0x5B,
    OBJR_0x05c = 0x5C,
    OBJR_0x05d = 0x5D,
    OBJR_0x05e = 0x5E,
    OBJR_0x05f = 0x5F,
    OBJR_0x060 = 0x60,
    OBJR_0x061 = 0x61,
    OBJR_0x062 = 0x62,
    OBJR_0x063 = 0x63,
    OBJR_0x064 = 0x64,
    OBJR_0x065 = 0x65,
    OBJR_0x066 = 0x66,
    OBJR_0x067 = 0x67,
    OBJR_0x068 = 0x68,
    OBJR_0x069 = 0x69,
    OBJR_0x06a = 0x6A,
    OBJR_0x06b = 0x6B,
    OBJR_0x06c = 0x6C,
    OBJR_0x06d = 0x6D,
    OBJR_0x06e = 0x6E,
    OBJR_0x06f = 0x6F,
    OBJR_0x070 = 0x70,
    OBJR_0x071 = 0x71,
    OBJR_0x072 = 0x72,
    OBJR_0x073 = 0x73,
    OBJR_0x074 = 0x74,
    OBJR_0x075 = 0x75,
    OBJR_0x076 = 0x76,
    OBJR_0x077 = 0x77,
    OBJR_0x078 = 0x78,
    OBJR_0x079 = 0x79,
    OBJR_0x07a = 0x7A,
    OBJR_0x07b = 0x7B,
    OBJR_0x07c = 0x7C,
    OBJR_MAX_KEYS = 0x7D
};

#define NBBY 8
struct wlc_obj_registry {
    struct obj_registry *objr;
    uint8 key_enab[OBJR_MAX_KEYS/NBBY + 1];
} __attribute__((packed));

struct wlc_hw_info {
    struct wl_info *wl;               /* 0x000 */
    struct wlc_obj_registry *objr;    /* 0x004 */
    struct wlc_info *wlc;             /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    uint32 PAD;                       /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
    uint32 PAD;                       /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    uint32 PAD;                       /* 0x054 */
    uint32 PAD;                       /* 0x058 */
    uint32 PAD;                       /* 0x05c */
    uint32 PAD;                       /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 PAD;                       /* 0x06c */
    uint32 PAD;                       /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    uint32 PAD;                       /* 0x07c */
    uint32 PAD;                       /* 0x080 */
    uint32 PAD;                       /* 0x084 */
    uint32 PAD;                       /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    uint32 PAD;                       /* 0x090 */
    uint32 PAD;                       /* 0x094 */
    uint32 PAD;                       /* 0x098 */
    uint32 PAD;                       /* 0x09c */
    uint32 PAD;                       /* 0x0a0 */
    uint32 PAD;                       /* 0x0a4 */
    uint32 PAD;                       /* 0x0a8 */
    uint32 PAD;                       /* 0x0ac */
    uint32 PAD;                       /* 0x0b0 */
    uint32 PAD;                       /* 0x0b4 */
    uint32 PAD;                       /* 0x0b8 */
    uint32 PAD;                       /* 0x0bc */
    uint32 PAD;                       /* 0x0c0 */
    uint32 PAD;                       /* 0x0c4 */
    uint32 PAD;                       /* 0x0c8 */
    uint32 PAD;                       /* 0x0cc */
    uint32 PAD;                       /* 0x0d0 */
    uint32 PAD;                       /* 0x0d4 */
    uint32 PAD;                       /* 0x0d8 */
    uint32 corerev;                   /* 0x0dc */
    uint32 PAD;                       /* 0x0e0 */
    uint32 PAD;                       /* 0x0e4 */
    uint32 PAD;                       /* 0x0e8 */
    uint32 PAD;                       /* 0x0ec */
    uint32 PAD;                       /* 0x0f0 */
    uint32 PAD;                       /* 0x0f4 */
    uint32 PAD;                       /* 0x0f8 */
    uint32 PAD;                       /* 0x0fc */
    uint32 PAD;                       /* 0x100 */
    uint32 PAD;                       /* 0x104 */
    uint32 PAD;                       /* 0x108 */
    uint32 PAD;                       /* 0x10c */
    uint32 PAD;                       /* 0x110 */
    uint32 PAD;                       /* 0x114 */
    uint32 PAD;                       /* 0x118 */
    uint32 PAD;                       /* 0x11c */
    uint32 PAD;                       /* 0x120 */
    uint32 PAD;                       /* 0x124 */
    uint32 machwcap;                  /* 0x128 */
    uint32 machwcap1;                 /* 0x12c */
    uint32 PAD;                       /* 0x130 */
    uint32 PAD;                       /* 0x134 */
    void *sih;                        /* 0x138 */
    char *vars;                       /* 0x13c */
    uint32 vars_size;                 /* 0x140 */
    struct d11regs *regs;             /* 0x144 */
    uint32 PAD;                       /* 0x148 */
    uint32 PAD;                       /* 0x14c */
    uint32 PAD;                       /* 0x150 */
    uint32 PAD;                       /* 0x154 */
    uint32 PAD;                       /* 0x158 */
    uint32 PAD;                       /* 0x15c */
    uint32 PAD;                       /* 0x160 */
    uint32 PAD;                       /* 0x164 */
    uint32 PAD;                       /* 0x168 */
    uint8 up;                         /* 0x16c */
    uint8 PAD;                        /* 0x16d */
    uint8 PAD;                        /* 0x16e */
    uint8 PAD;                        /* 0x16f */
    uint32 PAD;                       /* 0x170 */
    uint32 PAD;                       /* 0x174 */
    uint32 PAD;                       /* 0x178 */
    uint32 PAD;                       /* 0x17c */
    uint32 PAD;                       /* 0x180 */
    uint32 PAD;                       /* 0x184 */
    uint32 PAD;                       /* 0x188 */
    uint32 PAD;                       /* 0x18c */
    uint32 PAD;                       /* 0x190 */
    uint32 PAD;                       /* 0x194 */
    uint32 PAD;                       /* 0x198 */
    uint32 PAD;                       /* 0x19c */
    uint32 PAD;                       /* 0x1a0 */
    uint8 clk;                        /* 0x1a4 */
    uint8 sbclk;                      /* 0x1a5 */
    uint8 phyclk;                     /* 0x1a6 */
    uint8 dma_lpbk;                   /* 0x1a7 */
    uint32 PAD;                       /* 0x1a8 */
    uint32 PAD;                       /* 0x1ac */
    uint32 PAD;                       /* 0x1b0 */
    uint32 PAD;                       /* 0x1b4 */
    uint32 PAD;                       /* 0x1b8 */
    uint32 PAD;                       /* 0x1bc */
    uint32 PAD;                       /* 0x1c0 */
    uint32 PAD;                       /* 0x1c4 */
    uint32 PAD;                       /* 0x1c8 */
    uint32 PAD;                       /* 0x1cc */
    uint32 PAD;                       /* 0x1d0 */
    uint32 PAD;                       /* 0x1d4 */
    uint32 PAD;                       /* 0x1d8 */
    uint32 PAD;                       /* 0x1dc */
    uint32 fifo_inuse;                /* 0x1e0 */
    uint32 PAD;                       /* 0x1e4 */
    uint32 PAD;                       /* 0x1e8 */
    uint32 PAD;                       /* 0x1ec */
    uint32 PAD;                       /* 0x1f0 */
    uint32 PAD;                       /* 0x1f4 */
    uint32 PAD;                       /* 0x1f8 */
    uint32 PAD;                       /* 0x1fc */
    uint32 PAD;                       /* 0x200 */
    uint32 PAD;                       /* 0x204 */
    uint32 PAD;                       /* 0x208 */
    uint32 PAD;                       /* 0x20c */
    uint32 PAD;                       /* 0x210 */
    uint32 PAD;                       /* 0x214 */
    uint32 PAD;                       /* 0x218 */
    uint32 d11axi_slave_base_addr;    /* 0x21c */
    uint32 PAD;                       /* 0x220 */
    uint32 PAD;                       /* 0x224 */
    uint32 PAD;                       /* 0x228 */
    uint32 PAD;                       /* 0x22c */
    uint32 PAD;                       /* 0x230 */
    uint32 PAD;                       /* 0x234 */
    uint32 PAD;                       /* 0x238 */
    uint32 PAD;                       /* 0x23c */
    uint32 PAD;                       /* 0x240 */
    uint32 PAD;                       /* 0x244 */
    uint32 PAD;                       /* 0x248 */
    uint32 PAD;                       /* 0x24c */
    uint32 PAD;                       /* 0x250 */
    uint32 PAD;                       /* 0x254 */
    uint32 PAD;                       /* 0x258 */
    uint32 PAD;                       /* 0x25c */
    uint32 PAD;                       /* 0x260 */
    uint32 PAD;                       /* 0x264 */
    uint32 PAD;                       /* 0x268 */
    uint32 PAD;                       /* 0x26c */
    uint32 PAD;                       /* 0x270 */
    uint32 PAD;                       /* 0x274 */
    uint32 PAD;                       /* 0x278 */
    uint32 PAD;                       /* 0x27c */
    uint32 PAD;                       /* 0x280 */
    uint32 PAD;                       /* 0x284 */
    uint32 PAD;                       /* 0x288 */
    uint32 PAD;                       /* 0x28c */
    uint32 PAD;                       /* 0x290 */
    uint32 PAD;                       /* 0x294 */
    uint32 PAD;                       /* 0x298 */
    uint32 PAD;                       /* 0x29c */
    uint32 PAD;                       /* 0x2a0 */
    uint32 PAD;                       /* 0x2a4 */
    uint32 PAD;                       /* 0x2a8 */
    uint32 PAD;                       /* 0x2ac */
    uint32 PAD;                       /* 0x2b0 */
    uint32 PAD;                       /* 0x2b4 */
    uint32 PAD;                       /* 0x2b8 */
    uint32 PAD;                       /* 0x2bc */
    uint32 PAD;                       /* 0x2c0 */
    uint32 PAD;                       /* 0x2c4 */
    uint32 PAD;                       /* 0x2c8 */
    uint32 PAD;                       /* 0x2cc */
    uint32 PAD;                       /* 0x2d0 */
    uint32 PAD;                       /* 0x2d4 */
    uint32 PAD;                       /* 0x2d8 */
    uint32 PAD;                       /* 0x2dc */
    uint32 PAD;                       /* 0x2e0 */
    uint32 PAD;                       /* 0x2e4 */
    uint32 PAD;                       /* 0x2e8 */
    uint32 PAD;                       /* 0x2ec */
    uint32 PAD;                       /* 0x2f0 */
    uint32 PAD;                       /* 0x2f4 */
    uint32 PAD;                       /* 0x2f8 */
    uint32 PAD;                       /* 0x2fc */
    uint32 PAD;                       /* 0x300 */
    uint32 PAD;                       /* 0x304 */
    uint32 PAD;                       /* 0x308 */
    uint32 PAD;                       /* 0x30c */
    uint32 PAD;                       /* 0x310 */
    uint32 PAD;                       /* 0x314 */
    uint32 PAD;                       /* 0x318 */
    uint32 PAD;                       /* 0x31c */
    uint32 PAD;                       /* 0x320 */
    uint32 PAD;                       /* 0x324 */
    uint32 PAD;                       /* 0x328 */
    uint32 PAD;                       /* 0x32c */
    uint32 PAD;                       /* 0x330 */
    uint32 PAD;                       /* 0x334 */
    uint32 PAD;                       /* 0x338 */
    uint32 PAD;                       /* 0x33c */
    uint32 PAD;                       /* 0x340 */
    uint32 PAD;                       /* 0x344 */
    uint32 PAD;                       /* 0x348 */
    uint32 PAD;                       /* 0x34c */
    uint32 PAD;                       /* 0x350 */
    uint32 PAD;                       /* 0x354 */
    uint32 PAD;                       /* 0x358 */
    uint32 PAD;                       /* 0x35c */
    uint32 PAD;                       /* 0x360 */
    uint32 PAD;                       /* 0x364 */
    uint32 PAD;                       /* 0x368 */
    uint32 PAD;                       /* 0x36c */
    uint32 PAD;                       /* 0x370 */
    uint32 PAD;                       /* 0x374 */
    uint32 PAD;                       /* 0x378 */
    uint32 PAD;                       /* 0x37c */
    uint32 PAD;                       /* 0x380 */
    uint32 PAD;                       /* 0x384 */
    uint32 PAD;                       /* 0x388 */
    uint32 PAD;                       /* 0x38c */
    uint32 PAD;                       /* 0x390 */
    uint32 PAD;                       /* 0x394 */
    uint32 PAD;                       /* 0x398 */
    uint32 PAD;                       /* 0x39c */
    uint32 PAD;                       /* 0x3a0 */
    uint32 PAD;                       /* 0x3a4 */
    uint32 PAD;                       /* 0x3a8 */
    uint32 PAD;                       /* 0x3ac */
    uint32 PAD;                       /* 0x3b0 */
    uint32 PAD;                       /* 0x3b4 */
    uint32 PAD;                       /* 0x3b8 */
    uint32 PAD;                       /* 0x3bc */
    uint32 PAD;                       /* 0x3c0 */
    uint32 PAD;                       /* 0x3c4 */
    uint32 PAD;                       /* 0x3c8 */
    uint32 PAD;                       /* 0x3cc */
    uint32 PAD;                       /* 0x3d0 */
    uint32 PAD;                       /* 0x3d4 */
    uint32 PAD;                       /* 0x3d8 */
    uint32 PAD;                       /* 0x3dc */
    uint32 PAD;                       /* 0x3e0 */
    uint32 PAD;                       /* 0x3e4 */
    uint32 PAD;                       /* 0x3e8 */
    uint32 PAD;                       /* 0x3ec */
    uint32 PAD;                       /* 0x3f0 */
    uint32 PAD;                       /* 0x3f4 */
    uint32 PAD;                       /* 0x3f8 */
    uint32 PAD;                       /* 0x3fc */
    uint32 PAD;                       /* 0x400 */
    uint32 PAD;                       /* 0x404 */
    uint32 PAD;                       /* 0x408 */
    uint32 PAD;                       /* 0x40c */
    uint32 PAD;                       /* 0x410 */
    uint32 PAD;                       /* 0x414 */
    uint32 PAD;                       /* 0x418 */
    uint32 PAD;                       /* 0x41c */
    uint32 PAD;                       /* 0x420 */
    uint32 PAD;                       /* 0x424 */
    uint32 PAD;                       /* 0x428 */
    uint32 PAD;                       /* 0x42c */
    uint32 PAD;                       /* 0x430 */
    uint32 PAD;                       /* 0x434 */
    uint32 PAD;                       /* 0x438 */
    uint32 PAD;                       /* 0x43c */
    uint32 PAD;                       /* 0x440 */
    uint32 PAD;                       /* 0x444 */
    uint32 PAD;                       /* 0x448 */
    uint32 PAD;                       /* 0x44c */
    uint32 PAD;                       /* 0x450 */
    uint32 PAD;                       /* 0x454 */
    uint32 PAD;                       /* 0x458 */
    uint32 PAD;                       /* 0x45c */
    uint32 PAD;                       /* 0x460 */
    uint32 PAD;                       /* 0x464 */
    uint32 PAD;                       /* 0x468 */
    uint32 PAD;                       /* 0x46c */
    uint32 PAD;                       /* 0x470 */
    uint32 PAD;                       /* 0x474 */
    uint32 PAD;                       /* 0x478 */
    uint32 PAD;                       /* 0x47c */
    uint32 PAD;                       /* 0x480 */
    uint32 PAD;                       /* 0x484 */
    uint32 PAD;                       /* 0x488 */
    uint32 PAD;                       /* 0x48c */
    uint32 PAD;                       /* 0x490 */
    uint32 PAD;                       /* 0x494 */
    uint32 PAD;                       /* 0x498 */
    uint32 PAD;                       /* 0x49c */
    uint32 PAD;                       /* 0x4a0 */
    uint32 PAD;                       /* 0x4a4 */
    uint32 PAD;                       /* 0x4a8 */
    uint32 PAD;                       /* 0x4ac */
    uint32 PAD;                       /* 0x4b0 */
    uint32 PAD;                       /* 0x4b4 */
    uint32 PAD;                       /* 0x4b8 */
    uint32 PAD;                       /* 0x4bc */
    uint32 PAD;                       /* 0x4c0 */
    uint32 PAD;                       /* 0x4c4 */
    uint32 PAD;                       /* 0x4c8 */
    uint32 PAD;                       /* 0x4cc */
    uint32 PAD;                       /* 0x4d0 */
    uint32 PAD;                       /* 0x4d4 */
    uint32 PAD;                       /* 0x4d8 */
    uint32 PAD;                       /* 0x4dc */
    uint32 PAD;                       /* 0x4e0 */
} __attribute__((packed));

struct wlc_pub_cmn {
    uint32 PAD;                       /* 0x000 */
    uint32 PAD;                       /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    uint32 PAD;                       /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
    uint32 PAD;                       /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    uint32 PAD;                       /* 0x054 */
    uint32 PAD;                       /* 0x058 */
    uint32 PAD;                       /* 0x05c */
    uint32 PAD;                       /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 PAD;                       /* 0x06c */
    uint32 PAD;                       /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    uint32 PAD;                       /* 0x07c */
    uint32 PAD;                       /* 0x080 */
    uint32 PAD;                       /* 0x084 */
    uint32 PAD;                       /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    uint32 PAD;                       /* 0x090 */
    uint32 PAD;                       /* 0x094 */
    uint32 PAD;                       /* 0x098 */
    uint32 PAD;                       /* 0x09c */
    uint32 PAD;                       /* 0x0a0 */
} __attribute__((packed));

struct wlc_pub {
    struct wlc_info *wlc;             /* 0x000 */
    struct wlc_pub_cmn *cmn;          /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    uint32 PAD;                       /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
    uint32 PAD;                       /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    uint32 PAD;                       /* 0x054 */
    uint32 PAD;                       /* 0x058 */
    uint32 PAD;                       /* 0x05c */
    uint32 PAD;                       /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 PAD;                       /* 0x06c */
    uint32 PAD;                       /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    uint32 PAD;                       /* 0x07c */
    uint32 PAD;                       /* 0x080 */
    uint32 PAD;                       /* 0x084 */
    uint32 PAD;                       /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    uint32 PAD;                       /* 0x090 */
    uint32 PAD;                       /* 0x094 */
    uint32 PAD;                       /* 0x098 */
    uint32 PAD;                       /* 0x09c */
    uint32 PAD;                       /* 0x0a0 */
    uint32 PAD;                       /* 0x0a4 */
    uint32 PAD;                       /* 0x0a8 */
    uint32 PAD;                       /* 0x0ac */
    uint32 PAD;                       /* 0x0b0 */
    uint32 PAD;                       /* 0x0b4 */
    uint32 PAD;                       /* 0x0b8 */
    uint32 PAD;                       /* 0x0bc */
    uint32 PAD;                       /* 0x0c0 */
    uint32 PAD;                       /* 0x0c4 */
    uint32 PAD;                       /* 0x0c8 */
    uint32 PAD;                       /* 0x0cc */
    uint32 PAD;                       /* 0x0d0 */
    uint32 PAD;                       /* 0x0d4 */
    uint32 PAD;                       /* 0x0d8 */
    uint32 PAD;                       /* 0x0dc */
    uint32 PAD;                       /* 0x0e0 */
    uint32 PAD;                       /* 0x0e4 */
    uint32 PAD;                       /* 0x0e8 */
    uint32 PAD;                       /* 0x0ec */
    uint32 PAD;                       /* 0x0f0 */
    uint32 PAD;                       /* 0x0f4 */
    uint32 PAD;                       /* 0x0f8 */
    uint32 PAD;                       /* 0x0fc */
    uint32 PAD;                       /* 0x100 */
    uint32 PAD;                       /* 0x104 */
    uint32 PAD;                       /* 0x108 */
    uint32 PAD;                       /* 0x10c */
    uint32 PAD;                       /* 0x110 */
    uint32 PAD;                       /* 0x114 */
    uint32 PAD;                       /* 0x118 */
    uint32 PAD;                       /* 0x11c */
    uint32 PAD;                       /* 0x120 */
    uint32 PAD;                       /* 0x124 */
    uint8 PAD;                        /* 0x128 */
    uint8 _mon;                       /* 0x129 */
    uint16 PAD;                       /* 0x12a */
    uint32 PAD;                       /* 0x12c */
    uint32 PAD;                       /* 0x130 */
    uint32 PAD;                       /* 0x134 */
    uint32 PAD;                       /* 0x138 */
    uint32 PAD;                       /* 0x13c */
    uint32 PAD;                       /* 0x140 */
    uint32 PAD;                       /* 0x144 */
    uint32 PAD;                       /* 0x148 */
    uint32 PAD;                       /* 0x14c */
    uint32 PAD;                       /* 0x150 */
    uint32 PAD;                       /* 0x154 */
    uint32 PAD;                       /* 0x158 */
    uint32 PAD;                       /* 0x15c */
    uint32 PAD;                       /* 0x160 */
    uint32 PAD;                       /* 0x164 */
    uint32 PAD;                       /* 0x168 */
    uint32 PAD;                       /* 0x16c */
} __attribute__((packed));

#define MAX_RSDB_MAC_NUM 2
struct wlc_cmn_info {
    struct wlc_info *wlc[MAX_RSDB_MAC_NUM];/* 0x000 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    uint32 PAD;                       /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
} __attribute__((packed));

struct wlc_info {
    struct wlc_pub *pub;              /* 0x000 */
    void *osh;                        /* 0x004 */
    struct wl_info *wl;               /* 0x008 */
    struct wlc_cmn_info *cmn;         /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    struct wlc_hw_info *hw;           /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    struct phy_info *pi;              /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
    uint32 PAD;                       /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    uint32 PAD;                       /* 0x054 */
    uint32 PAD;                       /* 0x058 */
    uint32 PAD;                       /* 0x05c */
    uint32 PAD;                       /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 PAD;                       /* 0x06c */
    uint32 PAD;                       /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    uint32 PAD;                       /* 0x07c */
    uint32 PAD;                       /* 0x080 */
    uint32 PAD;                       /* 0x084 */
    uint32 PAD;                       /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    uint32 PAD;                       /* 0x090 */
    uint32 PAD;                       /* 0x094 */
    uint32 PAD;                       /* 0x098 */
    uint32 PAD;                       /* 0x09c */
    void *scan;                       /* 0x0a0 */
    uint32 PAD;                       /* 0x0a4 */
    uint32 PAD;                       /* 0x0a8 */
    uint32 PAD;                       /* 0x0ac */
    uint32 PAD;                       /* 0x0b0 */
    uint32 PAD;                       /* 0x0b4 */
    uint32 PAD;                       /* 0x0b8 */
    uint32 PAD;                       /* 0x0bc */
    uint32 PAD;                       /* 0x0c0 */
    uint32 PAD;                       /* 0x0c4 */
    uint32 PAD;                       /* 0x0c8 */
    uint32 PAD;                       /* 0x0cc */
    uint32 PAD;                       /* 0x0d0 */
    uint32 PAD;                       /* 0x0d4 */
    uint32 PAD;                       /* 0x0d8 */
    uint32 PAD;                       /* 0x0dc */
    uint32 PAD;                       /* 0x0e0 */
    uint32 PAD;                       /* 0x0e4 */
    uint32 PAD;                       /* 0x0e8 */
    uint32 PAD;                       /* 0x0ec */
    uint32 PAD;                       /* 0x0f0 */
    uint32 PAD;                       /* 0x0f4 */
    uint32 PAD;                       /* 0x0f8 */
    uint32 PAD;                       /* 0x0fc */
    uint32 PAD;                       /* 0x100 */
    uint32 PAD;                       /* 0x104 */
    uint32 PAD;                       /* 0x108 */
    uint32 PAD;                       /* 0x10c */
    uint32 PAD;                       /* 0x110 */
    uint32 PAD;                       /* 0x114 */
    uint32 PAD;                       /* 0x118 */
    uint32 PAD;                       /* 0x11c */
    uint32 monitor;                   /* 0x120 */
    uint32 PAD;                       /* 0x124 */
    uint32 PAD;                       /* 0x128 */
    uint32 PAD;                       /* 0x12c */
    uint32 PAD;                       /* 0x130 */
    uint32 PAD;                       /* 0x134 */
    uint32 PAD;                       /* 0x138 */
    uint32 PAD;                       /* 0x13c */
    uint32 PAD;                       /* 0x140 */
    uint32 PAD;                       /* 0x144 */
    uint32 PAD;                       /* 0x148 */
    uint32 PAD;                       /* 0x14c */
    uint32 PAD;                       /* 0x150 */
    uint32 PAD;                       /* 0x154 */
    uint32 PAD;                       /* 0x158 */
    uint32 PAD;                       /* 0x15c */
    uint32 PAD;                       /* 0x160 */
    uint32 PAD;                       /* 0x164 */
    uint32 PAD;                       /* 0x168 */
    uint32 PAD;                       /* 0x16c */
    uint32 PAD;                       /* 0x170 */
    uint32 PAD;                       /* 0x174 */
    uint32 PAD;                       /* 0x178 */
    uint32 PAD;                       /* 0x17c */
    uint32 PAD;                       /* 0x180 */
    uint32 PAD;                       /* 0x184 */
    uint32 PAD;                       /* 0x188 */
    uint32 PAD;                       /* 0x18c */
    uint32 PAD;                       /* 0x190 */
    uint32 PAD;                       /* 0x194 */
    uint32 PAD;                       /* 0x198 */
    uint32 PAD;                       /* 0x19c */
    uint32 PAD;                       /* 0x1a0 */
    uint32 PAD;                       /* 0x1a4 */
    uint32 PAD;                       /* 0x1a8 */
    uint32 PAD;                       /* 0x1ac */
    uint32 PAD;                       /* 0x1b0 */
    uint32 PAD;                       /* 0x1b4 */
    uint32 PAD;                       /* 0x1b8 */
    uint32 PAD;                       /* 0x1bc */
    uint32 PAD;                       /* 0x1c0 */
    uint32 PAD;                       /* 0x1c4 */
    uint32 PAD;                       /* 0x1c8 */
    uint32 PAD;                       /* 0x1cc */
    uint32 PAD;                       /* 0x1d0 */
    uint32 PAD;                       /* 0x1d4 */
    uint32 PAD;                       /* 0x1d8 */
    uint32 PAD;                       /* 0x1dc */
    uint32 PAD;                       /* 0x1e0 */
    uint32 PAD;                       /* 0x1e4 */
    uint32 PAD;                       /* 0x1e8 */
    uint32 PAD;                       /* 0x1ec */
    uint32 PAD;                       /* 0x1f0 */
    uint32 PAD;                       /* 0x1f4 */
    uint32 PAD;                       /* 0x1f8 */
    uint32 PAD;                       /* 0x1fc */
    uint32 PAD;                       /* 0x200 */
    uint32 PAD;                       /* 0x204 */
    uint32 PAD;                       /* 0x208 */
    uint32 PAD;                       /* 0x20c */
    uint32 PAD;                       /* 0x210 */
    uint32 PAD;                       /* 0x214 */
    uint32 PAD;                       /* 0x218 */
    uint32 PAD;                       /* 0x21c */
    uint32 PAD;                       /* 0x220 */
    uint32 PAD;                       /* 0x224 */
    uint32 PAD;                       /* 0x228 */
    uint32 PAD;                       /* 0x22c */
    uint32 PAD;                       /* 0x230 */
    uint32 PAD;                       /* 0x234 */
    uint32 PAD;                       /* 0x238 */
    uint32 PAD;                       /* 0x23c */
    uint32 PAD;                       /* 0x240 */
    uint32 PAD;                       /* 0x244 */
    uint32 PAD;                       /* 0x248 */
    uint32 PAD;                       /* 0x24c */
    uint32 PAD;                       /* 0x250 */
    uint32 PAD;                       /* 0x254 */
    uint32 PAD;                       /* 0x258 */
    uint32 PAD;                       /* 0x25c */
    uint32 PAD;                       /* 0x260 */
    uint32 PAD;                       /* 0x264 */
    uint32 PAD;                       /* 0x268 */
    uint32 PAD;                       /* 0x26c */
    uint32 PAD;                       /* 0x270 */
    uint32 PAD;                       /* 0x274 */
    uint32 PAD;                       /* 0x278 */
    uint32 PAD;                       /* 0x27c */
    uint32 PAD;                       /* 0x280 */
    uint32 PAD;                       /* 0x284 */
    uint32 PAD;                       /* 0x288 */
    uint32 PAD;                       /* 0x28c */
    uint32 PAD;                       /* 0x290 */
    uint32 PAD;                       /* 0x294 */
    uint32 PAD;                       /* 0x298 */
    uint32 PAD;                       /* 0x29c */
    uint32 PAD;                       /* 0x2a0 */
    uint32 PAD;                       /* 0x2a4 */
    uint32 PAD;                       /* 0x2a8 */
    uint32 PAD;                       /* 0x2ac */
    uint32 PAD;                       /* 0x2b0 */
    uint32 PAD;                       /* 0x2b4 */
    uint32 PAD;                       /* 0x2b8 */
    uint32 PAD;                       /* 0x2bc */
    uint32 PAD;                       /* 0x2c0 */
    uint32 PAD;                       /* 0x2c4 */
    uint32 PAD;                       /* 0x2c8 */
    uint32 PAD;                       /* 0x2cc */
    uint32 PAD;                       /* 0x2d0 */
    uint32 PAD;                       /* 0x2d4 */
    uint32 PAD;                       /* 0x2d8 */
    uint32 PAD;                       /* 0x2dc */
    uint32 PAD;                       /* 0x2e0 */
    uint32 PAD;                       /* 0x2e4 */
    uint32 PAD;                       /* 0x2e8 */
    uint32 PAD;                       /* 0x2ec */
    uint32 PAD;                       /* 0x2f0 */
    uint32 PAD;                       /* 0x2f4 */
    uint32 PAD;                       /* 0x2f8 */
    uint32 PAD;                       /* 0x2fc */
    uint32 PAD;                       /* 0x300 */
    uint32 PAD;                       /* 0x304 */
    uint32 PAD;                       /* 0x308 */
    uint32 PAD;                       /* 0x30c */
    uint32 PAD;                       /* 0x310 */
    uint32 PAD;                       /* 0x314 */
    uint32 PAD;                       /* 0x318 */
    uint32 PAD;                       /* 0x31c */
    uint32 PAD;                       /* 0x320 */
    uint32 PAD;                       /* 0x324 */
    uint32 PAD;                       /* 0x328 */
    uint32 PAD;                       /* 0x32c */
    uint32 PAD;                       /* 0x330 */
    uint32 PAD;                       /* 0x334 */
    uint32 PAD;                       /* 0x338 */
    uint32 PAD;                       /* 0x33c */
    uint32 PAD;                       /* 0x340 */
    uint32 PAD;                       /* 0x344 */
    uint32 PAD;                       /* 0x348 */
    uint32 PAD;                       /* 0x34c */
    uint32 PAD;                       /* 0x350 */
    uint32 PAD;                       /* 0x354 */
    uint32 PAD;                       /* 0x358 */
    uint32 PAD;                       /* 0x35c */
    uint32 PAD;                       /* 0x360 */
    uint32 PAD;                       /* 0x364 */
    uint32 PAD;                       /* 0x368 */
    uint32 PAD;                       /* 0x36c */
    uint32 PAD;                       /* 0x370 */
    uint32 PAD;                       /* 0x374 */
    uint32 PAD;                       /* 0x378 */
    uint32 PAD;                       /* 0x37c */
    uint32 PAD;                       /* 0x380 */
    uint32 PAD;                       /* 0x384 */
    uint32 PAD;                       /* 0x388 */
    uint32 PAD;                       /* 0x38c */
    uint32 PAD;                       /* 0x390 */
    uint32 PAD;                       /* 0x394 */
    uint32 PAD;                       /* 0x398 */
    uint32 PAD;                       /* 0x39c */
    uint32 PAD;                       /* 0x3a0 */
    uint32 PAD;                       /* 0x3a4 */
    uint32 PAD;                       /* 0x3a8 */
    uint32 PAD;                       /* 0x3ac */
    uint32 PAD;                       /* 0x3b0 */
    uint32 PAD;                       /* 0x3b4 */
    uint32 PAD;                       /* 0x3b8 */
    uint32 PAD;                       /* 0x3bc */
    uint32 PAD;                       /* 0x3c0 */
    uint32 PAD;                       /* 0x3c4 */
    uint32 PAD;                       /* 0x3c8 */
    uint32 PAD;                       /* 0x3cc */
    uint32 PAD;                       /* 0x3d0 */
    uint32 PAD;                       /* 0x3d4 */
    uint32 PAD;                       /* 0x3d8 */
    uint32 PAD;                       /* 0x3dc */
    uint32 PAD;                       /* 0x3e0 */
    uint32 PAD;                       /* 0x3e4 */
    uint32 PAD;                       /* 0x3e8 */
    uint32 PAD;                       /* 0x3ec */
    uint32 PAD;                       /* 0x3f0 */
    uint32 PAD;                       /* 0x3f4 */
    uint32 PAD;                       /* 0x3f8 */
    uint32 PAD;                       /* 0x3fc */
    uint32 PAD;                       /* 0x400 */
    uint32 PAD;                       /* 0x404 */
    uint32 PAD;                       /* 0x408 */
    uint32 PAD;                       /* 0x40c */
    uint32 PAD;                       /* 0x410 */
    uint32 PAD;                       /* 0x414 */
    uint32 PAD;                       /* 0x418 */
    void *mon_info;                   /* 0x41c */
    uint32 PAD;                       /* 0x420 */
    uint32 PAD;                       /* 0x424 */
    uint32 PAD;                       /* 0x428 */
    uint32 PAD;                       /* 0x42c */
    uint32 PAD;                       /* 0x430 */
    uint32 PAD;                       /* 0x434 */
    struct wlc_obj_registry *objr;    /* 0x438 */
    uint32 PAD;                       /* 0x43c */
    uint32 PAD;                       /* 0x440 */
    uint32 PAD;                       /* 0x444 */
    uint32 PAD;                       /* 0x448 */
    uint32 PAD;                       /* 0x44c */
    uint32 PAD;                       /* 0x450 */
    uint32 PAD;                       /* 0x454 */
    uint32 PAD;                       /* 0x458 */
    uint32 PAD;                       /* 0x45c */
    uint32 PAD;                       /* 0x460 */
    uint32 PAD;                       /* 0x464 */
    uint32 PAD;                       /* 0x468 */
    uint32 PAD;                       /* 0x46c */
    uint32 PAD;                       /* 0x470 */
    uint32 PAD;                       /* 0x474 */
    uint32 PAD;                       /* 0x478 */
    uint32 PAD;                       /* 0x47c */
    uint32 PAD;                       /* 0x480 */
    uint32 PAD;                       /* 0x484 */
    uint32 PAD;                       /* 0x488 */
    uint32 PAD;                       /* 0x48c */
    uint32 PAD;                       /* 0x490 */
    uint32 PAD;                       /* 0x494 */
    uint32 PAD;                       /* 0x498 */
    uint32 PAD;                       /* 0x49c */
    uint32 PAD;                       /* 0x4a0 */
    uint32 PAD;                       /* 0x4a4 */
    uint32 PAD;                       /* 0x4a8 */
    uint32 PAD;                       /* 0x4ac */
    uint32 PAD;                       /* 0x4b0 */
    uint32 PAD;                       /* 0x4b4 */
    uint32 PAD;                       /* 0x4b8 */
    uint32 PAD;                       /* 0x4bc */
    uint32 PAD;                       /* 0x4c0 */
    uint32 PAD;                       /* 0x4c4 */
    uint32 PAD;                       /* 0x4c8 */
    uint32 PAD;                       /* 0x4cc */
    uint32 PAD;                       /* 0x4d0 */
    uint32 PAD;                       /* 0x4d4 */
    uint32 PAD;                       /* 0x4d8 */
    uint32 PAD;                       /* 0x4dc */
    uint32 PAD;                       /* 0x4e0 */
    uint32 PAD;                       /* 0x4e4 */
    uint32 PAD;                       /* 0x4e8 */
    uint32 PAD;                       /* 0x4ec */
    uint32 PAD;                       /* 0x4f0 */
    void *rsdbinfo;                   /* 0x4f4 */
    uint32 PAD;                       /* 0x4f8 */
    uint32 PAD;                       /* 0x4fc */
    uint32 PAD;                       /* 0x500 */
    uint32 PAD;                       /* 0x504 */
    uint32 PAD;                       /* 0x508 */
    uint32 PAD;                       /* 0x50c */
    uint32 PAD;                       /* 0x510 */
    uint32 PAD;                       /* 0x514 */
    uint32 PAD;                       /* 0x518 */
    uint32 PAD;                       /* 0x51c */
    uint32 PAD;                       /* 0x520 */
    uint32 PAD;                       /* 0x524 */
    uint32 PAD;                       /* 0x528 */
    uint32 PAD;                       /* 0x52c */
    uint32 PAD;                       /* 0x530 */
    uint32 PAD;                       /* 0x534 */
    uint32 PAD;                       /* 0x538 */
    uint32 PAD;                       /* 0x53c */
    uint32 PAD;                       /* 0x540 */
    uint32 PAD;                       /* 0x544 */
    uint32 PAD;                       /* 0x548 */
    uint32 PAD;                       /* 0x54c */
    uint32 PAD;                       /* 0x550 */
    uint32 PAD;                       /* 0x554 */
    uint32 PAD;                       /* 0x558 */
    uint32 PAD;                       /* 0x55c */
    uint32 PAD;                       /* 0x560 */
    uint32 PAD;                       /* 0x564 */
    uint32 PAD;                       /* 0x568 */
    uint32 PAD;                       /* 0x56c */
    uint32 PAD;                       /* 0x570 */
    uint32 PAD;                       /* 0x574 */
    uint32 PAD;                       /* 0x578 */
    uint32 PAD;                       /* 0x57c */
    uint32 PAD;                       /* 0x580 */
    uint32 PAD;                       /* 0x584 */
    uint32 PAD;                       /* 0x588 */
    uint32 PAD;                       /* 0x58c */
    uint32 PAD;                       /* 0x590 */
    uint32 PAD;                       /* 0x594 */
    uint32 PAD;                       /* 0x598 */
    uint32 PAD;                       /* 0x59c */
    uint32 PAD;                       /* 0x5a0 */
    uint32 PAD;                       /* 0x5a4 */
    uint32 PAD;                       /* 0x5a8 */
    uint32 PAD;                       /* 0x5ac */
    uint32 PAD;                       /* 0x5b0 */
    uint32 PAD;                       /* 0x5b4 */
    uint32 PAD;                       /* 0x5b8 */
    uint32 PAD;                       /* 0x5bc */
    uint32 PAD;                       /* 0x5c0 */
    uint32 PAD;                       /* 0x5c4 */
    uint32 PAD;                       /* 0x5c8 */
    uint32 PAD;                       /* 0x5cc */
    uint32 PAD;                       /* 0x5d0 */
    uint32 PAD;                       /* 0x5d4 */
    uint32 PAD;                       /* 0x5d8 */
    uint32 PAD;                       /* 0x5dc */
    uint32 PAD;                       /* 0x5e0 */
    uint32 PAD;                       /* 0x5e4 */
    uint32 PAD;                       /* 0x5e8 */
    uint32 PAD;                       /* 0x5ec */
    uint32 PAD;                       /* 0x5f0 */
    uint32 PAD;                       /* 0x5f4 */
    uint32 PAD;                       /* 0x5f8 */
    uint32 PAD;                       /* 0x5fc */
    uint32 PAD;                       /* 0x600 */
    uint32 PAD;                       /* 0x604 */
    uint32 PAD;                       /* 0x608 */
    uint32 PAD;                       /* 0x60c */
    uint32 PAD;                       /* 0x610 */
    uint32 PAD;                       /* 0x614 */
    uint32 PAD;                       /* 0x618 */
    uint32 PAD;                       /* 0x61c */
    uint32 PAD;                       /* 0x620 */
    uint32 PAD;                       /* 0x624 */
    uint32 PAD;                       /* 0x628 */
    uint32 PAD;                       /* 0x62c */
    uint32 PAD;                       /* 0x630 */
    uint32 PAD;                       /* 0x634 */
    uint32 PAD;                       /* 0x638 */
    uint32 PAD;                       /* 0x63c */
    uint32 PAD;                       /* 0x640 */
    uint32 PAD;                       /* 0x644 */
    uint32 PAD;                       /* 0x648 */
    uint32 PAD;                       /* 0x64c */
    uint32 PAD;                       /* 0x650 */
    uint32 PAD;                       /* 0x654 */
    uint32 PAD;                       /* 0x658 */
    uint32 PAD;                       /* 0x65c */
    uint32 PAD;                       /* 0x660 */
    uint32 PAD;                       /* 0x664 */
    uint32 PAD;                       /* 0x668 */
    uint32 PAD;                       /* 0x66c */
    uint32 PAD;                       /* 0x670 */
    uint32 PAD;                       /* 0x674 */
    uint32 PAD;                       /* 0x678 */
    uint32 PAD;                       /* 0x67c */
    uint32 PAD;                       /* 0x680 */
    uint32 PAD;                       /* 0x684 */
    uint32 PAD;                       /* 0x688 */
    uint32 PAD;                       /* 0x68c */
    uint32 PAD;                       /* 0x690 */
    uint32 PAD;                       /* 0x694 */
    uint32 PAD;                       /* 0x698 */
    uint32 PAD;                       /* 0x69c */
    uint32 PAD;                       /* 0x6a0 */
    uint32 PAD;                       /* 0x6a4 */
    uint32 PAD;                       /* 0x6a8 */
    uint32 PAD;                       /* 0x6ac */
    uint32 PAD;                       /* 0x6b0 */
    uint32 PAD;                       /* 0x6b4 */
    uint32 PAD;                       /* 0x6b8 */
    uint32 PAD;                       /* 0x6bc */
    uint32 PAD;                       /* 0x6c0 */
    uint32 PAD;                       /* 0x6c4 */
    uint32 PAD;                       /* 0x6c8 */
    uint32 PAD;                       /* 0x6cc */
    uint32 PAD;                       /* 0x6d0 */
    uint32 PAD;                       /* 0x6d4 */
    uint32 PAD;                       /* 0x6d8 */
    uint32 PAD;                       /* 0x6dc */
    uint32 PAD;                       /* 0x6e0 */
    uint32 PAD;                       /* 0x6e4 */
    uint32 PAD;                       /* 0x6e8 */
    uint32 PAD;                       /* 0x6ec */
    uint32 PAD;                       /* 0x6f0 */
    uint32 PAD;                       /* 0x6f4 */
    uint32 PAD;                       /* 0x6f8 */
    uint32 PAD;                       /* 0x6fc */
    uint32 PAD;                       /* 0x700 */
    uint32 PAD;                       /* 0x704 */
    uint32 PAD;                       /* 0x708 */
    uint32 PAD;                       /* 0x70c */
    uint32 PAD;                       /* 0x710 */
} __attribute__((packed));

struct wl_sc {
    uint32 PAD;                       /* 0x000 */
    uint32 PAD;                       /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    uint32 PAD;                       /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
    uint32 PAD;                       /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    uint32 PAD;                       /* 0x054 */
    uint32 PAD;                       /* 0x058 */
    uint32 PAD;                       /* 0x05c */
    uint32 PAD;                       /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 PAD;                       /* 0x06c */
    uint32 PAD;                       /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    uint32 PAD;                       /* 0x07c */
    uint32 PAD;                       /* 0x080 */
    uint32 PAD;                       /* 0x084 */
    uint32 PAD;                       /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    uint32 PAD;                       /* 0x090 */
    uint32 PAD;                       /* 0x094 */
} __attribute__((packed));

struct hnd_dev {
    char name[16];                    /* 0x000 */
    uint32 devid;                     /* 0x010 */
    uint32 flags;                     /* 0x014 */
    union {
        struct hnd_dev_ops *ops;          /* 0x018 */
        struct hnd_dev_ops *funcs;        /* 0x018 */
    };
    void *softc;                      /* 0x01c */
    struct hnd_dev *next;             /* 0x020 */
    struct hnd_dev *chained;          /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
} __attribute__((packed));

struct hnd_dev_ops {
    void *(*probe)(struct hnd_dev *dev, volatile void *regs, uint bus, uint16 device, uint coreid, uint unit);
    int (*open)(struct hnd_dev *dev);
    int (*close)(struct hnd_dev *dev);
    int (*xmit)(struct hnd_dev *src, struct hnd_dev *dev, void *lbuf);
    int (*xmit_ctl)(struct hnd_dev *src, struct hnd_dev *dev, void *lbuf);
    int (*recv)(struct hnd_dev *src, struct hnd_dev *dev, void *pkt);
    int (*ioctl)(struct hnd_dev *dev, uint32 cmd, void *buffer, int len, int *used, int *needed, int set);
    void (*txflowcontrol)(struct hnd_dev *dev, bool state, int prio);
    void (*poll)(struct hnd_dev *dev);
    void (*wowldown)(struct hnd_dev *src);
    int32 (*flowring_link_update)(struct hnd_dev *dev, uint16 flowid, uint8 op, uint8 *sa, uint8 *da, uint8 tid, uint8 *mode);
} __attribute__((packed));

struct wl_info {
    void *osh;                        /* 0x000 */
    uint32 PAD;                       /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    struct wlc_hw_info *wlc_hw;       /* 0x018 */
    struct hnd_dev *dev;              /* 0x01c */
    uint32 PAD;                       /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
    uint32 PAD;                       /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    struct wlc_obj_registry *objr;    /* 0x054 */
    struct wl_sc *wl_sc;              /* 0x058 */
    struct wlc_pub *pub;              /* 0x05c */
    struct wlc_info *wlc;             /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 PAD;                       /* 0x06c */
    uint32 PAD;                       /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    uint32 PAD;                       /* 0x07c */
    uint32 PAD;                       /* 0x080 */
    uint32 PAD;                       /* 0x084 */
    uint32 PAD;                       /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    uint32 PAD;                       /* 0x090 */
    uint32 monitor_type;              /* 0x094 */
    uint32 PAD;                       /* 0x098 */
    uint32 PAD;                       /* 0x09c */
    uint32 PAD;                       /* 0x0a0 */
    uint32 PAD;                       /* 0x0a4 */
    uint32 PAD;                       /* 0x0a8 */
    uint32 PAD;                       /* 0x0ac */
    uint32 PAD;                       /* 0x0b0 */
    uint32 PAD;                       /* 0x0b4 */
    uint32 PAD;                       /* 0x0b8 */
    uint32 PAD;                       /* 0x0bc */
    uint32 PAD;                       /* 0x0c0 */
    uint32 PAD;                       /* 0x0c4 */
    uint32 PAD;                       /* 0x0c8 */
    uint32 PAD;                       /* 0x0cc */
    uint32 PAD;                       /* 0x0d0 */
    uint32 PAD;                       /* 0x0d4 */
    uint32 PAD;                       /* 0x0d8 */
    uint32 PAD;                       /* 0x0dc */
    uint32 PAD;                       /* 0x0e0 */
} __attribute__((packed));

struct shared_phy {
    void *osh;                        /* 0x000 */
    void *sih;                        /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    uint32 PAD;                       /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
    uint32 PAD;                       /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    uint32 PAD;                       /* 0x054 */
    uint32 PAD;                       /* 0x058 */
    uint32 PAD;                       /* 0x05c */
    uint32 PAD;                       /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 PAD;                       /* 0x06c */
    uint32 PAD;                       /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    uint32 PAD;                       /* 0x07c */
    uint32 PAD;                       /* 0x080 */
    uint32 PAD;                       /* 0x084 */
    uint32 PAD;                       /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    uint32 PAD;                       /* 0x090 */
} __attribute__((packed));

struct phy_pub {
    uint32 PAD;                       /* 0x000 */
    uint32 PAD;                       /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
} __attribute__((packed));

struct phy_info {
    struct phy_pub *pubpi_ro;         /* 0x004 */
    struct phy_pub *pubpi;            /* 0x004 */
    struct shared_phy *sh;            /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    uint32 PAD;                       /* 0x020 */
    uint32 PAD;                       /* 0x024 */
    uint32 PAD;                       /* 0x028 */
    uint32 PAD;                       /* 0x02c */
    uint32 PAD;                       /* 0x030 */
    uint32 PAD;                       /* 0x034 */
    uint32 PAD;                       /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    uint32 PAD;                       /* 0x044 */
    uint32 PAD;                       /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    uint32 PAD;                       /* 0x054 */
    uint32 PAD;                       /* 0x058 */
    uint32 PAD;                       /* 0x05c */
    uint32 PAD;                       /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 PAD;                       /* 0x06c */
    uint32 PAD;                       /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    uint32 PAD;                       /* 0x07c */
    uint32 PAD;                       /* 0x080 */
    uint32 PAD;                       /* 0x084 */
    uint32 PAD;                       /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    uint32 PAD;                       /* 0x090 */
    uint32 PAD;                       /* 0x094 */
    uint32 PAD;                       /* 0x098 */
    uint32 PAD;                       /* 0x09c */
    uint32 PAD;                       /* 0x0a0 */
    uint32 PAD;                       /* 0x0a4 */
    uint32 PAD;                       /* 0x0a8 */
    uint32 PAD;                       /* 0x0ac */
    uint32 PAD;                       /* 0x0b0 */
    uint32 PAD;                       /* 0x0b4 */
    uint32 PAD;                       /* 0x0b8 */
    uint32 PAD;                       /* 0x0bc */
    uint32 PAD;                       /* 0x0c0 */
    uint32 PAD;                       /* 0x0c4 */
    uint32 PAD;                       /* 0x0c8 */
    uint32 PAD;                       /* 0x0cc */
    uint32 PAD;                       /* 0x0d0 */
    uint32 PAD;                       /* 0x0d4 */
    uint32 PAD;                       /* 0x0d8 */
    uint32 PAD;                       /* 0x0dc */
    uint32 PAD;                       /* 0x0e0 */
    uint32 PAD;                       /* 0x0e4 */
    uint32 PAD;                       /* 0x0e8 */
    uint32 PAD;                       /* 0x0ec */
    uint32 PAD;                       /* 0x0f0 */
    uint32 PAD;                       /* 0x0f4 */
    uint32 PAD;                       /* 0x0f8 */
    uint32 PAD;                       /* 0x0fc */
    uint32 PAD;                       /* 0x100 */
    uint32 PAD;                       /* 0x104 */
    uint32 PAD;                       /* 0x108 */
    uint32 PAD;                       /* 0x10c */
    uint32 PAD;                       /* 0x110 */
    uint32 PAD;                       /* 0x114 */
    uint32 PAD;                       /* 0x118 */
    uint32 PAD;                       /* 0x11c */
    uint32 PAD;                       /* 0x120 */
    uint32 PAD;                       /* 0x124 */
    uint32 PAD;                       /* 0x128 */
    uint32 PAD;                       /* 0x12c */
    uint32 PAD;                       /* 0x130 */
    uint32 PAD;                       /* 0x134 */
    uint32 PAD;                       /* 0x138 */
    uint32 PAD;                       /* 0x13c */
    uint32 PAD;                       /* 0x140 */
    uint32 PAD;                       /* 0x144 */
    uint32 PAD;                       /* 0x148 */
    uint32 PAD;                       /* 0x14c */
    uint32 PAD;                       /* 0x150 */
    uint32 PAD;                       /* 0x154 */
    uint32 PAD;                       /* 0x158 */
    uint32 PAD;                       /* 0x15c */
    uint32 PAD;                       /* 0x160 */
    uint32 PAD;                       /* 0x164 */
    uint32 PAD;                       /* 0x168 */
    uint32 PAD;                       /* 0x16c */
    uint32 PAD;                       /* 0x170 */
    uint32 PAD;                       /* 0x174 */
    uint32 PAD;                       /* 0x178 */
    uint32 PAD;                       /* 0x17c */
    uint32 PAD;                       /* 0x180 */
    uint32 PAD;                       /* 0x184 */
    uint32 PAD;                       /* 0x188 */
    uint32 PAD;                       /* 0x18c */
} __attribute__((packed));

struct hnd_cons {
    uint   vcons_in;
    uint   vcons_out;
    char   *buf;
    uint   buf_size;
    uint   idx;
    uint   out_idx;
    uint   cbuf_idx;
    char   cbuf[];
} __attribute__((packed));

struct hnd_debug {
    uint32  magic;
    uint32  version;
    uint32  fwid;
    char    epivers[32];
    void    *trap_ptr;
    struct hnd_cons *console;
    uint32  ram_base;
    uint32  ram_size;
    uint32  rom_base;
    uint32  rom_size;
    void    *event_log_top;
} __attribute__((packed));

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
