/*
 * lib/route/link/api.c		Link Info API
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup link
 * @defgroup link_info Link Info API
 * @brief
 *
 * @par 1) Registering/Unregistering a new link info type
 * @code
 * static struct rtnl_link_info_ops vlan_info_ops = {
 * 	.io_name		= "vlan",
 * 	.io_alloc		= vlan_alloc,
 * 	.io_parse		= vlan_parse,
 * 	.io_dump[NL_DUMP_BRIEF]	= vlan_dump_brief,
 * 	.io_dump[NL_DUMP_FULL]	= vlan_dump_full,
 * 	.io_free		= vlan_free,
 * };
 *
 * static void __init vlan_init(void)
 * {
 * 	rtnl_link_register_info(&vlan_info_ops);
 * }
 *
 * static void __exit vlan_exit(void)
 * {
 * 	rtnl_link_unregister_info(&vlan_info_ops);
 * }
 * @endcode
 *
 * @{
 */

#include <netlink-local.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/link.h>
#include <netlink/route/link/info-api.h>

static struct rtnl_link_info_ops *info_ops;

struct rtnl_link_info_ops *rtnl_link_info_ops_lookup(const char *name)
{
	struct rtnl_link_info_ops *ops;

	for (ops = info_ops; ops; ops = ops->io_next)
		if (!strcmp(ops->io_name, name))
			return ops;

	return NULL;
}

int rtnl_link_register_info(struct rtnl_link_info_ops *ops)
{
	if (ops->io_name == NULL)
		return -NLE_INVAL;

	if (rtnl_link_info_ops_lookup(ops->io_name))
		return -NLE_EXIST;

	NL_DBG(1, "Registered link info operations %s\n", ops->io_name);

	ops->io_next = info_ops;
	info_ops = ops;

	return 0;
}

int rtnl_link_unregister_info(struct rtnl_link_info_ops *ops)
{
	struct rtnl_link_info_ops *t, **tp;

	for (tp = &info_ops; (t=*tp) != NULL; tp = &t->io_next)
		if (t == ops)
			break;

	if (!t)
		return -NLE_OPNOTSUPP;

	if (t->io_refcnt > 0)
		return -NLE_BUSY;

	NL_DBG(1, "Unregistered link info perations %s\n", ops->io_name);

	*tp = t->io_next;
	return 0;
}

/** @} */

