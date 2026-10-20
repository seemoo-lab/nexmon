/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2014 Cong Wang <xiyou.wangcong@gmail.com>
 */

/**
 * @ingroup link
 * @defgroup ifb Intermediate Functional Block
 *
 * @details
 * \b Link Type Name: "ifb"
 *
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>

#include "link-api.h"

static struct rtnl_link_info_ops ifb_info_ops = {
	.io_name		= "ifb",
};

static void _nl_init ifb_init(void)
{
	rtnl_link_register_info(&ifb_info_ops);
}

static void _nl_exit ifb_exit(void)
{
	rtnl_link_unregister_info(&ifb_info_ops);
}

/** @} */
