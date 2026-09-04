/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef NETLINK_LINK_GENEVE_H_
#define NETLINK_LINK_GENEVE_H_

#include <netlink/netlink.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTNL_GENEVE_ID_MAX 16777215

#define RTNL_LINK_GENEVE_F_COLLECT_METADATA     (1<<0)

extern struct rtnl_link *rtnl_link_geneve_alloc(void);
extern int rtnl_link_is_geneve(struct rtnl_link *);

extern int rtnl_link_geneve_set_id(struct rtnl_link *, uint32_t);
extern int rtnl_link_geneve_get_id(struct rtnl_link *, uint32_t *);

extern int rtnl_link_geneve_set_remote(struct rtnl_link *, struct nl_addr *);
extern int rtnl_link_geneve_get_remote(struct rtnl_link *, struct nl_addr **);

extern int rtnl_link_geneve_set_ttl(struct rtnl_link *, uint8_t);
extern int rtnl_link_geneve_get_ttl(struct rtnl_link *);

extern int rtnl_link_geneve_set_tos(struct rtnl_link *, uint8_t);
extern int rtnl_link_geneve_get_tos(struct rtnl_link *);

extern int rtnl_link_geneve_set_port(struct rtnl_link *, uint32_t);
extern int rtnl_link_geneve_get_port(struct rtnl_link *, uint32_t *);

extern int rtnl_link_geneve_set_label(struct rtnl_link *, uint32_t);
extern int rtnl_link_geneve_get_label(struct rtnl_link *, uint32_t *);

extern int rtnl_link_geneve_set_udp_csum(struct rtnl_link *, uint8_t);
extern int rtnl_link_geneve_get_udp_csum(struct rtnl_link *);

extern int rtnl_link_geneve_set_udp_zero_csum6_tx(struct rtnl_link *, uint8_t);
extern int rtnl_link_geneve_get_udp_zero_csum6_tx(struct rtnl_link *);

extern int rtnl_link_geneve_set_udp_zero_csum6_rx(struct rtnl_link *, uint8_t);
extern int rtnl_link_geneve_get_udp_zero_csum6_rx(struct rtnl_link *);

extern int rtnl_link_geneve_set_flags(struct rtnl_link *, uint8_t flags, int enable);
extern int rtnl_link_geneve_get_flags(struct rtnl_link *, uint8_t *flags);

#ifdef __cplusplus
}
#endif

#endif
