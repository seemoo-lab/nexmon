/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2014 Dan Williams <dcbw@redhat.com>
 */

#ifndef NETLINK_LINK_INET6_H_
#define NETLINK_LINK_INET6_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *		rtnl_link_inet6_addrgenmode2str  (uint8_t mode,
							  char *buf,
							  size_t len);

uint8_t			rtnl_link_inet6_str2addrgenmode  (const char *mode);

extern int		rtnl_link_inet6_get_token(struct rtnl_link *,
						  struct nl_addr **);

extern int		rtnl_link_inet6_set_token(struct rtnl_link *,
						  struct nl_addr *);

extern int		rtnl_link_inet6_get_addr_gen_mode(struct rtnl_link *,
							  uint8_t *);

extern int		rtnl_link_inet6_set_addr_gen_mode(struct rtnl_link *,
							  uint8_t);

extern int		rtnl_link_inet6_get_flags(struct rtnl_link *,
							  uint32_t *);

extern int		rtnl_link_inet6_set_flags(struct rtnl_link *,
							  uint32_t);

extern int		rtnl_link_inet6_get_conf(struct rtnl_link *,
						 unsigned int,
						 uint32_t *);

/* Link Flags Translations */
extern char *	rtnl_link_inet6_flags2str(int, char *, size_t);
extern int		rtnl_link_inet6_str2flags(const char *);

#ifdef __cplusplus
}
#endif

#endif
