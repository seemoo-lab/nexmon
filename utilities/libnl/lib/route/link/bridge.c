/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2010-2013 Thomas Graf <tgraf@suug.ch>
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
#include <linux/rtnetlink.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/bridge.h>
#include <netlink/list.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-priv-static-route/nl-priv-static-route.h"

#define VLAN_VID_MASK           0x0fff /* VLAN Identifier */

/** @cond SKIP */
#define BRIDGE_ATTR_PORT_STATE		(1UL << 0)
#define BRIDGE_ATTR_PRIORITY		(1UL << 1)
#define BRIDGE_ATTR_COST		(1UL << 2)
#define BRIDGE_ATTR_FLAGS		(1UL << 3)
#define BRIDGE_ATTR_PORT_VLAN           (1UL << 4)
#define BRIDGE_ATTR_HWMODE		(1UL << 5)
#define BRIDGE_ATTR_CONFIG_MODE		(1UL << 6)
#define BRIDGE_ATTR_MST			(1UL << 7)

#define PRIV_FLAG_NEW_ATTRS		(1UL << 0)

struct bridge_data
{
	uint8_t				b_port_state;
	uint8_t				b_priv_flags; /* internal flags */
	uint16_t			b_hwmode;
	uint16_t			b_priority;
	uint16_t			b_config_mode;
	uint32_t			b_cost;
	uint32_t			b_flags;
	uint32_t			b_flags_mask;
	uint32_t			ce_mask; /* HACK to support attr macros */
	struct rtnl_link_bridge_vlan	vlan_info;
	struct nl_list_head		mst_list;
};

struct mst_state_entry {
	struct nl_list_head list_node;
	uint16_t msti; /* unique within a list */
	uint8_t state;
};

static void set_bit(unsigned nr, uint32_t *addr)
{
	if (nr < RTNL_LINK_BRIDGE_VLAN_BITMAP_MAX)
		addr[nr / 32] |= (((uint32_t) 1) << (nr % 32));
}

static void unset_bit(unsigned nr, uint32_t *addr)
{
	if (nr < RTNL_LINK_BRIDGE_VLAN_BITMAP_MAX)
		addr[nr / 32] &= ~(((uint32_t) 1) << (nr % 32));
}

static bool vlan_id_untagged(struct rtnl_link_bridge_vlan *vlan_info, uint16_t vid)
{
	uint32_t mask, bit;

	_nl_assert(vid / 32u < ARRAY_SIZE(vlan_info->untagged_bitmap));

	mask = vlan_info->untagged_bitmap[vid / 32];
	bit = (((uint32_t) 1) << vid % 32);

	return mask & bit;
}

static int find_next_bit(int i, uint32_t x)
{
	int j;

	if (i >= 32)
		return -1;

	/* find first bit */
	if (i < 0)
		return __builtin_ffs(x);

	/* mask off prior finds to get next */
	j = __builtin_ffs(x >> i);
	return j ? j + i : 0;
}

static struct rtnl_link_af_ops bridge_ops;

#define IS_BRIDGE_LINK_ASSERT(link) \
	if (!rtnl_link_is_bridge(link)) { \
		APPBUG("A function was expecting a link object of type bridge."); \
		return -NLE_OPNOTSUPP; \
	}

static inline struct bridge_data *bridge_data(struct rtnl_link *link)
{
	return rtnl_link_af_data(link, &bridge_ops);
}

static void *bridge_alloc(struct rtnl_link *link)
{
	struct bridge_data *bridge_data = calloc(1, sizeof(struct bridge_data));

	if (bridge_data == NULL)
		return NULL;

	nl_init_list_head(&bridge_data->mst_list);

	return bridge_data;
}

static struct mst_state_entry *mst_state_entry_alloc(void)
{
	struct mst_state_entry *entry;

	entry = calloc(1, sizeof(*entry));
	if (entry == NULL)
		return NULL;

	nl_init_list_head(&entry->list_node);

	return entry;
}

static struct mst_state_entry *mst_state_entry_create(uint16_t msti,
						      uint8_t state)
{
	struct mst_state_entry *entry;

	entry = mst_state_entry_alloc();
	if (entry == NULL)
		return NULL;

	entry->msti = msti;
	entry->state = state;

	return entry;
}

static void mst_state_entry_del(struct mst_state_entry *entry)
{
	nl_list_del(&entry->list_node);
	free(entry);
}

static void mst_list_clear(struct nl_list_head *mst_list)
{
	struct mst_state_entry *entry;
	struct mst_state_entry *entry_safe;

	nl_list_for_each_entry_safe(entry, entry_safe, mst_list, list_node)
		mst_state_entry_del(entry);
}

static void bridge_data_free(struct bridge_data *bd)
{
	mst_list_clear(&bd->mst_list);
	free(bd);
}

static void bridge_free(struct rtnl_link *link, void *data)
{
	bridge_data_free(data);
}

static struct mst_state_entry *find_mst_state_entry(struct bridge_data *bd,
						    uint16_t msti)
{
	struct mst_state_entry *entry;

	nl_list_for_each_entry(entry, &bd->mst_list, list_node) {
		if (entry->msti == msti)
			return entry;
	}

	return NULL;
}

static struct mst_state_entry *
mst_state_entry_clone(struct mst_state_entry *src)
{
	return mst_state_entry_create(src->msti, src->state);
}

static void *bridge_clone(struct rtnl_link *link, void *data)
{
	struct bridge_data *src_bd = (struct bridge_data *)data;
	struct bridge_data *dst_bd;
	struct mst_state_entry *entry;

	dst_bd = calloc(1, sizeof(*dst_bd));
	if (!dst_bd)
		return NULL;

	memcpy(dst_bd, src_bd, sizeof(*dst_bd));

	nl_init_list_head(&dst_bd->mst_list);

	nl_list_for_each_entry(entry, &src_bd->mst_list, list_node) {
		struct mst_state_entry *entry_copy =
			mst_state_entry_clone(entry);

		if (!entry_copy) {
			bridge_data_free(dst_bd);
			return NULL;
		}

		nl_list_add_tail(&entry_copy->list_node, &dst_bd->mst_list);
	}

	return dst_bd;
}

static struct nla_policy br_attrs_policy[IFLA_BRPORT_MAX+1] = {
	[IFLA_BRPORT_STATE]			= { .type = NLA_U8 },
	[IFLA_BRPORT_PRIORITY]			= { .type = NLA_U16 },
	[IFLA_BRPORT_COST]			= { .type = NLA_U32 },
	[IFLA_BRPORT_MODE]			= { .type = NLA_U8 },
	[IFLA_BRPORT_GUARD]			= { .type = NLA_U8 },
	[IFLA_BRPORT_PROTECT]			= { .type = NLA_U8 },
	[IFLA_BRPORT_FAST_LEAVE]		= { .type = NLA_U8 },
	[IFLA_BRPORT_LEARNING]			= { .type = NLA_U8 },
	[IFLA_BRPORT_LEARNING_SYNC]		= { .type = NLA_U8 },
	[IFLA_BRPORT_UNICAST_FLOOD]		= { .type = NLA_U8 },
	[IFLA_BRPORT_PROXYARP]			= { .type = NLA_U8 },
	[IFLA_BRPORT_PROXYARP_WIFI]		= { .type = NLA_U8 },
	[IFLA_BRPORT_MCAST_FLOOD]		= { .type = NLA_U8 },
	[IFLA_BRPORT_MCAST_TO_UCAST]		= { .type = NLA_U8 },
	[IFLA_BRPORT_VLAN_TUNNEL]		= { .type = NLA_U8 },
	[IFLA_BRPORT_BCAST_FLOOD]		= { .type = NLA_U8 },
	[IFLA_BRPORT_NEIGH_SUPPRESS]		= { .type = NLA_U8 },
	[IFLA_BRPORT_ISOLATED]			= { .type = NLA_U8 },
	[IFLA_BRPORT_LOCKED]			= { .type = NLA_U8 },
	[IFLA_BRPORT_MAB]			= { .type = NLA_U8 },
	[IFLA_BRPORT_NEIGH_VLAN_SUPPRESS]	= { .type = NLA_U8 },
};

static void check_flag(struct rtnl_link *link, struct nlattr *attrs[],
		       int type, int flag)
{
	if (attrs[type] && nla_get_u8(attrs[type]))
		rtnl_link_bridge_set_flags(link, flag);
}

static int bridge_parse_protinfo(struct rtnl_link *link, struct nlattr *attr,
				 void *data)
{
	struct bridge_data *bd = data;
	struct nlattr *br_attrs[IFLA_BRPORT_MAX+1];
	int err;

	/* Backwards compatibility */
	if (!nla_is_nested(attr)) {
		if (nla_len(attr) < 1)
			return -NLE_RANGE;

		bd->b_port_state = nla_get_u8(attr);
		bd->ce_mask |= BRIDGE_ATTR_PORT_STATE;

		return 0;
	}

	if ((err = nla_parse_nested(br_attrs, IFLA_BRPORT_MAX, attr,
	     br_attrs_policy)) < 0)
		return err;

	bd->b_priv_flags |= PRIV_FLAG_NEW_ATTRS;

	if (br_attrs[IFLA_BRPORT_STATE]) {
		bd->b_port_state = nla_get_u8(br_attrs[IFLA_BRPORT_STATE]);
		bd->ce_mask |= BRIDGE_ATTR_PORT_STATE;
	}

	if (br_attrs[IFLA_BRPORT_PRIORITY]) {
		bd->b_priority = nla_get_u16(br_attrs[IFLA_BRPORT_PRIORITY]);
		bd->ce_mask |= BRIDGE_ATTR_PRIORITY;
	}

	if (br_attrs[IFLA_BRPORT_COST]) {
		bd->b_cost = nla_get_u32(br_attrs[IFLA_BRPORT_COST]);
		bd->ce_mask |= BRIDGE_ATTR_COST;
	}

	check_flag(link, br_attrs, IFLA_BRPORT_MODE, RTNL_BRIDGE_HAIRPIN_MODE);
	check_flag(link, br_attrs, IFLA_BRPORT_GUARD, RTNL_BRIDGE_BPDU_GUARD);
	check_flag(link, br_attrs, IFLA_BRPORT_PROTECT, RTNL_BRIDGE_ROOT_BLOCK);
	check_flag(link, br_attrs, IFLA_BRPORT_FAST_LEAVE, RTNL_BRIDGE_FAST_LEAVE);
	check_flag(link, br_attrs, IFLA_BRPORT_UNICAST_FLOOD,
	           RTNL_BRIDGE_UNICAST_FLOOD);
	check_flag(link, br_attrs, IFLA_BRPORT_LEARNING, RTNL_BRIDGE_LEARNING);
	check_flag(link, br_attrs, IFLA_BRPORT_LEARNING_SYNC,
	           RTNL_BRIDGE_LEARNING_SYNC);
	check_flag(link, br_attrs, IFLA_BRPORT_PROXYARP, RTNL_BRIDGE_PROXYARP);
	check_flag(link, br_attrs, IFLA_BRPORT_PROXYARP_WIFI,
		   RTNL_BRIDGE_PROXYARP_WIFI);
	check_flag(link, br_attrs, IFLA_BRPORT_MCAST_FLOOD,
		   RTNL_BRIDGE_MCAST_FLOOD);
	check_flag(link, br_attrs, IFLA_BRPORT_MCAST_TO_UCAST,
		   RTNL_BRIDGE_MCAST_TO_UCAST);
	check_flag(link, br_attrs, IFLA_BRPORT_VLAN_TUNNEL,
		   RTNL_BRIDGE_VLAN_TUNNEL);
	check_flag(link, br_attrs, IFLA_BRPORT_BCAST_FLOOD,
		   RTNL_BRIDGE_BCAST_FLOOD);
	check_flag(link, br_attrs, IFLA_BRPORT_NEIGH_SUPPRESS,
		   RTNL_BRIDGE_NEIGH_SUPPRESS);
	check_flag(link, br_attrs, IFLA_BRPORT_ISOLATED, RTNL_BRIDGE_ISOLATED);
	check_flag(link, br_attrs, IFLA_BRPORT_LOCKED, RTNL_BRIDGE_LOCKED);
	check_flag(link, br_attrs, IFLA_BRPORT_MAB, RTNL_BRIDGE_MAB);
	check_flag(link, br_attrs, IFLA_BRPORT_NEIGH_VLAN_SUPPRESS,
		   RTNL_BRIDGE_NEIGH_VLAN_SUPPRESS);

	return 0;
}

static const struct nla_policy br_mst_entry_policy[IFLA_BRIDGE_MST_ENTRY_MAX + 1] = {
	[IFLA_BRIDGE_MST_ENTRY_MSTI]		= { .type = NLA_U16 },
	[IFLA_BRIDGE_MST_ENTRY_STATE]		= { .type = NLA_U8 },
};

static int bridge_parse_mst_state_entry(struct nlattr *attr,
					struct bridge_data *bd)
{
	struct nlattr *tb[IFLA_BRIDGE_MST_ENTRY_MAX + 1];
	struct mst_state_entry *new_entry;
	struct mst_state_entry *existing_entry;
	uint16_t msti;
	uint8_t state;

	if (nla_parse_nested(tb, IFLA_BRIDGE_MST_ENTRY_MAX, attr,
			     br_mst_entry_policy) < 0)
		return -EINVAL;

	if (!tb[IFLA_BRIDGE_MST_ENTRY_MSTI] ||
	    !tb[IFLA_BRIDGE_MST_ENTRY_STATE]) {
		return -EINVAL;
	}

	msti = nla_get_u16(tb[IFLA_BRIDGE_MST_ENTRY_MSTI]);
	state = nla_get_u8(tb[IFLA_BRIDGE_MST_ENTRY_STATE]);

	existing_entry = find_mst_state_entry(bd, msti);
	if (existing_entry) {
		existing_entry->state = state;
		return 0;
	}

	new_entry = mst_state_entry_create(msti, state);
	if (!new_entry)
		return -ENOMEM;

	nl_list_add_tail(&new_entry->list_node, &bd->mst_list);
	bd->ce_mask |= BRIDGE_ATTR_MST;

	return 0;
}

static int bridge_parse_mst(struct nlattr *mst_attr, struct bridge_data *bd) {
	struct nlattr *attr;
	int remaining;

	nla_for_each_nested(attr, mst_attr, remaining) {
		int err = 0;

		switch (nla_type(attr)) {
		case IFLA_BRIDGE_MST_ENTRY:
			err = bridge_parse_mst_state_entry(attr, bd);
			break;
		default:
			continue;
		}

		if (err)
			return err;
	}

	return 0;
}

static int bridge_parse_af_full(struct rtnl_link *link, struct nlattr *attr_full,
                                void *data)
{
	struct bridge_data *bd = data;
	struct bridge_vlan_info *vinfo = NULL;
	uint16_t vid_range_start = 0;
	uint16_t vid_range_flags = -1;

	struct nlattr *attr;
	int remaining;

	nla_for_each_nested(attr, attr_full, remaining) {

		if (nla_type(attr) == IFLA_BRIDGE_MODE) {
			bd->b_hwmode = nla_get_u16(attr);
			bd->ce_mask |= BRIDGE_ATTR_HWMODE;
			continue;
		} else if (nla_type(attr) == IFLA_BRIDGE_MST) {
			int err = bridge_parse_mst(attr, bd);
			if (err < 0)
				return err;
			continue;
		} else if (nla_type(attr) != IFLA_BRIDGE_VLAN_INFO)
			continue;

		if (nla_len(attr) != sizeof(struct bridge_vlan_info))
			return -EINVAL;

		vinfo = nla_data(attr);
		if (!vinfo->vid || vinfo->vid >= VLAN_VID_MASK)
			return -EINVAL;


		if (vinfo->flags & BRIDGE_VLAN_INFO_RANGE_BEGIN) {
			vid_range_start = vinfo->vid;
			vid_range_flags = (vinfo->flags ^ BRIDGE_VLAN_INFO_RANGE_BEGIN);
			continue;
		}

		if (vinfo->flags & BRIDGE_VLAN_INFO_RANGE_END) {
			/* sanity check the range flags */
			if (vid_range_flags != (vinfo->flags ^ BRIDGE_VLAN_INFO_RANGE_END)) {
				NL_DBG(1, "VLAN range flags differ; can not handle it.\n");
				return -EINVAL;
			}
		} else {
			vid_range_start = vinfo->vid;
		}

		for (; vid_range_start <= vinfo->vid; vid_range_start++) {
			if (vinfo->flags & BRIDGE_VLAN_INFO_PVID)
				bd->vlan_info.pvid = vinfo->vid;

			if (vinfo->flags & BRIDGE_VLAN_INFO_UNTAGGED)
				set_bit(vid_range_start, bd->vlan_info.untagged_bitmap);

			set_bit(vid_range_start, bd->vlan_info.vlan_bitmap);
			bd->ce_mask |= BRIDGE_ATTR_PORT_VLAN;
		}

		vid_range_flags = -1;
	}

	return 0;
}

int _nl_bridge_fill_vlan_info(struct nl_msg *msg, struct rtnl_link_bridge_vlan * vlan_info)
{
	struct bridge_vlan_info vinfo;
	int i = -1, j, k;
	int start = -1, prev = -1;
	int done;
	bool untagged = false;

	for (k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++)
	{
		int base_bit;
		uint32_t a = vlan_info->vlan_bitmap[k];

		base_bit = k * 32;
		i = -1;
		done = 0;
		while (!done)
		{
			j = find_next_bit(i, a);
			if (j > 0)
			{
				/* Skip if id equal to pvid */
				if (vlan_info->pvid != 0 && j - 1 + base_bit == vlan_info->pvid)
					goto nxt;
				/* first hit of any bit */
				if (start < 0 && prev < 0)
				{
					start = prev = j - 1 + base_bit;
					/* Start range attribute */
					untagged = vlan_id_untagged(vlan_info,start);
					vinfo.flags = BRIDGE_VLAN_INFO_RANGE_BEGIN;
					vinfo.flags |= untagged ? BRIDGE_VLAN_INFO_UNTAGGED : 0;
					vinfo.vid = start;
					goto nxt;
				}
				/* this bit is a continuation of prior bits */
				if (j - 2 + base_bit == prev)
				{
					prev++;
					/* Hit end of untagged/tagged range */
					if (untagged != vlan_id_untagged(vlan_info,prev))
					{
						/* put vlan into attributes */
						if (start == prev-1)
						{
							/* only 1 vid in range */
							vinfo.flags &= ~BRIDGE_VLAN_INFO_RANGE_BEGIN;
							NLA_PUT(msg,IFLA_BRIDGE_VLAN_INFO,sizeof(vinfo),&vinfo);
						}
						else
						{
							/* end of untagged/tagged range */
							NLA_PUT(msg,IFLA_BRIDGE_VLAN_INFO,sizeof(vinfo),&vinfo);

							vinfo.flags = BRIDGE_VLAN_INFO_RANGE_END;
							vinfo.flags |= untagged ? BRIDGE_VLAN_INFO_UNTAGGED : 0;
							vinfo.vid = prev-1;
							NLA_PUT(msg,IFLA_BRIDGE_VLAN_INFO,sizeof(vinfo),&vinfo);
						}
						/* start of new range */
						untagged = !untagged;
						vinfo.flags = BRIDGE_VLAN_INFO_RANGE_BEGIN;
						vinfo.flags |= untagged ? BRIDGE_VLAN_INFO_UNTAGGED : 0;
						vinfo.vid = prev;
					}
					goto nxt;
				}
			}
			else
				done = 1;

			if (start >= 0)
			{
				if (done && k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN - 1)
					break;

				if (vinfo.flags & BRIDGE_VLAN_INFO_RANGE_BEGIN && start != prev)
				{
					NLA_PUT(msg,IFLA_BRIDGE_VLAN_INFO,sizeof(vinfo),&vinfo);

					vinfo.flags = BRIDGE_VLAN_INFO_RANGE_END;
					vinfo.flags |= untagged ? BRIDGE_VLAN_INFO_UNTAGGED : 0;
					vinfo.vid = prev;
					NLA_PUT(msg,IFLA_BRIDGE_VLAN_INFO,sizeof(vinfo),&vinfo);
				}
				else if (start == prev)
				{
					vinfo.flags = untagged ? BRIDGE_VLAN_INFO_UNTAGGED : 0;
					vinfo.vid = start;
					NLA_PUT(msg,IFLA_BRIDGE_VLAN_INFO,sizeof(vinfo),&vinfo);
				}

				if (done)
					break;
			}
			if (j > 0)
			{
				start = prev = j - 1 + base_bit;
				untagged = vlan_id_untagged(vlan_info,start);
				vinfo.flags = BRIDGE_VLAN_INFO_RANGE_BEGIN;
				vinfo.flags |= untagged ? BRIDGE_VLAN_INFO_UNTAGGED : 0;
				vinfo.vid = start;
			}
nxt:
			i = j;
		}
	}

	if (vlan_info->pvid != 0)
	{
		untagged = vlan_id_untagged(vlan_info,vlan_info->pvid);
		vinfo.flags = BRIDGE_VLAN_INFO_PVID;
		vinfo.flags |= untagged ? BRIDGE_VLAN_INFO_UNTAGGED : 0;
		vinfo.vid = vlan_info->pvid;
		NLA_PUT(msg,IFLA_BRIDGE_VLAN_INFO,sizeof(vinfo),&vinfo);
	}

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static int bridge_fill_mst(struct nl_msg *msg, struct nl_list_head *mst_list) {
	struct nlattr *attr = NULL;
	struct nlattr *entry_attr = NULL;
	struct mst_state_entry *entry;

	if (nl_list_empty(mst_list))
		return 0;

	attr = nla_nest_start(msg, IFLA_BRIDGE_MST);
	if (!attr)
		goto err_out;

	nl_list_for_each_entry(entry, mst_list, list_node) {
		entry_attr = nla_nest_start(msg, IFLA_BRIDGE_MST_ENTRY);
		if (!entry_attr)
			goto err_out_nest_cancel_attr;

		NLA_PUT_U16(msg, IFLA_BRIDGE_MST_ENTRY_MSTI, entry->msti);
		NLA_PUT_U8(msg, IFLA_BRIDGE_MST_ENTRY_STATE, entry->state);

		nla_nest_end(msg, entry_attr);
	}

	nla_nest_end(msg, attr);

	return 0;

nla_put_failure:
	nla_nest_cancel(msg, entry_attr);
err_out_nest_cancel_attr:
	nla_nest_cancel(msg, attr);
err_out:
	return -NLE_MSGSIZE;
}

static int bridge_fill_af(struct rtnl_link *link, struct nl_msg *msg,
		   void *data)
{
	struct bridge_data *bd = data;

	if (bd->ce_mask & BRIDGE_ATTR_HWMODE)
	{
		NLA_PUT_U16(msg, IFLA_BRIDGE_MODE, bd->b_hwmode);
		bd->b_config_mode = BRIDGE_FLAGS_SELF;
		bd->ce_mask |= BRIDGE_ATTR_CONFIG_MODE;
	}

	if (bd->ce_mask & BRIDGE_ATTR_MST)
	{
		if (bridge_fill_mst(msg, &bd->mst_list) < 0) {
			goto nla_put_failure;
		}
	}

	if (bd->ce_mask & BRIDGE_ATTR_CONFIG_MODE)
		NLA_PUT_U16(msg, IFLA_BRIDGE_FLAGS, bd->b_config_mode);

	if (bd->ce_mask & BRIDGE_ATTR_PORT_VLAN) {
		if (_nl_bridge_fill_vlan_info(msg, &bd->vlan_info)) {
			goto nla_put_failure;
		}
	}

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static int bridge_fill_pi(struct rtnl_link *link, struct nl_msg *msg,
		   void *data)
{
	struct bridge_data *bd = data;

	if (bd->ce_mask & BRIDGE_ATTR_FLAGS) {
		if (bd->b_flags_mask & RTNL_BRIDGE_BPDU_GUARD) {
			NLA_PUT_U8(msg, IFLA_BRPORT_GUARD,
				   !!(bd->b_flags & RTNL_BRIDGE_BPDU_GUARD));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_HAIRPIN_MODE) {
			NLA_PUT_U8(msg, IFLA_BRPORT_MODE,
				   !!(bd->b_flags & RTNL_BRIDGE_HAIRPIN_MODE));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_FAST_LEAVE) {
			NLA_PUT_U8(msg, IFLA_BRPORT_FAST_LEAVE,
				   !!(bd->b_flags & RTNL_BRIDGE_FAST_LEAVE));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_ROOT_BLOCK) {
			NLA_PUT_U8(msg, IFLA_BRPORT_PROTECT,
				   !!(bd->b_flags & RTNL_BRIDGE_ROOT_BLOCK));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_UNICAST_FLOOD) {
			NLA_PUT_U8(msg, IFLA_BRPORT_UNICAST_FLOOD,
				   !!(bd->b_flags & RTNL_BRIDGE_UNICAST_FLOOD));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_LEARNING) {
			NLA_PUT_U8(msg, IFLA_BRPORT_LEARNING,
				   !!(bd->b_flags & RTNL_BRIDGE_LEARNING));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_LEARNING_SYNC) {
			NLA_PUT_U8(msg, IFLA_BRPORT_LEARNING_SYNC,
				   !!(bd->b_flags & RTNL_BRIDGE_LEARNING_SYNC));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_PROXYARP) {
			NLA_PUT_U8(msg, IFLA_BRPORT_PROXYARP,
				   !!(bd->b_flags & RTNL_BRIDGE_PROXYARP));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_PROXYARP_WIFI) {
			NLA_PUT_U8(msg, IFLA_BRPORT_PROXYARP_WIFI,
				   !!(bd->b_flags & RTNL_BRIDGE_PROXYARP_WIFI));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_MCAST_FLOOD) {
			NLA_PUT_U8(msg, IFLA_BRPORT_MCAST_FLOOD,
				   !!(bd->b_flags & RTNL_BRIDGE_MCAST_FLOOD));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_MCAST_TO_UCAST) {
			NLA_PUT_U8(msg, IFLA_BRPORT_MCAST_TO_UCAST,
				   !!(bd->b_flags &
				      RTNL_BRIDGE_MCAST_TO_UCAST));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_VLAN_TUNNEL) {
			NLA_PUT_U8(msg, IFLA_BRPORT_VLAN_TUNNEL,
				   !!(bd->b_flags & RTNL_BRIDGE_VLAN_TUNNEL));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_BCAST_FLOOD) {
			NLA_PUT_U8(msg, IFLA_BRPORT_BCAST_FLOOD,
				   !!(bd->b_flags & RTNL_BRIDGE_BCAST_FLOOD));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_NEIGH_SUPPRESS) {
			NLA_PUT_U8(msg, IFLA_BRPORT_NEIGH_SUPPRESS,
				   !!(bd->b_flags &
				      RTNL_BRIDGE_NEIGH_SUPPRESS));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_ISOLATED) {
			NLA_PUT_U8(msg, IFLA_BRPORT_ISOLATED,
				   !!(bd->b_flags & RTNL_BRIDGE_ISOLATED));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_LOCKED) {
			NLA_PUT_U8(msg, IFLA_BRPORT_LOCKED,
				   !!(bd->b_flags & RTNL_BRIDGE_LOCKED));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_MAB) {
			NLA_PUT_U8(msg, IFLA_BRPORT_MAB,
				   !!(bd->b_flags & RTNL_BRIDGE_MAB));
		}
		if (bd->b_flags_mask & RTNL_BRIDGE_NEIGH_VLAN_SUPPRESS) {
			NLA_PUT_U8(msg, IFLA_BRPORT_NEIGH_VLAN_SUPPRESS,
				   !!(bd->b_flags &
				      RTNL_BRIDGE_NEIGH_VLAN_SUPPRESS));
		}
	}

	if (bd->ce_mask & BRIDGE_ATTR_COST)
		NLA_PUT_U32(msg, IFLA_BRPORT_COST, bd->b_cost);

	if (bd->ce_mask & BRIDGE_ATTR_PRIORITY)
		NLA_PUT_U16(msg, IFLA_BRPORT_PRIORITY, bd->b_priority);

	if (bd->ce_mask & BRIDGE_ATTR_PORT_STATE)
		NLA_PUT_U8(msg, IFLA_BRPORT_STATE, bd->b_port_state);

	return 0;

nla_put_failure:
	return -NLE_MSGSIZE;
}

static int bridge_override_rtm(struct rtnl_link *link) {
        struct bridge_data *bd;

        if (!rtnl_link_is_bridge(link))
                return 0;

        bd = bridge_data(link);

        if (bd->ce_mask & (BRIDGE_ATTR_FLAGS | BRIDGE_ATTR_MST))
                return 1;

        return 0;
}

static int bridge_get_af(struct nl_msg *msg, uint32_t *ext_filter_mask)
{
	*ext_filter_mask |= RTEXT_FILTER_BRVLAN | RTEXT_FILTER_MST;
	return 0;
}

static void dump_bitmap(struct nl_dump_params *p, const uint32_t *b)
{
	int i = -1, j, k;
	int start = -1, prev = -1;
	int done, found = 0;

	for (k = 0; k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; k++) {
		int base_bit;
		uint32_t a = b[k];

		base_bit = k * 32;
		i = -1;
		done = 0;
		while (!done) {
			j = find_next_bit(i, a);
			if (j > 0) {
				/* first hit of any bit */
				if (start < 0 && prev < 0) {
					start = prev = j - 1 + base_bit;
					goto next;
				}
				/* this bit is a continuation of prior bits */
				if (j - 2 + base_bit == prev) {
					prev++;
					goto next;
				}
			} else
				done = 1;

			if (start >= 0) {
				found++;
				if (done && k < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN - 1)
					break;

				nl_dump(p, " %d", start);
				if (start != prev)
					nl_dump(p, "-%d", prev);

				if (done)
					break;
			}
			if (j > 0)
				start = prev = j - 1 + base_bit;
next:
			i = j;
		}
	}
	if (!found)
		nl_dump(p, " <none>");

	return;
}

static void rtnl_link_bridge_dump_vlans(struct nl_dump_params *p,
					struct bridge_data *bd)
{
	nl_dump(p, "pvid %u", bd->vlan_info.pvid);

	nl_dump(p, "   all vlans:");
	dump_bitmap(p, bd->vlan_info.vlan_bitmap);

	nl_dump(p, "   untagged vlans:");
	dump_bitmap(p, bd->vlan_info.untagged_bitmap);
}

static const char *const br_port_state_names[] = {
	[BR_STATE_DISABLED] = "disabled",
	[BR_STATE_LISTENING] = "listening",
	[BR_STATE_LEARNING] = "learning",
	[BR_STATE_FORWARDING] = "forwarding",
	[BR_STATE_BLOCKING] = "blocking",
};

static const char *stp_state2str(uint8_t state) {
	if (state > BR_STATE_BLOCKING)
		return "unknown";

	return br_port_state_names[state];
}

static void bridge_dump_details(struct rtnl_link *link,
				struct nl_dump_params *p, void *data)
{
	struct bridge_data *bd = data;

	nl_dump_line(p, "    bridge: ");

	if (bd->ce_mask & BRIDGE_ATTR_PORT_STATE)
		nl_dump(p, "port-state %u ", bd->b_port_state);

	if (bd->ce_mask & BRIDGE_ATTR_PRIORITY)
		nl_dump(p, "prio %u ", bd->b_priority);

	if (bd->ce_mask & BRIDGE_ATTR_COST)
		nl_dump(p, "cost %u ", bd->b_cost);

	if (bd->ce_mask & BRIDGE_ATTR_HWMODE) {
		char hbuf[32];

		rtnl_link_bridge_hwmode2str(bd->b_hwmode, hbuf, sizeof(hbuf));
		nl_dump(p, "hwmode %s", hbuf);
	}

	if (bd->ce_mask & BRIDGE_ATTR_PORT_VLAN)
		rtnl_link_bridge_dump_vlans(p, bd);

	if (bd->ce_mask & BRIDGE_ATTR_FLAGS) {
		char buf[256];

		rtnl_link_bridge_flags2str(bd->b_flags & bd->b_flags_mask,
					   buf, sizeof(buf));
		nl_dump(p, "%s", buf);
	}

	nl_dump(p, "\n");

	if (bd->ce_mask & BRIDGE_ATTR_MST && !nl_list_empty(&bd->mst_list)) {
		struct mst_state_entry *entry;

		nl_dump_line(p, "      mst:\n");

		nl_list_for_each_entry(entry, &bd->mst_list, list_node) {
			nl_dump_line(p, "        instance %u: %s\n",
				     entry->msti, stp_state2str(entry->state));
		}
	}
}

static bool mst_state_entries_are_equal(const struct mst_state_entry *a,
					const struct mst_state_entry *b)
{
	return a->msti == b->msti && a->state == b->state;
}

/**
 * Compares two MST lists for equality
 * @arg list_1		The first list
 * @arg list_2		The second list
 *
 * This comparison checks that the MST lists have the same entries and that they
 * are in the same order (although the ordering is not significant to the
 * kernel).
 *
 * @return true if the lists are equal, false otherwise
 */
static bool msts_lists_are_equal(struct nl_list_head *list_1,
				 struct nl_list_head *list_2)
{
	struct mst_state_entry *entry_a;
	struct mst_state_entry *entry_b;

	entry_a =
		nl_list_entry(list_1->next, struct mst_state_entry, list_node);
	entry_b =
		nl_list_entry(list_2->next, struct mst_state_entry, list_node);

	/* while both lists have items left to process */
	while (&entry_a->list_node != list_1 && &entry_b->list_node != list_2) {
		if (!mst_state_entries_are_equal(entry_a, entry_b)) {
			return false;
		}

		entry_a = nl_list_entry(entry_a->list_node.next,
					struct mst_state_entry, list_node);
		entry_b = nl_list_entry(entry_b->list_node.next,
					struct mst_state_entry, list_node);
	}

	/* return true only if both lists were the same length */
	return &entry_a->list_node == list_1 && &entry_b->list_node == list_2;
}

static int bridge_compare(struct rtnl_link *_a, struct rtnl_link *_b,
			  int family, uint32_t attrs, int flags)
{
	struct bridge_data *a = bridge_data(_a);
	struct bridge_data *b = bridge_data(_b);
	int diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(BRIDGE_ATTR_PORT_STATE,
		      a->b_port_state != b->b_port_state);
	diff |= _DIFF(BRIDGE_ATTR_PRIORITY, a->b_priority != b->b_priority);
	diff |= _DIFF(BRIDGE_ATTR_COST, a->b_cost != b->b_cost);
	diff |= _DIFF(BRIDGE_ATTR_PORT_VLAN,
		      memcmp(&a->vlan_info, &b->vlan_info,
			     sizeof(struct rtnl_link_bridge_vlan)));
	diff |= _DIFF(BRIDGE_ATTR_HWMODE, a->b_hwmode != b->b_hwmode);
	diff |= _DIFF(BRIDGE_ATTR_CONFIG_MODE, a->b_config_mode != b->b_config_mode);
	diff |= _DIFF(BRIDGE_ATTR_MST,
		      !msts_lists_are_equal(&a->mst_list, &b->mst_list));

	if (flags & LOOSE_COMPARISON)
		diff |= _DIFF(BRIDGE_ATTR_FLAGS,
			      (a->b_flags ^ b->b_flags) & b->b_flags_mask);
	else
		diff |= _DIFF(BRIDGE_ATTR_FLAGS, a->b_flags != b->b_flags);
#undef _DIFF

	return diff;
}
/** @endcond */

/**
 * Allocate link object of type bridge
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_bridge_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "bridge") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Create a new kernel bridge device
 * @arg sk              netlink socket
 * @arg name            name of the bridge device or NULL
 *
 * Creates a new bridge device in the kernel. If no name is
 * provided, the kernel will automatically pick a name of the
 * form "type%d" (e.g. bridge0, vlan1, etc.)
 *
 * @return 0 on success or a negative error code
*/
int rtnl_link_bridge_add(struct nl_sock *sk, const char *name)
{
	int err;
	struct rtnl_link *link;

	if (!(link = rtnl_link_bridge_alloc()))
		return -NLE_NOMEM;

	if(name)
		rtnl_link_set_name(link, name);

	err = rtnl_link_add(sk, link, NLM_F_CREATE);
	rtnl_link_put(link);

	return err;
}

/**
 * Check if a link is a bridge
 * @arg link		Link object
 *
 * @return 1 if the link is a bridge, 0 otherwise.
 */
int rtnl_link_is_bridge(struct rtnl_link *link)
{
	return link->l_family == AF_BRIDGE &&
	       link->l_af_ops == &bridge_ops;
}

/**
 * Check if bridge has extended information
 * @arg link		Link object of type bridge
 *
 * Checks if the bridge object has been constructed based on
 * information that is only available in newer kernels. This
 * affectes the following functions:
 *  - rtnl_link_bridge_get_cost()
 *  - rtnl_link_bridge_get_priority()
 *  - rtnl_link_bridge_get_flags()
 *
 * @return 1 if extended information is available, otherwise 0 is returned.
 */
int rtnl_link_bridge_has_ext_info(struct rtnl_link *link)
{
	struct bridge_data *bd;

	if (!rtnl_link_is_bridge(link))
		return 0;

	bd = bridge_data(link);
	return !!(bd->b_priv_flags & PRIV_FLAG_NEW_ATTRS);
}

/**
 * Set Spanning Tree Protocol (STP) port state
 * @arg link		Link object of type bridge
 * @arg state		New STP port state
 *
 * The value of state must be one of the following:
 *   - BR_STATE_DISABLED
 *   - BR_STATE_LISTENING
 *   - BR_STATE_LEARNING
 *   - BR_STATE_FORWARDING
 *   - BR_STATE_BLOCKING
 *
 * @see rtnl_link_bridge_get_port_state()
 *
 * @return 0 on success or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 * @retval -NLE_INVAL Invalid state value (0..BR_STATE_BLOCKING)
 */
int rtnl_link_bridge_set_port_state(struct rtnl_link *link, uint8_t state)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	if (state > BR_STATE_BLOCKING)
		return -NLE_INVAL;

	bd->b_port_state = state;
	bd->ce_mask |= BRIDGE_ATTR_PORT_STATE;

	return 0;
}

/**
 * Get Spanning Tree Protocol (STP) port state
 * @arg link		Link object of type bridge
 *
 * @see rtnl_link_bridge_set_port_state()
 *
 * @return The STP port state or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 */
int rtnl_link_bridge_get_port_state(struct rtnl_link *link)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	return bd->b_port_state;
}

/**
 * Set priority
 * @arg link		Link object of type bridge
 * @arg prio		Bridge priority
 *
 * @see rtnl_link_bridge_get_priority()
 *
 * @return 0 on success or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 */
int rtnl_link_bridge_set_priority(struct rtnl_link *link, uint16_t prio)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	bd->b_priority = prio;
	bd->ce_mask |= BRIDGE_ATTR_PRIORITY;

	return 0;
}

/**
 * Get priority
 * @arg link		Link object of type bridge
 *
 * @see rtnl_link_bridge_set_priority()
 *
 * @return 0 on success or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 */
int rtnl_link_bridge_get_priority(struct rtnl_link *link)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	return bd->b_priority;
}

/**
 * Set Spanning Tree Protocol (STP) path cost
 * @arg link		Link object of type bridge
 * @arg cost		New STP path cost value
 *
 * @see rtnl_link_bridge_get_cost()
 *
 * @return The bridge priority or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 */
int rtnl_link_bridge_set_cost(struct rtnl_link *link, uint32_t cost)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	bd->b_cost = cost;
	bd->ce_mask |= BRIDGE_ATTR_COST;

	return 0;
}

/**
 * Get Spanning Tree Protocol (STP) path cost
 * @arg link		Link object of type bridge
 * @arg cost		Pointer to store STP cost value
 *
 * @see rtnl_link_bridge_set_cost()
 *
 * @return 0 on success or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 * @retval -NLE_INVAL `cost` is not a valid pointer
 */
int rtnl_link_bridge_get_cost(struct rtnl_link *link, uint32_t *cost)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	if (!cost)
		return -NLE_INVAL;

	*cost = bd->b_cost;

	return 0;
}

/**
 * Unset flags
 * @arg link		Link object of type bridge
 * @arg flags		Bridging flags to unset
 *
 * @see rtnl_link_bridge_set_flags()
 * @see rtnl_link_bridge_get_flags()
 *
 * @return 0 on success or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 */
int rtnl_link_bridge_unset_flags(struct rtnl_link *link, unsigned int flags)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	bd->b_flags_mask |= flags;
	bd->b_flags &= ~flags;
	bd->ce_mask |= BRIDGE_ATTR_FLAGS;

	return 0;
}

/**
 * Set flags
 * @arg link		Link object of type bridge
 * @arg flags		Bridging flags to set
 *
 * Valid flags are:
 *   - RTNL_BRIDGE_HAIRPIN_MODE
 *   - RTNL_BRIDGE_BPDU_GUARD
 *   - RTNL_BRIDGE_ROOT_BLOCK
 *   - RTNL_BRIDGE_FAST_LEAVE
 *   - RTNL_BRIDGE_UNICAST_FLOOD
 *   - RTNL_BRIDGE_LEARNING
 *   - RTNL_BRIDGE_LEARNING_SYNC
 *   - RTNL_BRIDGE_PROXYARP
 *   - RTNL_BRIDGE_PROXYARP_WIFI
 *   - RTNL_BRIDGE_MCAST_FLOOD
 *   - RTNL_BRIDGE_MCAST_TO_UCAST
 *   - RTNL_BRIDGE_VLAN_TUNNEL
 *   - RTNL_BRIDGE_BCAST_FLOOD
 *   - RTNL_BRIDGE_NEIGH_SUPPRESS
 *   - RTNL_BRIDGE_ISOLATED
 *   - RTNL_BRIDGE_LOCKED
 *   - RTNL_BRIDGE_MAB
 *   - RTNL_BRIDGE_NEIGH_VLAN_SUPPRESS
 *
 * @see rtnl_link_bridge_unset_flags()
 * @see rtnl_link_bridge_get_flags()
 *
 * @return 0 on success or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 */
int rtnl_link_bridge_set_flags(struct rtnl_link *link, unsigned int flags)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	bd->b_flags_mask |= flags;
	bd->b_flags |= flags;
	bd->ce_mask |= BRIDGE_ATTR_FLAGS;

	return 0;
}

/**
 * Get flags
 * @arg link		Link object of type bridge
 *
 * @see rtnl_link_bridge_set_flags()
 * @see rtnl_link_bridge_unset_flags()
 *
 * @return Flags or a negative error code.
 * @retval -NLE_OPNOTSUPP Link is not a bridge
 */
int rtnl_link_bridge_get_flags(struct rtnl_link *link)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	return bd->b_flags;
}

/**
 * Set link change type to self
 * @arg link		Link Object of type bridge
 *
 * This will set the bridge change flag to self, meaning that changes to
 * be applied with this link object will be applied directly to the physical
 * device in a bridge instead of the virtual device.
 *
 * @return 0 on success or negative error code
 * @return -NLE_OPNOTSUP Link is not a bridge
 */
int rtnl_link_bridge_set_self(struct rtnl_link *link)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	bd->b_config_mode = BRIDGE_FLAGS_SELF;
	bd->ce_mask |= BRIDGE_ATTR_CONFIG_MODE;

	return 0;
}

/**
 * Set link change type to master
 * @arg link		Link Object of type bridge
 *
 * This will set the bridge change flag to master, meaning that changes to
 * be applied with this link object will be applied directly to the virtual
 * device in a bridge instead of the physical device.
 *
 * @return 0 on success or negative error code
 * @return -NLE_OPNOTSUP Link is not a bridge
 */
int rtnl_link_bridge_set_master(struct rtnl_link *link)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	bd->b_config_mode = BRIDGE_FLAGS_MASTER;
	bd->ce_mask |= BRIDGE_ATTR_CONFIG_MODE;

	return 0;
}

/**
 * Get hardware mode
 * @arg link            Link object of type bridge
 * @arg hwmode          Output argument.
 *
 * @see rtnl_link_bridge_set_hwmode()
 *
 * @return 0 if hardware mode is present and returned in hwmode
 * @return -NLE_NOATTR if hardware mode is not present
 * @return -NLE_OPNOTSUP Link is not a bridge
 */
int rtnl_link_bridge_get_hwmode(struct rtnl_link *link, uint16_t *hwmode)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	if (!(bd->ce_mask & BRIDGE_ATTR_HWMODE))
		return -NLE_NOATTR;

	*hwmode = bd->b_hwmode;
	return 0;
}

/**
 * Set hardware mode
 * @arg link		Link object of type bridge
 * @arg hwmode		Hardware mode to set on link
 *
 * This will set the hardware mode of a link when it supports hardware
 * offloads for bridging.
 * @see rtnl_link_bridge_get_hwmode()
 *
 * Valid modes are:
 *   - RTNL_BRIDGE_HWMODE_VEB
 *   - RTNL_BRIDGE_HWMODE_VEPA
 *
 * When setting hardware mode, the change type will be set to self.
 * @see rtnl_link_bridge_set_self()
 *
 * @return 0 on success or negative error code
 * @return -NLE_OPNOTSUP Link is not a bridge
 * @return -NLE_INVAL when specified hwmode is unsupported.
 */
int rtnl_link_bridge_set_hwmode(struct rtnl_link *link, uint16_t hwmode)
{
	int err;
	struct bridge_data *bd = bridge_data(link);

	if (hwmode > RTNL_BRIDGE_HWMODE_MAX)
		return -NLE_INVAL;

	if ((err = rtnl_link_bridge_set_self(link)) < 0)
		return err;

	bd->b_hwmode = hwmode;
	bd->ce_mask |= BRIDGE_ATTR_HWMODE;

	return 0;
}


static const struct trans_tbl bridge_flags[] = {
	__ADD(RTNL_BRIDGE_HAIRPIN_MODE,		hairpin_mode),
	__ADD(RTNL_BRIDGE_BPDU_GUARD, 		bpdu_guard),
	__ADD(RTNL_BRIDGE_ROOT_BLOCK,		root_block),
	__ADD(RTNL_BRIDGE_FAST_LEAVE,		fast_leave),
	__ADD(RTNL_BRIDGE_UNICAST_FLOOD,	flood),
	__ADD(RTNL_BRIDGE_LEARNING,		learning),
	__ADD(RTNL_BRIDGE_LEARNING_SYNC,	learning_sync),
	__ADD(RTNL_BRIDGE_PROXYARP,		proxy_arp),
	__ADD(RTNL_BRIDGE_PROXYARP_WIFI,	proxy_arp_wifi),
	__ADD(RTNL_BRIDGE_MCAST_FLOOD,		mcast_flood),
	__ADD(RTNL_BRIDGE_MCAST_TO_UCAST,	mcast_to_unicast),
	__ADD(RTNL_BRIDGE_VLAN_TUNNEL,		vlan_tunnel),
	__ADD(RTNL_BRIDGE_BCAST_FLOOD,		bcast_flood),
	__ADD(RTNL_BRIDGE_NEIGH_SUPPRESS,	neigh_suppress),
	__ADD(RTNL_BRIDGE_ISOLATED,		isolated),
	__ADD(RTNL_BRIDGE_LOCKED,		locked),
	__ADD(RTNL_BRIDGE_MAB,			mab),
	__ADD(RTNL_BRIDGE_NEIGH_VLAN_SUPPRESS,	neigh_vlan_suppress),
};

/**
 * @name Flag Translation
 * @{
 */

char *rtnl_link_bridge_flags2str(int flags, char *buf, size_t len)
{
	return __flags2str(flags, buf, len, bridge_flags, ARRAY_SIZE(bridge_flags));
}

int rtnl_link_bridge_str2flags(const char *name)
{
	return __str2flags(name, bridge_flags, ARRAY_SIZE(bridge_flags));
}

/** @} */

static const struct trans_tbl port_states[] = {
	__ADD(BR_STATE_DISABLED, disabled),
	__ADD(BR_STATE_LISTENING, listening),
	__ADD(BR_STATE_LEARNING, learning),
	__ADD(BR_STATE_FORWARDING, forwarding),
	__ADD(BR_STATE_BLOCKING, blocking),
};

/**
 * @name Port State Translation
 * @{
 */

char *rtnl_link_bridge_portstate2str(int st, char *buf, size_t len)
{
	return __type2str(st, buf, len, port_states, ARRAY_SIZE(port_states));
}

int rtnl_link_bridge_str2portstate(const char *name)
{
	return __str2type(name, port_states, ARRAY_SIZE(port_states));
}

/** @} */

static const struct trans_tbl hw_modes[] = {
	__ADD(RTNL_BRIDGE_HWMODE_VEB, veb),
	__ADD(RTNL_BRIDGE_HWMODE_VEPA, vepa),
	__ADD(RTNL_BRIDGE_HWMODE_UNDEF, undef),
};

/**
 * @name Hardware Mode Translation
 * @{
 */

char *rtnl_link_bridge_hwmode2str(uint16_t st, char *buf, size_t len) {
	return __type2str(st, buf, len, hw_modes, ARRAY_SIZE(hw_modes));
}

uint16_t rtnl_link_bridge_str2hwmode(const char *name)
{
	return __str2type(name, hw_modes, ARRAY_SIZE(hw_modes));
}

/** @} */

/**
 * Enable the ability to set vlan info
 * @arg link		Link object of type bridge
 *
 * @return 0 on success or negative error code
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 */
int rtnl_link_bridge_enable_vlan(struct rtnl_link *link)
{
	struct bridge_data *bd = bridge_data(link);

	IS_BRIDGE_LINK_ASSERT(link);

	bd->ce_mask |= BRIDGE_ATTR_PORT_VLAN;

	return 0;
}

/**
 * @name Quality of Service
 * @{
 */

/**
 * Set port vlan membership range
 * @arg link		Link object of type bridge
 * @arg start		Start of membership range.
 * @arg end			End of membership range.
 * @arg untagged	Set membership range to be untagged.
 *
 * This will set the vlan membership range for a bridge port.
 * This will unset the untagged membership if untagged is false.
 * Supported range is 1-4094
 *
 * @return 0 on success or negative error code
 * @return -NLE_NOATTR		if port vlan attribute not present
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 * @return -NLE_INVAL 		range is not in supported range.
 */
int rtnl_link_bridge_set_port_vlan_map_range (struct rtnl_link *link, uint16_t start, uint16_t end, int untagged)
{
	struct rtnl_link_bridge_vlan * vinfo;

	IS_BRIDGE_LINK_ASSERT(link);

	vinfo = rtnl_link_bridge_get_port_vlan(link);

	if (!vinfo)
		return -NLE_NOATTR;

	if (start == 0 || start > end || end >= VLAN_VID_MASK)
		return -NLE_INVAL;

	for (uint16_t i = start; i <= end; i++)
	{
		set_bit(i,vinfo->vlan_bitmap);
		if (untagged) {
			set_bit(i,vinfo->untagged_bitmap);
		} else {
			unset_bit(i,vinfo->untagged_bitmap);
		}
	}
	return 0;
}

/**
 * Unset port vlan membership range
 * @arg link		Link object of type bridge
 * @arg start		Start of membership range.
 * @arg end			End of membership range.
 *
 * This will unset the vlan membership range for a bridge port
 * for both tagged and untagged membership.
 * Supported range is 1-4094
 *
 * @return 0 on success or negative error code
 * @return -NLE_NOATTR		if port vlan attribute not present
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 * @return -NLE_INVAL 		range is not in supported range.
 */
int rtnl_link_bridge_unset_port_vlan_map_range (struct rtnl_link *link, uint16_t start, uint16_t end)
{
	struct rtnl_link_bridge_vlan * vinfo;

	IS_BRIDGE_LINK_ASSERT(link);

	vinfo = rtnl_link_bridge_get_port_vlan(link);

	if (!vinfo)
		return -NLE_NOATTR;

	if (start == 0 || start > end || end >= VLAN_VID_MASK)
		return -NLE_INVAL;

	for (uint16_t i = start; i <= end; i++)
	{
		unset_bit(i,vinfo->vlan_bitmap);
		unset_bit(i,vinfo->untagged_bitmap);
	}
	return 0;
}

/**
 * Set port primary vlan id
 * @arg link		Link object of type bridge
 * @arg pvid		PVID to set.
 * @arg untagged	Set vlan id to be untagged.
 *
 * This will set the primary vlan id for a bridge port.
 * Supported range is 0-4094, Setting pvid to 0 will unset it.
 * You will most likely want to set/unset pvid in the vlan map.
 * @see rtnl_link_bridge_set_port_vlan_map_range()
 * @see rtnl_link_bridge_unset_port_vlan_map_range()
 *
 * @return 0 on success or negative error code
 * @return -NLE_NOATTR		if port vlan attribute not present
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 * @return -NLE_INVAL 		PVID is above supported range.
 */
int rtnl_link_bridge_set_port_vlan_pvid (struct rtnl_link *link, uint16_t pvid)
{
	struct rtnl_link_bridge_vlan * vinfo;

	IS_BRIDGE_LINK_ASSERT(link);

	vinfo = rtnl_link_bridge_get_port_vlan(link);

	if (!vinfo)
		return -NLE_NOATTR;

	if (pvid >= VLAN_VID_MASK)
		return -NLE_INVAL;

	vinfo->pvid = pvid;

	return 0;
}

/** @} */

int rtnl_link_bridge_pvid(struct rtnl_link *link)
{
	struct bridge_data *bd;

	IS_BRIDGE_LINK_ASSERT(link);

	bd = link->l_af_data[AF_BRIDGE];
	if (bd->ce_mask & BRIDGE_ATTR_PORT_VLAN)
		return (int) bd->vlan_info.pvid;

	return -EINVAL;
}

int rtnl_link_bridge_has_vlan(struct rtnl_link *link)
{
	struct bridge_data *bd;
	int i;

	IS_BRIDGE_LINK_ASSERT(link);

	bd = link->l_af_data[AF_BRIDGE];
	if (bd->ce_mask & BRIDGE_ATTR_PORT_VLAN) {
		if (bd->vlan_info.pvid)
			return 1;

		for (i = 0; i < RTNL_LINK_BRIDGE_VLAN_BITMAP_LEN; ++i) {
			if (bd->vlan_info.vlan_bitmap[i] ||
			    bd->vlan_info.untagged_bitmap[i])
				return 1;
		}
	}
	return 0;
}

struct rtnl_link_bridge_vlan *rtnl_link_bridge_get_port_vlan(struct rtnl_link *link)
{
	struct bridge_data *data;

	if (!rtnl_link_is_bridge(link))
		return NULL;

	data = link->l_af_data[AF_BRIDGE];
	if (data && (data->ce_mask & BRIDGE_ATTR_PORT_VLAN))
		return &data->vlan_info;

	return NULL;
}

/**
 * Set the Multiple Spanning Tree (MST) port state for a given MST instance
 * @arg link		Link object of type bridge
 * @arg instance	MST instance number
 * @arg state		Port state to set (BR_STATE_*)
 *
 * @return 0 on success or a negative error code
 * @return -NLE_INVAL 		link is NULL
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 * @return -NLE_NOMEM		Memory allocation failed
 */
int rtnl_link_bridge_set_mst_port_state(struct rtnl_link *link,
					uint16_t instance, uint8_t state)
{
	struct bridge_data *bd;
	struct mst_state_entry *existing_entry;
	struct mst_state_entry *new_entry;

	if (link == NULL)
		return -NLE_INVAL;

	IS_BRIDGE_LINK_ASSERT(link);

	bd = bridge_data(link);
	if (bd == NULL)
		return -NLE_OPNOTSUPP;

	existing_entry = find_mst_state_entry(bd, instance);

	if (existing_entry != NULL) {
		existing_entry->state = state;
		return 0;
	}

	new_entry = mst_state_entry_create(instance, state);
	if (new_entry == NULL)
		return -NLE_NOMEM;

	nl_list_add_tail(&new_entry->list_node, &bd->mst_list);
	bd->ce_mask |= BRIDGE_ATTR_MST;

	return 0;
}

/**
 * Get the Multiple Spanning Tree (MST) port state for a given MST instance
 * @arg link		Link object of type bridge
 * @arg instance	MST instance number
 *
 * @return The state (BR_STATE_*) on success, or a negative error code
 * @return -NLE_INVAL 		link is NULL
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 * @return -NLE_OBJ_NOTFOUND	MST instance not found
 */
int rtnl_link_bridge_get_mst_port_state(struct rtnl_link *link,
					uint16_t instance)
{
	struct bridge_data *bd;
	struct mst_state_entry *entry;

	if (link == NULL)
		return -NLE_INVAL;

	IS_BRIDGE_LINK_ASSERT(link);

	bd = bridge_data(link);
	if (bd == NULL)
		return -NLE_OPNOTSUPP;

	entry = find_mst_state_entry(bd, instance);

	if (entry == NULL)
		return -NLE_OBJ_NOTFOUND;

	return entry->state;
}

/**
 * Delete the Multiple Spanning Tree (MST) port state for a given MST instance
 * @arg link		Link object of type bridge
 * @arg instance	MST instance number
 *
 * @return 0 on success or a negative error code
 * @return -NLE_INVAL 		link is NULL
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 * @return -NLE_OBJ_NOTFOUND	MST instance not found
 */
int rtnl_link_bridge_del_mst_port_state(struct rtnl_link *link,
					uint16_t instance)
{
	struct bridge_data *bd;
	struct mst_state_entry *entry;

	if (link == NULL)
		return -NLE_INVAL;

	IS_BRIDGE_LINK_ASSERT(link);

	bd = bridge_data(link);
	if (bd == NULL)
		return -NLE_OPNOTSUPP;

	entry = find_mst_state_entry(bd, instance);

	if (entry == NULL)
		return -NLE_OBJ_NOTFOUND;

	mst_state_entry_del(entry);

	if (nl_list_empty(&bd->mst_list))
		bd->ce_mask &= ~BRIDGE_ATTR_MST;

	return 0;
}

/**
 * Delete all Multiple Spanning Tree (MST) port state information
 * @arg link		Link object of type bridge
 *
 * @return 0 on success or a negative error code
 * @return -NLE_INVAL 		link is NULL
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 */
int rtnl_link_bridge_clear_mst_port_state_info(struct rtnl_link *link)
{
	struct bridge_data *bd;

	if (link == NULL)
		return -NLE_INVAL;

	IS_BRIDGE_LINK_ASSERT(link);

	bd = bridge_data(link);
	if (bd == NULL)
		return -NLE_OPNOTSUPP;

	mst_list_clear(&bd->mst_list);
	bd->ce_mask &= ~BRIDGE_ATTR_MST;

	return 0;
}

/**
 * Iterate over all Multiple Spanning Tree (MST) port state entries
 * @arg link		Link object of type bridge
 * @arg cb		Callback function
 * @arg arg		User provided data argument to pass to the callback
 *			function
 *
 * The callback function is called for each MST entry. It is passed the MST
 * instance ID, state (BR_STATE_*), and an optional user provided data argument.
 * MST entries should not be added or removed by the callback function.
 *
 * @return 0 on success or a negative error code
 * @return -NLE_INVAL 		link or cb is NULL
 * @return -NLE_OPNOTSUP 	Link is not a bridge
 */
int rtnl_link_bridge_foreach_mst_entry(
	struct rtnl_link *link,
	void (*cb)(uint16_t instance, uint8_t state, void *arg), void *arg)
{
	struct bridge_data *bd;
	struct mst_state_entry *entry;

	if (link == NULL || cb == NULL)
		return -NLE_INVAL;

	IS_BRIDGE_LINK_ASSERT(link);

	bd = bridge_data(link);
	if (bd == NULL)
		return -NLE_OPNOTSUPP;

	nl_list_for_each_entry(entry, &bd->mst_list, list_node) {
		cb(entry->msti, entry->state, arg);
	}

	return 0;
}

static struct rtnl_link_af_ops bridge_ops = {
	.ao_family			= AF_BRIDGE,
	.ao_alloc			= &bridge_alloc,
	.ao_clone			= &bridge_clone,
	.ao_free			= &bridge_free,
	.ao_parse_protinfo		= &bridge_parse_protinfo,
	.ao_dump[NL_DUMP_DETAILS]	= &bridge_dump_details,
	.ao_compare			= &bridge_compare,
	.ao_parse_af_full		= &bridge_parse_af_full,
	.ao_get_af			= &bridge_get_af,
	.ao_fill_af			= &bridge_fill_af,
	.ao_fill_pi			= &bridge_fill_pi,
	.ao_fill_pi_flags	= NLA_F_NESTED,
	.ao_override_rtm		= &bridge_override_rtm,
	.ao_fill_af_no_nest	= 1,
};

static void _nl_init bridge_init(void)
{
	rtnl_link_af_register(&bridge_ops);
}

static void _nl_exit bridge_exit(void)
{
	rtnl_link_af_unregister(&bridge_ops);
}

/** @} */
