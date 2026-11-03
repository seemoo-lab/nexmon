#ifndef NEXMON_IOCTLS_H
#define NEXMON_IOCTLS_H

/* see include/dhdioctl.h in bcmdhd driver */
typedef struct nex_ioctl {
    uint cmd;   /* common ioctl definition */
    void *buf;  /* pointer to user buffer */
    uint len;   /* length of user buffer */
    bool set;   /* get or set request (optional) */
    uint used;  /* bytes read or written (optional) */
    uint needed;    /* bytes needed (optional) */
    uint driver;    /* to identify target driver */
} nex_ioctl_t;

#define NEX_GET_CHANSPEC_OVERRIDE       418
#define NEX_SET_CHANSPEC_OVERRIDE       419
#define NEX_READ_D11_SHM                430
#define NEX_WRITE_D11_SHM               431

/* Direct D11 SHM / register access request structure (Linux 7.4 f6e39ae4ea84) */
struct nex_d11_shm_req {
    uint32_t offset;    /* Byte offset in D11 SHM or D11/backplane reg */
    uint32_t val;       /* Read or write value */
    uint16_t width;     /* Access width: 2 (16-bit word, default) or 4 (32-bit dword) */
    uint16_t is_reg;    /* 0 = D11 SHM (0xE8004000), 1 = D11 reg (0x18001000), 2 = raw backplane */
};

/* Standard D11 memory map definitions */
#ifndef D11_BASE_ADDR
#define D11_BASE_ADDR           0x18001000
#endif
#ifndef D11_AXI_BASE_ADDR
#define D11_AXI_BASE_ADDR       0xE8000000
#endif
#ifndef D11_SHM_BASE_ADDR
#define D11_SHM_BASE_ADDR       (D11_AXI_BASE_ADDR + 0x4000)
#endif

#ifndef D11REG_ADDR
#define D11REG_ADDR(offset)     (D11_BASE_ADDR + (offset))
#endif
#ifndef D11IHR_ADDR
#define D11IHR_ADDR(offset)     (D11_AXI_BASE_ADDR + 0x400 + (2 * (offset)))
#endif
#ifndef D11SHM_ADDR
#define D11SHM_ADDR(offset)     (D11_SHM_BASE_ADDR + (offset))
#endif

/* Broadcom D11 object memory indirection registers */
#ifndef OBJADDR_SHM_SEL
#define OBJADDR_SHM_SEL         0x00010000
#endif
#ifndef D11_OBJADDR_REG
#define D11_OBJADDR_REG         D11REG_ADDR(0x160)
#endif
#ifndef D11_OBJDATA_REG
#define D11_OBJDATA_REG         D11REG_ADDR(0x164)
#endif

#endif /* NEXMON_IOCTLS_H */
