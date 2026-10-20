/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#include "nl-default.h"

#include <netlink/idiag/meminfo.h>

#include "nl-idiag.h"

/**
 * @ingroup idiag
 * @defgroup idiagnl_meminfo Inet Diag Memory Info
 *
 * @details
 * @idiagnl_doc{idiagnl_meminfo, Inet Diag Memory Info Documentation}
 * @{
 */
struct idiagnl_meminfo *idiagnl_meminfo_alloc(void)
{
	return (struct idiagnl_meminfo *) nl_object_alloc(&idiagnl_meminfo_obj_ops);
}

void idiagnl_meminfo_get(struct idiagnl_meminfo *minfo)
{
	nl_object_get((struct nl_object *) minfo);
}

void idiagnl_meminfo_put(struct idiagnl_meminfo *minfo)
{
	nl_object_put((struct nl_object *) minfo);
}

/**
 * @name Attributes
 * @{
 */
uint32_t idiagnl_meminfo_get_rmem(const struct idiagnl_meminfo *minfo)
{
	return minfo->idiag_rmem;
}

void idiagnl_meminfo_set_rmem(struct idiagnl_meminfo *minfo, uint32_t rmem)
{
	minfo->idiag_rmem = rmem;
}

uint32_t idiagnl_meminfo_get_wmem(const struct idiagnl_meminfo *minfo)
{
	return minfo->idiag_wmem;
}

void idiagnl_meminfo_set_wmem(struct idiagnl_meminfo *minfo, uint32_t wmem)
{
	minfo->idiag_wmem = wmem;
}

uint32_t idiagnl_meminfo_get_fmem(const struct idiagnl_meminfo *minfo)
{
	return minfo->idiag_fmem;
}

void idiagnl_meminfo_set_fmem(struct idiagnl_meminfo *minfo, uint32_t fmem)
{
	minfo->idiag_fmem = fmem;
}

uint32_t idiagnl_meminfo_get_tmem(const struct idiagnl_meminfo *minfo)
{
	return minfo->idiag_tmem;
}

void idiagnl_meminfo_set_tmem(struct idiagnl_meminfo *minfo, uint32_t tmem)
{
	minfo->idiag_tmem = tmem;
}
/** @} */

/** @cond SKIP */
static uint64_t idiagnl_meminfo_compare(struct nl_object *_a, struct nl_object *_b,
                                     uint64_t attrs, int flags)
{
	struct idiagnl_meminfo *a = (struct idiagnl_meminfo *) _a;
	struct idiagnl_meminfo *b = (struct idiagnl_meminfo *) _b;

	/* meminfo is a very simple object. It has no attribe flags (ce_mask),
	 * hence compare just returns 0 or 1, not a bit mask of attributes. */
	return a->idiag_rmem != b->idiag_rmem ||
	       a->idiag_wmem != b->idiag_wmem ||
	       a->idiag_fmem != b->idiag_fmem ||
	       a->idiag_tmem != b->idiag_tmem;
}

struct nl_object_ops idiagnl_meminfo_obj_ops = {
	.oo_name	= "idiag/idiag_meminfo",
	.oo_size	= sizeof(struct idiagnl_meminfo),
	.oo_compare     = idiagnl_meminfo_compare,
};
/** @endcond */
/** @} */
