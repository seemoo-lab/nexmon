/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __NL_SHARED_CORE_NL_CORE_H__
#define __NL_SHARED_CORE_NL_CORE_H__

#define NL_SOCK_PASSCRED (1 << 1)
#define NL_OWN_PORT (1 << 2)
#define NL_MSG_PEEK (1 << 3)
#define NL_MSG_PEEK_EXPLICIT (1 << 4)
#define NL_NO_AUTO_ACK (1 << 5)

struct nl_sock {
	struct sockaddr_nl s_local;
	struct sockaddr_nl s_peer;
	int s_fd;
	int s_proto;
	unsigned int s_seq_next;
	unsigned int s_seq_expect;
	int s_flags;
	struct nl_cb *s_cb;
	size_t s_bufsize;
};

static inline int wait_for_ack(struct nl_sock *sk)
{
	if (sk->s_flags & NL_NO_AUTO_ACK)
		return 0;
	else
		return nl_wait_for_ack(sk);
}

#define LOOSE_COMPARISON 1
#define ID_COMPARISON 2

#define NL_OBJ_MARK 1

struct nl_data {
	size_t d_size;
	void *d_data;
};

struct nl_addr {
	int a_family;
	unsigned int a_maxsize;
	unsigned int a_len;
	int a_prefixlen;
	int a_refcnt;
	char a_addr[0];
};

#define NL_MSG_CRED_PRESENT 1

struct nl_msg {
	int nm_protocol;
	int nm_flags;
	struct sockaddr_nl nm_src;
	struct sockaddr_nl nm_dst;
	struct ucred nm_creds;
	struct nlmsghdr *nm_nlh;
	size_t nm_size;
	int nm_refcnt;
};

/*****************************************************************************/

int nl_getprotobyname(const char *name);

bool nl_getprotobynumber(int proto, char *out_name, size_t name_len);

extern const char *nl_strerror_l(int err);

extern int __nl_read_num_str_file(const char *path,
				  int (*cb)(long, const char *));

extern int __trans_list_add(int, const char *, struct nl_list_head *);
extern void __trans_list_clear(struct nl_list_head *);

extern char *__type2str(int, char *, size_t, const struct trans_tbl *, size_t);
extern int __str2type(const char *, const struct trans_tbl *, size_t);

extern char *__list_type2str(int, char *, size_t, struct nl_list_head *);
extern int __list_str2type(const char *, struct nl_list_head *);

extern char *__flags2str(int, char *, size_t, const struct trans_tbl *, size_t);
extern int __str2flags(const char *, const struct trans_tbl *, size_t);

#endif /* __NL_SHARED_CORE_NL_CORE_H__ */
