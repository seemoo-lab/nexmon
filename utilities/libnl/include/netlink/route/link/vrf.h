/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2015 Cumulus Networks. All rights reserved.
 * Copyright (c) 2015 David Ahern <dsa@cumulusnetworks.com>
 */

#ifndef NETLINK_LINK_VRF_H_
#define NETLINK_LINK_VRF_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_link *rtnl_link_vrf_alloc(void);
extern int rtnl_link_is_vrf(struct rtnl_link *link);
extern int rtnl_link_vrf_get_tableid(struct rtnl_link *link, uint32_t *id);
extern int rtnl_link_vrf_set_tableid(struct rtnl_link *link, uint32_t id);

#ifdef __cplusplus
}
#endif

#endif
