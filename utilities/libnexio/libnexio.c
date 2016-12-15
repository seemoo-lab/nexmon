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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <sys/ioctl.h>
#ifdef BUILD_ON_RPI
#include <linux/if.h>
#else
#include <net/if.h>
#endif
#include <stdbool.h>
#include <errno.h>

#define WLC_IOCTL_MAGIC          0x14e46c77
#define DHD_IOCTL_MAGIC          0x00444944

#define WLC_GET_MAGIC                     0
#define DHD_GET_MAGIC                     0

typedef struct nex_ioctl {
    unsigned int cmd;   /* common ioctl definition */
    void *buf;  /* pointer to user buffer */
    unsigned int len;   /* length of user buffer */
    bool set;   /* get or set request (optional) */
    unsigned int used;  /* bytes read or written (optional) */
    unsigned int needed;    /* bytes needed (optional) */
    unsigned int driver;    /* to identify target driver */
} nex_ioctl_t;

static int
__nex_driver_io(struct ifreq *ifr, nex_ioctl_t *ioc)
{
    int s;
    int ret = 0;

    /* pass ioctl data */
    ifr->ifr_data = (void *) ioc;

    /* open socket to kernel */
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        printf("error: socket\n");

    ret = ioctl(s, SIOCDEVPRIVATE, ifr);
    if (ret < 0 && errno != EAGAIN)
        printf("%s: error\n", __FUNCTION__);

    /* cleanup */
    close(s);
    return ret;
}

/* This function is called by ioctl_setinformation_fe or ioctl_queryinformation_fe
 * for executing  remote commands or local commands
 */
int
nex_ioctl(struct ifreq *ifr, int cmd, void *buf, int len, bool set)
{
    nex_ioctl_t ioc;
    int ret = 0;

    /* By default try to execute wl commands */
    int driver_magic = WLC_IOCTL_MAGIC;
    int get_magic = WLC_GET_MAGIC;

    // For local dhd commands execute dhd
    //driver_magic = DHD_IOCTL_MAGIC;
    //get_magic = DHD_GET_MAGIC;

    /* do it */
    ioc.cmd = cmd;
    ioc.buf = buf;
    ioc.len = len;
    ioc.set = set;
    ioc.driver = driver_magic;

    ret = __nex_driver_io(ifr, &ioc);
    if (ret < 0 && cmd != get_magic)
        ret = -1;
    return ret;
}
