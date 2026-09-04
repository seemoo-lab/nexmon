/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2010 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_CLI_CLS_H_
#define __NETLINK_CLI_CLS_H_

#include <netlink/route/classifier.h>
#include <netlink/cli/tc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_cls *	nl_cli_cls_alloc(void);
extern struct nl_cache *	nl_cli_cls_alloc_cache(struct nl_sock *,
						       int, uint32_t);
extern void			nl_cli_cls_parse_proto(struct rtnl_cls *, char *);
extern struct rtnl_ematch_tree *nl_cli_cls_parse_ematch(struct rtnl_cls *, char *);

#ifdef __cplusplus
}
#endif

#endif
