/*
 * netlink/route/class-modules.h       Class Module API
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_CLASS_MODULES_H_
#define NETLINK_CLASS_MODULES_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Class operations
 * @ingroup class_api
 */
struct rtnl_class_ops
{
	/**
	 * Kind/Name of class
	 */
	char co_kind[32];

	/**
	 * Dump callbacks
	 */
	void (*co_dump[NL_DUMP_MAX+1])(struct rtnl_class *,
				       struct nl_dump_params *);

	/**
	 * Must return the contents supposed to be in TCA_OPTIONS
	 */
	struct nl_msg *(*co_get_opts)(struct rtnl_class *);

	/**
	 * TCA_OPTIONS message parser
	 */
	int  (*co_msg_parser)(struct rtnl_class *);

	/**
	 * Called before a class object gets destroyed
	 */
	void (*co_free_data)(struct rtnl_class *);

	/**
	 * Called whenever a class object needs to be cloned
	 */
	int (*co_clone)(struct rtnl_class *, struct rtnl_class *);

	/**
	 * INTERNAL (Do not use)
	 */
	struct rtnl_class_ops *co_next;
};

extern int			rtnl_class_register(struct rtnl_class_ops *);
extern int			rtnl_class_unregister(struct rtnl_class_ops *);
extern struct rtnl_class_ops *	rtnl_class_lookup_ops(struct rtnl_class *);
extern struct rtnl_class_ops *	__rtnl_class_lookup_ops(const char *);

#ifdef __cplusplus
}
#endif

#endif
