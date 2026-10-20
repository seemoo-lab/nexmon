/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2011 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_DSMARK_H_
#define NETLINK_DSMARK_H_

#include <netlink/netlink.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int	rtnl_class_dsmark_set_bitmask(struct rtnl_class *, uint8_t);
extern int	rtnl_class_dsmark_get_bitmask(struct rtnl_class *);

extern int	rtnl_class_dsmark_set_value(struct rtnl_class *, uint8_t);
extern int	rtnl_class_dsmark_get_value(struct rtnl_class *);

extern int	rtnl_qdisc_dsmark_set_indices(struct rtnl_qdisc *, uint16_t);
extern int	rtnl_qdisc_dsmark_get_indices(struct rtnl_qdisc *);

extern int	rtnl_qdisc_dsmark_set_default_index(struct rtnl_qdisc *,
						    uint16_t);
extern int	rtnl_qdisc_dsmark_get_default_index(struct rtnl_qdisc *);

extern int	rtnl_qdisc_dsmark_set_set_tc_index(struct rtnl_qdisc *, int);
extern int	rtnl_qdisc_dsmark_get_set_tc_index(struct rtnl_qdisc *);

#ifdef __cplusplus
}
#endif

#endif
