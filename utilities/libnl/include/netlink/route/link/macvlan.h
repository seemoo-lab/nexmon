/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Michael Braun <michael-dev@fami-braun.de>
 */

#ifndef NETLINK_LINK_MACVLAN_H_
#define NETLINK_LINK_MACVLAN_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_link *rtnl_link_macvlan_alloc(void);

extern int		rtnl_link_is_macvlan(struct rtnl_link *);

extern char *		rtnl_link_macvlan_mode2str(int, char *, size_t);
extern int		rtnl_link_macvlan_str2mode(const char *);

extern char *		rtnl_link_macvlan_flags2str(int, char *, size_t);
extern int		rtnl_link_macvlan_str2flags(const char *);

extern char *		rtnl_link_macvlan_macmode2str(int, char *, size_t);
extern int		rtnl_link_macvlan_str2macmode(const char *);

extern int		rtnl_link_macvlan_set_mode(struct rtnl_link *,
			                           uint32_t);
extern uint32_t		rtnl_link_macvlan_get_mode(struct rtnl_link *);

extern int		rtnl_link_macvlan_set_flags(struct rtnl_link *,
						 uint16_t);
extern int		rtnl_link_macvlan_unset_flags(struct rtnl_link *,
						   uint16_t);
extern uint16_t		rtnl_link_macvlan_get_flags(struct rtnl_link *);

extern int		rtnl_link_macvlan_set_macmode(struct rtnl_link *,
						      uint32_t);
extern int		rtnl_link_macvlan_get_macmode(struct rtnl_link *link,
						      uint32_t *out_macmode);

extern int		rtnl_link_macvlan_count_macaddr(struct rtnl_link *link,
							uint32_t *out_count);
extern int		rtnl_link_macvlan_get_macaddr(struct rtnl_link *link,
						      uint32_t idx,
						      const struct nl_addr **addr);
extern int		rtnl_link_macvlan_add_macaddr(struct rtnl_link *link,
						      struct nl_addr *addr);
extern int		rtnl_link_macvlan_del_macaddr(struct rtnl_link *link,
						      struct nl_addr *addr);

#ifdef __cplusplus
}
#endif

#endif
