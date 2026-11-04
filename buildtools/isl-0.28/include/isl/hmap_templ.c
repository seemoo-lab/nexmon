/*
 * Copyright 2011      INRIA Saclay
 * Copyright 2013      Ecole Normale Superieure
 *
 * Use of this software is governed by the MIT license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 * and Ecole Normale Superieure, 45 rue d'Ulm, 75230 Paris, France
 */

#include <isl/ctx.h>
#include <isl/hash.h>
#include <isl/stream.h>

#define ISL_xCAT(A,B) A ## B
#define ISL_CAT(A,B) ISL_xCAT(A,B)
#define ISL_xFN(TYPE,NAME) TYPE ## _ ## NAME
#define ISL_FN(TYPE,NAME) ISL_xFN(TYPE,NAME)
#define ISL_xKV(TYPE1,TYPE2,NAME) isl_ ## TYPE1 ## _ ## TYPE2 ## _ ## NAME
#define ISL_yKV(TYPE1,TYPE2,NAME) ISL_xKV(TYPE1,TYPE2,NAME)
#define ISL_KV(NAME) ISL_yKV(ISL_KEY,ISL_VAL,NAME)
#define ISL_S(NAME) struct ISL_KV(NAME)

#define ISL_HMAP_EL		ISL_KV(pair)

#define ISL_HBASE		ISL_HMAP
#define ISL_HBASE_SUFFIX	ISL_HMAP_SUFFIX
#define ISL_HBASE_EL		ISL_HMAP_EL

struct ISL_HMAP_EL {
	ISL_KEY *key;
	ISL_VAL *val;
};
typedef struct ISL_HMAP_EL ISL_HMAP_EL;

static void ISL_FN(ISL_HMAP_EL,free)(__isl_take ISL_HMAP_EL *pair)
{
	ISL_FN(ISL_KEY,free)(pair->key);
	ISL_FN(ISL_VAL,free)(pair->val);
	free(pair);
}

/* Add a mapping from pair->key to pair->val to the associative array
 * pointed to by user.
 */
static isl_bool add_entry(void **entry, void *user)
{
	ISL_HMAP_EL *pair = *entry;
	ISL_HMAP **hmap = (ISL_HMAP **) user;
	ISL_KEY *key;
	ISL_VAL *val;

	key = ISL_FN(ISL_KEY,copy)(pair->key);
	val = ISL_FN(ISL_VAL,copy)(pair->val);
	*hmap = ISL_FN(ISL_HMAP,set)(*hmap, key, val);

	if (!*hmap)
		return isl_bool_error;

	return isl_bool_true;
}

static isl_bool has_key(const void *entry, const void *c_key)
{
	const ISL_HMAP_EL *pair = entry;
	ISL_KEY *key = (ISL_KEY *) c_key;

	return ISL_KEY_IS_EQUAL(pair->key, key);
}

/* Does "pair" contain "val" as its value?
 */
static isl_bool has_val(ISL_HMAP_EL *pair, __isl_keep ISL_VAL *val)
{
	return ISL_VAL_IS_EQUAL(pair->val, val);
}

/* Free "key" and "val".
 */
static void free_key_val(__isl_take ISL_KEY *key, __isl_take ISL_VAL *val)
{
	ISL_FN(ISL_KEY,free)(key);
	ISL_FN(ISL_VAL,free)(val);
}

/* Replace the value of "pair" by "val".
 */
static void set_val(__isl_keep ISL_HMAP_EL *pair, __isl_take ISL_VAL *val)
{
	ISL_FN(ISL_VAL,free)(pair->val);
	pair->val = val;
}

/* Create a new entry from "key" and "val".
 */
static __isl_give ISL_HMAP_EL *create_entry(__isl_take ISL_KEY *key,
	__isl_take ISL_VAL *val)
{
	ISL_HMAP_EL *pair;

	pair = isl_alloc_type(ISL_FN(ISL_KEY,get_ctx)(key), ISL_HMAP_EL);
	if (!pair)
		goto error;

	pair->key = key;
	pair->val = val;
	return pair;
error:
	free_key_val(key, val);
	return NULL;
}

/* Optional "val" parameter declaration.
 */
#define OPT_VAL_PARAM		, __isl_take ISL_VAL *val
/* Optional "val" argument.
 */
#define OPT_VAL_ARG		, val

#ifdef ISL_HMAP_IS_EQUAL
#define ISL_HBASE_IS_EQUAL		ISL_HMAP_IS_EQUAL

/* Does "hmap" have an entry with key pair->key and value pair->val?
 */
static isl_bool has_entry(void **entry, void *user)
{
	ISL_HMAP_EL *pair = *entry;
	ISL_HMAP *hmap = user;
	ISL_MAYBE(ISL_VAL) maybe_val;
	isl_bool equal;

	maybe_val = ISL_FN(ISL_HMAP,try_get)(hmap, pair->key);
	if (maybe_val.valid < 0 || !maybe_val.valid)
		return maybe_val.valid;
	equal = ISL_VAL_IS_EQUAL(maybe_val.value, pair->val);
	ISL_FN(ISL_VAL,free)(maybe_val.value);
	return equal;
}

#endif

/* Print the given key-value pair to "p".
 */
static __isl_give isl_printer *print(__isl_take isl_printer *p,
	ISL_HMAP_EL *pair)
{
	p = ISL_KEY_PRINT(p, pair->key);
	p = isl_printer_print_str(p, ": ");
	p = ISL_VAL_PRINT(p, pair->val);

	return p;
}

#ifdef ISL_HMAP_HAVE_READ_FROM_STR
#define ISL_HBASE_HAVE_READ_FROM_STR

/* Read a key-value pair from "s", separated by a colon, and store it in "hmap".
 */
static __isl_give ISL_HMAP *read_entry(isl_stream *s, __isl_take ISL_HMAP *hmap)
{
	ISL_KEY *key;
	ISL_VAL *val = NULL;

	key = ISL_KEY_READ(s);
	if (isl_stream_eat(s, ':') >= 0)
		val = ISL_VAL_READ(s);
	return ISL_FN(ISL_HMAP,set)(hmap, key, val);
}

#endif

#include <isl/hbase_templ.c>

/* If "hmap" contains a value associated to "key", then return
 * (isl_bool_true, copy of value).
 * Otherwise, return
 * (isl_bool_false, NULL).
 * If an error occurs, then return
 * (isl_bool_error, NULL).
 */
__isl_give ISL_MAYBE(ISL_VAL) ISL_FN(ISL_HMAP,try_get)(
	__isl_keep ISL_HMAP *hmap, __isl_keep ISL_KEY *key)
{
	struct isl_hash_table_entry *entry;
	ISL_HMAP_EL *pair;
	uint32_t hash;
	ISL_MAYBE(ISL_VAL) res = { isl_bool_false, NULL };

	if (!hmap || !key)
		goto error;

	hash = ISL_FN(ISL_KEY,get_hash)(key);
	entry = isl_hash_table_find(hmap->ctx, &hmap->table, hash,
					&has_key, key, 0);

	if (!entry)
		goto error;
	if (entry == isl_hash_table_entry_none)
		return res;

	pair = entry->data;

	res.valid = isl_bool_true;
	res.value = ISL_FN(ISL_VAL,copy)(pair->val);
	if (!res.value)
		res.valid = isl_bool_error;
	return res;
error:
	res.valid = isl_bool_error;
	res.value = NULL;
	return res;
}

/* If "hmap" contains a value associated to "key", then return
 * isl_bool_true.  Otherwise, return isl_bool_false.
 * Return isl_bool_error on error.
 */
isl_bool ISL_FN(ISL_HMAP,has)(__isl_keep ISL_HMAP *hmap,
	__isl_keep ISL_KEY *key)
{
	ISL_MAYBE(ISL_VAL) res;

	res = ISL_FN(ISL_HMAP,try_get)(hmap, key);
	ISL_FN(ISL_VAL,free)(res.value);

	return res.valid;
}

/* If "hmap" contains a value associated to "key", then return
 * a copy of that value.  Otherwise, return NULL.
 * Return NULL on error.
 */
__isl_give ISL_VAL *ISL_FN(ISL_HMAP,get)(__isl_keep ISL_HMAP *hmap,
	__isl_take ISL_KEY *key)
{
	ISL_VAL *res;

	res = ISL_FN(ISL_HMAP,try_get)(hmap, key).value;
	ISL_FN(ISL_KEY,free)(key);
	return res;
}

/* Add a mapping from "key" to "val" to "hmap".
 */
__isl_give ISL_HMAP *ISL_FN(ISL_HMAP,set)(__isl_take ISL_HMAP *hmap,
	__isl_take ISL_KEY *key, __isl_take ISL_VAL *val)
{
	if (!val)
		key = ISL_FN(ISL_KEY,free)(key);
	return ISL_FN(ISL_HMAP,add)(hmap, key, val);
}

/* Internal data structure for isl_*_to_*_foreach.
 *
 * fn is the function that should be called on each entry.
 * user is the user-specified final argument to fn.
 */
ISL_S(foreach_data) {
	isl_stat (*fn)(__isl_take ISL_KEY *key, __isl_take ISL_VAL *val,
		void *user);
	void *user;
};

/* Call data->fn on a copy of the key and value in *entry.
 */
static isl_stat call_on_copy(void **entry, void *user)
{
	ISL_HMAP_EL *pair = *entry;
	ISL_S(foreach_data) *data = (ISL_S(foreach_data) *) user;

	return data->fn(ISL_FN(ISL_KEY,copy)(pair->key),
			ISL_FN(ISL_VAL,copy)(pair->val), data->user);
}

/* Call "fn" on each pair of key and value in "hmap".
 */
isl_stat ISL_FN(ISL_HMAP,foreach)(__isl_keep ISL_HMAP *hmap,
	isl_stat (*fn)(__isl_take ISL_KEY *key, __isl_take ISL_VAL *val,
		void *user),
	void *user)
{
	ISL_S(foreach_data) data = { fn, user };

	if (!hmap)
		return isl_stat_error;

	return isl_hash_table_foreach(hmap->ctx, &hmap->table,
				      &call_on_copy, &data);
}

/* Internal data structure for isl_*_to_*_every.
 *
 * "test" is the function that should be called on each entry,
 * until any invocation returns isl_bool_false.
 * "test_user" is the user-specified final argument to "test".
 */
ISL_S(every_data) {
	isl_bool (*test)(__isl_keep ISL_KEY *key, __isl_keep ISL_VAL *val,
		void *user);
	void *test_user;
};

/* Call data->test on the key and value in *entry.
 */
static isl_bool call_on_pair(void **entry, void *user)
{
	ISL_HMAP_EL *pair = *entry;
	ISL_S(every_data) *data = (ISL_S(every_data) *) user;

	return data->test(pair->key, pair->val, data->test_user);
}

/* Does "test" succeed on every entry of "hmap"?
 */
isl_bool ISL_FN(ISL_HMAP,every)(__isl_keep ISL_HMAP *hmap,
	isl_bool (*test)(__isl_keep ISL_KEY *key, __isl_keep ISL_VAL *val,
		void *user),
	void *user)
{
	ISL_S(every_data) data = { test, user };

	if (!hmap)
		return isl_bool_error;

	return isl_hash_table_every(hmap->ctx, &hmap->table,
				      &call_on_pair, &data);
}
