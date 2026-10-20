/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2016 Intel Corp. All rights reserved.
 * Copyright (c) 2016 Jef Oliver <jef.oliver@intel.com>
 */

/**
 * @ingroup link
 * @defgroup sriov SRIOV
 * SR-IOV VF link module
 *
 * @details
 * SR-IOV (Single Root Input/Output Virtualization) is a network interface
 * that allows for the isolation of the PCI Express resources. In a virtual
 * environment, SR-IOV allows multiple virtual machines can share a single
 * PCI Express hardware interface. This is done via VFs (Virtual Functions),
 * virtual hardware devices with their own PCI address.
 *
 * @{
 */

#include "nl-default.h"

#include <linux/if_ether.h>
#include <linux/if_link.h>

#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/link/sriov.h>

#include "nl-route.h"
#include "link-sriov.h"
#include "link-api.h"

/** @cond SKIP */
struct rtnl_link_vf {
	struct nl_list_head vf_list;
	int ce_refcnt;
	uint32_t ce_mask;
	uint32_t vf_index;
	uint64_t vf_guid_node;
	uint64_t vf_guid_port;
	uint32_t vf_linkstate;
	struct nl_addr *vf_lladdr;
	uint32_t vf_max_tx_rate;
	uint32_t vf_min_tx_rate;
	uint32_t vf_rate;
	uint32_t vf_rss_query_en;
	uint32_t vf_spoofchk;
	uint64_t vf_stats[RTNL_LINK_VF_STATS_MAX + 1];
	uint32_t vf_trust;
	struct nl_vf_vlans *vf_vlans;
};

#define SRIOVON "on"
#define SRIOVOFF "off"

#define SET_VF_STAT(link, vf_num, stb, stat, attr) \
	vf_data->vf_stats[stat] = nla_get_u64(stb[attr])

/* SRIOV-VF Attributes */
#define SRIOV_ATTR_INDEX 		(1 <<  0)
#define SRIOV_ATTR_ADDR 		(1 <<  1)
#define SRIOV_ATTR_VLAN 		(1 <<  2)
#define SRIOV_ATTR_TX_RATE 		(1 <<  3)
#define SRIOV_ATTR_SPOOFCHK 		(1 <<  4)
#define SRIOV_ATTR_RATE_MAX 		(1 <<  5)
#define SRIOV_ATTR_RATE_MIN 		(1 <<  6)
#define SRIOV_ATTR_LINK_STATE 		(1 <<  7)
#define SRIOV_ATTR_RSS_QUERY_EN 	(1 <<  8)
#define SRIOV_ATTR_STATS 		(1 <<  9)
#define SRIOV_ATTR_TRUST 		(1 << 10)
#define SRIOV_ATTR_IB_NODE_GUID 	(1 << 11)
#define SRIOV_ATTR_IB_PORT_GUID 	(1 << 12)

static struct nla_policy sriov_info_policy[IFLA_VF_MAX+1] = {
	[IFLA_VF_MAC]		= { .minlen = sizeof(struct ifla_vf_mac) },
	[IFLA_VF_VLAN]		= { .minlen = sizeof(struct ifla_vf_vlan) },
	[IFLA_VF_VLAN_LIST]     = { .type = NLA_NESTED },
	[IFLA_VF_TX_RATE]	= { .minlen = sizeof(struct ifla_vf_tx_rate) },
	[IFLA_VF_SPOOFCHK]	= { .minlen = sizeof(struct ifla_vf_spoofchk) },
	[IFLA_VF_RATE]		= { .minlen = sizeof(struct ifla_vf_rate) },
	[IFLA_VF_LINK_STATE]	= { .minlen = sizeof(struct ifla_vf_link_state) },
	[IFLA_VF_RSS_QUERY_EN]	= { .minlen = sizeof(struct ifla_vf_rss_query_en) },
	[IFLA_VF_STATS]		= { .type = NLA_NESTED },
	[IFLA_VF_TRUST]		= { .minlen = sizeof(struct ifla_vf_trust) },
	[IFLA_VF_IB_NODE_GUID]	= { .minlen = sizeof(struct ifla_vf_guid) },
	[IFLA_VF_IB_PORT_GUID]	= { .minlen = sizeof(struct ifla_vf_guid) },
};

static struct nla_policy sriov_stats_policy[IFLA_VF_STATS_MAX+1] = {
	[IFLA_VF_STATS_RX_PACKETS]	= { .type = NLA_U64 },
	[IFLA_VF_STATS_TX_PACKETS]	= { .type = NLA_U64 },
	[IFLA_VF_STATS_RX_BYTES]	= { .type = NLA_U64 },
	[IFLA_VF_STATS_TX_BYTES]	= { .type = NLA_U64 },
	[IFLA_VF_STATS_BROADCAST]	= { .type = NLA_U64 },
	[IFLA_VF_STATS_MULTICAST]	= { .type = NLA_U64 },
};

/** @endcond */

/* Clone SRIOV VF list in link object */
int rtnl_link_sriov_clone(struct rtnl_link *dst, struct rtnl_link *src) {
	int err = 0;
	struct nl_addr *vf_addr;
	struct rtnl_link_vf *s_list, *d_vf, *s_vf, *next, *dest_h = NULL;
	nl_vf_vlans_t *src_vlans = NULL, *dst_vlans = NULL;
	nl_vf_vlan_info_t *src_vlan_info = NULL, *dst_vlan_info = NULL;

	if (!rtnl_link_has_vf_list(src))
		return 0;

	dst->l_vf_list = rtnl_link_vf_alloc();
	if (!dst->l_vf_list)
		return -NLE_NOMEM;
	dest_h = dst->l_vf_list;
	s_list = src->l_vf_list;

	nl_list_for_each_entry_safe(s_vf, next, &s_list->vf_list, vf_list) {
		if (!(d_vf = rtnl_link_vf_alloc()))
			return -NLE_NOMEM;

		memcpy(d_vf, s_vf, sizeof(*s_vf));

		if (s_vf->ce_mask & SRIOV_ATTR_ADDR) {
			vf_addr = nl_addr_clone(s_vf->vf_lladdr);
			if (!vf_addr) {
				rtnl_link_vf_put(d_vf);
				return -NLE_NOMEM;
			}
			d_vf->vf_lladdr = vf_addr;
		}

		if (s_vf->ce_mask & SRIOV_ATTR_VLAN) {
			src_vlans = s_vf->vf_vlans;
			src_vlan_info = src_vlans->vlans;

			err = rtnl_link_vf_vlan_alloc(&dst_vlans,
						      src_vlans->size);
			if (err < 0) {
				rtnl_link_vf_put(d_vf);
				return err;
			}
			dst_vlan_info = dst_vlans->vlans;
			memcpy(dst_vlans, src_vlans, sizeof(nl_vf_vlans_t));
			memcpy(dst_vlan_info, src_vlan_info,
			       dst_vlans->size * sizeof(*dst_vlan_info));
			d_vf->vf_vlans = dst_vlans;
		}

		nl_list_add_head(&d_vf->vf_list, &dest_h->vf_list);
		dest_h = d_vf;
	}

	return 0;
}

/* Dump VLAN details for each SRIOV VF */
static void dump_sriov_vlans(nl_vf_vlans_t *vlans,
			     struct nl_dump_params *p) {
	char buf[64];
	int cur = 0;
	nl_vf_vlan_info_t *vlan_data;
	uint16_t prot;

	vlan_data = vlans->vlans;
	nl_dump(p, "\t      VLANS:\n");
	while (cur < vlans->size) {
		nl_dump(p, "\t      vlan %u", vlan_data[cur].vf_vlan);
		if (vlan_data[cur].vf_vlan_qos)
			nl_dump(p, " qos %u", vlan_data[cur].vf_vlan_qos);
		if (vlan_data[cur].vf_vlan_proto) {
			prot = vlan_data[cur].vf_vlan_proto;
			nl_dump(p, " proto %s",
				rtnl_link_vf_vlanproto2str(prot, buf,
							   sizeof(buf)));
		}
		nl_dump(p, "\n");
		cur++;
	}

	return;
}

/* Dump details for each SRIOV VF */
static void dump_vf_details(struct rtnl_link_vf *vf_data,
			    struct nl_dump_params *p) {
	char buf[64];
	int err = 0;
	struct nl_vf_rate vf_rate;
	uint32_t v = 0;

	nl_dump(p, "\tvf %u: ", vf_data->vf_index);
	if (vf_data->ce_mask & SRIOV_ATTR_LINK_STATE) {
		v = vf_data->vf_linkstate;
		nl_dump(p, "state %s ",
			rtnl_link_vf_linkstate2str(v, buf, sizeof(buf)));
	}
	if (vf_data->ce_mask & SRIOV_ATTR_ADDR) {
		nl_dump(p, "addr %s ",
			nl_addr2str(vf_data->vf_lladdr, buf, sizeof(buf)));
	}
	nl_dump(p, "\n");

	v = vf_data->vf_spoofchk;
	nl_dump(p, "\t      spoofchk %s ", v ? SRIOVON : SRIOVOFF);
	v = vf_data->vf_trust;
	nl_dump(p, "trust %s ", v ? SRIOVON : SRIOVOFF);
	v = vf_data->vf_rss_query_en;
	nl_dump(p, "rss_query %s\n", v ? SRIOVON : SRIOVOFF);

	err = rtnl_link_vf_get_rate(vf_data, &vf_rate);
	if (!err) {
		if (vf_rate.api == RTNL_LINK_VF_RATE_API_OLD)
			nl_dump(p, "\t      rate_api old rate %u\n",
				vf_rate.rate);
		else if (vf_rate.api == RTNL_LINK_VF_RATE_API_NEW)
			nl_dump(p, "\t      rate_api new min_rate %u "
					"max_rate %u\n", vf_rate.min_tx_rate,
				vf_rate.max_tx_rate);
	}
	if (vf_data->ce_mask & SRIOV_ATTR_VLAN)
		dump_sriov_vlans(vf_data->vf_vlans, p);

	return;
}

/* Loop through SRIOV VF list dump details */
void rtnl_link_sriov_dump_details(struct rtnl_link *link,
				  struct nl_dump_params *p) {
	struct rtnl_link_vf *vf_data, *list, *next;

	if (!rtnl_link_has_vf_list(link))
		BUG();

	nl_dump(p, "    SRIOV VF List\n");
	list = link->l_vf_list;
	nl_list_for_each_entry_safe(vf_data, next, &list->vf_list, vf_list) {
		if (vf_data->ce_mask & SRIOV_ATTR_INDEX)
			dump_vf_details(vf_data, p);
	}

	return;
}

/* Dump stats for each SRIOV VF */
static void dump_vf_stats(struct rtnl_link_vf *vf_data,
			  struct nl_dump_params *p) {
	char *unit;
	float res;

	nl_dump(p, "    VF %u Stats:\n", vf_data->vf_index);
	nl_dump_line(p, "\tRX:    %-14s %-10s   %-10s %-10s\n",
		     "bytes", "packets", "multicast", "broadcast");

	res = nl_cancel_down_bytes(vf_data->vf_stats[RTNL_LINK_VF_STATS_RX_BYTES],
				   &unit);

	nl_dump_line(p,
		"\t%10.2f %3s   %10" PRIu64 "   %10" PRIu64 " %10" PRIu64 "\n",
		res, unit,
		vf_data->vf_stats[RTNL_LINK_VF_STATS_RX_PACKETS],
		vf_data->vf_stats[RTNL_LINK_VF_STATS_MULTICAST],
		vf_data->vf_stats[RTNL_LINK_VF_STATS_BROADCAST]);

	nl_dump_line(p, "\tTX:    %-14s %-10s\n", "bytes", "packets");

	res = nl_cancel_down_bytes(vf_data->vf_stats[RTNL_LINK_VF_STATS_TX_BYTES],
				   &unit);

	nl_dump_line(p, "\t%10.2f %3s   %10" PRIu64 "\n", res, unit,
		vf_data->vf_stats[RTNL_LINK_VF_STATS_TX_PACKETS]);

	return;
}

/* Loop through SRIOV VF list dump stats */
void rtnl_link_sriov_dump_stats(struct rtnl_link *link,
				struct nl_dump_params *p) {
	struct rtnl_link_vf *vf_data, *list, *next;

	list = link->l_vf_list;
	nl_list_for_each_entry_safe(vf_data, next, &list->vf_list, vf_list) {
		if (vf_data->ce_mask & SRIOV_ATTR_INDEX)
			dump_vf_stats(vf_data, p);
	}
	nl_dump(p, "\n");

	return;
}

/* Free stored SRIOV VF data */
void rtnl_link_sriov_free_data(struct rtnl_link *link) {
	struct rtnl_link_vf *list, *vf, *next;

	if (!rtnl_link_has_vf_list(link))
		return;

	list = link->l_vf_list;
	nl_list_for_each_entry_safe(vf, next, &list->vf_list, vf_list) {
		nl_list_del(&vf->vf_list);
		rtnl_link_vf_put(vf);
	}

	rtnl_link_vf_put(link->l_vf_list);

	return;
}

/* Fill VLAN info array */
static int rtnl_link_vf_vlan_info(int len, struct ifla_vf_vlan_info **vi,
				  nl_vf_vlans_t **nvi) {
	int cur = 0, err;
	nl_vf_vlans_t *vlans;

	if (len <= 0)
		return 0;

	if ((err = rtnl_link_vf_vlan_alloc(&vlans, len)) < 0)
		return err;

	cur = 0;
	while (cur < len) {
		vlans->vlans[cur].vf_vlan = vi[cur]->vlan ? vi[cur]->vlan : 0;
		vlans->vlans[cur].vf_vlan_qos = vi[cur]->qos ? vi[cur]->qos : 0;
		if (vi[cur]->vlan_proto) {
			vlans->vlans[cur].vf_vlan_proto = ntohs(vi[cur]->vlan_proto);
		} else {
			vlans->vlans[cur].vf_vlan_proto = ETH_P_8021Q;
		}
		cur++;
	}

	*nvi = vlans;
	return 0;
}

/* Fill the IFLA_VF_VLAN attribute */
static void sriov_fill_vf_vlan(struct nl_msg *msg, nl_vf_vlan_info_t *vinfo,
			       uint32_t index) {
	struct ifla_vf_vlan vlan;

	vlan.vf = index;
	vlan.vlan = vinfo[0].vf_vlan;
	vlan.qos = vinfo[0].vf_vlan_qos;
	NLA_PUT(msg, IFLA_VF_VLAN, sizeof(vlan), &vlan);

nla_put_failure:
	return;
}

/* Fill the IFLA_VF_VLAN_LIST attribute */
static int sriov_fill_vf_vlan_list(struct nl_msg *msg, nl_vf_vlans_t *vlans,
				   uint32_t index) {
	int cur = 0;
	nl_vf_vlan_info_t *vlan_info = vlans->vlans;
	struct ifla_vf_vlan_info vlan;
	struct nlattr *list;

	if (!(list = nla_nest_start(msg, IFLA_VF_VLAN_LIST)))
		return -NLE_MSGSIZE;

	vlan.vf = index;
	while (cur < vlans->size) {
		vlan.vlan = vlan_info[cur].vf_vlan;
		vlan.qos = vlan_info[cur].vf_vlan_qos;
		vlan.vlan_proto = vlan_info[cur].vf_vlan_proto;

		NLA_PUT(msg, IFLA_VF_VLAN_INFO, sizeof(vlan), &vlan);

		cur++;
	}

nla_put_failure:
	nla_nest_end(msg, list);

	return 0;
}

/* Fill individual IFLA_VF_INFO attributes */
static int sriov_fill_vfinfo(struct nl_msg *msg,
			     struct rtnl_link_vf *vf_data) {
	int err = 0, new_rate = 0;
	nl_vf_vlans_t *vlan_list;
	nl_vf_vlan_info_t *vlan_info;
	struct ifla_vf_guid vf_node_guid;
	struct ifla_vf_guid vf_port_guid;
	struct ifla_vf_link_state vf_link_state;
	struct ifla_vf_mac vf_mac;
	struct ifla_vf_rate new_vf_rate;
	struct ifla_vf_rss_query_en vf_rss_query_en;
	struct ifla_vf_spoofchk vf_spoofchk;
	struct ifla_vf_trust vf_trust;
	struct ifla_vf_tx_rate vf_rate;
	struct nlattr *list;
	uint16_t proto;

	if (!(vf_data->ce_mask & SRIOV_ATTR_INDEX))
		return -NLE_MISSING_ATTR;

	if (!(list = nla_nest_start(msg, IFLA_VF_INFO)))
		return -NLE_MSGSIZE;

	/* IFLA_VF_MAC */
	if (vf_data->ce_mask & SRIOV_ATTR_ADDR) {
		vf_mac.vf = vf_data->vf_index;
		memset(vf_mac.mac, 0, sizeof(vf_mac.mac));
		memcpy(vf_mac.mac, nl_addr_get_binary_addr(vf_data->vf_lladdr),
		       nl_addr_get_len(vf_data->vf_lladdr));
		NLA_PUT(msg, IFLA_VF_MAC, sizeof(vf_mac), &vf_mac);
	}

	/* IFLA_VF_VLAN IFLA_VF_VLAN_LIST */
	if (vf_data->ce_mask & SRIOV_ATTR_VLAN) {
		vlan_list = vf_data->vf_vlans;
		vlan_info = vlan_list->vlans;
		proto = vlan_info[0].vf_vlan_proto;
		if (!proto)
			proto = ETH_P_8021Q;

		if ((vlan_list->size == 1) && (proto == ETH_P_8021Q))
			sriov_fill_vf_vlan(msg, vlan_info, vf_data->vf_index);
		else
			err = sriov_fill_vf_vlan_list(msg, vlan_list,
						      vf_data->vf_index);
	}

	/* IFLA_VF_TX_RATE */
	if (vf_data->ce_mask & SRIOV_ATTR_TX_RATE) {
		vf_rate.vf = vf_data->vf_index;
		vf_rate.rate = vf_data->vf_rate;

		NLA_PUT(msg, IFLA_VF_TX_RATE, sizeof(vf_rate), &vf_rate);
	}

	/* IFLA_VF_RATE */
	new_vf_rate.min_tx_rate = 0;
	new_vf_rate.max_tx_rate = 0;
	new_vf_rate.vf = vf_data->vf_index;
	if (vf_data->ce_mask & SRIOV_ATTR_RATE_MIN) {
		new_vf_rate.min_tx_rate = vf_data->vf_min_tx_rate;
		new_rate = 1;
	}
	if (vf_data->ce_mask & SRIOV_ATTR_RATE_MAX) {
		new_vf_rate.max_tx_rate = vf_data->vf_max_tx_rate;
		new_rate = 1;
	}
	if (new_rate)
		NLA_PUT(msg, IFLA_VF_RATE, sizeof(new_vf_rate), &new_vf_rate);

	/* IFLA_VF_SPOOFCHK */
	if (vf_data->ce_mask & SRIOV_ATTR_SPOOFCHK) {
		vf_spoofchk.vf = vf_data->vf_index;
		vf_spoofchk.setting = vf_data->vf_spoofchk;

		NLA_PUT(msg, IFLA_VF_SPOOFCHK, sizeof(vf_spoofchk),
			&vf_spoofchk);
	}

	/* IFLA_VF_LINK_STATE */
	if (vf_data->ce_mask & SRIOV_ATTR_LINK_STATE) {
		vf_link_state.vf = vf_data->vf_index;
		vf_link_state.link_state = vf_data->vf_linkstate;

		NLA_PUT(msg, IFLA_VF_LINK_STATE, sizeof(vf_link_state),
			&vf_link_state);
	}

	/* IFLA_VF_RSS_QUERY_EN */
	if (vf_data->ce_mask & SRIOV_ATTR_RSS_QUERY_EN) {
		vf_rss_query_en.vf = vf_data->vf_index;
		vf_rss_query_en.setting = vf_data->vf_rss_query_en;

		NLA_PUT(msg, IFLA_VF_RSS_QUERY_EN, sizeof(vf_rss_query_en),
			&vf_rss_query_en);
	}

	/* IFLA_VF_TRUST */
	if (vf_data->ce_mask & SRIOV_ATTR_TRUST) {
		vf_trust.vf = vf_data->vf_index;
		vf_trust.setting = vf_data->vf_trust;

		NLA_PUT(msg, IFLA_VF_TRUST, sizeof(vf_trust), &vf_trust);
	}

	/* IFLA_VF_IB_NODE_GUID */
	if (vf_data->ce_mask & SRIOV_ATTR_IB_NODE_GUID) {
		vf_node_guid.vf = vf_data->vf_index;
		vf_node_guid.guid = vf_data->vf_guid_node;

		NLA_PUT(msg, IFLA_VF_IB_NODE_GUID, sizeof(vf_node_guid),
			&vf_node_guid);
	}

	/* IFLA_VF_IB_PORT_GUID */
	if (vf_data->ce_mask & SRIOV_ATTR_IB_PORT_GUID) {
		vf_port_guid.vf = vf_data->vf_index;
		vf_port_guid.guid = vf_data->vf_guid_port;

		NLA_PUT(msg, IFLA_VF_IB_PORT_GUID, sizeof(vf_port_guid),
			&vf_port_guid);
	}

nla_put_failure:
	nla_nest_end(msg, list);

	return err;
}

/* Fill the IFLA_VFINFO_LIST attribute */
int rtnl_link_sriov_fill_vflist(struct nl_msg *msg, struct rtnl_link *link) {
	int err = 0;
	struct nlattr *data;
	struct rtnl_link_vf *list, *vf, *next;

	if (!(err = rtnl_link_has_vf_list(link)))
		return 0;

	if (!(data = nla_nest_start(msg, IFLA_VFINFO_LIST)))
		return -NLE_MSGSIZE;

	list = link->l_vf_list;
	nl_list_for_each_entry_safe(vf, next, &list->vf_list, vf_list) {
		if (vf->ce_mask & SRIOV_ATTR_INDEX) {
			if ((err = sriov_fill_vfinfo(msg, vf)) < 0)
				goto nla_nest_list_failure;
		}
	}

nla_nest_list_failure:
	nla_nest_end(msg, data);

	return err;
}

/* Parse IFLA_VFINFO_LIST and IFLA_VF_INFO attributes */
int rtnl_link_sriov_parse_vflist(struct rtnl_link *link, struct nlattr **tb) {
	int err, len, list_len, list_rem;
	struct ifla_vf_mac *vf_lladdr;
	struct ifla_vf_vlan *vf_vlan;
	struct ifla_vf_vlan_info *vf_vlan_info[MAX_VLAN_LIST_LEN];
	struct ifla_vf_tx_rate *vf_tx_rate;
	struct ifla_vf_spoofchk *vf_spoofchk;
	struct ifla_vf_link_state *vf_linkstate;
	struct ifla_vf_rate *vf_rate;
	struct ifla_vf_rss_query_en *vf_rss_query;
	struct ifla_vf_trust *vf_trust;
	struct nlattr *nla, *nla_list, *t[IFLA_VF_MAX+1],
		*stb[RTNL_LINK_VF_STATS_MAX+1];
	nl_vf_vlans_t *vf_vlans = NULL;
	struct rtnl_link_vf *vf_data, *vf_head = NULL;

	len = nla_len(tb[IFLA_VFINFO_LIST]);
	link->l_vf_list = rtnl_link_vf_alloc();
	if (!link->l_vf_list)
		return -NLE_NOMEM;
	vf_head = link->l_vf_list;

	for (nla = nla_data(tb[IFLA_VFINFO_LIST]); nla_ok(nla, len);
	     nla = nla_next(nla, &len)) {
		err = nla_parse(t, IFLA_VF_MAX, nla_data(nla), nla_len(nla),
				sriov_info_policy);
		if (err < 0)
			return err;

		vf_data = rtnl_link_vf_alloc();
		if (!vf_data)
			return -NLE_NOMEM;

		if (t[IFLA_VF_MAC]) {
			vf_lladdr = nla_data(t[IFLA_VF_MAC]);

			vf_data->vf_index = vf_lladdr->vf;
			vf_data->ce_mask |= SRIOV_ATTR_INDEX;

			vf_data->vf_lladdr = nl_addr_build(AF_LLC,
							   vf_lladdr->mac, 6);
			if (vf_data->vf_lladdr == NULL) {
				rtnl_link_vf_put(vf_data);
				return -NLE_NOMEM;
			}
			nl_addr_set_family(vf_data->vf_lladdr, AF_LLC);
			vf_data->ce_mask |= SRIOV_ATTR_ADDR;
		}

		if (t[IFLA_VF_VLAN_LIST]) {
			list_len = 0;
			nla_for_each_nested(nla_list, t[IFLA_VF_VLAN_LIST],
					    list_rem) {
				if (list_len >= MAX_VLAN_LIST_LEN)
					break;
				vf_vlan_info[list_len] = nla_data(nla_list);
				list_len++;
			}

			err = rtnl_link_vf_vlan_info(list_len, vf_vlan_info,
						     &vf_vlans);
			if (err < 0) {
				rtnl_link_vf_put(vf_data);
				return err;
			}

			vf_data->vf_vlans = vf_vlans;
			vf_data->ce_mask |= SRIOV_ATTR_VLAN;
		} else if (t[IFLA_VF_VLAN]) {
			vf_vlan = nla_data(t[IFLA_VF_VLAN]);

			if (vf_vlan->vlan) {
				err = rtnl_link_vf_vlan_alloc(&vf_vlans, 1);
				if (err < 0) {
					rtnl_link_vf_put(vf_data);
					return err;
				}

				vf_vlans->vlans[0].vf_vlan = vf_vlan->vlan;
				vf_vlans->vlans[0].vf_vlan_qos = vf_vlan->qos;
				vf_vlans->vlans[0].vf_vlan_proto = ETH_P_8021Q;

				vf_data->vf_vlans = vf_vlans;
				vf_data->ce_mask |= SRIOV_ATTR_VLAN;
			}
		}

		if (t[IFLA_VF_TX_RATE]) {
			vf_tx_rate = nla_data(t[IFLA_VF_TX_RATE]);

			if (vf_tx_rate->rate) {
				vf_data->vf_rate = vf_tx_rate->rate;
				vf_data->ce_mask |= SRIOV_ATTR_TX_RATE;
			}
		}

		if (t[IFLA_VF_SPOOFCHK]) {
			vf_spoofchk = nla_data(t[IFLA_VF_SPOOFCHK]);

			if (vf_spoofchk->setting != ((uint32_t)-1)) {
				vf_data->vf_spoofchk = vf_spoofchk->setting ? 1 : 0;
				vf_data->ce_mask |= SRIOV_ATTR_SPOOFCHK;
			}
		}

		if (t[IFLA_VF_LINK_STATE]) {
			vf_linkstate = nla_data(t[IFLA_VF_LINK_STATE]);

			vf_data->vf_linkstate = vf_linkstate->link_state;
			vf_data->ce_mask |= SRIOV_ATTR_LINK_STATE;
		}

		if (t[IFLA_VF_RATE]) {
			vf_rate = nla_data(t[IFLA_VF_RATE]);

			if (vf_rate->max_tx_rate) {
				vf_data->vf_max_tx_rate = vf_rate->max_tx_rate;
				vf_data->ce_mask |= SRIOV_ATTR_RATE_MAX;
			}
			if (vf_rate->min_tx_rate) {
				vf_data->vf_min_tx_rate = vf_rate->min_tx_rate;
				vf_data->ce_mask |= SRIOV_ATTR_RATE_MIN;
			}
		}

		if (t[IFLA_VF_RSS_QUERY_EN]) {
			vf_rss_query = nla_data(t[IFLA_VF_RSS_QUERY_EN]);

			if (vf_rss_query->setting != ((uint32_t)-1)) {
				vf_data->vf_rss_query_en = vf_rss_query->setting ? 1 : 0;
				vf_data->ce_mask |= SRIOV_ATTR_RSS_QUERY_EN;
			}
		}

		if (t[IFLA_VF_STATS]) {
			err = nla_parse_nested(stb, RTNL_LINK_VF_STATS_MAX,
					       t[IFLA_VF_STATS],
					       sriov_stats_policy);
			if (err < 0) {
				rtnl_link_vf_put(vf_data);
				return err;
			}

			SET_VF_STAT(link, cur, stb,
				    RTNL_LINK_VF_STATS_RX_PACKETS,
				    IFLA_VF_STATS_RX_PACKETS);
			SET_VF_STAT(link, cur, stb,
				    RTNL_LINK_VF_STATS_TX_PACKETS,
				    IFLA_VF_STATS_TX_PACKETS);
			SET_VF_STAT(link, cur, stb,
				    RTNL_LINK_VF_STATS_RX_BYTES,
				    IFLA_VF_STATS_RX_BYTES);
			SET_VF_STAT(link, cur, stb,
				    RTNL_LINK_VF_STATS_TX_BYTES,
				    IFLA_VF_STATS_TX_BYTES);
			SET_VF_STAT(link, cur, stb,
				    RTNL_LINK_VF_STATS_BROADCAST,
				    IFLA_VF_STATS_BROADCAST);
			SET_VF_STAT(link, cur, stb,
				    RTNL_LINK_VF_STATS_MULTICAST,
				    IFLA_VF_STATS_MULTICAST);

			vf_data->ce_mask |= SRIOV_ATTR_STATS;
		}

		if (t[IFLA_VF_TRUST]) {
			vf_trust = nla_data(t[IFLA_VF_TRUST]);

			if (vf_trust->setting != ((uint32_t)-1)) {
				vf_data->vf_trust = vf_trust->setting ? 1 : 0;
				vf_data->ce_mask |= SRIOV_ATTR_TRUST;
			}
		}

		nl_list_add_head(&vf_data->vf_list, &vf_head->vf_list);
		vf_head = vf_data;
	}

	return 0;
}

/**
 * @name SR-IOV Sub-Object
 * @{
 */

/**
 * Add a SRIOV VF object to a link object
 * @param link  	Link object to add to
 * @param vf_data 	SRIOV VF object to add
 *
 * @return 0 if SRIOV VF object added successfully
 * @return -NLE_OBJ_NOTFOUND if \p link or \p vf_data not provided
 * @return -NLE_NOMEM if out of memory
 */
int rtnl_link_vf_add(struct rtnl_link *link, struct rtnl_link_vf *vf_data) {
	struct rtnl_link_vf *vf_head = NULL;

	if (!link||!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (!link->l_vf_list) {
		link->l_vf_list = rtnl_link_vf_alloc();
		if (!link->l_vf_list)
			return -NLE_NOMEM;
	}

	vf_head = vf_data;
	vf_head->ce_refcnt++;

	vf_head = link->l_vf_list;
	nl_list_add_head(&vf_data->vf_list, &vf_head->vf_list);
	link->l_vf_list = vf_head;

	rtnl_link_set_vf_list(link);

	return 0;
}

/**
 * Allocate a new SRIOV VF object
 *
 * @return NULL if out of memory
 * @return New VF Object
 *
 * @see rtnl_link_vf_put()
 *
 * The SRIOV VF object must be returned to the link object with
 * rtnl_link_vf_put() when operations are done to prevent memory leaks.
 */
struct rtnl_link_vf *rtnl_link_vf_alloc(void) {
	struct rtnl_link_vf *vf;

	if (!(vf = calloc(1, sizeof(*vf))))
		return NULL;

	NL_INIT_LIST_HEAD(&vf->vf_list);
	vf->ce_refcnt = 1;

	NL_DBG(4, "Allocated new SRIOV VF object %p\n", vf);

	return vf;
}

/**
 * Free SRIOV VF object.
 * @arg vf_data 	SRIOV VF data object
 */
void rtnl_link_vf_free(struct rtnl_link_vf *vf_data) {
	if (!vf_data)
		return;

	if (vf_data->ce_refcnt > 0)
		NL_DBG(1, "Warning: Freeing SRIOV VF object in use...\n");

	if (vf_data->ce_mask & SRIOV_ATTR_ADDR)
		nl_addr_put(vf_data->vf_lladdr);
	if (vf_data->ce_mask & SRIOV_ATTR_VLAN)
		rtnl_link_vf_vlan_put(vf_data->vf_vlans);

	NL_DBG(4, "Freed SRIOV VF object %p\n", vf_data);
	free(vf_data);

	return;
}

/**
 * Lookup SRIOV VF in link object by VF index.
 *
 * @return NULL if VF not found
 * @return VF Object
 *
 * @see rtnl_link_vf_put()
 *
 * The SRIOV VF object must be returned to the link object with
 * rtnl_link_vf_put() when operations are done to prevent memory leaks.
 */
struct rtnl_link_vf *rtnl_link_vf_get(struct rtnl_link *link, uint32_t vf_num) {
	struct rtnl_link_vf *list, *vf, *next, *ret = NULL;

	list = link->l_vf_list;
	nl_list_for_each_entry_safe(vf, next, &list->vf_list, vf_list) {
		if (vf->vf_index == vf_num) {
			ret = vf;
			break;
		}
	}

	if (ret) {
		ret->ce_refcnt++;
		NL_DBG(4, "New reference to SRIOV VF object %p, total %i\n",
		       ret, ret->ce_refcnt);
	}

	return ret;
}

/**
 * Return SRIOV VF object to the owning link object.
 * @arg vf_data 	SRIOV VF data object
 *
 * @see rtnl_link_vf_alloc()
 * @see rtnl_link_vf_get()
 */
void rtnl_link_vf_put(struct rtnl_link_vf *vf_data) {
	if (!vf_data)
		return;

	vf_data->ce_refcnt--;
	NL_DBG(4, "Returned SRIOV VF object reference %p, %i remaining\n",
	       vf_data, vf_data->ce_refcnt);

	if (vf_data->ce_refcnt < 0)
		BUG();

	if (vf_data->ce_refcnt <= 0)
		rtnl_link_vf_free(vf_data);

	return;
}

/**
 * Get link layer address of SRIOV Virtual Function
 * @arg vf_data 	SRIOV VF object
 * @arg addr 		Pointer to store Link Layer address
 *
 * @see rtnl_link_get_num_vf()
 * @see rtnl_link_vf_set_addr()
 *
 * @copydoc pointer_lifetime_warning
 * @return 0 if addr is present and addr is set to pointer containing address
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the link layer address is not set
 */
int rtnl_link_vf_get_addr(struct rtnl_link_vf *vf_data, struct nl_addr **addr)
{
	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (vf_data->ce_mask & SRIOV_ATTR_ADDR)
		*addr = vf_data->vf_lladdr;
	else
		return -NLE_NOATTR;

	return 0;
}

/**
 * Set link layer address of SRIOV Virtual Function object
 * @param vf_data 	SRIOV VF object
 * @param addr 		New link layer address
 *
 * This function increments the reference counter of the address object
 * and overwrites any existing link layer address previously assigned.
 *
 * @see rtnl_link_vf_get_addr()
 */
void rtnl_link_vf_set_addr(struct rtnl_link_vf *vf_data, struct nl_addr *addr) {
	if (vf_data->vf_lladdr)
		nl_addr_put(vf_data->vf_lladdr);

	nl_addr_get(addr);
	vf_data->vf_lladdr = addr;
	vf_data->ce_mask |= SRIOV_ATTR_ADDR;

	return;
}

/**
 * Set the Infiniband node GUID for the SRIOV Virtual Function object
 * @param vf_data 	SRIOV VF object
 * @param guid  	node GUID
 */
void rtnl_link_vf_set_ib_node_guid(struct rtnl_link_vf *vf_data,
				   uint64_t guid) {
	vf_data->vf_guid_node = guid;
	vf_data->ce_mask |= SRIOV_ATTR_IB_NODE_GUID;

	return;
}

/**
 * Set the Infiniband port GUID for the SRIOV Virtual Function object
 * @param vf_data 	SRIOV VF object
 * @param guid  	port GUID
 */
void rtnl_link_vf_set_ib_port_guid(struct rtnl_link_vf *vf_data,
				   uint64_t guid) {
	vf_data->vf_guid_port = guid;
	vf_data->ce_mask |= SRIOV_ATTR_IB_PORT_GUID;

	return;
}

/**
 * Get index of SRIOV Virtual Function
 * @arg vf_data 	SRIOV VF object
 * @arg vf_index 	Pointer to store VF index
 *
 * @see rtnl_link_get_num_vf()
 *
 * @return 0 if index is present and vf_index is set
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the VF index is not set
 */
int rtnl_link_vf_get_index(struct rtnl_link_vf *vf_data, uint32_t *vf_index)
{
	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (vf_data->ce_mask & SRIOV_ATTR_INDEX)
		*vf_index = vf_data->vf_index;
	else
		return -NLE_NOATTR;

	return 0;
}

/**
 * Set index of SRIOV Virtual Function object
 * @param vf_data 	SRIOV VF object
 * @param vf_index 	Index value
 *
 * @see rtnl_link_vf_get_index()
 */
void rtnl_link_vf_set_index(struct rtnl_link_vf *vf_data, uint32_t vf_index)
{
	vf_data->vf_index = vf_index;
	vf_data->ce_mask |= SRIOV_ATTR_INDEX;

	return;
}

/**
 * Get link state of SRIOV Virtual Function
 * @arg vf_data  	SRIOV VF object
 * @arg vf_linkstate 	Pointer to store VF link state
 *
 * @see rtnl_link_get_num_vf()
 * @see rtnl_link_set_linkstate()
 *
 * @return 0 if link state is present and vf_linkstate is set
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the VF link state is not set
 */
int rtnl_link_vf_get_linkstate(struct rtnl_link_vf *vf_data,
			       uint32_t *vf_linkstate)
{
	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (vf_data->ce_mask & SRIOV_ATTR_LINK_STATE)
		*vf_linkstate = vf_data->vf_linkstate;
	else
		return -NLE_NOATTR;

	return 0;
}

/**
 * Set link state of SRIOV Virtual Function object
 * @param vf_data 	SRIOV VF object
 * @param vf_linkstate Link state value
 *
 * @see rtnl_link_get_linkstate()
 *
 * Not all hardware supports setting link state. If the feature is unsupported,
 * the link change request will fail with -NLE_OPNOTSUPP
 */
void rtnl_link_vf_set_linkstate(struct rtnl_link_vf *vf_data,
				uint32_t vf_linkstate) {
	vf_data->vf_linkstate = vf_linkstate;
	vf_data->ce_mask |= SRIOV_ATTR_LINK_STATE;

	return;
}

/**
 * Get TX Rate Limit of SRIOV Virtual Function
 * @arg vf_data 	SRIOV VF object
 * @arg vf_rate 	Pointer to store VF rate limiting data
 *
 * @see rtnl_link_get_num_vf()
 * @see rtnl_link_set_rate()
 *
 * When the older rate API has been implemented, the rate member of the struct
 * will be set, and the api member will be set to RTNL_LINK_VF_API_OLD.
 * When the newer rate API has been implemented, the max_tx_rate
 * and/or the minx_tx_rate will be set, and the api member will be set to
 * RTNL_LINK_VF_API_NEW.
 *
 * Old rate API supports only a maximum TX rate.
 *   ip link set dev vf 0 rate
 * New rate API supports minumum and maximum TX rates.
 *   ip link set dev vf 0 min_tx_rate
 *   ip link set dev vf 0 max_tx_rate
 *
 * @return 0 if rate is present and vf_rate is set
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the VF rate is not set
 */
int rtnl_link_vf_get_rate(struct rtnl_link_vf *vf_data,
			  struct nl_vf_rate *vf_rate)
{
	int set = 0;

	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	vf_rate->api = RTNL_LINK_VF_RATE_API_UNSPEC;
	vf_rate->rate = 0;
	vf_rate->max_tx_rate = 0;
	vf_rate->min_tx_rate = 0;

	if (vf_data->ce_mask & SRIOV_ATTR_RATE_MAX) {
		if (vf_data->vf_max_tx_rate) {
			vf_rate->api = RTNL_LINK_VF_RATE_API_NEW;
			vf_rate->max_tx_rate = vf_data->vf_max_tx_rate;
			set = 1;
		}
	}
	if (vf_data->ce_mask & SRIOV_ATTR_RATE_MIN) {
		if (vf_data->vf_min_tx_rate) {
			vf_rate->api = RTNL_LINK_VF_RATE_API_NEW;
			vf_rate->min_tx_rate = vf_data->vf_min_tx_rate;
			set = 1;
		}
	}
	if ((!set) && (vf_data->ce_mask & SRIOV_ATTR_TX_RATE)) {
		if (vf_data->vf_rate) {
			vf_rate->api = RTNL_LINK_VF_RATE_API_OLD;
			vf_rate->rate = vf_data->vf_rate;
			set = 1;
		}
	}

	if (!set)
		return -NLE_NOATTR;

	return 0;
}

/**
 * Set TX Rate Limit of SRIOV Virtual Function object
 * @param vf_data 	SRIOV VF object
 * @param vf_rate 	Rate limiting structure
 *
 * @see rtnl_link_vf_get_rate()
 *
 * When setting the rate, the API level must be specificed.
 * Valid API levels:
 *   RTNL_LINK_VF_RATE_API_NEW
 *   RTNL_LINK_VF_RATE_API_OLD
 *
 * When using the new API, if either the min_tx_rate or
 * max_tx_rate has been set, and the other is being changed,
 * you must specify the currently set values to preserve
 * them. If this is not done, that setting will be disabled.
 *
 * Old rate API supports only a maximum TX rate.
 *   ip link set dev vf 0 rate
 * New rate API supports minumum and maximum TX rates.
 *   ip link set dev vf 0 min_tx_rate
 *   ip link set dev vf 0 max_tx_rate
 *
 * Not all hardware supports min_tx_rate.
 */
void rtnl_link_vf_set_rate(struct rtnl_link_vf *vf_data,
			   struct nl_vf_rate *vf_rate) {
	if (vf_rate->api == RTNL_LINK_VF_RATE_API_OLD) {
		vf_data->vf_rate = vf_rate->rate;
		vf_data->ce_mask |= SRIOV_ATTR_TX_RATE;
	} else if (vf_rate->api == RTNL_LINK_VF_RATE_API_NEW) {
		vf_data->vf_max_tx_rate = vf_rate->max_tx_rate;
		vf_data->ce_mask |= SRIOV_ATTR_RATE_MAX;

		vf_data->vf_min_tx_rate = vf_rate->min_tx_rate;
		vf_data->ce_mask |= SRIOV_ATTR_RATE_MIN;
	}

	return;
}

/**
 * Get RSS Query EN value of SRIOV Virtual Function
 * @arg vf_data 	SRIOV VF object
 * @arg vf_rss_query_en	Pointer to store VF RSS Query value
 *
 * @see rtnl_link_get_num_vf()
 * @see rtnl_link_vf_set_rss_query_en()
 *
 * @return 0 if rss_query_en is present and vf_rss_query_en is set
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the VF RSS Query EN value is not set
 */
int rtnl_link_vf_get_rss_query_en(struct rtnl_link_vf *vf_data,
				  uint32_t *vf_rss_query_en)
{
	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (vf_data->ce_mask & SRIOV_ATTR_RSS_QUERY_EN)
		*vf_rss_query_en = vf_data->vf_rss_query_en;
	else
		return -NLE_NOATTR;

	return 0;
}

/**
 * Set RSS configuration querying of SRIOV Virtual Function Object
 * @arg vf_data 	SRIOV VF object
 * @arg vf_rss_query_en	RSS Query value
 *
 * @see rtnl_link_vf_get_rss_query_en()
 */
void rtnl_link_vf_set_rss_query_en(struct rtnl_link_vf *vf_data,
				  uint32_t vf_rss_query_en) {
	vf_data->vf_rss_query_en = vf_rss_query_en;
	vf_data->ce_mask |= SRIOV_ATTR_RSS_QUERY_EN;

	return;
}

/**
 * Get spoof checking value of SRIOV Virtual Function
 * @arg vf_data 	SRIOV VF object
 * @arg vf_spoofchk 	Pointer to store VF spoofchk value
 *
 * @see rtnl_link_get_num_vf()
 * @see rtnl_link_set_spoofchk()
 *
 * @return 0 if spoofchk is present and vf_spoofchk is set
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the VF spoofcheck is not set
 */
int rtnl_link_vf_get_spoofchk(struct rtnl_link_vf *vf_data,
			      uint32_t *vf_spoofchk)
{
	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (vf_data->ce_mask & SRIOV_ATTR_SPOOFCHK)
		*vf_spoofchk = vf_data->vf_spoofchk;
	else
		return -NLE_NOATTR;

	return 0;
}

/**
 * Set spoof checking value of SRIOV Virtual Function Object
 * @param vf_data
 * @param vf_spoofchk
 *
 * @see rtnl_link_vf_get_spoofchk()
 */
void rtnl_link_vf_set_spoofchk(struct rtnl_link_vf *vf_data,
			       uint32_t vf_spoofchk) {
	vf_data->vf_spoofchk = vf_spoofchk;
	vf_data->ce_mask |= SRIOV_ATTR_SPOOFCHK;

	return;
}

/**
 * Get value of stat counter for SRIOV Virtual Function
 * @arg vf_data 	SRIOV VF object
 * @arg stat 		Identifier of statistical counter
 * @arg vf_stat 	Pointer to store VF stat value in
 *
 * @see rtnl_link_get_num_vf()
 *
 * @return 0 if stat is present and vf_stat is set
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the VF stat is not set
 */
int rtnl_link_vf_get_stat(struct rtnl_link_vf *vf_data,
			  rtnl_link_vf_stats_t stat, uint64_t *vf_stat)
{
	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (vf_data->ce_mask & SRIOV_ATTR_STATS)
		*vf_stat = vf_data->vf_stats[stat];
	else
		return -NLE_NOATTR;

	return 0;
}

/**
 * Get trust setting of SRIOV Virtual Function
 * @arg vf_data 	SRIOV VF object
 * @arg vf_trust 	Pointer to store VF trust value
 *
 * @see rtnl_link_get_num_vf()
 * @see rtnl_link_set_trust()
 *
 * @return 0 if trust is present and vf_trust is set
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the VF trust setting is not set
 */
int rtnl_link_vf_get_trust(struct rtnl_link_vf *vf_data, uint32_t *vf_trust)
{
	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (vf_data->ce_mask & SRIOV_ATTR_TRUST)
		*vf_trust = vf_data->vf_trust;
	else
		return -NLE_NOATTR;

	return 0;
}

/**
 * Set user trust setting on SRIOV Virtual Function Object
 * @param vf_data
 * @param vf_trust
 *
 * @see rtnl_link_vf_get_trust()
 */
void rtnl_link_vf_set_trust(struct rtnl_link_vf *vf_data, uint32_t vf_trust) {
	vf_data->vf_trust = vf_trust;
	vf_data->ce_mask |= SRIOV_ATTR_TRUST;

	return;
}

/**
 * Get an array of VLANS on SRIOV Virtual Function
 * @arg vf_data 	SRIOV VF object
 * @arg vf_vlans 	Pointer to nl_vf_vlans_t struct to store vlan info.
 *
 * @see rtnl_link_get_num_vf()
 *
 * The SRIOV VF VLANs object must be returned to the SRIOV VF object with
 * rtnl_link_vf_vlans_put() when operations are done to prevent memory leaks.
 *
 * @copydoc pointer_lifetime_warning
 * @return 0 if VLAN info is present and vf_vlans is set
 * @return -NLE_OBJ_NOTFOUND if information for VF info is not found
 * @return -NLE_NOATTR if the VF vlans is not set
 */
int rtnl_link_vf_get_vlans(struct rtnl_link_vf *vf_data,
			   nl_vf_vlans_t **vf_vlans) {
	nl_vf_vlans_t *vf;

	if (!vf_data)
		return -NLE_OBJ_NOTFOUND;

	if (vf_data->ce_mask & SRIOV_ATTR_VLAN) {
		vf = vf_data->vf_vlans;
		vf->ce_refcnt++;
		*vf_vlans = vf;
	} else
		return -NLE_NOATTR;

	return 0;
}

/**
 * Add a SRIOV VF VLANs object to the SRIOV Virtual Function Object
 * @param vf_data 	SRIOV VF object
 * @param vf_vlans 	SRIOV VF VLANs object
 *
 * @see rtnl_link_vf_get_vlans()
 * @see rtnl_link_vf_vlan_alloc()
 *
 * This function assigns ownership of the SRIOV VF object \p vf_vlans
 * to the SRIOV Virtual Function object \p vf_data. Do not use
 * rtnl_link_vf_vlan_put() on \p vf_vlans after this.
 */
void rtnl_link_vf_set_vlans(struct rtnl_link_vf *vf_data,
			    nl_vf_vlans_t *vf_vlans) {
	if (!vf_data||!vf_vlans)
		return;

	vf_data->vf_vlans = vf_vlans;
	vf_data->vf_vlans->ce_refcnt++;
	vf_data->ce_mask |= SRIOV_ATTR_VLAN;

	return;
}

/**
 * Allocate a SRIOV VF VLAN object
 * @param vf_vlans 	Pointer to store VLAN object at
 * @param vlan_count 	Number of VLANs that will be stored in VLAN object
 *
 * The SRIOV VF VLANs object must be returned to the sRIOV VF object with
 * rtnl_link_vf_vlan_put() when operations are done to prevent memory leaks.
 *
 * @return 0 if VLAN object is created and vf_vlans is set.
 * @return -NLE_NOMEM if object could not be allocated.
 * @return -NLE_INVAL if vlan_count is more than supported by SRIOV VF
 */
int rtnl_link_vf_vlan_alloc(nl_vf_vlans_t **vf_vlans, int vlan_count) {
	nl_vf_vlans_t *vlans;
	nl_vf_vlan_info_t *vlan_info;

	if (vlan_count > MAX_VLAN_LIST_LEN)
		return -NLE_INVAL;

	vlans = calloc(1, sizeof(*vlans));
	if (!vlans)
		return -NLE_NOMEM;

	vlan_info = calloc(vlan_count+1, sizeof(*vlan_info));
	if (!vlan_info) {
		free(vlans);
		return -NLE_NOMEM;
	}

	NL_DBG(4, "Allocated new SRIOV VF VLANs object %p\n", vlans);

	vlans->ce_refcnt = 1;
	vlans->size = vlan_count;
	vlans->vlans = vlan_info;
	*vf_vlans = vlans;

	return 0;
}

/**
 * Free an allocated SRIOV VF VLANs object
 * @param vf_vlans 	SRIOV VF VLANs object
 */
void rtnl_link_vf_vlan_free(nl_vf_vlans_t *vf_vlans) {
	if (!vf_vlans)
		return;

	if (vf_vlans->ce_refcnt > 0)
		NL_DBG(1, "Warning: Freeing SRIOV VF VLANs object in use...\n");

	NL_DBG(4, "Freed SRIOV VF object %p\n", vf_vlans);
	free(vf_vlans->vlans);
	free(vf_vlans);

	return;
}

/**
 * Return SRIOV VF VLANs object to the owning SRIOV VF object.
 * @param vf_vlans 	SRIOV VF VLANs object
 */
void rtnl_link_vf_vlan_put(nl_vf_vlans_t *vf_vlans) {
	if (!vf_vlans)
		return;

	vf_vlans->ce_refcnt--;
	NL_DBG(4, "Returned SRIOV VF VLANs object reference %p, %i remaining\n",
	       vf_vlans, vf_vlans->ce_refcnt);

	if (vf_vlans->ce_refcnt < 0)
		BUG();

	if (vf_vlans->ce_refcnt <= 0)
		rtnl_link_vf_vlan_free(vf_vlans);

	return;
}

/** @} */

/**
 * @name Utilities
 * @{
 */

static const struct trans_tbl vf_link_states[] = {
	__ADD(IFLA_VF_LINK_STATE_AUTO, autodetect),
	__ADD(IFLA_VF_LINK_STATE_ENABLE, up),
	__ADD(IFLA_VF_LINK_STATE_DISABLE, down),
};

char *rtnl_link_vf_linkstate2str(uint32_t ls, char *buf, size_t len)
{
	return __type2str(ls, buf, len, vf_link_states,
			  ARRAY_SIZE(vf_link_states));
}

int rtnl_link_vf_str2linkstate(const char *name)
{
	return __str2type(name, vf_link_states, ARRAY_SIZE(vf_link_states));
}

static const struct trans_tbl vf_vlan_proto[] = {
	__ADD(ETH_P_8021Q, 8021Q),
	__ADD(ETH_P_8021AD, 8021AD),
};

char *rtnl_link_vf_vlanproto2str(uint16_t proto, char *buf, size_t len)
{
	return __type2str(proto, buf, len, vf_vlan_proto,
			  ARRAY_SIZE(vf_vlan_proto));
}

int rtnl_link_vf_str2vlanproto(const char *name)
{
	return __str2type(name, vf_vlan_proto, ARRAY_SIZE(vf_vlan_proto));
}

/* Return a guid from a format checked string.
 * Format string must be xx:xx:xx:xx:xx:xx:xx:xx where XX can be an
 * arbitrary hex digit
 *
 * Function modified from original at iproute2/lib/utils.c:get_guid()
 * Original by Eli Cohen <eli@mellanox.com>.
 * iproute2 git commit d91fb3f4c7e4dba806541bdc90b1fb60a3581541
 */
int rtnl_link_vf_str2guid(uint64_t *guid, const char *guid_s) {
	unsigned long int tmp;
	char *endptr;
	int i;

	if (strlen(guid_s) != RTNL_VF_GUID_STR_LEN)
		return -1;

	for (i = 0; i < 7; i++) {
		if (guid_s[2 + i * 3] != ':')
			return -1;
	}

	*guid = 0;
	for (i = 0; i < 8; i++) {
		tmp = strtoul(guid_s + i * 3, &endptr, 16);
		if (endptr != guid_s + i * 3 + 2)
			return -1;

		if (tmp > 255)
			return -1;

		*guid |= tmp << (56 - 8 * i);
	}

	return 0;
}

/** @} */

/** @} */
