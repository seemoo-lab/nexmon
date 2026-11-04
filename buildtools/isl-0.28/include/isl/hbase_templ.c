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
#include <isl/printer.h>
#include <isl/stream.h>

#define ISL_xFN(TYPE,NAME) TYPE ## _ ## NAME
#define ISL_FN(TYPE,NAME) ISL_xFN(TYPE,NAME)

struct ISL_HBASE {
	int ref;
	isl_ctx *ctx;
	struct isl_hash_table table;
};

__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,alloc)(isl_ctx *ctx, int min_size)
{
	ISL_HBASE *hbase;

	hbase = isl_calloc_type(ctx, ISL_HBASE);
	if (!hbase)
		return NULL;

	hbase->ctx = ctx;
	isl_ctx_ref(ctx);
	hbase->ref = 1;

	if (isl_hash_table_init(ctx, &hbase->table, min_size) < 0)
		return ISL_FN(ISL_HBASE,free)(hbase);

	return hbase;
}

static isl_stat free_entry(void **entry, void *user)
{
	ISL_HBASE_EL *el = *entry;
	ISL_FN(ISL_HBASE_EL,free)(el);
	*entry = NULL;
	return isl_stat_ok;
}

__isl_null ISL_HBASE *ISL_FN(ISL_HBASE,free)(__isl_take ISL_HBASE *hbase)
{
	if (!hbase)
		return NULL;
	if (--hbase->ref > 0)
		return NULL;
	isl_hash_table_foreach(hbase->ctx, &hbase->table, &free_entry, NULL);
	isl_hash_table_clear(&hbase->table);
	isl_ctx_deref(hbase->ctx);
	free(hbase);
	return NULL;
}

isl_ctx *ISL_FN(ISL_HBASE,get_ctx)(__isl_keep ISL_HBASE *hbase)
{
	return hbase ? hbase->ctx : NULL;
}

/* Call "test" on every entry of "hbase".
 */
static isl_bool ISL_FN(ISL_HBASE,every_entry)(__isl_keep ISL_HBASE *hbase,
	isl_bool (*test)(void **entry, void *user), void *user)
{
	if (!hbase)
		return isl_bool_error;

	return isl_hash_table_every(hbase->ctx, &hbase->table, test, user);
}

__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,dup)(__isl_keep ISL_HBASE *hbase)
{
	ISL_HBASE *dup;

	if (!hbase)
		return NULL;

	dup = ISL_FN(ISL_HBASE,alloc)(hbase->ctx, hbase->table.n);
	if (ISL_FN(ISL_HBASE,every_entry)(hbase, &add_entry, &dup) < 0)
		return ISL_FN(ISL_HBASE,free)(dup);

	return dup;
}

__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,cow)(__isl_take ISL_HBASE *hbase)
{
	if (!hbase)
		return NULL;

	if (hbase->ref == 1)
		return hbase;
	hbase->ref--;
	return ISL_FN(ISL_HBASE,dup)(hbase);
}

__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,copy)(__isl_keep ISL_HBASE *hbase)
{
	if (!hbase)
		return NULL;

	hbase->ref++;
	return hbase;
}

/* Add an entry with "key" and "val" (if specified) to "hhase".
 * If "key" was already mapped to something else, then that mapping
 * is replaced.
 * If key happened to be mapped to "val" already, then we leave
 * "hmap" untouched.
 */
static __isl_give ISL_HBASE *ISL_FN(ISL_HBASE,add)(__isl_take ISL_HBASE *hbase,
	__isl_take ISL_KEY *key OPT_VAL_PARAM)
{
	struct isl_hash_table_entry *entry;
	uint32_t hash;

	if (!hbase || !key)
		goto error;

	hash = ISL_FN(ISL_KEY,get_hash)(key);
	entry = isl_hash_table_find(hbase->ctx, &hbase->table, hash,
					&has_key, key, 0);
	if (!entry)
		goto error;
	if (entry != isl_hash_table_entry_none) {
		isl_bool equal;
		equal = has_val(entry->data OPT_VAL_ARG);
		if (equal < 0)
			goto error;
		if (equal) {
			free_key_val(key OPT_VAL_ARG);
			return hbase;
		}
	}

	hbase = ISL_FN(ISL_HBASE,cow)(hbase);
	if (!hbase)
		goto error;

	entry = isl_hash_table_find(hbase->ctx, &hbase->table, hash,
					&has_key, key, 1);

	if (!entry)
		goto error;

	if (entry->data) {
		set_val(entry->data OPT_VAL_ARG);
		ISL_FN(ISL_KEY,free)(key);
		return hbase;
	}

	entry->data = create_entry(key OPT_VAL_ARG);
	if (!entry->data)
		return ISL_FN(ISL_HBASE,free)(hbase);

	return hbase;
error:
	free_key_val(key OPT_VAL_ARG);
	return ISL_FN(ISL_HBASE,free)(hbase);
}

/* Remove the mapping between "key" and its associated value (if any)
 * from "hbase".
 *
 * If "key" is not mapped to anything, then we leave "hbase" untouched.
 */
__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,drop)(__isl_take ISL_HBASE *hbase,
	__isl_take ISL_KEY *key)
{
	struct isl_hash_table_entry *entry;
	uint32_t hash;

	if (!hbase || !key)
		goto error;

	hash = ISL_FN(ISL_KEY,get_hash)(key);
	entry = isl_hash_table_find(hbase->ctx, &hbase->table, hash,
					&has_key, key, 0);
	if (!entry)
		goto error;
	if (entry == isl_hash_table_entry_none) {
		ISL_FN(ISL_KEY,free)(key);
		return hbase;
	}

	hbase = ISL_FN(ISL_HBASE,cow)(hbase);
	if (!hbase)
		goto error;
	entry = isl_hash_table_find(hbase->ctx, &hbase->table, hash,
					&has_key, key, 0);
	ISL_FN(ISL_KEY,free)(key);

	if (!entry)
		return ISL_FN(ISL_HBASE,free)(hbase);
	if (entry == isl_hash_table_entry_none)
		isl_die(hbase->ctx, isl_error_internal,
			"missing entry" , return ISL_FN(ISL_HBASE,free)(hbase));

	ISL_FN(ISL_HBASE_EL,free)(entry->data);
	isl_hash_table_remove(hbase->ctx, &hbase->table, entry);

	return hbase;
error:
	ISL_FN(ISL_KEY,free)(key);
	ISL_FN(ISL_HBASE,free)(hbase);
	return NULL;
}

#ifdef ISL_HBASE_IS_EQUAL

/* Is "hbase1" (obviously) equal to "hbase2"?
 *
 * In particular, do the two hash tables have
 * the same number of entries and does every entry of the first
 * also appear in the second?
 */
isl_bool ISL_HBASE_IS_EQUAL(__isl_keep ISL_HBASE *hbase1,
	__isl_keep ISL_HBASE *hbase2)
{
	if (!hbase1 || !hbase2)
		return isl_bool_error;
	if (hbase1 == hbase2)
		return isl_bool_true;
	if (hbase1->table.n != hbase2->table.n)
		return isl_bool_false;
	return ISL_FN(ISL_HBASE,every_entry)(hbase1, &has_entry, hbase2);
}

#endif

/* Internal data structure for print_pair.
 *
 * p is the printer on which the associative array is being printed.
 * first is set if the current key-value pair is the first to be printed.
 */
ISL_S(print_data) {
	isl_printer *p;
	int first;
};

/* Print the given key-value pair to data->p.
 */
static isl_bool print_entry(void **entry, void *user)
{
	ISL_S(print_data) *data = user;

	if (!data->first)
		data->p = isl_printer_print_str(data->p, ", ");
	data->p = print(data->p, *entry);
	data->first = 0;

	return isl_bool_true;
}

/* Print "hbase" to "p".
 */
__isl_give isl_printer *ISL_FN(isl_printer_print,ISL_HBASE_SUFFIX)(
	__isl_take isl_printer *p, __isl_keep ISL_HBASE *hbase)
{
	ISL_S(print_data) data;

	if (!p || !hbase)
		return isl_printer_free(p);

	p = isl_printer_print_str(p, "{");
	data.p = p;
	data.first = 1;
	if (ISL_FN(ISL_HBASE,every_entry)(hbase, &print_entry, &data) < 0)
		data.p = isl_printer_free(data.p);
	p = data.p;
	p = isl_printer_print_str(p, "}");

	return p;
}

void ISL_FN(ISL_HBASE,dump)(__isl_keep ISL_HBASE *hbase)
{
	isl_printer *printer;

	if (!hbase)
		return;

	printer = isl_printer_to_file(ISL_FN(ISL_HBASE,get_ctx)(hbase), stderr);
	printer = ISL_FN(isl_printer_print,ISL_HBASE_SUFFIX)(printer, hbase);
	printer = isl_printer_end_line(printer);

	isl_printer_free(printer);
}

/* Return a string representation of "hbase".
 */
__isl_give char *ISL_FN(ISL_HBASE,to_str)(__isl_keep ISL_HBASE *hbase)
{
	isl_printer *p;
	char *s;

	if (!hbase)
		return NULL;
	p = isl_printer_to_str(ISL_FN(ISL_HBASE,get_ctx)(hbase));
	p = ISL_FN(isl_printer_print,ISL_HBASE_SUFFIX)(p, hbase);
	s = isl_printer_get_str(p);
	isl_printer_free(p);

	return s;
}

#ifdef ISL_HBASE_HAVE_READ_FROM_STR

/* Read a hash table from "s".
 * The input format corresponds to the way associative arrays are printed
 * by isl_printer_print_*_to_*.
 * The entries are separated by a comma and
 * the entire hash table is surrounded by braces.
 */
__isl_give ISL_HBASE *ISL_FN(isl_stream_read,ISL_HBASE_SUFFIX)(isl_stream *s)
{
	isl_ctx *ctx;
	ISL_HBASE *hbase;

	if (!s)
		return NULL;
	ctx = isl_stream_get_ctx(s);
	hbase = ISL_FN(ISL_HBASE,alloc)(ctx, 0);
	if (!hbase)
		return NULL;
	if (isl_stream_eat(s, '{') < 0)
		return ISL_FN(ISL_HBASE,free)(hbase);
	if (isl_stream_eat_if_available(s, '}'))
		return hbase;
	do {
		hbase = read_entry(s, hbase);
		if (!hbase)
			return NULL;
	} while (isl_stream_eat_if_available(s, ','));
	if (isl_stream_eat(s, '}') < 0)
		return ISL_FN(ISL_HBASE,free)(hbase);
	return hbase;
}

/* Read a hash table from the string "str".
 * The input format corresponds to the way hash tables are printed
 * by isl_printer_print_*_to_*.
 * In particular, the entries are separated by a comma and
 * the entire hash table is surrounded by braces.
 */
__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,read_from_str)(isl_ctx *ctx,
	const char *str)
{
	ISL_HBASE *hbase;
	isl_stream *s;

	s = isl_stream_new_str(ctx, str);
	if (!s)
		return NULL;
	hbase = ISL_FN(isl_stream_read,ISL_HBASE_SUFFIX)(s);
	isl_stream_free(s);
	return hbase;
}

#endif
