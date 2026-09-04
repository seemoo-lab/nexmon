/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2014 Cong Wang <xiyou.wangcong@gmail.com>
 */

/**
 * @ingroup qdisc
 * @ingroup class
 * @defgroup qdisc_hfsc Hierarchical Fair Service Curve (HFSC)
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>
#include <netlink/route/link.h>
#include <netlink/route/qdisc/hfsc.h>

#include "tc-api.h"

/** @cond SKIP */
struct rtnl_hfsc_qdisc {
	uint32_t qh_defcls;
	uint32_t qh_mask;
};

struct rtnl_hfsc_class {
	struct tc_service_curve ch_rsc;
	struct tc_service_curve ch_fsc;
	struct tc_service_curve ch_usc;
	uint32_t ch_mask;
};

#define SCH_HFSC_CLS_HAS_RSC		0x001
#define SCH_HFSC_CLS_HAS_FSC		0x002
#define SCH_HFSC_CLS_HAS_USC		0x004

#define SCH_HFSC_QD_HAS_DEFCLS		0x01
/** @endcond */

static struct nla_policy hfsc_policy[TCA_HFSC_MAX + 1] = {
	[TCA_HFSC_RSC]  = { .minlen = sizeof(struct tc_service_curve) },
	[TCA_HFSC_FSC]  = { .minlen = sizeof(struct tc_service_curve) },
	[TCA_HFSC_USC]  = { .minlen = sizeof(struct tc_service_curve) },
};

static int hfsc_qdisc_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct rtnl_hfsc_qdisc *hfsc = data;
	struct tc_hfsc_qopt *opts;

	opts = (struct tc_hfsc_qopt *) tc->tc_opts->d_data;
	hfsc->qh_defcls = opts->defcls;
	hfsc->qh_mask |= SCH_HFSC_QD_HAS_DEFCLS;
	return 0;
}

static int hfsc_class_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct nlattr *tb[TCA_HFSC_MAX + 1];
	struct rtnl_hfsc_class *hfsc = data;
	int err;

	if ((err = tca_parse(tb, TCA_HFSC_MAX, tc, hfsc_policy)) < 0)
		return err;

	if (tb[TCA_HFSC_RSC]) {
		struct tc_service_curve tsc;

		nla_memcpy(&tsc, tb[TCA_HFSC_RSC], sizeof(tsc));
		hfsc->ch_rsc = tsc;
		hfsc->ch_mask |= SCH_HFSC_CLS_HAS_RSC;
	}

	if (tb[TCA_HFSC_FSC]) {
		struct tc_service_curve tsc;

		nla_memcpy(&tsc, tb[TCA_HFSC_FSC], sizeof(tsc));
		hfsc->ch_fsc = tsc;
		hfsc->ch_mask |= SCH_HFSC_CLS_HAS_FSC;
	}

	if (tb[TCA_HFSC_USC]) {
		struct tc_service_curve tsc;

		nla_memcpy(&tsc, tb[TCA_HFSC_USC], sizeof(tsc));
		hfsc->ch_usc = tsc;
		hfsc->ch_mask |= SCH_HFSC_CLS_HAS_USC;
	}

	return 0;
}

static void hfsc_qdisc_dump_line(struct rtnl_tc *tc, void *data,
				struct nl_dump_params *p)
{
	struct rtnl_hfsc_qdisc *hfsc = data;

	if (!hfsc)
		return;

	if (hfsc->qh_mask & SCH_HFSC_QD_HAS_DEFCLS) {
		char buf[64];
		nl_dump(p, " default-class %s",
			rtnl_tc_handle2str(hfsc->qh_defcls, buf, sizeof(buf)));
	}
}

static void hfsc_dump_tsc(struct nl_dump_params *p, struct tc_service_curve *tsc)
{
	nl_dump(p, " m1 %u d %u m2 %u\n", tsc->m1, tsc->d, tsc->m2);
}

static void hfsc_class_dump_line(struct rtnl_tc *tc, void *data,
				struct nl_dump_params *p)
{
	struct rtnl_hfsc_class *hfsc = data;

	if (!hfsc)
		return;
	if (hfsc->ch_mask & SCH_HFSC_CLS_HAS_RSC)
		hfsc_dump_tsc(p, &hfsc->ch_rsc);
	if (hfsc->ch_mask & SCH_HFSC_CLS_HAS_FSC)
		hfsc_dump_tsc(p, &hfsc->ch_fsc);
	if (hfsc->ch_mask & SCH_HFSC_CLS_HAS_USC)
		hfsc_dump_tsc(p, &hfsc->ch_usc);
}

static void hfsc_class_dump_details(struct rtnl_tc *tc, void *data,
				   struct nl_dump_params *p)
{
	return;
}

static int hfsc_qdisc_msg_fill(struct rtnl_tc *tc, void *data,
			      struct nl_msg *msg)
{
	struct rtnl_hfsc_qdisc *hfsc = data;
	struct tc_hfsc_qopt opts = {0};

	if (!hfsc)
		BUG();

	opts.defcls = hfsc->qh_defcls;
	return nlmsg_append(msg, &opts, sizeof(opts), NL_DONTPAD);
}

static int hfsc_class_msg_fill(struct rtnl_tc *tc, void *data,
			      struct nl_msg *msg)
{
	struct rtnl_hfsc_class *hfsc = data;
	struct tc_service_curve tsc;

	if (!hfsc)
		BUG();

	if (hfsc->ch_mask & SCH_HFSC_CLS_HAS_RSC) {
		tsc = hfsc->ch_rsc;
		NLA_PUT(msg, TCA_HFSC_RSC, sizeof(tsc), &tsc);
	}

	if (hfsc->ch_mask & SCH_HFSC_CLS_HAS_FSC) {
		tsc = hfsc->ch_fsc;
		NLA_PUT(msg, TCA_HFSC_FSC, sizeof(tsc), &tsc);
	}

	if (hfsc->ch_mask & SCH_HFSC_CLS_HAS_USC) {
		tsc = hfsc->ch_usc;
		NLA_PUT(msg, TCA_HFSC_USC, sizeof(tsc), &tsc);
	}

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static struct rtnl_tc_ops hfsc_qdisc_ops;
static struct rtnl_tc_ops hfsc_class_ops;

static struct rtnl_hfsc_qdisc *hfsc_qdisc_data(const struct rtnl_qdisc *qdisc, int *err)
{
	return rtnl_tc_data_check(TC_CAST(qdisc), &hfsc_qdisc_ops, err);
}

static struct rtnl_hfsc_class *hfsc_class_data(const struct rtnl_class *class, int *err)
{
	return rtnl_tc_data_check(TC_CAST(class), &hfsc_class_ops, err);
}

/**
 * @name Attribute Modifications
 * @{
 */

/**
 * Return default class of HFSC qdisc
 * @arg qdisc		hfsc qdisc object
 *
 * Returns the classid of the class where all unclassified traffic
 * goes to.
 *
 * @return classid or TC_H_UNSPEC if unspecified.
 */
uint32_t rtnl_qdisc_hfsc_get_defcls(const struct rtnl_qdisc *qdisc)
{
	struct rtnl_hfsc_qdisc *hfsc;

	if ((hfsc = hfsc_qdisc_data(qdisc, NULL)) &&
	    (hfsc->qh_mask & SCH_HFSC_QD_HAS_DEFCLS))
		return hfsc->qh_defcls;

	return TC_H_UNSPEC;
}

/**
 * Set default class of the hfsc qdisc to the specified value
 * @arg qdisc		qdisc to change
 * @arg defcls		new default class
 */
int rtnl_qdisc_hfsc_set_defcls(struct rtnl_qdisc *qdisc, uint32_t defcls)
{
	struct rtnl_hfsc_qdisc *hfsc;
	int err;

	if (!(hfsc = hfsc_qdisc_data(qdisc, &err)))
		return err;

	hfsc->qh_defcls = defcls;
	hfsc->qh_mask |= SCH_HFSC_QD_HAS_DEFCLS;

	return 0;
}

int rtnl_class_hfsc_get_rsc(const struct rtnl_class *class, struct tc_service_curve *tsc)
{
	struct rtnl_hfsc_class *hfsc;
	int err = -NLE_OPNOTSUPP;

	if ((hfsc = hfsc_class_data(class, &err)) &&
	    (hfsc->ch_mask & SCH_HFSC_CLS_HAS_RSC)) {
		*tsc = hfsc->ch_rsc;
		return 0;
	}

	return err;
}

int rtnl_class_hfsc_set_rsc(struct rtnl_class *class, const struct tc_service_curve *tsc)
{
	struct rtnl_hfsc_class *hfsc;
	int err;

	if (!(hfsc = hfsc_class_data(class, &err)))
		return err;

	hfsc->ch_rsc = *tsc;
	hfsc->ch_mask |= SCH_HFSC_CLS_HAS_RSC;

	return 0;
}

int rtnl_class_hfsc_get_fsc(const struct rtnl_class *class, struct tc_service_curve *tsc)
{
	struct rtnl_hfsc_class *hfsc;
	int err = -NLE_OPNOTSUPP;

	if ((hfsc = hfsc_class_data(class, &err)) &&
	    (hfsc->ch_mask & SCH_HFSC_CLS_HAS_FSC)) {
		*tsc = hfsc->ch_fsc;
		return 0;
	}

	return err;
}

int rtnl_class_hfsc_set_fsc(struct rtnl_class *class, const struct tc_service_curve *tsc)
{
	struct rtnl_hfsc_class *hfsc;
	int err;

	if (!(hfsc = hfsc_class_data(class, &err)))
		return err;

	hfsc->ch_fsc = *tsc;
	hfsc->ch_mask |= SCH_HFSC_CLS_HAS_FSC;

	return 0;
}

int rtnl_class_hfsc_get_usc(const struct rtnl_class *class, struct tc_service_curve *tsc)
{
	struct rtnl_hfsc_class *hfsc;
	int err = -NLE_OPNOTSUPP;

	if ((hfsc = hfsc_class_data(class, &err)) &&
	    (hfsc->ch_mask & SCH_HFSC_CLS_HAS_USC)) {
		*tsc = hfsc->ch_usc;
		return 0;
	}

	return err;
}

int rtnl_class_hfsc_set_usc(struct rtnl_class *class, const struct tc_service_curve *tsc)
{
	struct rtnl_hfsc_class *hfsc;
	int err;

	if (!(hfsc = hfsc_class_data(class, &err)))
		return err;

	hfsc->ch_usc = *tsc;
	hfsc->ch_mask |= SCH_HFSC_CLS_HAS_USC;

	return 0;
}

/** @} */

static struct rtnl_tc_ops hfsc_qdisc_ops = {
	.to_kind		= "hfsc",
	.to_type		= RTNL_TC_TYPE_QDISC,
	.to_size		= sizeof(struct rtnl_hfsc_qdisc),
	.to_msg_parser		= hfsc_qdisc_msg_parser,
	.to_dump[NL_DUMP_LINE]	= hfsc_qdisc_dump_line,
	.to_msg_fill		= hfsc_qdisc_msg_fill,
};

static struct rtnl_tc_ops hfsc_class_ops = {
	.to_kind		= "hfsc",
	.to_type		= RTNL_TC_TYPE_CLASS,
	.to_size		= sizeof(struct rtnl_hfsc_class),
	.to_msg_parser		= hfsc_class_msg_parser,
	.to_dump = {
	    [NL_DUMP_LINE]	= hfsc_class_dump_line,
	    [NL_DUMP_DETAILS]	= hfsc_class_dump_details,
	},
	.to_msg_fill		= hfsc_class_msg_fill,
};

static void _nl_init hfsc_init(void)
{
	rtnl_tc_register(&hfsc_qdisc_ops);
	rtnl_tc_register(&hfsc_class_ops);
}

static void _nl_exit hfsc_exit(void)
{
	rtnl_tc_unregister(&hfsc_qdisc_ops);
	rtnl_tc_unregister(&hfsc_class_ops);
}

/** @} */
