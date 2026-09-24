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
 * Based on:                                                               *
 *                                                                         *
 * This code is based on the ldpreloadhook example by Pau Oliva Fora       *
 * <pofłeslack.org> and the idea of hooking ioctls to fake a monitor mode  *
 * interface, which was presented by Omri Ildis, Yuval Ofir and Ruby       *
 * Feinstein at recon2013.                                                 *
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

#include <stdarg.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <linux/if_arp.h>
#include <linux/sockios.h>
#include <linux/wireless.h>
#include <monitormode.h>
#include <errno.h>
#include <net/if.h>
#include <nexioctls.h>
#include <string.h>

#define WLC_GET_MONITOR                 107
#define WLC_SET_MONITOR                 108

struct nexio {
    struct ifreq *ifr;
    int sock_rx_ioctl;
    int sock_rx_frame;
    int sock_tx;
};

extern int nex_ioctl(struct nexio *nexio, int cmd, void *buf, int len, bool set);
extern struct nexio *nex_init_ioctl(const char *ifname);

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

#define REAL_LIBC RTLD_NEXT

typedef int request_t;

typedef void (*sighandler_t)(int);

static struct nexio *nexio = NULL;

static const char *ifname = "wlan0";

static int (*func_sendto) (int, const void *, size_t, int, const struct sockaddr *, socklen_t) = NULL;
static int (*func_ioctl) (int, request_t, void *) = NULL;
static int (*func_socket) (int, int, int) = NULL;
static int (*func_bind) (int, const struct sockaddr *, int) = NULL;
static int (*func_write) (int, const void *, size_t) = NULL;

static void _libmexmon_init() __attribute__ ((constructor));
static void _libmexmon_init() {
    nexio = nex_init_ioctl(ifname);

    if (! func_ioctl)
        func_ioctl = (int (*) (int, request_t, void *)) dlsym (REAL_LIBC, "ioctl");

    if (! func_socket)
        func_socket = (int (*) (int, int, int)) dlsym (REAL_LIBC, "socket");

    if (! func_bind)
        func_bind = (int (*) (int, const struct sockaddr *, int)) dlsym (REAL_LIBC, "bind");

    if (! func_write)
        func_write = (int (*) (int, const void *, size_t)) dlsym (REAL_LIBC, "write");

    if (! func_sendto)
        func_sendto = (int (*) (int, const void *, size_t, int, const struct sockaddr *, socklen_t)) dlsym (REAL_LIBC, "sendto");
}

int
ioctl(int fd, request_t request, ...)
{
    va_list args;
    void *argp;
    int ret;
    
    va_start (args, request);
    argp = va_arg (args, void *);
    va_end (args);

    ret = func_ioctl(fd, request, argp);
    //if (ret < 0) {
    //    printf("LIBNEXMON: original response: %d, request: 0x%x\n", ret, request);
    //}

    switch (request) {
        case SIOCGIFHWADDR:
            {
                int buf;
                struct ifreq* p_ifr = (struct ifreq *) argp;
                if (!strncmp(p_ifr->ifr_ifrn.ifrn_name, ifname, strlen(ifname))) {
                    nex_ioctl(nexio, WLC_GET_MONITOR, &buf, 4, false);
                    
                    if (buf & MONITOR_IEEE80211) p_ifr->ifr_hwaddr.sa_family = ARPHRD_IEEE80211;
                    else if (buf & MONITOR_RADIOTAP) p_ifr->ifr_hwaddr.sa_family = ARPHRD_IEEE80211_RADIOTAP;
                    else if (buf & MONITOR_DISABLED || buf & MONITOR_LOG_ONLY || buf & MONITOR_DROP_FRM || buf & MONITOR_IPV4_UDP)
                        p_ifr->ifr_hwaddr.sa_family = ARPHRD_ETHER;

                    ret = 0;
                }
            }
            break;

        case SIOCGIWMODE:
            {
                int buf;
                struct iwreq* p_wrq = (struct iwreq*) argp;
                
                if (!strncmp(p_wrq->ifr_ifrn.ifrn_name, ifname, strlen(ifname))) {
                    nex_ioctl(nexio, WLC_GET_MONITOR, &buf, 4, false);

                    if (buf & MONITOR_RADIOTAP || buf & MONITOR_IEEE80211 || buf & MONITOR_LOG_ONLY || buf & MONITOR_DROP_FRM || buf & MONITOR_IPV4_UDP) {
                        p_wrq->u.mode = IW_MODE_MONITOR;
                    }

                    ret = 0;
                }
            }
            break;

        case SIOCSIWMODE:
            {
                int buf;
                struct iwreq* p_wrq = (struct iwreq*) argp;

                if (!strncmp(p_wrq->ifr_ifrn.ifrn_name, ifname, strlen(ifname))) {
                    if (p_wrq->u.mode == IW_MODE_MONITOR) {
                        buf = MONITOR_RADIOTAP;
                    } else {
                        buf = MONITOR_DISABLED;
                    }

                    ret = nex_ioctl(nexio, WLC_SET_MONITOR, &buf, 4, true);
                }
            }
            break;

        case SIOCSIWFREQ: // set channel/frequency (Hz)
            {
                //if (ret < 0)
                    //printf("LIBNEXMON: SIOCSIWFREQ not implemented\n");
            }
            break;

        case SIOCGIWFREQ: // get channel/frequency (Hz)
            {
                //if (ret < 0)
                    //printf("LIBNEXMON: SIOCGIWFREQ not implemented\n");
            }
            break;
    }
    return ret;
}

void
hexdump(const char *desc, const void *addr, int len)
{
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != 0)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

static char sock_types[][16] = { 
    "SOCK_STREAM", 
    "SOCK_DGRAM", 
    "SOCK_RAW", 
    "SOCK_RDM", 
    "SOCK_SEQPACKET",
};

static char domain_types[][16] = { 
    "AF_UNSPEC", 
    "AF_UNIX", 
    "AF_INET", 
    "AF_AX25", 
    "AF_IPX", 
    "AF_APPLETALK", 
    "AF_NETROM", 
    "AF_BRIDGE",
    "AF_ATMPVC",
    "AF_X25",
    "AF_INET6",
    "AF_ROSE",
    "AF_DECnet",
    "AF_NETBEUI",
    "AF_SECURITY",
    "AF_KEY",
    "AF_NETLINK",
    "AF_PACKET",
    "AF_ASH",
    "AF_ECONET",
    "AF_ATMSVC",
    "AF_RDS",
    "AF_SNA",
    "AF_IRDA",
    "AF_PPPOX",
    "AF_WANPIPE",
    "AF_LLC",
    "AF_IB",
    "AF_MPLS",
    "AF_CAN",
    "AF_TIPC",
    "AF_BLUETOOTH",
    "AF_IUCV",
    "AF_RXRPC",
    "AF_ISDN",
    "AF_PHONET",
    "AF_IEEE802154",
    "AF_CAIF",
    "AF_ALG",
    "AF_NFC",
    "AF_VSOCK",
    "AF_KCM",
    "AF_QIPCRTR",
    "AF_SMC"
};

int socket_to_type[100] = { 0 };
char bound_to_correct_if[100] = { 0 };

int
socket(int domain, int type, int protocol)
{
    int ret;

    ret = func_socket(domain, type, protocol);

    // save the socket type
    if (ret < sizeof(socket_to_type)/sizeof(socket_to_type[0]))
        socket_to_type[ret] = type;

    //if ((type - 1 < sizeof(sock_types)/sizeof(sock_types[0])) && (domain - 1 < sizeof(domain_types)/sizeof(domain_types[0])))
    //    printf("LIBNEXMON: %d = %s(%s(%d), %s(%d), %d)\n", ret, __FUNCTION__, domain_types[domain], domain, sock_types[type - 1], type, protocol);

    return ret;
}

int
bind(int sockfd, const struct sockaddr *addr, int addrlen)
{
    int ret;
    struct sockaddr_ll *sll = (struct sockaddr_ll *) addr;

    ret = func_bind(sockfd, addr, addrlen);

    char sll_ifname[IF_NAMESIZE] = { 0 };
    if_indextoname(sll->sll_ifindex, sll_ifname);

    if ((sockfd < sizeof(bound_to_correct_if)/sizeof(bound_to_correct_if[0])) && !strncmp(ifname, sll_ifname, sizeof(ifname)))
        bound_to_correct_if[sockfd] = 1;

    //printf("LIBNEXMON: %d = %s(%d, 0x%p, %d) sll_ifindex=%d ifname=%s\n", ret, __FUNCTION__, sockfd, addr, addrlen, sll->sll_ifindex, sll_ifname);

    return ret;    
}

struct inject_frame {
    unsigned short len;
    unsigned char pad;
    unsigned char type;
    char data[];
};

ssize_t
write(int fd, const void *buf, size_t count)
{
    ssize_t ret;

    // check if the user wants to write on a raw socket
    if ((fd > 2) && (fd < sizeof(socket_to_type)/sizeof(socket_to_type[0])) && (socket_to_type[fd] == SOCK_RAW) && (bound_to_correct_if[fd] == 1)) {
        struct inject_frame *buf_dup = (struct inject_frame *) malloc(count + sizeof(struct inject_frame));

        buf_dup->len = count + sizeof(struct inject_frame);
        buf_dup->pad = 0;
        buf_dup->type = 1;
        memcpy(buf_dup->data, buf, count);

        nex_ioctl(nexio, NEX_INJECT_FRAME, buf_dup, count + sizeof(struct inject_frame), true);

        free(buf_dup);

        ret = count;
    } else {
        // otherwise write the regular frame to the socket
        ret = func_write(fd, buf, count);
    }

    return ret;
}

ssize_t
sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)
{
    ssize_t ret;

    // check if the user wants to write on a raw socket
    if ((sockfd > 2) && (sockfd < sizeof(socket_to_type)/sizeof(socket_to_type[0])) && (socket_to_type[sockfd] == SOCK_RAW) && (bound_to_correct_if[sockfd] == 1)) {
        struct inject_frame *buf_dup = (struct inject_frame *) malloc(len + sizeof(struct inject_frame));

        buf_dup->len = len + sizeof(struct inject_frame);
        buf_dup->pad = 0;
        buf_dup->type = 1;
        memcpy(buf_dup->data, buf, len);

        nex_ioctl(nexio, NEX_INJECT_FRAME, buf_dup, len + sizeof(struct inject_frame), true);

        free(buf_dup);

        ret = len;
    } else {
        // otherwise write the regular frame to the socket
        ret = func_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    }

    return ret;
}
