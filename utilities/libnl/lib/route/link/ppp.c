/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2016 Jonas Johansson <jonasj76@gmail.com>
 */

/**
 * @ingroup link
 * @defgroup ppp PPP
 *
 * @details
 * \b Link Type Name: "ppp"
 *
 * @route_doc{link_ppp, PPP Documentation}
 * @{
 */

#include "nl-default.h"

#include <netlink/route/link/ppp.h>
#include <netlink/netlink.h>

#include "nl-route.h"
#include "link-api.h"

/** @cond SKIP */
#define PPP_ATTR_FD		(1<<0)

struct ppp_info
{
	int32_t			pi_fd;
	uint32_t		ce_mask;
};

/** @endcond */

static struct nla_policy ppp_nl_policy[IFLA_PPP_MAX+1] = {
	[IFLA_PPP_DEV_FD]	= { .type = NLA_S32 },
};

static int ppp_alloc(struct rtnl_link *link)
{
	struct ppp_info *info;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*info));
	else {
		if ((info = calloc(1, sizeof(*info))) == NULL)
			return -NLE_NOMEM;

		link->l_info = info;
	}

	return 0;
}

static int ppp_parse(struct rtnl_link *link, struct nlattr *data,
                         struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_PPP_MAX+1];
	struct ppp_info *info;
	int err;

	NL_DBG(3, "Parsing PPP link info\n");

	if ((err = nla_parse_nested(tb, IFLA_PPP_MAX, data, ppp_nl_policy)) < 0)
		goto errout;

	if ((err = ppp_alloc(link)) < 0)
		goto errout;

	info = link->l_info;

	if (tb[IFLA_PPP_DEV_FD]) {
		info->pi_fd = nla_get_s32(tb[IFLA_PPP_DEV_FD]);
		info->ce_mask |= PPP_ATTR_FD;
	}

	err = 0;
errout:
	return err;
}

static void ppp_free(struct rtnl_link *link)
{
	free(link->l_info);
	link->l_info = NULL;
}

static int ppp_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct ppp_info *vdst, *vsrc = src->l_info;
	int err;

	dst->l_info = NULL;
	if ((err = rtnl_link_set_type(dst, "ppp")) < 0)
		return err;
	vdst = dst->l_info;

	if (!vdst || !vsrc)
		return -NLE_NOMEM;

	memcpy(vdst, vsrc, sizeof(struct ppp_info));

	return 0;
}

static int ppp_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct ppp_info *info = link->l_info;
	struct nlattr *data;

	if (!(data = nla_nest_start(msg, IFLA_INFO_DATA)))
		return -NLE_MSGSIZE;

	if (info->ce_mask & PPP_ATTR_FD)
		NLA_PUT_S32(msg, IFLA_PPP_DEV_FD, info->pi_fd);

	nla_nest_end(msg, data);

nla_put_failure:

	return 0;
}

static struct rtnl_link_info_ops ppp_info_ops = {
	.io_name		= "ppp",
	.io_alloc		= ppp_alloc,
	.io_parse		= ppp_parse,
	.io_clone		= ppp_clone,
	.io_put_attrs		= ppp_put_attrs,
	.io_free		= ppp_free,
};

/** @cond SKIP */
#define IS_PPP_LINK_ASSERT(link) \
	if ((link)->l_info_ops != &ppp_info_ops) { \
		APPBUG("Link is not a PPP link. set type \"ppp\" first."); \
		return -NLE_OPNOTSUPP; \
	}
/** @endcond */

/**
 * @name PPP Object
 * @{
 */

/**
 * Allocate link object of type PPP
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_ppp_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "ppp") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Set PPP file descriptor
 * @arg link		Link object
 * @arg flags		PPP file descriptor
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_link_ppp_set_fd(struct rtnl_link *link, int32_t fd)
{
	struct ppp_info *info = link->l_info;

	IS_PPP_LINK_ASSERT(link);

	info->pi_fd |= fd;
	info->ce_mask |= PPP_ATTR_FD;

	return 0;
}

/**
 * Get PPP file descriptor
 * @arg link		Link object
 *
 * @return PPP file descriptor, 0 if not set or a negative error code.
 */
int rtnl_link_ppp_get_fd(struct rtnl_link *link, int32_t *fd)
{
	struct ppp_info *info = link->l_info;

	IS_PPP_LINK_ASSERT(link);

	if (!(info->ce_mask & PPP_ATTR_FD))
		return -NLE_NOATTR;

	if (fd)
		*fd = info->pi_fd;

	return 0;
}

/** @} */

static void _nl_init ppp_init(void)
{
	rtnl_link_register_info(&ppp_info_ops);
}

static void _nl_exit ppp_exit(void)
{
	rtnl_link_unregister_info(&ppp_info_ops);
}

/** @} */
