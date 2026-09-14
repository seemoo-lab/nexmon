/* SPDX-License-Identifier: LGPL-2.1-only */

#include "nl-default.h"

#include <linux/lwtunnel.h>
#include <linux-private/linux/if_tunnel.h>

#include <netlink/route/nexthop.h>

#include "nexthop-encap.h"
#include "nl-aux-core/nl-core.h"
#include "nl-aux-route/nl-route.h"
#include "nl-route.h"

struct ip_tunnel_encap {
	struct nl_addr *dst;
	struct nl_addr *src;
	uint64_t id;
	uint16_t flags;
	uint8_t tos;
	uint8_t ttl;
};

static void ip_encap_dump(void *priv, struct nl_dump_params *dp)
{
	struct ip_tunnel_encap *encap_info = priv;
	char buf[256];

	nl_dump(dp, "%s ", nl_addr2str(encap_info->dst, buf, sizeof(buf)));

	if (encap_info->src)
		nl_dump(dp, "src %s ",
			nl_addr2str(encap_info->src, buf, sizeof(buf)));

	if (encap_info->id)
		nl_dump(dp, "id %llu ", (unsigned long long)encap_info->id);

	if (encap_info->ttl)
		nl_dump(dp, "ttl %u ", encap_info->ttl);

	if (encap_info->tos)
		nl_dump(dp, "tos %u ", encap_info->tos);

	if (encap_info->flags) {
		if (encap_info->flags & TUNNEL_KEY)
			nl_dump(dp, "key ");
		if (encap_info->flags & TUNNEL_CSUM)
			nl_dump(dp, "csum ");
		if (encap_info->flags & TUNNEL_SEQ)
			nl_dump(dp, "seq ");
	}
}

static int ip_encap_build_msg(struct nl_msg *msg, void *priv)
{
	struct ip_tunnel_encap *encap_info = priv;

	NLA_PUT_ADDR(msg, LWTUNNEL_IP_DST, encap_info->dst);
	if (encap_info->src)
		NLA_PUT_ADDR(msg, LWTUNNEL_IP_SRC, encap_info->src);
	if (encap_info->id)
		NLA_PUT_U64(msg, LWTUNNEL_IP_ID, htonll(encap_info->id));
	if (encap_info->ttl)
		NLA_PUT_U8(msg, LWTUNNEL_IP_TTL, encap_info->ttl);
	if (encap_info->tos)
		NLA_PUT_U8(msg, LWTUNNEL_IP_TOS, encap_info->tos);
	if (encap_info->flags)
		NLA_PUT_U16(msg, LWTUNNEL_IP_FLAGS, encap_info->flags);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static void ip_encap_destructor(void *priv)
{
	struct ip_tunnel_encap *encap_info = priv;

	nl_addr_put(encap_info->dst);
	nl_addr_put(encap_info->src);
}

static void *ip_encap_clone(void *priv)
{
	struct ip_tunnel_encap *src = priv;
	struct ip_tunnel_encap *clone;

	if (!src)
		return NULL;

	clone = calloc(1, sizeof(*clone));
	if (!clone)
		return NULL;

	clone->dst = nl_addr_get(src->dst);
	if (src->src)
		clone->src = nl_addr_get(src->src);
	clone->id = src->id;
	clone->flags = src->flags;
	clone->tos = src->tos;
	clone->ttl = src->ttl;

	return clone;
}

static struct nla_policy ip_encap_policy[LWTUNNEL_IP_MAX + 1] = {
	[LWTUNNEL_IP_DST] = { .type = NLA_BINARY, .minlen = 4, .maxlen = 4 },
	[LWTUNNEL_IP_SRC] = { .type = NLA_BINARY, .minlen = 4, .maxlen = 4 },
	[LWTUNNEL_IP_ID] = { .type = NLA_U64 },
	[LWTUNNEL_IP_TTL] = { .type = NLA_U8 },
	[LWTUNNEL_IP_TOS] = { .type = NLA_U8 },
	[LWTUNNEL_IP_FLAGS] = { .type = NLA_U16 },
};

static int ip_encap_parse_msg(struct nlattr *nla,
			      struct rtnl_nh_encap **encap_out)
{
	_nl_auto_rtnl_nh_encap struct rtnl_nh_encap *nh_encap = NULL;
	_nl_auto_nl_addr struct nl_addr *dst = NULL;
	_nl_auto_nl_addr struct nl_addr *src = NULL;
	struct nlattr *tb[LWTUNNEL_IP_MAX + 1];
	uint16_t flags = 0;
	uint64_t id = 0;
	uint8_t ttl = 0;
	uint8_t tos = 0;
	int err;

	err = nla_parse_nested(tb, LWTUNNEL_IP_MAX, nla, ip_encap_policy);
	if (err < 0)
		return err;

	if (!tb[LWTUNNEL_IP_DST])
		return -NLE_INVAL;

	dst = nl_addr_alloc_attr(tb[LWTUNNEL_IP_DST], AF_INET);
	if (!dst)
		return -NLE_NOMEM;

	if (tb[LWTUNNEL_IP_SRC]) {
		src = nl_addr_alloc_attr(tb[LWTUNNEL_IP_SRC], AF_INET);
		if (!src)
			return -NLE_NOMEM;
	}

	if (tb[LWTUNNEL_IP_ID])
		id = ntohll(nla_get_u64(tb[LWTUNNEL_IP_ID]));
	if (tb[LWTUNNEL_IP_TTL])
		ttl = nla_get_u8(tb[LWTUNNEL_IP_TTL]);
	if (tb[LWTUNNEL_IP_TOS])
		tos = nla_get_u8(tb[LWTUNNEL_IP_TOS]);
	if (tb[LWTUNNEL_IP_FLAGS])
		flags = nla_get_u16(tb[LWTUNNEL_IP_FLAGS]);

	nh_encap = rtnl_nh_encap_alloc();
	if (!nh_encap)
		return -NLE_NOMEM;

	err = rtnl_nh_encap_ip(nh_encap, dst);
	if (err < 0)
		return err;

	if (src) {
		err = rtnl_nh_set_encap_ip_src(nh_encap, src);
		if (err < 0)
			return err;
	}

	if (id) {
		err = rtnl_nh_set_encap_ip_id(nh_encap, id);
		if (err < 0)
			return err;
	}

	if (ttl) {
		err = rtnl_nh_set_encap_ip_ttl(nh_encap, ttl);
		if (err < 0)
			return err;
	}

	if (tos) {
		err = rtnl_nh_set_encap_ip_tos(nh_encap, tos);
		if (err < 0)
			return err;
	}

	if (tb[LWTUNNEL_IP_FLAGS]) {
		err = rtnl_nh_set_encap_ip_flags(nh_encap, flags);
		if (err < 0)
			return err;
	}

	*encap_out = _nl_steal_pointer(&nh_encap);

	return 0;
}

static int ip_encap_compare(void *_a, void *_b)
{
	struct ip_tunnel_encap *a = _a;
	struct ip_tunnel_encap *b = _b;
	int diff = 0;

	diff |= (a->id != b->id);
	diff |= (a->flags != b->flags);
	diff |= (a->tos != b->tos);
	diff |= (a->ttl != b->ttl);
	diff |= nl_addr_cmp(a->src, b->src);
	diff |= nl_addr_cmp(a->dst, b->dst);

	return diff;
}

const struct nh_encap_ops ip_encap_ops = {
	.encap_type = LWTUNNEL_ENCAP_IP,
	.build_msg = ip_encap_build_msg,
	.parse_msg = ip_encap_parse_msg,
	.compare = ip_encap_compare,
	.clone = ip_encap_clone,
	.dump = ip_encap_dump,
	.destructor = ip_encap_destructor,
};

int rtnl_nh_encap_ip(struct rtnl_nh_encap *nh_encap, struct nl_addr *dst)
{
	struct ip_tunnel_encap *ip_encap;

	if (!dst || !nh_encap)
		return -NLE_INVAL;
	if (nl_addr_get_family(dst) != AF_INET)
		return -NLE_INVAL;

	ip_encap = calloc(1, sizeof(*ip_encap));
	if (!ip_encap)
		return -NLE_NOMEM;

	ip_encap->dst = nl_addr_get(dst);

	nh_encap->priv = ip_encap;
	nh_encap->ops = &ip_encap_ops;

	return 0;
}

static struct ip_tunnel_encap *nh_encap_get_ip(struct rtnl_nh_encap *nh_encap)
{
	return (struct ip_tunnel_encap *)nh_encap_check_and_get_priv(
		nh_encap, LWTUNNEL_ENCAP_IP);
}

struct nl_addr *rtnl_nh_get_encap_ip_dst(struct rtnl_nh_encap *nh_encap)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return NULL;

	return ip_encap->dst;
}

int rtnl_nh_set_encap_ip_src(struct rtnl_nh_encap *nh_encap,
			     struct nl_addr *src)
{
	struct ip_tunnel_encap *ip_encap;
	struct nl_addr *old_src;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return -NLE_INVAL;
	if (src && nl_addr_get_family(src) != AF_INET)
		return -NLE_INVAL;

	old_src = ip_encap->src;
	if (src)
		ip_encap->src = nl_addr_get(src);
	else
		ip_encap->src = NULL;

	nl_addr_put(old_src);

	return 0;
}

struct nl_addr *rtnl_nh_get_encap_ip_src(struct rtnl_nh_encap *nh_encap)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return NULL;

	return ip_encap->src;
}

int rtnl_nh_set_encap_ip_ttl(struct rtnl_nh_encap *nh_encap, uint8_t ttl)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return -NLE_INVAL;

	ip_encap->ttl = ttl;
	return 0;
}

int rtnl_nh_get_encap_ip_ttl(struct rtnl_nh_encap *nh_encap)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return -NLE_INVAL;

	return ip_encap->ttl;
}

int rtnl_nh_set_encap_ip_tos(struct rtnl_nh_encap *nh_encap, uint8_t tos)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return -NLE_INVAL;

	ip_encap->tos = tos;
	return 0;
}

int rtnl_nh_get_encap_ip_tos(struct rtnl_nh_encap *nh_encap)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return -NLE_INVAL;

	return ip_encap->tos;
}

int rtnl_nh_set_encap_ip_id(struct rtnl_nh_encap *nh_encap, uint64_t id)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return -NLE_INVAL;

	ip_encap->id = id;
	return 0;
}

uint64_t rtnl_nh_get_encap_ip_id(struct rtnl_nh_encap *nh_encap)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return 0;

	return ip_encap->id;
}

int rtnl_nh_set_encap_ip_flags(struct rtnl_nh_encap *nh_encap, uint16_t flags)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return -NLE_INVAL;

	ip_encap->flags = flags;
	return 0;
}

int rtnl_nh_get_encap_ip_flags(struct rtnl_nh_encap *nh_encap)
{
	struct ip_tunnel_encap *ip_encap;

	ip_encap = nh_encap_get_ip(nh_encap);
	if (!ip_encap)
		return -NLE_INVAL;

	return ip_encap->flags;
}
