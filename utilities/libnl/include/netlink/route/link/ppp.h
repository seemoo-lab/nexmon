/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2016 Jonas Johansson <jonasj76@gmail.com>
 */

#ifndef NETLINK_LINK_PPP_H_
#define NETLINK_LINK_PPP_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_link	*rtnl_link_ppp_alloc(void);
extern int		rtnl_link_ppp_set_fd(struct rtnl_link *, int32_t);
extern int		rtnl_link_ppp_get_fd(struct rtnl_link *, int32_t *);

#ifdef __cplusplus
}
#endif

#endif
