#ifndef STRUCTS_COMMON_H
#define STRUCTS_COMMON_H

#include <types.h>

#ifndef PAD
#define _PADLINE(line)  pad ## line
#define _XSTR(line) _PADLINE(line)
#define PAD     _XSTR(__LINE__)
#endif

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
    uint16 vendor;                    /* 0x044 */
    uint16 device;                    /* 0x046 */
    uint32 corerev;                   /* 0x048 */
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
    void *sih;                        /* 0x07c */
    char *vars;                       /* 0x080 */
    uint32 vars_size;                 /* 0x084 */
    volatile struct d11regs *regs;    /* 0x088 */
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
} __attribute__((packed));

struct wlc_info {
    void *pub;                        /* 0x000 */
    void *osh;                        /* 0x004 */
    struct wl_info *wl;               /* 0x008 */
    volatile struct d11regs *regs;    /* 0x00c */
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
    unsigned short PAD;                 /* SPR_PSM_0x0c                     0x48C */
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
    unsigned short psm_pc_reg_0;        /* SPR_PSM_0x4e                     0x4CE */
    unsigned short psm_pc_reg_1;        /* SPR_PC0 - Link Register 0        0x4D0 */
    unsigned short psm_pc_reg_2;        /* SPR_PC1 - Link Register 1        0x4D2 */
    unsigned short psm_pc_reg_3;        /* SPR_PC2 - Link Register 2        0x4D4 */
    unsigned short PAD;                 /* SPR_PC2 - Link Register 6        0x4D6 */
    unsigned short PAD;                 /* SPR_PSM_COND - PSM external condition bits 0x4D8 */
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
