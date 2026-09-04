/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup rtnl
 * @defgroup route Routing
 * @brief
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/data.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/route.h>
#include <netlink/route/link.h>

#include "nl-route.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-priv-dynamic-core/cache-api.h"
#include "nl-aux-route/nl-route.h"

static struct nl_cache_ops rtnl_route_ops;

static int route_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
			    struct nlmsghdr *nlh, struct nl_parser_param *pp)
{
	struct rtnl_route *route;
	int err;

	if ((err = rtnl_route_parse(nlh, &route)) < 0)
		return err;

	err = pp->pp_cb((struct nl_object *) route, pp);

	rtnl_route_put(route);
	return err;
}

static int route_request_update(struct nl_cache *c, struct nl_sock *h)
{
	struct rtmsg rhdr = {
		.rtm_family = c->c_iarg1,
	};

	if (c->c_iarg2 & ROUTE_CACHE_CONTENT)
		rhdr.rtm_flags |= RTM_F_CLONED;

	return nl_send_simple(h, RTM_GETROUTE, NLM_F_DUMP, &rhdr, sizeof(rhdr));
}

/**
 * @name Cache Management
 * @{
 */

/**
 * Build a route cache holding all routes currently configured in the kernel
 * @arg sk		Netlink socket.
 * @arg family		Address family of routes to cover or AF_UNSPEC
 * @arg flags		Flags
 * @arg result		Result pointer
 *
 * Allocates a new cache, initializes it properly and updates it to
 * contain all routes currently configured in the kernel.
 *
 * Valid flags:
 *   * ROUTE_CACHE_CONTENT - Cache will contain contents of routing cache
 *                           instead of actual routes.
 *
 * @note The caller is responsible for destroying and freeing the
 *       cache after using it.
 * @return 0 on success or a negative error code.
 */
int rtnl_route_alloc_cache(struct nl_sock *sk, int family, int flags,
			   struct nl_cache **result)
{
	struct nl_cache *cache;
	int err;

	if (!(cache = nl_cache_alloc(&rtnl_route_ops)))
		return -NLE_NOMEM;

	cache->c_iarg1 = family;
	cache->c_iarg2 = flags;

	if (sk && (err = nl_cache_refill(sk, cache)) < 0) {
		free(cache);
		return err;
	}

	*result = cache;
	return 0;
}

/** @} */

/**
 * @name Route Addition
 * @{
 */

static int build_route_msg(struct rtnl_route *tmpl, int cmd, int flags,
			   struct nl_msg **result)
{
	struct nl_msg *msg;
	int err;

	if (!(msg = nlmsg_alloc_simple(cmd, flags)))
		return -NLE_NOMEM;

	if ((err = rtnl_route_build_msg(msg, tmpl)) < 0) {
		nlmsg_free(msg);
		return err;
	}

	*result = msg;
	return 0;
}

int rtnl_route_build_add_request(struct rtnl_route *tmpl, int flags,
				 struct nl_msg **result)
{
	return build_route_msg(tmpl, RTM_NEWROUTE, NLM_F_CREATE | flags,
			       result);
}

int rtnl_route_lookup(struct nl_sock *sk, struct nl_addr *dst,
                      struct rtnl_route **result)
{
	_nl_auto_nl_msg struct nl_msg *msg = NULL;
	_nl_auto_rtnl_route struct rtnl_route *tmpl = NULL;
	struct nl_object *obj;
	int err;

	tmpl = rtnl_route_alloc();
	rtnl_route_set_dst(tmpl, dst);
	err = build_route_msg(tmpl, RTM_GETROUTE, 0, &msg);
	if (err < 0)
		return err;

	err = nl_send_auto(sk, msg);
	if (err < 0)
		return err;

	if ((err = nl_pickup(sk, route_msg_parser, &obj)) < 0)
		return err;

	*result = (struct rtnl_route *)obj;
	wait_for_ack(sk);
	return 0;
}

int rtnl_route_add(struct nl_sock *sk, struct rtnl_route *route, int flags)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_route_build_add_request(route, flags, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

int rtnl_route_build_del_request(struct rtnl_route *tmpl, int flags,
				 struct nl_msg **result)
{
	return build_route_msg(tmpl, RTM_DELROUTE, flags, result);
}

int rtnl_route_delete(struct nl_sock *sk, struct rtnl_route *route, int flags)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_route_build_del_request(route, flags, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

/** @} */

static struct nl_af_group route_groups[] = {
	{ AF_INET,	RTNLGRP_IPV4_ROUTE },
	{ AF_INET6,	RTNLGRP_IPV6_ROUTE },
	{ AF_MPLS,	RTNLGRP_MPLS_ROUTE },
	{ AF_DECnet,	RTNLGRP_DECnet_ROUTE },
	{ END_OF_GROUP_LIST },
};

static struct nl_cache_ops rtnl_route_ops = {
	.co_name		= "route/route",
	.co_hdrsize		= sizeof(struct rtmsg),
	.co_msgtypes		= {
					{ RTM_NEWROUTE, NL_ACT_NEW, "new" },
					{ RTM_DELROUTE, NL_ACT_DEL, "del" },
					{ RTM_GETROUTE, NL_ACT_GET, "get" },
					END_OF_MSGTYPES_LIST,
				  },
	.co_protocol		= NETLINK_ROUTE,
	.co_groups		= route_groups,
	.co_request_update	= route_request_update,
	.co_msg_parser		= route_msg_parser,
	.co_obj_ops		= &route_obj_ops,
};

static void _nl_init route_init(void)
{
	nl_cache_mngt_register(&rtnl_route_ops);
}

static void _nl_exit route_exit(void)
{
	nl_cache_mngt_unregister(&rtnl_route_ops);
}

/** @} */
