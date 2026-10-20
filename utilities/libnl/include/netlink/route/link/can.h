/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2012 Benedikt Spranger <b.spranger@linutronix.de>
 */

#ifndef NETLINK_LINK_CAN_H_
#define NETLINK_LINK_CAN_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <linux/can/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

struct can_bittiming_const;
struct can_bittiming;
struct can_berr_counter;

extern int rtnl_link_is_can(struct rtnl_link *link);

extern char *rtnl_link_can_ctrlmode2str(int, char *, size_t);
extern int rtnl_link_can_str2ctrlmode(const char *);

extern int rtnl_link_can_restart(struct rtnl_link *);
extern int rtnl_link_can_freq(struct rtnl_link *, uint32_t *);
extern int rtnl_link_can_state(struct rtnl_link *, uint32_t *);

extern int rtnl_link_can_berr_rx(struct rtnl_link *);
extern int rtnl_link_can_berr_tx(struct rtnl_link *);
extern int rtnl_link_can_berr(struct rtnl_link *, struct can_berr_counter *);

extern int rtnl_link_can_get_bt_const(struct rtnl_link *,
                                      struct can_bittiming_const *);
extern int rtnl_link_can_get_bittiming(struct rtnl_link *,
                                       struct can_bittiming *);
extern int rtnl_link_can_set_bittiming(struct rtnl_link *,
                                       const struct can_bittiming *);

extern int rtnl_link_can_get_bitrate(struct rtnl_link *, uint32_t *);
extern int rtnl_link_can_set_bitrate(struct rtnl_link *, uint32_t);

extern int rtnl_link_can_get_sample_point(struct rtnl_link *, uint32_t *);
extern int rtnl_link_can_set_sample_point(struct rtnl_link *, uint32_t);

extern int rtnl_link_can_get_restart_ms(struct rtnl_link *, uint32_t *);
extern int rtnl_link_can_set_restart_ms(struct rtnl_link *, uint32_t);

extern int rtnl_link_can_get_ctrlmode(struct rtnl_link *, uint32_t *);
extern int rtnl_link_can_set_ctrlmode(struct rtnl_link *, uint32_t);
extern int rtnl_link_can_unset_ctrlmode(struct rtnl_link *, uint32_t);

extern int rtnl_link_can_get_data_bittiming_const(struct rtnl_link *,
                                                  struct can_bittiming_const *);
extern int rtnl_link_can_set_data_bittiming_const(struct rtnl_link *,
                                                  const struct can_bittiming_const *);
extern int rtnl_link_can_get_data_bittiming(struct rtnl_link *,
                                            struct can_bittiming *);
extern int rtnl_link_can_set_data_bittiming(struct rtnl_link *,
                                            const struct can_bittiming *);

extern int rtnl_link_can_get_device_stats(struct rtnl_link *,
				          struct can_device_stats *);

#ifdef __cplusplus
}
#endif

#endif
