/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Cong Wang <xiyou.wangcong@gmail.com>
 */

#ifndef NETLINK_LINK_VETH_H_
#define NETLINK_LINK_VETH_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_link *rtnl_link_veth_alloc(void);
extern void rtnl_link_veth_release(struct rtnl_link *);

extern int rtnl_link_is_veth(struct rtnl_link *);

extern struct rtnl_link *rtnl_link_veth_get_peer(struct rtnl_link *);
extern int rtnl_link_veth_add(struct nl_sock *sock, const char *name,
			      const char *peer, pid_t pid);

#ifdef __cplusplus
}
#endif

#endif
