/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef NETLINK_LINK_IP6VTI_H_
#define NETLINK_LINK_IP6VTI_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

	int rtnl_link_is_ip6vti(struct rtnl_link *link);

	extern struct rtnl_link *rtnl_link_ip6vti_alloc(void);
	extern int rtnl_link_ip6vti_add(struct nl_sock *sk, const char *name);

	extern int rtnl_link_ip6vti_set_link(struct rtnl_link *link, uint32_t index);
	extern int rtnl_link_ip6vti_get_link(struct rtnl_link *link, uint32_t *index);

	extern int rtnl_link_ip6vti_set_ikey(struct rtnl_link *link, uint32_t ikey);
	extern int rtnl_link_ip6vti_get_ikey(struct rtnl_link *link, uint32_t *ikey);

	extern int rtnl_link_ip6vti_set_okey(struct rtnl_link *link, uint32_t okey);
	extern int rtnl_link_ip6vti_get_okey(struct rtnl_link *link, uint32_t *okey);

	extern int rtnl_link_ip6vti_set_local(struct rtnl_link *link, struct in6_addr *local);
	extern int rtnl_link_ip6vti_get_local(struct rtnl_link *link, struct in6_addr *remote);

	extern int rtnl_link_ip6vti_set_remote(struct rtnl_link *link, struct in6_addr *remote);
	extern int rtnl_link_ip6vti_get_remote(struct rtnl_link *link, struct in6_addr *remote);

	extern int rtnl_link_ip6vti_set_fwmark(struct rtnl_link *link, uint32_t fwmark);
	extern int rtnl_link_ip6vti_get_fwmark(struct rtnl_link *link, uint32_t *fwmark);

#ifdef __cplusplus
}
#endif

#endif
