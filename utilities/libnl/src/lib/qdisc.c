/*
 * src/lib/qdisc.c     CLI QDisc Helpers
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
 * @defgroup cli_qdisc Queueing Disciplines
 *
 * @{
 */

#include <netlink/cli/utils.h>
#include <netlink/cli/qdisc.h>

struct rtnl_qdisc *nl_cli_qdisc_alloc(void)
{
	struct rtnl_qdisc *qdisc;

	qdisc = rtnl_qdisc_alloc();
	if (!qdisc)
		nl_cli_fatal(ENOMEM, "Unable to allocate qdisc object");

	return qdisc;
}

void nl_cli_qdisc_parse_dev(struct rtnl_qdisc *qdisc, struct nl_cache *link_cache, char *arg)
{
	int ival;

	if (!(ival = rtnl_link_name2i(link_cache, arg)))
		nl_cli_fatal(ENOENT, "Link \"%s\" does not exist", arg);

	rtnl_qdisc_set_ifindex(qdisc, ival);
}

void nl_cli_qdisc_parse_parent(struct rtnl_qdisc *qdisc, char *arg)
{
	uint32_t parent;
	int err;

	if ((err = rtnl_tc_str2handle(arg, &parent)) < 0)
		nl_cli_fatal(err, "Unable to parse handle \"%s\": %s",
		      arg, nl_geterror(err));

	rtnl_qdisc_set_parent(qdisc, parent);
}

void nl_cli_qdisc_parse_handle(struct rtnl_qdisc *qdisc, char *arg)
{
	uint32_t handle;
	int err;

	if ((err = rtnl_tc_str2handle(arg, &handle)) < 0)
		nl_cli_fatal(err, "Unable to parse handle \"%s\": %s",
		      arg, nl_geterror(err));

	rtnl_qdisc_set_handle(qdisc, handle);
}

void nl_cli_qdisc_parse_kind(struct rtnl_qdisc *qdisc, char *arg)
{
	rtnl_qdisc_set_kind(qdisc, arg);
}

/** @} */
