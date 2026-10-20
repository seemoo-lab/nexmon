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
 * @defgroup XFRM User Template Object
 *
 * Abstract data type representing XFRM SA properties
 *
 * @{
 *
 * Header
 * ------
 * ~~~~{.c}
 * #include <netlink/xfrm/template.h>
 * ~~~~
 */

#include "nl-default.h"

#include <netlink/xfrm/template.h>

#include "nl-xfrm.h"
#include "nl-priv-dynamic-core/nl-core.h"

void xfrmnl_user_tmpl_free(struct xfrmnl_user_tmpl* utmpl)
{
	if (!utmpl)
		return;

	nl_addr_put (utmpl->id.daddr);
	nl_addr_put (utmpl->saddr);
	free(utmpl);
}

/**
 * @name Creating User Template Object
 * @{
 */

/**
 * Allocate new user template object.
 * @return Newly allocated user template object or NULL
 */
struct xfrmnl_user_tmpl* xfrmnl_user_tmpl_alloc()
{
	struct xfrmnl_user_tmpl* utmpl;

	utmpl = calloc(1, sizeof(struct xfrmnl_user_tmpl));
	if (!utmpl)
		return NULL;

	nl_init_list_head(&utmpl->utmpl_list);

	return utmpl;
}

/**
 * Clone existing user template object.
 * @arg utmpl		Selector object.
 * @return Newly allocated user template object being a duplicate of the
 *         specified user template object or NULL if a failure occured.
 */
struct xfrmnl_user_tmpl* xfrmnl_user_tmpl_clone(struct xfrmnl_user_tmpl* utmpl)
{
	struct xfrmnl_user_tmpl* new;

	new = xfrmnl_user_tmpl_alloc();
	if (!new)
		return NULL;

	memcpy(new, utmpl, sizeof(struct xfrmnl_user_tmpl));
	new->id.daddr = nl_addr_clone (utmpl->id.daddr);
	new->saddr    = nl_addr_clone (utmpl->saddr);

	return new;
}

/** @} */

/**
 * @name XFRM Template Mode Translations
 * @{
 */
static const struct trans_tbl tmpl_modes[] = {
	__ADD(XFRM_MODE_TRANSPORT, transport),
	__ADD(XFRM_MODE_TUNNEL, tunnel),
	__ADD(XFRM_MODE_ROUTEOPTIMIZATION, route optimization),
	__ADD(XFRM_MODE_IN_TRIGGER, in trigger),
	__ADD(XFRM_MODE_BEET, beet),
};

char* xfrmnl_user_tmpl_mode2str(int mode, char *buf, size_t len)
{
	return __type2str (mode, buf, len, tmpl_modes, ARRAY_SIZE(tmpl_modes));
}

int xfrmnl_user_tmpl_str2mode(const char *name)
{
	return __str2type (name, tmpl_modes, ARRAY_SIZE(tmpl_modes));
}
/** @} */

/**
 * @name Miscellaneous
 * @{
 */

/**
 * Compares two user template objects.
 * @arg a		A user template object.
 * @arg b		Another user template object.
 *
 * @return Non zero if difference is found, 0 otherwise if both
 * the objects are identical.
 */
int xfrmnl_user_tmpl_cmp(struct xfrmnl_user_tmpl* a, struct xfrmnl_user_tmpl* b)
{
	/* Check for any differences */
	if ((nl_addr_cmp_prefix (a->id.daddr, b->id.daddr) != 0) ||
	    (a->id.spi != b->id.spi) ||
	    (a->id.proto && (a->id.proto != b->id.proto)) ||
	    (nl_addr_cmp_prefix (a->saddr, b->saddr) != 0) ||
	    (a->family != b->family) ||
	    (a->reqid != b->reqid) ||
	    (a->mode != b->mode) ||
	    (a->share != b->share) ||
	    (a->aalgos != b->aalgos) ||
	    (a->ealgos != b->ealgos) ||
	    (a->calgos != b->calgos))
		return 1;

	/* The objects are identical */
	return 0;
}

void xfrmnl_user_tmpl_dump(struct xfrmnl_user_tmpl* tmpl, struct nl_dump_params *p)
{
	char    dst[INET6_ADDRSTRLEN+5], src[INET6_ADDRSTRLEN+5];
	char    buf [128];

	nl_dump_line(p, "\t\tsrc %s dst %s family: %s \n",
	             nl_addr2str(tmpl->saddr, src, sizeof(src)),
	             nl_addr2str (tmpl->id.daddr, dst, sizeof (dst)),
	             nl_af2str (tmpl->family, buf, 128));
	nl_dump_line (p, "\t\tprotocol: %s spi: 0x%x reqid: %u mode: %s\n",
	              nl_ip_proto2str (tmpl->id.proto, buf, sizeof(buf)),
	              tmpl->id.spi, tmpl->reqid,
	              xfrmnl_user_tmpl_mode2str (tmpl->mode, buf, 128));
	nl_dump_line (p, "\t\tAuth Algo: 0x%x Crypto Algo: 0x%x Compr Algo: 0x%x\n",
	              tmpl->aalgos, tmpl->ealgos, tmpl->calgos);

	return;
}

/** @} */

/**
 * @name Attributes
 * @{
 */
struct nl_addr* xfrmnl_user_tmpl_get_daddr (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->id.daddr;
}

int xfrmnl_user_tmpl_set_daddr (struct xfrmnl_user_tmpl* utmpl, struct nl_addr* addr)
{
	/* Increment reference counter on this to keep this address
	 * object around while user template in use */
	nl_addr_get(addr);

	utmpl->id.daddr = addr;

	return 0;
}

int xfrmnl_user_tmpl_get_spi (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->id.spi;
}

int xfrmnl_user_tmpl_set_spi (struct xfrmnl_user_tmpl* utmpl, unsigned int spi)
{
	utmpl->id.spi = spi;

	return 0;
}

int xfrmnl_user_tmpl_get_proto (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->id.proto;
}

int xfrmnl_user_tmpl_set_proto (struct xfrmnl_user_tmpl* utmpl, unsigned int protocol)
{
	utmpl->id.proto = protocol;

	return 0;
}

int xfrmnl_user_tmpl_get_family(struct xfrmnl_user_tmpl *utmpl)
{
	return utmpl->family;
}

int xfrmnl_user_tmpl_set_family(struct xfrmnl_user_tmpl *utmpl, unsigned int family)
{
	utmpl->family = family;

	return 0;
}

struct nl_addr* xfrmnl_user_tmpl_get_saddr (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->saddr;
}

int xfrmnl_user_tmpl_set_saddr (struct xfrmnl_user_tmpl* utmpl, struct nl_addr* addr)
{
	/* Increment reference counter on this to keep this address
	 * object around while user template in use */
	nl_addr_get(addr);

	utmpl->saddr = addr;

	return 0;
}

int xfrmnl_user_tmpl_get_reqid (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->reqid;
}

int xfrmnl_user_tmpl_set_reqid (struct xfrmnl_user_tmpl* utmpl, unsigned int reqid)
{
	utmpl->reqid = reqid;

	return 0;
}

int xfrmnl_user_tmpl_get_mode (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->mode;
}

int xfrmnl_user_tmpl_set_mode (struct xfrmnl_user_tmpl* utmpl, unsigned int mode)
{
	utmpl->mode = mode;

	return 0;
}

int xfrmnl_user_tmpl_get_share (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->share;
}

int xfrmnl_user_tmpl_set_share (struct xfrmnl_user_tmpl* utmpl, unsigned int share)
{
	utmpl->share = share;

	return 0;
}

int xfrmnl_user_tmpl_get_optional (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->optional;
}

int xfrmnl_user_tmpl_set_optional (struct xfrmnl_user_tmpl* utmpl, unsigned int optional)
{
	utmpl->optional = optional;

	return 0;
}

int xfrmnl_user_tmpl_get_aalgos (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->aalgos;
}

int xfrmnl_user_tmpl_set_aalgos (struct xfrmnl_user_tmpl* utmpl, unsigned int aalgos)
{
	utmpl->aalgos = aalgos;

	return 0;
}

int xfrmnl_user_tmpl_get_ealgos (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->ealgos;
}

int xfrmnl_user_tmpl_set_ealgos (struct xfrmnl_user_tmpl* utmpl, unsigned int ealgos)
{
	utmpl->ealgos = ealgos;

	return 0;
}

int xfrmnl_user_tmpl_get_calgos (struct xfrmnl_user_tmpl* utmpl)
{
	return utmpl->calgos;
}

int xfrmnl_user_tmpl_set_calgos (struct xfrmnl_user_tmpl* utmpl, unsigned int calgos)
{
	utmpl->calgos = calgos;

	return 0;
}

/** @} */
