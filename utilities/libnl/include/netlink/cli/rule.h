/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_CLI_RULE_H_
#define __NETLINK_CLI_RULE_H_

#include <netlink/route/rule.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_rule *nl_cli_rule_alloc(void);
extern struct nl_cache *nl_cli_rule_alloc_cache(struct nl_sock *);
extern void nl_cli_rule_parse_family(struct rtnl_rule *, char *);

#ifdef __cplusplus
}
#endif

#endif
