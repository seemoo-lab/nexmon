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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <linux/if_arp.h>
#include <linux/sockios.h>
#include <linux/wireless.h>
#include <monitormode.h>
#include <string.h>

#define WLC_GET_MONITOR                 107
#define WLC_SET_MONITOR                 108

#include <libnexio.h>

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

#define REAL_LIBC RTLD_NEXT

#ifdef __ANDROID__
typedef int request_t;
#else
typedef unsigned long request_t;
#endif

typedef void (*sighandler_t)(int);

static struct nexio *nexio = NULL;

static const char *ifname = "wlan0";

static void
_libfakeioctl_ensure_init(void)
{
    if (!nexio) {
        const char *env_if = getenv("NEXMON_IFNAME");
        if (env_if && env_if[0] != '\0')
            ifname = env_if;
        nexio = nex_init_ioctl(ifname);
    }
}

static void _libfakeioctl_init(void) __attribute__ ((constructor));
static void _libfakeioctl_init(void) {
    _libfakeioctl_ensure_init();
}

int ioctl (int fd, request_t request, ...){ 
    static int (*func_ioctl) (int, request_t, ...) = NULL;
    va_list args;
    void *argp;
    int ret;
    
    if (! func_ioctl)
        func_ioctl = (int (*) (int, request_t, ...)) dlsym (REAL_LIBC, "ioctl");
    va_start (args, request);
    argp = va_arg (args, void *);
    va_end (args);

    ret = func_ioctl(fd, request, argp);

    switch (request) {
        case SIOCGIFHWADDR:
            if (argp != NULL) {
                int buf = 0;
                struct ifreq* p_ifr = (struct ifreq *) argp;
                if (!strncmp(p_ifr->ifr_ifrn.ifrn_name, ifname, IFNAMSIZ)) {
                    _libfakeioctl_ensure_init();
                    if (nex_ioctl(nexio, WLC_GET_MONITOR, &buf, sizeof(buf), false) == 0) {
                        if (buf & MONITOR_IEEE80211)
                            p_ifr->ifr_hwaddr.sa_family = ARPHRD_IEEE80211;
                        else if (buf & MONITOR_RADIOTAP)
                            p_ifr->ifr_hwaddr.sa_family = ARPHRD_IEEE80211_RADIOTAP;
                        else if (buf == MONITOR_DISABLED || (buf & (MONITOR_LOG_ONLY | MONITOR_DROP_FRM | MONITOR_IPV4_UDP)))
                            p_ifr->ifr_hwaddr.sa_family = ARPHRD_ETHER;
                    }
                }
            }
            break;

        case SIOCGIWMODE:
            if (argp != NULL) {
                int buf = 0;
                struct iwreq* p_wrq = (struct iwreq*) argp;

                if (!strncmp(p_wrq->ifr_ifrn.ifrn_name, ifname, IFNAMSIZ)) {
                    _libfakeioctl_ensure_init();
                    if (nex_ioctl(nexio, WLC_GET_MONITOR, &buf, sizeof(buf), false) == 0) {
                        if (buf & (MONITOR_RADIOTAP | MONITOR_IEEE80211 | MONITOR_LOG_ONLY | MONITOR_DROP_FRM | MONITOR_IPV4_UDP)) {
                            p_wrq->u.mode = IW_MODE_MONITOR;
                        } else if (buf == MONITOR_DISABLED) {
                            p_wrq->u.mode = IW_MODE_INFRA;
                        }
                        ret = 0;
                    }
                }
            }
            break;

        case SIOCSIWMODE:
            if (argp != NULL) {
                int buf = 0;
                struct iwreq* p_wrq = (struct iwreq*) argp;

                if (!strncmp(p_wrq->ifr_ifrn.ifrn_name, ifname, IFNAMSIZ)) {
                    _libfakeioctl_ensure_init();
                    if (p_wrq->u.mode == IW_MODE_MONITOR) {
                        buf = MONITOR_RADIOTAP;
                    } else {
                        buf = MONITOR_DISABLED;
                    }

                    ret = nex_ioctl(nexio, WLC_SET_MONITOR, &buf, sizeof(buf), true);
                }
            }
            break;
    }
    return ret;
}
