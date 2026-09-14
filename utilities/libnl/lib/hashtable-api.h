/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Private resizable hash table API for libnl.
 */

#ifndef NETLINK_HASHTABLE_API_H_
#define NETLINK_HASHTABLE_API_H_

#include "nl-priv-dynamic-core/object-api.h"

/* Opaque resizable hash table handle. */
typedef struct nl_rhash_table nl_rhash_table_t;

/* Allocation / Deletion */
nl_rhash_table_t *nl_rhash_table_alloc(void);
void nl_rhash_table_free(nl_rhash_table_t *ht);

/* Access helpers */
struct nl_object *nl_rhash_table_lookup(nl_rhash_table_t *ht,
					struct nl_object *obj);
int nl_rhash_table_add(nl_rhash_table_t *ht, struct nl_object *obj);
int nl_rhash_table_del(nl_rhash_table_t *ht, struct nl_object *obj);

#endif /* NETLINK_HASHTABLE_API_H_ */
