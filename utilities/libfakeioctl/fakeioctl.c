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
 * Based on:                                                               *
 *                                                                         *
 * This code is based on the ldpreloadhook example by Pau Oliva Fora       *
 * <pofłeslack.org> and the idea of hooking ioctls to fake a monitor mode  *
 * interface, which was presented by Omri Ildis, Yuval Ofir and Ruby       *
 * Feinstein at recon2013.                                                 *
 *                                                                         *
 * Warning:                                                                *
 *                                                                         *
 * Our software may damage your hardware and may void your hardware’s      *
 * warranty! You use our tools at your own risk and responsibility!        *
 *                                                                         *
 * License:                                                                *
 * Copyright (c) 2015 NexMon Team                                          *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining   *
 * a copy of this software and associated documentation files (the         *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute copies of the Software, and to permit persons to whom the    *
 * Software is furnished to do so, subject to the following conditions:    *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * Any use of the Software which results in an academic publication or     *
 * other publication which includes a bibliography must include a citation *
 * to the author's publication "M. Schulz, D. Wegemer and M. Hollick.      *
 * NexMon: A Cookbook for Firmware Modifications on Smartphones to Enable  *
 * Monitor Mode.".                                                         *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 *                                                                         *
 **************************************************************************/

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