/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2022 Stanislav Zaikin <zstaseg@gmail.com>
 */

#ifndef __NETLINK_CLI_NH_H_
#define __NETLINK_CLI_NH_H_

#include <netlink/route/nh.h>
#include <netlink/cli/utils.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_nh *nl_cli_nh_alloc(void);
extern struct nl_cache *nl_cli_nh_alloc_cache_family(struct nl_sock *, int);
extern struct nl_cache *nl_cli_nh_alloc_cache_family_flags(struct nl_sock *,
							   int, unsigned int);
extern struct nl_cache *nl_cli_nh_alloc_cache(struct nl_sock *);
extern struct nl_cache *nl_cli_nh_alloc_cache_flags(struct nl_sock *,
						    unsigned int);

extern void nl_cli_nh_parse_family(struct rtnl_nh *, char *);

#ifdef __cplusplus
}
#endif

#endif
