/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef __NETLINK_NL_AUX_ROUTE_NL_ROUTE_H__
#define __NETLINK_NL_AUX_ROUTE_NL_ROUTE_H__

#include "base/nl-base-utils.h"

#include <netlink/route/action.h>
#include <netlink/route/nexthop.h>

struct rtnl_link;
void rtnl_link_put(struct rtnl_link *);
#define _nl_auto_rtnl_link _nl_auto(_nl_auto_rtnl_link_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_link *, _nl_auto_rtnl_link_fcn,
			   rtnl_link_put);

struct rtnl_addr;
void rtnl_addr_put(struct rtnl_addr *);
#define _nl_auto_rtnl_addr _nl_auto(_nl_auto_rtnl_addr_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_addr *, _nl_auto_rtnl_addr_fcn,
			   rtnl_addr_put);

struct rtnl_route;
void rtnl_route_put(struct rtnl_route *);
#define _nl_auto_rtnl_route _nl_auto(_nl_auto_rtnl_route_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_route *, _nl_auto_rtnl_route_fcn,
			   rtnl_route_put);

struct rtnl_mdb;
void rtnl_mdb_put(struct rtnl_mdb *);
#define _nl_auto_rtnl_mdb _nl_auto(_nl_auto_rtnl_mdb_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_mdb *, _nl_auto_rtnl_mdb_fcn,
			   rtnl_mdb_put);

struct rtnl_nexthop;
void rtnl_route_nh_free(struct rtnl_nexthop *);
#define _nl_auto_rtnl_nexthop _nl_auto(_nl_auto_rtnl_nexthop_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_nexthop *, _nl_auto_rtnl_nexthop_fcn,
			   rtnl_route_nh_free);

struct rtnl_nh;
void rtnl_nh_put(struct rtnl_nh *);
#define _nl_auto_rtnl_nh _nl_auto(_nl_auto_rtnl_nh_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_nh *, _nl_auto_rtnl_nh_fcn, rtnl_nh_put);

#define _nl_auto_rtnl_nh_encap _nl_auto(_nl_auto_rtnl_nh_encap_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_nh_encap *, _nl_auto_rtnl_nh_encap_fcn,
			   rtnl_nh_encap_free);

struct rtnl_link_af_ops;
void rtnl_link_af_ops_put(struct rtnl_link_af_ops *);
#define _nl_auto_rtnl_link_af_ops _nl_auto(_nl_auto_rtnl_link_af_ops_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_link_af_ops *,
			   _nl_auto_rtnl_link_af_ops_fcn, rtnl_link_af_ops_put);

#define _nl_auto_rtnl_act _nl_auto(_nl_auto_rtnl_act_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_act *, _nl_auto_rtnl_act_fcn,
			   rtnl_act_put);

#define _nl_auto_rtnl_act_all _nl_auto(_nl_auto_rtnl_act_fcn_all)
_NL_AUTO_DEFINE_FCN_INDIRECT0(struct rtnl_act *, _nl_auto_rtnl_act_fcn_all,
			      rtnl_act_put_all);

struct rtnl_ematch_tree;
void rtnl_ematch_tree_free(struct rtnl_ematch_tree *);
#define _nl_auto_rtnl_ematch_tree _nl_auto(_nl_auto_rtnl_ematch_tree_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_ematch_tree *,
			   _nl_auto_rtnl_ematch_tree_fcn,
			   rtnl_ematch_tree_free);

struct rtnl_cls;
void rtnl_cls_put(struct rtnl_cls *);
#define _nl_auto_rtnl_cls _nl_auto(_nl_auto_rtnl_cls_fcn)
_NL_AUTO_DEFINE_FCN_TYPED0(struct rtnl_cls *, _nl_auto_rtnl_cls_fcn,
			   rtnl_cls_put);

/*****************************************************************************/

static inline int _rtnl_act_append_get(struct rtnl_act **head,
				       struct rtnl_act *new)
{
	int r;

	r = rtnl_act_append(head, new);
	if (r >= 0)
		rtnl_act_get(new);
	return r;
}

static inline int _rtnl_act_append_take(struct rtnl_act **head,
					struct rtnl_act *new)
{
	int r;

	r = rtnl_act_append(head, new);
	if (r < 0)
		rtnl_act_put(new);
	return r;
}

#endif /* __NETLINK_NL_AUX_ROUTE_NL_ROUTE_H__ */
