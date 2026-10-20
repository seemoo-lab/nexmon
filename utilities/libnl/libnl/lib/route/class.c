/*
 * lib/route/class.c            Queueing Classes
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup tc
 * @defgroup class Queueing Classes
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/tc.h>
#include <netlink/route/class.h>
#include <netlink/route/class-modules.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/classifier.h>
#include <netlink/utils.h>

static struct nl_cache_ops rtnl_class_ops;

static int class_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
			    struct nlmsghdr *n, struct nl_parser_param *pp)
{
	int err;
	struct rtnl_class *class;
	struct rtnl_class_ops *cops;

	class = rtnl_class_alloc();
	if (!class) {
		err = -NLE_NOMEM;
		goto errout;
	}
	class->ce_msgtype = n->nlmsg_type;

	err = tca_msg_parser(n, (struct rtnl_tca *) class);
	if (err < 0)
		goto errout_free;

	cops = rtnl_class_lookup_ops(class);
	if (cops && cops->co_msg_parser) {
		err = cops->co_msg_parser(class);
		if (err < 0)
			goto errout_free;
	}

	err = pp->pp_cb((struct nl_object *) class, pp);
errout_free:
	rtnl_class_put(class);
errout:
	return err;
}

static int class_request_update(struct nl_cache *cache, struct nl_sock *sk)
{
	struct tcmsg tchdr = {
		.tcm_family = AF_UNSPEC,
		.tcm_ifindex = cache->c_iarg1,
	};

	return nl_send_simple(sk, RTM_GETTCLASS, NLM_F_DUMP, &tchdr,
			      sizeof(tchdr));
}

/**
 * @name Addition/Modification
 * @{
 */

static int class_build(struct rtnl_class *class, int type, int flags,
		       struct nl_msg **result)
{
	struct rtnl_class_ops *cops;
	int err;

	err = tca_build_msg((struct rtnl_tca *) class, type, flags, result);
	if (err < 0)
		return err;

	cops = rtnl_class_lookup_ops(class);
	if (cops && cops->co_get_opts) {
		struct nl_msg *opts;
		
		opts = cops->co_get_opts(class);
		if (opts) {
			err = nla_put_nested(*result, TCA_OPTIONS, opts);
			nlmsg_free(opts);
			if (err < 0)
				goto errout;
		}
	}

	return 0;
errout:
	nlmsg_free(*result);
	return err;
}

/**
 * Build a netlink message to add a new class
 * @arg class		class to add 
 * @arg flags		additional netlink message flags
 * @arg result		Pointer to store resulting message.
 *
 * Builds a new netlink message requesting an addition of a class.
 * The netlink message header isn't fully equipped with all relevant
 * fields and must be sent out via nl_send_auto_complete() or
 * supplemented as needed. 
 *
 * Common message flags
 *   - NLM_F_REPLACE - replace possibly existing classes
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_class_build_add_request(struct rtnl_class *class, int flags,
				 struct nl_msg **result)
{
	return class_build(class, RTM_NEWTCLASS, NLM_F_CREATE | flags, result);
}

/**
 * Add a new class
 * @arg sk		Netlink socket.
 * @arg class		class to delete
 * @arg flags		additional netlink message flags
 *
 * Builds a netlink message by calling rtnl_qdisc_build_add_request(),
 * sends the request to the kernel and waits for the next ACK to be
 * received and thus blocks until the request has been processed.
 *
 * Common message flags
 *   - NLM_F_REPLACE - replace possibly existing classes
 *
 * @return 0 on success or a negative error code
 */
int rtnl_class_add(struct nl_sock *sk, struct rtnl_class *class, int flags)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_class_build_add_request(class, flags, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

int rtnl_class_build_delete_request(struct rtnl_class *class,
									struct nl_msg **result)
{
	struct nl_msg *msg;
	struct tcmsg tchdr;
	int required = TCA_ATTR_IFINDEX | TCA_ATTR_PARENT;

	if ((class->ce_mask & required) != required)
		BUG();

	msg = nlmsg_alloc_simple(RTM_DELTCLASS, 0);
	if (!msg)
		return -NLE_NOMEM;

	tchdr.tcm_family = AF_UNSPEC;
	tchdr.tcm_handle = class->c_handle;
	tchdr.tcm_parent = class->c_parent;
	tchdr.tcm_ifindex = class->c_ifindex;
	if (nlmsg_append(msg, &tchdr, sizeof(tchdr), NLMSG_ALIGNTO) < 0) {
		nlmsg_free(msg);
		return -NLE_MSGSIZE;
	}

	*result = msg;
	return 0;
}

/**
 * Delete a class
 * @arg sk		Netlink socket.
 * @arg class		class to delete
 *
 * Builds a netlink message by calling rtnl_class_build_delete_request(),
 * sends the request to the kernel and waits for the ACK to be
 * received and thus blocks until the request has been processed.
 *
 * @return 0 on success or a negative error code
 */
int rtnl_class_delete(struct nl_sock *sk, struct rtnl_class *class)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_class_build_delete_request(class, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

/** @} */

/**
 * @name Cache Management
 * @{
 */

/**
 * Build a class cache including all classes attached to the specified interface
 * @arg sk		Netlink socket.
 * @arg ifindex		interface index of the link the classes are
 *                      attached to.
 *
 * Allocates a new cache, initializes it properly and updates it to
 * include all classes attached to the specified interface.
 *
 * @return The cache or NULL if an error has occured.
 */
int rtnl_class_alloc_cache(struct nl_sock *sk, int ifindex,
			   struct nl_cache **result)
{
	struct nl_cache * cache;
	int err;
	
	cache = nl_cache_alloc(&rtnl_class_ops);
	if (!cache)
		return -NLE_NOMEM;

	cache->c_iarg1 = ifindex;
	
	if (sk && (err = nl_cache_refill(sk, cache)) < 0) {
		nl_cache_free(cache);
		return err;
	}

	*result = cache;
	return 0;
}

/**
 * Look up class by its handle in the provided cache
 * @arg cache		class cache
 * @arg ifindex		interface the class is attached to
 * @arg handle		class handle
 * @return pointer to class inside the cache or NULL if no match was found.
 */
struct rtnl_class *rtnl_class_get(struct nl_cache *cache, int ifindex,
								  uint32_t handle)
{
	struct rtnl_class *class;
	
	if (cache->c_ops != &rtnl_class_ops)
		return NULL;

	nl_list_for_each_entry(class, &cache->c_items, ce_list) {
		if (class->c_handle == handle && class->c_ifindex == ifindex) {
			nl_object_get((struct nl_object *) class);
			return class;
		}
	}
	return NULL;
}

/** @} */

static struct nl_cache_ops rtnl_class_ops = {
	.co_name		= "route/class",
	.co_hdrsize		= sizeof(struct tcmsg),
	.co_msgtypes		= {
					{ RTM_NEWTCLASS, NL_ACT_NEW, "new" },
					{ RTM_DELTCLASS, NL_ACT_DEL, "del" },
					{ RTM_GETTCLASS, NL_ACT_GET, "get" },
					END_OF_MSGTYPES_LIST,
				  },
	.co_protocol		= NETLINK_ROUTE,
	.co_request_update	= &class_request_update,
	.co_msg_parser		= &class_msg_parser,
	.co_obj_ops		= &class_obj_ops,
};

static void __init class_init(void)
{
	nl_cache_mngt_register(&rtnl_class_ops);
}

static void __exit class_exit(void)
{
	nl_cache_mngt_unregister(&rtnl_class_ops);
}

/** @} */
