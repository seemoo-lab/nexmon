/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

#ifndef NETLINK_DATA_H_
#define NETLINK_DATA_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nlattr;

struct nl_data;

/* General */
extern struct nl_data *	nl_data_alloc(const void *, size_t);
extern struct nl_data * nl_data_alloc_attr(const struct nlattr *);
extern struct nl_data *	nl_data_clone(const struct nl_data *);
extern int		nl_data_append(struct nl_data *, const void *, size_t);
extern void		nl_data_free(struct nl_data *);

/* Access Functions */
extern void *		nl_data_get(const struct nl_data *);
extern size_t		nl_data_get_size(const struct nl_data *);

/* Misc */
extern int		nl_data_cmp(const struct nl_data *,
				    const struct nl_data *);

#ifdef __cplusplus
}
#endif

#endif
