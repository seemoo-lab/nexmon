/*
 * lib/route/sch/fifo.c		(p|b)fifo
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
 * @defgroup fifo Packet/Bytes FIFO (pfifo/bfifo)
 * @brief
 *
 * The FIFO qdisc comes in two flavours:
 * @par bfifo (Byte FIFO)
 * Allows enqueuing until the currently queued volume in bytes exceeds
 * the configured limit.backlog contains currently enqueued volume in bytes.
 *
 * @par pfifo (Packet FIFO)
 * Allows enquueing until the currently queued number of packets
 * exceeds the configured limit.
 *
 * The configuration is exactly the same, the decision which of
 * the two variations is going to be used is made based on the
 * kind of the qdisc (rtnl_qdisc_set_kind()).
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc-modules.h>
#include <netlink/route/sch/fifo.h>
#include <netlink/utils.h>

/** @cond SKIP */
#define SCH_FIFO_ATTR_LIMIT 1
/** @endcond */

static inline struct rtnl_fifo *fifo_qdisc(struct rtnl_qdisc *qdisc)
{
	return (struct rtnl_fifo *) qdisc->q_subdata;
}

static inline struct rtnl_fifo *fifo_alloc(struct rtnl_qdisc *qdisc)
{
	if (!qdisc->q_subdata)
		qdisc->q_subdata = calloc(1, sizeof(struct rtnl_fifo));

	return fifo_qdisc(qdisc);
}

static int fifo_msg_parser(struct rtnl_qdisc *qdisc)
{
	struct rtnl_fifo *fifo;
	struct tc_fifo_qopt *opt;

	if (qdisc->q_opts->d_size < sizeof(struct tc_fifo_qopt))
		return -NLE_INVAL;

	fifo = fifo_alloc(qdisc);
	if (!fifo)
		return -NLE_NOMEM;

	opt = (struct tc_fifo_qopt *) qdisc->q_opts->d_data;
	fifo->qf_limit = opt->limit;
	fifo->qf_mask = SCH_FIFO_ATTR_LIMIT;

	return 0;
}

static void fifo_free_data(struct rtnl_qdisc *qdisc)
{
	free(qdisc->q_subdata);
}

static void pfifo_dump_line(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	struct rtnl_fifo *fifo = fifo_qdisc(qdisc);

	if (fifo)
		nl_dump(p, " limit %u packets", fifo->qf_limit);
}

static void bfifo_dump_line(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	struct rtnl_fifo *fifo = fifo_qdisc(qdisc);

	if (fifo) {
		char *unit;
		double r;

		r = nl_cancel_down_bytes(fifo->qf_limit, &unit);
		nl_dump(p, " limit %.1f%s", r, unit);
	}
}

static struct nl_msg *fifo_get_opts(struct rtnl_qdisc *qdisc)
{
	struct rtnl_fifo *fifo;
	struct tc_fifo_qopt opts;
	struct nl_msg *msg;

	fifo = fifo_qdisc(qdisc);
	if (!fifo || !(fifo->qf_mask & SCH_FIFO_ATTR_LIMIT))
		return NULL;

	msg = nlmsg_alloc();
	if (!msg)
		goto errout;

	memset(&opts, 0, sizeof(opts));
	opts.limit = fifo->qf_limit;

	if (nlmsg_append(msg, &opts, sizeof(opts), NL_DONTPAD) < 0)
		goto errout;

	return msg;
errout:
	nlmsg_free(msg);
	return NULL;
}

/**
 * @name Attribute Modification
 * @{
 */

/**
 * Set limit of FIFO qdisc.
 * @arg qdisc		FIFO qdisc to be modified.
 * @arg limit		New limit.
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_fifo_set_limit(struct rtnl_qdisc *qdisc, int limit)
{
	struct rtnl_fifo *fifo;
	
	fifo = fifo_alloc(qdisc);
	if (!fifo)
		return -NLE_NOMEM;
		
	fifo->qf_limit = limit;
	fifo->qf_mask |= SCH_FIFO_ATTR_LIMIT;

	return 0;
}

/**
 * Get limit of a FIFO qdisc.
 * @arg qdisc		FIFO qdisc.
 * @return Numeric limit or a negative error code.
 */
int rtnl_qdisc_fifo_get_limit(struct rtnl_qdisc *qdisc)
{
	struct rtnl_fifo *fifo;
	
	fifo = fifo_qdisc(qdisc);
	if (fifo && fifo->qf_mask & SCH_FIFO_ATTR_LIMIT)
		return fifo->qf_limit;
	else
		return -NLE_NOATTR;
}

/** @} */

static struct rtnl_qdisc_ops pfifo_ops = {
	.qo_kind		= "pfifo",
	.qo_msg_parser		= fifo_msg_parser,
	.qo_free_data		= fifo_free_data,
	.qo_dump[NL_DUMP_LINE]	= pfifo_dump_line,
	.qo_get_opts		= fifo_get_opts,
};

static struct rtnl_qdisc_ops bfifo_ops = {
	.qo_kind		= "bfifo",
	.qo_msg_parser		= fifo_msg_parser,
	.qo_free_data		= fifo_free_data,
	.qo_dump[NL_DUMP_LINE]	= bfifo_dump_line,
	.qo_get_opts		= fifo_get_opts,
};

static void __init fifo_init(void)
{
	rtnl_qdisc_register(&pfifo_ops);
	rtnl_qdisc_register(&bfifo_ops);
}

static void __exit fifo_exit(void)
{
	rtnl_qdisc_unregister(&pfifo_ops);
	rtnl_qdisc_unregister(&bfifo_ops);
}

/** @} */
