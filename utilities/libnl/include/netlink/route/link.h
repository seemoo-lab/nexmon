/*
 * netlink/route/link.h		Links (Interfaces)
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_LINK_H_
#define NETLINK_LINK_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rtnl_link;

enum rtnl_link_st {
	RTNL_LINK_RX_PACKETS,
	RTNL_LINK_TX_PACKETS,
	RTNL_LINK_RX_BYTES,
	RTNL_LINK_TX_BYTES,
	RTNL_LINK_RX_ERRORS,
	RTNL_LINK_TX_ERRORS,
	RTNL_LINK_RX_DROPPED,
	RTNL_LINK_TX_DROPPED,
	RTNL_LINK_RX_COMPRESSED,
	RTNL_LINK_TX_COMPRESSED,
	RTNL_LINK_RX_FIFO_ERR,
	RTNL_LINK_TX_FIFO_ERR,
	RTNL_LINK_RX_LEN_ERR,
	RTNL_LINK_RX_OVER_ERR,
	RTNL_LINK_RX_CRC_ERR,
	RTNL_LINK_RX_FRAME_ERR,
	RTNL_LINK_RX_MISSED_ERR,
	RTNL_LINK_TX_ABORT_ERR,
	RTNL_LINK_TX_CARRIER_ERR,
	RTNL_LINK_TX_HBEAT_ERR,
	RTNL_LINK_TX_WIN_ERR,
	RTNL_LINK_TX_COLLISIONS,
	RTNL_LINK_MULTICAST,
	__RTNL_LINK_STATS_MAX,
};

#define RTNL_LINK_STATS_MAX (__RTNL_LINK_STATS_MAX - 1)

/* link object allocation/freeage */
extern struct rtnl_link *rtnl_link_alloc(void);
extern void	rtnl_link_put(struct rtnl_link *);
extern void	rtnl_link_free(struct rtnl_link *);

/* link cache management */
extern int	rtnl_link_alloc_cache(struct nl_sock *, struct nl_cache **);
extern struct rtnl_link *rtnl_link_get(struct nl_cache *, int);
extern struct rtnl_link *rtnl_link_get_by_name(struct nl_cache *, const char *);


extern int	rtnl_link_build_change_request(struct rtnl_link *,
					       struct rtnl_link *, int,
					       struct nl_msg **);
extern int	rtnl_link_change(struct nl_sock *, struct rtnl_link *,
				 struct rtnl_link *, int);

/* Name <-> Index Translations */
extern char * 	rtnl_link_i2name(struct nl_cache *, int, char *, size_t);
extern int	rtnl_link_name2i(struct nl_cache *, const char *);

/* Name <-> Statistic Translations */
extern char *	rtnl_link_stat2str(int, char *, size_t);
extern int	rtnl_link_str2stat(const char *);

/* Link Flags Translations */
extern char *	rtnl_link_flags2str(int, char *, size_t);
extern int	rtnl_link_str2flags(const char *);

extern char *	rtnl_link_operstate2str(int, char *, size_t);
extern int	rtnl_link_str2operstate(const char *);

extern char *	rtnl_link_mode2str(int, char *, size_t);
extern int	rtnl_link_str2mode(const char *);

/* Access Functions */
extern void	rtnl_link_set_qdisc(struct rtnl_link *, const char *);
extern char *	rtnl_link_get_qdisc(struct rtnl_link *);

extern void	rtnl_link_set_name(struct rtnl_link *, const char *);
extern char *	rtnl_link_get_name(struct rtnl_link *);

extern void	rtnl_link_set_flags(struct rtnl_link *, unsigned int);
extern void	rtnl_link_unset_flags(struct rtnl_link *, unsigned int);
extern unsigned int rtnl_link_get_flags(struct rtnl_link *);

extern void	rtnl_link_set_mtu(struct rtnl_link *, unsigned int);
extern unsigned int rtnl_link_get_mtu(struct rtnl_link *);

extern void	rtnl_link_set_txqlen(struct rtnl_link *, unsigned int);
extern unsigned int rtnl_link_get_txqlen(struct rtnl_link *);

extern void	rtnl_link_set_weight(struct rtnl_link *, unsigned int);
extern unsigned int rtnl_link_get_weight(struct rtnl_link *);

extern void	rtnl_link_set_ifindex(struct rtnl_link *, int);
extern int	rtnl_link_get_ifindex(struct rtnl_link *);

extern void	rtnl_link_set_family(struct rtnl_link *, int);
extern int	rtnl_link_get_family(struct rtnl_link *);

extern void	rtnl_link_set_arptype(struct rtnl_link *, unsigned int);
extern unsigned int rtnl_link_get_arptype(struct rtnl_link *);

extern void	rtnl_link_set_addr(struct rtnl_link *, struct nl_addr *);
extern struct nl_addr *rtnl_link_get_addr(struct rtnl_link *);

extern void	rtnl_link_set_broadcast(struct rtnl_link *, struct nl_addr *);
extern struct nl_addr *rtnl_link_get_broadcast(struct rtnl_link *);

extern void	rtnl_link_set_link(struct rtnl_link *, int);
extern int	rtnl_link_get_link(struct rtnl_link *);

extern void	rtnl_link_set_master(struct rtnl_link *, int);
extern int	rtnl_link_get_master(struct rtnl_link *);

extern void	rtnl_link_set_operstate(struct rtnl_link *, uint8_t);
extern uint8_t	rtnl_link_get_operstate(struct rtnl_link *);

extern void	rtnl_link_set_linkmode(struct rtnl_link *, uint8_t);
extern uint8_t	rtnl_link_get_linkmode(struct rtnl_link *);

extern uint64_t rtnl_link_get_stat(struct rtnl_link *, int);

extern int	rtnl_link_set_info_type(struct rtnl_link *, const char *);
extern char *	rtnl_link_get_info_type(struct rtnl_link *);

#ifdef __cplusplus
}
#endif

#endif
