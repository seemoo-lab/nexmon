/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2017 Volodymyr Bendiuga <volodymyr.bendiuga@gmail.com>
 */

/**
 * @ingroup cls
 * @defgroup cls_mall Match-all Classifier
 *
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/route/classifier.h>
#include <netlink/route/cls/matchall.h>
#include <netlink/route/action.h>

#include "tc-api.h"
#include "nl-aux-route/nl-route.h"

struct rtnl_mall {
	uint32_t m_classid;
	uint32_t m_flags;
	struct rtnl_act *m_act;
	int m_mask;
};

#define MALL_ATTR_CLASSID 0x01
#define MALL_ATTR_FLAGS   0x02
#define MALL_ATTR_ACTION  0x03


static struct nla_policy mall_policy[TCA_MATCHALL_MAX + 1] = {
	[TCA_MATCHALL_CLASSID]	= { .type = NLA_U32 },
	[TCA_MATCHALL_FLAGS]	= { .type = NLA_U32 },
};

/**
 * @name Attribute Modifications
 * @{
 */

int rtnl_mall_set_classid(struct rtnl_cls *cls, uint32_t classid)
{
	struct rtnl_mall *mall;
	if (!(mall = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	mall->m_classid = classid;
	mall->m_mask |= MALL_ATTR_CLASSID;

	return 0;
}

int rtnl_mall_get_classid(struct rtnl_cls *cls, uint32_t *classid)
{
	struct rtnl_mall *mall;

	if (!(mall = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(mall->m_mask & MALL_ATTR_CLASSID))
		return -NLE_INVAL;

	*classid = mall->m_classid;
	return 0;
}

int rtnl_mall_set_flags(struct rtnl_cls *cls, uint32_t flags)
{
	struct rtnl_mall *mall;

	if (!(mall = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	mall->m_flags = flags;
	mall->m_mask |= MALL_ATTR_FLAGS;

	return 0;
}

int rtnl_mall_get_flags(struct rtnl_cls *cls, uint32_t *flags)
{
	struct rtnl_mall *mall;

	if (!(mall = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(mall->m_mask & MALL_ATTR_FLAGS))
		return -NLE_INVAL;

	*flags = mall->m_flags;
	return 0;
}

int rtnl_mall_append_action(struct rtnl_cls *cls, struct rtnl_act *act)
{
	struct rtnl_mall *mall;
	int err;

	if (!act)
		return 0;

	if (!(mall = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if ((err = _rtnl_act_append_get(&mall->m_act, act)) < 0)
		return err;

	mall->m_mask |= MALL_ATTR_ACTION;
	return 0;
}

struct rtnl_act *rtnl_mall_get_first_action(struct rtnl_cls *cls)
{
	struct rtnl_mall *mall;
	struct rtnl_act *act;

	if (!(mall = rtnl_tc_data(TC_CAST(cls))))
		return NULL;

	if (!(mall->m_mask & MALL_ATTR_ACTION))
		return NULL;

	act = mall->m_act;
	rtnl_act_get(act);

	return act;
}

int rtnl_mall_del_action(struct rtnl_cls *cls, struct rtnl_act *act)
{
	struct rtnl_mall *mall;
	int ret;

	if (!act)
		return 0;

	if (!(mall = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (!(mall->m_mask & MALL_ATTR_ACTION))
		return -NLE_INVAL;

	ret = rtnl_act_remove(&mall->m_act, act);
	if (ret < 0)
		return ret;

	rtnl_act_put(act);

	return 0;
}

/** @} */

static void mall_free_data(struct rtnl_tc *tc, void *data)
{
	struct rtnl_mall *mall = data;

	if (mall->m_act)
		rtnl_act_put_all(&mall->m_act);
}

static int mall_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct rtnl_mall *mall = data;
	struct nlattr *tb[TCA_MATCHALL_MAX + 1];
	int err;

	err = tca_parse(tb, TCA_MATCHALL_MAX, tc, mall_policy);
	if (err < 0)
		return err;

	if (tb[TCA_MATCHALL_CLASSID]) {
		mall->m_classid = nla_get_u32(tb[TCA_MATCHALL_CLASSID]);
		mall->m_mask |= MALL_ATTR_CLASSID;
	}

	if (tb[TCA_MATCHALL_FLAGS]) {
		mall->m_flags = nla_get_u32(tb[TCA_MATCHALL_FLAGS]);
		mall->m_mask |= MALL_ATTR_FLAGS;
	}

	if (tb[TCA_MATCHALL_ACT]) {
		mall->m_mask |= MALL_ATTR_ACTION;
		err = rtnl_act_parse(&mall->m_act, tb[TCA_MATCHALL_ACT]);
		if (err < 0)
			return err;
	}

	return 0;
}

static int mall_msg_fill(struct rtnl_tc *tc, void *data, struct nl_msg *msg)
{
	struct rtnl_mall *mall = data;

	if (!mall)
		return 0;

	if (mall->m_mask & MALL_ATTR_CLASSID)
		NLA_PUT_U32(msg, TCA_MATCHALL_CLASSID, mall->m_classid);

	if (mall->m_mask & MALL_ATTR_FLAGS)
		NLA_PUT_U32(msg, TCA_MATCHALL_FLAGS, mall->m_flags);

	if (mall->m_mask & MALL_ATTR_ACTION) {
		int err;

		err = rtnl_act_fill(msg, TCA_MATCHALL_ACT, mall->m_act);
		if (err < 0)
			return err;
	}

	return 0;

nla_put_failure:
	return -NLE_NOMEM;
}

static int mall_clone(void *_dst, void *_src)
{
	struct rtnl_mall *dst = _dst, *src = _src;
	struct rtnl_act *next, *new;
	int err;

	dst->m_act = NULL;

	if (src->m_act) {
		if (!(dst->m_act = rtnl_act_alloc()))
			return -NLE_NOMEM;

		/* action nl list next and prev pointers must be updated */
		nl_init_list_head(&dst->m_act->ce_list);

		memcpy(dst->m_act, src->m_act, sizeof(struct rtnl_act));
		next = rtnl_act_next(src->m_act);
		while (next) {
			new = (struct rtnl_act *) nl_object_clone((struct nl_object *) next);
			if (!new)
				return -NLE_NOMEM;

			err = _rtnl_act_append_take(&dst->m_act, new);
			if (err < 0)
				return err;

			next = rtnl_act_next(next);
		}
	}

	return 0;
}

static void mall_dump_line(struct rtnl_tc *tc, void *data,
			   struct nl_dump_params *p)
{
	struct rtnl_mall *mall = data;
	char buf[32];

	if (!mall)
		return;

	if (mall->m_mask & MALL_ATTR_CLASSID)
		nl_dump(p, " target %s",
			rtnl_tc_handle2str(mall->m_classid, buf, sizeof(buf)));
}

static void mall_dump_details(struct rtnl_tc *tc, void *data,
			      struct nl_dump_params *p)
{
	struct rtnl_mall *mall = data;

	if (!mall)
		return;

	nl_dump(p, "no details for match-all");
}

static struct rtnl_tc_ops mall_ops = {
	.to_kind		= "matchall",
	.to_type		= RTNL_TC_TYPE_CLS,
	.to_size		= sizeof(struct rtnl_mall),
	.to_msg_parser		= mall_msg_parser,
	.to_free_data		= mall_free_data,
	.to_clone		= mall_clone,
	.to_msg_fill		= mall_msg_fill,
	.to_dump = {
	    [NL_DUMP_LINE]	= mall_dump_line,
	    [NL_DUMP_DETAILS]	= mall_dump_details,
	},
};

static void _nl_init mall_init(void)
{
	rtnl_tc_register(&mall_ops);
}

static void _nl_exit mall_exit(void)
{
	rtnl_tc_unregister(&mall_ops);
}

/** @} */
