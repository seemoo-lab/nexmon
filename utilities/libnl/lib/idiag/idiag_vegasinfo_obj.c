/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#include "nl-default.h"

#include <netlink/idiag/vegasinfo.h>

#include "nl-idiag.h"

/**
 * @ingroup idiag
 * @defgroup idiagnl_vegasinfo Inet Diag TCP Vegas Info
 *
 * @details
 * @idiagnl_doc{idiagnl_vegasinfo, Inet Diag TCP Vegas Info Documentation}
 * @{
 */
struct idiagnl_vegasinfo *idiagnl_vegasinfo_alloc(void)
{
	return (struct idiagnl_vegasinfo *) nl_object_alloc(&idiagnl_vegasinfo_obj_ops);
}

void idiagnl_vegasinfo_get(struct idiagnl_vegasinfo *vinfo)
{
	nl_object_get((struct nl_object *) vinfo);
}

void idiagnl_vegasinfo_put(struct idiagnl_vegasinfo *vinfo)
{
	nl_object_put((struct nl_object *) vinfo);
}

/**
 * @name Attributes
 * @{
 */
uint32_t idiagnl_vegasinfo_get_enabled(const struct idiagnl_vegasinfo *vinfo)
{
	return vinfo->tcpv_enabled;
}

void idiagnl_vegasinfo_set_enabled(struct idiagnl_vegasinfo *vinfo, uint32_t
		enabled)
{
	vinfo->tcpv_enabled = enabled;
}

uint32_t idiagnl_vegasinfo_get_rttcnt(const struct idiagnl_vegasinfo *vinfo)
{
	return vinfo->tcpv_rttcnt;
}

void idiagnl_vegasinfo_set_rttcnt(struct idiagnl_vegasinfo *vinfo, uint32_t
		rttcnt)
{
	vinfo->tcpv_rttcnt = rttcnt;
}

uint32_t idiagnl_vegasinfo_get_rtt(const struct idiagnl_vegasinfo *vinfo)
{
	return vinfo->tcpv_rtt;
}

void idiagnl_vegasinfo_set_rtt(struct idiagnl_vegasinfo *vinfo, uint32_t rtt)
{
	vinfo->tcpv_rtt = rtt;
}

uint32_t idiagnl_vegasinfo_get_minrtt(const struct idiagnl_vegasinfo *vinfo)
{
	return vinfo->tcpv_minrtt;
}

void idiagnl_vegasinfo_set_minrtt(struct idiagnl_vegasinfo *vinfo, uint32_t
		minrtt)
{
	vinfo->tcpv_minrtt = minrtt;
}
/** @} */

/** @cond SKIP */
static uint64_t idiagnl_vegasinfo_compare(struct nl_object *_a, struct nl_object *_b,
                                          uint64_t attrs, int flags)
{
	struct idiagnl_vegasinfo *a = (struct idiagnl_vegasinfo *) _a;
	struct idiagnl_vegasinfo *b = (struct idiagnl_vegasinfo *) _b;

	/* vegasinfo is a very simple object. It has no attribe flags (ce_mask),
	 * hence compare just returns 0 or 1, not a bit mask of attributes. */
	return a->tcpv_enabled != b->tcpv_enabled ||
	       a->tcpv_rttcnt != b->tcpv_rttcnt ||
	       a->tcpv_rtt != b->tcpv_rtt ||
	       a->tcpv_minrtt != b->tcpv_minrtt;
}

struct nl_object_ops idiagnl_vegasinfo_obj_ops = {
	.oo_name	= "idiag/idiag_vegasinfo",
	.oo_size	= sizeof(struct idiagnl_vegasinfo),
	.oo_compare     = idiagnl_vegasinfo_compare,
};
/** @endcond */
/** @} */
