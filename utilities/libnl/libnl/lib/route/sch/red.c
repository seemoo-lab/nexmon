/*
 * lib/route/sch/red.c		RED Qdisc
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup qdisc_api
 * @defgroup red Random Early Detection (RED)
 * @brief
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc-modules.h>
#include <netlink/route/sch/red.h>

/** @cond SKIP */
#define RED_ATTR_LIMIT		0x01
#define RED_ATTR_QTH_MIN	0x02
#define RED_ATTR_QTH_MAX	0x04
#define RED_ATTR_FLAGS		0x08
#define RED_ATTR_WLOG		0x10
#define RED_ATTR_PLOG		0x20
#define RED_ATTR_SCELL_LOG	0x40
/** @endcond */

static inline struct rtnl_red *red_qdisc(struct rtnl_qdisc *qdisc)
{
	return (struct rtnl_red *) qdisc->q_subdata;
}

static inline struct rtnl_red *red_alloc(struct rtnl_qdisc *qdisc)
{
	if (!qdisc->q_subdata)
		qdisc->q_subdata = calloc(1, sizeof(struct rtnl_red));

	return red_qdisc(qdisc);
}

static struct nla_policy red_policy[TCA_RED_MAX+1] = {
	[TCA_RED_PARMS]		= { .minlen = sizeof(struct tc_red_qopt) },
};

static int red_msg_parser(struct rtnl_qdisc *qdisc)
{
	struct nlattr *tb[TCA_RED_MAX+1];
	struct rtnl_red *red;
	struct tc_red_qopt *opts;
	int err;

	if (!(qdisc->ce_mask & TCA_ATTR_OPTS))
		return 0;

	err = tca_parse(tb, TCA_RED_MAX, (struct rtnl_tca *) qdisc, red_policy);
	if (err < 0)
		return err;

	if (!tb[TCA_RED_PARMS])
		return -NLE_MISSING_ATTR;

	red = red_alloc(qdisc);
	if (!red)
		return -NLE_NOMEM;

	opts = nla_data(tb[TCA_RED_PARMS]);

	red->qr_limit = opts->limit;
	red->qr_qth_min = opts->qth_min;
	red->qr_qth_max = opts->qth_max;
	red->qr_flags = opts->flags;
	red->qr_wlog = opts->Wlog;
	red->qr_plog = opts->Plog;
	red->qr_scell_log = opts->Scell_log;

	red->qr_mask = (RED_ATTR_LIMIT | RED_ATTR_QTH_MIN | RED_ATTR_QTH_MAX |
			RED_ATTR_FLAGS | RED_ATTR_WLOG | RED_ATTR_PLOG |
			RED_ATTR_SCELL_LOG);

	return 0;
}

static void red_dump_line(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	struct rtnl_red *red = red_qdisc(qdisc);

	if (red) {
		/* XXX: limit, min, max, flags */
	}
}

static void red_dump_details(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	struct rtnl_red *red = red_qdisc(qdisc);

	if (red) {
		/* XXX: wlog, plog, scell_log */
	}
}

static void red_dump_stats(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	struct rtnl_red *red = red_qdisc(qdisc);

	if (red) {
		/* XXX: xstats */
	}
}

static struct nl_msg *red_get_opts(struct rtnl_qdisc *qdisc)
{
	struct rtnl_red *red;
	struct nl_msg *msg;

	red = red_qdisc(qdisc);
	if (!red)
		return NULL;

	msg = nlmsg_alloc();
	if (!msg)
		goto errout;

#if 0
	memset(&opts, 0, sizeof(opts));
	opts.quantum = sfq->qs_quantum;
	opts.perturb_period = sfq->qs_perturb;
	opts.limit = sfq->qs_limit;

	if (nlmsg_append(msg, &opts, sizeof(opts), NL_DONTPAD) < 0)
		goto errout;
#endif

	return msg;
errout:
	nlmsg_free(msg);
	return NULL;
}

/**
 * @name Attribute Access
 * @{
 */

/**
 * Set limit of RED qdisc.
 * @arg qdisc		RED qdisc to be modified.
 * @arg limit		New limit in number of packets.
 * @return 0 on success or a negative error code.
 */
int rtnl_red_set_limit(struct rtnl_qdisc *qdisc, int limit)
{
	struct rtnl_red *red;

	red = red_alloc(qdisc);
	if (!red)
		return -NLE_NOMEM;

	red->qr_limit = limit;
	red->qr_mask |= RED_ATTR_LIMIT;

	return 0;
}

/**
 * Get limit of RED qdisc.
 * @arg qdisc		RED qdisc.
 * @return Limit or a negative error code.
 */
int rtnl_red_get_limit(struct rtnl_qdisc *qdisc)
{
	struct rtnl_red *red;

	red = red_qdisc(qdisc);
	if (red && (red->qr_mask & RED_ATTR_LIMIT))
		return red->qr_limit;
	else
		return -NLE_NOATTR;
}

/** @} */

static struct rtnl_qdisc_ops red_ops = {
	.qo_kind		= "red",
	.qo_msg_parser		= red_msg_parser,
	.qo_dump = {
	    [NL_DUMP_LINE]	= red_dump_line,
	    [NL_DUMP_DETAILS]	= red_dump_details,
	    [NL_DUMP_STATS]	= red_dump_stats,
	},
	.qo_get_opts		= red_get_opts,
};

static void __init red_init(void)
{
	rtnl_qdisc_register(&red_ops);
}

static void __exit red_exit(void)
{
	rtnl_qdisc_unregister(&red_ops);
}

/** @} */
