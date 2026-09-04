/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_CLI_ROUTE_H_
#define __NETLINK_CLI_ROUTE_H_

#include <netlink/route/route.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_route *nl_cli_route_alloc(void);

extern struct nl_cache *nl_cli_route_alloc_cache(struct nl_sock *, int);

extern void	nl_cli_route_parse_family(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_dst(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_src(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_pref_src(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_metric(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_nexthop(struct rtnl_route *, char *, struct nl_cache *);
extern void	nl_cli_route_parse_table(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_prio(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_scope(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_protocol(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_type(struct rtnl_route *, char *);
extern void	nl_cli_route_parse_iif(struct rtnl_route *, char *, struct nl_cache *);

#ifdef __cplusplus
}
#endif

#endif
