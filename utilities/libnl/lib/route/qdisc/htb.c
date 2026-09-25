/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2011 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2005-2006 Petr Gotthard <petr.gotthard@siemens.com>
 * Copyright (c) 2005-2006 Siemens AG Oesterreich
 */

/**
 * @ingroup qdisc
 * @ingroup class
 * @defgroup qdisc_htb Hierachical Token Bucket (HTB)
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>
#include <netlink/route/link.h>
#include <netlink/route/qdisc/htb.h>

#include "tc-api.h"

/** @cond SKIP */
struct rtnl_htb_qdisc {
	uint32_t qh_rate2quantum;
	uint32_t qh_defcls;
	uint32_t qh_mask;
	uint32_t qh_direct_pkts;
};

struct rtnl_htb_class {
	uint32_t ch_prio;
	struct rtnl_ratespec ch_rate;
	struct rtnl_ratespec ch_ceil;
	uint32_t ch_rbuffer;
	uint32_t ch_cbuffer;
	uint32_t ch_quantum;
	uint32_t ch_mask;
	uint32_t ch_level;
};

#define SCH_HTB_HAS_RATE2QUANTUM	0x01
#define SCH_HTB_HAS_DEFCLS		0x02

#define SCH_HTB_HAS_PRIO		0x001
#define SCH_HTB_HAS_RATE		0x002
#define SCH_HTB_HAS_CEIL		0x004
#define SCH_HTB_HAS_RBUFFER		0x008
#define SCH_HTB_HAS_CBUFFER		0x010
#define SCH_HTB_HAS_QUANTUM		0x020
#define SCH_HTB_HAS_LEVEL		0x040
/** @endcond */

static struct nla_policy htb_policy[TCA_HTB_MAX+1] = {
	[TCA_HTB_INIT]	= { .minlen = sizeof(struct tc_htb_glob) },
	[TCA_HTB_PARMS] = { .minlen = sizeof(struct tc_htb_opt) },
	[TCA_HTB_RATE64] = { .minlen = sizeof(uint64_t) },
	[TCA_HTB_CEIL64] = { .minlen = sizeof(uint64_t) },
};

static int htb_qdisc_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct nlattr *tb[TCA_HTB_MAX + 1];
	struct rtnl_htb_qdisc *htb = data;
	int err;

	if ((err = tca_parse(tb, TCA_HTB_MAX, tc, htb_policy)) < 0)
		return err;

	if (tb[TCA_HTB_INIT]) {
		struct tc_htb_glob opts;

		nla_memcpy(&opts, tb[TCA_HTB_INIT], sizeof(opts));
		htb->qh_rate2quantum = opts.rate2quantum;
		htb->qh_defcls = opts.defcls;
		htb->qh_direct_pkts = opts.direct_pkts;

		htb->qh_mask = (SCH_HTB_HAS_RATE2QUANTUM | SCH_HTB_HAS_DEFCLS);
	}

	return 0;
}

static int htb_class_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct nlattr *tb[TCA_HTB_MAX + 1];
	struct rtnl_htb_class *htb = data;
	int err;

	if ((err = tca_parse(tb, TCA_HTB_MAX, tc, htb_policy)) < 0)
		return err;

	if (tb[TCA_HTB_PARMS]) {
		struct tc_htb_opt opts;

		nla_memcpy(&opts, tb[TCA_HTB_PARMS], sizeof(opts));
		htb->ch_prio = opts.prio;
		rtnl_copy_ratespec(&htb->ch_rate, &opts.rate);
		rtnl_copy_ratespec(&htb->ch_ceil, &opts.ceil);

		if (tb[TCA_HTB_RATE64])
		        nla_memcpy(&htb->ch_rate.rs_rate64, tb[TCA_HTB_RATE64], sizeof(uint64_t));
		if (tb[TCA_HTB_CEIL64])
		        nla_memcpy(&htb->ch_ceil.rs_rate64, tb[TCA_HTB_CEIL64], sizeof(uint64_t));

		htb->ch_rbuffer = rtnl_tc_calc_bufsize64(nl_ticks2us(opts.buffer),
		                                         htb->ch_rate.rs_rate64);
		htb->ch_cbuffer = rtnl_tc_calc_bufsize64(nl_ticks2us(opts.cbuffer),
		                                         htb->ch_ceil.rs_rate64);
		htb->ch_quantum = opts.quantum;
		htb->ch_level = opts.level;

		rtnl_tc_set_mpu(tc, htb->ch_rate.rs_mpu);
		rtnl_tc_set_overhead(tc, htb->ch_rate.rs_overhead);

		htb->ch_mask = (SCH_HTB_HAS_PRIO | SCH_HTB_HAS_RATE |
				SCH_HTB_HAS_CEIL | SCH_HTB_HAS_RBUFFER |
				SCH_HTB_HAS_CBUFFER | SCH_HTB_HAS_QUANTUM |
				SCH_HTB_HAS_LEVEL);
	}

	return 0;
}

static void htb_qdisc_dump_line(struct rtnl_tc *tc, void *data,
				struct nl_dump_params *p)
{
	struct rtnl_htb_qdisc *htb = data;

	if (!htb)
		return;

	if (htb->qh_mask & SCH_HTB_HAS_RATE2QUANTUM)
		nl_dump(p, " r2q %u", htb->qh_rate2quantum);

	if (htb->qh_mask & SCH_HTB_HAS_DEFCLS) {
		char buf[64];
		nl_dump(p, " default-class %s",
			rtnl_tc_handle2str(htb->qh_defcls, buf, sizeof(buf)));
	}
}

static void htb_class_dump_line(struct rtnl_tc *tc, void *data,
				struct nl_dump_params *p)
{
	struct rtnl_htb_class *htb = data;

	if (!htb)
		return;

	if (htb->ch_mask & SCH_HTB_HAS_RATE) {
		double r, rbit;
		char *ru, *rubit;

		r = nl_cancel_down_bytes(htb->ch_rate.rs_rate64, &ru);
		rbit = nl_cancel_down_bits(htb->ch_rate.rs_rate64*8, &rubit);

		nl_dump(p, " rate %.2f%s/s (%.0f%s) log %u",
			r, ru, rbit, rubit, 1<<htb->ch_rate.rs_cell_log);
	}
}

static void htb_class_dump_details(struct rtnl_tc *tc, void *data,
				   struct nl_dump_params *p)
{
	struct rtnl_htb_class *htb = data;

	if (!htb)
		return;

	/* line 1 */
	if (htb->ch_mask & SCH_HTB_HAS_CEIL) {
		double r, rbit;
		char *ru, *rubit;

		r = nl_cancel_down_bytes(htb->ch_ceil.rs_rate64, &ru);
		rbit = nl_cancel_down_bits(htb->ch_ceil.rs_rate64*8, &rubit);

		nl_dump(p, " ceil %.2f%s/s (%.0f%s) log %u",
			r, ru, rbit, rubit, 1<<htb->ch_ceil.rs_cell_log);
	}

	if (htb->ch_mask & SCH_HTB_HAS_PRIO)
		nl_dump(p, " prio %u", htb->ch_prio);

	if (htb->ch_mask & SCH_HTB_HAS_RBUFFER) {
		double b;
		char *bu;

		b = nl_cancel_down_bytes(htb->ch_rbuffer, &bu);
		nl_dump(p, " rbuffer %.2f%s", b, bu);
	}

	if (htb->ch_mask & SCH_HTB_HAS_CBUFFER) {
		double b;
		char *bu;

		b = nl_cancel_down_bytes(htb->ch_cbuffer, &bu);
		nl_dump(p, " cbuffer %.2f%s", b, bu);
	}

	if (htb->ch_mask & SCH_HTB_HAS_QUANTUM)
		nl_dump(p, " quantum %u", htb->ch_quantum);
}

static int htb_qdisc_msg_fill(struct rtnl_tc *tc, void *data,
			      struct nl_msg *msg)
{
	struct rtnl_htb_qdisc *htb = data;
	struct tc_htb_glob opts = {
		.version = TC_HTB_PROTOVER,
		.rate2quantum = 10,
	};

	if (htb) {
		if (htb->qh_mask & SCH_HTB_HAS_RATE2QUANTUM)
			opts.rate2quantum = htb->qh_rate2quantum;

		if (htb->qh_mask & SCH_HTB_HAS_DEFCLS)
			opts.defcls = htb->qh_defcls;
	}

	return nla_put(msg, TCA_HTB_INIT, sizeof(opts), &opts);
}

static int htb_class_msg_fill(struct rtnl_tc *tc, void *data,
			      struct nl_msg *msg)
{
	struct rtnl_htb_class *htb = data;
	uint32_t mtu, rtable[RTNL_TC_RTABLE_SIZE], ctable[RTNL_TC_RTABLE_SIZE];
	struct tc_htb_opt opts;
	int buffer, cbuffer;
	uint64_t rate64;
	uint64_t ceil64;

	if (!htb || !(htb->ch_mask & SCH_HTB_HAS_RATE))
		BUG();

	memset(&opts, 0, sizeof(opts));

	/* if not set, zero (0) is used as priority */
	if (htb->ch_mask & SCH_HTB_HAS_PRIO)
		opts.prio = htb->ch_prio;

	mtu = rtnl_tc_get_mtu(tc);

	rtnl_tc_build_rate_table(tc, &htb->ch_rate, rtable);
	rtnl_rcopy_ratespec(&opts.rate, &htb->ch_rate);
	rate64 = htb->ch_rate.rs_rate64;

	if (htb->ch_mask & SCH_HTB_HAS_CEIL) {
		rtnl_tc_build_rate_table(tc, &htb->ch_ceil, ctable);
		rtnl_rcopy_ratespec(&opts.ceil, &htb->ch_ceil);
		ceil64 = htb->ch_ceil.rs_rate64;
	} else {
		/*
		 * If not set, configured rate is used as ceil, which implies
		 * no borrowing.
		 */
		memcpy(&opts.ceil, &opts.rate, sizeof(struct tc_ratespec));
		ceil64 = rate64;
	}

	if (htb->ch_mask & SCH_HTB_HAS_RBUFFER)
		buffer = htb->ch_rbuffer;
	else
		buffer = rate64 / nl_get_psched_hz() + mtu; /* XXX */

	opts.buffer = nl_us2ticks(rtnl_tc_calc_txtime64(buffer, rate64));

	if (htb->ch_mask & SCH_HTB_HAS_CBUFFER)
		cbuffer = htb->ch_cbuffer;
	else
		cbuffer = ceil64 / nl_get_psched_hz() + mtu; /* XXX */

	opts.cbuffer = nl_us2ticks(rtnl_tc_calc_txtime64(cbuffer, ceil64));

	if (htb->ch_mask & SCH_HTB_HAS_QUANTUM)
		opts.quantum = htb->ch_quantum;

	NLA_PUT(msg, TCA_HTB_PARMS, sizeof(opts), &opts);
	if (rate64 > 0xFFFFFFFFull)
		NLA_PUT(msg, TCA_HTB_RATE64, sizeof(uint64_t), &rate64);
	if (ceil64 > 0xFFFFFFFFull)
		NLA_PUT(msg, TCA_HTB_CEIL64, sizeof(uint64_t), &ceil64);
	NLA_PUT(msg, TCA_HTB_RTAB, sizeof(rtable), &rtable);
	NLA_PUT(msg, TCA_HTB_CTAB, sizeof(ctable), &ctable);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static struct rtnl_tc_ops htb_qdisc_ops;
static struct rtnl_tc_ops htb_class_ops;

static struct rtnl_htb_qdisc *htb_qdisc_data(struct rtnl_qdisc *qdisc, int *err)
{
	return rtnl_tc_data_check(TC_CAST(qdisc), &htb_qdisc_ops, err);
}

static struct rtnl_htb_class *htb_class_data(struct rtnl_class *class, int *err)
{
	return rtnl_tc_data_check(TC_CAST(class), &htb_class_ops, err);
}

/**
 * @name Attribute Modifications
 * @{
 */

/**
 * Return rate/quantum ratio of HTB qdisc
 * @arg qdisc		htb qdisc object
 *
 * @return rate/quantum ratio or 0 if unspecified
 */
uint32_t rtnl_htb_get_rate2quantum(struct rtnl_qdisc *qdisc)
{
	struct rtnl_htb_qdisc *htb;

	if ((htb = htb_qdisc_data(qdisc, NULL)) &&
	    (htb->qh_mask & SCH_HTB_HAS_RATE2QUANTUM))
		return htb->qh_rate2quantum;

	return 0;
}

int rtnl_htb_set_rate2quantum(struct rtnl_qdisc *qdisc, uint32_t rate2quantum)
{
	struct rtnl_htb_qdisc *htb;
	int err;

	if (!(htb = htb_qdisc_data(qdisc, &err)))
		return err;

	htb->qh_rate2quantum = rate2quantum;
	htb->qh_mask |= SCH_HTB_HAS_RATE2QUANTUM;

	return 0;
}

/**
 * Return default class of HTB qdisc
 * @arg qdisc		htb qdisc object
 *
 * Returns the classid of the class where all unclassified traffic
 * goes to.
 *
 * @return classid or TC_H_UNSPEC if unspecified.
 */
uint32_t rtnl_htb_get_defcls(struct rtnl_qdisc *qdisc)
{
	struct rtnl_htb_qdisc *htb;

	if ((htb = htb_qdisc_data(qdisc, NULL)) &&
	    htb->qh_mask & SCH_HTB_HAS_DEFCLS)
		return htb->qh_defcls;

	return TC_H_UNSPEC;
}

/**
 * Set default class of the htb qdisc to the specified value
 * @arg qdisc		qdisc to change
 * @arg defcls		new default class
 */
int rtnl_htb_set_defcls(struct rtnl_qdisc *qdisc, uint32_t defcls)
{
	struct rtnl_htb_qdisc *htb;
	int err;

	if (!(htb = htb_qdisc_data(qdisc, &err)))
		return err;

	htb->qh_defcls = defcls;
	htb->qh_mask |= SCH_HTB_HAS_DEFCLS;

	return 0;
}

uint32_t rtnl_htb_get_prio(struct rtnl_class *class)
{
	struct rtnl_htb_class *htb;

	if ((htb = htb_class_data(class, NULL)) &&
	    (htb->ch_mask & SCH_HTB_HAS_PRIO))
		return htb->ch_prio;

	return 0;
}

int rtnl_htb_set_prio(struct rtnl_class *class, uint32_t prio)
{
	struct rtnl_htb_class *htb;
	int err;

	if (!(htb = htb_class_data(class, &err)))
		return err;

	htb->ch_prio = prio;
	htb->ch_mask |= SCH_HTB_HAS_PRIO;

	return 0;
}

/**
 * Return rate of HTB class
 * @arg class		htb class object
 *
 * @return Rate in bytes/s or 0 if unspecified. If the value
 *   cannot be represented as 32 bit integer, (1<<32) is returned.
 *   Use rtnl_htb_get_rate64() instead.
 */
uint32_t rtnl_htb_get_rate(struct rtnl_class *class)
{
	struct rtnl_htb_class *htb;

	if (   !(htb = htb_class_data(class, NULL))
	    || !(htb->ch_mask & SCH_HTB_HAS_RATE))
	    return 0;

	if (htb->ch_rate.rs_rate64 > 0xFFFFFFFFull)
		return 0xFFFFFFFFull;

	return htb->ch_rate.rs_rate64;
}

/**
 * Return rate of HTB class
 * @arg class		htb class object
 * @arg out_rate64      on success, the set rate.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_htb_get_rate64(struct rtnl_class *class, uint64_t *out_rate64)
{
	struct rtnl_htb_class *htb;

	if (!(htb = htb_class_data(class, NULL)))
		return -NLE_INVAL;
	if (!(htb->ch_mask & SCH_HTB_HAS_RATE))
		return -NLE_NOATTR;

	*out_rate64 = htb->ch_rate.rs_rate64;
	return 0;
}

/**
 * Set rate of HTB class
 * @arg class		htb class object
 * @arg rate		new rate in bytes per second
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_htb_set_rate(struct rtnl_class *class, uint32_t rate)
{
	return rtnl_htb_set_rate64(class, rate);
}

/**
 * Set rate of HTB class
 * @arg class		htb class object
 * @arg rate		new rate in bytes per second
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_htb_set_rate64(struct rtnl_class *class, uint64_t rate)
{
	struct rtnl_htb_class *htb;
	int err;

	if (!(htb = htb_class_data(class, &err)))
		return err;

	htb->ch_rate.rs_cell_log = UINT8_MAX; /* use default value */
	htb->ch_rate.rs_rate64 = rate;
	htb->ch_mask |= SCH_HTB_HAS_RATE;

	return 0;
}

/**
 * Return ceil rate of HTB class
 * @arg class		htb class object
 *
 * @return Ceil rate in bytes/s or 0 if unspecified.  If the value
 *   cannot be represented as 32 bit integer, (1<<32) is returned.
 *   Use rtnl_htb_get_ceil64() instead.
 */
uint32_t rtnl_htb_get_ceil(struct rtnl_class *class)
{
	struct rtnl_htb_class *htb;

	if (   !(htb = htb_class_data(class, NULL))
	    || !(htb->ch_mask & SCH_HTB_HAS_CEIL))
		return 0;

	if (htb->ch_ceil.rs_rate64 > 0xFFFFFFFFull)
		return 0xFFFFFFFFull;

	return htb->ch_ceil.rs_rate64;
}

/**
 * Return ceil rate of HTB class
 * @arg class		htb class object
 * @arg out_ceil64      on success, the set ceil value.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_htb_get_ceil64(struct rtnl_class *class, uint64_t *out_ceil64)
{
	struct rtnl_htb_class *htb;

	if (!(htb = htb_class_data(class, NULL)))
		return -NLE_INVAL;
	if (!(htb->ch_mask & SCH_HTB_HAS_CEIL))
		return -NLE_NOATTR;

	*out_ceil64 = htb->ch_ceil.rs_rate64;
	return 0;
}

/**
 * Set ceil rate of HTB class
 * @arg class		htb class object
 * @arg ceil		new ceil rate number of bytes per second
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_htb_set_ceil(struct rtnl_class *class, uint32_t ceil)
{
	return rtnl_htb_set_ceil64(class, ceil);
}

/**
 * Set ceil rate of HTB class
 * @arg class		htb class object
 * @arg ceil64		new ceil rate number of bytes per second
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_htb_set_ceil64(struct rtnl_class *class, uint64_t ceil64)
{
	struct rtnl_htb_class *htb;
	int err;

	if (!(htb = htb_class_data(class, &err)))
		return err;

	htb->ch_ceil.rs_cell_log = UINT8_MAX; /* use default value */
	htb->ch_ceil.rs_rate64 = ceil64;
	htb->ch_mask |= SCH_HTB_HAS_CEIL;

	return 0;
}

/**
 * Return burst buffer size of HTB class
 * @arg class		htb class object
 *
 * @return Burst buffer size or 0 if unspecified
 */
uint32_t rtnl_htb_get_rbuffer(struct rtnl_class *class)
{
	struct rtnl_htb_class *htb;

	if ((htb = htb_class_data(class, NULL)) &&
	     htb->ch_mask & SCH_HTB_HAS_RBUFFER)
		return htb->ch_rbuffer;

	return 0;
}

/**
 * Set size of the rate bucket of HTB class.
 * @arg class		HTB class to be modified.
 * @arg rbuffer		New size in bytes.
 */
int rtnl_htb_set_rbuffer(struct rtnl_class *class, uint32_t rbuffer)
{
	struct rtnl_htb_class *htb;
	int err;

	if (!(htb = htb_class_data(class, &err)))
		return err;

	htb->ch_rbuffer = rbuffer;
	htb->ch_mask |= SCH_HTB_HAS_RBUFFER;

	return 0;
}

/**
 * Return ceil burst buffer size of HTB class
 * @arg class		htb class object
 *
 * @return Ceil burst buffer size or 0 if unspecified
 */
uint32_t rtnl_htb_get_cbuffer(struct rtnl_class *class)
{
	struct rtnl_htb_class *htb;

	if ((htb = htb_class_data(class, NULL)) &&
	     htb->ch_mask & SCH_HTB_HAS_CBUFFER)
		return htb->ch_cbuffer;

	return 0;
}

/**
 * Set size of the ceil bucket of HTB class.
 * @arg class		HTB class to be modified.
 * @arg cbuffer		New size in bytes.
 */
int rtnl_htb_set_cbuffer(struct rtnl_class *class, uint32_t cbuffer)
{
	struct rtnl_htb_class *htb;
	int err;

	if (!(htb = htb_class_data(class, &err)))
		return err;

	htb->ch_cbuffer = cbuffer;
	htb->ch_mask |= SCH_HTB_HAS_CBUFFER;

	return 0;
}

/**
 * Return quantum of HTB class
 * @arg class		htb class object
 *
 * See XXX[quantum def]
 *
 * @return Quantum or 0 if unspecified.
 */
uint32_t rtnl_htb_get_quantum(struct rtnl_class *class)
{
	struct rtnl_htb_class *htb;

	if ((htb = htb_class_data(class, NULL)) &&
	    htb->ch_mask & SCH_HTB_HAS_QUANTUM)
		return htb->ch_quantum;

	return 0;
}

/**
 * Set quantum of HTB class (overwrites value calculated based on r2q)
 * @arg class		htb class object
 * @arg quantum		new quantum in number of bytes
 *
 * See XXX[quantum def]
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_htb_set_quantum(struct rtnl_class *class, uint32_t quantum)
{
	struct rtnl_htb_class *htb;
	int err;

	if (!(htb = htb_class_data(class, &err)))
		return err;

	htb->ch_quantum = quantum;
	htb->ch_mask |= SCH_HTB_HAS_QUANTUM;

	return 0;
}

/**
 * Return level of HTB class
 * @arg class		htb class object
 *
 * Returns the level of the HTB class. Leaf classes are assigned level
 * 0, root classes have level (TC_HTB_MAXDEPTH - 1). Interior classes
 * have a level of one less than their parent.
 *
 * @return Level or a negative error code.
 */
int rtnl_htb_get_level(struct rtnl_class *class)
{
	struct rtnl_htb_class *htb;
	int err = -NLE_OPNOTSUPP;

	if ((htb = htb_class_data(class, &err)) &&
	    (htb->ch_mask & SCH_HTB_HAS_LEVEL))
		return htb->ch_level;

	return err;
}

/**
 * Set level of HTB class
 * @arg class		htb class object
 * @arg level		new level of HTB class
 *
 * Sets the level of a HTB class. Note that changing the level of a HTB
 * class does not change the level of its in kernel counterpart. This
 * function is provided only to create HTB objects which can be compared
 * against or filtered upon.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_htb_set_level(struct rtnl_class *class, int level)
{
	struct rtnl_htb_class *htb;
	int err;

	if (!(htb = htb_class_data(class, &err)))
		return err;

	htb->ch_level = level;
	htb->ch_mask |= SCH_HTB_HAS_LEVEL;

	return 0;
}

/** @} */

static struct rtnl_tc_ops htb_qdisc_ops = {
	.to_kind		= "htb",
	.to_type		= RTNL_TC_TYPE_QDISC,
	.to_size		= sizeof(struct rtnl_htb_qdisc),
	.to_msg_parser		= htb_qdisc_msg_parser,
	.to_dump[NL_DUMP_LINE]	= htb_qdisc_dump_line,
	.to_msg_fill		= htb_qdisc_msg_fill,
};

static struct rtnl_tc_ops htb_class_ops = {
	.to_kind		= "htb",
	.to_type		= RTNL_TC_TYPE_CLASS,
	.to_size		= sizeof(struct rtnl_htb_class),
	.to_msg_parser		= htb_class_msg_parser,
	.to_dump = {
	    [NL_DUMP_LINE]	= htb_class_dump_line,
	    [NL_DUMP_DETAILS]	= htb_class_dump_details,
	},
	.to_msg_fill		= htb_class_msg_fill,
};

static void _nl_init htb_init(void)
{
	rtnl_tc_register(&htb_qdisc_ops);
	rtnl_tc_register(&htb_class_ops);
}

static void _nl_exit htb_exit(void)
{
	rtnl_tc_unregister(&htb_qdisc_ops);
	rtnl_tc_unregister(&htb_class_ops);
}

/** @} */
