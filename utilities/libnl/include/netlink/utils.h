/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2012 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_UTILS_H_
#define NETLINK_UTILS_H_

#include <netlink/netlink.h>
#include <netlink/list.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && __GNUC__ > 5
#define _nl_attribute_printf(a, b) __attribute__((__format__(printf, a, b)))
#else
#define _nl_attribute_printf(a, b)
#endif

/**
 * @name Probability Constants
 * @{
 */

/**
 * Lower probability limit
 * @ingroup utils
 */
#define NL_PROB_MIN 0x0

/**
 * Upper probability limit nl_dump_type
 * @ingroup utils
 */
#define NL_PROB_MAX 0xffffffff

/** @} */

enum {
	NL_BYTE_RATE,
	NL_BIT_RATE,
};

/* unit pretty-printing */
extern double	nl_cancel_down_bytes(unsigned long long, char **);
extern double	nl_cancel_down_bits(unsigned long long, char **);
extern int	nl_rate2str(unsigned long long, int, char *, size_t);
extern double	nl_cancel_down_us(uint32_t, char **);

/* generic unit translations */
extern long	nl_size2int(const char *);
extern char *	nl_size2str(const size_t, char *, const size_t);
extern long	nl_prob2int(const char *);

/* time translations */
extern int	nl_get_user_hz(void);
extern int	nl_get_psched_hz(void);
extern uint32_t	nl_us2ticks(uint32_t);
extern uint32_t	nl_ticks2us(uint32_t);
extern int	nl_str2msec(const char *, uint64_t *);
extern char *	nl_msec2str(uint64_t, char *, size_t);

/* link layer protocol translations */
extern char *	nl_llproto2str(int, char *, size_t);
extern int	nl_str2llproto(const char *);

/* ethernet protocol translations */
extern char *	nl_ether_proto2str(int, char *, size_t);
extern int	nl_str2ether_proto(const char *);

/* IP protocol translations */
extern char *	nl_ip_proto2str(int, char *, size_t);
extern int	nl_str2ip_proto(const char *);

/* Dumping helpers */
extern void	nl_new_line(struct nl_dump_params *);
extern void	nl_dump(struct nl_dump_params *, const char *, ...) _nl_attribute_printf(2, 3);
extern void	nl_dump_line(struct nl_dump_params *, const char *, ...) _nl_attribute_printf(2, 3);

enum {
	NL_CAPABILITY_NONE,

	/**
	 * rtnl_route_build_msg() no longer guesses the route scope
	 * if explicitly set to RT_SCOPE_NOWHERE.
	 * @ingroup utils
	 */
	NL_CAPABILITY_ROUTE_BUILD_MSG_SET_SCOPE         = 1,
#define NL_CAPABILITY_ROUTE_BUILD_MSG_SET_SCOPE NL_CAPABILITY_ROUTE_BUILD_MSG_SET_SCOPE

	/**
	 * rtnl_link_veth_get_peer() now returns a reference that is owned by the
	 * caller and must be released by the caller with rtnl_link_put().
	 */
	NL_CAPABILITY_ROUTE_LINK_VETH_GET_PEER_OWN_REFERENCE = 2,
#define NL_CAPABILITY_ROUTE_LINK_VETH_GET_PEER_OWN_REFERENCE NL_CAPABILITY_ROUTE_LINK_VETH_GET_PEER_OWN_REFERENCE

	/**
	 * rtnl_u32_add_action() and rtnl_basic_add_action() now grab a reference to act
	 * caller are free to release its own
	 */
	NL_CAPABILITY_ROUTE_LINK_CLS_ADD_ACT_OWN_REFERENCE = 3,
#define NL_CAPABILITY_ROUTE_LINK_CLS_ADD_ACT_OWN_REFERENCE NL_CAPABILITY_ROUTE_LINK_CLS_ADD_ACT_OWN_REFERENCE

	/**
	 * Indicate that the local port is unspecified until the user accesses
	 * it (via nl_socket_get_local_port()) or until nl_connect(). More importantly,
	 * if the port is left unspecified, nl_connect() will retry generating another
	 * port when bind() fails with ADDRINUSE.
	 */
	NL_CAPABILITY_NL_CONNECT_RETRY_GENERATE_PORT_ON_ADDRINUSE = 4,
#define NL_CAPABILITY_NL_CONNECT_RETRY_GENERATE_PORT_ON_ADDRINUSE NL_CAPABILITY_NL_CONNECT_RETRY_GENERATE_PORT_ON_ADDRINUSE

	/**
	 * Indicate that rtnl_link_get_kernel() fails with -NLE_OPNOTSUPP in case
	 * of older kernals not supporting lookup by ifname. This changes behavior
	 * from returning -NLE_INVAL to return -NLE_OPNOTSUPP.
	 */
	NL_CAPABILITY_ROUTE_LINK_GET_KERNEL_FAIL_OPNOTSUPP = 5,
#define NL_CAPABILITY_ROUTE_LINK_GET_KERNEL_FAIL_OPNOTSUPP NL_CAPABILITY_ROUTE_LINK_GET_KERNEL_FAIL_OPNOTSUPP

	/**
	 * Also consider the a_cacheinfo field (ADDR_ATTR_CACHEINFO) that contains the
	 * address timestamps and expiry when comparing struct rtnl_addr objects with
	 * nl_object_diff().
	 */
	NL_CAPABILITY_ROUTE_ADDR_COMPARE_CACHEINFO = 6,
#define NL_CAPABILITY_ROUTE_ADDR_COMPARE_CACHEINFO NL_CAPABILITY_ROUTE_ADDR_COMPARE_CACHEINFO

	/**
	 * The library version is libnl3 3.2.26 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_2_26 = 7,
#define NL_CAPABILITY_VERSION_3_2_26 NL_CAPABILITY_VERSION_3_2_26

	/**
	 * nl_recv() fails with NLE_MSG_TRUNC if a message got truncated
	 * with NL_MSG_PEEK disabled. Previously, the failed message was wrongly
	 * discarded and the next message received.
	 */
	NL_CAPABILITY_NL_RECV_FAIL_TRUNC_NO_PEEK = 8,
#define NL_CAPABILITY_NL_RECV_FAIL_TRUNC_NO_PEEK NL_CAPABILITY_NL_RECV_FAIL_TRUNC_NO_PEEK

	/**
	 * rtnl_link_build_change_request() and rtnl_link_change() would set ifi.ifi_flags but leave
	 * ifi.ifi_change at zero. This was later fixed to set ifi.ifi_change to the flags that are actually
	 * set in changes.
	 */
	NL_CAPABILITY_LINK_BUILD_CHANGE_REQUEST_SET_CHANGE = 9,
#define NL_CAPABILITY_LINK_BUILD_CHANGE_REQUEST_SET_CHANGE NL_CAPABILITY_LINK_BUILD_CHANGE_REQUEST_SET_CHANGE

	/**
	 * Between 3.2.14 (64fcb47a36ec12d7e7f00605f6a8952ce985dd08) and 3.2.22 (8571f58f23763d8db7365d02c9b27832ad3d7005),
	 * rtnl_neigh_get() behaved differently and only returned objects with family AF_UNSPEC.
	 * This capability indicates, that the function was fixed. The absense of the capability,
	 * doesn't indicate however which behavior the function will have. So beware. */
	NL_CAPABILITY_RTNL_NEIGH_GET_FILTER_AF_UNSPEC_FIX = 10,
#define NL_CAPABILITY_RTNL_NEIGH_GET_FILTER_AF_UNSPEC_FIX NL_CAPABILITY_RTNL_NEIGH_GET_FILTER_AF_UNSPEC_FIX

	/**
	 * The library version is libnl3 3.2.27 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_2_27 = 11,
#define NL_CAPABILITY_VERSION_3_2_27 NL_CAPABILITY_VERSION_3_2_27

	/**
	 * Properly serialize vlan protocol IFLA_VLAN_PROTOCOL.
	 */
	NL_CAPABILITY_RTNL_LINK_VLAN_PROTOCOL_SERIALZE = 12,
#define NL_CAPABILITY_RTNL_LINK_VLAN_PROTOCOL_SERIALZE NL_CAPABILITY_RTNL_LINK_VLAN_PROTOCOL_SERIALZE

	/**
	 * Properly read gre REMOTE port.
	 */
	NL_CAPABILITY_RTNL_LINK_PARSE_GRE_REMOTE = 13,
#define NL_CAPABILITY_RTNL_LINK_PARSE_GRE_REMOTE NL_CAPABILITY_RTNL_LINK_PARSE_GRE_REMOTE

	/**
	 * Don't skip over vlan ingress-map entries with "to" field zero when serializing
	 * a netlink message. Previously such entires would be ignored which inhibits the
	 * user from clearing ingress map entries.
	 */
	NL_CAPABILITY_RTNL_LINK_VLAN_INGRESS_MAP_CLEAR = 14,
#define NL_CAPABILITY_RTNL_LINK_VLAN_INGRESS_MAP_CLEAR NL_CAPABILITY_RTNL_LINK_VLAN_INGRESS_MAP_CLEAR

	/**
	 * Consider vxlan link info for nl_object_diff().
	 */
	NL_CAPABILITY_RTNL_LINK_VXLAN_IO_COMPARE = 15,
#define NL_CAPABILITY_RTNL_LINK_VXLAN_IO_COMPARE NL_CAPABILITY_RTNL_LINK_VXLAN_IO_COMPARE

	/**
	 * Support 64 bit attributes for nl_object_diff().
	 */
	NL_CAPABILITY_NL_OBJECT_DIFF64 = 16,
#define NL_CAPABILITY_NL_OBJECT_DIFF64 NL_CAPABILITY_NL_OBJECT_DIFF64

	/**
	 * Support omitting @key argument to xfrmnl_sa_get_*_params() to check
	 * for required buffer size for key.
	 */
	NL_CAPABILITY_XFRM_SA_KEY_SIZE = 17,
#define NL_CAPABILITY_XFRM_SA_KEY_SIZE NL_CAPABILITY_XFRM_SA_KEY_SIZE

	/**
	 * Properly handle nl_object_identity() for AF_INET and AF_INET6 addresses
	 * and properly handle the peer/IFA_ADDRESS for IPv4 addresses.
	 */
	NL_CAPABILITY_RTNL_ADDR_PEER_FIX = 18,
#define NL_CAPABILITY_RTNL_ADDR_PEER_FIX NL_CAPABILITY_RTNL_ADDR_PEER_FIX

	/**
	 * The library version is libnl3 3.2.28 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_2_28 = 19,
#define NL_CAPABILITY_VERSION_3_2_28 NL_CAPABILITY_VERSION_3_2_28

	/**
	 * After NL_CAPABILITY_RTNL_ADDR_PEER_FIX, a follow up regression to lookup
	 * IPv4 addresses in the cache was fixed (PR#105).
	 */
	NL_CAPABILITY_RTNL_ADDR_PEER_ID_FIX = 20,
#define NL_CAPABILITY_RTNL_ADDR_PEER_ID_FIX NL_CAPABILITY_RTNL_ADDR_PEER_ID_FIX

	/**
	 * nl_addr_fill_sockaddr() properly checks that the provided address to
	 * avoid read-out-of-bounds for invalid addresses.
	 */
	NL_CAPABILITY_NL_ADDR_FILL_SOCKADDR = 21,
#define NL_CAPABILITY_NL_ADDR_FILL_SOCKADDR NL_CAPABILITY_NL_ADDR_FILL_SOCKADDR

	/**
	 * Support omitting @ctx_str argument to xfrmnl_sa_get_sec_ctx() to check
	 * for required buffer size for context string.
	 */
	NL_CAPABILITY_XFRM_SEC_CTX_LEN = 22,
#define NL_CAPABILITY_XFRM_SEC_CTX_LEN NL_CAPABILITY_XFRM_SEC_CTX_LEN

	/**
	 * rtnl_link_build_add_request() would set ifi.ifi_flags but leave ifi.ifi_change at zero.
	 * This was later fixed to set ifi.ifi_change to the flags that are actually
	 * set
	 */
	NL_CAPABILITY_LINK_BUILD_ADD_REQUEST_SET_CHANGE = 23,
#define NL_CAPABILITY_LINK_BUILD_ADD_REQUEST_SET_CHANGE NL_CAPABILITY_LINK_BUILD_ADD_REQUEST_SET_CHANGE

	/* Older versions of libnl3 would not use MSG_PEEK for nl_recvmsgs() unless calling
	 * nl_socket_enable_msg_peek(). Instead, the user had to specify the buffer size via
	 * nl_socket_set_msg_buf_size(), which in turn would default to 4*getpagesize().
	 *
	 * The default value might not be large enough, so users who were not aware of the
	 * problem easily ended up using a too small receive buffer. Usually, one wants to
	 * avoid MSG_PEEK for recvmsg() because it requires an additional syscall.
	 *
	 * Now, as indicated by this capability, nl_recvmsgs() would use MSG_PEEK by default. The
	 * user still can explicitly disable MSG_PEEK by calling nl_socket_disable_msg_peek() or
	 * by setting the nl_socket_set_msg_buf_size() to a non-zero value.
	 */
	NL_CAPABILITY_NL_RECVMSGS_PEEK_BY_DEFAULT = 24,
#define NL_CAPABILITY_NL_RECVMSGS_PEEK_BY_DEFAULT NL_CAPABILITY_NL_RECVMSGS_PEEK_BY_DEFAULT

	/**
	 * The library version is libnl3 3.2.29 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_2_29 = 25,
#define NL_CAPABILITY_VERSION_3_2_29 NL_CAPABILITY_VERSION_3_2_29

	/**
	 * Support omitting @ctx_str argument to xfrmnl_sp_get_sec_ctx() to check
	 * for required buffer size for context string.
	 */
	NL_CAPABILITY_XFRM_SP_SEC_CTX_LEN = 26,
#define NL_CAPABILITY_XFRM_SP_SEC_CTX_LEN NL_CAPABILITY_XFRM_SP_SEC_CTX_LEN

	/**
	 * The library version is libnl3 3.3.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_3_0 = 27,
#define NL_CAPABILITY_VERSION_3_3_0 NL_CAPABILITY_VERSION_3_3_0

	/**
	 * The library version is libnl3 3.4.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_4_0 = 28,
#define NL_CAPABILITY_VERSION_3_4_0 NL_CAPABILITY_VERSION_3_4_0

	/**
	 * Fixed memory corruption in rtnl_link_vlan_set_egress_map(). Previously, if you tried
	 * to add more then 4 mappings, a buffer overflow occured. Also fixed nl_object_clone()
	 * for VLAN links.
	 */
	NL_CAPABILITY_ROUTE_FIX_VLAN_SET_EGRESS_MAP = 29,
#define NL_CAPABILITY_ROUTE_FIX_VLAN_SET_EGRESS_MAP NL_CAPABILITY_ROUTE_FIX_VLAN_SET_EGRESS_MAP

	/**
	 * The library version is libnl3 3.5.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_5_0 = 30,
#define NL_CAPABILITY_VERSION_3_5_0 NL_CAPABILITY_VERSION_3_5_0

	/**
	 * nl_object_identical() can consider objects identical, if they both lack the same
	 * set of ID attributes.
	 */
	NL_CAPABILITY_NL_OBJECT_IDENTICAL_PARTIAL = 31,
#define NL_CAPABILITY_NL_OBJECT_IDENTICAL_PARTIAL NL_CAPABILITY_NL_OBJECT_IDENTICAL_PARTIAL

	/**
	 * The library version is libnl3 3.6.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_6_0 = 32,
#define NL_CAPABILITY_VERSION_3_6_0 NL_CAPABILITY_VERSION_3_6_0

	/**
	 * The library version is libnl3 3.7.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_7_0 = 33,
#define NL_CAPABILITY_VERSION_3_7_0 NL_CAPABILITY_VERSION_3_7_0

	/**
	 * The library version is libnl3 3.8.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_8_0 = 34,
#define NL_CAPABILITY_VERSION_3_8_0 NL_CAPABILITY_VERSION_3_8_0

	/**
	 * The library version is libnl3 3.9.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_9_0 = 35,
#define NL_CAPABILITY_VERSION_3_9_0 NL_CAPABILITY_VERSION_3_9_0

	/**
	 * The library version is libnl3 3.10.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_10_0 = 36,
#define NL_CAPABILITY_VERSION_3_10_0 NL_CAPABILITY_VERSION_3_10_0

	/**
	 * The library version is libnl3 3.11.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_11_0 = 37,
#define NL_CAPABILITY_VERSION_3_11_0 NL_CAPABILITY_VERSION_3_11_0

	/**
	 * The library version is libnl3 3.12.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_12_0 = 38,
#define NL_CAPABILITY_VERSION_3_12_0 NL_CAPABILITY_VERSION_3_12_0

	/**
	 * The library version is libnl3 3.13.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_13_0 = 39,
#define NL_CAPABILITY_VERSION_3_13_0 NL_CAPABILITY_VERSION_3_13_0

	/**
	 * The library version is libnl3 3.14.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_14_0 = 40,
#define NL_CAPABILITY_VERSION_3_14_0 NL_CAPABILITY_VERSION_3_14_0

	/**
	 * Fixes parsing bonding properties miimon, hashing_type, and
	 * min_links from netlink.
	 */
	NL_CAPABILITY_ROUTE_FIX_PARSE_BONDING = 41,
#define NL_CAPABILITY_ROUTE_FIX_PARSE_BONDING NL_CAPABILITY_ROUTE_FIX_PARSE_BONDING

	/**
	 * The library version is libnl3 3.15.0 or newer. This capability should never be backported.
	 */
	NL_CAPABILITY_VERSION_3_15_0 = 42,
#define NL_CAPABILITY_VERSION_3_15_0 NL_CAPABILITY_VERSION_3_15_0

	__NL_CAPABILITY_MAX,
	NL_CAPABILITY_MAX = (__NL_CAPABILITY_MAX - 1),
#define NL_CAPABILITY_MAX NL_CAPABILITY_MAX

	/**
	 * The range 0x7000 to 0x7FFF is reserved for private capabilities. Upstream libnl3 will
	 * not register capabilities in this range. However, instead of adding private capabilities,
	 * better register their number with upstream libnl3. */
#define NL_CAPABILITY_IS_USER_RESERVED(cap)    ( ((cap) & ~0x0FFF) == 0x7000 )
};
int nl_has_capability (int capability);

#ifdef __cplusplus
}
#endif

#endif
