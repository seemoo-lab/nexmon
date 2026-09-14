/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008-2010 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_CLI_LINK_H_
#define __NETLINK_CLI_LINK_H_

#include <netlink/route/link.h>
#include <netlink/cli/utils.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_link *nl_cli_link_alloc(void);
extern struct nl_cache *nl_cli_link_alloc_cache_family(struct nl_sock *, int);
extern struct nl_cache *nl_cli_link_alloc_cache_family_flags(struct nl_sock *, int,
							     unsigned int);
extern struct nl_cache *nl_cli_link_alloc_cache(struct nl_sock *);
extern struct nl_cache *nl_cli_link_alloc_cache_flags(struct nl_sock *,
						      unsigned int);

extern void nl_cli_link_parse_family(struct rtnl_link *, char *);
extern void nl_cli_link_parse_name(struct rtnl_link *, char *);
extern void nl_cli_link_parse_mtu(struct rtnl_link *, char *);
extern void nl_cli_link_parse_ifindex(struct rtnl_link *, char *);
extern void nl_cli_link_parse_txqlen(struct rtnl_link *, char *);
extern void nl_cli_link_parse_weight(struct rtnl_link *, char *);
extern void nl_cli_link_parse_ifalias(struct rtnl_link *, char *);

#ifdef __cplusplus
}
#endif

#endif
