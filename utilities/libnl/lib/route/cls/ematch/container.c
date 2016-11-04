/*
 * lib/route/cls/ematch/container.c	Container Ematch
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/cls/ematch.h>

static int container_parse(struct rtnl_ematch *m, void *data, size_t len)
{
	memcpy(m->e_data, data, sizeof(uint32_t));

	return 0;
}

static struct rtnl_ematch_ops container_ops = {
	.eo_kind	= TCF_EM_CONTAINER,
	.eo_name	= "container",
	.eo_datalen	= sizeof(uint32_t),
	.eo_parse	= container_parse,
};

static void __init container_init(void)
{
	rtnl_ematch_register(&container_ops);
}

static void __exit container_exit(void)
{
	rtnl_ematch_unregister(&container_ops);
}
