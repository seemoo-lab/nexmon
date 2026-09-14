/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2018 Volodymyr Bendiuga <volodymyr.bendiuga@gmail.com>
 */

#include "nl-default.h"

#include <linux/ethtool.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/route/classifier.h>
#include <netlink/route/action.h>
#include <netlink/route/cls/flower.h>

#include "tc-api.h"
#include "nl-aux-route/nl-route.h"

/** @cond SKIP */
struct rtnl_flower {
	struct rtnl_act *cf_act;
	int cf_mask;
	uint32_t cf_flags;
	uint16_t cf_proto;
	uint16_t cf_vlan_id;
	uint16_t cf_vlan_ethtype;
	uint8_t cf_vlan_prio;
	uint8_t cf_src_mac[ETH_ALEN];
	uint8_t cf_src_mac_mask[ETH_ALEN];
	uint8_t cf_dst_mac[ETH_ALEN];
	uint8_t cf_dst_mac_mask[ETH_ALEN];
	in_addr_t cf_ipv4_src;
	in_addr_t cf_ipv4_src_mask;
	in_addr_t cf_ipv4_dst;
	in_addr_t cf_ipv4_dst_mask;
	uint8_t cf_ip_dscp;
	uint8_t cf_ip_dscp_mask;
};

#define FLOWER_ATTR_FLAGS         (1 << 0)
#define FLOWER_ATTR_ACTION        (1 << 1)
#define FLOWER_ATTR_VLAN_ID       (1 << 2)
#define FLOWER_ATTR_VLAN_PRIO     (1 << 3)
#define FLOWER_ATTR_VLAN_ETH_TYPE (1 << 4)
#define FLOWER_ATTR_DST_MAC       (1 << 5)
#define FLOWER_ATTR_DST_MAC_MASK  (1 << 6)
#define FLOWER_ATTR_SRC_MAC       (1 << 7)
#define FLOWER_ATTR_SRC_MAC_MASK  (1 << 8)
#define FLOWER_ATTR_IP_DSCP       (1 << 9)
#define FLOWER_ATTR_IP_DSCP_MASK  (1 << 10)
#define FLOWER_ATTR_PROTO         (1 << 11)
#define FLOWER_ATTR_IPV4_SRC      (1 << 12)
#define FLOWER_ATTR_IPV4_SRC_MASK (1 << 13)
#define FLOWER_ATTR_IPV4_DST      (1 << 14)
#define FLOWER_ATTR_IPV4_DST_MASK (1 << 15)
/** @endcond */

#define FLOWER_DSCP_MAX             0xe0
#define FLOWER_DSCP_MASK_MAX        0xe0
#define FLOWER_VID_MAX              4095
#define FLOWER_VLAN_PRIO_MAX        7

static struct nla_policy flower_policy[TCA_FLOWER_MAX + 1] = {
	[TCA_FLOWER_FLAGS]             = { .type = NLA_U32 },
	[TCA_FLOWER_KEY_ETH_TYPE]      = { .type = NLA_U16 },
	[TCA_FLOWER_KEY_ETH_DST]       = { .maxlen = ETH_ALEN },
	[TCA_FLOWER_KEY_ETH_DST_MASK]  = { .maxlen = ETH_ALEN },
	[TCA_FLOWER_KEY_ETH_SRC]       = { .maxlen = ETH_ALEN },
	[TCA_FLOWER_KEY_ETH_SRC_MASK]  = { .maxlen = ETH_ALEN },
	[TCA_FLOWER_KEY_VLAN_ID]       = { .type = NLA_U16 },
	[TCA_FLOWER_KEY_VLAN_PRIO]     = { .type = NLA_U8 },
	[TCA_FLOWER_KEY_IP_TOS]        = { .type = NLA_U8 },
	[TCA_FLOWER_KEY_IP_TOS_MASK]   = { .type = NLA_U8 },
	[TCA_FLOWER_KEY_VLAN_ETH_TYPE] = { .type = NLA_U16 },
	[TCA_FLOWER_KEY_IPV4_SRC]      = { .type = NLA_U32 },
	[TCA_FLOWER_KEY_IPV4_SRC_MASK] = { .type = NLA_U32 },
	[TCA_FLOWER_KEY_IPV4_DST]      = { .type = NLA_U32 },
	[TCA_FLOWER_KEY_IPV4_DST_MASK] = { .type = NLA_U32 },
};

static int flower_msg_parser(struct rtnl_tc *tc, void *data)
{
	struct rtnl_flower *f = data;
	struct nlattr *tb[TCA_FLOWER_MAX + 1];
	int err;

	err = tca_parse(tb, TCA_FLOWER_MAX, tc, flower_policy);
	if (err < 0)
		return err;

	if (tb[TCA_FLOWER_FLAGS]) {
		f->cf_flags = nla_get_u32(tb[TCA_FLOWER_FLAGS]);
		f->cf_mask |= FLOWER_ATTR_FLAGS;
	}

	if (tb[TCA_FLOWER_ACT]) {
		err = rtnl_act_parse(&f->cf_act, tb[TCA_FLOWER_ACT]);
		if (err)
			return err;

		f->cf_mask |= FLOWER_ATTR_ACTION;
	}

	if (tb[TCA_FLOWER_KEY_ETH_TYPE]) {
		f->cf_proto = nla_get_u16(tb[TCA_FLOWER_KEY_ETH_TYPE]);
		f->cf_mask |= FLOWER_ATTR_PROTO;
	}

	if (tb[TCA_FLOWER_KEY_VLAN_ID]) {
		f->cf_vlan_id = nla_get_u16(tb[TCA_FLOWER_KEY_VLAN_ID]);
		f->cf_mask |= FLOWER_ATTR_VLAN_ID;
	}

	if (tb[TCA_FLOWER_KEY_VLAN_PRIO]) {
		f->cf_vlan_prio = nla_get_u8(tb[TCA_FLOWER_KEY_VLAN_PRIO]);
		f->cf_mask |= FLOWER_ATTR_VLAN_PRIO;
	}

	if (tb[TCA_FLOWER_KEY_VLAN_ETH_TYPE]) {
		f->cf_vlan_ethtype = nla_get_u16(tb[TCA_FLOWER_KEY_VLAN_ETH_TYPE]);
		f->cf_mask |= FLOWER_ATTR_VLAN_ETH_TYPE;
	}

	if (tb[TCA_FLOWER_KEY_ETH_DST]) {
		nla_memcpy(f->cf_dst_mac, tb[TCA_FLOWER_KEY_ETH_DST], ETH_ALEN);
		f->cf_mask |= FLOWER_ATTR_DST_MAC;
	}

	if (tb[TCA_FLOWER_KEY_ETH_DST_MASK]) {
		nla_memcpy(f->cf_dst_mac_mask, tb[TCA_FLOWER_KEY_ETH_DST_MASK], ETH_ALEN);
		f->cf_mask |= FLOWER_ATTR_DST_MAC_MASK;
	}

	if (tb[TCA_FLOWER_KEY_ETH_SRC]) {
		nla_memcpy(f->cf_src_mac, tb[TCA_FLOWER_KEY_ETH_SRC], ETH_ALEN);
		f->cf_mask |= FLOWER_ATTR_SRC_MAC;
	}

	if (tb[TCA_FLOWER_KEY_ETH_SRC_MASK]) {
		nla_memcpy(f->cf_src_mac_mask, tb[TCA_FLOWER_KEY_ETH_SRC_MASK], ETH_ALEN);
		f->cf_mask |= FLOWER_ATTR_SRC_MAC_MASK;
	}

	if (tb[TCA_FLOWER_KEY_IP_TOS]) {
		f->cf_ip_dscp = nla_get_u8(tb[TCA_FLOWER_KEY_IP_TOS]);
		f->cf_mask |= FLOWER_ATTR_IP_DSCP;
	}

	if (tb[TCA_FLOWER_KEY_IP_TOS_MASK]) {
		f->cf_ip_dscp_mask = nla_get_u8(tb[TCA_FLOWER_KEY_IP_TOS_MASK]);
		f->cf_mask |= FLOWER_ATTR_IP_DSCP_MASK;
	}

	if (tb[TCA_FLOWER_KEY_IPV4_SRC]) {
		f->cf_ipv4_src = nla_get_u32(tb[TCA_FLOWER_KEY_IPV4_SRC]);
		f->cf_mask |= FLOWER_ATTR_IPV4_SRC;
	}

	if (tb[TCA_FLOWER_KEY_IPV4_SRC_MASK]) {
		f->cf_ipv4_src_mask =
			nla_get_u32(tb[TCA_FLOWER_KEY_IPV4_SRC_MASK]);
		f->cf_mask |= FLOWER_ATTR_IPV4_SRC_MASK;
	}

	if (tb[TCA_FLOWER_KEY_IPV4_DST]) {
		f->cf_ipv4_dst = nla_get_u32(tb[TCA_FLOWER_KEY_IPV4_DST]);
		f->cf_mask |= FLOWER_ATTR_IPV4_DST;
	}

	if (tb[TCA_FLOWER_KEY_IPV4_DST_MASK]) {
		f->cf_ipv4_dst_mask =
			nla_get_u32(tb[TCA_FLOWER_KEY_IPV4_DST_MASK]);
		f->cf_mask |= FLOWER_ATTR_IPV4_DST_MASK;
	}

	return 0;
}

static int flower_msg_fill(struct rtnl_tc *tc, void *data, struct nl_msg *msg)
{
	struct rtnl_flower *f = data;
	int err;

	if (!f)
		return 0;

	if (f->cf_mask & FLOWER_ATTR_FLAGS)
		NLA_PUT_U32(msg, TCA_FLOWER_FLAGS, f->cf_flags);

	if (f->cf_mask & FLOWER_ATTR_ACTION) {
		err = rtnl_act_fill(msg, TCA_FLOWER_ACT, f->cf_act);
		if (err)
			return err;
	}

	if (f->cf_mask & FLOWER_ATTR_PROTO)
		NLA_PUT_U16(msg, TCA_FLOWER_KEY_ETH_TYPE, f->cf_proto);

	if (f->cf_mask & FLOWER_ATTR_VLAN_ID)
		NLA_PUT_U16(msg, TCA_FLOWER_KEY_VLAN_ID, f->cf_vlan_id);

	if (f->cf_mask & FLOWER_ATTR_VLAN_PRIO)
		NLA_PUT_U8(msg, TCA_FLOWER_KEY_VLAN_PRIO, f->cf_vlan_prio);

	if (f->cf_mask & FLOWER_ATTR_VLAN_ETH_TYPE)
		NLA_PUT_U16(msg, TCA_FLOWER_KEY_VLAN_ETH_TYPE, f->cf_vlan_ethtype);

	if (f->cf_mask & FLOWER_ATTR_DST_MAC)
		NLA_PUT(msg, TCA_FLOWER_KEY_ETH_DST, ETH_ALEN, f->cf_dst_mac);

	if (f->cf_mask & FLOWER_ATTR_DST_MAC_MASK)
		NLA_PUT(msg, TCA_FLOWER_KEY_ETH_DST_MASK, ETH_ALEN, f->cf_dst_mac_mask);

	if (f->cf_mask & FLOWER_ATTR_SRC_MAC)
		NLA_PUT(msg, TCA_FLOWER_KEY_ETH_SRC, ETH_ALEN, f->cf_src_mac);

	if (f->cf_mask & FLOWER_ATTR_SRC_MAC_MASK)
		NLA_PUT(msg, TCA_FLOWER_KEY_ETH_SRC_MASK, ETH_ALEN, f->cf_src_mac_mask);

	if (f->cf_mask & FLOWER_ATTR_IP_DSCP)
		NLA_PUT_U8(msg, TCA_FLOWER_KEY_IP_TOS, f->cf_ip_dscp);

	if (f->cf_mask & FLOWER_ATTR_IP_DSCP_MASK)
		NLA_PUT_U8(msg, TCA_FLOWER_KEY_IP_TOS_MASK, f->cf_ip_dscp_mask);

	if (f->cf_mask & FLOWER_ATTR_IPV4_SRC)
		NLA_PUT_U32(msg, TCA_FLOWER_KEY_IPV4_SRC, f->cf_ipv4_src);

	if (f->cf_mask & FLOWER_ATTR_IPV4_SRC_MASK)
		NLA_PUT_U32(msg, TCA_FLOWER_KEY_IPV4_SRC_MASK,
			    f->cf_ipv4_src_mask);

	if (f->cf_mask & FLOWER_ATTR_IPV4_DST)
		NLA_PUT_U32(msg, TCA_FLOWER_KEY_IPV4_DST, f->cf_ipv4_dst);

	if (f->cf_mask & FLOWER_ATTR_IPV4_DST_MASK)
		NLA_PUT_U32(msg, TCA_FLOWER_KEY_IPV4_DST_MASK,
			    f->cf_ipv4_dst_mask);

	return 0;

nla_put_failure:
	return -NLE_NOMEM;
}

static void flower_free_data(struct rtnl_tc *tc, void *data)
{
	struct rtnl_flower *f = data;

	if (f->cf_act)
		rtnl_act_put_all(&f->cf_act);
}

static int flower_clone(void *_dst, void *_src)
{
	struct rtnl_flower *dst = _dst, *src = _src;

	if (src->cf_act) {
		if (!(dst->cf_act = rtnl_act_alloc()))
			return -NLE_NOMEM;

		memcpy(dst->cf_act, src->cf_act, sizeof(struct rtnl_act));

		/* action nl list next and prev pointers must be updated */
		nl_init_list_head(&dst->cf_act->ce_list);

		if (   src->cf_act->c_opts
		    && !(dst->cf_act->c_opts = nl_data_clone(src->cf_act->c_opts)))
			return -NLE_NOMEM;

		if (   src->cf_act->c_xstats
		    && !(dst->cf_act->c_xstats = nl_data_clone(src->cf_act->c_xstats)))
			return -NLE_NOMEM;

		if (   src->cf_act->c_subdata
		    && !(dst->cf_act->c_subdata = nl_data_clone(src->cf_act->c_subdata)))
			return -NLE_NOMEM;

		if (dst->cf_act->c_link) {
			nl_object_get(OBJ_CAST(dst->cf_act->c_link));
		}

		dst->cf_act->a_next = NULL;   /* Only clone first in chain */
	}

	return 0;
}

static void flower_dump_details(struct rtnl_tc *tc, void *data,
                                struct nl_dump_params *p)
{
	struct rtnl_flower *f = data;
	char addr_str[INET_ADDRSTRLEN];
	char mask_str[INET_ADDRSTRLEN];

	if (!f)
		return;

	if (f->cf_mask & FLOWER_ATTR_FLAGS)
		nl_dump(p, " flags %u", f->cf_flags);

	if (f->cf_mask & FLOWER_ATTR_PROTO)
		nl_dump(p, " protocol %u", f->cf_proto);

	if (f->cf_mask & FLOWER_ATTR_VLAN_ID)
		nl_dump(p, " vlan_id %u", f->cf_vlan_id);

	if (f->cf_mask & FLOWER_ATTR_VLAN_PRIO)
		nl_dump(p, " vlan_prio %u", f->cf_vlan_prio);

	if (f->cf_mask & FLOWER_ATTR_VLAN_ETH_TYPE)
		nl_dump(p, " vlan_ethtype %u", f->cf_vlan_ethtype);

	if (f->cf_mask & FLOWER_ATTR_DST_MAC)
		nl_dump(p, " dst_mac %02x:%02x:%02x:%02x:%02x:%02x",
		        f->cf_dst_mac[0], f->cf_dst_mac[1],
		        f->cf_dst_mac[2], f->cf_dst_mac[3],
		        f->cf_dst_mac[4], f->cf_dst_mac[5]);

	if (f->cf_mask & FLOWER_ATTR_DST_MAC_MASK)
		nl_dump(p, " dst_mac_mask %02x:%02x:%02x:%02x:%02x:%02x",
		        f->cf_dst_mac_mask[0], f->cf_dst_mac_mask[1],
		        f->cf_dst_mac_mask[2], f->cf_dst_mac_mask[3],
		        f->cf_dst_mac_mask[4], f->cf_dst_mac_mask[5]);

	if (f->cf_mask & FLOWER_ATTR_SRC_MAC)
		nl_dump(p, " src_mac %02x:%02x:%02x:%02x:%02x:%02x",
		        f->cf_src_mac[0], f->cf_src_mac[1],
		        f->cf_src_mac[2], f->cf_src_mac[3],
		        f->cf_src_mac[4], f->cf_src_mac[5]);

	if (f->cf_mask & FLOWER_ATTR_SRC_MAC_MASK)
		nl_dump(p, " src_mac_mask %02x:%02x:%02x:%02x:%02x:%02x",
		        f->cf_src_mac_mask[0], f->cf_src_mac_mask[1],
		        f->cf_src_mac_mask[2], f->cf_src_mac_mask[3],
		        f->cf_src_mac_mask[4], f->cf_src_mac_mask[5]);

	if (f->cf_mask & FLOWER_ATTR_IP_DSCP)
		nl_dump(p, " dscp %u", f->cf_ip_dscp);

	if (f->cf_mask & FLOWER_ATTR_IP_DSCP_MASK)
		nl_dump(p, " dscp_mask %u", f->cf_ip_dscp_mask);

	if (f->cf_mask & FLOWER_ATTR_IPV4_SRC) {
		_nl_inet_ntop4(f->cf_ipv4_src, addr_str);
		_nl_inet_ntop4(f->cf_ipv4_src_mask, mask_str);
		nl_dump(p, "IPv4 src %s mask %s\n", addr_str, mask_str);
	}

	if (f->cf_mask & FLOWER_ATTR_IPV4_DST) {
		_nl_inet_ntop4(f->cf_ipv4_dst, addr_str);
		_nl_inet_ntop4(f->cf_ipv4_dst_mask, mask_str);
		nl_dump(p, "IPv4 dst %s mask %s\n", addr_str, mask_str);
	}
}

/**
 * @name Attribute Modification
 * @{
 */

/**
 * Set protocol for flower classifier
 * @arg cls		Flower classifier.
 * @arg proto		protocol (ETH_P_*)
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_proto(struct rtnl_cls *cls, uint16_t proto)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	f->cf_proto = htons(proto);
	f->cf_mask |= FLOWER_ATTR_PROTO;

	return 0;
}

/**
 * Get protocol for flower classifier
 * @arg cls		Flower classifier.
 * @arg proto		protocol
 * @return 0 on success or a negative error code.
*/
int rtnl_flower_get_proto(struct rtnl_cls *cls, uint16_t *proto)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(f->cf_mask & FLOWER_ATTR_PROTO))
		return -NLE_MISSING_ATTR;

	*proto = ntohs(f->cf_proto);

	return 0;
}

/**
 * Set vlan id for flower classifier
 * @arg cls		Flower classifier.
 * @arg vid		vlan id
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_vlan_id(struct rtnl_cls *cls, uint16_t vid)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (vid > FLOWER_VID_MAX)
		return -NLE_RANGE;

	f->cf_vlan_id = vid;
	f->cf_mask |= FLOWER_ATTR_VLAN_ID;

	return 0;
}

/**
 * Get vlan id for flower classifier
 * @arg cls		Flower classifier.
 * @arg vid		vlan id
 * @return 0 on success or a negative error code.
*/
int rtnl_flower_get_vlan_id(struct rtnl_cls *cls, uint16_t *vid)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(f->cf_mask & FLOWER_ATTR_VLAN_ID))
		return -NLE_MISSING_ATTR;

	*vid = f->cf_vlan_id;

	return 0;
}

/**
 * Set vlan priority for flower classifier
 * @arg cls		Flower classifier.
 * @arg prio		vlan priority
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_vlan_prio(struct rtnl_cls *cls, uint8_t prio)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (prio > FLOWER_VLAN_PRIO_MAX)
		return -NLE_RANGE;

	f->cf_vlan_prio = prio;
	f->cf_mask |= FLOWER_ATTR_VLAN_PRIO;

	return 0;
}

/**
 * Get vlan prio for flower classifier
 * @arg cls		Flower classifier.
 * @arg prio		vlan priority
 * @return 0 on success or a negative error code.
*/
int rtnl_flower_get_vlan_prio(struct rtnl_cls *cls, uint8_t *prio)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(f->cf_mask & FLOWER_ATTR_VLAN_PRIO))
		return -NLE_MISSING_ATTR;

	*prio = f->cf_vlan_prio;

	return 0;
}

/**
 * Set vlan ethertype for flower classifier
 * @arg cls		Flower classifier.
 * @arg ethtype		vlan ethertype
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_vlan_ethtype(struct rtnl_cls *cls, uint16_t ethtype)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (!(f->cf_mask & FLOWER_ATTR_PROTO))
		return -NLE_MISSING_ATTR;

	if (f->cf_proto != htons(ETH_P_8021Q))
		return -NLE_INVAL;

	f->cf_vlan_ethtype = htons(ethtype);
	f->cf_mask |= FLOWER_ATTR_VLAN_ETH_TYPE;

	return 0;
}

/**
 * Set destination mac address for flower classifier
 * @arg cls		Flower classifier.
 * @arg mac		destination mac address
 * @arg mask		mask for mac address
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_dst_mac(struct rtnl_cls *cls, unsigned char *mac,
                            unsigned char *mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (mac) {
		memcpy(f->cf_dst_mac, mac, ETH_ALEN);
		f->cf_mask |= FLOWER_ATTR_DST_MAC;

		if (mask) {
			memcpy(f->cf_dst_mac_mask, mask, ETH_ALEN);
			f->cf_mask |= FLOWER_ATTR_DST_MAC_MASK;
		}

		return 0;
	}

	return -NLE_FAILURE;
}

/**
 * Get destination mac address for flower classifier
 * @arg cls		Flower classifier.
 * @arg mac		destination mac address
 * @arg mask		mask for mac address
 * @return 0 on success or a negative error code.
*/
int rtnl_flower_get_dst_mac(struct rtnl_cls *cls, unsigned char *mac,
                            unsigned char *mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(f->cf_mask & FLOWER_ATTR_DST_MAC))
		return -NLE_MISSING_ATTR;

	if (mac)
		memcpy(mac, f->cf_dst_mac, ETH_ALEN);

	if (mask)
		memcpy(mask, f->cf_dst_mac_mask, ETH_ALEN);

	return 0;
}

/**
 * Set source mac address for flower classifier
 * @arg cls		Flower classifier.
 * @arg mac		source mac address
 * @arg mask		mask for mac address
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_src_mac(struct rtnl_cls *cls, unsigned char *mac,
                            unsigned char *mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (mac) {
		memcpy(f->cf_src_mac, mac, ETH_ALEN);
		f->cf_mask |= FLOWER_ATTR_SRC_MAC;

		if (mask) {
			memcpy(f->cf_src_mac_mask, mask, ETH_ALEN);
			f->cf_mask |= FLOWER_ATTR_SRC_MAC_MASK;
		}

		return 0;
	}

	return -NLE_FAILURE;
}

/**
 * Get source mac address for flower classifier
 * @arg cls		Flower classifier.
 * @arg mac		source mac address
 * @arg mask		mask for mac address
 * @return 0 on success or a negative error code.
*/
int rtnl_flower_get_src_mac(struct rtnl_cls *cls, unsigned char *mac,
                            unsigned char *mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(f->cf_mask & FLOWER_ATTR_SRC_MAC))
		return -NLE_MISSING_ATTR;

	if (mac)
		memcpy(mac, f->cf_src_mac, ETH_ALEN);

	if (mask)
		memcpy(mask, f->cf_src_mac_mask, ETH_ALEN);

	return 0;
}

/**
 * Set dscp value for flower classifier
 * @arg cls		Flower classifier.
 * @arg dscp		dscp value
 * @arg mask		mask for dscp value
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_ip_dscp(struct rtnl_cls *cls, uint8_t dscp, uint8_t mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (dscp > FLOWER_DSCP_MAX)
		return -NLE_RANGE;

	if (mask > FLOWER_DSCP_MASK_MAX)
		return -NLE_RANGE;

	f->cf_ip_dscp = dscp;
	f->cf_mask |= FLOWER_ATTR_IP_DSCP;

	if (mask) {
		f->cf_ip_dscp_mask = mask;
		f->cf_mask |= FLOWER_ATTR_IP_DSCP_MASK;
	}

	return 0;
}

/**
 * Get dscp value for flower classifier
 * @arg cls		Flower classifier.
 * @arg dscp		dscp value
 * @arg mask		mask for dscp value
 * @return 0 on success or a negative error code.
*/
int rtnl_flower_get_ip_dscp(struct rtnl_cls *cls, uint8_t *dscp, uint8_t *mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(f->cf_mask & FLOWER_ATTR_IP_DSCP))
		return -NLE_MISSING_ATTR;

	*dscp = f->cf_ip_dscp;
	*mask = f->cf_ip_dscp_mask;

	return 0;
}

/**
 * Set IPv4 source address for flower classifier
 * @arg cls		Flower classifier.
 * @arg addr		IPv4 source address
 * @arg mask		mask for IPv4 source address
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_ipv4_src(struct rtnl_cls *cls, in_addr_t addr,
			     in_addr_t mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (addr) {
		f->cf_ipv4_src = addr;
		f->cf_mask |= FLOWER_ATTR_IPV4_SRC;

		if (mask) {
			f->cf_ipv4_src_mask = mask;
			f->cf_mask |= FLOWER_ATTR_IPV4_SRC_MASK;
		}

		return 0;
	}

	return -NLE_FAILURE;
}

/**
 * Get IPv4 source address for flower classifier
 * @arg cls		Flower classifier.
 * @arg addr		IPv4 source address
 * @arg mask		mask for IPv4 source address
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_get_ipv4_src(struct rtnl_cls *cls, in_addr_t *out_addr,
			     in_addr_t *out_mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(f->cf_mask & FLOWER_ATTR_IPV4_SRC))
		return -NLE_MISSING_ATTR;

	if (out_addr)
		*out_addr = f->cf_ipv4_src;

	if (out_mask) {
		if (f->cf_mask & FLOWER_ATTR_IPV4_SRC_MASK)
			*out_mask = f->cf_ipv4_src_mask;
		else
			*out_mask = 0xffffffff;
	}

	return 0;
}

/**
 * Set IPv4 destination address for flower classifier
 * @arg cls		Flower classifier.
 * @arg addr		IPv4 destination address
 * @arg mask		mask for IPv4 destination address
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_ipv4_dst(struct rtnl_cls *cls, in_addr_t addr,
			     in_addr_t mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (addr) {
		f->cf_ipv4_dst = addr;
		f->cf_mask |= FLOWER_ATTR_IPV4_DST;

		if (mask) {
			f->cf_ipv4_dst_mask = mask;
			f->cf_mask |= FLOWER_ATTR_IPV4_DST_MASK;
		}

		return 0;
	}

	return -NLE_FAILURE;
}

/**
 * Get IPv4 destination address for flower classifier
 * @arg cls		Flower classifier.
 * @arg addr		IPv4 destination address
 * @arg mask		mask for IPv4 destination address
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_get_ipv4_dst(struct rtnl_cls *cls, in_addr_t *out_addr,
			     in_addr_t *out_mask)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return -NLE_INVAL;

	if (!(f->cf_mask & FLOWER_ATTR_IPV4_DST))
		return -NLE_MISSING_ATTR;

	if (out_addr)
		*out_addr = f->cf_ipv4_dst;

	if (out_mask) {
		if (f->cf_mask & FLOWER_ATTR_IPV4_DST_MASK)
			*out_mask = f->cf_ipv4_dst_mask;
		else
			*out_mask = 0xffffffff;
	}

	return 0;
}

/**
 * Append action for flower classifier
 * @arg cls		Flower classifier.
 * @arg act		action to append
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_append_action(struct rtnl_cls *cls, struct rtnl_act *act)
{
	struct rtnl_flower *f;
	int err;

	if (!act)
		return 0;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if ((err = _rtnl_act_append_get(&f->cf_act, act)) < 0)
		return err;

	f->cf_mask |= FLOWER_ATTR_ACTION;
	return 0;
}

/**
 * Delete action from flower classifier
 * @arg cls		Flower classifier.
 * @arg act		action to delete
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_del_action(struct rtnl_cls *cls, struct rtnl_act *act)
{
	struct rtnl_flower *f;
	int ret;

	if (!act)
		return 0;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	if (!(f->cf_mask & FLOWER_ATTR_ACTION))
		return -NLE_INVAL;

	ret = rtnl_act_remove(&f->cf_act, act);
	if (ret)
		return ret;

	if (!f->cf_act)
		f->cf_mask &= ~FLOWER_ATTR_ACTION;
	rtnl_act_put(act);

	return 0;
}

/**
 * Get action from flower classifier
 * @arg cls		Flower classifier.
 * @return action on success or NULL on error.
 */
struct rtnl_act* rtnl_flower_get_action(struct rtnl_cls *cls)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data_peek(TC_CAST(cls))))
		return NULL;

	if (!(f->cf_mask & FLOWER_ATTR_ACTION))
		return NULL;

	rtnl_act_get(f->cf_act);

	return f->cf_act;
}

/**
 * Set flags for flower classifier
 * @arg cls		Flower classifier.
 * @arg flags		(TCA_CLS_FLAGS_SKIP_HW | TCA_CLS_FLAGS_SKIP_SW)
 * @return 0 on success or a negative error code.
 */
int rtnl_flower_set_flags(struct rtnl_cls *cls, int flags)
{
	struct rtnl_flower *f;

	if (!(f = rtnl_tc_data(TC_CAST(cls))))
		return -NLE_NOMEM;

	f->cf_flags = flags;
	f->cf_mask |= FLOWER_ATTR_FLAGS;

	return 0;
}

/** @} */

static struct rtnl_tc_ops flower_ops = {
	.to_kind                = "flower",
	.to_type                = RTNL_TC_TYPE_CLS,
	.to_size                = sizeof(struct rtnl_flower),
	.to_msg_parser          = flower_msg_parser,
	.to_free_data           = flower_free_data,
	.to_clone               = flower_clone,
	.to_msg_fill            = flower_msg_fill,
	.to_dump = {
	    [NL_DUMP_DETAILS]   = flower_dump_details,
	},
};

static void _nl_init flower_init(void)
{
	rtnl_tc_register(&flower_ops);
}

static void _nl_exit flower_exit(void)
{
	rtnl_tc_unregister(&flower_ops);
}
