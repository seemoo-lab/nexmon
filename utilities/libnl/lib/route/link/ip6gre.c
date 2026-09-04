/* SPDX-License-Identifier: LGPL-2.1-only */

/**
 * @ingroup link
 * @defgroup ip6gre IP6GRE
 * ip6gre link module
 *
 * @details
 * \b Link Type Name: "ip6gre"
 *
 * @route_doc{link_ip6gre, IP6GRE Documentation}
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
#include <netlink/route/link/ip6gre.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-aux-route/nl-route.h"

#define IP6GRE_ATTR_LINK          (1 << 0)
#define IP6GRE_ATTR_IFLAGS        (1 << 1)
#define IP6GRE_ATTR_OFLAGS        (1 << 2)
#define IP6GRE_ATTR_IKEY          (1 << 3)
#define IP6GRE_ATTR_OKEY          (1 << 4)
#define IP6GRE_ATTR_LOCAL         (1 << 5)
#define IP6GRE_ATTR_REMOTE        (1 << 6)
#define IP6GRE_ATTR_TTL           (1 << 7)
#define IP6GRE_ATTR_ENCAPLIMIT    (1 << 8)
#define IP6GRE_ATTR_FLOWINFO      (1 << 9)
#define IP6GRE_ATTR_FLAGS         (1 << 10)
#define IP6GRE_ATTR_FWMARK        (1 << 11)

struct ip6gre_info
{
	uint8_t             ttl;
	uint8_t             encaplimit;
	uint16_t            iflags;
	uint16_t            oflags;
	uint32_t            ikey;
	uint32_t            okey;
	uint32_t            link;
	uint32_t            flowinfo;
	uint32_t            flags;
	struct in6_addr     local;
	struct in6_addr     remote;
	uint32_t            fwmark;
	uint32_t            ip6gre_mask;
};

static  struct nla_policy ip6gre_policy[IFLA_GRE_MAX + 1] = {
	[IFLA_GRE_LINK]         = { .type = NLA_U32 },
	[IFLA_GRE_IFLAGS]       = { .type = NLA_U16 },
	[IFLA_GRE_OFLAGS]       = { .type = NLA_U16 },
	[IFLA_GRE_IKEY]         = { .type = NLA_U32 },
	[IFLA_GRE_OKEY]         = { .type = NLA_U32 },
	[IFLA_GRE_LOCAL]        = { .minlen = sizeof(struct in6_addr) },
	[IFLA_GRE_REMOTE]       = { .minlen = sizeof(struct in6_addr) },
	[IFLA_GRE_TTL]          = { .type = NLA_U8 },
	[IFLA_GRE_ENCAP_LIMIT]  = { .type = NLA_U8 },
	[IFLA_GRE_FLOWINFO]     = { .type = NLA_U32 },
	[IFLA_GRE_FLAGS]        = { .type = NLA_U32 },
	[IFLA_GRE_FWMARK]       = { .type = NLA_U32 },
};

static int ip6gre_alloc(struct rtnl_link *link)
{
	struct ip6gre_info *ip6gre;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*ip6gre));
	else {
		ip6gre = calloc(1, sizeof(*ip6gre));
		if (!ip6gre)
			return -NLE_NOMEM;

		link->l_info = ip6gre;
	}

	return 0;
}

static int ip6gre_parse(struct rtnl_link *link, struct nlattr *data,
		        struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_GRE_MAX + 1];
	struct ip6gre_info *ip6gre;
	int err;

	NL_DBG(3, "Parsing IP6GRE link info\n");

	err = nla_parse_nested(tb, IFLA_GRE_MAX, data, ip6gre_policy);
	if (err < 0)
		goto errout;

	err = ip6gre_alloc(link);
	if (err < 0)
		goto errout;

	ip6gre = link->l_info;

	if (tb[IFLA_GRE_LINK]) {
		ip6gre->link = nla_get_u32(tb[IFLA_GRE_LINK]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_LINK;
	}

	if (tb[IFLA_GRE_IFLAGS]) {
		ip6gre->iflags = nla_get_u16(tb[IFLA_GRE_IFLAGS]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_IFLAGS;
	}

	if (tb[IFLA_GRE_OFLAGS]) {
		ip6gre->oflags = nla_get_u16(tb[IFLA_GRE_OFLAGS]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_OFLAGS;
	}

	if (tb[IFLA_GRE_IKEY]) {
		ip6gre->ikey = nla_get_u32(tb[IFLA_GRE_IKEY]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_IKEY;
	}

	if (tb[IFLA_GRE_OKEY]) {
		ip6gre->okey = nla_get_u32(tb[IFLA_GRE_OKEY]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_OKEY;
	}

	if (tb[IFLA_GRE_LOCAL]) {
		nla_memcpy(&ip6gre->local, tb[IFLA_GRE_LOCAL], sizeof(struct in6_addr));
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_LOCAL;
	}

	if (tb[IFLA_GRE_REMOTE]) {
		nla_memcpy(&ip6gre->remote, tb[IFLA_GRE_REMOTE], sizeof(struct in6_addr));
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_REMOTE;
	}

	if (tb[IFLA_GRE_TTL]) {
		ip6gre->ttl = nla_get_u8(tb[IFLA_GRE_TTL]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_TTL;
	}

	if (tb[IFLA_GRE_ENCAP_LIMIT]) {
		ip6gre->encaplimit = nla_get_u8(tb[IFLA_GRE_ENCAP_LIMIT]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_ENCAPLIMIT;
	}

	if (tb[IFLA_GRE_FLOWINFO]) {
		ip6gre->flowinfo = nla_get_u32(tb[IFLA_GRE_FLOWINFO]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_FLOWINFO;
	}

	if (tb[IFLA_GRE_FLAGS]) {
		ip6gre->flags = nla_get_u32(tb[IFLA_GRE_FLAGS]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_FLAGS;
	}

	if (tb[IFLA_GRE_FWMARK]) {
		ip6gre->fwmark = nla_get_u32(tb[IFLA_GRE_FWMARK]);
		ip6gre->ip6gre_mask |= IP6GRE_ATTR_FWMARK;
	}

	err = 0;

 errout:
	return err;
}

static int ip6gre_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct ip6gre_info *ip6gre = link->l_info;
	struct nlattr *data;

	data = nla_nest_start(msg, IFLA_INFO_DATA);
	if (!data)
		return -NLE_MSGSIZE;

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_LINK)
		NLA_PUT_U32(msg, IFLA_GRE_LINK, ip6gre->link);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_IFLAGS)
		NLA_PUT_U16(msg, IFLA_GRE_IFLAGS, ip6gre->iflags);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_OFLAGS)
		NLA_PUT_U16(msg, IFLA_GRE_OFLAGS, ip6gre->oflags);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_IKEY)
		NLA_PUT_U32(msg, IFLA_GRE_IKEY, ip6gre->ikey);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_OKEY)
		NLA_PUT_U32(msg, IFLA_GRE_OKEY, ip6gre->okey);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_LOCAL)
		NLA_PUT(msg, IFLA_GRE_LOCAL, sizeof(struct in6_addr), &ip6gre->local);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_REMOTE)
		NLA_PUT(msg, IFLA_GRE_REMOTE, sizeof(struct in6_addr), &ip6gre->remote);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_TTL)
		NLA_PUT_U8(msg, IFLA_GRE_TTL, ip6gre->ttl);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_ENCAPLIMIT)
		NLA_PUT_U8(msg, IFLA_GRE_ENCAP_LIMIT, ip6gre->encaplimit);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_FLOWINFO)
		NLA_PUT_U32(msg, IFLA_GRE_FLOWINFO, ip6gre->flowinfo);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_FLAGS)
		NLA_PUT_U32(msg, IFLA_GRE_FLAGS, ip6gre->flags);

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_FWMARK)
		NLA_PUT_U32(msg, IFLA_GRE_FWMARK, ip6gre->fwmark);

	nla_nest_end(msg, data);

 nla_put_failure:

	return 0;
}

static void ip6gre_free(struct rtnl_link *link)
{
	struct ip6gre_info *ip6gre = link->l_info;

	free(ip6gre);
	link->l_info = NULL;
}

static void ip6gre_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
	nl_dump(p, "ip6gre : %s", link->l_name);
}

static void ip6gre_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct ip6gre_info *ip6gre = link->l_info;
	char addr[INET6_ADDRSTRLEN];

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_LINK) {
		_nl_auto_rtnl_link struct rtnl_link *parent = NULL;
		char *name;

		nl_dump(p, "      link ");

		name = NULL;
		parent = link_lookup(link->ce_cache, ip6gre->link);
		if (parent)
			name = rtnl_link_get_name(parent);

		if (name)
			nl_dump_line(p, "%s\n", name);
		else
			nl_dump_line(p, "%u\n", ip6gre->link);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_IFLAGS) {
		nl_dump(p, "      iflags ");
		nl_dump_line(p, "%x\n", ip6gre->iflags);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_OFLAGS) {
		nl_dump(p, "      oflags ");
		nl_dump_line(p, "%x\n", ip6gre->oflags);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_IKEY) {
		nl_dump(p, "    ikey   ");
		nl_dump_line(p, "%x\n",ip6gre->ikey);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_OKEY) {
		nl_dump(p, "      okey ");
		nl_dump_line(p, "%x\n", ip6gre->okey);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_LOCAL) {
		nl_dump(p, "      local ");
		nl_dump_line(p, "%s\n",
			     _nl_inet_ntop(AF_INET6, &ip6gre->local, addr));
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_REMOTE) {
		nl_dump(p, "      remote ");
		nl_dump_line(p, "%s\n",
			     _nl_inet_ntop(AF_INET6, &ip6gre->remote, addr));
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_TTL) {
		nl_dump(p, "      ttl ");
		nl_dump_line(p, "%u\n", ip6gre->ttl);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_ENCAPLIMIT) {
		nl_dump(p, "      encaplimit ");
		nl_dump_line(p, "%u\n", ip6gre->encaplimit);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_FLOWINFO) {
		nl_dump(p, "      flowinfo ");
		nl_dump_line(p, "%x\n", ip6gre->flowinfo);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_FLAGS) {
		nl_dump(p, "      flags ");
		nl_dump_line(p, "%x\n", ip6gre->flags);
	}

	if (ip6gre->ip6gre_mask & IP6GRE_ATTR_FWMARK) {
		nl_dump(p, "    fwmark   ");
		nl_dump_line(p, "%x\n", ip6gre->fwmark);
	}
}

static int ip6gre_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct ip6gre_info *ip6gre_dst, *ip6gre_src = src->l_info;
	int err;

	dst->l_info = NULL;

	err = rtnl_link_set_type(dst, "ip6gre");
	if (err < 0)
		return err;

	ip6gre_dst = dst->l_info;

	if (!ip6gre_dst || !ip6gre_src)
		BUG();

	memcpy(ip6gre_dst, ip6gre_src, sizeof(struct ip6gre_info));

	return 0;
}

static struct rtnl_link_info_ops ip6gre_info_ops = {
	.io_name                = "ip6gre",
	.io_alloc               = ip6gre_alloc,
	.io_parse               = ip6gre_parse,
	.io_dump = {
		[NL_DUMP_LINE]  = ip6gre_dump_line,
		[NL_DUMP_DETAILS] = ip6gre_dump_details,
	},
	.io_clone               = ip6gre_clone,
	.io_put_attrs           = ip6gre_put_attrs,
	.io_free                = ip6gre_free,
};

#define IS_IP6GRE_LINK_ASSERT(link)                                             \
        if ((link)->l_info_ops != &ip6gre_info_ops) {                           \
                APPBUG("Link is not a ip6gre link. set type \"ip6gre\" first.");\
                return -NLE_OPNOTSUPP;                                          \
        }

#define HAS_IP6GRE_ATTR_ASSERT(link,attr)                                       \
        if (!((link)->ip6gre_mask & (attr)))                                    \
                return -NLE_NOATTR;

struct rtnl_link *rtnl_link_ip6gre_alloc(void)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_alloc();
	if (!link)
		return NULL;

	err = rtnl_link_set_type(link, "ip6gre");
	if (err < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a IP6GRE link
 * @arg link            Link object
 *
 * @return True if link is a IP6GRE link, otherwise 0 is returned.
 */
int rtnl_link_is_ip6gre(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "ip6gre");
}

/**
 * Create a new IP6GRE tunnel device
 * @arg sock            netlink socket
 * @arg name            name of the tunnel deviceL
 *
 * Creates a new ip6gre tunnel device in the kernel
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_add(struct nl_sock *sk, const char *name)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_ip6gre_alloc();
	if (!link)
		return -NLE_NOMEM;

	if(name)
		rtnl_link_set_name(link, name);

	err = rtnl_link_add(sk, link, NLM_F_CREATE);
	rtnl_link_put(link);

	return err;
}

/**
 * Set IP6GRE tunnel interface index
 * @arg link            Link object
 * @arg index           interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_link(struct rtnl_link *link, uint32_t index)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->link = index;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_LINK;

	return 0;
}

/**
 * Get IP6GRE tunnel interface index
 * @arg link            Link object
 * @arg index           addr to fill in with the interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_link(struct rtnl_link *link, uint32_t *index)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_LINK);

	*index = ip6gre->link;

	return 0;
}

/**
 * Set IP6GRE tunnel set iflags
 * @arg link            Link object
 * @arg iflags          ip6gre iflags
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_iflags(struct rtnl_link *link, uint16_t iflags)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->iflags = iflags;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_IFLAGS;

	return 0;
}

/**
 * Get IP6GRE tunnel iflags
 * @arg link            Link object
 * @arg iflags          addr to fill in with the iflags
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_iflags(struct rtnl_link *link, uint16_t *iflags)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_IFLAGS);

	*iflags = ip6gre->iflags;

	return 0;
}

/**
 * Set IP6GRE tunnel set oflags
 * @arg link            Link object
 * @arg oflags          ip6gre oflags
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_oflags(struct rtnl_link *link, uint16_t oflags)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->oflags = oflags;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_OFLAGS;

	return 0;
}

/**
 * Get IP6GRE tunnel oflags
 * @arg link            Link object
 * @arg oflags          addr to fill in with the oflags
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_oflags(struct rtnl_link *link, uint16_t *oflags)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_OFLAGS);

	*oflags = ip6gre->oflags;

	return 0;
}

/**
 * Set IP6GRE tunnel set ikey
 * @arg link            Link object
 * @arg ikey            ip6gre ikey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_ikey(struct rtnl_link *link, uint32_t ikey)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->ikey = ikey;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_IKEY;

	return 0;
}

/**
 * Get IP6GRE tunnel ikey
 * @arg link            Link object
 * @arg ikey            addr to fill in with the ikey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_ikey(struct rtnl_link *link, uint32_t *ikey)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_IKEY);

	*ikey = ip6gre->ikey;

	return 0;
}

/**
 * Set IP6GRE tunnel set okey
 * @arg link            Link object
 * @arg okey            ip6gre okey
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_okey(struct rtnl_link *link, uint32_t okey)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->okey = okey;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_OKEY;

	return 0;
}

/**
 * Get IP6GRE tunnel okey
 * @arg link            Link object
 * @arg okey            addr to fill in with the okey
 *
 * @return okey value
 */
int rtnl_link_ip6gre_get_okey(struct rtnl_link *link, uint32_t *okey)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_OKEY);

	*okey = ip6gre->okey;

	return 0;
}

/**
 * Set IP6GRE tunnel local address
 * @arg link            Link object
 * @arg local           local address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_local(struct rtnl_link *link, struct in6_addr *local)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	memcpy(&ip6gre->local, local, sizeof(struct in6_addr));
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_LOCAL;

	return 0;
}

/**
 * Get IP6GRE tunnel local address
 * @arg link            Link object
 * @arg local           addr to fill in with local address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_local(struct rtnl_link *link, struct in6_addr *local)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_LOCAL);

	memcpy(local, &ip6gre->local, sizeof(struct in6_addr));

	return 0;
}

/**
 * Set IP6GRE tunnel remote address
 * @arg link            Link object
 * @arg remote          remote address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_remote(struct rtnl_link *link, struct in6_addr *remote)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	memcpy(&ip6gre->remote, remote, sizeof(struct in6_addr));
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_REMOTE;

	return 0;
}

/**
 * Get IP6GRE tunnel remote address
 * @arg link            Link object
 * @arg remote          addr to fill in with remote address
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_remote(struct rtnl_link *link, struct in6_addr *remote)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_REMOTE);

	memcpy(remote, &ip6gre->remote, sizeof(struct in6_addr));

	return 0;
}

/**
 * Set IP6GRE tunnel ttl
 * @arg link            Link object
 * @arg ttl             tunnel ttl
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_ttl(struct rtnl_link *link, uint8_t ttl)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->ttl = ttl;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_TTL;

	return 0;
}

/**
 * Set IP6GRE tunnel ttl
 * @arg link            Link object
 * @arg ttl             addr to fill in with the ttl
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_ttl(struct rtnl_link *link, uint8_t *ttl)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_TTL);

	*ttl = ip6gre->ttl;

	return 0;
}

/**
 * Set IP6GRE tunnel encap limit
 * @arg link            Link object
 * @arg encaplimit      tunnel encap limit value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_encaplimit(struct rtnl_link *link, uint8_t encaplimit)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->encaplimit = encaplimit;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_ENCAPLIMIT;

	return 0;
}

/**
 * Get IP6GRE tunnel encap limit
 * @arg link            Link object
 * @arg encaplimit      addr to fill in with the encaplimit
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_encaplimit(struct rtnl_link *link, uint8_t *encaplimit)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_ENCAPLIMIT);

	*encaplimit = ip6gre->encaplimit;

	return 0;
}

/**
 * Set IP6GRE tunnel flowinfo
 * @arg link            Link object
 * @arg flowinfo        flowinfo value
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_flowinfo(struct rtnl_link *link, uint32_t flowinfo)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->flowinfo = flowinfo;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_FLOWINFO;

	return 0;
}

/**
 * Get IP6GRE flowinfo
 * @arg link            Link object
 * @arg flowinfo        addr to fill in with the flowinfo
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_flowinfo(struct rtnl_link *link, uint32_t *flowinfo)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_FLOWINFO);

	*flowinfo = ip6gre->flowinfo;

	return 0;
}

/**
 * Set IP6GRE tunnel flags
 * @arg link            Link object
 * @arg flags           tunnel flags
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_flags(struct rtnl_link *link, uint32_t flags)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->flags = flags;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_FLAGS;

	return 0;
}

/**
 * Get IP6GRE flags
 * @arg link            Link object
 * @arg flags           addr to fill in with the tunnel flags
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_flags(struct rtnl_link *link, uint32_t *flags)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_FLAGS);

	*flags = ip6gre->flags;

	return 0;
}

/**
 * Set IP6GRE tunnel fwmark
 * @arg link            Link object
 * @arg fwmark          fwmark
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_set_fwmark(struct rtnl_link *link, uint32_t fwmark)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	ip6gre->fwmark = fwmark;
	ip6gre->ip6gre_mask |= IP6GRE_ATTR_FWMARK;

	return 0;
}

/**
 * Get IP6GRE tunnel fwmark
 * @arg link            Link object
 * @arg fwmark          addr to fill in with the fwmark
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ip6gre_get_fwmark(struct rtnl_link *link, uint32_t *fwmark)
{
	struct ip6gre_info *ip6gre = link->l_info;

	IS_IP6GRE_LINK_ASSERT(link);

	HAS_IP6GRE_ATTR_ASSERT(ip6gre, IP6GRE_ATTR_FWMARK);

	*fwmark = ip6gre->fwmark;

	return 0;
}

static void _nl_init ip6gre_init(void)
{
	rtnl_link_register_info(&ip6gre_info_ops);
}

static void _nl_exit ip6gre_exit(void)
{
	rtnl_link_unregister_info(&ip6gre_info_ops);
}
