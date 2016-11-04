/*
 * nl80211 linux driver interface.
 *
 * Copyright (C) 1999-2013, Broadcom Corporation
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $Id: $
 */
#include <errno.h>
#include <linux/nl80211.h>
#include <dhdioctl.h>
#include "dhdu_nl80211.h"

/* libnl 1.x compatibility code */
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
static inline struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}

static inline void nl_socket_free(struct nl_sock *h)
{
	nl_handle_destroy(h);
}
#endif /* CONFIG_LIBNL20 && CONFIG_LIBNL30 */

static int dhd_nl_error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int dhd_nl_finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int dhd_nl_ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int dhd_nl_valid_handler(struct nl_msg *msg, void *arg)
{
	return NL_SKIP;
}

int dhd_nl_sock_connect(struct dhd_netlink_info *dhd_nli)
{
	dhd_nli->nl = nl_socket_alloc();
	if (dhd_nli->nl == NULL)
		return -1;

	if (genl_connect(dhd_nli->nl) < 0) {
		fprintf(stderr, "netlink connection failed\n");
		goto err;
	}

	dhd_nli->nl_id = genl_ctrl_resolve(dhd_nli->nl, "nl80211");
	if (dhd_nli->nl_id < 0) {
		fprintf(stderr, "'nl80211' netlink not found\n");
		goto err;
	}

	dhd_nli->cb = nl_cb_alloc(NL_CB_DEBUG);
	if (dhd_nli->cb == NULL)
		goto err;

	nl_socket_set_cb(dhd_nli->nl, dhd_nli->cb);
	return 0;

err:
	nl_cb_put(dhd_nli->cb);
	nl_socket_free(dhd_nli->nl);
	fprintf(stderr, "nl80211 connection failed\n");
	return -1;
}

void dhd_nl_sock_disconnect(struct dhd_netlink_info *dhd_nli)
{
	nl_cb_put(dhd_nli->cb);
	nl_socket_free(dhd_nli->nl);
}

int dhd_nl_do_testmode(struct dhd_netlink_info *dhd_nli, dhd_ioctl_t *ioc)
{
	struct nl_msg *msg;
	int err;

	msg = nlmsg_alloc();
	if (msg == NULL)
		return -ENOMEM;

	/* fill testmode message */
	genlmsg_put(msg, 0, 0, dhd_nli->nl_id, 0, 0,
		NL80211_CMD_TESTMODE, 0);
	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, dhd_nli->ifidx);
	NLA_PUT(msg, NL80211_ATTR_TESTDATA, sizeof(*ioc), ioc);

	err = nl_send_auto_complete(dhd_nli->nl, msg);
	if (err < 0)
		goto out;

	err = 1;
	nl_cb_err(dhd_nli->cb, NL_CB_CUSTOM, dhd_nl_error_handler, &err);
	nl_cb_set(dhd_nli->cb, NL_CB_ACK, NL_CB_CUSTOM,
		dhd_nl_ack_handler, &err);
	nl_cb_set(dhd_nli->cb, NL_CB_FINISH, NL_CB_CUSTOM,
		dhd_nl_finish_handler, &err);
	nl_cb_set(dhd_nli->cb, NL_CB_VALID, NL_CB_CUSTOM,
		dhd_nl_valid_handler, &err);
	while (err > 0)
		nl_recvmsgs(dhd_nli->nl, dhd_nli->cb);
out:
	nlmsg_free(msg);
	return err;

nla_put_failure:
	fprintf(stderr, "setting netlink attribute failed\n");
	err = -EFAULT;
	goto out;
}
