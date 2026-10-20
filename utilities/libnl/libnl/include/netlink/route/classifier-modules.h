/*
 * netlink/route/classifier-modules.h   Classifier Module API
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2009 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_CLASS_MODULES_H_
#define NETLINK_CLASS_MODULES_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Classifier operations
 * @ingroup cls_api
 */
struct rtnl_cls_ops
{
	/**
	 * Name of classifier module
	 */
	char co_kind[32];


	/**
	 * Size of private classifier data
	 */
	size_t co_size;

	/**
	 * Dump callbacks
	 */
	void (*co_dump[NL_DUMP_MAX+1])(struct rtnl_cls *,
				       struct nl_dump_params *);
	/**
	 * Must return the contents supposed to be in TCA_OPTIONS
	 */
	int (*co_get_opts)(struct rtnl_cls *, struct nl_msg *);

	/**
	 * TCA_OPTIONS message parser
	 */
	int (*co_msg_parser)(struct rtnl_cls *);

	/**
	 * Called before a class object gets destroyed
	 */
	void (*co_free_data)(struct rtnl_cls *);

	/**
	 * Called whenever a classifier object needs to be cloned
	 */
	int (*co_clone)(struct rtnl_cls *, struct rtnl_cls *);

	/**
	 * INTERNAL (Do not use)
	 */
	struct rtnl_cls_ops *co_next;
};

extern int 			rtnl_cls_register(struct rtnl_cls_ops *);
extern int 			rtnl_cls_unregister(struct rtnl_cls_ops *);
extern struct rtnl_cls_ops *	rtnl_cls_lookup_ops(struct rtnl_cls *);
extern struct rtnl_cls_ops *	__rtnl_cls_lookup_ops(const char *kind);

#ifdef __cplusplus
}
#endif

#endif
