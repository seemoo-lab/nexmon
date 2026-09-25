/*
 * lib/route/cls/cgroup.c	Control Groups Classifier
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2009 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup cls_api
 * @defgroup cgroup Control Groups Classifier
 *
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/route/classifier.h>
#include <netlink/route/classifier-modules.h>
#include <netlink/route/cls/cgroup.h>
#include <netlink/route/cls/ematch.h>

/** @cond SKIP */
#define CGROUP_ATTR_EMATCH      0x001
/** @endcond */

static struct nla_policy cgroup_policy[TCA_CGROUP_MAX+1] = {
	[TCA_CGROUP_EMATCHES]	= { .type = NLA_NESTED },
};

static void cgroup_free_data(struct rtnl_cls *cls)
{
	struct rtnl_cgroup *cg = rtnl_cls_data(cls);

	rtnl_ematch_tree_free(cg->cg_ematch);
}

static int cgroup_msg_parser(struct rtnl_cls *cls)
{
	struct rtnl_cgroup *cg = rtnl_cls_data(cls);
	struct nlattr *tb[TCA_CGROUP_MAX + 1];
	int err;

	err = tca_parse(tb, TCA_CGROUP_MAX, (struct rtnl_tca *) cls,
			cgroup_policy);
	if (err < 0)
		return err;

	if (tb[TCA_CGROUP_EMATCHES]) {
		if ((err = rtnl_ematch_parse(tb[TCA_CGROUP_EMATCHES],
					     &cg->cg_ematch)) < 0)
			return err;
		cg->cg_mask |= CGROUP_ATTR_EMATCH;
	}

#if 0
	TODO:
	TCA_CGROUP_ACT,
	TCA_CGROUP_POLICE,
#endif

	return 0;
}

static void cgroup_dump_line(struct rtnl_cls *cls, struct nl_dump_params *p)
{
	struct rtnl_cgroup *cg = rtnl_cls_data(cls);

	if (cg->cg_mask & CGROUP_ATTR_EMATCH)
		nl_dump(p, " ematch");
	else
		nl_dump(p, " match-all");
}

static void cgroup_dump_details(struct rtnl_cls *cls, struct nl_dump_params *p)
{
	struct rtnl_cgroup *cg = rtnl_cls_data(cls);

	if (cg->cg_mask & CGROUP_ATTR_EMATCH) {
		nl_dump(p, "\n");
		nl_dump_line(p, "    ematch ");
		rtnl_ematch_tree_dump(cg->cg_ematch, p);
	}
}

/**
 * @name Attribute Modifications
 * @{
 */

int rtnl_cgroup_set_ematch(struct rtnl_cls *cls, struct rtnl_ematch_tree *tree)
{
	struct rtnl_cgroup *cg = rtnl_cls_data(cls);

	if (cg->cg_ematch) {
		rtnl_ematch_tree_free(cg->cg_ematch);
		cg->cg_mask &= ~CGROUP_ATTR_EMATCH;
	}

	cg->cg_ematch = tree;

	if (tree)
		cg->cg_mask |= CGROUP_ATTR_EMATCH;

	return 0;
}

struct rtnl_ematch_tree *rtnl_cgroup_get_ematch(struct rtnl_cls *cls)
{
	struct rtnl_cgroup *cg = rtnl_cls_data(cls);
	return cg->cg_ematch;
}

static struct rtnl_cls_ops cgroup_ops = {
	.co_kind		= "cgroup",
	.co_size		= sizeof(struct rtnl_cgroup),
	.co_msg_parser		= cgroup_msg_parser,
	.co_free_data		= cgroup_free_data,
	.co_dump = {
	    [NL_DUMP_LINE]	= cgroup_dump_line,
	    [NL_DUMP_DETAILS]	= cgroup_dump_details,
	},
};

static void __init cgroup_init(void)
{
	rtnl_cls_register(&cgroup_ops);
}

static void __exit cgroup_exit(void)
{
	rtnl_cls_unregister(&cgroup_ops);
}

/** @} */
