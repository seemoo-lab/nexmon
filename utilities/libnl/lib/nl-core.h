/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2014 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __LIB_NL_CORE_H__
#define __LIB_NL_CORE_H__

struct nl_cb
{
	nl_recvmsg_msg_cb_t	cb_set[NL_CB_TYPE_MAX+1];
	void *			cb_args[NL_CB_TYPE_MAX+1];

	nl_recvmsg_err_cb_t	cb_err;
	void *			cb_err_arg;

	/** May be used to replace nl_recvmsgs with your own implementation
	 * in all internal calls to nl_recvmsgs. */
	int			(*cb_recvmsgs_ow)(struct nl_sock *,
						  struct nl_cb *);

	/** Overwrite internal calls to nl_recv, must return the number of
	 * octets read and allocate a buffer for the received data. */
	int			(*cb_recv_ow)(struct nl_sock *,
					      struct sockaddr_nl *,
					      unsigned char **,
					      struct ucred **);

	/** Overwrites internal calls to nl_send, must send the netlink
	 * message. */
	int			(*cb_send_ow)(struct nl_sock *,
					      struct nl_msg *);

	int			cb_refcnt;
	/** indicates the callback that is currently active */
	enum nl_cb_type		cb_active;
};

static inline int nl_cb_call(struct nl_cb *cb, enum nl_cb_type type, struct nl_msg *msg)
{
	int ret;

	cb->cb_active = type;
	ret = cb->cb_set[type](msg, cb->cb_args[type]);
	cb->cb_active = __NL_CB_TYPE_MAX;
	return ret;
}

int _nl_socket_is_local_port_unspecified (struct nl_sock *sk);
uint32_t _nl_socket_set_local_port_no_release(struct nl_sock *sk, int generate_other);

void _nl_socket_used_ports_release_all(const uint32_t *used_ports);
void _nl_socket_used_ports_set(uint32_t *used_ports, uint32_t port);

extern int nl_cache_parse(struct nl_cache_ops *, struct sockaddr_nl *,
			  struct nlmsghdr *, struct nl_parser_param *);

extern void dump_from_ops(struct nl_object *, struct nl_dump_params *);

#endif /* __LIB_NL_CORE_H__ */
