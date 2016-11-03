/*
 * netlink/cli/link.h     CLI Link Helpers
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_CLI_LINK_H_
#define __NETLINK_CLI_LINK_H_

#include <netlink/route/link.h>
#include <netlink/cli/utils.h>

#define nl_cli_link_alloc_cache(sk) \
		nl_cli_alloc_cache((sk), "link", rtnl_link_alloc_cache)

extern struct rtnl_link *nl_cli_link_alloc(void);

extern void nl_cli_link_parse_family(struct rtnl_link *, char *);
extern void nl_cli_link_parse_name(struct rtnl_link *, char *);
extern void nl_cli_link_parse_mtu(struct rtnl_link *, char *);
extern void nl_cli_link_parse_ifindex(struct rtnl_link *, char *);
extern void nl_cli_link_parse_txqlen(struct rtnl_link *, char *);
extern void nl_cli_link_parse_weight(struct rtnl_link *, char *);

#endif
