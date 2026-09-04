/* SPDX-License-Identifier: LGPL-2.1-only */

#include "nl-default.h"

#include <linux/ila.h>
#include <linux/lwtunnel.h>

#include <netlink/route/nexthop.h>

#include "nexthop-encap.h"
#include "nl-aux-route/nl-route.h"
#include "nl-route.h"

struct ila_tunnel_encap {
	uint64_t locator;
	uint8_t csum_mode;
	uint8_t ident_type;
	uint8_t hook_type;
	bool has_csum_mode : 1;
	bool has_ident_type : 1;
	bool has_hook_type : 1;
};

static void ila_encap_dump(void *priv, struct nl_dump_params *dp)
{
	struct ila_tunnel_encap *ila_encap = priv;

	nl_dump(dp, " locator 0x%llx ", (unsigned long long)ila_encap->locator);

	if (ila_encap->has_csum_mode)
		nl_dump(dp, " csum-mode %u ", ila_encap->csum_mode);

	if (ila_encap->has_ident_type)
		nl_dump(dp, " ident-type %u ", ila_encap->ident_type);

	if (ila_encap->has_hook_type)
		nl_dump(dp, " hook-type %u ", ila_encap->hook_type);
}

static int ila_encap_build_msg(struct nl_msg *msg, void *priv)
{
	struct ila_tunnel_encap *ila_encap = priv;

	NLA_PUT_U64(msg, ILA_ATTR_LOCATOR, ila_encap->locator);

	if (ila_encap->has_csum_mode)
		NLA_PUT_U8(msg, ILA_ATTR_CSUM_MODE, ila_encap->csum_mode);

	if (ila_encap->has_ident_type)
		NLA_PUT_U8(msg, ILA_ATTR_IDENT_TYPE, ila_encap->ident_type);

	if (ila_encap->has_hook_type)
		NLA_PUT_U8(msg, ILA_ATTR_HOOK_TYPE, ila_encap->hook_type);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static void *ila_encap_clone(void *priv)
{
	return priv ? _nl_memdup(priv, sizeof(struct ila_tunnel_encap)) : NULL;
}

static int ila_encap_compare(void *_a, void *_b)
{
	struct ila_tunnel_encap *a = _a;
	struct ila_tunnel_encap *b = _b;

	if (!a || !b)
		return a != b;

	if (a->locator != b->locator)
		return 1;

	if (a->has_csum_mode != b->has_csum_mode)
		return 1;

	if (a->has_csum_mode && a->csum_mode != b->csum_mode)
		return 1;

	if (a->has_ident_type != b->has_ident_type)
		return 1;

	if (a->has_ident_type && a->ident_type != b->ident_type)
		return 1;

	if (a->has_hook_type != b->has_hook_type)
		return 1;

	if (a->has_hook_type && a->hook_type != b->hook_type)
		return 1;

	return 0;
}

static struct nla_policy ila_encap_policy[ILA_ATTR_MAX + 1] = {
	[ILA_ATTR_LOCATOR] = { .type = NLA_U64 },
	[ILA_ATTR_CSUM_MODE] = { .type = NLA_U8 },
	[ILA_ATTR_IDENT_TYPE] = { .type = NLA_U8 },
	[ILA_ATTR_HOOK_TYPE] = { .type = NLA_U8 },
};

static int ila_encap_parse_msg(struct nlattr *nla,
			       struct rtnl_nh_encap **encap_out)
{
	_nl_auto_rtnl_nh_encap struct rtnl_nh_encap *nh_encap = NULL;
	struct nlattr *tb[ILA_ATTR_MAX + 1];
	uint64_t locator;
	int err;

	err = nla_parse_nested(tb, ILA_ATTR_MAX, nla, ila_encap_policy);
	if (err < 0)
		return err;

	if (!tb[ILA_ATTR_LOCATOR])
		return -NLE_INVAL;

	locator = nla_get_u64(tb[ILA_ATTR_LOCATOR]);

	nh_encap = rtnl_nh_encap_alloc();
	if (!nh_encap)
		return -NLE_NOMEM;

	err = rtnl_nh_encap_ila(nh_encap, locator);
	if (err < 0)
		return err;

	if (tb[ILA_ATTR_CSUM_MODE]) {
		err = rtnl_nh_set_encap_ila_csum_mode(
			nh_encap, nla_get_u8(tb[ILA_ATTR_CSUM_MODE]));
		if (err < 0)
			return err;
	}

	if (tb[ILA_ATTR_IDENT_TYPE]) {
		err = rtnl_nh_set_encap_ila_ident_type(
			nh_encap, nla_get_u8(tb[ILA_ATTR_IDENT_TYPE]));
		if (err < 0)
			return err;
	}

	if (tb[ILA_ATTR_HOOK_TYPE]) {
		err = rtnl_nh_set_encap_ila_hook_type(
			nh_encap, nla_get_u8(tb[ILA_ATTR_HOOK_TYPE]));
		if (err < 0)
			return err;
	}

	*encap_out = _nl_steal_pointer(&nh_encap);

	return 0;
}

const struct nh_encap_ops ila_encap_ops = {
	.encap_type = LWTUNNEL_ENCAP_ILA,
	.build_msg = ila_encap_build_msg,
	.parse_msg = ila_encap_parse_msg,
	.compare = ila_encap_compare,
	.clone = ila_encap_clone,
	.dump = ila_encap_dump,
};

static struct ila_tunnel_encap *nh_encap_get_ila(struct rtnl_nh_encap *nh_encap)
{
	return nh_encap_check_and_get_priv(nh_encap, LWTUNNEL_ENCAP_ILA);
}

int rtnl_nh_encap_ila(struct rtnl_nh_encap *nh_encap, uint64_t locator)
{
	struct ila_tunnel_encap *ila_encap;

	if (!nh_encap)
		return -NLE_INVAL;

	ila_encap = calloc(1, sizeof(*ila_encap));
	if (!ila_encap)
		return -NLE_NOMEM;

	ila_encap->locator = locator;

	nh_encap->priv = ila_encap;
	nh_encap->ops = &ila_encap_ops;

	return 0;
}

int rtnl_nh_get_encap_ila_locator(struct rtnl_nh_encap *nh_encap,
				  uint64_t *locator)
{
	struct ila_tunnel_encap *ila_encap;

	if (!locator)
		return -NLE_INVAL;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	*locator = ila_encap->locator;

	return 0;
}

int rtnl_nh_set_encap_ila_csum_mode(struct rtnl_nh_encap *nh_encap,
				    uint8_t csum_mode)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	ila_encap->csum_mode = csum_mode;
	ila_encap->has_csum_mode = true;

	return 0;
}

int rtnl_nh_clear_encap_ila_csum_mode(struct rtnl_nh_encap *nh_encap)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	ila_encap->has_csum_mode = false;
	ila_encap->csum_mode = 0;

	return 0;
}

int rtnl_nh_get_encap_ila_csum_mode(struct rtnl_nh_encap *nh_encap)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	if (!ila_encap->has_csum_mode)
		return -NLE_MISSING_ATTR;

	return ila_encap->csum_mode;
}

int rtnl_nh_set_encap_ila_ident_type(struct rtnl_nh_encap *nh_encap,
				     uint8_t ident_type)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	ila_encap->ident_type = ident_type;
	ila_encap->has_ident_type = true;

	return 0;
}

int rtnl_nh_clear_encap_ila_ident_type(struct rtnl_nh_encap *nh_encap)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	ila_encap->has_ident_type = false;
	ila_encap->ident_type = 0;

	return 0;
}

int rtnl_nh_get_encap_ila_ident_type(struct rtnl_nh_encap *nh_encap)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	if (!ila_encap->has_ident_type)
		return -NLE_MISSING_ATTR;

	return ila_encap->ident_type;
}

int rtnl_nh_set_encap_ila_hook_type(struct rtnl_nh_encap *nh_encap,
				    uint8_t hook_type)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	ila_encap->hook_type = hook_type;
	ila_encap->has_hook_type = true;

	return 0;
}

int rtnl_nh_clear_encap_ila_hook_type(struct rtnl_nh_encap *nh_encap)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	ila_encap->has_hook_type = false;
	ila_encap->hook_type = 0;

	return 0;
}

int rtnl_nh_get_encap_ila_hook_type(struct rtnl_nh_encap *nh_encap)
{
	struct ila_tunnel_encap *ila_encap;

	ila_encap = nh_encap_get_ila(nh_encap);
	if (!ila_encap)
		return -NLE_INVAL;

	if (!ila_encap->has_hook_type)
		return -NLE_MISSING_ATTR;

	return ila_encap->hook_type;
}
