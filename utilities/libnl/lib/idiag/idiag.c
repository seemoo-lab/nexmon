/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

/**
 * @defgroup  idiag Inet Diag library (libnl-idiag)
 * @brief
 * @{
 */

#include "nl-default.h"

#include <linux/inet_diag.h>

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/idiag/idiagnl.h>

#include "nl-priv-dynamic-core/nl-core.h"

/**
 * @name Socket Creation
 * @{
 */

/**
 * Create and connect idiag netlink socket.
 * @arg sk    Netlink socket.
 *
 * Creates a NETLINK_INET_DIAG socket, binds the socket, and issues a connection
 * attemp.
 *
 * @see nl_connect()
 *
 * @return 0 on success or a negative error code.
 */
int idiagnl_connect(struct nl_sock *sk)
{
	return nl_connect(sk, NETLINK_INET_DIAG);
}

/** @} */

/**
 * @name Sending
 * @{
 */

/**
 * Send trivial idiag netlink message
 * @arg sk	Netlink socket.
 * @arg flags	Message flags
 * @arg family	Address family
 * @arg states	Socket states to query
 * @arg ext	Inet Diag attribute extensions to query. Note that this only supports
 *   8 bit arguments. Flags outside uint8_t range are silently ignored.
 *
 * @return 0 on success or a negative error code. Due to a bug, this function
 * returns the number of bytes sent. Treat any non-negative number as success.
 */
int idiagnl_send_simple(struct nl_sock *sk, int flags, uint8_t family,
		uint16_t states, uint16_t ext)
{
	struct inet_diag_req req;
	memset(&req, 0, sizeof(req));

	flags |= NLM_F_ROOT;

	req.idiag_family = family;
	req.idiag_states = states;
	req.idiag_ext = ext;

	return nl_send_simple(sk, TCPDIAG_GETSOCK, flags, &req, sizeof(req));
}

/** @} */

/**
 * @name Inet Diag flag and attribute conversions
 * @{
 */

static const struct trans_tbl idiag_states[] = {
	__ADD(TCP_ESTABLISHED, established),
	__ADD(TCP_SYN_SENT, syn_sent),
	__ADD(TCP_SYN_RECV, syn_recv),
	__ADD(TCP_FIN_WAIT1, fin_wait),
	__ADD(TCP_FIN_WAIT2, fin_wait2),
	__ADD(TCP_TIME_WAIT, time_wait),
	__ADD(TCP_CLOSE, close),
	__ADD(TCP_CLOSE_WAIT, close_wait),
	__ADD(TCP_LAST_ACK, last_ack),
	__ADD(TCP_LISTEN, listen),
	__ADD(TCP_CLOSING, closing),
};

/**
 * Convert inet diag socket states to strings.
 * @arg state	  inetdiag socket state (e.g., TCP_ESTABLISHED)
 * @arg buf	  output buffer which will hold string result
 * @arg len	  length in bytes of the output buffer
 *
 * @return string representation of the inetdiag socket state or an empty
 * string.
 */
char * idiagnl_state2str(int state, char *buf, size_t len)
{
	return __type2str(state, buf, len, idiag_states,
			ARRAY_SIZE(idiag_states));
}

/**
 * Convert inet diag socket state string to int.
 * @arg name	inetdiag socket state string
 *
 * @return the int representation of the socket state strign or a negative error
 * code.
 */
int idiagnl_str2state(const char *name)
{
	return __str2type(name, idiag_states, ARRAY_SIZE(idiag_states));
}

static const struct trans_tbl idiag_timers[] = {
	__ADD(IDIAGNL_TIMER_OFF, off),
	__ADD(IDIAGNL_TIMER_ON, on),
	__ADD(IDIAGNL_TIMER_KEEPALIVE, keepalive),
	__ADD(IDIAGNL_TIMER_TIMEWAIT, timewait),
	__ADD(IDIAGNL_TIMER_PERSIST, persist),
	__ADD(IDIAGNL_TIMER_UNKNOWN, unknown),
};

/**
 * Convert inet diag timer types to strings.
 * @arg timer	  inetdiag timer (e.g., IDIAGNL_TIMER_ON)
 * @arg buf	  output buffer which will hold string result
 * @arg len	  length in bytes of the output buffer
 *
 * @return string representation of the inetdiag timer type or an empty string.
 */
char * idiagnl_timer2str(int timer, char *buf, size_t len)
{
	return __type2str(timer, buf, len, idiag_timers,
	    ARRAY_SIZE(idiag_timers));
}

/**
 * Convert inet diag timer string to int.
 * @arg name	inetdiag timer string
 *
 * @return the int representation of the timer string or a negative error code.
 */
int idiagnl_str2timer(const char *name)
{
	return __str2type(name, idiag_timers, ARRAY_SIZE(idiag_timers));
}

static const struct trans_tbl idiag_attrs[] = {
	__ADD(INET_DIAG_NONE, none),
	__ADD(INET_DIAG_MEMINFO, meminfo),
	__ADD(INET_DIAG_INFO, info),
	__ADD(INET_DIAG_VEGASINFO, vegasinfo),
	__ADD(INET_DIAG_CONG, congestion),
	__ADD(INET_DIAG_TOS, tos),
	__ADD(INET_DIAG_TCLASS, tclass),
	__ADD(INET_DIAG_SKMEMINFO, skmeminfo),
	__ADD(INET_DIAG_SHUTDOWN, shutdown),
};

/**
 * Convert inet diag extension type to a string.
 * @arg attrs	  inet diag extension type (e.g. INET_DIAG_MEMINFO)
 * @arg buf	  output buffer which will hold string result
 * @arg len	  length in bytes of the output buffer
 *
 * @return string representation of inet diag extension type or an empty string.
 * @deprecated: don't use this function. It is not very useful and should
 * never have been exposed as public API.
 */
char *idiagnl_attrs2str(int attrs, char *buf, size_t len)
{
	return __type2str(attrs, buf, len, idiag_attrs, ARRAY_SIZE(idiag_attrs));
}

static const struct trans_tbl idiag_exts[] = {
	__ADD((1 << (INET_DIAG_MEMINFO - 1)), meminfo),
	__ADD((1 << (INET_DIAG_INFO - 1)), info),
	__ADD((1 << (INET_DIAG_VEGASINFO - 1)), vegasinfo),
	__ADD((1 << (INET_DIAG_CONG - 1)), congestion),
	__ADD((1 << (INET_DIAG_TOS - 1)), tos),
	__ADD((1 << (INET_DIAG_TCLASS - 1)), tclass),
	__ADD((1 << (INET_DIAG_SKMEMINFO - 1)), skmeminfo),
	__ADD((1 << (INET_DIAG_SHUTDOWN - 1)), shutdown),
};

/**
 * Convert inet diag extension flags to a string.
 * @arg attrs	inet diag extension flags (e.g.
 *   ( (1<<(INET_DIAG_MEMINFO-1)) | (1<<(INET_DIAG_CONG-1)) | (1<<(INET_DIAG_TOS-1)) ) )
 * @arg buf	Output buffer to hold string representation
 * @arg len	length in bytes of the output buffer
 */
char *idiagnl_exts2str(uint8_t attrs, char *buf, size_t len)
{
	return __flags2str(attrs, buf, len, idiag_exts, ARRAY_SIZE(idiag_exts));
}

static const struct trans_tbl idiagnl_tcpstates[] = {
	__ADD(TCP_CA_Open, open),
	__ADD(TCP_CA_Disorder, disorder),
	__ADD(TCP_CA_CWR, cwr),
	__ADD(TCP_CA_Recovery, recovery),
	__ADD(TCP_CA_Loss, loss),
};

/**
 * Convert inetdiag tcp states to strings.
 * @arg state	TCP state (e.g., TCP_CA_Open)
 * @arg buf	output buffer which will hold string result
 * @arg len	length in bytes of the output buffer
 */
char *idiagnl_tcpstate2str(uint8_t state, char *buf, size_t len)
{
	return __type2str(state, buf, len, idiagnl_tcpstates,
			ARRAY_SIZE(idiagnl_tcpstates));
}

static const struct trans_tbl idiagnl_tcpopt_attrs[] = {
	__ADD(TCPI_OPT_TIMESTAMPS, timestamps),
	__ADD(TCPI_OPT_SACK, sACK),
	__ADD(TCPI_OPT_WSCALE, wscale),
	__ADD(TCPI_OPT_ECN, ecn),
};

/**
 * Convert TCP option attributes to string
 * @arg attrs	  TCP option attributes to convert (e.g., TCPI_OPT_SACK |
 *  TCPI_OPT_WSCALE)
 * @arg	buf	  Output buffer for string
 * @arg len	  Length in bytes of output buffer
 *
 * @return buffer with string representation or empty string
 */
char *idiagnl_tcpopts2str(uint8_t attrs, char *buf, size_t len)
{
	return __flags2str(attrs, buf, len, idiagnl_tcpopt_attrs,
			ARRAY_SIZE(idiagnl_tcpopt_attrs));
}

/**
 * Convert shutdown state to string.
 * @arg shutdown    Shutdown state (e.g., idiag_msg->shutdown)
 * @arg buf	    Ouput buffer to hold string representation
 * @arg len	    Length in bytes of output buffer
 *
 * @return string representation of shutdown state or NULL
 */
char * idiagnl_shutdown2str(uint8_t shutdown, char *buf, size_t len)
{
	if (shutdown == 0) {
		snprintf(buf, len, " ");
		return buf;
	} else if (shutdown == 1) {
		snprintf(buf, len, "receive shutdown");
		return buf;
	} else if (shutdown == 2) {
		snprintf(buf, len, "send shutdown");
		return buf;
	}

	return NULL;
}

/** @} */
/** @} */
