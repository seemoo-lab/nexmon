/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2008 Patrick McHardy <kaber@trash.net>
 */

#ifndef NETLINK_NETFILTER_H_
#define NETLINK_NETFILTER_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char *			nfnl_verdict2str(unsigned int, char *, size_t);
extern unsigned int		nfnl_str2verdict(const char *);

extern char *			nfnl_inet_hook2str(unsigned int, char *, size_t);
extern unsigned int		nfnl_str2inet_hook(const char *);

#ifdef __cplusplus
}
#endif

#endif
