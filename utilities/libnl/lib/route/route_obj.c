/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup route
 * @defgroup route_obj Route Object
 *
 * @par Attributes
 * @code
 * Name                                           Default
 * -------------------------------------------------------------
 * routing table                                  RT_TABLE_MAIN
 * scope                                          RT_SCOPE_NOWHERE
 * tos                                            0
 * protocol                                       RTPROT_STATIC
 * prio                                           0
 * family                                         AF_UNSPEC
 * type                                           RTN_UNICAST
 * iif                                            NULL
 * @endcode
 *
 * @{
 */

#include "nl-default.h"

#include <linux/in_route.h>

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/data.h>
#include <netlink/hashtable.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/route.h>
#include <netlink/route/link.h>
#include <netlink/route/nexthop.h>

#include "nl-route.h"
#include "nl-aux-route/nl-route.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nexthop-encap.h"

/** @cond SKIP */
struct rtnl_route {
	NLHDR_COMMON

	uint8_t rt_family;
	uint8_t rt_tos;
	uint8_t rt_protocol;
	uint8_t rt_scope;
	uint8_t rt_type;
	uint8_t rt_nmetrics;
	uint8_t rt_ttl_propagate;
	uint32_t rt_flags;
	struct nl_addr *rt_dst;
	struct nl_addr *rt_src;
	uint32_t rt_table;
	uint32_t rt_iif;
	uint32_t rt_prio;
	uint32_t rt_metrics[RTAX_MAX];
	uint32_t rt_metrics_mask;
	uint32_t rt_nr_nh;
	uint32_t rt_nhid;
	struct nl_addr *rt_pref_src;
	struct nl_list_head rt_nexthops;
	struct rtnl_rtcacheinfo rt_cacheinfo;
	uint32_t rt_flag_mask;
};

#define ROUTE_ATTR_FAMILY    0x000001
#define ROUTE_ATTR_TOS       0x000002
#define ROUTE_ATTR_TABLE     0x000004
#define ROUTE_ATTR_PROTOCOL  0x000008
#define ROUTE_ATTR_SCOPE     0x000010
#define ROUTE_ATTR_TYPE      0x000020
#define ROUTE_ATTR_FLAGS     0x000040
#define ROUTE_ATTR_DST       0x000080
#define ROUTE_ATTR_SRC       0x000100
#define ROUTE_ATTR_IIF       0x000200
#define ROUTE_ATTR_OIF       0x000400
#define ROUTE_ATTR_GATEWAY   0x000800
#define ROUTE_ATTR_PRIO      0x001000
#define ROUTE_ATTR_PREF_SRC  0x002000
#define ROUTE_ATTR_METRICS   0x004000
#define ROUTE_ATTR_MULTIPATH 0x008000
#define ROUTE_ATTR_REALMS    0x010000
#define ROUTE_ATTR_CACHEINFO 0x020000
#define ROUTE_ATTR_TTL_PROPAGATE 0x040000
#define ROUTE_ATTR_NHID      0x080000
/** @endcond */

static void route_constructor(struct nl_object *c)
{
	struct rtnl_route *r = (struct rtnl_route *) c;

	r->rt_family = AF_UNSPEC;
	r->rt_scope = RT_SCOPE_NOWHERE;
	r->rt_table = RT_TABLE_MAIN;
	r->rt_protocol = RTPROT_STATIC;
	r->rt_type = RTN_UNICAST;
	r->rt_prio = 0;

	nl_init_list_head(&r->rt_nexthops);
}

static void route_free_data(struct nl_object *c)
{
	struct rtnl_route *r = (struct rtnl_route *) c;
	struct rtnl_nexthop *nh, *tmp;

	if (r == NULL)
		return;

	nl_addr_put(r->rt_dst);
	nl_addr_put(r->rt_src);
	nl_addr_put(r->rt_pref_src);

	nl_list_for_each_entry_safe(nh, tmp, &r->rt_nexthops, rtnh_list) {
		rtnl_route_remove_nexthop(r, nh);
		rtnl_route_nh_free(nh);
	}
}

static int route_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct rtnl_route *dst = (struct rtnl_route *) _dst;
	struct rtnl_route *src = (struct rtnl_route *) _src;
	struct rtnl_nexthop *nh, *new;

	dst->rt_dst = NULL;
	dst->rt_src = NULL;
	dst->rt_pref_src = NULL;
	nl_init_list_head(&dst->rt_nexthops);
	dst->rt_nr_nh = 0;

	if (src->rt_dst) {
		if (!(dst->rt_dst = nl_addr_clone(src->rt_dst)))
			return -NLE_NOMEM;
	}

	if (src->rt_src) {
		if (!(dst->rt_src = nl_addr_clone(src->rt_src)))
			return -NLE_NOMEM;
	}

	if (src->rt_pref_src) {
		if (!(dst->rt_pref_src = nl_addr_clone(src->rt_pref_src)))
			return -NLE_NOMEM;
	}

	nl_list_for_each_entry(nh, &src->rt_nexthops, rtnh_list) {
		new = rtnl_route_nh_clone(nh);
		if (!new)
			return -NLE_NOMEM;

		rtnl_route_add_nexthop(dst, new);
	}

	return 0;
}

static void route_dump_line(struct nl_object *a, struct nl_dump_params *p)
{
	struct rtnl_route *r = (struct rtnl_route *) a;
	int cache = 0, flags;
	char buf[64];

	if (r->rt_flags & RTM_F_CLONED)
		cache = 1;

	nl_dump_line(p, "%s ", nl_af2str(r->rt_family, buf, sizeof(buf)));

	if (cache)
		nl_dump(p, "cache ");

	if (!(r->ce_mask & ROUTE_ATTR_DST) ||
	    (nl_addr_get_prefixlen(r->rt_dst) == 0 &&
	     nl_addr_get_len(r->rt_dst) > 0 && nl_addr_iszero(r->rt_dst)))
		nl_dump(p, "default ");
	else
		nl_dump(p, "%s ", nl_addr2str(r->rt_dst, buf, sizeof(buf)));

	if (r->ce_mask & ROUTE_ATTR_TABLE && !cache)
		nl_dump(p, "table %s ",
			rtnl_route_table2str(r->rt_table, buf, sizeof(buf)));

	if (r->ce_mask & ROUTE_ATTR_TYPE)
		nl_dump(p, "type %s ",
			nl_rtntype2str(r->rt_type, buf, sizeof(buf)));

	if (r->ce_mask & ROUTE_ATTR_TOS && r->rt_tos != 0)
		nl_dump(p, "tos %#x ", r->rt_tos);

	if (r->ce_mask & ROUTE_ATTR_NHID)
		nl_dump(p, "nhid %u ", r->rt_nhid);

	if (r->ce_mask & ROUTE_ATTR_MULTIPATH) {
		struct rtnl_nexthop *nh;

		nl_list_for_each_entry(nh, &r->rt_nexthops, rtnh_list) {
			p->dp_ivar = NH_DUMP_FROM_ONELINE;
			rtnl_route_nh_dump(nh, p);
		}
	}

	flags = r->rt_flags & ~(RTM_F_CLONED);
	if (r->ce_mask & ROUTE_ATTR_FLAGS && flags) {

		nl_dump(p, "<");

#define PRINT_FLAG(f) if (flags & RTNH_F_##f) { \
		flags &= ~RTNH_F_##f; nl_dump(p, #f "%s", flags ? "," : ""); }
		PRINT_FLAG(DEAD);
		PRINT_FLAG(ONLINK);
		PRINT_FLAG(PERVASIVE);
#undef PRINT_FLAG

#define PRINT_FLAG(f) if (flags & RTM_F_##f) { \
		flags &= ~RTM_F_##f; nl_dump(p, #f "%s", flags ? "," : ""); }
		PRINT_FLAG(NOTIFY);
		PRINT_FLAG(EQUALIZE);
		PRINT_FLAG(PREFIX);
#undef PRINT_FLAG

#define PRINT_FLAG(f) if (flags & RTCF_##f) { \
		flags &= ~RTCF_##f; nl_dump(p, #f "%s", flags ? "," : ""); }
		PRINT_FLAG(NOTIFY);
		PRINT_FLAG(REDIRECTED);
		PRINT_FLAG(DOREDIRECT);
		PRINT_FLAG(DIRECTSRC);
		PRINT_FLAG(DNAT);
		PRINT_FLAG(BROADCAST);
		PRINT_FLAG(MULTICAST);
		PRINT_FLAG(LOCAL);
#undef PRINT_FLAG

		nl_dump(p, ">");
	}

	nl_dump(p, "\n");
}

static void route_dump_details(struct nl_object *a, struct nl_dump_params *p)
{
	_nl_auto_nl_cache struct nl_cache *link_cache = NULL;
	struct rtnl_route *r = (struct rtnl_route *) a;
	char buf[256];
	int i;

	link_cache = nl_cache_mngt_require_safe("route/link");

	route_dump_line(a, p);
	nl_dump_line(p, "    ");

	if (r->ce_mask & ROUTE_ATTR_PREF_SRC)
		nl_dump(p, "preferred-src %s ",
			nl_addr2str(r->rt_pref_src, buf, sizeof(buf)));

	if (r->ce_mask & ROUTE_ATTR_SCOPE && r->rt_scope != RT_SCOPE_NOWHERE)
		nl_dump(p, "scope %s ",
			rtnl_scope2str(r->rt_scope, buf, sizeof(buf)));

	if (r->ce_mask & ROUTE_ATTR_PRIO)
		nl_dump(p, "priority %#x ", r->rt_prio);

	if (r->ce_mask & ROUTE_ATTR_PROTOCOL)
		nl_dump(p, "protocol %s ",
			rtnl_route_proto2str(r->rt_protocol, buf, sizeof(buf)));

	if (r->ce_mask & ROUTE_ATTR_IIF) {
		if (link_cache) {
			nl_dump(p, "iif %s ",
				rtnl_link_i2name(link_cache, r->rt_iif,
						 buf, sizeof(buf)));
		} else
			nl_dump(p, "iif %d ", r->rt_iif);
	}

	if (r->ce_mask & ROUTE_ATTR_SRC)
		nl_dump(p, "src %s ", nl_addr2str(r->rt_src, buf, sizeof(buf)));

	if (r->ce_mask & ROUTE_ATTR_TTL_PROPAGATE) {
		nl_dump(p, " ttl-propagate %s",
			r->rt_ttl_propagate ? "enabled" : "disabled");
	}

	if (r->ce_mask & ROUTE_ATTR_NHID)
		nl_dump(p, "nhid %u ", r->rt_nhid);

	nl_dump(p, "\n");

	if (r->ce_mask & ROUTE_ATTR_MULTIPATH) {
		struct rtnl_nexthop *nh;

		nl_list_for_each_entry(nh, &r->rt_nexthops, rtnh_list) {
			nl_dump_line(p, "    ");
			p->dp_ivar = NH_DUMP_FROM_DETAILS;
			rtnl_route_nh_dump(nh, p);
			nl_dump(p, "\n");
		}
	}

	if ((r->ce_mask & ROUTE_ATTR_CACHEINFO) && r->rt_cacheinfo.rtci_error) {
		nl_dump_line(p, "    cacheinfo error %d (%s)\n",
			r->rt_cacheinfo.rtci_error,
			nl_strerror_l(-r->rt_cacheinfo.rtci_error));
	}

	if (r->ce_mask & ROUTE_ATTR_METRICS) {
		nl_dump_line(p, "    metrics [");
		for (i = 0; i < RTAX_MAX; i++)
			if (r->rt_metrics_mask & (1 << i))
				nl_dump(p, "%s %u ",
					rtnl_route_metric2str(i+1,
							      buf, sizeof(buf)),
					r->rt_metrics[i]);
		nl_dump(p, "]\n");
	}
}

static void route_dump_stats(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_route *route = (struct rtnl_route *) obj;

	route_dump_details(obj, p);

	if (route->ce_mask & ROUTE_ATTR_CACHEINFO) {
		struct rtnl_rtcacheinfo *ci = &route->rt_cacheinfo;

		nl_dump_line(p, "    used %u refcnt %u last-use %us "
				"expires %us\n",
			     ci->rtci_used, ci->rtci_clntref,
			     ci->rtci_last_use / nl_get_user_hz(),
			     ci->rtci_expires / nl_get_user_hz());
	}
}

static void route_keygen(struct nl_object *obj, uint32_t *hashkey,
			  uint32_t table_sz)
{
	struct rtnl_route *route = (struct rtnl_route *) obj;
	unsigned int rkey_sz;
	struct nl_addr *addr = NULL;
	_nl_auto_free struct route_hash_key {
		uint8_t		rt_family;
		uint8_t		rt_tos;
		uint32_t	rt_table;
		uint32_t	rt_prio;
		char 		rt_addr[0];
	} _nl_packed *rkey = NULL;
	char buf[INET6_ADDRSTRLEN+5];

	if (route->rt_dst)
		addr = route->rt_dst;

	rkey_sz = sizeof(*rkey);
	if (addr)
		rkey_sz += nl_addr_get_len(addr);
	rkey = calloc(1, rkey_sz);
	if (!rkey) {
		NL_DBG(2, "Warning: calloc failed for %d bytes...\n", rkey_sz);
		*hashkey = 0;
		return;
	}
	rkey->rt_family = route->rt_family;
	rkey->rt_tos = route->rt_tos;
	rkey->rt_table = route->rt_table;
	rkey->rt_prio = route->rt_prio;
	if (addr)
		memcpy(rkey->rt_addr, nl_addr_get_binary_addr(addr),
			nl_addr_get_len(addr));

	*hashkey = nl_hash(rkey, rkey_sz, 0) % table_sz;

	NL_DBG(5,
	       "route %p key (fam %d tos %d table %d prio %d addr %s) keysz %d hash 0x%x\n",
	       route, rkey->rt_family, rkey->rt_tos, rkey->rt_table,
	       rkey->rt_prio, nl_addr2str(addr, buf, sizeof(buf)), rkey_sz,
	       *hashkey);

	return;
}

static uint32_t route_id_attrs_get(struct nl_object *obj)
{
	struct rtnl_route *route = (struct rtnl_route *)obj;
	struct nl_object_ops *ops = obj->ce_ops;
	uint32_t rv = ops->oo_id_attrs;

	/* MPLS address family does not allow RTA_PRIORITY to be set */
	if (route->rt_family == AF_MPLS)
		rv &= ~ROUTE_ATTR_PRIO;

	return rv;
}

static uint64_t route_compare(struct nl_object *_a, struct nl_object *_b,
			      uint64_t attrs, int flags)
{
	struct rtnl_route *a = (struct rtnl_route *) _a;
	struct rtnl_route *b = (struct rtnl_route *) _b;
	struct rtnl_nexthop *nh_a, *nh_b;
	int i, found;
	uint64_t diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(ROUTE_ATTR_FAMILY, a->rt_family != b->rt_family);
	diff |= _DIFF(ROUTE_ATTR_TOS, a->rt_tos != b->rt_tos);
	diff |= _DIFF(ROUTE_ATTR_TABLE, a->rt_table != b->rt_table);
	diff |= _DIFF(ROUTE_ATTR_PROTOCOL, a->rt_protocol != b->rt_protocol);
	diff |= _DIFF(ROUTE_ATTR_SCOPE, a->rt_scope != b->rt_scope);
	diff |= _DIFF(ROUTE_ATTR_TYPE, a->rt_type != b->rt_type);
	diff |= _DIFF(ROUTE_ATTR_PRIO, a->rt_prio != b->rt_prio);
	diff |= _DIFF(ROUTE_ATTR_DST, nl_addr_cmp(a->rt_dst, b->rt_dst));
	diff |= _DIFF(ROUTE_ATTR_SRC, nl_addr_cmp(a->rt_src, b->rt_src));
	diff |= _DIFF(ROUTE_ATTR_IIF, a->rt_iif != b->rt_iif);
	diff |= _DIFF(ROUTE_ATTR_PREF_SRC,
		      nl_addr_cmp(a->rt_pref_src, b->rt_pref_src));
	diff |= _DIFF(ROUTE_ATTR_TTL_PROPAGATE,
		      a->rt_ttl_propagate != b->rt_ttl_propagate);
	diff |= _DIFF(ROUTE_ATTR_NHID, a->rt_nhid != b->rt_nhid);

	if (flags & LOOSE_COMPARISON) {
		nl_list_for_each_entry(nh_b, &b->rt_nexthops, rtnh_list) {
			found = 0;
			nl_list_for_each_entry(nh_a, &a->rt_nexthops,
					       rtnh_list) {
				if (!rtnl_route_nh_compare(nh_a, nh_b,
							nh_b->ce_mask, 1)) {
					found = 1;
					break;
				}
			}

			if (!found)
				goto nh_mismatch;
		}

		for (i = 0; i < RTAX_MAX - 1; i++) {
			if (a->rt_metrics_mask & (1 << i) &&
			    (!(b->rt_metrics_mask & (1 << i)) ||
			     a->rt_metrics[i] != b->rt_metrics[i]))
				diff |= _DIFF(ROUTE_ATTR_METRICS, 1);
		}

		diff |= _DIFF(ROUTE_ATTR_FLAGS,
			  (a->rt_flags ^ b->rt_flags) & b->rt_flag_mask);
	} else {
		if (a->rt_nr_nh != b->rt_nr_nh)
			goto nh_mismatch;

		/* search for a dup in each nh of a */
		nl_list_for_each_entry(nh_a, &a->rt_nexthops, rtnh_list) {
			found = 0;
			nl_list_for_each_entry(nh_b, &b->rt_nexthops,
					       rtnh_list) {
				if (rtnl_route_nh_identical(nh_a, nh_b)) {
					found = 1;
					break;
				}
			}
			if (!found)
				goto nh_mismatch;
		}

		/* search for a dup in each nh of b, covers case where a has
		 * dupes itself */
		nl_list_for_each_entry(nh_b, &b->rt_nexthops, rtnh_list) {
			found = 0;
			nl_list_for_each_entry(nh_a, &a->rt_nexthops,
					       rtnh_list) {
				if (rtnl_route_nh_identical(nh_a, nh_b)) {
					found = 1;
					break;
				}
			}
			if (!found)
				goto nh_mismatch;
		}

		for (i = 0; i < RTAX_MAX - 1; i++) {
			if ((a->rt_metrics_mask & (1 << i)) ^
			    (b->rt_metrics_mask & (1 << i)))
				diff |= _DIFF(ROUTE_ATTR_METRICS, 1);
			else
				diff |= _DIFF(ROUTE_ATTR_METRICS,
					a->rt_metrics[i] != b->rt_metrics[i]);
		}

		diff |= _DIFF(ROUTE_ATTR_FLAGS, a->rt_flags != b->rt_flags);
	}

out:
	return diff;

nh_mismatch:
	diff |= _DIFF(ROUTE_ATTR_MULTIPATH, 1);
	goto out;
#undef _DIFF
}

static int route_update(struct nl_object *old_obj, struct nl_object *new_obj)
{
	struct rtnl_route *new_route = (struct rtnl_route *) new_obj;
	struct rtnl_route *old_route = (struct rtnl_route *) old_obj;
	struct rtnl_nexthop *new_nh;
	int action = new_obj->ce_msgtype;
	char buf[INET6_ADDRSTRLEN+5];

	/*
	 * ipv6 ECMP route notifications from the kernel come as
	 * separate notifications, one for every nexthop. This update
	 * function collapses such route msgs into a single
	 * route with multiple nexthops. The resulting object looks
	 * similar to a ipv4 ECMP route
	 */
	if (new_route->rt_family != AF_INET6 ||
	    new_route->rt_table == RT_TABLE_LOCAL)
		return -NLE_OPNOTSUPP;

	/*
	 * For routes that are already multipath,
	 * or dont have a nexthop dont do anything
	 */
	if (rtnl_route_get_nnexthops(new_route) != 1)
		return -NLE_OPNOTSUPP;

	/*
	 * Get the only nexthop entry from the new route. For
	 * IPv6 we always get a route with a 0th NH
	 * filled or nothing at all
	 */
	new_nh = rtnl_route_nexthop_n(new_route, 0);
	if (!new_nh || !rtnl_route_nh_get_gateway(new_nh))
		return -NLE_OPNOTSUPP;

	switch(action) {
	case RTM_NEWROUTE : {
		struct rtnl_nexthop *cloned_nh;
		struct rtnl_nexthop *old_nh;

		/*
		 * Do not add the nexthop to old route if it was already added before
		 */
		nl_list_for_each_entry(old_nh, &old_route->rt_nexthops, rtnh_list) {
			if (rtnl_route_nh_identical(old_nh, new_nh)) {
				return 0;
			}
		}

		/*
		 * Add the nexthop to old route
		 */
		cloned_nh = rtnl_route_nh_clone(new_nh);
		if (!cloned_nh)
			return -NLE_NOMEM;
		rtnl_route_add_nexthop(old_route, cloned_nh);

		NL_DBG(2, "Route obj %p updated. Added "
			"nexthop %p via %s\n", old_route, cloned_nh,
			nl_addr2str(cloned_nh->rtnh_gateway, buf,
					sizeof(buf)));
	}
		break;
	case RTM_DELROUTE : {
		struct rtnl_nexthop *old_nh;

		/*
		 * Only take care of nexthop deletes and not
		 * route deletes. So, if there is only one nexthop
		 * quite likely we did not update it. So dont do
		 * anything and return
		 */
		if (rtnl_route_get_nnexthops(old_route) <= 1)
			return -NLE_OPNOTSUPP;

		/*
		 * Find the next hop in old route and delete it
		 */
		nl_list_for_each_entry(old_nh, &old_route->rt_nexthops,
			rtnh_list) {
			if (rtnl_route_nh_identical(old_nh, new_nh)) {

				rtnl_route_remove_nexthop(old_route, old_nh);

				NL_DBG(2, "Route obj %p updated. Removed "
					"nexthop %p via %s\n", old_route,
					old_nh,
					nl_addr2str(old_nh->rtnh_gateway, buf,
					sizeof(buf)));

				rtnl_route_nh_free(old_nh);
				break;
			}
		}
	}
		break;
	default:
		NL_DBG(2, "Unknown action associated "
			"to object %p during route update\n", new_obj);
		return -NLE_OPNOTSUPP;
	}

	return NLE_SUCCESS;
}

static const struct trans_tbl route_attrs[] = {
	__ADD(ROUTE_ATTR_FAMILY, family),
	__ADD(ROUTE_ATTR_TOS, tos),
	__ADD(ROUTE_ATTR_TABLE, table),
	__ADD(ROUTE_ATTR_PROTOCOL, protocol),
	__ADD(ROUTE_ATTR_SCOPE, scope),
	__ADD(ROUTE_ATTR_TYPE, type),
	__ADD(ROUTE_ATTR_FLAGS, flags),
	__ADD(ROUTE_ATTR_DST, dst),
	__ADD(ROUTE_ATTR_SRC, src),
	__ADD(ROUTE_ATTR_IIF, iif),
	__ADD(ROUTE_ATTR_OIF, oif),
	__ADD(ROUTE_ATTR_GATEWAY, gateway),
	__ADD(ROUTE_ATTR_PRIO, prio),
	__ADD(ROUTE_ATTR_PREF_SRC, pref_src),
	__ADD(ROUTE_ATTR_METRICS, metrics),
	__ADD(ROUTE_ATTR_MULTIPATH, multipath),
	__ADD(ROUTE_ATTR_REALMS, realms),
	__ADD(ROUTE_ATTR_CACHEINFO, cacheinfo),
	__ADD(ROUTE_ATTR_TTL_PROPAGATE, ttl_propagate),
	__ADD(ROUTE_ATTR_NHID, nhid),
};

static char *route_attrs2str(int attrs, char *buf, size_t len)
{
	return __flags2str(attrs, buf, len, route_attrs,
			   ARRAY_SIZE(route_attrs));
}

/**
 * @name Allocation/Freeing
 * @{
 */

struct rtnl_route *rtnl_route_alloc(void)
{
	return (struct rtnl_route *) nl_object_alloc(&route_obj_ops);
}

void rtnl_route_get(struct rtnl_route *route)
{
	nl_object_get((struct nl_object *) route);
}

void rtnl_route_put(struct rtnl_route *route)
{
	nl_object_put((struct nl_object *) route);
}

/** @} */

/**
 * @name Attributes
 * @{
 */

void rtnl_route_set_table(struct rtnl_route *route, uint32_t table)
{
	route->rt_table = table;
	route->ce_mask |= ROUTE_ATTR_TABLE;
}

uint32_t rtnl_route_get_table(struct rtnl_route *route)
{
	return route->rt_table;
}

void rtnl_route_set_scope(struct rtnl_route *route, uint8_t scope)
{
	route->rt_scope = scope;
	route->ce_mask |= ROUTE_ATTR_SCOPE;
}

uint8_t rtnl_route_get_scope(struct rtnl_route *route)
{
	return route->rt_scope;
}

void rtnl_route_set_tos(struct rtnl_route *route, uint8_t tos)
{
	route->rt_tos = tos;
	route->ce_mask |= ROUTE_ATTR_TOS;
}

uint8_t rtnl_route_get_tos(struct rtnl_route *route)
{
	return route->rt_tos;
}

void rtnl_route_set_protocol(struct rtnl_route *route, uint8_t protocol)
{
	route->rt_protocol = protocol;
	route->ce_mask |= ROUTE_ATTR_PROTOCOL;
}

uint8_t rtnl_route_get_protocol(struct rtnl_route *route)
{
	return route->rt_protocol;
}

void rtnl_route_set_priority(struct rtnl_route *route, uint32_t prio)
{
	route->rt_prio = prio;
	route->ce_mask |= ROUTE_ATTR_PRIO;
}

uint32_t rtnl_route_get_priority(struct rtnl_route *route)
{
	return route->rt_prio;
}

int rtnl_route_set_family(struct rtnl_route *route, uint8_t family)
{
	switch(family) {
	case AF_INET:
	case AF_INET6:
	case AF_DECnet:
	case AF_MPLS:
		route->rt_family = family;
		route->ce_mask |= ROUTE_ATTR_FAMILY;
		return 0;
	}

	return -NLE_AF_NOSUPPORT;
}

uint8_t rtnl_route_get_family(struct rtnl_route *route)
{
	return route->rt_family;
}

int rtnl_route_set_dst(struct rtnl_route *route, struct nl_addr *addr)
{
	if (route->ce_mask & ROUTE_ATTR_FAMILY) {
		if (addr->a_family != route->rt_family)
			return -NLE_AF_MISMATCH;
	} else
		route->rt_family = addr->a_family;

	if (route->rt_dst)
		nl_addr_put(route->rt_dst);

	nl_addr_get(addr);
	route->rt_dst = addr;

	route->ce_mask |= (ROUTE_ATTR_DST | ROUTE_ATTR_FAMILY);

	return 0;
}

struct nl_addr *rtnl_route_get_dst(struct rtnl_route *route)
{
	return route->rt_dst;
}

int rtnl_route_set_src(struct rtnl_route *route, struct nl_addr *addr)
{
	if (addr->a_family == AF_INET)
		return -NLE_SRCRT_NOSUPPORT;

	if (route->ce_mask & ROUTE_ATTR_FAMILY) {
		if (addr->a_family != route->rt_family)
			return -NLE_AF_MISMATCH;
	} else
		route->rt_family = addr->a_family;

	if (route->rt_src)
		nl_addr_put(route->rt_src);

	nl_addr_get(addr);
	route->rt_src = addr;
	route->ce_mask |= (ROUTE_ATTR_SRC | ROUTE_ATTR_FAMILY);

	return 0;
}

struct nl_addr *rtnl_route_get_src(struct rtnl_route *route)
{
	return route->rt_src;
}

int rtnl_route_set_type(struct rtnl_route *route, uint8_t type)
{
	if (type > RTN_MAX)
		return -NLE_RANGE;

	route->rt_type = type;
	route->ce_mask |= ROUTE_ATTR_TYPE;

	return 0;
}

uint8_t rtnl_route_get_type(struct rtnl_route *route)
{
	return route->rt_type;
}

void rtnl_route_set_flags(struct rtnl_route *route, uint32_t flags)
{
	route->rt_flag_mask |= flags;
	route->rt_flags |= flags;
	route->ce_mask |= ROUTE_ATTR_FLAGS;
}

void rtnl_route_unset_flags(struct rtnl_route *route, uint32_t flags)
{
	route->rt_flag_mask |= flags;
	route->rt_flags &= ~flags;
	route->ce_mask |= ROUTE_ATTR_FLAGS;
}

uint32_t rtnl_route_get_flags(struct rtnl_route *route)
{
	return route->rt_flags;
}

int rtnl_route_set_metric(struct rtnl_route *route, int metric, uint32_t value)
{
	if (metric > RTAX_MAX || metric < 1)
		return -NLE_RANGE;

	route->rt_metrics[metric - 1] = value;

	if (!(route->rt_metrics_mask & (1 << (metric - 1)))) {
		route->rt_nmetrics++;
		route->rt_metrics_mask |= (1 << (metric - 1));
	}

	route->ce_mask |= ROUTE_ATTR_METRICS;

	return 0;
}

int rtnl_route_unset_metric(struct rtnl_route *route, int metric)
{
	if (metric > RTAX_MAX || metric < 1)
		return -NLE_RANGE;

	if (route->rt_metrics_mask & (1 << (metric - 1))) {
		route->rt_nmetrics--;
		route->rt_metrics_mask &= ~(1 << (metric - 1));
	}

	return 0;
}

int rtnl_route_get_metric(struct rtnl_route *route, int metric, uint32_t *value)
{
	if (metric > RTAX_MAX || metric < 1)
		return -NLE_RANGE;

	if (!(route->rt_metrics_mask & (1 << (metric - 1))))
		return -NLE_OBJ_NOTFOUND;

	if (value)
		*value = route->rt_metrics[metric - 1];

	return 0;
}

int rtnl_route_set_pref_src(struct rtnl_route *route, struct nl_addr *addr)
{
	if (route->ce_mask & ROUTE_ATTR_FAMILY) {
		if (addr->a_family != route->rt_family)
			return -NLE_AF_MISMATCH;
	} else
		route->rt_family = addr->a_family;

	if (route->rt_pref_src)
		nl_addr_put(route->rt_pref_src);

	nl_addr_get(addr);
	route->rt_pref_src = addr;
	route->ce_mask |= (ROUTE_ATTR_PREF_SRC | ROUTE_ATTR_FAMILY);

	return 0;
}

struct nl_addr *rtnl_route_get_pref_src(struct rtnl_route *route)
{
	return route->rt_pref_src;
}

void rtnl_route_set_iif(struct rtnl_route *route, int ifindex)
{
	route->rt_iif = ifindex;
	route->ce_mask |= ROUTE_ATTR_IIF;
}

int rtnl_route_get_iif(struct rtnl_route *route)
{
	return route->rt_iif;
}

void rtnl_route_add_nexthop(struct rtnl_route *route, struct rtnl_nexthop *nh)
{
	nl_list_add_tail(&nh->rtnh_list, &route->rt_nexthops);
	route->rt_nr_nh++;
	route->ce_mask |= ROUTE_ATTR_MULTIPATH;
}

void rtnl_route_remove_nexthop(struct rtnl_route *route, struct rtnl_nexthop *nh)
{
	if (route->ce_mask & ROUTE_ATTR_MULTIPATH) {
		route->rt_nr_nh--;
		nl_list_del(&nh->rtnh_list);
	}
}

struct nl_list_head *rtnl_route_get_nexthops(struct rtnl_route *route)
{
	if (route->ce_mask & ROUTE_ATTR_MULTIPATH)
		return &route->rt_nexthops;

	return NULL;
}

int rtnl_route_get_nnexthops(struct rtnl_route *route)
{
	if (route->ce_mask & ROUTE_ATTR_MULTIPATH)
		return route->rt_nr_nh;

	return 0;
}

void rtnl_route_foreach_nexthop(struct rtnl_route *r,
                                void (*cb)(struct rtnl_nexthop *, void *),
                                void *arg)
{
	struct rtnl_nexthop *nh;

	if (r->ce_mask & ROUTE_ATTR_MULTIPATH) {
		nl_list_for_each_entry(nh, &r->rt_nexthops, rtnh_list) {
			cb(nh, arg);
		}
	}
}

struct rtnl_nexthop *rtnl_route_nexthop_n(struct rtnl_route *r, int n)
{
	struct rtnl_nexthop *nh;

	if (r->ce_mask & ROUTE_ATTR_MULTIPATH && n >= 0 &&
	    ((unsigned)n) < r->rt_nr_nh) {
		int i;

		i = 0;
		nl_list_for_each_entry(nh, &r->rt_nexthops, rtnh_list) {
			if (i == n)
				return nh;
			i++;
		}
	}
	return NULL;
}

void rtnl_route_set_ttl_propagate(struct rtnl_route *route, uint8_t ttl_prop)
{
	route->rt_ttl_propagate = ttl_prop;
	route->ce_mask |= ROUTE_ATTR_TTL_PROPAGATE;
}

int rtnl_route_get_ttl_propagate(struct rtnl_route *route)
{
	if (!route)
		return -NLE_INVAL;
	if (!(route->ce_mask & ROUTE_ATTR_TTL_PROPAGATE))
		return -NLE_MISSING_ATTR;
	return route->rt_ttl_propagate;
}

void rtnl_route_set_nhid(struct rtnl_route *route, uint32_t nhid)
{
	route->rt_nhid = nhid;

	if (nhid > 0)
		route->ce_mask |= ROUTE_ATTR_NHID;
	else
		route->ce_mask &= ~ROUTE_ATTR_NHID;
}

uint32_t rtnl_route_get_nhid(struct rtnl_route *route)
{
	return route->rt_nhid;
}

/** @} */

/**
 * @name Utilities
 * @{
 */

/**
 * Guess scope of a route object.
 * @arg route		Route object.
 *
 * Guesses the scope of a route object, based on the following rules:
 * @code
 *   1) Local route -> local scope
 *   2) At least one nexthop not directly connected -> universe scope
 *   3) All others -> link scope
 * @endcode
 *
 * @return Scope value.
 */
int rtnl_route_guess_scope(struct rtnl_route *route)
{
	if (route->rt_type == RTN_LOCAL)
		return RT_SCOPE_HOST;

	if (route->rt_family == AF_MPLS)
		return RT_SCOPE_UNIVERSE;

	if (!nl_list_empty(&route->rt_nexthops)) {
		struct rtnl_nexthop *nh;

		/*
		 * Use scope uiniverse if there is at least one nexthop which
		 * is not directly connected
		 */
		nl_list_for_each_entry(nh, &route->rt_nexthops, rtnh_list) {
			if (nh->rtnh_gateway || nh->rtnh_via)
				return RT_SCOPE_UNIVERSE;
		}
	}

	return RT_SCOPE_LINK;
}

/** @} */

static struct nl_addr *rtnl_route_parse_via(struct nlattr *nla)
{
	int alen = nla_len(nla) - offsetof(struct rtvia, rtvia_addr);
	struct rtvia *via = nla_data(nla);

	return nl_addr_build(via->rtvia_family, via->rtvia_addr, alen);
}

static int rtnl_route_put_via(struct nl_msg *msg, struct nl_addr *addr)
{
	unsigned int alen = nl_addr_get_len(addr);
	struct nlattr *nla;
	struct rtvia *via;

	nla = nla_reserve(msg, RTA_VIA, alen + sizeof(*via));
	if (!nla)
		return -EMSGSIZE;

	via = nla_data(nla);
	via->rtvia_family = nl_addr_get_family(addr);
	memcpy(via->rtvia_addr, nl_addr_get_binary_addr(addr), alen);

	return 0;
}

static struct nla_policy route_policy[RTA_MAX+1] = {
	[RTA_IIF]	= { .type = NLA_U32 },
	[RTA_OIF]	= { .type = NLA_U32 },
	[RTA_PRIORITY]	= { .type = NLA_U32 },
	[RTA_FLOW]	= { .type = NLA_U32 },
	[RTA_CACHEINFO]	= { .minlen = sizeof(struct rta_cacheinfo) },
	[RTA_METRICS]	= { .type = NLA_NESTED },
	[RTA_MULTIPATH]	= { .type = NLA_NESTED },
	[RTA_TTL_PROPAGATE] = { .type = NLA_U8 },
	[RTA_ENCAP]	= { .type = NLA_NESTED },
	[RTA_ENCAP_TYPE] = { .type = NLA_U16 },
	[RTA_NH_ID]	= { .type = NLA_U32 },
};

static int parse_multipath(struct rtnl_route *route, struct nlattr *attr)
{
	struct rtnexthop *rtnh = nla_data(attr);
	size_t tlen = nla_len(attr);
	int err;

	while (tlen >= sizeof(*rtnh) && tlen >= rtnh->rtnh_len) {
		_nl_auto_rtnl_nexthop struct rtnl_nexthop *nh = NULL;

		nh = rtnl_route_nh_alloc();
		if (!nh)
			return -NLE_NOMEM;

		rtnl_route_nh_set_weight(nh, rtnh->rtnh_hops);
		rtnl_route_nh_set_ifindex(nh, rtnh->rtnh_ifindex);
		rtnl_route_nh_set_flags(nh, rtnh->rtnh_flags);

		if (rtnh->rtnh_len > sizeof(*rtnh)) {
			struct nlattr *ntb[RTA_MAX + 1];

			err = nla_parse(ntb, RTA_MAX, (struct nlattr *)
					RTNH_DATA(rtnh),
					rtnh->rtnh_len - sizeof(*rtnh),
					route_policy);
			if (err < 0)
				return err;

			if (ntb[RTA_GATEWAY]) {
				_nl_auto_nl_addr struct nl_addr *addr = NULL;

				addr = nl_addr_alloc_attr(ntb[RTA_GATEWAY],
							  route->rt_family);
				if (!addr)
					return -NLE_NOMEM;

				rtnl_route_nh_set_gateway(nh, addr);
			}

			if (ntb[RTA_FLOW]) {
				uint32_t realms;

				realms = nla_get_u32(ntb[RTA_FLOW]);
				rtnl_route_nh_set_realms(nh, realms);
			}

			if (ntb[RTA_NEWDST]) {
				_nl_auto_nl_addr struct nl_addr *addr = NULL;

				addr = nl_addr_alloc_attr(ntb[RTA_NEWDST],
							  route->rt_family);
				if (!addr)
					return -NLE_NOMEM;

				err = rtnl_route_nh_set_newdst(nh, addr);
				if (err < 0)
					return err;
			}

			if (ntb[RTA_VIA]) {
				_nl_auto_nl_addr struct nl_addr *addr = NULL;

				addr = rtnl_route_parse_via(ntb[RTA_VIA]);
				if (!addr)
					return -NLE_NOMEM;

				err = rtnl_route_nh_set_via(nh, addr);
				if (err < 0)
					return err;
			}

			if (ntb[RTA_ENCAP] && ntb[RTA_ENCAP_TYPE]) {
				_nl_auto_rtnl_nh_encap struct rtnl_nh_encap
					*encap = NULL;

				err = nh_encap_parse_msg(ntb[RTA_ENCAP],
							 ntb[RTA_ENCAP_TYPE],
							 &encap);
				if (err < 0)
					return err;

				err = rtnl_route_nh_set_encap(
					nh, _nl_steal_pointer(&encap));
				if (err < 0)
					return err;
			}
		}

		rtnl_route_add_nexthop(route, _nl_steal_pointer(&nh));
		tlen -= RTNH_ALIGN(rtnh->rtnh_len);
		rtnh = RTNH_NEXT(rtnh);
	}

	return 0;
}

int rtnl_route_parse(struct nlmsghdr *nlh, struct rtnl_route **result)
{
	_nl_auto_rtnl_route struct rtnl_route *route = NULL;
	_nl_auto_rtnl_nexthop struct rtnl_nexthop *old_nh = NULL;
	_nl_auto_nl_addr struct nl_addr *src = NULL;
	_nl_auto_nl_addr struct nl_addr *dst = NULL;
	struct nlattr *tb[RTA_MAX + 1];
	struct rtmsg *rtm;
	int family;
	int err;

	route = rtnl_route_alloc();
	if (!route)
		return -NLE_NOMEM;

	route->ce_msgtype = nlh->nlmsg_type;

	err = nlmsg_parse(nlh, sizeof(struct rtmsg), tb, RTA_MAX, route_policy);
	if (err < 0)
		return err;

	rtm = nlmsg_data(nlh);
	route->rt_family = family = rtm->rtm_family;
	route->rt_tos = rtm->rtm_tos;
	route->rt_table = rtm->rtm_table;
	route->rt_type = rtm->rtm_type;
	route->rt_scope = rtm->rtm_scope;
	route->rt_protocol = rtm->rtm_protocol;
	route->rt_flags = rtm->rtm_flags;
	route->rt_prio = 0;

	route->ce_mask |= ROUTE_ATTR_FAMILY | ROUTE_ATTR_TOS |
			  ROUTE_ATTR_TABLE | ROUTE_ATTR_TYPE |
			  ROUTE_ATTR_SCOPE | ROUTE_ATTR_PROTOCOL |
			  ROUTE_ATTR_FLAGS;

	/* right now MPLS does not allow rt_prio to be set, so don't
	 * assume it is unless it comes from an attribute
	 */
	if (family != AF_MPLS)
		route->ce_mask |= ROUTE_ATTR_PRIO;

	if (tb[RTA_DST]) {
		if (!(dst = nl_addr_alloc_attr(tb[RTA_DST], family)))
			return -NLE_NOMEM;
	} else {
		int len;

		switch (family) {
			case AF_INET:
				len = 4;
				break;

			case AF_INET6:
				len = 16;
				break;
			default:
				len = 0;
				break;
		}

		if (!(dst = nl_addr_build(family, NULL, len)))
			return -NLE_NOMEM;
	}

	nl_addr_set_prefixlen(dst, rtm->rtm_dst_len);
	err = rtnl_route_set_dst(route, dst);
	if (err < 0)
		return err;

	if (tb[RTA_SRC]) {
		if (!(src = nl_addr_alloc_attr(tb[RTA_SRC], family)))
			return -NLE_NOMEM;
	} else if (rtm->rtm_src_len)
		if (!(src = nl_addr_alloc(0)))
			return -NLE_NOMEM;

	if (src) {
		nl_addr_set_prefixlen(src, rtm->rtm_src_len);
		rtnl_route_set_src(route, src);
	}

	if (tb[RTA_TABLE])
		rtnl_route_set_table(route, nla_get_u32(tb[RTA_TABLE]));

	if (tb[RTA_IIF])
		rtnl_route_set_iif(route, nla_get_u32(tb[RTA_IIF]));

	if (tb[RTA_PRIORITY])
		rtnl_route_set_priority(route, nla_get_u32(tb[RTA_PRIORITY]));

	if (tb[RTA_PREFSRC]) {
		_nl_auto_nl_addr struct nl_addr *addr = NULL;

		if (!(addr = nl_addr_alloc_attr(tb[RTA_PREFSRC], family)))
			return -NLE_NOMEM;
		rtnl_route_set_pref_src(route, addr);
	}

	if (tb[RTA_METRICS]) {
		struct nlattr *mtb[RTAX_MAX + 1];
		int i;

		err = nla_parse_nested(mtb, RTAX_MAX, tb[RTA_METRICS], NULL);
		if (err < 0)
			return err;

		for (i = 1; i <= RTAX_MAX; i++) {
			if (mtb[i] && _nla_len(mtb[i]) >= sizeof(uint32_t)) {
				uint32_t m = nla_get_u32(mtb[i]);

				err = rtnl_route_set_metric(route, i, m);
				if (err < 0)
					return err;
			}
		}
	}

	if (tb[RTA_MULTIPATH]) {
		if ((err = parse_multipath(route, tb[RTA_MULTIPATH])) < 0)
			return err;
	}

	if (tb[RTA_CACHEINFO]) {
		nla_memcpy(&route->rt_cacheinfo, tb[RTA_CACHEINFO],
			   sizeof(route->rt_cacheinfo));
		route->ce_mask |= ROUTE_ATTR_CACHEINFO;
	}

	if (tb[RTA_OIF]) {
		if (!old_nh && !(old_nh = rtnl_route_nh_alloc()))
			return -NLE_NOMEM;

		rtnl_route_nh_set_ifindex(old_nh, nla_get_u32(tb[RTA_OIF]));
	}

	if (tb[RTA_GATEWAY]) {
		_nl_auto_nl_addr struct nl_addr *addr = NULL;

		if (!old_nh && !(old_nh = rtnl_route_nh_alloc()))
			return -NLE_NOMEM;

		if (!(addr = nl_addr_alloc_attr(tb[RTA_GATEWAY], family)))
			return -NLE_NOMEM;

		rtnl_route_nh_set_gateway(old_nh, addr);
	}

	if (tb[RTA_FLOW]) {
		if (!old_nh && !(old_nh = rtnl_route_nh_alloc()))
			return -NLE_NOMEM;

		rtnl_route_nh_set_realms(old_nh, nla_get_u32(tb[RTA_FLOW]));
	}

	if (tb[RTA_NEWDST]) {
		_nl_auto_nl_addr struct nl_addr *addr = NULL;

		if (!old_nh && !(old_nh = rtnl_route_nh_alloc()))
			return -NLE_NOMEM;

		addr = nl_addr_alloc_attr(tb[RTA_NEWDST], route->rt_family);
		if (!addr)
			return -NLE_NOMEM;

		err = rtnl_route_nh_set_newdst(old_nh, addr);
		if (err < 0)
			return err;
	}

	if (tb[RTA_VIA]) {
		int alen = nla_len(tb[RTA_VIA]) - offsetof(struct rtvia, rtvia_addr);
		_nl_auto_nl_addr struct nl_addr *addr = NULL;
		struct rtvia *via = nla_data(tb[RTA_VIA]);

		if (!old_nh && !(old_nh = rtnl_route_nh_alloc()))
			return -NLE_NOMEM;

		addr = nl_addr_build(via->rtvia_family, via->rtvia_addr, alen);
		if (!addr)
			return -NLE_NOMEM;

		err = rtnl_route_nh_set_via(old_nh, addr);
		if (err < 0)
			return err;
	}

	if (tb[RTA_TTL_PROPAGATE]) {
		rtnl_route_set_ttl_propagate(route,
					     nla_get_u8(tb[RTA_TTL_PROPAGATE]));
	}

	if (tb[RTA_ENCAP] && tb[RTA_ENCAP_TYPE]) {
		_nl_auto_rtnl_nh_encap struct rtnl_nh_encap *encap = NULL;

		if (!old_nh && !(old_nh = rtnl_route_nh_alloc()))
			return -NLE_NOMEM;

		err = nh_encap_parse_msg(tb[RTA_ENCAP], tb[RTA_ENCAP_TYPE],
					 &encap);
		if (err < 0)
			return err;

		err = rtnl_route_nh_set_encap(old_nh,
					      _nl_steal_pointer(&encap));
		if (err < 0)
			return err;
	}

	if (tb[RTA_NH_ID]) {
		rtnl_route_set_nhid(route, nla_get_u32(tb[RTA_NH_ID]));
	}

	if (old_nh) {
		rtnl_route_nh_set_flags(old_nh, rtm->rtm_flags & 0xff);
		if (route->rt_nr_nh == 0) {
			/* If no nexthops have been provided via RTA_MULTIPATH
			 * we add it as regular nexthop to maintain backwards
			 * compatibility */
			rtnl_route_add_nexthop(route, _nl_steal_pointer(&old_nh));
		} else {
			/* Kernel supports new style nexthop configuration,
			 * verify that it is a duplicate and discard nexthop. */
			struct rtnl_nexthop *first;

			first = nl_list_first_entry(&route->rt_nexthops,
						    struct rtnl_nexthop,
						    rtnh_list);
			if (!first)
				BUG();

			if (rtnl_route_nh_compare(old_nh, first,
						  old_nh->ce_mask, 0)) {
				return -NLE_INVAL;
			}
		}
	}

	*result = _nl_steal_pointer(&route);
	return 0;
}

int rtnl_route_build_msg(struct nl_msg *msg, struct rtnl_route *route)
{
	int i;
	struct nlattr *metrics;
	struct rtmsg rtmsg = {
		.rtm_family = route->rt_family,
		.rtm_tos = route->rt_tos,
		.rtm_table = route->rt_table,
		.rtm_protocol = route->rt_protocol,
		.rtm_scope = route->rt_scope,
		.rtm_type = route->rt_type,
		.rtm_flags = route->rt_flags,
	};

	if (route->rt_dst == NULL)
		return -NLE_MISSING_ATTR;

	rtmsg.rtm_dst_len = nl_addr_get_prefixlen(route->rt_dst);
	if (route->rt_src)
		rtmsg.rtm_src_len = nl_addr_get_prefixlen(route->rt_src);

	if (!(route->ce_mask & ROUTE_ATTR_SCOPE))
		rtmsg.rtm_scope = rtnl_route_guess_scope(route);

	if (rtnl_route_get_nnexthops(route) == 1) {
		struct rtnl_nexthop *nh;
		nh = rtnl_route_nexthop_n(route, 0);
		rtmsg.rtm_flags |= nh->rtnh_flags;
	}

	if (nlmsg_append(msg, &rtmsg, sizeof(rtmsg), NLMSG_ALIGNTO) < 0)
		goto nla_put_failure;

	/* Additional table attribute replacing the 8bit in the header, was
	 * required to allow more than 256 tables. MPLS does not allow the
	 * table attribute to be set
	 */
	if (route->rt_family != AF_MPLS)
		NLA_PUT_U32(msg, RTA_TABLE, route->rt_table);

	if (nl_addr_get_len(route->rt_dst))
		NLA_PUT_ADDR(msg, RTA_DST, route->rt_dst);

	if (route->ce_mask & ROUTE_ATTR_PRIO)
		NLA_PUT_U32(msg, RTA_PRIORITY, route->rt_prio);

	if (route->ce_mask & ROUTE_ATTR_SRC)
		NLA_PUT_ADDR(msg, RTA_SRC, route->rt_src);

	if (route->ce_mask & ROUTE_ATTR_PREF_SRC)
		NLA_PUT_ADDR(msg, RTA_PREFSRC, route->rt_pref_src);

	if (route->ce_mask & ROUTE_ATTR_IIF)
		NLA_PUT_U32(msg, RTA_IIF, route->rt_iif);

	if (route->ce_mask & ROUTE_ATTR_TTL_PROPAGATE)
		NLA_PUT_U8(msg, RTA_TTL_PROPAGATE, route->rt_ttl_propagate);

	if (route->rt_nmetrics > 0) {
		uint32_t val;

		metrics = nla_nest_start(msg, RTA_METRICS);
		if (metrics == NULL)
			goto nla_put_failure;

		for (i = 1; i <= RTAX_MAX; i++) {
			if (!rtnl_route_get_metric(route, i, &val))
				NLA_PUT_U32(msg, i, val);
		}

		nla_nest_end(msg, metrics);
	}

	/* Nexthop specification and nexthop id are mutually exclusive */
	if (route->ce_mask & ROUTE_ATTR_NHID) {
		NLA_PUT_U32(msg, RTA_NH_ID, route->rt_nhid);
	} else if (rtnl_route_get_nnexthops(route) == 1) {
		struct rtnl_nexthop *nh;

		nh = rtnl_route_nexthop_n(route, 0);
		if (nh->rtnh_gateway)
			NLA_PUT_ADDR(msg, RTA_GATEWAY, nh->rtnh_gateway);
		if (nh->rtnh_ifindex)
			NLA_PUT_U32(msg, RTA_OIF, nh->rtnh_ifindex);
		if (nh->rtnh_realms)
			NLA_PUT_U32(msg, RTA_FLOW, nh->rtnh_realms);
		if (nh->rtnh_newdst)
			NLA_PUT_ADDR(msg, RTA_NEWDST, nh->rtnh_newdst);
		if (nh->rtnh_via && rtnl_route_put_via(msg, nh->rtnh_via) < 0)
			goto nla_put_failure;
		if (nh->rtnh_encap &&
		    nh_encap_build_msg(msg, nh->rtnh_encap) < 0)
			goto nla_put_failure;
	} else if (rtnl_route_get_nnexthops(route) > 1) {
		struct nlattr *multipath;
		struct rtnl_nexthop *nh;

		if (!(multipath = nla_nest_start(msg, RTA_MULTIPATH)))
			goto nla_put_failure;

		nl_list_for_each_entry(nh, &route->rt_nexthops, rtnh_list) {
			struct rtnexthop *rtnh;

			rtnh = nlmsg_reserve(msg, sizeof(*rtnh), NLMSG_ALIGNTO);
			if (!rtnh)
				goto nla_put_failure;

			rtnh->rtnh_flags = nh->rtnh_flags;
			rtnh->rtnh_hops = nh->rtnh_weight;
			rtnh->rtnh_ifindex = nh->rtnh_ifindex;

			if (nh->rtnh_gateway)
				NLA_PUT_ADDR(msg, RTA_GATEWAY,
					     nh->rtnh_gateway);

			if (nh->rtnh_newdst)
				NLA_PUT_ADDR(msg, RTA_NEWDST, nh->rtnh_newdst);

			if (nh->rtnh_via &&
			    rtnl_route_put_via(msg, nh->rtnh_via) < 0)
				goto nla_put_failure;

			if (nh->rtnh_realms)
				NLA_PUT_U32(msg, RTA_FLOW, nh->rtnh_realms);

			if (nh->rtnh_encap &&
			    nh_encap_build_msg(msg, nh->rtnh_encap) < 0)
				goto nla_put_failure;

			rtnh->rtnh_len = (char *) nlmsg_tail(msg->nm_nlh) -
						(char *) rtnh;
		}

		nla_nest_end(msg, multipath);
	}

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

/** @cond SKIP */
struct nl_object_ops route_obj_ops = {
	.oo_name		= "route/route",
	.oo_size		= sizeof(struct rtnl_route),
	.oo_constructor		= route_constructor,
	.oo_free_data		= route_free_data,
	.oo_clone		= route_clone,
	.oo_dump = {
	    [NL_DUMP_LINE]	= route_dump_line,
	    [NL_DUMP_DETAILS]	= route_dump_details,
	    [NL_DUMP_STATS]	= route_dump_stats,
	},
	.oo_compare		= route_compare,
	.oo_keygen		= route_keygen,
	.oo_update		= route_update,
	.oo_attrs2str		= route_attrs2str,
	.oo_id_attrs		= (ROUTE_ATTR_FAMILY | ROUTE_ATTR_TOS |
				   ROUTE_ATTR_TABLE | ROUTE_ATTR_DST |
				   ROUTE_ATTR_PRIO),
	.oo_id_attrs_get	= route_id_attrs_get,
};
/** @endcond */

/** @} */
