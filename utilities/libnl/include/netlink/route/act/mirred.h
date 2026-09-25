/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Cong Wang <xiyou.wangcong@gmail.com>
 */

#ifndef NETLINK_MIRRED_H_
#define NETLINK_MIRRED_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/action.h>
#include <linux/tc_act/tc_mirred.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int rtnl_mirred_set_action(struct rtnl_act *, int);
extern int rtnl_mirred_get_action(struct rtnl_act *);
extern int rtnl_mirred_set_ifindex(struct rtnl_act *, uint32_t);
extern uint32_t rtnl_mirred_get_ifindex(struct rtnl_act *);
extern int rtnl_mirred_set_policy(struct rtnl_act *, int);
extern int rtnl_mirred_get_policy(struct rtnl_act *);

#ifdef __cplusplus
}
#endif

#endif
