/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2011-2013 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup link
 * @defgroup bonding Bonding
 *
 * @details
 * \b Link Type Name: "bond"
 *
 * @route_doc{link_bonding, Bonding Documentation}
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/route/link/bonding.h>

#include "nl-route.h"
#include "link-api.h"

#define BOND_HAS_MODE		(1 << 0)
#define BOND_HAS_ACTIVE_SLAVE	(1 << 1)
#define BOND_HAS_HASHING_TYPE	(1 << 2)
#define BOND_HAS_MIIMON		(1 << 3)
#define BOND_HAS_MIN_LINKS	(1 << 4)

struct bond_info {
	uint32_t ce_mask; /* to support attr macros */
	uint8_t bn_mode;
	uint8_t hashing_type;
	uint32_t ifindex;
	uint32_t miimon;
	uint32_t min_links;
};

static int bond_info_alloc(struct rtnl_link *link)
{
	struct bond_info *bn;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*bn));
	else {
		bn = calloc(1, sizeof(*bn));
		if (!bn)
			return -NLE_NOMEM;

		link->l_info = bn;
	}

	return 0;
}

static struct nla_policy bonding_nl_policy[IFLA_BOND_MAX + 1] = {
	[IFLA_BOND_MODE]             = { .type = NLA_U8 },
	[IFLA_BOND_ACTIVE_SLAVE]     = { .type = NLA_U32 },
	[IFLA_BOND_MIIMON]	     = { .type = NLA_U32 },
	[IFLA_BOND_XMIT_HASH_POLICY] = { .type = NLA_U8 },
	[IFLA_BOND_MIN_LINKS]	     = { .type = NLA_U32 },
};

static int bond_info_parse(struct rtnl_link *link, struct nlattr *data,
			   struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_BOND_MAX + 1];
	struct bond_info *bn;
	int err;

	err = nla_parse_nested(tb, IFLA_BOND_MAX, data, bonding_nl_policy);
	if (err < 0)
		return err;

	err = bond_info_alloc(link);
	if (err < 0)
		return err;

	bn = link->l_info;

	if (tb[IFLA_BOND_MODE]) {
		bn->bn_mode = nla_get_u8(tb[IFLA_BOND_MODE]);
		bn->ce_mask |= BOND_HAS_MODE;
	}

	if (tb[IFLA_BOND_ACTIVE_SLAVE]) {
		bn->ifindex = nla_get_u32(tb[IFLA_BOND_ACTIVE_SLAVE]);
		bn->ce_mask |= BOND_HAS_ACTIVE_SLAVE;
	}

	if (tb[IFLA_BOND_MIIMON]) {
		bn->miimon = nla_get_u32(tb[IFLA_BOND_MIIMON]);
		bn->ce_mask |= BOND_HAS_MIIMON;
	}

	if (tb[IFLA_BOND_XMIT_HASH_POLICY]) {
		bn->hashing_type = nla_get_u8(tb[IFLA_BOND_XMIT_HASH_POLICY]);
		bn->ce_mask |= BOND_HAS_HASHING_TYPE;
	}

	if (tb[IFLA_BOND_MIN_LINKS]) {
		bn->min_links = nla_get_u32(tb[IFLA_BOND_MIN_LINKS]);
		bn->ce_mask |= BOND_HAS_MIN_LINKS;
	}

	return 0;
}

static void bond_info_free(struct rtnl_link *link)
{
	_nl_clear_free(&link->l_info);
}

static int bond_info_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct bond_info *bond_dst, *bond_src = src->l_info;
	int err;

	_nl_assert(bond_src);

	err = bond_info_alloc(dst);
	if (err)
		return err;

	bond_dst = dst->l_info;

	_nl_assert(bond_dst);

	*bond_dst = *bond_src;

	return 0;
}

static int bond_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct bond_info *bn = link->l_info;
	struct nlattr *data;

	data = nla_nest_start(msg, IFLA_INFO_DATA);
	if (!data)
		return -NLE_MSGSIZE;
	if (bn->ce_mask & BOND_HAS_MODE)
		NLA_PUT_U8(msg, IFLA_BOND_MODE, bn->bn_mode);

	if (bn->ce_mask & BOND_HAS_ACTIVE_SLAVE)
		NLA_PUT_U32(msg, IFLA_BOND_ACTIVE_SLAVE, bn->ifindex);

	if (bn->ce_mask & BOND_HAS_HASHING_TYPE)
		NLA_PUT_U8(msg, IFLA_BOND_XMIT_HASH_POLICY, bn->hashing_type);

	if (bn->ce_mask & BOND_HAS_MIIMON)
		NLA_PUT_U32(msg, IFLA_BOND_MIIMON, bn->miimon);

	if (bn->ce_mask & BOND_HAS_MIN_LINKS)
		NLA_PUT_U32(msg, IFLA_BOND_MIN_LINKS, bn->min_links);

	nla_nest_end(msg, data);
	return 0;

nla_put_failure:
	nla_nest_cancel(msg, data);
	return -NLE_MSGSIZE;
}

static int bond_info_compare(struct rtnl_link *link_a, struct rtnl_link *link_b,
			     int flags)
{
	struct bond_info *a = link_a->l_info;
	struct bond_info *b = link_b->l_info;
	uint32_t attrs = flags & LOOSE_COMPARISON ? b->ce_mask : ~0u;
	int diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(BOND_HAS_MODE, a->bn_mode != b->bn_mode);
	diff |= _DIFF(BOND_HAS_ACTIVE_SLAVE, a->ifindex != b->ifindex);
	diff |= _DIFF(BOND_HAS_HASHING_TYPE,
		      a->hashing_type != b->hashing_type);
	diff |= _DIFF(BOND_HAS_MIIMON, a->miimon != b->miimon);
	diff |= _DIFF(BOND_HAS_MIN_LINKS, a->min_links != b->min_links);
#undef _DIFF

	return diff;
}

static struct rtnl_link_info_ops bonding_info_ops = {
	.io_name		= "bond",
	.io_alloc		= bond_info_alloc,
	.io_parse		= bond_info_parse,
	.io_clone		= bond_info_clone,
	.io_put_attrs		= bond_put_attrs,
	.io_free		= bond_info_free,
	.io_compare		= bond_info_compare,
};

#define IS_BOND_INFO_ASSERT(link)                                                    \
	do {                                                                         \
		if (link->l_info_ops != &bonding_info_ops) {                         \
			APPBUG("Link is not a bond link. Set type \"bond\" first."); \
		}                                                                    \
	} while (0)

/**
 * Set active slave for bond
 * @arg link            Link object of type bond
 * @arg active          ifindex of active slave to set
 *
 * @return void
 */
void rtnl_link_bond_set_activeslave(struct rtnl_link *link, int active_slave)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	bn->ifindex = active_slave;

	bn->ce_mask |= BOND_HAS_ACTIVE_SLAVE;
}

/**
 * Get active slave for bond
 * @arg link		Link object of type bond
 * @arg active_slave	Output argument
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bond_get_activeslave(struct rtnl_link *link, int *active_slave)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	if (!(bn->ce_mask & BOND_HAS_ACTIVE_SLAVE))
		return -NLE_NOATTR;

	if (!active_slave)
		return -NLE_INVAL;

	*active_slave = bn->ifindex;

	return 0;
}

/**
 * Set bond mode
 * @arg link            Link object of type bond
 * @arg mode            bond mode to set
 *
 * @return void
 */
void rtnl_link_bond_set_mode(struct rtnl_link *link, uint8_t mode)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	bn->bn_mode = mode;

	bn->ce_mask |= BOND_HAS_MODE;
}

/**
 * Get bond mode
 * @arg link		Link object of type bond
 * @arg mode		Output argument
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bond_get_mode(struct rtnl_link *link, uint8_t *mode)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	if (!(bn->ce_mask & BOND_HAS_MODE))
		return -NLE_NOATTR;

	if (!mode)
		return -NLE_INVAL;

	*mode = bn->bn_mode;

	return 0;
}

/**
 * Set hashing type
 * @arg link            Link object of type bond
 * @arg type            bond hashing type to set
 *
 * @return void
 */
void rtnl_link_bond_set_hashing_type(struct rtnl_link *link, uint8_t type)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	bn->hashing_type = type;

	bn->ce_mask |= BOND_HAS_HASHING_TYPE;
}

/**
 * Get hashing type
 * @arg link		Link object of type bond
 * @arg type		Output argument
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bond_get_hashing_type(struct rtnl_link *link, uint8_t *type)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	if (!(bn->ce_mask & BOND_HAS_HASHING_TYPE))
		return -NLE_NOATTR;

	if (!type)
		return -NLE_INVAL;

	*type = bn->hashing_type;

	return 0;
}

/**
 * Set MII monitoring interval
 * @arg link            Link object of type bond
 * @arg miimon          interval in milliseconds
 *
 * @return void
 */
void rtnl_link_bond_set_miimon(struct rtnl_link *link, uint32_t miimon)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	bn->miimon = miimon;

	bn->ce_mask |= BOND_HAS_MIIMON;
}

/**
 * Get MII monitoring interval
 * @arg link            Link object of type bond
 * @arg miimon		Output argument
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bond_get_miimon(struct rtnl_link *link, uint32_t *miimon)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	if (!(bn->ce_mask & BOND_HAS_MIIMON))
		return -NLE_NOATTR;

	if (!miimon)
		return -NLE_INVAL;

	*miimon = bn->miimon;

	return 0;
}

/**
 * Set the minimum number of member ports that must be up before
 * marking the bond device as up
 * @arg link            Link object of type bond
 * @arg min_links       Number of links
 *
 * @return void
 */
void rtnl_link_bond_set_min_links(struct rtnl_link *link, uint32_t min_links)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	bn->min_links = min_links;

	bn->ce_mask |= BOND_HAS_MIN_LINKS;
}
/**
 * Get the minimum number of member ports that must be up before
 * marking the bond device as up
 * @arg link		Link object of type bond
 * @arg min_links	Output argument.
 *
 * @return Zero on success, otherwise a negative error code.
 * @retval -NLE_NOATTR
 * @retval -NLE_INVAL
 */
int rtnl_link_bond_get_min_links(struct rtnl_link *link, uint32_t *min_links)
{
	struct bond_info *bn = link->l_info;

	IS_BOND_INFO_ASSERT(link);

	if (!(bn->ce_mask & BOND_HAS_MIN_LINKS))
		return -NLE_NOATTR;

	if (!min_links)
		return -NLE_INVAL;

	*min_links = bn->min_links;

	return 0;
}

/**
 * Allocate link object of type bond
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_bond_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "bond") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Create a new kernel bonding device
 * @arg sock		netlink socket
 * @arg name		name of bonding device or NULL
 * @arg opts		bonding options (currently unused)
 *
 * Creates a new bonding device in the kernel. If no name is
 * provided, the kernel will automatically pick a name of the
 * form "type%d" (e.g. bond0, vlan1, etc.)
 *
 * The \a opts argument is currently unused. In the future, it
 * may be used to carry additional bonding options to be set
 * when creating the bonding device.
 *
 * @note When letting the kernel assign a name, it will become
 *       difficult to retrieve the interface afterwards because
 *       you have to guess the name the kernel has chosen. It is
 *       therefore not recommended to not provide a device name.
 *
 * @see rtnl_link_bond_enslave()
 * @see rtnl_link_bond_release()
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_bond_add(struct nl_sock *sock, const char *name,
		       struct rtnl_link *opts)
{
	struct rtnl_link *link;
	int err;

	if (!(link = rtnl_link_bond_alloc()))
		return -NLE_NOMEM;

	if (!name && opts)
		name = rtnl_link_get_name(opts);

	if (name)
		rtnl_link_set_name(link, name);

	err = rtnl_link_add(sock, link, NLM_F_CREATE);

	rtnl_link_put(link);

	return err;
}

/**
 * Check if a link is a bond
 * @arg link		Link object
 *
 * @return 1 if the link is a bond, 0 otherwise.
 */
int rtnl_link_is_bond(struct rtnl_link *link)
{
	return link->l_info_ops == &bonding_info_ops;
}

/**
 * Add a link to a bond (enslave)
 * @arg sock		netlink socket
 * @arg master		ifindex of bonding master
 * @arg slave		ifindex of slave link to add to bond
 *
 * This function is identical to rtnl_link_bond_enslave() except that
 * it takes interface indices instead of rtnl_link objcets.
 *
 * @see rtnl_link_bond_enslave()
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_bond_enslave_ifindex(struct nl_sock *sock, int master,
				   int slave)
{
	struct rtnl_link *link;
	int err;

	if (!(link = rtnl_link_bond_alloc()))
		return -NLE_NOMEM;

	rtnl_link_set_ifindex(link, slave);
	rtnl_link_set_master(link, master);
	
	if ((err = rtnl_link_change(sock, link, link, 0)) < 0)
		goto errout;

	rtnl_link_put(link);

	/*
	 * Due to the kernel not signaling whether this opertion is
	 * supported or not, we will retrieve the attribute to see  if the
	 * request was successful. If the master assigned remains unchanged
	 * we will return NLE_OPNOTSUPP to allow performing backwards
	 * compatibility of some sort.
	 */
	if ((err = rtnl_link_get_kernel(sock, slave, NULL, &link)) < 0)
		return err;

	if (rtnl_link_get_master(link) != master)
		err = -NLE_OPNOTSUPP;

errout:
	rtnl_link_put(link);

	return err;
}

/**
 * Add a link to a bond (enslave)
 * @arg sock		netlink socket
 * @arg master		bonding master
 * @arg slave		slave link to add to bond
 *
 * Constructs a RTM_NEWLINK or RTM_SETLINK message adding the slave to
 * the master and sends the request via the specified netlink socket.
 *
 * @note The feature of enslaving/releasing via netlink has only been added
 *       recently to the kernel (Feb 2011). Also, the kernel does not signal
 *       if the operation is not supported. Therefore this function will
 *       verify if the master assignment has changed and will return
 *       -NLE_OPNOTSUPP if it did not.
 *
 * @see rtnl_link_bond_enslave_ifindex()
 * @see rtnl_link_bond_release()
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_bond_enslave(struct nl_sock *sock, struct rtnl_link *master,
			   struct rtnl_link *slave)
{
	return rtnl_link_bond_enslave_ifindex(sock,
				rtnl_link_get_ifindex(master),
				rtnl_link_get_ifindex(slave));
}

/**
 * Release a link from a bond
 * @arg sock		netlink socket
 * @arg slave		slave link to be released
 *
 * This function is identical to rtnl_link_bond_release() except that
 * it takes an interface index instead of a rtnl_link object.
 *
 * @see rtnl_link_bond_release()
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_bond_release_ifindex(struct nl_sock *sock, int slave)
{
	return rtnl_link_bond_enslave_ifindex(sock, 0, slave);
}

/**
 * Release a link from a bond
 * @arg sock		netlink socket
 * @arg slave		slave link to be released
 *
 * Constructs a RTM_NEWLINK or RTM_SETLINK message releasing the slave from
 * its master and sends the request via the specified netlink socket.
 *
 * @note The feature of enslaving/releasing via netlink has only been added
 *       recently to the kernel (Feb 2011). Also, the kernel does not signal
 *       if the operation is not supported. Therefore this function will
 *       verify if the master assignment has changed and will return
 *       -NLE_OPNOTSUPP if it did not.
 *
 * @see rtnl_link_bond_release_ifindex()
 * @see rtnl_link_bond_enslave()
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_bond_release(struct nl_sock *sock, struct rtnl_link *slave)
{
	return rtnl_link_bond_release_ifindex(sock,
				rtnl_link_get_ifindex(slave));
}

static void _nl_init bonding_init(void)
{
	rtnl_link_register_info(&bonding_info_ops);
}

static void _nl_exit bonding_exit(void)
{
	rtnl_link_unregister_info(&bonding_info_ops);
}

/** @} */
