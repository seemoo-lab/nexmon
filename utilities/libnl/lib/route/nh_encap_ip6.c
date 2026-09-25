/* SPDX-License-Identifier: LGPL-2.1-only */

#include "nl-default.h"

#include <linux/lwtunnel.h>
#include <linux-private/linux/if_tunnel.h>

#include <netlink/route/nexthop.h>

#include "nexthop-encap.h"
#include "nl-aux-core/nl-core.h"
#include "nl-aux-route/nl-route.h"
#include "nl-route.h"

struct ip6_tunnel_encap {
	struct nl_addr *dst;
	struct nl_addr *src;
	uint64_t id;
	uint8_t tc;
	uint8_t hoplimit;
	uint16_t flags;
};

static void ip6_encap_dump(void *priv, struct nl_dump_params *dp)
{
	struct ip6_tunnel_encap *encap_info = priv;
	char buf[256];

	if (encap_info->id)
		nl_dump(dp, "id %llu ", (unsigned long long)encap_info->id);

	nl_dump(dp, "%s ", nl_addr2str(encap_info->dst, buf, sizeof(buf)));

	if (encap_info->src)
		nl_dump(dp, "src %s ",
			nl_addr2str(encap_info->src, buf, sizeof(buf)));

	if (encap_info->tc)
		nl_dump(dp, "tc %u ", encap_info->tc);

	if (encap_info->hoplimit)
		nl_dump(dp, "hoplimit %u ", encap_info->hoplimit);

	if (encap_info->flags) {
		if (encap_info->flags & TUNNEL_KEY)
			nl_dump(dp, "key ");
		if (encap_info->flags & TUNNEL_CSUM)
			nl_dump(dp, "csum ");
		if (encap_info->flags & TUNNEL_SEQ)
			nl_dump(dp, "seq ");
	}
}

static int ip6_encap_build_msg(struct nl_msg *msg, void *priv)
{
	struct ip6_tunnel_encap *encap_info = priv;

	if (encap_info->id)
		NLA_PUT_U64(msg, LWTUNNEL_IP6_ID, htonll(encap_info->id));
	NLA_PUT_ADDR(msg, LWTUNNEL_IP6_DST, encap_info->dst);
	if (encap_info->src)
		NLA_PUT_ADDR(msg, LWTUNNEL_IP6_SRC, encap_info->src);
	if (encap_info->tc)
		NLA_PUT_U8(msg, LWTUNNEL_IP6_TC, encap_info->tc);
	if (encap_info->hoplimit)
		NLA_PUT_U8(msg, LWTUNNEL_IP6_HOPLIMIT, encap_info->hoplimit);
	if (encap_info->flags)
		NLA_PUT_U16(msg, LWTUNNEL_IP6_FLAGS, encap_info->flags);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static void ip6_encap_destructor(void *priv)
{
	struct ip6_tunnel_encap *encap_info = priv;

	nl_addr_put(encap_info->dst);
	nl_addr_put(encap_info->src);
}

static void *ip6_encap_clone(void *priv)
{
	struct ip6_tunnel_encap *src = priv;
	struct ip6_tunnel_encap *clone;

	if (!src)
		return NULL;

	clone = calloc(1, sizeof(*clone));
	if (!clone)
		return NULL;

	clone->dst = nl_addr_get(src->dst);
	if (src->src)
		clone->src = nl_addr_get(src->src);
	clone->id = src->id;
	clone->tc = src->tc;
	clone->hoplimit = src->hoplimit;
	clone->flags = src->flags;

	return clone;
}

static struct nla_policy ip6_encap_policy[LWTUNNEL_IP6_MAX + 1] = {
	[LWTUNNEL_IP6_ID] = { .type = NLA_U64 },
	[LWTUNNEL_IP6_DST] = { .type = NLA_BINARY, .minlen = 16, .maxlen = 16 },
	[LWTUNNEL_IP6_SRC] = { .type = NLA_BINARY, .minlen = 16, .maxlen = 16 },
	[LWTUNNEL_IP6_HOPLIMIT] = { .type = NLA_U8 },
	[LWTUNNEL_IP6_TC] = { .type = NLA_U8 },
	[LWTUNNEL_IP6_FLAGS] = { .type = NLA_U16 },
};

static int ip6_encap_parse_msg(struct nlattr *nla,
			       struct rtnl_nh_encap **encap_out)
{
	_nl_auto_rtnl_nh_encap struct rtnl_nh_encap *nh_encap = NULL;
	_nl_auto_nl_addr struct nl_addr *dst = NULL;
	_nl_auto_nl_addr struct nl_addr *src = NULL;
	struct nlattr *tb[LWTUNNEL_IP6_MAX + 1];
	uint16_t flags = 0;
	uint64_t id = 0;
	uint8_t hoplimit = 0;
	uint8_t tc = 0;
	int err;

	err = nla_parse_nested(tb, LWTUNNEL_IP6_MAX, nla, ip6_encap_policy);
	if (err < 0)
		return err;

	if (!tb[LWTUNNEL_IP6_DST])
		return -NLE_INVAL;

	dst = nl_addr_alloc_attr(tb[LWTUNNEL_IP6_DST], AF_INET6);
	if (!dst)
		return -NLE_NOMEM;

	if (tb[LWTUNNEL_IP6_SRC]) {
		src = nl_addr_alloc_attr(tb[LWTUNNEL_IP6_SRC], AF_INET6);
		if (!src)
			return -NLE_NOMEM;
	}

	if (tb[LWTUNNEL_IP6_HOPLIMIT])
		hoplimit = nla_get_u8(tb[LWTUNNEL_IP6_HOPLIMIT]);

	if (tb[LWTUNNEL_IP6_TC])
		tc = nla_get_u8(tb[LWTUNNEL_IP6_TC]);

	if (tb[LWTUNNEL_IP6_ID])
		id = ntohll(nla_get_u64(tb[LWTUNNEL_IP6_ID]));

	if (tb[LWTUNNEL_IP6_FLAGS])
		flags = nla_get_u16(tb[LWTUNNEL_IP6_FLAGS]);

	nh_encap = rtnl_nh_encap_alloc();
	if (!nh_encap)
		return -NLE_NOMEM;

	err = rtnl_nh_encap_ip6(nh_encap, dst);
	if (err < 0)
		return err;

	if (src) {
		err = rtnl_nh_set_encap_ip6_src(nh_encap, src);
		if (err < 0)
			return err;
	}

	if (id) {
		err = rtnl_nh_set_encap_ip6_id(nh_encap, id);
		if (err < 0)
			return err;
	}

	if (hoplimit) {
		err = rtnl_nh_set_encap_ip6_hoplimit(nh_encap, hoplimit);
		if (err < 0)
			return err;
	}

	if (tc) {
		err = rtnl_nh_set_encap_ip6_tc(nh_encap, tc);
		if (err < 0)
			return err;
	}

	if (flags) {
		err = rtnl_nh_set_encap_ip6_flags(nh_encap, flags);
		if (err < 0)
			return err;
	}

	*encap_out = _nl_steal_pointer(&nh_encap);

	return 0;
}

static int ip6_encap_compare(void *_a, void *_b)
{
	struct ip6_tunnel_encap *a = _a;
	struct ip6_tunnel_encap *b = _b;
	int diff = 0;

	diff |= (a->tc != b->tc);
	diff |= (a->hoplimit != b->hoplimit);
	diff |= (a->flags != b->flags);
	diff |= (a->id != b->id);
	diff |= nl_addr_cmp(a->src, b->src);
	diff |= nl_addr_cmp(a->dst, b->dst);

	return diff;
}

const struct nh_encap_ops ip6_encap_ops = {
	.encap_type = LWTUNNEL_ENCAP_IP6,
	.build_msg = ip6_encap_build_msg,
	.parse_msg = ip6_encap_parse_msg,
	.compare = ip6_encap_compare,
	.clone = ip6_encap_clone,
	.dump = ip6_encap_dump,
	.destructor = ip6_encap_destructor,
};

int rtnl_nh_encap_ip6(struct rtnl_nh_encap *nh_encap, struct nl_addr *dst)
{
	struct ip6_tunnel_encap *ip6_encap;

	if (!dst || !nh_encap)
		return -NLE_INVAL;
	if (nl_addr_get_family(dst) != AF_INET6)
		return -NLE_INVAL;

	ip6_encap = calloc(1, sizeof(*ip6_encap));
	if (!ip6_encap)
		return -NLE_NOMEM;

	ip6_encap->dst = nl_addr_get(dst);

	nh_encap->priv = ip6_encap;
	nh_encap->ops = &ip6_encap_ops;

	return 0;
}

static struct ip6_tunnel_encap *nh_encap_get_ip6(struct rtnl_nh_encap *nh_encap)
{
	return (struct ip6_tunnel_encap *)nh_encap_check_and_get_priv(
		nh_encap, LWTUNNEL_ENCAP_IP6);
}

struct nl_addr *rtnl_nh_get_encap_ip6_dst(struct rtnl_nh_encap *nh_encap)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return NULL;

	return ip6_encap->dst;
}

int rtnl_nh_set_encap_ip6_src(struct rtnl_nh_encap *nh_encap,
			      struct nl_addr *src)
{
	struct ip6_tunnel_encap *ip6_encap;
	struct nl_addr *old_src;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return -NLE_INVAL;
	if (src && nl_addr_get_family(src) != AF_INET6)
		return -NLE_INVAL;

	old_src = ip6_encap->src;
	if (src)
		ip6_encap->src = nl_addr_get(src);
	else
		ip6_encap->src = NULL;

	nl_addr_put(old_src);

	return 0;
}

struct nl_addr *rtnl_nh_get_encap_ip6_src(struct rtnl_nh_encap *nh_encap)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return NULL;

	return ip6_encap->src;
}

int rtnl_nh_set_encap_ip6_tc(struct rtnl_nh_encap *nh_encap, uint8_t tc)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return -NLE_INVAL;

	ip6_encap->tc = tc;
	return 0;
}

int rtnl_nh_get_encap_ip6_tc(struct rtnl_nh_encap *nh_encap)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return -NLE_INVAL;

	return ip6_encap->tc;
}

int rtnl_nh_set_encap_ip6_hoplimit(struct rtnl_nh_encap *nh_encap,
				   uint8_t hoplimit)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return -NLE_INVAL;

	ip6_encap->hoplimit = hoplimit;
	return 0;
}

int rtnl_nh_get_encap_ip6_hoplimit(struct rtnl_nh_encap *nh_encap)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return -NLE_INVAL;

	return ip6_encap->hoplimit;
}

int rtnl_nh_set_encap_ip6_flags(struct rtnl_nh_encap *nh_encap, uint16_t flags)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return -NLE_INVAL;

	ip6_encap->flags = flags;
	return 0;
}

int rtnl_nh_get_encap_ip6_flags(struct rtnl_nh_encap *nh_encap)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return -NLE_INVAL;

	return ip6_encap->flags;
}

int rtnl_nh_set_encap_ip6_id(struct rtnl_nh_encap *nh_encap, uint64_t id)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return -NLE_INVAL;

	ip6_encap->id = id;

	return 0;
}

uint64_t rtnl_nh_get_encap_ip6_id(struct rtnl_nh_encap *nh_encap)
{
	struct ip6_tunnel_encap *ip6_encap;

	ip6_encap = nh_encap_get_ip6(nh_encap);
	if (!ip6_encap)
		return 0;

	return ip6_encap->id;
}
