/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_ADDR_H_
#define NETLINK_ADDR_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nlattr;

struct nl_addr;

/* Creation */
extern struct nl_addr *	nl_addr_alloc(size_t);
extern struct nl_addr *	nl_addr_alloc_attr(const struct nlattr *, int);
extern struct nl_addr *	nl_addr_build(int, const void *, size_t);
extern int		nl_addr_parse(const char *, int, struct nl_addr **);
extern struct nl_addr *	nl_addr_clone(const struct nl_addr *);

/* Usage Management */
extern struct nl_addr *	nl_addr_get(struct nl_addr *);
extern void		nl_addr_put(struct nl_addr *);
extern int		nl_addr_shared(const struct nl_addr *);

extern int		nl_addr_cmp(const struct nl_addr *,
				    const struct nl_addr *);
extern int		nl_addr_cmp_prefix(const struct nl_addr *,
					   const struct nl_addr *);
extern int		nl_addr_iszero(const struct nl_addr *);
extern int		nl_addr_valid(const char *, int);
extern int      	nl_addr_guess_family(const struct nl_addr *);
extern int		nl_addr_fill_sockaddr(const struct nl_addr *,
					      struct sockaddr *, socklen_t *);
extern int		nl_addr_info(const struct nl_addr *,
				     struct addrinfo **);
extern int		nl_addr_resolve(const struct nl_addr *, char *, size_t);

/* Access Functions */
extern void		nl_addr_set_family(struct nl_addr *, int);
extern int		nl_addr_get_family(const struct nl_addr *);
extern int		nl_addr_set_binary_addr(struct nl_addr *, const void *,
						size_t);
extern void *		nl_addr_get_binary_addr(const struct nl_addr *);
extern unsigned int	nl_addr_get_len(const struct nl_addr *);
extern void		nl_addr_set_prefixlen(struct nl_addr *, int);
extern unsigned int	nl_addr_get_prefixlen(const struct nl_addr *);

/* Address Family Translations */
extern char *		nl_af2str(int, char *, size_t);
extern int		nl_str2af(const char *);

/* Translations to Strings */
extern char *		nl_addr2str(const struct nl_addr *, char *, size_t);

#ifdef __cplusplus
}
#endif

#endif
