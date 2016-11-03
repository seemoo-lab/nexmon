/*
 * src/lib/link.c     CLI Link Helpers
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup cli
 * @defgroup cli_link Links
 *
 * @{
 */

#include <netlink/cli/utils.h>
#include <netlink/cli/link.h>

struct rtnl_link *nl_cli_link_alloc(void)
{
	struct rtnl_link *link;

	link = rtnl_link_alloc();
	if (!link)
		nl_cli_fatal(ENOMEM, "Unable to allocate link object");

	return link;
}

void nl_cli_link_parse_family(struct rtnl_link *link, char *arg)
{
	int family;

	if ((family = nl_str2af(arg)) == AF_UNSPEC)
		nl_cli_fatal(EINVAL,
			     "Unable to translate address family \"%s\"", arg);

	rtnl_link_set_family(link, family);
}

void nl_cli_link_parse_name(struct rtnl_link *link, char *arg)
{
	rtnl_link_set_name(link, arg);
}

void nl_cli_link_parse_mtu(struct rtnl_link *link, char *arg)
{
	uint32_t mtu = nl_cli_parse_u32(arg);
	rtnl_link_set_mtu(link, mtu);
}

void nl_cli_link_parse_ifindex(struct rtnl_link *link, char *arg)
{
	uint32_t index = nl_cli_parse_u32(arg);
	rtnl_link_set_ifindex(link, index);
}

void nl_cli_link_parse_txqlen(struct rtnl_link *link, char *arg)
{
	uint32_t qlen = nl_cli_parse_u32(arg);
	rtnl_link_set_txqlen(link, qlen);
}

void nl_cli_link_parse_weight(struct rtnl_link *link, char *arg)
{
	uint32_t weight = nl_cli_parse_u32(arg);
	rtnl_link_set_weight(link, weight);
}

/** @} */
