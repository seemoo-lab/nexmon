/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2015 Cong Wang <cwang@twopensource.com>
 */

/**
 * @ingroup link
 * @defgroup ipvlan IPVLAN
 * IP-based Virtual LAN link module
 *
 * @details
 * \b Link Type Name: "ipvlan"
 *
 * @route_doc{link_ipvlan, IPVLAN Documentation}
 *
 * @{
 */

#include "nl-default.h"

#include <linux/if_link.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/ipvlan.h>

#include "nl-route.h"
#include "link-api.h"

/** @cond SKIP */
#define IPVLAN_HAS_MODE	(1<<0)

struct ipvlan_info
{
	uint16_t		ipi_mode;
	uint32_t		ipi_mask;
};

/** @endcond */

static struct nla_policy ipvlan_policy[IFLA_IPVLAN_MAX+1] = {
	[IFLA_IPVLAN_MODE]	= { .type = NLA_U16 },
};

static int ipvlan_alloc(struct rtnl_link *link)
{
	struct ipvlan_info *ipi;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*ipi));
	else {
		if ((ipi = calloc(1, sizeof(*ipi))) == NULL)
			return -NLE_NOMEM;

		link->l_info = ipi;
	}

	return 0;
}

static int ipvlan_parse(struct rtnl_link *link, struct nlattr *data,
                         struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_IPVLAN_MAX+1];
	struct ipvlan_info *ipi;
	int err;

	NL_DBG(3, "Parsing IPVLAN link info\n");

	if ((err = nla_parse_nested(tb, IFLA_IPVLAN_MAX, data, ipvlan_policy)) < 0)
		goto errout;

	if ((err = ipvlan_alloc(link)) < 0)
		goto errout;

	ipi = link->l_info;

	if (tb[IFLA_IPVLAN_MODE]) {
		ipi->ipi_mode = nla_get_u16(tb[IFLA_IPVLAN_MODE]);
		ipi->ipi_mask |= IPVLAN_HAS_MODE;
	}

	err = 0;
errout:
	return err;
}

static void ipvlan_free(struct rtnl_link *link)
{
	free(link->l_info);
	link->l_info = NULL;
}

static void ipvlan_dump(struct rtnl_link *link, struct nl_dump_params *p)
{
	char buf[64];
	struct ipvlan_info *ipi = link->l_info;

	if (ipi->ipi_mask & IPVLAN_HAS_MODE) {
		rtnl_link_ipvlan_mode2str(ipi->ipi_mode, buf, sizeof(buf));
		nl_dump(p, "ipvlan-mode %s", buf);
	}
}

static int ipvlan_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct ipvlan_info *vdst, *vsrc = src->l_info;
	int err;

	dst->l_info = NULL;
	if ((err = rtnl_link_set_type(dst, "ipvlan")) < 0)
		return err;
	vdst = dst->l_info;

	if (!vdst || !vsrc)
		return -NLE_NOMEM;

	memcpy(vdst, vsrc, sizeof(struct ipvlan_info));

	return 0;
}

static int ipvlan_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct ipvlan_info *ipi = link->l_info;
	struct nlattr *data;

	if (!(data = nla_nest_start(msg, IFLA_INFO_DATA)))
		return -NLE_MSGSIZE;

	if (ipi->ipi_mask & IPVLAN_HAS_MODE)
		NLA_PUT_U16(msg, IFLA_IPVLAN_MODE, ipi->ipi_mode);

	nla_nest_end(msg, data);

nla_put_failure:

	return 0;
}

static struct rtnl_link_info_ops ipvlan_info_ops = {
	.io_name		= "ipvlan",
	.io_alloc		= ipvlan_alloc,
	.io_parse		= ipvlan_parse,
	.io_dump = {
	    [NL_DUMP_LINE]	= ipvlan_dump,
	    [NL_DUMP_DETAILS]	= ipvlan_dump,
	},
	.io_clone		= ipvlan_clone,
	.io_put_attrs		= ipvlan_put_attrs,
	.io_free		= ipvlan_free,
};

/** @cond SKIP */
#define IS_IPVLAN_LINK_ASSERT(link) \
	if ((link)->l_info_ops != &ipvlan_info_ops) { \
		APPBUG("Link is not a ipvlan link. set type \"ipvlan\" first."); \
		return -NLE_OPNOTSUPP; \
	}
/** @endcond */

/**
 * @name IPVLAN Object
 * @{
 */

/**
 * Allocate link object of type IPVLAN
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_ipvlan_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "ipvlan") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a IPVLAN link
 * @arg link		Link object
 *
 * @return True if link is a IPVLAN link, otherwise false is returned.
 */
int rtnl_link_is_ipvlan(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "ipvlan");
}

/**
 * Set IPVLAN MODE
 * @arg link		Link object
 * @arg mode		IPVLAN mode
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_ipvlan_set_mode(struct rtnl_link *link, uint16_t mode)
{
	struct ipvlan_info *ipi = link->l_info;

	IS_IPVLAN_LINK_ASSERT(link);

	ipi->ipi_mode = mode;
	ipi->ipi_mask |= IPVLAN_HAS_MODE;

	return 0;
}

/**
 * Get IPVLAN Mode
 * @arg link		Link object
 * @arg out_mode        on success, return the mode
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_ipvlan_get_mode(struct rtnl_link *link, uint16_t *out_mode)
{
	struct ipvlan_info *ipi = link->l_info;

	IS_IPVLAN_LINK_ASSERT(link);

	if (!(ipi->ipi_mask & IPVLAN_HAS_MODE))
		return -NLE_INVAL;
	*out_mode = ipi->ipi_mode;
	return 0;
}

/** @} */

static const struct trans_tbl ipvlan_modes[] = {
	__ADD(IPVLAN_MODE_L2, l2),
	__ADD(IPVLAN_MODE_L3, l3),
};

/**
 * @name Mode Translation
 * @{
 */

char *rtnl_link_ipvlan_mode2str(int mode, char *buf, size_t len)
{
	return __type2str(mode, buf, len, ipvlan_modes, ARRAY_SIZE(ipvlan_modes));
}

int rtnl_link_ipvlan_str2mode(const char *name)
{
	return __str2type(name, ipvlan_modes, ARRAY_SIZE(ipvlan_modes));
}

/** @} */

static void _nl_init ipvlan_init(void)
{
	rtnl_link_register_info(&ipvlan_info_ops);
}

static void _nl_exit ipvlan_exit(void)
{
	rtnl_link_unregister_info(&ipvlan_info_ops);
}

/** @} */
