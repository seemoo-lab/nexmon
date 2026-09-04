/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef NETLINK_LINK_IP6GRE_H_
#define NETLINK_LINK_IP6GRE_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

	extern int rtnl_link_is_ip6gre(struct rtnl_link *link);

	extern struct rtnl_link *rtnl_link_ip6gre_alloc(void);
	extern int rtnl_link_ip6gre_add(struct nl_sock *sk, const char *name);

	extern int rtnl_link_ip6gre_set_link(struct rtnl_link *link, uint32_t index);
	extern int rtnl_link_ip6gre_get_link(struct rtnl_link *link, uint32_t *index);

	extern int rtnl_link_ip6gre_set_iflags(struct rtnl_link *link, uint16_t iflags);
	extern int rtnl_link_ip6gre_get_iflags(struct rtnl_link *link, uint16_t *iflags);

	extern int rtnl_link_ip6gre_set_oflags(struct rtnl_link *link, uint16_t oflags);
	extern int rtnl_link_ip6gre_get_oflags(struct rtnl_link *link, uint16_t *oflags);

	extern int rtnl_link_ip6gre_set_ikey(struct rtnl_link *link, uint32_t ikey);
	extern int rtnl_link_ip6gre_get_ikey(struct rtnl_link *link, uint32_t *ikey);

	extern int rtnl_link_ip6gre_set_okey(struct rtnl_link *link, uint32_t okey);
	extern int rtnl_link_ip6gre_get_okey(struct rtnl_link *link, uint32_t *okey);

	extern int rtnl_link_ip6gre_set_local(struct rtnl_link *link, struct in6_addr *local);
	extern int rtnl_link_ip6gre_get_local(struct rtnl_link *link, struct in6_addr *local);

	extern int rtnl_link_ip6gre_set_remote(struct rtnl_link *link, struct in6_addr *remote);
	extern int rtnl_link_ip6gre_get_remote(struct rtnl_link *link, struct in6_addr *remote);

	extern int rtnl_link_ip6gre_set_ttl(struct rtnl_link *link, uint8_t ttl);
	extern int rtnl_link_ip6gre_get_ttl(struct rtnl_link *link, uint8_t *ttl);

	extern int rtnl_link_ip6gre_set_encaplimit(struct rtnl_link *link, uint8_t encaplimit);
	extern int rtnl_link_ip6gre_get_encaplimit(struct rtnl_link *link, uint8_t *encaplimit);

	extern int rtnl_link_ip6gre_set_flowinfo(struct rtnl_link *link, uint32_t flowinfo);
	extern int rtnl_link_ip6gre_get_flowinfo(struct rtnl_link *link, uint32_t *flowinfo);

	extern int rtnl_link_ip6gre_set_flags(struct rtnl_link *link, uint32_t flags);
	extern int rtnl_link_ip6gre_get_flags(struct rtnl_link *link, uint32_t *flags);

	extern int rtnl_link_ip6gre_set_fwmark(struct rtnl_link *link, uint32_t fwmark);
	extern int rtnl_link_ip6gre_get_fwmark(struct rtnl_link *link, uint32_t *fwmark);

#ifdef __cplusplus
}
#endif

#endif
