/*
 * netlink/route/link/info-api.h	Link Info API
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_LINK_INFO_API_H_
#define NETLINK_LINK_INFO_API_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ingroup link_info
 *
 * Link info operations
 */
struct rtnl_link_info_ops
{
	/** Name of operations, must match name on kernel side */
	char *		io_name;

	/** Reference count (internal, do not use) */
	int		io_refcnt;

	/** Called to assign an info type to a link.
	 * Has to allocate enough resources to hold attributes. Can
	 * use link->l_info to store a pointer. */
	int	      (*io_alloc)(struct rtnl_link *);

	/** Called to parse the link info attribute.
	 * Must parse the attribute and assign all values to the link.
	 */
	int	      (*io_parse)(struct rtnl_link *,
				  struct nlattr *,
				  struct nlattr *);

	/** Called when the link object is dumped.
	 * Must dump the info type specific attributes. */
	void	      (*io_dump[NL_DUMP_MAX+1])(struct rtnl_link *,
						struct nl_dump_params *);

	/** Called when a link object is cloned.
	 * Must clone all info type specific attributes. */
	int	      (*io_clone)(struct rtnl_link *, struct rtnl_link *);

	/** Called when construction a link netlink message.
	 * Must append all info type specific attributes to the message. */
	int	      (*io_put_attrs)(struct nl_msg *, struct rtnl_link *);

	/** Called to release all resources previously allocated
	 * in either io_alloc() or io_parse(). */
	void	      (*io_free)(struct rtnl_link *);

	struct rtnl_link_info_ops *	io_next;
};

extern struct rtnl_link_info_ops *rtnl_link_info_ops_lookup(const char *);

extern int			rtnl_link_register_info(struct rtnl_link_info_ops *);
extern int			rtnl_link_unregister_info(struct rtnl_link_info_ops *);

#endif
