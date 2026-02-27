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

struct wl_rxsts {
    uint32      pkterror;       /* error flags per pkt */
    uint32      phytype;        /* 802.11 A/B/G ... */
    uint32      PAD;
    uint16      chanspec;       /* channel spec */
    uint16      PAD;
    uint32      datarate;       /* rate in 500kbps (0 for HT frame) */
    uint8       mcs;            /* MCS for HT frame */
    uint8       htflags;        /* HT modulation flags */
    uint8       PAD;
    uint8       PAD;
    uint32      antenna;        /* antenna pkts received on */
    uint32      pktlength;      /* pkt length minus bcm phy hdr */
    uint32      mactime;        /* time stamp from mac, count per 1us */
    uint32      sq;             /* signal quality */
    int32       signal;         /* in dBm */
    int32       noise;          /* in dBm */
    uint32      preamble;       /* Unknown, short, long */
    uint32      encoding;       /* Unknown, CCK, PBCC, OFDM, HT */
    uint32      nfrmtype;       /* special 802.11n frames(AMPDU, AMSDU) */
    void        *wlif;          /* wl interface */
} __attribute__((packed));

struct osl_info {
	unsigned int pktalloced;
	int PAD[1];
	void *callback_when_dropped;
	unsigned int bustype;
} __attribute__((packed));

typedef struct sk_buff {
	struct sk_buff *next;       /* 0x00 */
	struct sk_buff *prev;       /* 0x04 */
	void *head;                 /* 0x08 - head of buffer */
    void *end;                  /* 0x0C - end of buffer */
	void *data;                 /* 0x10 - data head pointer */
	short len;                  /* 0x14 */
    short prio;                 /* 0x16 */
    short PAD;                  /* 0x18 */
    short PAD;                  /* 0x1A */
    int PAD;                    /* 0x1C */
    int flags;                  /* 0x20 */
    int PAD;                    /* 0x24 */
    int PAD;                    /* 0x28 */
    int lifetime_end;           /* 0x2C */
    int PAD;                    /* 0x30 */
    void *scb;                  /* 0x34 verified */
} __attribute__((packed)) sk_buff;

#define HNDRTE_DEV_NAME_MAX 16

typedef struct hndrte_dev {
    char                        name[HNDRTE_DEV_NAME_MAX];
    struct hndrte_devfuncs      *funcs;
    uint32                      devid;
    void                        *softc;     /* Software context */
    uint32                      flags;      /* RTEDEVFLAG_XXXX */
    struct hndrte_dev           *next;
    struct hndrte_dev           *chained;
    void                        *pdev;
} hndrte_dev;

struct hndrte_devfuncs {
    void *(*probe)(struct hndrte_dev *dev, void *regs, uint bus,
                   uint16 device, uint coreid, uint unit);
    int (*open)(struct hndrte_dev *dev);
    int (*close)(struct hndrte_dev *dev);
    int (*xmit)(struct hndrte_dev *src, struct hndrte_dev *dev, void *lb);
    int (*recv)(struct hndrte_dev *src, struct hndrte_dev *dev, void *pkt);
    int (*ioctl)(struct hndrte_dev *dev, uint32 cmd, void *buffer, int len,
                 int *used, int *needed, int set);
    void (*txflowcontrol) (struct hndrte_dev *dev, bool state, int prio);
    void (*poll)(struct hndrte_dev *dev);
    int (*xmit_ctl)(struct hndrte_dev *src, struct hndrte_dev *dev, void *lb);
    int (*xmit2)(struct hndrte_dev *src, struct hndrte_dev *dev, void *lb, int8 ch);
};

struct tunables {
    char gap[62];
    short somebnd; // @ 0x38
    short rxbnd; // @ 0x40
};

struct wlc_hwband {
    int bandtype;               /* 0x00 */
    int bandunit;               /* 0x04 */
    char mhfs;                  /* 0x05 */
    char PAD[10];               /* 0x06 */
    char bandhw_stf_ss_mode;    /* 0x13 */
    short CWmin;                /* 0x14 */
    short CWmax;                /* 0x16 */
    int core_flags;             /* 0x18 */
    short phytype;              /* 0x1C */
    short phyrev;               /* 0x1E */
    short radioid;              /* 0x20 */
    short radiorev;             /* 0x22 */
    void *pi;                   /* 0x24 */
    char abgphy_encore;         /* 0x25 */
};

struct wlc_hw_info {
    struct wlc_info *wlc;       /* 0x00 */
    int PAD[4];                 /* 0x04 */
    struct dma_info *di[6];     /* 0x14 - only 4 bytes */
    int PAD;                    // 0x2C
    int PAD;                    // 0x30
    int PAD;                    // 0x34
    int PAD;                    // 0x38
    int PAD;                    // 0x3c
    int PAD;                    // 0x40
    int PAD;                    // 0x44
    int PAD;                    // 0x48
    int PAD;                    // 0x4c
    int PAD;                    // 0x50
    int PAD;                    // 0x54
    int PAD;                    // 0x58
    int PAD;                    // 0x5c
    int PAD;                    // 0x60
    int PAD;                    // 0x64
    int PAD;                    // 0x68
    struct d11regs* regs;       // 0x6C 
    char PAD[2];                // 0x70
    char ucode_loaded;          // 0x72
    char PAD;                   // 0x73
    int PAD;                    // 0x74
    int sih;                    /* 0x78 */
    int vars;                   /* 0x7C */
    int vars_size;              /* 0x80 */
    int PAD;                    /* 0x84 */
    int physhim;                /* 0x88 */
    int phy_sh;                 /* 0x8C */
    char up;                    /* 0x90 */
    char PAD;
    char PAD;
    char PAD;
    int PAD[26];
    int maccontrol; // @ 0xe7
    int PAD[18];
    sk_buff *some_skbuff_ptr; // @ 0x134

};

/**
 *  Name might be inaccurate
 */
struct wl_info {
    int unit;
    void *pub;
    struct wlc_info *wlc;
    struct wlc_hw_info *wlc_hw;
    struct hndrte_dev *dev;
};

/**
 *  Name might be inaccurate
 */
struct sdiox_info {
    int unit;
    void *something;
    void *sdio; // sdio_info struct
    void *osh;
    void *device_address;
} __attribute__((packed));

struct wlcband {
    int bandtype;                       /* 0x000 verified */
    int bandunit;                       /* 0x004 */
    short phytype;                      /* 0x008 */
    short phyrev;                       /* 0x00A */
    short radioid;                      /* 0x00C */
    short radiorev;                     /* 0x00E */
    void *pi;                           /* 0x010 */
    char abgphy_encore;                 /* 0x014 */
    char gmode;                         /* 0x015 */
    char PAD;                           /* 0x016 */
    char PAD;                           /* 0x017 */
    void *hwrs_scb;                     /* 0x018 verified */
    int defrateset;                     /* 0x01C */
    int rspec_override;                 /* 0x020 */
    int mrspec_override;                /* 0x024 */
    char band_stf_ss_mode;              /* 0x028 */
    char band_stf_stbc_tx;              /* 0x029 */
    int hw_rateset;                     /* 0x030 */
    char basic_rate;                    /* 0x034 */
} __attribute__((packed));

struct wlc_info {
    struct wlc_pub *pub;                /* 0x000 */
    struct osl_info *osh;               /* 0x004 */
    void *wl;                           /* 0x008 */
    volatile struct d11regs *regs;      /* 0x00C verified */
    struct wlc_hw_info *hw;             /* 0x010 */
    int PAD;                            /* 0x014 */
    int PAD;                            /* 0x018 */
    void *core;                         /* 0x01C */
    int PAD;                            /* 0x020 */
    int PAD;                            /* 0x024 */
    int PAD;                            /* 0x028 */
    int PAD;                            /* 0x02C */
    struct wlcband *band;               /* 0x030 */
    int PAD;                            /* 0x034 */
    struct wlcband *bandstate[2];       /* 0x038 */
    int PAD;                            /* 0x040 */
    int PAD;                            /* 0x044 */
    int PAD;                            /* 0x048 */
    int PAD;                            /* 0x04C */
    int PAD;                            /* 0x050 */
    int PAD;                            /* 0x054 */
    int PAD;                            /* 0x058 */
    int PAD;                            /* 0x05C */
    int PAD;                            /* 0x060 */
    int PAD;                            /* 0x064 */
    int PAD;                            /* 0x068 */
    int PAD;                            /* 0x06C */
    int PAD;                            /* 0x070 */
    int PAD;                            /* 0x074 */
    int PAD;                            /* 0x078 */
    int PAD;                            /* 0x07C */
    int PAD;                            /* 0x080 */
    int PAD;                            /* 0x084 */
    int PAD;                            /* 0x088 */
    int PAD;                            /* 0x08C */
    int PAD;                            /* 0x090 */
    int PAD;                            /* 0x094 */
    int PAD;                            /* 0x098 */
    int PAD;                            /* 0x09C */
    int PAD;                            /* 0x0A0 */
    int PAD;                            /* 0x0A4 */
    int PAD;                            /* 0x0A8 */
    int PAD;                            /* 0x0AC */
    int PAD;                            /* 0x0B0 */
    int PAD;                            /* 0x0B4 */
    int PAD;                            /* 0x0B8 */
    int PAD;                            /* 0x0BC */
    int PAD;                            /* 0x0C0 */
    int PAD;                            /* 0x0C4 */
    int PAD;                            /* 0x0C8 */
    int PAD;                            /* 0x0CC */
    int PAD;                            /* 0x0D0 */
    int PAD;                            /* 0x0D4 */
    int PAD;                            /* 0x0D8 */
    int PAD;                            /* 0x0DC */
    int PAD;                            /* 0x0E0 */
    int PAD;                            /* 0x0E4 */
    int PAD;                            /* 0x0E8 */
    int PAD;                            /* 0x0EC */
    int PAD;                            /* 0x0F0 */
    int PAD;                            /* 0x0F4 */
    int PAD;                            /* 0x0F8 */
    int PAD;                            /* 0x0FC */
    int PAD;                            /* 0x100 */
    int PAD;                            /* 0x104 */
    int PAD;                            /* 0x108 */
    int PAD;                            /* 0x10C */
    int PAD;                            /* 0x110 */
    int PAD;                            /* 0x114 */
    int PAD;                            /* 0x118 */
    int PAD;                            /* 0x11C */
    int PAD;                            /* 0x120 */
    int PAD;                            /* 0x124 */
    int PAD;                            /* 0x128 */
    int PAD;                            /* 0x12C */
    int PAD;                            /* 0x130 */
    int PAD;                            /* 0x134 */
    int PAD;                            /* 0x138 */
    int PAD;                            /* 0x13C */
    int PAD;                            /* 0x140 */
    int PAD;                            /* 0x144 */
    int PAD;                            /* 0x148 */
    int PAD;                            /* 0x14C */
    int PAD;                            /* 0x150 */
    int PAD;                            /* 0x154 */
    int PAD;                            /* 0x158 */
    void *cmi;                          /* 0x15C */
    int PAD;                            /* 0x160 */
    int PAD;                            /* 0x164 */
    int PAD;                            /* 0x168 */
    int PAD;                            /* 0x16C */
    int PAD;                            /* 0x170 */
    int PAD;                            /* 0x174 */
    int PAD;                            /* 0x178 */
    int PAD;                            /* 0x17C */
    int PAD;                            /* 0x180 */
    int PAD;                            /* 0x184 */
    int PAD;                            /* 0x188 */
    int PAD;                            /* 0x18C */
    int PAD;                            /* 0x190 */
    int PAD;                            /* 0x194 */
    int PAD;                            /* 0x198 */
    int PAD;                            /* 0x19C */
    int PAD;                            /* 0x1A0 */
    int PAD;                            /* 0x1A4 */
    int PAD;                            /* 0x1A8 */
    int PAD;                            /* 0x1AC */
    int PAD;                            /* 0x1B0 */
    int PAD;                            /* 0x1B4 */
    int PAD;                            /* 0x1B8 */
    int PAD;                            /* 0x1BC */
    int PAD;                            /* 0x1C0 */
    int PAD;                            /* 0x1C4 */
    char monitor;                       /* 0x1C8 verified */
    char bcnmisc_ibss;                  /* 0x1C9 */
    char bcnmisc_scan;                  /* 0x1CA */
    char bcnmisc_monitor;               /* 0x1CB */
    int PAD;                            /* 0x1CC */
    int PAD;                            /* 0x1D0 */
    int PAD;                            /* 0x1D4 */
    int PAD;                            /* 0x1D8 */
    int PAD;                            /* 0x1DC */
    int PAD;                            /* 0x1E0 */
    int PAD;                            /* 0x1E4 */
    int PAD;                            /* 0x1E8 */
    int PAD;                            /* 0x1EC */
    int PAD;                            /* 0x1F0 */
    int PAD;                            /* 0x1F4 */
    int PAD;                            /* 0x1F8 */
    int PAD;                            /* 0x1FC */
    int PAD;                            /* 0x200 */
    int PAD;                            /* 0x204 */
    int PAD;                            /* 0x208 */
    int PAD;                            /* 0x20C */
    int PAD;                            /* 0x210 */
    int PAD;                            /* 0x214 */
    int PAD;                            /* 0x218 */
    int PAD;                            /* 0x21C */
    int PAD;                            /* 0x220 */
    int PAD;                            /* 0x224 */
    short PAD;                          /* 0x228 */
    short wme_dp;                       /* 0x22A */
    int PAD;                            /* 0x22C */
    int PAD;                            /* 0x230 */
    int PAD;                            /* 0x234 */
    int PAD;                            /* 0x238 */
    int PAD;                            /* 0x23C */
    int PAD;                            /* 0x240 */
    int PAD;                            /* 0x244 */
    int PAD;                            /* 0x248 */
    unsigned short tx_prec_map;         /* 0x24C */
    short PAD;                          /* 0x24E */
    int PAD;                            /* 0x250 */
    int PAD;                            /* 0x254 */
    int PAD;                            /* 0x258 */
    int PAD;                            /* 0x25C */
    int PAD;                            /* 0x260 */
    int PAD;                            /* 0x264 */
    int PAD;                            /* 0x268 */
    int PAD;                            /* 0x26C */
    int PAD;                            /* 0x270 */
    int PAD;                            /* 0x274 */
    int PAD;                            /* 0x278 */
    int PAD;                            /* 0x27C */
    int PAD;                            /* 0x280 */
    int PAD;                            /* 0x284 */
    int PAD;                            /* 0x288 */
    int PAD;                            /* 0x28C */
    int PAD;                            /* 0x290 */
    int PAD;                            /* 0x294 */
    int PAD;                            /* 0x298 */
    int PAD;                            /* 0x29C */
    int PAD;                            /* 0x2A0 */
    int PAD;                            /* 0x2A4 */
    int PAD;                            /* 0x2A8 */
    int PAD;                            /* 0x2AC */
    int PAD;                            /* 0x2B0 */
    int PAD;                            /* 0x2B4 */
    int PAD;                            /* 0x2B8 */
    int PAD;                            /* 0x2BC */
    int PAD;                            /* 0x2C0 */
    int PAD;                            /* 0x2C4 */
    int PAD;                            /* 0x2C8 */
    int PAD;                            /* 0x2CC */
    int PAD;                            /* 0x2D0 */
    int PAD;                            /* 0x2D4 */
    int PAD;                            /* 0x2D8 */
    int PAD;                            /* 0x2DC */
    int PAD;                            /* 0x2E0 */
    int PAD;                            /* 0x2E4 */
    int PAD;                            /* 0x2E8 */
    int PAD;                            /* 0x2EC */
    int PAD;                            /* 0x2F0 */
    int PAD;                            /* 0x2F4 */
    int PAD;                            /* 0x2F8 */
    int PAD;                            /* 0x2FC */
    int PAD;                            /* 0X300 */
    int PAD;                            /* 0X304 */
    int PAD;                            /* 0X308 */
    int PAD;                            /* 0X30C */
    int PAD;                            /* 0X310 */
    int PAD;                            /* 0X314 */
    int PAD;                            /* 0X318 */
    int PAD;                            /* 0X31C */
    int PAD;                            /* 0X320 */
    int PAD;                            /* 0X324 */
    int PAD;                            /* 0X328 */
    int PAD;                            /* 0X32C */
    int PAD;                            /* 0X330 */
    int PAD;                            /* 0X334 */
    int PAD;                            /* 0X338 */
    void *scan_results;                            /* 0X33C */
    int PAD;                            /* 0X340 */
    void *custom_scan_results;                            /* 0X344 */
    int PAD;                            /* 0X348 */
    int PAD;                            /* 0X34C */
    int PAD;                            /* 0X350 */
    int PAD;                            /* 0X354 */
    int PAD;                            /* 0X358 */
    int PAD;                            /* 0X35C */
    int PAD;                            /* 0X360 */
    short *field_364;                   /* 0X364 */
    int PAD;                            /* 0X368 */
    int PAD;                            /* 0X36C */
    int PAD;                            /* 0X370 */
    int PAD;                            /* 0X374 */
    int PAD;                            /* 0X378 */
    int PAD;                            /* 0X37C */
    int PAD;                            /* 0X380 */
    int PAD;                            /* 0X384 */
    int PAD;                            /* 0X388 */
    int PAD;                            /* 0X38C */
    int PAD;                            /* 0X390 */
    int PAD;                            /* 0X394 */
    int PAD;                            /* 0X398 */
    int PAD;                            /* 0X39C */
    int PAD;                            /* 0X3A0 */
    int PAD;                            /* 0X3A4 */
    int PAD;                            /* 0X3A8 */
    int PAD;                            /* 0X3AC */
    int PAD;                            /* 0X3B0 */
    int PAD;                            /* 0X3B4 */
    int PAD;                            /* 0X3B8 */
    int PAD;                            /* 0X3BC */
    int PAD;                            /* 0X3C0 */
    int PAD;                            /* 0X3C4 */
    int PAD;                            /* 0X3C8 */
    int PAD;                            /* 0X3CC */
    int PAD;                            /* 0X3D0 */
    int PAD;                            /* 0X3D4 */
    int PAD;                            /* 0X3D8 */
    int PAD;                            /* 0X3DC */
    int PAD;                            /* 0X3E0 */
    int PAD;                            /* 0X3E4 */
    int PAD;                            /* 0X3E8 */
    int PAD;                            /* 0X3EC */
    int PAD;                            /* 0X3F0 */
    int PAD;                            /* 0X3F4 */
    int PAD;                            /* 0X3F8 */
    int PAD;                            /* 0X3FC */
    int PAD;                            /* 0X400 */
    int PAD;                            /* 0X404 */
    int PAD;                            /* 0X408 */
    int PAD;                            /* 0X40C */
    int PAD;                            /* 0X410 */
    int PAD;                            /* 0X414 */
    int PAD;                            /* 0X418 */
    int PAD;                            /* 0X41C */
    int PAD;                            /* 0X420 */
    int PAD;                            /* 0X424 */
    int PAD;                            /* 0X428 */
    int PAD;                            /* 0X42C */
    int PAD;                            /* 0X430 */
    int PAD;                            /* 0X434 */
    int PAD;                            /* 0X438 */
    int PAD;                            /* 0X43C */
    int PAD;                            /* 0X440 */
    int PAD;                            /* 0X444 */
    int PAD;                            /* 0X448 */
    int PAD;                            /* 0X44C */
    int PAD;                            /* 0X450 */
    int PAD;                            /* 0X454 */
    int PAD;                            /* 0X458 */
    int PAD;                            /* 0X45C */
    int PAD;                            /* 0X460 */
    int PAD;                            /* 0X464 */
    int PAD;                            /* 0X468 */
    int PAD;                            /* 0X46C */
    int PAD;                            /* 0X470 */
    int PAD;                            /* 0X474 */
    int PAD;                            /* 0X478 */
    int PAD;                            /* 0X47C */
    int PAD;                            /* 0X480 */
    int PAD;                            /* 0X484 */
    int PAD;                            /* 0X488 */
    int PAD;                            /* 0X48C */
    int PAD;                            /* 0X490 */
    int PAD;                            /* 0X494 */
    int PAD;                            /* 0X498 */
    int PAD;                            /* 0X49C */
    int PAD;                            /* 0X4A0 */
    int PAD;                            /* 0X4A4 */
    int PAD;                            /* 0X4A8 */
    int PAD;                            /* 0X4AC */
    int PAD;                            /* 0X4B0 */
    int PAD;                            /* 0X4B4 */
    int PAD;                            /* 0X4B8 */
    int PAD;                            /* 0X4BC */
    int PAD;                            /* 0X4C0 */
    int PAD;                            /* 0X4C4 */
    int PAD;                            /* 0X4C8 */
    int PAD;                            /* 0X4CC */
    int PAD;                            /* 0X4D0 */
    int PAD;                            /* 0X4D4 */
    int PAD;                            /* 0X4D8 */
    int PAD;                            /* 0X4DC */
    int PAD;                            /* 0X4E0 */
    int PAD;                            /* 0X4E4 */
    int PAD;                            /* 0X4E8 */
    int PAD;                            /* 0X4EC */
    int PAD;                            /* 0X4F0 */
    int PAD;                            /* 0X4F4 */
    int PAD;                            /* 0X4F8 */
    int PAD;                            /* 0X4FC */
    int PAD;                            /* 0X500 */
    int PAD;                            /* 0X504 */
    int PAD;                            /* 0X508 */
    int PAD;                            /* 0X50C */
    short some_chanspec;                /* 0X510 */
    short PAD;                          /* 0X512 */
    int PAD;                            /* 0X514 */
    int PAD;                            /* 0X518 */
    int PAD;                            /* 0X51C */
    int PAD;                            /* 0X520 */
    int PAD;                            /* 0X524 */
    int PAD;                            /* 0X528 */
    int PAD;                            /* 0X52C */
    int PAD;                            /* 0X530 */
    int PAD;                            /* 0X534 */
    int PAD;                            /* 0X538 */
    int PAD;                            /* 0X53C */
    int PAD;                            /* 0X540 */
    int PAD;                            /* 0X544 */
    int PAD;                            /* 0X548 */
    int PAD;                            /* 0X54C */
    int PAD;                            /* 0X550 */
    int PAD;                            /* 0X554 */
    int PAD;                            /* 0X558 */
    int PAD;                            /* 0X55C */
    int PAD;                            /* 0X560 */
    int PAD;                            /* 0X564 */
    int PAD;                            /* 0X568 */
    int PAD;                            /* 0X56C */
    int PAD;                            /* 0X570 */
    int PAD;                            /* 0X574 */
    int PAD;                            /* 0X578 */
    int PAD;                            /* 0X57C */
    int PAD;                            /* 0X580 */
    int PAD;                            /* 0X584 */
    int PAD;                            /* 0X588 */
    int PAD;                            /* 0X58C */
    int PAD;                            /* 0X590 */
    int PAD;                            /* 0X594 */
    int PAD;                            /* 0X598 */
    int PAD;                            /* 0X59C */
    int PAD;                            /* 0X5A0 */
    int PAD;                            /* 0X5A4 */
    int PAD;                            /* 0X5A8 */
    int PAD;                            /* 0X5AC */
    int PAD;                            /* 0X5B0 */
    int PAD;                            /* 0X5B4 */
    int PAD;                            /* 0X5B8 */
    int PAD;                            /* 0X5BC */
    int PAD;                            /* 0X5C0 */
    int PAD;                            /* 0X5C4 */
    int PAD;                            /* 0X5C8 */
    int PAD;                            /* 0X5CC */
    int PAD;                            /* 0X5D0 */
    int PAD;                            /* 0X5D4 */
    int PAD;                            /* 0X5D8 */
    int PAD;                            /* 0X5DC */
    int PAD;                            /* 0X5E0 */
    int PAD;                            /* 0X5E4 */
    int PAD;                            /* 0X5E8 */
    int PAD;                            /* 0X5EC */
    int PAD;                            /* 0X5F0 */
    int PAD;                            /* 0X5F4 */
    int PAD;                            /* 0X5F8 */
    int PAD;                            /* 0X5FC */
    int PAD;                            /* 0X600 */
    int PAD;                            /* 0X604 */
    int PAD;                            /* 0X608 */
    int PAD;                            /* 0X60C */
    int PAD;                            /* 0X610 */
    int PAD;                            /* 0X614 */
    int PAD;                            /* 0X618 */
    int PAD;                            /* 0X61C */
    int PAD;                            /* 0X620 */
    int PAD;                            /* 0X624 */
    int PAD;                            /* 0X628 */
    int PAD;                            /* 0X62C */
    int PAD;                            /* 0X630 */
    int PAD;                            /* 0X634 */
    int PAD;                            /* 0X638 */
    int PAD;                            /* 0X63C */
    int PAD;                            /* 0X640 */
    int PAD;                            /* 0X644 */
    int PAD;                            /* 0X648 */
    int PAD;                            /* 0X64C */
    int PAD;                            /* 0X650 */
    int PAD;                            /* 0X654 */
    int PAD;                            /* 0X658 */
    int PAD;                            /* 0X65C */
    int PAD;                            /* 0X660 */
    int PAD;                            /* 0X664 */
    int PAD;                            /* 0X668 */
    int PAD;                            /* 0X66C */
    int PAD;                            /* 0X670 */
    int PAD;                            /* 0X674 */
    int PAD;                            /* 0X678 */
    int PAD;                            /* 0X67C */
    int PAD;                            /* 0X680 */
    int PAD;                            /* 0X684 */
    int PAD;                            /* 0X688 */
    int PAD;                            /* 0X68C */
    int PAD;                            /* 0X690 */
    int PAD;                            /* 0X694 */
    int PAD;                            /* 0X698 */
    int PAD;                            /* 0X69C */
    int PAD;                            /* 0X6A0 */
    int PAD;                            /* 0X6A4 */
    int PAD;                            /* 0X6A8 */
    int PAD;                            /* 0X6AC */
    int PAD;                            /* 0X6B0 */
    int PAD;                            /* 0X6B4 */
    int PAD;                            /* 0X6B8 */
    int PAD;                            /* 0X6BC */
    int PAD;                            /* 0X6C0 */
    int PAD;                            /* 0X6C4 */
    int PAD;                            /* 0X6C8 */
    int PAD;                            /* 0X6CC */
    int PAD;                            /* 0X6D0 */
    int PAD;                            /* 0X6D4 */
    int PAD;                            /* 0X6D8 */
    int PAD;                            /* 0X6DC */
    int PAD;                            /* 0X6E0 */
    int PAD;                            /* 0X6E4 */
    int PAD;                            /* 0X6E8 */
    int PAD;                            /* 0X6EC */
    int PAD;                            /* 0X6F0 */
    int PAD;                            /* 0X6F4 */
    int PAD;                            /* 0X6F8 */
    int PAD;                            /* 0X6FC */
    int PAD;                            /* 0X700 */
    int PAD;                            /* 0X704 */
    int PAD;                            /* 0X708 */
    int PAD;                            /* 0X70C */
    int PAD;                            /* 0X710 */
    int PAD;                            /* 0X714 */
    int PAD;                            /* 0X718 */
    int PAD;                            /* 0X71C */
    int PAD;                            /* 0X720 */
    int PAD;                            /* 0X724 */
    int PAD;                            /* 0X728 */
    int PAD;                            /* 0X72C */
    int PAD;                            /* 0X730 */
    int PAD;                            /* 0X734 */
    int PAD;                            /* 0X738 */
    int PAD;                            /* 0X73C */
    int PAD;                            /* 0X740 */
    int PAD;                            /* 0X744 */
    int PAD;                            /* 0X748 */
    int PAD;                            /* 0X74C */
    int PAD;                            /* 0X750 */
    int PAD;                            /* 0X754 */
    int PAD;                            /* 0X758 */
    int PAD;                            /* 0X75C */
    int PAD;                            /* 0X760 */
    int PAD;                            /* 0X764 */
    int PAD;                            /* 0X768 */
    int PAD;                            /* 0X76C */
    int PAD;                            /* 0X770 */
    int PAD;                            /* 0X774 */
    int PAD;                            /* 0X778 */
    int PAD;                            /* 0X77C */
    int PAD;                            /* 0X780 */
    int PAD;                            /* 0X784 */
    int PAD;                            /* 0X788 */
    int PAD;                            /* 0X78C */
    int PAD;                            /* 0X790 */
    int PAD;                            /* 0X794 */
    int PAD;                            /* 0X798 */
    int PAD;                            /* 0X79C */
    int PAD;                            /* 0X7A0 */
    int PAD;                            /* 0X7A4 */
    int PAD;                            /* 0X7A8 */
    int PAD;                            /* 0X7AC */
    int PAD;                            /* 0X7B0 */
    int PAD;                            /* 0X7B4 */
    int PAD;                            /* 0X7B8 */
    int PAD;                            /* 0X7BC */
    int PAD;                            /* 0X7C0 */
    int PAD;                            /* 0X7C4 */
    int PAD;                            /* 0X7C8 */
    int PAD;                            /* 0X7CC */
    int PAD;                            /* 0X7D0 */
    int PAD;                            /* 0X7D4 */
    int PAD;                            /* 0X7D8 */
    int PAD;                            /* 0X7DC */
    int PAD;                            /* 0X7E0 */
    int PAD;                            /* 0X7E4 */
    int PAD;                            /* 0X7E8 */
    int PAD;                            /* 0X7EC */
    int PAD;                            /* 0X7F0 */
    int PAD;                            /* 0X7F4 */
    int PAD;                            /* 0X7F8 */
    int PAD;                            /* 0X7FC */
    int PAD;                            /* 0X800 */
    void *active_queue;                 /* 0x804 */
};

struct wlc_pub {
    struct wlc_info *wlc;               /* 0x000 */
    int PAD;                            /* 0x004 */
    int PAD;                            /* 0x008 */
    int PAD;                            /* 0x00C */
    int PAD;                            /* 0x010 */
    int PAD;                            /* 0x014 */
    int PAD;                            /* 0x018 */
    int PAD;                            /* 0x01C */
    int PAD;                            /* 0x020 */
    char up_maybe;                      /* 0x024 */
    char field_25;                      /* 0x025 */
    char field_26;                      /* 0x026 */
    char field_27;                      /* 0x027 */
    struct tunables *tunables;          /* 0x028 */
    int PAD;                            /* 0x02C */
    int field_30;                       /* 0x030 */
    int PAD;                            /* 0x034 */
    int PAD;                            /* 0x038 */
    int PAD;                            /* 0x03C */
    int PAD;                            /* 0x040 */
    char PAD;                           /* 0x044 */
    char PAD;                           /* 0x045 */
    char field_46;                      /* 0x046 */
    char PAD;                           /* 0x047 */
    int PAD;                            /* 0x048 */
    char associated;                    /* 0x04C */
    char PAD;                           /* 0x04D */
    char PAD;                           /* 0x04E */
    char PAD;                           /* 0x04F */
    int PAD;                            /* 0x050 */
    char gap2[147];
    char is_amsdu; // @ 0xe7
} __attribute__((packed));

struct wlc_bsscfg {
    void *wlc;                          /* 0x000 */
    char associated;                    /* 0x004 */
    char PAD;                           /* 0x005 */
    char PAD;                           /* 0x006 */
    char PAD;                           /* 0x007 */
    int PAD;                            /* 0x008 */
    int PAD;                            /* 0x00C */
    int PAD;                            /* 0x010 */
    int PAD;                            /* 0x014 */
    int PAD;                            /* 0x018 */
    int PAD;                            /* 0x01C */
    int PAD;                            /* 0x020 */
    int PAD;                            /* 0x024 */
    int PAD;                            /* 0x028 */
    int PAD;                            /* 0x02C */
    int PAD;                            /* 0x030 */
    int PAD;                            /* 0x034 */
    int PAD;                            /* 0x038 */
    int PAD;                            /* 0x03C */
    int PAD;                            /* 0x040 */
    int PAD;                            /* 0x044 */
    int PAD;                            /* 0x048 */
    int PAD;                            /* 0x04C */
    int PAD;                            /* 0x050 */
    int PAD;                            /* 0x054 */
    int PAD;                            /* 0x058 */
    int PAD;                            /* 0x05C */
    int PAD;                            /* 0x060 */
    int PAD;                            /* 0x064 */
    int PAD;                            /* 0x068 */
    int PAD;                            /* 0x06C */
    int PAD;                            /* 0x070 */
    int PAD;                            /* 0x074 */
    int PAD;                            /* 0x078 */
    int PAD;                            /* 0x07C */
    int PAD;                            /* 0x080 */
    int PAD;                            /* 0x084 */
    int PAD;                            /* 0x088 */
    int PAD;                            /* 0x08C */
    int PAD;                            /* 0x090 */
    int PAD;                            /* 0x094 */
    int PAD;                            /* 0x098 */
    int PAD;                            /* 0x09C */
    int PAD;                            /* 0x0A0 */
    int PAD;                            /* 0x0A4 */
    int PAD;                            /* 0x0A8 */
    int PAD;                            /* 0x0AC */
    int PAD;                            /* 0x0B0 */
    int PAD;                            /* 0x0B4 */
    int PAD;                            /* 0x0B8 */
    int PAD;                            /* 0x0BC */
    int PAD;                            /* 0x0C0 */
    int PAD;                            /* 0x0C4 */
    int PAD;                            /* 0x0C8 */
    int PAD;                            /* 0x0CC */
    int PAD;                            /* 0x0D0 */
    int PAD;                            /* 0x0D4 */
    int PAD;                            /* 0x0D8 */
    int PAD;                            /* 0x0DC */
    int PAD;                            /* 0x0E0 */
    int PAD;                            /* 0x0E4 */
    int PAD;                            /* 0x0E8 */
    int PAD;                            /* 0x0EC */
    int PAD;                            /* 0x0F0 */
    int PAD;                            /* 0x0F4 */
    int PAD;                            /* 0x0F8 */
    int PAD;                            /* 0x0FC */
    int PAD;                            /* 0x100 */
    int PAD;                            /* 0x104 */
    int PAD;                            /* 0x108 */
    int PAD;                            /* 0x10C */
    int PAD;                            /* 0x110 */
    int PAD;                            /* 0x114 */
    int PAD;                            /* 0x118 */
    int PAD;                            /* 0x11C */
    int PAD;                            /* 0x120 */
    int PAD;                            /* 0x124 */
    int PAD;                            /* 0x128 */
    int PAD;                            /* 0x12C */
    int PAD;                            /* 0x130 */
    int PAD;                            /* 0x134 */
    int PAD;                            /* 0x138 */
    int PAD;                            /* 0x13C */
    int PAD;                            /* 0x140 */
    int PAD;                            /* 0x144 */
    int PAD;                            /* 0x148 */
    int PAD;                            /* 0x14C */
    int PAD;                            /* 0x150 */
    int PAD;                            /* 0x154 */
    int PAD;                            /* 0x158 */
    int PAD;                            /* 0x15C */
    int PAD;                            /* 0x160 */
    int PAD;                            /* 0x164 */
    int PAD;                            /* 0x168 */
    int PAD;                            /* 0x16C */
    int PAD;                            /* 0x170 */
    int PAD;                            /* 0x174 */
    int PAD;                            /* 0x178 */
    int PAD;                            /* 0x17C */
    int PAD;                            /* 0x180 */
    int PAD;                            /* 0x184 */
    int PAD;                            /* 0x188 */
    int PAD;                            /* 0x18C */
    int PAD;                            /* 0x190 */
    int PAD;                            /* 0x194 */
    int PAD;                            /* 0x198 */
    int PAD;                            /* 0x19C */
    int PAD;                            /* 0x1A0 */
    int PAD;                            /* 0x1A4 */
    int PAD;                            /* 0x1A8 */
    int PAD;                            /* 0x1AC */
    int PAD;                            /* 0x1B0 */
    int PAD;                            /* 0x1B4 */
    int PAD;                            /* 0x1B8 */
    int PAD;                            /* 0x1BC */
    int PAD;                            /* 0x1C0 */
    int PAD;                            /* 0x1C4 */
    int PAD;                            /* 0x1C8 */
    int PAD;                            /* 0x1CC */
    int PAD;                            /* 0x1D0 */
    int PAD;                            /* 0x1D4 */
    int PAD;                            /* 0x1D8 */
    int PAD;                            /* 0x1DC */
    int PAD;                            /* 0x1E0 */
    int PAD;                            /* 0x1E4 */
    int PAD;                            /* 0x1E8 */
    int PAD;                            /* 0x1EC */
    int PAD;                            /* 0x1F0 */
    int PAD;                            /* 0x1F4 */
    int PAD;                            /* 0x1F8 */
    int PAD;                            /* 0x1FC */
    int PAD;                            /* 0x200 */
    int PAD;                            /* 0x204 */
    int PAD;                            /* 0x208 */
    int PAD;                            /* 0x20C */
    int PAD;                            /* 0x210 */
    int PAD;                            /* 0x214 */
    int PAD;                            /* 0x218 */
    int PAD;                            /* 0x21C */
    int PAD;                            /* 0x220 */
    int PAD;                            /* 0x224 */
    int PAD;                            /* 0x228 */
    int PAD;                            /* 0x22C */
    int PAD;                            /* 0x230 */
    int PAD;                            /* 0x234 */
    int PAD;                            /* 0x238 */
    int PAD;                            /* 0x23C */
    int PAD;                            /* 0x240 */
    int PAD;                            /* 0x244 */
    int PAD;                            /* 0x248 */
    int PAD;                            /* 0x24C */
    int PAD;                            /* 0x250 */
    int PAD;                            /* 0x254 */
    int PAD;                            /* 0x258 */
    int PAD;                            /* 0x25C */
    int PAD;                            /* 0x260 */
    int PAD;                            /* 0x264 */
    int PAD;                            /* 0x268 */
    int PAD;                            /* 0x26C */
    int PAD;                            /* 0x270 */
    int PAD;                            /* 0x274 */
    int PAD;                            /* 0x278 */
    int PAD;                            /* 0x27C */
    int PAD;                            /* 0x280 */
    int PAD;                            /* 0x284 */
    int PAD;                            /* 0x288 */
    int PAD;                            /* 0x28C */
    int PAD;                            /* 0x290 */
    int PAD;                            /* 0x294 */
    int PAD;                            /* 0x298 */
    int PAD;                            /* 0x29C */
    int PAD;                            /* 0x2A0 */
    int PAD;                            /* 0x2A4 */
    int PAD;                            /* 0x2A8 */
    int PAD;                            /* 0x2AC */
    int PAD;                            /* 0x2B0 */
    int PAD;                            /* 0x2B4 */
    int PAD;                            /* 0x2B8 */
    int PAD;                            /* 0x2BC */
    int PAD;                            /* 0x2C0 */
    int PAD;                            /* 0x2C4 */
    int PAD;                            /* 0x2C8 */
    int PAD;                            /* 0x2CC */
    int PAD;                            /* 0x2D0 */
    int PAD;                            /* 0x2D4 */
    int PAD;                            /* 0x2D8 */
    int PAD;                            /* 0x2DC */
    int PAD;                            /* 0x2E0 */
    int PAD;                            /* 0x2E4 */
    int PAD;                            /* 0x2E8 */
    int PAD;                            /* 0x2EC */
    int PAD;                            /* 0x2F0 */
    int PAD;                            /* 0x2F4 */
    int PAD;                            /* 0x2F8 */
    int PAD;                            /* 0x2FC */
    int PAD;                            /* 0X300 */
    int PAD;                            /* 0X304 */
    int PAD;                            /* 0X308 */
    int PAD;                            /* 0X30C */
    int PAD;                            /* 0X310 */
    int PAD;                            /* 0X314 */
    int PAD;                            /* 0X318 */
    int PAD;                            /* 0X31C */
    int PAD;                            /* 0X320 */
    int PAD;                            /* 0X324 */
    int PAD;                            /* 0X328 */
    int PAD;                            /* 0X32C */
    int PAD;                            /* 0X330 */
    int PAD;                            /* 0X334 */
    int PAD;                            /* 0X338 */
    int PAD;                            /* 0X33C */
    int PAD;                            /* 0X340 */
    int PAD;                            /* 0X344 */
    int PAD;                            /* 0X348 */
    int PAD;                            /* 0X34C */
    int PAD;                            /* 0X350 */
    int PAD;                            /* 0X354 */
    int PAD;                            /* 0X358 */
    int PAD;                            /* 0X35C */
    int PAD;                            /* 0X360 */
    int PAD;                            /* 0X364 */
    int PAD;                            /* 0X368 */
    int PAD;                            /* 0X36C */
    int PAD;                            /* 0X370 */
    int PAD;                            /* 0X374 */
    int PAD;                            /* 0X378 */
    int PAD;                            /* 0X37C */
    int PAD;                            /* 0X380 */
    int PAD;                            /* 0X384 */
    int PAD;                            /* 0X388 */
    int PAD;                            /* 0X38C */
    int PAD;                            /* 0X390 */
    int PAD;                            /* 0X394 */
    int PAD;                            /* 0X398 */
    int PAD;                            /* 0X39C */
    int PAD;                            /* 0X3A0 */
    int PAD;                            /* 0X3A4 */
    int PAD;                            /* 0X3A8 */
    int PAD;                            /* 0X3AC */
    int PAD;                            /* 0X3B0 */
    int PAD;                            /* 0X3B4 */
    int PAD;                            /* 0X3B8 */
    int PAD;                            /* 0X3BC */
    int PAD;                            /* 0X3C0 */
    int PAD;                            /* 0X3C4 */
    int PAD;                            /* 0X3C8 */
    int PAD;                            /* 0X3CC */
    int PAD;                            /* 0X3D0 */
    int PAD;                            /* 0X3D4 */
    int PAD;                            /* 0X3D8 */
    int PAD;                            /* 0X3DC */
    int PAD;                            /* 0X3E0 */
    int PAD;                            /* 0X3E4 */
    int PAD;                            /* 0X3E8 */
    int PAD;                            /* 0X3EC */
    int PAD;                            /* 0X3F0 */
    int PAD;                            /* 0X3F4 */
    int PAD;                            /* 0X3F8 */
    int PAD;                            /* 0X3FC */
    int PAD;                            /* 0X400 */
    int PAD;                            /* 0X404 */
    int PAD;                            /* 0X408 */
    int PAD;                            /* 0X40C */
    int PAD;                            /* 0X410 */
    int PAD;                            /* 0X414 */
    int PAD;                            /* 0X418 */
    int PAD;                            /* 0X41C */
    int PAD;                            /* 0X420 */
    int PAD;                            /* 0X424 */
    int PAD;                            /* 0X428 */
    int PAD;                            /* 0X42C */
    int PAD;                            /* 0X430 */
    int PAD;                            /* 0X434 */
    int PAD;                            /* 0X438 */
    int PAD;                            /* 0X43C */
    int PAD;                            /* 0X440 */
    int PAD;                            /* 0X444 */
    int PAD;                            /* 0X448 */
    int PAD;                            /* 0X44C */
    int PAD;                            /* 0X450 */
    int PAD;                            /* 0X454 */
    int PAD;                            /* 0X458 */
    int PAD;                            /* 0X45C */
    int PAD;                            /* 0X460 */
    int PAD;                            /* 0X464 */
    int PAD;                            /* 0X468 */
    int PAD;                            /* 0X46C */
    int PAD;                            /* 0X470 */
    int PAD;                            /* 0X474 */
    int PAD;                            /* 0X478 */
    int PAD;                            /* 0X47C */
    int PAD;                            /* 0X480 */
    int PAD;                            /* 0X484 */
    int PAD;                            /* 0X488 */
    int PAD;                            /* 0X48C */
    int PAD;                            /* 0X490 */
    int PAD;                            /* 0X494 */
    int PAD;                            /* 0X498 */
    int PAD;                            /* 0X49C */
    int PAD;                            /* 0X4A0 */
    int PAD;                            /* 0X4A4 */
    int PAD;                            /* 0X4A8 */
    int PAD;                            /* 0X4AC */
    int PAD;                            /* 0X4B0 */
    int PAD;                            /* 0X4B4 */
    int PAD;                            /* 0X4B8 */
    int PAD;                            /* 0X4BC */
    int PAD;                            /* 0X4C0 */
    int PAD;                            /* 0X4C4 */
    int PAD;                            /* 0X4C8 */
    int PAD;                            /* 0X4CC */
    int PAD;                            /* 0X4D0 */
    int PAD;                            /* 0X4D4 */
    int PAD;                            /* 0X4D8 */
    int PAD;                            /* 0X4DC */
    int PAD;                            /* 0X4E0 */
    int PAD;                            /* 0X4E4 */
    int PAD;                            /* 0X4E8 */
    int PAD;                            /* 0X4EC */
    int PAD;                            /* 0X4F0 */
    int PAD;                            /* 0X4F4 */
    int PAD;                            /* 0X4F8 */
    int PAD;                            /* 0X4FC */
    int PAD;                            /* 0X500 */
    int PAD;                            /* 0X504 */
    int PAD;                            /* 0X508 */
    int PAD;                            /* 0X50C */
    int PAD;                            /* 0X510 */
    int PAD;                            /* 0X514 */
    int PAD;                            /* 0X518 */
    int PAD;                            /* 0X51C */
    int PAD;                            /* 0X520 */
    int PAD;                            /* 0X524 */
    int PAD;                            /* 0X528 */
    int PAD;                            /* 0X52C */
    int PAD;                            /* 0X530 */
    int PAD;                            /* 0X534 */
    int PAD;                            /* 0X538 */
    int PAD;                            /* 0X53C */
    int PAD;                            /* 0X540 */
    short PAD;                          /* 0X544 */
    short field_546;                    /* 0X546 */
    int PAD;                            /* 0X548 */
    int PAD;                            /* 0X54C */
    int PAD;                            /* 0X550 */
    int PAD;                            /* 0X554 */
    int PAD;                            /* 0X558 */
    int PAD;                            /* 0X55C */
    int PAD;                            /* 0X560 */
    int PAD;                            /* 0X564 */
    int PAD;                            /* 0X568 */
    int PAD;                            /* 0X56C */
    int PAD;                            /* 0X570 */
    int PAD;                            /* 0X574 */
    int PAD;                            /* 0X578 */
    int PAD;                            /* 0X57C */
    int PAD;                            /* 0X580 */
    int PAD;                            /* 0X584 */
} __attribute__((packed));

struct hnddma_pub {
    void *di_fn;                    /* DMA function pointers */
    unsigned int txavail;           /* # free tx descriptors */
    unsigned int dmactrlflags;      /* dma control flags */
    /* rx error counters */
    unsigned int rxgiants;          /* rx giant frames */
    unsigned int rxnobuf;           /* rx out of dma descriptors */
    /* tx error counters */
    unsigned int txnobuf;           /* tx out of dma descriptors */
} __attribute__((packed));

struct dma_info {
    struct hnddma_pub hnddma;   /* exported structure */
    int msg_level;              /* message level pointer */
    int something;
    char name[8];               /* callers name for diag msgs */
    void *osh;                  
    void *sih;                  
    bool dma64;                 /* this dma engine is operating in 64-bit mode */
    bool addrext;               /* this dma engine supports DmaExtendedAddrChanges */
    char gap2[2];
    void *txregs;               /* 64-bit dma tx engine registers */
    void *rxregs;               /* 64-bit dma rx engine registers */
    void *txd;                  /* pointer to dma64 tx descriptor ring */
    void *rxd;                  /* pointer to dma64 rx descriptor ring */
    short dmadesc_align;        /* alignment requirement for dma descriptors */
    short ntxd;                 /* # tx descriptors tunable */
    short txin;                 /* index of next descriptor to reclaim */
    short txout;                /* index of next descriptor to post */
    void **txp;                 /* pointer to parallel array of pointers to packets */
    void *tx_dmah;              /* DMA MAP meta-data handle */
    int txp_dmah;               
    int txdpa;                  /* Aligned physical address of descriptor ring */
    int txdpaorig;              /* Original physical address of descriptor ring */
    short txdalign;             /* #bytes added to alloc'd mem to align txd */
    int txdalloc;               /* #bytes allocated for the ring */
    int xmtptrbase;             /* When using unaligned descriptors, the ptr register
                                 * is not just an index, it needs all 13 bits to be
                                 * an offset from the addr register.
                                 */
    short PAD;
    short nrxd;
    short rxin;
    short rxout;
    short PAD;
    void **rxp;
    int PAD;
    int PAD;
    int rxdpa;
    short rxdalign;
    short PAD;
    int PAD;
    int PAD;
    int PAD;
    int rxbufsize;              /* rx buffer size in bytes, not including the extra headroom */
    int rxextrahdrroom;         /* extra rx headroom. */

} __attribute__((packed));

struct intctrlregs {
    unsigned int intstatus;
    unsigned int intmask;
};

/* read: 32-bit register that can be read as 32-bit or as 2 16-bit
 * write: only low 16b-it half can be written
 */
union pmqreg {
    unsigned int pmqhostdata;    /* read only! */
    struct {
        unsigned short pmqctrlstatus;  /* read/write */
        unsigned short PAD;
    } w;
};

/* dma registers per channel(xmt or rcv) */
struct dma64regs {
    unsigned int control;    /* enable, et al */
    unsigned int ptr;    /* last descriptor posted to chip */
    unsigned int addrlow;    /* desc ring base address low 32-bits (8K aligned) */
    unsigned int addrhigh;   /* desc ring base address bits 63:32 (8K aligned) */
    unsigned int status0;    /* current descriptor, xmt state */
    unsigned int status1;    /* active descriptor, xmt error */
};

/* 4byte-wide pio register set per channel(xmt or rcv) */
struct pio4regs {
    unsigned int fifocontrol;
    unsigned int fifodata;
};

struct fifo64 {
    struct dma64regs dmaxmt;    /* dma tx */
    struct pio4regs piotx;  /* pio tx */
    struct dma64regs dmarcv;    /* dma rx */
    struct pio4regs piorx;  /* pio rx */
};

struct dma32diag {  /* diag access */
    unsigned int fifoaddr;   /* diag address */
    unsigned int fifodatalow;    /* low 32bits of data */
    unsigned int fifodatahigh;   /* high 32bits of data */
    unsigned int pad;        /* reserved */
};

/*
 * Host Interface Registers
 */
struct d11regs {
    /* Device Control ("semi-standard host registers") */
    unsigned int PAD[3];     /* 0x0 - 0x8 */
    unsigned int biststatus; /* 0xC */
    unsigned int biststatus2;    /* 0x10 */
    unsigned int PAD;        /* 0x14 */
    unsigned int gptimer;        /* 0x18 */
    unsigned int usectimer;  /* 0x1c *//* for corerev >= 26 */

    /* Interrupt Control *//* 0x20 */
    struct intctrlregs intctrlregs[8];

    unsigned int PAD[40];        /* 0x60 - 0xFC */

    unsigned int intrcvlazy[4];  /* 0x100 - 0x10C */

    unsigned int PAD[4];     /* 0x110 - 0x11c */

    unsigned int maccontrol; /* 0x120 */
    unsigned int maccommand; /* 0x124 */
    unsigned int macintstatus;   /* 0x128 */
    unsigned int macintmask; /* 0x12C */

    /* Transmit Template Access */
    unsigned int tplatewrptr;    /* 0x130 */
    unsigned int tplatewrdata;   /* 0x134 */
    unsigned int PAD[2];     /* 0x138 - 0x13C */

    /* PMQ registers */
    union pmqreg pmqreg;    /* 0x140 */
    unsigned int pmqpatl;        /* 0x144 */
    unsigned int pmqpath;        /* 0x148 */
    unsigned int PAD;        /* 0x14C */

    unsigned int chnstatus;  /* 0x150 */
    unsigned int psmdebug;   /* 0x154 */
    unsigned int phydebug;   /* 0x158 */
    unsigned int machwcap;   /* 0x15C */

    /* Extended Internal Objects */
    unsigned int objaddr;        /* 0x160 */
    unsigned int objdata;        /* 0x164 */
    unsigned int PAD[2];     /* 0x168 - 0x16c */

    unsigned int frmtxstatus;    /* 0x170 */
    unsigned int frmtxstatus2;   /* 0x174 */
    unsigned int PAD[2];     /* 0x178 - 0x17c */

    /* TSF host access */
    unsigned int tsf_timerlow;   /* 0x180 */
    unsigned int tsf_timerhigh;  /* 0x184 */
    unsigned int tsf_cfprep; /* 0x188 */
    unsigned int tsf_cfpstart;   /* 0x18c */
    unsigned int tsf_cfpmaxdur32;    /* 0x190 */
    unsigned int PAD[3];     /* 0x194 - 0x19c */

    unsigned int maccontrol1;    /* 0x1a0 */
    unsigned int machwcap1;  /* 0x1a4 */
    unsigned int PAD[14];        /* 0x1a8 - 0x1dc */

    /* Clock control and hardware workarounds*/
    unsigned int clk_ctl_st; /* 0x1e0 */
    unsigned int hw_war;
    unsigned int d11_phypllctl;  /* the phypll request/avail bits are
                 * moved to clk_ctl_st
                 */
    unsigned int PAD[5];     /* 0x1ec - 0x1fc */

    /* 0x200-0x37F dma/pio registers */
    struct fifo64 fifo64regs[6];

    /* FIFO diagnostic port access */
    struct dma32diag dmafifo;   /* 0x380 - 0x38C */

    unsigned int aggfifocnt; /* 0x390 */
    unsigned int aggfifodata;    /* 0x394 */
    unsigned int PAD[16];        /* 0x398 - 0x3d4 */
    unsigned short radioregaddr;   /* 0x3d8 */
    unsigned short radioregdata;   /* 0x3da */

    /*
     * time delay between the change on rf disable input and
     * radio shutdown
     */
    unsigned int rfdisabledly;   /* 0x3DC */

    /* PHY register access */
    unsigned short phyversion; /* 0x3e0 - 0x0 */
    unsigned short phybbconfig;    /* 0x3e2 - 0x1 */
    unsigned short phyadcbias; /* 0x3e4 - 0x2  Bphy only */
    unsigned short phyanacore; /* 0x3e6 - 0x3  pwwrdwn on aphy */
    unsigned short phyrxstatus0;   /* 0x3e8 - 0x4 */
    unsigned short phyrxstatus1;   /* 0x3ea - 0x5 */
    unsigned short phycrsth;   /* 0x3ec - 0x6 */
    unsigned short phytxerror; /* 0x3ee - 0x7 */
    unsigned short phychannel; /* 0x3f0 - 0x8 */
    unsigned short PAD[1];     /* 0x3f2 - 0x9 */
    unsigned short phytest;        /* 0x3f4 - 0xa */
    unsigned short phy4waddr;  /* 0x3f6 - 0xb */
    unsigned short phy4wdatahi;    /* 0x3f8 - 0xc */
    unsigned short phy4wdatalo;    /* 0x3fa - 0xd */
    unsigned short phyregaddr; /* 0x3fc - 0xe */
    unsigned short phyregdata; /* 0x3fe - 0xf */

    /* IHR *//* 0x400 - 0x7FE */

    /* RXE Block */
    unsigned short PAD;                 /* SPR_RXE_0x00                     0x400 */
    unsigned short PAD;                 /* SPR_RXE_Copy_Offset              0x402 */
    unsigned short PAD;                 /* SPR_RXE_Copy_Length              0x404 */
    unsigned short rcv_fifo_ctl;        /* SPR_RXE_FIFOCTL0                 0x406 */
    unsigned short PAD;                 /* SPR_RXE_FIFOCTL1                 0x408 */
    unsigned short rcv_frm_cnt;         /* SPR_Received_Frame_Count         0x40a */
    unsigned short PAD;                 /* SPR_RXE_0x0c                     0x40c */
    unsigned short PAD;                 /* SPR_RXE_RXHDR_OFFSET             0x40e */
    unsigned short PAD;                 /* SPR_RXE_RXHDR_LEN                0x410 */
    unsigned short PAD;                 /* SPR_RXE_PHYRXSTAT0               0x412 */
    unsigned short rssi;                /* SPR_RXE_PHYRXSTAT1               0x414 */
    unsigned short PAD;                 /* SPR_RXE_0x16                     0x416 */
    unsigned short PAD;                 /* SPR_RXE_FRAMELEN                 0x418 */
    unsigned short PAD;                 /* SPR_RXE_0x1a                     0x41a */
    unsigned short PAD;                 /* SPR_RXE_ENCODING                 0x41c */
    unsigned short PAD;                 /* SPR_RXE_0x1e                     0x41e */
    unsigned short rcm_ctl;             /* SPR_RCM_Control                  0x420 */
    unsigned short rcm_mat_data;        /* SPR_RCM_Match_Data               0x422 */
    unsigned short rcm_mat_mask;        /* SPR_RCM_Match_Mask               0x424 */
    unsigned short rcm_mat_dly;         /* SPR_RCM_Match_Delay              0x426 */
    unsigned short rcm_cond_mask_l;     /* SPR_RCM_Condition_Mask_Low       0x428 */
    unsigned short rcm_cond_mask_h;     /* SPR_RCM_Condition_Mask_High      0x42A */
    unsigned short rcm_cond_dly;        /* SPR_RCM_Condition_Delay          0x42C */
    unsigned short PAD;                 /* SPR_RXE_0x2e                     0x42E */
    unsigned short ext_ihr_addr;        /* SPR_Ext_IHR_Address              0x430 */
    unsigned short ext_ihr_data;        /* SPR_Ext_IHR_Data                 0x432 */
    unsigned short rxe_phyrs_2;         /* SPR_RXE_PHYRXSTAT2               0x434 */
    unsigned short rxe_phyrs_3;         /* SPR_RXE_PHYRXSTAT3               0x436 */
    unsigned short phy_mode;            /* SPR_PHY_Mode                     0x438 */
    unsigned short rcmta_ctl;           /* SPR_RCM_TA_Control               0x43a */
    unsigned short rcmta_size;          /* SPR_RCM_TA_Size                  0x43c */
    unsigned short rcmta_addr0;         /* SPR_RCM_TA_Address_0             0x43e */
    unsigned short rcmta_addr1;         /* SPR_RCM_TA_Address_1             0x440 */
    unsigned short rcmta_addr2;         /* SPR_RCM_TA_Address_2             0x442 */
    unsigned short PAD[30];             /* SPR_RXE_0x44 ... 0x7e            0x444 */


    /* PSM Block *//* 0x480 - 0x500 */

    unsigned short PAD;                 /* SPR_MAC_MAX_NAP                  0x480 */
    unsigned short psm_maccontrol_h;    /* SPR_MAC_CTLHI                    0x482 */
    unsigned short psm_macintstatus_l;  /* SPR_MAC_IRQLO                    0x484 */
    unsigned short psm_macintstatus_h;  /* SPR_MAC_IRQHI                    0x486 */
    unsigned short psm_macintmask_l;    /* SPR_MAC_IRQMASKLO                0x488 */
    unsigned short psm_macintmask_h;    /* SPR_MAC_IRQMASKHI                0x48A */
    unsigned short psm_0x0c;            /* SPR_PSM_0x0c                     0x48C */
    unsigned short psm_maccommand;      /* SPR_MAC_CMD                      0x48E */
    unsigned short psm_brc;             /* SPR_BRC                          0x490 */
    unsigned short psm_phy_hdr_param;   /* SPR_PHY_HDR_Parameter            0x492 */
    unsigned short psm_postcard;        /* SPR_Postcard                     0x494 */
    unsigned short psm_pcard_loc_l;     /* SPR_Postcard_Location_Low        0x496 */
    unsigned short psm_pcard_loc_h;     /* SPR_Postcard_Location_High       0x498 */
    unsigned short psm_gpio_in;         /* SPR_GPIO_IN                      0x49A */
    unsigned short psm_gpio_out;        /* SPR_GPIO_OUT                     0x49C */
    unsigned short psm_gpio_oe;         /* SPR_GPIO_OUTEN                   0x49E */

    unsigned short psm_bred_0;          /* SPR_BRED0                        0x4A0 */
    unsigned short psm_bred_1;          /* SPR_BRED1                        0x4A2 */
    unsigned short psm_bred_2;          /* SPR_BRED2                        0x4A4 */
    unsigned short psm_bred_3;          /* SPR_BRED3                        0x4A6 */
    unsigned short psm_brcl_0;          /* SPR_BRCL0                        0x4A8 */
    unsigned short psm_brcl_1;          /* SPR_BRCL1                        0x4AA */
    unsigned short psm_brcl_2;          /* SPR_BRCL2                        0x4AC */
    unsigned short psm_brcl_3;          /* SPR_BRCL3                        0x4AE */
    unsigned short psm_brpo_0;          /* SPR_BRPO0                        0x4B0 */
    unsigned short psm_brpo_1;          /* SPR_BRPO1                        0x4B2 */
    unsigned short psm_brpo_2;          /* SPR_BRPO2                        0x4B4 */
    unsigned short psm_brpo_3;          /* SPR_BRPO3                        0x4B6 */
    unsigned short psm_brwk_0;          /* SPR_BRWK0                        0x4B8 */
    unsigned short psm_brwk_1;          /* SPR_BRWK1                        0x4BA */
    unsigned short psm_brwk_2;          /* SPR_BRWK2                        0x4BC */
    unsigned short psm_brwk_3;          /* SPR_BRWK3                        0x4BE */

    unsigned short psm_base_0;          /* SPR_BASE0 - Offset Register 0    0x4C0 */
    unsigned short psm_base_1;          /* SPR_BASE1 - Offset Register 1    0x4C2 */
    unsigned short psm_base_2;          /* SPR_BASE2 - Offset Register 2    0x4C4 */
    unsigned short psm_base_3;          /* SPR_BASE3 - Offset Register 3    0x4C6 */
    unsigned short psm_base_4;          /* SPR_BASE4 - Offset Register 4    0x4C8 */
    unsigned short psm_base_5;          /* SPR_BASE5 - Offset Register 5    0x4CA */
    unsigned short psm_base_6;          /* SPR_BASE6 - Do not use (broken)  0x4CC */
    unsigned short psm_ihr_err;         /* SPR_PSM_0x4e                     0x4CE */
    unsigned short psm_pc_reg_0;        /* SPR_PC0 - Link Register 0        0x4D0 */
    unsigned short psm_pc_reg_1;        /* SPR_PC1 - Link Register 1        0x4D2 */
    unsigned short psm_pc_reg_2;        /* SPR_PC2 - Link Register 2        0x4D4 */
    unsigned short psm_pc_reg_3;        /* SPR_PC2 - Link Register 6        0x4D6 */
    unsigned short psm_brc_1;           /* SPR_PSM_COND - PSM external condition bits 0x4D8 */
    unsigned short PAD;                 /* SPR_PSM_0x5a ... 0x7e            0x4DA */
    unsigned short PAD;                 /* SPR_PSM_0x5c                     0x4DC */
    unsigned short PAD;                 /* SPR_PSM_0x5e                     0x4DE */
    unsigned short PAD;                 /* SPR_PSM_0x60                     0x4E0 */
    unsigned short PAD;                 /* SPR_PSM_0x62                     0x4E2 */
    unsigned short PAD;                 /* SPR_PSM_0x64                     0x4E4 */
    unsigned short PAD;                 /* SPR_PSM_0x66                     0x4E6 */
    unsigned short PAD;                 /* SPR_PSM_0x68                     0x4E8 */
    unsigned short PAD;                 /* SPR_PSM_0x6a                     0x4EA */
    unsigned short PAD;                 /* SPR_PSM_0x6c                     0x4EC */
    unsigned short PAD;                 /* SPR_PSM_0x6e                     0x4EE */
    unsigned short psm_corectlsts;      /* SPR_PSM_0x70                     0x4F0 *//* Corerev >= 13 */  
    unsigned short PAD;                 /* SPR_PSM_0x72                     0x4F2 */
    unsigned short PAD;                 /* SPR_PSM_0x74                     0x4F4 */
    unsigned short PAD;                 /* SPR_PSM_0x76                     0x4F6 */
    unsigned short PAD;                 /* SPR_PSM_0x78                     0x4F8 */
    unsigned short PAD;                 /* SPR_PSM_0x7a                     0x4FA */
    unsigned short PAD;                 /* SPR_PSM_0x7c                     0x4FC */
    unsigned short PAD;                 /* SPR_PSM_0x7e                     0x4FE */

    /* TXE0 Block *//* 0x500 - 0x580 */
    unsigned short txe_ctl;             /* SPR_TXE0_CTL                     0x500 */
    unsigned short txe_aux;             /* SPR_TXE0_AUX                     0x502 */
    unsigned short txe_ts_loc;          /* SPR_TXE0_TS_LOC                  0x504 */
    unsigned short txe_time_out;        /* SPR_TXE0_TIMEOUT                 0x506 */
    unsigned short txe_wm_0;            /* SPR_TXE0_WM0                     0x508 */
    unsigned short txe_wm_1;            /* SPR_TXE0_WM1                     0x50A */
    unsigned short txe_phyctl;          /* SPR_TXE0_PHY_CTL                 0x50C */
    unsigned short txe_status;          /* SPR_TXE0_STATUS                  0x50E */
    unsigned short txe_mmplcp0;         /* SPR_TXE0_0x10                    0x510 */
    unsigned short txe_mmplcp1;         /* SPR_TXE0_0x12                    0x512 */
    unsigned short txe_phyctl1;         /* SPR_TXE0_0x14                    0x514 */

    unsigned short PAD;                 /* SPR_TXE0_0x16                    0x516 */
    unsigned short PAD;                 /* SPR_TX_STATUS0                   0x518 */
    unsigned short PAD;                 /* SPR_TX_STATUS1                   0x51a */
    unsigned short PAD;                 /* SPR_TX_STATUS2                   0x51c */
    unsigned short PAD;                 /* SPR_TX_STATUS3                   0x51e */

    /* Transmit control */
    unsigned short xmtfifodef;          /* SPR_TXE0_FIFO_Def                0x520 */
    unsigned short xmtfifo_frame_cnt;   /* SPR_TXE0_0x22                    0x522 *//* Corerev >= 16 */
    unsigned short xmtfifo_byte_cnt;    /* SPR_TXE0_0x24                    0x524 *//* Corerev >= 16 */
    unsigned short xmtfifo_head;        /* SPR_TXE0_0x26                    0x526 *//* Corerev >= 16 */
    unsigned short xmtfifo_rd_ptr;      /* SPR_TXE0_0x28                    0x528 *//* Corerev >= 16 */
    unsigned short xmtfifo_wr_ptr;      /* SPR_TXE0_0x2a                    0x52A *//* Corerev >= 16 */
    unsigned short xmtfifodef1;         /* SPR_TXE0_0x2c                    0x52C *//* Corerev >= 16 */

    unsigned short PAD;                 /* SPR_TXE0_0x2e                    0x52E */
    unsigned short PAD;                 /* SPR_TXE0_0x30                    0x530 */
    unsigned short PAD;                 /* SPR_TXE0_0x32                    0x532 */
    unsigned short PAD;                 /* SPR_TXE0_0x34                    0x534 */
    unsigned short PAD;                 /* SPR_TXE0_0x36                    0x536 */
    unsigned short PAD;                 /* SPR_TXE0_0x38                    0x538 */
    unsigned short PAD;                 /* SPR_TXE0_0x3a                    0x53A */
    unsigned short PAD;                 /* SPR_TXE0_0x3c                    0x53C */
    unsigned short PAD;                 /* SPR_TXE0_0x3e                    0x53E */

    unsigned short xmtfifocmd;          /* SPR_TXE0_FIFO_CMD                0x540 */
    unsigned short xmtfifoflush;        /* SPR_TXE0_FIFO_FLUSH              0x542 */
    unsigned short xmtfifothresh;       /* SPR_TXE0_FIFO_THRES              0x544 */
    unsigned short xmtfifordy;          /* SPR_TXE0_FIFO_RDY                0x546 */
    unsigned short xmtfifoprirdy;       /* SPR_TXE0_FIFO_PRI_RDY            0x548 */
    unsigned short xmtfiforqpri;        /* SPR_TXE0_FIFO_RQ_PRI             0x54A */
    unsigned short xmttplatetxptr;      /* SPR_TXE0_Template_TX_Pointer     0x54C */
    unsigned short PAD;                 /* SPR_TXE0_0x4e                    0x54E */
    unsigned short xmttplateptr;        /* SPR_TXE0_Template_Pointer        0x550 */
    unsigned short smpl_clct_strptr;    /* SPR_TXE0_0x52                    0x552 *//* Corerev >= 22 */
    unsigned short smpl_clct_stpptr;    /* SPR_TXE0_0x54                    0x554 *//* Corerev >= 22 */
    unsigned short smpl_clct_curptr;    /* SPR_TXE0_0x56                    0x556 *//* Corerev >= 22 */
    unsigned short PAD;                 /* SPR_TXE0_0x58                    0x558 */
    unsigned short PAD;                 /* SPR_TXE0_0x5a                    0x55A */
    unsigned short PAD;                 /* SPR_TXE0_0x5c                    0x55C */
    unsigned short PAD;                 /* SPR_TXE0_0x5e                    0x55E */
    unsigned short xmttplatedatalo;     /* SPR_TXE0_Template_Data_Low       0x560 */
    unsigned short xmttplatedatahi;     /* SPR_TXE0_Template_Data_High      0x562 */

    unsigned short PAD;                 /* SPR_TXE0_0x64                    0x564 */
    unsigned short PAD;                 /* SPR_TXE0_0x66                    0x566 */

    unsigned short xmtsel;              /* SPR_TXE0_SELECT                  0x568 */
    unsigned short xmttxcnt;            /* 0x56A */
    unsigned short xmttxshmaddr;        /* 0x56C */

    unsigned short PAD[0x09];  /* 0x56E - 0x57E */

    /* TXE1 Block */
    unsigned short PAD[0x40];  /* 0x580 - 0x5FE */

    /* TSF Block */
    unsigned short PAD[0X02];  /* 0x600 - 0x602 */
    unsigned short tsf_cfpstrt_l;  /* 0x604 */
    unsigned short tsf_cfpstrt_h;  /* 0x606 */
    unsigned short PAD[0X05];  /* 0x608 - 0x610 */
    unsigned short tsf_cfppretbtt; /* 0x612 */
    unsigned short PAD[0XD];   /* 0x614 - 0x62C */
    unsigned short tsf_clk_frac_l; /* 0x62E */
    unsigned short tsf_clk_frac_h; /* 0x630 */
    unsigned short PAD[0X14];  /* 0x632 - 0x658 */
    unsigned short tsf_random; /* 0x65A */
    unsigned short PAD[0x05];  /* 0x65C - 0x664 */
    /* GPTimer 2 registers */
    unsigned short tsf_gpt2_stat;  /* 0x666 */
    unsigned short tsf_gpt2_ctr_l; /* 0x668 */
    unsigned short tsf_gpt2_ctr_h; /* 0x66A */
    unsigned short tsf_gpt2_val_l; /* 0x66C */
    unsigned short tsf_gpt2_val_h; /* 0x66E */
    unsigned short tsf_gptall_stat;    /* 0x670 */
    unsigned short PAD[0x07];  /* 0x672 - 0x67E */

    /* IFS Block */
    unsigned short ifs_sifs_rx_tx_tx;  /* 0x680 */
    unsigned short ifs_sifs_nav_tx;    /* 0x682 */
    unsigned short ifs_slot;   /* 0x684 */
    unsigned short PAD;        /* 0x686 */
    unsigned short ifs_ctl;        /* 0x688 */
    unsigned short PAD[0x3];   /* 0x68a - 0x68F */
    unsigned short ifsstat;        /* 0x690 */
    unsigned short ifsmedbusyctl;  /* 0x692 */
    unsigned short iftxdur;        /* 0x694 */
    unsigned short PAD[0x3];   /* 0x696 - 0x69b */
    /* EDCF support in dot11macs */
    unsigned short ifs_aifsn;  /* 0x69c */
    unsigned short ifs_ctl1;   /* 0x69e */

    /* slow clock registers */
    unsigned short scc_ctl;        /* 0x6a0 */
    unsigned short scc_timer_l;    /* 0x6a2 */
    unsigned short scc_timer_h;    /* 0x6a4 */
    unsigned short scc_frac;   /* 0x6a6 */
    unsigned short scc_fastpwrup_dly;  /* 0x6a8 */
    unsigned short scc_per;        /* 0x6aa */
    unsigned short scc_per_frac;   /* 0x6ac */
    unsigned short scc_cal_timer_l;    /* 0x6ae */
    unsigned short scc_cal_timer_h;    /* 0x6b0 */
    unsigned short PAD;        /* 0x6b2 */

    unsigned short PAD[0x26];

    /* NAV Block */
    unsigned short nav_ctl;        /* 0x700 */
    unsigned short navstat;        /* 0x702 */
    unsigned short PAD[0x3e];  /* 0x702 - 0x77E */

    /* WEP/PMQ Block *//* 0x780 - 0x7FE */
    unsigned short PAD[0x20];  /* 0x780 - 0x7BE */

    unsigned short wepctl;     /* 0x7C0 */
    unsigned short wepivloc;   /* 0x7C2 */
    unsigned short wepivkey;   /* 0x7C4 */
    unsigned short wepwkey;        /* 0x7C6 */

    unsigned short PAD[4];     /* 0x7C8 - 0x7CE */
    unsigned short pcmctl;     /* 0X7D0 */
    unsigned short pcmstat;        /* 0X7D2 */
    unsigned short PAD[6];     /* 0x7D4 - 0x7DE */

    unsigned short pmqctl;     /* 0x7E0 */
    unsigned short pmqstatus;  /* 0x7E2 */
    unsigned short pmqpat0;        /* 0x7E4 */
    unsigned short pmqpat1;        /* 0x7E6 */
    unsigned short pmqpat2;        /* 0x7E8 */

    unsigned short pmqdat;     /* 0x7EA */
    unsigned short pmqdator;   /* 0x7EC */
    unsigned short pmqhst;     /* 0x7EE */
    unsigned short pmqpath0;   /* 0x7F0 */
    unsigned short pmqpath1;   /* 0x7F2 */
    unsigned short pmqpath2;   /* 0x7F4 */
    unsigned short pmqdath;        /* 0x7F6 */

    unsigned short PAD[0x04];  /* 0x7F8 - 0x7FE */

    /* SHM *//* 0x800 - 0xEFE */
    unsigned short PAD[0x380]; /* 0x800 - 0xEFE */
} __attribute__((packed));

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

struct bdc_ethernet_ip_udp_header {
    struct bdc_header bdc;
    struct ethernet_header ethernet;
    struct ip_header ip;
    struct udp_header udp;
} __attribute__((packed));

struct bdc_ethernet_ipv6_udp_header {
    struct bdc_header bdc;
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

