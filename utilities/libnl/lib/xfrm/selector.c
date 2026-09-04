/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @ingroup xfrmnl
 * @defgroup XFRM Address Selector
 *
 * Abstract data type representing XFRM SA/SP selector properties
 *
 * @{
 *
 * Header
 * ------
 * ~~~~{.c}
 * #include <netlink/xfrm/selector.h>
 * ~~~~
 */

#include "nl-default.h"

#include <netlink/xfrm/selector.h>

/* Selector, used as selector both on policy rules (SPD) and SAs. */
struct xfrmnl_sel {
	uint32_t        refcnt;
	struct nl_addr* daddr;
	struct nl_addr* saddr;
	uint16_t        dport;
	uint16_t        dport_mask;
	uint16_t        sport;
	uint16_t        sport_mask;
	uint16_t        family;
	uint8_t         prefixlen_d;
	uint8_t         prefixlen_s;
	uint8_t         proto;
	int32_t         ifindex;
	uint32_t        user;
};

static void sel_destroy(struct xfrmnl_sel* sel)
{
	if (!sel)
		return;

	if (sel->refcnt != 1)
	{
		fprintf(stderr, "BUG: %s:%d\n", __FILE__, __LINE__);
		assert(0);
	}

	nl_addr_put (sel->daddr);
	nl_addr_put (sel->saddr);
	free(sel);
}

/**
 * @name Creating Selector
 * @{
 */

/**
 * Allocate new selector object.
 * @return Newly allocated selector object or NULL
 */
struct xfrmnl_sel* xfrmnl_sel_alloc()
{
	struct xfrmnl_sel* sel;

	sel = calloc(1, sizeof(struct xfrmnl_sel));
	if (!sel)
		return NULL;

	sel->refcnt = 1;

	return sel;
}

/**
 * Clone existing selector object.
 * @arg sel		Selector object.
 * @return Newly allocated selector object being a duplicate of the
 *         specified selector object or NULL if a failure occured.
 */
struct xfrmnl_sel* xfrmnl_sel_clone(struct xfrmnl_sel* sel)
{
	struct xfrmnl_sel* new;

	new = xfrmnl_sel_alloc();
	if (!new)
		return NULL;

	memcpy(new, sel, sizeof(struct xfrmnl_sel));
	new->daddr = nl_addr_clone(sel->daddr);
	new->saddr = nl_addr_clone(sel->saddr);

	return new;
}

/** @} */

/**
 * @name Managing Usage References
 * @{
 */

struct xfrmnl_sel* xfrmnl_sel_get(struct xfrmnl_sel* sel)
{
	sel->refcnt++;

	return sel;
}

void xfrmnl_sel_put(struct xfrmnl_sel* sel)
{
	if (!sel)
		return;

	if (sel->refcnt == 1)
		sel_destroy(sel);
	else
		sel->refcnt--;
}

/**
 * Check whether an selector object is shared.
 * @arg addr		Selector object.
 * @return Non-zero if the selector object is shared, otherwise 0.
 */
int xfrmnl_sel_shared(struct xfrmnl_sel* sel)
{
	return sel->refcnt > 1;
}

/** @} */

/**
 * @name Miscellaneous
 * @{
 */

/**
 * Compares two selector objects.
 * @arg a		A selector object.
 * @arg b		Another selector object.
 *
 * @return Non zero if difference is found, 0 otherwise if both
 * the objects are identical.
 */
int xfrmnl_sel_cmp(struct xfrmnl_sel* a, struct xfrmnl_sel* b)
{
	/* Check for any differences */
	if ((nl_addr_cmp_prefix (a->daddr, b->daddr) != 0) ||
	    (nl_addr_cmp_prefix (a->saddr, b->saddr) != 0) ||
	    ((a->sport & a->sport_mask) != (b->sport & b->sport_mask)) ||
	    ((a->dport & a->dport_mask) != (b->dport & b->dport_mask)) ||
	    (a->family != b->family) ||
	    (a->proto && (a->proto != b->proto)) ||
	    (a->ifindex && a->ifindex != b->ifindex) ||
	    (a->user != b->user))
		return 1;

	/* The objects are identical */
	return 0;
}

void xfrmnl_sel_dump(struct xfrmnl_sel* sel, struct nl_dump_params *p)
{
	char    dst[INET6_ADDRSTRLEN+5], src[INET6_ADDRSTRLEN+5];
	char    buf [128];

	nl_dump_line(p, "\t\tsrc %s dst %s family: %s\n", nl_addr2str(sel->saddr, src, sizeof(src)),
	              nl_addr2str (sel->daddr, dst, sizeof (dst)), nl_af2str (sel->family, buf, 128));
	nl_dump_line (p, "\t\tsrc port/mask: %d/%d dst port/mask: %d/%d\n",
	              sel->dport, sel->dport_mask, sel->sport, sel->sport_mask);
	nl_dump_line (p, "\t\tprotocol: %s ifindex: %u user: %u\n",
	              nl_ip_proto2str (sel->proto, buf, sizeof(buf)), sel->ifindex, sel->user);

	return;
}


/** @} */

/**
 * @name Attributes
 * @{
 */
struct nl_addr* xfrmnl_sel_get_daddr (struct xfrmnl_sel* sel)
{
	return sel->daddr;
}

int xfrmnl_sel_set_daddr (struct xfrmnl_sel* sel, struct nl_addr* addr)
{
	/* Increment reference counter on this to keep this address
	 * object around while selector in use */
	nl_addr_get(addr);

	sel->daddr = addr;

	return 0;
}

struct nl_addr* xfrmnl_sel_get_saddr (struct xfrmnl_sel* sel)
{
	return sel->saddr;
}

int xfrmnl_sel_set_saddr (struct xfrmnl_sel* sel, struct nl_addr* addr)
{
	/* Increment reference counter on this to keep this address
	 * object around while selector in use */
	nl_addr_get(addr);

	sel->saddr = addr;

	return 0;
}

int xfrmnl_sel_get_dport (struct xfrmnl_sel* sel)
{
	return sel->dport;
}

int xfrmnl_sel_set_dport (struct xfrmnl_sel* sel, unsigned int dport)
{
	sel->dport = dport;

	return 0;
}

int xfrmnl_sel_get_dportmask (struct xfrmnl_sel* sel)
{
	return sel->dport_mask;
}

int xfrmnl_sel_set_dportmask (struct xfrmnl_sel* sel, unsigned int dport_mask)
{
	sel->dport_mask = dport_mask;

	return 0;
}

int xfrmnl_sel_get_sport (struct xfrmnl_sel* sel)
{
	return sel->sport;
}

int xfrmnl_sel_set_sport (struct xfrmnl_sel* sel, unsigned int sport)
{
	sel->sport = sport;

	return 0;
}

int xfrmnl_sel_get_sportmask (struct xfrmnl_sel* sel)
{
	return sel->sport_mask;
}

int xfrmnl_sel_set_sportmask (struct xfrmnl_sel* sel, unsigned int sport_mask)
{
	sel->sport_mask = sport_mask;

	return 0;
}

int xfrmnl_sel_get_family(struct xfrmnl_sel *sel)
{
	return sel->family;
}

int xfrmnl_sel_set_family(struct xfrmnl_sel *sel, unsigned int family)
{
	sel->family = family;

	return 0;
}

int xfrmnl_sel_get_prefixlen_d (struct xfrmnl_sel* sel)
{
	return sel->prefixlen_d;
}

int xfrmnl_sel_set_prefixlen_d (struct xfrmnl_sel* sel, unsigned int prefixlen)
{
	sel->prefixlen_d = prefixlen;

	return 0;
}

int xfrmnl_sel_get_prefixlen_s (struct xfrmnl_sel* sel)
{
	return sel->prefixlen_s;
}

int xfrmnl_sel_set_prefixlen_s (struct xfrmnl_sel* sel, unsigned int prefixlen)
{
	sel->prefixlen_s = prefixlen;

	return 0;
}

int xfrmnl_sel_get_proto (struct xfrmnl_sel* sel)
{
	return sel->proto;
}

int xfrmnl_sel_set_proto (struct xfrmnl_sel* sel, unsigned int protocol)
{
	sel->proto = protocol;

	return 0;
}

int xfrmnl_sel_get_ifindex (struct xfrmnl_sel* sel)
{
	return sel->ifindex;
}

int xfrmnl_sel_set_ifindex (struct xfrmnl_sel* sel, unsigned int ifindex)
{
	sel->ifindex = ifindex;

	return 0;
}

int xfrmnl_sel_get_userid (struct xfrmnl_sel* sel)
{
	return sel->user;
}

int xfrmnl_sel_set_userid (struct xfrmnl_sel* sel, unsigned int userid)
{
	sel->user   = userid;
	return 0;
}


/** @} */
