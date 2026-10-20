/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2011 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup link
 * @defgroup dummy Dummy
 *
 * @details
 * \b Link Type Name: "dummy"
 *
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>

#include "link-api.h"

static struct rtnl_link_info_ops dummy_info_ops = {
	.io_name		= "dummy",
};

static void _nl_init dummy_init(void)
{
	rtnl_link_register_info(&dummy_info_ops);
}

static void _nl_exit dummy_exit(void)
{
	rtnl_link_unregister_info(&dummy_info_ops);
}

/** @} */
