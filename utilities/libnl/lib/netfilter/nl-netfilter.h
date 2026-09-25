/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#ifndef __LIB_NETFILTER_NL_NETFILTER_H__
#define __LIB_NETFILTER_NL_NETFILTER_H__

#include <netlink/netfilter/ct.h>

#include "nl-priv-dynamic-core/object-api.h"

union nfnl_ct_proto {
	struct {
		uint16_t src;
		uint16_t dst;
	} port;
	struct {
		uint16_t id;
		uint8_t type;
		uint8_t code;
	} icmp;
};

struct nfnl_ct_dir {
	struct nl_addr *src;
	struct nl_addr *dst;
	union nfnl_ct_proto proto;
	uint64_t packets;
	uint64_t bytes;
};

union nfnl_ct_protoinfo {
	struct {
		uint8_t state;
	} tcp;
};

struct nfnl_ct {
	NLHDR_COMMON

	uint8_t ct_family;
	uint8_t ct_proto;
	union nfnl_ct_protoinfo ct_protoinfo;

	uint32_t ct_status;
	uint32_t ct_status_mask;
	uint32_t ct_timeout;
	uint32_t ct_mark;
	uint32_t ct_use;
	uint32_t ct_id;
	uint16_t ct_zone;

	struct nfnl_ct_dir ct_orig;
	struct nfnl_ct_dir ct_repl;

	struct nfnl_ct_timestamp ct_tstamp;
};

union nfnl_exp_protodata {
	struct {
		uint16_t src;
		uint16_t dst;
	} port;
	struct {
		uint16_t id;
		uint8_t type;
		uint8_t code;
	} icmp;
};

// Allow for different master/expect l4 protocols
struct nfnl_exp_proto {
	uint8_t l4protonum;
	union nfnl_exp_protodata l4protodata;
};

struct nfnl_exp_dir {
	struct nl_addr *src;
	struct nl_addr *dst;
	struct nfnl_exp_proto proto;
};

struct nfnl_exp {
	NLHDR_COMMON

	uint8_t exp_family;
	uint32_t exp_timeout;
	uint32_t exp_id;
	uint16_t exp_zone;
	uint32_t exp_class;
	uint32_t exp_flags;
	char *exp_helper_name;
	char *exp_fn;
	uint8_t exp_nat_dir;

	struct nfnl_exp_dir exp_expect;
	struct nfnl_exp_dir exp_master;
	struct nfnl_exp_dir exp_mask;
	struct nfnl_exp_dir exp_nat;
};

struct nfnl_log_msg {
	NLHDR_COMMON

	uint8_t log_msg_family;
	uint8_t log_msg_hook;
	uint16_t log_msg_hwproto;
	uint32_t log_msg_mark;
	struct timeval log_msg_timestamp;
	uint32_t log_msg_indev;
	uint32_t log_msg_outdev;
	uint32_t log_msg_physindev;
	uint32_t log_msg_physoutdev;
	uint8_t log_msg_hwaddr[8];
	int log_msg_hwaddr_len;
	void *log_msg_payload;
	int log_msg_payload_len;
	char *log_msg_prefix;
	uint32_t log_msg_uid;
	uint32_t log_msg_gid;
	uint32_t log_msg_seq;
	uint32_t log_msg_seq_global;
	uint16_t log_msg_hwtype;
	uint16_t log_msg_hwlen;
	void *log_msg_hwheader;
	int log_msg_hwheader_len;
	uint16_t log_msg_vlan_tag;
	uint16_t log_msg_vlan_proto;
	uint32_t log_msg_ct_info;
	struct nfnl_ct *log_msg_ct;
};

struct nfnl_queue_msg {
	NLHDR_COMMON

	uint16_t queue_msg_group;
	uint8_t queue_msg_family;
	uint8_t queue_msg_hook;
	uint16_t queue_msg_hwproto;
	uint32_t queue_msg_packetid;
	uint32_t queue_msg_mark;
	struct timeval queue_msg_timestamp;
	uint32_t queue_msg_indev;
	uint32_t queue_msg_outdev;
	uint32_t queue_msg_physindev;
	uint32_t queue_msg_physoutdev;
	uint8_t queue_msg_hwaddr[8];
	int queue_msg_hwaddr_len;
	void *queue_msg_payload;
	int queue_msg_payload_len;
	uint32_t queue_msg_verdict;
};

#endif /* __LIB_NETFILTER_NL_NETFILTER_H__*/
