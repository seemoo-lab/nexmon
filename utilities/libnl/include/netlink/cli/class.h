/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2010 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_CLI_CLASS_H_
#define __NETLINK_CLI_CLASS_H_

#include <netlink/route/class.h>
#include <netlink/cli/tc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct rtnl_class *nl_cli_class_alloc(void);
extern struct nl_cache *nl_cli_class_alloc_cache(struct nl_sock *, int);

#ifdef __cplusplus
}
#endif

#endif
