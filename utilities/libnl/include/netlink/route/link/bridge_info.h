/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2022 MaxLinear, Inc.
 */

#ifndef NETLINK_LINK_BRIDGE_INFO_H_
#define NETLINK_LINK_BRIDGE_INFO_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void rtnl_link_bridge_set_ageing_time(struct rtnl_link *link,
					     uint32_t ageing_time);
extern int rtnl_link_bridge_get_ageing_time(struct rtnl_link *link,
					    uint32_t *ageing_time);

extern void rtnl_link_bridge_set_vlan_filtering(struct rtnl_link *link,
						uint8_t vlan_filtering);
extern int rtnl_link_bridge_get_vlan_filtering(struct rtnl_link *link,
					       uint8_t *vlan_filtering);

extern void rtnl_link_bridge_set_vlan_protocol(struct rtnl_link *link,
					       uint16_t vlan_protocol);
extern int rtnl_link_bridge_get_vlan_protocol(struct rtnl_link *link,
					      uint16_t *vlan_protocol);

extern void rtnl_link_bridge_set_vlan_default_pvid(struct rtnl_link *link,
						   uint16_t default_pvid);
extern int rtnl_link_bridge_get_vlan_default_pvid(struct rtnl_link *link,
						  uint16_t *default_pvid);

extern void rtnl_link_bridge_set_vlan_stats_enabled(struct rtnl_link *link,
						    uint8_t vlan_stats_enabled);
extern int rtnl_link_bridge_get_vlan_stats_enabled(struct rtnl_link *link,
						   uint8_t *vlan_stats_enabled);

extern void rtnl_link_bridge_set_nf_call_iptables(struct rtnl_link *link,
						  uint8_t call_enabled);
extern int rtnl_link_bridge_get_nf_call_iptables(struct rtnl_link *link,
						 uint8_t *call_enabled);

extern void rtnl_link_bridge_set_nf_call_ip6tables(struct rtnl_link *link,
						   uint8_t call_enabled);
extern int rtnl_link_bridge_get_nf_call_ip6tables(struct rtnl_link *link,
						  uint8_t *call_enabled);

extern void rtnl_link_bridge_set_nf_call_arptables(struct rtnl_link *link,
						   uint8_t call_enabled);
extern int rtnl_link_bridge_get_nf_call_arptables(struct rtnl_link *link,
						  uint8_t *call_enabled);

extern void rtnl_link_bridge_set_stp_state(struct rtnl_link *link,
					   uint32_t stp_state);
extern int rtnl_link_bridge_get_stp_state(struct rtnl_link *link,
					  uint32_t *stp_state);

extern void rtnl_link_bridge_set_mcast_router(struct rtnl_link *link,
					      uint8_t type);
extern int rtnl_link_bridge_get_mcast_router(struct rtnl_link *link,
					     uint8_t *type);

extern void rtnl_link_bridge_set_mcast_snooping(struct rtnl_link *link,
						uint8_t value);
extern int rtnl_link_bridge_get_mcast_snooping(struct rtnl_link *link,
					       uint8_t *value);

extern int rtnl_link_bridge_set_boolopt(struct rtnl_link *link, int opt,
					int value);

extern int rtnl_link_bridge_get_boolopt(struct rtnl_link *link, int opt);

#ifdef __cplusplus
}
#endif

#endif
