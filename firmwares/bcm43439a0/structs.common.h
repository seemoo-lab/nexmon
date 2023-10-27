#ifndef STRUCTS_COMMON_H
#define STRUCTS_COMMON_H

#include <types.h>

#ifndef PAD
#define _PADLINE(line)  pad ## line
#define _XSTR(line) _PADLINE(line)
#define PAD     _XSTR(__LINE__)
#endif

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
    struct wl_if *wlif;
} __attribute__((packed));

struct sk_buff {
    union {                         /* 0x000 */
        uint32  u32;
        struct {
            uint16  pktid;
            uint8   refcnt;
            uint8   poolid;
        };
    } mem;
    union {                         /* 0x004 */
        uint32 offsets;
        struct {
            uint32 head_off : 21;
            uint32 end_off  : 11;
        };
    };
    char *data;                     /* 0x008 */
    uint16 len;                     /* 0x00c */
    uint16 flags;                   /* 0x00e */
    union {                         /* 0x010 */
        uint32 reset;
        struct {
            union {
                uint16 dmapad;
                uint16 rxcpl_id;
            };
            uint8  dataOff;
            uint8  ifidx;
        };
    };
    union {                         /* 0x014 */
        struct {
            uint16 nextid;
            uint16 linkid;
        };
        void *freelist;
    };
    uint32      pkttag[8];          /* 0x018 */
} __attribute__((packed));
typedef struct sk_buff sk_buff;
typedef struct sk_buff lbuf;

struct osl_info {
    uint32 PAD;                       /* 0x000 */
    uint32 PAD;                       /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
} __attribute__((packed));

struct phy_pub {
    uint32 phy_type;                  /* 0x000 */
    uint32 phy_rev;                   /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 coreflags;                 /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
} __attribute__((packed));

struct wlc_phy_shim_info {
    struct wlc_hw_info *wlc_hw;       /* 0x000 */
    void *wlc;                        /* 0x004 */
    void *wl;                         /* 0x008 */
} __attribute__((packed));

struct shared_phy {
    struct phy_info *phy_head;        /* 0x000 */
    uint32 unit;                      /* 0x004 */
    struct osl_info *osh;             /* 0x008 */
    void *sih;                        /* 0x00c */
    struct wlc_phy_shim_info *physhim;/* 0x010 */
    uint32 corerev;                   /* 0x014 */
    uint32 machwcap;                  /* 0x018 */
    uint32 PAD;                       /* 0x01c */
    uint32 now;                       /* 0x020 */
    uint16 vid;                       /* 0x024 */
    uint16 did;                       /* 0x026 */
    uint32 chip;                      /* 0x028 */
    uint32 chiprev;                   /* 0x02c */
    uint32 chippkg;                   /* 0x030 */
    uint32 sromrev;                   /* 0x034 */
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
} __attribute__((packed));

struct phy_info {
    struct phy_pub pubpi_ro;          /* 0x000 */
    struct phy_pub pubpi;             /* 0x01c */
    struct shared_phy *sh;            /* 0x038 */
    uint32 PAD;                       /* 0x03c */
    uint32 PAD;                       /* 0x040 */
    struct d11regs *regs;             /* 0x044 */
    struct phy_info *next;            /* 0x048 */
    char *vars;                       /* 0x04c */
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
    uint32 PAD;                       /* 0x4e4 */
    uint32 PAD;                       /* 0x4e8 */
    uint32 PAD;                       /* 0x4ec */
    uint32 PAD;                       /* 0x4f0 */
    uint32 PAD;                       /* 0x4f4 */
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
    uint32 PAD;                       /* 0x714 */
    uint32 PAD;                       /* 0x718 */
    uint32 PAD;                       /* 0x71c */
    uint32 PAD;                       /* 0x720 */
    uint32 PAD;                       /* 0x724 */
    uint32 PAD;                       /* 0x728 */
    uint32 PAD;                       /* 0x72c */
    uint32 PAD;                       /* 0x730 */
    uint32 PAD;                       /* 0x734 */
    uint32 PAD;                       /* 0x738 */
    uint32 PAD;                       /* 0x73c */
    uint32 PAD;                       /* 0x740 */
    uint32 PAD;                       /* 0x744 */
    uint32 PAD;                       /* 0x748 */
    uint32 PAD;                       /* 0x74c */
    uint32 PAD;                       /* 0x750 */
    uint32 PAD;                       /* 0x754 */
    uint32 PAD;                       /* 0x758 */
    uint32 PAD;                       /* 0x75c */
    uint32 PAD;                       /* 0x760 */
    uint32 PAD;                       /* 0x764 */
    uint32 PAD;                       /* 0x768 */
    uint32 PAD;                       /* 0x76c */
    uint32 PAD;                       /* 0x770 */
    uint32 PAD;                       /* 0x774 */
    uint32 PAD;                       /* 0x778 */
    uint32 PAD;                       /* 0x77c */
    uint32 PAD;                       /* 0x780 */
    uint32 PAD;                       /* 0x784 */
    uint32 PAD;                       /* 0x788 */
    uint32 PAD;                       /* 0x78c */
    uint32 PAD;                       /* 0x790 */
    uint32 PAD;                       /* 0x794 */
    uint32 PAD;                       /* 0x798 */
    uint32 PAD;                       /* 0x79c */
    uint32 PAD;                       /* 0x7a0 */
    uint32 PAD;                       /* 0x7a4 */
    uint32 PAD;                       /* 0x7a8 */
    uint32 PAD;                       /* 0x7ac */
    uint32 PAD;                       /* 0x7b0 */
    uint32 PAD;                       /* 0x7b4 */
    uint32 PAD;                       /* 0x7b8 */
    uint32 PAD;                       /* 0x7bc */
    uint32 PAD;                       /* 0x7c0 */
    uint32 PAD;                       /* 0x7c4 */
    uint32 PAD;                       /* 0x7c8 */
    uint32 PAD;                       /* 0x7cc */
    uint32 PAD;                       /* 0x7d0 */
    uint32 PAD;                       /* 0x7d4 */
    uint32 PAD;                       /* 0x7d8 */
    uint32 PAD;                       /* 0x7dc */
    uint32 PAD;                       /* 0x7e0 */
    uint32 PAD;                       /* 0x7e4 */
    uint32 PAD;                       /* 0x7e8 */
    uint32 PAD;                       /* 0x7ec */
    uint32 PAD;                       /* 0x7f0 */
    uint32 PAD;                       /* 0x7f4 */
    uint32 PAD;                       /* 0x7f8 */
    uint32 PAD;                       /* 0x7fc */
    uint32 PAD;                       /* 0x800 */
    uint32 PAD;                       /* 0x804 */
    uint32 PAD;                       /* 0x808 */
    uint32 PAD;                       /* 0x80c */
    uint32 PAD;                       /* 0x810 */
    uint32 PAD;                       /* 0x814 */
    uint32 PAD;                       /* 0x818 */
    uint32 PAD;                       /* 0x81c */
    uint32 PAD;                       /* 0x820 */
    uint32 PAD;                       /* 0x824 */
    uint32 PAD;                       /* 0x828 */
    uint32 PAD;                       /* 0x82c */
    uint32 PAD;                       /* 0x830 */
    uint32 PAD;                       /* 0x834 */
    uint32 PAD;                       /* 0x838 */
    uint32 PAD;                       /* 0x83c */
    uint32 PAD;                       /* 0x840 */
    uint32 PAD;                       /* 0x844 */
    uint32 PAD;                       /* 0x848 */
    uint32 PAD;                       /* 0x84c */
    uint32 PAD;                       /* 0x850 */
    uint32 PAD;                       /* 0x854 */
    uint32 PAD;                       /* 0x858 */
    uint32 PAD;                       /* 0x85c */
    uint32 PAD;                       /* 0x860 */
    uint32 PAD;                       /* 0x864 */
    uint32 PAD;                       /* 0x868 */
    uint32 PAD;                       /* 0x86c */
    uint32 PAD;                       /* 0x870 */
    uint32 PAD;                       /* 0x874 */
    uint32 PAD;                       /* 0x878 */
    uint32 PAD;                       /* 0x87c */
    uint32 PAD;                       /* 0x880 */
    uint32 PAD;                       /* 0x884 */
    uint32 PAD;                       /* 0x888 */
    uint32 PAD;                       /* 0x88c */
    uint32 PAD;                       /* 0x890 */
    uint32 PAD;                       /* 0x894 */
    uint32 PAD;                       /* 0x898 */
    uint32 PAD;                       /* 0x89c */
    uint32 PAD;                       /* 0x8a0 */
    uint32 PAD;                       /* 0x8a4 */
    uint32 PAD;                       /* 0x8a8 */
    uint32 PAD;                       /* 0x8ac */
    uint32 PAD;                       /* 0x8b0 */
    uint32 PAD;                       /* 0x8b4 */
    uint32 PAD;                       /* 0x8b8 */
    uint32 PAD;                       /* 0x8bc */
    uint32 PAD;                       /* 0x8c0 */
    uint32 PAD;                       /* 0x8c4 */
    uint32 PAD;                       /* 0x8c8 */
    uint32 PAD;                       /* 0x8cc */
    uint32 PAD;                       /* 0x8d0 */
    uint32 PAD;                       /* 0x8d4 */
    uint32 PAD;                       /* 0x8d8 */
    uint32 PAD;                       /* 0x8dc */
    uint32 PAD;                       /* 0x8e0 */
    uint32 PAD;                       /* 0x8e4 */
    uint32 PAD;                       /* 0x8e8 */
    uint32 PAD;                       /* 0x8ec */
    uint32 PAD;                       /* 0x8f0 */
    uint32 PAD;                       /* 0x8f4 */
    uint32 PAD;                       /* 0x8f8 */
    uint32 PAD;                       /* 0x8fc */
    uint32 PAD;                       /* 0x900 */
    uint32 PAD;                       /* 0x904 */
    uint32 PAD;                       /* 0x908 */
    uint32 PAD;                       /* 0x90c */
    uint32 PAD;                       /* 0x910 */
    uint32 PAD;                       /* 0x914 */
    uint32 PAD;                       /* 0x918 */
    uint32 PAD;                       /* 0x91c */
    uint32 PAD;                       /* 0x920 */
    uint32 PAD;                       /* 0x924 */
    uint32 PAD;                       /* 0x928 */
    uint32 PAD;                       /* 0x92c */
    uint32 PAD;                       /* 0x930 */
    uint32 PAD;                       /* 0x934 */
    uint32 PAD;                       /* 0x938 */
    uint32 PAD;                       /* 0x93c */
    uint32 PAD;                       /* 0x940 */
    uint32 PAD;                       /* 0x944 */
    uint32 PAD;                       /* 0x948 */
    uint32 PAD;                       /* 0x94c */
    uint32 PAD;                       /* 0x950 */
    uint32 PAD;                       /* 0x954 */
    uint32 PAD;                       /* 0x958 */
    uint32 PAD;                       /* 0x95c */
    uint32 PAD;                       /* 0x960 */
    uint32 PAD;                       /* 0x964 */
    uint32 PAD;                       /* 0x968 */
    uint32 PAD;                       /* 0x96c */
    uint32 PAD;                       /* 0x970 */
    uint32 PAD;                       /* 0x974 */
    uint32 PAD;                       /* 0x978 */
    uint32 PAD;                       /* 0x97c */
    uint32 PAD;                       /* 0x980 */
    uint32 PAD;                       /* 0x984 */
    uint32 PAD;                       /* 0x988 */
    uint32 PAD;                       /* 0x98c */
    uint32 PAD;                       /* 0x990 */
    uint32 PAD;                       /* 0x994 */
    uint32 PAD;                       /* 0x998 */
    uint32 PAD;                       /* 0x99c */
    uint32 PAD;                       /* 0x9a0 */
    uint32 PAD;                       /* 0x9a4 */
    uint32 PAD;                       /* 0x9a8 */
    uint32 PAD;                       /* 0x9ac */
    uint32 PAD;                       /* 0x9b0 */
    uint32 PAD;                       /* 0x9b4 */
    uint32 PAD;                       /* 0x9b8 */
    uint32 PAD;                       /* 0x9bc */
    uint32 PAD;                       /* 0x9c0 */
    uint32 PAD;                       /* 0x9c4 */
    uint32 PAD;                       /* 0x9c8 */
    uint32 PAD;                       /* 0x9cc */
    uint32 PAD;                       /* 0x9d0 */
    uint32 PAD;                       /* 0x9d4 */
    uint32 PAD;                       /* 0x9d8 */
    uint32 PAD;                       /* 0x9dc */
    uint32 PAD;                       /* 0x9e0 */
    uint32 PAD;                       /* 0x9e4 */
    uint32 PAD;                       /* 0x9e8 */
    uint32 PAD;                       /* 0x9ec */
    uint32 PAD;                       /* 0x9f0 */
    uint32 PAD;                       /* 0x9f4 */
    uint32 PAD;                       /* 0x9f8 */
    uint32 PAD;                       /* 0x9fc */
    uint32 PAD;                       /* 0xa00 */
    uint32 PAD;                       /* 0xa04 */
    uint32 PAD;                       /* 0xa08 */
    uint32 PAD;                       /* 0xa0c */
    uint32 PAD;                       /* 0xa10 */
    uint32 PAD;                       /* 0xa14 */
    uint32 PAD;                       /* 0xa18 */
    uint32 PAD;                       /* 0xa1c */
    uint32 PAD;                       /* 0xa20 */
    uint32 PAD;                       /* 0xa24 */
    uint32 PAD;                       /* 0xa28 */
    uint32 PAD;                       /* 0xa2c */
    uint32 PAD;                       /* 0xa30 */
    uint32 PAD;                       /* 0xa34 */
    uint32 PAD;                       /* 0xa38 */
    uint32 PAD;                       /* 0xa3c */
    uint32 PAD;                       /* 0xa40 */
    uint32 PAD;                       /* 0xa44 */
    uint32 PAD;                       /* 0xa48 */
    uint32 PAD;                       /* 0xa4c */
    uint32 PAD;                       /* 0xa50 */
    uint32 PAD;                       /* 0xa54 */
    uint32 PAD;                       /* 0xa58 */
    uint32 PAD;                       /* 0xa5c */
    uint32 PAD;                       /* 0xa60 */
    uint32 PAD;                       /* 0xa64 */
    uint32 PAD;                       /* 0xa68 */
    uint32 PAD;                       /* 0xa6c */
    uint32 PAD;                       /* 0xa70 */
    uint32 PAD;                       /* 0xa74 */
    uint32 PAD;                       /* 0xa78 */
    uint32 PAD;                       /* 0xa7c */
    uint32 PAD;                       /* 0xa80 */
    uint32 PAD;                       /* 0xa84 */
    uint32 PAD;                       /* 0xa88 */
    uint32 PAD;                       /* 0xa8c */
    uint32 PAD;                       /* 0xa90 */
    uint32 PAD;                       /* 0xa94 */
    uint32 PAD;                       /* 0xa98 */
    uint32 PAD;                       /* 0xa9c */
    uint32 PAD;                       /* 0xaa0 */
    uint32 PAD;                       /* 0xaa4 */
    uint32 PAD;                       /* 0xaa8 */
    uint32 PAD;                       /* 0xaac */
    uint32 PAD;                       /* 0xab0 */
    uint32 PAD;                       /* 0xab4 */
    uint32 PAD;                       /* 0xab8 */
    uint32 PAD;                       /* 0xabc */
    uint32 PAD;                       /* 0xac0 */
    uint32 PAD;                       /* 0xac4 */
    uint32 PAD;                       /* 0xac8 */
    uint32 PAD;                       /* 0xacc */
    uint32 PAD;                       /* 0xad0 */
    uint32 PAD;                       /* 0xad4 */
    uint32 PAD;                       /* 0xad8 */
    uint32 PAD;                       /* 0xadc */
    uint32 PAD;                       /* 0xae0 */
    uint32 PAD;                       /* 0xae4 */
    uint32 PAD;                       /* 0xae8 */
    uint32 PAD;                       /* 0xaec */
} __attribute__((packed));

struct wlc_hwband {
    uint32 bandtype;                  /* 0x000 */
    uint32 PAD;                       /* 0x004 */
    uint32 PAD;                       /* 0x008 */
    uint32 PAD;                       /* 0x00c */
    uint32 PAD;                       /* 0x010 */
    uint16 CWmin;                     /* 0x014 */
    uint16 CWmax;                     /* 0x016 */
    uint32 PAD;                       /* 0x018 */
    uint16 phytype;                   /* 0x01c */
    uint16 phyrev;                    /* 0x01e */
    uint16 radioid;                   /* 0x020 */
    uint16 radiorev;                  /* 0x022 */
    struct phy_info *pi;              /* 0x024 */
    uint8 abgphy_encore;              /* 0x028 */
    uint8 PAD;                        /* 0x029 */
    uint16 PAD;                       /* 0x02a */
} __attribute((packed));

struct wlc_hw_info {
    struct wlc_info *wlc;             /* 0x000 */
    uint32 PAD;                       /* 0x004 */
    void *osh;                        /* 0x008 */
    uint32 unit;                      /* 0x00c */
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
    uint16 vendorid;                  /* 0x044 */
    uint16 deviceid;                  /* 0x046 */
    uint32 corerev;                   /* 0x048 */
    uint32 PAD;                       /* 0x04c */
    uint32 PAD;                       /* 0x050 */
    uint32 PAD;                       /* 0x054 */
    uint32 PAD;                       /* 0x058 */
    uint32 PAD;                       /* 0x05c */
    uint32 PAD;                       /* 0x060 */
    uint32 PAD;                       /* 0x064 */
    uint32 PAD;                       /* 0x068 */
    uint32 machwcap;                  /* 0x06c */
    uint32 machwcap_backup;           /* 0x070 */
    uint32 PAD;                       /* 0x074 */
    uint32 PAD;                       /* 0x078 */
    void *sih;                        /* 0x07c */
    char *vars;                       /* 0x080 */
    uint32 vars_size;                 /* 0x084 */
    struct d11regs *regs;             /* 0x088 */
    uint32 PAD;                       /* 0x08c */
    void *phy_sh;                     /* 0x090 */
    struct wlc_hwband *band;          /* 0x094 */
    struct wlc_hwband *bandstate[2];  /* 0x098 */
    //uint32 PAD;                       /* 0x09c */
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
    uint16 *xmtfifo_sz;               /* 0x0d8 */
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
} __attribute__((packed));

struct wlc_if_stats {
    /* transmit stat counters */
    uint32  txframe;        /* tx data frames */
    uint32  txbyte;         /* tx data bytes */
    uint32  txerror;        /* tx data errors (derived: sum of others) */
    uint32  txnobuf;        /* tx out of buffer errors */
    uint32  txrunt;         /* tx runt frames */
    uint32  txfail;         /* tx failed frames */
    uint32  rxframe;        /* rx data frames */
    uint32  rxbyte;         /* rx data bytes */
    uint32  rxerror;        /* rx data errors (derived: sum of others) */
    uint32  rxnobuf;        /* rx out of buffer errors */
    uint32  rxrunt;         /* rx runt frames */
    uint32  rxfragerr;      /* rx fragment errors */
    uint32  txretry;        /* tx retry frames */
    uint32  txretrie;       /* tx multiple retry frames */
    uint32  txfrmsnt;       /* tx sent frames */
    uint32  txmulti;        /* tx mulitcast sent frames */
    uint32  txfrag;         /* tx fragments sent */
    uint32  rxmulti;        /* rx multicast frames */
};

struct wl_if {
    struct wlc_if *wlcif;
    struct hndrte_dev *dev;
};

struct wlc_if {
    struct wlc_if    *next;
    uint8       type;
    uint8       index;
    uint8       flags;
    struct wl_if     *wlif;
    void *qi;
    union {
        struct scb *scb;
        struct wlc_bsscfg *bsscfg;
    } u;
    struct wlc_if_stats  _cnt;
};

struct wlc_info {
    void *pub;                        /* 0x000 */
    void *osh;                        /* 0x004 */
    struct wl_info *wl;               /* 0x008 */
    struct d11regs *regs;             /* 0x00c */
    struct wlc_hw_info *hw;           /* 0x010 */
    uint32 PAD;                       /* 0x014 */
    uint32 PAD;                       /* 0x018 */
    void *core;                       /* 0x01c */
    void *band;                       /* 0x020 */
    void *corestate;                  /* 0x024 */
    void *bandstate[2];               /* 0x028 */
//    uint32 PAD;                       /* 0x02c */
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
    uint32 PAD;                       /* 0x190 */
    uint32 PAD;                       /* 0x194 */
    uint32 PAD;                       /* 0x198 */
    uint32 PAD;                       /* 0x19c */
    uint32 PAD;                       /* 0x1a0 */
    uint32 PAD;                       /* 0x1a4 */
    uint32 PAD;                       /* 0x1a8 */
    uint32 PAD;                       /* 0x1ac */
    uint32 machwcap;                  /* 0x1b0 */
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
    uint32 monitor;                   /* 0x204 */
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
    uint32 PAD;                       /* 0x4e4 */
    uint32 PAD;                       /* 0x4e8 */
    uint32 PAD;                       /* 0x4ec */
    uint32 PAD;                       /* 0x4f0 */
    uint32 PAD;                       /* 0x4f4 */
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
    struct wlc_if *wlcif_list;        /* 0x5b8 */
    uint32 PAD;                       /* 0x5bc */
    uint32 PAD;                       /* 0x5c0 */
    uint32 PAD;                       /* 0x5c4 */
    uint32 PAD;                       /* 0x5c8 */
    uint32 PAD;                       /* 0x5cc */
    uint32 PAD;                       /* 0x5d0 */
    uint32 PAD;                       /* 0x5d4 */
    uint32 PAD;                       /* 0x5d8 */
    uint32 PAD;                       /* 0x5dc */
    uint32 hwrxoff;                   /* 0x5e0 */
    uint32 hwrxoff_pktget;            /* 0x5e4 */
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
    uint32 PAD;                       /* 0x714 */
    uint32 PAD;                       /* 0x718 */
    uint32 PAD;                       /* 0x71c */
    uint32 PAD;                       /* 0x720 */
    uint32 PAD;                       /* 0x724 */
    uint32 PAD;                       /* 0x728 */
    uint32 PAD;                       /* 0x72c */
    uint32 PAD;                       /* 0x730 */
    uint32 PAD;                       /* 0x734 */
    uint32 PAD;                       /* 0x738 */
    uint32 PAD;                       /* 0x73c */
    uint32 PAD;                       /* 0x740 */
    uint32 PAD;                       /* 0x744 */
    uint32 PAD;                       /* 0x748 */
    uint32 PAD;                       /* 0x74c */
    uint32 PAD;                       /* 0x750 */
    uint32 PAD;                       /* 0x754 */
    uint32 PAD;                       /* 0x758 */
    uint32 PAD;                       /* 0x75c */
    uint32 PAD;                       /* 0x760 */
    uint32 PAD;                       /* 0x764 */
    uint32 PAD;                       /* 0x768 */
    uint32 PAD;                       /* 0x76c */
    uint32 PAD;                       /* 0x770 */
    uint32 PAD;                       /* 0x774 */
    uint32 PAD;                       /* 0x778 */
    uint32 PAD;                       /* 0x77c */
    uint32 PAD;                       /* 0x780 */
    uint32 PAD;                       /* 0x784 */
    uint32 PAD;                       /* 0x788 */
    uint32 PAD;                       /* 0x78c */
    uint32 PAD;                       /* 0x790 */
    uint32 PAD;                       /* 0x794 */
    uint32 PAD;                       /* 0x798 */
    uint32 PAD;                       /* 0x79c */
    uint32 PAD;                       /* 0x7a0 */
    uint32 PAD;                       /* 0x7a4 */
    uint32 PAD;                       /* 0x7a8 */
    uint32 PAD;                       /* 0x7ac */
    uint32 PAD;                       /* 0x7b0 */
    uint32 PAD;                       /* 0x7b4 */
    uint32 PAD;                       /* 0x7b8 */
    uint32 PAD;                       /* 0x7bc */
    uint32 PAD;                       /* 0x7c0 */
    uint32 PAD;                       /* 0x7c4 */
    uint32 PAD;                       /* 0x7c8 */
    uint32 PAD;                       /* 0x7cc */
    uint32 PAD;                       /* 0x7d0 */
    uint32 PAD;                       /* 0x7d4 */
    uint32 PAD;                       /* 0x7d8 */
    uint32 PAD;                       /* 0x7dc */
    uint32 PAD;                       /* 0x7e0 */
    uint32 PAD;                       /* 0x7e4 */
    uint32 PAD;                       /* 0x7e8 */
    uint32 PAD;                       /* 0x7ec */
    uint32 PAD;                       /* 0x7f0 */
    uint32 PAD;                       /* 0x7f4 */
    uint32 PAD;                       /* 0x7f8 */
    uint32 PAD;                       /* 0x7fc */
    uint32 PAD;                       /* 0x800 */
    uint32 PAD;                       /* 0x804 */
    uint32 PAD;                       /* 0x808 */
    uint32 PAD;                       /* 0x80c */
    uint32 PAD;                       /* 0x810 */
    uint32 PAD;                       /* 0x814 */
    uint32 PAD;                       /* 0x818 */
    uint32 PAD;                       /* 0x81c */
    uint32 PAD;                       /* 0x820 */
    uint32 PAD;                       /* 0x824 */
    uint32 PAD;                       /* 0x828 */
    uint32 PAD;                       /* 0x82c */
    uint32 PAD;                       /* 0x830 */
    uint32 PAD;                       /* 0x834 */
    uint32 PAD;                       /* 0x838 */
    uint32 PAD;                       /* 0x83c */
    uint32 PAD;                       /* 0x840 */
    uint32 PAD;                       /* 0x844 */
    uint32 PAD;                       /* 0x848 */
    uint32 PAD;                       /* 0x84c */
    uint32 PAD;                       /* 0x850 */
    uint32 PAD;                       /* 0x854 */
    uint32 PAD;                       /* 0x858 */
    uint32 PAD;                       /* 0x85c */
    uint32 PAD;                       /* 0x860 */
    uint32 PAD;                       /* 0x864 */
    uint32 PAD;                       /* 0x868 */
    uint32 PAD;                       /* 0x86c */
    uint32 PAD;                       /* 0x870 */
} __attribute__((packed));

struct wl_info {
    uint32 unit;                      /* 0x000 */
    void *pub;                        /* 0x004 */
    struct wlc_info *wlc;             /* 0x008 */
    struct wlc_hw_info *wlc_hw;       /* 0x00c */
    struct hndrte_dev *dev;           /* 0x010 */
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
} __attribute__((packed));

#define HNDRTE_DEV_NAME_MAX 16
struct hndrte_dev {
    char name[HNDRTE_DEV_NAME_MAX];
    struct hndrte_devfuncs *funcs;
    uint32 devid;
    void *softc;
    uint32 flags;
    struct hndrte_dev *next;
    struct hndrte_dev *chained;
    void *pdev;
} __attribute__((packed));

struct hndrte_devfuncs {
    void *(*probe)(struct hndrte_dev *dev, void *regs, uint bus, uint16 device, uint coreid, uint unit);
    int (*open)(struct hndrte_dev *dev);
    int (*close)(struct hndrte_dev *dev);
    int (*xmit)(struct hndrte_dev *src, struct hndrte_dev *dev, lbuf *lb);
    int (*recv)(struct hndrte_dev *src, struct hndrte_dev *dev, void *pkt);
    int (*ioctl)(struct hndrte_dev *dev, uint32 cmd, void *buffer, int len, int *used, int *needed, int set);
    void (*txflowcontrol) (struct hndrte_dev *dev, bool state, int prio);
    void (*poll)(struct hndrte_dev *dev);
    int (*xmit_ctl)(struct hndrte_dev *src, struct hndrte_dev *dev, lbuf *lb);
    int (*xmit2)(struct hndrte_dev *src, struct hndrte_dev *dev, lbuf *lb, uint32 ep_idx);
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

struct d11regs {
  uint32 off_0x000;
  uint32 off_0x004;
  uint32 off_0x008;
  uint32 biststatus;
  uint32 biststatus2;
  uint32 off_0x014;
  uint32 gptimer;
  uint32 usectimer;
  struct intctrlregs intctrlregs[8];
  uint32 off_0x060;
  uint32 off_0x064;
  uint32 off_0x068;
  uint32 off_0x06c;
  uint32 off_0x070;
  uint32 off_0x074;
  uint32 off_0x078;
  uint32 off_0x07c;
  uint32 off_0x080;
  uint32 off_0x084;
  uint32 off_0x088;
  uint32 off_0x08c;
  uint32 off_0x090;
  uint32 off_0x094;
  uint32 off_0x098;
  uint32 off_0x09c;
  uint32 off_0x0a0;
  uint32 off_0x0a4;
  uint32 off_0x0a8;
  uint32 off_0x0ac;
  uint32 off_0x0b0;
  uint32 off_0x0b4;
  uint32 off_0x0b8;
  uint32 off_0x0bc;
  uint32 off_0x0c0;
  uint32 off_0x0c4;
  uint32 off_0x0c8;
  uint32 off_0x0cc;
  uint32 off_0x0d0;
  uint32 off_0x0d4;
  uint32 off_0x0d8;
  uint32 off_0x0dc;
  uint32 off_0x0e0;
  uint32 off_0x0e4;
  uint32 off_0x0e8;
  uint32 off_0x0ec;
  uint32 off_0x0f0;
  uint32 off_0x0f4;
  uint32 off_0x0f8;
  uint32 off_0x0fc;
  uint32 intrcvlazy[4];
  uint32 off_0x110;
  uint32 off_0x114;
  uint32 off_0x118;
  uint32 off_0x11c;
  uint32 maccontrol;
  uint32 maccommand;
  uint32 macintstatus;
  uint32 macintmask;
  uint32 tplatewrptr;
  uint32 tplatewrdata;
  uint32 off_0x138;
  uint32 off_0x13c;
  uint32 pmqreg;
  uint32 pmqpatl;
  uint32 pmqpath;
  uint32 off_0x14c;
  uint32 chnstatus;
  uint32 psmdebug;
  uint32 phydebug;
  uint32 machwcap;
  uint32 objaddr;
  uint32 objdata;
  uint32 off_0x168;
  uint32 off_0x16c;
  uint32 frmtxstatus;
  uint32 frmtxstatus2;
  uint32 off_0x178;
  uint32 off_0x17c;
  uint32 tsf_timerlow;
  uint32 tsf_timerhigh;
  uint32 tsf_cfprep;
  uint32 tsf_cfpstart;
  uint32 tsf_cfpmaxdur32;
  uint32 avbrx_timestamp;
  uint32 avbtx_timestamp;
  uint32 off_0x19c;
  uint32 maccontrol1;
  uint32 machwcap1;
  uint32 off_0x1a8;
  uint32 off_0x1ac;
  uint32 off_0x1b0;
  uint32 off_0x1b4;
  uint32 off_0x1b8;
  uint32 off_0x1bc;
  uint32 off_0x1c0;
  uint32 off_0x1c4;
  uint32 off_0x1c8;
  uint32 off_0x1cc;
  uint32 off_0x1d0;
  uint32 off_0x1d4;
  uint32 off_0x1d8;
  uint32 off_0x1dc;
  uint32 clk_ctl_st;
  uint32 hw_war;
  uint32 off_0x1e8;
  uint32 off_0x1ec;
  uint32 off_0x1f0;
  uint32 off_0x1f4;
  uint32 off_0x1f8;
  uint32 off_0x1fc;
  uint32 fifo_0x200;
  uint32 fifo_0x204;
  uint32 fifo_0x208;
  uint32 fifo_0x20c;
  uint32 fifo_0x210;
  uint32 fifo_0x214;
  uint32 fifo_0x218;
  uint32 fifo_0x21c;
  uint32 fifo_0x220;
  uint32 fifo_0x224;
  uint32 fifo_0x228;
  uint32 fifo_0x22c;
  uint32 fifo_0x230;
  uint32 fifo_0x234;
  uint32 fifo_0x238;
  uint32 fifo_0x23c;
  uint32 fifo_0x240;
  uint32 fifo_0x244;
  uint32 fifo_0x248;
  uint32 fifo_0x24c;
  uint32 fifo_0x250;
  uint32 fifo_0x254;
  uint32 fifo_0x258;
  uint32 fifo_0x25c;
  uint32 fifo_0x260;
  uint32 fifo_0x264;
  uint32 fifo_0x268;
  uint32 fifo_0x26c;
  uint32 fifo_0x270;
  uint32 fifo_0x274;
  uint32 fifo_0x278;
  uint32 fifo_0x27c;
  uint32 fifo_0x280;
  uint32 fifo_0x284;
  uint32 fifo_0x288;
  uint32 fifo_0x28c;
  uint32 fifo_0x290;
  uint32 fifo_0x294;
  uint32 fifo_0x298;
  uint32 fifo_0x29c;
  uint32 fifo_0x2a0;
  uint32 fifo_0x2a4;
  uint32 fifo_0x2a8;
  uint32 fifo_0x2ac;
  uint32 fifo_0x2b0;
  uint32 fifo_0x2b4;
  uint32 fifo_0x2b8;
  uint32 fifo_0x2bc;
  uint32 fifo_0x2c0;
  uint32 fifo_0x2c4;
  uint32 fifo_0x2c8;
  uint32 fifo_0x2cc;
  uint32 fifo_0x2d0;
  uint32 fifo_0x2d4;
  uint32 fifo_0x2d8;
  uint32 fifo_0x2dc;
  uint32 fifo_0x2e0;
  uint32 fifo_0x2e4;
  uint32 fifo_0x2e8;
  uint32 fifo_0x2ec;
  uint32 fifo_0x2f0;
  uint32 fifo_0x2f4;
  uint32 fifo_0x2f8;
  uint32 fifo_0x2fc;
  uint32 fifo_0x300;
  uint32 fifo_0x304;
  uint32 fifo_0x308;
  uint32 fifo_0x30c;
  uint32 fifo_0x310;
  uint32 fifo_0x314;
  uint32 fifo_0x318;
  uint32 fifo_0x31c;
  uint32 fifo_0x320;
  uint32 fifo_0x324;
  uint32 fifo_0x328;
  uint32 fifo_0x32c;
  uint32 fifo_0x330;
  uint32 fifo_0x334;
  uint32 fifo_0x338;
  uint32 fifo_0x33c;
  uint32 fifo_0x340;
  uint32 fifo_0x344;
  uint32 fifo_0x348;
  uint32 fifo_0x34c;
  uint32 fifo_0x350;
  uint32 fifo_0x354;
  uint32 fifo_0x358;
  uint32 fifo_0x35c;
  uint32 fifo_0x360;
  uint32 fifo_0x364;
  uint32 fifo_0x368;
  uint32 fifo_0x36c;
  uint32 fifo_0x370;
  uint32 fifo_0x374;
  uint32 fifo_0x378;
  uint32 fifo_0x37c;
  uint32 dmafifo_0x380;
  uint32 dmafifo_0x384;
  uint32 dmafifo_0x388;
  uint32 dmafifo_0x38c;
  uint32 aggfifocnt;
  uint32 aggfifodata;
  uint32 off_0x398;
  uint32 off_0x39c;
  uint32 off_0x3a0;
  uint32 off_0x3a4;
  uint32 off_0x3a8;
  uint32 off_0x3ac;
  uint32 off_0x3b0;
  uint32 off_0x3b4;
  uint32 off_0x3b8;
  uint32 off_0x3bc;
  uint32 off_0x3c0;
  uint32 off_0x3c4;
  uint16 altradioregaddr;
  uint16 altradioregdata;
  uint32 off_0x3cc;
  uint32 off_0x3d0;
  uint32 off_0x3d4;
  uint16 radioregaddr;
  uint16 radioregdata;
  uint32 off_0x3dc;
  uint16 phyversion;
  uint16 phybbconfig;
  uint16 phyadcbias;
  uint16 phyanacore;
  uint16 phyrxstatus0;
  uint16 phyrxstatus1;
  uint16 phycrsth;
  uint16 phytxerror;
  uint16 phychannel;
  uint16 off_0x3f2;
  uint16 phytest;
  uint16 phy4waddr;
  uint16 phy4wdatahi;
  uint16 phy4wdatalo;
  uint16 phyregaddr;
  uint16 phyregdata;
  uint16 IHR_0x400;
  uint16 IHR_0x402;
  uint16 IHR_0x404;
  uint16 rcv_fifo_ctl;
  uint16 rcv_ctl;
  uint16 rcv_frm_cnt;
  uint16 rcv_status_len;
  uint16 IHR_0x40e;
  uint16 IHR_0x410;
  uint16 IHR_0x412;
  uint16 rssi;
  uint16 IHR_0x416;
  uint16 rxe_rxcnt;
  uint16 rxe_status1;
  uint16 rxe_status2;
  uint16 rxe_plcp_len;
  uint16 rcm_ctl;
  uint16 rcm_mat_data;
  uint16 rcm_mat_mask;
  uint16 rcm_mat_dly;
  uint16 rcm_cond_mask_l;
  uint16 rcm_cond_mask_h;
  uint16 rcm_cond_dly;
  uint16 IHR_0x42e;
  uint16 ext_ihr_addr;
  uint16 ext_ihr_data;
  uint16 rxe_phyrs_2;
  uint16 rxe_phyrs_3;
  uint16 phy_mode;
  uint16 rcmta_ctl;
  uint16 rcmta_size;
  uint16 rcmta_addr0;
  uint16 rcmta_addr1;
  uint16 rcmta_addr2;
  uint16 IHR_0x444;
  uint16 IHR_0x446;
  uint16 IHR_0x448;
  uint16 IHR_0x44a;
  uint16 IHR_0x44c;
  uint16 IHR_0x44e;
  uint16 IHR_0x450;
  uint16 IHR_0x452;
  uint16 IHR_0x454;
  uint16 IHR_0x456;
  uint16 IHR_0x458;
  uint16 IHR_0x45a;
  uint16 IHR_0x45c;
  uint16 ext_ihr_status;
  uint16 radio_ihr_addr;
  uint16 radio_ihr_data;
  uint16 IHR_0x464;
  uint16 IHR_0x466;
  uint16 radio_ihr_status;
  uint16 IHR_0x46a;
  uint16 IHR_0x46c;
  uint16 IHR_0x46e;
  uint16 IHR_0x470;
  uint16 IHR_0x472;
  uint16 IHR_0x474;
  uint16 IHR_0x476;
  uint16 IHR_0x478;
  uint16 IHR_0x47a;
  uint16 IHR_0x47c;
  uint16 IHR_0x47e;
  uint16 IHR_0x480;
  uint16 psm_maccontrol_h;
  uint16 psm_macintstatus_l;
  uint16 psm_macintstatus_h;
  uint16 psm_macintmask_l;
  uint16 psm_macintmask_h;
  uint16 IHR_0x48c;
  uint16 psm_maccommand;
  uint16 psm_brc;
  uint16 psm_phy_hdr_param;
  uint16 psm_int_sel_0;
  uint16 psm_int_sel_1;
  uint16 IHR_0x498;
  uint16 psm_gpio_in;
  uint16 psm_gpio_out;
  uint16 psm_gpio_oe;
  uint16 psm_bred_0;
  uint16 psm_bred_1;
  uint16 psm_bred_2;
  uint16 psm_bred_3;
  uint16 psm_brcl_0;
  uint16 psm_brcl_1;
  uint16 psm_brcl_2;
  uint16 psm_brcl_3;
  uint16 psm_brpo_0;
  uint16 psm_brpo_1;
  uint16 psm_brpo_2;
  uint16 psm_brpo_3;
  uint16 psm_brwk_0;
  uint16 psm_brwk_1;
  uint16 psm_brwk_2;
  uint16 psm_brwk_3;
  uint16 psm_base_0;
  uint16 psm_base_1;
  uint16 psm_base_2;
  uint16 psm_base_3;
  uint16 psm_base_4;
  uint16 psm_base_5;
  uint16 psm_base_6;
  uint16 psm_ihr_err;
  uint16 psm_pc_reg_0;
  uint16 psm_pc_reg_1;
  uint16 psm_pc_reg_2;
  uint16 IHR_0x4d6;
  uint16 psm_brc_1;
  uint16 IHR_0x4da;
  uint16 IHR_0x4dc;
  uint16 IHR_0x4de;
  uint16 IHR_0x4e0;
  uint16 IHR_0x4e2;
  uint16 IHR_0x4e4;
  uint16 IHR_0x4e6;
  uint16 IHR_0x4e8;
  uint16 psm_sbreg_addr;
  uint16 psm_sbreg_dataL;
  uint16 psm_sbreg_dataH;
  uint16 psm_corectlsts;
  uint16 IHR_0x4f2;
  uint16 psm_sbbar0;
  uint16 psm_sbbar1;
  uint16 psm_sbbar2;
  uint16 psm_sbbar3;
  uint16 IHR_0x4fc;
  uint16 IHR_0x4fe;
  uint16 txe_ctl;
  uint16 txe_aux;
  uint16 txe_ts_loc;
  uint16 txe_time_out;
  uint16 txe_wm_0;
  uint16 txe_wm_1;
  uint16 txe_phyctl;
  uint16 txe_status;
  uint16 txe_mmplcp0;
  uint16 txe_mmplcp1;
  uint16 txe_phyctl1;
  uint16 txe_phyctl2;
  uint16 IHR_0x518;
  uint16 IHR_0x51a;
  uint16 IHR_0x51c;
  uint16 IHR_0x51e;
  uint16 xmtfifodef;
  uint16 xmtfifo_frame_cnt;
  uint16 xmtfifo_byte_cnt;
  uint16 xmtfifo_head;
  uint16 xmtfifo_rd_ptr;
  uint16 xmtfifo_wr_ptr;
  uint16 xmtfifodef1;
  uint16 aggfifo_cmd;
  uint16 aggfifo_stat;
  uint16 aggfifo_cfgctl;
  uint16 aggfifo_cfgdata;
  uint16 aggfifo_mpdunum;
  uint16 aggfifo_len;
  uint16 aggfifo_bmp;
  uint16 aggfifo_ackedcnt;
  uint16 aggfifo_sel;
  uint16 xmtfifocmd;
  uint16 xmtfifoflush;
  uint16 xmtfifothresh;
  uint16 xmtfifordy;
  uint16 xmtfifoprirdy;
  uint16 xmtfiforqpri;
  uint16 xmttplatetxptr;
  uint16 IHR_0x54e;
  uint16 xmttplateptr;
  uint16 smpl_clct_strptr;
  uint16 smpl_clct_stpptr;
  uint16 smpl_clct_curptr;
  uint16 aggfifo_data;
  uint16 IHR_0x55a;
  uint16 IHR_0x55c;
  uint16 IHR_0x55e;
  uint16 xmttplatedatalo;
  uint16 xmttplatedatahi;
  uint16 IHR_0x564;
  uint16 IHR_0x566;
  uint16 xmtsel;
  uint16 xmttxcnt;
  uint16 xmttxshmaddr;
  uint16 IHR_0x56e;
  uint16 xmt_ampdu_ctl;
  uint16 IHR_0x572;
  uint16 IHR_0x574;
  uint16 IHR_0x576;
  uint16 IHR_0x578;
  uint16 IHR_0x57a;
  uint16 IHR_0x57c;
  uint16 IHR_0x57e;
  uint16 IHR_0x580;
  uint16 IHR_0x582;
  uint16 IHR_0x584;
  uint16 IHR_0x586;
  uint16 IHR_0x588;
  uint16 IHR_0x58a;
  uint16 IHR_0x58c;
  uint16 IHR_0x58e;
  uint16 IHR_0x590;
  uint16 IHR_0x592;
  uint16 IHR_0x594;
  uint16 IHR_0x596;
  uint16 IHR_0x598;
  uint16 IHR_0x59a;
  uint16 IHR_0x59c;
  uint16 IHR_0x59e;
  uint16 IHR_0x5a0;
  uint16 IHR_0x5a2;
  uint16 IHR_0x5a4;
  uint16 IHR_0x5a6;
  uint16 IHR_0x5a8;
  uint16 IHR_0x5aa;
  uint16 IHR_0x5ac;
  uint16 IHR_0x5ae;
  uint16 IHR_0x5b0;
  uint16 IHR_0x5b2;
  uint16 IHR_0x5b4;
  uint16 IHR_0x5b6;
  uint16 IHR_0x5b8;
  uint16 IHR_0x5ba;
  uint16 IHR_0x5bc;
  uint16 IHR_0x5be;
  uint16 IHR_0x5c0;
  uint16 IHR_0x5c2;
  uint16 IHR_0x5c4;
  uint16 IHR_0x5c6;
  uint16 IHR_0x5c8;
  uint16 IHR_0x5ca;
  uint16 IHR_0x5cc;
  uint16 IHR_0x5ce;
  uint16 IHR_0x5d0;
  uint16 IHR_0x5d2;
  uint16 IHR_0x5d4;
  uint16 IHR_0x5d6;
  uint16 IHR_0x5d8;
  uint16 IHR_0x5da;
  uint16 IHR_0x5dc;
  uint16 IHR_0x5de;
  uint16 IHR_0x5e0;
  uint16 IHR_0x5e2;
  uint16 IHR_0x5e4;
  uint16 IHR_0x5e6;
  uint16 IHR_0x5e8;
  uint16 IHR_0x5ea;
  uint16 IHR_0x5ec;
  uint16 IHR_0x5ee;
  uint16 IHR_0x5f0;
  uint16 IHR_0x5f2;
  uint16 IHR_0x5f4;
  uint16 IHR_0x5f6;
  uint16 IHR_0x5f8;
  uint16 IHR_0x5fa;
  uint16 IHR_0x5fc;
  uint16 IHR_0x5fe;
  uint16 IHR_0x600;
  uint16 IHR_0x602;
  uint16 tsf_cfpstrt_l;
  uint16 tsf_cfpstrt_h;
  uint16 IHR_0x608;
  uint16 IHR_0x60a;
  uint16 IHR_0x60c;
  uint16 IHR_0x60e;
  uint16 IHR_0x610;
  uint16 tsf_cfppretbtt;
  uint16 IHR_0x614;
  uint16 IHR_0x616;
  uint16 IHR_0x618;
  uint16 IHR_0x61a;
  uint16 IHR_0x61c;
  uint16 IHR_0x61e;
  uint16 IHR_0x620;
  uint16 IHR_0x622;
  uint16 IHR_0x624;
  uint16 IHR_0x626;
  uint16 IHR_0x628;
  uint16 IHR_0x62a;
  uint16 IHR_0x62c;
  uint16 tsf_clk_frac_l;
  uint16 tsf_clk_frac_h;
  uint16 IHR_0x632;
  uint16 IHR_0x634;
  uint16 IHR_0x636;
  uint16 IHR_0x638;
  uint16 IHR_0x63a;
  uint16 IHR_0x63c;
  uint16 IHR_0x63e;
  uint16 IHR_0x640;
  uint16 IHR_0x642;
  uint16 IHR_0x644;
  uint16 IHR_0x646;
  uint16 IHR_0x648;
  uint16 IHR_0x64a;
  uint16 IHR_0x64c;
  uint16 IHR_0x64e;
  uint16 IHR_0x650;
  uint16 IHR_0x652;
  uint16 IHR_0x654;
  uint16 IHR_0x656;
  uint16 IHR_0x658;
  uint16 tsf_random;
  uint16 IHR_0x65c;
  uint16 IHR_0x65e;
  uint16 IHR_0x660;
  uint16 IHR_0x662;
  uint16 IHR_0x664;
  uint16 tsf_gpt2_stat;
  uint16 tsf_gpt2_ctr_l;
  uint16 tsf_gpt2_ctr_h;
  uint16 tsf_gpt2_val_l;
  uint16 tsf_gpt2_val_h;
  uint16 tsf_gptall_stat;
  uint16 IHR_0x672;
  uint16 IHR_0x674;
  uint16 IHR_0x676;
  uint16 IHR_0x678;
  uint16 IHR_0x67a;
  uint16 IHR_0x67c;
  uint16 IHR_0x67e;
  uint16 ifs_sifs_rx_tx_tx;
  uint16 ifs_sifs_nav_tx;
  uint16 ifs_slot;
  uint16 IHR_0x686;
  uint16 ifs_ctl;
  uint16 ifs_boff;
  uint16 IHR_0x68c;
  uint16 IHR_0x68e;
  uint16 ifsstat;
  uint16 ifsmedbusyctl;
  uint16 iftxdur;
  uint16 IHR_0x696;
  uint16 ifsstat1;
  uint16 IHR_0x69a;
  uint16 ifs_aifsn;
  uint16 ifs_ctl1;
  uint16 scc_ctl;
  uint16 scc_timer_l;
  uint16 scc_timer_h;
  uint16 scc_frac;
  uint16 scc_fastpwrup_dly;
  uint16 scc_per;
  uint16 scc_per_frac;
  uint16 scc_cal_timer_l;
  uint16 scc_cal_timer_h;
  uint16 IHR_0x6b2;
  uint16 btcx_ctrl;
  uint16 btcx_stat;
  uint16 btcx_trans_ctrl;
  uint16 btcx_pri_win;
  uint16 btcx_tx_conf_timer;
  uint16 btcx_ant_sw_timer;
  uint16 btcx_prv_rfact_timer;
  uint16 btcx_cur_rfact_timer;
  uint16 btcx_rfact_dur_timer;
  uint16 ifs_ctl_sel_pricrs;
  uint16 ifs_ctl_sel_seccrs;
  uint16 IHR_0x6ca;
  uint16 IHR_0x6cc;
  uint16 IHR_0x6ce;
  uint16 IHR_0x6d0;
  uint16 IHR_0x6d2;
  uint16 IHR_0x6d4;
  uint16 IHR_0x6d6;
  uint16 IHR_0x6d8;
  uint16 IHR_0x6da;
  uint16 IHR_0x6dc;
  uint16 IHR_0x6de;
  uint16 IHR_0x6e0;
  uint16 IHR_0x6e2;
  uint16 IHR_0x6e4;
  uint16 IHR_0x6e6;
  uint16 IHR_0x6e8;
  uint16 IHR_0x6ea;
  uint16 IHR_0x6ec;
  uint16 IHR_0x6ee;
  uint16 btcx_eci_addr;
  uint16 btcx_eci_data;
  uint16 btcx_eci_mask_addr;
  uint16 btcx_eci_mask_data;
  uint16 coex_io_mask;
  uint16 btcx_eci_event_addr;
  uint16 btcx_eci_event_data;
  uint16 IHR_0x6fe;
  uint16 nav_ctl;
  uint16 navstat;
  uint16 IHR_0x704;
  uint16 IHR_0x706;
  uint16 IHR_0x708;
  uint16 IHR_0x70a;
  uint16 IHR_0x70c;
  uint16 IHR_0x70e;
  uint16 IHR_0x710;
  uint16 IHR_0x712;
  uint16 IHR_0x714;
  uint16 IHR_0x716;
  uint16 IHR_0x718;
  uint16 IHR_0x71a;
  uint16 IHR_0x71c;
  uint16 IHR_0x71e;
  uint16 IHR_0x720;
  uint16 IHR_0x722;
  uint16 IHR_0x724;
  uint16 IHR_0x726;
  uint16 IHR_0x728;
  uint16 IHR_0x72a;
  uint16 IHR_0x72c;
  uint16 IHR_0x72e;
  uint16 IHR_0x730;
  uint16 IHR_0x732;
  uint16 IHR_0x734;
  uint16 IHR_0x736;
  uint16 IHR_0x738;
  uint16 IHR_0x73a;
  uint16 IHR_0x73c;
  uint16 IHR_0x73e;
  uint16 IHR_0x740;
  uint16 IHR_0x742;
  uint16 IHR_0x744;
  uint16 IHR_0x746;
  uint16 IHR_0x748;
  uint16 IHR_0x74a;
  uint16 IHR_0x74c;
  uint16 IHR_0x74e;
  uint16 IHR_0x750;
  uint16 IHR_0x752;
  uint16 IHR_0x754;
  uint16 IHR_0x756;
  uint16 IHR_0x758;
  uint16 IHR_0x75a;
  uint16 IHR_0x75c;
  uint16 IHR_0x75e;
  uint16 IHR_0x760;
  uint16 IHR_0x762;
  uint16 IHR_0x764;
  uint16 IHR_0x766;
  uint16 IHR_0x768;
  uint16 IHR_0x76a;
  uint16 IHR_0x76c;
  uint16 IHR_0x76e;
  uint16 IHR_0x770;
  uint16 IHR_0x772;
  uint16 IHR_0x774;
  uint16 IHR_0x776;
  uint16 IHR_0x778;
  uint16 IHR_0x77a;
  uint16 IHR_0x77c;
  uint16 IHR_0x77e;
  uint16 IHR_0x780;
  uint16 IHR_0x782;
  uint16 IHR_0x784;
  uint16 IHR_0x786;
  uint16 IHR_0x788;
  uint16 IHR_0x78a;
  uint16 IHR_0x78c;
  uint16 IHR_0x78e;
  uint16 IHR_0x790;
  uint16 IHR_0x792;
  uint16 IHR_0x794;
  uint16 IHR_0x796;
  uint16 IHR_0x798;
  uint16 IHR_0x79a;
  uint16 IHR_0x79c;
  uint16 IHR_0x79e;
  uint16 IHR_0x7a0;
  uint16 IHR_0x7a2;
  uint16 IHR_0x7a4;
  uint16 IHR_0x7a6;
  uint16 IHR_0x7a8;
  uint16 IHR_0x7aa;
  uint16 IHR_0x7ac;
  uint16 IHR_0x7ae;
  uint16 IHR_0x7b0;
  uint16 IHR_0x7b2;
  uint16 IHR_0x7b4;
  uint16 IHR_0x7b6;
  uint16 IHR_0x7b8;
  uint16 IHR_0x7ba;
  uint16 IHR_0x7bc;
  uint16 IHR_0x7be;
  uint16 wepctl;
  uint16 wepivloc;
  uint16 wepivkey;
  uint16 wepwkey;
  uint16 IHR_0x7c8;
  uint16 IHR_0x7ca;
  uint16 IHR_0x7cc;
  uint16 IHR_0x7ce;
  uint16 pcmctl;
  uint16 pcmstat;
  uint16 IHR_0x7d4;
  uint16 IHR_0x7d6;
  uint16 IHR_0x7d8;
  uint16 IHR_0x7da;
  uint16 IHR_0x7dc;
  uint16 IHR_0x7de;
  uint16 pmqctl;
  uint16 pmqstatus;
  uint16 pmqpat0;
  uint16 pmqpat1;
  uint16 pmqpat2;
  uint16 pmqdat;
  uint16 pmqdataor_mat;
  uint16 pmqdataor_all;
  uint16 IHR_0x7f0;
  uint16 IHR_0x7f2;
  uint16 IHR_0x7f4;
  uint16 IHR_0x7f6;
  uint16 IHR_0x7f8;
  uint16 IHR_0x7fa;
  uint16 IHR_0x7fc;
  uint16 IHR_0x7fe;
  uint16 IHR_0x800;
  uint16 IHR_0x802;
  uint16 IHR_0x804;
  uint16 IHR_0x806;
  uint16 IHR_0x808;
  uint16 IHR_0x80a;
  uint16 IHR_0x80c;
  uint16 IHR_0x80e;
  uint16 IHR_0x810;
  uint16 IHR_0x812;
  uint16 IHR_0x814;
  uint16 IHR_0x816;
  uint16 IHR_0x818;
  uint16 IHR_0x81a;
  uint16 IHR_0x81c;
  uint16 IHR_0x81e;
  uint16 IHR_0x820;
  uint16 IHR_0x822;
  uint16 IHR_0x824;
  uint16 IHR_0x826;
  uint16 IHR_0x828;
  uint16 IHR_0x82a;
  uint16 IHR_0x82c;
  uint16 IHR_0x82e;
  uint16 IHR_0x830;
  uint16 IHR_0x832;
  uint16 IHR_0x834;
  uint16 IHR_0x836;
  uint16 IHR_0x838;
  uint16 IHR_0x83a;
  uint16 IHR_0x83c;
  uint16 IHR_0x83e;
  uint16 IHR_0x840;
  uint16 IHR_0x842;
  uint16 IHR_0x844;
  uint16 IHR_0x846;
  uint16 IHR_0x848;
  uint16 IHR_0x84a;
  uint16 IHR_0x84c;
  uint16 IHR_0x84e;
  uint16 IHR_0x850;
  uint16 IHR_0x852;
  uint16 IHR_0x854;
  uint16 IHR_0x856;
  uint16 IHR_0x858;
  uint16 IHR_0x85a;
  uint16 IHR_0x85c;
  uint16 IHR_0x85e;
  uint16 IHR_0x860;
  uint16 IHR_0x862;
  uint16 IHR_0x864;
  uint16 IHR_0x866;
  uint16 IHR_0x868;
  uint16 IHR_0x86a;
  uint16 IHR_0x86c;
  uint16 IHR_0x86e;
  uint16 IHR_0x870;
  uint16 IHR_0x872;
  uint16 IHR_0x874;
  uint16 IHR_0x876;
  uint16 IHR_0x878;
  uint16 IHR_0x87a;
  uint16 IHR_0x87c;
  uint16 IHR_0x87e;
  uint16 IHR_0x880;
  uint16 IHR_0x882;
  uint16 IHR_0x884;
  uint16 IHR_0x886;
  uint16 IHR_0x888;
  uint16 IHR_0x88a;
  uint16 IHR_0x88c;
  uint16 IHR_0x88e;
  uint16 IHR_0x890;
  uint16 IHR_0x892;
  uint16 IHR_0x894;
  uint16 IHR_0x896;
  uint16 IHR_0x898;
  uint16 IHR_0x89a;
  uint16 IHR_0x89c;
  uint16 IHR_0x89e;
  uint16 IHR_0x8a0;
  uint16 IHR_0x8a2;
  uint16 IHR_0x8a4;
  uint16 IHR_0x8a6;
  uint16 IHR_0x8a8;
  uint16 IHR_0x8aa;
  uint16 IHR_0x8ac;
  uint16 IHR_0x8ae;
  uint16 IHR_0x8b0;
  uint16 IHR_0x8b2;
  uint16 IHR_0x8b4;
  uint16 IHR_0x8b6;
  uint16 IHR_0x8b8;
  uint16 IHR_0x8ba;
  uint16 IHR_0x8bc;
  uint16 IHR_0x8be;
  uint16 IHR_0x8c0;
  uint16 IHR_0x8c2;
  uint16 IHR_0x8c4;
  uint16 IHR_0x8c6;
  uint16 IHR_0x8c8;
  uint16 IHR_0x8ca;
  uint16 IHR_0x8cc;
  uint16 IHR_0x8ce;
  uint16 IHR_0x8d0;
  uint16 IHR_0x8d2;
  uint16 IHR_0x8d4;
  uint16 IHR_0x8d6;
  uint16 IHR_0x8d8;
  uint16 IHR_0x8da;
  uint16 IHR_0x8dc;
  uint16 IHR_0x8de;
  uint16 IHR_0x8e0;
  uint16 IHR_0x8e2;
  uint16 IHR_0x8e4;
  uint16 IHR_0x8e6;
  uint16 IHR_0x8e8;
  uint16 IHR_0x8ea;
  uint16 IHR_0x8ec;
  uint16 IHR_0x8ee;
  uint16 IHR_0x8f0;
  uint16 IHR_0x8f2;
  uint16 IHR_0x8f4;
  uint16 IHR_0x8f6;
  uint16 IHR_0x8f8;
  uint16 IHR_0x8fa;
  uint16 IHR_0x8fc;
  uint16 IHR_0x8fe;
  uint16 IHR_0x900;
  uint16 IHR_0x902;
  uint16 IHR_0x904;
  uint16 IHR_0x906;
  uint16 IHR_0x908;
  uint16 IHR_0x90a;
  uint16 IHR_0x90c;
  uint16 IHR_0x90e;
  uint16 IHR_0x910;
  uint16 IHR_0x912;
  uint16 IHR_0x914;
  uint16 IHR_0x916;
  uint16 IHR_0x918;
  uint16 IHR_0x91a;
  uint16 IHR_0x91c;
  uint16 IHR_0x91e;
  uint16 IHR_0x920;
  uint16 IHR_0x922;
  uint16 IHR_0x924;
  uint16 IHR_0x926;
  uint16 IHR_0x928;
  uint16 IHR_0x92a;
  uint16 IHR_0x92c;
  uint16 IHR_0x92e;
  uint16 IHR_0x930;
  uint16 IHR_0x932;
  uint16 IHR_0x934;
  uint16 IHR_0x936;
  uint16 IHR_0x938;
  uint16 IHR_0x93a;
  uint16 IHR_0x93c;
  uint16 IHR_0x93e;
  uint16 IHR_0x940;
  uint16 IHR_0x942;
  uint16 IHR_0x944;
  uint16 IHR_0x946;
  uint16 IHR_0x948;
  uint16 IHR_0x94a;
  uint16 IHR_0x94c;
  uint16 IHR_0x94e;
  uint16 IHR_0x950;
  uint16 IHR_0x952;
  uint16 IHR_0x954;
  uint16 IHR_0x956;
  uint16 IHR_0x958;
  uint16 IHR_0x95a;
  uint16 IHR_0x95c;
  uint16 IHR_0x95e;
  uint16 IHR_0x960;
  uint16 IHR_0x962;
  uint16 IHR_0x964;
  uint16 IHR_0x966;
  uint16 IHR_0x968;
  uint16 IHR_0x96a;
  uint16 IHR_0x96c;
  uint16 IHR_0x96e;
  uint16 IHR_0x970;
  uint16 IHR_0x972;
  uint16 IHR_0x974;
  uint16 IHR_0x976;
  uint16 IHR_0x978;
  uint16 IHR_0x97a;
  uint16 IHR_0x97c;
  uint16 IHR_0x97e;
  uint16 IHR_0x980;
  uint16 IHR_0x982;
  uint16 IHR_0x984;
  uint16 IHR_0x986;
  uint16 IHR_0x988;
  uint16 IHR_0x98a;
  uint16 IHR_0x98c;
  uint16 IHR_0x98e;
  uint16 IHR_0x990;
  uint16 IHR_0x992;
  uint16 IHR_0x994;
  uint16 IHR_0x996;
  uint16 IHR_0x998;
  uint16 IHR_0x99a;
  uint16 IHR_0x99c;
  uint16 IHR_0x99e;
  uint16 IHR_0x9a0;
  uint16 IHR_0x9a2;
  uint16 IHR_0x9a4;
  uint16 IHR_0x9a6;
  uint16 IHR_0x9a8;
  uint16 IHR_0x9aa;
  uint16 IHR_0x9ac;
  uint16 IHR_0x9ae;
  uint16 IHR_0x9b0;
  uint16 IHR_0x9b2;
  uint16 IHR_0x9b4;
  uint16 IHR_0x9b6;
  uint16 IHR_0x9b8;
  uint16 IHR_0x9ba;
  uint16 IHR_0x9bc;
  uint16 IHR_0x9be;
  uint16 IHR_0x9c0;
  uint16 IHR_0x9c2;
  uint16 IHR_0x9c4;
  uint16 IHR_0x9c6;
  uint16 IHR_0x9c8;
  uint16 IHR_0x9ca;
  uint16 IHR_0x9cc;
  uint16 IHR_0x9ce;
  uint16 IHR_0x9d0;
  uint16 IHR_0x9d2;
  uint16 IHR_0x9d4;
  uint16 IHR_0x9d6;
  uint16 IHR_0x9d8;
  uint16 IHR_0x9da;
  uint16 IHR_0x9dc;
  uint16 IHR_0x9de;
  uint16 IHR_0x9e0;
  uint16 IHR_0x9e2;
  uint16 IHR_0x9e4;
  uint16 IHR_0x9e6;
  uint16 IHR_0x9e8;
  uint16 IHR_0x9ea;
  uint16 IHR_0x9ec;
  uint16 IHR_0x9ee;
  uint16 IHR_0x9f0;
  uint16 IHR_0x9f2;
  uint16 IHR_0x9f4;
  uint16 IHR_0x9f6;
  uint16 IHR_0x9f8;
  uint16 IHR_0x9fa;
  uint16 IHR_0x9fc;
  uint16 IHR_0x9fe;
  uint16 IHR_0xa00;
  uint16 IHR_0xa02;
  uint16 IHR_0xa04;
  uint16 IHR_0xa06;
  uint16 IHR_0xa08;
  uint16 IHR_0xa0a;
  uint16 IHR_0xa0c;
  uint16 IHR_0xa0e;
  uint16 IHR_0xa10;
  uint16 IHR_0xa12;
  uint16 IHR_0xa14;
  uint16 IHR_0xa16;
  uint16 IHR_0xa18;
  uint16 IHR_0xa1a;
  uint16 IHR_0xa1c;
  uint16 IHR_0xa1e;
  uint16 IHR_0xa20;
  uint16 IHR_0xa22;
  uint16 IHR_0xa24;
  uint16 IHR_0xa26;
  uint16 IHR_0xa28;
  uint16 IHR_0xa2a;
  uint16 IHR_0xa2c;
  uint16 IHR_0xa2e;
  uint16 IHR_0xa30;
  uint16 IHR_0xa32;
  uint16 IHR_0xa34;
  uint16 IHR_0xa36;
  uint16 IHR_0xa38;
  uint16 IHR_0xa3a;
  uint16 IHR_0xa3c;
  uint16 IHR_0xa3e;
  uint16 IHR_0xa40;
  uint16 IHR_0xa42;
  uint16 IHR_0xa44;
  uint16 IHR_0xa46;
  uint16 IHR_0xa48;
  uint16 IHR_0xa4a;
  uint16 IHR_0xa4c;
  uint16 IHR_0xa4e;
  uint16 IHR_0xa50;
  uint16 IHR_0xa52;
  uint16 IHR_0xa54;
  uint16 IHR_0xa56;
  uint16 IHR_0xa58;
  uint16 IHR_0xa5a;
  uint16 IHR_0xa5c;
  uint16 IHR_0xa5e;
  uint16 IHR_0xa60;
  uint16 IHR_0xa62;
  uint16 IHR_0xa64;
  uint16 IHR_0xa66;
  uint16 IHR_0xa68;
  uint16 IHR_0xa6a;
  uint16 IHR_0xa6c;
  uint16 IHR_0xa6e;
  uint16 IHR_0xa70;
  uint16 IHR_0xa72;
  uint16 IHR_0xa74;
  uint16 IHR_0xa76;
  uint16 IHR_0xa78;
  uint16 IHR_0xa7a;
  uint16 IHR_0xa7c;
  uint16 IHR_0xa7e;
  uint16 IHR_0xa80;
  uint16 IHR_0xa82;
  uint16 IHR_0xa84;
  uint16 IHR_0xa86;
  uint16 IHR_0xa88;
  uint16 IHR_0xa8a;
  uint16 IHR_0xa8c;
  uint16 IHR_0xa8e;
  uint16 IHR_0xa90;
  uint16 IHR_0xa92;
  uint16 IHR_0xa94;
  uint16 IHR_0xa96;
  uint16 IHR_0xa98;
  uint16 IHR_0xa9a;
  uint16 IHR_0xa9c;
  uint16 IHR_0xa9e;
  uint16 IHR_0xaa0;
  uint16 IHR_0xaa2;
  uint16 IHR_0xaa4;
  uint16 IHR_0xaa6;
  uint16 IHR_0xaa8;
  uint16 IHR_0xaaa;
  uint16 IHR_0xaac;
  uint16 IHR_0xaae;
  uint16 IHR_0xab0;
  uint16 IHR_0xab2;
  uint16 IHR_0xab4;
  uint16 IHR_0xab6;
  uint16 IHR_0xab8;
  uint16 IHR_0xaba;
  uint16 IHR_0xabc;
  uint16 IHR_0xabe;
  uint16 IHR_0xac0;
  uint16 IHR_0xac2;
  uint16 IHR_0xac4;
  uint16 IHR_0xac6;
  uint16 IHR_0xac8;
  uint16 IHR_0xaca;
  uint16 IHR_0xacc;
  uint16 IHR_0xace;
  uint16 IHR_0xad0;
  uint16 IHR_0xad2;
  uint16 IHR_0xad4;
  uint16 IHR_0xad6;
  uint16 IHR_0xad8;
  uint16 IHR_0xada;
  uint16 IHR_0xadc;
  uint16 IHR_0xade;
  uint16 ClkGateReqCtrl0;
  uint16 ClkGateReqCtrl1;
  uint16 ClkGateReqCtrl2;
  uint16 ClkGateUcodeReq0;
  uint16 ClkGateUcodeReq1;
  uint16 ClkGateUcodeReq2;
  uint16 ClkGateStretch0;
  uint16 ClkGateStretch1;
  uint16 ClkGateMisc;
  uint16 ClkGateDivCtrl;
  uint16 ClkGatePhyClkCtrl;
  uint16 ClkGateSts;
  uint16 ClkGateExtReq0;
  uint16 ClkGateExtReq1;
  uint16 ClkGateExtReq2;
  uint16 ClkGateUcodePhyClkCtrl;
  uint16 IHR_0xb00;
  uint16 IHR_0xb02;
  uint16 IHR_0xb04;
  uint16 IHR_0xb06;
  uint16 IHR_0xb08;
  uint16 IHR_0xb0a;
  uint16 IHR_0xb0c;
  uint16 IHR_0xb0e;
  uint16 IHR_0xb10;
  uint16 IHR_0xb12;
  uint16 IHR_0xb14;
  uint16 IHR_0xb16;
  uint16 IHR_0xb18;
  uint16 IHR_0xb1a;
  uint16 IHR_0xb1c;
  uint16 IHR_0xb1e;
  uint16 IHR_0xb20;
  uint16 IHR_0xb22;
  uint16 IHR_0xb24;
  uint16 IHR_0xb26;
  uint16 IHR_0xb28;
  uint16 IHR_0xb2a;
  uint16 IHR_0xb2c;
  uint16 IHR_0xb2e;
  uint16 IHR_0xb30;
  uint16 IHR_0xb32;
  uint16 IHR_0xb34;
  uint16 IHR_0xb36;
  uint16 IHR_0xb38;
  uint16 IHR_0xb3a;
  uint16 IHR_0xb3c;
  uint16 IHR_0xb3e;
  uint16 IHR_0xb40;
  uint16 IHR_0xb42;
  uint16 IHR_0xb44;
  uint16 IHR_0xb46;
  uint16 IHR_0xb48;
  uint16 IHR_0xb4a;
  uint16 IHR_0xb4c;
  uint16 IHR_0xb4e;
  uint16 IHR_0xb50;
  uint16 IHR_0xb52;
  uint16 IHR_0xb54;
  uint16 IHR_0xb56;
  uint16 IHR_0xb58;
  uint16 IHR_0xb5a;
  uint16 IHR_0xb5c;
  uint16 IHR_0xb5e;
  uint16 IHR_0xb60;
  uint16 IHR_0xb62;
  uint16 IHR_0xb64;
  uint16 IHR_0xb66;
  uint16 IHR_0xb68;
  uint16 IHR_0xb6a;
  uint16 IHR_0xb6c;
  uint16 IHR_0xb6e;
  uint16 IHR_0xb70;
  uint16 IHR_0xb72;
  uint16 IHR_0xb74;
  uint16 IHR_0xb76;
  uint16 IHR_0xb78;
  uint16 IHR_0xb7a;
  uint16 IHR_0xb7c;
  uint16 IHR_0xb7e;
  uint16 IHR_0xb80;
  uint16 IHR_0xb82;
  uint16 IHR_0xb84;
  uint16 IHR_0xb86;
  uint16 IHR_0xb88;
  uint16 IHR_0xb8a;
  uint16 IHR_0xb8c;
  uint16 IHR_0xb8e;
  uint16 IHR_0xb90;
  uint16 IHR_0xb92;
  uint16 IHR_0xb94;
  uint16 IHR_0xb96;
  uint16 IHR_0xb98;
  uint16 IHR_0xb9a;
  uint16 IHR_0xb9c;
  uint16 IHR_0xb9e;
  uint16 IHR_0xba0;
  uint16 IHR_0xba2;
  uint16 IHR_0xba4;
  uint16 IHR_0xba6;
  uint16 IHR_0xba8;
  uint16 IHR_0xbaa;
  uint16 IHR_0xbac;
  uint16 IHR_0xbae;
  uint16 IHR_0xbb0;
  uint16 IHR_0xbb2;
  uint16 IHR_0xbb4;
  uint16 IHR_0xbb6;
  uint16 IHR_0xbb8;
  uint16 IHR_0xbba;
  uint16 IHR_0xbbc;
  uint16 IHR_0xbbe;
  uint16 IHR_0xbc0;
  uint16 IHR_0xbc2;
  uint16 IHR_0xbc4;
  uint16 IHR_0xbc6;
  uint16 IHR_0xbc8;
  uint16 IHR_0xbca;
  uint16 IHR_0xbcc;
  uint16 IHR_0xbce;
  uint16 IHR_0xbd0;
  uint16 IHR_0xbd2;
  uint16 IHR_0xbd4;
  uint16 IHR_0xbd6;
  uint16 IHR_0xbd8;
  uint16 IHR_0xbda;
  uint16 IHR_0xbdc;
  uint16 IHR_0xbde;
  uint16 IHR_0xbe0;
  uint16 IHR_0xbe2;
  uint16 IHR_0xbe4;
  uint16 IHR_0xbe6;
  uint16 IHR_0xbe8;
  uint16 IHR_0xbea;
  uint16 IHR_0xbec;
  uint16 IHR_0xbee;
  uint16 IHR_0xbf0;
  uint16 IHR_0xbf2;
  uint16 IHR_0xbf4;
  uint16 IHR_0xbf6;
  uint16 IHR_0xbf8;
  uint16 IHR_0xbfa;
  uint16 IHR_0xbfc;
  uint16 IHR_0xbfe;
  uint16 IHR_0xc00;
  uint16 IHR_0xc02;
  uint16 IHR_0xc04;
  uint16 IHR_0xc06;
  uint16 IHR_0xc08;
  uint16 IHR_0xc0a;
  uint16 IHR_0xc0c;
  uint16 IHR_0xc0e;
  uint16 IHR_0xc10;
  uint16 IHR_0xc12;
  uint16 IHR_0xc14;
  uint16 IHR_0xc16;
  uint16 IHR_0xc18;
  uint16 IHR_0xc1a;
  uint16 IHR_0xc1c;
  uint16 IHR_0xc1e;
  uint16 IHR_0xc20;
  uint16 IHR_0xc22;
  uint16 IHR_0xc24;
  uint16 IHR_0xc26;
  uint16 IHR_0xc28;
  uint16 IHR_0xc2a;
  uint16 IHR_0xc2c;
  uint16 IHR_0xc2e;
  uint16 IHR_0xc30;
  uint16 IHR_0xc32;
  uint16 IHR_0xc34;
  uint16 IHR_0xc36;
  uint16 IHR_0xc38;
  uint16 IHR_0xc3a;
  uint16 IHR_0xc3c;
  uint16 IHR_0xc3e;
  uint16 IHR_0xc40;
  uint16 IHR_0xc42;
  uint16 IHR_0xc44;
  uint16 IHR_0xc46;
  uint16 IHR_0xc48;
  uint16 IHR_0xc4a;
  uint16 IHR_0xc4c;
  uint16 IHR_0xc4e;
  uint16 IHR_0xc50;
  uint16 IHR_0xc52;
  uint16 IHR_0xc54;
  uint16 IHR_0xc56;
  uint16 IHR_0xc58;
  uint16 IHR_0xc5a;
  uint16 IHR_0xc5c;
  uint16 IHR_0xc5e;
  uint16 IHR_0xc60;
  uint16 IHR_0xc62;
  uint16 IHR_0xc64;
  uint16 IHR_0xc66;
  uint16 IHR_0xc68;
  uint16 IHR_0xc6a;
  uint16 IHR_0xc6c;
  uint16 IHR_0xc6e;
  uint16 IHR_0xc70;
  uint16 IHR_0xc72;
  uint16 IHR_0xc74;
  uint16 IHR_0xc76;
  uint16 IHR_0xc78;
  uint16 IHR_0xc7a;
  uint16 IHR_0xc7c;
  uint16 IHR_0xc7e;
  uint16 IHR_0xc80;
  uint16 IHR_0xc82;
  uint16 IHR_0xc84;
  uint16 IHR_0xc86;
  uint16 IHR_0xc88;
  uint16 IHR_0xc8a;
  uint16 IHR_0xc8c;
  uint16 IHR_0xc8e;
  uint16 IHR_0xc90;
  uint16 IHR_0xc92;
  uint16 IHR_0xc94;
  uint16 IHR_0xc96;
  uint16 IHR_0xc98;
  uint16 IHR_0xc9a;
  uint16 IHR_0xc9c;
  uint16 IHR_0xc9e;
  uint16 IHR_0xca0;
  uint16 IHR_0xca2;
  uint16 IHR_0xca4;
  uint16 IHR_0xca6;
  uint16 IHR_0xca8;
  uint16 IHR_0xcaa;
  uint16 IHR_0xcac;
  uint16 IHR_0xcae;
  uint16 IHR_0xcb0;
  uint16 IHR_0xcb2;
  uint16 IHR_0xcb4;
  uint16 IHR_0xcb6;
  uint16 IHR_0xcb8;
  uint16 IHR_0xcba;
  uint16 IHR_0xcbc;
  uint16 IHR_0xcbe;
  uint16 IHR_0xcc0;
  uint16 IHR_0xcc2;
  uint16 IHR_0xcc4;
  uint16 IHR_0xcc6;
  uint16 IHR_0xcc8;
  uint16 IHR_0xcca;
  uint16 IHR_0xccc;
  uint16 IHR_0xcce;
  uint16 IHR_0xcd0;
  uint16 IHR_0xcd2;
  uint16 IHR_0xcd4;
  uint16 IHR_0xcd6;
  uint16 IHR_0xcd8;
  uint16 IHR_0xcda;
  uint16 IHR_0xcdc;
  uint16 IHR_0xcde;
  uint16 IHR_0xce0;
  uint16 IHR_0xce2;
  uint16 IHR_0xce4;
  uint16 IHR_0xce6;
  uint16 IHR_0xce8;
  uint16 IHR_0xcea;
  uint16 IHR_0xcec;
  uint16 IHR_0xcee;
  uint16 IHR_0xcf0;
  uint16 IHR_0xcf2;
  uint16 IHR_0xcf4;
  uint16 IHR_0xcf6;
  uint16 IHR_0xcf8;
  uint16 IHR_0xcfa;
  uint16 IHR_0xcfc;
  uint16 IHR_0xcfe;
  uint16 IHR_0xd00;
  uint16 IHR_0xd02;
  uint16 IHR_0xd04;
  uint16 IHR_0xd06;
  uint16 IHR_0xd08;
  uint16 IHR_0xd0a;
  uint16 IHR_0xd0c;
  uint16 IHR_0xd0e;
  uint16 IHR_0xd10;
  uint16 IHR_0xd12;
  uint16 IHR_0xd14;
  uint16 IHR_0xd16;
  uint16 IHR_0xd18;
  uint16 IHR_0xd1a;
  uint16 IHR_0xd1c;
  uint16 IHR_0xd1e;
  uint16 IHR_0xd20;
  uint16 IHR_0xd22;
  uint16 IHR_0xd24;
  uint16 IHR_0xd26;
  uint16 IHR_0xd28;
  uint16 IHR_0xd2a;
  uint16 IHR_0xd2c;
  uint16 IHR_0xd2e;
  uint16 IHR_0xd30;
  uint16 IHR_0xd32;
  uint16 IHR_0xd34;
  uint16 IHR_0xd36;
  uint16 IHR_0xd38;
  uint16 IHR_0xd3a;
  uint16 IHR_0xd3c;
  uint16 IHR_0xd3e;
  uint16 IHR_0xd40;
  uint16 IHR_0xd42;
  uint16 IHR_0xd44;
  uint16 IHR_0xd46;
  uint16 IHR_0xd48;
  uint16 IHR_0xd4a;
  uint16 IHR_0xd4c;
  uint16 IHR_0xd4e;
  uint16 IHR_0xd50;
  uint16 IHR_0xd52;
  uint16 IHR_0xd54;
  uint16 IHR_0xd56;
  uint16 IHR_0xd58;
  uint16 IHR_0xd5a;
  uint16 IHR_0xd5c;
  uint16 IHR_0xd5e;
  uint16 IHR_0xd60;
  uint16 IHR_0xd62;
  uint16 IHR_0xd64;
  uint16 IHR_0xd66;
  uint16 IHR_0xd68;
  uint16 IHR_0xd6a;
  uint16 IHR_0xd6c;
  uint16 IHR_0xd6e;
  uint16 IHR_0xd70;
  uint16 IHR_0xd72;
  uint16 IHR_0xd74;
  uint16 IHR_0xd76;
  uint16 IHR_0xd78;
  uint16 IHR_0xd7a;
  uint16 IHR_0xd7c;
  uint16 IHR_0xd7e;
  uint16 IHR_0xd80;
  uint16 IHR_0xd82;
  uint16 IHR_0xd84;
  uint16 IHR_0xd86;
  uint16 IHR_0xd88;
  uint16 IHR_0xd8a;
  uint16 IHR_0xd8c;
  uint16 IHR_0xd8e;
  uint16 IHR_0xd90;
  uint16 IHR_0xd92;
  uint16 IHR_0xd94;
  uint16 IHR_0xd96;
  uint16 IHR_0xd98;
  uint16 IHR_0xd9a;
  uint16 IHR_0xd9c;
  uint16 IHR_0xd9e;
  uint16 IHR_0xda0;
  uint16 IHR_0xda2;
  uint16 IHR_0xda4;
  uint16 IHR_0xda6;
  uint16 IHR_0xda8;
  uint16 IHR_0xdaa;
  uint16 IHR_0xdac;
  uint16 IHR_0xdae;
  uint16 IHR_0xdb0;
  uint16 IHR_0xdb2;
  uint16 IHR_0xdb4;
  uint16 IHR_0xdb6;
  uint16 IHR_0xdb8;
  uint16 IHR_0xdba;
  uint16 IHR_0xdbc;
  uint16 IHR_0xdbe;
  uint16 IHR_0xdc0;
  uint16 IHR_0xdc2;
  uint16 IHR_0xdc4;
  uint16 IHR_0xdc6;
  uint16 IHR_0xdc8;
  uint16 IHR_0xdca;
  uint16 IHR_0xdcc;
  uint16 IHR_0xdce;
  uint16 IHR_0xdd0;
  uint16 IHR_0xdd2;
  uint16 IHR_0xdd4;
  uint16 IHR_0xdd6;
  uint16 IHR_0xdd8;
  uint16 IHR_0xdda;
  uint16 IHR_0xddc;
  uint16 IHR_0xdde;
  uint16 IHR_0xde0;
  uint16 IHR_0xde2;
  uint16 IHR_0xde4;
  uint16 IHR_0xde6;
  uint16 IHR_0xde8;
  uint16 IHR_0xdea;
  uint16 IHR_0xdec;
  uint16 IHR_0xdee;
  uint16 IHR_0xdf0;
  uint16 IHR_0xdf2;
  uint16 IHR_0xdf4;
  uint16 IHR_0xdf6;
  uint16 IHR_0xdf8;
  uint16 IHR_0xdfa;
  uint16 IHR_0xdfc;
  uint16 IHR_0xdfe;
  uint16 IHR_0xe00;
  uint16 IHR_0xe02;
  uint16 IHR_0xe04;
  uint16 IHR_0xe06;
  uint16 IHR_0xe08;
  uint16 IHR_0xe0a;
  uint16 IHR_0xe0c;
  uint16 IHR_0xe0e;
  uint16 IHR_0xe10;
  uint16 IHR_0xe12;
  uint16 IHR_0xe14;
  uint16 IHR_0xe16;
  uint16 IHR_0xe18;
  uint16 IHR_0xe1a;
  uint16 IHR_0xe1c;
  uint16 IHR_0xe1e;
  uint16 IHR_0xe20;
  uint16 IHR_0xe22;
  uint16 IHR_0xe24;
  uint16 IHR_0xe26;
  uint16 IHR_0xe28;
  uint16 IHR_0xe2a;
  uint16 IHR_0xe2c;
  uint16 IHR_0xe2e;
  uint16 IHR_0xe30;
  uint16 IHR_0xe32;
  uint16 IHR_0xe34;
  uint16 IHR_0xe36;
  uint16 IHR_0xe38;
  uint16 IHR_0xe3a;
  uint16 IHR_0xe3c;
  uint16 IHR_0xe3e;
  uint16 IHR_0xe40;
  uint16 IHR_0xe42;
  uint16 IHR_0xe44;
  uint16 IHR_0xe46;
  uint16 IHR_0xe48;
  uint16 IHR_0xe4a;
  uint16 IHR_0xe4c;
  uint16 IHR_0xe4e;
  uint16 IHR_0xe50;
  uint16 IHR_0xe52;
  uint16 IHR_0xe54;
  uint16 IHR_0xe56;
  uint16 IHR_0xe58;
  uint16 IHR_0xe5a;
  uint16 IHR_0xe5c;
  uint16 IHR_0xe5e;
  uint16 IHR_0xe60;
  uint16 IHR_0xe62;
  uint16 IHR_0xe64;
  uint16 IHR_0xe66;
  uint16 IHR_0xe68;
  uint16 IHR_0xe6a;
  uint16 IHR_0xe6c;
  uint16 IHR_0xe6e;
  uint16 IHR_0xe70;
  uint16 IHR_0xe72;
  uint16 IHR_0xe74;
  uint16 IHR_0xe76;
  uint16 IHR_0xe78;
  uint16 IHR_0xe7a;
  uint16 IHR_0xe7c;
  uint16 IHR_0xe7e;
  uint16 IHR_0xe80;
  uint16 IHR_0xe82;
  uint16 IHR_0xe84;
  uint16 IHR_0xe86;
  uint16 IHR_0xe88;
  uint16 IHR_0xe8a;
  uint16 IHR_0xe8c;
  uint16 IHR_0xe8e;
  uint16 IHR_0xe90;
  uint16 IHR_0xe92;
  uint16 IHR_0xe94;
  uint16 IHR_0xe96;
  uint16 IHR_0xe98;
  uint16 IHR_0xe9a;
  uint16 IHR_0xe9c;
  uint16 IHR_0xe9e;
  uint16 IHR_0xea0;
  uint16 IHR_0xea2;
  uint16 IHR_0xea4;
  uint16 IHR_0xea6;
  uint16 IHR_0xea8;
  uint16 IHR_0xeaa;
  uint16 IHR_0xeac;
  uint16 IHR_0xeae;
  uint16 IHR_0xeb0;
  uint16 IHR_0xeb2;
  uint16 IHR_0xeb4;
  uint16 IHR_0xeb6;
  uint16 IHR_0xeb8;
  uint16 IHR_0xeba;
  uint16 IHR_0xebc;
  uint16 IHR_0xebe;
  uint16 IHR_0xec0;
  uint16 IHR_0xec2;
  uint16 IHR_0xec4;
  uint16 IHR_0xec6;
  uint16 IHR_0xec8;
  uint16 IHR_0xeca;
  uint16 IHR_0xecc;
  uint16 IHR_0xece;
  uint16 IHR_0xed0;
  uint16 IHR_0xed2;
  uint16 IHR_0xed4;
  uint16 IHR_0xed6;
  uint16 IHR_0xed8;
  uint16 IHR_0xeda;
  uint16 IHR_0xedc;
  uint16 IHR_0xede;
  uint16 IHR_0xee0;
  uint16 IHR_0xee2;
  uint16 IHR_0xee4;
  uint16 IHR_0xee6;
  uint16 IHR_0xee8;
  uint16 IHR_0xeea;
  uint16 IHR_0xeec;
  uint16 IHR_0xeee;
  uint16 IHR_0xef0;
  uint16 IHR_0xef2;
  uint16 IHR_0xef4;
  uint16 IHR_0xef6;
  uint16 IHR_0xef8;
  uint16 IHR_0xefa;
  uint16 IHR_0xefc;
  uint16 IHR_0xefe;
  uint16 sbconfig_0xf00;
  uint16 sbconfig_0xf02;
  uint16 sbconfig_0xf04;
  uint16 sbconfig_0xf06;
  uint16 sbconfig_0xf08;
  uint16 sbconfig_0xf0a;
  uint16 sbconfig_0xf0c;
  uint16 sbconfig_0xf0e;
  uint16 sbconfig_0xf10;
  uint16 sbconfig_0xf12;
  uint16 sbconfig_0xf14;
  uint16 sbconfig_0xf16;
  uint16 sbconfig_0xf18;
  uint16 sbconfig_0xf1a;
  uint16 sbconfig_0xf1c;
  uint16 sbconfig_0xf1e;
  uint16 sbconfig_0xf20;
  uint16 sbconfig_0xf22;
  uint16 sbconfig_0xf24;
  uint16 sbconfig_0xf26;
  uint16 sbconfig_0xf28;
  uint16 sbconfig_0xf2a;
  uint16 sbconfig_0xf2c;
  uint16 sbconfig_0xf2e;
  uint16 sbconfig_0xf30;
  uint16 sbconfig_0xf32;
  uint16 sbconfig_0xf34;
  uint16 sbconfig_0xf36;
  uint16 sbconfig_0xf38;
  uint16 sbconfig_0xf3a;
  uint16 sbconfig_0xf3c;
  uint16 sbconfig_0xf3e;
  uint16 sbconfig_0xf40;
  uint16 sbconfig_0xf42;
  uint16 sbconfig_0xf44;
  uint16 sbconfig_0xf46;
  uint16 sbconfig_0xf48;
  uint16 sbconfig_0xf4a;
  uint16 sbconfig_0xf4c;
  uint16 sbconfig_0xf4e;
  uint16 sbconfig_0xf50;
  uint16 sbconfig_0xf52;
  uint16 sbconfig_0xf54;
  uint16 sbconfig_0xf56;
  uint16 sbconfig_0xf58;
  uint16 sbconfig_0xf5a;
  uint16 sbconfig_0xf5c;
  uint16 sbconfig_0xf5e;
  uint16 sbconfig_0xf60;
  uint16 sbconfig_0xf62;
  uint16 sbconfig_0xf64;
  uint16 sbconfig_0xf66;
  uint16 sbconfig_0xf68;
  uint16 sbconfig_0xf6a;
  uint16 sbconfig_0xf6c;
  uint16 sbconfig_0xf6e;
  uint16 sbconfig_0xf70;
  uint16 sbconfig_0xf72;
  uint16 sbconfig_0xf74;
  uint16 sbconfig_0xf76;
  uint16 sbconfig_0xf78;
  uint16 sbconfig_0xf7a;
  uint16 sbconfig_0xf7c;
  uint16 sbconfig_0xf7e;
  uint16 sbconfig_0xf80;
  uint16 sbconfig_0xf82;
  uint16 sbconfig_0xf84;
  uint16 sbconfig_0xf86;
  uint16 sbconfig_0xf88;
  uint16 sbconfig_0xf8a;
  uint16 sbconfig_0xf8c;
  uint16 sbconfig_0xf8e;
  uint16 sbconfig_0xf90;
  uint16 sbconfig_0xf92;
  uint16 sbconfig_0xf94;
  uint16 sbconfig_0xf96;
  uint16 sbconfig_0xf98;
  uint16 sbconfig_0xf9a;
  uint16 sbconfig_0xf9c;
  uint16 sbconfig_0xf9e;
  uint16 sbconfig_0xfa0;
  uint16 sbconfig_0xfa2;
  uint16 sbconfig_0xfa4;
  uint16 sbconfig_0xfa6;
  uint16 sbconfig_0xfa8;
  uint16 sbconfig_0xfaa;
  uint16 sbconfig_0xfac;
  uint16 sbconfig_0xfae;
  uint16 sbconfig_0xfb0;
  uint16 sbconfig_0xfb2;
  uint16 sbconfig_0xfb4;
  uint16 sbconfig_0xfb6;
  uint16 sbconfig_0xfb8;
  uint16 sbconfig_0xfba;
  uint16 sbconfig_0xfbc;
  uint16 sbconfig_0xfbe;
  uint16 sbconfig_0xfc0;
  uint16 sbconfig_0xfc2;
  uint16 sbconfig_0xfc4;
  uint16 sbconfig_0xfc6;
  uint16 sbconfig_0xfc8;
  uint16 sbconfig_0xfca;
  uint16 sbconfig_0xfcc;
  uint16 sbconfig_0xfce;
  uint16 sbconfig_0xfd0;
  uint16 sbconfig_0xfd2;
  uint16 sbconfig_0xfd4;
  uint16 sbconfig_0xfd6;
  uint16 sbconfig_0xfd8;
  uint16 sbconfig_0xfda;
  uint16 sbconfig_0xfdc;
  uint16 sbconfig_0xfde;
  uint16 sbconfig_0xfe0;
  uint16 sbconfig_0xfe2;
  uint16 sbconfig_0xfe4;
  uint16 sbconfig_0xfe6;
  uint16 sbconfig_0xfe8;
  uint16 sbconfig_0xfea;
  uint16 sbconfig_0xfec;
  uint16 sbconfig_0xfee;
  uint16 sbconfig_0xff0;
  uint16 sbconfig_0xff2;
  uint16 sbconfig_0xff4;
  uint16 sbconfig_0xff6;
  uint16 sbconfig_0xff8;
  uint16 sbconfig_0xffa;
  uint16 sbconfig_0xffc;
  uint16 sbconfig_0xffe;
} __attribute__((packed));

/*
 * Host Interface Registers
 */
//struct d11regs {
//    /* Device Control ("semi-standard host registers") */
//    unsigned int PAD[3];     /* 0x0 - 0x8 */
//    unsigned int biststatus; /* 0xC */
//    unsigned int biststatus2;    /* 0x10 */
//    unsigned int PAD;        /* 0x14 */
//    unsigned int gptimer;        /* 0x18 */
//    unsigned int usectimer;  /* 0x1c *//* for corerev >= 26 */
//
//    /* Interrupt Control *//* 0x20 */
//    struct intctrlregs intctrlregs[8];
//
//    unsigned int PAD[40];        /* 0x60 - 0xFC */
//
//    unsigned int intrcvlazy[4];  /* 0x100 - 0x10C */
//
//    unsigned int PAD[4];     /* 0x110 - 0x11c */
//
//    uint32 maccontrol; /* 0x120 */
//    unsigned int maccommand; /* 0x124 */
//    unsigned int macintstatus;   /* 0x128 */
//    unsigned int macintmask; /* 0x12C */
//
//    /* Transmit Template Access */
//    unsigned int tplatewrptr;    /* 0x130 */
//    unsigned int tplatewrdata;   /* 0x134 */
//    unsigned int PAD[2];     /* 0x138 - 0x13C */
//
//    /* PMQ registers */
//    union pmqreg pmqreg;    /* 0x140 */
//    unsigned int pmqpatl;        /* 0x144 */
//    unsigned int pmqpath;        /* 0x148 */
//    unsigned int PAD;        /* 0x14C */
//
//    unsigned int chnstatus;  /* 0x150 */
//    unsigned int psmdebug;   /* 0x154 */
//    unsigned int phydebug;   /* 0x158 */
//    unsigned int machwcap;   /* 0x15C */
//
//    /* Extended Internal Objects */
//    unsigned int objaddr;        /* 0x160 */
//    unsigned int objdata;        /* 0x164 */
//    unsigned int PAD[2];     /* 0x168 - 0x16c */
//
//    unsigned int frmtxstatus;    /* 0x170 */
//    unsigned int frmtxstatus2;   /* 0x174 */
//    unsigned int PAD[2];     /* 0x178 - 0x17c */
//
//    /* TSF host access */
//    unsigned int tsf_timerlow;   /* 0x180 */
//    unsigned int tsf_timerhigh;  /* 0x184 */
//    unsigned int tsf_cfprep; /* 0x188 */
//    unsigned int tsf_cfpstart;   /* 0x18c */
//    unsigned int tsf_cfpmaxdur32;    /* 0x190 */
//    unsigned int PAD[3];     /* 0x194 - 0x19c */
//
//    unsigned int maccontrol1;    /* 0x1a0 */
//    unsigned int machwcap1;  /* 0x1a4 */
//    unsigned int PAD[14];        /* 0x1a8 - 0x1dc */
//
//    /* Clock control and hardware workarounds*/
//    unsigned int clk_ctl_st; /* 0x1e0 */
//    unsigned int hw_war;
//    unsigned int d11_phypllctl;  /* the phypll request/avail bits are
//                 * moved to clk_ctl_st
//                 */
//    unsigned int PAD[5];     /* 0x1ec - 0x1fc */
//
//    /* 0x200-0x37F dma/pio registers */
//    struct fifo64 fifo64regs[6];
//
//    /* FIFO diagnostic port access */
//    struct dma32diag dmafifo;   /* 0x380 - 0x38C */
//
//    unsigned int aggfifocnt; /* 0x390 */
//    unsigned int aggfifodata;    /* 0x394 */
//    unsigned int PAD[16];        /* 0x398 - 0x3d4 */
//    unsigned short radioregaddr;   /* 0x3d8 */
//    unsigned short radioregdata;   /* 0x3da */
//
//    /*
//     * time delay between the change on rf disable input and
//     * radio shutdown
//     */
//    unsigned int rfdisabledly;   /* 0x3DC */
//
//    /* PHY register access */
//    unsigned short phyversion; /* 0x3e0 - 0x0 */
//    unsigned short phybbconfig;    /* 0x3e2 - 0x1 */
//    unsigned short phyadcbias; /* 0x3e4 - 0x2  Bphy only */
//    unsigned short phyanacore; /* 0x3e6 - 0x3  pwwrdwn on aphy */
//    unsigned short phyrxstatus0;   /* 0x3e8 - 0x4 */
//    unsigned short phyrxstatus1;   /* 0x3ea - 0x5 */
//    unsigned short phycrsth;   /* 0x3ec - 0x6 */
//    unsigned short phytxerror; /* 0x3ee - 0x7 */
//    unsigned short phychannel; /* 0x3f0 - 0x8 */
//    unsigned short PAD[1];     /* 0x3f2 - 0x9 */
//    unsigned short phytest;        /* 0x3f4 - 0xa */
//    unsigned short phy4waddr;  /* 0x3f6 - 0xb */
//    unsigned short phy4wdatahi;    /* 0x3f8 - 0xc */
//    unsigned short phy4wdatalo;    /* 0x3fa - 0xd */
//    unsigned short phyregaddr; /* 0x3fc - 0xe */
//    unsigned short phyregdata; /* 0x3fe - 0xf */
//
//    /* IHR *//* 0x400 - 0x7FE */
//
//    /* RXE Block */
//    unsigned short PAD;                 /* SPR_RXE_0x00                     0x400 */
//    unsigned short PAD;                 /* SPR_RXE_Copy_Offset              0x402 */
//    unsigned short PAD;                 /* SPR_RXE_Copy_Length              0x404 */
//    unsigned short rcv_fifo_ctl;        /* SPR_RXE_FIFOCTL0                 0x406 */
//    unsigned short PAD;                 /* SPR_RXE_FIFOCTL1                 0x408 */
//    unsigned short rcv_frm_cnt;         /* SPR_Received_Frame_Count         0x40a */
//    unsigned short PAD;                 /* SPR_RXE_0x0c                     0x40c */
//    unsigned short PAD;                 /* SPR_RXE_RXHDR_OFFSET             0x40e */
//    unsigned short PAD;                 /* SPR_RXE_RXHDR_LEN                0x410 */
//    unsigned short PAD;                 /* SPR_RXE_PHYRXSTAT0               0x412 */
//    unsigned short rssi;                /* SPR_RXE_PHYRXSTAT1               0x414 */
//    unsigned short PAD;                 /* SPR_RXE_0x16                     0x416 */
//    unsigned short PAD;                 /* SPR_RXE_FRAMELEN                 0x418 */
//    unsigned short PAD;                 /* SPR_RXE_0x1a                     0x41a */
//    unsigned short PAD;                 /* SPR_RXE_ENCODING                 0x41c */
//    unsigned short PAD;                 /* SPR_RXE_0x1e                     0x41e */
//    unsigned short rcm_ctl;             /* SPR_RCM_Control                  0x420 */
//    unsigned short rcm_mat_data;        /* SPR_RCM_Match_Data               0x422 */
//    unsigned short rcm_mat_mask;        /* SPR_RCM_Match_Mask               0x424 */
//    unsigned short rcm_mat_dly;         /* SPR_RCM_Match_Delay              0x426 */
//    unsigned short rcm_cond_mask_l;     /* SPR_RCM_Condition_Mask_Low       0x428 */
//    unsigned short rcm_cond_mask_h;     /* SPR_RCM_Condition_Mask_High      0x42A */
//    unsigned short rcm_cond_dly;        /* SPR_RCM_Condition_Delay          0x42C */
//    unsigned short PAD;                 /* SPR_RXE_0x2e                     0x42E */
//    unsigned short ext_ihr_addr;        /* SPR_Ext_IHR_Address              0x430 */
//    unsigned short ext_ihr_data;        /* SPR_Ext_IHR_Data                 0x432 */
//    unsigned short rxe_phyrs_2;         /* SPR_RXE_PHYRXSTAT2               0x434 */
//    unsigned short rxe_phyrs_3;         /* SPR_RXE_PHYRXSTAT3               0x436 */
//    unsigned short phy_mode;            /* SPR_PHY_Mode                     0x438 */
//    unsigned short rcmta_ctl;           /* SPR_RCM_TA_Control               0x43a */
//    unsigned short rcmta_size;          /* SPR_RCM_TA_Size                  0x43c */
//    unsigned short rcmta_addr0;         /* SPR_RCM_TA_Address_0             0x43e */
//    unsigned short rcmta_addr1;         /* SPR_RCM_TA_Address_1             0x440 */
//    unsigned short rcmta_addr2;         /* SPR_RCM_TA_Address_2             0x442 */
//    unsigned short PAD[30];             /* SPR_RXE_0x44 ... 0x7e            0x444 */
//
//
//    /* PSM Block *//* 0x480 - 0x500 */
//
//    unsigned short PAD;                 /* SPR_MAC_MAX_NAP                  0x480 */
//    unsigned short psm_maccontrol_h;    /* SPR_MAC_CTLHI                    0x482 */
//    unsigned short psm_macintstatus_l;  /* SPR_MAC_IRQLO                    0x484 */
//    unsigned short psm_macintstatus_h;  /* SPR_MAC_IRQHI                    0x486 */
//    unsigned short psm_macintmask_l;    /* SPR_MAC_IRQMASKLO                0x488 */
//    unsigned short psm_macintmask_h;    /* SPR_MAC_IRQMASKHI                0x48A */
//    unsigned short PAD;                 /* SPR_PSM_0x0c                     0x48C */
//    unsigned short psm_maccommand;      /* SPR_MAC_CMD                      0x48E */
//    unsigned short psm_brc;             /* SPR_BRC                          0x490 */
//    unsigned short psm_phy_hdr_param;   /* SPR_PHY_HDR_Parameter            0x492 */
//    unsigned short psm_postcard;        /* SPR_Postcard                     0x494 */
//    unsigned short psm_pcard_loc_l;     /* SPR_Postcard_Location_Low        0x496 */
//    unsigned short psm_pcard_loc_h;     /* SPR_Postcard_Location_High       0x498 */
//    unsigned short psm_gpio_in;         /* SPR_GPIO_IN                      0x49A */
//    unsigned short psm_gpio_out;        /* SPR_GPIO_OUT                     0x49C */
//    unsigned short psm_gpio_oe;         /* SPR_GPIO_OUTEN                   0x49E */
//
//    unsigned short psm_bred_0;          /* SPR_BRED0                        0x4A0 */
//    unsigned short psm_bred_1;          /* SPR_BRED1                        0x4A2 */
//    unsigned short psm_bred_2;          /* SPR_BRED2                        0x4A4 */
//    unsigned short psm_bred_3;          /* SPR_BRED3                        0x4A6 */
//    unsigned short psm_brcl_0;          /* SPR_BRCL0                        0x4A8 */
//    unsigned short psm_brcl_1;          /* SPR_BRCL1                        0x4AA */
//    unsigned short psm_brcl_2;          /* SPR_BRCL2                        0x4AC */
//    unsigned short psm_brcl_3;          /* SPR_BRCL3                        0x4AE */
//    unsigned short psm_brpo_0;          /* SPR_BRPO0                        0x4B0 */
//    unsigned short psm_brpo_1;          /* SPR_BRPO1                        0x4B2 */
//    unsigned short psm_brpo_2;          /* SPR_BRPO2                        0x4B4 */
//    unsigned short psm_brpo_3;          /* SPR_BRPO3                        0x4B6 */
//    unsigned short psm_brwk_0;          /* SPR_BRWK0                        0x4B8 */
//    unsigned short psm_brwk_1;          /* SPR_BRWK1                        0x4BA */
//    unsigned short psm_brwk_2;          /* SPR_BRWK2                        0x4BC */
//    unsigned short psm_brwk_3;          /* SPR_BRWK3                        0x4BE */
//
//    unsigned short psm_base_0;          /* SPR_BASE0 - Offset Register 0    0x4C0 */
//    unsigned short psm_base_1;          /* SPR_BASE1 - Offset Register 1    0x4C2 */
//    unsigned short psm_base_2;          /* SPR_BASE2 - Offset Register 2    0x4C4 */
//    unsigned short psm_base_3;          /* SPR_BASE3 - Offset Register 3    0x4C6 */
//    unsigned short psm_base_4;          /* SPR_BASE4 - Offset Register 4    0x4C8 */
//    unsigned short psm_base_5;          /* SPR_BASE5 - Offset Register 5    0x4CA */
//    unsigned short psm_base_6;          /* SPR_BASE6 - Do not use (broken)  0x4CC */
//    unsigned short psm_pc_reg_0;        /* SPR_PSM_0x4e                     0x4CE */
//    unsigned short psm_pc_reg_1;        /* SPR_PC0 - Link Register 0        0x4D0 */
//    unsigned short psm_pc_reg_2;        /* SPR_PC1 - Link Register 1        0x4D2 */
//    unsigned short psm_pc_reg_3;        /* SPR_PC2 - Link Register 2        0x4D4 */
//    unsigned short PAD;                 /* SPR_PC2 - Link Register 6        0x4D6 */
//    unsigned short PAD;                 /* SPR_PSM_COND - PSM external condition bits 0x4D8 */
//    unsigned short PAD;                 /* SPR_PSM_0x5a ... 0x7e            0x4DA */
//    unsigned short PAD;                 /* SPR_PSM_0x5c                     0x4DC */
//    unsigned short PAD;                 /* SPR_PSM_0x5e                     0x4DE */
//    unsigned short PAD;                 /* SPR_PSM_0x60                     0x4E0 */
//    unsigned short PAD;                 /* SPR_PSM_0x62                     0x4E2 */
//    unsigned short PAD;                 /* SPR_PSM_0x64                     0x4E4 */
//    unsigned short PAD;                 /* SPR_PSM_0x66                     0x4E6 */
//    unsigned short PAD;                 /* SPR_PSM_0x68                     0x4E8 */
//    unsigned short PAD;                 /* SPR_PSM_0x6a                     0x4EA */
//    unsigned short PAD;                 /* SPR_PSM_0x6c                     0x4EC */
//    unsigned short PAD;                 /* SPR_PSM_0x6e                     0x4EE */
//    unsigned short psm_corectlsts;      /* SPR_PSM_0x70                     0x4F0 *//* Corerev >= 13 */  
//    unsigned short PAD;                 /* SPR_PSM_0x72                     0x4F2 */
//    unsigned short PAD;                 /* SPR_PSM_0x74                     0x4F4 */
//    unsigned short PAD;                 /* SPR_PSM_0x76                     0x4F6 */
//    unsigned short PAD;                 /* SPR_PSM_0x78                     0x4F8 */
//    unsigned short PAD;                 /* SPR_PSM_0x7a                     0x4FA */
//    unsigned short PAD;                 /* SPR_PSM_0x7c                     0x4FC */
//    unsigned short PAD;                 /* SPR_PSM_0x7e                     0x4FE */
//
//    /* TXE0 Block *//* 0x500 - 0x580 */
//    unsigned short txe_ctl;             /* SPR_TXE0_CTL                     0x500 */
//    unsigned short txe_aux;             /* SPR_TXE0_AUX                     0x502 */
//    unsigned short txe_ts_loc;          /* SPR_TXE0_TS_LOC                  0x504 */
//    unsigned short txe_time_out;        /* SPR_TXE0_TIMEOUT                 0x506 */
//    unsigned short txe_wm_0;            /* SPR_TXE0_WM0                     0x508 */
//    unsigned short txe_wm_1;            /* SPR_TXE0_WM1                     0x50A */
//    unsigned short txe_phyctl;          /* SPR_TXE0_PHY_CTL                 0x50C */
//    unsigned short txe_status;          /* SPR_TXE0_STATUS                  0x50E */
//    unsigned short txe_mmplcp0;         /* SPR_TXE0_0x10                    0x510 */
//    unsigned short txe_mmplcp1;         /* SPR_TXE0_0x12                    0x512 */
//    unsigned short txe_phyctl1;         /* SPR_TXE0_0x14                    0x514 */
//
//    unsigned short PAD;                 /* SPR_TXE0_0x16                    0x516 */
//    unsigned short PAD;                 /* SPR_TX_STATUS0                   0x518 */
//    unsigned short PAD;                 /* SPR_TX_STATUS1                   0x51a */
//    unsigned short PAD;                 /* SPR_TX_STATUS2                   0x51c */
//    unsigned short PAD;                 /* SPR_TX_STATUS3                   0x51e */
//
//    /* Transmit control */
//    unsigned short xmtfifodef;          /* SPR_TXE0_FIFO_Def                0x520 */
//    unsigned short xmtfifo_frame_cnt;   /* SPR_TXE0_0x22                    0x522 *//* Corerev >= 16 */
//    unsigned short xmtfifo_byte_cnt;    /* SPR_TXE0_0x24                    0x524 *//* Corerev >= 16 */
//    unsigned short xmtfifo_head;        /* SPR_TXE0_0x26                    0x526 *//* Corerev >= 16 */
//    unsigned short xmtfifo_rd_ptr;      /* SPR_TXE0_0x28                    0x528 *//* Corerev >= 16 */
//    unsigned short xmtfifo_wr_ptr;      /* SPR_TXE0_0x2a                    0x52A *//* Corerev >= 16 */
//    unsigned short xmtfifodef1;         /* SPR_TXE0_0x2c                    0x52C *//* Corerev >= 16 */
//
//    unsigned short PAD;                 /* SPR_TXE0_0x2e                    0x52E */
//    unsigned short PAD;                 /* SPR_TXE0_0x30                    0x530 */
//    unsigned short PAD;                 /* SPR_TXE0_0x32                    0x532 */
//    unsigned short PAD;                 /* SPR_TXE0_0x34                    0x534 */
//    unsigned short PAD;                 /* SPR_TXE0_0x36                    0x536 */
//    unsigned short PAD;                 /* SPR_TXE0_0x38                    0x538 */
//    unsigned short PAD;                 /* SPR_TXE0_0x3a                    0x53A */
//    unsigned short PAD;                 /* SPR_TXE0_0x3c                    0x53C */
//    unsigned short PAD;                 /* SPR_TXE0_0x3e                    0x53E */
//
//    unsigned short xmtfifocmd;          /* SPR_TXE0_FIFO_CMD                0x540 */
//    unsigned short xmtfifoflush;        /* SPR_TXE0_FIFO_FLUSH              0x542 */
//    unsigned short xmtfifothresh;       /* SPR_TXE0_FIFO_THRES              0x544 */
//    unsigned short xmtfifordy;          /* SPR_TXE0_FIFO_RDY                0x546 */
//    unsigned short xmtfifoprirdy;       /* SPR_TXE0_FIFO_PRI_RDY            0x548 */
//    unsigned short xmtfiforqpri;        /* SPR_TXE0_FIFO_RQ_PRI             0x54A */
//    unsigned short xmttplatetxptr;      /* SPR_TXE0_Template_TX_Pointer     0x54C */
//    unsigned short PAD;                 /* SPR_TXE0_0x4e                    0x54E */
//    unsigned short xmttplateptr;        /* SPR_TXE0_Template_Pointer        0x550 */
//    unsigned short smpl_clct_strptr;    /* SPR_TXE0_0x52                    0x552 *//* Corerev >= 22 */
//    unsigned short smpl_clct_stpptr;    /* SPR_TXE0_0x54                    0x554 *//* Corerev >= 22 */
//    unsigned short smpl_clct_curptr;    /* SPR_TXE0_0x56                    0x556 *//* Corerev >= 22 */
//    unsigned short PAD;                 /* SPR_TXE0_0x58                    0x558 */
//    unsigned short PAD;                 /* SPR_TXE0_0x5a                    0x55A */
//    unsigned short PAD;                 /* SPR_TXE0_0x5c                    0x55C */
//    unsigned short PAD;                 /* SPR_TXE0_0x5e                    0x55E */
//    unsigned short xmttplatedatalo;     /* SPR_TXE0_Template_Data_Low       0x560 */
//    unsigned short xmttplatedatahi;     /* SPR_TXE0_Template_Data_High      0x562 */
//
//    unsigned short PAD;                 /* SPR_TXE0_0x64                    0x564 */
//    unsigned short PAD;                 /* SPR_TXE0_0x66                    0x566 */
//
//    unsigned short xmtsel;              /* SPR_TXE0_SELECT                  0x568 */
//    unsigned short xmttxcnt;            /* 0x56A */
//    unsigned short xmttxshmaddr;        /* 0x56C */
//
//    unsigned short PAD[0x09];  /* 0x56E - 0x57E */
//
//    /* TXE1 Block */
//    unsigned short PAD[0x40];  /* 0x580 - 0x5FE */
//
//    /* TSF Block */
//    unsigned short PAD[0X02];  /* 0x600 - 0x602 */
//    unsigned short tsf_cfpstrt_l;  /* 0x604 */
//    unsigned short tsf_cfpstrt_h;  /* 0x606 */
//    unsigned short PAD[0X05];  /* 0x608 - 0x610 */
//    unsigned short tsf_cfppretbtt; /* 0x612 */
//    unsigned short PAD[0XD];   /* 0x614 - 0x62C */
//    unsigned short tsf_clk_frac_l; /* 0x62E */
//    unsigned short tsf_clk_frac_h; /* 0x630 */
//    unsigned short PAD[0X14];  /* 0x632 - 0x658 */
//    unsigned short tsf_random; /* 0x65A */
//    unsigned short PAD[0x05];  /* 0x65C - 0x664 */
//    /* GPTimer 2 registers */
//    unsigned short tsf_gpt2_stat;  /* 0x666 */
//    unsigned short tsf_gpt2_ctr_l; /* 0x668 */
//    unsigned short tsf_gpt2_ctr_h; /* 0x66A */
//    unsigned short tsf_gpt2_val_l; /* 0x66C */
//    unsigned short tsf_gpt2_val_h; /* 0x66E */
//    unsigned short tsf_gptall_stat;    /* 0x670 */
//    unsigned short PAD[0x07];  /* 0x672 - 0x67E */
//
//    /* IFS Block */
//    unsigned short ifs_sifs_rx_tx_tx;  /* 0x680 */
//    unsigned short ifs_sifs_nav_tx;    /* 0x682 */
//    unsigned short ifs_slot;   /* 0x684 */
//    unsigned short PAD;        /* 0x686 */
//    unsigned short ifs_ctl;        /* 0x688 */
//    unsigned short PAD[0x3];   /* 0x68a - 0x68F */
//    unsigned short ifsstat;        /* 0x690 */
//    unsigned short ifsmedbusyctl;  /* 0x692 */
//    unsigned short iftxdur;        /* 0x694 */
//    unsigned short PAD[0x3];   /* 0x696 - 0x69b */
//    /* EDCF support in dot11macs */
//    unsigned short ifs_aifsn;  /* 0x69c */
//    unsigned short ifs_ctl1;   /* 0x69e */
//
//    /* slow clock registers */
//    unsigned short scc_ctl;        /* 0x6a0 */
//    unsigned short scc_timer_l;    /* 0x6a2 */
//    unsigned short scc_timer_h;    /* 0x6a4 */
//    unsigned short scc_frac;   /* 0x6a6 */
//    unsigned short scc_fastpwrup_dly;  /* 0x6a8 */
//    unsigned short scc_per;        /* 0x6aa */
//    unsigned short scc_per_frac;   /* 0x6ac */
//    unsigned short scc_cal_timer_l;    /* 0x6ae */
//    unsigned short scc_cal_timer_h;    /* 0x6b0 */
//    unsigned short PAD;        /* 0x6b2 */
//
//    unsigned short PAD[0x26];
//
//    /* NAV Block */
//    unsigned short nav_ctl;        /* 0x700 */
//    unsigned short navstat;        /* 0x702 */
//    unsigned short PAD[0x3e];  /* 0x702 - 0x77E */
//
//    /* WEP/PMQ Block *//* 0x780 - 0x7FE */
//    unsigned short PAD[0x20];  /* 0x780 - 0x7BE */
//
//    unsigned short wepctl;     /* 0x7C0 */
//    unsigned short wepivloc;   /* 0x7C2 */
//    unsigned short wepivkey;   /* 0x7C4 */
//    unsigned short wepwkey;        /* 0x7C6 */
//
//    unsigned short PAD[4];     /* 0x7C8 - 0x7CE */
//    unsigned short pcmctl;     /* 0X7D0 */
//    unsigned short pcmstat;        /* 0X7D2 */
//    unsigned short PAD[6];     /* 0x7D4 - 0x7DE */
//
//    unsigned short pmqctl;     /* 0x7E0 */
//    unsigned short pmqstatus;  /* 0x7E2 */
//    unsigned short pmqpat0;        /* 0x7E4 */
//    unsigned short pmqpat1;        /* 0x7E6 */
//    unsigned short pmqpat2;        /* 0x7E8 */
//
//    unsigned short pmqdat;     /* 0x7EA */
//    unsigned short pmqdator;   /* 0x7EC */
//    unsigned short pmqhst;     /* 0x7EE */
//    unsigned short pmqpath0;   /* 0x7F0 */
//    unsigned short pmqpath1;   /* 0x7F2 */
//    unsigned short pmqpath2;   /* 0x7F4 */
//    unsigned short pmqdath;        /* 0x7F6 */
//
//    unsigned short PAD[0x04];  /* 0x7F8 - 0x7FE */
//
//    /* SHM *//* 0x800 - 0xEFE */
//    unsigned short PAD[0x380]; /* 0x800 - 0xEFE */
//} __attribute__((packed));

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
        uint16 length;
        uint16 checksum_coverage;
    } len_chk_cov;
    uint16 checksum;
} __attribute__((packed));

struct ethernet_ip_udp_header {
    struct ethernet_header ethernet;
    struct ip_header ip;
    struct udp_header udp;
} __attribute__((packed));

struct nexmon_header {
    uint32 hooked_fct;
    uint32 args[3];
    uint8 payload[1];
} __attribute__((packed));

typedef void (*to_fun_t)(void *arg);

typedef struct _ctimeout {
    struct _ctimeout *next;
    uint32 ms;
    to_fun_t fun;
    void *arg;
    bool expired;
} ctimeout_t;

struct hndrte_timer {
    uint32 *context;
    void *data;
    void (*mainfn)(struct hndrte_timer *);
    void (*auxfn)(void *context);
    ctimeout_t t;
    int interval;
    int set;
    int periodic;
    bool _freedone;
} __attribute__((packed));

#endif /*STRUCTS_COMMON_H */
