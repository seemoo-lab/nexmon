/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2011 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup qdisc
 * @defgroup qdisc_blackhole Blackhole
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>

#include "tc-api.h"

static struct rtnl_tc_ops blackhole_ops = {
	.to_kind		= "blackhole",
	.to_type		= RTNL_TC_TYPE_QDISC,
};

static void _nl_init blackhole_init(void)
{
	rtnl_tc_register(&blackhole_ops);
}

static void _nl_exit blackhole_exit(void)
{
	rtnl_tc_unregister(&blackhole_ops);
}

/** @} */
