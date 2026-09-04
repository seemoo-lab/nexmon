/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2016 Sushma Sitaram <sushma.sitaram@intel.com>
 */

/**
 * @ingroup act
 * @defgroup act_gact GACT Editing
 *
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/route/act/gact.h>

#include "tc-api.h"

struct rtnl_gact {
	struct tc_gact g_parm;
};

static struct nla_policy gact_policy[TCA_GACT_MAX + 1] = {
	[TCA_GACT_PARMS]             = { .minlen = sizeof(struct tc_gact) },
};

static int gact_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct rtnl_gact *u = data;
	struct nlattr *tb[TCA_GACT_MAX + 1];
	int err;

	err = tca_parse(tb, TCA_GACT_MAX, tc, gact_policy);
	if (err < 0)
		return err;

	if (!tb[TCA_GACT_PARMS])
		return -NLE_MISSING_ATTR;

	nla_memcpy(&u->g_parm, tb[TCA_GACT_PARMS], sizeof(u->g_parm));

	return 0;
}

static void gact_free_data(struct rtnl_tc *tc, void *data)
{
}

static void gact_dump_line(struct rtnl_tc *tc, void *data,
			  struct nl_dump_params *p)
{
	struct rtnl_gact *u = data;

	if (!u)
		return;

	switch(u->g_parm.action){
	case TC_ACT_UNSPEC:
		nl_dump(p, " continue");
		break;
	case TC_ACT_SHOT:
		nl_dump(p, " drop");
		break;
	case TC_ACT_RECLASSIFY:
		nl_dump(p, " reclassify");
		break;
	case TC_ACT_OK:
		nl_dump(p, " pass");
		break;
	}

}

static void gact_dump_details(struct rtnl_tc *tc, void *data,
			     struct nl_dump_params *p)
{
}

static void gact_dump_stats(struct rtnl_tc *tc, void *data,
			   struct nl_dump_params *p)
{
	struct rtnl_gact *u = data;

	if (!u)
		return;
	/* TODO */
}


static int gact_msg_fill(struct rtnl_tc *tc, void *data, struct nl_msg *msg)
{
	struct rtnl_gact *u = data;

	if (!u)
		return 0;

	NLA_PUT(msg, TCA_GACT_PARMS, sizeof(u->g_parm), &u->g_parm);

	return 0;

nla_put_failure:
	return -NLE_NOMEM;
}

/**
 * @name Attribute Modifications
 * @{
 */

int rtnl_gact_set_action(struct rtnl_act *act, int action)
{
	struct rtnl_gact *u;

	if (!(u = (struct rtnl_gact *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	u->g_parm.action = action;

	return 0;
}

int rtnl_gact_get_action(struct rtnl_act *act)
{
	struct rtnl_gact *u;

	if (!(u = (struct rtnl_gact *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;
	return u->g_parm.action;
}


/** @} */

static struct rtnl_tc_ops gact_ops = {
	.to_kind		= "gact",
	.to_type		= RTNL_TC_TYPE_ACT,
	.to_size		= sizeof(struct rtnl_gact),
	.to_msg_parser		= gact_msg_parser,
	.to_free_data		= gact_free_data,
	.to_clone		= NULL,
	.to_msg_fill		= gact_msg_fill,
	.to_dump = {
	    [NL_DUMP_LINE]	= gact_dump_line,
	    [NL_DUMP_DETAILS]	= gact_dump_details,
	    [NL_DUMP_STATS]	= gact_dump_stats,
	},
};

static void _nl_init gact_init(void)
{
	rtnl_tc_register(&gact_ops);
}

static void _nl_exit gact_exit(void)
{
	rtnl_tc_unregister(&gact_ops);
}

/** @} */
