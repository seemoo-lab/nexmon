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

Copyright (c) 2015 NexMon Team

#include <stdarg.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/if_arp.h>
#include <linux/sockios.h>
#include <linux/wireless.h>

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

#define REAL_LIBC RTLD_NEXT

typedef int request_t;

typedef void (*sighandler_t)(int);

static int sa_family = 0;
static char *sa_family_str = NULL;

static void _libfakeioctl_init() __attribute__ ((constructor));
static void _libfakeioctl_init() {
    const char *env_var = getenv("NEXMON_SA_FAMILY");
    if (env_var != NULL) {
        if (!strcmp(env_var, "ARPHRD_IEEE80211")) {
            sa_family = ARPHRD_IEEE80211;
            sa_family_str = "ARPHRD_IEEE80211";
        } else {
            sa_family = ARPHRD_IEEE80211_RADIOTAP;
            sa_family_str = "ARPHRD_IEEE80211_RADIOTAP";
        }
    } else {
        sa_family = ARPHRD_IEEE80211_RADIOTAP;
        sa_family_str = "ARPHRD_IEEE80211_RADIOTAP";
    }

    printf("####################################################\n");
    printf("## nexmon ioctl hook active\n");
    printf("## sa_family = %s\n", sa_family_str);
    printf("## to change sa_family, set NEXMON_SA_FAMILY\n");
    printf("## environment variable to ARPHRD_IEEE80211\n");
    printf("####################################################\n");

}

int ioctl (int fd, request_t request, ...){ 
    static int (*func_ioctl) (int, request_t, void *) = NULL;
    va_list args;
    void *argp;
    int ret;
    struct ifreq* p_ifr;
    struct iwreq* p_wrq;

    if (! func_ioctl)
        func_ioctl = (int (*) (int, request_t, void *)) dlsym (REAL_LIBC, "ioctl");
    va_start (args, request);
    argp = va_arg (args, void *);
    va_end (args);

    ret = func_ioctl (fd, request, argp);

    switch (request) {
        case SIOCGIFHWADDR:
            p_ifr = (struct ifreq *) argp;
            p_ifr->ifr_hwaddr.sa_family = sa_family;
            break;
        case SIOCGIWMODE:
            p_wrq = (struct iwreq*) argp;
            p_wrq->u.mode = IW_MODE_MONITOR;
            break;
    }

    return ret;
}
