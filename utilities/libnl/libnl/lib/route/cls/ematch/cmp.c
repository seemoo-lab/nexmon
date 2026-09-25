/*
 * lib/route/cls/ematch/cmp.c	Simple packet data comparison ematch
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup ematch
 * @defgroup em_cmp Simple packet data comparison
 *
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/cls/ematch.h>
#include <linux/tc_ematch/tc_em_cmp.h>

void rtnl_ematch_cmp_set(struct rtnl_ematch *ematch,
			 struct tcf_em_cmp *cfg)
{
	memcpy(rtnl_ematch_data(ematch), cfg, sizeof(*cfg));
}

struct tcf_em_cmp *rtnl_ematch_cmp_get(struct rtnl_ematch *ematch)
{
	return rtnl_ematch_data(ematch);
}

static const char *align_txt(struct tcf_em_cmp *cmp)
{
	switch (cmp->align) {
	case TCF_EM_ALIGN_U8:
		return "u8";
	case TCF_EM_ALIGN_U16:
		return (cmp->flags & TCF_EM_CMP_TRANS) ? "h16" : "u16";
	case TCF_EM_ALIGN_U32:
		return (cmp->flags & TCF_EM_CMP_TRANS) ? "h32" : "u32";
	default:
		return (cmp->flags & TCF_EM_CMP_TRANS) ? "h?" : "u?";
	}
}

static const char *layer_txt(struct tcf_em_cmp *cmp)
{
	switch (cmp->layer) {
	case TCF_LAYER_LINK:
		return "link";
	case TCF_LAYER_NETWORK:
		return "network";
	case TCF_LAYER_TRANSPORT:
		return "transport";
	default:
		return "?";
	}
}

static const char *relation_txt(struct tcf_em_cmp *cmp)
{
	switch (cmp->opnd) {
	case TCF_EM_OPND_EQ:
		return "eq";
	case TCF_EM_OPND_LT:
		return "lt";
	case TCF_EM_OPND_GT:
		return "gt";
	default:
		return "?";
	}
}

static int cmp_parse(struct rtnl_ematch *m, void *data, size_t len)
{
	memcpy(rtnl_ematch_data(m), data, len);

	return 0;
}

static void cmp_dump(struct rtnl_ematch *m, struct nl_dump_params *p)
{
	struct tcf_em_cmp *cmp = rtnl_ematch_data(m);

	nl_dump(p, "%s at %s+%u ",
		align_txt(cmp), layer_txt(cmp), cmp->off);

	if (cmp->mask)
		nl_dump(p, "& 0x%x ", cmp->mask);

	nl_dump(p, "%s %u", relation_txt(cmp), cmp->val);
}

static struct rtnl_ematch_ops cmp_ops = {
	.eo_kind	= TCF_EM_CMP,
	.eo_name	= "cmp",
	.eo_datalen	= sizeof(struct tcf_em_cmp),
	.eo_parse	= cmp_parse,
	.eo_dump	= cmp_dump,
};

static void __init cmp_init(void)
{
	rtnl_ematch_register(&cmp_ops);
}

static void __exit cmp_exit(void)
{
	rtnl_ematch_unregister(&cmp_ops);
}

/** @} */
