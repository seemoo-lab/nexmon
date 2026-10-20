/*
 * lib/route/cls/basic.c	Basic Classifier
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup cls
 * @defgroup basic Basic Classifier
 *
 * @par Introduction
 * The basic classifier is the simplest form of a classifier. It does
 * not have any special classification capabilities, instead it can be
 * used to classify exclusively based on extended matches or to
 * create a "catch-all" filter.
 *
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/classifier.h>
#include <netlink/route/classifier-modules.h>
#include <netlink/route/cls/basic.h>
#include <netlink/route/cls/ematch.h>

struct rtnl_basic
{
	uint32_t			b_classid;
	struct rtnl_ematch_tree *	b_ematch;
	int				b_mask;
};

/** @cond SKIP */
#define BASIC_ATTR_CLASSID	0x001
#define BASIC_ATTR_EMATCH	0x002
/** @endcond */

static struct nla_policy basic_policy[TCA_FW_MAX+1] = {
	[TCA_BASIC_CLASSID]	= { .type = NLA_U32 },
	[TCA_BASIC_EMATCHES]	= { .type = NLA_NESTED },
	[TCA_BASIC_ACT]		= { .type = NLA_NESTED },
	[TCA_BASIC_POLICE]	= { .type = NLA_NESTED },
};

static int basic_clone(struct rtnl_cls *_dst, struct rtnl_cls *_src)
{
	return -NLE_OPNOTSUPP;
}

static void basic_free_data(struct rtnl_cls *cls)
{
	struct rtnl_basic *basic = rtnl_cls_data(cls);

	rtnl_ematch_tree_free(basic->b_ematch);
}

static int basic_msg_parser(struct rtnl_cls *cls)
{
	struct nlattr *tb[TCA_BASIC_MAX + 1];
	struct rtnl_basic *basic = rtnl_cls_data(cls);
	int err;

	err = tca_parse(tb, TCA_BASIC_MAX, (struct rtnl_tca *) cls, basic_policy);
	if (err < 0)
		return err;

	if (tb[TCA_BASIC_CLASSID]) {
		basic->b_classid = nla_get_u32(tb[TCA_BASIC_CLASSID]);
		basic->b_mask |= BASIC_ATTR_CLASSID;
	}

	if (tb[TCA_BASIC_EMATCHES]) {
		if ((err = rtnl_ematch_parse(tb[TCA_BASIC_EMATCHES],
					     &basic->b_ematch)) < 0)
			return err;

		if (basic->b_ematch)
			basic->b_mask |= BASIC_ATTR_EMATCH;
	}

	if (tb[TCA_BASIC_ACT]) {
		/* XXX */
	}

	if (tb[TCA_BASIC_POLICE]) {
		/* XXX */
	}

	return 0;
}

static void basic_dump_line(struct rtnl_cls *cls, struct nl_dump_params *p)
{
	struct rtnl_basic *b = rtnl_cls_data(cls);
	char buf[32];

	if (b->b_mask & BASIC_ATTR_EMATCH)
		nl_dump(p, " ematch");
	else
		nl_dump(p, " match-all");

	if (b->b_mask & BASIC_ATTR_CLASSID)
		nl_dump(p, " classify-to %s",
			rtnl_tc_handle2str(b->b_classid, buf, sizeof(buf)));
}

static void basic_dump_details(struct rtnl_cls *cls, struct nl_dump_params *p)
{
	struct rtnl_basic *b = rtnl_cls_data(cls);

	if (b->b_mask & BASIC_ATTR_EMATCH) {
		nl_dump(p, "\n");
		nl_dump_line(p, "    ematch ");
		rtnl_ematch_tree_dump(b->b_ematch, p);
	} else
		nl_dump(p, "no options.\n");
}

static int basic_get_opts(struct rtnl_cls *cls, struct nl_msg *msg)
{
	struct rtnl_basic *b = rtnl_cls_data(cls);

	if (!(b->b_mask & BASIC_ATTR_CLASSID))
		return -NLE_MISSING_ATTR;

	NLA_PUT_U32(msg, TCA_BASIC_CLASSID, b->b_classid);

	return 0;

nla_put_failure:
	return -NLE_NOMEM;
}

/**
 * @name Attribute Modifications
 * @{
 */

int rtnl_basic_set_classid(struct rtnl_cls *cls, uint32_t classid)
{
	struct rtnl_basic *b = rtnl_cls_data(cls);

	b->b_classid = classid;
	b->b_mask |= BASIC_ATTR_CLASSID;

	return 0;
}

uint32_t rtnl_basic_get_classid(struct rtnl_cls *cls)
{
	struct rtnl_basic *b = rtnl_cls_data(cls);

	return b->b_classid;
}

int rtnl_basic_set_ematch(struct rtnl_cls *cls, struct rtnl_ematch_tree *tree)
{
	struct rtnl_basic *b = rtnl_cls_data(cls);

	if (b->b_ematch) {
		rtnl_ematch_tree_free(b->b_ematch);
		b->b_mask &= ~BASIC_ATTR_EMATCH;
	}

	b->b_ematch = tree;

	if (tree)
		b->b_mask |= BASIC_ATTR_EMATCH;

	return 0;
}

struct rtnl_ematch_tree *rtnl_basic_get_ematch(struct rtnl_cls *cls)
{
	struct rtnl_basic *b = rtnl_cls_data(cls);
	return b->b_ematch;
}

/** @} */

static struct rtnl_cls_ops basic_ops = {
	.co_kind		= "basic",
	.co_size		= sizeof(struct rtnl_basic),
	.co_msg_parser		= basic_msg_parser,
	.co_clone		= basic_clone,
	.co_free_data		= basic_free_data,
	.co_get_opts		= basic_get_opts,
	.co_dump = {
	    [NL_DUMP_LINE]	= basic_dump_line,
	    [NL_DUMP_DETAILS]	= basic_dump_details,
	},
};

static void __init basic_init(void)
{
	rtnl_cls_register(&basic_ops);
}

static void __exit basic_exit(void)
{
	rtnl_cls_unregister(&basic_ops);
}

/** @} */
