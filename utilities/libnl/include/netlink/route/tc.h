/*
 * netlink/route/tc.h		Traffic Control
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2006 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_TC_H_
#define NETLINK_TC_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/data.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * TC statistics identifiers
 * @ingroup tc
 */
enum rtnl_tc_stats_id {
	RTNL_TC_PACKETS,	/**< Packets seen */
	RTNL_TC_BYTES,		/**< Bytes seen */
	RTNL_TC_RATE_BPS,	/**< Current bits/s (rate estimator) */
	RTNL_TC_RATE_PPS,	/**< Current packet/s (rate estimator) */
	RTNL_TC_QLEN,		/**< Queue length */
	RTNL_TC_BACKLOG,	/**< Backlog length */
	RTNL_TC_DROPS,		/**< Packets dropped */
	RTNL_TC_REQUEUES,	/**< Number of requeues */
	RTNL_TC_OVERLIMITS,	/**< Number of overlimits */
	__RTNL_TC_STATS_MAX,
};

#define RTNL_TC_STATS_MAX (__RTNL_TC_STATS_MAX - 1)

extern int rtnl_tc_calc_txtime(int, int);
extern int rtnl_tc_calc_bufsize(int, int);
extern int rtnl_tc_calc_cell_log(int);

/**
 * Number of entries in a transmission time lookup table
 * @ingroup tc
 */
#define RTNL_TC_RTABLE_SIZE	256

extern int rtnl_tc_build_rate_table(uint32_t *, uint8_t, uint8_t, int, int);


/* TC Handle Translations */
extern char *		rtnl_tc_handle2str(uint32_t, char *, size_t);
extern int		rtnl_tc_str2handle(const char *, uint32_t *);

#ifdef __cplusplus
}
#endif

#endif
