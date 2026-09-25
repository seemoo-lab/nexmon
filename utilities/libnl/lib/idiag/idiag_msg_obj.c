/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#include "nl-default.h"

#include <linux/inet_diag.h>

#include <netlink/hashtable.h>
#include <netlink/idiag/msg.h>
#include <netlink/idiag/meminfo.h>
#include <netlink/idiag/vegasinfo.h>
#include <netlink/idiag/idiagnl.h>

#include "nl-idiag.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-aux-core/nl-core.h"
#include "nl-priv-dynamic-core/cache-api.h"

/** @cond SKIP */
struct idiagnl_msg {
	NLHDR_COMMON

	uint8_t			    idiag_family;
	uint8_t			    idiag_state;
	uint8_t			    idiag_timer;
	uint8_t			    idiag_retrans;
	uint16_t		    idiag_sport;
	uint16_t		    idiag_dport;
	struct nl_addr *	    idiag_src;
	struct nl_addr *	    idiag_dst;
	uint32_t		    idiag_ifindex;
	uint32_t		    idiag_expires;
	uint32_t		    idiag_rqueue;
	uint32_t		    idiag_wqueue;
	uint32_t		    idiag_uid;
	uint32_t		    idiag_inode;

	uint8_t			    idiag_tos;
	uint8_t			    idiag_tclass;
	uint8_t			    idiag_shutdown;
	char *			    idiag_cong;
	struct idiagnl_meminfo *    idiag_meminfo;
	struct idiagnl_vegasinfo *  idiag_vegasinfo;
	struct tcp_info		    idiag_tcpinfo;
	uint32_t		    idiag_skmeminfo[SK_MEMINFO_VARS];
};

#define IDIAGNL_ATTR_FAMILY                     (0x1 << 1)
#define IDIAGNL_ATTR_STATE                      (0x1 << 2)
#define IDIAGNL_ATTR_TIMER                      (0x1 << 3)
#define IDIAGNL_ATTR_RETRANS                    (0x1 << 4)
#define IDIAGNL_ATTR_SPORT                      (0x1 << 5)
#define IDIAGNL_ATTR_DPORT                      (0x1 << 6)
#define IDIAGNL_ATTR_SRC                        (0x1 << 7)
#define IDIAGNL_ATTR_DST                        (0x1 << 8)
#define IDIAGNL_ATTR_IFINDEX                    (0x1 << 9)
#define IDIAGNL_ATTR_EXPIRES                    (0x1 << 10)
#define IDIAGNL_ATTR_RQUEUE                     (0x1 << 11)
#define IDIAGNL_ATTR_WQUEUE                     (0x1 << 12)
#define IDIAGNL_ATTR_UID                        (0x1 << 13)
#define IDIAGNL_ATTR_INODE                      (0x1 << 14)
#define IDIAGNL_ATTR_TOS                        (0x1 << 15)
#define IDIAGNL_ATTR_TCLASS                     (0x1 << 16)
#define IDIAGNL_ATTR_SHUTDOWN                   (0x1 << 17)
#define IDIAGNL_ATTR_CONG                       (0x1 << 18)
#define IDIAGNL_ATTR_MEMINFO                    (0x1 << 19)
#define IDIAGNL_ATTR_VEGASINFO                  (0x1 << 20)
#define IDIAGNL_ATTR_TCPINFO                    (0x1 << 21)
#define IDIAGNL_ATTR_SKMEMINFO                  (0x1 << 22)

#define _INET_DIAG_ALL ((1<<(INET_DIAG_MAX+1))-1)
/** @endcond */

/**
 * @ingroup idiag
 * @defgroup idiagnl_msg Inet Diag Messages
 *
 * @details
 * @idiagnl_doc{idiagnl_msg, Inet Diag Message Documentation}
 * @{
 */
struct idiagnl_msg *idiagnl_msg_alloc(void)
{
	return (struct idiagnl_msg *) nl_object_alloc(&idiagnl_msg_obj_ops);
}

void idiagnl_msg_get(struct idiagnl_msg *msg)
{
	nl_object_get((struct nl_object *) msg);
}

void idiagnl_msg_put(struct idiagnl_msg *msg)
{
	nl_object_put((struct nl_object *) msg);
}

static struct nl_cache_ops idiagnl_msg_ops;

static int idiagnl_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
		struct nlmsghdr *nlh, struct nl_parser_param *pp)
{
	struct idiagnl_msg *msg = NULL;
	int err = 0;

	if ((err = idiagnl_msg_parse(nlh, &msg)) < 0)
		return err;

	err = pp->pp_cb((struct nl_object *) msg, pp);
	idiagnl_msg_put(msg);

	return err;
}

static int idiagnl_request_update(struct nl_cache *cache, struct nl_sock *sk)
{
	int family = cache->c_iarg1;
	int states = cache->c_iarg2;

	/* idiagnl_send_simple()'s "ext" argument is u16, which is too small for _INET_DIAG_ALL,
	 * which is more than 16 bits on recent kernels.
	 *
	 * Actually, internally idiagnl_send_simple() sets "struct inet_diag_req"'s "idiag_ext"
	 * field, which is only 8 bits. So, it's even worse.
	 *
	 * FIXME: this probably should be fixed (by adding idiagnl_send_simple2() function), but for
	 *    the moment it means we cannot request more than 0xFF.
	 */

	return idiagnl_send_simple(sk, 0, family, states, (uint16_t) _INET_DIAG_ALL);
}

static struct nl_cache_ops idiagnl_msg_ops = {
	.co_name		= "idiag/idiag",
	.co_hdrsize		= sizeof(struct inet_diag_msg),
	.co_msgtypes		= {
		{ TCPDIAG_GETSOCK, NL_ACT_NEW, "new" },
		{ DCCPDIAG_GETSOCK, NL_ACT_NEW, "new" },
		END_OF_MSGTYPES_LIST,
	},
	.co_protocol		= NETLINK_INET_DIAG,
	.co_request_update	= idiagnl_request_update,
	.co_msg_parser		= idiagnl_msg_parser,
	.co_obj_ops		= &idiagnl_msg_obj_ops,
};

static void _nl_init idiagnl_init(void)
{
	nl_cache_mngt_register(&idiagnl_msg_ops);
}

static void _nl_exit idiagnl_exit(void)
{
	nl_cache_mngt_unregister(&idiagnl_msg_ops);
}

/**
 * @name Cache Management
 * @{
 */

/**
 * Build an inetdiag cache to hold socket state information.
 * @arg	sk      Netlink socket
 * @arg family  The address family to query
 * @arg states  Socket states to query
 * @arg result  Result pointer
 *
 * @note The caller is responsible for destroying and free the cache after using
 *  it.
 * @return 0 on success of a negative error code.
 */
int idiagnl_msg_alloc_cache(struct nl_sock *sk, int family, int states,
		struct nl_cache **result)
{
	struct nl_cache *cache = NULL;
	int err;

	if (!(cache = nl_cache_alloc(&idiagnl_msg_ops)))
		return -NLE_NOMEM;

	cache->c_iarg1 = family;
	cache->c_iarg2 = states;

	if (sk && (err = nl_cache_refill(sk, cache)) < 0) {
		free(cache);
		return err;
	}

	*result = cache;
	return 0;
}

/** @} */

/**
 * @name Attributes
 * @{
 */

uint8_t idiagnl_msg_get_family(const struct idiagnl_msg *msg)
{
	return msg->idiag_family;
}

void idiagnl_msg_set_family(struct idiagnl_msg *msg, uint8_t family)
{
	msg->idiag_family = family;
	msg->ce_mask |= IDIAGNL_ATTR_FAMILY;
}

uint8_t idiagnl_msg_get_state(const struct idiagnl_msg *msg)
{
	return msg->idiag_state;
}

void idiagnl_msg_set_state(struct idiagnl_msg *msg, uint8_t state)
{
	msg->idiag_state = state;
	msg->ce_mask |= IDIAGNL_ATTR_STATE;
}

uint8_t idiagnl_msg_get_timer(const struct idiagnl_msg *msg)
{
	return msg->idiag_timer;
}

void idiagnl_msg_set_timer(struct idiagnl_msg *msg, uint8_t timer)
{
	msg->idiag_timer = timer;
	msg->ce_mask |= IDIAGNL_ATTR_TIMER;
}

uint8_t idiagnl_msg_get_retrans(const struct idiagnl_msg *msg)
{
	return msg->idiag_retrans;
}

void idiagnl_msg_set_retrans(struct idiagnl_msg *msg, uint8_t retrans)
{
	msg->idiag_retrans = retrans;
	msg->ce_mask |= IDIAGNL_ATTR_RETRANS;
}

uint16_t idiagnl_msg_get_sport(struct idiagnl_msg *msg)
{
	return msg->idiag_sport;
}

void idiagnl_msg_set_sport(struct idiagnl_msg *msg, uint16_t port)
{
	msg->idiag_sport = port;
	msg->ce_mask |= IDIAGNL_ATTR_SPORT;
}

uint16_t idiagnl_msg_get_dport(struct idiagnl_msg *msg)
{
	return msg->idiag_dport;
}

void idiagnl_msg_set_dport(struct idiagnl_msg *msg, uint16_t port)
{
	msg->idiag_dport = port;
	msg->ce_mask |= IDIAGNL_ATTR_DPORT;
}

struct nl_addr *idiagnl_msg_get_src(const struct idiagnl_msg *msg)
{
	return msg->idiag_src;
}

int idiagnl_msg_set_src(struct idiagnl_msg *msg, struct nl_addr *addr)
{
	if (msg->idiag_src)
		nl_addr_put(msg->idiag_src);

	nl_addr_get(addr);
	msg->idiag_src = addr;
	msg->ce_mask |= IDIAGNL_ATTR_SRC;

	return 0;
}

struct nl_addr *idiagnl_msg_get_dst(const struct idiagnl_msg *msg)
{
	return msg->idiag_dst;
}

int idiagnl_msg_set_dst(struct idiagnl_msg *msg, struct nl_addr *addr)
{
	if (msg->idiag_dst)
		nl_addr_put(msg->idiag_dst);

	nl_addr_get(addr);
	msg->idiag_dst = addr;
	msg->ce_mask |= IDIAGNL_ATTR_DST;

	return 0;
}

uint32_t idiagnl_msg_get_ifindex(const struct idiagnl_msg *msg)
{
	return msg->idiag_ifindex;
}

void idiagnl_msg_set_ifindex(struct idiagnl_msg *msg, uint32_t ifindex)
{
	msg->idiag_ifindex = ifindex;
	msg->ce_mask |= IDIAGNL_ATTR_IFINDEX;
}

uint32_t idiagnl_msg_get_expires(const struct idiagnl_msg *msg)
{
	return msg->idiag_expires;
}

void idiagnl_msg_set_expires(struct idiagnl_msg *msg, uint32_t expires)
{
	msg->idiag_expires = expires;
	msg->ce_mask |= IDIAGNL_ATTR_EXPIRES;
}

uint32_t idiagnl_msg_get_rqueue(const struct idiagnl_msg *msg)
{
	return msg->idiag_rqueue;
}

void idiagnl_msg_set_rqueue(struct idiagnl_msg *msg, uint32_t rqueue)
{
	msg->idiag_rqueue = rqueue;
	msg->ce_mask |= IDIAGNL_ATTR_RQUEUE;
}

uint32_t idiagnl_msg_get_wqueue(const struct idiagnl_msg *msg)
{
	return msg->idiag_wqueue;
}

void idiagnl_msg_set_wqueue(struct idiagnl_msg *msg, uint32_t wqueue)
{
	msg->idiag_wqueue = wqueue;
	msg->ce_mask |= IDIAGNL_ATTR_WQUEUE;
}

uint32_t idiagnl_msg_get_uid(const struct idiagnl_msg *msg)
{
	return msg->idiag_uid;
}

void idiagnl_msg_set_uid(struct idiagnl_msg *msg, uint32_t uid)
{
	msg->idiag_uid = uid;
	msg->ce_mask |= IDIAGNL_ATTR_UID;
}

uint32_t idiagnl_msg_get_inode(const struct idiagnl_msg *msg)
{
	return msg->idiag_inode;
}

void idiagnl_msg_set_inode(struct idiagnl_msg *msg, uint32_t inode)
{
	msg->idiag_inode = inode;
	msg->ce_mask |= IDIAGNL_ATTR_INODE;
}

uint8_t idiagnl_msg_get_tos(const struct idiagnl_msg *msg)
{
	return msg->idiag_tos;
}

void idiagnl_msg_set_tos(struct idiagnl_msg *msg, uint8_t tos)
{
	msg->idiag_tos = tos;
	msg->ce_mask |= IDIAGNL_ATTR_TOS;
}

uint8_t idiagnl_msg_get_tclass(const struct idiagnl_msg *msg)
{
	return msg->idiag_tclass;
}

void idiagnl_msg_set_tclass(struct idiagnl_msg *msg, uint8_t tclass)
{
	msg->idiag_tclass = tclass;
	msg->ce_mask |= IDIAGNL_ATTR_TCLASS;
}

uint8_t	idiagnl_msg_get_shutdown(const struct idiagnl_msg *msg)
{
	return msg->idiag_shutdown;
}

void  idiagnl_msg_set_shutdown(struct idiagnl_msg *msg, uint8_t shutdown)
{
	msg->idiag_shutdown = shutdown;
	msg->ce_mask |= IDIAGNL_ATTR_SHUTDOWN;
}

char *idiagnl_msg_get_cong(const struct idiagnl_msg *msg)
{
	return msg->idiag_cong;
}

void idiagnl_msg_set_cong(struct idiagnl_msg *msg, char *cong)
{
	free (msg->idiag_cong);
	msg->idiag_cong = strdup(cong);
	msg->ce_mask |= IDIAGNL_ATTR_CONG;
}

struct idiagnl_meminfo *idiagnl_msg_get_meminfo(const struct idiagnl_msg *msg)
{
	return msg->idiag_meminfo;
}

void idiagnl_msg_set_meminfo(struct idiagnl_msg *msg, struct idiagnl_meminfo *minfo)
{
	if (msg->idiag_meminfo)
		idiagnl_meminfo_put(msg->idiag_meminfo);

	idiagnl_meminfo_get(minfo);
	msg->idiag_meminfo = minfo;
	msg->ce_mask |= IDIAGNL_ATTR_MEMINFO;
}

struct idiagnl_vegasinfo *idiagnl_msg_get_vegasinfo(const struct idiagnl_msg *msg)
{
	return msg->idiag_vegasinfo;
}

void idiagnl_msg_set_vegasinfo(struct idiagnl_msg *msg, struct idiagnl_vegasinfo *vinfo)
{
	if (msg->idiag_vegasinfo)
		idiagnl_vegasinfo_put(msg->idiag_vegasinfo);

	idiagnl_vegasinfo_get(vinfo);
	msg->idiag_vegasinfo = vinfo;
	msg->ce_mask |= IDIAGNL_ATTR_VEGASINFO;
}

struct tcp_info idiagnl_msg_get_tcpinfo(const struct idiagnl_msg *msg)
{
	return msg->idiag_tcpinfo;
}

void idiagnl_msg_set_tcpinfo(struct idiagnl_msg *msg, struct tcp_info *tinfo)
{
	memcpy(&msg->idiag_tcpinfo, tinfo, sizeof(struct tcp_info));
	msg->ce_mask |= IDIAGNL_ATTR_TCPINFO;
}

/** @} */

static void idiag_msg_dump_line(struct nl_object *a, struct nl_dump_params *p)
{
	struct idiagnl_msg *msg = (struct idiagnl_msg *) a;
	char buf[64] = { 0 };

	nl_dump_line(p, "family: %s ", nl_af2str(msg->idiag_family, buf, sizeof(buf)));
	nl_dump(p, "src: %s:%d ", nl_addr2str(msg->idiag_src, buf, sizeof(buf)),
			ntohs(msg->idiag_sport));
	nl_dump(p, "dst: %s:%d ", nl_addr2str(msg->idiag_dst, buf, sizeof(buf)),
			ntohs(msg->idiag_dport));
	nl_dump(p, "iif: %d ", msg->idiag_ifindex);
	nl_dump(p, "\n");
}

static void idiag_msg_dump_details(struct nl_object *a, struct nl_dump_params *p)
{
	struct idiagnl_msg *msg = (struct idiagnl_msg *) a;
	char buf[64], buf2[64];

	nl_dump(p, "\nfamily: %s\n", nl_af2str(msg->idiag_family, buf, sizeof(buf)));
	nl_dump(p, "state: %s\n",
			idiagnl_state2str(msg->idiag_state, buf, sizeof(buf)));
	nl_dump(p, "timer (%s, %s, retransmits: %d)\n",
			idiagnl_timer2str(msg->idiag_timer, buf, sizeof(buf)),
			nl_msec2str(msg->idiag_expires, buf2, sizeof(buf2)),
			msg->idiag_retrans);

	nl_dump(p, "source: %s:%d\n", nl_addr2str(msg->idiag_src, buf, sizeof(buf)),
			ntohs(msg->idiag_sport));
	nl_dump(p, "destination: %s:%d\n", nl_addr2str(msg->idiag_dst, buf, sizeof(buf)),
			ntohs(msg->idiag_dport));

	nl_dump(p, "ifindex: %d\n", msg->idiag_ifindex);
	nl_dump(p, "rqueue: %-6d wqueue: %-6d\n", msg->idiag_rqueue, msg->idiag_wqueue);
	nl_dump(p, "uid %d\n", msg->idiag_uid);
	nl_dump(p, "inode %d\n", msg->idiag_inode);
	if (msg->idiag_shutdown) {
		nl_dump(p, "socket shutdown: %s\n",
				idiagnl_shutdown2str(msg->idiag_shutdown,
					buf, sizeof(buf)));
	}

	nl_dump(p, "tos: 0x%x\n", msg->idiag_tos);
	nl_dump(p, "traffic class: %d\n", msg->idiag_tclass);
	nl_dump(p, "congestion algorithm: %s\n", msg->idiag_cong ? msg->idiag_cong : "");
}

static void idiag_msg_dump_stats(struct nl_object *obj, struct nl_dump_params *p)
{
	struct idiagnl_msg *msg = (struct idiagnl_msg *) obj;
	char buf[64];

	idiag_msg_dump_details(obj, p);

	nl_dump(p, "tcp info:  [\n");
	nl_dump(p, "\tsocket state: %s\n",
			idiagnl_state2str(msg->idiag_tcpinfo.tcpi_state,
				buf, sizeof(buf)));
	nl_dump(p, "\ttcp state: %s\n",
			idiagnl_tcpstate2str(msg->idiag_tcpinfo.tcpi_ca_state,
				buf, sizeof(buf)));
	nl_dump(p, "\tretransmits: %d\n",
			msg->idiag_tcpinfo.tcpi_retransmits);
	nl_dump(p, "\tprobes: %d\n",
			msg->idiag_tcpinfo.tcpi_probes);
	nl_dump(p, "\tbackoff: %d\n",
			msg->idiag_tcpinfo.tcpi_backoff);
	nl_dump(p, "\toptions: %s\n",
			idiagnl_tcpopts2str(msg->idiag_tcpinfo.tcpi_options,
				buf, sizeof(buf)));
	nl_dump(p, "\tsnd_wscale: %d\n", msg->idiag_tcpinfo.tcpi_snd_wscale);
	nl_dump(p, "\trcv_wscale: %d\n", msg->idiag_tcpinfo.tcpi_rcv_wscale);
	nl_dump(p, "\trto: %d\n", msg->idiag_tcpinfo.tcpi_rto);
	nl_dump(p, "\tato: %d\n", msg->idiag_tcpinfo.tcpi_ato);
	nl_dump(p, "\tsnd_mss: %s\n", nl_size2str(msg->idiag_tcpinfo.tcpi_snd_mss,
				buf, sizeof(buf)));
	nl_dump(p, "\trcv_mss: %s\n", nl_size2str(msg->idiag_tcpinfo.tcpi_rcv_mss,
				buf, sizeof(buf)));
	nl_dump(p, "\tunacked: %d\n", msg->idiag_tcpinfo.tcpi_unacked);
	nl_dump(p, "\tsacked: %d\n", msg->idiag_tcpinfo.tcpi_sacked);

	nl_dump(p, "\tlost: %d\n", msg->idiag_tcpinfo.tcpi_lost);
	nl_dump(p, "\tretransmit segments: %d\n",
			msg->idiag_tcpinfo.tcpi_retrans);
	nl_dump(p, "\tfackets: %d\n",
			msg->idiag_tcpinfo.tcpi_fackets);
	nl_dump(p, "\tlast data sent: %s\n",
			nl_msec2str(msg->idiag_tcpinfo.tcpi_last_data_sent, buf,
				sizeof(buf)));
	nl_dump(p, "\tlast ack sent: %s\n",
			nl_msec2str(msg->idiag_tcpinfo.tcpi_last_ack_sent, buf, sizeof(buf)));
	nl_dump(p, "\tlast data recv: %s\n",
			nl_msec2str(msg->idiag_tcpinfo.tcpi_last_data_recv, buf,
				sizeof(buf)));
	nl_dump(p, "\tlast ack recv: %s\n",
			nl_msec2str(msg->idiag_tcpinfo.tcpi_last_ack_recv, buf,
				sizeof(buf)));
	nl_dump(p, "\tpath mtu: %s\n",
			nl_size2str(msg->idiag_tcpinfo.tcpi_pmtu, buf,
				sizeof(buf)));
	nl_dump(p, "\trcv ss threshold: %d\n",
			msg->idiag_tcpinfo.tcpi_rcv_ssthresh);
	nl_dump(p, "\tsmoothed round trip time: %d\n",
			msg->idiag_tcpinfo.tcpi_rtt);
	nl_dump(p, "\tround trip time variation: %d\n",
			msg->idiag_tcpinfo.tcpi_rttvar);
	nl_dump(p, "\tsnd ss threshold: %s\n",
			nl_size2str(msg->idiag_tcpinfo.tcpi_snd_ssthresh, buf,
				sizeof(buf)));
	nl_dump(p, "\tsend congestion window: %d\n",
			msg->idiag_tcpinfo.tcpi_snd_cwnd);
	nl_dump(p, "\tadvertised mss: %s\n",
			nl_size2str(msg->idiag_tcpinfo.tcpi_advmss, buf,
				sizeof(buf)));
	nl_dump(p, "\treordering: %d\n",
			msg->idiag_tcpinfo.tcpi_reordering);
	nl_dump(p, "\trcv rround trip time: %d\n",
			msg->idiag_tcpinfo.tcpi_rcv_rtt);
	nl_dump(p, "\treceive queue space: %s\n",
			nl_size2str(msg->idiag_tcpinfo.tcpi_rcv_space, buf,
				sizeof(buf)));
	nl_dump(p, "\ttotal retransmits: %d\n",
			msg->idiag_tcpinfo.tcpi_total_retrans);
	nl_dump(p, "]\n");

	if (msg->idiag_meminfo) {
		nl_dump(p, "meminfo:  [\n");
		nl_dump(p, "\trmem: %s\n",
				nl_size2str(msg->idiag_meminfo->idiag_rmem,
					    buf,
					    sizeof(buf)));
		nl_dump(p, "\twmem: %s\n",
				nl_size2str(msg->idiag_meminfo->idiag_wmem,
					    buf,
					    sizeof(buf)));
		nl_dump(p, "\tfmem: %s\n",
				nl_size2str(msg->idiag_meminfo->idiag_fmem,
					    buf,
					    sizeof(buf)));
		nl_dump(p, "\ttmem: %s\n",
				nl_size2str(msg->idiag_meminfo->idiag_tmem,
					    buf,
					    sizeof(buf)));
		nl_dump(p, "]\n");
	}

	if (msg->idiag_vegasinfo) {
		nl_dump(p, "vegasinfo:  [\n");
		nl_dump(p, "\tvegas enabled: %d\n",
				msg->idiag_vegasinfo->tcpv_enabled);
		if (msg->idiag_vegasinfo->tcpv_enabled) {
			nl_dump(p, "\trtt cnt: %d",
					msg->idiag_vegasinfo->tcpv_rttcnt);
			nl_dump(p, "\trtt (propagation delay): %d",
					msg->idiag_vegasinfo->tcpv_rtt);
			nl_dump(p, "\tmin rtt: %d",
					msg->idiag_vegasinfo->tcpv_minrtt);
		}
		nl_dump(p, "]\n");
	}

	if (msg->ce_mask & IDIAGNL_ATTR_MEMINFO) {
		nl_dump(p, "skmeminfo:  [\n");
		nl_dump(p, "\trmem alloc: %d\n",
				msg->idiag_skmeminfo[SK_MEMINFO_RMEM_ALLOC]);
		nl_dump(p, "\trcv buf: %s\n",
				nl_size2str(msg->idiag_skmeminfo[SK_MEMINFO_RCVBUF],
					buf, sizeof(buf)));
		nl_dump(p, "\twmem alloc: %d\n",
				msg->idiag_skmeminfo[SK_MEMINFO_WMEM_ALLOC]);
		nl_dump(p, "\tsnd buf: %s\n",
				nl_size2str(msg->idiag_skmeminfo[SK_MEMINFO_SNDBUF],
					buf, sizeof(buf)));
		nl_dump(p, "\tfwd alloc: %d\n",
				msg->idiag_skmeminfo[SK_MEMINFO_FWD_ALLOC]);
		nl_dump(p, "\twmem queued: %s\n",
				nl_size2str(msg->idiag_skmeminfo[SK_MEMINFO_WMEM_QUEUED],
					buf, sizeof(buf)));
		nl_dump(p, "\topt mem: %d\n",
				msg->idiag_skmeminfo[SK_MEMINFO_OPTMEM]);
		nl_dump(p, "\tbacklog: %d\n",
				msg->idiag_skmeminfo[SK_MEMINFO_BACKLOG]);
		nl_dump(p, "]\n\n");
	}
}

static void idiagnl_msg_free(struct nl_object *a)
{
	struct idiagnl_msg *msg = (struct idiagnl_msg *) a;
	if (a == NULL)
		return;

	free(msg->idiag_cong);
	nl_addr_put(msg->idiag_src);
	nl_addr_put(msg->idiag_dst);
	idiagnl_meminfo_put(msg->idiag_meminfo);
	idiagnl_vegasinfo_put(msg->idiag_vegasinfo);
}

static int idiagnl_msg_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct idiagnl_msg *dst = (struct idiagnl_msg *) _dst;
	struct idiagnl_msg *src = (struct idiagnl_msg *) _src;

	dst->idiag_src = NULL;
	dst->idiag_dst = NULL;
	dst->idiag_cong = NULL;
	dst->idiag_meminfo = NULL;
	dst->idiag_vegasinfo = NULL;
	dst->ce_mask &= ~(IDIAGNL_ATTR_CONG |
	                  IDIAGNL_ATTR_SRC |
	                  IDIAGNL_ATTR_DST |
	                  IDIAGNL_ATTR_MEMINFO |
	                  IDIAGNL_ATTR_VEGASINFO);

	if (src->idiag_cong) {
		if (!(dst->idiag_cong = strdup(src->idiag_cong)))
			return -NLE_NOMEM;
		dst->ce_mask |= IDIAGNL_ATTR_CONG;
	}

	if (src->idiag_src) {
		if (!(dst->idiag_src = nl_addr_clone(src->idiag_src)))
			return -NLE_NOMEM;
		dst->ce_mask |= IDIAGNL_ATTR_SRC;
	}

	if (src->idiag_dst) {
		if (!(dst->idiag_dst = nl_addr_clone(src->idiag_dst)))
			return -NLE_NOMEM;
		dst->ce_mask |= IDIAGNL_ATTR_DST;
	}

	if (src->idiag_meminfo) {
		if (!(dst->idiag_meminfo = (struct idiagnl_meminfo *) nl_object_clone((struct nl_object *) src->idiag_meminfo)))
			return -NLE_NOMEM;
		dst->ce_mask |= IDIAGNL_ATTR_MEMINFO;
	}

	if (src->idiag_vegasinfo) {
		if (!(dst->idiag_vegasinfo = (struct idiagnl_vegasinfo *) nl_object_clone((struct nl_object *) src->idiag_vegasinfo)))
			return -NLE_NOMEM;
		dst->ce_mask |= IDIAGNL_ATTR_VEGASINFO;
	}

	return 0;
}

static struct nla_policy ext_policy[INET_DIAG_MAX+1] = {
	[INET_DIAG_MEMINFO]    = { .minlen = sizeof(struct inet_diag_meminfo) },
	[INET_DIAG_INFO]       = { .minlen = sizeof(struct tcp_info)	},
	[INET_DIAG_VEGASINFO]  = { .minlen = sizeof(struct tcpvegas_info) },
	[INET_DIAG_CONG]       = { .type = NLA_STRING },
	[INET_DIAG_TOS]        = { .type = NLA_U8 },
	[INET_DIAG_TCLASS]     = { .type = NLA_U8 },
	/* Older kernel doesn't have SK_MEMINFO_BACKLOG */
	[INET_DIAG_SKMEMINFO]  = { .minlen = (sizeof(uint32_t) * (SK_MEMINFO_OPTMEM + 1)) },
	[INET_DIAG_SHUTDOWN]   = { .type = NLA_U8 },
};

int idiagnl_msg_parse(struct nlmsghdr *nlh, struct idiagnl_msg **result)
{
	struct idiagnl_msg *msg = NULL;
	struct inet_diag_msg *raw_msg = NULL;
	struct nl_addr *src = NULL, *dst = NULL;
	struct nlattr *tb[INET_DIAG_MAX+1];
	int err = 0;

	msg = idiagnl_msg_alloc();
	if (!msg)
		goto errout_nomem;

	err = nlmsg_parse(nlh, sizeof(struct inet_diag_msg), tb, INET_DIAG_MAX,
			ext_policy);
	if (err < 0)
		goto errout;

	raw_msg = nlmsg_data(nlh);
	msg->idiag_family = raw_msg->idiag_family;
	msg->idiag_state = raw_msg->idiag_state;
	msg->idiag_timer = raw_msg->idiag_timer;
	msg->idiag_retrans = raw_msg->idiag_retrans;
	msg->idiag_expires = raw_msg->idiag_expires;
	msg->idiag_rqueue = raw_msg->idiag_rqueue;
	msg->idiag_wqueue = raw_msg->idiag_wqueue;
	msg->idiag_uid = raw_msg->idiag_uid;
	msg->idiag_inode = raw_msg->idiag_inode;
	msg->idiag_sport = raw_msg->id.idiag_sport;
	msg->idiag_dport = raw_msg->id.idiag_dport;
	msg->idiag_ifindex = raw_msg->id.idiag_if;

	msg->ce_mask = (IDIAGNL_ATTR_FAMILY |
	                IDIAGNL_ATTR_STATE |
	                IDIAGNL_ATTR_TIMER |
	                IDIAGNL_ATTR_RETRANS |
	                IDIAGNL_ATTR_EXPIRES |
	                IDIAGNL_ATTR_RQUEUE |
	                IDIAGNL_ATTR_WQUEUE |
	                IDIAGNL_ATTR_UID |
	                IDIAGNL_ATTR_INODE |
	                IDIAGNL_ATTR_SPORT |
	                IDIAGNL_ATTR_DPORT |
	                IDIAGNL_ATTR_IFINDEX);

	dst = nl_addr_build(raw_msg->idiag_family, raw_msg->id.idiag_dst,
			sizeof(raw_msg->id.idiag_dst));
	if (!dst)
		goto errout_nomem;

	err = idiagnl_msg_set_dst(msg, dst);
	if (err < 0)
		goto errout;

	nl_addr_put(dst);

	src = nl_addr_build(raw_msg->idiag_family, raw_msg->id.idiag_src,
			sizeof(raw_msg->id.idiag_src));
	if (!src)
		goto errout_nomem;

	err = idiagnl_msg_set_src(msg, src);
	if (err < 0)
		goto errout;

	nl_addr_put(src);

	if (tb[INET_DIAG_TOS]) {
		msg->idiag_tos = nla_get_u8(tb[INET_DIAG_TOS]);
		msg->ce_mask |= IDIAGNL_ATTR_TOS;
	}

	if (tb[INET_DIAG_TCLASS]) {
		msg->idiag_tclass = nla_get_u8(tb[INET_DIAG_TCLASS]);
		msg->ce_mask |= IDIAGNL_ATTR_TCLASS;
	}

	if (tb[INET_DIAG_SHUTDOWN]) {
		msg->idiag_shutdown = nla_get_u8(tb[INET_DIAG_SHUTDOWN]);
		msg->ce_mask |= IDIAGNL_ATTR_SHUTDOWN;
	}

	if (tb[INET_DIAG_CONG]) {
		msg->idiag_cong = nla_strdup(tb[INET_DIAG_CONG]);
		msg->ce_mask |= IDIAGNL_ATTR_CONG;
	}

	if (tb[INET_DIAG_INFO]) {
		nla_memcpy(&msg->idiag_tcpinfo, tb[INET_DIAG_INFO],
				sizeof(msg->idiag_tcpinfo));
		msg->ce_mask |= IDIAGNL_ATTR_TCPINFO;
	}

	if (tb[INET_DIAG_MEMINFO]) {
		struct idiagnl_meminfo *minfo = idiagnl_meminfo_alloc();
		struct inet_diag_meminfo *raw_minfo = NULL;

		if (!minfo)
			goto errout_nomem;

		raw_minfo = (struct inet_diag_meminfo *)
			nla_data(tb[INET_DIAG_MEMINFO]);

		idiagnl_meminfo_set_rmem(minfo, raw_minfo->idiag_rmem);
		idiagnl_meminfo_set_wmem(minfo, raw_minfo->idiag_wmem);
		idiagnl_meminfo_set_fmem(minfo, raw_minfo->idiag_fmem);
		idiagnl_meminfo_set_tmem(minfo, raw_minfo->idiag_tmem);

		msg->idiag_meminfo = minfo;
		msg->ce_mask |= IDIAGNL_ATTR_MEMINFO;
	}

	if (tb[INET_DIAG_VEGASINFO]) {
		struct idiagnl_vegasinfo *vinfo = idiagnl_vegasinfo_alloc();
		struct tcpvegas_info *raw_vinfo = NULL;

		if (!vinfo)
			goto errout_nomem;

		raw_vinfo = (struct tcpvegas_info *)
			nla_data(tb[INET_DIAG_VEGASINFO]);

		idiagnl_vegasinfo_set_enabled(vinfo, raw_vinfo->tcpv_enabled);
		idiagnl_vegasinfo_set_rttcnt(vinfo, raw_vinfo->tcpv_rttcnt);
		idiagnl_vegasinfo_set_rtt(vinfo, raw_vinfo->tcpv_rtt);
		idiagnl_vegasinfo_set_minrtt(vinfo, raw_vinfo->tcpv_minrtt);

		msg->idiag_vegasinfo = vinfo;
		msg->ce_mask |= IDIAGNL_ATTR_VEGASINFO;
	}

	if (tb[INET_DIAG_SKMEMINFO]) {
		nla_memcpy(&msg->idiag_skmeminfo, tb[INET_DIAG_SKMEMINFO],
				sizeof(msg->idiag_skmeminfo));
		msg->ce_mask |= IDIAGNL_ATTR_SKMEMINFO;
	}

	*result = msg;
	return 0;

errout:
	idiagnl_msg_put(msg);
	return err;

errout_nomem:
	err = -NLE_NOMEM;
	goto errout;
}

static const struct trans_tbl idiagnl_attrs[] = {
	__ADD(IDIAGNL_ATTR_FAMILY, family),
	__ADD(IDIAGNL_ATTR_STATE, state),
	__ADD(IDIAGNL_ATTR_TIMER, timer),
	__ADD(IDIAGNL_ATTR_RETRANS, retrans),
	__ADD(IDIAGNL_ATTR_SPORT, sport),
	__ADD(IDIAGNL_ATTR_DPORT, dport),
	__ADD(IDIAGNL_ATTR_SRC, src),
	__ADD(IDIAGNL_ATTR_DST, dst),
	__ADD(IDIAGNL_ATTR_IFINDEX, ifindex),
	__ADD(IDIAGNL_ATTR_EXPIRES, expires),
	__ADD(IDIAGNL_ATTR_RQUEUE, rqueue),
	__ADD(IDIAGNL_ATTR_WQUEUE, wqueue),
	__ADD(IDIAGNL_ATTR_UID, uid),
	__ADD(IDIAGNL_ATTR_INODE, inode),
	__ADD(IDIAGNL_ATTR_TOS, tos),
	__ADD(IDIAGNL_ATTR_TCLASS, tclass),
	__ADD(IDIAGNL_ATTR_SHUTDOWN, shutdown),
	__ADD(IDIAGNL_ATTR_CONG, cong),
	__ADD(IDIAGNL_ATTR_MEMINFO, meminfo),
	__ADD(IDIAGNL_ATTR_VEGASINFO, vegasinfo),
	__ADD(IDIAGNL_ATTR_TCPINFO, tcpinfo),
	__ADD(IDIAGNL_ATTR_SKMEMINFO, skmeminfo),
};

static char *_idiagnl_attrs2str(int attrs, char *buf, size_t len)
{
	return __flags2str(attrs, buf, len, idiagnl_attrs,
	                   ARRAY_SIZE(idiagnl_attrs));
}

static uint64_t idiagnl_compare(struct nl_object *_a, struct nl_object *_b,
                                uint64_t attrs, int flags)
{
	struct idiagnl_msg *a = (struct idiagnl_msg *) _a;
	struct idiagnl_msg *b = (struct idiagnl_msg *) _b;
	uint64_t diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(IDIAGNL_ATTR_FAMILY, a->idiag_family != b->idiag_family);
	diff |= _DIFF(IDIAGNL_ATTR_STATE, a->idiag_state != b->idiag_state);
	diff |= _DIFF(IDIAGNL_ATTR_TIMER, a->idiag_timer != b->idiag_timer);
	diff |= _DIFF(IDIAGNL_ATTR_RETRANS,
		      a->idiag_retrans != b->idiag_retrans);
	diff |= _DIFF(IDIAGNL_ATTR_SPORT, a->idiag_sport != b->idiag_sport);
	diff |= _DIFF(IDIAGNL_ATTR_DPORT, a->idiag_dport != b->idiag_dport);
	diff |= _DIFF(IDIAGNL_ATTR_SRC,
		      nl_addr_cmp(a->idiag_src, b->idiag_src));
	diff |= _DIFF(IDIAGNL_ATTR_DST,
		      nl_addr_cmp(a->idiag_dst, b->idiag_dst));
	diff |= _DIFF(IDIAGNL_ATTR_IFINDEX,
		      a->idiag_ifindex != b->idiag_ifindex);
	diff |= _DIFF(IDIAGNL_ATTR_EXPIRES,
		      a->idiag_expires != b->idiag_expires);
	diff |= _DIFF(IDIAGNL_ATTR_RQUEUE, a->idiag_rqueue != b->idiag_rqueue);
	diff |= _DIFF(IDIAGNL_ATTR_WQUEUE, a->idiag_wqueue != b->idiag_wqueue);
	diff |= _DIFF(IDIAGNL_ATTR_UID, a->idiag_uid != b->idiag_uid);
	diff |= _DIFF(IDIAGNL_ATTR_INODE, a->idiag_inode != b->idiag_inode);
	diff |= _DIFF(IDIAGNL_ATTR_TOS, a->idiag_tos != b->idiag_tos);
	diff |= _DIFF(IDIAGNL_ATTR_TCLASS, a->idiag_tclass != b->idiag_tclass);
	diff |= _DIFF(IDIAGNL_ATTR_SHUTDOWN,
		      a->idiag_shutdown != b->idiag_shutdown);
	diff |= _DIFF(IDIAGNL_ATTR_CONG, strcmp(a->idiag_cong, b->idiag_cong));
	diff |= _DIFF(IDIAGNL_ATTR_MEMINFO,
		      nl_object_diff((struct nl_object *)a->idiag_meminfo,
				     (struct nl_object *)b->idiag_meminfo));
	diff |= _DIFF(IDIAGNL_ATTR_VEGASINFO,
		      nl_object_diff((struct nl_object *)a->idiag_vegasinfo,
				     (struct nl_object *)b->idiag_vegasinfo));
	diff |= _DIFF(IDIAGNL_ATTR_TCPINFO,
		      memcmp(&a->idiag_tcpinfo, &b->idiag_tcpinfo,
			     sizeof(a->idiag_tcpinfo)));
	diff |= _DIFF(IDIAGNL_ATTR_SKMEMINFO,
		      memcmp(a->idiag_skmeminfo, b->idiag_skmeminfo,
			     sizeof(a->idiag_skmeminfo)));
#undef _DIFF

	return diff;
}

static void idiagnl_keygen(struct nl_object *obj, uint32_t *hashkey,
        uint32_t table_sz)
{
	struct idiagnl_msg *msg = (struct idiagnl_msg *)obj;
	unsigned int key_sz;
	struct idiagnl_hash_key {
		uint8_t	family;
		uint32_t src_hash;
		uint32_t dst_hash;
		uint16_t sport;
		uint16_t dport;
	} _nl_packed key;

	key_sz = sizeof(key);
	key.family = msg->idiag_family;
	key.src_hash = 0;
	key.dst_hash = 0;
	key.sport = msg->idiag_sport;
	key.dport = msg->idiag_dport;

	if (msg->idiag_src) {
		key.src_hash = nl_hash (nl_addr_get_binary_addr(msg->idiag_src),
		                        nl_addr_get_len(msg->idiag_src), 0);
	}
	if (msg->idiag_dst) {
		key.dst_hash = nl_hash (nl_addr_get_binary_addr(msg->idiag_dst),
		                        nl_addr_get_len(msg->idiag_dst), 0);
	}

	*hashkey = nl_hash(&key, key_sz, 0) % table_sz;

	NL_DBG(5, "idiagnl %p key (fam %d src_hash %d dst_hash %d sport %d dport %d) keysz %d, hash 0x%x\n",
	       msg, key.family, key.src_hash, key.dst_hash, key.sport, key.dport, key_sz, *hashkey);

	return;
}

/** @cond SKIP */
struct nl_object_ops idiagnl_msg_obj_ops = {
	.oo_name			 = "idiag/idiag_msg",
	.oo_size			 = sizeof(struct idiagnl_msg),
	.oo_free_data			 = idiagnl_msg_free,
	.oo_clone			 = idiagnl_msg_clone,
	.oo_dump			 = {
		[NL_DUMP_LINE]		 = idiag_msg_dump_line,
		[NL_DUMP_DETAILS]	 = idiag_msg_dump_details,
		[NL_DUMP_STATS]		 = idiag_msg_dump_stats,
	},
	.oo_compare			= idiagnl_compare,
	.oo_keygen			= idiagnl_keygen,
	.oo_attrs2str			= _idiagnl_attrs2str,
	.oo_id_attrs                    = (IDIAGNL_ATTR_FAMILY |
	                                   IDIAGNL_ATTR_SRC |
	                                   IDIAGNL_ATTR_DST |
	                                   IDIAGNL_ATTR_SPORT |
	                                   IDIAGNL_ATTR_DPORT),
};
/** @endcond */

/** @} */
