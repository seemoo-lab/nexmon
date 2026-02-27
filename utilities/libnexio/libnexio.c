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

#ifdef USE_VENDOR_CMD
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/nl80211.h>
#include <byteswap.h>

#define BROADCOM_OUI 0x001018
#define BRCMF_VNDR_CMDS_DCMD 1
#endif

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
#define NEXIO_TYPE_VENDOR_CMD	3

struct nexio {
	int type;
	struct ifreq *ifr;
	int sock_rx_ioctl;
	int sock_rx_frame;
	int sock_tx;
	unsigned int securitycookie;
#ifdef USE_VENDOR_CMD
	struct nl_sock *nl_sock;
	int nl80211_id;
	struct nl_cb *nl_cb;
	int res;
#endif
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

#ifdef USE_VENDOR_CMD
struct brcmf_vndr_dcmd_hdr {
	unsigned int cmd;
	int len;
	unsigned int offset;
	unsigned int set;
	unsigned int magic;
} __attribute__((packed));
#endif

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

#ifdef USE_VENDOR_CMD
static int
__nex_driver_vendor_cmd_valid_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attr;
	struct genlmsghdr *gnlh;
	char *data;
	int len;
	struct nex_ioctl *ioc = (struct nex_ioctl *) arg;

	gnlh = nlmsg_data(nlmsg_hdr(msg));
	attr = nla_find(genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NL80211_ATTR_VENDOR_DATA);
	if (!attr) {
		printf("%s: no NL80211_ATTR_VENDOR_DATA found in response\n", __FUNCTION__);
		return NL_SKIP;
	}

	data = (char *) nla_data(attr) + sizeof(uint32_t);
	len = nla_len(attr) - 3 * sizeof(uint32_t);

	if (ioc->buf == NULL) {
		printf("%s: can not return data, no nex ioctl buffer\n", __FUNCTION__);
		return NL_SKIP;
	}

    len = (len > ioc->len) ? ioc->len : len;
	if (ioc->set == 0 && len > 0)
		memcpy(ioc->buf, data, len);

	return NL_OK;
}

static int
__nex_driver_finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int
__nex_driver_ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int
__nex_driver_vendor_cmd(struct nexio *nexio, struct nex_ioctl *ioc)
{
	int ret = 0;
	unsigned int devidx;
	char *dcmd_buf;
	struct brcmf_vndr_dcmd_hdr *dcmd_hdr;
	struct nl_msg *msg;

	devidx = if_nametoindex(nexio->ifr->ifr_name);
	if (!devidx) {
		printf("%s: error getting interface index\n", __FUNCTION__);
		return -ENODEV;
	}

	dcmd_buf = (char *) malloc(sizeof(struct brcmf_vndr_dcmd_hdr) + ioc->len);
	if (!dcmd_buf) {
		printf("%s: error allocating vendor dcommand buffer\n", __FUNCTION__);
		return -ENOMEM;
	}

	dcmd_hdr = (struct brcmf_vndr_dcmd_hdr *) dcmd_buf;
	dcmd_hdr->cmd = ioc->cmd;
	dcmd_hdr->len = ioc->len;
	dcmd_hdr->offset = sizeof(struct brcmf_vndr_dcmd_hdr);
	dcmd_hdr->set = ioc->set;
	dcmd_hdr->magic = ioc->driver;

	if (ioc->len > 0)
		memcpy(dcmd_buf + sizeof(struct brcmf_vndr_dcmd_hdr), ioc->buf, ioc->len);

	msg = nlmsg_alloc();
	if (!msg) {
		printf("%s: error allocating netlink message\n", __FUNCTION__);
		free(dcmd_buf);
		return -ENOMEM;
	}

	genlmsg_put(msg, 0, 0, nexio->nl80211_id, 0, 0, NL80211_CMD_VENDOR, 0);
	nla_put_u32(msg, NL80211_ATTR_IFINDEX, devidx);
	nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, BROADCOM_OUI);
	nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, BRCMF_VNDR_CMDS_DCMD);
	nla_put(msg, NL80211_ATTR_VENDOR_DATA, sizeof(struct brcmf_vndr_dcmd_hdr) + ioc->len, dcmd_buf);
	free(dcmd_buf);

	ret = nl_send_auto(nexio->nl_sock, msg);
	if (ret < 0) {
		printf("%s: error sending netlink message\n", __FUNCTION__);
		goto out;
	}

	nl_cb_set(nexio->nl_cb, NL_CB_VALID, NL_CB_CUSTOM, __nex_driver_vendor_cmd_valid_handler, ioc);

	nexio->res = 1;
	while (nexio->res > 0)
		nl_recvmsgs(nexio->nl_sock, nexio->nl_cb);

out:
	nl_cb_put(nexio->nl_cb);
	nlmsg_free(msg);
	return ret;
}
#endif

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
#ifdef USE_VENDOR_CMD
		case NEXIO_TYPE_VENDOR_CMD:
			ret = __nex_driver_vendor_cmd(nexio, &ioc);
			break;
#endif
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

#ifdef USE_VENDOR_CMD
struct nexio *
nex_init_vendor_cmd(const char *ifname)
{
	int err;

	struct nexio *nexio = (struct nexio *) malloc(sizeof(struct nexio));
	memset(nexio, 0, sizeof(struct nexio));

	nexio->ifr = (struct ifreq *) malloc(sizeof(struct ifreq));
	memset(nexio->ifr, 0, sizeof(struct ifreq));
	snprintf(nexio->ifr->ifr_name, sizeof(nexio->ifr->ifr_name), "%s", ifname);

	nexio->nl_sock = nl_socket_alloc();
	if (!nexio->nl_sock) {
		printf("%s: failed to allocate netlink socket, error %d\n", __FUNCTION__, -ENOMEM);
		free(nexio);
		return 0;
	}

	if (genl_connect(nexio->nl_sock)) {
		printf("%s: failed to connect to generic netlink, error %d\n", __FUNCTION__, -ENOLINK);
		nl_close(nexio->nl_sock);
		nl_socket_free(nexio->nl_sock);
		free(nexio);
		return 0;
	}

	nexio->nl80211_id = genl_ctrl_resolve(nexio->nl_sock, "nl80211");
	if (nexio->nl80211_id < 0) {
		printf("%s: nl80211 not found, error %d\n", __FUNCTION__, -ENOENT);
		nl_close(nexio->nl_sock);
		nl_socket_free(nexio->nl_sock);
		free(nexio);
		return 0;
	}

	nexio->nl_cb = nl_socket_get_cb(nexio->nl_sock);

	nl_cb_set(nexio->nl_cb, NL_CB_FINISH, NL_CB_CUSTOM, __nex_driver_finish_handler, &(nexio->res));
	nl_cb_set(nexio->nl_cb, NL_CB_ACK, NL_CB_CUSTOM, __nex_driver_ack_handler, &(nexio->res));

	nexio->type = NEXIO_TYPE_VENDOR_CMD;

	return nexio;
}
#endif
