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
 * @defgroup sp Security Policy
 * @brief
 */

#include "nl-default.h"

#include <time.h>
#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/object.h>
#include <netlink/xfrm/selector.h>
#include <netlink/xfrm/lifetime.h>
#include <netlink/xfrm/template.h>
#include <netlink/xfrm/sp.h>

#include "nl-xfrm.h"
#include "nl-priv-dynamic-core/object-api.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-priv-dynamic-core/cache-api.h"
#include "nl-aux-core/nl-core.h"
#include "nl-aux-xfrm/nl-xfrm.h"

struct xfrmnl_userpolicy_type {
	uint8_t                         type;
	uint16_t                        reserved1;
	uint16_t                        reserved2;
};

struct xfrmnl_sp {
	NLHDR_COMMON

	struct xfrmnl_sel*              sel;
	struct xfrmnl_ltime_cfg*        lft;
	struct xfrmnl_lifetime_cur      curlft;
	uint32_t                        priority;
	uint32_t                        index;
	uint8_t                         dir;
	uint8_t                         action;
	uint8_t                         flags;
	uint8_t                         share;
	struct xfrmnl_user_sec_ctx*     sec_ctx;
	struct xfrmnl_userpolicy_type   uptype;
	uint32_t                        nr_user_tmpl;
	struct nl_list_head             usertmpl_list;
	struct xfrmnl_mark              mark;
	uint32_t                        if_id;
};

/** @cond SKIP */
#define XFRM_SP_ATTR_SEL            0x01
#define XFRM_SP_ATTR_LTIME_CFG      0x02
#define XFRM_SP_ATTR_LTIME_CUR      0x04
#define XFRM_SP_ATTR_PRIO           0x08
#define XFRM_SP_ATTR_INDEX          0x10
#define XFRM_SP_ATTR_DIR            0x20
#define XFRM_SP_ATTR_ACTION         0x40
#define XFRM_SP_ATTR_FLAGS          0x80
#define XFRM_SP_ATTR_SHARE          0x100
#define XFRM_SP_ATTR_POLTYPE        0x200
#define XFRM_SP_ATTR_SECCTX         0x400
#define XFRM_SP_ATTR_TMPL           0x800
#define XFRM_SP_ATTR_MARK           0x1000
#define XFRM_SP_ATTR_IF_ID          0x2000

static struct nl_cache_ops  xfrmnl_sp_ops;
static struct nl_object_ops xfrm_sp_obj_ops;
/** @endcond */

static void xfrm_sp_alloc_data(struct nl_object *c)
{
	struct xfrmnl_sp* sp =   nl_object_priv (c);

	if ((sp->sel = xfrmnl_sel_alloc ()) == NULL)
		return;

	if ((sp->lft = xfrmnl_ltime_cfg_alloc ()) == NULL)
		return;

	nl_init_list_head(&sp->usertmpl_list);

	return;
}

static void xfrm_sp_free_data(struct nl_object *c)
{
	struct xfrmnl_sp* sp =   nl_object_priv (c);
	struct xfrmnl_user_tmpl *utmpl, *tmp;

	if (sp == NULL)
		return;

	xfrmnl_sel_put (sp->sel);
	xfrmnl_ltime_cfg_put (sp->lft);

	if (sp->sec_ctx) {
		free(sp->sec_ctx);
	}

	nl_list_for_each_entry_safe(utmpl, tmp, &sp->usertmpl_list, utmpl_list) {
		xfrmnl_sp_remove_usertemplate (sp, utmpl);
		xfrmnl_user_tmpl_free (utmpl);
	}
}

static int xfrm_sp_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct xfrmnl_sp*        dst = nl_object_priv(_dst);
	struct xfrmnl_sp*        src = nl_object_priv(_src);
	struct xfrmnl_user_tmpl *utmpl;
	struct xfrmnl_user_tmpl *new;

	dst->sel = NULL;
	dst->lft = NULL;
	dst->sec_ctx = NULL;
	nl_init_list_head(&dst->usertmpl_list);

	if (src->sel) {
		if ((dst->sel = xfrmnl_sel_clone (src->sel)) == NULL)
			return -NLE_NOMEM;
	}

	if (src->lft) {
		if ((dst->lft = xfrmnl_ltime_cfg_clone (src->lft)) == NULL)
			return -NLE_NOMEM;
	}

	if (src->sec_ctx) {
		uint32_t len =   sizeof (struct xfrmnl_user_sec_ctx) + src->sec_ctx->ctx_len;

		if ((dst->sec_ctx = malloc (len)) == NULL)
			return -NLE_NOMEM;
		memcpy(dst->sec_ctx, src->sec_ctx, len);
	}

	nl_list_for_each_entry(utmpl, &src->usertmpl_list, utmpl_list) {
		new = xfrmnl_user_tmpl_clone (utmpl);
		if (!new)
			return -NLE_NOMEM;
		xfrmnl_sp_add_usertemplate(dst, new);
	}

	return 0;
}

static uint64_t xfrm_sp_compare(struct nl_object *_a, struct nl_object *_b,
				uint64_t attrs, int flags)
{
	struct xfrmnl_sp* a  =   (struct xfrmnl_sp *) _a;
	struct xfrmnl_sp* b  =   (struct xfrmnl_sp *) _b;
	struct xfrmnl_user_tmpl *tmpl_a, *tmpl_b;
	uint64_t diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(XFRM_SP_ATTR_SEL, xfrmnl_sel_cmp(a->sel, b->sel));
	diff |= _DIFF(XFRM_SP_ATTR_LTIME_CFG,
		      xfrmnl_ltime_cfg_cmp(a->lft, b->lft));
	diff |= _DIFF(XFRM_SP_ATTR_PRIO, a->priority != b->priority);
	diff |= _DIFF(XFRM_SP_ATTR_INDEX, a->index != b->index);
	diff |= _DIFF(XFRM_SP_ATTR_DIR, a->dir != b->dir);
	diff |= _DIFF(XFRM_SP_ATTR_ACTION, a->action != b->action);
	diff |= _DIFF(XFRM_SP_ATTR_FLAGS, a->flags != b->flags);
	diff |= _DIFF(XFRM_SP_ATTR_SHARE, a->share != b->share);
	diff |= _DIFF(XFRM_SP_ATTR_SECCTX,
		      ((a->sec_ctx->len != b->sec_ctx->len) ||
		       (a->sec_ctx->exttype != b->sec_ctx->exttype) ||
		       (a->sec_ctx->ctx_alg != b->sec_ctx->ctx_alg) ||
		       (a->sec_ctx->ctx_doi != b->sec_ctx->ctx_doi) ||
		       (a->sec_ctx->ctx_len != b->sec_ctx->ctx_len) ||
		       strcmp(a->sec_ctx->ctx, b->sec_ctx->ctx)));
	diff |= _DIFF(XFRM_SP_ATTR_POLTYPE, (a->uptype.type != b->uptype.type));
	diff |= _DIFF(XFRM_SP_ATTR_TMPL, (a->nr_user_tmpl != b->nr_user_tmpl));
	diff |= _DIFF(XFRM_SP_ATTR_MARK,
		      (a->mark.m != b->mark.m) || (a->mark.v != b->mark.v));
	diff |= _DIFF(XFRM_SP_ATTR_IF_ID, a->if_id != b->if_id);

	/* Compare the templates */
	nl_list_for_each_entry(tmpl_b, &b->usertmpl_list, utmpl_list)
	nl_list_for_each_entry(tmpl_a, &a->usertmpl_list, utmpl_list)
	diff |= xfrmnl_user_tmpl_cmp (tmpl_a, tmpl_b);
#undef _DIFF

	return diff;
}

/**
 * @name XFRM SP Attribute Translations
 * @{
 */
static const struct trans_tbl sp_attrs[] = {
	__ADD(XFRM_SP_ATTR_SEL, selector),
	__ADD(XFRM_SP_ATTR_LTIME_CFG, lifetime_cfg),
	__ADD(XFRM_SP_ATTR_LTIME_CUR, lifetime_cur),
	__ADD(XFRM_SP_ATTR_PRIO, priority),
	__ADD(XFRM_SP_ATTR_INDEX, index),
	__ADD(XFRM_SP_ATTR_DIR, direction),
	__ADD(XFRM_SP_ATTR_ACTION, action),
	__ADD(XFRM_SP_ATTR_FLAGS, flags),
	__ADD(XFRM_SP_ATTR_SHARE, share),
	__ADD(XFRM_SP_ATTR_POLTYPE, policy_type),
	__ADD(XFRM_SP_ATTR_SECCTX, security_context),
	__ADD(XFRM_SP_ATTR_TMPL, user_template),
	__ADD(XFRM_SP_ATTR_MARK, mark),
	__ADD(XFRM_SP_ATTR_IF_ID, if_id),
};

static char* xfrm_sp_attrs2str(int attrs, char *buf, size_t len)
{
	return __flags2str (attrs, buf, len, sp_attrs, ARRAY_SIZE(sp_attrs));
}
/** @} */

/**
 * @name XFRM SP Action Translations
 * @{
 */
static const struct trans_tbl sa_actions[] = {
	__ADD(XFRM_POLICY_ALLOW, allow),
	__ADD(XFRM_POLICY_BLOCK, block),
};

char* xfrmnl_sp_action2str(int action, char *buf, size_t len)
{
	return __type2str (action, buf, len, sa_actions, ARRAY_SIZE(sa_actions));
}

int xfrmnl_sp_str2action(const char *name)
{
	return __str2type (name, sa_actions, ARRAY_SIZE(sa_actions));
}
/** @} */

/**
 * @name XFRM SP Flags Translations
 * @{
 */
static const struct trans_tbl sp_flags[] = {
	__ADD(XFRM_POLICY_LOCALOK, allow policy override by user),
	__ADD(XFRM_POLICY_ICMP, auto include ICMP in policy),
};

char* xfrmnl_sp_flags2str(int flags, char *buf, size_t len)
{
	return __flags2str (flags, buf, len, sp_flags, ARRAY_SIZE(sp_flags));
}

int xfrmnl_sp_str2flag(const char *name)
{
	return __str2flags(name, sp_flags, ARRAY_SIZE(sp_flags));
}
/** @} */

/**
 * @name XFRM SP Type Translations
 * @{
 */
static const struct trans_tbl sp_types[] = {
	__ADD(XFRM_POLICY_TYPE_MAIN, main),
	__ADD(XFRM_POLICY_TYPE_SUB, sub),
	__ADD(XFRM_POLICY_TYPE_MAX, max),
	__ADD(XFRM_POLICY_TYPE_ANY, any),
};

char* xfrmnl_sp_type2str(int type, char *buf, size_t len)
{
	return __type2str(type, buf, len, sp_types, ARRAY_SIZE(sp_types));
}

int xfrmnl_sp_str2type(const char *name)
{
	return __str2type(name, sp_types, ARRAY_SIZE(sp_types));
}
/** @} */

/**
 * @name XFRM SP Direction Translations
 * @{
 */
static const struct trans_tbl sp_dir[] = {
	__ADD(XFRM_POLICY_IN, in),
	__ADD(XFRM_POLICY_OUT, out),
	__ADD(XFRM_POLICY_FWD, fwd),
	__ADD(XFRM_POLICY_MASK, mask),
};

char* xfrmnl_sp_dir2str(int dir, char *buf, size_t len)
{
	return __type2str (dir, buf, len, sp_dir, ARRAY_SIZE(sp_dir));
}

int xfrmnl_sp_str2dir(const char *name)
{
	return __str2type (name, sp_dir, ARRAY_SIZE(sp_dir));
}

int xfrmnl_sp_index2dir (unsigned int index)
{
	return index & 0x7;
}
/** @} */

/**
 * @name XFRM SP Share Translations
 * @{
 */
static const struct trans_tbl sp_share[] = {
	__ADD(XFRM_SHARE_ANY, any),
	__ADD(XFRM_SHARE_SESSION, session),
	__ADD(XFRM_SHARE_USER, user),
	__ADD(XFRM_SHARE_UNIQUE, unique),
};

char* xfrmnl_sp_share2str(int share, char *buf, size_t len)
{
	return __type2str (share, buf, len, sp_share, ARRAY_SIZE(sp_share));
}

int xfrmnl_sp_str2share(const char *name)
{
	return __str2type (name, sp_share, ARRAY_SIZE(sp_share));
}
/** @} */

static void xfrm_sp_dump_line(struct nl_object *a, struct nl_dump_params *p)
{
	struct xfrmnl_sp*   sp  =   (struct xfrmnl_sp *) a;
	char                dir[32], action[32], share[32], flags[32];
	char                dst[INET6_ADDRSTRLEN+5], src[INET6_ADDRSTRLEN+5];
	time_t              add_time, use_time;
	struct tm           *add_time_tm, *use_time_tm;
	struct tm           tm_buf;

	nl_addr2str(xfrmnl_sel_get_saddr (sp->sel), src, sizeof(src));
	nl_addr2str (xfrmnl_sel_get_daddr (sp->sel), dst, sizeof (dst));
	nl_af2str (xfrmnl_sel_get_family (sp->sel), dir, 32);
	nl_dump_line(p, "src %s dst %s family: %s\n", src, dst, dir);
	nl_dump_line (p, "src port/mask: %d/%d dst port/mask: %d/%d\n",
	              xfrmnl_sel_get_dport (sp->sel), xfrmnl_sel_get_dportmask (sp->sel),
	              xfrmnl_sel_get_sport (sp->sel), xfrmnl_sel_get_sportmask (sp->sel));
	nl_dump_line (p, "protocol: %s ifindex: %u uid: %u\n",
	              nl_ip_proto2str (xfrmnl_sel_get_proto (sp->sel), dir, sizeof(dir)),
	              xfrmnl_sel_get_ifindex (sp->sel),
	              xfrmnl_sel_get_userid (sp->sel));

	xfrmnl_sp_dir2str (sp->dir, dir, 32);
	xfrmnl_sp_action2str (sp->action, action, 32);
	xfrmnl_sp_share2str (sp->share, share, 32);
	xfrmnl_sp_flags2str (sp->flags, flags, 32);
	nl_dump_line(p, "\tdir: %s action: %s index: %u priority: %u share: %s flags: %s(0x%x) \n",
	             dir, action, sp->index, sp->priority, share, flags, sp->flags);

	nl_dump_line(p, "\tlifetime configuration: \n");
	if (sp->lft->soft_byte_limit == XFRM_INF)
		sprintf (dir, "INF");
	else
		sprintf (dir, "%" PRIu64, sp->lft->soft_byte_limit);
	if (sp->lft->soft_packet_limit == XFRM_INF)
		sprintf (action, "INF");
	else
		sprintf (action, "%" PRIu64, sp->lft->soft_packet_limit);
	if (sp->lft->hard_byte_limit == XFRM_INF)
		sprintf (flags, "INF");
	else
		sprintf (flags, "%" PRIu64, sp->lft->hard_byte_limit);
	if (sp->lft->hard_packet_limit == XFRM_INF)
		sprintf (share, "INF");
	else
		sprintf (share, "%" PRIu64, sp->lft->hard_packet_limit);
	nl_dump_line(p, "\t\tsoft limit: %s (bytes), %s (packets) \n", dir,
		     action);
	nl_dump_line(p, "\t\thard limit: %s (bytes), %s (packets) \n", flags,
		     share);
	nl_dump_line(
		p,
		"\t\tsoft add_time: %llu (seconds), soft use_time: %llu (seconds) \n",
		(long long unsigned)sp->lft->soft_add_expires_seconds,
		(long long unsigned)sp->lft->soft_use_expires_seconds);
	nl_dump_line(
		p,
		"\t\thard add_time: %llu (seconds), hard use_time: %llu (seconds) \n",
		(long long unsigned)sp->lft->hard_add_expires_seconds,
		(long long unsigned)sp->lft->hard_use_expires_seconds);

	nl_dump_line(p, "\tlifetime current: \n");
	nl_dump_line(p, "\t\t%llu bytes, %llu packets\n",
		     (long long unsigned)sp->curlft.bytes,
		     (long long unsigned)sp->curlft.packets);

	if (sp->curlft.add_time != 0)
	{
		add_time = sp->curlft.add_time;
		add_time_tm = gmtime_r (&add_time, &tm_buf);
		strftime (dst, INET6_ADDRSTRLEN+5, "%Y-%m-%d %H-%M-%S", add_time_tm);
	}
	else
	{
		sprintf (dst, "%s", "-");
	}

	if (sp->curlft.use_time != 0)
	{
		use_time = sp->curlft.use_time;
		use_time_tm = gmtime_r (&use_time, &tm_buf);
		strftime (src, INET6_ADDRSTRLEN+5, "%Y-%m-%d %H-%M-%S", use_time_tm);
	}
	else
	{
		sprintf (src, "%s", "-");
	}
	nl_dump_line(p, "\t\tadd_time: %s, use_time: %s\n", dst, src);

	if (sp->ce_mask & XFRM_SP_ATTR_SECCTX)
	{
		nl_dump_line(p, "\tUser security context: \n");
		nl_dump_line(p, "\t\tlen: %d exttype: %d Algo: %d DOI: %d ctxlen: %d\n",
		             sp->sec_ctx->len, sp->sec_ctx->exttype,
		             sp->sec_ctx->ctx_alg, sp->sec_ctx->ctx_doi, sp->sec_ctx->ctx_len);
		nl_dump_line (p, "\t\tctx: %s \n", sp->sec_ctx->ctx);
	}

	xfrmnl_sp_type2str (sp->uptype.type, flags, 32);
	if (sp->ce_mask & XFRM_SP_ATTR_POLTYPE)
		nl_dump_line(p, "\tUser policy type: %s\n", flags);

	if (sp->ce_mask & XFRM_SP_ATTR_TMPL)
	{
		struct xfrmnl_user_tmpl*    utmpl;

		nl_dump_line(p, "\tUser template: \n");

		nl_list_for_each_entry(utmpl, &sp->usertmpl_list, utmpl_list)
			xfrmnl_user_tmpl_dump (utmpl, p);
	}

	if (sp->ce_mask & XFRM_SP_ATTR_MARK)
		nl_dump_line(p, "\tMark mask: 0x%x Mark value: 0x%x\n", sp->mark.m, sp->mark.v);


	if (sp->ce_mask & XFRM_SP_ATTR_IF_ID)
		nl_dump_line(p, "\tXFRM interface ID: 0x%x\n", sp->if_id);

	nl_dump(p, "\n");
}

static void xfrm_sp_dump_details(struct nl_object *a, struct nl_dump_params *p)
{
	xfrm_sp_dump_line(a, p);
}

static void xfrm_sp_dump_stats(struct nl_object *a, struct nl_dump_params *p)
{
	xfrm_sp_dump_details(a, p);

	return;
}

/**
 * @name XFRM SP Object Allocation/Freeage
 * @{
 */

struct xfrmnl_sp* xfrmnl_sp_alloc(void)
{
	return (struct xfrmnl_sp*) nl_object_alloc(&xfrm_sp_obj_ops);
}

void xfrmnl_sp_put(struct xfrmnl_sp* sp)
{
	nl_object_put((struct nl_object *) sp);
}

/** @} */

/**
 * @name SP Cache Managament
 * @{
 */

/**
 * Build a SP cache including all SPs currently configured in the kernel.
 * @arg sock		Netlink socket.
 * @arg result		Pointer to store resulting cache.
 *
 * Allocates a new SP cache, initializes it properly and updates it
 * to include all SPs currently configured in the kernel.
 *
 * @return 0 on success or a negative error code.
 */
int xfrmnl_sp_alloc_cache(struct nl_sock *sock, struct nl_cache **result)
{
	return nl_cache_alloc_and_fill(&xfrmnl_sp_ops, sock, result);
}

/**
 * Look up a SP by policy id and direction
 * @arg cache		SP cache
 * @arg index		Policy Id
 * @arg dir         direction
 * @return sp handle or NULL if no match was found.
 */
struct xfrmnl_sp* xfrmnl_sp_get(struct nl_cache* cache, unsigned int index, unsigned int dir)
{
	struct xfrmnl_sp *sp;

	//nl_list_for_each_entry(sp, &cache->c_items, ce_list) {
	for (sp = (struct xfrmnl_sp*)nl_cache_get_first (cache);
	     sp != NULL;
	     sp = (struct xfrmnl_sp*)nl_cache_get_next ((struct nl_object*)sp))
	{
		if (sp->index == index && sp->dir == dir)
		{
			nl_object_get((struct nl_object *) sp);
			return sp;
		}
	}

	return NULL;
}


/** @} */


static struct nla_policy xfrm_sp_policy[XFRMA_MAX+1] = {
	[XFRMA_POLICY]          = { .minlen = sizeof(struct xfrm_userpolicy_info)},
	[XFRMA_SEC_CTX]         = { .minlen = sizeof(struct xfrm_sec_ctx) },
	[XFRMA_TMPL]            = { .minlen = sizeof(struct xfrm_user_tmpl) },
	[XFRMA_POLICY_TYPE]     = { .minlen = sizeof(struct xfrm_userpolicy_type)},
	[XFRMA_MARK]            = { .minlen = sizeof(struct xfrm_mark) },
	[XFRMA_IF_ID]           = { .type = NLA_U32 },
};

static int xfrm_sp_request_update(struct nl_cache *c, struct nl_sock *h)
{
	return nl_send_simple (h, XFRM_MSG_GETPOLICY, NLM_F_DUMP, NULL, 0);
}

int xfrmnl_sp_parse(struct nlmsghdr *n, struct xfrmnl_sp **result)
{
	_nl_auto_nl_addr struct nl_addr *addr1 = NULL;
	_nl_auto_nl_addr struct nl_addr *addr2 = NULL;
	_nl_auto_xfrmnl_sp struct xfrmnl_sp *sp = NULL;
	struct nlattr                   *tb[XFRMA_MAX + 1];
	struct xfrm_userpolicy_info     *sp_info;
	int                             len, err;

	sp = xfrmnl_sp_alloc();
	if (!sp)
		return -NLE_NOMEM;

	sp->ce_msgtype = n->nlmsg_type;
	if (n->nlmsg_type == XFRM_MSG_DELPOLICY)
		sp_info = (struct xfrm_userpolicy_info*)((char *)nlmsg_data(n) + sizeof (struct xfrm_userpolicy_id) + NLA_HDRLEN);
	else
		sp_info = nlmsg_data(n);

	err = nlmsg_parse(n, sizeof(struct xfrm_userpolicy_info), tb, XFRMA_MAX, xfrm_sp_policy);
	if (err < 0)
		return err;

	if (!(addr1 = _nl_addr_build(sp_info->sel.family, &sp_info->sel.daddr)))
		return -NLE_NOMEM;
	nl_addr_set_prefixlen (addr1, sp_info->sel.prefixlen_d);
	xfrmnl_sel_set_daddr (sp->sel, addr1);
	xfrmnl_sel_set_prefixlen_d (sp->sel, sp_info->sel.prefixlen_d);

	if (!(addr2 = _nl_addr_build(sp_info->sel.family, &sp_info->sel.saddr)))
		return -NLE_NOMEM;
	nl_addr_set_prefixlen (addr2, sp_info->sel.prefixlen_s);
	xfrmnl_sel_set_saddr (sp->sel, addr2);
	xfrmnl_sel_set_prefixlen_s (sp->sel, sp_info->sel.prefixlen_s);

	xfrmnl_sel_set_dport (sp->sel, ntohs (sp_info->sel.dport));
	xfrmnl_sel_set_dportmask (sp->sel, ntohs (sp_info->sel.dport_mask));
	xfrmnl_sel_set_sport (sp->sel, ntohs (sp_info->sel.sport));
	xfrmnl_sel_set_sportmask (sp->sel, ntohs (sp_info->sel.sport_mask));
	xfrmnl_sel_set_family (sp->sel, sp_info->sel.family);
	xfrmnl_sel_set_proto (sp->sel, sp_info->sel.proto);
	xfrmnl_sel_set_ifindex (sp->sel, sp_info->sel.ifindex);
	xfrmnl_sel_set_userid (sp->sel, sp_info->sel.user);
	sp->ce_mask             |= XFRM_SP_ATTR_SEL;

	sp->lft->soft_byte_limit    =   sp_info->lft.soft_byte_limit;
	sp->lft->hard_byte_limit    =   sp_info->lft.hard_byte_limit;
	sp->lft->soft_packet_limit  =   sp_info->lft.soft_packet_limit;
	sp->lft->hard_packet_limit  =   sp_info->lft.hard_packet_limit;
	sp->lft->soft_add_expires_seconds   =   sp_info->lft.soft_add_expires_seconds;
	sp->lft->hard_add_expires_seconds   =   sp_info->lft.hard_add_expires_seconds;
	sp->lft->soft_use_expires_seconds   =   sp_info->lft.soft_use_expires_seconds;
	sp->lft->hard_use_expires_seconds   =   sp_info->lft.hard_use_expires_seconds;
	sp->ce_mask             |= XFRM_SP_ATTR_LTIME_CFG;

	sp->curlft.bytes        = sp_info->curlft.bytes;
	sp->curlft.packets      = sp_info->curlft.packets;
	sp->curlft.add_time     = sp_info->curlft.add_time;
	sp->curlft.use_time     = sp_info->curlft.use_time;
	sp->ce_mask             |= XFRM_SP_ATTR_LTIME_CUR;

	sp->priority            = sp_info->priority;
	sp->index               = sp_info->index;
	sp->dir                 = sp_info->dir;
	sp->action              = sp_info->action;
	sp->flags               = sp_info->flags;
	sp->share               = sp_info->share;
	sp->ce_mask             |= (XFRM_SP_ATTR_PRIO | XFRM_SP_ATTR_INDEX |
	                            XFRM_SP_ATTR_DIR | XFRM_SP_ATTR_ACTION |
	                            XFRM_SP_ATTR_FLAGS | XFRM_SP_ATTR_SHARE);

	if (tb[XFRMA_SEC_CTX]) {
		struct xfrm_user_sec_ctx* ctx = nla_data(tb[XFRMA_SEC_CTX]);

		len = sizeof (struct xfrmnl_user_sec_ctx) + ctx->ctx_len;
		if ((sp->sec_ctx = calloc (1, len)) == NULL)
			return -NLE_NOMEM;
		memcpy ((void *)sp->sec_ctx, (void *)ctx, len);
		sp->ce_mask     |= XFRM_SP_ATTR_SECCTX;
	}

	if (tb[XFRMA_POLICY_TYPE]) {
		struct xfrm_userpolicy_type* up = nla_data(tb[XFRMA_POLICY_TYPE]);

		memcpy ((void *)&sp->uptype, (void *)up, sizeof (struct xfrm_userpolicy_type));
		sp->ce_mask     |= XFRM_SP_ATTR_POLTYPE;
	}

	if (tb[XFRMA_TMPL]) {
		struct xfrm_user_tmpl*      tmpl = nla_data(tb[XFRMA_TMPL]);
		uint32_t                    i;
		uint32_t                    num_tmpls = nla_len(tb[XFRMA_TMPL]) / sizeof (*tmpl);

		for (i = 0; (i < num_tmpls) && (tmpl); i ++, tmpl++)
		{
			_nl_auto_xfrmnl_user_tmpl struct xfrmnl_user_tmpl *sputmpl = NULL;
			_nl_auto_nl_addr struct nl_addr *addr1 = NULL;
			_nl_auto_nl_addr struct nl_addr *addr2 = NULL;

			if ((sputmpl = xfrmnl_user_tmpl_alloc ()) == NULL)
				return -NLE_NOMEM;

			if (!(addr1 = _nl_addr_build(tmpl->family, &tmpl->id.daddr)))
				return -NLE_NOMEM;
			xfrmnl_user_tmpl_set_daddr (sputmpl, addr1);
			xfrmnl_user_tmpl_set_spi (sputmpl, ntohl(tmpl->id.spi));
			xfrmnl_user_tmpl_set_proto (sputmpl, tmpl->id.proto);
			xfrmnl_user_tmpl_set_family (sputmpl, tmpl->family);

			if (!(addr2 = _nl_addr_build(tmpl->family, &tmpl->saddr)))
				return -NLE_NOMEM;
			xfrmnl_user_tmpl_set_saddr (sputmpl, addr2);

			xfrmnl_user_tmpl_set_reqid (sputmpl, tmpl->reqid);
			xfrmnl_user_tmpl_set_mode (sputmpl, tmpl->mode);
			xfrmnl_user_tmpl_set_share (sputmpl, tmpl->share);
			xfrmnl_user_tmpl_set_optional (sputmpl, tmpl->optional);
			xfrmnl_user_tmpl_set_aalgos (sputmpl, tmpl->aalgos);
			xfrmnl_user_tmpl_set_ealgos (sputmpl, tmpl->ealgos);
			xfrmnl_user_tmpl_set_calgos (sputmpl, tmpl->calgos);
			xfrmnl_sp_add_usertemplate (sp, _nl_steal_pointer(&sputmpl));

			sp->ce_mask     |=  XFRM_SP_ATTR_TMPL;
		}
	}

	if (tb[XFRMA_MARK]) {
		struct xfrm_mark* m =   nla_data(tb[XFRMA_MARK]);
		sp->mark.m  =   m->m;
		sp->mark.v  =   m->v;
		sp->ce_mask |= XFRM_SP_ATTR_MARK;
	}

	if (tb[XFRMA_IF_ID]) {
		sp->if_id = nla_get_u32(tb[XFRMA_IF_ID]);
		sp->ce_mask |= XFRM_SP_ATTR_IF_ID;
	}

	*result = _nl_steal_pointer(&sp);
	return 0;
}

static int xfrm_sp_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
				struct nlmsghdr *n, struct nl_parser_param *pp)
{
	struct xfrmnl_sp*   sp;
	int                 err;

	if ((err = xfrmnl_sp_parse(n, &sp)) < 0)
	{
		printf ("received error: %d \n", err);
		return err;
	}

	err = pp->pp_cb((struct nl_object *) sp, pp);

	xfrmnl_sp_put(sp);
	return err;
}

/**
 * @name XFRM SP Get
 * @{
 */

int xfrmnl_sp_build_get_request(unsigned int index, unsigned int dir, unsigned int mark_v, unsigned int mark_m, struct nl_msg **result)
{
	struct nl_msg               *msg;
	struct xfrm_userpolicy_id   spid;
	struct xfrm_mark            mark;

	memset(&spid, 0, sizeof(spid));
	spid.index          = index;
	spid.dir            = dir;

	if (!(msg = nlmsg_alloc_simple(XFRM_MSG_GETPOLICY, 0)))
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &spid, sizeof(spid), NLMSG_ALIGNTO) < 0)
		goto nla_put_failure;

	if ((mark_m & mark_v) != 0)
	{
		memset(&mark, 0, sizeof(struct xfrm_mark));
		mark.m = mark_m;
		mark.v = mark_v;

		NLA_PUT (msg, XFRMA_MARK, sizeof (struct xfrm_mark), &mark);
	}

	*result = msg;
	return 0;

nla_put_failure:
	nlmsg_free(msg);
	return -NLE_MSGSIZE;
}

int xfrmnl_sp_get_kernel(struct nl_sock* sock, unsigned int index, unsigned int dir, unsigned int mark_v, unsigned int mark_m, struct xfrmnl_sp** result)
{
	struct nl_msg       *msg = NULL;
	struct nl_object    *obj;
	int err;

	if ((err = xfrmnl_sp_build_get_request(index, dir, mark_m, mark_v, &msg)) < 0)
		return err;

	err = nl_send_auto(sock, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	if ((err = nl_pickup(sock, &xfrm_sp_msg_parser, &obj)) < 0)
		return err;

	/* We have used xfrm_sp_msg_parser(), object is definitely a xfrm ae */
	*result = (struct xfrmnl_sp *) obj;

	/* If an object has been returned, we also need to wait for the ACK */
	if (err == 0 && obj)
		nl_wait_for_ack(sock);

	return 0;
}

/** @} */

static int build_xfrm_sp_message(struct xfrmnl_sp *tmpl, int cmd, int flags, struct nl_msg **result)
{
	struct nl_msg*              msg;
	struct xfrm_userpolicy_info sp_info;
	uint32_t                    len;
	struct nl_addr*             addr;

	if (!(tmpl->ce_mask & XFRM_SP_ATTR_DIR) ||
			(!(tmpl->ce_mask & XFRM_SP_ATTR_INDEX) &&
			 !(tmpl->ce_mask & XFRM_SP_ATTR_SEL)))
		return -NLE_MISSING_ATTR;

	memset ((void*)&sp_info, 0, sizeof (sp_info));
	if (tmpl->ce_mask & XFRM_SP_ATTR_SEL)
	{
		addr = xfrmnl_sel_get_daddr (tmpl->sel);
		memcpy ((void*)&sp_info.sel.daddr, (void*)nl_addr_get_binary_addr (addr), sizeof (uint8_t) * nl_addr_get_len (addr));
		addr = xfrmnl_sel_get_saddr (tmpl->sel);
		memcpy ((void*)&sp_info.sel.saddr, (void*)nl_addr_get_binary_addr (addr), sizeof (uint8_t) * nl_addr_get_len (addr));
		sp_info.sel.dport       =   htons (xfrmnl_sel_get_dport (tmpl->sel));
		sp_info.sel.dport_mask  =   htons (xfrmnl_sel_get_dportmask (tmpl->sel));
		sp_info.sel.sport       =   htons (xfrmnl_sel_get_sport (tmpl->sel));
		sp_info.sel.sport_mask  =   htons (xfrmnl_sel_get_sportmask (tmpl->sel));
		sp_info.sel.family      =   xfrmnl_sel_get_family (tmpl->sel);
		sp_info.sel.prefixlen_d =   xfrmnl_sel_get_prefixlen_d (tmpl->sel);
		sp_info.sel.prefixlen_s =   xfrmnl_sel_get_prefixlen_s (tmpl->sel);
		sp_info.sel.proto       =   xfrmnl_sel_get_proto (tmpl->sel);
		sp_info.sel.ifindex     =   xfrmnl_sel_get_ifindex (tmpl->sel);
		sp_info.sel.user        =   xfrmnl_sel_get_userid (tmpl->sel);
	}

	if (tmpl->ce_mask & XFRM_SP_ATTR_LTIME_CFG)
	{
		sp_info.lft.soft_byte_limit = xfrmnl_ltime_cfg_get_soft_bytelimit (tmpl->lft);
		sp_info.lft.hard_byte_limit = xfrmnl_ltime_cfg_get_hard_bytelimit (tmpl->lft);
		sp_info.lft.soft_packet_limit = xfrmnl_ltime_cfg_get_soft_packetlimit (tmpl->lft);
		sp_info.lft.hard_packet_limit = xfrmnl_ltime_cfg_get_hard_packetlimit (tmpl->lft);
		sp_info.lft.soft_add_expires_seconds = xfrmnl_ltime_cfg_get_soft_addexpires (tmpl->lft);
		sp_info.lft.hard_add_expires_seconds = xfrmnl_ltime_cfg_get_hard_addexpires (tmpl->lft);
		sp_info.lft.soft_use_expires_seconds = xfrmnl_ltime_cfg_get_soft_useexpires (tmpl->lft);
		sp_info.lft.hard_use_expires_seconds = xfrmnl_ltime_cfg_get_hard_useexpires (tmpl->lft);
	}

	//Skip current lifetime: cur lifetime can be updated only via AE

	if (tmpl->ce_mask & XFRM_SP_ATTR_PRIO)
		sp_info.priority    = tmpl->priority;

	if (tmpl->ce_mask & XFRM_SP_ATTR_INDEX)
		sp_info.index       = tmpl->index;

	if (tmpl->ce_mask & XFRM_SP_ATTR_DIR)
		sp_info.dir         = tmpl->dir;

	if (tmpl->ce_mask & XFRM_SP_ATTR_ACTION)
		sp_info.action      = tmpl->action;

	if (tmpl->ce_mask & XFRM_SP_ATTR_FLAGS)
		sp_info.flags       = tmpl->flags;

	if (tmpl->ce_mask & XFRM_SP_ATTR_SHARE)
		sp_info.share       = tmpl->share;

	msg = nlmsg_alloc_simple(cmd, flags);
	if (!msg)
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &sp_info, sizeof(sp_info), NLMSG_ALIGNTO) < 0)
		goto nla_put_failure;

	if (tmpl->ce_mask & XFRM_SP_ATTR_SECCTX) {
		len = (sizeof (struct xfrm_user_sec_ctx)) + tmpl->sec_ctx->ctx_len;
		NLA_PUT (msg, XFRMA_SEC_CTX, len, tmpl->sec_ctx);
	}

	if (tmpl->ce_mask & XFRM_SP_ATTR_POLTYPE) {
		len = sizeof (struct xfrm_userpolicy_type);
		NLA_PUT (msg, XFRMA_POLICY_TYPE, len, &tmpl->uptype);
	}

	if (tmpl->ce_mask & XFRM_SP_ATTR_TMPL) {
		struct nlattr*              tmpls;
		struct xfrmnl_user_tmpl*    utmpl;
		struct nl_addr*             addr;

		if (!(tmpls = nla_nest_start(msg, XFRMA_TMPL)))
			goto nla_put_failure;

		nl_list_for_each_entry(utmpl, &tmpl->usertmpl_list, utmpl_list) {
			struct xfrm_user_tmpl*  tmpl;

			tmpl = nlmsg_reserve(msg, sizeof(*tmpl), NLMSG_ALIGNTO);
			if (!tmpl)
				goto nla_put_failure;
			addr = xfrmnl_user_tmpl_get_daddr (utmpl);
			memcpy ((void *)&tmpl->id.daddr, nl_addr_get_binary_addr (addr),
			         nl_addr_get_len (addr));
			tmpl->id.spi    =   htonl(xfrmnl_user_tmpl_get_spi (utmpl));
			tmpl->id.proto  =   xfrmnl_user_tmpl_get_proto (utmpl);
			tmpl->family    =   xfrmnl_user_tmpl_get_family (utmpl);
			addr = xfrmnl_user_tmpl_get_saddr (utmpl);
			memcpy ((void *)&tmpl->saddr, nl_addr_get_binary_addr (addr),
			        nl_addr_get_len (addr));
			tmpl->reqid     =   xfrmnl_user_tmpl_get_reqid (utmpl);
			tmpl->mode      =   xfrmnl_user_tmpl_get_mode (utmpl);
			tmpl->share     =   xfrmnl_user_tmpl_get_share (utmpl);
			tmpl->optional  =   xfrmnl_user_tmpl_get_optional (utmpl);
			tmpl->aalgos    =   xfrmnl_user_tmpl_get_aalgos (utmpl);
			tmpl->ealgos    =   xfrmnl_user_tmpl_get_ealgos (utmpl);
			tmpl->calgos    =   xfrmnl_user_tmpl_get_calgos (utmpl);
		}
		nla_nest_end(msg, tmpls);
	}

	if (tmpl->ce_mask & XFRM_SP_ATTR_MARK) {
		NLA_PUT (msg, XFRMA_MARK, sizeof (struct xfrm_mark), &tmpl->mark);
	}

	if (tmpl->ce_mask & XFRM_SP_ATTR_IF_ID) {
		NLA_PUT_U32 (msg, XFRMA_IF_ID, tmpl->if_id);
	}

	*result = msg;
	return 0;

nla_put_failure:
	nlmsg_free(msg);
	return -NLE_MSGSIZE;
}

/**
 * @name XFRM SP Add
 * @{
 */

int xfrmnl_sp_build_add_request(struct xfrmnl_sp* tmpl, int flags, struct nl_msg **result)
{
	return build_xfrm_sp_message (tmpl, XFRM_MSG_NEWPOLICY, flags, result);
}

int xfrmnl_sp_add(struct nl_sock* sk, struct xfrmnl_sp* tmpl, int flags)
{
	int             err;
	struct nl_msg   *msg;

	if ((err = xfrmnl_sp_build_add_request(tmpl, flags, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return nl_wait_for_ack(sk);
}

/**
 * @name XFRM SP Update
 * @{
 */

int xfrmnl_sp_build_update_request(struct xfrmnl_sp* tmpl, int flags, struct nl_msg **result)
{
	return build_xfrm_sp_message (tmpl, XFRM_MSG_UPDPOLICY, flags, result);
}

int xfrmnl_sp_update(struct nl_sock* sk, struct xfrmnl_sp* tmpl, int flags)
{
	int             err;
	struct nl_msg   *msg;

	if ((err = xfrmnl_sp_build_update_request(tmpl, flags, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return nl_wait_for_ack(sk);
}

/** @} */

/**
 * \brief      Builds a xfrm_sp_delete_message. Uses either index and direction
 *             or security-context (not set is a valid value), selector and
 *             direction for identification.
 *             Returns error if necessary values aren't set.
 *
 * \param      tmpl    The policy template.
 * \param      cmd     The command. Should be XFRM_MSG_DELPOLICY.
 * \param      flags   Additional flags
 * \param      result  Resulting message.
 *
 * \return     0 if successful, else error value < 0
 */
static int build_xfrm_sp_delete_message(struct xfrmnl_sp *tmpl, int cmd, int flags, struct nl_msg **result)
{
	struct nl_msg*              msg;
	struct xfrm_userpolicy_id   spid;
	struct nl_addr*             addr;
	uint32_t 					len;

	if (!(tmpl->ce_mask & XFRM_SP_ATTR_DIR) ||
			(!(tmpl->ce_mask & XFRM_SP_ATTR_INDEX) &&
			 !(tmpl->ce_mask & XFRM_SP_ATTR_SEL)))
		return -NLE_MISSING_ATTR;

	memset(&spid, 0, sizeof(spid));
	spid.dir            = tmpl->dir;
	if(tmpl->ce_mask & XFRM_SP_ATTR_INDEX)
		spid.index          = tmpl->index;

	if (tmpl->ce_mask & XFRM_SP_ATTR_SEL)
	{
		addr = xfrmnl_sel_get_daddr (tmpl->sel);
		memcpy ((void*)&spid.sel.daddr, (void*)nl_addr_get_binary_addr (addr), sizeof (uint8_t) * nl_addr_get_len (addr));
		addr = xfrmnl_sel_get_saddr (tmpl->sel);
		memcpy ((void*)&spid.sel.saddr, (void*)nl_addr_get_binary_addr (addr), sizeof (uint8_t) * nl_addr_get_len (addr));
		spid.sel.dport       =   htons (xfrmnl_sel_get_dport (tmpl->sel));
		spid.sel.dport_mask  =   htons (xfrmnl_sel_get_dportmask (tmpl->sel));
		spid.sel.sport       =   htons (xfrmnl_sel_get_sport (tmpl->sel));
		spid.sel.sport_mask  =   htons (xfrmnl_sel_get_sportmask (tmpl->sel));
		spid.sel.family      =   xfrmnl_sel_get_family (tmpl->sel);
		spid.sel.prefixlen_d =   xfrmnl_sel_get_prefixlen_d (tmpl->sel);
		spid.sel.prefixlen_s =   xfrmnl_sel_get_prefixlen_s (tmpl->sel);
		spid.sel.proto       =   xfrmnl_sel_get_proto (tmpl->sel);
		spid.sel.ifindex     =   xfrmnl_sel_get_ifindex (tmpl->sel);
		spid.sel.user        =   xfrmnl_sel_get_userid (tmpl->sel);
	}

	msg = nlmsg_alloc_simple(cmd, flags);
	if (!msg)
		return -NLE_NOMEM;

	if (nlmsg_append(msg, &spid, sizeof(spid), NLMSG_ALIGNTO) < 0)
		goto nla_put_failure;

	if (tmpl->ce_mask & XFRM_SP_ATTR_SECCTX) {
		len = (sizeof (struct xfrm_user_sec_ctx)) + tmpl->sec_ctx->ctx_len;
		NLA_PUT (msg, XFRMA_SEC_CTX, len, tmpl->sec_ctx);
	}

	if (tmpl->ce_mask & XFRM_SP_ATTR_MARK) {
		len = sizeof (struct xfrm_mark);
		NLA_PUT (msg, XFRMA_MARK, len, &tmpl->mark);
	}

	if (tmpl->ce_mask & XFRM_SP_ATTR_IF_ID) {
		NLA_PUT_U32 (msg, XFRMA_IF_ID, tmpl->if_id);
	}

	*result = msg;
	return 0;

nla_put_failure:
	nlmsg_free(msg);
	return -NLE_MSGSIZE;
}

/**
 * @name XFRM SA Delete
 * @{
 */

int xfrmnl_sp_build_delete_request(struct xfrmnl_sp* tmpl, int flags, struct nl_msg **result)
{
	return build_xfrm_sp_delete_message (tmpl, XFRM_MSG_DELPOLICY, flags, result);
}

int xfrmnl_sp_delete(struct nl_sock* sk, struct xfrmnl_sp* tmpl, int flags)
{
	int             err;
	struct nl_msg   *msg;

	if ((err = xfrmnl_sp_build_delete_request(tmpl, flags, &msg)) < 0)
		return err;

	err = nl_send_auto_complete(sk, msg);
	nlmsg_free(msg);
	if (err < 0)
		return err;

	return nl_wait_for_ack(sk);
}

/** @} */


/**
 * @name Attributes
 * @{
 */

struct xfrmnl_sel* xfrmnl_sp_get_sel (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_SEL)
		return sp->sel;
	else
		return NULL;
}

int xfrmnl_sp_set_sel (struct xfrmnl_sp* sp, struct xfrmnl_sel* sel)
{
	/* Release any previously held selector object from the SP */
	if (sp->sel)
		xfrmnl_sel_put (sp->sel);

	/* Increment ref count on new selector and save it in the SP */
	xfrmnl_sel_get (sel);
	sp->sel     =   sel;
	sp->ce_mask |=  XFRM_SP_ATTR_SEL;

	return 0;
}

struct xfrmnl_ltime_cfg* xfrmnl_sp_get_lifetime_cfg (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_LTIME_CFG)
		return sp->lft;
	else
		return NULL;
}

int xfrmnl_sp_set_lifetime_cfg (struct xfrmnl_sp* sp, struct xfrmnl_ltime_cfg* ltime)
{
	/* Release any previously held lifetime cfg object from the SP */
	if (sp->lft)
		xfrmnl_ltime_cfg_put (sp->lft);

	/* Increment ref count on new lifetime object and save it in the SP */
	xfrmnl_ltime_cfg_get (ltime);
	sp->lft     =   ltime;
	sp->ce_mask |=  XFRM_SP_ATTR_LTIME_CFG;

	return 0;
}

int xfrmnl_sp_get_curlifetime (struct xfrmnl_sp* sp, unsigned long long int* curr_bytes,
                               unsigned long long int* curr_packets, unsigned long long int* curr_add_time, unsigned long long int* curr_use_time)
{
	if (sp == NULL || curr_bytes == NULL || curr_packets == NULL || curr_add_time == NULL || curr_use_time == NULL)
		return -1;

	*curr_bytes     =   sp->curlft.bytes;
	*curr_packets   =   sp->curlft.packets;
	*curr_add_time  =   sp->curlft.add_time;
	*curr_use_time  =   sp->curlft.use_time;

	return 0;
}

int xfrmnl_sp_get_priority (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_PRIO)
		return sp->priority;
	else
		return -1;
}

int xfrmnl_sp_set_priority (struct xfrmnl_sp* sp, unsigned int prio)
{
	sp->priority    = prio;
	sp->ce_mask     |= XFRM_SP_ATTR_PRIO;

	return 0;
}

int xfrmnl_sp_get_index (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_INDEX)
		return sp->index;
	else
		return -1;
}

int xfrmnl_sp_set_index (struct xfrmnl_sp* sp, unsigned int index)
{
	sp->index       = index;
	sp->ce_mask     |= XFRM_SP_ATTR_INDEX;

	return 0;
}

int xfrmnl_sp_get_dir (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_DIR)
		return sp->dir;
	else
		return -1;
}

int xfrmnl_sp_set_dir (struct xfrmnl_sp* sp, unsigned int dir)
{
	sp->dir         = dir;
	sp->ce_mask     |= XFRM_SP_ATTR_DIR;

	return 0;
}

int xfrmnl_sp_get_action (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_ACTION)
		return sp->action;
	else
		return -1;
}

int xfrmnl_sp_set_action (struct xfrmnl_sp* sp, unsigned int action)
{
	sp->action      = action;
	sp->ce_mask     |= XFRM_SP_ATTR_ACTION;

	return 0;
}

int xfrmnl_sp_get_flags (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_FLAGS)
		return sp->flags;
	else
		return -1;
}

int xfrmnl_sp_set_flags (struct xfrmnl_sp* sp, unsigned int flags)
{
	sp->flags       = flags;
	sp->ce_mask     |= XFRM_SP_ATTR_FLAGS;

	return 0;
}

int xfrmnl_sp_get_share (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_SHARE)
		return sp->share;
	else
		return -1;
}

int xfrmnl_sp_set_share (struct xfrmnl_sp* sp, unsigned int share)
{
	sp->share       = share;
	sp->ce_mask     |= XFRM_SP_ATTR_SHARE;

	return 0;
}

/**
 * Get the security context.
 *
 * @arg sp              The xfrmnl_sp object.
 * @arg len             An optional output value for the ctx_str length including the xfrmnl_sp header.
 * @arg exttype         An optional output value.
 * @arg alg             An optional output value for the security context algorithm.
 * @arg doi             An optional output value for the security context domain of interpretation.
 * @arg ctx_len         An optional output value for the security context length, including the
 *                      terminating null byte ('\0').
 * @arg ctx_str         An optional buffer large enough for the security context string. It must
 *                      contain at least @ctx_len bytes. You are advised to create the ctx_str
 *                      buffer one element larger and ensure NUL termination yourself.
 *
 * Warning: you must ensure that @ctx_str is large enough. If you don't know the length before-hand,
 * call xfrmnl_sp_get_sec_ctx() without @ctx_str argument to query only the required buffer size.
 * This modified API is available in all versions of libnl3 that support the capability
 * @def NL_CAPABILITY_XFRM_SP_SEC_CTX_LEN (@see nl_has_capability for further information).
 *
 * @return 0 on success or a negative error code.
 */
int xfrmnl_sp_get_sec_ctx (struct xfrmnl_sp* sp, unsigned int* len, unsigned int* exttype, unsigned int* alg, unsigned int* doi, unsigned int* ctx_len, char* ctx_str)
{
	if (sp->ce_mask & XFRM_SP_ATTR_SECCTX)
	{
		if (len)
			*len = sizeof (struct xfrmnl_user_sec_ctx) + sp->sec_ctx->ctx_len;
		if (exttype)
			*exttype = sp->sec_ctx->exttype;
		if (alg)
			*alg = sp->sec_ctx->ctx_alg;
		if (doi)
			*doi = sp->sec_ctx->ctx_doi;
		if (ctx_len)
			*ctx_len = sp->sec_ctx->ctx_len;
		if (ctx_str)
			memcpy ((void *)ctx_str, (void *)sp->sec_ctx->ctx, sp->sec_ctx->ctx_len);
	}
	else
		return -1;

	return 0;
}
/**
 * @brief      Set security context (ctx_str) for XFRM Polixy.
 *
 * @param      sp       XFRM Policy
 * @param      len      !!! depricated unused parameter !!!
 * @param      exttype  netlink message attribute - probably XFRMA_SEC_CTX
 * @param      alg      security context algorithm
 * @param      doi      security context domain interpretation
 * @param      ctx_len  Length of the context string.
 * @param      ctx_str  The context string.
 *
 * @return     0 if sucessfull, else -1
 */
int xfrmnl_sp_set_sec_ctx (struct xfrmnl_sp* sp, unsigned int len, unsigned int exttype, unsigned int alg, unsigned int doi, unsigned int ctx_len, char* ctx_str)
{
	/* Free up the old context string and allocate new one */
	if (sp->sec_ctx)
		free (sp->sec_ctx);
	if ((sp->sec_ctx = calloc (1, sizeof (struct xfrmnl_user_sec_ctx) + 1 + ctx_len)) == NULL)
		return -1;

	/* Save the new info */
	sp->sec_ctx->len        =   sizeof (struct xfrmnl_user_sec_ctx) + ctx_len;
	sp->sec_ctx->exttype    =   exttype;
	sp->sec_ctx->ctx_alg    =   alg;
	sp->sec_ctx->ctx_doi    =   doi;
	sp->sec_ctx->ctx_len    =   ctx_len;
	memcpy ((void *)sp->sec_ctx->ctx, (void *)ctx_str, ctx_len);
	sp->sec_ctx->ctx[ctx_len] = '\0';

	sp->ce_mask |= XFRM_SP_ATTR_SECCTX;

	return 0;
}

int xfrmnl_sp_get_userpolicy_type (struct xfrmnl_sp* sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_POLTYPE)
		return sp->uptype.type;
	else
		return -1;
}

int xfrmnl_sp_set_userpolicy_type (struct xfrmnl_sp* sp, unsigned int type)
{
	sp->uptype.type = type;
	sp->ce_mask     |= XFRM_SP_ATTR_POLTYPE;

	return 0;
}

void xfrmnl_sp_add_usertemplate(struct xfrmnl_sp *sp, struct xfrmnl_user_tmpl *utmpl)
{
	nl_list_add_tail(&utmpl->utmpl_list, &sp->usertmpl_list);
	sp->nr_user_tmpl++;
	sp->ce_mask |= XFRM_SP_ATTR_TMPL;
}

void xfrmnl_sp_remove_usertemplate(struct xfrmnl_sp *sp, struct xfrmnl_user_tmpl *utmpl)
{
	if (sp->ce_mask & XFRM_SP_ATTR_TMPL) {
		sp->nr_user_tmpl--;
		nl_list_del(&utmpl->utmpl_list);
		if (sp->nr_user_tmpl == 0)
			sp->ce_mask &= ~XFRM_SP_ATTR_TMPL;
	}
}

struct nl_list_head *xfrmnl_sp_get_usertemplates(struct xfrmnl_sp *sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_TMPL)
		return &sp->usertmpl_list;

	return NULL;
}

int xfrmnl_sp_get_nusertemplates(struct xfrmnl_sp *sp)
{
	if (sp->ce_mask & XFRM_SP_ATTR_TMPL)
		return sp->nr_user_tmpl;

	return 0;
}

void xfrmnl_sp_foreach_usertemplate(struct xfrmnl_sp *r,
                                    void (*cb)(struct xfrmnl_user_tmpl *, void *),
                                    void *arg)
{
	struct xfrmnl_user_tmpl *utmpl;

	if (r->ce_mask & XFRM_SP_ATTR_TMPL) {
		nl_list_for_each_entry(utmpl, &r->usertmpl_list, utmpl_list) {
			cb(utmpl, arg);
		}
	}
}

struct xfrmnl_user_tmpl *xfrmnl_sp_usertemplate_n(struct xfrmnl_sp *r, int n)
{
	struct xfrmnl_user_tmpl *utmpl;

	if (r->ce_mask & XFRM_SP_ATTR_TMPL && n >= 0 &&
	    ((unsigned)n) < r->nr_user_tmpl) {
		uint32_t i;

		i = 0;
		nl_list_for_each_entry(utmpl, &r->usertmpl_list, utmpl_list) {
			if (i == ((unsigned)n))
				return utmpl;
			i++;
		}
	}
		return NULL;
}

int xfrmnl_sp_get_mark (struct xfrmnl_sp* sp, unsigned int* mark_mask, unsigned int* mark_value)
{
	if (mark_mask == NULL || mark_value == NULL)
		return -1;

	if (sp->ce_mask & XFRM_SP_ATTR_MARK)
	{
		*mark_mask  =   sp->mark.m;
		*mark_value  =   sp->mark.v;

		return 0;
	}
	else
		return -1;
}

int xfrmnl_sp_set_mark (struct xfrmnl_sp* sp, unsigned int value, unsigned int mask)
{
	sp->mark.v  = value;
	sp->mark.m  = mask;
	sp->ce_mask |= XFRM_SP_ATTR_MARK;

	return 0;
}

int xfrmnl_sp_get_if_id (struct xfrmnl_sp* sp, uint32_t* if_id)
{
	if (if_id == NULL)
		return -NLE_INVAL;

	if (!(sp->ce_mask & XFRM_SP_ATTR_IF_ID))
		return -NLE_NOATTR;

	*if_id = sp->if_id;

	return 0;
}

int xfrmnl_sp_set_if_id (struct xfrmnl_sp* sp, uint32_t if_id)
{
	sp->if_id = if_id;
	sp->ce_mask |= XFRM_SP_ATTR_IF_ID;

	return 0;
}

/** @} */

static struct nl_object_ops xfrm_sp_obj_ops = {
	.oo_name        =   "xfrm/sp",
	.oo_size        =   sizeof(struct xfrmnl_sp),
	.oo_constructor =   xfrm_sp_alloc_data,
	.oo_free_data   =   xfrm_sp_free_data,
	.oo_clone       =   xfrm_sp_clone,
	.oo_dump        =   {
	                        [NL_DUMP_LINE]      =   xfrm_sp_dump_line,
	                        [NL_DUMP_DETAILS]   =   xfrm_sp_dump_details,
	                        [NL_DUMP_STATS]     =   xfrm_sp_dump_stats,
	                    },
	.oo_compare     =   xfrm_sp_compare,
	.oo_attrs2str   =   xfrm_sp_attrs2str,
	.oo_id_attrs    =   (XFRM_SP_ATTR_SEL | XFRM_SP_ATTR_INDEX | XFRM_SP_ATTR_DIR),
};

static struct nl_af_group xfrm_sp_groups[] = {
	{ AF_UNSPEC, XFRMNLGRP_POLICY },
	{ END_OF_GROUP_LIST },
};

static struct nl_cache_ops xfrmnl_sp_ops = {
	.co_name            = "xfrm/sp",
	.co_hdrsize         = sizeof(struct xfrm_userpolicy_info),
	.co_msgtypes        = {
	                        { XFRM_MSG_NEWPOLICY, NL_ACT_NEW, "new" },
	                        { XFRM_MSG_DELPOLICY, NL_ACT_DEL, "del" },
	                        { XFRM_MSG_GETPOLICY, NL_ACT_GET, "get" },
	                        { XFRM_MSG_UPDPOLICY, NL_ACT_NEW, "update" },
	                        END_OF_MSGTYPES_LIST,
	                     },
	.co_protocol        = NETLINK_XFRM,
	.co_groups          = xfrm_sp_groups,
	.co_request_update  = xfrm_sp_request_update,
	.co_msg_parser      = xfrm_sp_msg_parser,
	.co_obj_ops         = &xfrm_sp_obj_ops,
};

/**
 * @name XFRM SA Cache Managament
 * @{
 */

static void _nl_init xfrm_sp_init(void)
{
	nl_cache_mngt_register(&xfrmnl_sp_ops);
}

static void _nl_exit xfrm_sp_exit(void)
{
	nl_cache_mngt_unregister(&xfrmnl_sp_ops);
}

/** @} */
