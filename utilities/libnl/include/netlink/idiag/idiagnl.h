/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2013 Sassano Systems LLC <joe@sassanosystems.com>
 */

#ifndef NETLINK_IDIAGNL_H_
#define NETLINK_IDIAGNL_H_

#include <netlink/netlink.h>
#include <linux/sock_diag.h>

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
 * The following part contains DEPRECATED names and defines.
 * Don't use them.
 *************************************************************/

/**
 * Inet Diag message types
 *
 * deprecated: use TCPDIAG_GETSOCK, DCCPDIAG_GETSOCK and
 * INET_DIAG_GETSOCK_MAX from linux/inet_diag.h
 */
#define IDIAG_TCPDIAG_GETSOCK	18
#define IDIAG_DCCPDIAG_GETSOCK	19
#define IDIAG_GETSOCK_MAX	24

/**
 * Socket state identifiers
 * @ingroup idiag
 * @deprecated: use instead the TCP_* defines from netinet/tcp.h.
 */
enum {
	IDIAG_SS_UNKNOWN           = 0,

	IDIAG_SS_ESTABLISHED       = 1,  /* TCP_ESTABLISHED */
	IDIAG_SS_SYN_SENT          = 2,  /* TCP_SYN_SENT */
	IDIAG_SS_SYN_RECV          = 3,  /* TCP_SYN_RECV */
	IDIAG_SS_FIN_WAIT1         = 4,  /* TCP_FIN_WAIT1 */
	IDIAG_SS_FIN_WAIT2         = 5,  /* TCP_FIN_WAIT2 */
	IDIAG_SS_TIME_WAIT         = 6,  /* TCP_TIME_WAIT */
	IDIAG_SS_CLOSE             = 7,  /* TCP_CLOSE */
	IDIAG_SS_CLOSE_WAIT        = 8,  /* TCP_CLOSE_WAIT */
	IDIAG_SS_LAST_ACK          = 9,  /* TCP_LAST_ACK */
	IDIAG_SS_LISTEN            = 10, /* TCP_LISTEN */
	IDIAG_SS_CLOSING           = 11, /* TCP_CLOSING */

	IDIAG_SS_MAX               = 12,
};

/**
 * Macro to represent all socket states.
 * @ingroup idiag
 * @deprecated
 */
#define IDIAG_SS_ALL  IDIAGNL_SS_ALL


/**
 * Inet Diag extended attributes
 * @ingroup idiag
 * @deprecated These attributes should not be used. They mirror the
 * INET_DIAG_* extension flags from kernel headers. Use those instead. */
enum {
	IDIAG_ATTR_NONE         = 0, /* INET_DIAG_NONE */
	IDIAG_ATTR_MEMINFO      = 1, /* INET_DIAG_MEMINFO */
	IDIAG_ATTR_INFO         = 2, /* INET_DIAG_INFO */
	IDIAG_ATTR_VEGASINFO    = 3, /* INET_DIAG_VEGASINFO */
	IDIAG_ATTR_CONG         = 4, /* INET_DIAG_CONG */
	IDIAG_ATTR_TOS          = 5, /* INET_DIAG_TOS */
	IDIAG_ATTR_TCLASS       = 6, /* INET_DIAG_TCLASS */
	IDIAG_ATTR_SKMEMINFO    = 7, /* INET_DIAG_SKMEMINFO */
	IDIAG_ATTR_SHUTDOWN     = 8, /* INET_DIAG_SHUTDOWN */

	/* IDIAG_ATTR_MAX was wrong, because it did not correspond to
	 * INET_DIAG_MAX. Anyway, freeze it to the previous value. */
	IDIAG_ATTR_MAX          = 9,

	IDIAG_ATTR_ALL          = (1<<IDIAG_ATTR_MAX) - 1,
};


/* deprectated keep these only for compatibility, DO NOT USE THEM */
#define IDIAG_SK_MEMINFO_RMEM_ALLOC                      0  /* SK_MEMINFO_RMEM_ALLOC */
#define IDIAG_SK_MEMINFO_RCVBUF                          1  /* SK_MEMINFO_RCVBUF */
#define IDIAG_SK_MEMINFO_WMEM_ALLOC                      2  /* SK_MEMINFO_WMEM_ALLOC */
#define IDIAG_SK_MEMINFO_SNDBUF                          3  /* SK_MEMINFO_SNDBUF */
#define IDIAG_SK_MEMINFO_FWD_ALLOC                       4  /* SK_MEMINFO_FWD_ALLOC */
#define IDIAG_SK_MEMINFO_WMEM_QUEUED                     5  /* SK_MEMINFO_WMEM_QUEUED */
#define IDIAG_SK_MEMINFO_OPTMEM                          6  /* SK_MEMINFO_OPTMEM */
#define IDIAG_SK_MEMINFO_BACKLOG                         7  /* SK_MEMINFO_BACKLOG */
#define IDIAG_SK_MEMINFO_VARS              SK_MEMINFO_VARS

/* deprecated names. */
#define IDIAG_TIMER_OFF                 IDIAGNL_TIMER_OFF
#define IDIAG_TIMER_ON                  IDIAGNL_TIMER_ON
#define IDIAG_TIMER_KEEPALIVE           IDIAGNL_TIMER_KEEPALIVE
#define IDIAG_TIMER_TIMEWAIT            IDIAGNL_TIMER_TIMEWAIT
#define IDIAG_TIMER_PERSIST             IDIAGNL_TIMER_PERSIST
#define IDIAG_TIMER_UNKNOWN             IDIAGNL_TIMER_UNKNOWN

/*************************************************************/

/**
 * Macro to represent all socket states.
 * @ingroup idiag
 */
#define IDIAGNL_SS_ALL (((1<<12)-1))

/**
 * Socket timer indentifiers
 * @ingroupd idiag
 */
enum {
	IDIAGNL_TIMER_OFF               = 0,
	IDIAGNL_TIMER_ON                = 1,
	IDIAGNL_TIMER_KEEPALIVE         = 2,
	IDIAGNL_TIMER_TIMEWAIT          = 3,
	IDIAGNL_TIMER_PERSIST           = 4,
	IDIAGNL_TIMER_UNKNOWN           = 5,
};

extern char *	idiagnl_state2str(int, char *, size_t);
extern int	idiagnl_str2state(const char *);

extern int	idiagnl_connect(struct nl_sock *);
extern int	idiagnl_send_simple(struct nl_sock *, int, uint8_t, uint16_t,
                                    uint16_t);

extern char *		idiagnl_timer2str(int, char *, size_t);
extern int		idiagnl_str2timer(const char *);
extern char *		idiagnl_attrs2str(int, char *, size_t);
extern char *		idiagnl_tcpstate2str(uint8_t, char *, size_t);
extern char *		idiagnl_tcpopts2str(uint8_t, char *, size_t);
extern char *		idiagnl_shutdown2str(uint8_t, char *, size_t);
extern char *		idiagnl_exts2str(uint8_t, char *, size_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NETLINK_IDIAGNL_H_ */
