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
#include <sys/time.h>
#include <unistd.h>

#include <sys/ioctl.h>
#ifdef BUILD_ON_RPI
#include <linux/if.h>
#else
#include <net/if.h>
#endif
#include <stdbool.h>
#include <errno.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#include <linux/netlink.h>

#define WLC_IOCTL_MAGIC          0x14e46c77
#define DHD_IOCTL_MAGIC          0x00444944

#define WLC_GET_MAGIC                     0
#define DHD_GET_MAGIC                     0

#define NEXUDP_IOCTL            		  0
#define NEXUDP_INJECT_RADIOTAP  		  1

#define NETLINK_USER                     31

#define IPADDR(a,b,c,d) ((d) << 24 | (c) << 16 | (b) << 8 | (a))

#define NEXIO_TYPE_IOCTL		0
#define NEXIO_TYPE_UDP			1
#define NEXIO_TYPE_NETLINK		2

struct nexio {
	int type;
	struct ifreq *ifr;
	int sock_rx_ioctl;
	int sock_rx_frame;
	int sock_tx;
	unsigned int securitycookie;
};

struct nex_ioctl {
    unsigned int cmd;   	/* common ioctl definition */
    void *buf;  			/* pointer to user buffer */
    unsigned int len;   	/* length of user buffer */
    bool set;   			/* get or set request (optional) */
    unsigned int used;  	/* bytes read or written (optional) */
    unsigned int needed;    /* bytes needed (optional) */
    unsigned int driver;    /* to identify target driver */
};

struct nexudp_header {
    char nex[3];
    char type;
    int securitycookie;
} __attribute__((packed));

struct nexudp_ioctl_header {
    struct nexudp_header nexudphdr;
    unsigned int cmd;
    unsigned int set;
    char payload[1];
} __attribute__((packed));


static int
__nex_driver_io(struct ifreq *ifr, struct nex_ioctl *ioc)
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
        printf("%s: error ret=%d errno=%d\n", __FUNCTION__, ret, errno);

    /* cleanup */
    close(s);
    return ret;
}

static int
__nex_driver_udp(struct nexio *nexio, struct nex_ioctl *ioc)
{
	int frame_len = ioc->len + sizeof(struct nexudp_ioctl_header) - sizeof(char);
	int rx_frame_len = 0;
	struct nexudp_ioctl_header *frame = (struct nexudp_ioctl_header *) malloc(frame_len);
	int ret = 0;

	memcpy(&frame->nexudphdr.nex, "NEX", 3);
	frame->nexudphdr.type = NEXUDP_IOCTL;
	frame->nexudphdr.securitycookie = nexio->securitycookie;

	frame->cmd = ioc->cmd;
	frame->set = ioc->set;

	memcpy(frame->payload, ioc->buf, ioc->len);

	send(nexio->sock_tx, frame, frame_len, 0);

	rx_frame_len = recv(nexio->sock_rx_ioctl, frame, frame_len, 0);

	if (ioc->set == 0 && rx_frame_len > 0 && *(unsigned int *) frame == ioc->cmd) {
		memcpy(ioc->buf, ((char *) frame) + sizeof(frame->cmd) + sizeof(frame->set),
			(rx_frame_len - sizeof(frame->cmd) - sizeof(frame->set)) < ioc->len ?
            (rx_frame_len - sizeof(frame->cmd) - sizeof(frame->set)) : ioc->len);
	}

	free(frame);

	if (rx_frame_len < 0) {
		ret = -1;
		printf("ERR (%s): no valid answer received\n", __FUNCTION__);
	}

	return ret;
}

static int
__nex_driver_netlink(struct nexio *nexio, struct nex_ioctl *ioc)
{
        int frame_len = ioc->len + sizeof(struct nexudp_ioctl_header) - sizeof(char);
        int rx_frame_len = 0;
        struct nexudp_ioctl_header *frame;
        int ret = 0;

        struct iovec iov = { 0 };
        struct msghdr msg = { 0 };

        struct nlmsghdr *nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(frame_len));
        memset(nlh, 0, NLMSG_SPACE(frame_len));
        nlh->nlmsg_len = NLMSG_SPACE(frame_len);
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 0;
        frame = (struct nexudp_ioctl_header *) NLMSG_DATA(nlh);

        memcpy(&frame->nexudphdr.nex, "NEX", 3);
        frame->nexudphdr.type = NEXUDP_IOCTL;
        frame->nexudphdr.securitycookie = nexio->securitycookie;

        frame->cmd = ioc->cmd;
        frame->set = ioc->set;

        memcpy(frame->payload, ioc->buf, ioc->len);

//	printf("%s: before send. nlh->nlmsg_len: %d ioc->len: %d\n", __FUNCTION__, nlh->nlmsg_len, ioc->len);
        send(nexio->sock_tx, nlh, nlh->nlmsg_len, 0);

        rx_frame_len = recv(nexio->sock_rx_ioctl, nlh, nlh->nlmsg_len, 0);

//	printf("%s: framelen %d %d %s\n", __FUNCTION__, rx_frame_len, nlh->nlmsg_len, (char *) frame);

        if (ioc->set == 0 && rx_frame_len > 0 && frame->cmd == ioc->cmd) {
                memcpy(ioc->buf, frame->payload,
                        (rx_frame_len - sizeof(struct nexudp_ioctl_header) + sizeof(char)) < ioc->len ?
            (rx_frame_len - sizeof(struct nexudp_ioctl_header) + sizeof(char)) : ioc->len);
        }

        free(nlh);

        if (rx_frame_len < 0) {
                ret = -1;
                printf("ERR (%s): no valid answer received\n", __FUNCTION__);
        }

        return ret;
}

/* This function is called by ioctl_setinformation_fe or ioctl_queryinformation_fe
 * for executing  remote commands or local commands
 */
int
nex_ioctl(struct nexio *nexio, int cmd, void *buf, int len, bool set)
{
    struct nex_ioctl ioc;
    int ret = 0;

    /* do it */
    ioc.cmd = cmd;
    ioc.buf = buf;
    ioc.len = len;
    ioc.set = set;
    ioc.driver = WLC_IOCTL_MAGIC;


    if (nexio != 0) {
	switch(nexio->type) {
		case NEXIO_TYPE_IOCTL:
			ret = __nex_driver_io(nexio->ifr, &ioc);
			break;
		case NEXIO_TYPE_UDP:
			ret = __nex_driver_udp(nexio, &ioc);
			break;
		case NEXIO_TYPE_NETLINK:
			ret = __nex_driver_netlink(nexio, &ioc);
			break;
		default:
			printf("%s: not initialized correctly\n", __FUNCTION__);
	}
    } else {
        printf("%s: not initialized\n", __FUNCTION__);
    }

    if (ret < 0 && cmd != WLC_GET_MAGIC)
        ret = -1;

    return ret;
}

struct nexio *
nex_init_ioctl(const char *ifname)
{
	struct nexio *nexio = (struct nexio *) malloc(sizeof(struct nexio));
	memset(nexio, 0, sizeof(struct nexio));

	nexio->ifr = (struct ifreq *) malloc(sizeof(struct ifreq));
	memset(nexio->ifr, 0, sizeof(struct ifreq));
	snprintf(nexio->ifr->ifr_name, sizeof(nexio->ifr->ifr_name), "%s", ifname);

	nexio->type = NEXIO_TYPE_IOCTL;

	return nexio;
}

struct nexio *
nex_init_udp(unsigned int securitycookie, unsigned int txip)
{
	struct nexio *nexio = (struct nexio *) malloc(sizeof(struct nexio));
	memset(nexio, 0, sizeof(struct nexio));

	nexio->securitycookie = securitycookie;

	struct sockaddr_in *sin_tx = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
	struct sockaddr_in *sin_rx_ioctl = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
	struct sockaddr_in *sin_rx_frame = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));

	memset(sin_tx, 0, sizeof(struct sockaddr_in));
	memset(sin_rx_ioctl, 0, sizeof(struct sockaddr_in));
	memset(sin_rx_frame, 0, sizeof(struct sockaddr_in));

	sin_tx->sin_family = AF_INET;
	sin_tx->sin_port = htons(5500);
	sin_tx->sin_addr.s_addr = txip;

	sin_rx_ioctl->sin_family = AF_INET;
	sin_rx_ioctl->sin_port = htons(5500);
	sin_rx_ioctl->sin_addr.s_addr = IPADDR(255,255,255,255);

	sin_rx_frame->sin_family = AF_INET;
	sin_rx_frame->sin_port = htons(5600);
	sin_rx_frame->sin_addr.s_addr = IPADDR(255,255,255,255);

    nexio->sock_tx = socket(sin_tx->sin_family, SOCK_DGRAM, IPPROTO_UDP);
    nexio->sock_rx_ioctl = socket(sin_rx_ioctl->sin_family, SOCK_DGRAM, IPPROTO_UDP);
    nexio->sock_rx_frame = socket(sin_rx_frame->sin_family, SOCK_DGRAM, IPPROTO_UDP);

    // Enable broadcasts for transmit socket
    int broadcast_enable = 1;
    setsockopt(nexio->sock_tx, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

    // Set 1 second timeout on ioctl receive socket
    struct timeval tv = {
    	.tv_sec = 1,
    	.tv_usec = 0
    };
    setsockopt(nexio->sock_rx_ioctl, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    connect(nexio->sock_tx, (struct sockaddr *) sin_tx, sizeof(struct sockaddr));
    bind(nexio->sock_rx_ioctl, (struct sockaddr *) sin_rx_ioctl, sizeof(struct sockaddr));
    bind(nexio->sock_rx_frame, (struct sockaddr *) sin_rx_frame, sizeof(struct sockaddr));

	nexio->type = NEXIO_TYPE_UDP;

	return nexio;
}

struct nexio *
nex_init_netlink(void)
{
//    printf("%s: Enter\n", __FUNCTION__);
    int err = 0;
    struct nexio *nexio = (struct nexio *) malloc(sizeof(struct nexio));
    memset(nexio, 0, sizeof(struct nexio));

    struct sockaddr_nl *snl_tx = (struct sockaddr_nl *) malloc(sizeof(struct sockaddr_nl));
    struct sockaddr_nl *snl_rx_ioctl = (struct sockaddr_nl *) malloc(sizeof(struct sockaddr_nl));

    memset(snl_tx, 0, sizeof(struct sockaddr_nl));
    memset(snl_rx_ioctl, 0, sizeof(struct sockaddr_nl));

    snl_tx->nl_family = AF_NETLINK;
    snl_tx->nl_pid = 0; /* For Linux Kernel */
    snl_tx->nl_groups = 0; /* unicast */

    snl_rx_ioctl->nl_family = AF_NETLINK;
    snl_rx_ioctl->nl_pid = getpid();

    nexio->sock_tx = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (nexio->sock_tx < 0) printf("%s: socket error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
    nexio->sock_rx_ioctl = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (nexio->sock_rx_ioctl < 0) printf("%s: socket error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));

    // Set 1 second timeout on ioctl receive socket
    struct timeval tv = {
        .tv_sec = 1,
        .tv_usec = 0
    };
    setsockopt(nexio->sock_rx_ioctl, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

//    printf("%s: Before connect\n", __FUNCTION__);
    err = bind(nexio->sock_rx_ioctl, (struct sockaddr *) snl_rx_ioctl, sizeof(struct sockaddr));
    if (err) printf("%s: bind error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));

    err = connect(nexio->sock_tx, (struct sockaddr *) snl_tx, sizeof(struct sockaddr));
    if (err) printf("%s: connect error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));

//    printf("%s: Exit\n", __FUNCTION__);

    nexio->type = NEXIO_TYPE_NETLINK;

    return nexio;
}
