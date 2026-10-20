/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_ROUTE_NEXTHOP_H_
#define NETLINK_ROUTE_NEXTHOP_H_

#include <netlink/netlink.h>
#include <netlink/addr.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rtnl_nexthop;
struct rtnl_nh_encap;

enum {
	NH_DUMP_FROM_ONELINE = -2,
	NH_DUMP_FROM_DETAILS = -1,
	NH_DUMP_FROM_ENV = 0,
	/* > 0 reserved for nexthop index */
};

extern struct rtnl_nexthop *rtnl_route_nh_alloc(void);
extern struct rtnl_nexthop *rtnl_route_nh_clone(struct rtnl_nexthop *);
extern void rtnl_route_nh_free(struct rtnl_nexthop *);

extern int rtnl_route_nh_compare(struct rtnl_nexthop *, struct rtnl_nexthop *,
				 uint32_t, int);

extern int rtnl_route_nh_identical(struct rtnl_nexthop *,
				   struct rtnl_nexthop *);

extern void rtnl_route_nh_dump(struct rtnl_nexthop *, struct nl_dump_params *);

extern void rtnl_route_nh_set_weight(struct rtnl_nexthop *, uint8_t);
extern uint8_t rtnl_route_nh_get_weight(struct rtnl_nexthop *);
extern void rtnl_route_nh_set_ifindex(struct rtnl_nexthop *, int);
extern int rtnl_route_nh_get_ifindex(struct rtnl_nexthop *);
extern void rtnl_route_nh_set_gateway(struct rtnl_nexthop *, struct nl_addr *);
extern struct nl_addr *rtnl_route_nh_get_gateway(struct rtnl_nexthop *);
extern void rtnl_route_nh_set_flags(struct rtnl_nexthop *, unsigned int);
extern void rtnl_route_nh_unset_flags(struct rtnl_nexthop *, unsigned int);
extern unsigned int rtnl_route_nh_get_flags(struct rtnl_nexthop *);
extern void rtnl_route_nh_set_realms(struct rtnl_nexthop *, uint32_t);
extern uint32_t rtnl_route_nh_get_realms(struct rtnl_nexthop *);

extern int rtnl_route_nh_set_newdst(struct rtnl_nexthop *, struct nl_addr *);
extern struct nl_addr *rtnl_route_nh_get_newdst(struct rtnl_nexthop *);
extern int rtnl_route_nh_set_via(struct rtnl_nexthop *, struct nl_addr *);
extern struct nl_addr *rtnl_route_nh_get_via(struct rtnl_nexthop *);
extern int rtnl_route_nh_set_encap(struct rtnl_nexthop *,
				   struct rtnl_nh_encap *);
extern struct rtnl_nh_encap *rtnl_route_nh_get_encap(struct rtnl_nexthop *);
extern char *rtnl_route_nh_flags2str(int, char *, size_t);
extern int rtnl_route_nh_str2flags(const char *);

/*
 * nexthop encapsulations
 */
extern struct rtnl_nh_encap *rtnl_nh_encap_alloc(void);
extern void rtnl_nh_encap_free(struct rtnl_nh_encap *nh_encap);
extern struct rtnl_nh_encap *rtnl_nh_encap_clone(struct rtnl_nh_encap *src);
extern int rtnl_nh_encap_get_type(struct rtnl_nh_encap *nh_encap);

/* MPLS encap */
extern int rtnl_nh_encap_mpls(struct rtnl_nh_encap *nh_encap,
			      struct nl_addr *dst, uint8_t ttl);
struct nl_addr *rtnl_nh_get_encap_mpls_dst(struct rtnl_nh_encap *);
extern int rtnl_nh_get_encap_mpls_ttl(struct rtnl_nh_encap *);

extern int rtnl_route_nh_encap_mpls(struct rtnl_nexthop *nh,
				    struct nl_addr *addr, uint8_t ttl);
extern struct nl_addr *rtnl_route_nh_get_encap_mpls_dst(struct rtnl_nexthop *);
extern uint8_t rtnl_route_nh_get_encap_mpls_ttl(struct rtnl_nexthop *);

/* IPv6 encap */
extern int rtnl_nh_encap_ip6(struct rtnl_nh_encap *nh_encap,
			     struct nl_addr *dst);
struct nl_addr *rtnl_nh_get_encap_ip6_dst(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip6_id(struct rtnl_nh_encap *, uint64_t);
extern uint64_t rtnl_nh_get_encap_ip6_id(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip6_hoplimit(struct rtnl_nh_encap *, uint8_t);
extern int rtnl_nh_get_encap_ip6_hoplimit(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip6_tc(struct rtnl_nh_encap *, uint8_t);
extern int rtnl_nh_get_encap_ip6_tc(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip6_src(struct rtnl_nh_encap *, struct nl_addr *);
struct nl_addr *rtnl_nh_get_encap_ip6_src(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip6_flags(struct rtnl_nh_encap *, uint16_t);
extern int rtnl_nh_get_encap_ip6_flags(struct rtnl_nh_encap *);

/* ILA encap */
extern int rtnl_nh_encap_ila(struct rtnl_nh_encap *nh_encap, uint64_t locator);
extern int rtnl_nh_get_encap_ila_locator(struct rtnl_nh_encap *nh_encap,
					 uint64_t *locator);
extern int rtnl_nh_set_encap_ila_csum_mode(struct rtnl_nh_encap *nh_encap,
					   uint8_t csum_mode);
extern int rtnl_nh_clear_encap_ila_csum_mode(struct rtnl_nh_encap *nh_encap);
extern int rtnl_nh_get_encap_ila_csum_mode(struct rtnl_nh_encap *nh_encap);
extern int rtnl_nh_set_encap_ila_ident_type(struct rtnl_nh_encap *nh_encap,
					    uint8_t ident_type);
extern int rtnl_nh_clear_encap_ila_ident_type(struct rtnl_nh_encap *nh_encap);
extern int rtnl_nh_get_encap_ila_ident_type(struct rtnl_nh_encap *nh_encap);
extern int rtnl_nh_set_encap_ila_hook_type(struct rtnl_nh_encap *nh_encap,
					   uint8_t hook_type);
extern int rtnl_nh_clear_encap_ila_hook_type(struct rtnl_nh_encap *nh_encap);
extern int rtnl_nh_get_encap_ila_hook_type(struct rtnl_nh_encap *nh_encap);

/* IPv4 encap */
extern int rtnl_nh_encap_ip(struct rtnl_nh_encap *nh_encap,
			    struct nl_addr *dst);
struct nl_addr *rtnl_nh_get_encap_ip_dst(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip_src(struct rtnl_nh_encap *, struct nl_addr *);
struct nl_addr *rtnl_nh_get_encap_ip_src(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip_ttl(struct rtnl_nh_encap *, uint8_t);
extern int rtnl_nh_get_encap_ip_ttl(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip_tos(struct rtnl_nh_encap *, uint8_t);
extern int rtnl_nh_get_encap_ip_tos(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip_id(struct rtnl_nh_encap *, uint64_t);
extern uint64_t rtnl_nh_get_encap_ip_id(struct rtnl_nh_encap *);
extern int rtnl_nh_set_encap_ip_flags(struct rtnl_nh_encap *, uint16_t);
extern int rtnl_nh_get_encap_ip_flags(struct rtnl_nh_encap *);
#ifdef __cplusplus
}
#endif

#endif
