/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#ifndef __LIB_XFRM_NL_XFRM_H__
#define __LIB_XFRM_NL_XFRM_H__

/* Lifetime configuration, used for both policy rules (SPD) and SAs. */
struct xfrmnl_ltime_cfg {
	uint32_t refcnt;
	uint64_t soft_byte_limit;
	uint64_t hard_byte_limit;
	uint64_t soft_packet_limit;
	uint64_t hard_packet_limit;
	uint64_t soft_add_expires_seconds;
	uint64_t hard_add_expires_seconds;
	uint64_t soft_use_expires_seconds;
	uint64_t hard_use_expires_seconds;
};

/* Current lifetime, used for both policy rules (SPD) and SAs. */
struct xfrmnl_lifetime_cur {
	uint64_t bytes;
	uint64_t packets;
	uint64_t add_time;
	uint64_t use_time;
};

struct xfrmnl_replay_state {
	uint32_t oseq;
	uint32_t seq;
	uint32_t bitmap;
};

struct xfrmnl_replay_state_esn {
	uint32_t bmp_len;
	uint32_t oseq;
	uint32_t seq;
	uint32_t oseq_hi;
	uint32_t seq_hi;
	uint32_t replay_window;
	uint32_t bmp[0];
};

struct xfrmnl_mark {
	uint32_t v; /* value */
	uint32_t m; /* mask */
};

struct xfrmnl_id {
	struct nl_addr *daddr;
	uint32_t spi;
	uint8_t proto;
};

struct xfrmnl_user_sec_ctx {
	uint16_t len;
	uint16_t exttype;
	uint8_t ctx_alg;
	uint8_t ctx_doi;
	uint16_t ctx_len;
	char ctx[0];
};

struct xfrmnl_user_tmpl {
	struct xfrmnl_id id;
	uint16_t family;
	struct nl_addr *saddr;
	uint32_t reqid;
	uint8_t mode;
	uint8_t share;
	uint8_t optional;
	uint32_t aalgos;
	uint32_t ealgos;
	uint32_t calgos;
	struct nl_list_head utmpl_list;
};

#endif /* __LIB_XFRM_NL_XFRM_H__  */
