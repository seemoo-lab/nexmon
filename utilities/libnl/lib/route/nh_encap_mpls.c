/* SPDX-License-Identifier: LGPL-2.1-only */

#include "nl-default.h"

#include <linux/mpls_iptunnel.h>
#include <linux/lwtunnel.h>

#include <netlink/route/nexthop.h>

#include "nexthop-encap.h"
#include "nl-aux-core/nl-core.h"
#include "nl-aux-route/nl-route.h"
#include "nl-route.h"

struct mpls_iptunnel_encap {
	struct nl_addr *dst;
	uint8_t ttl;
};

static void mpls_encap_dump(void *priv, struct nl_dump_params *dp)
{
	struct mpls_iptunnel_encap *encap_info = priv;
	char buf[256];

	nl_dump(dp, "%s ", nl_addr2str(encap_info->dst, buf, sizeof(buf)));

	if (encap_info->ttl)
		nl_dump(dp, "ttl %u ", encap_info->ttl);
}

static int mpls_encap_build_msg(struct nl_msg *msg, void *priv)
{
	struct mpls_iptunnel_encap *encap_info = priv;

	NLA_PUT_ADDR(msg, MPLS_IPTUNNEL_DST, encap_info->dst);
	if (encap_info->ttl)
		NLA_PUT_U8(msg, MPLS_IPTUNNEL_TTL, encap_info->ttl);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static void mpls_encap_destructor(void *priv)
{
	struct mpls_iptunnel_encap *encap_info = priv;

	nl_addr_put(encap_info->dst);
}

static void *mpls_encap_clone(void *priv)
{
	struct mpls_iptunnel_encap *src = priv;
	struct mpls_iptunnel_encap *clone;

	if (!src)
		return NULL;

	clone = calloc(1, sizeof(*clone));
	if (!clone)
		return NULL;

	clone->dst = nl_addr_get(src->dst);
	clone->ttl = src->ttl;

	return clone;
}

static struct nla_policy mpls_encap_policy[MPLS_IPTUNNEL_MAX + 1] = {
	[MPLS_IPTUNNEL_DST] = { .type = NLA_U32 },
	[MPLS_IPTUNNEL_TTL] = { .type = NLA_U8 },
};

static int mpls_encap_parse_msg(struct nlattr *nla,
				struct rtnl_nh_encap **encap_out)
{
	_nl_auto_rtnl_nh_encap struct rtnl_nh_encap *nh_encap = NULL;
	_nl_auto_nl_addr struct nl_addr *labels = NULL;
	struct nlattr *tb[MPLS_IPTUNNEL_MAX + 1];
	uint8_t ttl = 0;
	int err;

	err = nla_parse_nested(tb, MPLS_IPTUNNEL_MAX, nla, mpls_encap_policy);
	if (err < 0)
		return err;

	if (!tb[MPLS_IPTUNNEL_DST])
		return -NLE_INVAL;

	labels = nl_addr_alloc_attr(tb[MPLS_IPTUNNEL_DST], AF_MPLS);
	if (!labels)
		return -NLE_NOMEM;

	if (tb[MPLS_IPTUNNEL_TTL])
		ttl = nla_get_u8(tb[MPLS_IPTUNNEL_TTL]);

	nh_encap = rtnl_nh_encap_alloc();
	if (!nh_encap)
		return -NLE_NOMEM;

	err = rtnl_nh_encap_mpls(nh_encap, labels, ttl);
	if (err < 0)
		return err;

	*encap_out = _nl_steal_pointer(&nh_encap);

	return 0;
}

static int mpls_encap_compare(void *_a, void *_b)
{
	struct mpls_iptunnel_encap *a = _a;
	struct mpls_iptunnel_encap *b = _b;
	int diff = 0;

	diff |= (a->ttl != b->ttl);
	diff |= nl_addr_cmp(a->dst, b->dst);

	return diff;
}

const struct nh_encap_ops mpls_encap_ops = {
	.encap_type = LWTUNNEL_ENCAP_MPLS,
	.build_msg = mpls_encap_build_msg,
	.parse_msg = mpls_encap_parse_msg,
	.compare = mpls_encap_compare,
	.clone = mpls_encap_clone,
	.dump = mpls_encap_dump,
	.destructor = mpls_encap_destructor,
};

int rtnl_nh_encap_mpls(struct rtnl_nh_encap *nh_encap, struct nl_addr *dst,
		       uint8_t ttl)
{
	struct mpls_iptunnel_encap *mpls_encap;

	if (!dst || !nh_encap)
		return -NLE_INVAL;

	mpls_encap = calloc(1, sizeof(*mpls_encap));
	if (!mpls_encap) {
		return -NLE_NOMEM;
	}

	mpls_encap->dst = nl_addr_get(dst);
	mpls_encap->ttl = ttl;

	nh_encap->priv = mpls_encap;
	nh_encap->ops = &mpls_encap_ops;

	return 0;
}

int rtnl_route_nh_encap_mpls(struct rtnl_nexthop *nh, struct nl_addr *addr,
			     uint8_t ttl)
{
	_nl_auto_rtnl_nh_encap struct rtnl_nh_encap *rtnh_encap = NULL;
	int ret;

	rtnh_encap = rtnl_nh_encap_alloc();
	if (!rtnh_encap)
		return -NLE_NOMEM;

	ret = rtnl_nh_encap_mpls(rtnh_encap, addr, ttl);
	if (ret < 0)
		return ret;

	rtnl_route_nh_set_encap(nh, _nl_steal_pointer(&rtnh_encap));

	return 0;
}

static struct mpls_iptunnel_encap *
nh_encap_get_mpls(struct rtnl_nh_encap *nh_encap)
{
	return (struct mpls_iptunnel_encap *)nh_encap_check_and_get_priv(
		nh_encap, LWTUNNEL_ENCAP_MPLS);
}

struct nl_addr *rtnl_nh_get_encap_mpls_dst(struct rtnl_nh_encap *nh_encap)
{
	struct mpls_iptunnel_encap *mpls_encap;

	mpls_encap = nh_encap_get_mpls(nh_encap);
	if (!mpls_encap)
		return NULL;

	return mpls_encap->dst;
}

struct nl_addr *rtnl_route_nh_get_encap_mpls_dst(struct rtnl_nexthop *nh)
{
	return rtnl_nh_get_encap_mpls_dst(nh->rtnh_encap);
}

int rtnl_nh_get_encap_mpls_ttl(struct rtnl_nh_encap *nh_encap)
{
	struct mpls_iptunnel_encap *mpls_encap;

	mpls_encap = nh_encap_get_mpls(nh_encap);
	if (!mpls_encap)
		return -NLE_INVAL;

	return mpls_encap->ttl;
}

uint8_t rtnl_route_nh_get_encap_mpls_ttl(struct rtnl_nexthop *nh)
{
	int ttl = rtnl_nh_get_encap_mpls_ttl(nh->rtnh_encap);

	if (ttl < 0)
		return 0;
	return (uint8_t)ttl;
}
