/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_FIFO_H_
#define NETLINK_FIFO_H_

#include <netlink/netlink.h>
#include <netlink/route/qdisc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int	rtnl_qdisc_fifo_set_limit(struct rtnl_qdisc *, int);
extern int	rtnl_qdisc_fifo_get_limit(struct rtnl_qdisc *);

#ifdef __cplusplus
}
#endif

#endif
