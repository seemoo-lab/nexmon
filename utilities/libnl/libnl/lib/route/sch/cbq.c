/*
 * lib/route/sch/cbq.c	Class Based Queueing
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc-modules.h>
#include <netlink/route/class.h>
#include <netlink/route/class-modules.h>
#include <netlink/route/link.h>
#include <netlink/route/sch/cbq.h>
#include <netlink/route/cls/police.h>

/**
 * @ingroup qdisc_api
 * @ingroup class_api
 * @defgroup cbq Class Based Queueing (CBQ)
 * @{
 */

static struct trans_tbl ovl_strategies[] = {
	__ADD(TC_CBQ_OVL_CLASSIC,classic)
	__ADD(TC_CBQ_OVL_DELAY,delay)
	__ADD(TC_CBQ_OVL_LOWPRIO,lowprio)
	__ADD(TC_CBQ_OVL_DROP,drop)
	__ADD(TC_CBQ_OVL_RCLASSIC,rclassic)
};

/**
 * Convert a CBQ OVL strategy to a character string
 * @arg type		CBQ OVL strategy
 * @arg buf		destination buffer
 * @arg len		length of destination buffer
 *
 * Converts a CBQ OVL strategy to a character string and stores in the
 * provided buffer. Returns the destination buffer or the type
 * encoded in hex if no match was found.
 */
char *nl_ovl_strategy2str(int type, char *buf, size_t len)
{
	return __type2str(type, buf, len, ovl_strategies,
			    ARRAY_SIZE(ovl_strategies));
}

/**
 * Convert a string to a CBQ OVL strategy
 * @arg name		CBQ OVL stragegy name
 *
 * Converts a CBQ OVL stragegy name to it's corresponding CBQ OVL strategy
 * type. Returns the type or -1 if none was found.
 */
int nl_str2ovl_strategy(const char *name)
{
	return __str2type(name, ovl_strategies, ARRAY_SIZE(ovl_strategies));
}

static struct nla_policy cbq_policy[TCA_CBQ_MAX+1] = {
	[TCA_CBQ_LSSOPT]	= { .minlen = sizeof(struct tc_cbq_lssopt) },
	[TCA_CBQ_RATE]		= { .minlen = sizeof(struct tc_ratespec) },
	[TCA_CBQ_WRROPT]	= { .minlen = sizeof(struct tc_cbq_wrropt) },
	[TCA_CBQ_OVL_STRATEGY]	= { .minlen = sizeof(struct tc_cbq_ovl) },
	[TCA_CBQ_FOPT]		= { .minlen = sizeof(struct tc_cbq_fopt) },
	[TCA_CBQ_POLICE]	= { .minlen = sizeof(struct tc_cbq_police) },
};

static inline struct rtnl_cbq *cbq_qdisc(struct rtnl_tca *tca)
{
	return (struct rtnl_cbq *) tca->tc_subdata;
}

static inline struct rtnl_cbq *cbq_alloc(struct rtnl_tca *tca)
{
	if (!tca->tc_subdata)
		tca->tc_subdata = calloc(1, sizeof(struct rtnl_qdisc));

	return cbq_qdisc(tca);
}


static int cbq_msg_parser(struct rtnl_tca *tca)
{
	struct nlattr *tb[TCA_CBQ_MAX + 1];
	struct rtnl_cbq *cbq;
	int err;

	err = tca_parse(tb, TCA_CBQ_MAX, tca, cbq_policy);
	if (err < 0)
		return err;

	cbq = cbq_alloc(tca);
	if (!cbq)
		return -NLE_NOMEM;

	nla_memcpy(&cbq->cbq_lss, tb[TCA_CBQ_LSSOPT], sizeof(cbq->cbq_lss));
	nla_memcpy(&cbq->cbq_rate, tb[TCA_CBQ_RATE], sizeof(cbq->cbq_rate));
	nla_memcpy(&cbq->cbq_wrr, tb[TCA_CBQ_WRROPT], sizeof(cbq->cbq_wrr));
	nla_memcpy(&cbq->cbq_fopt, tb[TCA_CBQ_FOPT], sizeof(cbq->cbq_fopt));
	nla_memcpy(&cbq->cbq_ovl, tb[TCA_CBQ_OVL_STRATEGY],
		   sizeof(cbq->cbq_ovl));
	nla_memcpy(&cbq->cbq_police, tb[TCA_CBQ_POLICE],
		    sizeof(cbq->cbq_police));
	
	return 0;
}

static int cbq_qdisc_msg_parser(struct rtnl_qdisc *qdisc)
{
	return cbq_msg_parser((struct rtnl_tca *) qdisc);
}

static int cbq_class_msg_parser(struct rtnl_class *class)
{
	return cbq_msg_parser((struct rtnl_tca *) class);
}

static void cbq_qdisc_free_data(struct rtnl_qdisc *qdisc)
{
	free(qdisc->q_subdata);
}

static int cbq_clone(struct rtnl_tca *_dst, struct rtnl_tca *_src)
{
	struct rtnl_cbq *src = cbq_qdisc(_src);

	if (src && !cbq_alloc(_dst))
		return -NLE_NOMEM;
	else
		return 0;
}

static int cbq_qdisc_clone(struct rtnl_qdisc *dst, struct rtnl_qdisc *src)
{
	return cbq_clone((struct rtnl_tca *) dst, (struct rtnl_tca *) src);
}

static void cbq_class_free_data(struct rtnl_class *class)
{
	free(class->c_subdata);
}

static int cbq_class_clone(struct rtnl_class *dst, struct rtnl_class *src)
{
	return cbq_clone((struct rtnl_tca *) dst, (struct rtnl_tca *) src);
}

static void cbq_dump_line(struct rtnl_tca *tca, struct nl_dump_params *p)
{
	struct rtnl_cbq *cbq;
	double r, rbit;
	char *ru, *rubit;

	cbq = cbq_qdisc(tca);
	if (!cbq)
		return;

	r = nl_cancel_down_bytes(cbq->cbq_rate.rate, &ru);
	rbit = nl_cancel_down_bits(cbq->cbq_rate.rate * 8, &rubit);

	nl_dump(p, " rate %.2f%s/s (%.0f%s) prio %u",
		r, ru, rbit, rubit, cbq->cbq_wrr.priority);
}

static void cbq_qdisc_dump_line(struct rtnl_qdisc *qdisc,
				struct nl_dump_params *p)
{
	cbq_dump_line((struct rtnl_tca *) qdisc, p);
}

static void cbq_class_dump_line(struct rtnl_class *class,
				struct nl_dump_params *p)
{
	cbq_dump_line((struct rtnl_tca *) class, p);
}

static void cbq_dump_details(struct rtnl_tca *tca, struct nl_dump_params *p)
{
	struct rtnl_cbq *cbq;
	char *unit, buf[32];
	double w;
	uint32_t el;

	cbq = cbq_qdisc(tca);
	if (!cbq)
		return;

	w = nl_cancel_down_bits(cbq->cbq_wrr.weight * 8, &unit);

	nl_dump(p, "avgpkt %u mpu %u cell %u allot %u weight %.0f%s\n",
		cbq->cbq_lss.avpkt,
		cbq->cbq_rate.mpu,
		1 << cbq->cbq_rate.cell_log,
		cbq->cbq_wrr.allot, w, unit);

	el = cbq->cbq_lss.ewma_log;
	nl_dump_line(p, "  minidle %uus maxidle %uus offtime "
				"%uus level %u ewma_log %u\n",
		nl_ticks2us(cbq->cbq_lss.minidle >> el),
		nl_ticks2us(cbq->cbq_lss.maxidle >> el),
		nl_ticks2us(cbq->cbq_lss.offtime >> el),
		cbq->cbq_lss.level,
		cbq->cbq_lss.ewma_log);

	nl_dump_line(p, "  penalty %uus strategy %s ",
		nl_ticks2us(cbq->cbq_ovl.penalty),
		nl_ovl_strategy2str(cbq->cbq_ovl.strategy, buf, sizeof(buf)));

	nl_dump(p, "split %s defmap 0x%08x ",
		rtnl_tc_handle2str(cbq->cbq_fopt.split, buf, sizeof(buf)),
		cbq->cbq_fopt.defmap);
	
	nl_dump(p, "police %s",
		nl_police2str(cbq->cbq_police.police, buf, sizeof(buf)));
}

static void cbq_qdisc_dump_details(struct rtnl_qdisc *qdisc,
				   struct nl_dump_params *p)
{
	cbq_dump_details((struct rtnl_tca *) qdisc, p);
}

static void cbq_class_dump_details(struct rtnl_class *class,
				   struct nl_dump_params *p)
{
	cbq_dump_details((struct rtnl_tca *) class, p);
}

static void cbq_dump_stats(struct rtnl_tca *tca, struct nl_dump_params *p)
{
	struct tc_cbq_xstats *x = tca_xstats(tca);

	if (!x)
		return;

	nl_dump_line(p, "            borrows    overact  "
			"  avgidle  undertime\n");
	nl_dump_line(p, "         %10u %10u %10u %10u\n",
		     x->borrows, x->overactions, x->avgidle, x->undertime);
}

static void cbq_qdisc_dump_stats(struct rtnl_qdisc *qdisc,
				 struct nl_dump_params *p)
{
	cbq_dump_stats((struct rtnl_tca *) qdisc, p);
}

static void cbq_class_dump_stats(struct rtnl_class *class,
				 struct nl_dump_params *p)
{
	cbq_dump_stats((struct rtnl_tca *) class, p);
}

static struct rtnl_qdisc_ops cbq_qdisc_ops = {
	.qo_kind		= "cbq",
	.qo_msg_parser		= cbq_qdisc_msg_parser,
	.qo_free_data		= cbq_qdisc_free_data,
	.qo_clone		= cbq_qdisc_clone,
	.qo_dump = {
	    [NL_DUMP_LINE]	= cbq_qdisc_dump_line,
	    [NL_DUMP_DETAILS]	= cbq_qdisc_dump_details,
	    [NL_DUMP_STATS]	= cbq_qdisc_dump_stats,
	},
};

static struct rtnl_class_ops cbq_class_ops = {
	.co_kind		= "cbq",
	.co_msg_parser		= cbq_class_msg_parser,
	.co_free_data		= cbq_class_free_data,
	.co_clone		= cbq_class_clone,
	.co_dump = {
	    [NL_DUMP_LINE]	= cbq_class_dump_line,
	    [NL_DUMP_DETAILS]	= cbq_class_dump_details,
	    [NL_DUMP_STATS]	= cbq_class_dump_stats,
	},
};

static void __init cbq_init(void)
{
	rtnl_qdisc_register(&cbq_qdisc_ops);
	rtnl_class_register(&cbq_class_ops);
}

static void __exit cbq_exit(void)
{
	rtnl_qdisc_unregister(&cbq_qdisc_ops);
	rtnl_class_unregister(&cbq_class_ops);
}

/** @} */
