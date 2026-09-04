/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2018 Volodymyr Bendiuga <volodymyr.bendiuga@gmail.com>
 */

#ifndef NETLINK_VLAN_H_
#define NETLINK_VLAN_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/action.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int rtnl_vlan_set_mode(struct rtnl_act *act, int mode);
extern int rtnl_vlan_get_mode(struct rtnl_act *act, int *out_mode);
extern int rtnl_vlan_set_action(struct rtnl_act *act, int action);
extern int rtnl_vlan_get_action(struct rtnl_act *act, int *out_action);
extern int rtnl_vlan_set_protocol(struct rtnl_act *act, uint16_t protocol);
extern int rtnl_vlan_get_protocol(struct rtnl_act *act, uint16_t *out_protocol);
extern int rtnl_vlan_set_vlan_id(struct rtnl_act *act, uint16_t vid);
extern int rtnl_vlan_get_vlan_id(struct rtnl_act *act, uint16_t *out_vid);
extern int rtnl_vlan_set_vlan_prio(struct rtnl_act *act, uint8_t prio);
extern int rtnl_vlan_get_vlan_prio(struct rtnl_act *act, uint8_t *out_prio);

#ifdef __cplusplus
}
#endif

#endif /* NETLINK_VLAN_H_ */
