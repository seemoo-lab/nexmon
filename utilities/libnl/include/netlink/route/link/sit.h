/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2014 Susant Sahani <susant@redhat.com>
 */

#ifndef NETLINK_LINK_SIT_H_
#define NETLINK_LINK_SIT_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

	extern struct rtnl_link *rtnl_link_sit_alloc(void);
	extern int rtnl_link_sit_add(struct nl_sock *sk, const char *name);

	extern int rtnl_link_is_sit(struct rtnl_link *link);

	extern int rtnl_link_sit_set_link(struct rtnl_link *link,  uint32_t index);
	extern uint32_t rtnl_link_sit_get_link(struct rtnl_link *link);

	extern int rtnl_link_sit_set_local(struct rtnl_link *link, uint32_t addr);
	extern uint32_t rtnl_link_sit_get_local(struct rtnl_link *link);

	extern int rtnl_link_sit_set_remote(struct rtnl_link *link, uint32_t addr);
	extern uint32_t rtnl_link_sit_get_remote(struct rtnl_link *link);

	extern int rtnl_link_sit_set_ttl(struct rtnl_link *link, uint8_t ttl);
	extern uint8_t rtnl_link_sit_get_ttl(struct rtnl_link *link);

	extern int rtnl_link_sit_set_tos(struct rtnl_link *link, uint8_t tos);
	extern uint8_t rtnl_link_sit_get_tos(struct rtnl_link *link);

	extern int rtnl_link_sit_set_pmtudisc(struct rtnl_link *link, uint8_t pmtudisc);
	extern uint8_t rtnl_link_sit_get_pmtudisc(struct rtnl_link *link);

	extern int rtnl_link_sit_set_flags(struct rtnl_link *link, uint16_t flags);
	extern uint16_t rtnl_link_sit_get_flags(struct rtnl_link *link);

	int rtnl_link_sit_set_proto(struct rtnl_link *link, uint8_t proto);
	uint8_t rtnl_link_sit_get_proto(struct rtnl_link *link);

	int rtnl_link_sit_set_ip6rd_prefix(struct rtnl_link *link, const struct in6_addr *prefix);
	int rtnl_link_sit_get_ip6rd_prefix(const struct rtnl_link *link, struct in6_addr *prefix);

	int rtnl_link_sit_set_ip6rd_prefixlen(struct rtnl_link *link, uint16_t prefixlen);
	int rtnl_link_sit_get_ip6rd_prefixlen(struct rtnl_link *link, uint16_t *prefixlen);

	int rtnl_link_sit_set_ip6rd_relay_prefix(struct rtnl_link *link, uint32_t prefix);
	int rtnl_link_sit_get_ip6rd_relay_prefix(const struct rtnl_link *link, uint32_t *prefix);

	int rtnl_link_sit_set_ip6rd_relay_prefixlen(struct rtnl_link *link, uint16_t prefix);
	int rtnl_link_sit_get_ip6rd_relay_prefixlen(struct rtnl_link *link, uint16_t *prefix);

	extern int rtnl_link_sit_set_fwmark(struct rtnl_link *link, uint32_t fwmark);
	extern int rtnl_link_sit_get_fwmark(struct rtnl_link *link, uint32_t *fwmark);

#ifdef __cplusplus
}
#endif

#endif
