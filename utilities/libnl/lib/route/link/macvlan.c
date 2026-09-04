/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Michael Braun <michael-dev@fami-braun.de>
 */

/**
 * @ingroup link
 * @defgroup macvlan MACVLAN/MACVTAP
 * MAC-based Virtual LAN link module
 *
 * @details
 * \b Link Type Name: "macvlan"
 *
 * @route_doc{link_macvlan, MACVLAN Documentation}
 * @route_doc{link_macvtap, MACVTAP Documentation}
 *
 * @{
 */

#include "nl-default.h"

#include <linux/if_link.h>

#include <linux/ethtool.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/macvlan.h>
#include <netlink/route/link/macvtap.h>

#include "nl-route.h"
#include "link-api.h"

/** @cond SKIP */
#define MACVLAN_HAS_MODE        (1<<0)
#define MACVLAN_HAS_FLAGS       (1<<1)
#define MACVLAN_HAS_MACADDR     (1<<2)

struct macvlan_info
{
	uint32_t                mvi_mode;
	uint16_t                mvi_flags; // there currently is only one flag and kernel has no flags_mask yet
	uint32_t                mvi_mask;
	uint32_t                mvi_maccount;
	uint32_t                mvi_macmode;
	struct nl_addr          **mvi_macaddr;
};

/** @endcond */

static struct nla_policy macvlan_policy[IFLA_MACVLAN_MAX+1] = {
	[IFLA_MACVLAN_MODE]             = { .type = NLA_U32 },
	[IFLA_MACVLAN_FLAGS]            = { .type = NLA_U16 },
	[IFLA_MACVLAN_MACADDR_MODE]     = { .type = NLA_U32 },
	[IFLA_MACVLAN_MACADDR]          = { .type = NLA_UNSPEC },
	[IFLA_MACVLAN_MACADDR_DATA]     = { .type = NLA_NESTED },
	[IFLA_MACVLAN_MACADDR_COUNT]    = { .type = NLA_U32 },
};

static int macvlan_alloc(struct rtnl_link *link)
{
	struct macvlan_info *mvi;
	uint32_t i;

	if (link->l_info) {
		mvi = link->l_info;
		for (i = 0; i < mvi->mvi_maccount; i++)
			nl_addr_put(mvi->mvi_macaddr[i]);
		free(mvi->mvi_macaddr);
		memset(mvi, 0, sizeof(*mvi));
	} else {
		if ((mvi = calloc(1, sizeof(*mvi))) == NULL)
			return -NLE_NOMEM;

		link->l_info = mvi;
	}
	mvi->mvi_macmode = MACVLAN_MACADDR_SET;

	return 0;
}

static int macvlan_parse(struct rtnl_link *link, struct nlattr *data,
                         struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_MACVLAN_MAX+1];
	struct macvlan_info *mvi;
	struct nlattr *nla;
	int len;
	int err;

	NL_DBG(3, "Parsing %s link info", link->l_info_ops->io_name);

	if ((err = nla_parse_nested(tb, IFLA_MACVLAN_MAX, data, macvlan_policy)) < 0)
		goto errout;

	if ((err = macvlan_alloc(link)) < 0)
		goto errout;

	mvi = link->l_info;

	if (tb[IFLA_MACVLAN_MODE]) {
		mvi->mvi_mode = nla_get_u32(tb[IFLA_MACVLAN_MODE]);
		mvi->mvi_mask |= MACVLAN_HAS_MODE;
	}

	if (tb[IFLA_MACVLAN_FLAGS]) {
		mvi->mvi_flags = nla_get_u16(tb[IFLA_MACVLAN_FLAGS]);
		mvi->mvi_mask |= MACVLAN_HAS_FLAGS;
	}

	if (   tb[IFLA_MACVLAN_MACADDR_COUNT]
	    && tb[IFLA_MACVLAN_MACADDR_DATA]) {
		mvi->mvi_maccount = nla_get_u32(tb[IFLA_MACVLAN_MACADDR_COUNT]);
		if (mvi->mvi_maccount > 0) {
			uint32_t i;

			nla = nla_data(tb[IFLA_MACVLAN_MACADDR_DATA]);
			len = nla_len(tb[IFLA_MACVLAN_MACADDR_DATA]);

			mvi->mvi_macaddr = calloc(mvi->mvi_maccount,
			                          sizeof(*(mvi->mvi_macaddr)));
			if (mvi->mvi_macaddr == NULL) {
				err = -NLE_NOMEM;
				goto errout;
			}

			i = 0;
			for (; nla_ok(nla, len); nla = nla_next(nla, &len)) {
				if (i >= mvi->mvi_maccount)
					break;
				if (nla_type(nla) != IFLA_MACVLAN_MACADDR ||
				    nla_len(nla) < ETH_ALEN)
					continue;
				mvi->mvi_macaddr[i] = nl_addr_alloc_attr(nla, AF_LLC);
				i++;
			}
		}
		mvi->mvi_mask |= MACVLAN_HAS_MACADDR;
	}

	err = 0;
errout:
	return err;
}

static void macvlan_free(struct rtnl_link *link)
{
	struct macvlan_info *mvi;
	uint32_t i;

	mvi = link->l_info;
	if (!mvi)
		return;

	for (i = 0; i < mvi->mvi_maccount; i++)
		nl_addr_put(mvi->mvi_macaddr[i]);
	free(mvi->mvi_macaddr);
	free(mvi);

	link->l_info = NULL;
}

static void macvlan_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	char buf[64];
	uint32_t i;
	struct macvlan_info *mvi = link->l_info;

	if (mvi->mvi_mask & MACVLAN_HAS_MODE) {
		rtnl_link_macvlan_mode2str(mvi->mvi_mode, buf, sizeof(buf));
		nl_dump(p, "    %s-mode %s", link->l_info_ops->io_name, buf);
	}

	if (mvi->mvi_mask & MACVLAN_HAS_FLAGS) {
		rtnl_link_macvlan_flags2str(mvi->mvi_flags, buf, sizeof(buf));
		nl_dump(p, " %s-flags %s", link->l_info_ops->io_name, buf);
	}

	if (mvi->mvi_mask & MACVLAN_HAS_MACADDR) {
		nl_dump(p, " macvlan-count %u", (unsigned) mvi->mvi_maccount);

		if (mvi->mvi_maccount)
			nl_dump(p, " macvlan-sourcemac");

		for (i = 0; i < mvi->mvi_maccount; i++) {
			nl_dump(p, " %s", nl_addr2str(mvi->mvi_macaddr[i], buf,
			        sizeof(buf)));
		}
	}
	nl_dump(p, "\n");
}

static int macvlan_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct macvlan_info *vdst, *vsrc = src->l_info;
	int err;
	uint32_t i;

	dst->l_info = NULL;
	if ((err = rtnl_link_set_type(dst, "macvlan")) < 0)
		return err;
	vdst = dst->l_info;

	if (!vdst || !vsrc)
		return -NLE_NOMEM;

	memcpy(vdst, vsrc, sizeof(struct macvlan_info));

	if (   vsrc->mvi_mask & MACVLAN_HAS_MACADDR
	    && vsrc->mvi_maccount > 0) {
		vdst->mvi_macaddr = calloc(vdst->mvi_maccount,
		                           sizeof(*(vdst->mvi_macaddr)));
		for (i = 0; i < vdst->mvi_maccount; i++)
			vdst->mvi_macaddr[i] = nl_addr_clone(vsrc->mvi_macaddr[i]);
	} else
		vdst->mvi_macaddr = NULL;

	return 0;
}

static int macvlan_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct macvlan_info *mvi = link->l_info;
	struct nlattr *data, *datamac = NULL;
	int ret;

	if (!(data = nla_nest_start(msg, IFLA_INFO_DATA)))
		return -NLE_MSGSIZE;

	ret = -NLE_NOMEM;

	if (mvi->mvi_mask & MACVLAN_HAS_MODE)
		NLA_PUT_U32(msg, IFLA_MACVLAN_MODE, mvi->mvi_mode);

	if (mvi->mvi_mask & MACVLAN_HAS_FLAGS)
		NLA_PUT_U16(msg, IFLA_MACVLAN_FLAGS, mvi->mvi_flags);

	if (mvi->mvi_mask & MACVLAN_HAS_MACADDR) {
		uint32_t i;

		NLA_PUT_U32(msg, IFLA_MACVLAN_MACADDR_MODE, mvi->mvi_macmode);
		datamac = nla_nest_start(msg, IFLA_MACVLAN_MACADDR_DATA);
		if (!datamac)
			goto nla_put_failure;

		for (i = 0; i < mvi->mvi_maccount; i++) {
			NLA_PUT_ADDR(msg, IFLA_MACVLAN_MACADDR,
			             mvi->mvi_macaddr[i]);
		}
	}

	ret = 0;

nla_put_failure:
	if (datamac)
		nla_nest_end(msg, datamac);

	nla_nest_end(msg, data);

	return ret;
}

static struct rtnl_link_info_ops macvlan_info_ops = {
	.io_name                = "macvlan",
	.io_alloc               = macvlan_alloc,
	.io_parse               = macvlan_parse,
	.io_dump = {
		[NL_DUMP_DETAILS] = macvlan_dump_details,
	},
	.io_clone               = macvlan_clone,
	.io_put_attrs           = macvlan_put_attrs,
	.io_free                = macvlan_free,
};

static struct rtnl_link_info_ops macvtap_info_ops = {
	.io_name                = "macvtap",
	.io_alloc               = macvlan_alloc,
	.io_parse               = macvlan_parse,
	.io_dump = {
		[NL_DUMP_DETAILS] = macvlan_dump_details,
	},
	.io_clone               = macvlan_clone,
	.io_put_attrs           = macvlan_put_attrs,
	.io_free                = macvlan_free,
};

/** @cond SKIP */
#define IS_MACVLAN_LINK_ASSERT(link) \
	if ((link)->l_info_ops != &macvlan_info_ops) { \
		APPBUG("Link is not a macvlan link. set type \"macvlan\" first."); \
		return -NLE_OPNOTSUPP; \
	}

#define IS_MACVTAP_LINK_ASSERT(link) \
	if ((link)->l_info_ops != &macvtap_info_ops) { \
		APPBUG("Link is not a macvtap link. set type \"macvtap\" first."); \
		return -NLE_OPNOTSUPP; \
	}
/** @endcond */

/**
 * @name MACVLAN Object
 * @{
 */

/**
 * Allocate link object of type MACVLAN
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_macvlan_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "macvlan") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a MACVLAN link
 * @arg link            Link object
 *
 * @return True if link is a MACVLAN link, otherwise false is returned.
 */
int rtnl_link_is_macvlan(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "macvlan");
}

/**
 * Set MACVLAN MODE
 * @arg link            Link object
 * @arg mode            MACVLAN mode
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_macvlan_set_mode(struct rtnl_link *link, uint32_t mode)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	mvi->mvi_mode = mode;
	mvi->mvi_mask |= MACVLAN_HAS_MODE;

	if (mode != MACVLAN_MODE_SOURCE) {
		uint32_t i;

		for (i = 0; i < mvi->mvi_maccount; i++)
			nl_addr_put(mvi->mvi_macaddr[i]);
		free(mvi->mvi_macaddr);
		mvi->mvi_maccount = 0;
		mvi->mvi_macaddr = NULL;
		mvi->mvi_macmode = MACVLAN_MACADDR_SET;
		mvi->mvi_mask &= ~MACVLAN_HAS_MACADDR;
	}

	return 0;
}

/**
 * Get MACVLAN Mode
 * @arg link            Link object
 *
 * @return MACVLAN mode, 0 if not set or a negative error code.
 */
uint32_t rtnl_link_macvlan_get_mode(struct rtnl_link *link)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	if (mvi->mvi_mask & MACVLAN_HAS_MODE)
		return mvi->mvi_mode;
	else
		return 0;
}

/**
 * Set MACVLAN MACMODE
 * @arg link            Link object
 * @arg mode            MACVLAN mac list modification mode
 *
 * Only for macvlan SOURCE mode.
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_macvlan_set_macmode(struct rtnl_link *link, uint32_t macmode)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	if (!(mvi->mvi_mask & MACVLAN_HAS_MODE) ||
	    (mvi->mvi_mode != MACVLAN_MODE_SOURCE))
		return -NLE_INVAL;

	mvi->mvi_macmode = macmode;
	mvi->mvi_mask |= MACVLAN_HAS_MACADDR;

	return 0;
}

/**
 * Get MACVLAN MACMODE
 * @arg link            Link object
 * @arg out_macmode     mac list modification mode
 *
 * Only for SOURCE mode.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macvlan_get_macmode(struct rtnl_link *link, uint32_t *out_macmode)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	if (!(mvi->mvi_mask & MACVLAN_HAS_MODE) ||
	    (mvi->mvi_mode != MACVLAN_MODE_SOURCE))
		return -NLE_INVAL;

	if (!(mvi->mvi_mask & MACVLAN_HAS_MACADDR))
		return -NLE_INVAL;

	*out_macmode = mvi->mvi_macmode;

	return 0;
}

/**
 * Set MACVLAN flags
 * @arg link            Link object
 * @arg flags           MACVLAN flags
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macvlan_set_flags(struct rtnl_link *link, uint16_t flags)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	mvi->mvi_flags |= flags;
	mvi->mvi_mask |= MACVLAN_HAS_FLAGS;

	return 0;
}

/**
 * Unset MACVLAN flags
 * @arg link            Link object
 * @arg flags           MACVLAN flags
 *
 * Note: kernel currently only has a single flag and lacks flags_mask to
 * indicate which flags shall be changed (it always all).
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macvlan_unset_flags(struct rtnl_link *link, uint16_t flags)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	mvi->mvi_flags &= ~flags;
	mvi->mvi_mask |= MACVLAN_HAS_FLAGS;

	return 0;
}

/**
 * Get MACVLAN flags
 * @arg link            Link object
 *
 * @return MACVLAN flags, 0 if none set, or a negative error code.
 */
uint16_t rtnl_link_macvlan_get_flags(struct rtnl_link *link)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	return mvi->mvi_flags;
}

/**
 * Get number of MAC-Addr for MACVLAN device in source mode
 * @arg link            Link object
 * @arg out_count       number of mac addresses
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macvlan_count_macaddr(struct rtnl_link *link, uint32_t *out_count)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	if (!(mvi->mvi_mask & MACVLAN_HAS_MODE) ||
	    (mvi->mvi_mode != MACVLAN_MODE_SOURCE))
		return -NLE_INVAL;

	if (!(mvi->mvi_mask & MACVLAN_HAS_MACADDR))
		return -NLE_INVAL;

	*out_count = mvi->mvi_maccount;

	return 0;
}

/**
 * Get configured remote MAC-Addr from MACVLAN device in source mode
 * @arg link            Link object
 * @arg out_addr        address object
 *
 * The returned nl_addr struct needs NOT to be released using nl_addr_put.
 * It is only valid until the address is not removed from this link object
 * or its mode is changed to non-source.
 *
 * @return 0 on success or negative error code
 */
int rtnl_link_macvlan_get_macaddr(struct rtnl_link *link, uint32_t idx,
                                  const struct nl_addr **out_addr)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVLAN_LINK_ASSERT(link);

	if (!(mvi->mvi_mask & MACVLAN_HAS_MODE) ||
	    (mvi->mvi_mode != MACVLAN_MODE_SOURCE))
		return -NLE_INVAL;

	if (!(mvi->mvi_mask & MACVLAN_HAS_MACADDR))
		return -NLE_INVAL;

	if (idx >= mvi->mvi_maccount)
		return -NLE_INVAL;

	*out_addr = mvi->mvi_macaddr[idx];
	return 0;
}

/**
 * Add MAC-Addr to MACVLAN device in source mode
 * @arg link            Link object
 * @arg addr            MAC-Addr
 *
 * addr is not release but cloned by this method.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macvlan_add_macaddr(struct rtnl_link *link, struct nl_addr *addr)
{
	struct macvlan_info *mvi = link->l_info;
	struct nl_addr **mvi_macaddr;
	size_t newsize;

	IS_MACVLAN_LINK_ASSERT(link);

	if (nl_addr_get_family(addr) != AF_LLC)
		return -NLE_INVAL;

	if (!(mvi->mvi_mask & MACVLAN_HAS_MODE) ||
	    (mvi->mvi_mode != MACVLAN_MODE_SOURCE))
		return -NLE_INVAL;

	if (!(mvi->mvi_mask & MACVLAN_HAS_MACADDR))
		return -NLE_INVAL;

	if (mvi->mvi_maccount >= UINT32_MAX)
		return -NLE_INVAL;

	newsize = (mvi->mvi_maccount + 1) * sizeof(*(mvi->mvi_macaddr));
	mvi_macaddr = realloc(mvi->mvi_macaddr, newsize);
	if (!mvi_macaddr)
		return -NLE_NOMEM;

	mvi->mvi_macaddr = mvi_macaddr;
	mvi->mvi_macaddr[mvi->mvi_maccount] = nl_addr_clone(addr);
	mvi->mvi_maccount++;

	mvi->mvi_mask |= MACVLAN_HAS_MACADDR;

	return 0;
}

/**
 * Remove MAC-Addr from MACVLAN device in source mode
 * @arg link            Link object
 * @arg addr            MAC-Addr
 *
 * addr is not release by this method.
 *
 * @return a negative error code on failure, or the number
 *   of deleted addresses on success.
 */
int rtnl_link_macvlan_del_macaddr(struct rtnl_link *link, struct nl_addr *addr)
{
	struct macvlan_info *mvi = link->l_info;
	uint32_t found, i;

	IS_MACVLAN_LINK_ASSERT(link);

	if (nl_addr_get_family(addr) != AF_LLC)
		return -NLE_INVAL;

	if (!(mvi->mvi_mask & MACVLAN_HAS_MODE) ||
	    (mvi->mvi_mode != MACVLAN_MODE_SOURCE))
		return -NLE_INVAL;

	if (!(mvi->mvi_mask & MACVLAN_HAS_MACADDR))
		return -NLE_INVAL;

	nl_addr_get(addr);

	found = 0; i = 0;
	while (i + found < mvi->mvi_maccount) {
		mvi->mvi_macaddr[i] = mvi->mvi_macaddr[i + found];
		if (found > 0)
			mvi->mvi_macaddr[i + found] = NULL;
		if (nl_addr_cmp(addr, mvi->mvi_macaddr[i]) == 0) {
			nl_addr_put(mvi->mvi_macaddr[i]);
			mvi->mvi_macaddr[i] = NULL;
			found++;
		} else
			i++;
	}

	nl_addr_put(addr);

	mvi->mvi_maccount -= found;

	return found > INT_MAX ? INT_MAX : (int) found;
}

/** @} */


/**
 * @name MACVTAP Object
 * @{
 */

/**
 * Allocate link object of type MACVTAP
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_macvtap_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "macvtap") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a MACVTAP link
 * @arg link            Link object
 *
 * @return True if link is a MACVTAP link, otherwise false is returned.
 */
int rtnl_link_is_macvtap(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "macvtap");
}

/**
 * Set MACVTAP MODE
 * @arg link            Link object
 * @arg mode            MACVTAP mode
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_macvtap_set_mode(struct rtnl_link *link, uint32_t mode)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVTAP_LINK_ASSERT(link);

	mvi->mvi_mode = mode;
	mvi->mvi_mask |= MACVLAN_HAS_MODE;

	return 0;
}

/**
 * Get MACVTAP Mode
 * @arg link            Link object
 *
 * @return MACVTAP mode, 0 if not set or a negative error code.
 */
uint32_t rtnl_link_macvtap_get_mode(struct rtnl_link *link)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVTAP_LINK_ASSERT(link);

	if (mvi->mvi_mask & MACVLAN_HAS_MODE)
		return mvi->mvi_mode;
	else
		return 0;
}

/**
 * Set MACVTAP flags
 * @arg link            Link object
 * @arg flags           MACVTAP flags
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macvtap_set_flags(struct rtnl_link *link, uint16_t flags)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVTAP_LINK_ASSERT(link);

	mvi->mvi_flags |= flags;
	mvi->mvi_mask |= MACVLAN_HAS_FLAGS;

	return 0;
}

/**
 * Unset MACVTAP flags
 * @arg link            Link object
 * @arg flags           MACVTAP flags
 *
 * Note: kernel currently only has a single flag and lacks flags_mask to
 * indicate which flags shall be changed (it always all).
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_macvtap_unset_flags(struct rtnl_link *link, uint16_t flags)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVTAP_LINK_ASSERT(link);

	mvi->mvi_flags &= ~flags;
	mvi->mvi_mask |= MACVLAN_HAS_FLAGS;

	return 0;
}

/**
 * Get MACVTAP flags
 * @arg link            Link object
 *
 * @return MACVTAP flags, 0 if none set, or a negative error code.
 */
uint16_t rtnl_link_macvtap_get_flags(struct rtnl_link *link)
{
	struct macvlan_info *mvi = link->l_info;

	IS_MACVTAP_LINK_ASSERT(link);

	return mvi->mvi_flags;
}

/** @} */


static const struct trans_tbl macvlan_flags[] = {
	__ADD(MACVLAN_FLAG_NOPROMISC, nopromisc),
};

static const struct trans_tbl macvlan_modes[] = {
	__ADD(MACVLAN_MODE_PRIVATE, private),
	__ADD(MACVLAN_MODE_VEPA, vepa),
	__ADD(MACVLAN_MODE_BRIDGE, bridge),
	__ADD(MACVLAN_MODE_PASSTHRU, passthru),
	__ADD(MACVLAN_MODE_SOURCE, source),
};

static const struct trans_tbl macvlan_macmodes[] = {
	__ADD(MACVLAN_MACADDR_ADD, "add"),
	__ADD(MACVLAN_MACADDR_DEL, "del"),
	__ADD(MACVLAN_MACADDR_SET, "set"),
	__ADD(MACVLAN_MACADDR_FLUSH, "flush"),
};

/**
 * @name Flag Translation
 * @{
 */

char *rtnl_link_macvlan_flags2str(int flags, char *buf, size_t len)
{
	return __flags2str(flags, buf, len, macvlan_flags, ARRAY_SIZE(macvlan_flags));
}

int rtnl_link_macvlan_str2flags(const char *name)
{
	return __str2flags(name, macvlan_flags, ARRAY_SIZE(macvlan_flags));
}

char *rtnl_link_macvtap_flags2str(int flags, char *buf, size_t len)
{
	return __flags2str(flags, buf, len, macvlan_flags, ARRAY_SIZE(macvlan_flags));
}

int rtnl_link_macvtap_str2flags(const char *name)
{
	return __str2flags(name, macvlan_flags, ARRAY_SIZE(macvlan_flags));
}

/** @} */

/**
 * @name Mode Translation
 * @{
 */

char *rtnl_link_macvlan_mode2str(int mode, char *buf, size_t len)
{
	return __type2str(mode, buf, len, macvlan_modes, ARRAY_SIZE(macvlan_modes));
}

int rtnl_link_macvlan_str2mode(const char *name)
{
	return __str2type(name, macvlan_modes, ARRAY_SIZE(macvlan_modes));
}

char *rtnl_link_macvlan_macmode2str(int mode, char *buf, size_t len)
{
	return __type2str(mode, buf, len, macvlan_macmodes,
	                  ARRAY_SIZE(macvlan_macmodes));
}

int rtnl_link_macvlan_str2macmode(const char *name)
{
	return __str2type(name, macvlan_macmodes, ARRAY_SIZE(macvlan_macmodes));
}

char *rtnl_link_macvtap_mode2str(int mode, char *buf, size_t len)
{
	return __type2str(mode, buf, len, macvlan_modes, ARRAY_SIZE(macvlan_modes));
}

int rtnl_link_macvtap_str2mode(const char *name)
{
	return __str2type(name, macvlan_modes, ARRAY_SIZE(macvlan_modes));
}

/** @} */

static void _nl_init macvlan_init(void)
{
	rtnl_link_register_info(&macvlan_info_ops);
	rtnl_link_register_info(&macvtap_info_ops);
}

static void _nl_exit macvlan_exit(void)
{
	rtnl_link_unregister_info(&macvlan_info_ops);
	rtnl_link_unregister_info(&macvtap_info_ops);
}

/** @} */
