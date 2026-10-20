/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2011-2013 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_LINK_BONDING_H_
#define NETLINK_LINK_BONDING_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_link *rtnl_link_bond_alloc(void);

extern int	rtnl_link_is_bond(struct rtnl_link *);

extern int	rtnl_link_bond_add(struct nl_sock *, const char *,
				   struct rtnl_link *);

extern int	rtnl_link_bond_enslave_ifindex(struct nl_sock *, int, int);
extern int	rtnl_link_bond_enslave(struct nl_sock *, struct rtnl_link *,
				       struct rtnl_link *);

extern int	rtnl_link_bond_release_ifindex(struct nl_sock *, int);
extern int	rtnl_link_bond_release(struct nl_sock *, struct rtnl_link *);

extern void	rtnl_link_bond_set_mode(struct rtnl_link *link, uint8_t mode);
extern int	rtnl_link_bond_get_mode(struct rtnl_link *link, uint8_t *mode);

extern void	rtnl_link_bond_set_activeslave(struct rtnl_link *link, int active_slave);
extern int	rtnl_link_bond_get_activeslave(struct rtnl_link *link, int *active_slave);

extern void	rtnl_link_bond_set_hashing_type(struct rtnl_link *link, uint8_t type);
extern int	rtnl_link_bond_get_hashing_type(struct rtnl_link *link, uint8_t *type);

extern void	rtnl_link_bond_set_miimon(struct rtnl_link *link, uint32_t miimon);
extern int	rtnl_link_bond_get_miimon(struct rtnl_link *link, uint32_t *miimon);

extern void	rtnl_link_bond_set_min_links(struct rtnl_link *link, uint32_t min_links);
extern int	rtnl_link_bond_get_min_links(struct rtnl_link *link, uint32_t *min_links);

#ifdef __cplusplus
}
#endif

#endif

