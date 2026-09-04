/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2022 Stanislav Zaikin <zstaseg@gmail.com>
 */

#include "nl-default.h"

#include <linux/nexthop.h>
#include <linux/lwtunnel.h>
#include <linux/mpls_iptunnel.h>

#include <netlink/route/nh.h>
#include <netlink/hashtable.h>
#include <netlink/route/nexthop.h>

#include "nexthop-encap.h"
#include "nl-aux-route/nl-route.h"
#include "nl-route.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-priv-dynamic-core/cache-api.h"

/** @cond SKIP */
struct rtnl_nh {
	NLHDR_COMMON

	uint8_t nh_family;
	uint32_t nh_flags;

	uint32_t nh_id;
	uint16_t nh_group_type;
	nl_nh_group_t *nh_group;
	uint32_t nh_oif;
	struct nl_addr *nh_gateway;
	struct rtnl_nh_encap *nh_encap;

	/* Resilient nexthop group parameters */
	uint16_t res_grp_buckets;
	uint32_t res_grp_idle_timer;
	uint32_t res_grp_unbalanced_timer;
};

#define NH_ATTR_FLAGS (1 << 0)
#define NH_ATTR_ID (1 << 1)
#define NH_ATTR_GROUP (1 << 2)
#define NH_ATTR_FLAG_BLACKHOLE (1 << 3)
#define NH_ATTR_OIF (1 << 4)
#define NH_ATTR_GATEWAY (1 << 5)
#define NH_ATTR_FLAG_GROUPS (1 << 6)
#define NH_ATTR_GROUP_TYPE (1 << 7)
#define NH_ATTR_FLAG_FDB (1 << 8)
/* Resilient nexthop group attributes */
#define NH_ATTR_RES_GROUP (1 << 9)
#define NH_ATTR_RES_BUCKETS (1 << 10)
#define NH_ATTR_RES_IDLE_TIMER (1 << 11)
#define NH_ATTR_RES_UNBALANCED_TIMER (1 << 12)
#define NH_ATTR_ENCAP (1 << 13)
/** @endcond */

struct nla_policy rtnl_nh_policy[NHA_MAX + 1] = {
	[NHA_UNSPEC] = { .type = NLA_UNSPEC },
	[NHA_ID] = { .type = NLA_U32 },
	[NHA_GROUP] = { .type = NLA_NESTED },
	[NHA_GROUP_TYPE] = { .type = NLA_U16 },
	[NHA_BLACKHOLE] = { .type = NLA_UNSPEC },
	[NHA_OIF] = { .type = NLA_U32 },
	[NHA_RES_GROUP] = { .type = NLA_NESTED },
	[NHA_ENCAP] = { .type = NLA_NESTED },
	[NHA_ENCAP_TYPE] = { .type = NLA_U16 },
};

static struct nl_cache_ops rtnl_nh_ops;
static struct nl_object_ops nh_obj_ops;

static nl_nh_group_t *rtnl_nh_grp_alloc(unsigned size)
{
	nl_nh_group_t *nhg;

	_nl_assert(size <= (unsigned)INT_MAX);

	if (!(nhg = calloc(1, sizeof(*nhg))))
		return NULL;

	nhg->size = size;

	if (!(nhg->entries = calloc(size, sizeof(*nhg->entries)))) {
		free(nhg);
		return NULL;
	}

	nhg->ce_refcnt = 1;

	return nhg;
}

static void rtnl_nh_grp_put(nl_nh_group_t *nhg)
{
	if (!nhg)
		return;

	_nl_assert(nhg->ce_refcnt > 0);

	nhg->ce_refcnt--;

	if (nhg->ce_refcnt > 0)
		return;

	free(nhg->entries);
	free(nhg);
}

static int rtnh_nh_grp_cmp(const nl_nh_group_t *a, const nl_nh_group_t *b)
{
	unsigned i;

	_NL_CMP_SELF(a, b);
	_NL_CMP_DIRECT(a->size, b->size);
	for (i = 0; i < a->size; i++) {
		_NL_CMP_DIRECT(a->entries[i].nh_id, b->entries[i].nh_id);
		_NL_CMP_DIRECT(a->entries[i].weight, b->entries[i].weight);
	}
	return 0;
}

static int rtnh_nh_grp_clone(nl_nh_group_t *src, nl_nh_group_t **dst)
{
	nl_nh_group_t *ret;
	unsigned i;

	ret = rtnl_nh_grp_alloc(src->size);

	if (!ret)
		return -NLE_NOMEM;

	for (i = 0; i < src->size; i++) {
		ret->entries[i].nh_id = src->entries[i].nh_id;
		ret->entries[i].weight = src->entries[i].weight;
	}

	*dst = ret;

	return NLE_SUCCESS;
}

struct rtnl_nh *rtnl_nh_alloc(void)
{
	return (struct rtnl_nh *)nl_object_alloc(&nh_obj_ops);
}

static int nh_clone(struct nl_object *_src, struct nl_object *_dst)
{
	struct rtnl_nh *dst = nl_object_priv(_dst);
	struct rtnl_nh *src = nl_object_priv(_src);

	dst->nh_flags = src->nh_flags;
	dst->nh_family = src->nh_family;
	dst->nh_id = src->nh_id;
	dst->nh_oif = src->nh_oif;
	dst->nh_group_type = src->nh_group_type;
	dst->res_grp_buckets = src->res_grp_buckets;
	dst->res_grp_idle_timer = src->res_grp_idle_timer;
	dst->res_grp_unbalanced_timer = src->res_grp_unbalanced_timer;
	dst->ce_mask = src->ce_mask;

	if (src->nh_encap) {
		dst->nh_encap = rtnl_nh_encap_clone(src->nh_encap);
		if (!dst->nh_encap)
			return -NLE_NOMEM;
		dst->ce_mask |= NH_ATTR_ENCAP;
	}

	if (src->nh_gateway) {
		dst->nh_gateway = nl_addr_clone(src->nh_gateway);
		if (!dst->nh_gateway) {
			return -NLE_NOMEM;
		}
	}

	if (src->nh_group) {
		if (rtnh_nh_grp_clone(src->nh_group, &dst->nh_group) < 0) {
			return -NLE_NOMEM;
		}
	}

	return 0;
}

static void nh_free(struct nl_object *obj)
{
	struct rtnl_nh *nh = nl_object_priv(obj);

	nl_addr_put(nh->nh_gateway);
	rtnl_nh_encap_free(nh->nh_encap);
	rtnl_nh_grp_put(nh->nh_group);
}

void rtnl_nh_put(struct rtnl_nh *nh)
{
	struct nl_object *obj = (struct nl_object *)nh;

	nl_object_put(obj);
}

static void nexthop_keygen(struct nl_object *obj, uint32_t *hashkey,
			   uint32_t table_sz)
{
	struct rtnl_nh *nexthop = nl_object_priv(obj);
	unsigned int lkey_sz;
	struct nexthop_hash_key {
		uint32_t nh_id;
	} _nl_packed lkey;

	lkey_sz = sizeof(lkey);
	lkey.nh_id = nexthop->nh_id;

	*hashkey = nl_hash(&lkey, lkey_sz, 0) % table_sz;

	return;
}

int rtnl_nh_set_gateway(struct rtnl_nh *nexthop, struct nl_addr *addr)
{
	struct nl_addr *old = NULL;

	if (!nexthop)
		return -NLE_INVAL;

	/* preserve old pointer to release after successful update */
	old = nexthop->nh_gateway;

	if (addr) {
		struct nl_addr *cloned = nl_addr_clone(addr);
		if (!cloned)
			return -NLE_NOMEM;

		nexthop->nh_gateway = cloned;
		nexthop->ce_mask |= NH_ATTR_GATEWAY;
	} else {
		nexthop->nh_gateway = NULL;
		nexthop->ce_mask &= ~NH_ATTR_GATEWAY;
	}

	if (old)
		nl_addr_put(old);

	return 0;
}

struct nl_addr *rtnl_nh_get_gateway(struct rtnl_nh *nexthop)
{
	return nexthop->nh_gateway;
}

/**
 * Set nexthop encapsulation
 * @arg nh          Nexthop object
 * @arg encap       Encapsulation descriptor
 *
 * Assigns ownership of the encapsulation object to the nexthop. Any
 * previously configured encapsulation is released. Passing a NULL
 * encapsulation clears the encapsulation on the nexthop.
 *
 * On failure, the function consumes and frees encap.
 *
 * @return 0 on success, or the appropriate error-code on failure.
 */
int rtnl_nh_set_encap(struct rtnl_nh *nh, struct rtnl_nh_encap *encap)
{
	if (!nh) {
		rtnl_nh_encap_free(encap);
		return -NLE_INVAL;
	}

	if (encap && !encap->ops) {
		rtnl_nh_encap_free(encap);
		return -NLE_INVAL;
	}

	rtnl_nh_encap_free(nh->nh_encap);

	if (encap) {
		nh->nh_encap = encap;
		nh->ce_mask |= NH_ATTR_ENCAP;
	} else {
		nh->nh_encap = NULL;
		nh->ce_mask &= ~NH_ATTR_ENCAP;
	}

	return 0;
}

struct rtnl_nh_encap *rtnl_nh_get_encap(struct rtnl_nh *nh)
{
	if (!nh || !(nh->ce_mask & NH_ATTR_ENCAP))
		return NULL;

	return nh->nh_encap;
}

int rtnl_nh_set_fdb(struct rtnl_nh *nexthop, int value)
{
	if (value)
		nexthop->ce_mask |= NH_ATTR_FLAG_FDB;
	else
		nexthop->ce_mask &= ~NH_ATTR_FLAG_FDB;

	return 0;
}

int rtnl_nh_get_oif(struct rtnl_nh *nexthop)
{
	if (nexthop->ce_mask & NH_ATTR_OIF)
		return nexthop->nh_oif;

	return 0;
}

int rtnl_nh_set_oif(struct rtnl_nh *nexthop, uint32_t ifindex)
{
	if (!nexthop)
		return -NLE_INVAL;

	nexthop->nh_oif = (uint32_t)ifindex;

	if (nexthop->nh_oif)
		nexthop->ce_mask |= NH_ATTR_OIF;
	else
		nexthop->ce_mask &= ~NH_ATTR_OIF;

	return 0;
}

int rtnl_nh_get_fdb(struct rtnl_nh *nexthop)
{
	return nexthop->ce_mask & NH_ATTR_FLAG_FDB;
}

int rtnl_nh_set_family(struct rtnl_nh *nexthop, uint8_t family)
{
	if (!nexthop)
		return -NLE_INVAL;

	nexthop->nh_family = family;

	return 0;
}

int rtnl_nh_get_family(struct rtnl_nh *nexthop)
{
	if (!nexthop)
		return -NLE_INVAL;

	return nexthop->nh_family;
}

int rtnl_nh_set_group_type(struct rtnl_nh *nexthop, uint16_t group_type)
{
	if (!nexthop)
		return -NLE_INVAL;

	nexthop->nh_group_type = group_type;
	nexthop->ce_mask |= NH_ATTR_GROUP_TYPE;

	return 0;
}

int rtnl_nh_get_group_type(struct rtnl_nh *nexthop)
{
	if (!nexthop)
		return -NLE_INVAL;

	if (!(nexthop->ce_mask & NH_ATTR_GROUP_TYPE))
		return -NLE_INVAL;

	return (int)nexthop->nh_group_type;
}

static int _nh_resilient_check(struct rtnl_nh *nexthop)
{
	if (!nexthop)
		return -NLE_INVAL;

	/* Group type must be explicitly set to resilient */
	if (nexthop->nh_group_type != NEXTHOP_GRP_TYPE_RES)
		return -NLE_INVAL;

	return 0;
}

int rtnl_nh_set_res_group_bucket_size(struct rtnl_nh *nexthop, uint16_t buckets)
{
	int err = _nh_resilient_check(nexthop);
	if (err < 0)
		return err;

	nexthop->res_grp_buckets = buckets;

	if (buckets) {
		nexthop->ce_mask |= NH_ATTR_RES_BUCKETS;
	} else {
		nexthop->ce_mask &= ~NH_ATTR_RES_BUCKETS;
	}

	return 0;
}

int rtnl_nh_get_res_group_bucket_size(struct rtnl_nh *nexthop)
{
	int err = _nh_resilient_check(nexthop);
	if (err < 0)
		return err;

	if (!(nexthop->ce_mask & NH_ATTR_RES_BUCKETS))
		return -NLE_MISSING_ATTR;

	return nexthop->res_grp_buckets;
}

int rtnl_nh_set_res_group_idle_timer(struct rtnl_nh *nexthop,
				     uint32_t idle_timer)
{
	int err = _nh_resilient_check(nexthop);
	if (err < 0)
		return err;

	nexthop->res_grp_idle_timer = idle_timer;
	nexthop->ce_mask |= NH_ATTR_RES_IDLE_TIMER;

	return 0;
}

int rtnl_nh_get_res_group_idle_timer(struct rtnl_nh *nexthop,
				     uint32_t *out_value)
{
	int err = _nh_resilient_check(nexthop);
	if (err < 0)
		return err;

	if (!out_value)
		return -NLE_INVAL;

	if (!(nexthop->ce_mask & NH_ATTR_RES_IDLE_TIMER))
		return -NLE_MISSING_ATTR;

	*out_value = nexthop->res_grp_idle_timer;

	return 0;
}

int rtnl_nh_set_res_group_unbalanced_timer(struct rtnl_nh *nexthop,
					   uint32_t unbalanced_timer)
{
	int err = _nh_resilient_check(nexthop);
	if (err < 0)
		return err;

	nexthop->res_grp_unbalanced_timer = unbalanced_timer;
	nexthop->ce_mask |= NH_ATTR_RES_UNBALANCED_TIMER;

	return 0;
}

int rtnl_nh_get_res_group_unbalanced_timer(struct rtnl_nh *nexthop,
					   uint32_t *out_value)
{
	int err = _nh_resilient_check(nexthop);
	if (err < 0)
		return err;

	if (!out_value)
		return -NLE_INVAL;

	if (!(nexthop->ce_mask & NH_ATTR_RES_UNBALANCED_TIMER))
		return -NLE_MISSING_ATTR;

	*out_value = nexthop->res_grp_unbalanced_timer;

	return 0;
}

int rtnl_nh_set_group(struct rtnl_nh *nexthop,
		      const nl_nh_group_info_t *entries, unsigned size)
{
	nl_nh_group_t *nhg = NULL;

	if (!nexthop)
		return -NLE_INVAL;

	if (size > 0 && !entries)
		return -NLE_INVAL;

	if (size == 0) {
		/* size is 0, thus we want to remove the nh group */

		rtnl_nh_grp_put(nexthop->nh_group);
		nexthop->nh_group = NULL;
		nexthop->ce_mask &= ~NH_ATTR_GROUP;

		return 0;
	}

	nhg = rtnl_nh_grp_alloc(size);
	if (!nhg)
		return -NLE_NOMEM;

	memcpy(nhg->entries, entries, size * sizeof(*nhg->entries));

	/* Replace an existing group if present. */
	rtnl_nh_grp_put(nexthop->nh_group);
	nexthop->nh_group = nhg;
	nexthop->ce_mask |= NH_ATTR_GROUP;

	return 0;
}

int rtnl_nh_get_group_entry(struct rtnl_nh *nexthop, int n)
{
	if (!(nexthop->ce_mask & NH_ATTR_GROUP) || !nexthop->nh_group)
		return -NLE_MISSING_ATTR;

	if (n < 0 || ((unsigned)n) >= nexthop->nh_group->size)
		return -NLE_INVAL;

	return nexthop->nh_group->entries[n].nh_id;
}

int rtnl_nh_get_group_size(struct rtnl_nh *nexthop)
{
	if (!(nexthop->ce_mask & NH_ATTR_GROUP) || !nexthop->nh_group)
		return -NLE_MISSING_ATTR;

	_nl_assert(nexthop->nh_group->size <= INT_MAX);

	return (int)nexthop->nh_group->size;
}

static int rtnl_nh_grp_info(unsigned size, const struct nexthop_grp *vi,
			    nl_nh_group_t **nvi)
{
	nl_nh_group_t *ret;
	unsigned i;

	if (!(ret = rtnl_nh_grp_alloc(size)))
		return -NLE_NOMEM;

	for (i = 0; i < size; i++) {
		ret->entries[i].nh_id = vi[i].id;
		ret->entries[i].weight = vi[i].weight;
	}

	*nvi = ret;
	return NLE_SUCCESS;
}

int rtnl_nh_get_id(struct rtnl_nh *nh)
{
	if (nh->ce_mask & NH_ATTR_ID)
		return nh->nh_id;

	return -NLE_INVAL;
}

int rtnl_nh_set_id(struct rtnl_nh *nh, uint32_t id)
{
	if (!nh)
		return -NLE_INVAL;

	nh->nh_id = id;

	if (nh->nh_id)
		nh->ce_mask |= NH_ATTR_ID;
	else
		nh->ce_mask &= ~NH_ATTR_ID;

	return 0;
}

/* ------------------------------------------------------------------------- */
/* Message construction & kernel interaction                                */
/* ------------------------------------------------------------------------- */

/* Build a netlink message representing the supplied nexthop object. */
static int rtnl_nh_build_msg(struct nl_msg *msg, struct rtnl_nh *nh)
{
	struct nhmsg hdr = {
		.nh_family = nh->nh_family,
		.nh_protocol = 0, /* kernel will fill in */
		.nh_flags = nh->nh_flags,
	};

	if (nlmsg_append(msg, &hdr, sizeof(hdr), NLMSG_ALIGNTO) < 0)
		return -NLE_MSGSIZE;

	/* Optional attributes */
	if (nh->ce_mask & NH_ATTR_ID)
		NLA_PUT_U32(msg, NHA_ID, nh->nh_id);

	if (nh->ce_mask & NH_ATTR_OIF)
		NLA_PUT_U32(msg, NHA_OIF, nh->nh_oif);

	if (nh->ce_mask & NH_ATTR_GATEWAY) {
		if (!nh->nh_gateway)
			return -NLE_INVAL;
		NLA_PUT_ADDR(msg, NHA_GATEWAY, nh->nh_gateway);
	}

	if (nh->ce_mask & NH_ATTR_FLAG_BLACKHOLE)
		NLA_PUT_FLAG(msg, NHA_BLACKHOLE);

	if (nh->ce_mask & NH_ATTR_ENCAP) {
		struct nlattr *encap;

		if (!nh->nh_encap || !nh->nh_encap->ops)
			return -NLE_INVAL;

		NLA_PUT_U16(msg, NHA_ENCAP_TYPE, nh->nh_encap->ops->encap_type);

		encap = nla_nest_start(msg, NHA_ENCAP);
		if (!encap)
			goto nla_put_failure;

		if (nh->nh_encap->ops->build_msg) {
			int err = nh->nh_encap->ops->build_msg(
				msg, nh->nh_encap->priv);
			if (err < 0)
				return err;
		}
		nla_nest_end(msg, encap);
	}

	/* Nexthop group */
	if (nh->ce_mask & NH_ATTR_GROUP) {
		struct nexthop_grp *grp;
		struct nlattr *attr;
		unsigned int sz;

		if (!nh->nh_group || nh->nh_group->size == 0)
			return -NLE_INVAL;

		sz = nh->nh_group->size * sizeof(struct nexthop_grp);
		attr = nla_reserve(msg, NHA_GROUP, sz);
		if (!attr)
			goto nla_put_failure;

		grp = nla_data(attr);
		for (unsigned int i = 0; i < nh->nh_group->size; i++) {
			grp[i].id = nh->nh_group->entries[i].nh_id;
			grp[i].weight = nh->nh_group->entries[i].weight;
			grp[i].resvd1 = 0;
			grp[i].resvd2 = 0;
		}

		/* Optional group type */
		if (nh->nh_group_type)
			NLA_PUT_U16(msg, NHA_GROUP_TYPE, nh->nh_group_type);

		/* If the group type is resilient and the caller supplied additional
		 * resilient parameters (bucket size, timers, ...), add them as a
		 * nested NHA_RES_GROUP attribute. Only pass through the parameters
		 * that were explicitly set on the nexthop object.
		 */
		if (nh->nh_group_type == NEXTHOP_GRP_TYPE_RES &&
		    (nh->ce_mask &
		     (NH_ATTR_RES_BUCKETS | NH_ATTR_RES_IDLE_TIMER |
		      NH_ATTR_RES_UNBALANCED_TIMER))) {
			struct nlattr *res_grp;

			res_grp = nla_nest_start(msg, NHA_RES_GROUP);
			if (!res_grp)
				goto nla_put_failure;

			if (nh->ce_mask & NH_ATTR_RES_BUCKETS)
				NLA_PUT_U16(msg, NHA_RES_GROUP_BUCKETS,
					    nh->res_grp_buckets);

			if (nh->ce_mask & NH_ATTR_RES_IDLE_TIMER)
				NLA_PUT_U32(msg, NHA_RES_GROUP_IDLE_TIMER,
					    nh->res_grp_idle_timer);

			if (nh->ce_mask & NH_ATTR_RES_UNBALANCED_TIMER)
				NLA_PUT_U32(msg, NHA_RES_GROUP_UNBALANCED_TIMER,
					    nh->res_grp_unbalanced_timer);

			nla_nest_end(msg, res_grp);
		}
	}

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

/* Helper to build generic nexthop request messages */
static int build_nh_msg(struct rtnl_nh *tmpl, int cmd, int flags,
			struct nl_msg **result)
{
	_nl_auto_nl_msg struct nl_msg *msg = NULL;
	int err;

	msg = nlmsg_alloc_simple(cmd, flags);
	if (!msg)
		return -NLE_NOMEM;

	err = rtnl_nh_build_msg(msg, tmpl);
	if (err < 0) {
		return err;
	}

	*result = _nl_steal_pointer(&msg);
	return 0;
}

static int rtnl_nh_build_add_request(struct rtnl_nh *tmpl, int flags,
				     struct nl_msg **result)
{
	return build_nh_msg(tmpl, RTM_NEWNEXTHOP, NLM_F_CREATE | flags, result);
}

int rtnl_nh_add(struct nl_sock *sk, struct rtnl_nh *nh, int flags)
{
	_nl_auto_nl_msg struct nl_msg *msg = NULL;
	int err;

	err = rtnl_nh_build_add_request(nh, flags, &msg);
	if (err < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

struct rtnl_nh_encap *rtnl_nh_encap_alloc(void)
{
	return calloc(1, sizeof(struct rtnl_nh_encap));
}

int rtnl_nh_encap_get_type(struct rtnl_nh_encap *nh_encap)
{
	if (!nh_encap)
		return -NLE_INVAL;
	if (!nh_encap->ops)
		return -NLE_INVAL;

	return nh_encap->ops->encap_type;
}

void rtnl_nh_encap_free(struct rtnl_nh_encap *nh_encap)
{
	if (!nh_encap)
		return;

	if (nh_encap->ops && nh_encap->ops->destructor)
		nh_encap->ops->destructor(nh_encap->priv);

	free(nh_encap->priv);
	free(nh_encap);
}

struct rtnl_nh_encap *rtnl_nh_encap_clone(struct rtnl_nh_encap *src)
{
	_nl_auto_rtnl_nh_encap struct rtnl_nh_encap *new_encap = NULL;

	if (!src)
		return NULL;

	new_encap = rtnl_nh_encap_alloc();
	if (!new_encap)
		return NULL;

	new_encap->ops = src->ops;
	if (new_encap->ops) {
		new_encap->priv = new_encap->ops->clone(src->priv);
		if (!new_encap->priv)
			return NULL;
	}

	return _nl_steal_pointer(&new_encap);
}

/*
 * Retrieve the encapsulation associated with a nexthop if any.
 */
struct rtnl_nh_encap *rtnl_route_nh_get_encap(struct rtnl_nexthop *nh)
{
	if (!nh)
		return NULL;

	return nh->rtnh_encap;
}

static struct nla_policy nh_res_group_policy[NHA_RES_GROUP_MAX + 1] = {
	[NHA_RES_GROUP_UNSPEC] = { .type = NLA_UNSPEC },
	[NHA_RES_GROUP_BUCKETS] = { .type = NLA_U16 },
	[NHA_RES_GROUP_IDLE_TIMER] = { .type = NLA_U32 },
	[NHA_RES_GROUP_UNBALANCED_TIMER] = { .type = NLA_U32 },
	[NHA_RES_GROUP_UNBALANCED_TIME] = { .type = NLA_U64 },
};

static int nexthop_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
			      struct nlmsghdr *n, struct nl_parser_param *pp)
{
	_nl_auto_rtnl_nh struct rtnl_nh *nexthop = NULL;
	struct nhmsg *ifi;
	struct nlattr *tb[NHA_MAX + 1];
	int err;
	int family;

	nexthop = rtnl_nh_alloc();
	if (nexthop == NULL)
		return -NLE_NOMEM;

	nexthop->ce_msgtype = n->nlmsg_type;

	if (!nlmsg_valid_hdr(n, sizeof(*ifi)))
		return -NLE_MSG_TOOSHORT;

	ifi = nlmsg_data(n);
	family = ifi->nh_family;
	nexthop->nh_family = family;
	nexthop->nh_flags = ifi->nh_flags;
	nexthop->ce_mask = (NH_ATTR_FLAGS);

	err = nlmsg_parse(n, sizeof(*ifi), tb, NHA_MAX, rtnl_nh_policy);
	if (err < 0)
		return err;

	if (tb[NHA_ID]) {
		nexthop->nh_id = nla_get_u32(tb[NHA_ID]);
		nexthop->ce_mask |= NH_ATTR_ID;
	}

	if (tb[NHA_OIF]) {
		nexthop->nh_oif = nla_get_u32(tb[NHA_OIF]);
		nexthop->ce_mask |= NH_ATTR_OIF;
	}

	if (tb[NHA_GATEWAY]) {
		nexthop->nh_gateway =
			nl_addr_alloc_attr(tb[NHA_GATEWAY], family);
		nexthop->ce_mask |= NH_ATTR_GATEWAY;
	}

	if (tb[NHA_GROUP_TYPE]) {
		nexthop->nh_group_type = nla_get_u16(tb[NHA_GROUP_TYPE]);
		nexthop->ce_mask |= NH_ATTR_GROUP_TYPE;
	}

	if (tb[NHA_ENCAP] && tb[NHA_ENCAP_TYPE]) {
		_nl_auto_rtnl_nh_encap struct rtnl_nh_encap *nh_encap = NULL;

		err = nh_encap_parse_msg(tb[NHA_ENCAP], tb[NHA_ENCAP_TYPE],
					 &nh_encap);
		if (err < 0)
			return err;

		err = rtnl_nh_set_encap(nexthop, _nl_steal_pointer(&nh_encap));
		if (err < 0)
			return err;
	}

	if (tb[NHA_BLACKHOLE]) {
		nexthop->ce_mask |= NH_ATTR_FLAG_BLACKHOLE;
	}

	if (tb[NHA_GROUPS]) {
		nexthop->ce_mask |= NH_ATTR_FLAG_GROUPS;
	}

	if (tb[NHA_FDB]) {
		nexthop->ce_mask |= NH_ATTR_FLAG_FDB;
	}

	if (tb[NHA_GROUP]) {
		nl_nh_group_t *nh_group = NULL;
		const void *data;
		unsigned size;
		unsigned len;

		data = nla_data(tb[NHA_GROUP]);
		len = _nla_len(tb[NHA_GROUP]);
		size = len / sizeof(struct nexthop_grp);

		err = rtnl_nh_grp_info(size, (const struct nexthop_grp *)data,
				       &nh_group);
		if (err < 0) {
			return err;
		}

		nexthop->nh_group = nh_group;
		nexthop->ce_mask |= NH_ATTR_GROUP;
	}

	/* Parse resilient nexthop group parameters if present */
	if (tb[NHA_RES_GROUP]) {
		struct nlattr *rg[NHA_RES_GROUP_MAX + 1];

		err = nla_parse_nested(rg, NHA_RES_GROUP_MAX, tb[NHA_RES_GROUP],
				       nh_res_group_policy);
		if (err < 0)
			return err;

		if (rg[NHA_RES_GROUP_BUCKETS]) {
			nexthop->res_grp_buckets =
				nla_get_u16(rg[NHA_RES_GROUP_BUCKETS]);
			nexthop->ce_mask |= NH_ATTR_RES_BUCKETS;
		}
		if (rg[NHA_RES_GROUP_IDLE_TIMER]) {
			nexthop->res_grp_idle_timer =
				nla_get_u32(rg[NHA_RES_GROUP_IDLE_TIMER]);
			nexthop->ce_mask |= NH_ATTR_RES_IDLE_TIMER;
		}
		if (rg[NHA_RES_GROUP_UNBALANCED_TIMER]) {
			nexthop->res_grp_unbalanced_timer =
				nla_get_u32(rg[NHA_RES_GROUP_UNBALANCED_TIMER]);
			nexthop->ce_mask |= NH_ATTR_RES_UNBALANCED_TIMER;
		}
		/* Mark that we have a resilient group block */
		nexthop->ce_mask |= NH_ATTR_RES_GROUP;
	}

	return pp->pp_cb((struct nl_object *)nexthop, pp);
}

static int nexthop_request_update(struct nl_cache *cache, struct nl_sock *sk)
{
	_nl_auto_nl_msg struct nl_msg *msg = NULL;
	int family = cache->c_iarg1;
	struct nhmsg hdr = { .nh_family = family };
	int err;

	msg = nlmsg_alloc_simple(RTM_GETNEXTHOP, NLM_F_DUMP);
	if (!msg)
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &hdr, sizeof(hdr), NLMSG_ALIGNTO) < 0)
		return -NLE_MSGSIZE;

	err = nl_send_auto(sk, msg);
	if (err < 0)
		return err;

	return NLE_SUCCESS;
}

static void dump_nh_group(nl_nh_group_t *group, struct nl_dump_params *dp)
{
	unsigned i;

	nl_dump(dp, " nh_grp:");
	for (i = 0; i < group->size; i++) {
		nl_dump(dp, " %u", group->entries[i].nh_id);
	}
}

static void nh_dump_line(struct nl_object *obj, struct nl_dump_params *dp)
{
	struct nl_cache *cache;
	char buf[128];
	struct rtnl_nh *nh = nl_object_priv(obj);

	cache = nl_cache_mngt_require_safe("route/nh");

	if (nh->ce_mask & NH_ATTR_ID)
		nl_dump(dp, "nhid %u", nh->nh_id);

	if (nh->ce_mask & NH_ATTR_OIF)
		nl_dump(dp, " oif %d", nh->nh_oif);

	if (nh->ce_mask & NH_ATTR_GATEWAY)
		nl_dump(dp, " via %s",
			nl_addr2str(nh->nh_gateway, buf, sizeof(buf)));

	if (nh->ce_mask & NH_ATTR_ENCAP && nh->nh_encap)
		nh_encap_dump(nh->nh_encap, dp);

	if (nh->ce_mask & NH_ATTR_FLAG_BLACKHOLE)
		nl_dump(dp, " blackhole");

	if (nh->ce_mask & NH_ATTR_FLAG_GROUPS)
		nl_dump(dp, " groups");

	if (nh->ce_mask & NH_ATTR_GROUP)
		dump_nh_group(nh->nh_group, dp);

	/* Dump resilient group parameters */
	if (nh->nh_group_type == NEXTHOP_GRP_TYPE_RES) {
		if (nh->ce_mask & NH_ATTR_RES_BUCKETS)
			nl_dump(dp, " buckets %u", nh->res_grp_buckets);
		if (nh->ce_mask & NH_ATTR_RES_IDLE_TIMER)
			nl_dump(dp, " idle-timer %u", nh->res_grp_idle_timer);
		if (nh->ce_mask & NH_ATTR_RES_UNBALANCED_TIMER)
			nl_dump(dp, " unbalanced-timer %u",
				nh->res_grp_unbalanced_timer);
	}

	if (nh->ce_mask & NH_ATTR_FLAG_FDB)
		nl_dump(dp, " fdb");

	nl_dump(dp, "\n");

	if (cache)
		nl_cache_put(cache);
}

static void nh_dump_details(struct nl_object *nh, struct nl_dump_params *dp)
{
	nh_dump_line(nh, dp);
}

static uint64_t nh_compare(struct nl_object *a, struct nl_object *b,
			   uint64_t attrs, int loose)
{
	int diff = 0;
	struct rtnl_nh *src = nl_object_priv(a);
	struct rtnl_nh *dst = nl_object_priv(b);

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(NH_ATTR_ID, src->nh_id != dst->nh_id);
	diff |= _DIFF(NH_ATTR_GATEWAY,
		      nl_addr_cmp(src->nh_gateway, dst->nh_gateway));
	diff |= _DIFF(NH_ATTR_OIF, src->nh_oif != dst->nh_oif);
	diff |= _DIFF(NH_ATTR_GROUP,
		      rtnh_nh_grp_cmp(src->nh_group, dst->nh_group));
	diff |= _DIFF(NH_ATTR_GROUP_TYPE,
		      src->nh_group_type != dst->nh_group_type);
	diff |= _DIFF(NH_ATTR_FLAG_FDB, false);
	diff |= _DIFF(NH_ATTR_FLAG_GROUPS, false);
	diff |= _DIFF(NH_ATTR_FLAG_BLACKHOLE, false);
	diff |= _DIFF(NH_ATTR_RES_BUCKETS,
		      src->res_grp_buckets != dst->res_grp_buckets);
	diff |= _DIFF(NH_ATTR_RES_IDLE_TIMER,
		      src->res_grp_idle_timer != dst->res_grp_idle_timer);
	diff |= _DIFF(NH_ATTR_RES_UNBALANCED_TIMER,
		      src->res_grp_unbalanced_timer !=
			      dst->res_grp_unbalanced_timer);
	diff |= _DIFF(NH_ATTR_ENCAP,
		      nh_encap_compare(src->nh_encap, dst->nh_encap));
#undef _DIFF

	return diff;
}

struct rtnl_nh *rtnl_nh_get(struct nl_cache *cache, int nhid)
{
	struct rtnl_nh *nh;

	if (cache->c_ops != &rtnl_nh_ops)
		return NULL;

	nl_list_for_each_entry(nh, &cache->c_items, ce_list) {
		if (nh->nh_id == ((unsigned)nhid)) {
			nl_object_get((struct nl_object *)nh);
			return nh;
		}
	}

	return NULL;
}

/**
 * Allocate nexthop cache and fill in all configured nexthops.
 * @arg sk		Netnexthop socket.
 * @arg family		nexthop address family or AF_UNSPEC
 * @arg result		Pointer to store resulting cache.
 * @arg flags		Flags to set in nexthop cache before filling
 *
 * Allocates and initializes a new nexthop cache. If \c sk is valid, a netnexthop
 * message is sent to the kernel requesting a full dump of all configured
 * nexthops. The returned messages are parsed and filled into the cache. If
 * the operation succeeds, the resulting cache will contain a nexthop object for
 * each nexthop configured in the kernel. If \c sk is NULL, returns 0 but the
 * cache is still empty.
 *
 * If \c family is set to an address family other than \c AF_UNSPEC the
 * contents of the cache can be limited to a specific address family.
 * Currently the following address families are supported:
 * - AF_BRIDGE
 * - AF_INET6
 *
 * @route_doc{nexthop_list, Get List of nexthops}
 * @see rtnl_nh_get()
 * @see rtnl_nh_get_by_name()
 * @return 0 on success or a negative error code.
 */
static int rtnl_nh_alloc_cache_flags(struct nl_sock *sk, int family,
				     struct nl_cache **result,
				     unsigned int flags)
{
	struct nl_cache *cache;
	int err;

	cache = nl_cache_alloc(&rtnl_nh_ops);
	if (!cache)
		return -NLE_NOMEM;

	cache->c_iarg1 = family;

	if (flags)
		nl_cache_set_flags(cache, flags);

	if (sk && (err = nl_cache_refill(sk, cache)) < 0) {
		nl_cache_free(cache);
		return err;
	}

	*result = cache;
	return 0;
}

/**
 * Allocate nexthop cache and fill in all configured nexthops.
 * @arg sk		Netnexthop socket.
 * @arg family		nexthop address family or AF_UNSPEC
 * @arg result		Pointer to store resulting cache.
 *
 * Allocates and initializes a new nexthop cache. If \c sk is valid, a netnexthop
 * message is sent to the kernel requesting a full dump of all configured
 * nexthops. The returned messages are parsed and filled into the cache. If
 * the operation succeeds, the resulting cache will contain a nexthop object for
 * each nexthop configured in the kernel. If \c sk is NULL, returns 0 but the
 * cache is still empty.
 *
 * If \c family is set to an address family other than \c AF_UNSPEC the
 * contents of the cache can be limited to a specific address family.
 * Currently the following address families are supported:
 * - AF_BRIDGE
 * - AF_INET6
 *
 * @route_doc{nexthop_list, Get List of nexthops}
 * @see rtnl_nh_get()
 * @see rtnl_nh_get_by_name()
 * @return 0 on success or a negative error code.
 */
int rtnl_nh_alloc_cache(struct nl_sock *sk, int family,
			struct nl_cache **result)
{
	return rtnl_nh_alloc_cache_flags(sk, family, result, 0);
}

static struct nl_object_ops nh_obj_ops = {
  .oo_name		= "route/nh",
  .oo_size		= sizeof(struct rtnl_nh),
  .oo_free_data		= nh_free,
  .oo_clone		= nh_clone,
  .oo_dump = {
      [NL_DUMP_LINE]	= nh_dump_line,
      [NL_DUMP_DETAILS]	= nh_dump_details,
  },
  .oo_compare		= nh_compare,
  .oo_keygen		= nexthop_keygen,
  .oo_attrs2str		= rtnl_route_nh_flags2str,
  .oo_id_attrs		= NH_ATTR_ID,
};

static struct nl_af_group nh_groups[] = {
	{ AF_UNSPEC, RTNLGRP_NEXTHOP },
	{ END_OF_GROUP_LIST },
};

static struct nl_cache_ops rtnl_nh_ops = {
  .co_name		= "route/nh",
  .co_hdrsize		= sizeof(struct nhmsg),
  .co_msgtypes		= {
          { RTM_NEWNEXTHOP, NL_ACT_NEW, "new" },
          { RTM_DELNEXTHOP, NL_ACT_DEL, "del" },
          { RTM_GETNEXTHOP, NL_ACT_GET, "get" },
          END_OF_MSGTYPES_LIST,
          },
  .co_protocol  = NETLINK_ROUTE,
  .co_groups		= nh_groups,
  .co_request_update	= nexthop_request_update,
  .co_msg_parser		= nexthop_msg_parser,
  .co_obj_ops		= &nh_obj_ops,
};

static void _nl_init nexthop_init(void)
{
	nl_cache_mngt_register(&rtnl_nh_ops);
}

static void _nl_exit nexthop_exit(void)
{
	nl_cache_mngt_unregister(&rtnl_nh_ops);
}
