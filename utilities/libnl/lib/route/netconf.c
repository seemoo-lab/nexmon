/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2017 David Ahern <dsa@cumulusnetworks.com>
 */

/**
 * @ingroup rtnl
 * @defgroup netconf Netconf
 * @brief
 *
 * @{
 */

#include "nl-default.h"

#include <linux/netconf.h>
#include <linux/socket.h>

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/netconf.h>
#include <netlink/hashtable.h>

#include "nl-route.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-priv-dynamic-core/cache-api.h"
#include "nl-priv-dynamic-core/object-api.h"

/** @cond SKIP */
#define NETCONF_ATTR_FAMILY		0x0001
#define NETCONF_ATTR_IFINDEX		0x0002
#define NETCONF_ATTR_RP_FILTER		0x0004
#define NETCONF_ATTR_FWDING		0x0008
#define NETCONF_ATTR_MC_FWDING		0x0010
#define NETCONF_ATTR_PROXY_NEIGH	0x0020
#define NETCONF_ATTR_IGNORE_RT_LINKDWN	0x0040
#define NETCONF_ATTR_INPUT		0x0080

struct rtnl_netconf
{
	NLHDR_COMMON

	int	family;
	int	ifindex;
	int	rp_filter;
	int	forwarding;
	int	mc_forwarding;
	int	proxy_neigh;
	int	ignore_routes_linkdown;
	int	input;
};

static struct nl_cache_ops rtnl_netconf_ops;
static struct nl_object_ops netconf_obj_ops;
/** @endcond */

static struct nla_policy devconf_ipv4_policy[NETCONFA_MAX+1] = {
	[NETCONFA_IFINDEX]	 = { .type = NLA_S32 },
	[NETCONFA_FORWARDING]	 = { .type = NLA_S32 },
	[NETCONFA_MC_FORWARDING] = { .type = NLA_S32 },
	[NETCONFA_RP_FILTER]	 = { .type = NLA_S32 },
	[NETCONFA_PROXY_NEIGH]	 = { .type = NLA_S32 },
	[NETCONFA_IGNORE_ROUTES_WITH_LINKDOWN]  = { .type = NLA_S32 },
};

static struct nla_policy devconf_ipv6_policy[NETCONFA_MAX+1] = {
	[NETCONFA_IFINDEX]	 = { .type = NLA_S32 },
	[NETCONFA_FORWARDING]	 = { .type = NLA_S32 },
	[NETCONFA_MC_FORWARDING] = { .type = NLA_S32 },
	[NETCONFA_PROXY_NEIGH]	 = { .type = NLA_S32 },
	[NETCONFA_IGNORE_ROUTES_WITH_LINKDOWN]  = { .type = NLA_S32 },
};

static struct nla_policy devconf_mpls_policy[NETCONFA_MAX+1] = {
	[NETCONFA_IFINDEX]	 = { .type = NLA_S32 },
	[NETCONFA_INPUT]	 = { .type = NLA_S32 },
};

static struct rtnl_netconf *rtnl_netconf_alloc(void)
{
	return (struct rtnl_netconf *) nl_object_alloc(&netconf_obj_ops);
}

static int netconf_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
			      struct nlmsghdr *nlh, struct nl_parser_param *pp)
{
	struct nlattr *tb[NETCONFA_MAX+1], *attr;
	struct rtnl_netconf *nc;
	struct netconfmsg *ncm;
	int err;

	ncm = nlmsg_data(nlh);
	switch (ncm->ncm_family) {
	case AF_INET:
		err = nlmsg_parse(nlh, sizeof(*ncm), tb, NETCONFA_MAX,
				  devconf_ipv4_policy);
		if (err < 0)
			return err;
		break;
	case AF_INET6:
		err = nlmsg_parse(nlh, sizeof(*ncm), tb, NETCONFA_MAX,
				  devconf_ipv6_policy);
		if (err < 0)
			return err;
		break;
	case AF_MPLS:
		err = nlmsg_parse(nlh, sizeof(*ncm), tb, NETCONFA_MAX,
				  devconf_mpls_policy);
		if (err < 0)
			return err;
		break;
	default:
		printf("unexpected netconf family: %d\n", ncm->ncm_family);
		return -1;
	}

	if (!tb[NETCONFA_IFINDEX])
		return -1;

	nc = rtnl_netconf_alloc();
	if (!nc)
		return -NLE_NOMEM;

	nc->ce_msgtype = nlh->nlmsg_type;
	nc->family = ncm->ncm_family;
	nc->ifindex = nla_get_s32(tb[NETCONFA_IFINDEX]);

	nc->ce_mask = NETCONF_ATTR_FAMILY | NETCONF_ATTR_IFINDEX;


	if (tb[NETCONFA_RP_FILTER]) {
		attr = tb[NETCONFA_RP_FILTER];
		nc->rp_filter = nla_get_s32(attr);
		nc->ce_mask |= NETCONF_ATTR_RP_FILTER;
	}

	if (tb[NETCONFA_FORWARDING]) {
		attr = tb[NETCONFA_FORWARDING];
		nc->forwarding = nla_get_s32(attr);
		nc->ce_mask |= NETCONF_ATTR_FWDING;
	}

	if (tb[NETCONFA_MC_FORWARDING]) {
		attr = tb[NETCONFA_MC_FORWARDING];
		nc->mc_forwarding = nla_get_s32(attr);
		nc->ce_mask |= NETCONF_ATTR_MC_FWDING;
	}

	if (tb[NETCONFA_PROXY_NEIGH]) {
		attr = tb[NETCONFA_PROXY_NEIGH];
		nc->proxy_neigh = nla_get_s32(attr);
		nc->ce_mask |= NETCONF_ATTR_PROXY_NEIGH;
	}

	if (tb[NETCONFA_IGNORE_ROUTES_WITH_LINKDOWN]) {
		attr = tb[NETCONFA_IGNORE_ROUTES_WITH_LINKDOWN];
		nc->ignore_routes_linkdown = nla_get_s32(attr);
		nc->ce_mask |= NETCONF_ATTR_IGNORE_RT_LINKDWN;
	}

	if (tb[NETCONFA_INPUT]) {
		attr = tb[NETCONFA_INPUT];
		nc->input = nla_get_s32(attr);
		nc->ce_mask |= NETCONF_ATTR_INPUT;
	}

	err = pp->pp_cb((struct nl_object *) nc, pp);

	rtnl_netconf_put(nc);
	return err;
}

static int netconf_request_update(struct nl_cache *cache, struct nl_sock *sk)
{
	struct netconfmsg nc = {
		.ncm_family = cache->c_iarg1,
	};

	return nl_send_simple(sk, RTM_GETNETCONF, NLM_F_DUMP, &nc, sizeof(nc));
}

static void netconf_dump_line(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_netconf *nc = (struct rtnl_netconf *) obj;
	struct nl_cache *link_cache;
	char buf[64];

	switch(nc->family) {
	case AF_INET:
		nl_dump(p, "ipv4 ");
		break;
	case AF_INET6:
		nl_dump(p, "ipv6 ");
		break;
	case AF_MPLS:
		nl_dump(p, "mpls ");
		break;
	default:
		return;
	}

	switch(nc->ifindex) {
	case NETCONFA_IFINDEX_ALL:
		nl_dump(p, "all ");
		break;
	case NETCONFA_IFINDEX_DEFAULT:
		nl_dump(p, "default ");
		break;
	default:
		link_cache = nl_cache_mngt_require_safe("route/link");
		if (link_cache) {
			nl_dump(p, "dev %s ",
				rtnl_link_i2name(link_cache, nc->ifindex,
						 buf, sizeof(buf)));
			nl_cache_put(link_cache);
		} else
			nl_dump(p, "dev %d ", nc->ifindex);
	}

	if (nc->ce_mask & NETCONF_ATTR_FWDING) {
		nl_dump(p, "forwarding %s ",
			nc->forwarding ? "on" : "off");
	}

	if (nc->ce_mask & NETCONF_ATTR_RP_FILTER) {
		if (nc->rp_filter == 0)
			nl_dump(p, "rp_filter off ");
		else if (nc->rp_filter == 1)
			nl_dump(p, "rp_filter strict ");
		else if (nc->rp_filter == 2)
			nl_dump(p, "rp_filter loose ");
		else
			nl_dump(p, "rp_filter unknown-mode ");
	}

	if (nc->ce_mask & NETCONF_ATTR_MC_FWDING) {
		nl_dump(p, "mc_forwarding %s ",
			nc->mc_forwarding ? "on" : "off");
	}

	if (nc->ce_mask & NETCONF_ATTR_PROXY_NEIGH)
		nl_dump(p, "proxy_neigh %d ", nc->proxy_neigh);

	if (nc->ce_mask & NETCONF_ATTR_IGNORE_RT_LINKDWN) {
		nl_dump(p, "ignore_routes_with_linkdown %s ",
			nc->ignore_routes_linkdown ? "on" : "off");
	}

	if (nc->ce_mask & NETCONF_ATTR_INPUT)
		nl_dump(p, "input %s ", nc->input ? "on" : "off");

	nl_dump(p, "\n");
}

static const struct trans_tbl netconf_attrs[] = {
	__ADD(NETCONF_ATTR_FAMILY, family),
	__ADD(NETCONF_ATTR_IFINDEX, ifindex),
	__ADD(NETCONF_ATTR_RP_FILTER, rp_filter),
	__ADD(NETCONF_ATTR_FWDING, forwarding),
	__ADD(NETCONF_ATTR_MC_FWDING, mc_forwarding),
	__ADD(NETCONF_ATTR_PROXY_NEIGH, proxy_neigh),
	__ADD(NETCONF_ATTR_IGNORE_RT_LINKDWN, ignore_routes_with_linkdown),
	__ADD(NETCONF_ATTR_INPUT, input),
};

static char *netconf_attrs2str(int attrs, char *buf, size_t len)
{
	return __flags2str(attrs, buf, len, netconf_attrs,
			   ARRAY_SIZE(netconf_attrs));
}

static void netconf_keygen(struct nl_object *obj, uint32_t *hashkey,
			   uint32_t table_sz)
{
	struct rtnl_netconf *nc = (struct rtnl_netconf *) obj;
	unsigned int nckey_sz;
	struct nc_hash_key {
		int        nc_family;
		int        nc_index;
	} _nl_packed nckey;

	nckey_sz = sizeof(nckey);
	nckey.nc_family = nc->family;
	nckey.nc_index = nc->ifindex;

	*hashkey = nl_hash(&nckey, nckey_sz, 0) % table_sz;

	NL_DBG(5, "netconf %p key (dev %d fam %d) keysz %d, hash 0x%x\n",
	       nc, nckey.nc_index, nckey.nc_family, nckey_sz, *hashkey);
}

static uint64_t netconf_compare(struct nl_object *_a, struct nl_object *_b,
			     uint64_t attrs, int flags)
{
	struct rtnl_netconf *a = (struct rtnl_netconf *) _a;
	struct rtnl_netconf *b = (struct rtnl_netconf *) _b;
	uint64_t diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(NETCONF_ATTR_FAMILY, a->family != b->family);
	diff |= _DIFF(NETCONF_ATTR_IFINDEX, a->ifindex != b->ifindex);
	diff |= _DIFF(NETCONF_ATTR_RP_FILTER, a->rp_filter != b->rp_filter);
	diff |= _DIFF(NETCONF_ATTR_FWDING, a->forwarding != b->forwarding);
	diff |= _DIFF(NETCONF_ATTR_MC_FWDING,
		      a->mc_forwarding != b->mc_forwarding);
	diff |= _DIFF(NETCONF_ATTR_PROXY_NEIGH,
		      a->proxy_neigh != b->proxy_neigh);
	diff |= _DIFF(NETCONF_ATTR_IGNORE_RT_LINKDWN,
		      a->ignore_routes_linkdown != b->ignore_routes_linkdown);
	diff |= _DIFF(NETCONF_ATTR_INPUT, a->input != b->input);
#undef _DIFF

	return diff;
}

static int netconf_update(struct nl_object *old_obj, struct nl_object *new_obj)
{
	struct rtnl_netconf *new_nc = (struct rtnl_netconf *) new_obj;
	struct rtnl_netconf *old_nc = (struct rtnl_netconf *) old_obj;
	int action = new_obj->ce_msgtype;

	switch(action) {
	case RTM_NEWNETCONF:
		if (new_nc->family != old_nc->family ||
		    new_nc->ifindex != old_nc->ifindex)
			return -NLE_OPNOTSUPP;

		if (new_nc->ce_mask & NETCONF_ATTR_RP_FILTER)
			old_nc->rp_filter = new_nc->rp_filter;
		if (new_nc->ce_mask & NETCONF_ATTR_FWDING)
			old_nc->forwarding = new_nc->forwarding;
		if (new_nc->ce_mask & NETCONF_ATTR_MC_FWDING)
			old_nc->mc_forwarding = new_nc->mc_forwarding;
		if (new_nc->ce_mask & NETCONF_ATTR_PROXY_NEIGH)
			old_nc->proxy_neigh = new_nc->proxy_neigh;
		if (new_nc->ce_mask & NETCONF_ATTR_IGNORE_RT_LINKDWN)
			old_nc->ignore_routes_linkdown = new_nc->ignore_routes_linkdown;

		break;
	default:
		return -NLE_OPNOTSUPP;
	}

	return NLE_SUCCESS;
}

/**
 * @name Cache Management
 * @{
 */

int rtnl_netconf_alloc_cache(struct nl_sock *sk, struct nl_cache **result)
{
	return nl_cache_alloc_and_fill(&rtnl_netconf_ops, sk, result);
}

/**
 * Search netconf in cache
 * @arg cache		netconf cache
 * @arg family		Address family of interest
 * @arg ifindex		Interface index of interest
 *
 * Searches netconf cache previously allocated with rtnl_netconf_alloc_cache()
 * for given index and family
 *
 * The reference counter is incremented before returning the netconf entry,
 * therefore the reference must be given back with rtnl_netconf_put() after
 * usage.
 *
 * @return netconf object or NULL if no match was found.
 */
struct rtnl_netconf *rtnl_netconf_get_by_idx(struct nl_cache *cache, int family,
					     int ifindex)
{
	struct rtnl_netconf *nc;

	if (!ifindex || !family || cache->c_ops != &rtnl_netconf_ops)
		return NULL;

	nl_list_for_each_entry(nc, &cache->c_items, ce_list) {
		if (nc->ifindex == ifindex &&
		    nc->family == family) {
			nl_object_get((struct nl_object *) nc);
			return nc;
		}
	}

	return NULL;
}

void rtnl_netconf_put(struct rtnl_netconf *nc)
{
	nl_object_put((struct nl_object *) nc);
}

/**
 * Search netconf in cache
 * @arg cache		netconf cache
 * @arg family		Address family of interest
 *
 * Searches netconf cache previously allocated with rtnl_netconf_alloc_cache()
 * for "all" netconf settings for given family
 *
 * The reference counter is incremented before returning the netconf entry,
 * therefore the reference must be given back with rtnl_netconf_put() after
 * usage.
 *
 * @return netconf object or NULL if no match was found.
 */
struct rtnl_netconf *rtnl_netconf_get_all(struct nl_cache *cache, int family)
{
	return rtnl_netconf_get_by_idx(cache, family, NETCONFA_IFINDEX_ALL);
}

/**
 * Search netconf in cache
 * @arg cache		netconf cache
 * @arg family		Address family of interest
 *
 * Searches netconf cache previously allocated with rtnl_netconf_alloc_cache()
 * for "default" netconf settings for given family
 *
 * The reference counter is incremented before returning the netconf entry,
 * therefore the reference must be given back with rtnl_netconf_put() after
 * usage.
 *
 * @return netconf object or NULL if no match was found.
 */
struct rtnl_netconf *rtnl_netconf_get_default(struct nl_cache *cache, int family)
{
	return rtnl_netconf_get_by_idx(cache, family, NETCONFA_IFINDEX_DEFAULT);
}

/** @} */

/**
 * @name Attributes
 * @{
 */

int rtnl_netconf_get_family(struct rtnl_netconf *nc, int *val)
{
	if (!nc)
		return -NLE_INVAL;
	if (!(nc->ce_mask & NETCONF_ATTR_FAMILY))
		return -NLE_MISSING_ATTR;
	if (val)
		*val = nc->family;
	return 0;
}
int rtnl_netconf_get_ifindex(struct rtnl_netconf *nc, int *val)
{
	if (!nc)
		return -NLE_INVAL;
	if (!(nc->ce_mask & NETCONF_ATTR_IFINDEX))
		return -NLE_MISSING_ATTR;
	if (val)
		*val = nc->ifindex;
	return 0;
}
int rtnl_netconf_get_forwarding(struct rtnl_netconf *nc, int *val)
{
	if (!nc)
		return -NLE_INVAL;
	if (!(nc->ce_mask & NETCONF_ATTR_FWDING))
		return -NLE_MISSING_ATTR;
	if (val)
		*val = nc->forwarding;
	return 0;
}
int rtnl_netconf_get_mc_forwarding(struct rtnl_netconf *nc, int *val)
{
	if (!nc)
		return -NLE_INVAL;
	if (!(nc->ce_mask & NETCONF_ATTR_MC_FWDING))
		return -NLE_MISSING_ATTR;
	if (val)
		*val = nc->mc_forwarding;
	return 0;
}
int rtnl_netconf_get_rp_filter(struct rtnl_netconf *nc, int *val)
{
	if (!nc)
		return -NLE_INVAL;
	if (!(nc->ce_mask & NETCONF_ATTR_RP_FILTER))
		return -NLE_MISSING_ATTR;
	if (val)
		*val = nc->rp_filter;
	return 0;
}
int rtnl_netconf_get_proxy_neigh(struct rtnl_netconf *nc, int *val)
{
	if (!nc)
		return -NLE_INVAL;
	if (!(nc->ce_mask & NETCONF_ATTR_PROXY_NEIGH))
		return -NLE_MISSING_ATTR;
	if (val)
		*val = nc->proxy_neigh;
	return 0;
}
int rtnl_netconf_get_ignore_routes_linkdown(struct rtnl_netconf *nc, int *val)
{
	if (!nc)
		return -NLE_INVAL;
	if (!(nc->ce_mask & NETCONF_ATTR_IGNORE_RT_LINKDWN))
		return -NLE_MISSING_ATTR;
	if (val)
		*val = nc->ignore_routes_linkdown;
	return 0;
}
int rtnl_netconf_get_input(struct rtnl_netconf *nc, int *val)
{
	if (!nc)
		return -NLE_INVAL;
	if (!(nc->ce_mask & NETCONF_ATTR_INPUT))
		return -NLE_MISSING_ATTR;
	if (val)
		*val = nc->input;
	return 0;
}


/** @} */

static struct nl_object_ops netconf_obj_ops = {
	.oo_name		= "route/netconf",
	.oo_size		= sizeof(struct rtnl_netconf),
	.oo_dump = {
	    [NL_DUMP_LINE] 	= netconf_dump_line,
	    [NL_DUMP_DETAILS] 	= netconf_dump_line,
	},
	.oo_compare		= netconf_compare,
	.oo_keygen		= netconf_keygen,
	.oo_update		= netconf_update,
	.oo_attrs2str		= netconf_attrs2str,
	.oo_id_attrs		= (NETCONF_ATTR_FAMILY      |
				   NETCONF_ATTR_IFINDEX)
};

static struct nl_af_group netconf_groups[] = {
	{ AF_INET,	RTNLGRP_IPV4_NETCONF },
	{ AF_INET6,	RTNLGRP_IPV6_NETCONF },
	{ AF_MPLS,	RTNLGRP_MPLS_NETCONF },
	{ END_OF_GROUP_LIST },
};

static struct nl_cache_ops rtnl_netconf_ops = {
	.co_name		= "route/netconf",
	.co_hdrsize		= sizeof(struct netconfmsg),
	.co_msgtypes		= {
					{ RTM_NEWNETCONF, NL_ACT_NEW, "new" },
					{ RTM_DELNETCONF, NL_ACT_DEL, "del" },
					{ RTM_GETNETCONF, NL_ACT_GET, "get" },
					END_OF_MSGTYPES_LIST,
				  },
	.co_protocol		= NETLINK_ROUTE,
	.co_groups		= netconf_groups,
	.co_request_update      = netconf_request_update,
	.co_msg_parser          = netconf_msg_parser,
	.co_obj_ops		= &netconf_obj_ops,
};

static void _nl_init netconf_init(void)
{
	nl_cache_mngt_register(&rtnl_netconf_ops);
}

static void _nl_exit netconf_exit(void)
{
	nl_cache_mngt_unregister(&rtnl_netconf_ops);
}

/** @} */
