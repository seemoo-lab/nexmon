/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2018 Volodymyr Bendiuga <volodymyr.bendiuga@westermo.se>
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/qdisc/mqprio.h>

#include "tc-api.h"

/** @cond SKIP */
struct rtnl_mqprio {
	uint8_t qm_num_tc;
	uint8_t qm_prio_map[TC_QOPT_BITMASK + 1];
	uint8_t qm_hw;
	uint16_t qm_count[TC_QOPT_MAX_QUEUE];
	uint16_t qm_offset[TC_QOPT_MAX_QUEUE];
	uint16_t qm_mode;
	uint16_t qm_shaper;
	uint64_t qm_min_rate[TC_QOPT_MAX_QUEUE];
	uint64_t qm_max_rate[TC_QOPT_MAX_QUEUE];
	uint32_t qm_mask;
};

#define SCH_MQPRIO_ATTR_NUMTC           (1 << 0)
#define SCH_MQPRIO_ATTR_PRIOMAP         (1 << 1)
#define SCH_MQPRIO_ATTR_HW              (1 << 2)
#define SCH_MQPRIO_ATTR_QUEUE           (1 << 3)
#define SCH_MQPRIO_ATTR_MODE            (1 << 4)
#define SCH_MQPRIO_ATTR_SHAPER          (1 << 5)
#define SCH_MQPRIO_ATTR_MIN_RATE        (1 << 6)
#define SCH_MQPRIO_ATTR_MAX_RATE        (1 << 7)
/** @endcond */

static struct nla_policy mqprio_policy[TCA_MQPRIO_MAX + 1] = {
	[TCA_MQPRIO_MODE]       = { .minlen = sizeof(uint16_t) },
	[TCA_MQPRIO_SHAPER]     = { .minlen = sizeof(uint16_t) },
	[TCA_MQPRIO_MIN_RATE64] = { .type = NLA_NESTED },
	[TCA_MQPRIO_MAX_RATE64] = { .type = NLA_NESTED },
};

static int mqprio_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct rtnl_mqprio *mqprio = data;
	struct tc_mqprio_qopt *qopt;
	struct nlattr *attr;
	int len, rem, i, err;

	if (tc->tc_opts->d_size < sizeof(*qopt))
		return -NLE_INVAL;

	qopt = (struct tc_mqprio_qopt *) tc->tc_opts->d_data;
	mqprio->qm_num_tc = qopt->num_tc;
	mqprio->qm_hw = qopt->hw;
	memcpy(mqprio->qm_prio_map, qopt->prio_tc_map,
	       TC_QOPT_MAX_QUEUE * sizeof(uint8_t));
	memcpy(mqprio->qm_count, qopt->count,
	       TC_QOPT_MAX_QUEUE * sizeof(uint16_t));
	memcpy(mqprio->qm_offset, qopt->offset,
	       TC_QOPT_MAX_QUEUE * sizeof(uint16_t));
	mqprio->qm_mask = (SCH_MQPRIO_ATTR_NUMTC | SCH_MQPRIO_ATTR_PRIOMAP |
	                   SCH_MQPRIO_ATTR_QUEUE | SCH_MQPRIO_ATTR_HW);

	len = tc->tc_opts->d_size - NLA_ALIGN(sizeof(*qopt));

	if (len > 0) {
		struct nlattr *tb[TCA_MQPRIO_MAX + 1];

		err = nla_parse(tb, TCA_MQPRIO_MAX, (struct nlattr *)
		                ((char *) tc->tc_opts->d_data + NLA_ALIGN(sizeof(*qopt))),
		                len, mqprio_policy);
		if (err < 0)
			return err;

		if (tb[TCA_MQPRIO_MODE]) {
			mqprio->qm_mode = nla_get_u16(tb[TCA_MQPRIO_MODE]);
			mqprio->qm_mask |= SCH_MQPRIO_ATTR_MODE;
		}

		if (tb[TCA_MQPRIO_SHAPER]) {
			mqprio->qm_shaper = nla_get_u16(tb[TCA_MQPRIO_SHAPER]);
			mqprio->qm_mask |= SCH_MQPRIO_ATTR_SHAPER;
		}

		if (tb[TCA_MQPRIO_MIN_RATE64]) {
			i = 0;
			nla_for_each_nested(attr, tb[TCA_MQPRIO_MIN_RATE64], rem) {
				if (nla_type(attr) != TCA_MQPRIO_MIN_RATE64)
					return -EINVAL;

				if (i >= mqprio->qm_num_tc)
					break;

				mqprio->qm_min_rate[i] = nla_get_u64(attr);
			}

			mqprio->qm_mask |= SCH_MQPRIO_ATTR_MIN_RATE;
		}

		if (tb[TCA_MQPRIO_MAX_RATE64]) {
			i = 0;
			nla_for_each_nested(attr, tb[TCA_MQPRIO_MAX_RATE64], rem) {
				if (nla_type(attr) != TCA_MQPRIO_MAX_RATE64)
					return -EINVAL;

				if (i >= mqprio->qm_num_tc)
					break;

				mqprio->qm_max_rate[i] = nla_get_u64(attr);
			}

			mqprio->qm_mask |= SCH_MQPRIO_ATTR_MAX_RATE;
		}
	}

	return 0;
}

static int mqprio_msg_fill(struct rtnl_tc *tc, void *data, struct nl_msg *msg)
{
	struct rtnl_mqprio *mqprio = data;
	struct tc_mqprio_qopt qopt = { 0 };
	struct nlattr *nest = NULL;
	int i;

	if (!mqprio ||
	    !(mqprio->qm_mask & SCH_MQPRIO_ATTR_NUMTC) ||
	    !(mqprio->qm_mask & SCH_MQPRIO_ATTR_PRIOMAP) ||
	    !(mqprio->qm_mask & SCH_MQPRIO_ATTR_QUEUE))
		return -NLE_INVAL;

	if (!(mqprio->qm_mask & SCH_MQPRIO_ATTR_HW))
		qopt.hw = 0;
	else
		qopt.hw = mqprio->qm_hw;

	qopt.num_tc = mqprio->qm_num_tc;
	memcpy(qopt.count, mqprio->qm_count, TC_QOPT_MAX_QUEUE * sizeof(uint16_t));
	memcpy(qopt.offset, mqprio->qm_offset, TC_QOPT_MAX_QUEUE * sizeof(uint16_t));
	memcpy(qopt.prio_tc_map, mqprio->qm_prio_map, TC_QOPT_MAX_QUEUE * sizeof(uint8_t));

	nlmsg_append(msg, &qopt, sizeof(qopt), NL_DONTPAD);

	if (mqprio->qm_hw) {
		if (mqprio->qm_mask & SCH_MQPRIO_ATTR_MODE)
			NLA_PUT_U16(msg, TCA_MQPRIO_MODE, mqprio->qm_mode);

		if (mqprio->qm_mask & SCH_MQPRIO_ATTR_SHAPER)
			NLA_PUT_U16(msg, TCA_MQPRIO_SHAPER, mqprio->qm_shaper);

		if (mqprio->qm_mask & SCH_MQPRIO_ATTR_MIN_RATE) {
			nest = nla_nest_start(msg, TCA_MQPRIO_MIN_RATE64);
			if (!nest)
				goto nla_put_failure;

			for (i = 0; i < mqprio->qm_num_tc; i++) {
				if (nla_put(msg, TCA_MQPRIO_MIN_RATE64,
				            sizeof(mqprio->qm_min_rate[i]),
				            &mqprio->qm_min_rate[i]) < 0)
					goto nla_nest_cancel;
			}
			nla_nest_end(msg, nest);
		}

		if (mqprio->qm_mask & SCH_MQPRIO_ATTR_MAX_RATE) {
			nest = nla_nest_start(msg, TCA_MQPRIO_MAX_RATE64);
			if (!nest)
				goto nla_put_failure;

			for (i = 0; i < mqprio->qm_num_tc; i++) {
				if (nla_put(msg, TCA_MQPRIO_MAX_RATE64,
				            sizeof(mqprio->qm_max_rate[i]),
				            &mqprio->qm_max_rate[i]) < 0)
					goto nla_nest_cancel;
			}
			nla_nest_end(msg, nest);
		}
	}

	return 0;

nla_nest_cancel:
	nla_nest_cancel(msg, nest);
	return -NLE_MSGSIZE;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static void mqprio_dump_line(struct rtnl_tc *tc, void *data,
                             struct nl_dump_params *p)
{
	struct rtnl_mqprio *mqprio = data;

	if (mqprio)
		nl_dump(p, " num_tc %u", mqprio->qm_num_tc);
}

static void mqprio_dump_details(struct rtnl_tc *tc, void *data,
                                struct nl_dump_params *p)
{
	struct rtnl_mqprio *mqprio = data;
	int i;

	if (!mqprio)
		return;

	nl_dump(p, "map [");

	for (i = 0; i <= TC_QOPT_BITMASK; i++)
		nl_dump(p, "%u%s", mqprio->qm_prio_map[i],
			i < TC_QOPT_BITMASK ? " " : "");

	nl_dump(p, "]\n");
	nl_new_line(p);
}

/**
 * @name Attribute Modification
 * @{
 */

/**
 * Set number of traffic classes.
 * @arg qdisc           MQPRIO qdisc to be modified.
 * @arg num_tc          Number of traffic classes to create.
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_set_num_tc(struct rtnl_qdisc *qdisc, int num_tc)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data(TC_CAST(qdisc))))
		return -NLE_NOMEM;

	mqprio->qm_num_tc = num_tc;
	mqprio->qm_mask |= SCH_MQPRIO_ATTR_NUMTC;
	return 0;
}

/**
 * Get number of traffic classes of MQPRIO qdisc.
 * @arg qdisc           MQPRIO qdisc.
 * @return Number of traffic classes or a negative error code.
 */
int rtnl_qdisc_mqprio_get_num_tc(struct rtnl_qdisc *qdisc)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data_peek(TC_CAST(qdisc))))
		return -NLE_INVAL;

	if (mqprio->qm_mask & SCH_MQPRIO_ATTR_NUMTC)
		return mqprio->qm_num_tc;
	else
		return -NLE_MISSING_ATTR;
}

/**
 * Set priomap of the MQPRIO qdisc.
 * @arg qdisc           MQPRIO qdisc to be modified.
 * @arg priomap         New priority mapping.
 * @arg len             Length of priomap (# of elements).
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_set_priomap(struct rtnl_qdisc *qdisc, uint8_t priomap[],
                                int len)
{
	struct rtnl_mqprio *mqprio;
	int i;

	if (!(mqprio = rtnl_tc_data(TC_CAST(qdisc))))
		return -NLE_NOMEM;

	if (!(mqprio->qm_mask & SCH_MQPRIO_ATTR_NUMTC))
		return -NLE_MISSING_ATTR;

	if (len > TC_QOPT_BITMASK + 1)
		return -NLE_RANGE;

	for (i = 0; i < len; i++) {
		if (priomap[i] > mqprio->qm_num_tc)
			return -NLE_RANGE;
	}

	memset(mqprio->qm_prio_map, 0, sizeof(mqprio->qm_prio_map));
	memcpy(mqprio->qm_prio_map, priomap, len * sizeof(uint8_t));
	mqprio->qm_mask |= SCH_MQPRIO_ATTR_PRIOMAP;

	return 0;
}

/**
 * Get priomap of MQPRIO qdisc.
 * @arg qdisc           MQPRIO qdisc.
 * @return Priority mapping as array of size TC_QOPT_BANDS+1
 *         or NULL if an error occured.
 */
uint8_t *rtnl_qdisc_mqprio_get_priomap(struct rtnl_qdisc *qdisc)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data_peek(TC_CAST(qdisc))))
		return NULL;

	if (mqprio->qm_mask & SCH_MQPRIO_ATTR_PRIOMAP)
		return mqprio->qm_prio_map;
	else
		return NULL;
}

/**
 * Offload to HW or run in SW (default).
 * @arg qdisc           MQPRIO qdisc to be modified.
 * @arg offload         1 - offload to HW, 0 - run in SW only (default).
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_hw_offload(struct rtnl_qdisc *qdisc, int offload)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data(TC_CAST(qdisc))))
		return -NLE_NOMEM;

	switch (offload) {
	case 0:
	case 1:
		mqprio->qm_hw = offload;
		break;
	default:
		return -NLE_INVAL;
	}

	mqprio->qm_mask |= SCH_MQPRIO_ATTR_HW;
	return 0;
}

/**
 * Check whether running in HW or SW.
 * @arg qdisc           MQPRIO qdisc to be modified.
 * @return 0 if running in SW, otherwise 1 (HW)
 */
int rtnl_qdisc_mqprio_get_hw_offload(struct rtnl_qdisc *qdisc)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data_peek(TC_CAST(qdisc))))
		return -NLE_INVAL;

	if (mqprio->qm_mask & SCH_MQPRIO_ATTR_HW)
		return mqprio->qm_hw;

	return 0;
}

/**
 * Set tc queue of the MQPRIO qdisc.
 * @arg qdisc           MQPRIO qdisc to be modified.
 * @arg count           count of queue range for each traffic class
 * @arg offset          offset of queue range for each traffic class
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_set_queue(struct rtnl_qdisc *qdisc, uint16_t count[],
                                uint16_t offset[], int len)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data(TC_CAST(qdisc))))
		return -NLE_NOMEM;

	if (!(mqprio->qm_mask & SCH_MQPRIO_ATTR_NUMTC))
		return -NLE_MISSING_ATTR;

	if (len < 0 || len > TC_QOPT_MAX_QUEUE)
		return -NLE_RANGE;

	memset(mqprio->qm_count, 0, sizeof(mqprio->qm_count));
	memset(mqprio->qm_offset, 0, sizeof(mqprio->qm_offset));
	memcpy(mqprio->qm_count, count, len * sizeof(uint16_t));
	memcpy(mqprio->qm_offset, offset, len * sizeof(uint16_t));
	mqprio->qm_mask |= SCH_MQPRIO_ATTR_QUEUE;

	return 0;
}

/**
 * Get tc queue of the MQPRIO qdisc.
 * @arg qdisc           MQPRIO qdisc to be modified.
 * @arg count           count of queue range for each traffic class
 * @arg offset          offset of queue range for each traffic class
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_get_queue(struct rtnl_qdisc *qdisc, uint16_t *count,
                                uint16_t *offset)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data_peek(TC_CAST(qdisc))))
		return -NLE_INVAL;

	if (!(mqprio->qm_mask & SCH_MQPRIO_ATTR_QUEUE))
		return -NLE_MISSING_ATTR;

	memcpy(count, mqprio->qm_count, TC_QOPT_MAX_QUEUE * sizeof(uint16_t));
	memcpy(offset, mqprio->qm_offset, TC_QOPT_MAX_QUEUE * sizeof(uint16_t));

	return 0;
}

/**
 * Set mode of mqprio Qdisc
 * @arg qdisc           MQPRIO qdisc to be modified.
 * @arg mode            one of: TC_MQPRIO_MODE_DCB, TC_MQPRIO_MODE_CHANNEL
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_set_mode(struct rtnl_qdisc *qdisc, uint16_t mode)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data(TC_CAST(qdisc))))
		return -NLE_NOMEM;

	if (!(mqprio->qm_mask & SCH_MQPRIO_ATTR_HW))
		return -NLE_MISSING_ATTR;

	mqprio->qm_mode = mode;
	mqprio->qm_mask |= SCH_MQPRIO_ATTR_MODE;

	return 0;
}

/**
 * Get mode of mqprio Qdisc
 * @arg qdisc           MQPRIO qdisc.
 * @return mode on success or negative error code.
 */
int rtnl_qdisc_mqprio_get_mode(struct rtnl_qdisc *qdisc)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data_peek(TC_CAST(qdisc))))
		return -NLE_INVAL;

	if (mqprio->qm_mask & SCH_MQPRIO_ATTR_MODE)
		return mqprio->qm_mode;
	else
		return -NLE_MISSING_ATTR;
}

/**
 * Set shaper of mqprio Qdisc
 * @arg qdisc           MQPRIO qdisc to be modified.
 * @arg shaper          one of: TC_MQPRIO_SHAPER_DCB, TC_MQPRIO_SHAPER_BW_RATE
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_set_shaper(struct rtnl_qdisc *qdisc, uint16_t shaper)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data(TC_CAST(qdisc))))
		return -NLE_NOMEM;

	if (!(mqprio->qm_mask & SCH_MQPRIO_ATTR_HW))
		return -NLE_MISSING_ATTR;

	mqprio->qm_shaper = shaper;
	mqprio->qm_mask |= SCH_MQPRIO_ATTR_SHAPER;

	return 0;
}

/**
 * Get shaper of mqprio Qdisc
 * @arg qdisc           MQPRIO qdisc.
 * @return shaper on success or negative error code.
 */
int rtnl_qdisc_mqprio_get_shaper(struct rtnl_qdisc *qdisc)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data_peek(TC_CAST(qdisc))))
		return -NLE_INVAL;

	if (mqprio->qm_mask & SCH_MQPRIO_ATTR_SHAPER)
		return mqprio->qm_shaper;
	else
		return -NLE_MISSING_ATTR;
}

/**
 * Set minimum value of bandwidth rate limit for each traffic class
 * @arg qdisc           MQPRIO qdisc.
 * @arg min             minimum rate for each traffic class
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_set_min_rate(struct rtnl_qdisc *qdisc, uint64_t min[], int len)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data(TC_CAST(qdisc))))
		return -NLE_NOMEM;

	if (!(mqprio->qm_mask & SCH_MQPRIO_ATTR_SHAPER))
		return -NLE_MISSING_ATTR;

	if (mqprio->qm_shaper != TC_MQPRIO_SHAPER_BW_RATE)
		return -NLE_INVAL;

	if (len < 0 || len > TC_QOPT_MAX_QUEUE)
		return -NLE_RANGE;

	memset(mqprio->qm_min_rate, 0, sizeof(mqprio->qm_min_rate));
	memcpy(mqprio->qm_min_rate, min, len * sizeof(uint64_t));
	mqprio->qm_mask |= SCH_MQPRIO_ATTR_MIN_RATE;

	return 0;
}

/**
 * Get minimum value of bandwidth rate limit for each traffic class
 * @arg qdisc           MQPRIO qdisc.
 * @arg min             minimum rate for each traffic class
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_get_min_rate(struct rtnl_qdisc *qdisc, uint64_t *min)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data_peek(TC_CAST(qdisc))))
		return -NLE_INVAL;

	if (mqprio->qm_mask & SCH_MQPRIO_ATTR_MIN_RATE) {
		memcpy(min, mqprio->qm_min_rate, TC_QOPT_MAX_QUEUE * sizeof(uint64_t));
		return 0;
	}

	return -NLE_MISSING_ATTR;
}

/**
 * Set maximum value of bandwidth rate limit for each traffic class
 * @arg qdisc           MQPRIO qdisc.
 * @arg max             maximum rate for each traffic class
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_set_max_rate(struct rtnl_qdisc *qdisc, uint64_t max[], int len)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data(TC_CAST(qdisc))))
		return -NLE_NOMEM;

	if (!(mqprio->qm_mask & SCH_MQPRIO_ATTR_SHAPER))
		return -NLE_MISSING_ATTR;

	if (mqprio->qm_shaper != TC_MQPRIO_SHAPER_BW_RATE)
		return -NLE_INVAL;

	if (len < 0 || len > TC_QOPT_MAX_QUEUE)
		return -NLE_RANGE;

	memset(mqprio->qm_max_rate, 0, sizeof(mqprio->qm_max_rate));
	memcpy(mqprio->qm_max_rate, max, len * sizeof(uint64_t));
	mqprio->qm_mask |= SCH_MQPRIO_ATTR_MAX_RATE;

	return 0;
}

/**
 * Get maximum value of bandwidth rate limit for each traffic class
 * @arg qdisc           MQPRIO qdisc.
 * @arg min             maximum rate for each traffic class
 * @return 0 on success or a negative error code.
 */
int rtnl_qdisc_mqprio_get_max_rate(struct rtnl_qdisc *qdisc, uint64_t *max)
{
	struct rtnl_mqprio *mqprio;

	if (!(mqprio = rtnl_tc_data_peek(TC_CAST(qdisc))))
		return -NLE_INVAL;

	if (mqprio->qm_mask & SCH_MQPRIO_ATTR_MAX_RATE) {
		memcpy(max, mqprio->qm_max_rate, TC_QOPT_MAX_QUEUE * sizeof(uint64_t));
		return 0;
	}

	return -NLE_MISSING_ATTR;
}

/** @} */

static struct rtnl_tc_ops mqprio_ops = {
	.to_kind                = "mqprio",
	.to_type                = RTNL_TC_TYPE_QDISC,
	.to_size                = sizeof(struct rtnl_mqprio),
	.to_msg_parser          = mqprio_msg_parser,
	.to_dump = {
	    [NL_DUMP_LINE]      = mqprio_dump_line,
	    [NL_DUMP_DETAILS]   = mqprio_dump_details,
	},
	.to_msg_fill            = mqprio_msg_fill,
};

static void _nl_init mqprio_init(void)
{
	rtnl_tc_register(&mqprio_ops);
}

static void _nl_exit mqprio_exit(void)
{
	rtnl_tc_unregister(&mqprio_ops);
}

/** @} */
