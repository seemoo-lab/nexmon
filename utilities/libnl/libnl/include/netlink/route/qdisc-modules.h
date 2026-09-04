/*
 * netlink/route/qdisc-modules.h       Qdisc Module API
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_QDISC_MODULES_H_
#define NETLINK_QDISC_MODULES_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Qdisc Operations
 * @ingroup qdisc
 */
struct rtnl_qdisc_ops
{
	/**
	 * Kind/Name of Qdisc
	 */
	char qo_kind[32];

	/**
	 * Dump callbacks
	 */
	void  (*qo_dump[NL_DUMP_MAX+1])(struct rtnl_qdisc *,
					struct nl_dump_params *);

	/**
	 * Must return the contents supposed to be in TCA_OPTIONS
	 */
	struct nl_msg *(*qo_get_opts)(struct rtnl_qdisc *);

	int (*qo_build_msg)(struct rtnl_qdisc *, struct nl_msg *);

	/**
	 * TCA_OPTIONS message parser
	 */
	int  (*qo_msg_parser)(struct rtnl_qdisc *);

	/**
	 * Called before a Qdisc object gets destroyed
	 */
	void (*qo_free_data)(struct rtnl_qdisc *);

	/**
	 * Called whenever a qdisc object needs to be cloned
	 */
	int  (*qo_clone)(struct rtnl_qdisc *, struct rtnl_qdisc *);

	/**
	 * INTERNAL (Do not use)
	 */
	struct rtnl_qdisc_ops *qo_next;
};

extern int			rtnl_qdisc_register(struct rtnl_qdisc_ops *);
extern int			rtnl_qdisc_unregister(struct rtnl_qdisc_ops *);
extern struct rtnl_qdisc_ops *	rtnl_qdisc_lookup_ops(struct rtnl_qdisc *);
extern struct rtnl_qdisc_ops *	__rtnl_qdisc_lookup_ops(const char *);

#ifdef __cplusplus
}
#endif

#endif
