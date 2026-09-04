/*
 * netlink/route/cls/basic.h	Basic Classifier
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_BASIC_H_
#define NETLINK_BASIC_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_cls_ops *rtnl_basic_get_ops(void);
extern int	rtnl_basic_set_classid(struct rtnl_cls *, uint32_t);
extern uint32_t	rtnl_basic_get_classid(struct rtnl_cls *);
extern int	rtnl_basic_set_ematch(struct rtnl_cls *,
				      struct rtnl_ematch_tree *);
extern struct rtnl_ematch_tree *
		rtnl_basic_get_ematch(struct rtnl_cls *);

#ifdef __cplusplus
}
#endif

#endif
