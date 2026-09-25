/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2015 Jonas Johansson <jonasj76@gmail.com>
 */

/**
 * @ingroup link
 * @defgroup team Team
 *
 * @details
 * \b Link Type Name: "team"
 *
 * @route_doc{link_team, Team Documentation}
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/route/link/team.h>

#include "link-api.h"

/**
 * Allocate link object of type team
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_team_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "team") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Create a new kernel team device
 * @arg sock		netlink socket
 * @arg name		name of team device or NULL
 * @arg opts		team options (currently unused)
 *
 * Creates a new team device in the kernel. If no name is
 * provided, the kernel will automatically pick a name of the
 * form "type%d" (e.g. team0, vlan1, etc.)
 *
 * The \a opts argument is currently unused. In the future, it
 * may be used to carry additional team options to be set
 * when creating the team device.
 *
 * @note When letting the kernel assign a name, it will become
 *       difficult to retrieve the interface afterwards because
 *       you have to guess the name the kernel has chosen. It is
 *       therefore not recommended to not provide a device name.
 *
 * @see rtnl_link_team_enslave()
 * @see rtnl_link_team_release()
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_team_add(struct nl_sock *sock, const char *name,
		       struct rtnl_link *opts)
{
	struct rtnl_link *link;
	int err;

	if (!(link = rtnl_link_team_alloc()))
		return -NLE_NOMEM;

	if (!name && opts)
		name = rtnl_link_get_name(opts);

	if (name)
		rtnl_link_set_name(link, name);

	err = rtnl_link_add(sock, link, NLM_F_CREATE);

	rtnl_link_put(link);

	return err;
}

static struct rtnl_link_info_ops team_info_ops = {
	.io_name		= "team",
};

static void _nl_init team_init(void)
{
	rtnl_link_register_info(&team_info_ops);
}

static void _nl_exit team_exit(void)
{
	rtnl_link_unregister_info(&team_info_ops);
}

/** @} */
