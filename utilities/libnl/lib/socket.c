/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2012 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup core_types
 * @defgroup socket Socket
 *
 * Representation of a netlink socket
 *
 * Related sections in the development guide:
 * - @core_doc{core_sockets, Netlink Sockets}
 *
 * @{
 *
 * Header
 * ------
 * ~~~~{.c}
 * #include <netlink/socket.h>
 * ~~~~
 */

#include "nl-default.h"

#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/socket.h>

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl-core.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-aux-core/nl-core.h"

#ifndef SOL_NETLINK
#define SOL_NETLINK 270
#endif

static int default_cb = NL_CB_DEFAULT;

static void _nl_init init_default_cb(void)
{
	char *nlcb;

	if ((nlcb = getenv("NLCB"))) {
		if (!strcasecmp(nlcb, "default"))
			default_cb = NL_CB_DEFAULT;
		else if (!strcasecmp(nlcb, "verbose"))
			default_cb = NL_CB_VERBOSE;
		else if (!strcasecmp(nlcb, "debug"))
			default_cb = NL_CB_DEBUG;
		else {
			fprintf(stderr, "Unknown value for NLCB, valid values: "
				"{default | verbose | debug}\n");
		}
	}
}

static uint32_t _badrandom_from_time(void)
{
	uint32_t result;
	uint64_t v64;
	time_t t;

	t = time(NULL);
	v64 = (uint64_t)t;
	result = (uint32_t)v64;

	/* XOR with the upper bits. Otherwise, coverity warns about only
	 * considering 32 bit from time_t.  Use the inverse, so that for the
	 * most part the bits don't change.  */
	result ^= (~(v64 >> 32));

	return result;
}

static uint32_t used_ports_map[32];
static NL_RW_LOCK(port_map_lock);

static uint32_t generate_local_port(void)
{
	int i, j, m;
	uint32_t n;
	static uint16_t idx_state = 0;
	uint32_t pid = getpid() & 0x3FFFFF;

	nl_write_lock(&port_map_lock);

	if (idx_state == 0) {
		uint32_t t = _badrandom_from_time();

		/* from time to time (on average each 2^15 calls), the idx_state will
		 * be zero again. No problem, just "seed" anew with time(). */
		idx_state = t ^ (t >> 16) ^ 0x3047;
	} else
		idx_state = idx_state + 20011; /* add prime number */

	i = idx_state >> 5;
	n = idx_state;
	for (j = 0; j < 32; j++) {
		/* walk the index somewhat randomized, with always leaving the block
		 * #0 as last. The reason is that libnl-1 will start at block #0,
		 * so just leave the first 32 ports preferably for libnl-1 owned sockets
		 * (this is relevant only if the applications ends up using both versions
		 * of the library and doesn't hurt otherwise). */
		if (j == 31)
			i = 0;
		else
			i = (((i-1) + 7) % 31) + 1;

		if (used_ports_map[i] == 0xFFFFFFFF)
			continue;

		for (m = 0; m < 32; m++) {
			n = (n + 13u) % 32u;
			if (1UL & (used_ports_map[i] >> n))
				continue;

			used_ports_map[i] |= (1UL << n);
			n += (i * 32);

			/* PID_MAX_LIMIT is currently at 2^22, leaving 10 bit
			 * to, i.e. 1024 unique ports per application. */

			nl_write_unlock(&port_map_lock);

			/* ensure we don't return zero. */
			pid = pid + (n << 22);
			return pid ? pid : 1024;
		}
	}

	nl_write_unlock(&port_map_lock);
	return 0;
}

static void release_local_port(uint32_t port)
{
	int nr;
	uint32_t mask;

	BUG_ON(port == 0);

	nr = port >> 22;
	mask = 1UL << (nr % 32);
	nr /= 32;

	nl_write_lock(&port_map_lock);
	BUG_ON((used_ports_map[nr] & mask) != mask);
	used_ports_map[nr] &= ~mask;
	nl_write_unlock(&port_map_lock);
}

/** \cond skip */
void _nl_socket_used_ports_release_all(const uint32_t *used_ports)
{
	int i;

	for (i = 0; i < 32; i++) {
		if (used_ports[i] != 0) {
			nl_write_lock(&port_map_lock);
			for (; i < 32; i++) {
				BUG_ON((used_ports_map[i] & used_ports[i]) != used_ports[i]);
				used_ports_map[i] &= ~(used_ports[i]);
			}
			nl_write_unlock(&port_map_lock);
			return;
		}
	}
}

void _nl_socket_used_ports_set(uint32_t *used_ports, uint32_t port)
{
	int nr;
	int32_t mask;

	nr = port >> 22;
	mask = 1UL << (nr % 32);
	nr /= 32;

	/*
	BUG_ON(port == 0 || (getpid() & 0x3FFFFF) != (port & 0x3FFFFF));
	BUG_ON(used_ports[nr] & mask);
	*/

	used_ports[nr] |= mask;
}
/** \endcond */

/**
 * @name Allocation
 * @{
 */

static struct nl_sock *__alloc_socket(struct nl_cb *cb)
{
	struct nl_sock *sk;

	sk = calloc(1, sizeof(*sk));
	if (!sk)
		return NULL;

	sk->s_fd = -1;
	sk->s_cb = nl_cb_get(cb);
	sk->s_local.nl_family = AF_NETLINK;
	sk->s_peer.nl_family = AF_NETLINK;
	sk->s_seq_next = _badrandom_from_time();
	sk->s_seq_expect = sk->s_seq_next;

	/* the port is 0 (unspecified), meaning NL_OWN_PORT */
	sk->s_flags = NL_OWN_PORT;

	return sk;
}

/**
 * Allocate new netlink socket
 *
 * @return Newly allocated netlink socket or NULL.
 */
struct nl_sock *nl_socket_alloc(void)
{
	struct nl_cb *cb;
	struct nl_sock *sk;

	cb = nl_cb_alloc(default_cb);
	if (!cb)
		return NULL;

	/* will increment cb reference count on success */
	sk = __alloc_socket(cb);

	nl_cb_put(cb);

	return sk;
}

/**
 * Allocate new socket with custom callbacks
 * @arg cb		Callback handler
 *
 * The reference to the callback handler is taken into account
 * automatically, it is released again upon calling nl_socket_free().
 *
 *@return Newly allocted socket handle or NULL.
 */
struct nl_sock *nl_socket_alloc_cb(struct nl_cb *cb)
{
	if (cb == NULL)
		BUG();

	return __alloc_socket(cb);
}

/**
 * Free a netlink socket.
 * @arg sk		Netlink socket.
 */
void nl_socket_free(struct nl_sock *sk)
{
	if (!sk)
		return;

	if (sk->s_fd >= 0)
		close(sk->s_fd);

	if (!(sk->s_flags & NL_OWN_PORT))
		release_local_port(sk->s_local.nl_pid);

	nl_cb_put(sk->s_cb);
	free(sk);
}

/** @} */

/**
 * @name Sequence Numbers
 * @{
 */

static int noop_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}


/**
 * Disable sequence number checking.
 * @arg sk		Netlink socket.
 *
 * Disables checking of sequence numbers on the netlink socket This is
 * required to allow messages to be processed which were not requested by
 * a preceding request message, e.g. netlink events.
 *
 * @note This function modifies the NL_CB_SEQ_CHECK configuration in
 * the callback handle associated with the socket.
 */
void nl_socket_disable_seq_check(struct nl_sock *sk)
{
	nl_cb_set(sk->s_cb, NL_CB_SEQ_CHECK,
		  NL_CB_CUSTOM, noop_seq_check, NULL);
}

/**
 * Use next sequence number
 * @arg sk		Netlink socket.
 *
 * Uses the next available sequence number and increases the counter
 * by one for subsequent calls.
 *
 * @return Unique serial sequence number
 */
unsigned int nl_socket_use_seq(struct nl_sock *sk)
{
	if (sk->s_seq_next == UINT_MAX) {
		sk->s_seq_next = 0;
		return UINT_MAX;
	}
	return sk->s_seq_next++;
}

/**
 * Disable automatic request for ACK
 * @arg sk		Netlink socket.
 *
 * The default behaviour of a socket is to request an ACK for
 * each message sent to allow for the caller to synchronize to
 * the completion of the netlink operation. This function
 * disables this behaviour and will result in requests being
 * sent which will not have the NLM_F_ACK flag set automatically.
 * However, it is still possible for the caller to set the
 * NLM_F_ACK flag explicitely.
 */
void nl_socket_disable_auto_ack(struct nl_sock *sk)
{
	sk->s_flags |= NL_NO_AUTO_ACK;
}

/**
 * Enable automatic request for ACK (default)
 * @arg sk		Netlink socket.
 * @see nl_socket_disable_auto_ack
 */
void nl_socket_enable_auto_ack(struct nl_sock *sk)
{
	sk->s_flags &= ~NL_NO_AUTO_ACK;
}

/** @} */

/** \cond skip */
int _nl_socket_is_local_port_unspecified(struct nl_sock *sk)
{
	return (sk->s_local.nl_pid == 0);
}

uint32_t _nl_socket_set_local_port_no_release(struct nl_sock *sk, int generate_other)
{
	uint32_t port;

	/* reset the port to generate_local_port(), but do not release
	 * the previously generated port. */

	if (generate_other)
		port = generate_local_port();
	else
		port = 0;
	sk->s_local.nl_pid = port;
	if (port == 0) {
		/* failed to find an unsed port. Restore the socket to have an
		 * unspecified port. */
		sk->s_flags |= NL_OWN_PORT;
	} else
		sk->s_flags &= ~NL_OWN_PORT;
	return port;
}
/** \endcond */

/**
 * @name Source Idenficiation
 * @{
 */

uint32_t nl_socket_get_local_port(const struct nl_sock *sk)
{
	if (sk->s_local.nl_pid == 0) {
		struct nl_sock *sk_mutable = (struct nl_sock *) sk;

		/* modify the const argument sk. This is justified, because
		 * nobody ever saw the local_port from externally. So, we
		 * initilize it on first use.
		 *
		 * Note that this also means that you cannot call this function
		 * from multiple threads without synchronization. But nl_sock
		 * is not automatically threadsafe anyway, so the user is not
		 * allowed to do that.
		 */
		sk_mutable->s_local.nl_pid = generate_local_port();
		if (sk_mutable->s_local.nl_pid == 0) {
			/* could not generate a local port. Assign UINT32_MAX to preserve
			 * backward compatibility. A user who cares can clear that anyway
			 * with nl_socket_set_local_port(). */
			sk_mutable->s_local.nl_pid = UINT32_MAX;
			sk_mutable->s_flags |= NL_OWN_PORT;
		} else
			sk_mutable->s_flags &= ~NL_OWN_PORT;
	}
	return sk->s_local.nl_pid;
}

/**
 * Set local port of socket
 * @arg sk		Netlink socket.
 * @arg port		Local port identifier
 *
 * Assigns a local port identifier to the socket.
 *
 * If port is 0, the port is reset to 'unspecified' as it is after newly
 * calling nl_socket_alloc().
 * Unspecified means, that the port will be generated automatically later
 * on first use (either on nl_socket_get_local_port() or nl_connect()).
 */
void nl_socket_set_local_port(struct nl_sock *sk, uint32_t port)
{
	if (!(sk->s_flags & NL_OWN_PORT))
		release_local_port(sk->s_local.nl_pid);
	sk->s_flags |= NL_OWN_PORT;
	sk->s_local.nl_pid = port;
}

/** @} */

/**
 * @name Group Subscriptions
 * @{
 */

/**
 * Join groups
 * @arg sk		Netlink socket
 * @arg group		Group identifier
 *
 * Joins the specified groups using the modern socket option which
 * is available since kernel version 2.6.14. It allows joining an
 * almost arbitary number of groups without limitation.  The list
 * of groups has to be terminated by 0 (%NFNLGRP_NONE).
 *
 * Make sure to use the correct group definitions as the older
 * bitmask definitions for nl_join_groups() are likely to still
 * be present for backward compatibility reasons.
 *
 * @return 0 on success or a negative error code.
 */
int nl_socket_add_memberships(struct nl_sock *sk, int group, ...)
{
	int err;
	va_list ap;

	if (sk->s_fd == -1)
		return -NLE_BAD_SOCK;

	va_start(ap, group);

	while (group != 0) {
		if (group < 0) {
			va_end(ap);
			return -NLE_INVAL;
		}

		err = setsockopt(sk->s_fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP,
						 &group, sizeof(group));
		if (err < 0) {
			va_end(ap);
			NL_DBG(4, "nl_socket_add_memberships(%p): setsockopt() failed with %d (%s)\n",
				sk, errno, nl_strerror_l(errno));
			return -nl_syserr2nlerr(errno);
		}

		group = va_arg(ap, int);
	}

	va_end(ap);

	return 0;
}

int nl_socket_add_membership(struct nl_sock *sk, int group)
{
	return nl_socket_add_memberships(sk, group, 0);
}

/**
 * Leave groups
 * @arg sk		Netlink socket
 * @arg group		Group identifier
 *
 * Leaves the specified groups using the modern socket option
 * which is available since kernel version 2.6.14. The list of groups
 * has to terminated by 0 (%NFNLGRP_NONE).
 *
 * @see nl_socket_add_membership
 * @return 0 on success or a negative error code.
 */
int nl_socket_drop_memberships(struct nl_sock *sk, int group, ...)
{
	int err;
	va_list ap;

	if (sk->s_fd == -1)
		return -NLE_BAD_SOCK;

	va_start(ap, group);

	while (group != 0) {
		if (group < 0) {
			va_end(ap);
			return -NLE_INVAL;
		}

		err = setsockopt(sk->s_fd, SOL_NETLINK, NETLINK_DROP_MEMBERSHIP,
						 &group, sizeof(group));
		if (err < 0) {
			va_end(ap);
			NL_DBG(4, "nl_socket_drop_memberships(%p): setsockopt() failed with %d (%s)\n",
				sk, errno, nl_strerror_l(errno));
			return -nl_syserr2nlerr(errno);
		}

		group = va_arg(ap, int);
	}

	va_end(ap);

	return 0;
}

int nl_socket_drop_membership(struct nl_sock *sk, int group)
{
	return nl_socket_drop_memberships(sk, group, 0);
}


/**
 * Join multicast groups (deprecated)
 * @arg sk		Netlink socket.
 * @arg groups		Bitmask of groups to join.
 *
 * This function defines the old way of joining multicast group which
 * has to be done prior to calling nl_connect(). It works on any kernel
 * version but is very limited as only 32 groups can be joined.
 */
void nl_join_groups(struct nl_sock *sk, int groups)
{
	sk->s_local.nl_groups |= groups;
}


/** @} */

/**
 * @name Peer Identfication
 * @{
 */

uint32_t nl_socket_get_peer_port(const struct nl_sock *sk)
{
	return sk->s_peer.nl_pid;
}

void nl_socket_set_peer_port(struct nl_sock *sk, uint32_t port)
{
	sk->s_peer.nl_pid = port;
}

uint32_t nl_socket_get_peer_groups(const struct nl_sock *sk)
{
	return sk->s_peer.nl_groups;
}

void nl_socket_set_peer_groups(struct nl_sock *sk, uint32_t groups)
{
	sk->s_peer.nl_groups = groups;
}



/** @} */

/**
 * @name File Descriptor
 * @{
 */

/**
 * Return the file descriptor of the backing socket
 * @arg sk		Netlink socket
 *
 * Only valid after calling nl_connect() to create and bind the respective
 * socket.
 *
 * @return File descriptor or -1 if not available.
 */
int nl_socket_get_fd(const struct nl_sock *sk)
{
	return sk->s_fd;
}

/**
 * Set the socket file descriptor externally which initializes the
 * socket similar to nl_connect().
 *
 * @arg sk         Netlink socket (required)
 * @arg protocol   The socket protocol (optional). Linux 2.6.32 supports
 *                 the socket option SO_PROTOCOL. In this case, you can set
 *                 protocol to a negative value and let it autodetect.
 *                 If you set it to a non-negative value, the detected protocol
 *                 must match the one provided.
 *                 To support older kernels, you must specify the protocol.
 * @arg fd         Socket file descriptor to use (required)
 *
 * Set the socket file descriptor. @fd must be valid and bind'ed.
 *
 * This is an alternative to nl_connect(). nl_connect() creates, binds and
 * sets the socket. With this function you can set the socket to an externally
 * created file descriptor.
 *
 * @see nl_connect()
 *
 * @return 0 on success or a negative error code. On error, @fd is not closed but
 * possibly unusable.
 *
 * @retval -NLE_BAD_SOCK Netlink socket is already connected
 * @retval -NLE_INVAL Socket is of unexpected type
 */
int nl_socket_set_fd(struct nl_sock *sk, int protocol, int fd)
{
	int err = 0;
	socklen_t addrlen;
	struct sockaddr_nl local = { 0 };
	int so_type = -1, so_protocol = -1;

	if (sk->s_fd != -1)
		return -NLE_BAD_SOCK;
	if (fd < 0)
		return -NLE_INVAL;

	addrlen = sizeof(local);
	err = getsockname(fd, (struct sockaddr *) &local,
	                  &addrlen);
	if (err < 0) {
		NL_DBG(4, "nl_socket_set_fd(%p,%d): getsockname() failed with %d (%s)\n",
		       sk, fd, errno, nl_strerror_l(errno));
		return -nl_syserr2nlerr(errno);
	}
	if (addrlen != sizeof(local))
		return -NLE_INVAL;
	if (local.nl_family != AF_NETLINK) {
		NL_DBG(4, "nl_socket_set_fd(%p,%d): getsockname() returned family %d instead of %d (AF_NETLINK)\n",
		       sk, fd, local.nl_family, AF_NETLINK);
		return -NLE_INVAL;
	}

	addrlen = sizeof(so_type);
	err = getsockopt(fd, SOL_SOCKET, SO_TYPE, &so_type, &addrlen);
	if (err < 0) {
		NL_DBG(4, "nl_socket_set_fd(%p,%d): getsockopt() for SO_TYPE failed with %d (%s)\n",
		       sk, fd, errno, nl_strerror_l(errno));
		return -nl_syserr2nlerr(errno);
	}
	if (addrlen != sizeof(so_type))
		return -NLE_INVAL;
	if (so_type != SOCK_RAW) {
		NL_DBG(4, "nl_socket_set_fd(%p,%d): getsockopt() returned SO_TYPE %d instead of %d (SOCK_RAW)\n",
		       sk, fd, so_type, SOCK_RAW);
		return -NLE_INVAL;
	}

#if SO_PROTOCOL
	addrlen = sizeof(so_protocol);
	err = getsockopt(fd, SOL_SOCKET, SO_PROTOCOL, &so_protocol, &addrlen);
	if (err < 0) {
		if (errno == ENOPROTOOPT)
			goto no_so_protocol;
		NL_DBG(4, "nl_socket_set_fd(%p,%d): getsockopt() for SO_PROTOCOL failed with %d (%s)\n",
		       sk, fd, errno, nl_strerror_l(errno));
		return -nl_syserr2nlerr(errno);
	}
	if (addrlen != sizeof(so_protocol))
		return -NLE_INVAL;
	if (protocol >= 0 && protocol != so_protocol) {
		NL_DBG(4, "nl_socket_set_fd(%p,%d): getsockopt() for SO_PROTOCOL returned %d instead of %d\n",
		       sk, fd, so_protocol, protocol);
		return -NLE_INVAL;
	}

	if (0)
#endif
	{
no_so_protocol:
		if (protocol < 0) {
			NL_DBG(4, "nl_socket_set_fd(%p,%d): unknown protocol and unable to detect it via SO_PROTOCOL socket option\n",
			       sk, fd);
			return -NLE_INVAL;
		}
		so_protocol = protocol;
	}

	nl_socket_set_local_port (sk, local.nl_pid);
	sk->s_local = local;
	sk->s_fd = fd;
	sk->s_proto = so_protocol;

	return 0;
}

/**
 * Set file descriptor of socket to non-blocking state
 * @arg sk		Netlink socket.
 *
 * @return 0 on success or a negative error code.
 */
int nl_socket_set_nonblocking(const struct nl_sock *sk)
{
	if (sk->s_fd == -1)
		return -NLE_BAD_SOCK;

	if (fcntl(sk->s_fd, F_SETFL, O_NONBLOCK) < 0) {
		NL_DBG(4, "nl_socket_set_nonblocking(%p): fcntl() failed with %d (%s)\n",
			sk, errno, nl_strerror_l(errno));
		return -nl_syserr2nlerr(errno);
	}

	return 0;
}

/**
 * Enable use of MSG_PEEK when reading from socket
 * @arg sk		Netlink socket.
 *
 * See also NL_CAPABILITY_NL_RECVMSGS_PEEK_BY_DEFAULT capability
 */
void nl_socket_enable_msg_peek(struct nl_sock *sk)
{
	sk->s_flags |= (NL_MSG_PEEK | NL_MSG_PEEK_EXPLICIT);
}

/**
 * Disable use of MSG_PEEK when reading from socket
 * @arg sk		Netlink socket.
 *
 * See also NL_CAPABILITY_NL_RECVMSGS_PEEK_BY_DEFAULT capability
 */
void nl_socket_disable_msg_peek(struct nl_sock *sk)
{
	sk->s_flags |= NL_MSG_PEEK_EXPLICIT;
	sk->s_flags &= ~NL_MSG_PEEK;
}

/** @} */

/**
 * @name Callback Handler
 * @{
 */

struct nl_cb *nl_socket_get_cb(const struct nl_sock *sk)
{
	return nl_cb_get(sk->s_cb);
}

void nl_socket_set_cb(struct nl_sock *sk, struct nl_cb *cb)
{
	if (cb == NULL)
		BUG();

	nl_cb_put(sk->s_cb);
	sk->s_cb = nl_cb_get(cb);
}

/**
 * Modify the callback handler associated with the socket
 * @arg sk		Netlink socket.
 * @arg type		which type callback to set
 * @arg kind		kind of callback
 * @arg func		callback function
 * @arg arg		argument to be passed to callback function
 *
 * @see nl_cb_set
 */
int nl_socket_modify_cb(struct nl_sock *sk, enum nl_cb_type type,
			enum nl_cb_kind kind, nl_recvmsg_msg_cb_t func,
			void *arg)
{
	return nl_cb_set(sk->s_cb, type, kind, func, arg);
}

/**
 * Modify the error callback handler associated with the socket
 * @arg sk		Netlink socket.
 * @arg kind		kind of callback
 * @arg func		callback function
 * @arg arg		argument to be passed to callback function
 *
 * @see nl_cb_err
 */
int nl_socket_modify_err_cb(struct nl_sock *sk, enum nl_cb_kind kind,
			    nl_recvmsg_err_cb_t func, void *arg)
{
	return nl_cb_err(sk->s_cb, kind, func, arg);
}

/** @} */

/**
 * @name Utilities
 * @{
 */

/**
 * Set socket buffer size of netlink socket.
 * @arg sk		Netlink socket.
 * @arg rxbuf		New receive socket buffer size in bytes.
 * @arg txbuf		New transmit socket buffer size in bytes.
 *
 * Sets the socket buffer size of a netlink socket to the specified
 * values \c rxbuf and \c txbuf. Providing a value of \c 0 assumes a
 * good default value.
 *
 * @note It is not required to call this function prior to nl_connect().
 * @return 0 on success or a negative error code.
 */
int nl_socket_set_buffer_size(struct nl_sock *sk, int rxbuf, int txbuf)
{
	int err;

	if (rxbuf <= 0)
		rxbuf = 32768;

	if (txbuf <= 0)
		txbuf = 32768;

	if (sk->s_fd == -1)
		return -NLE_BAD_SOCK;

	err = setsockopt(sk->s_fd, SOL_SOCKET, SO_SNDBUF,
			 &txbuf, sizeof(txbuf));
	if (err < 0) {
		NL_DBG(4, "nl_socket_set_buffer_size(%p): setsockopt() failed with %d (%s)\n",
			sk, errno, nl_strerror_l(errno));
		return -nl_syserr2nlerr(errno);
	}

	err = setsockopt(sk->s_fd, SOL_SOCKET, SO_RCVBUF,
			 &rxbuf, sizeof(rxbuf));
	if (err < 0) {
		NL_DBG(4, "nl_socket_set_buffer_size(%p): setsockopt() failed with %d (%s)\n",
			sk, errno, nl_strerror_l(errno));
		return -nl_syserr2nlerr(errno);
	}

	return 0;
}

/**
 * Set default message buffer size of netlink socket.
 * @arg sk		Netlink socket.
 * @arg bufsize		Default message buffer size in bytes.
 *
 * Sets the default message buffer size to the specified length in bytes.
 * The default message buffer size limits the maximum message size the
 * socket will be able to receive. It is generally recommneded to specify
 * a buffer size no less than the size of a memory page.
 *
 * Setting the @bufsize to zero means to use a default of 4 times getpagesize().
 *
 * When MSG_PEEK is enabled, the buffer size is used for the initial choice
 * of the buffer while peeking. It still makes sense to choose an optimal value
 * to avoid realloc().
 *
 * When MSG_PEEK is disabled, the buffer size is important because a too small
 * size will lead to failure of receiving the message via nl_recvmsgs().
 *
 * By default, MSG_PEEK is enabled unless the user calls either nl_socket_disable_msg_peek()/
 * nl_socket_enable_msg_peek() or sets the message buffer size to a positive value.
 * See capability NL_CAPABILITY_NL_RECVMSGS_PEEK_BY_DEFAULT for that.
 *
 * @return 0 on success or a negative error code.
 */
int nl_socket_set_msg_buf_size(struct nl_sock *sk, size_t bufsize)
{
	sk->s_bufsize = bufsize;

	return 0;
}

/**
 * Get default message buffer size of netlink socket.
 * @arg sk		Netlink socket.
 *
 * @return Size of default message buffer.
 */
size_t nl_socket_get_msg_buf_size(struct nl_sock *sk)
{
	return sk->s_bufsize;
}

/**
 * Enable/disable credential passing on netlink socket.
 * @arg sk		Netlink socket.
 * @arg state		New state (0 - disabled, 1 - enabled)
 *
 * @return 0 on success or a negative error code
 */
int nl_socket_set_passcred(struct nl_sock *sk, int state)
{
	int err;

	if (sk->s_fd == -1)
		return -NLE_BAD_SOCK;

	err = setsockopt(sk->s_fd, SOL_SOCKET, SO_PASSCRED,
			 &state, sizeof(state));
	if (err < 0) {
		NL_DBG(4, "nl_socket_set_passcred(%p): setsockopt() failed with %d (%s)\n",
			sk, errno, nl_strerror_l(errno));
		return -nl_syserr2nlerr(errno);
	}

	if (state)
		sk->s_flags |= NL_SOCK_PASSCRED;
	else
		sk->s_flags &= ~NL_SOCK_PASSCRED;

	return 0;
}

/**
 * Enable/disable receival of additional packet information
 * @arg sk		Netlink socket.
 * @arg state		New state (0 - disabled, 1 - enabled)
 *
 * @return 0 on success or a negative error code
 */
int nl_socket_recv_pktinfo(struct nl_sock *sk, int state)
{
	int err;

	if (sk->s_fd == -1)
		return -NLE_BAD_SOCK;

	err = setsockopt(sk->s_fd, SOL_NETLINK, NETLINK_PKTINFO,
			 &state, sizeof(state));
	if (err < 0) {
		NL_DBG(4, "nl_socket_recv_pktinfo(%p): setsockopt() failed with %d (%s)\n",
			sk, errno, nl_strerror_l(errno));
		return -nl_syserr2nlerr(errno);
	}

	return 0;
}

/** @} */

/** @} */
