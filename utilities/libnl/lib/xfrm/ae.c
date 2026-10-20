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
 * @defgroup ae Attribute Element
 * @brief
 *
 * The AE interface allows a user to retrieve and update various
 * Security Association (SA) attributes such as lifetime, replay state etc.
 *
 * @par AE Flags
 * @code
 * XFRM_AE_UNSPEC
 * XFRM_AE_RTHR=1
 * XFRM_AE_RVAL=2
 * XFRM_AE_LVAL=4
 * XFRM_AE_ETHR=8
 * XFRM_AE_CR=16
 * XFRM_AE_CE=32
 * XFRM_AE_CU=64
 * @endcode
 *
 * @par AE Identification
 * An AE is uniquely identified by the attributes listed below, whenever
 * you refer to an existing AE all of the attributes must be set. There is
 * no cache support for AE since you can retrieve the AE for any given combination
 * of attributes mentioned below, but not all at once since they just characterize
 * an SA.
 *   - destination address (xfrmnl_ae_set_daddr())
 *   - SPI (xfrmnl_ae_set_spi)
 *   - protocol (xfrmnl_ae_set_proto)
 *   - mark (xfrmnl_ae_set_mark)
 *
 * @par Changeable Attributes
 * \anchor ae_changeable
 *  - current lifetime (xfrmnl_ae_set_curlifetime())
 *  - replay properties (xfrmnl_ae_set_replay_maxage(), xfrmnl_ae_set_replay_maxdiff())
 *  - replay state (xfrmnl_ae_set_replay_state(), xfrmnl_ae_set_replay_state_esn))
 *
 * @par Required Caches for Dumping
 * None
 *
 * @par TODO
 * None
 *
 * @par 1) Retrieving AE information for a given SA tuple
 * @code
 * // Create a netlink socket and connect it to XFRM subsystem in
 * the kernel to be able to send/receive info from userspace.
 * struct nl_sock* sk = nl_socket_alloc ();
 * nl_connect (sk, NETLINK_XFRM);
 *
 * // AEs can then be looked up by the SA tuple, destination address,
 * SPI, protocol, mark:
 * struct xfrmnl_ae *ae;
 * xfrmnl_ae_get_kernel(sk, dst_addr, spi, proto,mark_mask, mark_value, &ae);
 *
 * // After successful usage, the object must be freed
 * xfrmnl_ae_put(ae);
 * @endcode
 *
 * @par 2) Updating AE
 * @code
 * // Allocate an empty AE handle to be filled out with the attributes
 * // of the new AE.
 * struct xfrmnl_ae *ae = xfrmnl_ae_alloc();
 *
 * // Fill out the attributes of the new AE
 * xfrmnl_ae_set_daddr(ae, dst_addr);
 * xfrmnl_ae_set_spi(ae, 0xDEADBEEF);
 * xfrmnl_ae_set_proto(ae, 50);
 * xfrmnl_ae_set_mark(ae, 0x0);
 * xfrmnl_ae_set_saddr(ae, src_addr);
 * xfrmnl_ae_set_curlifetime(ae, 540, 10, 0xAABB1122, 0x0);
 *
 * // Build the netlink message and send it to the kernel, the operation will
 * // block until the operation has been completed. Alternatively, a netlink message
 * // can be built using xfrmnl_ae_build_get_request () API and be sent using
 * // nl_send_auto(). Further the result from the kernel can be parsed using
 * // xfrmnl_ae_parse() API.
 * xfrmnl_ae_set(sk, ae, NLM_F_REPLACE);
 *
 * // Free the memory
 * xfrmnl_ae_put(ae);
 * @endcode
 *
 * @{
 */

#include "nl-default.h"

#include <time.h>
#include <linux/xfrm.h>

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/object.h>
#include <netlink/xfrm/ae.h>

#include "nl-xfrm.h"
#include "nl-priv-dynamic-core/object-api.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-priv-dynamic-core/cache-api.h"
#include "nl-aux-core/nl-core.h"
#include "nl-aux-xfrm/nl-xfrm.h"

/** @cond SKIP */

struct xfrmnl_sa_id {
	struct nl_addr* daddr;
	uint32_t        spi;
	uint16_t        family;
	uint8_t         proto;
};

struct xfrmnl_ae {
	NLHDR_COMMON

	struct xfrmnl_sa_id             sa_id;
	struct nl_addr*                 saddr;
	uint32_t                        flags;
	uint32_t                        reqid;
	struct xfrmnl_mark              mark;
	struct xfrmnl_lifetime_cur      lifetime_cur;
	uint32_t                        replay_maxage;
	uint32_t                        replay_maxdiff;
	struct xfrmnl_replay_state      replay_state;
	struct xfrmnl_replay_state_esn* replay_state_esn;
};

#define XFRM_AE_ATTR_DADDR          0x01
#define XFRM_AE_ATTR_SPI            0x02
#define XFRM_AE_ATTR_PROTO          0x04
#define XFRM_AE_ATTR_SADDR          0x08
#define XFRM_AE_ATTR_FLAGS          0x10
#define XFRM_AE_ATTR_REQID          0x20
#define XFRM_AE_ATTR_MARK           0x40
#define XFRM_AE_ATTR_LIFETIME       0x80
#define XFRM_AE_ATTR_REPLAY_MAXAGE  0x100
#define XFRM_AE_ATTR_REPLAY_MAXDIFF 0x200
#define XFRM_AE_ATTR_REPLAY_STATE   0x400
#define XFRM_AE_ATTR_FAMILY         0x800

static struct nl_object_ops xfrm_ae_obj_ops;
/** @endcond */


static void xfrm_ae_free_data(struct nl_object *c)
{
	struct xfrmnl_ae* ae =   nl_object_priv (c);

	if (ae == NULL)
		return;

	nl_addr_put (ae->sa_id.daddr);
	nl_addr_put (ae->saddr);

	if (ae->replay_state_esn)
		free (ae->replay_state_esn);
}

static int xfrm_ae_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct xfrmnl_ae* dst = nl_object_priv(_dst);
	struct xfrmnl_ae* src = nl_object_priv(_src);

	dst->sa_id.daddr = NULL;
	dst->saddr = NULL;
	dst->replay_state_esn = NULL;

	if (src->sa_id.daddr) {
		if ((dst->sa_id.daddr = nl_addr_clone (src->sa_id.daddr)) == NULL)
			return -NLE_NOMEM;
	}

	if (src->saddr) {
		if ((dst->saddr = nl_addr_clone (src->saddr)) == NULL)
			return -NLE_NOMEM;
	}

	if (src->replay_state_esn) {
		uint32_t len = sizeof (struct xfrmnl_replay_state_esn) + (sizeof (uint32_t) * src->replay_state_esn->bmp_len);

		if ((dst->replay_state_esn = malloc (len)) == NULL)
			return -NLE_NOMEM;
		memcpy (dst->replay_state_esn, src->replay_state_esn, len);
	}

	return 0;
}

static uint64_t xfrm_ae_compare(struct nl_object *_a, struct nl_object *_b,
				uint64_t attrs, int flags)
{
	struct xfrmnl_ae* a  =   (struct xfrmnl_ae *) _a;
	struct xfrmnl_ae* b  =   (struct xfrmnl_ae *) _b;
	uint64_t diff = 0;
	int found = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(XFRM_AE_ATTR_DADDR,
		      nl_addr_cmp(a->sa_id.daddr, b->sa_id.daddr));
	diff |= _DIFF(XFRM_AE_ATTR_SPI, a->sa_id.spi != b->sa_id.spi);
	diff |= _DIFF(XFRM_AE_ATTR_PROTO, a->sa_id.proto != b->sa_id.proto);
	diff |= _DIFF(XFRM_AE_ATTR_SADDR, nl_addr_cmp(a->saddr, b->saddr));
	diff |= _DIFF(XFRM_AE_ATTR_FLAGS, a->flags != b->flags);
	diff |= _DIFF(XFRM_AE_ATTR_REQID, a->reqid != b->reqid);
	diff |= _DIFF(XFRM_AE_ATTR_MARK,
		      (a->mark.v & a->mark.m) != (b->mark.v & b->mark.m));
	diff |= _DIFF(XFRM_AE_ATTR_REPLAY_MAXAGE,
		      a->replay_maxage != b->replay_maxage);
	diff |= _DIFF(XFRM_AE_ATTR_REPLAY_MAXDIFF,
		      a->replay_maxdiff != b->replay_maxdiff);

	/* Compare replay states */
	found = AVAILABLE_MISMATCH (a, b, XFRM_AE_ATTR_REPLAY_STATE);
	if (found == 0) // attribute exists in both objects
	{
		if (((a->replay_state_esn != NULL) && (b->replay_state_esn == NULL)) ||
			((a->replay_state_esn == NULL) && (b->replay_state_esn != NULL)))
			found |= 1;

		if (found == 0) // same replay type. compare actual values
		{
			if (a->replay_state_esn)
			{
				if (a->replay_state_esn->bmp_len != b->replay_state_esn->bmp_len)
					diff |= 1;
				else
				{
					uint32_t len = sizeof (struct xfrmnl_replay_state_esn) + (sizeof (uint32_t) * a->replay_state_esn->bmp_len);
					diff |= memcmp (a->replay_state_esn, b->replay_state_esn, len);
				}
			}
			else
			{
				if ((a->replay_state.oseq != b->replay_state.oseq) ||
				    (a->replay_state.seq != b->replay_state.seq) ||
				    (a->replay_state.bitmap != b->replay_state.bitmap))
					diff |= 1;
			}
		}
	}
#undef _DIFF

	return diff;
}

/**
 * @name XFRM AE Attribute Translations
 * @{
 */
static const struct trans_tbl ae_attrs[] =
{
	__ADD(XFRM_AE_ATTR_DADDR, daddr),
	__ADD(XFRM_AE_ATTR_SPI, spi),
	__ADD(XFRM_AE_ATTR_PROTO, protocol),
	__ADD(XFRM_AE_ATTR_SADDR, saddr),
	__ADD(XFRM_AE_ATTR_FLAGS, flags),
	__ADD(XFRM_AE_ATTR_REQID, reqid),
	__ADD(XFRM_AE_ATTR_MARK, mark),
	__ADD(XFRM_AE_ATTR_LIFETIME, cur_lifetime),
	__ADD(XFRM_AE_ATTR_REPLAY_MAXAGE, replay_maxage),
	__ADD(XFRM_AE_ATTR_REPLAY_MAXDIFF, replay_maxdiff),
	__ADD(XFRM_AE_ATTR_REPLAY_STATE, replay_state),
};

static char* xfrm_ae_attrs2str (int attrs, char *buf, size_t len)
{
	return __flags2str(attrs, buf, len, ae_attrs, ARRAY_SIZE(ae_attrs));
}
/** @} */

/**
 * @name XFRM AE Flags Translations
 * @{
 */

static const struct trans_tbl ae_flags[] = {
	__ADD(XFRM_AE_UNSPEC, unspecified),
	__ADD(XFRM_AE_RTHR, replay threshold),
	__ADD(XFRM_AE_RVAL, replay value),
	__ADD(XFRM_AE_LVAL, lifetime value),
	__ADD(XFRM_AE_ETHR, expiry time threshold),
	__ADD(XFRM_AE_CR, replay update event),
	__ADD(XFRM_AE_CE, timer expiry event),
	__ADD(XFRM_AE_CU, policy update event),
};

char* xfrmnl_ae_flags2str(int flags, char *buf, size_t len)
{
	return __flags2str (flags, buf, len, ae_flags, ARRAY_SIZE(ae_flags));
}

int xfrmnl_ae_str2flag(const char *name)
{
	return __str2flags(name, ae_flags, ARRAY_SIZE(ae_flags));
}
/** @} */

static void xfrm_ae_dump_line(struct nl_object *a, struct nl_dump_params *p)
{
	char                dst[INET6_ADDRSTRLEN+5], src[INET6_ADDRSTRLEN+5];
	struct xfrmnl_ae*   ae  =   (struct xfrmnl_ae *) a;
	char                flags[128], buf[128];
	time_t              add_time, use_time;
	struct tm           *add_time_tm, *use_time_tm;
	struct tm           tm_buf;

	nl_dump_line(p, "src %s dst %s \n", nl_addr2str(ae->saddr, src, sizeof(src)),
				nl_addr2str(ae->sa_id.daddr, dst, sizeof(dst)));

	nl_dump_line(p, "\tproto %s spi 0x%x reqid %u ",
				nl_ip_proto2str (ae->sa_id.proto, buf, sizeof (buf)),
				ae->sa_id.spi, ae->reqid);

	xfrmnl_ae_flags2str(ae->flags, flags, sizeof (flags));
	nl_dump_line(p, "flags %s(0x%x) mark mask/value 0x%x/0x%x \n", flags,
				ae->flags, ae->mark.m, ae->mark.v);

	nl_dump_line(p, "\tlifetime current: \n");
	nl_dump_line(p, "\t\tbytes %llu packets %llu \n",
		     (long long unsigned)ae->lifetime_cur.bytes,
		     (long long unsigned)ae->lifetime_cur.packets);
	if (ae->lifetime_cur.add_time != 0)
	{
		add_time = ae->lifetime_cur.add_time;
		add_time_tm = gmtime_r (&add_time, &tm_buf);
		strftime (flags, 128, "%Y-%m-%d %H-%M-%S", add_time_tm);
	}
	else
	{
		sprintf (flags, "%s", "-");
	}

	if (ae->lifetime_cur.use_time != 0)
	{
		use_time = ae->lifetime_cur.use_time;
		use_time_tm = gmtime_r (&use_time, &tm_buf);
		strftime (buf, 128, "%Y-%m-%d %H-%M-%S", use_time_tm);
	}
	else
	{
		sprintf (buf, "%s", "-");
	}
	nl_dump_line(p, "\t\tadd_time: %s, use_time: %s\n", flags, buf);

	nl_dump_line(p, "\treplay info: \n");
	nl_dump_line(p, "\t\tmax age %u max diff %u \n", ae->replay_maxage, ae->replay_maxdiff);

	nl_dump_line(p, "\treplay state info: \n");
	if (ae->replay_state_esn)
	{
		nl_dump_line(p, "\t\toseq %u seq %u oseq_hi %u seq_hi %u replay window: %u \n",
					ae->replay_state_esn->oseq, ae->replay_state_esn->seq,
					ae->replay_state_esn->oseq_hi, ae->replay_state_esn->seq_hi,
					ae->replay_state_esn->replay_window);
	}
	else
	{
		nl_dump_line(p, "\t\toseq %u seq %u bitmap: %u \n", ae->replay_state.oseq,
					ae->replay_state.seq, ae->replay_state.bitmap);
	}

	nl_dump(p, "\n");
}

static void xfrm_ae_dump_details(struct nl_object *a, struct nl_dump_params *p)
{
	xfrm_ae_dump_line(a, p);
}

static void xfrm_ae_dump_stats(struct nl_object *a, struct nl_dump_params *p)
{
	xfrm_ae_dump_details(a, p);
}


static int build_xfrm_ae_message(struct xfrmnl_ae *tmpl, int cmd, int flags,
			   struct nl_msg **result)
{
	struct nl_msg*          msg;
	struct xfrm_aevent_id   ae_id;

	if (!(tmpl->ce_mask & XFRM_AE_ATTR_DADDR) ||
		!(tmpl->ce_mask & XFRM_AE_ATTR_SPI) ||
		!(tmpl->ce_mask & XFRM_AE_ATTR_PROTO))
		return -NLE_MISSING_ATTR;

	memset(&ae_id, 0, sizeof(ae_id));

	memcpy (&ae_id.sa_id.daddr, nl_addr_get_binary_addr (tmpl->sa_id.daddr), sizeof (uint8_t) * nl_addr_get_len (tmpl->sa_id.daddr));
	ae_id.sa_id.spi    = htonl(tmpl->sa_id.spi);
	ae_id.sa_id.family = tmpl->sa_id.family;
	ae_id.sa_id.proto  = tmpl->sa_id.proto;

	if (tmpl->ce_mask & XFRM_AE_ATTR_SADDR)
		memcpy (&ae_id.saddr, nl_addr_get_binary_addr (tmpl->saddr), sizeof (uint8_t) * nl_addr_get_len (tmpl->saddr));

	if (tmpl->ce_mask & XFRM_AE_ATTR_FLAGS)
		ae_id.flags    = tmpl->flags;

	if (tmpl->ce_mask & XFRM_AE_ATTR_REQID)
		ae_id.reqid    = tmpl->reqid;

	msg = nlmsg_alloc_simple(cmd, flags);
	if (!msg)
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &ae_id, sizeof(ae_id), NLMSG_ALIGNTO) < 0)
		goto nla_put_failure;

	if (tmpl->ce_mask & XFRM_AE_ATTR_MARK)
		NLA_PUT (msg, XFRMA_MARK, sizeof (struct xfrmnl_mark), &tmpl->mark);

	if (tmpl->ce_mask & XFRM_AE_ATTR_LIFETIME)
		NLA_PUT (msg, XFRMA_LTIME_VAL, sizeof (struct xfrmnl_lifetime_cur), &tmpl->lifetime_cur);

	if (tmpl->ce_mask & XFRM_AE_ATTR_REPLAY_MAXAGE)
		NLA_PUT_U32 (msg, XFRMA_ETIMER_THRESH, tmpl->replay_maxage);

	if (tmpl->ce_mask & XFRM_AE_ATTR_REPLAY_MAXDIFF)
		NLA_PUT_U32 (msg, XFRMA_REPLAY_THRESH, tmpl->replay_maxdiff);

	if (tmpl->ce_mask & XFRM_AE_ATTR_REPLAY_STATE) {
		if (tmpl->replay_state_esn) {
			uint32_t len = sizeof (struct xfrm_replay_state_esn) + (sizeof (uint32_t) * tmpl->replay_state_esn->bmp_len);
			NLA_PUT (msg, XFRMA_REPLAY_ESN_VAL, len, tmpl->replay_state_esn);
		}
		else {
			NLA_PUT (msg, XFRMA_REPLAY_VAL, sizeof (struct xfrmnl_replay_state), &tmpl->replay_state);
		}
	}

	*result = msg;
	return 0;

nla_put_failure:
	nlmsg_free(msg);
	return -NLE_MSGSIZE;
}

/**
 * @name XFRM AE Update
 * @{
 */

int xfrmnl_ae_set(struct nl_sock* sk, struct xfrmnl_ae* ae, int flags)
{
	int err;
	struct nl_msg *msg;

	if ((err = build_xfrm_ae_message(ae, XFRM_MSG_NEWAE, flags|NLM_F_REPLACE, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return nl_wait_for_ack(sk);
}

/** @} */

/**
 * @name XFRM AE Object Allocation/Freeage
 * @{
 */

struct xfrmnl_ae* xfrmnl_ae_alloc(void)
{
	return (struct xfrmnl_ae*) nl_object_alloc(&xfrm_ae_obj_ops);
}

void xfrmnl_ae_put(struct xfrmnl_ae* ae)
{
	nl_object_put((struct nl_object *) ae);
}

/** @} */

static struct nla_policy xfrm_ae_policy[XFRMA_MAX+1] = {
	[XFRMA_LTIME_VAL]       = { .minlen = sizeof(struct xfrm_lifetime_cur) },
	[XFRMA_REPLAY_VAL]      = { .minlen = sizeof(struct xfrm_replay_state) },
	[XFRMA_REPLAY_THRESH]   = { .type = NLA_U32 },
	[XFRMA_ETIMER_THRESH]   = { .type = NLA_U32 },
	[XFRMA_SRCADDR]         = { .minlen = sizeof(xfrm_address_t) },
	[XFRMA_MARK]            = { .minlen = sizeof(struct xfrm_mark) },
	[XFRMA_REPLAY_ESN_VAL]  = { .minlen = sizeof(struct xfrm_replay_state_esn) },
};

int xfrmnl_ae_parse(struct nlmsghdr *n, struct xfrmnl_ae **result)
{
	_nl_auto_xfrmnl_ae struct xfrmnl_ae *ae = NULL;
	struct nlattr           *tb[XFRMA_MAX + 1];
	struct xfrm_aevent_id*  ae_id;
	int err;

	ae = xfrmnl_ae_alloc();
	if (!ae)
		return -NLE_NOMEM;

	ae->ce_msgtype = n->nlmsg_type;
	ae_id = nlmsg_data(n);

	err = nlmsg_parse(n, sizeof(struct xfrm_aevent_id), tb, XFRMA_MAX, xfrm_ae_policy);
	if (err < 0)
		return err;

	if (!(ae->sa_id.daddr =
		      _nl_addr_build(ae_id->sa_id.family, &ae_id->sa_id.daddr)))
		return -NLE_NOMEM;
	ae->sa_id.family= ae_id->sa_id.family;
	ae->sa_id.spi   = ntohl(ae_id->sa_id.spi);
	ae->sa_id.proto = ae_id->sa_id.proto;
	if (!(ae->saddr = _nl_addr_build(ae_id->sa_id.family, &ae_id->saddr)))
		return -NLE_NOMEM;
	ae->reqid       = ae_id->reqid;
	ae->flags       = ae_id->flags;
	ae->ce_mask |= (XFRM_AE_ATTR_DADDR | XFRM_AE_ATTR_FAMILY | XFRM_AE_ATTR_SPI |
					XFRM_AE_ATTR_PROTO | XFRM_AE_ATTR_SADDR | XFRM_AE_ATTR_REQID |
					XFRM_AE_ATTR_FLAGS);

	if (tb[XFRMA_MARK]) {
		struct xfrm_mark* m =   nla_data(tb[XFRMA_MARK]);
		ae->mark.m  =   m->m;
		ae->mark.v  =   m->v;
		ae->ce_mask |= XFRM_AE_ATTR_MARK;
	}

	if (tb[XFRMA_LTIME_VAL]) {
		struct xfrm_lifetime_cur* cur =   nla_data(tb[XFRMA_LTIME_VAL]);

		ae->lifetime_cur.bytes      =   cur->bytes;
		ae->lifetime_cur.packets    =   cur->packets;
		ae->lifetime_cur.add_time   =   cur->add_time;
		ae->lifetime_cur.use_time   =   cur->use_time;
		ae->ce_mask |= XFRM_AE_ATTR_LIFETIME;
	}

	if (tb[XFRM_AE_ETHR]) {
		ae->replay_maxage       =   *(uint32_t*)nla_data(tb[XFRM_AE_ETHR]);
		ae->ce_mask |= XFRM_AE_ATTR_REPLAY_MAXAGE;
	}

	if (tb[XFRM_AE_RTHR]) {
		ae->replay_maxdiff      =   *(uint32_t*)nla_data(tb[XFRM_AE_RTHR]);
		ae->ce_mask |= XFRM_AE_ATTR_REPLAY_MAXDIFF;
	}

	if (tb[XFRMA_REPLAY_ESN_VAL]) {
		struct xfrm_replay_state_esn* esn =  nla_data (tb[XFRMA_REPLAY_ESN_VAL]);
		uint32_t len = sizeof (struct xfrmnl_replay_state_esn) +  (sizeof (uint32_t) * esn->bmp_len);

		if ((ae->replay_state_esn = calloc (1, len)) == NULL)
			return -NLE_NOMEM;
		ae->replay_state_esn->oseq       =  esn->oseq;
		ae->replay_state_esn->seq        =  esn->seq;
		ae->replay_state_esn->oseq_hi    =  esn->oseq_hi;
		ae->replay_state_esn->seq_hi     =  esn->seq_hi;
		ae->replay_state_esn->replay_window   =   esn->replay_window;
		ae->replay_state_esn->bmp_len    =   esn->bmp_len;
		memcpy (ae->replay_state_esn->bmp, esn->bmp, sizeof (uint32_t) * esn->bmp_len);
		ae->ce_mask |= XFRM_AE_ATTR_REPLAY_STATE;
	}
	else
	{
		struct xfrm_replay_state* replay_state = nla_data (tb[XFRMA_REPLAY_VAL]);
		ae->replay_state.oseq       =   replay_state->oseq;
		ae->replay_state.seq        =   replay_state->seq;
		ae->replay_state.bitmap     =   replay_state->bitmap;
		ae->ce_mask |= XFRM_AE_ATTR_REPLAY_STATE;

		ae->replay_state_esn = NULL;
	}

	*result = _nl_steal_pointer(&ae);
	return 0;
}

static int xfrm_ae_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
				struct nlmsghdr *n, struct nl_parser_param *pp)
{
	struct xfrmnl_ae*    ae;
	int err;

	if ((err = xfrmnl_ae_parse(n, &ae)) < 0)
		return err;

	err = pp->pp_cb((struct nl_object *) ae, pp);

	xfrmnl_ae_put(ae);
	return err;
}

/**
 * @name XFRM AE Get
 * @{
 */

int xfrmnl_ae_build_get_request(struct nl_addr* daddr, unsigned int spi, unsigned int protocol,
                                unsigned int mark_mask, unsigned int mark_value, struct nl_msg **result)
{
	struct nl_msg *msg;
	struct xfrm_aevent_id   ae_id;
	struct xfrmnl_mark   mark;

	if (!daddr || !spi)
	{
		fprintf(stderr, "APPLICATION BUG: %s:%d:%s: A valid destination address, spi must be specified\n",
				__FILE__, __LINE__, __func__);
		assert(0);
		return -NLE_MISSING_ATTR;
	}

	memset(&ae_id, 0, sizeof(ae_id));
	memcpy (&ae_id.sa_id.daddr, nl_addr_get_binary_addr (daddr), sizeof (uint8_t) * nl_addr_get_len (daddr));
	ae_id.sa_id.spi    = htonl(spi);
	ae_id.sa_id.family = nl_addr_get_family (daddr);
	ae_id.sa_id.proto  = protocol;

	if (!(msg = nlmsg_alloc_simple(XFRM_MSG_GETAE, 0)))
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &ae_id, sizeof(ae_id), NLMSG_ALIGNTO) < 0)
		goto nla_put_failure;

	mark.m  =   mark_mask;
	mark.v  =   mark_value;
	NLA_PUT (msg, XFRMA_MARK, sizeof (struct xfrmnl_mark), &mark);

	*result = msg;
	return 0;

nla_put_failure:
	nlmsg_free(msg);
	return -NLE_MSGSIZE;
}

int xfrmnl_ae_get_kernel(struct nl_sock* sock, struct nl_addr* daddr, unsigned int spi, unsigned int protocol,
                         unsigned int mark_mask, unsigned int mark_value, struct xfrmnl_ae** result)
{
	struct nl_msg *msg = NULL;
	struct nl_object *obj;
	int err;

	if ((err = xfrmnl_ae_build_get_request(daddr, spi, protocol, mark_mask, mark_value, &msg)) < 0)
		return err;

	err = nl_send_auto(sock, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	if ((err = nl_pickup(sock, &xfrm_ae_msg_parser, &obj)) < 0)
		return err;

	/* We have used xfrm_ae_msg_parser(), object is definitely a xfrm ae */
	*result = (struct xfrmnl_ae *) obj;

	/* If an object has been returned, we also need to wait for the ACK */
	if (err == 0 && obj)
		nl_wait_for_ack(sock);

	return 0;
}

/** @} */

/**
 * @name Attributes
 * @{
 */

static inline int __assign_addr(struct xfrmnl_ae* ae, struct nl_addr **pos,
					struct nl_addr *new, int flag, int nocheck)
{
	if (!nocheck) {
		if (ae->ce_mask & XFRM_AE_ATTR_FAMILY) {
			if (nl_addr_get_family (new) != ae->sa_id.family)
				return -NLE_AF_MISMATCH;
		} else {
			ae->sa_id.family = nl_addr_get_family (new);
			ae->ce_mask |= XFRM_AE_ATTR_FAMILY;
		}
	}

	if (*pos)
		nl_addr_put(*pos);

	nl_addr_get(new);
	*pos = new;

	ae->ce_mask |= flag;

	return 0;
}


struct nl_addr* xfrmnl_ae_get_daddr (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_DADDR)
		return ae->sa_id.daddr;
	else
		return NULL;
}

int xfrmnl_ae_set_daddr (struct xfrmnl_ae* ae, struct nl_addr* addr)
{
	return __assign_addr(ae, &ae->sa_id.daddr, addr, XFRM_AE_ATTR_DADDR, 0);
}

int xfrmnl_ae_get_spi (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_SPI)
		return ae->sa_id.spi;
	else
		return -1;
}

int xfrmnl_ae_set_spi (struct xfrmnl_ae* ae, unsigned int spi)
{
	ae->sa_id.spi = spi;
	ae->ce_mask |= XFRM_AE_ATTR_SPI;

	return 0;
}

int xfrmnl_ae_get_family (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_FAMILY)
		return ae->sa_id.family;
	else
		return -1;
}

int xfrmnl_ae_set_family (struct xfrmnl_ae* ae, unsigned int family)
{
	ae->sa_id.family = family;
	ae->ce_mask |= XFRM_AE_ATTR_FAMILY;

	return 0;
}

int xfrmnl_ae_get_proto (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_PROTO)
		return ae->sa_id.proto;
	else
		return -1;
}

int xfrmnl_ae_set_proto (struct xfrmnl_ae* ae, unsigned int protocol)
{
	ae->sa_id.proto = protocol;
	ae->ce_mask |= XFRM_AE_ATTR_PROTO;

	return 0;
}

struct nl_addr* xfrmnl_ae_get_saddr (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_SADDR)
		return ae->saddr;
	else
		return NULL;
}

int xfrmnl_ae_set_saddr (struct xfrmnl_ae* ae, struct nl_addr* addr)
{
	return 	__assign_addr(ae, &ae->saddr, addr, XFRM_AE_ATTR_SADDR, 1);
}

int xfrmnl_ae_get_flags (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_FLAGS)
		return ae->flags;
	else
		return -1;
}

int xfrmnl_ae_set_flags (struct xfrmnl_ae* ae, unsigned int flags)
{
	ae->flags = flags;
	ae->ce_mask |= XFRM_AE_ATTR_FLAGS;

	return 0;
}

int xfrmnl_ae_get_reqid (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_REQID)
		return ae->reqid;
	else
		return -1;
}

int xfrmnl_ae_set_reqid (struct xfrmnl_ae* ae, unsigned int reqid)
{
	ae->reqid = reqid;
	ae->ce_mask |= XFRM_AE_ATTR_REQID;

	return 0;
}

int xfrmnl_ae_get_mark (struct xfrmnl_ae* ae, unsigned int* mark_mask, unsigned int* mark_value)
{
	if (mark_mask == NULL || mark_value == NULL)
		return -1;

	if (ae->ce_mask & XFRM_AE_ATTR_MARK)
	{
		*mark_mask  =   ae->mark.m;
		*mark_value  =   ae->mark.v;

		return 0;
	}
	else
		return -1;
}

int xfrmnl_ae_set_mark (struct xfrmnl_ae* ae, unsigned int value, unsigned int mask)
{
	ae->mark.v  = value;
	ae->mark.m  = mask;
	ae->ce_mask |= XFRM_AE_ATTR_MARK;

	return 0;
}

int xfrmnl_ae_get_curlifetime (struct xfrmnl_ae* ae, unsigned long long int* curr_bytes,
                               unsigned long long int* curr_packets, unsigned long long int* curr_add_time,
                               unsigned long long int* curr_use_time)
{
	if (curr_bytes == NULL || curr_packets == NULL || curr_add_time == NULL || curr_use_time == NULL)
		return -1;

	if (ae->ce_mask & XFRM_AE_ATTR_LIFETIME)
	{
		*curr_bytes     =   ae->lifetime_cur.bytes;
		*curr_packets   =   ae->lifetime_cur.packets;
		*curr_add_time  =   ae->lifetime_cur.add_time;
		*curr_use_time  =   ae->lifetime_cur.use_time;

		return 0;
	}
	else
		return -1;
}

int xfrmnl_ae_set_curlifetime (struct xfrmnl_ae* ae, unsigned long long int curr_bytes,
                               unsigned long long int curr_packets, unsigned long long int curr_add_time,
                               unsigned long long int curr_use_time)
{
	ae->lifetime_cur.bytes = curr_bytes;
	ae->lifetime_cur.packets = curr_packets;
	ae->lifetime_cur.add_time = curr_add_time;
	ae->lifetime_cur.use_time = curr_use_time;
	ae->ce_mask |= XFRM_AE_ATTR_LIFETIME;

	return 0;
}

int xfrmnl_ae_get_replay_maxage (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_REPLAY_MAXAGE)
		return ae->replay_maxage;
	else
		return -1;
}

int xfrmnl_ae_set_replay_maxage (struct xfrmnl_ae* ae, unsigned int replay_maxage)
{
	ae->replay_maxage  = replay_maxage;
	ae->ce_mask |= XFRM_AE_ATTR_REPLAY_MAXAGE;

	return 0;
}

int xfrmnl_ae_get_replay_maxdiff (struct xfrmnl_ae* ae)
{
	if (ae->ce_mask & XFRM_AE_ATTR_REPLAY_MAXDIFF)
		return ae->replay_maxdiff;
	else
		return -1;
}

int xfrmnl_ae_set_replay_maxdiff (struct xfrmnl_ae* ae, unsigned int replay_maxdiff)
{
	ae->replay_maxdiff  = replay_maxdiff;
	ae->ce_mask |= XFRM_AE_ATTR_REPLAY_MAXDIFF;

	return 0;
}

int xfrmnl_ae_get_replay_state (struct xfrmnl_ae* ae, unsigned int* oseq, unsigned int* seq, unsigned int* bmp)
{
	if (ae->ce_mask & XFRM_AE_ATTR_REPLAY_STATE)
	{
		if (ae->replay_state_esn == NULL)
		{
			*oseq   =   ae->replay_state.oseq;
			*seq    =   ae->replay_state.seq;
			*bmp    =   ae->replay_state.bitmap;

			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
		return -1;
}

int xfrmnl_ae_set_replay_state (struct xfrmnl_ae* ae, unsigned int oseq, unsigned int seq, unsigned int bitmap)
{
	ae->replay_state.oseq = oseq;
	ae->replay_state.seq = seq;
	ae->replay_state.bitmap = bitmap;
	ae->ce_mask |= XFRM_AE_ATTR_REPLAY_STATE;

	return 0;
}

int xfrmnl_ae_get_replay_state_esn(struct xfrmnl_ae* ae, unsigned int* oseq, unsigned int* seq, unsigned int* oseq_hi,
                                   unsigned int* seq_hi, unsigned int* replay_window, unsigned int* bmp_len, unsigned int* bmp)
{
	if (ae->ce_mask & XFRM_AE_ATTR_REPLAY_STATE)
	{
		if (ae->replay_state_esn)
		{
			*oseq   =   ae->replay_state_esn->oseq;
			*seq    =   ae->replay_state_esn->seq;
			*oseq_hi=   ae->replay_state_esn->oseq_hi;
			*seq_hi =   ae->replay_state_esn->seq_hi;
			*replay_window  =   ae->replay_state_esn->replay_window;
			*bmp_len        =   ae->replay_state_esn->bmp_len; // In number of 32 bit words
			memcpy (bmp, ae->replay_state_esn->bmp, ae->replay_state_esn->bmp_len * sizeof (uint32_t));

			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
		return -1;
}

int xfrmnl_ae_set_replay_state_esn(struct xfrmnl_ae* ae, unsigned int oseq, unsigned int seq,
                                   unsigned int oseq_hi, unsigned int seq_hi, unsigned int replay_window,
                                   unsigned int bmp_len, unsigned int* bmp)
{
	/* Free the old replay ESN state and allocate new one */
	if (ae->replay_state_esn)
		free (ae->replay_state_esn);

	if ((ae->replay_state_esn = calloc (1, sizeof (struct xfrmnl_replay_state_esn) + sizeof (uint32_t) * bmp_len)) == NULL)
		return -1;

	ae->replay_state_esn->oseq = oseq;
	ae->replay_state_esn->seq = seq;
	ae->replay_state_esn->oseq_hi = oseq_hi;
	ae->replay_state_esn->seq_hi = seq_hi;
	ae->replay_state_esn->replay_window = replay_window;
	ae->replay_state_esn->bmp_len = bmp_len; // In number of 32 bit words
	memcpy (ae->replay_state_esn->bmp, bmp, bmp_len * sizeof (uint32_t));
	ae->ce_mask |= XFRM_AE_ATTR_REPLAY_STATE;

	return 0;
}

/** @} */

static struct nl_object_ops xfrm_ae_obj_ops = {
	.oo_name        =   "xfrm/ae",
	.oo_size        =   sizeof(struct xfrmnl_ae),
	.oo_free_data   =   xfrm_ae_free_data,
	.oo_clone       =   xfrm_ae_clone,
	.oo_dump        =   {
	                        [NL_DUMP_LINE]      =   xfrm_ae_dump_line,
	                        [NL_DUMP_DETAILS]   =   xfrm_ae_dump_details,
	                        [NL_DUMP_STATS]     =   xfrm_ae_dump_stats,
	                    },
	.oo_compare     =   xfrm_ae_compare,
	.oo_attrs2str   =   xfrm_ae_attrs2str,
	.oo_id_attrs    =   (XFRM_AE_ATTR_DADDR | XFRM_AE_ATTR_SPI | XFRM_AE_ATTR_PROTO),
};

/** @} */

