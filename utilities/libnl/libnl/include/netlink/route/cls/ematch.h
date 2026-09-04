/*
 * netlink/route/cls/ematch.h		Extended Matches
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_CLS_EMATCH_H_
#define NETLINK_CLS_EMATCH_H_

#include <netlink/netlink.h>
#include <netlink/route/classifier.h>
#include <linux/pkt_cls.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rtnl_ematch;
struct rtnl_ematch_tree;

struct rtnl_ematch_ops
{
	int				eo_kind;
	const char *			eo_name;
	size_t				eo_datalen;

	int			      (*eo_parse)(struct rtnl_ematch *,
						  void *, size_t);
	void			      (*eo_dump)(struct rtnl_ematch *,
						 struct nl_dump_params *);
	struct nl_list_head		eo_list;
};

extern int	rtnl_ematch_register(struct rtnl_ematch_ops *);
extern int	rtnl_ematch_unregister(struct rtnl_ematch_ops *);

extern struct rtnl_ematch_ops *
		rtnl_ematch_lookup_ops(int);
extern struct rtnl_ematch_ops *
		rtnl_ematch_lookup_ops_name(const char *);

extern struct rtnl_ematch *
		rtnl_ematch_alloc(struct rtnl_ematch_ops *);
extern void	rtnl_ematch_add_child(struct rtnl_ematch *,
				      struct rtnl_ematch *);
extern void	rtnl_ematch_unlink(struct rtnl_ematch *);
extern void	rtnl_ematch_free(struct rtnl_ematch *);

extern void *	rtnl_ematch_data(struct rtnl_ematch *);
extern void	rtnl_ematch_set_flags(struct rtnl_ematch *, uint16_t);
extern void	rtnl_ematch_unset_flags(struct rtnl_ematch *, uint16_t);
extern uint16_t	rtnl_ematch_get_flags(struct rtnl_ematch *);

extern struct rtnl_ematch_tree *
		rtnl_ematch_tree_alloc(uint16_t);
extern void	rtnl_ematch_tree_free(struct rtnl_ematch_tree *);

extern int	rtnl_ematch_parse(struct nlattr *, struct rtnl_ematch_tree **);
extern void	rtnl_ematch_tree_add_tail(struct rtnl_ematch_tree *,
					  struct rtnl_ematch *);
extern void	rtnl_ematch_tree_dump(struct rtnl_ematch_tree *,
				      struct nl_dump_params *);

#ifdef __cplusplus
}
#endif

#endif
