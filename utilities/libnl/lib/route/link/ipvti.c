/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2014 Susant Sahani <susant@redhat.com>
 */

/**
 * @ingroup link
 * @defgroup ipvti IPVTI
 * ipvti link module
 *
 * @details
 * \b Link Type Name: "ipvti"
 *
 * @route_doc{link_ipvti, IPVTI Documentation}
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
#include <netlink/route/link/ipvti.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-aux-route/nl-route.h"

#define IPVTI_ATTR_LINK		 (1 << 0)
#define IPVTI_ATTR_IKEY		 (1 << 1)
#define IPVTI_ATTR_OKEY		 (1 << 2)
#define IPVTI_ATTR_LOCAL	 (1 << 3)
#define IPVTI_ATTR_REMOTE	 (1 << 4)
#define IPVTI_ATTR_FWMARK	 (1 << 5)

struct ipvti_info
{
	uint32_t   link;
	uint32_t   ikey;
	uint32_t   okey;
	uint32_t   local;
	uint32_t   remote;
	uint32_t   fwmark;
	uint32_t   ipvti_mask;
};

static	struct nla_policy ipvti_policy[IFLA_VTI_MAX + 1] = {
	[IFLA_VTI_LINK]     = { .type = NLA_U32 },
	[IFLA_VTI_IKEY]     = { .type = NLA_U32 },
	[IFLA_VTI_OKEY]     = { .type = NLA_U32 },
	[IFLA_VTI_LOCAL]    = { .type = NLA_U32 },
	[IFLA_VTI_REMOTE]   = { .type = NLA_U32 },
	[IFLA_VTI_FWMARK]   = { .type = NLA_U32 },
};

static int ipvti_alloc(struct rtnl_link *link)
{
	struct ipvti_info *ipvti;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*ipvti));
	else {
		ipvti = calloc(1, sizeof(*ipvti));
		if (!ipvti)
			return -NLE_NOMEM;

		link->l_info = ipvti;
	}

	return 0;
}

static int ipvti_parse(struct rtnl_link *link, struct nlattr *data,
		       struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_VTI_MAX + 1];
	struct ipvti_info *ipvti;
	int err;

	NL_DBG(3, "Parsing IPVTI link info\n");

	err = nla_parse_nested(tb, IFLA_VTI_MAX, data, ipvti_policy);
	if (err < 0)
		goto errout;

	err = ipvti_alloc(link);
	if (err < 0)
		goto errout;

	ipvti = link->l_info;

	if (tb[IFLA_VTI_LINK]) {
		ipvti->link = nla_get_u32(tb[IFLA_VTI_LINK]);
		ipvti->ipvti_mask |= IPVTI_ATTR_LINK;
	}

	if (tb[IFLA_VTI_IKEY]) {
		ipvti->ikey = nla_get_u32(tb[IFLA_VTI_IKEY]);
		ipvti->ipvti_mask |= IPVTI_ATTR_IKEY;
	}

	if (tb[IFLA_VTI_OKEY]) {
		ipvti->okey = nla_get_u32(tb[IFLA_VTI_OKEY]);
		ipvti->ipvti_mask |= IPVTI_ATTR_OKEY;
	}

	if (tb[IFLA_VTI_LOCAL]) {
		ipvti->local = nla_get_u32(tb[IFLA_VTI_LOCAL]);
		ipvti->ipvti_mask |= IPVTI_ATTR_LOCAL;
	}

	if (tb[IFLA_VTI_REMOTE]) {
		ipvti->remote = nla_get_u32(tb[IFLA_VTI_REMOTE]);
		ipvti->ipvti_mask |= IPVTI_ATTR_REMOTE;
	}

	if (tb[IFLA_VTI_FWMARK]) {
		ipvti->fwmark = nla_get_u32(tb[IFLA_VTI_FWMARK]);
		ipvti->ipvti_mask |= IPVTI_ATTR_FWMARK;
	}

	err = 0;

errout:
	return err;
}

static int ipvti_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct ipvti_info *ipvti = link->l_info;
	struct nlattr *data;

	data = nla_nest_start(msg, IFLA_INFO_DATA);
	if (!data)
		return -NLE_MSGSIZE;

	if (ipvti->ipvti_mask & IPVTI_ATTR_LINK)
		NLA_PUT_U32(msg, IFLA_VTI_LINK, ipvti->link);

	if (ipvti->ipvti_mask & IPVTI_ATTR_IKEY)
		NLA_PUT_U32(msg, IFLA_VTI_IKEY, ipvti->ikey);

	if (ipvti->ipvti_mask & IPVTI_ATTR_OKEY)
		NLA_PUT_U32(msg, IFLA_VTI_OKEY, ipvti->okey);

	if (ipvti->ipvti_mask & IPVTI_ATTR_LOCAL)
		NLA_PUT_U32(msg, IFLA_VTI_LOCAL, ipvti->local);

	if (ipvti->ipvti_mask & IPVTI_ATTR_REMOTE)
		NLA_PUT_U32(msg, IFLA_VTI_REMOTE, ipvti->remote);

	if (ipvti->ipvti_mask & IPVTI_ATTR_FWMARK)
		NLA_PUT_U32(msg, IFLA_VTI_FWMARK, ipvti->fwmark);

	nla_nest_end(msg, data);

nla_put_failure:

	return 0;
}

static void ipvti_free(struct rtnl_link *link)
{
	struct ipvti_info *ipvti = link->l_info;

	free(ipvti);
	link->l_info = NULL;
}

static void ipvti_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
	nl_dump(p, "ipvti : %s", link->l_name);
}

static void ipvti_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct ipvti_info *ipvti = link->l_info;
	char addr[INET_ADDRSTRLEN];

	if (ipvti->ipvti_mask & IPVTI_ATTR_LINK) {
		_nl_auto_rtnl_link struct rtnl_link *parent = NULL;
		char *name;

		nl_dump(p, "      link ");

		name = NULL;
		parent = link_lookup(link->ce_cache, ipvti->link);
		if (parent)
			name = rtnl_link_get_name(parent);

		if (name)
			nl_dump_line(p, "%s\n", name);
		else
			nl_dump_line(p, "%u\n", ipvti->link);
	}

	if (ipvti->ipvti_mask & IPVTI_ATTR_IKEY) {
		nl_dump(p, "      ikey   ");
		nl_dump_line(p, "%x\n",ipvti->ikey);
	}

	if (ipvti->ipvti_mask & IPVTI_ATTR_OKEY) {
		nl_dump(p, "      okey ");
		nl_dump_line(p, "%x\n", ipvti->okey);
	}

	if (ipvti->ipvti_mask & IPVTI_ATTR_LOCAL) {
		nl_dump(p, "      local ");
		_nl_inet_ntop4(ipvti->local, addr);
		nl_dump_line(p, "%s\n", addr);
	}

	if (ipvti->ipvti_mask & IPVTI_ATTR_REMOTE) {
		nl_dump(p, "      remote ");
		_nl_inet_ntop4(ipvti->remote, addr);
		nl_dump_line(p, "%s\n", addr);
	}

	if (ipvti->ipvti_mask & IPVTI_ATTR_FWMARK) {
		nl_dump(p, "      fwmark ");
		nl_dump_line(p, "%x\n", ipvti->fwmark);
	}
}

static int ipvti_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct ipvti_info *ipvti_dst, *ipvti_src = src->l_info;
	int err;

	dst->l_info = NULL;

	err = rtnl_link_set_type(dst, "vti");
	if (err < 0)
		return err;

	ipvti_dst = dst->l_info;

	if (!ipvti_dst || !ipvti_src)
		BUG();

	memcpy(ipvti_dst, ipvti_src, sizeof(struct ipvti_info));

	return 0;
}

static struct rtnl_link_info_ops ipvti_info_ops = {
	.io_name                = "vti",
	.io_alloc               = ipvti_alloc,
	.io_parse               = ipvti_parse,
	.io_dump = {
		[NL_DUMP_LINE]  = ipvti_dump_line,
		[NL_DUMP_DETAILS] = ipvti_dump_details,
	},
	.io_clone               = ipvti_clone,
	.io_put_attrs           = ipvti_put_attrs,
	.io_free                = ipvti_free,
};

#define IS_IPVTI_LINK_ASSERT(link)                                          \
        if ((link)->l_info_ops != &ipvti_info_ops) {                        \
                APPBUG("Link is not a ipvti link. set type \"vti\" first.");\
                return -NLE_OPNOTSUPP;                                      \
        }

struct rtnl_link *rtnl_link_ipvti_alloc(void)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_alloc();
	if (!link)
		return NULL;

	err = rtnl_link_set_type(link, "vti");
	if (err < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a IPVTI link
 * @arg link            Link object
 *
 * @return True if link is a IPVTI link, otherwise 0 is returned.
 */
int rtnl_link_is_ipvti(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "vti");
}
/**
 * Create a new ipvti tunnel device
 * @arg sock            netlink socket
 * @arg name            name of the tunnel deviceL
 *
 * Creates a new ipvti tunnel device in the kernel
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvti_add(struct nl_sock *sk, const char *name)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_ipvti_alloc();
	if (!link)
		return -NLE_NOMEM;

	if(name)
		rtnl_link_set_name(link, name);

	err = rtnl_link_add(sk, link, NLM_F_CREATE);
	rtnl_link_put(link);

	return err;
}
/**
 * Set IPVTI tunnel interface index
 * @arg link            Link object
 * @arg index           interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvti_set_link(struct rtnl_link *link, uint32_t index)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	ipvti->link = index;
	ipvti->ipvti_mask |= IPVTI_ATTR_LINK;

	return 0;
}

/**
 * Get IPVTI tunnel interface index
 * @arg link            Link object
 *
 * @return interface index
 */
uint32_t rtnl_link_ipvti_get_link(struct rtnl_link *link)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	return ipvti->link;
}

/**
 * Set IPVTI tunnel set ikey
 * @arg link            Link object
 * @arg ikey            gre ikey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvti_set_ikey(struct rtnl_link *link, uint32_t ikey)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	ipvti->ikey = ikey;
	ipvti->ipvti_mask |= IPVTI_ATTR_IKEY;

	return 0;
}

/**
 * Get IPVTI tunnel ikey
 * @arg link            Link object
 *
 * @return ikey
 */
uint32_t rtnl_link_ipvti_get_ikey(struct rtnl_link *link)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	return ipvti->ikey;
}

/**
 * Set IPVTI tunnel set okey
 * @arg link            Link object
 * @arg okey            gre okey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvti_set_okey(struct rtnl_link *link, uint32_t okey)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	ipvti->okey = okey;
	ipvti->ipvti_mask |= IPVTI_ATTR_OKEY;

	return 0;
}

/**
 * Get IPVTI tunnel okey
 * @arg link            Link object
 *
 * @return okey value
 */
uint32_t rtnl_link_ipvti_get_okey(struct rtnl_link *link)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	return ipvti->okey;
}

/**
 * Set IPVTI tunnel local address
 * @arg link            Link object
 * @arg addr            local address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvti_set_local(struct rtnl_link *link, uint32_t addr)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	ipvti->local = addr;
	ipvti->ipvti_mask |= IPVTI_ATTR_LOCAL;

	return 0;
}

/**
 * Get IPVTI tunnel local address
 * @arg link            Link object
 *
 * @return local address
 */
uint32_t rtnl_link_ipvti_get_local(struct rtnl_link *link)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	return ipvti->local;
}

/**
 * Set IPVTI tunnel remote address
 * @arg link            Link object
 * @arg remote          remote address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvti_set_remote(struct rtnl_link *link, uint32_t remote)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	ipvti->remote = remote;
	ipvti->ipvti_mask |= IPVTI_ATTR_REMOTE;

	return 0;
}

/**
 * Get IPVTI tunnel remote address
 * @arg link            Link object
 *
 * @return remote address  on success or a negative error code
 */
uint32_t rtnl_link_ipvti_get_remote(struct rtnl_link *link)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	return ipvti->remote;
}

/**
 * Set IPVTI tunnel fwmark
 * @arg link            Link object
 * @arg fwmark          fwmark
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvti_set_fwmark(struct rtnl_link *link, uint32_t fwmark)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	ipvti->fwmark = fwmark;
	ipvti->ipvti_mask |= IPVTI_ATTR_FWMARK;

	return 0;
}

/**
 * Get IPVTI tunnel fwmark
 * @arg link            Link object
 * @arg fwmark          addr to fill in with the fwmark
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvti_get_fwmark(struct rtnl_link *link, uint32_t *fwmark)
{
	struct ipvti_info *ipvti = link->l_info;

	IS_IPVTI_LINK_ASSERT(link);

	if (!(ipvti->ipvti_mask & IPVTI_ATTR_FWMARK))
		return -NLE_NOATTR;

	*fwmark = ipvti->fwmark;

	return 0;
}

static void _nl_init ipvti_init(void)
{
	rtnl_link_register_info(&ipvti_info_ops);
}

static void _nl_exit ipvti_exit(void)
{
	rtnl_link_unregister_info(&ipvti_info_ops);
}
