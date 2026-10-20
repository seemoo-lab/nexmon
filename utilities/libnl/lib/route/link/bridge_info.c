/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2022 MaxLinear, Inc.
 */

/**
 * @ingroup link
 * @defgroup bridge Bridging
 *
 * @details
 * @{
 */

#include "nl-default.h"

#include <linux/if_bridge.h>

#include <netlink/route/link/bridge_info.h>

#include "nl-route.h"
#include "link-api.h"

#define BRIDGE_ATTR_VLAN_FILTERING (1 << 0)
#define BRIDGE_ATTR_VLAN_PROTOCOL (1 << 1)
#define BRIDGE_ATTR_VLAN_STATS_ENABLED (1 << 2)
#define BRIDGE_ATTR_AGEING_TIME (1 << 3)
#define BRIDGE_ATTR_VLAN_DEFAULT_PVID (1 << 4)
#define BRIDGE_ATTR_NF_CALL_IPTABLES (1 << 5)
#define BRIDGE_ATTR_NF_CALL_IP6TABLES (1 << 6)
#define BRIDGE_ATTR_NF_CALL_ARPTABLES (1 << 7)
#define BRIDGE_ATTR_STP_STATE (1 << 8)
#define BRIDGE_ATTR_MCAST_ROUTER (1 << 9)
#define BRIDGE_ATTR_MCAST_SNOOPING (1 << 10)
#define BRIDGE_ATTR_BOOLOPT (1 << 11)

struct bridge_info {
	uint32_t ce_mask; /* to support attr macros */
	uint32_t b_ageing_time;
	uint16_t b_vlan_protocol;
	uint16_t b_vlan_default_pvid;
	uint8_t b_vlan_filtering;
	uint8_t b_vlan_stats_enabled;
	uint8_t b_nf_call_iptables;
	uint8_t b_nf_call_ip6tables;
	uint8_t b_nf_call_arptables;
	uint32_t b_stp_state;
	uint8_t b_mcast_router;
	uint8_t b_mcast_snooping;
	struct br_boolopt_multi b_boolopts;
};

static const struct nla_policy bi_attrs_policy[IFLA_BR_MAX + 1] = {
	[IFLA_BR_AGEING_TIME] = { .type = NLA_U32 },
	[IFLA_BR_VLAN_DEFAULT_PVID] = { .type = NLA_U16 },
	[IFLA_BR_VLAN_FILTERING] = { .type = NLA_U8 },
	[IFLA_BR_VLAN_PROTOCOL] = { .type = NLA_U16 },
	[IFLA_BR_VLAN_STATS_ENABLED] = { .type = NLA_U8 },
	[IFLA_BR_NF_CALL_IPTABLES] = { .type = NLA_U8 },
	[IFLA_BR_NF_CALL_IP6TABLES] = { .type = NLA_U8 },
	[IFLA_BR_NF_CALL_ARPTABLES] = { .type = NLA_U8 },
	[IFLA_BR_STP_STATE] = { .type = NLA_U32 },
	[IFLA_BR_MCAST_ROUTER] = { .type = NLA_U8 },
	[IFLA_BR_MCAST_SNOOPING] = { .type = NLA_U8 },
	[IFLA_BR_MULTI_BOOLOPT] = { .type = NLA_BINARY,
				    .minlen = sizeof(struct br_boolopt_multi),
				    .maxlen = sizeof(struct br_boolopt_multi) },
};

static inline struct bridge_info *bridge_info(struct rtnl_link *link)
{
	return link->l_info;
}

static int bridge_info_alloc(struct rtnl_link *link)
{
	struct bridge_info *bi;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*bi));
	else {
		bi = calloc(1, sizeof(*bi));
		if (!bi)
			return -NLE_NOMEM;

		link->l_info = bi;
	}

	return 0;
}

static int bridge_info_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct bridge_info *bi_dst, *bi_src = src->l_info;
	int err;

	_nl_assert(bi_src);

	err = bridge_info_alloc(dst);
	if (err)
		return err;

	bi_dst = dst->l_info;

	_nl_assert(bi_dst);

	*bi_dst = *bi_src;

	return 0;
}

static int bridge_info_parse(struct rtnl_link *link, struct nlattr *data,
			     struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_BR_MAX + 1];
	struct bridge_info *bi;
	int err;

	NL_DBG(3, "Parsing Bridge link info\n");

	if ((err = nla_parse_nested(tb, IFLA_BR_MAX, data, bi_attrs_policy)) <
	    0)
		return err;

	if ((err = bridge_info_alloc(link)) < 0)
		return err;

	bi = link->l_info;

	if (tb[IFLA_BR_AGEING_TIME]) {
		bi->b_ageing_time = nla_get_u32(tb[IFLA_BR_AGEING_TIME]);
		bi->ce_mask |= BRIDGE_ATTR_AGEING_TIME;
	}

	if (tb[IFLA_BR_VLAN_DEFAULT_PVID]) {
		bi->b_vlan_default_pvid =
			nla_get_u16(tb[IFLA_BR_VLAN_DEFAULT_PVID]);
		bi->ce_mask |= BRIDGE_ATTR_VLAN_DEFAULT_PVID;
	}

	if (tb[IFLA_BR_VLAN_FILTERING]) {
		bi->b_vlan_filtering = nla_get_u8(tb[IFLA_BR_VLAN_FILTERING]);
		bi->ce_mask |= BRIDGE_ATTR_VLAN_FILTERING;
	}

	if (tb[IFLA_BR_VLAN_PROTOCOL]) {
		bi->b_vlan_protocol =
			ntohs(nla_get_u16(tb[IFLA_BR_VLAN_PROTOCOL]));
		bi->ce_mask |= BRIDGE_ATTR_VLAN_PROTOCOL;
	}

	if (tb[IFLA_BR_VLAN_STATS_ENABLED]) {
		bi->b_vlan_stats_enabled =
			nla_get_u8(tb[IFLA_BR_VLAN_STATS_ENABLED]);
		bi->ce_mask |= BRIDGE_ATTR_VLAN_STATS_ENABLED;
	}

	if (tb[IFLA_BR_NF_CALL_IPTABLES]) {
		bi->b_nf_call_iptables =
			nla_get_u8(tb[IFLA_BR_NF_CALL_IPTABLES]);
		bi->ce_mask |= BRIDGE_ATTR_NF_CALL_IPTABLES;
	}

	if (tb[IFLA_BR_NF_CALL_IP6TABLES]) {
		bi->b_nf_call_ip6tables =
			nla_get_u8(tb[IFLA_BR_NF_CALL_IP6TABLES]);
		bi->ce_mask |= BRIDGE_ATTR_NF_CALL_IP6TABLES;
	}

	if (tb[IFLA_BR_NF_CALL_ARPTABLES]) {
		bi->b_nf_call_arptables =
			nla_get_u8(tb[IFLA_BR_NF_CALL_ARPTABLES]);
		bi->ce_mask |= BRIDGE_ATTR_NF_CALL_ARPTABLES;
	}

	if (tb[IFLA_BR_STP_STATE]) {
		bi->b_stp_state = nla_get_u32(tb[IFLA_BR_STP_STATE]);
		bi->ce_mask |= BRIDGE_ATTR_STP_STATE;
	}

	if (tb[IFLA_BR_MCAST_ROUTER]) {
		bi->b_mcast_router = nla_get_u8(tb[IFLA_BR_MCAST_ROUTER]);
		bi->ce_mask |= BRIDGE_ATTR_MCAST_ROUTER;
	}

	if (tb[IFLA_BR_MCAST_SNOOPING]) {
		bi->b_mcast_snooping = nla_get_u8(tb[IFLA_BR_MCAST_SNOOPING]);
		bi->ce_mask |= BRIDGE_ATTR_MCAST_SNOOPING;
	}

	if (tb[IFLA_BR_MULTI_BOOLOPT]) {
		nla_memcpy(&bi->b_boolopts, tb[IFLA_BR_MULTI_BOOLOPT],
			   sizeof(bi->b_boolopts));
		bi->ce_mask |= BRIDGE_ATTR_BOOLOPT;
	}

	return 0;
}

static int bridge_info_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct bridge_info *bi = link->l_info;
	struct nlattr *data;

	data = nla_nest_start(msg, IFLA_INFO_DATA);
	if (!data)
		return -NLE_MSGSIZE;

	if (bi->ce_mask & BRIDGE_ATTR_AGEING_TIME)
		NLA_PUT_U32(msg, IFLA_BR_AGEING_TIME, bi->b_ageing_time);

	if (bi->ce_mask & BRIDGE_ATTR_VLAN_FILTERING)
		NLA_PUT_U8(msg, IFLA_BR_VLAN_FILTERING, bi->b_vlan_filtering);

	if (bi->ce_mask & BRIDGE_ATTR_VLAN_DEFAULT_PVID)
		NLA_PUT_U16(msg, IFLA_BR_VLAN_DEFAULT_PVID,
			    bi->b_vlan_default_pvid);

	if (bi->ce_mask & BRIDGE_ATTR_VLAN_PROTOCOL)
		NLA_PUT_U16(msg, IFLA_BR_VLAN_PROTOCOL,
			    htons(bi->b_vlan_protocol));

	if (bi->ce_mask & BRIDGE_ATTR_VLAN_STATS_ENABLED)
		NLA_PUT_U8(msg, IFLA_BR_VLAN_STATS_ENABLED,
			   bi->b_vlan_stats_enabled);

	if (bi->ce_mask & BRIDGE_ATTR_NF_CALL_IPTABLES)
		NLA_PUT_U8(msg, IFLA_BR_NF_CALL_IPTABLES,
			   bi->b_nf_call_iptables);

	if (bi->ce_mask & BRIDGE_ATTR_NF_CALL_IP6TABLES)
		NLA_PUT_U8(msg, IFLA_BR_NF_CALL_IP6TABLES,
			   bi->b_nf_call_ip6tables);

	if (bi->ce_mask & BRIDGE_ATTR_NF_CALL_ARPTABLES)
		NLA_PUT_U8(msg, IFLA_BR_NF_CALL_ARPTABLES,
			   bi->b_nf_call_arptables);

	if (bi->ce_mask & BRIDGE_ATTR_STP_STATE)
		NLA_PUT_U32(msg, IFLA_BR_STP_STATE, bi->b_stp_state);

	if (bi->ce_mask & BRIDGE_ATTR_MCAST_ROUTER)
		NLA_PUT_U8(msg, IFLA_BR_MCAST_ROUTER, bi->b_mcast_router);

	if (bi->ce_mask & BRIDGE_ATTR_MCAST_SNOOPING)
		NLA_PUT_U8(msg, IFLA_BR_MCAST_SNOOPING, bi->b_mcast_snooping);

	if (bi->ce_mask & BRIDGE_ATTR_BOOLOPT)
		NLA_PUT(msg, IFLA_BR_MULTI_BOOLOPT, sizeof(bi->b_boolopts),
			&bi->b_boolopts);

	nla_nest_end(msg, data);
	return 0;

nla_put_failure:
	nla_nest_cancel(msg, data);
	return -NLE_MSGSIZE;
}

static void bridge_info_free(struct rtnl_link *link)
{
	_nl_clear_free(&link->l_info);
}

static int bridge_info_compare(struct rtnl_link *link_a,
			       struct rtnl_link *link_b, int flags)
{
	struct bridge_info *a = link_a->l_info;
	struct bridge_info *b = link_b->l_info;
	uint32_t attrs = flags & LOOSE_COMPARISON ? b->ce_mask : ~0u;
	int diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(BRIDGE_ATTR_VLAN_FILTERING,
		      a->b_vlan_filtering != b->b_vlan_filtering);
	diff |= _DIFF(BRIDGE_ATTR_VLAN_PROTOCOL,
		      a->b_vlan_protocol != b->b_vlan_protocol);
	diff |= _DIFF(BRIDGE_ATTR_VLAN_STATS_ENABLED,
		      a->b_vlan_stats_enabled != b->b_vlan_stats_enabled);
	diff |= _DIFF(BRIDGE_ATTR_AGEING_TIME,
		      a->b_ageing_time != b->b_ageing_time);
	diff |= _DIFF(BRIDGE_ATTR_VLAN_DEFAULT_PVID,
		      a->b_vlan_default_pvid != b->b_vlan_default_pvid);
	diff |= _DIFF(BRIDGE_ATTR_NF_CALL_IPTABLES,
		      a->b_nf_call_iptables != b->b_nf_call_iptables);
	diff |= _DIFF(BRIDGE_ATTR_NF_CALL_IP6TABLES,
		      a->b_nf_call_ip6tables != b->b_nf_call_ip6tables);
	diff |= _DIFF(BRIDGE_ATTR_NF_CALL_ARPTABLES,
		      a->b_nf_call_arptables != b->b_nf_call_arptables);
	diff |= _DIFF(BRIDGE_ATTR_STP_STATE, a->b_stp_state != b->b_stp_state);
	diff |= _DIFF(BRIDGE_ATTR_MCAST_ROUTER,
		      a->b_mcast_router != b->b_mcast_router);
	diff |= _DIFF(BRIDGE_ATTR_MCAST_SNOOPING,
		      a->b_mcast_snooping != b->b_mcast_snooping);

	if (flags & LOOSE_COMPARISON)
		diff |= _DIFF(
			BRIDGE_ATTR_BOOLOPT,
			(b->b_boolopts.optmask & ~a->b_boolopts.optmask) ||
				((a->b_boolopts.optval ^ b->b_boolopts.optval) &
				 b->b_boolopts.optmask));
	else
		diff |= _DIFF(
			BRIDGE_ATTR_BOOLOPT,
			(a->b_boolopts.optmask != b->b_boolopts.optmask) ||
				(a->b_boolopts.optval != b->b_boolopts.optval));
#undef _DIFF

	return diff;
}

static struct rtnl_link_info_ops bridge_info_ops = {
	.io_name = "bridge",
	.io_alloc = bridge_info_alloc,
	.io_clone = bridge_info_clone,
	.io_parse = bridge_info_parse,
	.io_put_attrs = bridge_info_put_attrs,
	.io_free = bridge_info_free,
	.io_compare = bridge_info_compare,
};

#define IS_BRIDGE_INFO_ASSERT(link)                                                      \
	do {                                                                             \
		if ((link)->l_info_ops != &bridge_info_ops) {                            \
			APPBUG("Link is not a bridge link. Set type \"bridge\" first."); \
		}                                                                        \
	} while (0)

/**
 * Set ageing time for dynamic forwarding entries
 * @arg link		Link object of type bridge
 * @arg ageing_time	Interval to set.
 *
 * @see rtnl_link_bridge_get_ageing_time()
 *
 * @return void
 */
void rtnl_link_bridge_set_ageing_time(struct rtnl_link *link,
				      uint32_t ageing_time)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_ageing_time = ageing_time;

	bi->ce_mask |= BRIDGE_ATTR_AGEING_TIME;
}

/**
 * Get ageing time for dynamic forwarding entries
 * @arg link		Link object of type bridge
 * @arg ageing_time	Output argument.
 *
 * @see rtnl_link_bridge_set_ageing_time()
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_ageing_time(struct rtnl_link *link,
				     uint32_t *ageing_time)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_AGEING_TIME))
		return -NLE_NOATTR;

	if (!ageing_time)
		return -NLE_INVAL;

	*ageing_time = bi->b_ageing_time;

	return 0;
}

/**
 * Set VLAN filtering flag
 * @arg link		Link object of type bridge
 * @arg vlan_filtering	VLAN_filtering boolean flag to set.
 *
 * @see rtnl_link_bridge_get_vlan_filtering()
 *
 * @return void
 */
void rtnl_link_bridge_set_vlan_filtering(struct rtnl_link *link,
					 uint8_t vlan_filtering)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_vlan_filtering = vlan_filtering;

	bi->ce_mask |= BRIDGE_ATTR_VLAN_FILTERING;
}

/**
 * Get VLAN filtering flag
 * @arg link		Link object of type bridge
 * @arg vlan_filtering	Output argument.
 *
 * @see rtnl_link_bridge_set_vlan_filtering()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_vlan_filtering(struct rtnl_link *link,
					uint8_t *vlan_filtering)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_VLAN_FILTERING))
		return -NLE_NOATTR;

	if (!vlan_filtering)
		return -NLE_INVAL;

	*vlan_filtering = bi->b_vlan_filtering;
	return 0;
}

/**
 * Set VLAN protocol
 * @arg link		Link object of type bridge
 * @arg vlan_protocol	VLAN protocol to set. The protocol
 *   numbers is in host byte order.
 *
 * @see rtnl_link_bridge_get_vlan_protocol()
 *
 * @return void
 */
void rtnl_link_bridge_set_vlan_protocol(struct rtnl_link *link,
					uint16_t vlan_protocol)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_vlan_protocol = vlan_protocol;

	bi->ce_mask |= BRIDGE_ATTR_VLAN_PROTOCOL;
}

/**
 * Get VLAN protocol
 * @arg link		Link object of type bridge
 * @arg vlan_protocol	Output argument. The protocol number is in host byte order.
 *
 * @see rtnl_link_bridge_set_vlan_protocol()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_vlan_protocol(struct rtnl_link *link,
				       uint16_t *vlan_protocol)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_VLAN_PROTOCOL))
		return -NLE_NOATTR;

	if (!vlan_protocol)
		return -NLE_INVAL;

	*vlan_protocol = bi->b_vlan_protocol;

	return 0;
}

/**
 * Set VLAN default pvid
 * @arg link			Link object of type bridge
 * @arg default pvid	VLAN default pvid to set.
 *
 * @see rtnl_link_bridge_get_vlan_default_pvid()
 *
 * @return void
 */
void rtnl_link_bridge_set_vlan_default_pvid(struct rtnl_link *link,
					    uint16_t default_pvid)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_vlan_default_pvid = default_pvid;

	bi->ce_mask |= BRIDGE_ATTR_VLAN_DEFAULT_PVID;
}

/**
 * Get VLAN default pvid
 * @arg link			Link object of type bridge
 * @arg default_pvid	Output argument.
 *
 * @see rtnl_link_bridge_set_vlan_default_pvid()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_vlan_default_pvid(struct rtnl_link *link,
					   uint16_t *default_pvid)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_VLAN_DEFAULT_PVID))
		return -NLE_NOATTR;

	if (!default_pvid)
		return -NLE_INVAL;

	*default_pvid = bi->b_vlan_default_pvid;

	return 0;
}

/**
 * Set VLAN stats enabled flag
 * @arg link		Link object of type bridge
 * @arg vlan_stats_enabled	VLAN stats enabled flag to set
 *
 * @see rtnl_link_bridge_get_vlan_stats_enabled()
 *
 * @return void
 */
void rtnl_link_bridge_set_vlan_stats_enabled(struct rtnl_link *link,
					     uint8_t vlan_stats_enabled)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_vlan_stats_enabled = vlan_stats_enabled;

	bi->ce_mask |= BRIDGE_ATTR_VLAN_STATS_ENABLED;
}

/**
 * Get VLAN stats enabled flag
 * @arg link		Link object of type bridge
 * @arg vlan_stats_enabled	Output argument.
 *
 * @see rtnl_link_bridge_set_vlan_stats_enabled()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_vlan_stats_enabled(struct rtnl_link *link,
					    uint8_t *vlan_stats_enabled)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_VLAN_STATS_ENABLED))
		return -NLE_NOATTR;

	if (!vlan_stats_enabled)
		return -NLE_INVAL;

	*vlan_stats_enabled = bi->b_vlan_stats_enabled;

	return 0;
}

/**
 * Set call enabled flag for passing IPv4 traffic to iptables
 * @arg link		Link object of type bridge
 * @arg call_enabled	call enabled boolean flag to set.
 *
 * @see rtnl_link_bridge_get_nf_call_iptables()
 *
 * @return void
 */
void rtnl_link_bridge_set_nf_call_iptables(struct rtnl_link *link,
					   uint8_t call_enabled)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_nf_call_iptables = call_enabled;

	bi->ce_mask |= BRIDGE_ATTR_NF_CALL_IPTABLES;
}

/**
 * Get call enabled flag for passing IPv4 traffic to iptables
 * @arg link		Link object of type bridge
 * @arg call_enabled	Output argument.
 *
 * @see rtnl_link_bridge_set_nf_call_iptables()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_nf_call_iptables(struct rtnl_link *link,
					  uint8_t *call_enabled)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_NF_CALL_IPTABLES))
		return -NLE_NOATTR;

	if (!call_enabled)
		return -NLE_INVAL;

	*call_enabled = bi->b_nf_call_iptables;

	return 0;
}

/**
 * Set call enabled flag for passing IPv6 traffic to ip6tables
 * @arg link		Link object of type bridge
 * @arg call_enabled	call enabled boolean flag to set.
 *
 * @see rtnl_link_bridge_get_nf_call_ip6tables()
 *
 * @return void
 */
void rtnl_link_bridge_set_nf_call_ip6tables(struct rtnl_link *link,
					    uint8_t call_enabled)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_nf_call_ip6tables = call_enabled;

	bi->ce_mask |= BRIDGE_ATTR_NF_CALL_IP6TABLES;
}

/**
 * Get call enabled flag for passing IPv6 traffic to iptables
 * @arg link		Link object of type bridge
 * @arg call_enabled	Output argument.
 *
 * @see rtnl_link_bridge_set_nf_call_ip6tables()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_nf_call_ip6tables(struct rtnl_link *link,
					   uint8_t *call_enabled)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_NF_CALL_IP6TABLES))
		return -NLE_NOATTR;

	if (!call_enabled)
		return -NLE_INVAL;

	*call_enabled = bi->b_nf_call_ip6tables;

	return 0;
}

/**
 * Set call enabled flag for passing ARP traffic to arptables
 * @arg link		Link object of type bridge
 * @arg call_enabled	call enabled boolean flag to set.
 *
 * @see rtnl_link_bridge_get_nf_call_arptables()
 *
 * @return void
 */
void rtnl_link_bridge_set_nf_call_arptables(struct rtnl_link *link,
					    uint8_t call_enabled)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_nf_call_arptables = call_enabled;

	bi->ce_mask |= BRIDGE_ATTR_NF_CALL_ARPTABLES;
}

/**
 * Get call enabled flag for passing ARP traffic to arptables
 * @arg link		Link object of type bridge
 * @arg call_enabled	Output argument.
 *
 * @see rtnl_link_bridge_set_nf_call_arptables()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_nf_call_arptables(struct rtnl_link *link,
					   uint8_t *call_enabled)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_NF_CALL_ARPTABLES))
		return -NLE_NOATTR;

	if (!call_enabled)
		return -NLE_INVAL;

	*call_enabled = bi->b_nf_call_arptables;

	return 0;
}

/**
 * Set STP state
 * @arg link		Link object of type bridge
 * @arg stp_state	STP state to set. Typically 0 or 1.
 *
 * @see rtnl_link_bridge_get_stp_state()
 *
 * @return void
 */
void rtnl_link_bridge_set_stp_state(struct rtnl_link *link, uint32_t stp_state)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_stp_state = stp_state;

	bi->ce_mask |= BRIDGE_ATTR_STP_STATE;
}

/**
 * Get STP state
 * @arg link		Link object of type bridge
 * @arg stp_state	Output argument.
 *
 * @see rtnl_link_bridge_set_stp_state()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_stp_state(struct rtnl_link *link, uint32_t *stp_state)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_STP_STATE))
		return -NLE_NOATTR;

	if (!stp_state)
		return -NLE_INVAL;

	*stp_state = bi->b_stp_state;
	return 0;
}

/**
 * Set multicast router type
 * @arg link	Link object of type bridge
 * @arg type	Multicast router type (MDB_RTR_TYPE_*)
 *
 * @see rtnl_link_bridge_get_mcast_router()
 *
 * @return void
 */
void rtnl_link_bridge_set_mcast_router(struct rtnl_link *link, uint8_t type)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_mcast_router = type;

	bi->ce_mask |= BRIDGE_ATTR_MCAST_ROUTER;
}

/**
 * Get multicast router type
 * @arg link	Link object of type bridge
 * @arg type	Output argument.
 *
 * @see rtnl_link_bridge_set_mcast_router()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_mcast_router(struct rtnl_link *link, uint8_t *type)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_MCAST_ROUTER))
		return -NLE_NOATTR;

	if (!type)
		return -NLE_INVAL;

	*type = bi->b_mcast_router;
	return 0;
}

/**
 * Set multicast snooping
 * @arg link	Link object of type bridge
 * @arg value	Value to set. Typically 0 or 1.
 *
 * @see rtnl_link_bridge_get_mcast_snooping()
 *
 * @return void
 */
void rtnl_link_bridge_set_mcast_snooping(struct rtnl_link *link, uint8_t value)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	bi->b_mcast_snooping = value;

	bi->ce_mask |= BRIDGE_ATTR_MCAST_SNOOPING;
}

/**
 * Get multicast snooping value
 * @arg link	Link object of type bridge
 * @arg value	Output argument.
 *
 * @see rtnl_link_bridge_set_mcast_snooping()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_mcast_snooping(struct rtnl_link *link, uint8_t *value)
{
	struct bridge_info *bi = bridge_info(link);

	IS_BRIDGE_INFO_ASSERT(link);

	if (!(bi->ce_mask & BRIDGE_ATTR_MCAST_SNOOPING))
		return -NLE_NOATTR;

	if (!value)
		return -NLE_INVAL;

	*value = bi->b_mcast_snooping;
	return 0;
}

/**
 * Set a the value of a boolopt
 * @arg link	Link object of type bridge
 * @arg opt	Option to modify (BR_BOOLOPT_*)
 * @arg value	Value to set the option to. 0 or 1.
 *
 * @see rtnl_link_bridge_get_boolopt()
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_set_boolopt(struct rtnl_link *link, int opt, int value)
{
	struct bridge_info *bi = bridge_info(link);
	uint32_t mask;

	IS_BRIDGE_INFO_ASSERT(link);

	if (opt < 0 || opt >= 32 || !(value == 0 || value == 1))
		return -NLE_INVAL;

	mask = 1ul << opt;

	if (value)
		bi->b_boolopts.optval |= mask;
	else
		bi->b_boolopts.optval &= ~mask;

	bi->b_boolopts.optmask |= mask;
	bi->ce_mask |= BRIDGE_ATTR_BOOLOPT;

	return 0;
}

/**
 * Get the value of a boolopt
 * @arg link	Link object of type bridge
 * @arg opt	Option to get (BR_BOOLOPT_*).
 *
 * @see rtnl_link_bridge_set_boolopt()
 *
 * @return The value of the boolopt (0 or 1), otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bridge_get_boolopt(struct rtnl_link *link, int opt)
{
	struct bridge_info *bi = bridge_info(link);
	uint32_t mask;

	IS_BRIDGE_INFO_ASSERT(link);

	if (opt < 0 || opt >= 32)
		return -NLE_INVAL;

	mask = 1ul << opt;

	if (!(bi->ce_mask & BRIDGE_ATTR_BOOLOPT) ||
	    !(bi->b_boolopts.optmask & mask))
		return -NLE_NOATTR;

	return !!(bi->b_boolopts.optval & mask);
}

static void _nl_init bridge_info_init(void)
{
	rtnl_link_register_info(&bridge_info_ops);
}

static void _nl_exit bridge_info_exit(void)
{
	rtnl_link_unregister_info(&bridge_info_ops);
}

/** @} */
