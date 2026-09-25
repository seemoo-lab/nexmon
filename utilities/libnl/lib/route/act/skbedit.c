/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2015 Cong Wang <xiyou.wangcong@gmail.com>
 */

/**
 * @ingroup act
 * @defgroup act_skbedit SKB Editing
 *
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/route/act/skbedit.h>

#include "nl-route.h"
#include "tc-api.h"

struct rtnl_skbedit {
	struct tc_skbedit s_parm;
	uint32_t s_flags;
	uint32_t s_mark;
	uint32_t s_prio;
	uint16_t s_queue_mapping;
};

static struct nla_policy skbedit_policy[TCA_SKBEDIT_MAX + 1] = {
	[TCA_SKBEDIT_PARMS]             = { .minlen = sizeof(struct tc_skbedit) },
	[TCA_SKBEDIT_PRIORITY]          = { .type = NLA_U32 },
	[TCA_SKBEDIT_QUEUE_MAPPING]     = { .type = NLA_U16 },
	[TCA_SKBEDIT_MARK]              = { .type = NLA_U32 },
};

static int skbedit_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct rtnl_skbedit *u = data;
	struct nlattr *tb[TCA_SKBEDIT_MAX + 1];
	int err;

	err = tca_parse(tb, TCA_SKBEDIT_MAX, tc, skbedit_policy);
	if (err < 0)
		return err;

	if (!tb[TCA_SKBEDIT_PARMS])
		return -NLE_MISSING_ATTR;

	u->s_flags = 0;
	if (tb[TCA_SKBEDIT_PRIORITY] != NULL) {
		u->s_flags |= SKBEDIT_F_PRIORITY;
		u->s_prio = nla_get_u32(tb[TCA_SKBEDIT_PRIORITY]);
	}

	if (tb[TCA_SKBEDIT_QUEUE_MAPPING] != NULL) {
		u->s_flags |= SKBEDIT_F_QUEUE_MAPPING;
		u->s_queue_mapping = nla_get_u16(tb[TCA_SKBEDIT_QUEUE_MAPPING]);
	}

	if (tb[TCA_SKBEDIT_MARK] != NULL) {
		u->s_flags |= SKBEDIT_F_MARK;
		u->s_mark = nla_get_u32(tb[TCA_SKBEDIT_MARK]);
	}

	return 0;
}

static void skbedit_free_data(struct rtnl_tc *tc, void *data)
{
}

static void skbedit_dump_line(struct rtnl_tc *tc, void *data,
			  struct nl_dump_params *p)
{
	struct rtnl_skbedit *u = data;

	if (!u)
		return;

	if (u->s_flags & SKBEDIT_F_PRIORITY)
		nl_dump(p, " priority %u", u->s_prio);

	if (u->s_flags & SKBEDIT_F_MARK)
		nl_dump(p, " mark %u", u->s_mark);

	if (u->s_flags & SKBEDIT_F_QUEUE_MAPPING)
		nl_dump(p, " queue_mapping %u", u->s_queue_mapping);

	switch(u->s_parm.action){
	case TC_ACT_UNSPEC:
		nl_dump(p, " unspecified");
		break;
	case TC_ACT_PIPE:
		nl_dump(p, " pipe");
		break;
	case TC_ACT_STOLEN:
		nl_dump(p, " stolen");
		break;
	case TC_ACT_SHOT:
		nl_dump(p, " shot");
		break;
	case TC_ACT_QUEUED:
		nl_dump(p, " queued");
		break;
	case TC_ACT_REPEAT:
		nl_dump(p, " repeat");
		break;
	}
}

static void skbedit_dump_details(struct rtnl_tc *tc, void *data,
			     struct nl_dump_params *p)
{
}

static void skbedit_dump_stats(struct rtnl_tc *tc, void *data,
			   struct nl_dump_params *p)
{
	struct rtnl_skbedit *u = data;

	if (!u)
		return;
	/* TODO */
}


static int skbedit_msg_fill(struct rtnl_tc *tc, void *data, struct nl_msg *msg)
{
	struct rtnl_skbedit *u = data;

	if (!u)
		return 0;

	NLA_PUT(msg, TCA_SKBEDIT_PARMS, sizeof(u->s_parm), &u->s_parm);

	if (u->s_flags & SKBEDIT_F_MARK)
		NLA_PUT_U32(msg, TCA_SKBEDIT_MARK, u->s_mark);

	if (u->s_flags & SKBEDIT_F_PRIORITY)
		NLA_PUT_U32(msg, TCA_SKBEDIT_PRIORITY, u->s_prio);

	if (u->s_flags & SKBEDIT_F_QUEUE_MAPPING)
		NLA_PUT_U32(msg, TCA_SKBEDIT_QUEUE_MAPPING, u->s_queue_mapping);

	return 0;

nla_put_failure:
	return -NLE_NOMEM;
}

/**
 * @name Attribute Modifications
 * @{
 */

int rtnl_skbedit_set_action(struct rtnl_act *act, int action)
{
	struct rtnl_skbedit *u;

	if (!(u = (struct rtnl_skbedit *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	u->s_parm.action = action;

	return 0;
}

int rtnl_skbedit_get_action(struct rtnl_act *act)
{
	struct rtnl_skbedit *u;

	if (!(u = (struct rtnl_skbedit *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;
	return u->s_parm.action;
}

int rtnl_skbedit_set_queue_mapping(struct rtnl_act *act, uint16_t index)
{
	struct rtnl_skbedit *u;

	if (!(u = (struct rtnl_skbedit *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	u->s_queue_mapping = index;
	u->s_flags |= SKBEDIT_F_QUEUE_MAPPING;
	return 0;
}

int rtnl_skbedit_get_queue_mapping(struct rtnl_act *act, uint16_t *index)
{
	struct rtnl_skbedit *u;

	u = (struct rtnl_skbedit *) rtnl_tc_data(TC_CAST(act));
	if (!u)
		return -NLE_NOMEM;
	if (!(u->s_flags & SKBEDIT_F_QUEUE_MAPPING))
		return -NLE_NOATTR;

	*index = u->s_queue_mapping;
	return 0;
}

int rtnl_skbedit_set_mark(struct rtnl_act *act, uint32_t mark)
{
	struct rtnl_skbedit *u;

	if (!(u = (struct rtnl_skbedit *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	u->s_mark = mark;
	u->s_flags |= SKBEDIT_F_MARK;
	return 0;
}

int rtnl_skbedit_get_mark(struct rtnl_act *act, uint32_t *mark)
{
	struct rtnl_skbedit *u;

	u = (struct rtnl_skbedit *) rtnl_tc_data(TC_CAST(act));
	if (!u)
		return -NLE_NOMEM;
	if (!(u->s_flags & SKBEDIT_F_MARK))
		return -NLE_NOATTR;

	*mark = u->s_mark;
	return 0;
}

int rtnl_skbedit_set_priority(struct rtnl_act *act, uint32_t prio)
{
	struct rtnl_skbedit *u;

	if (!(u = (struct rtnl_skbedit *) rtnl_tc_data(TC_CAST(act))))
		return -NLE_NOMEM;

	u->s_prio = prio;
	u->s_flags |= SKBEDIT_F_PRIORITY;
	return 0;
}

int rtnl_skbedit_get_priority(struct rtnl_act *act, uint32_t *prio)
{
	struct rtnl_skbedit *u;

	u = (struct rtnl_skbedit *) rtnl_tc_data(TC_CAST(act));
	if (!u)
		return -NLE_NOMEM;
	if (!(u->s_flags & SKBEDIT_F_PRIORITY))
		return -NLE_NOATTR;

	*prio = u->s_prio;
	return 0;
}

/** @} */

static struct rtnl_tc_ops skbedit_ops = {
	.to_kind		= "skbedit",
	.to_type		= RTNL_TC_TYPE_ACT,
	.to_size		= sizeof(struct rtnl_skbedit),
	.to_msg_parser		= skbedit_msg_parser,
	.to_free_data		= skbedit_free_data,
	.to_clone		= NULL,
	.to_msg_fill		= skbedit_msg_fill,
	.to_dump = {
	    [NL_DUMP_LINE]	= skbedit_dump_line,
	    [NL_DUMP_DETAILS]	= skbedit_dump_details,
	    [NL_DUMP_STATS]	= skbedit_dump_stats,
	},
};

static void _nl_init skbedit_init(void)
{
	rtnl_tc_register(&skbedit_ops);
}

static void _nl_exit skbedit_exit(void)
{
	rtnl_tc_unregister(&skbedit_ops);
}

/** @} */
