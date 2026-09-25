/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2011 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2005 Petr Gotthard <petr.gotthard@siemens.com>
 * Copyright (c) 2005 Siemens AG Oesterreich
 */

#ifndef NETLINK_HTB_H_
#define NETLINK_HTB_H_

#include <netlink/netlink.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t	rtnl_htb_get_rate2quantum(struct rtnl_qdisc *);
extern int	rtnl_htb_set_rate2quantum(struct rtnl_qdisc *, uint32_t);
extern uint32_t	rtnl_htb_get_defcls(struct rtnl_qdisc *);
extern int	rtnl_htb_set_defcls(struct rtnl_qdisc *, uint32_t);

extern uint32_t	rtnl_htb_get_prio(struct rtnl_class *);
extern int	rtnl_htb_set_prio(struct rtnl_class *, uint32_t);

extern uint32_t  rtnl_htb_get_rate(struct rtnl_class *);
extern int       rtnl_htb_set_rate(struct rtnl_class *, uint32_t);
extern uint32_t  rtnl_htb_get_ceil(struct rtnl_class *);
extern int       rtnl_htb_set_ceil(struct rtnl_class *, uint32_t);

extern int       rtnl_htb_get_rate64(struct rtnl_class *, uint64_t *);
extern int       rtnl_htb_set_rate64(struct rtnl_class *, uint64_t);
extern int       rtnl_htb_get_ceil64(struct rtnl_class *, uint64_t *);
extern int       rtnl_htb_set_ceil64(struct rtnl_class *, uint64_t);

extern uint32_t	rtnl_htb_get_rbuffer(struct rtnl_class *);
extern int	rtnl_htb_set_rbuffer(struct rtnl_class *, uint32_t);
extern uint32_t	rtnl_htb_get_cbuffer(struct rtnl_class *);
extern int	rtnl_htb_set_cbuffer(struct rtnl_class *, uint32_t);
extern uint32_t	rtnl_htb_get_quantum(struct rtnl_class *);
extern int	rtnl_htb_set_quantum(struct rtnl_class *, uint32_t);
extern int	rtnl_htb_set_level(struct rtnl_class *, int);
extern int	rtnl_htb_get_level(struct rtnl_class *);

#ifdef __cplusplus
}
#endif

#endif
