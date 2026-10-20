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

#endif /* NEXMON_IOCTLS_H */
