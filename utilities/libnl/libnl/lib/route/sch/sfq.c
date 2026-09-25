/*
 * lib/route/sch/sfq.c		SFQ Qdisc
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
 * @defgroup sfq Stochastic Fairness Queueing (SFQ)
 * @brief
 *
 * @par Parameter Description
 * - \b Quantum: Number of bytes to send out per slot and round.
 * - \b Perturbation: Timer period between changing the hash function.
 * - \b Limit:  Upper limit of queue in number of packets before SFQ starts
 *	        dropping packets.
 * - \b Divisor: Hash table divisor, i.e. size of hash table.
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc-modules.h>
#include <netlink/route/sch/sfq.h>

/** @cond SKIP */
#define SCH_SFQ_ATTR_QUANTUM	0x01
#define SCH_SFQ_ATTR_PERTURB	0x02
#define SCH_SFQ_ATTR_LIMIT	0x04
#define SCH_SFQ_ATTR_DIVISOR	0x08
#define SCH_SFQ_ATTR_FLOWS	0x10
/** @endcond */

static inline struct rtnl_sfq *sfq_qdisc(struct rtnl_qdisc *qdisc)
{
	return (struct rtnl_sfq *) qdisc->q_subdata;
}

static inline struct rtnl_sfq *sfq_alloc(struct rtnl_qdisc *qdisc)
{
	if (!qdisc->q_subdata)
		qdisc->q_subdata = calloc(1, sizeof(struct rtnl_sfq));

	return sfq_qdisc(qdisc);
}

static int sfq_msg_parser(struct rtnl_qdisc *qdisc)
{
	struct rtnl_sfq *sfq;
	struct tc_sfq_qopt *opts;

	if (!(qdisc->ce_mask & TCA_ATTR_OPTS))
		return 0;

	if (qdisc->q_opts->d_size < sizeof(*opts))
		return -NLE_INVAL;

	sfq = sfq_alloc(qdisc);
	if (!sfq)
		return -NLE_NOMEM;

	opts = (struct tc_sfq_qopt *) qdisc->q_opts->d_data;

	sfq->qs_quantum = opts->quantum;
	sfq->qs_perturb = opts->perturb_period;
	sfq->qs_limit = opts->limit;
	sfq->qs_divisor = opts->divisor;
	sfq->qs_flows = opts->flows;

	sfq->qs_mask = (SCH_SFQ_ATTR_QUANTUM | SCH_SFQ_ATTR_PERTURB |
			SCH_SFQ_ATTR_LIMIT | SCH_SFQ_ATTR_DIVISOR |
			SCH_SFQ_ATTR_FLOWS);

	return 0;
}

static void sfq_free_data(struct rtnl_qdisc *qdisc)
{
	free(qdisc->q_subdata);
}

static void sfq_dump_line(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	struct rtnl_sfq *sfq = sfq_qdisc(qdisc);

	if (sfq)
		nl_dump(p, " quantum %u perturb %us", sfq->qs_quantum,
			nl_ticks2us(sfq->qs_perturb * nl_get_hz()));
}

static void sfq_dump_details(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	struct rtnl_sfq *sfq = sfq_qdisc(qdisc);

	if (sfq)
		nl_dump(p, "limit %u divisor %u",
			sfq->qs_limit, sfq->qs_divisor);
}

static struct nl_msg *sfq_get_opts(struct rtnl_qdisc *qdisc)
{
	struct rtnl_sfq *sfq;
	struct tc_sfq_qopt opts;
	struct nl_msg *msg;

	sfq = sfq_qdisc(qdisc);
	if (!sfq)
		return NULL;

	msg = nlmsg_alloc();
	if (!msg)
		goto errout;

	memset(&opts, 0, sizeof(opts));
	opts.quantum = sfq->qs_quantum;
	opts.perturb_period = sfq->qs_perturb;
	opts.limit = sfq->qs_limit;

	if (nlmsg_append(msg, &opts, sizeof(opts), NL_DONTPAD) < 0)
		goto errout;

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
 * Set quantum of SFQ qdisc.
 * @arg qdisc		SFQ qdisc to be modified.
 * @arg quantum		New quantum in bytes.
 * @return 0 on success or a negative error code.
 */
int rtnl_sfq_set_quantum(struct rtnl_qdisc *qdisc, int quantum)
{
	struct rtnl_sfq *sfq;
	
	sfq = sfq_alloc(qdisc);
	if (!sfq)
		return -NLE_NOMEM;

	sfq->qs_quantum = quantum;
	sfq->qs_mask |= SCH_SFQ_ATTR_QUANTUM;

	return 0;
}

/**
 * Get quantum of SFQ qdisc.
 * @arg qdisc		SFQ qdisc.
 * @return Quantum in bytes or a negative error code.
 */
int rtnl_sfq_get_quantum(struct rtnl_qdisc *qdisc)
{
	struct rtnl_sfq *sfq;

	sfq = sfq_qdisc(qdisc);
	if (sfq && sfq->qs_mask & SCH_SFQ_ATTR_QUANTUM)
		return sfq->qs_quantum;
	else
		return -NLE_NOATTR;
}

/**
 * Set limit of SFQ qdisc.
 * @arg qdisc		SFQ qdisc to be modified.
 * @arg limit		New limit in number of packets.
 * @return 0 on success or a negative error code.
 */
int rtnl_sfq_set_limit(struct rtnl_qdisc *qdisc, int limit)
{
	struct rtnl_sfq *sfq;

	sfq = sfq_alloc(qdisc);
	if (!sfq)
		return -NLE_NOMEM;

	sfq->qs_limit = limit;
	sfq->qs_mask |= SCH_SFQ_ATTR_LIMIT;

	return 0;
}

/**
 * Get limit of SFQ qdisc.
 * @arg qdisc		SFQ qdisc.
 * @return Limit or a negative error code.
 */
int rtnl_sfq_get_limit(struct rtnl_qdisc *qdisc)
{
	struct rtnl_sfq *sfq;

	sfq = sfq_qdisc(qdisc);
	if (sfq && sfq->qs_mask & SCH_SFQ_ATTR_LIMIT)
		return sfq->qs_limit;
	else
		return -NLE_NOATTR;
}

/**
 * Set perturbation interval of SFQ qdisc.
 * @arg qdisc		SFQ qdisc to be modified.
 * @arg perturb		New perturbation interval in seconds.
 * @note A value of 0 disables perturbation altogether.
 * @return 0 on success or a negative error code.
 */
int rtnl_sfq_set_perturb(struct rtnl_qdisc *qdisc, int perturb)
{
	struct rtnl_sfq *sfq;

	sfq = sfq_alloc(qdisc);
	if (!sfq)
		return -NLE_NOMEM;

	sfq->qs_perturb = perturb;
	sfq->qs_mask |= SCH_SFQ_ATTR_PERTURB;

	return 0;
}

/**
 * Get perturbation interval of SFQ qdisc.
 * @arg qdisc		SFQ qdisc.
 * @return Perturbation interval in seconds or a negative error code.
 */
int rtnl_sfq_get_perturb(struct rtnl_qdisc *qdisc)
{
	struct rtnl_sfq *sfq;

	sfq = sfq_qdisc(qdisc);
	if (sfq && sfq->qs_mask & SCH_SFQ_ATTR_PERTURB)
		return sfq->qs_perturb;
	else
		return -NLE_NOATTR;
}

/**
 * Get divisor of SFQ qdisc.
 * @arg qdisc		SFQ qdisc.
 * @return Divisor in number of entries or a negative error code.
 */
int rtnl_sfq_get_divisor(struct rtnl_qdisc *qdisc)
{
	struct rtnl_sfq *sfq;

	sfq = sfq_qdisc(qdisc);
	if (sfq && sfq->qs_mask & SCH_SFQ_ATTR_DIVISOR)
		return sfq->qs_divisor;
	else
		return -NLE_NOATTR;
}

/** @} */

static struct rtnl_qdisc_ops sfq_ops = {
	.qo_kind		= "sfq",
	.qo_msg_parser		= sfq_msg_parser,
	.qo_free_data		= sfq_free_data,
	.qo_dump = {
	    [NL_DUMP_LINE]	= sfq_dump_line,
	    [NL_DUMP_DETAILS]	= sfq_dump_details,
	},
	.qo_get_opts		= sfq_get_opts,
};

static void __init sfq_init(void)
{
	rtnl_qdisc_register(&sfq_ops);
}

static void __exit sfq_exit(void)
{
	rtnl_qdisc_unregister(&sfq_ops);
}

/** @} */
