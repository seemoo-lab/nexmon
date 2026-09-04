/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008-2011 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NETLINK_CLI_QDISC_H_
#define __NETLINK_CLI_QDISC_H_

#include <netlink/route/qdisc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define nl_cli_qdisc_alloc_cache(sk) \
		nl_cli_alloc_cache((sk), "queueing disciplines", \
				   rtnl_qdisc_alloc_cache)

extern struct rtnl_qdisc *nl_cli_qdisc_alloc(void);

#ifdef __cplusplus
}
#endif

#endif
