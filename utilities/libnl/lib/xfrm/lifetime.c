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
 * @defgroup XFRM Lifetime Configuration Object
 *
 * Abstract data type representing XFRM SA lifetime properties
 *
 * @{
 *
 * Header
 * ------
 * ~~~~{.c}
 * #include <netlink/xfrm/lifetime.h>
 * ~~~~
 */

#include "nl-default.h"

#include <netlink/xfrm/lifetime.h>

#include "nl-xfrm.h"

static void ltime_cfg_destroy(struct xfrmnl_ltime_cfg* ltime)
{
	if (!ltime)
		return;

	if (ltime->refcnt != 1)
	{
		fprintf(stderr, "BUG: %s:%d\n", __FILE__, __LINE__);
		assert(0);
	}

	free(ltime);
}

/**
 * @name Creating Selector
 * @{
 */

/**
 * Allocate new lifetime config object.
 * @return Newly allocated lifetime config object or NULL
 */
struct xfrmnl_ltime_cfg* xfrmnl_ltime_cfg_alloc()
{
	struct xfrmnl_ltime_cfg* ltime;

	ltime = calloc(1, sizeof(struct xfrmnl_ltime_cfg));
	if (!ltime)
		return NULL;

	ltime->refcnt = 1;

	return ltime;
}

/**
 * Clone existing lifetime config object.
 * @arg ltime		Selector object.
 * @return Newly allocated lifetime config object being a duplicate of the
 *         specified lifetime config object or NULL if a failure occured.
 */
struct xfrmnl_ltime_cfg* xfrmnl_ltime_cfg_clone(struct xfrmnl_ltime_cfg* ltime)
{
	struct xfrmnl_ltime_cfg* new;

	new = xfrmnl_ltime_cfg_alloc();
	if (new)
		memcpy ((void*)new, (void*)ltime, sizeof (struct xfrmnl_ltime_cfg));

	return new;
}

/** @} */

/**
 * @name Managing Usage References
 * @{
 */

struct xfrmnl_ltime_cfg* xfrmnl_ltime_cfg_get(struct xfrmnl_ltime_cfg* ltime)
{
	ltime->refcnt++;

	return ltime;
}

void xfrmnl_ltime_cfg_put(struct xfrmnl_ltime_cfg* ltime)
{
	if (!ltime)
		return;

	if (ltime->refcnt == 1)
		ltime_cfg_destroy(ltime);
	else
		ltime->refcnt--;
}

/**
 * Check whether an lifetime config object is shared.
 * @arg addr		Selector object.
 * @return Non-zero if the lifetime config object is shared, otherwise 0.
 */
int xfrmnl_ltime_cfg_shared(struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->refcnt > 1;
}

/** @} */

/**
 * @name Miscellaneous
 * @{
 */

/**
 * Compares two lifetime config objects.
 * @arg a		A lifetime config object.
 * @arg b		Another lifetime config object.
 *
 * @return Non zero if difference is found, 0 otherwise if both
 * the objects are identical.
 */
int xfrmnl_ltime_cfg_cmp(struct xfrmnl_ltime_cfg* a, struct xfrmnl_ltime_cfg* b)
{
	/* Check for any differences */
	if ((a->soft_byte_limit != b->soft_byte_limit) ||
		(a->soft_packet_limit != b->soft_packet_limit) ||
		(a->hard_byte_limit != b->hard_byte_limit) ||
		(a->hard_packet_limit != b->hard_packet_limit) ||
		(a->soft_add_expires_seconds != b->soft_add_expires_seconds) ||
		(a->hard_add_expires_seconds != b->hard_add_expires_seconds) ||
		(a->soft_use_expires_seconds != b->soft_use_expires_seconds) ||
		(a->hard_use_expires_seconds != b->hard_use_expires_seconds))
		return 1;

	/* The objects are identical */
	return 0;
}

/** @} */

/**
 * @name Attributes
 * @{
 */
unsigned long long xfrmnl_ltime_cfg_get_soft_bytelimit (struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->soft_byte_limit;
}

int xfrmnl_ltime_cfg_set_soft_bytelimit (struct xfrmnl_ltime_cfg* ltime, unsigned long long soft_byte_limit)
{
	ltime->soft_byte_limit = soft_byte_limit;

	return 0;
}

unsigned long long xfrmnl_ltime_cfg_get_hard_bytelimit (struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->hard_byte_limit;
}

int xfrmnl_ltime_cfg_set_hard_bytelimit (struct xfrmnl_ltime_cfg* ltime, unsigned long long hard_byte_limit)
{
	ltime->hard_byte_limit = hard_byte_limit;

	return 0;
}

unsigned long long xfrmnl_ltime_cfg_get_soft_packetlimit (struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->soft_packet_limit;
}

int xfrmnl_ltime_cfg_set_soft_packetlimit (struct xfrmnl_ltime_cfg* ltime, unsigned long long soft_packet_limit)
{
	ltime->soft_packet_limit = soft_packet_limit;

	return 0;
}

unsigned long long xfrmnl_ltime_cfg_get_hard_packetlimit (struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->hard_packet_limit;
}

int xfrmnl_ltime_cfg_set_hard_packetlimit (struct xfrmnl_ltime_cfg* ltime, unsigned long long hard_packet_limit)
{
	ltime->hard_packet_limit = hard_packet_limit;

	return 0;
}

unsigned long long xfrmnl_ltime_cfg_get_soft_addexpires (struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->soft_add_expires_seconds;
}

int xfrmnl_ltime_cfg_set_soft_addexpires (struct xfrmnl_ltime_cfg* ltime, unsigned long long soft_add_expires_seconds)
{
	ltime->soft_add_expires_seconds = soft_add_expires_seconds;

	return 0;
}

unsigned long long xfrmnl_ltime_cfg_get_hard_addexpires (struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->hard_add_expires_seconds;
}

int xfrmnl_ltime_cfg_set_hard_addexpires (struct xfrmnl_ltime_cfg* ltime, unsigned long long hard_add_expires_seconds)
{
	ltime->hard_add_expires_seconds = hard_add_expires_seconds;

	return 0;
}

unsigned long long xfrmnl_ltime_cfg_get_soft_useexpires (struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->soft_use_expires_seconds;
}

int xfrmnl_ltime_cfg_set_soft_useexpires (struct xfrmnl_ltime_cfg* ltime, unsigned long long soft_use_expires_seconds)
{
	ltime->soft_use_expires_seconds = soft_use_expires_seconds;

	return 0;
}

unsigned long long xfrmnl_ltime_cfg_get_hard_useexpires (struct xfrmnl_ltime_cfg* ltime)
{
	return ltime->hard_use_expires_seconds;
}

int xfrmnl_ltime_cfg_set_hard_useexpires (struct xfrmnl_ltime_cfg* ltime, unsigned long long hard_use_expires_seconds)
{
	ltime->hard_use_expires_seconds = hard_use_expires_seconds;

	return 0;
}

/** @} */
