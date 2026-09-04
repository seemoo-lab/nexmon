/*
 * lib/route/link.c	Links (Interfaces)
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup rtnl
 * @defgroup link Links (Interfaces)
 * @brief
 *
 * @par Link Identification
 * A link can be identified by either its interface index or by its
 * name. The kernel favours the interface index but falls back to the
 * interface name if the interface index is lesser-than 0 for kernels
 * >= 2.6.11. Therefore you can request changes without mapping a
 * interface name to the corresponding index first.
 *
 * @par Changeable Attributes
 * @anchor link_changeable
 *  - Link layer address
 *  - Link layer broadcast address
 *  - device mapping (ifmap) (>= 2.6.9)
 *  - MTU (>= 2.6.9)
 *  - Transmission queue length (>= 2.6.9)
 *  - Weight (>= 2.6.9)
 *  - Link name (only via access through interface index) (>= 2.6.9)
 *  - Flags (>= 2.6.9)
 *    - IFF_DEBUG
 *    - IFF_NOTRAILERS
 *    - IFF_NOARP
 *    - IFF_DYNAMIC
 *    - IFF_MULTICAST
 *    - IFF_PORTSEL
 *    - IFF_AUTOMEDIA
 *    - IFF_UP
 *    - IFF_PROMISC
 *    - IFF_ALLMULTI
 *
 * @par Link Flags (linux/if.h)
 * @anchor link_flags
 * @code
 *   IFF_UP            Status of link (up|down)
 *   IFF_BROADCAST     Indicates this link allows broadcasting
 *   IFF_MULTICAST     Indicates this link allows multicasting
 *   IFF_ALLMULTI      Indicates this link is doing multicast routing
 *   IFF_DEBUG         Tell the driver to do debugging (currently unused)
 *   IFF_LOOPBACK      This is the loopback link
 *   IFF_POINTOPOINT   Point-to-point link
 *   IFF_NOARP         Link is unable to perform ARP
 *   IFF_PROMISC       Status of promiscious mode flag
 *   IFF_MASTER        Used by teql
 *   IFF_SLAVE         Used by teql
 *   IFF_PORTSEL       Indicates this link allows port selection
 *   IFF_AUTOMEDIA     Indicates this link selects port automatically
 *   IFF_DYNAMIC       Indicates the address of this link is dynamic
 *   IFF_RUNNING       Link is running and carrier is ok.
 *   IFF_NOTRAILERS    Unused, BSD compat.
 * @endcode
 *
 * @par Notes on IFF_PROMISC and IFF_ALLMULTI flags
 * Although you can query the status of IFF_PROMISC and IFF_ALLMULTI
 * they do not represent the actual state in the kernel but rather
 * whether the flag has been enabled/disabled by userspace. The link
 * may be in promiscious mode even if IFF_PROMISC is not set in a link
 * dump request response because promiscity might be needed by the driver
 * for a period of time.
 *
 * @note The unit of the transmission queue length depends on the
 *       link type, a common unit is \a packets.
 *
 * @par 1) Retrieving information about available links
 * @code
 * // The first step is to retrieve a list of all available interfaces within
 * // the kernel and put them into a cache.
 * struct nl_cache *cache = rtnl_link_alloc_cache(sk);
 *
 * // In a second step, a specific link may be looked up by either interface
 * // index or interface name.
 * struct rtnl_link *link = rtnl_link_get_by_name(cache, "lo");
 *
 * // rtnl_link_get_by_name() is the short version for translating the
 * // interface name to an interface index first like this:
 * int ifindex = rtnl_link_name2i(cache, "lo");
 * struct rtnl_link *link = rtnl_link_get(cache, ifindex);
 *
 * // After successful usage, the object must be given back to the cache
 * rtnl_link_put(link);
 * @endcode
 *
 * @par 2) Changing link attributes
 * @code
 * // In order to change any attributes of an existing link, we must allocate
 * // a new link to hold the change requests:
 * struct rtnl_link *request = rtnl_link_alloc();
 *
 * // Now we can go on and specify the attributes we want to change:
 * rtnl_link_set_weight(request, 300);
 * rtnl_link_set_mtu(request, 1360);
 *
 * // We can also shut an interface down administratively
 * rtnl_link_unset_flags(request, rtnl_link_str2flags("up"));
 *
 * // Actually, we should know which link to change, so let's look it up
 * struct rtnl_link *old = rtnl_link_get(cache, "eth0");
 *
 * // Two ways exist to commit this change request, the first one is to
 * // build the required netlink message and send it out in one single
 * // step:
 * rtnl_link_change(sk, old, request);
 *
 * // An alternative way is to build the netlink message and send it
 * // out yourself using nl_send_auto_complete()
 * struct nl_msg *msg = rtnl_link_build_change_request(old, request);
 * nl_send_auto_complete(sk, nlmsg_hdr(msg));
 * nlmsg_free(msg);
 *
 * // Don't forget to give back the link object ;->
 * rtnl_link_put(old);
 * @endcode
 *
 * @par 3) Link Type Specific Attributes
 * @code
 * // Some link types offer additional parameters and statistics specific
 * // to their type. F.e. a VLAN link can be configured like this:
 * //
 * // Allocate a new link and set the info type to "vlan". This is required
 * // to prepare the link to hold vlan specific attributes.
 * struct rtnl_link *request = rtnl_link_alloc();
 * rtnl_link_set_info_type(request, "vlan");
 *
 * // Now vlan specific attributes can be set:
 * rtnl_link_vlan_set_id(request, 10);
 * rtnl_link_vlan_set_ingress_map(request, 2, 8);
 *
 * // Of course the attributes can also be read, check the info type
 * // to make sure you are using the right access functions:
 * char *type = rtnl_link_get_info_type(link);
 * if (!strcmp(type, "vlan"))
 * 	int id = rtnl_link_vlan_get_id(link);
 * @endcode
 * @{
 */

#include <netlink-local.h>
#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link.h>
#include <netlink/route/link/info-api.h>

/** @cond SKIP */
#define LINK_ATTR_MTU     0x0001
#define LINK_ATTR_LINK    0x0002
#define LINK_ATTR_TXQLEN  0x0004
#define LINK_ATTR_WEIGHT  0x0008
#define LINK_ATTR_MASTER  0x0010
#define LINK_ATTR_QDISC   0x0020
#define LINK_ATTR_MAP     0x0040
#define LINK_ATTR_ADDR    0x0080
#define LINK_ATTR_BRD     0x0100
#define LINK_ATTR_FLAGS   0x0200
#define LINK_ATTR_IFNAME  0x0400
#define LINK_ATTR_IFINDEX 0x0800
#define LINK_ATTR_FAMILY  0x1000
#define LINK_ATTR_ARPTYPE 0x2000
#define LINK_ATTR_STATS   0x4000
#define LINK_ATTR_CHANGE  0x8000
#define LINK_ATTR_OPERSTATE 0x10000
#define LINK_ATTR_LINKMODE  0x20000
#define LINK_ATTR_LINKINFO  0x40000

static struct nl_cache_ops rtnl_link_ops;
static struct nl_object_ops link_obj_ops;
/** @endcond */

static void release_link_info(struct rtnl_link *link)
{
	struct rtnl_link_info_ops *io = link->l_info_ops;

	if (io != NULL) {
		io->io_refcnt--;
		io->io_free(link);
		link->l_info_ops = NULL;
	}
}

static void link_free_data(struct nl_object *c)
{
	struct rtnl_link *link = nl_object_priv(c);

	if (link) {
		struct rtnl_link_info_ops *io;

		if ((io = link->l_info_ops) != NULL)
			release_link_info(link);

		nl_addr_put(link->l_addr);
		nl_addr_put(link->l_bcast);
	}
}

static int link_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct rtnl_link *dst = nl_object_priv(_dst);
	struct rtnl_link *src = nl_object_priv(_src);
	int err;

	if (src->l_addr)
		if (!(dst->l_addr = nl_addr_clone(src->l_addr)))
			return -NLE_NOMEM;

	if (src->l_bcast)
		if (!(dst->l_bcast = nl_addr_clone(src->l_bcast)))
			return -NLE_NOMEM;

	if (src->l_info_ops && src->l_info_ops->io_clone) {
		err = src->l_info_ops->io_clone(dst, src);
		if (err < 0)
			return err;
	}

	return 0;
}

static struct nla_policy link_policy[IFLA_MAX+1] = {
	[IFLA_IFNAME]	= { .type = NLA_STRING,
			    .maxlen = IFNAMSIZ },
	[IFLA_MTU]	= { .type = NLA_U32 },
	[IFLA_TXQLEN]	= { .type = NLA_U32 },
	[IFLA_LINK]	= { .type = NLA_U32 },
	[IFLA_WEIGHT]	= { .type = NLA_U32 },
	[IFLA_MASTER]	= { .type = NLA_U32 },
	[IFLA_OPERSTATE]= { .type = NLA_U8 },
	[IFLA_LINKMODE] = { .type = NLA_U8 },
	[IFLA_LINKINFO]	= { .type = NLA_NESTED },
	[IFLA_QDISC]	= { .type = NLA_STRING,
			    .maxlen = IFQDISCSIZ },
	[IFLA_STATS]	= { .minlen = sizeof(struct rtnl_link_stats) },
	[IFLA_MAP]	= { .minlen = sizeof(struct rtnl_link_ifmap) },
};

static struct nla_policy link_info_policy[IFLA_INFO_MAX+1] = {
	[IFLA_INFO_KIND]	= { .type = NLA_STRING },
	[IFLA_INFO_DATA]	= { .type = NLA_NESTED },
	[IFLA_INFO_XSTATS]	= { .type = NLA_NESTED },
};

static int link_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
			   struct nlmsghdr *n, struct nl_parser_param *pp)
{
	struct rtnl_link *link;
	struct ifinfomsg *ifi;
	struct nlattr *tb[IFLA_MAX+1];
	int err;

	link = rtnl_link_alloc();
	if (link == NULL) {
		err = -NLE_NOMEM;
		goto errout;
	}
		
	link->ce_msgtype = n->nlmsg_type;

	err = nlmsg_parse(n, sizeof(*ifi), tb, IFLA_MAX, link_policy);
	if (err < 0)
		goto errout;

	if (tb[IFLA_IFNAME] == NULL) {
		err = -NLE_MISSING_ATTR;
		goto errout;
	}

	nla_strlcpy(link->l_name, tb[IFLA_IFNAME], IFNAMSIZ);

	ifi = nlmsg_data(n);
	link->l_family = ifi->ifi_family;
	link->l_arptype = ifi->ifi_type;
	link->l_index = ifi->ifi_index;
	link->l_flags = ifi->ifi_flags;
	link->l_change = ifi->ifi_change;
	link->ce_mask = (LINK_ATTR_IFNAME | LINK_ATTR_FAMILY |
			  LINK_ATTR_ARPTYPE| LINK_ATTR_IFINDEX |
			  LINK_ATTR_FLAGS | LINK_ATTR_CHANGE);

	if (tb[IFLA_STATS]) {
		struct rtnl_link_stats *st = nla_data(tb[IFLA_STATS]);
		
		link->l_stats[RTNL_LINK_RX_PACKETS]	= st->rx_packets;
		link->l_stats[RTNL_LINK_RX_BYTES]	= st->rx_bytes;
		link->l_stats[RTNL_LINK_RX_ERRORS]	= st->rx_errors;
		link->l_stats[RTNL_LINK_RX_DROPPED]	= st->rx_dropped;
		link->l_stats[RTNL_LINK_RX_COMPRESSED]	= st->rx_compressed;
		link->l_stats[RTNL_LINK_RX_FIFO_ERR]	= st->rx_fifo_errors;
		link->l_stats[RTNL_LINK_TX_PACKETS]	= st->tx_packets;
		link->l_stats[RTNL_LINK_TX_BYTES]	= st->tx_bytes;
		link->l_stats[RTNL_LINK_TX_ERRORS]	= st->tx_errors;
		link->l_stats[RTNL_LINK_TX_DROPPED]	= st->tx_dropped;
		link->l_stats[RTNL_LINK_TX_COMPRESSED]	= st->tx_compressed;
		link->l_stats[RTNL_LINK_TX_FIFO_ERR]	= st->tx_fifo_errors;
		link->l_stats[RTNL_LINK_RX_LEN_ERR]	= st->rx_length_errors;
		link->l_stats[RTNL_LINK_RX_OVER_ERR]	= st->rx_over_errors;
		link->l_stats[RTNL_LINK_RX_CRC_ERR]	= st->rx_crc_errors;
		link->l_stats[RTNL_LINK_RX_FRAME_ERR]	= st->rx_frame_errors;
		link->l_stats[RTNL_LINK_RX_MISSED_ERR]	= st->rx_missed_errors;
		link->l_stats[RTNL_LINK_TX_ABORT_ERR]	= st->tx_aborted_errors;
		link->l_stats[RTNL_LINK_TX_CARRIER_ERR]	= st->tx_carrier_errors;
		link->l_stats[RTNL_LINK_TX_HBEAT_ERR]	= st->tx_heartbeat_errors;
		link->l_stats[RTNL_LINK_TX_WIN_ERR]	= st->tx_window_errors;
		link->l_stats[RTNL_LINK_MULTICAST]	= st->multicast;

		link->ce_mask |= LINK_ATTR_STATS;
	}

	if (tb[IFLA_TXQLEN]) {
		link->l_txqlen = nla_get_u32(tb[IFLA_TXQLEN]);
		link->ce_mask |= LINK_ATTR_TXQLEN;
	}

	if (tb[IFLA_MTU]) {
		link->l_mtu = nla_get_u32(tb[IFLA_MTU]);
		link->ce_mask |= LINK_ATTR_MTU;
	}

	if (tb[IFLA_ADDRESS]) {
		link->l_addr = nl_addr_alloc_attr(tb[IFLA_ADDRESS], AF_UNSPEC);
		if (link->l_addr == NULL) {
			err = -NLE_NOMEM;
			goto errout;
		}
		nl_addr_set_family(link->l_addr,
				   nl_addr_guess_family(link->l_addr));
		link->ce_mask |= LINK_ATTR_ADDR;
	}

	if (tb[IFLA_BROADCAST]) {
		link->l_bcast = nl_addr_alloc_attr(tb[IFLA_BROADCAST],
						   AF_UNSPEC);
		if (link->l_bcast == NULL) {
			err = -NLE_NOMEM;
			goto errout;
		}
		nl_addr_set_family(link->l_bcast,
				   nl_addr_guess_family(link->l_bcast));
		link->ce_mask |= LINK_ATTR_BRD;
	}

	if (tb[IFLA_LINK]) {
		link->l_link = nla_get_u32(tb[IFLA_LINK]);
		link->ce_mask |= LINK_ATTR_LINK;
	}

	if (tb[IFLA_WEIGHT]) {
		link->l_weight = nla_get_u32(tb[IFLA_WEIGHT]);
		link->ce_mask |= LINK_ATTR_WEIGHT;
	}

	if (tb[IFLA_QDISC]) {
		nla_strlcpy(link->l_qdisc, tb[IFLA_QDISC], IFQDISCSIZ);
		link->ce_mask |= LINK_ATTR_QDISC;
	}

	if (tb[IFLA_MAP]) {
		nla_memcpy(&link->l_map, tb[IFLA_MAP], 
			   sizeof(struct rtnl_link_ifmap));
		link->ce_mask |= LINK_ATTR_MAP;
	}

	if (tb[IFLA_MASTER]) {
		link->l_master = nla_get_u32(tb[IFLA_MASTER]);
		link->ce_mask |= LINK_ATTR_MASTER;
	}

	if (tb[IFLA_OPERSTATE]) {
		link->l_operstate = nla_get_u8(tb[IFLA_OPERSTATE]);
		link->ce_mask |= LINK_ATTR_OPERSTATE;
	}

	if (tb[IFLA_LINKMODE]) {
		link->l_linkmode = nla_get_u8(tb[IFLA_LINKMODE]);
		link->ce_mask |= LINK_ATTR_LINKMODE;
	}

	if (tb[IFLA_LINKINFO]) {
		struct nlattr *li[IFLA_INFO_MAX+1];

		err = nla_parse_nested(li, IFLA_INFO_MAX, tb[IFLA_LINKINFO],
				       link_info_policy);
		if (err < 0)
			goto errout;

		if (li[IFLA_INFO_KIND] &&
		    (li[IFLA_INFO_DATA] || li[IFLA_INFO_XSTATS])) {
			struct rtnl_link_info_ops *ops;
			char *kind;

			kind = nla_get_string(li[IFLA_INFO_KIND]);
			ops = rtnl_link_info_ops_lookup(kind);
			if (ops != NULL) {
				ops->io_refcnt++;
				link->l_info_ops = ops;
				err = ops->io_parse(link, li[IFLA_INFO_DATA],
						    li[IFLA_INFO_XSTATS]);
				if (err < 0)
					goto errout;
			} else {
				/* XXX: Warn about unparsed info? */
			}
		}
	}

	err = pp->pp_cb((struct nl_object *) link, pp);
errout:
	rtnl_link_put(link);
	return err;
}

static int link_request_update(struct nl_cache *cache, struct nl_sock *sk)
{
	return nl_rtgen_request(sk, RTM_GETLINK, AF_UNSPEC, NLM_F_DUMP);
}

static void link_dump_line(struct nl_object *obj, struct nl_dump_params *p)
{
	char buf[128];
	struct nl_cache *cache = dp_cache(obj);
	struct rtnl_link *link = (struct rtnl_link *) obj;

	nl_dump_line(p, "%s %s ", link->l_name,
		     nl_llproto2str(link->l_arptype, buf, sizeof(buf)));

	if (link->l_addr && !nl_addr_iszero(link->l_addr))
		nl_dump(p, "%s ", nl_addr2str(link->l_addr, buf, sizeof(buf)));

	if (link->ce_mask & LINK_ATTR_MASTER) {
		struct rtnl_link *master = rtnl_link_get(cache, link->l_master);
		nl_dump(p, "master %s ", master ? master->l_name : "inv");
		if (master)
			rtnl_link_put(master);
	}

	rtnl_link_flags2str(link->l_flags, buf, sizeof(buf));
	if (buf[0])
		nl_dump(p, "<%s> ", buf);

	if (link->ce_mask & LINK_ATTR_LINK) {
		struct rtnl_link *ll = rtnl_link_get(cache, link->l_link);
		nl_dump(p, "slave-of %s ", ll ? ll->l_name : "NONE");
		if (ll)
			rtnl_link_put(ll);
	}

	if (link->l_info_ops && link->l_info_ops->io_dump[NL_DUMP_LINE])
		link->l_info_ops->io_dump[NL_DUMP_LINE](link, p);

	nl_dump(p, "\n");
}

static void link_dump_details(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_link *link = (struct rtnl_link *) obj;
	char buf[64];

	link_dump_line(obj, p);

	nl_dump_line(p, "    mtu %u ", link->l_mtu);
	nl_dump(p, "txqlen %u weight %u ", link->l_txqlen, link->l_weight);

	if (link->ce_mask & LINK_ATTR_QDISC)
		nl_dump(p, "qdisc %s ", link->l_qdisc);

	if (link->ce_mask & LINK_ATTR_MAP && link->l_map.lm_irq)
		nl_dump(p, "irq %u ", link->l_map.lm_irq);

	if (link->ce_mask & LINK_ATTR_IFINDEX)
		nl_dump(p, "index %u ", link->l_index);


	nl_dump(p, "\n");
	nl_dump_line(p, "    ");

	if (link->ce_mask & LINK_ATTR_BRD)
		nl_dump(p, "brd %s ", nl_addr2str(link->l_bcast, buf,
						   sizeof(buf)));

	if ((link->ce_mask & LINK_ATTR_OPERSTATE) &&
	    link->l_operstate != IF_OPER_UNKNOWN) {
		rtnl_link_operstate2str(link->l_operstate, buf, sizeof(buf));
		nl_dump(p, "state %s ", buf);
	}

	nl_dump(p, "mode %s\n",
		rtnl_link_mode2str(link->l_linkmode, buf, sizeof(buf)));

	if (link->l_info_ops && link->l_info_ops->io_dump[NL_DUMP_DETAILS])
		link->l_info_ops->io_dump[NL_DUMP_DETAILS](link, p);
}

static void link_dump_stats(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_link *link = (struct rtnl_link *) obj;
	char *unit, fmt[64];
	float res;
	
	link_dump_details(obj, p);

	nl_dump_line(p, "    Stats:    bytes    packets     errors "
			"   dropped   fifo-err compressed\n");

	res = nl_cancel_down_bytes(link->l_stats[RTNL_LINK_RX_BYTES], &unit);

	strcpy(fmt, "     RX %X.2f %s %10llu %10llu %10llu %10llu %10llu\n");
	fmt[9] = *unit == 'B' ? '9' : '7';
	
	nl_dump_line(p, fmt, res, unit,
		link->l_stats[RTNL_LINK_RX_PACKETS],
		link->l_stats[RTNL_LINK_RX_ERRORS],
		link->l_stats[RTNL_LINK_RX_DROPPED],
		link->l_stats[RTNL_LINK_RX_FIFO_ERR],
		link->l_stats[RTNL_LINK_RX_COMPRESSED]);

	res = nl_cancel_down_bytes(link->l_stats[RTNL_LINK_TX_BYTES], &unit);

	strcpy(fmt, "     TX %X.2f %s %10llu %10llu %10llu %10llu %10llu\n");
	fmt[9] = *unit == 'B' ? '9' : '7';
	
	nl_dump_line(p, fmt, res, unit,
		link->l_stats[RTNL_LINK_TX_PACKETS],
		link->l_stats[RTNL_LINK_TX_ERRORS],
		link->l_stats[RTNL_LINK_TX_DROPPED],
		link->l_stats[RTNL_LINK_TX_FIFO_ERR],
		link->l_stats[RTNL_LINK_TX_COMPRESSED]);

	nl_dump_line(p, "    Errors:  length       over        crc "
			"     frame     missed  multicast\n");

	nl_dump_line(p, "     RX  %10" PRIu64 " %10" PRIu64 " %10"
				PRIu64 " %10" PRIu64 " %10" PRIu64 " %10"
				PRIu64 "\n",
		link->l_stats[RTNL_LINK_RX_LEN_ERR],
		link->l_stats[RTNL_LINK_RX_OVER_ERR],
		link->l_stats[RTNL_LINK_RX_CRC_ERR],
		link->l_stats[RTNL_LINK_RX_FRAME_ERR],
		link->l_stats[RTNL_LINK_RX_MISSED_ERR],
		link->l_stats[RTNL_LINK_MULTICAST]);

	nl_dump_line(p, "            aborted    carrier  heartbeat "
			"    window  collision\n");
	
	nl_dump_line(p, "     TX  %10" PRIu64 " %10" PRIu64 " %10"
			PRIu64 " %10" PRIu64 " %10" PRIu64 "\n",
		link->l_stats[RTNL_LINK_TX_ABORT_ERR],
		link->l_stats[RTNL_LINK_TX_CARRIER_ERR],
		link->l_stats[RTNL_LINK_TX_HBEAT_ERR],
		link->l_stats[RTNL_LINK_TX_WIN_ERR],
		link->l_stats[RTNL_LINK_TX_COLLISIONS]);

	if (link->l_info_ops && link->l_info_ops->io_dump[NL_DUMP_STATS])
		link->l_info_ops->io_dump[NL_DUMP_STATS](link, p);
}

static void link_dump_env(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_link *link = (struct rtnl_link *) obj;
	struct nl_cache *cache = dp_cache(obj);
	char buf[128];
	int i;

	nl_dump_line(p, "LINK_NAME=%s\n", link->l_name);
	nl_dump_line(p, "LINK_IFINDEX=%u\n", link->l_index);
	nl_dump_line(p, "LINK_FAMILY=%s\n",
		     nl_af2str(link->l_family, buf, sizeof(buf)));
	nl_dump_line(p, "LINK_TYPE=%s\n",
		     nl_llproto2str(link->l_arptype, buf, sizeof(buf)));
	if (link->ce_mask & LINK_ATTR_ADDR)
		nl_dump_line(p, "LINK_ADDRESS=%s\n",
			     nl_addr2str(link->l_addr, buf, sizeof(buf)));
	nl_dump_line(p, "LINK_MTU=%u\n", link->l_mtu);
	nl_dump_line(p, "LINK_TXQUEUELEN=%u\n", link->l_txqlen);
	nl_dump_line(p, "LINK_WEIGHT=%u\n", link->l_weight);

	rtnl_link_flags2str(link->l_flags & ~IFF_RUNNING, buf, sizeof(buf));
	if (buf[0])
		nl_dump_line(p, "LINK_FLAGS=%s\n", buf);

	if (link->ce_mask & LINK_ATTR_QDISC)
		nl_dump_line(p, "LINK_QDISC=%s\n", link->l_qdisc);

	if (link->ce_mask & LINK_ATTR_LINK) {
		struct rtnl_link *ll = rtnl_link_get(cache, link->l_link);

		nl_dump_line(p, "LINK_LINK_IFINDEX=%d\n", link->l_link);
		if (ll) {
			nl_dump_line(p, "LINK_LINK_IFNAME=%s\n", ll->l_name);
			rtnl_link_put(ll);
		}
	}

	if (link->ce_mask & LINK_ATTR_MASTER) {
		struct rtnl_link *master = rtnl_link_get(cache, link->l_master);
		nl_dump_line(p, "LINK_MASTER=%s\n",
			     master ? master->l_name : "none");
		if (master)
			rtnl_link_put(master);
	}

	if (link->ce_mask & LINK_ATTR_BRD)
		nl_dump_line(p, "LINK_BROADCAST=%s\n",
			     nl_addr2str(link->l_bcast, buf, sizeof(buf)));

	if (link->ce_mask & LINK_ATTR_STATS) {
		for (i = 0; i <= RTNL_LINK_STATS_MAX; i++) {
			char *c = buf;

			sprintf(buf, "LINK_");
			rtnl_link_stat2str(i, buf + 5, sizeof(buf) - 5);
			while (*c) {
				*c = toupper(*c);
				c++;
			}
			nl_dump_line(p, "%s=%" PRIu64 "\n", buf, link->l_stats[i]);
		}
	}

	if (link->l_info_ops && link->l_info_ops->io_dump[NL_DUMP_ENV])
		link->l_info_ops->io_dump[NL_DUMP_ENV](link, p);
}

#if 0
static int link_handle_event(struct nl_object *a, struct rtnl_link_event_cb *cb)
{
	struct rtnl_link *l = (struct rtnl_link *) a;
	struct nl_cache *c = dp_cache(a);
	int nevents = 0;

	if (l->l_change == ~0U) {
		if (l->ce_msgtype == RTM_NEWLINK)
			cb->le_register(l);
		else
			cb->le_unregister(l);

		return 1;
	}

	if (l->l_change & IFF_SLAVE) {
		if (l->l_flags & IFF_SLAVE) {
			struct rtnl_link *m = rtnl_link_get(c, l->l_master);
			cb->le_new_bonding(l, m);
			if (m)
				rtnl_link_put(m);
		} else
			cb->le_cancel_bonding(l);
	}

#if 0
	if (l->l_change & IFF_UP && l->l_change & IFF_RUNNING)
		dp_dump_line(p, line++, "link %s changed state to %s.\n",
			l->l_name, l->l_flags & IFF_UP ? "up" : "down");

	if (l->l_change & IFF_PROMISC) {
		dp_new_line(p, line++);
		dp_dump(p, "link %s %s promiscuous mode.\n",
		    l->l_name, l->l_flags & IFF_PROMISC ? "entered" : "left");
	}

	if (line == 0)
		dp_dump_line(p, line++, "link %s sent unknown event.\n",
			     l->l_name);
#endif

	return nevents;
}
#endif

static int link_compare(struct nl_object *_a, struct nl_object *_b,
			uint32_t attrs, int flags)
{
	struct rtnl_link *a = (struct rtnl_link *) _a;
	struct rtnl_link *b = (struct rtnl_link *) _b;
	int diff = 0;

#define LINK_DIFF(ATTR, EXPR) ATTR_DIFF(attrs, LINK_ATTR_##ATTR, a, b, EXPR)

	diff |= LINK_DIFF(IFINDEX,	a->l_index != b->l_index);
	diff |= LINK_DIFF(MTU,		a->l_mtu != b->l_mtu);
	diff |= LINK_DIFF(LINK,		a->l_link != b->l_link);
	diff |= LINK_DIFF(TXQLEN,	a->l_txqlen != b->l_txqlen);
	diff |= LINK_DIFF(WEIGHT,	a->l_weight != b->l_weight);
	diff |= LINK_DIFF(MASTER,	a->l_master != b->l_master);
	diff |= LINK_DIFF(FAMILY,	a->l_family != b->l_family);
	diff |= LINK_DIFF(OPERSTATE,	a->l_operstate != b->l_operstate);
	diff |= LINK_DIFF(LINKMODE,	a->l_linkmode != b->l_linkmode);
	diff |= LINK_DIFF(QDISC,	strcmp(a->l_qdisc, b->l_qdisc));
	diff |= LINK_DIFF(IFNAME,	strcmp(a->l_name, b->l_name));
	diff |= LINK_DIFF(ADDR,		nl_addr_cmp(a->l_addr, b->l_addr));
	diff |= LINK_DIFF(BRD,		nl_addr_cmp(a->l_bcast, b->l_bcast));

	if (flags & LOOSE_COMPARISON)
		diff |= LINK_DIFF(FLAGS,
				  (a->l_flags ^ b->l_flags) & b->l_flag_mask);
	else
		diff |= LINK_DIFF(FLAGS, a->l_flags != b->l_flags);

#undef LINK_DIFF

	return diff;
}

static struct trans_tbl link_attrs[] = {
	__ADD(LINK_ATTR_MTU, mtu)
	__ADD(LINK_ATTR_LINK, link)
	__ADD(LINK_ATTR_TXQLEN, txqlen)
	__ADD(LINK_ATTR_WEIGHT, weight)
	__ADD(LINK_ATTR_MASTER, master)
	__ADD(LINK_ATTR_QDISC, qdisc)
	__ADD(LINK_ATTR_MAP, map)
	__ADD(LINK_ATTR_ADDR, address)
	__ADD(LINK_ATTR_BRD, broadcast)
	__ADD(LINK_ATTR_FLAGS, flags)
	__ADD(LINK_ATTR_IFNAME, name)
	__ADD(LINK_ATTR_IFINDEX, ifindex)
	__ADD(LINK_ATTR_FAMILY, family)
	__ADD(LINK_ATTR_ARPTYPE, arptype)
	__ADD(LINK_ATTR_STATS, stats)
	__ADD(LINK_ATTR_CHANGE, change)
	__ADD(LINK_ATTR_OPERSTATE, operstate)
	__ADD(LINK_ATTR_LINKMODE, linkmode)
};

static char *link_attrs2str(int attrs, char *buf, size_t len)
{
	return __flags2str(attrs, buf, len, link_attrs,
			   ARRAY_SIZE(link_attrs));
}

/**
 * @name Allocation/Freeing
 * @{
 */

struct rtnl_link *rtnl_link_alloc(void)
{
	return (struct rtnl_link *) nl_object_alloc(&link_obj_ops);
}

void rtnl_link_put(struct rtnl_link *link)
{
	nl_object_put((struct nl_object *) link);
}

/** @} */

/**
 * @name Cache Management
 * @{
 */


/**
 * Allocate link cache and fill in all configured links.
 * @arg sk		Netlink socket.
 * @arg result		Pointer to store resulting cache.
 *
 * Allocates a new link cache, initializes it properly and updates it
 * to include all links currently configured in the kernel.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_alloc_cache(struct nl_sock *sk, struct nl_cache **result)
{
	return nl_cache_alloc_and_fill(&rtnl_link_ops, sk, result);
}

/**
 * Look up link by interface index in the provided cache
 * @arg cache		link cache
 * @arg ifindex		link interface index
 *
 * The caller owns a reference on the returned object and
 * must give the object back via rtnl_link_put().
 *
 * @return pointer to link inside the cache or NULL if no match was found.
 */
struct rtnl_link *rtnl_link_get(struct nl_cache *cache, int ifindex)
{
	struct rtnl_link *link;

	if (cache->c_ops != &rtnl_link_ops)
		return NULL;

	nl_list_for_each_entry(link, &cache->c_items, ce_list) {
		if (link->l_index == ifindex) {
			nl_object_get((struct nl_object *) link);
			return link;
		}
	}

	return NULL;
}

/**
 * Look up link by link name in the provided cache
 * @arg cache		link cache
 * @arg name		link name
 *
 * The caller owns a reference on the returned object and
 * must give the object back via rtnl_link_put().
 *
 * @return pointer to link inside the cache or NULL if no match was found.
 */
struct rtnl_link *rtnl_link_get_by_name(struct nl_cache *cache,
					 const char *name)
{
	struct rtnl_link *link;

	if (cache->c_ops != &rtnl_link_ops)
		return NULL;

	nl_list_for_each_entry(link, &cache->c_items, ce_list) {
		if (!strcmp(name, link->l_name)) {
			nl_object_get((struct nl_object *) link);
			return link;
		}
	}

	return NULL;
}

/** @} */

/**
 * @name Link Modifications
 * @{
 */

/**
 * Builds a netlink change request message to change link attributes
 * @arg old		link to be changed
 * @arg tmpl		template with requested changes
 * @arg flags		additional netlink message flags
 *
 * Builds a new netlink message requesting a change of link attributes.
 * The netlink message header isn't fully equipped with all relevant
 * fields and must be sent out via nl_send_auto_complete() or
 * supplemented as needed.
 * \a old must point to a link currently configured in the kernel
 * and \a tmpl must contain the attributes to be changed set via
 * \c rtnl_link_set_* functions.
 *
 * @return New netlink message
 * @note Not all attributes can be changed, see
 *       \ref link_changeable "Changeable Attributes" for more details.
 */
int rtnl_link_build_change_request(struct rtnl_link *old,
				   struct rtnl_link *tmpl, int flags,
				   struct nl_msg **result)
{
	struct nl_msg *msg;
	struct ifinfomsg ifi = {
		.ifi_family = old->l_family,
		.ifi_index = old->l_index,
	};

	if (tmpl->ce_mask & LINK_ATTR_FLAGS) {
		ifi.ifi_flags = old->l_flags & ~tmpl->l_flag_mask;
		ifi.ifi_flags |= tmpl->l_flags;
	}

	msg = nlmsg_alloc_simple(RTM_SETLINK, flags);
	if (!msg)
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &ifi, sizeof(ifi), NLMSG_ALIGNTO) < 0)
		goto nla_put_failure;

	if (tmpl->ce_mask & LINK_ATTR_ADDR)
		NLA_PUT_ADDR(msg, IFLA_ADDRESS, tmpl->l_addr);

	if (tmpl->ce_mask & LINK_ATTR_BRD)
		NLA_PUT_ADDR(msg, IFLA_BROADCAST, tmpl->l_bcast);

	if (tmpl->ce_mask & LINK_ATTR_MTU)
		NLA_PUT_U32(msg, IFLA_MTU, tmpl->l_mtu);

	if (tmpl->ce_mask & LINK_ATTR_TXQLEN)
		NLA_PUT_U32(msg, IFLA_TXQLEN, tmpl->l_txqlen);

	if (tmpl->ce_mask & LINK_ATTR_WEIGHT)
		NLA_PUT_U32(msg, IFLA_WEIGHT, tmpl->l_weight);

	if (tmpl->ce_mask & LINK_ATTR_IFNAME)
		NLA_PUT_STRING(msg, IFLA_IFNAME, tmpl->l_name);

	if (tmpl->ce_mask & LINK_ATTR_OPERSTATE)
		NLA_PUT_U8(msg, IFLA_OPERSTATE, tmpl->l_operstate);

	if (tmpl->ce_mask & LINK_ATTR_LINKMODE)
		NLA_PUT_U8(msg, IFLA_LINKMODE, tmpl->l_linkmode);

	if ((tmpl->ce_mask & LINK_ATTR_LINKINFO) && tmpl->l_info_ops &&
	    tmpl->l_info_ops->io_put_attrs) {
		struct nlattr *info;

		if (!(info = nla_nest_start(msg, IFLA_LINKINFO)))
			goto nla_put_failure;

		NLA_PUT_STRING(msg, IFLA_INFO_KIND, tmpl->l_info_ops->io_name);

		if (tmpl->l_info_ops->io_put_attrs(msg, tmpl) < 0)
			goto nla_put_failure;

		nla_nest_end(msg, info);
	}

	*result = msg;
	return 0;

nla_put_failure:
	nlmsg_free(msg);
	return -NLE_MSGSIZE;
}

/**
 * Change link attributes
 * @arg sk		Netlink socket.
 * @arg old		link to be changed
 * @arg tmpl		template with requested changes
 * @arg flags		additional netlink message flags
 *
 * Builds a new netlink message by calling rtnl_link_build_change_request(),
 * sends the request to the kernel and waits for the next ACK to be
 * received, i.e. blocks until the request has been processed.
 *
 * @return 0 on success or a negative error code
 * @note Not all attributes can be changed, see
 *       \ref link_changeable "Changeable Attributes" for more details.
 */
int rtnl_link_change(struct nl_sock *sk, struct rtnl_link *old,
		     struct rtnl_link *tmpl, int flags)
{
	struct nl_msg *msg;
	int err;
	
	if ((err = rtnl_link_build_change_request(old, tmpl, flags, &msg)) < 0)
		return err;
	
	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return wait_for_ack(sk);
}

/** @} */

/**
 * @name Name <-> Index Translations
 * @{
 */

/**
 * Translate an interface index to the corresponding link name
 * @arg cache		link cache
 * @arg ifindex		link interface index
 * @arg dst		destination buffer
 * @arg len		length of destination buffer
 *
 * Translates the specified interface index to the corresponding
 * link name and stores the name in the destination buffer.
 *
 * @return link name or NULL if no match was found.
 */
char * rtnl_link_i2name(struct nl_cache *cache, int ifindex, char *dst,
			size_t len)
{
	struct rtnl_link *link = rtnl_link_get(cache, ifindex);

	if (link) {
		strncpy(dst, link->l_name, len - 1);
		rtnl_link_put(link);
		return dst;
	}

	return NULL;
}

/**
 * Translate a link name to the corresponding interface index
 * @arg cache		link cache
 * @arg name		link name
 *
 * @return interface index or 0 if no match was found.
 */
int rtnl_link_name2i(struct nl_cache *cache, const char *name)
{
	int ifindex = 0;
	struct rtnl_link *link;
	
	link = rtnl_link_get_by_name(cache, name);
	if (link) {
		ifindex = link->l_index;
		rtnl_link_put(link);
	}

	return ifindex;
}

/** @} */

/**
 * @name Link Flags Translations
 * @{
 */

static struct trans_tbl link_flags[] = {
	__ADD(IFF_LOOPBACK, loopback)
	__ADD(IFF_BROADCAST, broadcast)
	__ADD(IFF_POINTOPOINT, pointopoint)
	__ADD(IFF_MULTICAST, multicast)
	__ADD(IFF_NOARP, noarp)
	__ADD(IFF_ALLMULTI, allmulti)
	__ADD(IFF_PROMISC, promisc)
	__ADD(IFF_MASTER, master)
	__ADD(IFF_SLAVE, slave)
	__ADD(IFF_DEBUG, debug)
	__ADD(IFF_DYNAMIC, dynamic)
	__ADD(IFF_AUTOMEDIA, automedia)
	__ADD(IFF_PORTSEL, portsel)
	__ADD(IFF_NOTRAILERS, notrailers)
	__ADD(IFF_UP, up)
	__ADD(IFF_RUNNING, running)
	__ADD(IFF_LOWER_UP, lowerup)
	__ADD(IFF_DORMANT, dormant)
	__ADD(IFF_ECHO, echo)
};

char * rtnl_link_flags2str(int flags, char *buf, size_t len)
{
	return __flags2str(flags, buf, len, link_flags,
			   ARRAY_SIZE(link_flags));
}

int rtnl_link_str2flags(const char *name)
{
	return __str2flags(name, link_flags, ARRAY_SIZE(link_flags));
}

/** @} */

/**
 * @name Link Statistics Translations
 * @{
 */

static struct trans_tbl link_stats[] = {
	__ADD(RTNL_LINK_RX_PACKETS, rx_packets)
	__ADD(RTNL_LINK_TX_PACKETS, tx_packets)
	__ADD(RTNL_LINK_RX_BYTES, rx_bytes)
	__ADD(RTNL_LINK_TX_BYTES, tx_bytes)
	__ADD(RTNL_LINK_RX_ERRORS, rx_errors)
	__ADD(RTNL_LINK_TX_ERRORS, tx_errors)
	__ADD(RTNL_LINK_RX_DROPPED, rx_dropped)
	__ADD(RTNL_LINK_TX_DROPPED, tx_dropped)
	__ADD(RTNL_LINK_RX_COMPRESSED, rx_compressed)
	__ADD(RTNL_LINK_TX_COMPRESSED, tx_compressed)
	__ADD(RTNL_LINK_RX_FIFO_ERR, rx_fifo_err)
	__ADD(RTNL_LINK_TX_FIFO_ERR, tx_fifo_err)
	__ADD(RTNL_LINK_RX_LEN_ERR, rx_len_err)
	__ADD(RTNL_LINK_RX_OVER_ERR, rx_over_err)
	__ADD(RTNL_LINK_RX_CRC_ERR, rx_crc_err)
	__ADD(RTNL_LINK_RX_FRAME_ERR, rx_frame_err)
	__ADD(RTNL_LINK_RX_MISSED_ERR, rx_missed_err)
	__ADD(RTNL_LINK_TX_ABORT_ERR, tx_abort_err)
	__ADD(RTNL_LINK_TX_CARRIER_ERR, tx_carrier_err)
	__ADD(RTNL_LINK_TX_HBEAT_ERR, tx_hbeat_err)
	__ADD(RTNL_LINK_TX_WIN_ERR, tx_win_err)
	__ADD(RTNL_LINK_TX_COLLISIONS, tx_collision)
	__ADD(RTNL_LINK_MULTICAST, multicast)
};

char *rtnl_link_stat2str(int st, char *buf, size_t len)
{
	return __type2str(st, buf, len, link_stats, ARRAY_SIZE(link_stats));
}

int rtnl_link_str2stat(const char *name)
{
	return __str2type(name, link_stats, ARRAY_SIZE(link_stats));
}

/** @} */

/**
 * @name Link Operstate Translations
 * @{
 */

static struct trans_tbl link_operstates[] = {
	__ADD(IF_OPER_UNKNOWN, unknown)
	__ADD(IF_OPER_NOTPRESENT, notpresent)
	__ADD(IF_OPER_DOWN, down)
	__ADD(IF_OPER_LOWERLAYERDOWN, lowerlayerdown)
	__ADD(IF_OPER_TESTING, testing)
	__ADD(IF_OPER_DORMANT, dormant)
	__ADD(IF_OPER_UP, up)
};

char *rtnl_link_operstate2str(int st, char *buf, size_t len)
{
	return __type2str(st, buf, len, link_operstates,
			  ARRAY_SIZE(link_operstates));
}

int rtnl_link_str2operstate(const char *name)
{
	return __str2type(name, link_operstates,
			  ARRAY_SIZE(link_operstates));
}

/** @} */

/**
 * @name Link Mode Translations
 * @{
 */

static struct trans_tbl link_modes[] = {
	__ADD(IF_LINK_MODE_DEFAULT, default)
	__ADD(IF_LINK_MODE_DORMANT, dormant)
};

char *rtnl_link_mode2str(int st, char *buf, size_t len)
{
	return __type2str(st, buf, len, link_modes, ARRAY_SIZE(link_modes));
}

int rtnl_link_str2mode(const char *name)
{
	return __str2type(name, link_modes, ARRAY_SIZE(link_modes));
}

/** @} */

/**
 * @name Attributes
 * @{
 */

void rtnl_link_set_qdisc(struct rtnl_link *link, const char *qdisc)
{
	strncpy(link->l_qdisc, qdisc, sizeof(link->l_qdisc) - 1);
	link->ce_mask |= LINK_ATTR_QDISC;
}

char *rtnl_link_get_qdisc(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_QDISC)
		return link->l_qdisc;
	else
		return NULL;
}

void rtnl_link_set_name(struct rtnl_link *link, const char *name)
{
	strncpy(link->l_name, name, sizeof(link->l_name) - 1);
	link->ce_mask |= LINK_ATTR_IFNAME;
}

char *rtnl_link_get_name(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_IFNAME)
		return link->l_name;
	else
		return NULL;
}

static inline void __assign_addr(struct rtnl_link *link, struct nl_addr **pos,
				 struct nl_addr *new, int flag)
{
	if (*pos)
		nl_addr_put(*pos);

	nl_addr_get(new);
	*pos = new;

	link->ce_mask |= flag;
}

void rtnl_link_set_addr(struct rtnl_link *link, struct nl_addr *addr)
{
	__assign_addr(link, &link->l_addr, addr, LINK_ATTR_ADDR);
}

struct nl_addr *rtnl_link_get_addr(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_ADDR)
		return link->l_addr;
	else
		return NULL;
}

void rtnl_link_set_broadcast(struct rtnl_link *link, struct nl_addr *brd)
{
	__assign_addr(link, &link->l_bcast, brd, LINK_ATTR_BRD);
}

struct nl_addr *rtnl_link_get_broadcast(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_BRD)
		return link->l_bcast;
	else
		return NULL;
}

void rtnl_link_set_flags(struct rtnl_link *link, unsigned int flags)
{
	link->l_flag_mask |= flags;
	link->l_flags |= flags;
	link->ce_mask |= LINK_ATTR_FLAGS;
}

void rtnl_link_unset_flags(struct rtnl_link *link, unsigned int flags)
{
	link->l_flag_mask |= flags;
	link->l_flags &= ~flags;
	link->ce_mask |= LINK_ATTR_FLAGS;
}

unsigned int rtnl_link_get_flags(struct rtnl_link *link)
{
	return link->l_flags;
}

void rtnl_link_set_family(struct rtnl_link *link, int family)
{
	link->l_family = family;
	link->ce_mask |= LINK_ATTR_FAMILY;
}

int rtnl_link_get_family(struct rtnl_link *link)
{
	if (link->l_family & LINK_ATTR_FAMILY)
		return link->l_family;
	else
		return AF_UNSPEC;
}

void rtnl_link_set_arptype(struct rtnl_link *link, unsigned int arptype)
{
	link->l_arptype = arptype;
}

unsigned int rtnl_link_get_arptype(struct rtnl_link *link)
{
	return link->l_arptype;
}

void rtnl_link_set_ifindex(struct rtnl_link *link, int ifindex)
{
	link->l_index = ifindex;
	link->ce_mask |= LINK_ATTR_IFINDEX;
}

int rtnl_link_get_ifindex(struct rtnl_link *link)
{
	return link->l_index;
}

void rtnl_link_set_mtu(struct rtnl_link *link, unsigned int mtu)
{
	link->l_mtu = mtu;
	link->ce_mask |= LINK_ATTR_MTU;
}

unsigned int rtnl_link_get_mtu(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_MTU)
		return link->l_mtu;
	else
		return 0;
}

void rtnl_link_set_txqlen(struct rtnl_link *link, unsigned int txqlen)
{
	link->l_txqlen = txqlen;
	link->ce_mask |= LINK_ATTR_TXQLEN;
}

unsigned int rtnl_link_get_txqlen(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_TXQLEN)
		return link->l_txqlen;
	else
		return UINT_MAX;
}

void rtnl_link_set_weight(struct rtnl_link *link, unsigned int weight)
{
	link->l_weight = weight;
	link->ce_mask |= LINK_ATTR_WEIGHT;
}

unsigned int rtnl_link_get_weight(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_WEIGHT)
		return link->l_weight;
	else
		return UINT_MAX;
}

void rtnl_link_set_link(struct rtnl_link *link, int ifindex)
{
	link->l_link = ifindex;
	link->ce_mask |= LINK_ATTR_LINK;
}

int rtnl_link_get_link(struct rtnl_link *link)
{
	return link->l_link;
}

void rtnl_link_set_master(struct rtnl_link *link, int ifindex)
{
	link->l_master = ifindex;
	link->ce_mask |= LINK_ATTR_MASTER;
}

int rtnl_link_get_master(struct rtnl_link *link)
{
	return link->l_master;
}

void rtnl_link_set_operstate(struct rtnl_link *link, uint8_t operstate)
{
	link->l_operstate = operstate;
	link->ce_mask |= LINK_ATTR_OPERSTATE;
}

uint8_t rtnl_link_get_operstate(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_OPERSTATE)
		return link->l_operstate;
	else
		return IF_OPER_UNKNOWN;
}

void rtnl_link_set_linkmode(struct rtnl_link *link, uint8_t linkmode)
{
	link->l_linkmode = linkmode;
	link->ce_mask |= LINK_ATTR_LINKMODE;
}

uint8_t rtnl_link_get_linkmode(struct rtnl_link *link)
{
	if (link->ce_mask & LINK_ATTR_LINKMODE)
		return link->l_linkmode;
	else
		return IF_LINK_MODE_DEFAULT;
}

uint64_t rtnl_link_get_stat(struct rtnl_link *link, int id)
{
	if (id < 0 || id > RTNL_LINK_STATS_MAX)
		return 0;

	return link->l_stats[id];
}

/**
 * Specify the info type of a link
 * @arg link	link object
 * @arg type	info type
 *
 * Looks up the info type and prepares the link to store info type
 * specific attributes. If an info type has been assigned already
 * it will be released with all changes lost.
 *
 * @return 0 on success or a negative errror code.
 */
int rtnl_link_set_info_type(struct rtnl_link *link, const char *type)
{
	struct rtnl_link_info_ops *io;
	int err;

	if ((io = rtnl_link_info_ops_lookup(type)) == NULL)
		return -NLE_OPNOTSUPP;

	if (link->l_info_ops)
		release_link_info(link);

	if ((err = io->io_alloc(link)) < 0)
		return err;

	link->l_info_ops = io;

	return 0;
}

/**
 * Return info type of a link
 * @arg link	link object
 *
 * @note The returned pointer is only valid as long as the link exists
 * @return Info type name or NULL if unknown.
 */
char *rtnl_link_get_info_type(struct rtnl_link *link)
{
	if (link->l_info_ops)
		return link->l_info_ops->io_name;
	else
		return NULL;
}

/** @} */

static struct nl_object_ops link_obj_ops = {
	.oo_name		= "route/link",
	.oo_size		= sizeof(struct rtnl_link),
	.oo_free_data		= link_free_data,
	.oo_clone		= link_clone,
	.oo_dump = {
	    [NL_DUMP_LINE]	= link_dump_line,
	    [NL_DUMP_DETAILS]	= link_dump_details,
	    [NL_DUMP_STATS]	= link_dump_stats,
	    [NL_DUMP_ENV]	= link_dump_env,
	},
	.oo_compare		= link_compare,
	.oo_attrs2str		= link_attrs2str,
	.oo_id_attrs		= LINK_ATTR_IFINDEX,
};

static struct nl_af_group link_groups[] = {
	{ AF_UNSPEC,	RTNLGRP_LINK },
	{ END_OF_GROUP_LIST },
};

static struct nl_cache_ops rtnl_link_ops = {
	.co_name		= "route/link",
	.co_hdrsize		= sizeof(struct ifinfomsg),
	.co_msgtypes		= {
					{ RTM_NEWLINK, NL_ACT_NEW, "new" },
					{ RTM_DELLINK, NL_ACT_DEL, "del" },
					{ RTM_GETLINK, NL_ACT_GET, "get" },
					END_OF_MSGTYPES_LIST,
				  },
	.co_protocol		= NETLINK_ROUTE,
	.co_groups		= link_groups,
	.co_request_update	= link_request_update,
	.co_msg_parser		= link_msg_parser,
	.co_obj_ops		= &link_obj_ops,
};

static void __init link_init(void)
{
	nl_cache_mngt_register(&rtnl_link_ops);
}

static void __exit link_exit(void)
{
	nl_cache_mngt_unregister(&rtnl_link_ops);
}

/** @} */
