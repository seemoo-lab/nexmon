/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_FIB_LOOKUP_H_
#define NETLINK_FIB_LOOKUP_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>
#include <netlink/fib_lookup/request.h>

#ifdef __cplusplus
extern "C" {
#endif

struct flnl_result;

extern struct flnl_result *	flnl_result_alloc(void);
extern void			flnl_result_put(struct flnl_result *);

extern struct nl_cache *	flnl_result_alloc_cache(void);

extern int			flnl_lookup_build_request(struct flnl_request *,
							  int,
							  struct nl_msg **);
extern int			flnl_lookup(struct nl_sock *,
					    struct flnl_request *,
					    struct nl_cache *);

extern int flnl_result_get_table_id(struct flnl_result *res);
extern int flnl_result_get_prefixlen(struct flnl_result *res);
extern int flnl_result_get_nexthop_sel(struct flnl_result *res);
extern int flnl_result_get_type(struct flnl_result *res);
extern int flnl_result_get_scope(struct flnl_result *res);
extern int flnl_result_get_error(struct flnl_result *res);

#ifdef __cplusplus
}
#endif

#endif
