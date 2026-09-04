/*
 * lib/route/qdisc.c            Queueing Disciplines
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
 * @defgroup qdisc Queueing Disciplines
 *
 * @par Qdisc Handles
 * In general, qdiscs are identified by the major part of a traffic control
 * handle (the upper 16 bits). A few special values exist though:
 *  - \c TC_H_ROOT: root qdisc (directly attached to the device)
 *  - \c TC_H_INGRESS: ingress qdisc (directly attached to the device)
 *  - \c TC_H_UNSPEC: unspecified qdisc (no reference)
 *
 * @par 1) Adding a Qdisc
 * @code
 * // Allocate a new empty qdisc to be filled out
 * struct rtnl_qdisc *qdisc = rtnl_qdisc_alloc();
 *
 * // ... specify the kind of the Qdisc
 * rtnl_qdisc_set_kind(qdisc, "pfifo");
 *
 * // Specify the device the qdisc should be attached to
 * rtnl_qdisc_set_ifindex(qdisc, ifindex);
 *
 * // ... specify the parent qdisc
 * rtnl_qdisc_set_parent(qdisc, TC_H_ROOT);
 *
 * // Specifying the handle is not required but makes reidentifying easier
 * // and may help to avoid adding a qdisc twice.
 * rtnl_qdisc_set_handle(qdisc, 0x000A0000);
 *
 * // Now on to specify the qdisc specific options, see the relevant qdisc
 * // modules for documentation, in this example we set the upper limit of
 * // the packet fifo qdisc to 64
 * rtnl_qdisc_fifo_set_limit(qdisc, 64);
 *
 * rtnl_qdisc_add(handle, qdisc, NLM_R_REPLACE);
 *
 * // Free up the memory
 * rtnl_qdisc_put(qdisc);
 * @endcode
 *
 * @par 2) Deleting a Qdisc
 * @code
 * // Allocate a new empty qdisc to be filled out with the parameters
 * // specifying the qdisc to be deleted. Alternatively a fully equiped
 * // Qdisc object from a cache can be used.
 * struct rtnl_qdisc *qdisc = rtnl_qdisc_alloc();
 *
 * // The interface index of the device the qdisc is on and the parent handle
 * // are the least required fields to be filled out.
 * // Note: Specify TC_H_ROOT or TC_H_INGRESS as parent handle to delete the
 * //       root respectively root ingress qdisc.
 * rtnl_qdisc_set_ifindex(qdisc, ifindex);
 * rtnl_qdisc_set_parent(qdisc, parent_handle);
 *
 * // If required for identification, the handle can be specified as well.
 * rtnl_qdisc_set_handle(qdisc, qdisc_handle);
 *
 * // Not required but maybe helpful as sanity check, the kind of the qdisc
 * // can be specified to avoid mistakes.
 * rtnl_qdisc_set_kind(qdisc, "pfifo");
 *
 * // Finally delete the qdisc with rtnl_qdisc_delete(), alternatively
 * // rtnl_qdisc_build_delete_request() can be invoked to generate an
 * // appropritate netlink message to send out.
 * rtnl_qdisc_delete(handle, qdisc);
 *
 * // Free up the memory
 * rtnl_qdisc_put(qdisc);
 * @endcode
 *
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>
#include <netlink/route/classifier.h>
#include <netlink/route/qdisc-modules.h>

static struct nl_cache_ops rtnl_qdisc_ops;

static int qdisc_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
			    struct nlmsghdr *n, struct nl_parser_param *pp)
{
	int err;
	struct rtnl_qdisc *qdisc;
	struct rtnl_qdisc_ops *qops;

	qdisc = rtnl_qdisc_alloc();
	if (!qdisc) {
		err = -NLE_NOMEM;
		goto errout;
	}

	qdisc->ce_msgtype = n->nlmsg_type;

	err = tca_msg_parser(n, (struct rtnl_tca *) qdisc);
	if (err < 0)
		goto errout_free;

	qops = rtnl_qdisc_lookup_ops(qdisc);
	if (qops && qops->qo_msg_parser) {
		err = qops->qo_msg_parser(qdisc);
		if (err < 0)
			goto errout_free;
	}

	err = pp->pp_cb((struct nl_object *) qdisc, pp);
errout_free:
	rtnl_qdisc_put(qdisc);
errout:
	return err;
}

static int qdisc_request_update(struct nl_cache *c, struct nl_sock *sk)
{
	struct tcmsg tchdr = {
		.tcm_family = AF_UNSPEC,
		.tcm_ifindex = c->c_iarg1,
	};

	return nl_send_simple(sk, RTM_GETQDISC, NLM_F_DUMP, &tchdr,
			      sizeof(tchdr));
}

/**
 * @name QDisc Addition
 * @{
 */

static int qdisc_build(struct rtnl_qdisc *qdisc, int type, int flags,
		       struct nl_msg **result)
{
	struct rtnl_qdisc_ops *qops;
	int err;

	err = tca_build_msg((struct rtnl_tca *) qdisc, type, flags, result);
	if (err < 0)
		return err;

	qops = rtnl_qdisc_lookup_ops(qdisc);
	if (qops && qops->qo_get_opts) {
		struct nl_msg *opts;
		
		opts = qops->qo_get_opts(qdisc);
		if (opts) {
			err = nla_put_nested(*result, TCA_OPTIONS, opts);
			nlmsg_free(opts);
			if (err < 0)
				goto errout;
		}
	}
	/* Some qdiscs don't accept properly nested messages (e.g. netem). To
	 * accomodate for this, they can complete the message themselves.
	 */		
	else if (qops && qops->qo_build_msg) {
		err = qops->qo_build_msg(qdisc, *result);
		if (err < 0)
			goto errout;
	}

	return 0;
errout:
	nlmsg_free(*result);

	return err;
}

/**
 * Build a netlink message to add a new qdisc
 * @arg qdisc		qdisc to add 
 * @arg flags		additional netlink message flags
 * @arg result		Pointer to store resulting message.
 *
 * Builds a new netlink message requesting an addition of a qdisc.
 * The netlink message header isn't fully equipped with all relevant
 * fields and must be sent out via nl_send_auto_complete() or
 * supplemented as needed. 
 *
 * Common message flags used:
 *  - NLM_F_REPLACE - replace a potential existing qdisc
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_build_add_request(struct rtnl_qdisc *qdisc, int flags,
				 struct nl_msg **result)
{
	return qdisc_build(qdisc, RTM_NEWQDISC, NLM_F_CREATE | flags, result);
}

/**
 * Add a new qdisc
 * @arg sk		Netlink socket.
 * @arg qdisc		qdisc to delete
 * @arg flags		additional netlink message flags
 *
 * Builds a netlink message by calling rtnl_qdisc_build_add_request(),
 * sends the request to the kernel and waits for the ACK to be
 * received and thus blocks until the request has been processed.
 *
 * Common message flags used:
 *  - NLM_F_REPLACE - replace a potential existing qdisc
 *
 * @return 0 on success or a negative error code
 */
int rtnl_qdisc_add(struct nl_sock *sk, struct rtnl_qdisc *qdisc,
		   int flags)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_qdisc_build_add_request(qdisc, flags, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

/** @} */

/**
 * @name QDisc Modification
 * @{
 */

/**
 * Build a netlink message to change attributes of a existing qdisc
 * @arg qdisc		qdisc to change
 * @arg new		new qdisc attributes
 * @arg result		Pointer to store resulting message.
 *
 * Builds a new netlink message requesting an change of qdisc
 * attributes. The netlink message header isn't fully equipped
 * with all relevant fields and must be sent out via
 * nl_send_auto_complete() or supplemented as needed. 
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_build_change_request(struct rtnl_qdisc *qdisc,
				    struct rtnl_qdisc *new,
				    struct nl_msg **result)
{
	return qdisc_build(qdisc, RTM_NEWQDISC, NLM_F_REPLACE, result);
}

/**
 * Change attributes of a qdisc
 * @arg sk		Netlink socket.
 * @arg qdisc		qdisc to change
 * @arg new		new qdisc attributes
 *
 * Builds a netlink message by calling rtnl_qdisc_build_change_request(),
 * sends the request to the kernel and waits for the ACK to be
 * received and thus blocks until the request has been processed.
 *
 * @return 0 on success or a negative error code
 */
int rtnl_qdisc_change(struct nl_sock *sk, struct rtnl_qdisc *qdisc,
		      struct rtnl_qdisc *new)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_qdisc_build_change_request(qdisc, new, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

/** @} */

/**
 * @name QDisc Deletion
 * @{
 */

/**
 * Build a netlink request message to delete a qdisc
 * @arg qdisc		qdisc to delete
 * @arg result		Pointer to store resulting message.
 *
 * Builds a new netlink message requesting a deletion of a qdisc.
 * The netlink message header isn't fully equipped with all relevant
 * fields and must thus be sent out via nl_send_auto_complete()
 * or supplemented as needed.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_build_delete_request(struct rtnl_qdisc *qdisc,
				    struct nl_msg **result)
{
	struct nl_msg *msg;
	struct tcmsg tchdr;
	int required = TCA_ATTR_IFINDEX | TCA_ATTR_PARENT;

	if ((qdisc->ce_mask & required) != required)
		BUG();

	msg = nlmsg_alloc_simple(RTM_DELQDISC, 0);
	if (!msg)
		return -NLE_NOMEM;

	tchdr.tcm_family = AF_UNSPEC;
	tchdr.tcm_handle = qdisc->q_handle;
	tchdr.tcm_parent = qdisc->q_parent;
	tchdr.tcm_ifindex = qdisc->q_ifindex;
	if (nlmsg_append(msg, &tchdr, sizeof(tchdr), NLMSG_ALIGNTO) < 0) {
		nlmsg_free(msg);
		return -NLE_MSGSIZE;
	}

	*result = msg;
	return 0;
}

/**
 * Delete a qdisc
 * @arg sk		Netlink socket.
 * @arg qdisc		qdisc to delete
 *
 * Builds a netlink message by calling rtnl_qdisc_build_delete_request(),
 * sends the request to the kernel and waits for the ACK to be
 * received and thus blocks until the request has been processed.
 *
 * @return 0 on success or a negative error code
 */
int rtnl_qdisc_delete(struct nl_sock *sk, struct rtnl_qdisc *qdisc)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_qdisc_build_delete_request(qdisc, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

/** @} */

/**
 * @name Qdisc Cache Management
 * @{
 */

/**
 * Build a qdisc cache including all qdiscs currently configured in
 * the kernel
 * @arg sk		Netlink socket.
 * @arg result		Pointer to store resulting message.
 *
 * Allocates a new cache, initializes it properly and updates it to
 * include all qdiscs currently configured in the kernel.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_alloc_cache(struct nl_sock *sk, struct nl_cache **result)
{
	return nl_cache_alloc_and_fill(&rtnl_qdisc_ops, sk, result);
}

/**
 * Look up qdisc by its parent in the provided cache
 * @arg cache		qdisc cache
 * @arg ifindex		interface the qdisc is attached to
 * @arg parent		parent handle
 * @return pointer to qdisc inside the cache or NULL if no match was found.
 */
struct rtnl_qdisc * rtnl_qdisc_get_by_parent(struct nl_cache *cache,
					     int ifindex, uint32_t parent)
{
	struct rtnl_qdisc *q;

	if (cache->c_ops != &rtnl_qdisc_ops)
		return NULL;

	nl_list_for_each_entry(q, &cache->c_items, ce_list) {
		if (q->q_parent == parent && q->q_ifindex == ifindex) {
			nl_object_get((struct nl_object *) q);
			return q;
		}
	}

	return NULL;
}

/**
 * Look up qdisc by its handle in the provided cache
 * @arg cache		qdisc cache
 * @arg ifindex		interface the qdisc is attached to
 * @arg handle		qdisc handle
 * @return pointer to qdisc inside the cache or NULL if no match was found.
 */
struct rtnl_qdisc * rtnl_qdisc_get(struct nl_cache *cache,
				   int ifindex, uint32_t handle)
{
	struct rtnl_qdisc *q;

	if (cache->c_ops != &rtnl_qdisc_ops)
		return NULL;

	nl_list_for_each_entry(q, &cache->c_items, ce_list) {
		if (q->q_handle == handle && q->q_ifindex == ifindex) {
			nl_object_get((struct nl_object *) q);
			return q;
		}
	}

	return NULL;
}

/** @} */

static struct nl_cache_ops rtnl_qdisc_ops = {
	.co_name		= "route/qdisc",
	.co_hdrsize		= sizeof(struct tcmsg),
	.co_msgtypes		= {
					{ RTM_NEWQDISC, NL_ACT_NEW, "new" },
					{ RTM_DELQDISC, NL_ACT_DEL, "del" },
					{ RTM_GETQDISC, NL_ACT_GET, "get" },
					END_OF_MSGTYPES_LIST,
				  },
	.co_protocol		= NETLINK_ROUTE,
	.co_request_update	= qdisc_request_update,
	.co_msg_parser		= qdisc_msg_parser,
	.co_obj_ops		= &qdisc_obj_ops,
};

static void __init qdisc_init(void)
{
	nl_cache_mngt_register(&rtnl_qdisc_ops);
}

static void __exit qdisc_exit(void)
{
	nl_cache_mngt_unregister(&rtnl_qdisc_ops);
}

/** @} */
