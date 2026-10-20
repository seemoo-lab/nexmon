/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2015 Jonas Johansson <jonasj76@gmail.com>
 */

#ifndef NETLINK_LINK_TEAM_H_
#define NETLINK_LINK_TEAM_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_link *rtnl_link_team_alloc(void);

extern int	rtnl_link_team_add(struct nl_sock *, const char *,
				   struct rtnl_link *);

#ifdef __cplusplus
}
#endif

#endif

