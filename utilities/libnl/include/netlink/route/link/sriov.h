/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2016 Intel Corp. All rights reserved.
 * Copyright (c) 2016 Jef Oliver <jef.oliver@intel.com>
 */

#ifndef NETLINK_LINK_SRIOV_H_
#define NETLINK_LINK_SRIOV_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTNL_VF_GUID_STR_LEN 23

/**
 * @ingroup sriov
 */
typedef enum {
	RTNL_LINK_VF_RATE_API_UNSPEC,	/*!< Unspecified API type */
	RTNL_LINK_VF_RATE_API_OLD,	/*!< Old Rate setting API */
	RTNL_LINK_VF_RATE_API_NEW,	/*!< New Rate setting API */
	__RTNL_LINK_VF_RATE_API_MAX,
} rtnl_link_rate_api_t;

#define RTNL_LINK_VF_RATE_API_MAX (__RTNL_LINK_VF_RATE_API_MAX - 1)

/**
 * @ingroup sriov
 */
typedef enum {
	RTNL_LINK_VF_STATS_RX_PACKETS,	/*!< Packets Received */
	RTNL_LINK_VF_STATS_TX_PACKETS,	/*!< Packets Sent */
	RTNL_LINK_VF_STATS_RX_BYTES,	/*!< Bytes Recieved */
	RTNL_LINK_VF_STATS_TX_BYTES,	/*!< Bytes Sent */
	RTNL_LINK_VF_STATS_BROADCAST,	/*!< Broadcast packets received */
	RTNL_LINK_VF_STATS_MULTICAST,	/*!< Multicast packets received */
	__RTNL_LINK_VF_STATS_MAX,
} rtnl_link_vf_stats_t;

#define RTNL_LINK_VF_STATS_MAX (__RTNL_LINK_VF_STATS_MAX - 1)

/**
 * @struct rtnl_link_vf sriov.h "netlink/route/link/sriov.h"
 * @brief SRIOV VF object
 * @ingroup sriov
 *
 * @copydoc private_struct
 */
struct rtnl_link_vf;

/**
 * @brief SRIOV VF VFLAN settings
 * @ingroup sriov
 */
typedef struct nl_vf_vlan_info {
	uint32_t	vf_vlan;	/*!< VLAN number */
	uint32_t	vf_vlan_qos;	/*!< VLAN QOS value */
	uint16_t	vf_vlan_proto;	/*!< VLAN protocol */
} nl_vf_vlan_info_t;

/**
 * @brief SRIOV VF VLANs information
 * @ingroup sriov
 */
typedef struct nl_vf_vlans {
	int			ce_refcnt;	/*!< Reference counter. Don't change this value */
	int			size;		/*!< Number of VLANs on the SRIOV VF */
	nl_vf_vlan_info_t *	vlans;		/*!< nl_vf_vlan_info_t array of SRIOV VF VLANs */
} nl_vf_vlans_t;

/**
 * @brief VF Rate information structure
 * @ingroup sriov
 */
struct nl_vf_rate {
	int		api;		/*!< rtnl_link_rate_api_t API Version to use */
	uint32_t	rate;		/*!< Old API Max Rate in Mbps */
	uint32_t	max_tx_rate;	/*!< New API Max Rate in Mbps */
	uint32_t	min_tx_rate;	/*!< New API Mix Rate in Mbps */
};

extern int rtnl_link_vf_add(struct rtnl_link *, struct rtnl_link_vf *);
extern struct rtnl_link_vf *rtnl_link_vf_alloc(void);
extern void rtnl_link_vf_free(struct rtnl_link_vf *);
extern struct rtnl_link_vf *rtnl_link_vf_get(struct rtnl_link *, uint32_t);
extern void rtnl_link_vf_put(struct rtnl_link_vf *);

extern int rtnl_link_vf_get_addr(struct rtnl_link_vf *, struct nl_addr **);
extern void rtnl_link_vf_set_addr(struct rtnl_link_vf *, struct nl_addr *);

extern void rtnl_link_vf_set_ib_node_guid(struct rtnl_link_vf *, uint64_t);
extern void rtnl_link_vf_set_ib_port_guid(struct rtnl_link_vf *, uint64_t);

extern int rtnl_link_vf_get_index(struct rtnl_link_vf *, uint32_t *);
extern void rtnl_link_vf_set_index(struct rtnl_link_vf *, uint32_t);

extern int rtnl_link_vf_get_linkstate(struct rtnl_link_vf *, uint32_t *);
extern void rtnl_link_vf_set_linkstate(struct rtnl_link_vf *, uint32_t);

extern int rtnl_link_vf_get_rate(struct rtnl_link_vf *, struct nl_vf_rate *);
extern void rtnl_link_vf_set_rate(struct rtnl_link_vf *, struct nl_vf_rate *);

extern int rtnl_link_vf_get_rss_query_en(struct rtnl_link_vf *, uint32_t *);
extern void rtnl_link_vf_set_rss_query_en(struct rtnl_link_vf *, uint32_t);

extern int rtnl_link_vf_get_spoofchk(struct rtnl_link_vf *, uint32_t *);
extern void rtnl_link_vf_set_spoofchk(struct rtnl_link_vf *, uint32_t);

extern int rtnl_link_vf_get_stat(struct rtnl_link_vf *, rtnl_link_vf_stats_t,
				 uint64_t *);

extern int rtnl_link_vf_get_trust(struct rtnl_link_vf *, uint32_t *);
extern void rtnl_link_vf_set_trust(struct rtnl_link_vf *, uint32_t);

extern int rtnl_link_vf_get_vlans(struct rtnl_link_vf *, nl_vf_vlans_t **);
extern void rtnl_link_vf_set_vlans(struct rtnl_link_vf *, nl_vf_vlans_t *);

extern int rtnl_link_vf_vlan_alloc(nl_vf_vlans_t **, int);
extern void rtnl_link_vf_vlan_free(nl_vf_vlans_t *vf_vlans);
extern void rtnl_link_vf_vlan_put(nl_vf_vlans_t *);

/* Utility functions */
extern char *rtnl_link_vf_linkstate2str(uint32_t, char *, size_t);
extern int rtnl_link_vf_str2linkstate(const char *);

extern char *rtnl_link_vf_vlanproto2str(uint16_t, char *, size_t);
extern int rtnl_link_vf_str2vlanproto(const char *);

extern int rtnl_link_vf_str2guid(uint64_t *, const char *);

#ifdef __cplusplus
}
#endif

#endif
