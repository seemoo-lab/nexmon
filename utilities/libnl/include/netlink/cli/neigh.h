/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_CLI_NEIGH_H_
#define __NETLINK_CLI_NEIGH_H_

#include <netlink/route/neighbour.h>

#ifdef __cplusplus
extern "C" {
#endif

#define nl_cli_neigh_alloc_cache(sk) \
		nl_cli_alloc_cache_flags((sk), "neighbour", NL_CACHE_AF_ITER, \
					 rtnl_neigh_alloc_cache_flags)

extern struct rtnl_neigh *nl_cli_neigh_alloc(void);
extern void nl_cli_neigh_parse_dst(struct rtnl_neigh *, char *);
extern void nl_cli_neigh_parse_lladdr(struct rtnl_neigh *, char *);
extern void nl_cli_neigh_parse_dev(struct rtnl_neigh *, struct nl_cache *, char *);
extern void nl_cli_neigh_parse_family(struct rtnl_neigh *, char *);
extern void nl_cli_neigh_parse_state(struct rtnl_neigh *, char *);

#ifdef __cplusplus
}
#endif

#endif
