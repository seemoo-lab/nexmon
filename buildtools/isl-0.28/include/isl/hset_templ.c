/*
 * Copyright 2026      Sven Verdoolaege
 *
 * Use of this software is governed by the MIT license
 */

#include <isl/ctx.h>
#include <isl/hash.h>
#include <isl/stream.h>

#define ISL_xFN(TYPE,NAME) TYPE ## _ ## NAME
#define ISL_FN(TYPE,NAME) ISL_xFN(TYPE,NAME)
#define ISL_S(NAME) struct ISL_FN(ISL_HSET,NAME)

#define ISL_KEY				ISL_HSET_EL

#define ISL_HBASE			ISL_HSET
#define ISL_HBASE_SUFFIX		ISL_HSET_SUFFIX
#define ISL_HBASE_EL			ISL_HSET_EL

/* Add "el" to "hset".
 */
static isl_bool add_entry(void **entry, void *user)
{
	ISL_HSET_EL *el = *entry;
	ISL_HSET **hset = (ISL_HSET **) user;

	*hset = ISL_FN(ISL_HSET,insert)(*hset, ISL_FN(ISL_HSET_EL,copy)(el));

	if (!*hset)
		return isl_bool_error;

	return isl_bool_true;
}

/* Does the hash table entry hold the given key?
 */
static isl_bool has_key(const void *entry, const void *c_key)
{
	const ISL_HSET_EL *el = entry;
	ISL_KEY *key = (ISL_KEY *) c_key;

	return ISL_HSET_EL_IS_EQUAL(el, key);
}

/* Does "el" contain the specified value?
 * Since no values are involved, this is always the case.
 */
static isl_bool has_val(ISL_HSET_EL *el)
{
	return isl_bool_true;
}

/* Free "key".
 */
static void free_key_val(__isl_take ISL_HSET_EL *key)
{
	ISL_FN(ISL_HSET_EL,free)(key);
}

/* Replace the value of "el" by the specified value.
 * Since no values are involved, nothing needs to be changed.
 */
static void set_val(__isl_keep ISL_HSET_EL *el)
{
}

/* Create a new entry from "el".
 * Since no values are involved, this is just "el" itself.
 */
static __isl_give ISL_HSET_EL *create_entry(__isl_take ISL_HSET_EL *el)
{
	return el;
}

/* Optional "val" parameter declaration.  No values are involved here.
 */
#define OPT_VAL_PARAM
/* Optional "val" argument.  No values are involved here.
 */
#define OPT_VAL_ARG

#ifdef ISL_HSET_IS_EQUAL
#define ISL_HBASE_IS_EQUAL		ISL_HSET_IS_EQUAL

/* Does "hset" have an entry "el"?
 */
static isl_bool has_entry(void **entry, void *user)
{
	ISL_HSET_EL *el = *entry;
	ISL_HSET *hset = user;

	return ISL_FN(ISL_HSET,has)(hset, el);
}

#endif

/* Print the given element to "p".
 */
static __isl_give isl_printer *print(__isl_take isl_printer *p, ISL_HSET_EL *el)
{
	p = ISL_HSET_EL_PRINT(p, el);

	return p;
}

#ifdef ISL_HSET_HAVE_READ_FROM_STR
#define ISL_HBASE_HAVE_READ_FROM_STR

/* Read an element from "s" and store it in "hset".
 */
static __isl_give ISL_HSET *read_entry(isl_stream *s, __isl_take ISL_HSET *hset)
{
	ISL_HSET_EL *el;

	el = ISL_HSET_EL_READ(s);
	return ISL_FN(ISL_HSET,insert)(hset, el);
}

#endif

#include <isl/hbase_templ.c>

/* If "hset" contains the element "el", then return
 * isl_bool_true.  Otherwise, return isl_bool_false.
 * Return isl_bool_error on error.
 */
isl_bool ISL_FN(ISL_HSET,has)(__isl_keep ISL_HSET *hset,
	__isl_keep ISL_HSET_EL *el)
{
	struct isl_hash_table_entry *entry;
	uint32_t hash;

	if (!hset || !el)
		return isl_bool_error;

	hash = ISL_FN(ISL_HSET_EL,get_hash)(el);
	entry = isl_hash_table_find(hset->ctx, &hset->table, hash,
					&has_key, el, 0);

	if (!entry)
		return isl_bool_error;

	return isl_bool_ok(entry != isl_hash_table_entry_none);
}

/* Add the element "el" to "hset".
 */
__isl_give ISL_HSET *ISL_FN(ISL_HSET,insert)(__isl_take ISL_HSET *hset,
	__isl_take ISL_HSET_EL *el)
{
	return ISL_FN(ISL_HSET,add)(hset, el);
}
