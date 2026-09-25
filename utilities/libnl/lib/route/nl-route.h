/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#ifndef __LIB_ROUTE_NL_ROUTE_H__
#define __LIB_ROUTE_NL_ROUTE_H__

#include <netlink/route/tc.h>

#include "nl-priv-dynamic-core/object-api.h"
#include "nl-priv-dynamic-route/nl-priv-dynamic-route.h"
#include "nl-aux-core/nl-core.h"

/*****************************************************************************/

struct rtnl_link_map {
	uint64_t lm_mem_start;
	uint64_t lm_mem_end;
	uint64_t lm_base_addr;
	uint16_t lm_irq;
	uint8_t lm_dma;
	uint8_t lm_port;
};

#define IFQDISCSIZ 32

struct rtnl_link {
	NLHDR_COMMON

	char l_name[IFNAMSIZ];
	uint32_t l_family;
	uint32_t l_arptype;
	uint32_t l_index;
	uint32_t l_flags;
	uint32_t l_change;
	uint32_t l_mtu;
	uint32_t l_link;
	int32_t l_link_netnsid;
	uint32_t l_txqlen;
	uint32_t l_weight;
	uint32_t l_master;
	struct nl_addr *l_addr;
	struct nl_addr *l_bcast;
	struct nl_addr *l_paddr;
	char l_qdisc[IFQDISCSIZ];
	struct rtnl_link_map l_map;
	uint64_t l_stats[RTNL_LINK_STATS_MAX + 1];
	uint32_t l_flag_mask;
	uint32_t l_num_vf;
	uint8_t l_operstate;
	uint8_t l_linkmode;
	/* 2 byte hole */
	char *l_info_kind;
	char *l_info_slave_kind;
	struct rtnl_link_info_ops *l_info_ops;
	void *l_af_data[AF_MAX];
	void *l_info;
	char *l_ifalias;
	uint32_t l_promiscuity;
	uint32_t l_num_tx_queues;
	uint32_t l_num_rx_queues;
	uint32_t l_gso_max_segs;
	uint32_t l_gso_max_size;
	uint32_t l_group;
	uint8_t l_carrier;
	/* 3 byte hole */
	uint32_t l_carrier_changes;
	struct rtnl_link_af_ops *l_af_ops;
	struct nl_data *l_phys_port_id;
	char l_phys_port_name[IFNAMSIZ];
	struct nl_data *l_phys_switch_id;
	int l_ns_fd;
	pid_t l_ns_pid;
	struct rtnl_link_vf *l_vf_list;
};

struct rtnl_nh_encap {
	const struct nh_encap_ops *ops;
	void *priv; /* private data for encap type */
};

struct rtnl_ratespec {
	uint64_t rs_rate64;
	uint16_t rs_overhead;
	int16_t rs_cell_align;
	uint16_t rs_mpu;
	uint8_t rs_cell_log;
};

#define TCKINDSIZ 32

#define NL_TC_GENERIC(pre)                           \
	NLHDR_COMMON                                 \
	uint32_t pre##_family;                       \
	uint32_t pre##_ifindex;                      \
	uint32_t pre##_handle;                       \
	uint32_t pre##_parent;                       \
	uint32_t pre##_info;                         \
	uint32_t pre##_mtu;                          \
	uint32_t pre##_mpu;                          \
	uint32_t pre##_overhead;                     \
	uint32_t pre##_linktype;                     \
	char pre##_kind[TCKINDSIZ];                  \
	struct nl_data *pre##_opts;                  \
	uint64_t pre##_stats[RTNL_TC_STATS_MAX + 1]; \
	struct nl_data *pre##_xstats;                \
	struct nl_data *pre##_subdata;               \
	struct rtnl_link *pre##_link;                \
	struct rtnl_tc_ops *pre##_ops;               \
	enum rtnl_tc_type pre##_type;                \
	uint32_t pre##_chain

struct rtnl_tc {
	NL_TC_GENERIC(tc);
};

struct rtnl_qdisc {
	NL_TC_GENERIC(q);
};

struct rtnl_act {
	NL_TC_GENERIC(c);
	struct rtnl_act *a_next;
};

/*****************************************************************************/

static inline void rtnl_copy_ratespec(struct rtnl_ratespec *dst,
				      struct tc_ratespec *src)
{
	dst->rs_cell_log = src->cell_log;
	dst->rs_overhead = src->overhead;
	dst->rs_cell_align = src->cell_align;
	dst->rs_mpu = src->mpu;
	dst->rs_rate64 = src->rate;
}

static inline void rtnl_rcopy_ratespec(struct tc_ratespec *dst,
				       struct rtnl_ratespec *src)
{
	dst->cell_log = src->rs_cell_log;
	dst->overhead = src->rs_overhead;
	dst->cell_align = src->rs_cell_align;
	dst->mpu = src->rs_mpu;
	dst->rate = src->rs_rate64 > 0xFFFFFFFFull ? 0xFFFFFFFFull :
						     (uint32_t)src->rs_rate64;
}

/*****************************************************************************/

static inline int build_sysconf_path(char **strp, const char *filename)
{
	char *sysconfdir;

	sysconfdir = getenv("NLSYSCONFDIR");

	if (!sysconfdir)
		sysconfdir = _NL_SYSCONFDIR_LIBNL;

	return asprintf(strp, "%s/%s", sysconfdir, filename);
}

/*****************************************************************************/

static inline int rtnl_tc_calc_txtime64(int bufsize, uint64_t rate)
{
	return ((double)bufsize / (double)rate) * 1000000.0;
}

static inline int rtnl_tc_calc_bufsize64(int txtime, uint64_t rate)
{
	return ((double)txtime * (double)rate) / 1000000.0;
}

#endif /* __LIB_ROUTE_NL_ROUTE_H__ */
