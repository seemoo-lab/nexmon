/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Yasunobu Chiba <yasu@dsl.gr.jp>
 */

/**
 * @ingroup link
 * @defgroup vxlan VXLAN
 * Virtual eXtensible Local Area Network link module
 *
 * @details
 * \b Link Type Name: "vxlan"
 *
 * @route_doc{link_vxlan, VXLAN Documentation}
 *
 * @{
 */

#include "nl-default.h"

#include <linux/if_link.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/vxlan.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-aux-route/nl-route.h"

/** @cond SKIP */
#define VXLAN_ATTR_ID                  (1<<0)
#define VXLAN_ATTR_GROUP               (1<<1)
#define VXLAN_ATTR_LINK                (1<<2)
#define VXLAN_ATTR_LOCAL               (1<<3)
#define VXLAN_ATTR_TTL                 (1<<4)
#define VXLAN_ATTR_TOS                 (1<<5)
#define VXLAN_ATTR_LEARNING            (1<<6)
#define VXLAN_ATTR_AGEING              (1<<7)
#define VXLAN_ATTR_LIMIT               (1<<8)
#define VXLAN_ATTR_PORT_RANGE          (1<<9)
#define VXLAN_ATTR_PROXY               (1<<10)
#define VXLAN_ATTR_RSC                 (1<<11)
#define VXLAN_ATTR_L2MISS              (1<<12)
#define VXLAN_ATTR_L3MISS              (1<<13)
#define VXLAN_ATTR_GROUP6              (1<<14)
#define VXLAN_ATTR_LOCAL6              (1<<15)
#define VXLAN_ATTR_PORT                (1<<16)
#define VXLAN_ATTR_UDP_CSUM            (1<<17)
#define VXLAN_ATTR_UDP_ZERO_CSUM6_TX   (1<<18)
#define VXLAN_ATTR_UDP_ZERO_CSUM6_RX   (1<<19)
#define VXLAN_ATTR_REMCSUM_TX          (1<<20)
#define VXLAN_ATTR_REMCSUM_RX          (1<<21)
#define VXLAN_ATTR_COLLECT_METADATA    (1<<22)
#define VXLAN_ATTR_LABEL               (1<<23)
#define VXLAN_ATTR_FLAGS               (1<<24)

struct vxlan_info
{
	uint32_t		vxi_id;
	uint32_t		vxi_group;
	struct in6_addr		vxi_group6;
	uint32_t		vxi_link;
	uint32_t		vxi_local;
	struct in6_addr		vxi_local6;
	uint8_t			vxi_ttl;
	uint8_t			vxi_tos;
	uint8_t			vxi_learning;
	uint8_t			vxi_flags;
	uint32_t		vxi_ageing;
	uint32_t		vxi_limit;
	struct ifla_vxlan_port_range	vxi_port_range;
	uint8_t			vxi_proxy;
	uint8_t			vxi_rsc;
	uint8_t			vxi_l2miss;
	uint8_t			vxi_l3miss;
	uint16_t		vxi_port;
	uint8_t			vxi_udp_csum;
	uint8_t			vxi_udp_zero_csum6_tx;
	uint8_t			vxi_udp_zero_csum6_rx;
	uint8_t			vxi_remcsum_tx;
	uint8_t			vxi_remcsum_rx;
	uint8_t			vxi_collect_metadata;
	uint32_t		vxi_label;
	uint32_t		ce_mask;
};

/** @endcond */

static struct nla_policy vxlan_policy[IFLA_VXLAN_MAX+1] = {
	[IFLA_VXLAN_ID] = { .type = NLA_U32 },
	[IFLA_VXLAN_GROUP] = { .minlen = sizeof(uint32_t) },
	[IFLA_VXLAN_GROUP6] = { .minlen = sizeof(struct in6_addr) },
	[IFLA_VXLAN_LINK] = { .type = NLA_U32 },
	[IFLA_VXLAN_LOCAL] = { .minlen = sizeof(uint32_t) },
	[IFLA_VXLAN_LOCAL6] = { .minlen = sizeof(struct in6_addr) },
	[IFLA_VXLAN_TTL] = { .type = NLA_U8 },
	[IFLA_VXLAN_TOS] = { .type = NLA_U8 },
	[IFLA_VXLAN_LABEL] = { .type = NLA_U32 },
	[IFLA_VXLAN_LEARNING] = { .type = NLA_U8 },
	[IFLA_VXLAN_AGEING] = { .type = NLA_U32 },
	[IFLA_VXLAN_LIMIT] = { .type = NLA_U32 },
	[IFLA_VXLAN_PORT_RANGE] = { .minlen = sizeof(struct ifla_vxlan_port_range) },
	[IFLA_VXLAN_PROXY] = { .type = NLA_U8 },
	[IFLA_VXLAN_RSC] = { .type = NLA_U8 },
	[IFLA_VXLAN_L2MISS] = { .type = NLA_U8 },
	[IFLA_VXLAN_L3MISS] = { .type = NLA_U8 },
	[IFLA_VXLAN_COLLECT_METADATA] = { .type = NLA_U8 },
	[IFLA_VXLAN_PORT] = { .type = NLA_U16 },
	[IFLA_VXLAN_UDP_CSUM] = { .type = NLA_U8 },
	[IFLA_VXLAN_UDP_ZERO_CSUM6_TX] = { .type = NLA_U8 },
	[IFLA_VXLAN_UDP_ZERO_CSUM6_RX] = { .type = NLA_U8 },
	[IFLA_VXLAN_REMCSUM_TX] = { .type = NLA_U8 },
	[IFLA_VXLAN_REMCSUM_RX] = { .type = NLA_U8 },
	[IFLA_VXLAN_GBP] = { .type = NLA_FLAG, },
	[IFLA_VXLAN_GPE] = { .type = NLA_FLAG, },
	[IFLA_VXLAN_REMCSUM_NOPARTIAL] = { .type = NLA_FLAG },
};

static int vxlan_alloc(struct rtnl_link *link)
{
	struct vxlan_info *vxi;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*vxi));
	else {
		if ((vxi = calloc(1, sizeof(*vxi))) == NULL)
			return -NLE_NOMEM;

		link->l_info = vxi;
	}

	return 0;
}

static int vxlan_parse(struct rtnl_link *link, struct nlattr *data,
		      struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_VXLAN_MAX+1];
	struct vxlan_info *vxi;
	int err;

	NL_DBG(3, "Parsing VXLAN link info\n");

	if ((err = nla_parse_nested(tb, IFLA_VXLAN_MAX, data, vxlan_policy)) < 0)
		goto errout;

	if ((err = vxlan_alloc(link)) < 0)
		goto errout;

	vxi = link->l_info;

	if (tb[IFLA_VXLAN_ID]) {
		vxi->vxi_id = nla_get_u32(tb[IFLA_VXLAN_ID]);
		vxi->ce_mask |= VXLAN_ATTR_ID;
	}

	if (tb[IFLA_VXLAN_GROUP6]) {
		nla_memcpy(&vxi->vxi_group6, tb[IFLA_VXLAN_GROUP6],
			   sizeof(vxi->vxi_group6));
		vxi->ce_mask |= VXLAN_ATTR_GROUP6;
	}

	if (tb[IFLA_VXLAN_GROUP]) {
		nla_memcpy(&vxi->vxi_group, tb[IFLA_VXLAN_GROUP],
				   sizeof(vxi->vxi_group));
		vxi->ce_mask |= VXLAN_ATTR_GROUP;
		vxi->ce_mask &= ~VXLAN_ATTR_GROUP6;
	}

	if (tb[IFLA_VXLAN_LINK]) {
		vxi->vxi_link = nla_get_u32(tb[IFLA_VXLAN_LINK]);
		vxi->ce_mask |= VXLAN_ATTR_LINK;
	}

	if (tb[IFLA_VXLAN_LOCAL6]) {
		nla_memcpy(&vxi->vxi_local6, tb[IFLA_VXLAN_LOCAL6],
			   sizeof(vxi->vxi_local6));
		vxi->ce_mask |= VXLAN_ATTR_LOCAL6;
	}

	if (tb[IFLA_VXLAN_LOCAL]) {
		nla_memcpy(&vxi->vxi_local, tb[IFLA_VXLAN_LOCAL],
				   sizeof(vxi->vxi_local));
		vxi->ce_mask |= VXLAN_ATTR_LOCAL;
		vxi->ce_mask &= ~VXLAN_ATTR_LOCAL6;
	}

	if (tb[IFLA_VXLAN_TTL]) {
		vxi->vxi_ttl = nla_get_u8(tb[IFLA_VXLAN_TTL]);
		vxi->ce_mask |= VXLAN_ATTR_TTL;
	}

	if (tb[IFLA_VXLAN_TOS]) {
		vxi->vxi_tos = nla_get_u8(tb[IFLA_VXLAN_TOS]);
		vxi->ce_mask |= VXLAN_ATTR_TOS;
	}

	if (tb[IFLA_VXLAN_LEARNING]) {
		vxi->vxi_learning = nla_get_u8(tb[IFLA_VXLAN_LEARNING]);
		vxi->ce_mask |= VXLAN_ATTR_LEARNING;
	}

	if (tb[IFLA_VXLAN_AGEING]) {
		vxi->vxi_ageing = nla_get_u32(tb[IFLA_VXLAN_AGEING]);
		vxi->ce_mask |= VXLAN_ATTR_AGEING;
	}

	if (tb[IFLA_VXLAN_LIMIT]) {
		vxi->vxi_limit = nla_get_u32(tb[IFLA_VXLAN_LIMIT]);
		vxi->ce_mask |= VXLAN_ATTR_LIMIT;
	}

	if (tb[IFLA_VXLAN_PORT_RANGE]) {
		nla_memcpy(&vxi->vxi_port_range, tb[IFLA_VXLAN_PORT_RANGE],
				   sizeof(vxi->vxi_port_range));
		vxi->ce_mask |= VXLAN_ATTR_PORT_RANGE;
	}

	if (tb[IFLA_VXLAN_PROXY]) {
		vxi->vxi_proxy = nla_get_u8(tb[IFLA_VXLAN_PROXY]);
		vxi->ce_mask |= VXLAN_ATTR_PROXY;
	}

	if (tb[IFLA_VXLAN_RSC]) {
		vxi->vxi_rsc = nla_get_u8(tb[IFLA_VXLAN_RSC]);
		vxi->ce_mask |= VXLAN_ATTR_RSC;
	}

	if (tb[IFLA_VXLAN_L2MISS]) {
		vxi->vxi_l2miss = nla_get_u8(tb[IFLA_VXLAN_L2MISS]);
		vxi->ce_mask |= VXLAN_ATTR_L2MISS;
	}

	if (tb[IFLA_VXLAN_L3MISS]) {
		vxi->vxi_l3miss = nla_get_u8(tb[IFLA_VXLAN_L3MISS]);
		vxi->ce_mask |= VXLAN_ATTR_L3MISS;
	}

	if (tb[IFLA_VXLAN_PORT]) {
		vxi->vxi_port = nla_get_u16(tb[IFLA_VXLAN_PORT]);
		vxi->ce_mask |= VXLAN_ATTR_PORT;
	}

	if (tb[IFLA_VXLAN_UDP_CSUM]) {
		vxi->vxi_udp_csum = nla_get_u8(tb[IFLA_VXLAN_UDP_CSUM]);
		vxi->ce_mask |= VXLAN_ATTR_UDP_CSUM;
	}

	if (tb[IFLA_VXLAN_UDP_ZERO_CSUM6_TX]) {
		vxi->vxi_udp_zero_csum6_tx = nla_get_u8(tb[IFLA_VXLAN_UDP_ZERO_CSUM6_TX]);
		vxi->ce_mask |= VXLAN_ATTR_UDP_ZERO_CSUM6_TX;
	}

	if (tb[IFLA_VXLAN_UDP_ZERO_CSUM6_RX]) {
		vxi->vxi_udp_zero_csum6_rx = nla_get_u8(tb[IFLA_VXLAN_UDP_ZERO_CSUM6_RX]);
		vxi->ce_mask |= VXLAN_ATTR_UDP_ZERO_CSUM6_RX;
	}

	if (tb[IFLA_VXLAN_REMCSUM_TX]) {
		vxi->vxi_remcsum_tx = nla_get_u8(tb[IFLA_VXLAN_REMCSUM_TX]);
		vxi->ce_mask |= VXLAN_ATTR_REMCSUM_TX;
	}

	if (tb[IFLA_VXLAN_REMCSUM_RX]) {
		vxi->vxi_remcsum_rx = nla_get_u8(tb[IFLA_VXLAN_REMCSUM_RX]);
		vxi->ce_mask |= VXLAN_ATTR_REMCSUM_RX;
	}

	if (tb[IFLA_VXLAN_GBP])
		vxi->vxi_flags |= RTNL_LINK_VXLAN_F_GBP;

	if (tb[IFLA_VXLAN_REMCSUM_NOPARTIAL])
		vxi->vxi_flags |= RTNL_LINK_VXLAN_F_REMCSUM_NOPARTIAL;

	if (tb[IFLA_VXLAN_COLLECT_METADATA]) {
		vxi->vxi_collect_metadata = nla_get_u8(tb[IFLA_VXLAN_COLLECT_METADATA]);
		vxi->ce_mask |= VXLAN_ATTR_COLLECT_METADATA;
	}

	if (tb[IFLA_VXLAN_LABEL]) {
		vxi->vxi_label = nla_get_u32(tb[IFLA_VXLAN_LABEL]);
		vxi->ce_mask |= VXLAN_ATTR_LABEL;
	}

	if (tb[IFLA_VXLAN_GPE])
		vxi->vxi_flags |= RTNL_LINK_VXLAN_F_GPE;

	err = 0;

errout:
	return err;
}

static void vxlan_free(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	free(vxi);
	link->l_info = NULL;
}

static void vxlan_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct vxlan_info *vxi = link->l_info;

	nl_dump(p, "vxlan-id %u", vxi->vxi_id);
}

static void vxlan_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct vxlan_info *vxi = link->l_info;
	char addr[INET6_ADDRSTRLEN];

	nl_dump_line(p, "    vxlan-id %u\n", vxi->vxi_id);

	if (vxi->ce_mask & VXLAN_ATTR_GROUP) {
		nl_dump(p, "      group ");
		nl_dump_line(p, "%s\n",
			     _nl_inet_ntop(AF_INET, &vxi->vxi_group, addr));
	} else if (vxi->ce_mask & VXLAN_ATTR_GROUP6) {
		nl_dump(p, "      group ");
		nl_dump_line(p, "%s\n",
			     _nl_inet_ntop(AF_INET6, &vxi->vxi_group6, addr));
	}

	if (vxi->ce_mask & VXLAN_ATTR_LINK) {
		_nl_auto_rtnl_link struct rtnl_link *parent = NULL;
		char *name;

		nl_dump(p, "      link ");

		name = NULL;
		parent = link_lookup(link->ce_cache, vxi->vxi_link);
		if (parent)
			name = rtnl_link_get_name(parent);

		if (name)
			nl_dump_line(p, "%s\n", name);
		else
			nl_dump_line(p, "%u\n", vxi->vxi_link);
	}

	if (vxi->ce_mask & VXLAN_ATTR_LOCAL) {
		nl_dump(p, "      local ");
		nl_dump_line(p, "%s\n",
			     _nl_inet_ntop(AF_INET, &vxi->vxi_local, addr));
	} else if (vxi->ce_mask & VXLAN_ATTR_LOCAL6) {
		nl_dump(p, "      local ");
		nl_dump_line(p, "%s\n",
			     _nl_inet_ntop(AF_INET6, &vxi->vxi_local6, addr));
	}

	if (vxi->ce_mask & VXLAN_ATTR_TTL) {
		nl_dump(p, "      ttl ");
		if(vxi->vxi_ttl)
			nl_dump_line(p, "%u\n", vxi->vxi_ttl);
		else
			nl_dump_line(p, "inherit\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_TOS) {
		nl_dump(p, "      tos ");
		if (vxi->vxi_tos == 1)
			nl_dump_line(p, "inherit\n");
		else
			nl_dump_line(p, "%#x\n", vxi->vxi_tos);
	}

	if (vxi->ce_mask & VXLAN_ATTR_LEARNING) {
		nl_dump(p, "      learning ");
		if (vxi->vxi_learning)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_learning);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_AGEING) {
		nl_dump(p, "      ageing ");
		if (vxi->vxi_ageing)
			nl_dump_line(p, "%u seconds\n", vxi->vxi_ageing);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_LIMIT) {
		nl_dump(p, "      limit ");
		if (vxi->vxi_limit)
			nl_dump_line(p, "%u\n", vxi->vxi_limit);
		else
			nl_dump_line(p, "unlimited\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_PORT_RANGE)
		nl_dump_line(p, "      port range %u - %u\n",
					 ntohs(vxi->vxi_port_range.low),
					 ntohs(vxi->vxi_port_range.high));

	if (vxi->ce_mask & VXLAN_ATTR_PROXY) {
		nl_dump(p, "      proxy ");
		if (vxi->vxi_proxy)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_proxy);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_RSC) {
		nl_dump(p, "      rsc ");
		if (vxi->vxi_rsc)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_rsc);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_L2MISS) {
		nl_dump(p, "      l2miss ");
		if (vxi->vxi_l2miss)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_l2miss);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_L3MISS) {
		nl_dump(p, "      l3miss ");
		if (vxi->vxi_l3miss)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_l3miss);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_PORT) {
		nl_dump(p, "      port ");
		nl_dump_line(p, "%u\n", ntohs(vxi->vxi_port));
	}

	if (vxi->ce_mask & VXLAN_ATTR_UDP_CSUM) {
		nl_dump(p, "      UDP checksums ");
		if (vxi->vxi_udp_csum)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_udp_csum);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_UDP_ZERO_CSUM6_TX) {
		nl_dump(p, "      udp-zero-csum6-tx ");
		if (vxi->vxi_udp_zero_csum6_tx)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_udp_zero_csum6_tx);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_UDP_ZERO_CSUM6_RX) {
		nl_dump(p, "      udp-zero-csum6-rx ");
		if (vxi->vxi_udp_zero_csum6_rx)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_udp_zero_csum6_rx);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_REMCSUM_TX) {
		nl_dump(p, "      remcsum-tx ");
		if (vxi->vxi_remcsum_tx)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_remcsum_tx);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_REMCSUM_RX) {
		nl_dump(p, "      remcsum-rx ");
		if (vxi->vxi_remcsum_rx)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_remcsum_rx);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->vxi_flags & RTNL_LINK_VXLAN_F_GBP)
		nl_dump(p, "      gbp\n");

	if (vxi->vxi_flags & RTNL_LINK_VXLAN_F_REMCSUM_NOPARTIAL)
		nl_dump(p, "      rncsum-nopartial\n");

	if (vxi->ce_mask & VXLAN_ATTR_COLLECT_METADATA) {
		nl_dump(p, "      remcsum-rx ");
		if (vxi->vxi_collect_metadata)
			nl_dump_line(p, "enabled (%#x)\n", vxi->vxi_collect_metadata);
		else
			nl_dump_line(p, "disabled\n");
	}

	if (vxi->ce_mask & VXLAN_ATTR_LABEL) {
		nl_dump(p, "      label ");
		nl_dump_line(p, "%u\n", ntohl(vxi->vxi_label));
	}

	if (vxi->vxi_flags & RTNL_LINK_VXLAN_F_GPE)
		nl_dump(p, "      gpe\n");
}

static int vxlan_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct vxlan_info *vdst, *vsrc = src->l_info;
	int err;

	dst->l_info = NULL;
	if ((err = rtnl_link_set_type(dst, "vxlan")) < 0)
		return err;
	vdst = dst->l_info;

	if (!vdst || !vsrc)
		return -NLE_NOMEM;

	memcpy(vdst, vsrc, sizeof(struct vxlan_info));

	return 0;
}

static int vxlan_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;
	struct nlattr *data;

	if (!(data = nla_nest_start(msg, IFLA_INFO_DATA)))
		return -NLE_MSGSIZE;

	if (vxi->ce_mask & VXLAN_ATTR_ID)
		NLA_PUT_U32(msg, IFLA_VXLAN_ID, vxi->vxi_id);

	if (vxi->ce_mask & VXLAN_ATTR_GROUP)
		NLA_PUT(msg, IFLA_VXLAN_GROUP, sizeof(vxi->vxi_group), &vxi->vxi_group);

	if (vxi->ce_mask & VXLAN_ATTR_GROUP6)
		NLA_PUT(msg, IFLA_VXLAN_GROUP6, sizeof(vxi->vxi_group6), &vxi->vxi_group6);

	if (vxi->ce_mask & VXLAN_ATTR_LINK)
		NLA_PUT_U32(msg, IFLA_VXLAN_LINK, vxi->vxi_link);

	if (vxi->ce_mask & VXLAN_ATTR_LOCAL)
		NLA_PUT(msg, IFLA_VXLAN_LOCAL, sizeof(vxi->vxi_local), &vxi->vxi_local);

	if (vxi->ce_mask & VXLAN_ATTR_LOCAL6)
		NLA_PUT(msg, IFLA_VXLAN_LOCAL6, sizeof(vxi->vxi_local6), &vxi->vxi_local6);

	if (vxi->ce_mask & VXLAN_ATTR_TTL)
		NLA_PUT_U8(msg, IFLA_VXLAN_TTL, vxi->vxi_ttl);

	if (vxi->ce_mask & VXLAN_ATTR_TOS)
		NLA_PUT_U8(msg, IFLA_VXLAN_TOS, vxi->vxi_tos);

	if (vxi->ce_mask & VXLAN_ATTR_LEARNING)
		NLA_PUT_U8(msg, IFLA_VXLAN_LEARNING, vxi->vxi_learning);

	if (vxi->ce_mask & VXLAN_ATTR_AGEING)
		NLA_PUT_U32(msg, IFLA_VXLAN_AGEING, vxi->vxi_ageing);

	if (vxi->ce_mask & VXLAN_ATTR_LIMIT)
		NLA_PUT_U32(msg, IFLA_VXLAN_LIMIT, vxi->vxi_limit);

	if (vxi->ce_mask & VXLAN_ATTR_PORT_RANGE)
		NLA_PUT(msg, IFLA_VXLAN_PORT_RANGE, sizeof(vxi->vxi_port_range),
				&vxi->vxi_port_range);

	if (vxi->ce_mask & VXLAN_ATTR_PROXY)
		NLA_PUT_U8(msg, IFLA_VXLAN_PROXY, vxi->vxi_proxy);

	if (vxi->ce_mask & VXLAN_ATTR_RSC)
		NLA_PUT_U8(msg, IFLA_VXLAN_RSC, vxi->vxi_rsc);

	if (vxi->ce_mask & VXLAN_ATTR_L2MISS)
		NLA_PUT_U8(msg, IFLA_VXLAN_L2MISS, vxi->vxi_l2miss);

	if (vxi->ce_mask & VXLAN_ATTR_L3MISS)
		NLA_PUT_U8(msg, IFLA_VXLAN_L3MISS, vxi->vxi_l3miss);

	if (vxi->ce_mask & VXLAN_ATTR_PORT)
		NLA_PUT_U16(msg, IFLA_VXLAN_PORT, vxi->vxi_port);

	if (vxi->ce_mask & VXLAN_ATTR_UDP_CSUM)
		NLA_PUT_U8(msg, IFLA_VXLAN_UDP_CSUM, vxi->vxi_udp_csum);

	if (vxi->ce_mask & VXLAN_ATTR_UDP_ZERO_CSUM6_TX)
		NLA_PUT_U8(msg, IFLA_VXLAN_UDP_ZERO_CSUM6_TX, vxi->vxi_udp_zero_csum6_tx);

	if (vxi->ce_mask & VXLAN_ATTR_UDP_ZERO_CSUM6_RX)
		NLA_PUT_U8(msg, IFLA_VXLAN_UDP_ZERO_CSUM6_RX, vxi->vxi_udp_zero_csum6_rx);

	if (vxi->ce_mask & VXLAN_ATTR_REMCSUM_TX)
		NLA_PUT_U8(msg, IFLA_VXLAN_REMCSUM_TX, vxi->vxi_remcsum_tx);

	if (vxi->ce_mask & VXLAN_ATTR_REMCSUM_RX)
		NLA_PUT_U8(msg, IFLA_VXLAN_REMCSUM_RX, vxi->vxi_remcsum_rx);

	if (vxi->vxi_flags & RTNL_LINK_VXLAN_F_GBP)
		NLA_PUT_FLAG(msg, IFLA_VXLAN_GBP);

	if (vxi->vxi_flags & RTNL_LINK_VXLAN_F_REMCSUM_NOPARTIAL)
		NLA_PUT_FLAG(msg, IFLA_VXLAN_REMCSUM_NOPARTIAL);

	if (vxi->ce_mask & VXLAN_ATTR_COLLECT_METADATA)
		NLA_PUT_U8(msg, IFLA_VXLAN_COLLECT_METADATA, vxi->vxi_collect_metadata);

	if (vxi->ce_mask & VXLAN_ATTR_LABEL)
		NLA_PUT_U32(msg, IFLA_VXLAN_LABEL, vxi->vxi_label);

	if (vxi->vxi_flags & RTNL_LINK_VXLAN_F_GPE)
		NLA_PUT_FLAG(msg, IFLA_VXLAN_GPE);

	nla_nest_end(msg, data);

nla_put_failure:

	return 0;
}

static int vxlan_compare(struct rtnl_link *link_a, struct rtnl_link *link_b,
			 int flags)
{
	struct vxlan_info *a = link_a->l_info;
	struct vxlan_info *b = link_b->l_info;
	int diff = 0;
	uint32_t attrs = flags & LOOSE_COMPARISON ? b->ce_mask :
						    ~((uint32_t)0u);

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(VXLAN_ATTR_ID, a->vxi_id != b->vxi_id);
	diff |= _DIFF(VXLAN_ATTR_GROUP, a->vxi_group != b->vxi_group);
	diff |= _DIFF(VXLAN_ATTR_LINK, a->vxi_link != b->vxi_link);
	diff |= _DIFF(VXLAN_ATTR_LOCAL, a->vxi_local != b->vxi_local);
	diff |= _DIFF(VXLAN_ATTR_TOS, a->vxi_tos != b->vxi_tos);
	diff |= _DIFF(VXLAN_ATTR_TTL, a->vxi_ttl != b->vxi_ttl);
	diff |= _DIFF(VXLAN_ATTR_LEARNING, a->vxi_learning != b->vxi_learning);
	diff |= _DIFF(VXLAN_ATTR_AGEING, a->vxi_ageing != b->vxi_ageing);
	diff |= _DIFF(VXLAN_ATTR_LIMIT, a->vxi_limit != b->vxi_limit);
	diff |= _DIFF(VXLAN_ATTR_PORT_RANGE,
		      a->vxi_port_range.low != b->vxi_port_range.low);
	diff |= _DIFF(VXLAN_ATTR_PORT_RANGE,
		      a->vxi_port_range.high != b->vxi_port_range.high);
	diff |= _DIFF(VXLAN_ATTR_PROXY, a->vxi_proxy != b->vxi_proxy);
	diff |= _DIFF(VXLAN_ATTR_RSC, a->vxi_proxy != b->vxi_proxy);
	diff |= _DIFF(VXLAN_ATTR_L2MISS, a->vxi_proxy != b->vxi_proxy);
	diff |= _DIFF(VXLAN_ATTR_L3MISS, a->vxi_proxy != b->vxi_proxy);
	diff |= _DIFF(VXLAN_ATTR_PORT, a->vxi_port != b->vxi_port);
	diff |= _DIFF(VXLAN_ATTR_GROUP6, memcmp(&a->vxi_group6, &b->vxi_group6,
						sizeof(a->vxi_group6)) != 0);
	diff |= _DIFF(VXLAN_ATTR_LOCAL6, memcmp(&a->vxi_local6, &b->vxi_local6,
						sizeof(a->vxi_local6)) != 0);
	diff |= _DIFF(VXLAN_ATTR_UDP_CSUM, a->vxi_udp_csum != b->vxi_udp_csum);
	diff |= _DIFF(VXLAN_ATTR_UDP_ZERO_CSUM6_TX,
		      a->vxi_udp_zero_csum6_tx != b->vxi_udp_zero_csum6_tx);
	diff |= _DIFF(VXLAN_ATTR_UDP_ZERO_CSUM6_RX,
		      a->vxi_udp_zero_csum6_rx != b->vxi_udp_zero_csum6_rx);
	diff |= _DIFF(VXLAN_ATTR_REMCSUM_TX,
		      a->vxi_remcsum_tx != b->vxi_remcsum_tx);
	diff |= _DIFF(VXLAN_ATTR_REMCSUM_RX,
		      a->vxi_remcsum_rx != b->vxi_remcsum_rx);
	diff |= _DIFF(VXLAN_ATTR_COLLECT_METADATA,
		      a->vxi_collect_metadata != b->vxi_collect_metadata);
	diff |= _DIFF(VXLAN_ATTR_LABEL, a->vxi_label != b->vxi_label);
	diff |= _DIFF(VXLAN_ATTR_FLAGS, a->vxi_flags != b->vxi_flags);
#undef _DIFF

	return diff;
}

static struct rtnl_link_info_ops vxlan_info_ops = {
	.io_name		= "vxlan",
	.io_alloc		= vxlan_alloc,
	.io_parse		= vxlan_parse,
	.io_dump = {
	    [NL_DUMP_LINE]	= vxlan_dump_line,
	    [NL_DUMP_DETAILS]	= vxlan_dump_details,
	},
	.io_clone		= vxlan_clone,
	.io_put_attrs		= vxlan_put_attrs,
	.io_free		= vxlan_free,
	.io_compare             = vxlan_compare,
};

/** @cond SKIP */
#define IS_VXLAN_LINK_ASSERT(link) \
	if ((link)->l_info_ops != &vxlan_info_ops) { \
		APPBUG("Link is not a vxlan link. set type \"vxlan\" first."); \
		return -NLE_OPNOTSUPP; \
	}
/** @endcond */

/**
 * @name VXLAN Object
 * @{
 */

/**
 * Allocate link object of type VXLAN
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_vxlan_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "vxlan") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a VXLAN link
 * @arg link		Link object
 *
 * @return True if link is a VXLAN link, otherwise false is returned.
 */
int rtnl_link_is_vxlan(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "vxlan");
}

/**
 * Set VXLAN Network Identifier
 * @arg link		Link object
 * @arg id		VXLAN network identifier (or VXLAN segment identifier)
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_id(struct rtnl_link *link, uint32_t id)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (id > VXLAN_ID_MAX)
		return -NLE_INVAL;

	vxi->vxi_id = id;
	vxi->ce_mask |= VXLAN_ATTR_ID;

	return 0;
}

/**
 * Get VXLAN Network Identifier
 * @arg link		Link object
 * @arg id			Pointer to store network identifier
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_id(struct rtnl_link *link, uint32_t *id)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if(!id)
		return -NLE_INVAL;

	if (vxi->ce_mask & VXLAN_ATTR_ID)
		*id = vxi->vxi_id;
	else
		return -NLE_AGAIN;

	return 0;
}

/**
 * Set VXLAN multicast IP address
 * @arg link		Link object
 * @arg addr		Multicast IP address to join
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_group(struct rtnl_link *link, struct nl_addr *addr)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if ((nl_addr_get_family(addr) == AF_INET) &&
	    (nl_addr_get_len(addr) == sizeof(vxi->vxi_group))) {
		memcpy(&vxi->vxi_group, nl_addr_get_binary_addr(addr),
		       sizeof(vxi->vxi_group));
		vxi->ce_mask |= VXLAN_ATTR_GROUP;
		vxi->ce_mask &= ~VXLAN_ATTR_GROUP6;
	} else if ((nl_addr_get_family(addr) == AF_INET6) &&
		   (nl_addr_get_len(addr) == sizeof(vxi->vxi_group6))) {
		memcpy(&vxi->vxi_group6, nl_addr_get_binary_addr(addr),
		       sizeof(vxi->vxi_group6));
		vxi->ce_mask |= VXLAN_ATTR_GROUP6;
		vxi->ce_mask &= ~VXLAN_ATTR_GROUP;
	} else
		return -NLE_INVAL;

	return 0;
}

/**
 * Get VXLAN multicast IP address
 * @arg link		Link object
 * @arg addr		Pointer to store multicast IP address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_group(struct rtnl_link *link, struct nl_addr **addr)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!addr)
		return -NLE_INVAL;

	if (vxi->ce_mask & VXLAN_ATTR_GROUP)
		*addr = nl_addr_build(AF_INET, &vxi->vxi_group, sizeof(vxi->vxi_group));
	else if (vxi->ce_mask & VXLAN_ATTR_GROUP6)
		*addr = nl_addr_build(AF_INET6, &vxi->vxi_group6, sizeof(vxi->vxi_group6));
	else
		return -NLE_AGAIN;

	return 0;
}

/**
 * Set physical device to use for VXLAN
 * @arg link		Link object
 * @arg index		Interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_link(struct rtnl_link *link, uint32_t index)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_link = index;
	vxi->ce_mask |= VXLAN_ATTR_LINK;

	return 0;
}

/**
 * Get physical device to use for VXLAN
 * @arg link		Link object
 * @arg index		Pointer to store interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_link(struct rtnl_link *link, uint32_t *index)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!index)
		return -NLE_INVAL;

	if (!(vxi->ce_mask & VXLAN_ATTR_LINK))
		return -NLE_AGAIN;

	*index = vxi->vxi_link;

	return 0;
}

/**
 * Set source address to use for VXLAN
 * @arg link		Link object
 * @arg addr		Local address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_local(struct rtnl_link *link, struct nl_addr *addr)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if ((nl_addr_get_family(addr) == AF_INET) &&
	    (nl_addr_get_len(addr) == sizeof(vxi->vxi_local))) {
		memcpy(&vxi->vxi_local, nl_addr_get_binary_addr(addr),
		       sizeof(vxi->vxi_local));
		vxi->ce_mask |= VXLAN_ATTR_LOCAL;
		vxi->ce_mask &= ~VXLAN_ATTR_LOCAL6;
	} else if ((nl_addr_get_family(addr) == AF_INET6) &&
		   (nl_addr_get_len(addr) == sizeof(vxi->vxi_local6))) {
		memcpy(&vxi->vxi_local6, nl_addr_get_binary_addr(addr),
		       sizeof(vxi->vxi_local6));
		vxi->ce_mask |= VXLAN_ATTR_LOCAL6;
		vxi->ce_mask &= ~VXLAN_ATTR_LOCAL;
	} else
		return -NLE_INVAL;

	return 0;
}

/**
 * Get source address to use for VXLAN
 * @arg link		Link object
 * @arg addr		Pointer to store local address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_local(struct rtnl_link *link, struct nl_addr **addr)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!addr)
		return -NLE_INVAL;

	if (vxi->ce_mask & VXLAN_ATTR_LOCAL)
		*addr = nl_addr_build(AF_INET, &vxi->vxi_local, sizeof(vxi->vxi_local));
	else if (vxi->ce_mask & VXLAN_ATTR_LOCAL6)
		*addr = nl_addr_build(AF_INET6, &vxi->vxi_local6, sizeof(vxi->vxi_local6));
	else
		return -NLE_AGAIN;

	return 0;
}

/**
 * Set IP TTL value to use for VXLAN
 * @arg link		Link object
 * @arg ttl			TTL value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_ttl(struct rtnl_link *link, uint8_t ttl)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_ttl = ttl;
	vxi->ce_mask |= VXLAN_ATTR_TTL;

	return 0;
}

/**
 * Get IP TTL value to use for VXLAN
 * @arg link		Link object
 *
 * @return TTL value on success or a negative error code
 */
int rtnl_link_vxlan_get_ttl(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_TTL))
		return -NLE_AGAIN;

	return vxi->vxi_ttl;
}

/**
 * Set IP ToS value to use for VXLAN
 * @arg link		Link object
 * @arg tos		ToS value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_tos(struct rtnl_link *link, uint8_t tos)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_tos = tos;
	vxi->ce_mask |= VXLAN_ATTR_TOS;

	return 0;
}

/**
 * Get IP ToS value to use for VXLAN
 * @arg link		Link object
 *
 * @return ToS value on success or a negative error code
 */
int rtnl_link_vxlan_get_tos(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_TOS))
		return -NLE_AGAIN;

	return vxi->vxi_tos;
}

/**
 * Set VXLAN learning status
 * @arg link		Link object
 * @arg learning	Learning status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_learning(struct rtnl_link *link, uint8_t learning)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_learning = learning;
	vxi->ce_mask |= VXLAN_ATTR_LEARNING;

	return 0;
}

/**
 * Get VXLAN learning status
 * @arg link		Link object
 *
 * @return Learning status value on success or a negative error code
 */
int rtnl_link_vxlan_get_learning(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_LEARNING))
		return -NLE_AGAIN;

	return vxi->vxi_learning;
}

/**
 * Enable VXLAN address learning
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_enable_learning(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_learning(link, 1);
}

/**
 * Disable VXLAN address learning
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_disable_learning(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_learning(link, 0);
}

/**
 * Set expiration timer value to use for VXLAN
 * @arg link		Link object
 * @arg expiry		Expiration timer value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_ageing(struct rtnl_link *link, uint32_t expiry)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_ageing = expiry;
	vxi->ce_mask |= VXLAN_ATTR_AGEING;

	return 0;
}

/**
 * Get expiration timer value to use for VXLAN
 * @arg link		Link object
 * @arg expiry		Pointer to store expiration timer value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_ageing(struct rtnl_link *link, uint32_t *expiry)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!expiry)
		return -NLE_INVAL;

	if (vxi->ce_mask & VXLAN_ATTR_AGEING)
		*expiry = vxi->vxi_ageing;
	else
		return -NLE_AGAIN;

	return 0;
}

/**
 * Set maximum number of forwarding database entries to use for VXLAN
 * @arg link		Link object
 * @arg limit		Maximum number
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_limit(struct rtnl_link *link, uint32_t limit)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_limit = limit;
	vxi->ce_mask |= VXLAN_ATTR_LIMIT;

	return 0;
}

/**
 * Get maximum number of forwarding database entries to use for VXLAN
 * @arg link		Link object
 * @arg limit		Pointer to store maximum number
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_limit(struct rtnl_link *link, uint32_t *limit)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!limit)
		return -NLE_INVAL;

	if (vxi->ce_mask & VXLAN_ATTR_LIMIT)
		*limit = vxi->vxi_limit;
	else
		return -NLE_AGAIN;

	return 0;
}

/**
 * Set range of UDP port numbers to use for VXLAN
 * @arg link		Link object
 * @arg range		Port number range
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_port_range(struct rtnl_link *link,
                                   struct ifla_vxlan_port_range *range)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!range)
		return -NLE_INVAL;

	memcpy(&vxi->vxi_port_range, range, sizeof(vxi->vxi_port_range));
	vxi->ce_mask |= VXLAN_ATTR_PORT_RANGE;

	return 0;
}

/**
 * Get range of UDP port numbers to use for VXLAN
 * @arg link		Link object
 * @arg range		Pointer to store port range
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_port_range(struct rtnl_link *link,
                                   struct ifla_vxlan_port_range *range)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!range)
		return -NLE_INVAL;

	if (vxi->ce_mask & VXLAN_ATTR_PORT_RANGE)
		memcpy(range, &vxi->vxi_port_range, sizeof(*range));
	else
		return -NLE_AGAIN;

	return 0;
}

/**
 * Set ARP proxy status to use for VXLAN
 * @arg link		Link object
 * @arg proxy		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_proxy(struct rtnl_link *link, uint8_t proxy)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_proxy = proxy;
	vxi->ce_mask |= VXLAN_ATTR_PROXY;

	return 0;
}

/**
 * Get ARP proxy status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_proxy(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_PROXY))
		return -NLE_AGAIN;

	return vxi->vxi_proxy;
}

/**
 * Enable ARP proxy
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_enable_proxy(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_proxy(link, 1);
}

/**
 * Disable ARP proxy
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_disable_proxy(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_proxy(link, 0);
}

/**
 * Set Route Short Circuit status to use for VXLAN
 * @arg link		Link object
 * @arg rsc			Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_rsc(struct rtnl_link *link, uint8_t rsc)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_rsc = rsc;
	vxi->ce_mask |= VXLAN_ATTR_RSC;

	return 0;
}

/**
 * Get Route Short Circuit status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_rsc(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_RSC))
		return -NLE_AGAIN;

	return vxi->vxi_rsc;
}

/**
 * Enable Route Short Circuit
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_enable_rsc(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_rsc(link, 1);
}

/**
 * Disable Route Short Circuit
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_disable_rsc(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_rsc(link, 0);
}

/**
 * Set netlink LLADDR miss notification status to use for VXLAN
 * @arg link		Link object
 * @arg miss		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_l2miss(struct rtnl_link *link, uint8_t miss)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_l2miss = miss;
	vxi->ce_mask |= VXLAN_ATTR_L2MISS;

	return 0;
}

/**
 * Get netlink LLADDR miss notification status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_l2miss(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_L2MISS))
		return -NLE_AGAIN;

	return vxi->vxi_l2miss;
}

/**
 * Enable netlink LLADDR miss notifications
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_enable_l2miss(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_l2miss(link, 1);
}

/**
 * Disable netlink LLADDR miss notifications
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_disable_l2miss(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_l2miss(link, 0);
}

/**
 * Set netlink IP ADDR miss notification status to use for VXLAN
 * @arg link		Link object
 * @arg miss		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_l3miss(struct rtnl_link *link, uint8_t miss)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_l3miss = miss;
	vxi->ce_mask |= VXLAN_ATTR_L3MISS;

	return 0;
}

/**
 * Get netlink IP ADDR miss notification status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_l3miss(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_L3MISS))
		return -NLE_AGAIN;

	return vxi->vxi_l3miss;
}

/**
 * Enable netlink IP ADDR miss notifications
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_enable_l3miss(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_l3miss(link, 1);
}

/**
 * Disable netlink IP ADDR miss notifications
 * @arg link		Link object
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_disable_l3miss(struct rtnl_link *link)
{
	return rtnl_link_vxlan_set_l3miss(link, 0);
}

/**
 * Set UDP destination port to use for VXLAN
 * @arg link		Link object
 * @arg port		Destination port
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_port(struct rtnl_link *link, uint32_t port)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_port = htons(port);
	vxi->ce_mask |= VXLAN_ATTR_PORT;

	return 0;
}

/**
 * Get UDP destination port to use for VXLAN
 * @arg link		Link object
 * @arg port		Pointer to store destination port
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_port(struct rtnl_link *link, uint32_t *port)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!port)
		return -NLE_INVAL;

	if (!(vxi->ce_mask & VXLAN_ATTR_PORT))
		return -NLE_NOATTR;

	*port = ntohs(vxi->vxi_port);

	return 0;
}

/**
 * Set UDP checksum status to use for VXLAN
 * @arg link		Link object
 * @arg csum		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_udp_csum(struct rtnl_link *link, uint8_t csum)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_udp_csum = csum;
	vxi->ce_mask |= VXLAN_ATTR_UDP_CSUM;

	return 0;
}

/**
 * Get UDP checksum status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_udp_csum(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_UDP_CSUM))
		return -NLE_NOATTR;

	return vxi->vxi_udp_csum;
}

/**
 * Set skip UDP checksum transmitted over IPv6 status to use for VXLAN
 * @arg link		Link object
 * @arg csum		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_udp_zero_csum6_tx(struct rtnl_link *link, uint8_t csum)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_udp_zero_csum6_tx = csum;
	vxi->ce_mask |= VXLAN_ATTR_UDP_ZERO_CSUM6_TX;

	return 0;
}

/**
 * Get skip UDP checksum transmitted over IPv6 status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_udp_zero_csum6_tx(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_UDP_ZERO_CSUM6_TX))
		return -NLE_NOATTR;

	return vxi->vxi_udp_zero_csum6_tx;
}

/**
 * Set skip UDP checksum received over IPv6 status to use for VXLAN
 * @arg link		Link object
 * @arg csum		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_udp_zero_csum6_rx(struct rtnl_link *link, uint8_t csum)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_udp_zero_csum6_rx = csum;
	vxi->ce_mask |= VXLAN_ATTR_UDP_ZERO_CSUM6_RX;

	return 0;
}

/**
 * Get skip UDP checksum received over IPv6 status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_udp_zero_csum6_rx(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_UDP_ZERO_CSUM6_RX))
		return -NLE_NOATTR;

	return vxi->vxi_udp_zero_csum6_rx;
}

/**
 * Set remote offload transmit checksum status to use for VXLAN
 * @arg link		Link object
 * @arg csum		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_remcsum_tx(struct rtnl_link *link, uint8_t csum)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_remcsum_tx = csum;
	vxi->ce_mask |= VXLAN_ATTR_REMCSUM_TX;

	return 0;
}

/**
 * Get remote offload transmit checksum status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_remcsum_tx(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_REMCSUM_TX))
		return -NLE_NOATTR;

	return vxi->vxi_remcsum_tx;
}

/**
 * Set remote offload receive checksum status to use for VXLAN
 * @arg link		Link object
 * @arg csum		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_remcsum_rx(struct rtnl_link *link, uint8_t csum)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_remcsum_rx = csum;
	vxi->ce_mask |= VXLAN_ATTR_REMCSUM_RX;

	return 0;
}

/**
 * Get remote offload receive checksum status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_remcsum_rx(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_REMCSUM_RX))
		return -NLE_NOATTR;

	return vxi->vxi_remcsum_rx;
}

/**
 * Set collect metadata status to use for VXLAN
 * @arg link		Link object
 * @arg collect		Status value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_collect_metadata(struct rtnl_link *link, uint8_t collect)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_collect_metadata = collect;
	vxi->ce_mask |= VXLAN_ATTR_COLLECT_METADATA;

	return 0;
}

/**
 * Get collect metadata status to use for VXLAN
 * @arg link		Link object
 *
 * @return Status value on success or a negative error code
 */
int rtnl_link_vxlan_get_collect_metadata(struct rtnl_link *link)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!(vxi->ce_mask & VXLAN_ATTR_COLLECT_METADATA))
		return -NLE_NOATTR;

	return vxi->vxi_collect_metadata;
}

/**
 * Set flow label to use for VXLAN
 * @arg link		Link object
 * @arg label		Destination label
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_label(struct rtnl_link *link, uint32_t label)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	vxi->vxi_label = htonl(label);
	vxi->ce_mask |= VXLAN_ATTR_LABEL;

	return 0;
}

/**
 * Get flow label to use for VXLAN
 * @arg link		Link object
 * @arg label		Pointer to store destination label
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_get_label(struct rtnl_link *link, uint32_t *label)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (!label)
		return -NLE_INVAL;

	if (!(vxi->ce_mask & VXLAN_ATTR_LABEL))
		return -NLE_NOATTR;

	*label = ntohl(vxi->vxi_label);

	return 0;
}

/**
 * Set VXLAN flags RTNL_LINK_VXLAN_F_*
 * @arg link		Link object
 * @flags               Which flags to set
 * @arg enable		Boolean enabling or disabling flag
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vxlan_set_flags(struct rtnl_link *link, uint32_t flags, int enable)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	if (flags & ~(RTNL_LINK_VXLAN_F_GBP | RTNL_LINK_VXLAN_F_GPE | RTNL_LINK_VXLAN_F_REMCSUM_NOPARTIAL))
		return -NLE_INVAL;

	if (enable)
		vxi->vxi_flags |= flags;
	else
		vxi->vxi_flags &= ~flags;

	return 0;
}

/**
 * Get VXLAN flags RTNL_LINK_VXLAN_F_*
 * @arg link		Link object
 * @arg out_flags       Output value for flags. Must be present.
 *
 * @return Zero on success or a negative error code
 */
int rtnl_link_vxlan_get_flags(struct rtnl_link *link, uint32_t *out_flags)
{
	struct vxlan_info *vxi = link->l_info;

	IS_VXLAN_LINK_ASSERT(link);

	*out_flags = vxi->vxi_flags;
	return 0;
}

/** @} */

static void _nl_init vxlan_init(void)
{
	rtnl_link_register_info(&vxlan_info_ops);
}

static void _nl_exit vxlan_exit(void)
{
	rtnl_link_unregister_info(&vxlan_info_ops);
}

/** @} */
