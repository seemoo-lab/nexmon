/*
 * lib/route/sch/blackhole.c	Blackhole Qdisc
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup qdisc_api
 * @defgroup blackhole Blackhole
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc-modules.h>

static struct rtnl_qdisc_ops blackhole_ops = {
	.qo_kind		= "blackhole",
};

static void __init blackhole_init(void)
{
	rtnl_qdisc_register(&blackhole_ops);
}

static void __exit blackhole_exit(void)
{
	rtnl_qdisc_unregister(&blackhole_ops);
}

/** @} */
