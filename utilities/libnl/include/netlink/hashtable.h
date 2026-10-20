/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2012 Cumulus Networks, Inc
 */

#ifndef NETLINK_HASHTABLE_H_
#define NETLINK_HASHTABLE_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nl_hash_node {
	uint32_t key;
	uint32_t key_size;
	struct nl_object *obj;
	struct nl_hash_node *next;
} nl_hash_node_t;

typedef struct nl_hash_table {
	int size;
	nl_hash_node_t **nodes;
} nl_hash_table_t;

/* Default hash table size */
#define NL_MAX_HASH_ENTRIES 1024

/* Access Functions */
extern nl_hash_table_t *nl_hash_table_alloc(int size);
extern void nl_hash_table_free(nl_hash_table_t *ht);

extern int nl_hash_table_add(nl_hash_table_t *ht, struct nl_object *obj);
extern int nl_hash_table_del(nl_hash_table_t *ht, struct nl_object *obj);

extern struct nl_object *nl_hash_table_lookup(nl_hash_table_t *ht,
					      struct nl_object *obj);
extern uint32_t nl_hash(void *k, size_t length, uint32_t initval);

#ifdef __cplusplus
}
#endif

#endif /* NETLINK_HASHTABLE_H_ */
