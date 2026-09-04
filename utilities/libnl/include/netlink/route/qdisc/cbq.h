/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_CBQ_H_
#define NETLINK_CBQ_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/qdisc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char * nl_ovl_strategy2str(int, char *, size_t);
extern int    nl_str2ovl_strategy(const char *);

#ifdef __cplusplus
}
#endif

#endif
