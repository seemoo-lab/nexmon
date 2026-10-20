/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2018 Volodymyr Bendiuga <volodymyr.bendiuga@westermo.se>
 */

#ifndef NETLINK_MQPRIO_H_
#define NETLINK_MQPRIO_H_

#include <netlink/netlink.h>
#include <netlink/route/qdisc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int rtnl_qdisc_mqprio_set_num_tc(struct rtnl_qdisc *qdisc, int num_tc);
extern int rtnl_qdisc_mqprio_get_num_tc(struct rtnl_qdisc *qdisc);
extern int rtnl_qdisc_mqprio_set_priomap(struct rtnl_qdisc *qdisc, uint8_t priomap[],
                                         int len);
extern uint8_t *rtnl_qdisc_mqprio_get_priomap(struct rtnl_qdisc *qdisc);
extern int rtnl_qdisc_mqprio_hw_offload(struct rtnl_qdisc *qdisc, int offload);
extern int rtnl_qdisc_mqprio_get_hw_offload(struct rtnl_qdisc *qdisc);
extern int rtnl_qdisc_mqprio_set_queue(struct rtnl_qdisc *qdisc, uint16_t count[],
                                       uint16_t offset[], int len);
extern int rtnl_qdisc_mqprio_get_queue(struct rtnl_qdisc *qdisc, uint16_t *count,
                                       uint16_t *offset);
extern int rtnl_qdisc_mqprio_set_mode(struct rtnl_qdisc *qdisc, uint16_t mode);
extern int rtnl_qdisc_mqprio_get_mode(struct rtnl_qdisc *qdisc);
extern int rtnl_qdisc_mqprio_set_shaper(struct rtnl_qdisc *qdisc, uint16_t shaper);
extern int rtnl_qdisc_mqprio_get_shaper(struct rtnl_qdisc *qdisc);
extern int rtnl_qdisc_mqprio_set_min_rate(struct rtnl_qdisc *qdisc, uint64_t min[],
                                          int len);
extern int rtnl_qdisc_mqprio_get_min_rate(struct rtnl_qdisc *qdisc, uint64_t *min);
extern int rtnl_qdisc_mqprio_set_max_rate(struct rtnl_qdisc *qdisc, uint64_t max[],
                                          int len);
extern int rtnl_qdisc_mqprio_get_max_rate(struct rtnl_qdisc *qdisc, uint64_t *max);

#ifdef __cplusplus
}
#endif

#endif /* NETLINK_MQPRIO_H_ */
