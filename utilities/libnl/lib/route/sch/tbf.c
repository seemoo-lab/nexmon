/*
 * lib/route/sch/tbf.c		TBF Qdisc
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
 * @defgroup tbf Token Bucket Filter (TBF)
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc-modules.h>
#include <netlink/route/class.h>
#include <netlink/route/class-modules.h>
#include <netlink/route/link.h>
#include <netlink/route/sch/tbf.h>

/** @cond SKIP */
#define TBF_ATTR_LIMIT			0x01
#define TBF_ATTR_RATE			0x02
#define TBF_ATTR_PEAKRATE		0x10
#define TBF_ATTR_MPU			0x80
/** @endcond */

static inline struct rtnl_tbf *tbf_qdisc(struct rtnl_qdisc *qdisc)
{
	return (struct rtnl_tbf *) qdisc->q_subdata;
}

static inline struct rtnl_tbf *tbf_alloc(struct rtnl_qdisc *qdisc)
{
	if (!qdisc->q_subdata)
		qdisc->q_subdata = calloc(1, sizeof(struct rtnl_tbf));

	return tbf_qdisc(qdisc);
}

static struct nla_policy tbf_policy[TCA_TBF_MAX+1] = {
	[TCA_TBF_PARMS]	= { .minlen = sizeof(struct tc_tbf_qopt) },
};

static int tbf_msg_parser(struct rtnl_qdisc *q)
{
	int err;
	struct nlattr *tb[TCA_TBF_MAX + 1];
	struct rtnl_tbf *tbf;

	err = tca_parse(tb, TCA_TBF_MAX, (struct rtnl_tca *) q, tbf_policy);
	if (err < 0)
		return err;
	
	tbf = tbf_alloc(q);
	if (!tbf)
		return -NLE_NOMEM;

	if (tb[TCA_TBF_PARMS]) {
		struct tc_tbf_qopt opts;
		int bufsize;

		nla_memcpy(&opts, tb[TCA_TBF_PARMS], sizeof(opts));
		tbf->qt_limit = opts.limit;
		tbf->qt_mpu = opts.rate.mpu;
	
		rtnl_copy_ratespec(&tbf->qt_rate, &opts.rate);
		tbf->qt_rate_txtime = opts.buffer;
		bufsize = rtnl_tc_calc_bufsize(nl_ticks2us(opts.buffer),
					       opts.rate.rate);
		tbf->qt_rate_bucket = bufsize;

		rtnl_copy_ratespec(&tbf->qt_peakrate, &opts.peakrate);
		tbf->qt_peakrate_txtime = opts.mtu;
		bufsize = rtnl_tc_calc_bufsize(nl_ticks2us(opts.mtu),
					       opts.peakrate.rate);
		tbf->qt_peakrate_bucket = bufsize;

		tbf->qt_mask = (TBF_ATTR_LIMIT | TBF_ATTR_MPU | TBF_ATTR_RATE |
				TBF_ATTR_PEAKRATE);
	}

	return 0;
}

static void tbf_free_data(struct rtnl_qdisc *qdisc)
{
	free(qdisc->q_subdata);
}

static void tbf_dump_line(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	double r, rbit, lim;
	char *ru, *rubit, *limu;
	struct rtnl_tbf *tbf = tbf_qdisc(qdisc);

	if (!tbf)
		return;

	r = nl_cancel_down_bytes(tbf->qt_rate.rs_rate, &ru);
	rbit = nl_cancel_down_bits(tbf->qt_rate.rs_rate*8, &rubit);
	lim = nl_cancel_down_bytes(tbf->qt_limit, &limu);

	nl_dump(p, " rate %.2f%s/s (%.0f%s) limit %.2f%s",
		r, ru, rbit, rubit, lim, limu);
}

static void tbf_dump_details(struct rtnl_qdisc *qdisc, struct nl_dump_params *p)
{
	struct rtnl_tbf *tbf = tbf_qdisc(qdisc);

	if (!tbf)
		return;

	if (1) {
		char *bu, *cu;
		double bs = nl_cancel_down_bytes(tbf->qt_rate_bucket, &bu);
		double cl = nl_cancel_down_bytes(1 << tbf->qt_rate.rs_cell_log,
						 &cu);

		nl_dump(p, "mpu %u rate-bucket-size %1.f%s "
			   "rate-cell-size %.1f%s\n",
			tbf->qt_mpu, bs, bu, cl, cu);

	}

	if (tbf->qt_mask & TBF_ATTR_PEAKRATE) {
		char *pru, *prbu, *bsu, *clu;
		double pr, prb, bs, cl;
		
		pr = nl_cancel_down_bytes(tbf->qt_peakrate.rs_rate, &pru);
		prb = nl_cancel_down_bits(tbf->qt_peakrate.rs_rate * 8, &prbu);
		bs = nl_cancel_down_bits(tbf->qt_peakrate_bucket, &bsu);
		cl = nl_cancel_down_bits(1 << tbf->qt_peakrate.rs_cell_log,
					 &clu);

		nl_dump_line(p, "    peak-rate %.2f%s/s (%.0f%s) "
				"bucket-size %.1f%s cell-size %.1f%s"
				"latency %.1f%s",
			     pr, pru, prb, prbu, bs, bsu, cl, clu);
	}
}

static struct nl_msg *tbf_get_opts(struct rtnl_qdisc *qdisc)
{
	struct tc_tbf_qopt opts;
	struct rtnl_tbf *tbf;
	struct nl_msg *msg;
	uint32_t rtab[RTNL_TC_RTABLE_SIZE];
	uint32_t ptab[RTNL_TC_RTABLE_SIZE];
	int required = TBF_ATTR_RATE | TBF_ATTR_LIMIT;

	memset(&opts, 0, sizeof(opts));

	tbf = tbf_qdisc(qdisc);
	if (!tbf)
		return NULL;

	if (!(tbf->qt_mask & required) != required)
		return NULL;

	opts.limit = tbf->qt_limit;
	opts.buffer = tbf->qt_rate_txtime;
	tbf->qt_rate.rs_mpu = tbf->qt_mpu;
	rtnl_rcopy_ratespec(&opts.rate, &tbf->qt_rate);

	rtnl_tc_build_rate_table(rtab, tbf->qt_mpu & 0xff, tbf->qt_mpu >> 8,
				 1 << tbf->qt_rate.rs_cell_log,
				 tbf->qt_rate.rs_rate);

	if (tbf->qt_mask & TBF_ATTR_PEAKRATE) {
		opts.mtu = tbf->qt_peakrate_txtime;
		tbf->qt_peakrate.rs_mpu = tbf->qt_mpu;
		rtnl_rcopy_ratespec(&opts.peakrate, &tbf->qt_peakrate);

		rtnl_tc_build_rate_table(ptab, tbf->qt_mpu & 0xff,
					 tbf->qt_mpu >> 8,
					 1 << tbf->qt_peakrate.rs_cell_log,
					 tbf->qt_peakrate.rs_rate);
	}

	msg = nlmsg_alloc();
	if (!msg)
		goto nla_put_failure;

	NLA_PUT(msg, TCA_TBF_PARMS, sizeof(opts), &opts);
	NLA_PUT(msg, TCA_TBF_RTAB, sizeof(rtab), rtab);

	if (tbf->qt_mask & TBF_ATTR_PEAKRATE)
		NLA_PUT(msg, TCA_TBF_PTAB, sizeof(ptab), ptab);

	return msg;

nla_put_failure:
	nlmsg_free(msg);
	return NULL;
}

/**
 * @name Attribute Access
 * @{
 */

/**
 * Set limit of TBF qdisc.
 * @arg qdisc		TBF qdisc to be modified.
 * @arg limit		New limit in bytes.
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_tbf_set_limit(struct rtnl_qdisc *qdisc, int limit)
{
	struct rtnl_tbf *tbf;
	
	tbf = tbf_alloc(qdisc);
	if (!tbf)
		return -NLE_NOMEM;

	tbf->qt_limit = limit;
	tbf->qt_mask |= TBF_ATTR_LIMIT;

	return 0;
}

static inline double calc_limit(struct rtnl_ratespec *spec, int latency,
				int bucket)
{
	double limit;

	limit = (double) spec->rs_rate * ((double) latency / 1000000.);
	limit += bucket;

	return limit;
}

/**
 * Set limit of TBF qdisc by latency.
 * @arg qdisc		TBF qdisc to be modified.
 * @arg latency		Latency in micro seconds.
 *
 * Calculates and sets the limit based on the desired latency and the
 * configured rate and peak rate. In order for this operation to succeed,
 * the rate and if required the peak rate must have been set in advance.
 *
 * @f[
 *   limit_n = \frac{{rate_n} \times {latency}}{10^6}+{bucketsize}_n
 * @f]
 * @f[
 *   limit = min(limit_{rate},limit_{peak})
 * @f]
 * 
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_tbf_set_limit_by_latency(struct rtnl_qdisc *qdisc, int latency)
{
	struct rtnl_tbf *tbf;
	double limit, limit2;

	tbf = tbf_alloc(qdisc);
	if (!tbf)
		return -NLE_NOMEM;

	if (!(tbf->qt_mask & TBF_ATTR_RATE))
		return -NLE_MISSING_ATTR;

	limit = calc_limit(&tbf->qt_rate, latency, tbf->qt_rate_bucket);

	if (tbf->qt_mask & TBF_ATTR_PEAKRATE) {
		limit2 = calc_limit(&tbf->qt_peakrate, latency,
				    tbf->qt_peakrate_bucket);

		if (limit2 < limit)
			limit = limit2;
	}

	return rtnl_qdisc_tbf_set_limit(qdisc, (int) limit);
}

/**
 * Get limit of TBF qdisc.
 * @arg qdisc		TBF qdisc.
 * @return Limit in bytes or a negative error code.
 */
int rtnl_qdisc_tbf_get_limit(struct rtnl_qdisc *qdisc)
{
	struct rtnl_tbf *tbf;
	
	tbf = tbf_qdisc(qdisc);
	if (tbf && (tbf->qt_mask & TBF_ATTR_LIMIT))
		return tbf->qt_limit;
	else
		return -NLE_NOATTR;
}

/**
 * Set MPU of TBF qdisc.
 * @arg qdisc		TBF qdisc to be modified.
 * @arg mpu		New MPU in bytes.
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_tbf_set_mpu(struct rtnl_qdisc *qdisc, int mpu)
{
	struct rtnl_tbf *tbf;
	
	tbf = tbf_alloc(qdisc);
	if (!tbf)
		return -NLE_NOMEM;

	tbf->qt_mpu = mpu;
	tbf->qt_mask |= TBF_ATTR_MPU;

	return 0;
}

/**
 * Get MPU of TBF qdisc.
 * @arg qdisc		TBF qdisc.
 * @return MPU in bytes or a negative error code.
 */
int rtnl_qdisc_tbf_get_mpu(struct rtnl_qdisc *qdisc)
{
	struct rtnl_tbf *tbf;
	
	tbf = tbf_qdisc(qdisc);
	if (tbf && (tbf->qt_mask & TBF_ATTR_MPU))
		return tbf->qt_mpu;
	else
		return -NLE_NOATTR;
}

static inline int calc_cell_log(int cell, int bucket)
{
	if (cell > 0)
		cell = rtnl_tc_calc_cell_log(cell);
	else {
		cell = 0;

		if (!bucket)
			bucket = 2047; /* defaults to cell_log=3 */

		while ((bucket >> cell) > 255)
			cell++;
	}

	return cell;
}

/**
 * Set rate of TBF qdisc.
 * @arg qdisc		TBF qdisc to be modified.
 * @arg rate		New rate in bytes per second.
 * @arg bucket		Size of bucket in bytes.
 * @arg cell		Size of a rate cell or 0 to get default value.
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_tbf_set_rate(struct rtnl_qdisc *qdisc, int rate, int bucket,
			    int cell)
{
	struct rtnl_tbf *tbf;
	int cell_log;
	
	tbf = tbf_alloc(qdisc);
	if (!tbf)
		return -NLE_NOMEM;

	cell_log = calc_cell_log(cell, bucket);
	if (cell_log < 0)
		return cell_log;

	tbf->qt_rate.rs_rate = rate;
	tbf->qt_rate_bucket = bucket;
	tbf->qt_rate.rs_cell_log = cell_log;
	tbf->qt_rate_txtime = rtnl_tc_calc_txtime(bucket, rate);
	tbf->qt_mask |= TBF_ATTR_RATE;

	return 0;
}

/**
 * Get rate of TBF qdisc.
 * @arg qdisc		TBF qdisc.
 * @return Rate in bytes per seconds or a negative error code.
 */
int rtnl_qdisc_tbf_get_rate(struct rtnl_qdisc *qdisc)
{
	struct rtnl_tbf *tbf;

	tbf = tbf_qdisc(qdisc);
	if (tbf && (tbf->qt_mask & TBF_ATTR_RATE))
		return tbf->qt_rate.rs_rate;
	else
		return -1;
}

/**
 * Get rate bucket size of TBF qdisc.
 * @arg qdisc		TBF qdisc.
 * @return Size of rate bucket or a negative error code.
 */
int rtnl_qdisc_tbf_get_rate_bucket(struct rtnl_qdisc *qdisc)
{
	struct rtnl_tbf *tbf;

	tbf = tbf_qdisc(qdisc);
	if (tbf && (tbf->qt_mask & TBF_ATTR_RATE))
		return tbf->qt_rate_bucket;
	else
		return -1;
}

/**
 * Get rate cell size of TBF qdisc.
 * @arg qdisc		TBF qdisc.
 * @return Size of rate cell in bytes or a negative error code.
 */
int rtnl_qdisc_tbf_get_rate_cell(struct rtnl_qdisc *qdisc)
{
	struct rtnl_tbf *tbf;

	tbf = tbf_qdisc(qdisc);
	if (tbf && (tbf->qt_mask & TBF_ATTR_RATE))
		return (1 << tbf->qt_rate.rs_cell_log);
	else
		return -1;
}

/**
 * Set peak rate of TBF qdisc.
 * @arg qdisc		TBF qdisc to be modified.
 * @arg rate		New peak rate in bytes per second.
 * @arg bucket		Size of peakrate bucket.
 * @arg cell		Size of a peakrate cell or 0 to get default value.
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_tbf_set_peakrate(struct rtnl_qdisc *qdisc, int rate, int bucket,
				int cell)
{
	struct rtnl_tbf *tbf;
	int cell_log;
	
	tbf = tbf_alloc(qdisc);
	if (!tbf)
		return -NLE_NOMEM;

	cell_log = calc_cell_log(cell, bucket);
	if (cell_log < 0)
		return cell_log;

	tbf->qt_peakrate.rs_rate = rate;
	tbf->qt_peakrate_bucket = bucket;
	tbf->qt_peakrate.rs_cell_log = cell_log;
	tbf->qt_peakrate_txtime = rtnl_tc_calc_txtime(bucket, rate);
	
	tbf->qt_mask |= TBF_ATTR_PEAKRATE;

	return 0;
}

/**
 * Get peak rate of TBF qdisc.
 * @arg qdisc		TBF qdisc.
 * @return Peak rate in bytes per seconds or a negative error code.
 */
int rtnl_qdisc_tbf_get_peakrate(struct rtnl_qdisc *qdisc)
{
	struct rtnl_tbf *tbf;

	tbf = tbf_qdisc(qdisc);
	if (tbf && (tbf->qt_mask & TBF_ATTR_PEAKRATE))
		return tbf->qt_peakrate.rs_rate;
	else
		return -1;
}

/**
 * Get peak rate bucket size of TBF qdisc.
 * @arg qdisc		TBF qdisc.
 * @return Size of peak rate bucket or a negative error code.
 */
int rtnl_qdisc_tbf_get_peakrate_bucket(struct rtnl_qdisc *qdisc)
{
	struct rtnl_tbf *tbf;

	tbf = tbf_qdisc(qdisc);
	if (tbf && (tbf->qt_mask & TBF_ATTR_PEAKRATE))
		return tbf->qt_peakrate_bucket;
	else
		return -1;
}

/**
 * Get peak rate cell size of TBF qdisc.
 * @arg qdisc		TBF qdisc.
 * @return Size of peak rate cell in bytes or a negative error code.
 */
int rtnl_qdisc_tbf_get_peakrate_cell(struct rtnl_qdisc *qdisc)
{
	struct rtnl_tbf *tbf;

	tbf = tbf_qdisc(qdisc);
	if (tbf && (tbf->qt_mask & TBF_ATTR_PEAKRATE))
		return (1 << tbf->qt_peakrate.rs_cell_log);
	else
		return -1;
}

/** @} */

static struct rtnl_qdisc_ops tbf_qdisc_ops = {
	.qo_kind		= "tbf",
	.qo_msg_parser		= tbf_msg_parser,
	.qo_dump = {
	    [NL_DUMP_LINE]	= tbf_dump_line,
	    [NL_DUMP_DETAILS]	= tbf_dump_details,
	},
	.qo_free_data		= tbf_free_data,
	.qo_get_opts		= tbf_get_opts,
};

static void __init tbf_init(void)
{
	rtnl_qdisc_register(&tbf_qdisc_ops);
}

static void __exit tbf_exit(void)
{
	rtnl_qdisc_unregister(&tbf_qdisc_ops);
}

/** @} */
