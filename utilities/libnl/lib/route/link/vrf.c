/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2015 Cumulus Networks. All rights reserved.
 * Copyright (c) 2015 David Ahern <dsa@cumulusnetworks.com>
 */

/**
 * @ingroup link
 * @defgroup vrf VRF
 * Virtual Routing and Forwarding link module
 *
 * @details
 * \b Link Type Name: "vrf"
 *
 * @route_doc{link_vrf, VRF Documentation}
 *
 * @{
 */

#include "nl-default.h"

#include <linux/if_link.h>
#include <linux/rtnetlink.h>

#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/utils.h>
#include <netlink/object.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/link/vrf.h>

#include "nl-route.h"
#include "link-api.h"

#define VRF_TABLE_ID_MAX  RT_TABLE_MAX

/** @cond SKIP */
#define VRF_HAS_TABLE_ID    (1<<0)

struct vrf_info {
	uint32_t        table_id;
	uint32_t        vi_mask;
};

/** @endcond */

static struct nla_policy vrf_policy[IFLA_VRF_MAX + 1] = {
	[IFLA_VRF_TABLE]         = { .type = NLA_U32 },
};

static int vrf_alloc(struct rtnl_link *link)
{
	struct vrf_info *vi;

	if (link->l_info) {
		memset(link->l_info, 0, sizeof (*vi));
		return 0;
	}

	if ((vi = calloc(1, sizeof(*vi))) == NULL)
		return -NLE_NOMEM;

	link->l_info = vi;

	return 0;
}

static int vrf_parse(struct rtnl_link *link, struct nlattr *data,
		     struct nlattr *xstats)
{
	struct nlattr *tb[IFLA_VRF_MAX+1];
	struct vrf_info *vi;
	int err;

	NL_DBG(3, "Parsing VRF link info");

	if ((err = nla_parse_nested(tb, IFLA_VRF_MAX, data, vrf_policy)) < 0)
		goto errout;

	if ((err = vrf_alloc(link)) < 0)
		goto errout;

	vi = link->l_info;

	if (tb[IFLA_VRF_TABLE]) {
		vi->table_id = nla_get_u32(tb[IFLA_VRF_TABLE]);
		vi->vi_mask |= VRF_HAS_TABLE_ID;
	}

	err = 0;

errout:
	return err;
}

static void vrf_free(struct rtnl_link *link)
{
	free(link->l_info);
	link->l_info = NULL;
}

static int vrf_clone(struct rtnl_link *dst, struct rtnl_link *src)
{
	struct vrf_info *vdst, *vsrc = src->l_info;
	int err;

	dst->l_info = NULL;
	if ((err = rtnl_link_set_type(dst, "vrf")) < 0)
		return err;
	vdst = dst->l_info;

	BUG_ON(!vdst || !vsrc);

	memcpy(vdst, vsrc, sizeof(struct vrf_info));

	return 0;
}

static int vrf_put_attrs(struct nl_msg *msg, struct rtnl_link *link)
{
	struct vrf_info *vi = link->l_info;
	struct nlattr *data;

	if (!(data = nla_nest_start(msg, IFLA_INFO_DATA)))
		return -NLE_NOMEM;

	if (vi->vi_mask & VRF_HAS_TABLE_ID) {
		NLA_PUT_U32(msg, IFLA_VRF_TABLE, vi->table_id);
	}

	nla_nest_end(msg, data);

nla_put_failure:

	return 0;
}

static void vrf_dump(struct rtnl_link *link, struct nl_dump_params *p)
{
	struct vrf_info *vi = link->l_info;

	if (vi->vi_mask & VRF_HAS_TABLE_ID) {
		nl_dump(p, "table-id %u", vi->table_id);
	}
}

static struct rtnl_link_info_ops vrf_info_ops = {
	.io_name                = "vrf",
	.io_alloc               = vrf_alloc,
	.io_parse               = vrf_parse,
	.io_dump = {
		[NL_DUMP_LINE]     = vrf_dump,
		[NL_DUMP_DETAILS]  = vrf_dump,
	},
	.io_clone               = vrf_clone,
	.io_put_attrs           = vrf_put_attrs,
	.io_free                = vrf_free,
};

/** @cond SKIP */
#define IS_VRF_LINK_ASSERT(link) \
	if ((link)->l_info_ops != &vrf_info_ops) { \
		APPBUG("Link is not a VRF link. set type \"vrf\" first."); \
		return -NLE_OPNOTSUPP; \
	}
/** @endcond */

/**
 * @name VRF Object
 * @{
 */

/**
 * Allocate link object of type VRF
 *
 * @return Allocated link object or NULL.
 */
struct rtnl_link *rtnl_link_vrf_alloc(void)
{
	struct rtnl_link *link;

	if (!(link = rtnl_link_alloc()))
		return NULL;

	if (rtnl_link_set_type(link, "vrf") < 0) {
		rtnl_link_put(link);
		return NULL;
	}

	return link;
}

/**
 * Check if link is a VRF link
 * @arg link           Link object
 *
 * @return True if link is a VRF link, otherwise false is returned.
 */
int rtnl_link_is_vrf(struct rtnl_link *link)
{
	return link->l_info_ops && !strcmp(link->l_info_ops->io_name, "vrf");
}

/**
 * Get VRF table id
 * @arg link           Link object
 * @arg id             Pointer to store table identifier
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vrf_get_tableid(struct rtnl_link *link, uint32_t *id)
{
	struct vrf_info *vi = link->l_info;

	IS_VRF_LINK_ASSERT(link);
	if(!id)
		return -NLE_INVAL;

	if (vi->vi_mask & VRF_HAS_TABLE_ID)
		*id = vi->table_id;
	else
		return -NLE_AGAIN;

	return 0;
}

/**
 * Set VRF table id
 * @arg link           Link object
 * @arg id             Table identifier associated with VRF link
 *
 * @return 0 on success or a negative error code
 */
int rtnl_link_vrf_set_tableid(struct rtnl_link *link, uint32_t id)
{
	struct vrf_info *vi = link->l_info;

	IS_VRF_LINK_ASSERT(link);

	_NL_STATIC_ASSERT(VRF_TABLE_ID_MAX == UINT32_MAX);

	vi->table_id = id;
	vi->vi_mask |= VRF_HAS_TABLE_ID;

	return 0;
}

/** @} */

static void _nl_init vrf_init(void)
{
	rtnl_link_register_info(&vrf_info_ops);
}

static void _nl_exit vrf_exit(void)
{
	rtnl_link_unregister_info(&vrf_info_ops);
}

/** @} */
