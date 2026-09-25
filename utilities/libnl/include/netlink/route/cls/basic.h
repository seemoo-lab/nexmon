/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008-2010 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_BASIC_H_
#define NETLINK_BASIC_H_

#include <netlink/netlink.h>
#include <netlink/route/classifier.h>
#include <netlink/route/cls/ematch.h>
#include <netlink/route/action.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void			rtnl_basic_set_target(struct rtnl_cls *, uint32_t);
extern uint32_t			rtnl_basic_get_target(struct rtnl_cls *);
extern void			rtnl_basic_set_ematch(struct rtnl_cls *,
						      struct rtnl_ematch_tree *);
extern struct rtnl_ematch_tree *rtnl_basic_get_ematch(struct rtnl_cls *);
extern int rtnl_basic_add_action(struct rtnl_cls *, struct rtnl_act *);
extern int rtnl_basic_del_action(struct rtnl_cls *, struct rtnl_act *);
extern struct rtnl_act* rtnl_basic_get_action(struct rtnl_cls *);

#ifdef __cplusplus
}
#endif

#endif
