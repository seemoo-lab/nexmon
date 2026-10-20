/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_GENL_PRIV_H_
#define NETLINK_GENL_PRIV_H_

#include <netlink/netlink.h>

#include "nl-priv-dynamic-core/object-api.h"
#include "nl-priv-dynamic-core/cache-api.h"

#define GENL_HDRSIZE(hdrlen) (GENL_HDRLEN + (hdrlen))

/*****************************************************************************/

/* WARNING: this symbol is wrongly exported in libnl-genl-3.sym. */
extern int		genl_resolve_id(struct genl_ops *ops);

/*****************************************************************************/

#define GENL_OP_HAS_POLICY	1
#define GENL_OP_HAS_DOIT	2
#define GENL_OP_HAS_DUMPIT	4

struct genl_family_grp {
        struct genl_family      *family;        /* private */
        struct nl_list_head     list;           /* private */
        char                    name[GENL_NAMSIZ];
        u_int32_t               id;
};

struct genl_family
{
	NLHDR_COMMON

	uint16_t		gf_id;
	char 			gf_name[GENL_NAMSIZ];
	uint32_t		gf_version;
	uint32_t		gf_hdrsize;
	uint32_t		gf_maxattr;

	struct nl_list_head	gf_ops;
	struct nl_list_head	gf_mc_grps;
};

#endif
