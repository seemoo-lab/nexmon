/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2014 Susant Sahani <susant@redhat.com>
 */

/**
 * @ingroup link
 * @defgroup ipip IPIP
 * ipip link module
 *
 * @details
 * \b Link Type Name: "ipip"
 *
 * @route_doc{link_ipip, IPIP Documentation}
 *
 * @{
 */

#include "nl-default.h"

#include <linux/if_tunnel.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/ipip.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-aux-route/nl-route.h"

#define IPIP_ATTR_LINK          (1 << 0)
#define IPIP_ATTR_LOCAL         (1 << 1)
#define IPIP_ATTR_REMOTE        (1 << 2)
#define IPIP_ATTR_TTL           (1 << 3)
#define IPIP_ATTR_TOS           (1 << 4)
#define IPIP_ATTR_PMTUDISC      (1 << 5)
#define IPIP_ATTR_FWMARK        (1 << 6)

struct ipip_info
{
	uint8_t    ttl;
	uint8_t    tos;
	uint8_t    pmtudisc;
	uint32_t   link;
	uint32_t   local;
	uint32_t   remote;
	uint32_t   fwmark;
	uint32_t   ipip_mask;
};

static struct nla_policy ipip_policy[IFLA_IPTUN_MAX + 1] = {
	[IFLA_IPTUN_LINK]       = { .type = NLA_U32 },
	[IFLA_IPTUN_LOCAL]      = { .type = NLA_U32 },
	[IFLA_IPTUN_REMOTE]     = { .type = NLA_U32 },
	[IFLA_IPTUN_TTL]        = { .type = NLA_U8 },
	[IFLA_IPTUN_TOS]        = { .type = NLA_U8 },
	[IFLA_IPTUN_PMTUDISC]   = { .type = NLA_U8 },
	[IFLA_IPTUN_FWMARK]     = { .type = NLA_U32 },
};

static int ipip_alloc(struct rtnl_link *link)
{
	struct ipip_info *ipip;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*ipip));
	else {
		ipip = calloc(1, sizeof(*ipip));
		if (!ipip)
			return -NLE_NOMEM;

		link->l_info = ipip;
	}

	return 0;
}

static int ipip_parse(struct rtnl_link *link, struct nlattr *data,
                      struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_IPTUN_MAX + 1];
	struct ipip_info *ipip;
	int err;

	NL_DBG(3, "Parsing IPIP link info\n");

	err = nla_parse_nested(tb, IFLA_IPTUN_MAX, data, ipip_policy);
	if (err < 0)
		goto errout;

	err = ipip_alloc(link);
	if (err < 0)
		goto errout;

	ipip = link->l_info;

	if (tb[IFLA_IPTUN_LINK]) {
		ipip->link = nla_get_u32(tb[IFLA_IPTUN_LINK]);
		ipip->ipip_mask |= IPIP_ATTR_LINK;
	}

	if (tb[IFLA_IPTUN_LOCAL]) {
		ipip->local = nla_get_u32(tb[IFLA_IPTUN_LOCAL]);
		ipip->ipip_mask |= IPIP_ATTR_LOCAL;
	}

	if (tb[IFLA_IPTUN_REMOTE]) {
		ipip->remote = nla_get_u32(tb[IFLA_IPTUN_REMOTE]);
		ipip->ipip_mask |= IPIP_ATTR_REMOTE;
	}

	if (tb[IFLA_IPTUN_TTL]) {
		ipip->ttl = nla_get_u8(tb[IFLA_IPTUN_TTL]);
		ipip->ipip_mask |= IPIP_ATTR_TTL;
	}

	if (tb[IFLA_IPTUN_TOS]) {
		ipip->tos = nla_get_u8(tb[IFLA_IPTUN_TOS]);
		ipip->ipip_mask |= IPIP_ATTR_TOS;
	}

	if (tb[IFLA_IPTUN_PMTUDISC]) {
		ipip->pmtudisc = nla_get_u8(tb[IFLA_IPTUN_PMTUDISC]);
		ipip->ipip_mask |= IPIP_ATTR_PMTUDISC;
	}

	if (tb[IFLA_IPTUN_FWMARK]) {
		ipip->fwmark = nla_get_u32(tb[IFLA_IPTUN_FWMARK]);
		ipip->ipip_mask |= IPIP_ATTR_FWMARK;
	}

	err = 0;

errout:
	return err;
}

static int ipip_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct ipip_info *ipip = link->l_info;
	struct nlattr *data;

	data = nla_nest_start(msg, IFLA_INFO_DATA);
	if (!data)
		return -NLE_MSGSIZE;

	if (ipip->ipip_mask & IPIP_ATTR_LINK)
		NLA_PUT_U32(msg, IFLA_IPTUN_LINK, ipip->link);

	if (ipip->ipip_mask & IPIP_ATTR_LOCAL)
		NLA_PUT_U32(msg, IFLA_IPTUN_LOCAL, ipip->local);

	if (ipip->ipip_mask & IPIP_ATTR_REMOTE)
		NLA_PUT_U32(msg, IFLA_IPTUN_REMOTE, ipip->remote);

	if (ipip->ipip_mask & IPIP_ATTR_TTL)
		NLA_PUT_U8(msg, IFLA_IPTUN_TTL, ipip->ttl);

	if (ipip->ipip_mask & IPIP_ATTR_TOS)
		NLA_PUT_U8(msg, IFLA_IPTUN_TOS, ipip->tos);

	if (ipip->ipip_mask & IPIP_ATTR_PMTUDISC)
		NLA_PUT_U8(msg, IFLA_IPTUN_PMTUDISC, ipip->pmtudisc);

	if (ipip->ipip_mask & IPIP_ATTR_FWMARK)
		NLA_PUT_U32(msg, IFLA_IPTUN_FWMARK, ipip->fwmark);

	nla_nest_end(msg, data);

nla_put_failure:
	return 0;
}

static void ipip_free(struct rtnl_link *link)
{
	struct ipip_info *ipip = link->l_info;

	free(ipip);
	link->l_info = NULL;
}

static void ipip_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
	nl_dump(p, "ipip : %s", link->l_name);
}

static void ipip_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct ipip_info *ipip = link->l_info;
	char addr[INET_ADDRSTRLEN];

	if (ipip->ipip_mask & IPIP_ATTR_LINK) {
		_nl_auto_rtnl_link struct rtnl_link *parent = NULL;
		char *name;

		nl_dump(p, "      link ");

		name = NULL;
		parent = link_lookup(link->ce_cache, ipip->link);
		if (parent)
			name = rtnl_link_get_name(parent);

		if (name)
			nl_dump_line(p, "%s\n", name);
		else
			nl_dump_line(p, "%u\n", ipip->link);
	}

	if (ipip->ipip_mask & IPIP_ATTR_LOCAL) {
		nl_dump(p, "      local ");
		_nl_inet_ntop4(ipip->local, addr);
		nl_dump_line(p, "%s\n", addr);
	}

	if (ipip->ipip_mask & IPIP_ATTR_REMOTE) {
		nl_dump(p, "      remote ");
		_nl_inet_ntop4(ipip->remote, addr);
		nl_dump_line(p, "%s\n", addr);
	}

	if (ipip->ipip_mask & IPIP_ATTR_TTL) {
		nl_dump(p, "      ttl ");
		nl_dump_line(p, "%u\n", ipip->ttl);
	}

	if (ipip->ipip_mask & IPIP_ATTR_TOS) {
		nl_dump(p, "      tos ");
		nl_dump_line(p, "%u\n", ipip->tos);
	}

	if (ipip->ipip_mask & IPIP_ATTR_PMTUDISC) {
		nl_dump(p, "      pmtudisc ");
		nl_dump_line(p, "enabled (%#x)\n", ipip->pmtudisc);
	}

	if (ipip->ipip_mask & IPIP_ATTR_FWMARK) {
		nl_dump(p, "      fwmark ");
		nl_dump_line(p, "%x\n", ipip->fwmark);
	}
}

static int ipip_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct ipip_info *ipip_dst, *ipip_src = src->l_info;
	int err;

	dst->l_info = NULL;

	err = rtnl_link_set_type(dst, "ipip");
	if (err < 0)
		return err;

	ipip_dst = dst->l_info;

	if (!ipip_dst || !ipip_src)
		BUG();

	memcpy(ipip_dst, ipip_src, sizeof(struct ipip_info));

	return 0;
}

static struct rtnl_link_info_ops ipip_info_ops = {
	.io_name                = "ipip",
	.io_alloc               = ipip_alloc,
	.io_parse               = ipip_parse,
	.io_dump = {
		[NL_DUMP_LINE]  = ipip_dump_line,
		[NL_DUMP_DETAILS] = ipip_dump_details,
	},
	.io_clone               = ipip_clone,
	.io_put_attrs           = ipip_put_attrs,
	.io_free                = ipip_free,
};

#define IS_IPIP_LINK_ASSERT(link)                                            \
        if ((link)->l_info_ops != &ipip_info_ops) {                          \
                APPBUG("Link is not a ipip link. set type \"ipip\" first."); \
                return -NLE_OPNOTSUPP;                                       \
        }

struct rtnl_link *rtnl_link_ipip_alloc(void)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_alloc();
	if (!link)
		return NULL;

	err = rtnl_link_set_type(link, "ipip");
	if (err < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a IPIP link
 * @arg link            Link object
 *
 * @return True if link is a IPIP link, otherwise false is returned.
 */
int rtnl_link_is_ipip(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "ipip");
}

/**
 * Create a new ipip tunnel device
 * @arg sock            netlink socket
 * @arg name            name of the tunnel deviceL
 *
 * Creates a new ipip tunnel device in the kernel
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_add(struct nl_sock *sk, const char *name)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_ipip_alloc();
	if (!link)
		return -NLE_NOMEM;

	if(name)
		rtnl_link_set_name(link, name);

	err = rtnl_link_add(sk, link, NLM_F_CREATE);
	rtnl_link_put(link);

	return err;
}

/**
 * Set IPIP tunnel interface index
 * @arg link            Link object
 * @arg index           interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_set_link(struct rtnl_link *link,  uint32_t index)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	ipip->link = index;
	ipip->ipip_mask |= IPIP_ATTR_LINK;

	return 0;
}

/**
 * Get IPIP tunnel interface index
 * @arg link            Link object
 *
 * @return interface index value
 */
uint32_t rtnl_link_ipip_get_link(struct rtnl_link *link)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	return ipip->link;
}

/**
 * Set IPIP tunnel local address
 * @arg link            Link object
 * @arg addr            local address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_set_local(struct rtnl_link *link, uint32_t addr)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	ipip->local = addr;
	ipip->ipip_mask |= IPIP_ATTR_LOCAL;

	return 0;
}

/**
 * Get IPIP tunnel local address
 * @arg link            Link object
 *
 * @return local address value
 */
uint32_t rtnl_link_ipip_get_local(struct rtnl_link *link)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	return ipip->local;
}

/**
 * Set IPIP tunnel remote address
 * @arg link            Link object
 * @arg remote          remote address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_set_remote(struct rtnl_link *link, uint32_t addr)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	ipip->remote = addr;
	ipip->ipip_mask |= IPIP_ATTR_REMOTE;

	return 0;
}

/**
 * Get IPIP tunnel remote address
 * @arg link            Link object
 *
 * @return remote address
 */
uint32_t rtnl_link_ipip_get_remote(struct rtnl_link *link)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	return ipip->remote;
}

/**
 * Set IPIP tunnel ttl
 * @arg link            Link object
 * @arg ttl             tunnel ttl
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_set_ttl(struct rtnl_link *link, uint8_t ttl)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	ipip->ttl = ttl;
	ipip->ipip_mask |= IPIP_ATTR_TTL;

	return 0;
}

/**
 * Get IPIP tunnel ttl
 * @arg link            Link object
 *
 * @return ttl value
 */
uint8_t rtnl_link_ipip_get_ttl(struct rtnl_link *link)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	return ipip->ttl;
}

/**
 * Set IPIP tunnel tos
 * @arg link            Link object
 * @arg tos             tunnel tos
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_set_tos(struct rtnl_link *link, uint8_t tos)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	ipip->tos = tos;
	ipip->ipip_mask |= IPIP_ATTR_TOS;

	return 0;
}

/**
 * Get IPIP tunnel tos
 * @arg link            Link object
 *
 * @return tos value
 */
uint8_t rtnl_link_ipip_get_tos(struct rtnl_link *link)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	return ipip->tos;
}

/**
 * Set IPIP tunnel path MTU discovery
 * @arg link            Link object
 * @arg pmtudisc        path MTU discovery
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_set_pmtudisc(struct rtnl_link *link, uint8_t pmtudisc)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	ipip->pmtudisc = pmtudisc;
	ipip->ipip_mask |= IPIP_ATTR_PMTUDISC;

	return 0;
}

/**
 * Get IPIP path MTU discovery
 * @arg link            Link object
 *
 * @return pmtudisc value
 */
uint8_t rtnl_link_ipip_get_pmtudisc(struct rtnl_link *link)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	return ipip->pmtudisc;
}

/**
 * Set IPIP tunnel fwmark
 * @arg link            Link object
 * @arg fwmark          fwmark
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_set_fwmark(struct rtnl_link *link, uint32_t fwmark)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	ipip->fwmark = fwmark;
	ipip->ipip_mask |= IPIP_ATTR_FWMARK;

	return 0;
}

/**
 * Get IPIP tunnel fwmark
 * @arg link            Link object
 * @arg fwmark          addr to fill in with the fwmark
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipip_get_fwmark(struct rtnl_link *link, uint32_t *fwmark)
{
	struct ipip_info *ipip = link->l_info;

	IS_IPIP_LINK_ASSERT(link);

	if (!(ipip->ipip_mask & IPIP_ATTR_FWMARK))
		return -NLE_NOATTR;

	*fwmark = ipip->fwmark;

	return 0;
}

static void _nl_init ipip_init(void)
{
	rtnl_link_register_info(&ipip_info_ops);
}

static void _nl_exit ipip_exit(void)
{
	rtnl_link_unregister_info(&ipip_info_ops);
}
