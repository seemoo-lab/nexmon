/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2007 Philip Craig <philipc@snapgear.com>
 * Copyright (c) 2007 Secure Computing Corporation
 */

#include "nl-default.h"

#include <netlink/netfilter/nfnl.h>
#include <netlink/netfilter/netfilter.h>
#include <netlink/netfilter/log_msg.h>
#include <netlink/netfilter/ct.h>
#include <netlink/route/link.h>

#include "nl-priv-dynamic-core/object-api.h"
#include "nl-netfilter.h"

/** @cond SKIP */
#define LOG_MSG_ATTR_FAMILY		(1UL << 0)
#define LOG_MSG_ATTR_HWPROTO		(1UL << 1)
#define LOG_MSG_ATTR_HOOK		(1UL << 2)
#define LOG_MSG_ATTR_MARK		(1UL << 3)
#define LOG_MSG_ATTR_TIMESTAMP		(1UL << 4)
#define LOG_MSG_ATTR_INDEV		(1UL << 5)
#define LOG_MSG_ATTR_OUTDEV		(1UL << 6)
#define LOG_MSG_ATTR_PHYSINDEV		(1UL << 7)
#define LOG_MSG_ATTR_PHYSOUTDEV		(1UL << 8)
#define LOG_MSG_ATTR_HWADDR		(1UL << 9)
#define LOG_MSG_ATTR_PAYLOAD		(1UL << 10)
#define LOG_MSG_ATTR_PREFIX		(1UL << 11)
#define LOG_MSG_ATTR_UID		(1UL << 12)
#define LOG_MSG_ATTR_GID		(1UL << 13)
#define LOG_MSG_ATTR_SEQ		(1UL << 14)
#define LOG_MSG_ATTR_SEQ_GLOBAL		(1UL << 15)
#define LOG_MSG_ATTR_HWTYPE		(1UL << 16)
#define LOG_MSG_ATTR_HWLEN		(1UL << 17)
#define LOG_MSG_ATTR_HWHEADER		(1UL << 18)
#define LOG_MSG_ATTR_VLAN_PROTO		(1UL << 19)
#define LOG_MSG_ATTR_VLAN_TAG		(1UL << 20)
#define LOG_MSG_ATTR_CT_INFO		(1UL << 21)
#define LOG_MSG_ATTR_CT			(1UL << 22)
/** @endcond */

static void log_msg_free_data(struct nl_object *c)
{
	struct nfnl_log_msg *msg = (struct nfnl_log_msg *) c;

	if (msg == NULL)
		return;

	free(msg->log_msg_payload);
	free(msg->log_msg_prefix);
	free(msg->log_msg_hwheader);
	if (msg->log_msg_ct)
		nfnl_ct_put(msg->log_msg_ct);
}

static int log_msg_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct nfnl_log_msg *dst = (struct nfnl_log_msg *) _dst;
	struct nfnl_log_msg *src = (struct nfnl_log_msg *) _src;
	int err;

	dst->log_msg_payload = NULL;
	dst->log_msg_payload_len = 0;
	dst->log_msg_prefix = NULL;
	dst->log_msg_hwheader = NULL;
	dst->log_msg_hwheader_len = 0;
	dst->log_msg_ct = NULL;

	if (src->log_msg_payload) {
		err = nfnl_log_msg_set_payload(dst, src->log_msg_payload,
		                               src->log_msg_payload_len);
		if (err < 0)
			return err;
	}

	if (src->log_msg_prefix) {
		err = nfnl_log_msg_set_prefix(dst, src->log_msg_prefix);
		if (err < 0)
			return err;
	}

	if (src->log_msg_hwheader) {
		err = nfnl_log_msg_set_hwheader(dst, src->log_msg_hwheader,
		                                src->log_msg_hwheader_len);
		if (err < 0)
			return err;
	}

	if (src->log_msg_ct) {
		dst->log_msg_ct = (struct nfnl_ct *) nl_object_clone((struct nl_object *) src->log_msg_ct);
		if (!dst->log_msg_ct) {
			return -NLE_NOMEM;
		}
	}

	return 0;
}

static void log_msg_dump(struct nl_object *a, struct nl_dump_params *p)
{
	struct nfnl_log_msg *msg = (struct nfnl_log_msg *) a;
	struct nl_cache *link_cache;
	char buf[64];

	link_cache = nl_cache_mngt_require_safe("route/link");

	nl_new_line(p);

	if (msg->ce_mask & LOG_MSG_ATTR_PREFIX)
		nl_dump(p, "%s", msg->log_msg_prefix);

	if (msg->ce_mask & LOG_MSG_ATTR_INDEV) {
		if (link_cache)
			nl_dump(p, "IN=%s ",
				rtnl_link_i2name(link_cache,
						 msg->log_msg_indev,
						 buf, sizeof(buf)));
		else
			nl_dump(p, "IN=%d ", msg->log_msg_indev);
	}

	if (msg->ce_mask & LOG_MSG_ATTR_PHYSINDEV) {
		if (link_cache)
			nl_dump(p, "PHYSIN=%s ",
				rtnl_link_i2name(link_cache,
						 msg->log_msg_physindev,
						 buf, sizeof(buf)));
		else
			nl_dump(p, "PHYSIN=%d ", msg->log_msg_physindev);
	}

	if (msg->ce_mask & LOG_MSG_ATTR_OUTDEV) {
		if (link_cache)
			nl_dump(p, "OUT=%s ",
				rtnl_link_i2name(link_cache,
						 msg->log_msg_outdev,
						 buf, sizeof(buf)));
		else
			nl_dump(p, "OUT=%d ", msg->log_msg_outdev);
	}

	if (msg->ce_mask & LOG_MSG_ATTR_PHYSOUTDEV) {
		if (link_cache)
			nl_dump(p, "PHYSOUT=%s ",
				rtnl_link_i2name(link_cache,
						 msg->log_msg_physoutdev,
						 buf, sizeof(buf)));
		else
			nl_dump(p, "PHYSOUT=%d ", msg->log_msg_physoutdev);
	}

	if (msg->ce_mask & LOG_MSG_ATTR_HWADDR) {
		int i;

		nl_dump(p, "MAC");
		for (i = 0; i < msg->log_msg_hwaddr_len; i++)
			nl_dump(p, "%c%02x", i?':':'=', msg->log_msg_hwaddr[i]);
		nl_dump(p, " ");
	}

	/* FIXME: parse the payload to get iptables LOG compatible format */

	if (msg->ce_mask & LOG_MSG_ATTR_FAMILY)
		nl_dump(p, "FAMILY=%s ",
			nl_af2str(msg->log_msg_family, buf, sizeof(buf)));

	if (msg->ce_mask & LOG_MSG_ATTR_HWPROTO)
		nl_dump(p, "HWPROTO=%s ",
			nl_ether_proto2str(ntohs(msg->log_msg_hwproto),
					   buf, sizeof(buf)));

	if (msg->ce_mask & LOG_MSG_ATTR_HOOK)
		nl_dump(p, "HOOK=%s ",
			nfnl_inet_hook2str(msg->log_msg_hook,
					   buf, sizeof(buf)));

	if (msg->ce_mask & LOG_MSG_ATTR_MARK)
		nl_dump(p, "MARK=%u ", msg->log_msg_mark);

	if (msg->ce_mask & LOG_MSG_ATTR_PAYLOAD)
		nl_dump(p, "PAYLOADLEN=%d ", msg->log_msg_payload_len);

	if (msg->ce_mask & LOG_MSG_ATTR_UID)
		nl_dump(p, "UID=%u ", msg->log_msg_uid);

	if (msg->ce_mask & LOG_MSG_ATTR_GID)
		nl_dump(p, "GID=%u ", msg->log_msg_gid);

	if (msg->ce_mask & LOG_MSG_ATTR_SEQ)
		nl_dump(p, "SEQ=%d ", msg->log_msg_seq);

	if (msg->ce_mask & LOG_MSG_ATTR_SEQ_GLOBAL)
		nl_dump(p, "SEQGLOBAL=%d ", msg->log_msg_seq_global);

	if (msg->ce_mask & LOG_MSG_ATTR_HWTYPE)
		nl_dump(p, "HWTYPE=%u ", msg->log_msg_hwtype);

	if (msg->ce_mask & LOG_MSG_ATTR_HWLEN)
		nl_dump(p, "HWLEN=%u ", msg->log_msg_hwlen);

	if (msg->ce_mask & LOG_MSG_ATTR_HWHEADER) {
		int i;

		nl_dump(p, "HWHEADER");
		for (i = 0; i < msg->log_msg_hwheader_len; i++)
			nl_dump(p, "%c%02x", i?':':'=', ((uint8_t*) msg->log_msg_hwheader) [i]);
		nl_dump(p, " ");
	}

	if (msg->ce_mask & LOG_MSG_ATTR_VLAN_TAG)
		nl_dump(p, "VLAN=%d CFI=%d PRIO=%d",
		        (int) nfnl_log_msg_get_vlan_id(msg),
		        (int) nfnl_log_msg_get_vlan_cfi(msg),
		        (int) nfnl_log_msg_get_vlan_prio(msg));

	if (msg->ce_mask & LOG_MSG_ATTR_CT_INFO)
		nl_dump(p, "CTINFO=%u ", msg->log_msg_ct_info);

	nl_dump(p, "\n");

	if (msg->ce_mask & LOG_MSG_ATTR_CT)
		ct_obj_ops.oo_dump[NL_DUMP_LINE]((struct nl_object *)msg->log_msg_ct, p);

	if (link_cache)
		nl_cache_put(link_cache);
}

/**
 * @name Allocation/Freeing
 * @{
 */

struct nfnl_log_msg *nfnl_log_msg_alloc(void)
{
	return (struct nfnl_log_msg *) nl_object_alloc(&log_msg_obj_ops);
}

void nfnl_log_msg_get(struct nfnl_log_msg *msg)
{
	nl_object_get((struct nl_object *) msg);
}

void nfnl_log_msg_put(struct nfnl_log_msg *msg)
{
	nl_object_put((struct nl_object *) msg);
}

/** @} */

/**
 * @name Attributes
 * @{
 */

void nfnl_log_msg_set_family(struct nfnl_log_msg *msg, uint8_t family)
{
	msg->log_msg_family = family;
	msg->ce_mask |= LOG_MSG_ATTR_FAMILY;
}

uint8_t nfnl_log_msg_get_family(const struct nfnl_log_msg *msg)
{
	if (msg->ce_mask & LOG_MSG_ATTR_FAMILY)
		return msg->log_msg_family;
	else
		return AF_UNSPEC;
}

void nfnl_log_msg_set_hwproto(struct nfnl_log_msg *msg, uint16_t hwproto)
{
	msg->log_msg_hwproto = hwproto;
	msg->ce_mask |= LOG_MSG_ATTR_HWPROTO;
}

int nfnl_log_msg_test_hwproto(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_HWPROTO);
}

uint16_t nfnl_log_msg_get_hwproto(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_hwproto;
}

void nfnl_log_msg_set_hook(struct nfnl_log_msg *msg, uint8_t hook)
{
	msg->log_msg_hook = hook;
	msg->ce_mask |= LOG_MSG_ATTR_HOOK;
}

int nfnl_log_msg_test_hook(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_HOOK);
}

uint8_t nfnl_log_msg_get_hook(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_hook;
}

void nfnl_log_msg_set_mark(struct nfnl_log_msg *msg, uint32_t mark)
{
	msg->log_msg_mark = mark;
	msg->ce_mask |= LOG_MSG_ATTR_MARK;
}

int nfnl_log_msg_test_mark(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_MARK);
}

uint32_t nfnl_log_msg_get_mark(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_mark;
}

void nfnl_log_msg_set_timestamp(struct nfnl_log_msg *msg, struct timeval *tv)
{
	msg->log_msg_timestamp.tv_sec = tv->tv_sec;
	msg->log_msg_timestamp.tv_usec = tv->tv_usec;
	msg->ce_mask |= LOG_MSG_ATTR_TIMESTAMP;
}

const struct timeval *nfnl_log_msg_get_timestamp(const struct nfnl_log_msg *msg)
{
	if (!(msg->ce_mask & LOG_MSG_ATTR_TIMESTAMP))
		return NULL;
	return &msg->log_msg_timestamp;
}

void nfnl_log_msg_set_indev(struct nfnl_log_msg *msg, uint32_t indev)
{
	msg->log_msg_indev = indev;
	msg->ce_mask |= LOG_MSG_ATTR_INDEV;
}

uint32_t nfnl_log_msg_get_indev(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_indev;
}

void nfnl_log_msg_set_outdev(struct nfnl_log_msg *msg, uint32_t outdev)
{
	msg->log_msg_outdev = outdev;
	msg->ce_mask |= LOG_MSG_ATTR_OUTDEV;
}

uint32_t nfnl_log_msg_get_outdev(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_outdev;
}

void nfnl_log_msg_set_physindev(struct nfnl_log_msg *msg, uint32_t physindev)
{
	msg->log_msg_physindev = physindev;
	msg->ce_mask |= LOG_MSG_ATTR_PHYSINDEV;
}

uint32_t nfnl_log_msg_get_physindev(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_physindev;
}

void nfnl_log_msg_set_physoutdev(struct nfnl_log_msg *msg, uint32_t physoutdev)
{
	msg->log_msg_physoutdev = physoutdev;
	msg->ce_mask |= LOG_MSG_ATTR_PHYSOUTDEV;
}

uint32_t nfnl_log_msg_get_physoutdev(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_physoutdev;
}

void nfnl_log_msg_set_hwaddr(struct nfnl_log_msg *msg, uint8_t *hwaddr, int len)
{
	if (len < 0)
		len = 0;
	else if (((unsigned)len) > sizeof(msg->log_msg_hwaddr))
		len = sizeof(msg->log_msg_hwaddr);
	msg->log_msg_hwaddr_len = len;
	memcpy(msg->log_msg_hwaddr, hwaddr, len);
	msg->ce_mask |= LOG_MSG_ATTR_HWADDR;
}

const uint8_t *nfnl_log_msg_get_hwaddr(const struct nfnl_log_msg *msg, int *len)
{
	if (!(msg->ce_mask & LOG_MSG_ATTR_HWADDR)) {
		*len = 0;
		return NULL;
	}

	*len = msg->log_msg_hwaddr_len;
	return msg->log_msg_hwaddr;
}

int nfnl_log_msg_set_payload(struct nfnl_log_msg *msg, uint8_t *payload, int len)
{
	uint8_t *p = NULL;

	if (len < 0)
		return -NLE_INVAL;

	p = _nl_memdup(payload, len);
	if (!p && len > 0)
		return -NLE_NOMEM;

	free(msg->log_msg_payload);
	msg->log_msg_payload = p;
	msg->log_msg_payload_len = len;
	if (len > 0)
		msg->ce_mask |= LOG_MSG_ATTR_PAYLOAD;
	else
		msg->ce_mask &= ~LOG_MSG_ATTR_PAYLOAD;
	return 0;
}

const void *nfnl_log_msg_get_payload(const struct nfnl_log_msg *msg, int *len)
{
	if (!(msg->ce_mask & LOG_MSG_ATTR_PAYLOAD)) {
		*len = 0;
		return NULL;
	}

	*len = msg->log_msg_payload_len;
	return msg->log_msg_payload;
}

int nfnl_log_msg_set_prefix(struct nfnl_log_msg *msg, void *prefix)
{
	char *p = NULL;

	if (prefix) {
		p = strdup(prefix);
		if (!p)
			return -NLE_NOMEM;
	}

	free(msg->log_msg_prefix);
	msg->log_msg_prefix = p;

	if (p)
		msg->ce_mask |= LOG_MSG_ATTR_PREFIX;
	else
		msg->ce_mask &= ~LOG_MSG_ATTR_PREFIX;
	return 0;
}

const char *nfnl_log_msg_get_prefix(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_prefix;
}

void nfnl_log_msg_set_uid(struct nfnl_log_msg *msg, uint32_t uid)
{
	msg->log_msg_uid = uid;
	msg->ce_mask |= LOG_MSG_ATTR_UID;
}

int nfnl_log_msg_test_uid(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_UID);
}

uint32_t nfnl_log_msg_get_uid(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_uid;
}

void nfnl_log_msg_set_gid(struct nfnl_log_msg *msg, uint32_t gid)
{
	msg->log_msg_gid = gid;
	msg->ce_mask |= LOG_MSG_ATTR_GID;
}

int nfnl_log_msg_test_gid(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_GID);
}

uint32_t nfnl_log_msg_get_gid(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_gid;
}


void nfnl_log_msg_set_seq(struct nfnl_log_msg *msg, uint32_t seq)
{
	msg->log_msg_seq = seq;
	msg->ce_mask |= LOG_MSG_ATTR_SEQ;
}

int nfnl_log_msg_test_seq(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_SEQ);
}

uint32_t nfnl_log_msg_get_seq(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_seq;
}

void nfnl_log_msg_set_seq_global(struct nfnl_log_msg *msg, uint32_t seq_global)
{
	msg->log_msg_seq_global = seq_global;
	msg->ce_mask |= LOG_MSG_ATTR_SEQ_GLOBAL;
}

int nfnl_log_msg_test_seq_global(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_SEQ_GLOBAL);
}

uint32_t nfnl_log_msg_get_seq_global(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_seq_global;
}

void nfnl_log_msg_set_hwtype(struct nfnl_log_msg *msg, uint16_t hwtype)
{
	msg->log_msg_hwtype = hwtype;
	msg->ce_mask |= LOG_MSG_ATTR_HWTYPE;
}

int nfnl_log_msg_test_hwtype(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_HWTYPE);
}

uint16_t nfnl_log_msg_get_hwtype(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_hwtype;
}

void nfnl_log_msg_set_hwlen(struct nfnl_log_msg *msg, uint16_t hwlen)
{
	msg->log_msg_hwlen = hwlen;
	msg->ce_mask |= LOG_MSG_ATTR_HWLEN;
}

int nfnl_log_msg_test_hwlen(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_HWLEN);
}

uint16_t nfnl_log_msg_get_hwlen(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_hwlen;
}

int nfnl_log_msg_set_hwheader(struct nfnl_log_msg *msg, void *data, int len)
{
	void *p = NULL;

	if (len < 0)
		return -NLE_INVAL;

	p = _nl_memdup(data, len);
	if (!p && len > 0)
		return -NLE_NOMEM;

	free(msg->log_msg_hwheader);
	msg->log_msg_hwheader = p;
	msg->log_msg_hwheader_len = len;
	if (len > 0)
		msg->ce_mask |= LOG_MSG_ATTR_HWHEADER;
	else
		msg->ce_mask &= ~LOG_MSG_ATTR_HWHEADER;
	return 0;
}

int nfnl_log_msg_test_hwheader(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_HWHEADER);
}

const void *nfnl_log_msg_get_hwheader(const struct nfnl_log_msg *msg, int *len)
{
	if (!(msg->ce_mask & LOG_MSG_ATTR_HWHEADER)) {
		*len = 0;
		return NULL;
	}

	*len = msg->log_msg_hwheader_len;
	return msg->log_msg_hwheader;
}

void nfnl_log_msg_set_vlan_proto(struct nfnl_log_msg *msg, uint16_t vlan_proto)
{
	msg->log_msg_vlan_proto = vlan_proto;
	msg->ce_mask |= LOG_MSG_ATTR_VLAN_PROTO;
}

int nfnl_log_msg_test_vlan_proto(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_VLAN_PROTO);
}

uint16_t nfnl_log_msg_get_vlan_proto(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_vlan_proto;
}

void nfnl_log_msg_set_vlan_tag(struct nfnl_log_msg *msg, uint16_t vlan_tag)
{
	msg->log_msg_vlan_tag = vlan_tag;
	msg->ce_mask |= LOG_MSG_ATTR_VLAN_TAG;
}

int nfnl_log_msg_test_vlan_tag(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_VLAN_TAG);
}

uint16_t nfnl_log_msg_get_vlan_tag(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_vlan_tag;
}

uint16_t nfnl_log_msg_get_vlan_id(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_vlan_tag & 0x0fff;
}

uint16_t nfnl_log_msg_get_vlan_cfi(const struct nfnl_log_msg *msg)
{
	return !!(msg->log_msg_vlan_tag & 0x1000);
}

uint16_t nfnl_log_msg_get_vlan_prio(const struct nfnl_log_msg *msg)
{
	return (msg->log_msg_vlan_tag & 0xe000 ) >> 13;
}

void nfnl_log_msg_set_ct_info(struct nfnl_log_msg *msg, uint32_t ct_info)
{
	msg->log_msg_ct_info = ct_info;
	msg->ce_mask |= LOG_MSG_ATTR_CT_INFO;
}

int nfnl_log_msg_test_ct_info(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_CT_INFO);
}

uint32_t nfnl_log_msg_get_ct_info(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_ct_info;
}

void nfnl_log_msg_set_ct(struct nfnl_log_msg *msg, struct nfnl_ct *ct)
{
	msg->log_msg_ct = (struct nfnl_ct *) nl_object_clone((struct nl_object *)ct);
	msg->ce_mask |= LOG_MSG_ATTR_CT;
}

int nfnl_log_msg_test_ct(const struct nfnl_log_msg *msg)
{
	return !!(msg->ce_mask & LOG_MSG_ATTR_CT);
}

struct nfnl_ct *nfnl_log_msg_get_ct(const struct nfnl_log_msg *msg)
{
	return msg->log_msg_ct;
}

/** @} */

struct nl_object_ops log_msg_obj_ops = {
	.oo_name		= "netfilter/log_msg",
	.oo_size		= sizeof(struct nfnl_log_msg),
	.oo_free_data		= log_msg_free_data,
	.oo_clone		= log_msg_clone,
	.oo_dump = {
	    [NL_DUMP_LINE]	= log_msg_dump,
	    [NL_DUMP_DETAILS]	= log_msg_dump,
	    [NL_DUMP_STATS]	= log_msg_dump,
	},
};

/** @} */
