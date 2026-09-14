/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef NETLINK_ROUTE_UTILS_PRIV_H_
#define NETLINK_ROUTE_UTILS_PRIV_H_

#include <netlink/route/link/bridge.h>

extern const uint8_t *const _nltst_map_stat_id_from_IPSTATS_MIB_v2;
extern int _nl_bridge_fill_vlan_info(struct nl_msg *msg,
				     struct rtnl_link_bridge_vlan *vlan_info);

#endif
