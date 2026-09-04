/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2019 Eyal Birger <eyal.birger@gmail.com>
 *
 * Based on lib/route/link/ipvti.c
 */

/**
 * @ingroup link
 * @defgroup xfrmi XFRMI
 * xfrmi link module
 *
 * @details
 * \b Link Type Name: "xfrmi"
 *
 * @route_doc{link_xfrmi, XFRMI Documentation}
 *
 * @{
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/xfrmi.h>

#include "nl-route.h"
#include "link-api.h"
#include "nl-aux-route/nl-route.h"

#define XFRMI_ATTR_LINK  (1 << 0)
#define XFRMI_ATTR_IF_ID (1 << 1)

#define XFRMI_LINK_TYPE_NAME "xfrm"

struct xfrmi_info {
	uint32_t link;
	uint32_t if_id;
	uint32_t xfrmi_mask;
};

static	struct nla_policy xfrmi_policy[IFLA_XFRM_MAX + 1] = {
	[IFLA_XFRM_LINK]  = { .type = NLA_U32 },
	[IFLA_XFRM_IF_ID] = { .type = NLA_U32 },
};

static int xfrmi_alloc(struct rtnl_link *link)
{
	struct xfrmi_info *xfrmi;

	if (link->l_info)
		memset(link->l_info, 0, sizeof(*xfrmi));
	else {
		xfrmi = calloc(1, sizeof(*xfrmi));
		if (!xfrmi)
			return -NLE_NOMEM;

		link->l_info = xfrmi;
	}

	return 0;
}

static int xfrmi_parse(struct rtnl_link *link, struct nlattr *data,
                       struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_XFRM_MAX + 1];
	struct xfrmi_info *xfrmi;
	int err;

	NL_DBG(3, "Parsing XFRMI link info\n");

	err = nla_parse_nested(tb, IFLA_XFRM_MAX, data, xfrmi_policy);
	if (err < 0)
		return err;

	err = xfrmi_alloc(link);
	if (err < 0)
		return err;

	xfrmi = link->l_info;

	if (tb[IFLA_XFRM_LINK]) {
		xfrmi->link = nla_get_u32(tb[IFLA_XFRM_LINK]);
		xfrmi->xfrmi_mask |= XFRMI_ATTR_LINK;
	}

	if (tb[IFLA_XFRM_IF_ID]) {
		xfrmi->if_id = nla_get_u32(tb[IFLA_XFRM_IF_ID]);
		xfrmi->xfrmi_mask |= XFRMI_ATTR_IF_ID;
	}

	return 0;
}

static int xfrmi_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct xfrmi_info *xfrmi = link->l_info;
	struct nlattr *data;

	data = nla_nest_start(msg, IFLA_INFO_DATA);
	if (!data)
		return -NLE_MSGSIZE;

	if (xfrmi->xfrmi_mask & XFRMI_ATTR_LINK)
		NLA_PUT_U32(msg, IFLA_XFRM_LINK, xfrmi->link);

	if (xfrmi->xfrmi_mask & XFRMI_ATTR_IF_ID)
		NLA_PUT_U32(msg, IFLA_XFRM_IF_ID, xfrmi->if_id);

	nla_nest_end(msg, data);

nla_put_failure:
	return 0;
}

static void xfrmi_free(struct rtnl_link *link)
{
	struct xfrmi_info *xfrmi = link->l_info;

	free(xfrmi);
	link->l_info = NULL;
}

static void xfrmi_dump_line(struct rtnl_link *link, struct nl_dump_params *p)
{
	nl_dump(p, "xfrmi : %s", link->l_name);
}

static void xfrmi_dump_details(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct xfrmi_info *xfrmi = link->l_info;

	if (xfrmi->xfrmi_mask & XFRMI_ATTR_LINK) {
		_nl_auto_rtnl_link struct rtnl_link *parent = NULL;
		char *name;

		nl_dump(p, "      link ");

		name = NULL;
		parent = link_lookup(link->ce_cache, xfrmi->link);
		if (parent)
			name = rtnl_link_get_name(parent);

		if (name)
			nl_dump_line(p, "%s\n", name);
		else
			nl_dump_line(p, "%u\n", xfrmi->link);
	}

	if (xfrmi->xfrmi_mask & XFRMI_ATTR_IF_ID) {
		nl_dump(p, "      if_id   ");
		nl_dump_line(p, "%x\n", xfrmi->if_id);
	}
}

static int xfrmi_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct xfrmi_info *xfrmi_dst, *xfrmi_src = src->l_info;
	int err;

	dst->l_info = NULL;

	err = rtnl_link_set_type(dst, XFRMI_LINK_TYPE_NAME);
	if (err < 0)
		return err;

	xfrmi_dst = dst->l_info;

	if (!xfrmi_dst || !xfrmi_src)
		BUG();

	memcpy(xfrmi_dst, xfrmi_src, sizeof(struct xfrmi_info));

	return 0;
}

static struct rtnl_link_info_ops xfrmi_info_ops = {
	.io_name                = XFRMI_LINK_TYPE_NAME,
	.io_alloc               = xfrmi_alloc,
	.io_parse               = xfrmi_parse,
	.io_dump = {
		[NL_DUMP_LINE]  = xfrmi_dump_line,
		[NL_DUMP_DETAILS] = xfrmi_dump_details,
	},
	.io_clone               = xfrmi_clone,
	.io_put_attrs           = xfrmi_put_attrs,
	.io_free                = xfrmi_free,
};

#define IS_XFRMI_LINK_ASSERT(link) do { \
		if ((link)->l_info_ops != &xfrmi_info_ops) { \
			APPBUG("Link is not a xfrmi link. set type \"xfrmi\" first."); \
			return -NLE_OPNOTSUPP; \
		} \
	} while(0)

struct rtnl_link *rtnl_link_xfrmi_alloc(void)
{
	struct rtnl_link *link;
	int err;

	link = rtnl_link_alloc();
	if (!link)
		return NULL;

	err = rtnl_link_set_type(link, XFRMI_LINK_TYPE_NAME);
	if (err < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a XFRMI link
 * @arg link            Link object
 *
 * @return True if link is a IXFRMI link, otherwise 0 is returned.
 */
int rtnl_link_is_xfrmi(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name,
					   XFRMI_LINK_TYPE_NAME);
}

/**
 * Set XFRMI link interface index
 * @arg link            Link object
 * @arg index           interface index
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_xfrmi_set_link(struct rtnl_link *link, uint32_t index)
{
	struct xfrmi_info *xfrmi = link->l_info;

	IS_XFRMI_LINK_ASSERT(link);

	xfrmi->link = index;
	xfrmi->xfrmi_mask |= XFRMI_ATTR_LINK;

	return 0;
}

/**
 * Get XFRMI link interface index
 * @arg link            Link object
 * @arg out_link        The output value on success
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_xfrmi_get_link(struct rtnl_link *link, uint32_t *out_link)
{
	struct xfrmi_info *xfrmi = link->l_info;

	IS_XFRMI_LINK_ASSERT(link);

	if (!(xfrmi->xfrmi_mask & XFRMI_ATTR_LINK))
		return -NLE_NOATTR;

	*out_link = xfrmi->link;
	return 0;
}

/**
 * Set XFRMI if_id
 * @arg link            Link object
 * @arg if_id            xfrm if_id
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_xfrmi_set_if_id(struct rtnl_link *link, uint32_t if_id)
{
	struct xfrmi_info *xfrmi = link->l_info;

	IS_XFRMI_LINK_ASSERT(link);

	xfrmi->if_id = if_id;
	xfrmi->xfrmi_mask |= XFRMI_ATTR_IF_ID;

	return 0;
}

/**
 * Get XFRMI if_id
 * @arg link            Link object
 * @arg out_if_id       The output value on success
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_xfrmi_get_if_id(struct rtnl_link *link, uint32_t *out_if_id)
{
	struct xfrmi_info *xfrmi = link->l_info;

	IS_XFRMI_LINK_ASSERT(link);

	if (!(xfrmi->xfrmi_mask & XFRMI_ATTR_IF_ID))
		return -NLE_NOATTR;

	*out_if_id = xfrmi->if_id;
	return 0;
}

static void _nl_init xfrmi_init(void)
{
	rtnl_link_register_info(&xfrmi_info_ops);
}

static void _nl_exit xfrmi_exit(void)
{
	rtnl_link_unregister_info(&xfrmi_info_ops);
}
