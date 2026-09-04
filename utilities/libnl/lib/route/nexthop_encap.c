/* SPDX-License-Identifier: LGPL-2.1-only */

#include "nl-default.h"

#include <linux/lwtunnel.h>

#include "nl-route.h"
#include "nexthop-encap.h"

static const struct lwtunnel_encap_type {
	const char *name;
	const struct nh_encap_ops *ops;
} lwtunnel_encap_types[__LWTUNNEL_ENCAP_MAX] = {
	[LWTUNNEL_ENCAP_NONE] = { .name = "none" },
	[LWTUNNEL_ENCAP_MPLS] = { .name = "mpls", .ops = &mpls_encap_ops },
	[LWTUNNEL_ENCAP_IP] = { .name = "ip", .ops = &ip_encap_ops },
	[LWTUNNEL_ENCAP_IP6] = { .name = "ip6", .ops = &ip6_encap_ops },
	[LWTUNNEL_ENCAP_ILA] = { .name = "ila", .ops = &ila_encap_ops },
	[LWTUNNEL_ENCAP_BPF] = { .name = "bpf" },
};

static const char *nh_encap_type2str(unsigned int type)
{
	const char *name;

	if (type > LWTUNNEL_ENCAP_MAX)
		return "unknown";

	name = lwtunnel_encap_types[type].name;

	return name ? name : "unknown";
}

void nh_encap_dump(struct rtnl_nh_encap *rtnh_encap, struct nl_dump_params *dp)
{
	if (!rtnh_encap->ops)
		return;

	nl_dump(dp, " encap %s ",
		nh_encap_type2str(rtnh_encap->ops->encap_type));

	if (rtnh_encap->ops->dump)
		rtnh_encap->ops->dump(rtnh_encap->priv, dp);
}

int nh_encap_build_msg(struct nl_msg *msg, struct rtnl_nh_encap *rtnh_encap)
{
	struct nlattr *encap;
	int err;

	if (!rtnh_encap->ops || !rtnh_encap->ops->build_msg) {
		NL_DBG(2, "Nexthop encap type not implemented\n");
		return -NLE_INVAL;
	}

	NLA_PUT_U16(msg, RTA_ENCAP_TYPE, rtnh_encap->ops->encap_type);

	encap = nla_nest_start(msg, RTA_ENCAP);
	if (!encap)
		goto nla_put_failure;

	err = rtnh_encap->ops->build_msg(msg, rtnh_encap->priv);
	if (err < 0)
		return err;

	nla_nest_end(msg, encap);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

int nh_encap_parse_msg(struct nlattr *encap, struct nlattr *encap_type,
		       struct rtnl_nh_encap **encap_out)
{
	uint16_t e_type = nla_get_u16(encap_type);

	if (e_type == LWTUNNEL_ENCAP_NONE) {
		NL_DBG(2, "RTA_ENCAP_TYPE should not be LWTUNNEL_ENCAP_NONE\n");

		goto unsupported_encap;
	}

	if (e_type > LWTUNNEL_ENCAP_MAX) {
		NL_DBG(2, "Unknown RTA_ENCAP_TYPE: %d\n", e_type);

		goto unsupported_encap;
	}

	if (!lwtunnel_encap_types[e_type].ops) {
		NL_DBG(2, "RTA_ENCAP_TYPE %s is not implemented\n",
		       lwtunnel_encap_types[e_type].name);

		goto unsupported_encap;
	}

	return lwtunnel_encap_types[e_type].ops->parse_msg(encap, encap_out);

unsupported_encap:
	/* If we don't yet support this lwtunnel, just return 0.
	 *
	 * Force encap_out to NULL so that subsequent calls to set
	 * it on a nexthop/route will simply reset the encapsulation
	 * on that nexthop/route.
	 */
	*encap_out = NULL;
	return 0;
}

int nh_encap_compare(struct rtnl_nh_encap *a, struct rtnl_nh_encap *b)
{
	if (!a && !b)
		return 0;

	if ((a && !b) || (!a && b) || (a->ops != b->ops))
		return 1;

	if (!a->ops || !a->ops->compare)
		return 0;

	return a->ops->compare(a->priv, b->priv);
}

void *nh_encap_check_and_get_priv(struct rtnl_nh_encap *nh_encap,
				  uint16_t encap_type)
{
	if (!nh_encap || !nh_encap->ops ||
	    nh_encap->ops->encap_type != encap_type) {
		return NULL;
	}

	return nh_encap->priv;
}
