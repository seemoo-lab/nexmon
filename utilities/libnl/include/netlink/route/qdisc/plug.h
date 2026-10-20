/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2012 Shriram Rajagopalan <rshriram@cs.ubc.ca>
 */

#ifndef NETLINK_PLUG_H_
#define NETLINK_PLUG_H_

#include <netlink/netlink.h>
#include <netlink/route/qdisc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int	rtnl_qdisc_plug_set_limit(struct rtnl_qdisc *, int);
extern int	rtnl_qdisc_plug_buffer(struct rtnl_qdisc *);
extern int	rtnl_qdisc_plug_release_one(struct rtnl_qdisc *);
extern int	rtnl_qdisc_plug_release_indefinite(struct rtnl_qdisc *);

#ifdef __cplusplus
}
#endif

#endif
