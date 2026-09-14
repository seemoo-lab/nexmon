/* SPDX-License-Identifier: LGPL-2.1-only */

/**
 * @ingroup link
 * @defgroup ip6vti IP6VTI
 * ip6vti link module
 *
 * @details
 * \b Link Type Name: "vti6"
 *
 * @route_doc{link_ip6vti, IP6VTI Documentation}
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
#include <netlink/route/link/ip6vti.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-aux-route/nl-route.h"

#define IP6VTI_ATTR_LINK      (1 << 0)
#define IP6VTI_ATTR_IKEY      (1 << 1)
#define IP6VTI_ATTR_OKEY      (1 << 2)
#define IP6VTI_ATTR_LOCAL     (1 << 3)
#define IP6VTI_ATTR_REMOTE    (1 << 4)
#define IP6VTI_ATTR_FWMARK    (1 << 5)

struct ip6vti_info
{
	uint32_t            link;
	uint32_t            ikey;
	uint32_t            okey;
	struct in6_addr     local;
	struct in6_addr     remote;
	uint32_t            fwmark;
	uint32_t            ip6vti_mask;
};

static  struct nla_policy ip6vti_policy[IFLA_VTI_MAX + 1] = {
	[IFLA_VTI_LINK]     = { .type = NLA_U32 },
	[IFLA_VTI_IKEY]     = { .type = NLA_U32 },
	[IFLA_VTI_OKEY]     = { .type = NLA_U32 },
	[IFLA_VTI_LOCAL]    = { .minlen = sizeof(struct in6_addr) },
	[IFLA_VTI_REMOTE]   = { .minlen = sizeof(struct in6_addr) },
	[IFLA_VTI_FWMARK]   = { .type = NLA_U32 },
};

static int ip6vti_alloc(struct rtnl_link *link)
{
	struct ip6vti_info *ip6vti;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*ip6vti));
	else {
		ip6vti = calloc(1, sizeof(*ip6vti));
		if (!ip6vti)
			return -NLE_NOMEM;

		link->l_info = ip6vti;
	}

	return 0;
}

static int ip6vti_parse(struct rtnl_link *link, struct nlattr *data,
		        struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_VTI_MAX + 1];
	struct ip6vti_info *ip6vti;
	int err;

	NL_DBG(3, "Parsing IP6VTI link info\n");

	err = nla_parse_nested(tb, IFLA_VTI_MAX, data, ip6vti_policy);
	if (err < 0)
		goto errout;

	err = ip6vti_alloc(link);
	if (err < 0)
		goto errout;

	ip6vti = link->l_info;

	if (tb[IFLA_VTI_LINK]) {
		ip6vti->link = nla_get_u32(tb[IFLA_VTI_LINK]);
		ip6vti->ip6vti_mask |= IP6VTI_ATTR_LINK;
	}

	if (tb[IFLA_VTI_IKEY]) {
		ip6vti->ikey = nla_get_u32(tb[IFLA_VTI_IKEY]);
		ip6vti->ip6vti_mask |= IP6VTI_ATTR_IKEY;
	}

	if (tb[IFLA_VTI_OKEY]) {
		ip6vti->okey = nla_get_u32(tb[IFLA_VTI_OKEY]);
		ip6vti->ip6vti_mask |= IP6VTI_ATTR_OKEY;
	}

	if (tb[IFLA_VTI_LOCAL]) {
		nla_memcpy(&ip6vti->local, tb[IFLA_VTI_LOCAL], sizeof(struct in6_addr));
		ip6vti->ip6vti_mask |= IP6VTI_ATTR_LOCAL;
	}

	if (tb[IFLA_VTI_REMOTE]) {
		nla_memcpy(&ip6vti->remote, tb[IFLA_VTI_REMOTE], sizeof(struct in6_addr));
		ip6vti->ip6vti_mask |= IP6VTI_ATTR_REMOTE;
	}

	if (tb[IFLA_VTI_FWMARK]) {
		ip6vti->fwmark = nla_get_u32(tb[IFLA_VTI_FWMARK]);
		ip6vti->ip6vti_mask |= IP6VTI_ATTR_FWMARK;
	}

	err = 0;

 errout:
	return err;
}

static int ip6vti_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct ip6vti_info *ip6vti = link->l_info;
	struct nlattr *data;

	data = nla_nest_start(msg, IFLA_INFO_DATA);
	if (!data)
		return -NLE_MSGSIZE;

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_LINK)
		NLA_PUT_U32(msg, IFLA_VTI_LINK, ip6vti->link);

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_IKEY)
		NLA_PUT_U32(msg, IFLA_VTI_IKEY, ip6vti->ikey);

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_OKEY)
		NLA_PUT_U32(msg, IFLA_VTI_OKEY, ip6vti->okey);

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_LOCAL)
		NLA_PUT(msg, IFLA_VTI_LOCAL, sizeof(struct in6_addr), &ip6vti->local);

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_REMOTE)
		NLA_PUT(msg, IFLA_VTI_REMOTE, sizeof(struct in6_addr), &ip6vti->remote);

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_FWMARK)
		NLA_PUT_U32(msg, IFLA_VTI_FWMARK, ip6vti->fwmark);

	nla_nest_end(msg, data);

nla_put_failure:

	return 0;
}

static void ip6vti_free(struct rtnl_link *link)
{
	struct ip6vti_info *ip6vti = link->l_info;

	free(ip6vti);
	link->l_info = NULL;
}

static void ip6vti_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
	nl_dump(p, "ip6vti : %s", link->l_name);
}

static void ip6vti_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct ip6vti_info *ip6vti = link->l_info;
	char addr[INET6_ADDRSTRLEN];

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_LINK) {
		_nl_auto_rtnl_link struct rtnl_link *parent = NULL;
		char *name;

		nl_dump(p, "      link ");

		name = NULL;
		parent = link_lookup(link->ce_cache, ip6vti->link);
		if (parent)
			name = rtnl_link_get_name(parent);

		if (name)
			nl_dump_line(p, "%s\n", name);
		else
			nl_dump_line(p, "%u\n", ip6vti->link);
	}

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_IKEY) {
		nl_dump(p, "      ikey   ");
		nl_dump_line(p, "%x\n",ip6vti->ikey);
	}

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_OKEY) {
		nl_dump(p, "      okey ");
		nl_dump_line(p, "%x\n", ip6vti->okey);
	}

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_LOCAL) {
		nl_dump(p, "      local ");
		nl_dump_line(p, "%s\n",
			     _nl_inet_ntop(AF_INET6, &ip6vti->local, addr));
	}

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_REMOTE) {
		nl_dump(p, "      remote ");
		nl_dump_line(p, "%s\n",
			     _nl_inet_ntop(AF_INET6, &ip6vti->remote, addr));
	}

	if (ip6vti->ip6vti_mask & IP6VTI_ATTR_FWMARK) {
		nl_dump(p, "      fwmark ");
		nl_dump_line(p, "%x\n", ip6vti->fwmark);
	}
}

static int ip6vti_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct ip6vti_info *ip6vti_dst, *ip6vti_src = src->l_info;
	int err;

	dst->l_info = NULL;

	err = rtnl_link_set_type(dst, "vti6");
	if (err < 0)
		return err;

	ip6vti_dst = dst->l_info;

	if (!ip6vti_dst || !ip6vti_src)
		BUG();

	memcpy(ip6vti_dst, ip6vti_src, sizeof(struct ip6vti_info));

	return 0;
}

static struct rtnl_link_info_ops ip6vti_info_ops = {
	.io_name                = "vti6",
	.io_alloc               = ip6vti_alloc,
	.io_parse               = ip6vti_parse,
	.io_dump = {
		[NL_DUMP_LINE]  = ip6vti_dump_line,
		[NL_DUMP_DETAILS] = ip6vti_dump_details,
	},
	.io_clone               = ip6vti_clone,
	.io_put_attrs           = ip6vti_put_attrs,
	.io_free                = ip6vti_free,
};

#define IS_IP6VTI_LINK_ASSERT(link)                                            \
        if ((link)->l_info_ops != &ip6vti_info_ops) {                          \
                APPBUG("Link is not a ip6vti link. set type \"vti6\" first."); \
                return -NLE_OPNOTSUPP;                                         \
        }

#define HAS_IP6VTI_ATTR_ASSERT(ip6vti,attr)                                    \
        if (!((ip6vti)->ip6vti_mask & (attr)))                                 \
                return -NLE_NOATTR;

struct rtnl_link *rtnl_link_ip6vti_alloc(void)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_alloc();
	if (!link)
		return NULL;

	err = rtnl_link_set_type(link, "vti6");
	if (err < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a IP6VTI link
 * @arg link            Link object
 *
 * @return True if link is a IP6VTI link, otherwise 0 is returned.
 */
int rtnl_link_is_ip6vti(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "vti6");
}
/**
 * Create a new vti6 tunnel device
 * @arg sock            netlink socket
 * @arg name            name of the tunnel deviceL
 *
 * Creates a new vti6 tunnel device in the kernel
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_add(struct nl_sock *sk, const char *name)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_ip6vti_alloc();
	if (!link)
		return -NLE_NOMEM;

	if(name)
		rtnl_link_set_name(link, name);

	err = rtnl_link_add(sk, link, NLM_F_CREATE);
	rtnl_link_put(link);

	return err;
}
/**
 * Set IP6VTI tunnel interface index
 * @arg link            Link object
 * @arg index           interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_set_link(struct rtnl_link *link, uint32_t index)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	ip6vti->link = index;
	ip6vti->ip6vti_mask |= IP6VTI_ATTR_LINK;

	return 0;
}

/**
 * Get IP6VTI tunnel interface index
 * @arg link            Link object
 * @arg index           addr to fill in with the interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_get_link(struct rtnl_link *link, uint32_t *index)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	HAS_IP6VTI_ATTR_ASSERT(ip6vti, IP6VTI_ATTR_LINK);

	*index = ip6vti->link;

	return 0;
}

/**
 * Set IP6VTI tunnel set ikey
 * @arg link            Link object
 * @arg ikey            gre ikey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_set_ikey(struct rtnl_link *link, uint32_t ikey)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	ip6vti->ikey = ikey;
	ip6vti->ip6vti_mask |= IP6VTI_ATTR_IKEY;

	return 0;
}

/**
 * Get IP6VTI tunnel ikey
 * @arg link            Link object
 * @arg ikey            addr to fill in with the ikey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_get_ikey(struct rtnl_link *link, uint32_t *ikey)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	HAS_IP6VTI_ATTR_ASSERT(ip6vti, IP6VTI_ATTR_IKEY);

	*ikey = ip6vti->ikey;

	return 0;
}

/**
 * Set IP6VTI tunnel set okey
 * @arg link            Link object
 * @arg okey            gre okey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_set_okey(struct rtnl_link *link, uint32_t okey)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	ip6vti->okey = okey;
	ip6vti->ip6vti_mask |= IP6VTI_ATTR_OKEY;

	return 0;
}

/**
 * Get IP6VTI tunnel okey
 * @arg link            Link object
 * @arg okey            addr to fill in with the okey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_get_okey(struct rtnl_link *link, uint32_t *okey)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	HAS_IP6VTI_ATTR_ASSERT(ip6vti, IP6VTI_ATTR_OKEY);

	*okey = ip6vti->okey;

	return 0;
}

/**
 * Set IP6VTI tunnel local address
 * @arg link            Link object
 * @arg local           local address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_set_local(struct rtnl_link *link, struct in6_addr *local)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	memcpy(&ip6vti->local, local, sizeof(struct in6_addr));
	ip6vti->ip6vti_mask |= IP6VTI_ATTR_LOCAL;

	return 0;
}

/**
 * Get IP6VTI tunnel local address
 * @arg link            Link object
 * @arg local           addr to fill in with remote address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_get_local(struct rtnl_link *link, struct in6_addr *local)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	HAS_IP6VTI_ATTR_ASSERT(ip6vti, IP6VTI_ATTR_LOCAL);

	memcpy(local, &ip6vti->local, sizeof(struct in6_addr));

	return 0;
}

/**
 * Set IP6VTI tunnel remote address
 * @arg link            Link object
 * @arg remote          remote address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_set_remote(struct rtnl_link *link, struct in6_addr *remote)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	memcpy(&ip6vti->remote, remote, sizeof(struct in6_addr));
	ip6vti->ip6vti_mask |= IP6VTI_ATTR_REMOTE;

	return 0;
}

/**
 * Get IP6VTI tunnel remote address
 * @arg link            Link object
 * @arg remote          addr to fill in with remote address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_get_remote(struct rtnl_link *link, struct in6_addr *remote)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	HAS_IP6VTI_ATTR_ASSERT(ip6vti, IP6VTI_ATTR_REMOTE);

	memcpy(remote, &ip6vti->remote, sizeof(struct in6_addr));

	return 0;
}

/**
 * Set IP6VTI tunnel fwmark
 * @arg link            Link object
 * @arg fwmark          fwmark
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_set_fwmark(struct rtnl_link *link, uint32_t fwmark)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	ip6vti->fwmark = fwmark;
	ip6vti->ip6vti_mask |= IP6VTI_ATTR_FWMARK;

	return 0;
}

/**
 * Get IP6VTI tunnel fwmark
 * @arg link            Link object
 * @arg fwmark          addr to fill in with the fwmark
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6vti_get_fwmark(struct rtnl_link *link, uint32_t *fwmark)
{
	struct ip6vti_info *ip6vti = link->l_info;

	IS_IP6VTI_LINK_ASSERT(link);

	HAS_IP6VTI_ATTR_ASSERT(ip6vti, IP6VTI_ATTR_FWMARK);

	*fwmark = ip6vti->fwmark;

	return 0;
}

static void _nl_init ip6vti_init(void)
{
	rtnl_link_register_info(&ip6vti_info_ops);
}

static void _nl_exit ip6vti_exit(void)
{
	rtnl_link_unregister_info(&ip6vti_info_ops);
}
