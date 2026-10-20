/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2015 Cong Wang <xiyou.wangcong@gmail.com>
 */

#ifndef NETLINK_SKBEDIT_H_
#define NETLINK_SKBEDIT_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/action.h>
#include <linux/tc_act/tc_skbedit.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int rtnl_skbedit_set_action(struct rtnl_act *act, int action);
extern int rtnl_skbedit_get_action(struct rtnl_act *act);
extern int rtnl_skbedit_set_queue_mapping(struct rtnl_act *act, uint16_t index);
extern int rtnl_skbedit_get_queue_mapping(struct rtnl_act *act, uint16_t *index);
extern int rtnl_skbedit_set_mark(struct rtnl_act *act, uint32_t mark);
extern int rtnl_skbedit_get_mark(struct rtnl_act *act, uint32_t *mark);
extern int rtnl_skbedit_set_priority(struct rtnl_act *act, uint32_t prio);
extern int rtnl_skbedit_get_priority(struct rtnl_act *act, uint32_t *prio);

#ifdef __cplusplus
}
#endif

#endif
