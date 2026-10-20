/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * lib/route/mdb.c		Multicast Database
 */

#include "nl-default.h"

#include <linux/if_bridge.h>

#include <netlink/netlink.h>
#include <netlink/route/mdb.h>
#include <netlink/route/nexthop.h>
#include <netlink/utils.h>
#include <netlink/route/rtnl.h>

#include "nl-route.h"
#include "nl-aux-route/nl-route.h"
#include "nl-priv-dynamic-core/object-api.h"
#include "nl-priv-dynamic-core/cache-api.h"

/** @cond SKIP */
#define MDB_ATTR_IFINDEX         0x000001
#define MDB_ATTR_ENTRIES         0x000002

struct rtnl_mdb {
	NLHDR_COMMON
	uint32_t ifindex;

	struct nl_list_head mdb_entry_list;
};

struct rtnl_mdb_entry {
	struct nl_list_head mdb_list;
	struct nl_addr *addr;
	uint32_t ifindex;
	uint16_t vid;
	uint16_t proto;
	uint8_t state;
};

static struct rtnl_mdb_entry *rtnl_mdb_entry_alloc(void);
static void rtnl_mdb_entry_free(struct rtnl_mdb_entry *mdb_entry);

static struct nl_cache_ops rtnl_mdb_ops;
static struct nl_object_ops mdb_obj_ops;
/** @endcond */

static void mdb_constructor(struct nl_object *obj)
{
	struct rtnl_mdb *_mdb = (struct rtnl_mdb *) obj;

	nl_init_list_head(&_mdb->mdb_entry_list);
}

static void mdb_free_data(struct nl_object *obj)
{
	struct rtnl_mdb *mdb = (struct rtnl_mdb *)obj;
	struct rtnl_mdb_entry *mdb_entry;
	struct rtnl_mdb_entry *mdb_entry_safe;

	nl_list_for_each_entry_safe(mdb_entry, mdb_entry_safe,
				    &mdb->mdb_entry_list, mdb_list)
		rtnl_mdb_entry_free(mdb_entry);
}

static int mdb_entry_equal(struct rtnl_mdb_entry *a, struct rtnl_mdb_entry *b)
{
	return    a->ifindex == b->ifindex
	       && a->vid == b->vid
	       && a->proto == b->proto
	       && a->state == b->state
	       && nl_addr_cmp(a->addr, b->addr) == 0;
}

static uint64_t mdb_compare(struct nl_object *_a, struct nl_object *_b,
			    uint64_t attrs, int flags)
{
	struct rtnl_mdb *a = (struct rtnl_mdb *) _a;
	struct rtnl_mdb *b = (struct rtnl_mdb *) _b;
	struct rtnl_mdb_entry *a_entry, *b_entry;
	uint64_t diff = 0;

#define _DIFF(ATTR, EXPR) ATTR_DIFF(attrs, ATTR, a, b, EXPR)
	diff |= _DIFF(MDB_ATTR_IFINDEX, a->ifindex != b->ifindex);
#undef _DIFF

	a_entry = nl_list_entry(a->mdb_entry_list.next, struct rtnl_mdb_entry, mdb_list);
	b_entry = nl_list_entry(b->mdb_entry_list.next, struct rtnl_mdb_entry, mdb_list);
	while (1) {
		if (   &a_entry->mdb_list == &a->mdb_entry_list
		    || &b_entry->mdb_list == &b->mdb_entry_list) {
			if (   &a_entry->mdb_list != &a->mdb_entry_list
			    || &b_entry->mdb_list != &b->mdb_entry_list)
				diff |= MDB_ATTR_ENTRIES;
			break;
		}
		if (!mdb_entry_equal(a_entry, b_entry)) {
			diff |= MDB_ATTR_ENTRIES;
			break;
		}
		a_entry = nl_list_entry(a_entry->mdb_list.next, struct rtnl_mdb_entry, mdb_list);
		b_entry = nl_list_entry(b_entry->mdb_list.next, struct rtnl_mdb_entry, mdb_list);
	}

	return diff;
}

static struct rtnl_mdb_entry *mdb_entry_clone(struct rtnl_mdb_entry *src)
{
	struct rtnl_mdb_entry *dst = rtnl_mdb_entry_alloc();
	if (!dst)
		return NULL;

	dst->ifindex = src->ifindex;
	dst->state = src->state;
	dst->vid = src->vid;
	dst->proto = src->proto;

	dst->addr = nl_addr_clone(src->addr);
	if (dst->addr == NULL) {
		free(dst);
		return NULL;
	}

	return dst;
}

static int mdb_clone(struct nl_object *_dst, struct nl_object *_src)
{
	struct rtnl_mdb *dst = nl_object_priv(_dst);
	struct rtnl_mdb *src = nl_object_priv(_src);
	struct rtnl_mdb_entry *entry;

	nl_init_list_head(&dst->mdb_entry_list);

	nl_list_for_each_entry(entry, &src->mdb_entry_list, mdb_list) {
		struct rtnl_mdb_entry *copy = mdb_entry_clone(entry);

		if (!copy)
			return -NLE_NOMEM;

		rtnl_mdb_add_entry(dst, copy);
	}

	return 0;
}

static int mdb_update(struct nl_object *old_obj, struct nl_object *new_obj)
{
	struct rtnl_mdb *old = (struct rtnl_mdb *) old_obj;
	struct rtnl_mdb *new = (struct rtnl_mdb *) new_obj;
	struct rtnl_mdb_entry *entry, *old_entry;
	int action = new_obj->ce_msgtype;

	if (new->ifindex != old->ifindex)
		return -NLE_OPNOTSUPP;

	switch (action) {
	case RTM_NEWMDB:
		nl_list_for_each_entry(entry, &new->mdb_entry_list, mdb_list) {
			struct rtnl_mdb_entry *copy = mdb_entry_clone(entry);

			if (!copy)
				return -NLE_NOMEM;

			rtnl_mdb_add_entry(old, copy);
		}
		break;
	case RTM_DELMDB:
		entry = nl_list_first_entry(&new->mdb_entry_list,
		                            struct rtnl_mdb_entry,
		                            mdb_list);
		nl_list_for_each_entry(old_entry, &old->mdb_entry_list, mdb_list) {
			if (   old_entry->ifindex == entry->ifindex
			    && !nl_addr_cmp(old_entry->addr, entry->addr)) {
				nl_list_del(&old_entry->mdb_list);
				break;
			}
		}
		break;
	}

	return NLE_SUCCESS;
}

static struct nla_policy mdb_policy[MDBA_MAX + 1] = {
	[MDBA_MDB] = {.type = NLA_NESTED},
};

static struct nla_policy mdb_db_policy[MDBA_MDB_MAX + 1] = {
	[MDBA_MDB_ENTRY] = {.type = NLA_NESTED},
};

static int mdb_msg_parser(struct nl_cache_ops *ops, struct sockaddr_nl *who,
			  struct nlmsghdr *nlh, struct nl_parser_param *pp)
{
	int err = 0;
	int rem = 0;
	struct nlattr *tb[MDBA_MAX + 1];
	struct br_port_msg *port;
	struct nlattr *nla;
	struct br_mdb_entry *e;
	_nl_auto_rtnl_mdb struct rtnl_mdb *mdb = rtnl_mdb_alloc();

	if (!mdb)
		return -NLE_NOMEM;

	err = nlmsg_parse(nlh, sizeof(struct br_port_msg), tb, MDBA_MAX,
	                  mdb_policy);
	if (err < 0)
		return err;

	mdb->ce_msgtype = nlh->nlmsg_type;

	port = nlmsg_data(nlh);
	mdb->ifindex = port->ifindex;
	mdb->ce_mask |= MDB_ATTR_IFINDEX;

	if (tb[MDBA_MDB]) {
		struct nlattr *db_attr[MDBA_MDB_MAX+1];

		err = nla_parse_nested(db_attr, MDBA_MDB_MAX, tb[MDBA_MDB],
				       mdb_db_policy);
		if (err < 0)
			return err;
		rem = nla_len(tb[MDBA_MDB]);

		for (nla = nla_data(tb[MDBA_MDB]); nla_ok(nla, rem);
		     nla = nla_next(nla, &rem)) {
			int rm = nla_len(nla);
			struct nlattr *nla2;

			for (nla2 = nla_data(nla); nla_ok(nla2, rm);
			     nla2 = nla_next(nla2, &rm)) {
				_nl_auto_nl_addr struct nl_addr *addr = NULL;
				struct rtnl_mdb_entry *entry;
				uint16_t proto;

				e = nla_data(nla2);

				proto = ntohs(e->addr.proto);

				if (proto == ETH_P_IP) {
					addr = nl_addr_build(
						AF_INET, &e->addr.u.ip4,
						sizeof(e->addr.u.ip4));
				} else if (proto == ETH_P_IPV6) {
					addr = nl_addr_build(
						AF_INET6, &e->addr.u.ip6,
						sizeof(e->addr.u.ip6));
				} else {
					addr = nl_addr_build(
						AF_LLC, e->addr.u.mac_addr,
						sizeof(e->addr.u.mac_addr));
				}
				if (!addr)
					return -NLE_NOMEM;

				entry = rtnl_mdb_entry_alloc();
				if (!entry)
					return -NLE_NOMEM;

				mdb->ce_mask |= MDB_ATTR_ENTRIES;

				entry->ifindex = e->ifindex;
				entry->vid = e->vid;
				entry->state = e->state;
				entry->proto = ntohs(e->addr.proto);
				entry->addr = _nl_steal_pointer(&addr);
				rtnl_mdb_add_entry(mdb, entry);
			}
		}
	}

	return pp->pp_cb((struct nl_object *) mdb, pp);
}

static int mdb_request_update(struct nl_cache *cache, struct nl_sock *sk)
{
	return nl_rtgen_request(sk, RTM_GETMDB, AF_BRIDGE, NLM_F_DUMP);
}

static void mdb_entry_dump_line(struct rtnl_mdb_entry *entry,
                                struct nl_dump_params *p)
{
	char buf[INET6_ADDRSTRLEN];

	nl_dump(p, "port %d ", entry->ifindex);
	nl_dump(p, "vid %d ", entry->vid);
	nl_dump(p, "proto 0x%04x ", entry->proto);
	nl_dump(p, "address %s\n", nl_addr2str(entry->addr, buf, sizeof(buf)));
}

static void mdb_dump_line(struct nl_object *obj, struct nl_dump_params *p)
{
	struct rtnl_mdb *mdb = (struct rtnl_mdb *) obj;
	struct rtnl_mdb_entry *_mdb;

	nl_dump(p, "dev %d \n", mdb->ifindex);

	nl_list_for_each_entry(_mdb, &mdb->mdb_entry_list, mdb_list) {
		p->dp_ivar = NH_DUMP_FROM_ONELINE;
		mdb_entry_dump_line(_mdb, p);
	}
}

static void mdb_dump_details(struct nl_object *obj, struct nl_dump_params *p)
{
	mdb_dump_line(obj, p);
}

static void mdb_dump_stats(struct nl_object *obj, struct nl_dump_params *p)
{
	mdb_dump_details(obj, p);
}

void rtnl_mdb_put(struct rtnl_mdb *mdb)
{
	nl_object_put((struct nl_object *) mdb);
}

/** @} */

/**
 * @name Cache Management
 * @{
 */
int rtnl_mdb_alloc_cache(struct nl_sock *sk, struct nl_cache **result)
{
	return nl_cache_alloc_and_fill(&rtnl_mdb_ops, sk, result);
}

/**
 * Build a neighbour cache including all MDB entries currently configured in the kernel.
 * @arg sock		Netlink socket.
 * @arg result		Pointer to store resulting cache.
 * @arg flags		Flags to apply to cache before filling
 *
 * Allocates a new MDB cache, initializes it properly and updates it
 * to include all Multicast Database entries currently configured in the kernel.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_mdb_alloc_cache_flags(struct nl_sock *sock, struct nl_cache **result,
			       unsigned int flags)
{
	struct nl_cache *cache;
	int err;

	cache = nl_cache_alloc(&rtnl_mdb_ops);
	if (!cache)
		return -NLE_NOMEM;

	nl_cache_set_flags(cache, flags);

	if (sock && (err = nl_cache_refill(sock, cache)) < 0) {
		nl_cache_free(cache);
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
uint32_t rtnl_mdb_get_ifindex(struct rtnl_mdb *mdb)
{
	return mdb->ifindex;
}

void rtnl_mdb_add_entry(struct rtnl_mdb *mdb, struct rtnl_mdb_entry *entry)
{
	nl_list_add_tail(&entry->mdb_list, &mdb->mdb_entry_list);
}

void rtnl_mdb_foreach_entry(struct rtnl_mdb *mdb,
                            void (*cb)(struct rtnl_mdb_entry *, void *),
                            void *arg)
{
	struct rtnl_mdb_entry *entry;

	nl_list_for_each_entry(entry, &mdb->mdb_entry_list, mdb_list) {
		cb(entry, arg);
	}
}

int rtnl_mdb_entry_get_ifindex(struct rtnl_mdb_entry *mdb_entry)
{
	return mdb_entry->ifindex;
}

int rtnl_mdb_entry_get_vid(struct rtnl_mdb_entry *mdb_entry)
{
	return mdb_entry->vid;
}

int rtnl_mdb_entry_get_state(struct rtnl_mdb_entry *mdb_entry)
{
	return mdb_entry->state;
}

struct nl_addr *rtnl_mdb_entry_get_addr(struct rtnl_mdb_entry *mdb_entry)
{
	return mdb_entry->addr;
}

uint16_t rtnl_mdb_entry_get_proto(struct rtnl_mdb_entry *mdb_entry)
{
	return mdb_entry->proto;
}

/** @} */

static struct nl_object_ops mdb_obj_ops = {
	.oo_name = "route/mdb",
	.oo_size = sizeof(struct rtnl_mdb),
	.oo_constructor = mdb_constructor,
	.oo_dump = {
	            [NL_DUMP_LINE]    = mdb_dump_line,
	            [NL_DUMP_DETAILS] = mdb_dump_details,
	            [NL_DUMP_STATS]   = mdb_dump_stats,
	},
	.oo_clone = mdb_clone,
	.oo_compare = mdb_compare,
	.oo_update = mdb_update,
	.oo_free_data = mdb_free_data,
};

struct rtnl_mdb *rtnl_mdb_alloc(void)
{
	return (struct rtnl_mdb *) nl_object_alloc(&mdb_obj_ops);
}

static struct rtnl_mdb_entry *rtnl_mdb_entry_alloc(void)
{
	struct rtnl_mdb_entry *mdb;

	mdb = calloc(1, sizeof(struct rtnl_mdb_entry));
	if (!mdb)
		return NULL;

	nl_init_list_head(&mdb->mdb_list);

	return mdb;

}

static void rtnl_mdb_entry_free(struct rtnl_mdb_entry *mdb_entry)
{
	nl_list_del(&mdb_entry->mdb_list);
	nl_addr_put(mdb_entry->addr);
	free(mdb_entry);
}

static struct nl_af_group mdb_groups[] = {
	{AF_BRIDGE, RTNLGRP_MDB},
	{END_OF_GROUP_LIST},
};

static struct nl_cache_ops rtnl_mdb_ops = {
	.co_name = "route/mdb",
	.co_hdrsize = sizeof(struct br_port_msg),
	.co_msgtypes = {
	                { RTM_NEWMDB, NL_ACT_NEW, "new"},
	                { RTM_DELMDB, NL_ACT_DEL, "del"},
	                { RTM_GETMDB, NL_ACT_GET, "get"},
	                END_OF_MSGTYPES_LIST,
	                },
	.co_protocol = NETLINK_ROUTE,
	.co_groups = mdb_groups,
	.co_request_update = mdb_request_update,
	.co_msg_parser = mdb_msg_parser,
	.co_obj_ops = &mdb_obj_ops,
};

static void _nl_init mdb_init(void)
{
	nl_cache_mngt_register(&rtnl_mdb_ops);
}

static void _nl_exit mdb_exit(void)
{
	nl_cache_mngt_unregister(&rtnl_mdb_ops);
}

/** @} */
