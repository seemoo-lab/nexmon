/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_CLS_POLICE_H_
#define NETLINK_CLS_POLICE_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char *	nl_police2str(int, char *, size_t);
extern int	nl_str2police(const char *);

#ifdef __cplusplus
}
#endif

#endif
