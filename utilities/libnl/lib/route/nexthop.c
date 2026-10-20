/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup route_obj
 * @defgroup nexthop Nexthop
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/route.h>

#include "nexthop-encap.h"
#include "nl-aux-route/nl-route.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-route.h"

/** @cond SKIP */
#define NH_ATTR_FLAGS 0x000001
#define NH_ATTR_WEIGHT 0x000002
#define NH_ATTR_IFINDEX 0x000004
#define NH_ATTR_GATEWAY 0x000008
#define NH_ATTR_REALMS 0x000010
#define NH_ATTR_NEWDST 0x000020
#define NH_ATTR_VIA 0x000040
#define NH_ATTR_ENCAP 0x000080
/** @endcond */

/**
 * @name Allocation/Freeing
 * @{
 */

struct rtnl_nexthop *rtnl_route_nh_alloc(void)
{
	struct rtnl_nexthop *nh;

	nh = calloc(1, sizeof(*nh));
	if (!nh)
		return NULL;

	nl_init_list_head(&nh->rtnh_list);

	return nh;
}

struct rtnl_nexthop *rtnl_route_nh_clone(struct rtnl_nexthop *src)
{
	_nl_auto_rtnl_nexthop struct rtnl_nexthop *nh = NULL;

	nh = rtnl_route_nh_alloc();
	if (!nh)
		return NULL;

	nh->rtnh_flags = src->rtnh_flags;
	nh->rtnh_flag_mask = src->rtnh_flag_mask;
	nh->rtnh_weight = src->rtnh_weight;
	nh->rtnh_ifindex = src->rtnh_ifindex;
	nh->rtnh_realms = src->rtnh_realms;
	nh->ce_mask = src->ce_mask;

	if (src->rtnh_gateway) {
		nh->rtnh_gateway = nl_addr_clone(src->rtnh_gateway);
		if (!nh->rtnh_gateway)
			return NULL;
	}

	if (src->rtnh_newdst) {
		nh->rtnh_newdst = nl_addr_clone(src->rtnh_newdst);
		if (!nh->rtnh_newdst)
			return NULL;
	}

	if (src->rtnh_via) {
		nh->rtnh_via = nl_addr_clone(src->rtnh_via);
		if (!nh->rtnh_via)
			return NULL;
	}

	/* Clone encapsulation information if present */
	if (src->rtnh_encap) {
		nh->rtnh_encap = rtnl_nh_encap_clone(src->rtnh_encap);
		if (!nh->rtnh_encap)
			return NULL;
	}

	return _nl_steal_pointer(&nh);
}

void rtnl_route_nh_free(struct rtnl_nexthop *nh)
{
	nl_addr_put(nh->rtnh_gateway);
	nl_addr_put(nh->rtnh_newdst);
	nl_addr_put(nh->rtnh_via);
	rtnl_nh_encap_free(nh->rtnh_encap);
	free(nh);
}

/** @} */

int rtnl_route_nh_compare(struct rtnl_nexthop *a, struct rtnl_nexthop *b,
			  uint32_t attrs, int loose)
{
	uint32_t diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(NH_ATTR_IFINDEX, a->rtnh_ifindex != b->rtnh_ifindex);
	diff |= _DIFF(NH_ATTR_WEIGHT, a->rtnh_weight != b->rtnh_weight);
	diff |= _DIFF(NH_ATTR_REALMS, a->rtnh_realms != b->rtnh_realms);
	diff |= _DIFF(NH_ATTR_GATEWAY,
		      nl_addr_cmp(a->rtnh_gateway, b->rtnh_gateway));
	diff |= _DIFF(NH_ATTR_NEWDST,
		      nl_addr_cmp(a->rtnh_newdst, b->rtnh_newdst));
	diff |= _DIFF(NH_ATTR_VIA, nl_addr_cmp(a->rtnh_via, b->rtnh_via));
	diff |= _DIFF(NH_ATTR_ENCAP,
		      nh_encap_compare(a->rtnh_encap, b->rtnh_encap));

	if (loose)
		diff |= _DIFF(NH_ATTR_FLAGS, (a->rtnh_flags ^ b->rtnh_flags) &
						     b->rtnh_flag_mask);
	else
		diff |= _DIFF(NH_ATTR_FLAGS, a->rtnh_flags != b->rtnh_flags);
#undef _DIFF

	return diff;
}

/**
 * Check if the fixed attributes of two nexthops are identical, and may
 * only differ in flags or weight.
 *
 * @arg a		a nexthop
 * @arg b		another nexthop
 *
 * @return true if both nexthop have equal attributes, otherwise false.
 */
int rtnl_route_nh_identical(struct rtnl_nexthop *a, struct rtnl_nexthop *b)
{
	return !rtnl_route_nh_compare(a, b,
				      NH_ATTR_IFINDEX | NH_ATTR_REALMS |
					      NH_ATTR_GATEWAY | NH_ATTR_NEWDST |
					      NH_ATTR_VIA | NH_ATTR_ENCAP,
				      0);
}

static void nh_dump_line(struct rtnl_nexthop *nh, struct nl_dump_params *dp)
{
	struct nl_cache *link_cache;
	char buf[128];

	link_cache = nl_cache_mngt_require_safe("route/link");

	if (nh->ce_mask & NH_ATTR_ENCAP)
		nh_encap_dump(nh->rtnh_encap, dp);

	if (nh->ce_mask & NH_ATTR_NEWDST)
		nl_dump(dp, "as to %s ",
			nl_addr2str(nh->rtnh_newdst, buf, sizeof(buf)));

	nl_dump(dp, "via");

	if (nh->ce_mask & NH_ATTR_VIA)
		nl_dump(dp, " %s", nl_addr2str(nh->rtnh_via, buf, sizeof(buf)));

	if (nh->ce_mask & NH_ATTR_GATEWAY)
		nl_dump(dp, " %s",
			nl_addr2str(nh->rtnh_gateway, buf, sizeof(buf)));

	if (nh->ce_mask & NH_ATTR_IFINDEX) {
		if (link_cache) {
			nl_dump(dp, " dev %s",
				rtnl_link_i2name(link_cache, nh->rtnh_ifindex,
						 buf, sizeof(buf)));
		} else
			nl_dump(dp, " dev %d", nh->rtnh_ifindex);
	}

	nl_dump(dp, " ");

	if (link_cache)
		nl_cache_put(link_cache);
}

static void nh_dump_details(struct rtnl_nexthop *nh, struct nl_dump_params *dp)
{
	struct nl_cache *link_cache;
	char buf[128];

	link_cache = nl_cache_mngt_require_safe("route/link");

	nl_dump(dp, "nexthop");

	if (nh->ce_mask & NH_ATTR_ENCAP)
		nh_encap_dump(nh->rtnh_encap, dp);

	if (nh->ce_mask & NH_ATTR_NEWDST)
		nl_dump(dp, " as to %s",
			nl_addr2str(nh->rtnh_newdst, buf, sizeof(buf)));

	if (nh->ce_mask & NH_ATTR_VIA)
		nl_dump(dp, " via %s",
			nl_addr2str(nh->rtnh_via, buf, sizeof(buf)));

	if (nh->ce_mask & NH_ATTR_GATEWAY)
		nl_dump(dp, " via %s",
			nl_addr2str(nh->rtnh_gateway, buf, sizeof(buf)));

	if (nh->ce_mask & NH_ATTR_IFINDEX) {
		if (link_cache) {
			nl_dump(dp, " dev %s",
				rtnl_link_i2name(link_cache, nh->rtnh_ifindex,
						 buf, sizeof(buf)));
		} else
			nl_dump(dp, " dev %d", nh->rtnh_ifindex);
	}

	if (nh->ce_mask & NH_ATTR_WEIGHT)
		nl_dump(dp, " weight %u", nh->rtnh_weight);

	if (nh->ce_mask & NH_ATTR_REALMS)
		nl_dump(dp, " realm %04x:%04x",
			RTNL_REALM_FROM(nh->rtnh_realms),
			RTNL_REALM_TO(nh->rtnh_realms));

	if (nh->ce_mask & NH_ATTR_FLAGS)
		nl_dump(dp, " <%s>",
			rtnl_route_nh_flags2str(nh->rtnh_flags, buf,
						sizeof(buf)));

	if (link_cache)
		nl_cache_put(link_cache);
}

void rtnl_route_nh_dump(struct rtnl_nexthop *nh, struct nl_dump_params *dp)
{
	switch (dp->dp_type) {
	case NL_DUMP_LINE:
		nh_dump_line(nh, dp);
		break;

	case NL_DUMP_DETAILS:
	case NL_DUMP_STATS:
		if (dp->dp_ivar == NH_DUMP_FROM_DETAILS)
			nh_dump_details(nh, dp);
		break;

	default:
		break;
	}
}

/**
 * @name Attributes
 * @{
 */

void rtnl_route_nh_set_weight(struct rtnl_nexthop *nh, uint8_t weight)
{
	nh->rtnh_weight = weight;
	nh->ce_mask |= NH_ATTR_WEIGHT;
}

uint8_t rtnl_route_nh_get_weight(struct rtnl_nexthop *nh)
{
	return nh->rtnh_weight;
}

void rtnl_route_nh_set_ifindex(struct rtnl_nexthop *nh, int ifindex)
{
	nh->rtnh_ifindex = ifindex;
	nh->ce_mask |= NH_ATTR_IFINDEX;
}

int rtnl_route_nh_get_ifindex(struct rtnl_nexthop *nh)
{
	return nh->rtnh_ifindex;
}

/* FIXME: Convert to return an int */
void rtnl_route_nh_set_gateway(struct rtnl_nexthop *nh, struct nl_addr *addr)
{
	struct nl_addr *old = nh->rtnh_gateway;

	if (addr) {
		nh->rtnh_gateway = nl_addr_get(addr);
		nh->ce_mask |= NH_ATTR_GATEWAY;
	} else {
		nh->ce_mask &= ~NH_ATTR_GATEWAY;
		nh->rtnh_gateway = NULL;
	}

	if (old)
		nl_addr_put(old);
}

struct nl_addr *rtnl_route_nh_get_gateway(struct rtnl_nexthop *nh)
{
	return nh->rtnh_gateway;
}

void rtnl_route_nh_set_flags(struct rtnl_nexthop *nh, unsigned int flags)
{
	nh->rtnh_flag_mask |= flags;
	nh->rtnh_flags |= flags;
	nh->ce_mask |= NH_ATTR_FLAGS;
}

void rtnl_route_nh_unset_flags(struct rtnl_nexthop *nh, unsigned int flags)
{
	nh->rtnh_flag_mask |= flags;
	nh->rtnh_flags &= ~flags;
	nh->ce_mask |= NH_ATTR_FLAGS;
}

unsigned int rtnl_route_nh_get_flags(struct rtnl_nexthop *nh)
{
	return nh->rtnh_flags;
}

void rtnl_route_nh_set_realms(struct rtnl_nexthop *nh, uint32_t realms)
{
	nh->rtnh_realms = realms;
	nh->ce_mask |= NH_ATTR_REALMS;
}

uint32_t rtnl_route_nh_get_realms(struct rtnl_nexthop *nh)
{
	return nh->rtnh_realms;
}

int rtnl_route_nh_set_newdst(struct rtnl_nexthop *nh, struct nl_addr *addr)
{
	struct nl_addr *old = nh->rtnh_newdst;

	if (addr) {
		nh->rtnh_newdst = nl_addr_get(addr);
		nh->ce_mask |= NH_ATTR_NEWDST;
	} else {
		nh->ce_mask &= ~NH_ATTR_NEWDST;
		nh->rtnh_newdst = NULL;
	}

	if (old)
		nl_addr_put(old);

	return 0;
}

struct nl_addr *rtnl_route_nh_get_newdst(struct rtnl_nexthop *nh)
{
	return nh->rtnh_newdst;
}

int rtnl_route_nh_set_via(struct rtnl_nexthop *nh, struct nl_addr *addr)
{
	struct nl_addr *old = nh->rtnh_via;

	if (addr) {
		nh->rtnh_via = nl_addr_get(addr);
		nh->ce_mask |= NH_ATTR_VIA;
	} else {
		nh->ce_mask &= ~NH_ATTR_VIA;
		nh->rtnh_via = NULL;
	}

	if (old)
		nl_addr_put(old);

	return 0;
}

struct nl_addr *rtnl_route_nh_get_via(struct rtnl_nexthop *nh)
{
	return nh->rtnh_via;
}

/**
 * Set nexthop encapsulation
 * @arg nh		Route nexthop object
 * @arg nh_encap	Encapsulation descriptor
 *
 * Assigns ownership of the encapsulation object to the route's nexthop.
 * Any previously configured encapsulation is released. Passing a NULL
 * encapsulation clears the encapsulation on the nexthop.
 *
 * On failure, the function consumes and frees \p nh_encap.
 *
 * @return 0 on success, or the appropriate error-code on failure.
 */
int rtnl_route_nh_set_encap(struct rtnl_nexthop *nh,
			    struct rtnl_nh_encap *nh_encap)
{
	if (!nh) {
		rtnl_nh_encap_free(nh_encap);
		return -NLE_INVAL;
	}

	if (nh_encap && !nh_encap->ops) {
		rtnl_nh_encap_free(nh_encap);
		return -NLE_INVAL;
	}

	rtnl_nh_encap_free(nh->rtnh_encap);

	if (nh_encap) {
		nh->rtnh_encap = nh_encap;
		nh->ce_mask |= NH_ATTR_ENCAP;
	} else {
		nh->rtnh_encap = NULL;
		nh->ce_mask &= ~NH_ATTR_ENCAP;
	}

	return 0;
}

/** @} */

/**
 * @name Nexthop Flags Translations
 * @{
 */

static const struct trans_tbl nh_flags[] = {
	__ADD(RTNH_F_DEAD, dead),
	__ADD(RTNH_F_PERVASIVE, pervasive),
	__ADD(RTNH_F_ONLINK, onlink),
};

char *rtnl_route_nh_flags2str(int flags, char *buf, size_t len)
{
	return __flags2str(flags, buf, len, nh_flags, ARRAY_SIZE(nh_flags));
}

int rtnl_route_nh_str2flags(const char *name)
{
	return __str2flags(name, nh_flags, ARRAY_SIZE(nh_flags));
}

/** @} */

/** @} */
