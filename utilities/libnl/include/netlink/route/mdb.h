/* SPDX-License-Identifier: LGPL-2.1-only */

#ifndef NETLINK_MDB_H_
#define NETLINK_MDB_H_

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/route/link.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rtnl_mdb;
struct rtnl_mdb_entry;

struct rtnl_mdb *rtnl_mdb_alloc(void);
void rtnl_mdb_put(struct rtnl_mdb *mdb);

int rtnl_mdb_alloc_cache(struct nl_sock *sk, struct nl_cache **result);
int rtnl_mdb_alloc_cache_flags(struct nl_sock *sock,
			       struct nl_cache **result,
			       unsigned int flags);

uint32_t rtnl_mdb_get_ifindex(struct rtnl_mdb *mdb);
void rtnl_mdb_add_entry(struct rtnl_mdb *mdb,
			struct rtnl_mdb_entry *_entry);

void rtnl_mdb_foreach_entry(struct rtnl_mdb *mdb,
			    void (*cb)(struct rtnl_mdb_entry *, void *),
			    void *arg);

int rtnl_mdb_entry_get_ifindex(struct rtnl_mdb_entry *mdb_entry);
int rtnl_mdb_entry_get_vid(struct rtnl_mdb_entry *mdb_entry);
int rtnl_mdb_entry_get_state(struct rtnl_mdb_entry *mdb_entry);
struct nl_addr *rtnl_mdb_entry_get_addr(struct rtnl_mdb_entry
					*mdb_entry);
uint16_t rtnl_mdb_entry_get_proto(struct rtnl_mdb_entry *mdb_entry);
#ifdef __cplusplus
}
#endif
#endif
