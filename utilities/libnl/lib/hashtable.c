/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2012 Cumulus Networks, Inc
 */

#include "nl-default.h"

#include "hashtable-api.h"

#include <netlink/object.h>
#include <netlink/hash.h>
#include <netlink/hashtable.h>

#include "nl-aux-core/nl-core.h"

/**
 * @ingroup core_types
 * @defgroup hashtable Hashtable
 * @{
 */

/* Generic helper to handle regular and resizeable hash tables */
static nl_hash_table_t *_nl_hash_table_init(nl_hash_table_t *ht, int size)
{
	ht->nodes = calloc(size, sizeof(*ht->nodes));
	if (!ht->nodes) {
		return NULL;
	}

	ht->size = size;

	return ht;
}

/**
 * Allocate hashtable
 * @arg size		Size of hashtable in number of elements
 *
 * @return Allocated hashtable or NULL.
 */
nl_hash_table_t *nl_hash_table_alloc(int size)
{
	nl_hash_table_t *ht;

	ht = calloc(1, sizeof(*ht));
	if (!ht)
		return NULL;

	return _nl_hash_table_init(ht, size);
}

/* Generic helper to handle regular and resizeable hash tables */
static void _nl_hash_table_free(nl_hash_table_t *ht)
{
	int i;

	for (i = 0; i < ht->size; i++) {
		nl_hash_node_t *node = ht->nodes[i];
		nl_hash_node_t *saved_node;

		while (node) {
			saved_node = node;
			node = node->next;
			nl_object_put(saved_node->obj);
			free(saved_node);
		}
	}

	free(ht->nodes);
}

/**
 * Free hashtable including all nodes
 * @arg ht		Hashtable
 *
 * @note Reference counter of all objects in the hashtable will be decremented.
 */
void nl_hash_table_free(nl_hash_table_t *ht)
{
	_nl_hash_table_free(ht);
	free(ht);
}

/**
 * Lookup identical object in hashtable
 * @arg ht		Hashtable
 * @arg	obj		Object to lookup
 *
 * Generates hashkey for `obj` and traverses the corresponding chain calling
 * `nl_object_identical()` on each trying to find a match.
 *
 * @return Pointer to object if match was found or NULL.
 */
struct nl_object *nl_hash_table_lookup(nl_hash_table_t *ht,
				       struct nl_object *obj)
{
	nl_hash_node_t *node;
	uint32_t key_hash;

	nl_object_keygen(obj, &key_hash, ht->size);
	node = ht->nodes[key_hash];

	while (node) {
		if (nl_object_identical(node->obj, obj))
			return node->obj;
		node = node->next;
	}

	return NULL;
}

/**
 * Add object to hashtable
 * @arg ht		Hashtable
 * @arg obj		Object to add
 *
 * Adds `obj` to the hashtable. Object type must support hashing, otherwise all
 * objects will be added to the chain `0`.
 *
 * @note The reference counter of the object is incremented.
 *
 * @return 0 on success or a negative error code
 * @retval -NLE_EXIST Identical object already present in hashtable
 */
int nl_hash_table_add(nl_hash_table_t *ht, struct nl_object *obj)
{
	nl_hash_node_t *node;
	uint32_t key_hash;

	nl_object_keygen(obj, &key_hash, ht->size);
	node = ht->nodes[key_hash];

	while (node) {
		if (nl_object_identical(node->obj, obj)) {
			NL_DBG(2,
			       "Warning: Add to hashtable found duplicate...\n");
			return -NLE_EXIST;
		}
		node = node->next;
	}

	NL_DBG(5, "adding cache entry of obj %p in table %p, with hash 0x%x\n",
	       obj, ht, key_hash);

	node = malloc(sizeof(nl_hash_node_t));
	if (!node)
		return -NLE_NOMEM;
	nl_object_get(obj);
	node->obj = obj;
	node->key = key_hash;
	node->key_size = sizeof(uint32_t);
	node->next = ht->nodes[key_hash];
	ht->nodes[key_hash] = node;

	return 0;
}

/**
 * Remove object from hashtable
 * @arg ht		Hashtable
 * @arg obj		Object to remove
 *
 * Remove `obj` from hashtable if it exists.
 *
 * @note Reference counter of object will be decremented.
 *
 * @return 0 on success or a negative error code.
 * @retval -NLE_OBJ_NOTFOUND Object not present in hashtable.
 */
int nl_hash_table_del(nl_hash_table_t *ht, struct nl_object *obj)
{
	nl_hash_node_t *node, *prev;
	uint32_t key_hash;

	nl_object_keygen(obj, &key_hash, ht->size);
	prev = node = ht->nodes[key_hash];

	while (node) {
		if (nl_object_identical(node->obj, obj)) {
			nl_object_put(obj);

			NL_DBG(5,
			       "deleting cache entry of obj %p in table %p, with"
			       " hash 0x%x\n",
			       obj, ht, key_hash);

			if (node == ht->nodes[key_hash])
				ht->nodes[key_hash] = node->next;
			else
				prev->next = node->next;

			free(node);

			return 0;
		}
		prev = node;
		node = node->next;
	}

	return -NLE_OBJ_NOTFOUND;
}

uint32_t nl_hash(void *k, size_t length, uint32_t initval)
{
	return (__nl_hash((char *)k, length, initval));
}

typedef struct nl_rhash_table {
	struct nl_hash_table hash_table;
	int orig_size; /* Original size of hashtable at creation time */

	/* Number of elements currently stored in the table. Needed to
	 * determine when the table needs to be resized.
	 */
	int nelements;
} nl_rhash_table_t;

/* Internal load factor threshold at which the table will be resized. The
 * value is represented as a fraction where NL_HT_LOAD_NUM / NL_HT_LOAD_DEN
 * is the maximal allowed load factor.
 *
 * A value of 3/4 (0.75) provides a good trade-off between memory usage and
 * lookup performance for a chaining hash table.
 */
#define NL_HT_LOAD_NUM 3
#define NL_HT_LOAD_DEN 4

/**
 * Private helper which performs a rehash of the table to a new size.  On
 * success the table fields (size/nodes) are updated and 0 is returned.
 * On allocation failure the table is left untouched and -NLE_NOMEM is
 * returned so that the caller can gracefully continue operating with the
 * original (but possibly crowded) table.
 */
static int nl_rhash_table_resize(nl_rhash_table_t *ht, int new_size)
{
	nl_hash_node_t **new_nodes;
	int i;

	if (new_size <= 0)
		return 0;

	new_nodes = calloc(new_size, sizeof(*new_nodes));
	if (!new_nodes)
		return -NLE_NOMEM;

	/* Re-hash all existing nodes into the new bucket array. */
	for (i = 0; i < ht->hash_table.size; i++) {
		nl_hash_node_t *node = ht->hash_table.nodes[i];
		while (node) {
			nl_hash_node_t *next = node->next;
			uint32_t new_hash;

			nl_object_keygen(node->obj, &new_hash, new_size);
			node->key = new_hash;

			/* Insert at head of the new bucket. */
			node->next = new_nodes[new_hash];
			new_nodes[new_hash] = node;

			node = next;
		}
	}

	free(ht->hash_table.nodes);
	ht->hash_table.nodes = new_nodes;
	ht->hash_table.size = new_size;

	return 0;
}

#define NL_INIT_RHASH_ENTRIES 8

/**
 * Allocate resizeable hashtable
 *
 * @return Allocated hashtable or NULL.
 */
nl_rhash_table_t *nl_rhash_table_alloc()
{
	nl_rhash_table_t *ht;

	ht = calloc(1, sizeof(*ht));
	if (!ht)
		goto errout;

	if (!_nl_hash_table_init(&ht->hash_table, NL_INIT_RHASH_ENTRIES)) {
		goto errout;
	}

	ht->orig_size = NL_INIT_RHASH_ENTRIES;
	ht->nelements = 0;

	return ht;
errout:
	free(ht);
	return NULL;
}

/**
 * Free resizeable hashtable including all nodes
 * @arg ht		Resizeable Hashtable
 *
 * @note Reference counter of all objects in the hashtable will be decremented.
 */
void nl_rhash_table_free(nl_rhash_table_t *ht)
{
	_nl_hash_table_free(&ht->hash_table);
	free(ht);
}

/**
 * Lookup identical object in resizeable hashtable
 * @arg ht		Resizeable Hashtable
 * @arg	obj		Object to lookup
 *
 * Generates hashkey for `obj` and traverses the corresponding chain calling
 * `nl_object_identical()` on each trying to find a match.
 *
 * @return Pointer to object if match was found or NULL.
 */
struct nl_object *nl_rhash_table_lookup(nl_rhash_table_t *ht,
					struct nl_object *obj)
{
	return nl_hash_table_lookup(&ht->hash_table, obj);
}

/**
 * Add object to resizeable hashtable
 * @arg ht		Resizeable Hashtable
 * @arg obj		Object to add
 *
 * Adds `obj` to the resizeable hashtable. Object type must support hashing,
 * otherwise all objects will be added to the chain `0`.
 *
 * @note The reference counter of the object is incremented.
 *
 * @return 0 on success or a negative error code
 * @retval -NLE_EXIST Identical object already present in hashtable
 */
int nl_rhash_table_add(nl_rhash_table_t *ht, struct nl_object *obj)
{
	int size;
	int rc;

	rc = nl_hash_table_add(&ht->hash_table, obj);
	if (rc < 0)
		return rc;

	if (obj->ce_ops->oo_keygen == NULL)
		return 0;

	/* Update element count and resize if load factor exceeded */
	ht->nelements++;

	size = ht->hash_table.size;
	if (ht->nelements * NL_HT_LOAD_DEN > size * NL_HT_LOAD_NUM) {
		/* Ignore allocation failure – operating with the old table
		 * keeps us functional albeit slower.
		 */
		nl_rhash_table_resize(ht, size * 2);
	}

	return 0;
}

/**
 * Remove object from resizeable hashtable
 * @arg ht		Resizeable Hashtable
 * @arg obj		Object to remove
 *
 * Remove `obj` from resizeable hashtable if it exists.
 *
 * @note Reference counter of object will be decremented.
 *
 * @return 0 on success or a negative error code.
 * @retval -NLE_OBJ_NOTFOUND Object not present in hashtable.
 */
int nl_rhash_table_del(nl_rhash_table_t *ht, struct nl_object *obj)
{
	int size;
	int rc;

	rc = nl_hash_table_del(&ht->hash_table, obj);
	if (rc < 0)
		return rc;

	if (obj->ce_ops->oo_keygen == NULL)
		return 0;

	/* Decrement element count; shrink table if it became sparse.
	 * We keep this simple and only shrink when the table is more than
	 * 4 times sparser than our desired load factor.
	 */
	if (ht->nelements > 0) {
		ht->nelements--;
	}
	size = ht->hash_table.size;
	if (size > ht->orig_size &&
	    ht->nelements * NL_HT_LOAD_DEN < (size / 4) * NL_HT_LOAD_NUM) {
		int new_size = size / 2;

		if (new_size < ht->orig_size)
			new_size = ht->orig_size;

		nl_rhash_table_resize(ht, new_size);
	}

	return 0;
}

/** @} */
