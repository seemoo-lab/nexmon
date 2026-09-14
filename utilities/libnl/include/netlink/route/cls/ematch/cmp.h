/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008-2010 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_CLS_EMATCH_CMP_H_
#define NETLINK_CLS_EMATCH_CMP_H_

#include <netlink/netlink.h>
#include <netlink/route/cls/ematch.h>
#include <linux/tc_ematch/tc_em_cmp.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tcf_em_cmp;

extern void	rtnl_ematch_cmp_set(struct rtnl_ematch *,
				    struct tcf_em_cmp *);
extern struct tcf_em_cmp *
		rtnl_ematch_cmp_get(struct rtnl_ematch *);

#ifdef __cplusplus
}
#endif

#endif
