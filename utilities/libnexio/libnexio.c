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
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <byteswap.h>

#include "libnexio.h"
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
#include <fcntl.h>

static int socket_cloexec(int domain, int type, int protocol) {
    int s = socket(domain, type | SOCK_CLOEXEC, protocol);
    if (s >= 0) return s;
    if (errno != EINVAL) return -1;
    s = socket(domain, type, protocol);
    if (s < 0) return -1;
    int fl = fcntl(s, F_GETFD);
    if (fl >= 0) fcntl(s, F_SETFD, fl | FD_CLOEXEC);
    return s;
}

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
	int sock_tx;
	unsigned int securitycookie;
	uint32_t nl_pid;
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

void nex_free(struct nexio *nexio);
/* internal alias used by the constructors' failure paths */
static void nexio_free(struct nexio *nexio) { nex_free(nexio); }

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

    /* open socket to kernel — close-on-exec to avoid fd leak on fork+exec */
    if ((s = socket_cloexec(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("error: socket\n");
        return -1;
    }

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
	int frame_len;
	int rx_frame_len = 0;
	struct nexudp_ioctl_header *frame;
	int ret = -1;

	if (ioc->len > 0x100000) {
		printf("ERR (%s): invalid ioctl length\n", __FUNCTION__);
		return -1;
	}

	frame_len = ioc->len + sizeof(struct nexudp_ioctl_header) - sizeof(char);
	frame = (struct nexudp_ioctl_header *) malloc(frame_len);
	if (!frame) {
		printf("ERR (%s): out of memory\n", __FUNCTION__);
		return -1;
	}

	memcpy(&frame->nexudphdr.nex, "NEX", 3);
	frame->nexudphdr.type = NEXUDP_IOCTL;
	frame->nexudphdr.securitycookie = nexio->securitycookie;

	frame->cmd = ioc->cmd;
	frame->set = ioc->set;

	if (ioc->len > 0 && ioc->buf)
		memcpy(frame->payload, ioc->buf, ioc->len);

	if (send(nexio->sock_tx, frame, frame_len, 0) < 0) {
		free(frame);
		printf("ERR (%s): send failed\n", __FUNCTION__);
		return -1;
	}

	rx_frame_len = recv(nexio->sock_rx_ioctl, frame, frame_len, 0);

	/* The reply is laid out as cmd(4) | set(4) | payload; require at least
	 * that 8-byte header before dereferencing / copying, and compute the
	 * payload length with signed arithmetic so a short reply cannot underflow
	 * into a huge unsigned copy count. */
	if (rx_frame_len < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			fprintf(stderr, "ERR (%s): timeout waiting for firmware reply (firmware wedged? SDIO -110?)\n", __FUNCTION__);
		} else {
			printf("ERR (%s): no valid answer received (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
		}
	} else if (ioc->set == 0) {
		if (rx_frame_len >= (int)(sizeof(frame->cmd) + sizeof(frame->set)) &&
		    *(unsigned int *) frame == (unsigned int) ioc->cmd) {
			int avail = rx_frame_len - (int)(sizeof(frame->cmd) + sizeof(frame->set));
			int copy = (avail < (int)ioc->len) ? avail : (int)ioc->len;
			if (copy > 0 && ioc->buf)
				memcpy(ioc->buf, ((char *) frame) + sizeof(frame->cmd) + sizeof(frame->set), copy);
			ret = 0;
		}
	} else {
		ret = 0;
	}

	free(frame);
	return ret;
}

static int
__nex_driver_netlink(struct nexio *nexio, struct nex_ioctl *ioc)
{
        int frame_len;
        int rx_frame_len = 0;
        struct nexudp_ioctl_header *frame;
        int ret = -1;

        struct nlmsghdr *nlh;

        if (ioc->len > 0x100000) {
                printf("ERR (%s): invalid ioctl length\n", __FUNCTION__);
                return -1;
        }

        frame_len = ioc->len + sizeof(struct nexudp_ioctl_header) - sizeof(char);
        nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(frame_len));
        if (!nlh) {
                printf("ERR (%s): out of memory\n", __FUNCTION__);
                return -1;
        }
        memset(nlh, 0, NLMSG_SPACE(frame_len));
        nlh->nlmsg_len = NLMSG_SPACE(frame_len);
        nlh->nlmsg_pid = nexio->nl_pid ? nexio->nl_pid : (uint32_t) getpid();
        nlh->nlmsg_flags = 0;
        frame = (struct nexudp_ioctl_header *) NLMSG_DATA(nlh);

        memcpy(&frame->nexudphdr.nex, "NEX", 3);
        frame->nexudphdr.type = NEXUDP_IOCTL;
        frame->nexudphdr.securitycookie = nexio->securitycookie;

        frame->cmd = ioc->cmd;
        frame->set = ioc->set;

        if (ioc->len > 0 && ioc->buf)
                memcpy(frame->payload, ioc->buf, ioc->len);

        if (send(nexio->sock_tx, nlh, nlh->nlmsg_len, 0) < 0) {
                free(nlh);
                printf("ERR (%s): send failed\n", __FUNCTION__);
                return -1;
        }

        rx_frame_len = recv(nexio->sock_rx_ioctl, nlh, NLMSG_SPACE(frame_len), 0);

        if (rx_frame_len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        fprintf(stderr, "ERR (%s): timeout waiting for netlink reply (firmware wedged?)\n", __FUNCTION__);
                } else {
                        printf("ERR (%s): no valid answer received (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
                }
        } else if (rx_frame_len >= (int)NLMSG_HDRLEN && nlh->nlmsg_type != NLMSG_ERROR) {
                int netlink_data_len = rx_frame_len - (int)NLMSG_HDRLEN;
                int ioctl_hdr_len = (int)(sizeof(struct nexudp_ioctl_header) - sizeof(char));

                if (ioc->set == 0) {
                        if (netlink_data_len >= ioctl_hdr_len &&
                            frame->cmd == (unsigned int)ioc->cmd) {
                                int avail = netlink_data_len - ioctl_hdr_len;
                                int copy = (avail < (int)ioc->len) ? avail : (int)ioc->len;
                                if (copy > 0 && ioc->buf)
                                        memcpy(ioc->buf, frame->payload, copy);
                                ret = 0;
                        }
                } else {
                        if (netlink_data_len >= 3 && memcmp(NLMSG_DATA(nlh), "ACK", 3) == 0)
                                ret = 0;
                }
        }

        free(nlh);
        return ret;
}

#ifdef USE_VENDOR_CMD
static int
__nex_driver_vendor_cmd_valid_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *attr;
	struct genlmsghdr *gnlh;
	char *data;
	int attr_len;
	size_t avail;
	size_t copy;
	struct nex_ioctl *ioc = (struct nex_ioctl *) arg;

	gnlh = nlmsg_data(nlmsg_hdr(msg));
	attr = nla_find(genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NL80211_ATTR_VENDOR_DATA);
	if (!attr) {
		printf("%s: no NL80211_ATTR_VENDOR_DATA found in response\n", __FUNCTION__);
		return NL_SKIP;
	}

	/* The vendor payload sits one u32 into the attribute and is three u32s
	 * shorter than it. Reject anything smaller before doing that arithmetic:
	 * "nla_len(attr) - 3 * sizeof(uint32_t)" mixes int with size_t, so a
	 * short attribute wrapped to a huge unsigned value and then to a negative
	 * int, and the clamp below (signed len against unsigned ioc->len) picked
	 * ioc->len -- making memcpy() read past the end of the attribute. */
	attr_len = nla_len(attr);
	if (attr_len < (int) (3 * sizeof(uint32_t))) {
		printf("%s: vendor data too short (%d bytes)\n", __FUNCTION__, attr_len);
		return NL_SKIP;
	}

	data = (char *) nla_data(attr) + sizeof(uint32_t);
	avail = (size_t) attr_len - 3 * sizeof(uint32_t);

	if (ioc->buf == NULL) {
		printf("%s: can not return data, no nex ioctl buffer\n", __FUNCTION__);
		return NL_SKIP;
	}

	copy = (avail < (size_t) ioc->len) ? avail : (size_t) ioc->len;
	if (ioc->set == 0 && copy > 0)
		memcpy(ioc->buf, data, copy);

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
__nex_driver_error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	(void) nla;
	int *ret = arg;
	*ret = (err && err->error < 0) ? err->error : -EIO;
	return NL_STOP;
}

static int
__nex_driver_vendor_cmd(struct nexio *nexio, struct nex_ioctl *ioc)
{
	int ret = 0;
	unsigned int devidx;
	size_t vndr_len;
	char *dcmd_buf;
	struct brcmf_vndr_dcmd_hdr *dcmd_hdr;
	struct nl_msg *msg;

	if (ioc->len > 0x100000 || (ioc->len > 0 && ioc->buf == NULL)) {
		printf("%s: invalid ioctl length or NULL buffer\n", __FUNCTION__);
		return -EINVAL;
	}

	devidx = if_nametoindex(nexio->ifr->ifr_name);
	if (!devidx) {
		printf("%s: error getting interface index\n", __FUNCTION__);
		return -ENODEV;
	}

	vndr_len = sizeof(struct brcmf_vndr_dcmd_hdr) + (size_t) ioc->len;
	dcmd_buf = (char *) malloc(vndr_len);
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

	if (ioc->len > 0 && ioc->buf)
		memcpy(dcmd_buf + sizeof(struct brcmf_vndr_dcmd_hdr), ioc->buf, ioc->len);

	msg = nlmsg_alloc_size(NLMSG_SPACE(vndr_len + 128));
	if (!msg) {
		printf("%s: error allocating netlink message\n", __FUNCTION__);
		free(dcmd_buf);
		return -ENOMEM;
	}

	if (!genlmsg_put(msg, 0, 0, nexio->nl80211_id, 0, 0, NL80211_CMD_VENDOR, 0) ||
	    nla_put_u32(msg, NL80211_ATTR_IFINDEX, devidx) < 0 ||
	    nla_put_u32(msg, NL80211_ATTR_VENDOR_ID, BROADCOM_OUI) < 0 ||
	    nla_put_u32(msg, NL80211_ATTR_VENDOR_SUBCMD, BRCMF_VNDR_CMDS_DCMD) < 0 ||
	    nla_put(msg, NL80211_ATTR_VENDOR_DATA, (int) vndr_len, dcmd_buf) < 0) {
		printf("%s: error building netlink attributes\n", __FUNCTION__);
		free(dcmd_buf);
		ret = -EMSGSIZE;
		goto out;
	}
	free(dcmd_buf);

	ret = nl_send_auto(nexio->nl_sock, msg);
	if (ret < 0) {
		printf("%s: error sending netlink message\n", __FUNCTION__);
		goto out;
	}

	nl_cb_set(nexio->nl_cb, NL_CB_VALID, NL_CB_CUSTOM, __nex_driver_vendor_cmd_valid_handler, ioc);

	nexio->res = 1;
	while (nexio->res > 0) {
		int r = nl_recvmsgs(nexio->nl_sock, nexio->nl_cb);
		if (r < 0) {
			ret = r;
			goto out;
		}
	}
	ret = (nexio->res < 0) ? nexio->res : 0;

out:
	/* nl_cb is owned by the handle (obtained once via nl_socket_get_cb in
	 * nex_init_vendor_cmd) and released in nex_free(); do not put it here or
	 * the refcount underflows across repeated nex_ioctl() calls. */
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
    struct nex_ioctl ioc = { 0 };
    int ret = -1;

    if (len < 0 || len > 0x100000 || (len > 0 && buf == NULL)) {
        printf("%s: invalid length %d or NULL buffer\n", __FUNCTION__, len);
        return -1;
    }

    /* do it */
    ioc.cmd = cmd;
    ioc.buf = buf;
    ioc.len = len;
    ioc.set = set;
    ioc.driver = WLC_IOCTL_MAGIC;

    if (nexio != NULL) {
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
			ret = -1;
			break;
	}
    } else {
        printf("%s: not initialized\n", __FUNCTION__);
        ret = -1;
    }

    if (ret < 0 && cmd != WLC_GET_MAGIC)
        ret = -1;

    return ret;
}

int
nex_ioctl_nonblock(struct nexio *nexio, int cmd, void *buf, int len)
{
    struct nex_ioctl ioc = { 0 };

    if (len < 0 || len > 0x100000 || (len > 0 && buf == NULL)) {
        printf("%s: invalid length %d or NULL buffer\n", __FUNCTION__, len);
        return -1;
    }

    ioc.cmd = cmd;
    ioc.buf = buf;
    ioc.len = len;
    ioc.set = true;
    ioc.driver = WLC_IOCTL_MAGIC;

    if (nexio != NULL) {
        switch(nexio->type) {
            case NEXIO_TYPE_NETLINK: {
                int frame_len = ioc.len + sizeof(struct nexudp_ioctl_header) - sizeof(char);
                struct nlmsghdr *nlh = (struct nlmsghdr *) malloc(NLMSG_SPACE(frame_len));
                if (!nlh) return -1;
                memset(nlh, 0, NLMSG_SPACE(frame_len));
                nlh->nlmsg_len = NLMSG_SPACE(frame_len);
                nlh->nlmsg_pid = nexio->nl_pid ? nexio->nl_pid : (uint32_t) getpid();
                nlh->nlmsg_flags = 0;
                struct nexudp_ioctl_header *frame = (struct nexudp_ioctl_header *) NLMSG_DATA(nlh);

                memcpy(&frame->nexudphdr.nex, "NEX", 3);
                frame->nexudphdr.type = NEXUDP_IOCTL;
                frame->nexudphdr.securitycookie = nexio->securitycookie;
                frame->cmd = ioc.cmd;
                frame->set = ioc.set;

                if (ioc.len > 0 && ioc.buf)
                    memcpy(frame->payload, ioc.buf, ioc.len);

                int ret = send(nexio->sock_tx, nlh, nlh->nlmsg_len, 0);
                free(nlh);
                return (ret < 0) ? -1 : 0;
            }
            default:
                return nex_ioctl(nexio, cmd, buf, len, true);
        }
    }
    return -1;
}

/* Release everything a nex_init_* constructor allocated: sockets, the ifreq,
 * and (for the vendor-cmd path) the netlink socket/callback. Safe on a
 * partially-initialised handle and on NULL. */
void
nex_free(struct nexio *nexio)
{
	if (!nexio)
		return;

	/* sock_tx may alias sock_rx_ioctl (netlink path); close each fd once. */
	if (nexio->sock_tx >= 0) {
		close(nexio->sock_tx);
		if (nexio->sock_rx_ioctl == nexio->sock_tx)
			nexio->sock_rx_ioctl = -1;
		nexio->sock_tx = -1;
	}
	if (nexio->sock_rx_ioctl >= 0) {
		close(nexio->sock_rx_ioctl);
		nexio->sock_rx_ioctl = -1;
	}

	if (nexio->ifr) {
		free(nexio->ifr);
		nexio->ifr = NULL;
	}

#ifdef USE_VENDOR_CMD
	if (nexio->nl_cb) {
		nl_cb_put(nexio->nl_cb);
		nexio->nl_cb = NULL;
	}
	if (nexio->nl_sock) {
		nl_close(nexio->nl_sock);
		nl_socket_free(nexio->nl_sock);
		nexio->nl_sock = NULL;
	}
#endif

	free(nexio);
}

struct nexio *
nex_init_ioctl(const char *ifname)
{
	struct nexio *nexio = (struct nexio *) malloc(sizeof(struct nexio));
	if (!nexio)
		return NULL;
	memset(nexio, 0, sizeof(struct nexio));

	nexio->ifr = (struct ifreq *) malloc(sizeof(struct ifreq));
	if (!nexio->ifr) {
		free(nexio);
		return NULL;
	}
	memset(nexio->ifr, 0, sizeof(struct ifreq));
	snprintf(nexio->ifr->ifr_name, sizeof(nexio->ifr->ifr_name), "%s", ifname);

	nexio->type = NEXIO_TYPE_IOCTL;
	nexio->sock_rx_ioctl = -1;
	nexio->sock_tx = -1;

	return nexio;
}

struct nexio *
nex_init_udp(unsigned int securitycookie, unsigned int txip)
{
	struct sockaddr_in sin_tx = { 0 };
	struct sockaddr_in sin_rx_ioctl = { 0 };
	int broadcast_enable = 1;
	int reuse_enable = 1;
	struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };

	struct nexio *nexio = (struct nexio *) malloc(sizeof(struct nexio));
	if (!nexio)
		return NULL;
	memset(nexio, 0, sizeof(struct nexio));
	nexio->sock_tx = nexio->sock_rx_ioctl = -1;

	nexio->securitycookie = securitycookie;

	/* Binding to 255.255.255.255 broadcast may require CAP_NET_RAW; warn if not root */
	if (geteuid() != 0) {
		fprintf(stderr, "WARN (%s): not running as root, UDP broadcast bind may fail without CAP_NET_RAW\n", __FUNCTION__);
	}

	sin_tx.sin_family = AF_INET;
	sin_tx.sin_port = htons(5500);
	sin_tx.sin_addr.s_addr = txip;

	sin_rx_ioctl.sin_family = AF_INET;
	sin_rx_ioctl.sin_port = htons(5500);
	sin_rx_ioctl.sin_addr.s_addr = IPADDR(255,255,255,255);

	nexio->sock_tx = socket_cloexec(sin_tx.sin_family, SOCK_DGRAM, IPPROTO_UDP);
	nexio->sock_rx_ioctl = socket_cloexec(sin_rx_ioctl.sin_family, SOCK_DGRAM, IPPROTO_UDP);

	if (nexio->sock_tx < 0 || nexio->sock_rx_ioctl < 0) {
		printf("%s: socket error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
		goto fail;
	}

	// Enable broadcasts for transmit socket
	setsockopt(nexio->sock_tx, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

	// Allow rapid reuse and set 1 second timeout on ioctl receive socket
	setsockopt(nexio->sock_rx_ioctl, SOL_SOCKET, SO_REUSEADDR, &reuse_enable, sizeof(reuse_enable));
	setsockopt(nexio->sock_rx_ioctl, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	if (connect(nexio->sock_tx, (struct sockaddr *) &sin_tx, sizeof(sin_tx)) < 0 ||
	    bind(nexio->sock_rx_ioctl, (struct sockaddr *) &sin_rx_ioctl, sizeof(sin_rx_ioctl)) < 0) {
		printf("%s: connect/bind error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
		goto fail;
	}

	nexio->type = NEXIO_TYPE_UDP;

	return nexio;

fail:
	nexio_free(nexio);
	return NULL;
}

struct nexio *
nex_init_netlink_ifname(const char *ifname)
{
//    printf("%s: Enter\n", __FUNCTION__);
    int err = 0;
    struct sockaddr_nl snl_tx = { 0 };
    struct sockaddr_nl snl_rx_ioctl = { 0 };
    socklen_t addrlen = sizeof(snl_rx_ioctl);
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };

    struct nexio *nexio = (struct nexio *) malloc(sizeof(struct nexio));
    if (!nexio)
        return NULL;
    memset(nexio, 0, sizeof(struct nexio));
    nexio->sock_tx = nexio->sock_rx_ioctl = -1;

    if (ifname && ifname[0]) {
        struct ifreq ifr = { 0 };
        int fd = socket_cloexec(AF_INET, SOCK_DGRAM, 0);

        if (fd < 0) {
            fprintf(stderr, "%s: socket error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
            nexio_free(nexio);
            return NULL;
        }

        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
        err = ioctl(fd, SIOCGIFINDEX, &ifr);
        close(fd);

        if (err < 0) {
            fprintf(stderr, "%s: could not resolve interface '%s' (%d: %s)\n",
                    __FUNCTION__, ifname, errno, strerror(errno));
            nexio_free(nexio);
            return NULL;
        }

        nexio->securitycookie = ifr.ifr_ifindex;
    }

    snl_tx.nl_family = AF_NETLINK;
    snl_tx.nl_pid = 0; /* For Linux Kernel */
    snl_tx.nl_groups = 0; /* unicast */

    snl_rx_ioctl.nl_family = AF_NETLINK;
    snl_rx_ioctl.nl_pid = 0; /* Let kernel assign a unique port ID per handle */

    nexio->sock_rx_ioctl = socket_cloexec(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if (nexio->sock_rx_ioctl < 0) {
        printf("%s: socket error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
        nexio_free(nexio);
        return NULL;
    }

    // Set 1 second timeout on ioctl receive socket
    setsockopt(nexio->sock_rx_ioctl, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

//    printf("%s: Before connect\n", __FUNCTION__);
    err = bind(nexio->sock_rx_ioctl, (struct sockaddr *) &snl_rx_ioctl, sizeof(snl_rx_ioctl));
    if (err) {
        printf("%s: bind error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
        nexio_free(nexio);
        return NULL;
    }

    if (getsockname(nexio->sock_rx_ioctl, (struct sockaddr *) &snl_rx_ioctl, &addrlen) == 0)
        nexio->nl_pid = snl_rx_ioctl.nl_pid;

    err = connect(nexio->sock_rx_ioctl, (struct sockaddr *) &snl_tx, sizeof(snl_tx));
    if (err) {
        printf("%s: connect error (%d: %s)\n", __FUNCTION__, errno, strerror(errno));
        nexio_free(nexio);
        return NULL;
    }

    /* Send and receive on the same socket.  The kernel can now reply to the
     * authenticated sender port ID rather than trusting nlmsg_pid. */
    nexio->sock_tx = nexio->sock_rx_ioctl;

//    printf("%s: Exit\n", __FUNCTION__);

    nexio->type = NEXIO_TYPE_NETLINK;

    return nexio;
}

struct nexio *
nex_init_netlink(void)
{
    return nex_init_netlink_ifname(NULL);
}

#ifdef USE_VENDOR_CMD
struct nexio *
nex_init_vendor_cmd(const char *ifname)
{
	if (!ifname)
		return NULL;

	struct nexio *nexio = (struct nexio *) malloc(sizeof(struct nexio));
	if (!nexio)
		return NULL;
	memset(nexio, 0, sizeof(struct nexio));
	nexio->sock_tx = nexio->sock_rx_ioctl = -1;

	nexio->ifr = (struct ifreq *) malloc(sizeof(struct ifreq));
	if (!nexio->ifr) {
		free(nexio);
		return NULL;
	}
	memset(nexio->ifr, 0, sizeof(struct ifreq));
	snprintf(nexio->ifr->ifr_name, sizeof(nexio->ifr->ifr_name), "%s", ifname);

	nexio->nl_sock = nl_socket_alloc();
	if (!nexio->nl_sock) {
		printf("%s: failed to allocate netlink socket, error %d\n", __FUNCTION__, -ENOMEM);
		nex_free(nexio);
		return 0;
	}

	if (genl_connect(nexio->nl_sock)) {
		printf("%s: failed to connect to generic netlink, error %d\n", __FUNCTION__, -ENOLINK);
		nex_free(nexio);
		return 0;
	}

	nexio->nl80211_id = genl_ctrl_resolve(nexio->nl_sock, "nl80211");
	if (nexio->nl80211_id < 0) {
		printf("%s: nl80211 not found, error %d\n", __FUNCTION__, -ENOENT);
		nex_free(nexio);
		return 0;
	}

	nexio->nl_cb = nl_socket_get_cb(nexio->nl_sock);
	if (!nexio->nl_cb) {
		nex_free(nexio);
		return 0;
	}

	nl_cb_err(nexio->nl_cb, NL_CB_CUSTOM, __nex_driver_error_handler, &(nexio->res));
	nl_cb_set(nexio->nl_cb, NL_CB_FINISH, NL_CB_CUSTOM, __nex_driver_finish_handler, &(nexio->res));
	nl_cb_set(nexio->nl_cb, NL_CB_ACK, NL_CB_CUSTOM, __nex_driver_ack_handler, &(nexio->res));

	nexio->type = NEXIO_TYPE_VENDOR_CMD;

	return nexio;
}
#endif

/* =========================================================================
 * Chanspec Encoding and Decoding Routines (Weak implementations for libnexio)
 * ========================================================================= */

#ifndef WL_CHANSPEC_BAND_MASK
#define WL_CHANSPEC_BAND_MASK       0xc000u
#define WL_CHANSPEC_BAND_SHIFT      14u
#define WL_CHANSPEC_BAND_2G         0x0000u
#define WL_CHANSPEC_BAND_6G         0x4000u
#define WL_CHANSPEC_BAND_4G         0x8000u
#define WL_CHANSPEC_BAND_5G         0xc000u
#endif

#ifndef WL_CHANSPEC_BW_MASK
#define WL_CHANSPEC_BW_MASK         0x3800u
#define WL_CHANSPEC_BW_SHIFT        11u
#define WL_CHANSPEC_BW_20           0x1000u
#define WL_CHANSPEC_BW_40           0x1800u
#define WL_CHANSPEC_BW_80           0x2000u
#define WL_CHANSPEC_BW_160          0x2800u
#define WL_CHANSPEC_BW_320          0x3000u
#endif

#ifndef WL_CHANSPEC_CTL_SB_MASK
#define WL_CHANSPEC_CTL_SB_MASK     0x0700u
#define WL_CHANSPEC_CTL_SB_SHIFT    8u
#define WL_CHANSPEC_CTL_SB_LLL      0x0000u
#define WL_CHANSPEC_CTL_SB_LLU      0x0100u
#endif

#ifndef WL_CHANSPEC_GE320_SB_MASK
#define WL_CHANSPEC_GE320_SB_MASK   0x07C0u
#define WL_CHANSPEC_GE320_SB_SHIFT  6u
#define WL_CHANSPEC_GE320_CHIDX_MASK 0x003fu
#define WL_CHANSPEC_GE320_CHIDX_SHIFT 0u
#endif

#ifndef WL_CHANSPEC_CHAN_MASK
#define WL_CHANSPEC_CHAN_MASK       0x00ffu
#endif

#ifndef INVCHANSPEC
#define INVCHANSPEC                 255u
#endif

#ifndef INVCHANNEL
#define INVCHANNEL                  255u
#endif

#ifndef CH_MIN_2G_CHANNEL
#define CH_MIN_2G_CHANNEL           1u
#define CH_MAX_2G_CHANNEL           14u
#define CH_MIN_5G_CHANNEL           36u
#define CH_MAX_5G_CHANNEL           181u
#define CH_MIN_6G_CHANNEL           1u
#define CH_MAX_6G_CHANNEL           233u
#define CH_MIN_6G_40M_CHANNEL       3u
#define CH_MAX_6G_40M_CHANNEL       227u
#define CH_MIN_6G_80M_CHANNEL       7u
#define CH_MAX_6G_80M_CHANNEL       215u
#define CH_MIN_6G_160M_CHANNEL      15u
#define CH_MAX_6G_160M_CHANNEL      207u
#define CH_MIN_6G_320M_CHANNEL      31u
#define CH_MAX_6G_320M_CHANNEL      199u
#define CH_160MHZ_APART             32u
#define CH_80MHZ_APART              16u
#define CH_40MHZ_APART              8u
#define CH_20MHZ_APART              4u
#define CH_10MHZ_APART              2u
#endif

#ifndef WF_IS_6G_PSC_CHAN
#define WF_IS_6G_PSC_CHAN(channel)  (((channel) % 16u) == 5u)
#endif

/* Must stay in sync with wf_5g_40m_chans / wf_5g_80m_chans in
 * utilities/nexutil/bcmwifi_channels.c. These weak fallbacks exist for tools
 * that link libnexio without the full channel library, so a shorter table here
 * means the same chanspec string parses differently depending on what a tool
 * linked against - and silently rejects valid upper-band channels. */
static const unsigned char libnexio_wf_5g_40m_chans[] = {
	38, 46, 54, 62, 102, 110, 118, 126, 134, 142, 151, 159, 167, 175
};
static const unsigned char libnexio_wf_5g_80m_chans[] = {
	42, 58, 106, 122, 138, 155, 171
};
static const unsigned char libnexio_wf_5g_160m_chans[] = {
	50, 114, 163
};
static const unsigned char libnexio_wf_6g_320m_chans[] = {
	31, 63, 95, 127, 159, 191
};

static unsigned int
libnexio_center_chan_to_edge(unsigned int bw)
{
	if (bw == WL_CHANSPEC_BW_40)
		return 2;
	if (bw == WL_CHANSPEC_BW_80)
		return 6;
	if (bw == WL_CHANSPEC_BW_160)
		return 14;
	if (bw == WL_CHANSPEC_BW_320)
		return 30;
	return 0;
}

static int
libnexio_channel_to_sb(unsigned int center_ch, unsigned int primary_ch, unsigned int bw)
{
	unsigned int delta = libnexio_center_chan_to_edge(bw);
	unsigned int lowest;
	unsigned int sb;

	if (center_ch <= delta)
		return -1;
	lowest = center_ch - delta;

	if (primary_ch < lowest || (primary_ch - lowest) % CH_20MHZ_APART)
		return -1;

	sb = (primary_ch - lowest) / CH_20MHZ_APART;
	if ((bw == WL_CHANSPEC_BW_20 && sb >= 1) ||
	    (bw == WL_CHANSPEC_BW_40 && sb >= 2) ||
	    (bw == WL_CHANSPEC_BW_80 && sb >= 4) ||
	    (bw == WL_CHANSPEC_BW_160 && sb >= 8) ||
	    (bw == WL_CHANSPEC_BW_320 && sb >= 16))
		return -1;

	return (int)sb;
}

static bool
libnexio_wf_valid_20MHz_chan(unsigned int channel, unsigned int band)
{
	if (band == WL_CHANSPEC_BAND_2G) {
		return (channel >= CH_MIN_2G_CHANNEL && channel <= CH_MAX_2G_CHANNEL);
	} else if (band == WL_CHANSPEC_BAND_5G) {
		size_t num = sizeof(libnexio_wf_5g_40m_chans) / sizeof(libnexio_wf_5g_40m_chans[0]);
		for (size_t i = 0; i < num; i++) {
			if (channel == (unsigned int)(libnexio_wf_5g_40m_chans[i] - 2) ||
			    channel == (unsigned int)(libnexio_wf_5g_40m_chans[i] + 2))
				return true;
		}
		if (channel == 173 || channel == 34 || channel == 38 || channel == 42 || channel == 46)
			return true;
	} else if (band == WL_CHANSPEC_BAND_6G) {
		if (channel >= CH_MIN_6G_CHANNEL && channel <= CH_MAX_6G_CHANNEL &&
		    (((channel - CH_MIN_6G_CHANNEL) % CH_20MHZ_APART) == 0 || channel == 2))
			return true;
	}
	return false;
}

static bool
libnexio_wf_valid_40MHz_center_chan(unsigned int center, unsigned int band)
{
	if (band == WL_CHANSPEC_BAND_2G) {
		return (center >= 3 && center <= 11);
	} else if (band == WL_CHANSPEC_BAND_5G) {
		size_t num = sizeof(libnexio_wf_5g_40m_chans) / sizeof(libnexio_wf_5g_40m_chans[0]);
		for (size_t i = 0; i < num; i++) {
			if (center == libnexio_wf_5g_40m_chans[i])
				return true;
		}
	} else if (band == WL_CHANSPEC_BAND_6G) {
		if (center >= CH_MIN_6G_40M_CHANNEL && center <= CH_MAX_6G_40M_CHANNEL &&
		    ((center - CH_MIN_6G_40M_CHANNEL) % 8) == 0)
			return true;
	}
	return false;
}

static bool
libnexio_wf_valid_80MHz_center_chan(unsigned int center, unsigned int band)
{
	if (band == WL_CHANSPEC_BAND_5G) {
		size_t num = sizeof(libnexio_wf_5g_80m_chans) / sizeof(libnexio_wf_5g_80m_chans[0]);
		for (size_t i = 0; i < num; i++) {
			if (center == libnexio_wf_5g_80m_chans[i])
				return true;
		}
	} else if (band == WL_CHANSPEC_BAND_6G) {
		if (center >= CH_MIN_6G_80M_CHANNEL && center <= CH_MAX_6G_80M_CHANNEL &&
		    ((center - CH_MIN_6G_80M_CHANNEL) % 16) == 0)
			return true;
	}
	return false;
}

static bool
libnexio_wf_valid_160MHz_center_chan(unsigned int center, unsigned int band)
{
	if (band == WL_CHANSPEC_BAND_5G) {
		size_t num = sizeof(libnexio_wf_5g_160m_chans) / sizeof(libnexio_wf_5g_160m_chans[0]);
		for (size_t i = 0; i < num; i++) {
			if (center == libnexio_wf_5g_160m_chans[i])
				return true;
		}
	} else if (band == WL_CHANSPEC_BAND_6G) {
		if (center >= CH_MIN_6G_160M_CHANNEL && center <= CH_MAX_6G_160M_CHANNEL &&
		    ((center - CH_MIN_6G_160M_CHANNEL) % CH_160MHZ_APART) == 0)
			return true;
	}
	return false;
}

static bool
libnexio_wf_valid_320MHz_center_chan(unsigned int center, unsigned int band)
{
	if (band == WL_CHANSPEC_BAND_6G) {
		if (center >= CH_MIN_6G_320M_CHANNEL && center <= CH_MAX_6G_320M_CHANNEL &&
		    ((center - CH_MIN_6G_320M_CHANNEL) % CH_160MHZ_APART) == 0)
			return true;
	}
	return false;
}

static chanspec_t
libnexio_wf_create_320MHz_chspec(unsigned int primary_channel, int scheme, unsigned int band)
{
	int sb = -1;
	size_t num = sizeof(libnexio_wf_6g_320m_chans) / sizeof(libnexio_wf_6g_320m_chans[0]);
	size_t idx = 0;

	if (band != WL_CHANSPEC_BAND_6G)
		return INVCHANSPEC;

	if (scheme != 2) {
		for (idx = 0; idx < num; idx += 2) {
			sb = libnexio_channel_to_sb(libnexio_wf_6g_320m_chans[idx], primary_channel, WL_CHANSPEC_BW_320);
			if (sb >= 0)
				break;
		}
	}
	if (sb < 0 && scheme != 1) {
		for (idx = 1; idx < num; idx += 2) {
			sb = libnexio_channel_to_sb(libnexio_wf_6g_320m_chans[idx], primary_channel, WL_CHANSPEC_BW_320);
			if (sb >= 0)
				break;
		}
	}
	if (sb >= 0) {
		return (chanspec_t)(((unsigned int)idx << WL_CHANSPEC_GE320_CHIDX_SHIFT) |
		                    ((unsigned int)sb << WL_CHANSPEC_GE320_SB_SHIFT) |
		                    WL_CHANSPEC_BW_320 | band);
	}
	return INVCHANSPEC;
}

static chanspec_t
libnexio_wf_create_chspec_from_primary(unsigned int pri_ch, unsigned int bw, unsigned int band)
{
	if (bw == WL_CHANSPEC_BW_20) {
		if (libnexio_wf_valid_20MHz_chan(pri_ch, band))
			return (chanspec_t)(pri_ch | WL_CHANSPEC_BW_20 | band);
		return INVCHANSPEC;
	}

	if (band == WL_CHANSPEC_BAND_5G) {
		const unsigned char *centers = NULL;
		size_t num = 0;
		if (bw == WL_CHANSPEC_BW_40) {
			centers = libnexio_wf_5g_40m_chans;
			num = sizeof(libnexio_wf_5g_40m_chans);
		} else if (bw == WL_CHANSPEC_BW_80) {
			centers = libnexio_wf_5g_80m_chans;
			num = sizeof(libnexio_wf_5g_80m_chans);
		} else if (bw == WL_CHANSPEC_BW_160) {
			centers = libnexio_wf_5g_160m_chans;
			num = sizeof(libnexio_wf_5g_160m_chans);
		}
		for (size_t i = 0; i < num; i++) {
			int sb = libnexio_channel_to_sb(centers[i], pri_ch, bw);
			if (sb >= 0)
				return (chanspec_t)(centers[i] | band | bw | ((unsigned int)sb << WL_CHANSPEC_CTL_SB_SHIFT));
		}
	} else if (band == WL_CHANSPEC_BAND_6G) {
		unsigned int apart = (bw == WL_CHANSPEC_BW_40) ? CH_40MHZ_APART :
		                     (bw == WL_CHANSPEC_BW_80) ? CH_80MHZ_APART :
		                     (bw == WL_CHANSPEC_BW_160) ? CH_160MHZ_APART : 0;
		if (apart) {
			unsigned int mask = ~(apart - 1);
			unsigned int base = 1 + ((pri_ch - 1) & mask);
			unsigned int center = base + libnexio_center_chan_to_edge(bw);
			int sb = libnexio_channel_to_sb(center, pri_ch, bw);
			if (sb >= 0)
				return (chanspec_t)(center | band | bw | ((unsigned int)sb << WL_CHANSPEC_CTL_SB_SHIFT));
		}
	}
	return INVCHANSPEC;
}

static uint8_t
libnexio_wf_center_to_primary20_chan(unsigned int center_ch, unsigned int bw, unsigned int sb)
{
	unsigned int delta = libnexio_center_chan_to_edge(bw);
	return (uint8_t)((center_ch - delta) + sb * CH_20MHZ_APART);
}

static uint8_t
libnexio_wf_chspec_primary20_chan(chanspec_t chspec)
{
	unsigned int center_chan;
	unsigned int bw;
	unsigned int sb;

	if ((chspec & WL_CHANSPEC_BW_MASK) == WL_CHANSPEC_BW_20) {
		return (uint8_t)(chspec & WL_CHANSPEC_CHAN_MASK);
	}

	bw = chspec & WL_CHANSPEC_BW_MASK;
	if (bw == WL_CHANSPEC_BW_320) {
		uint8_t ch_id = (uint8_t)((chspec & WL_CHANSPEC_GE320_CHIDX_MASK) >> WL_CHANSPEC_GE320_CHIDX_SHIFT);
		if (ch_id < 6)
			center_chan = 31 + ch_id * 32;
		else
			return 255;
		sb = (chspec & WL_CHANSPEC_GE320_SB_MASK) >> WL_CHANSPEC_GE320_SB_SHIFT;
	} else {
		center_chan = chspec & WL_CHANSPEC_CHAN_MASK;
		sb = (chspec & WL_CHANSPEC_CTL_SB_MASK) >> WL_CHANSPEC_CTL_SB_SHIFT;
	}

	return libnexio_wf_center_to_primary20_chan(center_chan, bw, sb);
}

static int
libnexio_read_uint(const char **p, unsigned int *num)
{
	unsigned long val;
	char *endp = NULL;

	val = strtoul(*p, &endp, 10);
	if (endp == *p)
		return 0;

	*p = endp;
	*num = (unsigned int)val;
	return 1;
}

__attribute__((weak)) chanspec_t
wf_chspec_aton(const char *a)
{
	chanspec_t chspec = INVCHANSPEC;
	unsigned int chspec_band = 0;
	unsigned int chspec_sb = 0;
	unsigned int chspec_bw = WL_CHANSPEC_BW_20;
	unsigned int bw = 20;
	unsigned int num = 0, pri_ch = 0;
	char c, sb_ul = '\0';
	bool band_specified = false;
	bool is_psc = false;
	bool explicit_psc_idx = false;

	if (!a)
		return 0;

	while (*a == ' ' || *a == '\t')
		a++;
	if (*a == '\0')
		return 0;

	if (strncasecmp(a, "psc", 3) == 0) {
		a += 3;
		is_psc = true;
		band_specified = true;
		chspec_band = WL_CHANSPEC_BAND_6G;
		while (*a == '/' || *a == '-' || *a == '_' || *a == ':' || *a == ' ' || *a == '\t')
			a++;
		if (*a == '#' || strncasecmp(a, "idx", 3) == 0) {
			explicit_psc_idx = true;
			if (*a == '#')
				a++;
			else
				a += 3;
			while (*a == '/' || *a == '-' || *a == '_' || *a == ':' || *a == ' ' || *a == '\t')
				a++;
		}
	} else if ((a[0] == '2' || a[0] == '5' || a[0] == '6') &&
	           (tolower((unsigned char)a[1]) == 'g')) {
		char b = a[0];
		a += 2;
		if (tolower((unsigned char)a[0]) == 'h' && tolower((unsigned char)a[1]) == 'z')
			a += 2;
		band_specified = true;
		if (b == '2')
			chspec_band = WL_CHANSPEC_BAND_2G;
		else if (b == '5')
			chspec_band = WL_CHANSPEC_BAND_5G;
		else
			chspec_band = WL_CHANSPEC_BAND_6G;

		while (*a == '/' || *a == '-' || *a == '_' || *a == ':' || *a == ' ' || *a == '\t')
			a++;

		if (strncasecmp(a, "psc", 3) == 0) {
			a += 3;
			is_psc = true;
			while (*a == '/' || *a == '-' || *a == '_' || *a == ':' || *a == ' ' || *a == '\t')
				a++;
			if (*a == '#' || strncasecmp(a, "idx", 3) == 0) {
				explicit_psc_idx = true;
				if (*a == '#')
					a++;
				else
					a += 3;
				while (*a == '/' || *a == '-' || *a == '_' || *a == ':' || *a == ' ' || *a == '\t')
					a++;
			}
		}
	}

	if (!libnexio_read_uint(&a, &num))
		return 0;

	if (is_psc) {
		if (explicit_psc_idx) {
			if (num < 1 || num > 15)
				return 0;
			pri_ch = 5 + (num - 1) * 16;
		} else if (WF_IS_6G_PSC_CHAN(num) && num <= CH_MAX_6G_CHANNEL) {
			pri_ch = num;
		} else if (num >= 1 && num <= 15) {
			pri_ch = 5 + (num - 1) * 16;
		} else {
			return 0;
		}
	} else if (band_specified) {
		pri_ch = num;
	} else {
		pri_ch = num;
		if (pri_ch <= CH_MAX_2G_CHANNEL) {
			chspec_band = WL_CHANSPEC_BAND_2G;
		} else if (pri_ch <= CH_MAX_5G_CHANNEL) {
			chspec_band = WL_CHANSPEC_BAND_5G;
		} else if (pri_ch <= CH_MAX_6G_CHANNEL) {
			chspec_band = WL_CHANSPEC_BAND_6G;
		} else {
			return 0;
		}
	}

	while (*a == ' ' || *a == '\t')
		a++;

	c = (char)tolower((unsigned char)a[0]);
	if (c == '\0') {
		chspec_bw = WL_CHANSPEC_BW_20;
		goto done_read;
	}

	if (c == 'u' || c == 'l') {
		a++;
		sb_ul = c;
		chspec_bw = WL_CHANSPEC_BW_40;
		goto done_read;
	}

	if (c == '/' || c == '-' || c == '_' || c == ':') {
		a++;
		while (*a == ' ' || *a == '\t')
			a++;
	} else {
		return 0;
	}

	if (!libnexio_read_uint(&a, &bw))
		return 0;

	if (tolower((unsigned char)a[0]) == 'm') {
		a++;
		if (tolower((unsigned char)a[0]) == 'h' && tolower((unsigned char)a[1]) == 'z')
			a += 2;
	}

	if (bw == 20) {
		chspec_bw = WL_CHANSPEC_BW_20;
	} else if (bw == 40) {
		chspec_bw = WL_CHANSPEC_BW_40;
	} else if (bw == 80) {
		chspec_bw = WL_CHANSPEC_BW_80;
	} else if (bw == 160) {
		chspec_bw = WL_CHANSPEC_BW_160;
	} else if (bw == 320) {
		chspec_bw = WL_CHANSPEC_BW_320;
	} else {
		return 0;
	}

	while (*a == ' ' || *a == '\t')
		a++;
	c = (char)tolower((unsigned char)a[0]);

	if (chspec_band == WL_CHANSPEC_BAND_2G && bw == 40) {
		if (c == 'u' || c == 'l') {
			a++;
			sb_ul = c;
			goto done_read;
		}
	}

	if (chspec_band == WL_CHANSPEC_BAND_6G && bw == 320) {
		if (c == '-' || c == '/' || c == '_') {
			a++;
			c = (char)tolower((unsigned char)a[0]);
			if (c == '1' || c == '2') {
				a++;
				sb_ul = c;
				goto done_read;
			}
		} else if (c == '1' || c == '2') {
			a++;
			sb_ul = c;
			goto done_read;
		} else if (c == 'o') {
			a++;
			sb_ul = c;
			goto done_read;
		}
	}

done_read:
	while (*a == ' ' || *a == '\t')
		a++;

	if (*a != '\0')
		return 0;

	if (sb_ul != '\0') {
		if (sb_ul == 'l' || sb_ul == 'u') {
			unsigned int center_ch;

			/*
			 * pri_ch is unsigned, so pri_ch - 2 wrapped to 0xFFFFFFFF for
			 * pri_ch < 2 and the result was OR'd straight into the
			 * chanspec with no validation: "1u" returned 0xffff as a
			 * success, "0l" returned 0x1802, and "5g34l" returned 0xd824
			 * (center 36, which is not a 40MHz center). Require a legal
			 * primary and a legal 40MHz center for the band, which is what
			 * the strong parser gets from
			 * wf_create_40MHz_chspec_primary_sb().
			 */
			if (sb_ul == 'l') {
				center_ch = pri_ch + CH_10MHZ_APART;
			} else {
				if (pri_ch < CH_10MHZ_APART)
					return 0;
				center_ch = pri_ch - CH_10MHZ_APART;
			}
			if (!libnexio_wf_valid_20MHz_chan(pri_ch, chspec_band) ||
			    !libnexio_wf_valid_40MHz_center_chan(center_ch, chspec_band))
				return 0;
			chspec_sb = (sb_ul == 'l') ? WL_CHANSPEC_CTL_SB_LLL : WL_CHANSPEC_CTL_SB_LLU;
			chspec = (chanspec_t)(center_ch | WL_CHANSPEC_BW_40 | chspec_band | chspec_sb);
		} else if (chspec_band == WL_CHANSPEC_BAND_6G && chspec_bw == WL_CHANSPEC_BW_320) {
			int scheme = (sb_ul == '1') ? 1 : (sb_ul == '2' || sb_ul == 'o') ? 2 : 0;
			if (!libnexio_wf_valid_20MHz_chan(pri_ch, chspec_band) &&
			    libnexio_wf_valid_320MHz_center_chan(pri_ch, chspec_band)) {
				unsigned int center_ch = pri_ch;
				pri_ch = center_ch - libnexio_center_chan_to_edge(WL_CHANSPEC_BW_320);
				if (scheme == 0) {
					for (size_t i = 0; i < sizeof(libnexio_wf_6g_320m_chans); i++) {
						if (center_ch == libnexio_wf_6g_320m_chans[i]) {
							scheme = (i & 1) ? 2 : 1;
							break;
						}
					}
				}
			}
			chspec = libnexio_wf_create_320MHz_chspec(pri_ch, scheme, chspec_band);
		} else {
			chspec = INVCHANSPEC;
		}
	} else if (chspec_bw == WL_CHANSPEC_BW_20) {
		if (libnexio_wf_valid_20MHz_chan(pri_ch, chspec_band))
			chspec = (chanspec_t)(pri_ch | WL_CHANSPEC_BW_20 | chspec_band);
	} else if (chspec_bw == WL_CHANSPEC_BW_320) {
		int scheme = 0;
		if (!libnexio_wf_valid_20MHz_chan(pri_ch, chspec_band) &&
		    libnexio_wf_valid_320MHz_center_chan(pri_ch, chspec_band)) {
			unsigned int center_ch = pri_ch;
			pri_ch = center_ch - libnexio_center_chan_to_edge(WL_CHANSPEC_BW_320);
			for (size_t i = 0; i < sizeof(libnexio_wf_6g_320m_chans); i++) {
				if (center_ch == libnexio_wf_6g_320m_chans[i]) {
					scheme = (i & 1) ? 2 : 1;
					break;
				}
			}
		}
		chspec = libnexio_wf_create_320MHz_chspec(pri_ch, scheme, chspec_band);
	} else {
		if (!libnexio_wf_valid_20MHz_chan(pri_ch, chspec_band)) {
			if (chspec_bw == WL_CHANSPEC_BW_40 && libnexio_wf_valid_40MHz_center_chan(pri_ch, chspec_band)) {
				pri_ch = pri_ch - libnexio_center_chan_to_edge(WL_CHANSPEC_BW_40);
			} else if (chspec_bw == WL_CHANSPEC_BW_80 && libnexio_wf_valid_80MHz_center_chan(pri_ch, chspec_band)) {
				pri_ch = pri_ch - libnexio_center_chan_to_edge(WL_CHANSPEC_BW_80);
			} else if (chspec_bw == WL_CHANSPEC_BW_160 && libnexio_wf_valid_160MHz_center_chan(pri_ch, chspec_band)) {
				pri_ch = pri_ch - libnexio_center_chan_to_edge(WL_CHANSPEC_BW_160);
			}
		}
		chspec = libnexio_wf_create_chspec_from_primary(pri_ch, chspec_bw, chspec_band);
	}

	if (chspec == INVCHANSPEC)
		return 0;

	return chspec;
}

__attribute__((weak)) char *
wf_chspec_ntoa(chanspec_t chspec, char *buf)
{
	const char *band = "";
	uint8_t pri_chan;
	unsigned int bw;

	if (!buf)
		return NULL;

	if ((chspec & WL_CHANSPEC_BAND_MASK) == WL_CHANSPEC_BAND_2G &&
	    (chspec & WL_CHANSPEC_CHAN_MASK) > CH_MAX_2G_CHANNEL)
		band = "2g";
	else if ((chspec & WL_CHANSPEC_BAND_MASK) == WL_CHANSPEC_BAND_5G &&
	         (chspec & WL_CHANSPEC_CHAN_MASK) <= CH_MAX_2G_CHANNEL)
		band = "5g";
	else if ((chspec & WL_CHANSPEC_BAND_MASK) == WL_CHANSPEC_BAND_6G)
		band = "6g";

	pri_chan = libnexio_wf_chspec_primary20_chan(chspec);
	if (pri_chan == 255)
		return NULL;

	bw = chspec & WL_CHANSPEC_BW_MASK;
	if (bw == WL_CHANSPEC_BW_20) {
		snprintf(buf, CHANSPEC_STR_LEN, "%s%u", band, pri_chan);
	} else if (bw == WL_CHANSPEC_BW_40) {
		const char *sb = ((chspec & WL_CHANSPEC_CTL_SB_MASK) == WL_CHANSPEC_CTL_SB_LLU) ? "u" : "l";
		if ((chspec & WL_CHANSPEC_BAND_MASK) == WL_CHANSPEC_BAND_2G)
			snprintf(buf, CHANSPEC_STR_LEN, "%s%u/40%s", band, pri_chan, sb);
		else
			snprintf(buf, CHANSPEC_STR_LEN, "%s%u/40", band, pri_chan);
	} else if (bw == WL_CHANSPEC_BW_80) {
		snprintf(buf, CHANSPEC_STR_LEN, "%s%u/80", band, pri_chan);
	} else if (bw == WL_CHANSPEC_BW_160) {
		snprintf(buf, CHANSPEC_STR_LEN, "%s%u/160", band, pri_chan);
	} else if (bw == WL_CHANSPEC_BW_320) {
		uint8_t ch_id = (uint8_t)((chspec & WL_CHANSPEC_GE320_CHIDX_MASK) >> WL_CHANSPEC_GE320_CHIDX_SHIFT);
		const char *sub = (ch_id & 1) ? "-2" : "-1";
		snprintf(buf, CHANSPEC_STR_LEN, "%s%u/320%s", band, pri_chan, sub);
	} else {
		return NULL;
	}

	return buf;
}

__attribute__((weak)) char *
wf_chspec_ntoa_ex(chanspec_t chspec, char *buf)
{
	if (!buf)
		return NULL;
	if (wf_chspec_ntoa(chspec, buf) == NULL)
		snprintf(buf, CHANSPEC_STR_LEN, "invalid 0x%04x", chspec);
	return buf;
}

__attribute__((weak)) bool
wf_chspec_is_6g_psc(chanspec_t chspec)
{
	uint8_t pri_ch;
	if ((chspec & WL_CHANSPEC_BAND_MASK) != WL_CHANSPEC_BAND_6G)
		return false;
	pri_ch = libnexio_wf_chspec_primary20_chan(chspec);
	if (pri_ch == 255)
		return false;
	return WF_IS_6G_PSC_CHAN(pri_ch);
}

