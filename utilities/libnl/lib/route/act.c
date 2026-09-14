/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Cong Wang <xiyou.wangcong@gmail.com>
 */

/**
 * @ingroup tc
 * @defgroup act Action
 * @{
 */

#include "nl-default.h"

#include <linux/gen_stats.h>

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/link.h>
#include <netlink/route/action.h>

#include "nl-route.h"
#include "tc-api.h"
#include "nl-priv-dynamic-core/object-api.h"
#include "nl-priv-dynamic-core/cache-api.h"
#include "nl-aux-route/nl-route.h"

static struct nl_object_ops act_obj_ops;
static struct nl_cache_ops rtnl_act_ops;

struct rtnl_act * rtnl_act_next(struct rtnl_act *act)
{
    if (act == NULL) {
        return NULL;
    }

    return act->a_next;
}

int rtnl_act_append(struct rtnl_act **head, struct rtnl_act *new)
{
	struct rtnl_act *p_act;
	int count = 1;

	if (*head == NULL) {
		*head = new;
		return 0;
	}

	p_act = *head;
	while (p_act->a_next) {
		++count;
		p_act = p_act->a_next;
	}

	if (count > TCA_ACT_MAX_PRIO)
		return -NLE_RANGE;

	p_act->a_next = new;
	return 0;
}

int rtnl_act_remove(struct rtnl_act **head, struct rtnl_act *act)
{
	struct rtnl_act *a, **ap;

	for (ap = head; (a = *ap) != NULL; ap = &a->a_next)
		if (a == act)
			break;
	if (a) {
		*ap = a->a_next;
		a->a_next = NULL;
		return 0;
	}

	return -NLE_OBJ_NOTFOUND;
}

static int rtnl_act_fill_one(struct nl_msg *msg, struct rtnl_act *act, int order)
{
	struct rtnl_tc *tc = TC_CAST(act);
	struct rtnl_tc_ops *ops;
	struct nlattr *nest;
	int err = -NLE_NOMEM;

	nest = nla_nest_start(msg, order);
	if (!nest)
		goto nla_put_failure;

	if (tc->ce_mask & TCA_ATTR_KIND)
	    NLA_PUT_STRING(msg, TCA_ACT_KIND, tc->tc_kind);

	ops = rtnl_tc_get_ops(tc);
	if (ops && (ops->to_msg_fill || ops->to_msg_fill_raw)) {
		struct nlattr *opts;
		void *data = rtnl_tc_data(tc);

		if (ops->to_msg_fill) {
			if (!(opts = nla_nest_start(msg, TCA_ACT_OPTIONS)))
				goto nla_put_failure;

			if ((err = ops->to_msg_fill(tc, data, msg)) < 0)
				goto nla_put_failure;

			nla_nest_end(msg, opts);
		} else if ((err = ops->to_msg_fill_raw(tc, data, msg)) < 0)
			goto nla_put_failure;
	}
	nla_nest_end(msg, nest);
	return 0;

nla_put_failure:
	return err;
}

int rtnl_act_fill(struct nl_msg *msg, int attrtype, struct rtnl_act *act)
{
	struct rtnl_act *p_act = act;
	struct nlattr *nest;
	int err, order = 0;

	nest = nla_nest_start(msg, attrtype);
	if (!nest)
		return -NLE_MSGSIZE;

	while (p_act) {
		err = rtnl_act_fill_one(msg, p_act, ++order);
		if (err < 0)
			return err;
		p_act = p_act->a_next;
	}

	nla_nest_end(msg, nest);
	return 0;
}

static int rtnl_act_msg_build(struct rtnl_act *act, int type, int flags,
		      struct nl_msg **result)
{
	struct nl_msg *msg;
	struct tcamsg tcahdr = {
		.tca_family = AF_UNSPEC,
	};
	int err = -NLE_MSGSIZE;

	msg = nlmsg_alloc_simple(type, flags);
	if (!msg)
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &tcahdr, sizeof(tcahdr), NLMSG_ALIGNTO) < 0)
		goto nla_put_failure;

	err = rtnl_act_fill(msg, TCA_ACT_TAB, act);
	if (err < 0)
		goto nla_put_failure;

	*result = msg;
	return 0;

nla_put_failure:
	nlmsg_free(msg);
	return err;
}

static int act_build(struct rtnl_act *act, int type, int flags,
		     struct nl_msg **result)
{
	int err;

	err = rtnl_act_msg_build(act, type, flags, result);
	if (err < 0)
		return err;
	return 0;
}

/**
 * @name Allocation/Freeing
 * @{
 */

struct rtnl_act *rtnl_act_alloc(void)
{
	struct rtnl_tc *tc;

	tc = TC_CAST(nl_object_alloc(&act_obj_ops));
	if (tc)
		tc->tc_type = RTNL_TC_TYPE_ACT;

	return (struct rtnl_act *) tc;
}

void rtnl_act_get(struct rtnl_act *act)
{
	nl_object_get(OBJ_CAST(act));
}

void rtnl_act_put(struct rtnl_act *act)
{
	nl_object_put((struct nl_object *) act);
}

/** @} */

/**
 * @name Addition/Modification/Deletion
 * @{
 */

/**
 * Build a netlink message requesting the addition of an action
 * @arg act		Action to add
 * @arg flags		Additional netlink message flags
 * @arg result		Pointer to store resulting netlink message
 *
 * The behaviour of this function is identical to rtnl_act_add() with
 * the exception that it will not send the message but return it int the
 * provided return pointer instead.
 *
 * @see rtnl_act_add()
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_act_build_add_request(struct rtnl_act *act, int flags,
			       struct nl_msg **result)
{
	return act_build(act, RTM_NEWACTION, flags, result);
}

/**
 * Add/Update action
 * @arg sk		Netlink socket
 * @arg act		Action to add/update
 * @arg flags		Additional netlink message flags
 *
 * Builds a \c RTM_NEWACTION netlink message requesting the addition
 * of a new action and sends the message to the kernel. The
 * configuration of the action is derived from the attributes of
 * the specified traffic class.
 *
 * The following flags may be specified:
 *  - \c NLM_F_CREATE:  Create action if it does not exist,
 *                      otherwise -NLE_OBJ_NOTFOUND is returned.
 *  - \c NLM_F_EXCL:    Return -NLE_EXISTS if an action with
 *                      matching handle exists already.
 *
 * Existing actions with matching handles will be updated, unless
 * the flag \c NLM_F_EXCL is specified. If no matching action
 * exists, it will be created if the flag \c NLM_F_CREATE is set,
 * otherwise the error -NLE_OBJ_NOTFOUND is returned.
 *
 * After sending, the function will wait for the ACK or an eventual
 * error message to be received and will therefore block until the
 * operation has been completed.
 *
 * @note Disabling auto-ack (nl_socket_disable_auto_ack()) will cause
 *       this function to return immediately after sending. In this case,
 *       it is the responsibility of the caller to handle any error
 *       messages returned.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_act_add(struct nl_sock *sk, struct rtnl_act *act, int flags)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_act_build_add_request(act, flags, &msg)) < 0)
		return err;

	return nl_send_sync(sk, msg);
}

/**
 * Build a netlink message to change action attributes
 * @arg act		Action to change
 * @arg flags		additional netlink message flags
 * @arg result		Pointer to store resulting message.
 *
 * Builds a new netlink message requesting a change of a neigh
 * attributes. The netlink message header isn't fully equipped with
 * all relevant fields and must thus be sent out via nl_send_auto_complete()
 * or supplemented as needed.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_act_build_change_request(struct rtnl_act *act, int flags,
				  struct nl_msg **result)
{
	return act_build(act, RTM_NEWACTION, NLM_F_REPLACE | flags, result);
}

/**
 * Change an action
 * @arg sk		Netlink socket.
 * @arg act		action to change
 * @arg flags		additional netlink message flags
 *
 * Builds a netlink message by calling rtnl_act_build_change_request(),
 * sends the request to the kernel and waits for the next ACK to be
 * received and thus blocks until the request has been processed.
 *
 * @return 0 on success or a negative error if an error occured.
 */
int rtnl_act_change(struct nl_sock *sk, struct rtnl_act *act, int flags)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_act_build_change_request(act, flags, &msg)) < 0)
		return err;

	return nl_send_sync(sk, msg);
}

/**
 * Build netlink message requesting the deletion of an action
 * @arg act		Action to delete
 * @arg flags		Additional netlink message flags
 * @arg result		Pointer to store resulting netlink message
 *
 * The behaviour of this function is identical to rtnl_act_delete() with
 * the exception that it will not send the message but return it in the
 * provided return pointer instead.
 *
 * @see rtnl_act_delete()
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_act_build_delete_request(struct rtnl_act *act, int flags,
				  struct nl_msg **result)
{
	return act_build(act, RTM_DELACTION, flags, result);
}

/**
 * Delete action
 * @arg sk		Netlink socket
 * @arg act		Action to delete
 * @arg flags		Additional netlink message flags
 *
 * Builds a \c RTM_DELACTION netlink message requesting the deletion
 * of an action and sends the message to the kernel.
 *
 * The message is constructed out of the following attributes:
 * - \c ifindex (required)
 * - \c prio (required)
 * - \c protocol (required)
 * - \c handle (required)
 * - \c parent (optional, if not specified parent equals root-qdisc)
 * - \c kind (optional, must match if provided)
 *
 * All other action attributes including all class type specific
 * attributes are ignored.
 *
 * After sending, the function will wait for the ACK or an eventual
 * error message to be received and will therefore block until the
 * operation has been completed.
 *
 * @note Disabling auto-ack (nl_socket_disable_auto_ack()) will cause
 *       this function to return immediately after sending. In this case,
 *       it is the responsibility of the caller to handle any error
 *       messages returned.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_act_delete(struct nl_sock *sk, struct rtnl_act *act, int flags)
{
	struct nl_msg *msg;
	int err;

	if ((err = rtnl_act_build_delete_request(act, flags, &msg)) < 0)
		return err;

	return nl_send_sync(sk, msg);
}

/** @} */

static void act_dump_line(struct rtnl_tc *tc, struct nl_dump_params *p)
{
}

void rtnl_act_put_all(struct rtnl_act **head)
{
	struct rtnl_act *curr, *next;

	curr = *head;
	while (curr) {
		next = curr->a_next;
		rtnl_act_put(curr);
		curr = next;
	}
	*head = NULL;
}

static struct nla_policy tc_act_stats_policy[TCA_STATS_MAX+1] = {
	[TCA_STATS_BASIC]    	= { .minlen = sizeof(struct gnet_stats_basic) },
	[TCA_STATS_QUEUE]    	= { .minlen = sizeof(struct gnet_stats_queue) },
	[TCA_STATS_RATE_EST] 	= { .minlen = sizeof(struct gnet_stats_rate_est) },
	[TCA_STATS_RATE_EST64] 	= { .minlen = sizeof(struct gnet_stats_rate_est64) },
};

int rtnl_act_parse(struct rtnl_act **head, struct nlattr *tb)
{
	_nl_auto_rtnl_act_all struct rtnl_act *tmp_head = NULL;
	struct rtnl_tc_ops *ops;
	struct nlattr *tb2[TCA_ACT_MAX + 1];
	struct nlattr *nla[TCA_ACT_MAX_PRIO + 1];
	char kind[TCKINDSIZ];
	int err, i;

	err = nla_parse(nla, TCA_ACT_MAX_PRIO, nla_data(tb),
			NLMSG_ALIGN(nla_len(tb)), NULL);
	if (err < 0)
		return err;

	for (i = 0; i < TCA_ACT_MAX_PRIO; i++) {
		_nl_auto_rtnl_act struct rtnl_act *act = NULL;
		struct rtnl_tc *tc;

		if (nla[i] == NULL)
			continue;

		act = rtnl_act_alloc();
		if (!act)
			return -NLE_NOMEM;

		tc = TC_CAST(act);
		err = nla_parse(tb2, TCA_ACT_MAX, nla_data(nla[i]),
				nla_len(nla[i]), NULL);
		if (err < 0)
			return err;

		if (tb2[TCA_ACT_KIND] == NULL)
			return -NLE_MISSING_ATTR;

		nla_strlcpy(kind, tb2[TCA_ACT_KIND], sizeof(kind));
		rtnl_tc_set_kind(tc, kind);

		if (tb2[TCA_ACT_OPTIONS]) {
			tc->tc_opts = nl_data_alloc_attr(tb2[TCA_ACT_OPTIONS]);
			if (!tc->tc_opts)
				return -NLE_NOMEM;
			tc->ce_mask |= TCA_ATTR_OPTS;
		}

		if (tb2[TCA_ACT_STATS]) {
			struct nlattr *tb3[TCA_STATS_MAX + 1];

			err = nla_parse_nested(tb3, TCA_STATS_MAX, tb2[TCA_ACT_STATS],
					       tc_act_stats_policy);
			if (err < 0)
				return err;

			if (tb3[TCA_STATS_BASIC]) {
				struct gnet_stats_basic bs;

				memcpy(&bs, nla_data(tb3[TCA_STATS_BASIC]),
				       sizeof(bs));
				tc->tc_stats[RTNL_TC_BYTES] = bs.bytes;
				tc->tc_stats[RTNL_TC_PACKETS] = bs.packets;
			}
			if (tb3[TCA_STATS_RATE_EST64]) {
				struct gnet_stats_rate_est64 re;

				memcpy(&re, nla_data(tb3[TCA_STATS_RATE_EST64]),
				       sizeof(re));
				tc->tc_stats[RTNL_TC_RATE_BPS] = re.bps;
				tc->tc_stats[RTNL_TC_RATE_PPS] = re.pps;
			} else if (tb3[TCA_STATS_RATE_EST]) {
				struct gnet_stats_rate_est *re;

				re = nla_data(tb3[TCA_STATS_RATE_EST]);
				tc->tc_stats[RTNL_TC_RATE_BPS] = re->bps;
				tc->tc_stats[RTNL_TC_RATE_PPS] = re->pps;
			}
			if (tb3[TCA_STATS_QUEUE]) {
				struct gnet_stats_queue *q;

				q = nla_data(tb3[TCA_STATS_QUEUE]);
				tc->tc_stats[RTNL_TC_DROPS] = q->drops;
				tc->tc_stats[RTNL_TC_OVERLIMITS] = q->overlimits;
			}
		}

		ops = rtnl_tc_get_ops(tc);
		if (ops && ops->to_msg_parser) {
			void *data = rtnl_tc_data(tc);

			if (!data)
				return -NLE_NOMEM;

			err = ops->to_msg_parser(tc, data);
			if (err < 0)
				return err;
		}
		err = _rtnl_act_append_take(&tmp_head, _nl_steal_pointer(&act));
		if (err < 0)
			return err;
	}

	*head = _nl_steal_pointer(&tmp_head);
	return 0;
}

static int rtnl_act_msg_parse(struct nlmsghdr *n, struct rtnl_act **act)
{
	struct rtnl_tc *tc = TC_CAST(*act);
	struct nl_cache *link_cache;
	struct nlattr *tb[TCAA_MAX + 1];
	struct tcamsg *tm;
	int err;

	tc->ce_msgtype = n->nlmsg_type;

	err = nlmsg_parse(n, sizeof(*tm), tb, TCAA_MAX, NULL);
	if (err < 0)
		return err;

	tm = nlmsg_data(n);
	tc->tc_family  = tm->tca_family;

	if (tb[TCA_ACT_TAB] == NULL)
		return -NLE_MISSING_ATTR;

	err = rtnl_act_parse(act, tb[TCA_ACT_TAB]);
	if (err < 0)
		return err;

	if ((link_cache = __nl_cache_mngt_require("route/link"))) {
		struct rtnl_link *link;

		if ((link = rtnl_link_get(link_cache, tc->tc_ifindex))) {
			rtnl_tc_set_link(tc, link);

			/* rtnl_tc_set_link incs refcnt */
			rtnl_link_put(link);
		}
	}

	return 0;
}
static int act_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
			  struct nlmsghdr *nlh, struct nl_parser_param *pp)
{
	struct rtnl_act *act, *p_act;
	int err;

	if (!(act = rtnl_act_alloc()))
		return -NLE_NOMEM;

	if ((err = rtnl_act_msg_parse(nlh, &act)) < 0)
		goto errout;

	p_act = act;
	while(p_act) {
		err = pp->pp_cb(OBJ_CAST(act), pp);
		if (err) {
			if (err > 0) {
				_nl_assert_not_reached();
				err = -NLE_FAILURE;
			}
			break;
		}
		p_act = p_act->a_next;
	}
errout:
	rtnl_act_put(act);

	return err;
}

static int act_request_update(struct nl_cache *cache, struct nl_sock *sk)
{
	struct tcamsg tcahdr = {
		.tca_family = AF_UNSPEC,
	};

	return nl_send_simple(sk, RTM_GETACTION, NLM_F_DUMP, &tcahdr,
			      sizeof(tcahdr));
}

static struct rtnl_tc_type_ops act_ops = {
	.tt_type		= RTNL_TC_TYPE_ACT,
	.tt_dump_prefix		= "act",
	.tt_dump = {
		[NL_DUMP_LINE]	= act_dump_line,
	},
};

static struct nl_cache_ops rtnl_act_ops = {
	.co_name		= "route/act",
	.co_hdrsize		= sizeof(struct tcmsg),
	.co_msgtypes		= {
					{ RTM_NEWACTION, NL_ACT_NEW, "new" },
					{ RTM_DELACTION, NL_ACT_DEL, "del" },
					{ RTM_GETACTION, NL_ACT_GET, "get" },
					END_OF_MSGTYPES_LIST,
				  },
	.co_protocol		= NETLINK_ROUTE,
	.co_request_update	= act_request_update,
	.co_msg_parser		= act_msg_parser,
	.co_obj_ops		= &act_obj_ops,
};

static struct nl_object_ops act_obj_ops = {
	.oo_name		= "route/act",
	.oo_size		= sizeof(struct rtnl_act),
	.oo_free_data		= rtnl_tc_free_data,
	.oo_clone		= rtnl_tc_clone,
	.oo_dump = {
	    [NL_DUMP_LINE]	= rtnl_tc_dump_line,
	    [NL_DUMP_DETAILS]	= rtnl_tc_dump_details,
	    [NL_DUMP_STATS]	= rtnl_tc_dump_stats,
	},
	.oo_compare		= rtnl_tc_compare,
	.oo_id_attrs		= (TCA_ATTR_IFINDEX | TCA_ATTR_HANDLE),
};

static void _nl_init act_init(void)
{
	rtnl_tc_type_register(&act_ops);
	nl_cache_mngt_register(&rtnl_act_ops);
}

static void _nl_exit act_exit(void)
{
	nl_cache_mngt_unregister(&rtnl_act_ops);
	rtnl_tc_type_unregister(&act_ops);
}

/** @} */
