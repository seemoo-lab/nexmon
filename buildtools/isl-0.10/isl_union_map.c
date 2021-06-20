/*
 * Copyright 2010-2011 INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#define ISL_DIM_H
#include <isl_map_private.h>
#include <isl/ctx.h>
#include <isl/hash.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl_space_private.h>
#include <isl_union_map_private.h>
#include <isl/union_set.h>

/* Is this union set a parameter domain?
 */
int isl_union_set_is_params(__isl_keep isl_union_set *uset)
{
	isl_set *set;
	int params;

	if (!uset)
		return -1;
	if (uset->table.n != 1)
		return 0;

	set = isl_set_from_union_set(isl_union_set_copy(uset));
	params = isl_set_is_params(set);
	isl_set_free(set);
	return params;
}

static __isl_give isl_union_map *isl_union_map_alloc(__isl_take isl_space *dim,
	int size)
{
	isl_union_map *umap;

	if (!dim)
		return NULL;

	umap = isl_calloc_type(dim->ctx, isl_union_map);
	if (!umap)
		return NULL;

	umap->ref = 1;
	umap->dim = dim;
	if (isl_hash_table_init(dim->ctx, &umap->table, size) < 0)
		goto error;

	return umap;
error:
	isl_space_free(dim);
	isl_union_map_free(umap);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_empty(__isl_take isl_space *dim)
{
	return isl_union_map_alloc(dim, 16);
}

__isl_give isl_union_set *isl_union_set_empty(__isl_take isl_space *dim)
{
	return isl_union_map_empty(dim);
}

isl_ctx *isl_union_map_get_ctx(__isl_keep isl_union_map *umap)
{
	return umap ? umap->dim->ctx : NULL;
}

isl_ctx *isl_union_set_get_ctx(__isl_keep isl_union_set *uset)
{
	return uset ? uset->dim->ctx : NULL;
}

__isl_give isl_space *isl_union_map_get_space(__isl_keep isl_union_map *umap)
{
	if (!umap)
		return NULL;
	return isl_space_copy(umap->dim);
}

__isl_give isl_space *isl_union_set_get_space(__isl_keep isl_union_set *uset)
{
	return isl_union_map_get_space(uset);
}

static int free_umap_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_map_free(map);
	return 0;
}

static int add_map(__isl_take isl_map *map, void *user)
{
	isl_union_map **umap = (isl_union_map **)user;

	*umap = isl_union_map_add_map(*umap, map);

	return 0;
}

__isl_give isl_union_map *isl_union_map_dup(__isl_keep isl_union_map *umap)
{
	isl_union_map *dup;

	if (!umap)
		return NULL;

	dup = isl_union_map_empty(isl_space_copy(umap->dim));
	if (isl_union_map_foreach_map(umap, &add_map, &dup) < 0)
		goto error;
	return dup;
error:
	isl_union_map_free(dup);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_cow(__isl_take isl_union_map *umap)
{
	if (!umap)
		return NULL;

	if (umap->ref == 1)
		return umap;
	umap->ref--;
	return isl_union_map_dup(umap);
}

struct isl_union_align {
	isl_reordering *exp;
	isl_union_map *res;
};

static int align_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_reordering *exp;
	struct isl_union_align *data = user;

	exp = isl_reordering_extend_space(isl_reordering_copy(data->exp),
				    isl_map_get_space(map));

	data->res = isl_union_map_add_map(data->res,
					isl_map_realign(isl_map_copy(map), exp));

	return 0;
}

/* Align the parameters of umap along those of model.
 * The result has the parameters of model first, in the same order
 * as they appear in model, followed by any remaining parameters of
 * umap that do not appear in model.
 */
__isl_give isl_union_map *isl_union_map_align_params(
	__isl_take isl_union_map *umap, __isl_take isl_space *model)
{
	struct isl_union_align data = { NULL, NULL };

	if (!umap || !model)
		goto error;

	if (isl_space_match(umap->dim, isl_dim_param, model, isl_dim_param)) {
		isl_space_free(model);
		return umap;
	}

	model = isl_space_params(model);
	data.exp = isl_parameter_alignment_reordering(umap->dim, model);
	if (!data.exp)
		goto error;

	data.res = isl_union_map_alloc(isl_space_copy(data.exp->dim),
					umap->table.n);
	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
					&align_entry, &data) < 0)
		goto error;

	isl_reordering_free(data.exp);
	isl_union_map_free(umap);
	isl_space_free(model);
	return data.res;
error:
	isl_reordering_free(data.exp);
	isl_union_map_free(umap);
	isl_union_map_free(data.res);
	isl_space_free(model);
	return NULL;
}

__isl_give isl_union_set *isl_union_set_align_params(
	__isl_take isl_union_set *uset, __isl_take isl_space *model)
{
	return isl_union_map_align_params(uset, model);
}

__isl_give isl_union_map *isl_union_map_union(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2)
{
	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_space(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_space(umap1));

	umap1 = isl_union_map_cow(umap1);

	if (!umap1 || !umap2)
		goto error;

	if (isl_union_map_foreach_map(umap2, &add_map, &umap1) < 0)
		goto error;

	isl_union_map_free(umap2);

	return umap1;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return NULL;
}

__isl_give isl_union_set *isl_union_set_union(__isl_take isl_union_set *uset1,
	__isl_take isl_union_set *uset2)
{
	return isl_union_map_union(uset1, uset2);
}

__isl_give isl_union_map *isl_union_map_copy(__isl_keep isl_union_map *umap)
{
	if (!umap)
		return NULL;

	umap->ref++;
	return umap;
}

__isl_give isl_union_set *isl_union_set_copy(__isl_keep isl_union_set *uset)
{
	return isl_union_map_copy(uset);
}

void *isl_union_map_free(__isl_take isl_union_map *umap)
{
	if (!umap)
		return NULL;

	if (--umap->ref > 0)
		return NULL;

	isl_hash_table_foreach(umap->dim->ctx, &umap->table,
			       &free_umap_entry, NULL);
	isl_hash_table_clear(&umap->table);
	isl_space_free(umap->dim);
	free(umap);
	return NULL;
}

void *isl_union_set_free(__isl_take isl_union_set *uset)
{
	return isl_union_map_free(uset);
}

static int has_dim(const void *entry, const void *val)
{
	isl_map *map = (isl_map *)entry;
	isl_space *dim = (isl_space *)val;

	return isl_space_is_equal(map->dim, dim);
}

__isl_give isl_union_map *isl_union_map_add_map(__isl_take isl_union_map *umap,
	__isl_take isl_map *map)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (!map || !umap)
		goto error;

	if (isl_map_plain_is_empty(map)) {
		isl_map_free(map);
		return umap;
	}

	if (!isl_space_match(map->dim, isl_dim_param, umap->dim, isl_dim_param)) {
		umap = isl_union_map_align_params(umap, isl_map_get_space(map));
		map = isl_map_align_params(map, isl_union_map_get_space(umap));
	}

	umap = isl_union_map_cow(umap);

	if (!map || !umap)
		goto error;

	hash = isl_space_get_hash(map->dim);
	entry = isl_hash_table_find(umap->dim->ctx, &umap->table, hash,
				    &has_dim, map->dim, 1);
	if (!entry)
		goto error;

	if (!entry->data)
		entry->data = map;
	else {
		entry->data = isl_map_union(entry->data, isl_map_copy(map));
		if (!entry->data)
			goto error;
		isl_map_free(map);
	}

	return umap;
error:
	isl_map_free(map);
	isl_union_map_free(umap);
	return NULL;
}

__isl_give isl_union_set *isl_union_set_add_set(__isl_take isl_union_set *uset,
	__isl_take isl_set *set)
{
	return isl_union_map_add_map(uset, (isl_map *)set);
}

__isl_give isl_union_map *isl_union_map_from_map(__isl_take isl_map *map)
{
	isl_space *dim;
	isl_union_map *umap;

	if (!map)
		return NULL;

	dim = isl_map_get_space(map);
	dim = isl_space_params(dim);
	umap = isl_union_map_empty(dim);
	umap = isl_union_map_add_map(umap, map);

	return umap;
}

__isl_give isl_union_set *isl_union_set_from_set(__isl_take isl_set *set)
{
	return isl_union_map_from_map((isl_map *)set);
}

struct isl_union_map_foreach_data
{
	int (*fn)(__isl_take isl_map *map, void *user);
	void *user;
};

static int call_on_copy(void **entry, void *user)
{
	isl_map *map = *entry;
	struct isl_union_map_foreach_data *data;
	data = (struct isl_union_map_foreach_data *)user;

	return data->fn(isl_map_copy(map), data->user);
}

int isl_union_map_n_map(__isl_keep isl_union_map *umap)
{
	return umap ? umap->table.n : 0;
}

int isl_union_set_n_set(__isl_keep isl_union_set *uset)
{
	return uset ? uset->table.n : 0;
}

int isl_union_map_foreach_map(__isl_keep isl_union_map *umap,
	int (*fn)(__isl_take isl_map *map, void *user), void *user)
{
	struct isl_union_map_foreach_data data = { fn, user };

	if (!umap)
		return -1;

	return isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				      &call_on_copy, &data);
}

static int copy_map(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_map **map_p = user;

	*map_p = isl_map_copy(map);

	return -1;
}

__isl_give isl_map *isl_map_from_union_map(__isl_take isl_union_map *umap)
{
	isl_ctx *ctx;
	isl_map *map = NULL;

	if (!umap)
		return NULL;
	ctx = isl_union_map_get_ctx(umap);
	if (umap->table.n != 1)
		isl_die(ctx, isl_error_invalid,
			"union map needs to contain elements in exactly "
			"one space", return isl_union_map_free(umap));

	isl_hash_table_foreach(ctx, &umap->table, &copy_map, &map);

	isl_union_map_free(umap);

	return map;
}

__isl_give isl_set *isl_set_from_union_set(__isl_take isl_union_set *uset)
{
	return isl_map_from_union_map(uset);
}

__isl_give isl_map *isl_union_map_extract_map(__isl_keep isl_union_map *umap,
	__isl_take isl_space *dim)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (!umap || !dim)
		goto error;

	hash = isl_space_get_hash(dim);
	entry = isl_hash_table_find(umap->dim->ctx, &umap->table, hash,
				    &has_dim, dim, 0);
	if (!entry)
		return isl_map_empty(dim);
	isl_space_free(dim);
	return isl_map_copy(entry->data);
error:
	isl_space_free(dim);
	return NULL;
}

__isl_give isl_set *isl_union_set_extract_set(__isl_keep isl_union_set *uset,
	__isl_take isl_space *dim)
{
	return (isl_set *)isl_union_map_extract_map(uset, dim);
}

/* Check if umap contains a map in the given space.
 */
__isl_give int isl_union_map_contains(__isl_keep isl_union_map *umap,
	__isl_keep isl_space *dim)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (!umap || !dim)
		return -1;

	hash = isl_space_get_hash(dim);
	entry = isl_hash_table_find(umap->dim->ctx, &umap->table, hash,
				    &has_dim, dim, 0);
	return !!entry;
}

__isl_give int isl_union_set_contains(__isl_keep isl_union_set *uset,
	__isl_keep isl_space *dim)
{
	return isl_union_map_contains(uset, dim);
}

int isl_union_set_foreach_set(__isl_keep isl_union_set *uset,
	int (*fn)(__isl_take isl_set *set, void *user), void *user)
{
	return isl_union_map_foreach_map(uset,
		(int(*)(__isl_take isl_map *, void*))fn, user);
}

struct isl_union_set_foreach_point_data {
	int (*fn)(__isl_take isl_point *pnt, void *user);
	void *user;
};

static int foreach_point(__isl_take isl_set *set, void *user)
{
	struct isl_union_set_foreach_point_data *data = user;
	int r;

	r = isl_set_foreach_point(set, data->fn, data->user);
	isl_set_free(set);

	return r;
}

int isl_union_set_foreach_point(__isl_keep isl_union_set *uset,
	int (*fn)(__isl_take isl_point *pnt, void *user), void *user)
{
	struct isl_union_set_foreach_point_data data = { fn, user };
	return isl_union_set_foreach_set(uset, &foreach_point, &data);
}

struct isl_union_map_gen_bin_data {
	isl_union_map *umap2;
	isl_union_map *res;
};

static int subtract_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_map *map = *entry;

	hash = isl_space_get_hash(map->dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, map->dim, 0);
	map = isl_map_copy(map);
	if (entry2) {
		int empty;
		map = isl_map_subtract(map, isl_map_copy(entry2->data));

		empty = isl_map_is_empty(map);
		if (empty < 0) {
			isl_map_free(map);
			return -1;
		}
		if (empty) {
			isl_map_free(map);
			return 0;
		}
	}
	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

static __isl_give isl_union_map *gen_bin_op(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2, int (*fn)(void **, void *))
{
	struct isl_union_map_gen_bin_data data = { NULL, NULL };

	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_space(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_space(umap1));

	if (!umap1 || !umap2)
		goto error;

	data.umap2 = umap2;
	data.res = isl_union_map_alloc(isl_space_copy(umap1->dim),
				       umap1->table.n);
	if (isl_hash_table_foreach(umap1->dim->ctx, &umap1->table,
				   fn, &data) < 0)
		goto error;

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return data.res;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	isl_union_map_free(data.res);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_subtract(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return gen_bin_op(umap1, umap2, &subtract_entry);
}

__isl_give isl_union_set *isl_union_set_subtract(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return isl_union_map_subtract(uset1, uset2);
}

struct isl_union_map_gen_bin_set_data {
	isl_set *set;
	isl_union_map *res;
};

static int intersect_params_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_set_data *data = user;
	isl_map *map = *entry;
	int empty;

	map = isl_map_copy(map);
	map = isl_map_intersect_params(map, isl_set_copy(data->set));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

static __isl_give isl_union_map *gen_bin_set_op(__isl_take isl_union_map *umap,
	__isl_take isl_set *set, int (*fn)(void **, void *))
{
	struct isl_union_map_gen_bin_set_data data = { NULL, NULL };

	umap = isl_union_map_align_params(umap, isl_set_get_space(set));
	set = isl_set_align_params(set, isl_union_map_get_space(umap));

	if (!umap || !set)
		goto error;

	data.set = set;
	data.res = isl_union_map_alloc(isl_space_copy(umap->dim),
				       umap->table.n);
	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				   fn, &data) < 0)
		goto error;

	isl_union_map_free(umap);
	isl_set_free(set);
	return data.res;
error:
	isl_union_map_free(umap);
	isl_set_free(set);
	isl_union_map_free(data.res);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_intersect_params(
	__isl_take isl_union_map *umap, __isl_take isl_set *set)
{
	return gen_bin_set_op(umap, set, &intersect_params_entry);
}

__isl_give isl_union_set *isl_union_set_intersect_params(
	__isl_take isl_union_set *uset, __isl_take isl_set *set)
{
	return isl_union_map_intersect_params(uset, set);
}

static __isl_give isl_union_map *union_map_intersect_params(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset)
{
	return isl_union_map_intersect_params(umap,
						isl_set_from_union_set(uset));
}

static __isl_give isl_union_map *union_map_gist_params(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset)
{
	return isl_union_map_gist_params(umap, isl_set_from_union_set(uset));
}

struct isl_union_map_match_bin_data {
	isl_union_map *umap2;
	isl_union_map *res;
	__isl_give isl_map *(*fn)(__isl_take isl_map*, __isl_take isl_map*);
};

static int match_bin_entry(void **entry, void *user)
{
	struct isl_union_map_match_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_map *map = *entry;
	int empty;

	hash = isl_space_get_hash(map->dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, map->dim, 0);
	if (!entry2)
		return 0;

	map = isl_map_copy(map);
	map = data->fn(map, isl_map_copy(entry2->data));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}
	if (empty) {
		isl_map_free(map);
		return 0;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

static __isl_give isl_union_map *match_bin_op(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2,
	__isl_give isl_map *(*fn)(__isl_take isl_map*, __isl_take isl_map*))
{
	struct isl_union_map_match_bin_data data = { NULL, NULL, fn };

	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_space(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_space(umap1));

	if (!umap1 || !umap2)
		goto error;

	data.umap2 = umap2;
	data.res = isl_union_map_alloc(isl_space_copy(umap1->dim),
				       umap1->table.n);
	if (isl_hash_table_foreach(umap1->dim->ctx, &umap1->table,
				   &match_bin_entry, &data) < 0)
		goto error;

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return data.res;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	isl_union_map_free(data.res);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_intersect(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return match_bin_op(umap1, umap2, &isl_map_intersect);
}

/* Compute the intersection of the two union_sets.
 * As a special case, if exactly one of the two union_sets
 * is a parameter domain, then intersect the parameter domain
 * of the other one with this set.
 */
__isl_give isl_union_set *isl_union_set_intersect(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	int p1, p2;

	p1 = isl_union_set_is_params(uset1);
	p2 = isl_union_set_is_params(uset2);
	if (p1 < 0 || p2 < 0)
		goto error;
	if (!p1 && p2)
		return union_map_intersect_params(uset1, uset2);
	if (p1 && !p2)
		return union_map_intersect_params(uset2, uset1);
	return isl_union_map_intersect(uset1, uset2);
error:
	isl_union_set_free(uset1);
	isl_union_set_free(uset2);
	return NULL;
}

static int gist_params_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_set_data *data = user;
	isl_map *map = *entry;
	int empty;

	map = isl_map_copy(map);
	map = isl_map_gist_params(map, isl_set_copy(data->set));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

__isl_give isl_union_map *isl_union_map_gist_params(
	__isl_take isl_union_map *umap, __isl_take isl_set *set)
{
	return gen_bin_set_op(umap, set, &gist_params_entry);
}

__isl_give isl_union_set *isl_union_set_gist_params(
	__isl_take isl_union_set *uset, __isl_take isl_set *set)
{
	return isl_union_map_gist_params(uset, set);
}

__isl_give isl_union_map *isl_union_map_gist(__isl_take isl_union_map *umap,
	__isl_take isl_union_map *context)
{
	return match_bin_op(umap, context, &isl_map_gist);
}

__isl_give isl_union_set *isl_union_set_gist(__isl_take isl_union_set *uset,
	__isl_take isl_union_set *context)
{
	if (isl_union_set_is_params(context))
		return union_map_gist_params(uset, context);
	return isl_union_map_gist(uset, context);
}

static __isl_give isl_map *lex_le_set(__isl_take isl_map *set1,
	__isl_take isl_map *set2)
{
	return isl_set_lex_le_set((isl_set *)set1, (isl_set *)set2);
}

static __isl_give isl_map *lex_lt_set(__isl_take isl_map *set1,
	__isl_take isl_map *set2)
{
	return isl_set_lex_lt_set((isl_set *)set1, (isl_set *)set2);
}

__isl_give isl_union_map *isl_union_set_lex_lt_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return match_bin_op(uset1, uset2, &lex_lt_set);
}

__isl_give isl_union_map *isl_union_set_lex_le_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return match_bin_op(uset1, uset2, &lex_le_set);
}

__isl_give isl_union_map *isl_union_set_lex_gt_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return isl_union_map_reverse(isl_union_set_lex_lt_union_set(uset2, uset1));
}

__isl_give isl_union_map *isl_union_set_lex_ge_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2)
{
	return isl_union_map_reverse(isl_union_set_lex_le_union_set(uset2, uset1));
}

__isl_give isl_union_map *isl_union_map_lex_gt_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return isl_union_map_reverse(isl_union_map_lex_lt_union_map(umap2, umap1));
}

__isl_give isl_union_map *isl_union_map_lex_ge_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return isl_union_map_reverse(isl_union_map_lex_le_union_map(umap2, umap1));
}

static int intersect_domain_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_space *dim;
	isl_map *map = *entry;
	int empty;

	dim = isl_map_get_space(map);
	dim = isl_space_domain(dim);
	hash = isl_space_get_hash(dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, dim, 0);
	isl_space_free(dim);
	if (!entry2)
		return 0;

	map = isl_map_copy(map);
	map = isl_map_intersect_domain(map, isl_set_copy(entry2->data));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}
	if (empty) {
		isl_map_free(map);
		return 0;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

/* Intersect the domain of "umap" with "uset".
 * If "uset" is a parameters domain, then intersect the parameter
 * domain of "umap" with this set.
 */
__isl_give isl_union_map *isl_union_map_intersect_domain(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset)
{
	if (isl_union_set_is_params(uset))
		return union_map_intersect_params(umap, uset);
	return gen_bin_op(umap, uset, &intersect_domain_entry);
}

static int gist_domain_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_space *dim;
	isl_map *map = *entry;
	int empty;

	dim = isl_map_get_space(map);
	dim = isl_space_domain(dim);
	hash = isl_space_get_hash(dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, dim, 0);
	isl_space_free(dim);
	if (!entry2)
		return 0;

	map = isl_map_copy(map);
	map = isl_map_gist_domain(map, isl_set_copy(entry2->data));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

/* Compute the gist of "umap" with respect to the domain "uset".
 * If "uset" is a parameters domain, then compute the gist
 * with respect to this parameter domain.
 */
__isl_give isl_union_map *isl_union_map_gist_domain(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset)
{
	if (isl_union_set_is_params(uset))
		return union_map_gist_params(umap, uset);
	return gen_bin_op(umap, uset, &gist_domain_entry);
}

static int gist_range_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_space *space;
	isl_map *map = *entry;
	int empty;

	space = isl_map_get_space(map);
	space = isl_space_range(space);
	hash = isl_space_get_hash(space);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, space, 0);
	isl_space_free(space);
	if (!entry2)
		return 0;

	map = isl_map_copy(map);
	map = isl_map_gist_range(map, isl_set_copy(entry2->data));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

/* Compute the gist of "umap" with respect to the range "uset".
 */
__isl_give isl_union_map *isl_union_map_gist_range(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset)
{
	return gen_bin_op(umap, uset, &gist_range_entry);
}

static int intersect_range_entry(void **entry, void *user)
{
	struct isl_union_map_gen_bin_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_space *dim;
	isl_map *map = *entry;
	int empty;

	dim = isl_map_get_space(map);
	dim = isl_space_range(dim);
	hash = isl_space_get_hash(dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, dim, 0);
	isl_space_free(dim);
	if (!entry2)
		return 0;

	map = isl_map_copy(map);
	map = isl_map_intersect_range(map, isl_set_copy(entry2->data));

	empty = isl_map_is_empty(map);
	if (empty < 0) {
		isl_map_free(map);
		return -1;
	}
	if (empty) {
		isl_map_free(map);
		return 0;
	}

	data->res = isl_union_map_add_map(data->res, map);

	return 0;
}

__isl_give isl_union_map *isl_union_map_intersect_range(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset)
{
	return gen_bin_op(umap, uset, &intersect_range_entry);
}

struct isl_union_map_bin_data {
	isl_union_map *umap2;
	isl_union_map *res;
	isl_map *map;
	int (*fn)(void **entry, void *user);
};

static int apply_range_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;
	int empty;

	if (!isl_space_tuple_match(data->map->dim, isl_dim_out,
				 map2->dim, isl_dim_in))
		return 0;

	map2 = isl_map_apply_range(isl_map_copy(data->map), isl_map_copy(map2));

	empty = isl_map_is_empty(map2);
	if (empty < 0) {
		isl_map_free(map2);
		return -1;
	}
	if (empty) {
		isl_map_free(map2);
		return 0;
	}

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

static int bin_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map = *entry;

	data->map = map;
	if (isl_hash_table_foreach(data->umap2->dim->ctx, &data->umap2->table,
				   data->fn, data) < 0)
		return -1;

	return 0;
}

static __isl_give isl_union_map *bin_op(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2, int (*fn)(void **entry, void *user))
{
	struct isl_union_map_bin_data data = { NULL, NULL, NULL, fn };

	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_space(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_space(umap1));

	if (!umap1 || !umap2)
		goto error;

	data.umap2 = umap2;
	data.res = isl_union_map_alloc(isl_space_copy(umap1->dim),
				       umap1->table.n);
	if (isl_hash_table_foreach(umap1->dim->ctx, &umap1->table,
				   &bin_entry, &data) < 0)
		goto error;

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return data.res;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	isl_union_map_free(data.res);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_apply_range(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &apply_range_entry);
}

__isl_give isl_union_map *isl_union_map_apply_domain(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	umap1 = isl_union_map_reverse(umap1);
	umap1 = isl_union_map_apply_range(umap1, umap2);
	return isl_union_map_reverse(umap1);
}

__isl_give isl_union_set *isl_union_set_apply(
	__isl_take isl_union_set *uset, __isl_take isl_union_map *umap)
{
	return isl_union_map_apply_range(uset, umap);
}

static int map_lex_lt_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	if (!isl_space_tuple_match(data->map->dim, isl_dim_out,
				 map2->dim, isl_dim_out))
		return 0;

	map2 = isl_map_lex_lt_map(isl_map_copy(data->map), isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_lex_lt_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &map_lex_lt_entry);
}

static int map_lex_le_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	if (!isl_space_tuple_match(data->map->dim, isl_dim_out,
				 map2->dim, isl_dim_out))
		return 0;

	map2 = isl_map_lex_le_map(isl_map_copy(data->map), isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_lex_le_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &map_lex_le_entry);
}

static int product_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	map2 = isl_map_product(isl_map_copy(data->map), isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_product(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &product_entry);
}

static int set_product_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_set *set2 = *entry;

	set2 = isl_set_product(isl_set_copy(data->map), isl_set_copy(set2));

	data->res = isl_union_set_add_set(data->res, set2);

	return 0;
}

__isl_give isl_union_set *isl_union_set_product(__isl_take isl_union_set *uset1,
	__isl_take isl_union_set *uset2)
{
	return bin_op(uset1, uset2, &set_product_entry);
}

static int range_product_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	if (!isl_space_tuple_match(data->map->dim, isl_dim_in,
				 map2->dim, isl_dim_in))
		return 0;

	map2 = isl_map_range_product(isl_map_copy(data->map),
				     isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_range_product(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &range_product_entry);
}

static int flat_range_product_entry(void **entry, void *user)
{
	struct isl_union_map_bin_data *data = user;
	isl_map *map2 = *entry;

	if (!isl_space_tuple_match(data->map->dim, isl_dim_in,
				 map2->dim, isl_dim_in))
		return 0;

	map2 = isl_map_flat_range_product(isl_map_copy(data->map),
					  isl_map_copy(map2));

	data->res = isl_union_map_add_map(data->res, map2);

	return 0;
}

__isl_give isl_union_map *isl_union_map_flat_range_product(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2)
{
	return bin_op(umap1, umap2, &flat_range_product_entry);
}

static __isl_give isl_union_set *cond_un_op(__isl_take isl_union_map *umap,
	int (*fn)(void **, void *))
{
	isl_union_set *res;

	if (!umap)
		return NULL;

	res = isl_union_map_alloc(isl_space_copy(umap->dim), umap->table.n);
	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table, fn, &res) < 0)
		goto error;

	isl_union_map_free(umap);
	return res;
error:
	isl_union_map_free(umap);
	isl_union_set_free(res);
	return NULL;
}

static int from_range_entry(void **entry, void *user)
{
	isl_map *set = *entry;
	isl_union_set **res = user;

	*res = isl_union_map_add_map(*res,
					isl_map_from_range(isl_set_copy(set)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_from_range(
	__isl_take isl_union_set *uset)
{
	return cond_un_op(uset, &from_range_entry);
}

__isl_give isl_union_map *isl_union_map_from_domain(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_reverse(isl_union_map_from_range(uset));
}

__isl_give isl_union_map *isl_union_map_from_domain_and_range(
	__isl_take isl_union_set *domain, __isl_take isl_union_set *range)
{
	return isl_union_map_apply_range(isl_union_map_from_domain(domain),
				         isl_union_map_from_range(range));
}

static __isl_give isl_union_map *un_op(__isl_take isl_union_map *umap,
	int (*fn)(void **, void *))
{
	umap = isl_union_map_cow(umap);
	if (!umap)
		return NULL;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table, fn, NULL) < 0)
		goto error;

	return umap;
error:
	isl_union_map_free(umap);
	return NULL;
}

static int affine_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_from_basic_map(isl_map_affine_hull(*map));

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_affine_hull(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &affine_entry);
}

__isl_give isl_union_set *isl_union_set_affine_hull(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_affine_hull(uset);
}

static int polyhedral_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_from_basic_map(isl_map_polyhedral_hull(*map));

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_polyhedral_hull(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &polyhedral_entry);
}

__isl_give isl_union_set *isl_union_set_polyhedral_hull(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_polyhedral_hull(uset);
}

static int simple_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_from_basic_map(isl_map_simple_hull(*map));

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_simple_hull(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &simple_entry);
}

__isl_give isl_union_set *isl_union_set_simple_hull(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_simple_hull(uset);
}

static int inplace_entry(void **entry, void *user)
{
	__isl_give isl_map *(*fn)(__isl_take isl_map *);
	isl_map **map = (isl_map **)entry;
	isl_map *copy;

	fn = *(__isl_give isl_map *(**)(__isl_take isl_map *)) user;
	copy = fn(isl_map_copy(*map));
	if (!copy)
		return -1;

	isl_map_free(*map);
	*map = copy;

	return 0;
}

static __isl_give isl_union_map *inplace(__isl_take isl_union_map *umap,
	__isl_give isl_map *(*fn)(__isl_take isl_map *))
{
	if (!umap)
		return NULL;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				    &inplace_entry, &fn) < 0)
		goto error;

	return umap;
error:
	isl_union_map_free(umap);
	return NULL;
}

__isl_give isl_union_map *isl_union_map_coalesce(
	__isl_take isl_union_map *umap)
{
	return inplace(umap, &isl_map_coalesce);
}

__isl_give isl_union_set *isl_union_set_coalesce(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_coalesce(uset);
}

__isl_give isl_union_map *isl_union_map_detect_equalities(
	__isl_take isl_union_map *umap)
{
	return inplace(umap, &isl_map_detect_equalities);
}

__isl_give isl_union_set *isl_union_set_detect_equalities(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_detect_equalities(uset);
}

__isl_give isl_union_map *isl_union_map_compute_divs(
	__isl_take isl_union_map *umap)
{
	return inplace(umap, &isl_map_compute_divs);
}

__isl_give isl_union_set *isl_union_set_compute_divs(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_compute_divs(uset);
}

static int lexmin_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_lexmin(*map);

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_lexmin(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &lexmin_entry);
}

__isl_give isl_union_set *isl_union_set_lexmin(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_lexmin(uset);
}

static int lexmax_entry(void **entry, void *user)
{
	isl_map **map = (isl_map **)entry;

	*map = isl_map_lexmax(*map);

	return *map ? 0 : -1;
}

__isl_give isl_union_map *isl_union_map_lexmax(
	__isl_take isl_union_map *umap)
{
	return un_op(umap, &lexmax_entry);
}

__isl_give isl_union_set *isl_union_set_lexmax(
	__isl_take isl_union_set *uset)
{
	return isl_union_map_lexmax(uset);
}

static int universe_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_map **res = user;

	map = isl_map_universe(isl_map_get_space(map));
	*res = isl_union_map_add_map(*res, map);

	return 0;
}

__isl_give isl_union_map *isl_union_map_universe(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &universe_entry);
}

__isl_give isl_union_set *isl_union_set_universe(__isl_take isl_union_set *uset)
{
	return isl_union_map_universe(uset);
}

static int reverse_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_map **res = user;

	*res = isl_union_map_add_map(*res, isl_map_reverse(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_reverse(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &reverse_entry);
}

static int params_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_set_add_set(*res, isl_map_params(isl_map_copy(map)));

	return 0;
}

/* Compute the parameter domain of the given union map.
 */
__isl_give isl_set *isl_union_map_params(__isl_take isl_union_map *umap)
{
	int empty;

	empty = isl_union_map_is_empty(umap);
	if (empty < 0)
		return isl_union_map_free(umap);
	if (empty)
		return isl_set_empty(isl_union_map_get_space(umap));
	return isl_set_from_union_set(cond_un_op(umap, &params_entry));
}

/* Compute the parameter domain of the given union set.
 */
__isl_give isl_set *isl_union_set_params(__isl_take isl_union_set *uset)
{
	return isl_union_map_params(uset);
}

static int domain_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_set_add_set(*res, isl_map_domain(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_set *isl_union_map_domain(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &domain_entry);
}

static int range_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_set_add_set(*res, isl_map_range(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_set *isl_union_map_range(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &range_entry);
}

static int domain_map_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_map_add_map(*res,
					isl_map_domain_map(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_domain_map(
	__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &domain_map_entry);
}

static int range_map_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_map_add_map(*res,
					isl_map_range_map(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_range_map(
	__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &range_map_entry);
}

static int deltas_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	if (!isl_space_tuple_match(map->dim, isl_dim_in, map->dim, isl_dim_out))
		return 0;

	*res = isl_union_set_add_set(*res, isl_map_deltas(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_set *isl_union_map_deltas(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &deltas_entry);
}

static int deltas_map_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_map **res = user;

	if (!isl_space_tuple_match(map->dim, isl_dim_in, map->dim, isl_dim_out))
		return 0;

	*res = isl_union_map_add_map(*res,
				     isl_map_deltas_map(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_deltas_map(
	__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &deltas_map_entry);
}

static int identity_entry(void **entry, void *user)
{
	isl_set *set = *entry;
	isl_union_map **res = user;

	*res = isl_union_map_add_map(*res, isl_set_identity(isl_set_copy(set)));

	return 0;
}

__isl_give isl_union_map *isl_union_set_identity(__isl_take isl_union_set *uset)
{
	return cond_un_op(uset, &identity_entry);
}

static int unwrap_entry(void **entry, void *user)
{
	isl_set *set = *entry;
	isl_union_set **res = user;

	if (!isl_set_is_wrapping(set))
		return 0;

	*res = isl_union_map_add_map(*res, isl_set_unwrap(isl_set_copy(set)));

	return 0;
}

__isl_give isl_union_map *isl_union_set_unwrap(__isl_take isl_union_set *uset)
{
	return cond_un_op(uset, &unwrap_entry);
}

static int wrap_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_set **res = user;

	*res = isl_union_set_add_set(*res, isl_map_wrap(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_set *isl_union_map_wrap(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &wrap_entry);
}

struct isl_union_map_is_subset_data {
	isl_union_map *umap2;
	int is_subset;
};

static int is_subset_entry(void **entry, void *user)
{
	struct isl_union_map_is_subset_data *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_map *map = *entry;

	hash = isl_space_get_hash(map->dim);
	entry2 = isl_hash_table_find(data->umap2->dim->ctx, &data->umap2->table,
				     hash, &has_dim, map->dim, 0);
	if (!entry2) {
		int empty = isl_map_is_empty(map);
		if (empty < 0)
			return -1;
		if (empty)
			return 0;
		data->is_subset = 0;
		return -1;
	}

	data->is_subset = isl_map_is_subset(map, entry2->data);
	if (data->is_subset < 0 || !data->is_subset)
		return -1;

	return 0;
}

int isl_union_map_is_subset(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2)
{
	struct isl_union_map_is_subset_data data = { NULL, 1 };

	umap1 = isl_union_map_copy(umap1);
	umap2 = isl_union_map_copy(umap2);
	umap1 = isl_union_map_align_params(umap1, isl_union_map_get_space(umap2));
	umap2 = isl_union_map_align_params(umap2, isl_union_map_get_space(umap1));

	if (!umap1 || !umap2)
		goto error;

	data.umap2 = umap2;
	if (isl_hash_table_foreach(umap1->dim->ctx, &umap1->table,
				   &is_subset_entry, &data) < 0 &&
	    data.is_subset)
		goto error;

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);

	return data.is_subset;
error:
	isl_union_map_free(umap1);
	isl_union_map_free(umap2);
	return -1;
}

int isl_union_set_is_subset(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2)
{
	return isl_union_map_is_subset(uset1, uset2);
}

int isl_union_map_is_equal(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2)
{
	int is_subset;

	if (!umap1 || !umap2)
		return -1;
	is_subset = isl_union_map_is_subset(umap1, umap2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_union_map_is_subset(umap2, umap1);
	return is_subset;
}

int isl_union_set_is_equal(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2)
{
	return isl_union_map_is_equal(uset1, uset2);
}

int isl_union_map_is_strict_subset(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2)
{
	int is_subset;

	if (!umap1 || !umap2)
		return -1;
	is_subset = isl_union_map_is_subset(umap1, umap2);
	if (is_subset != 1)
		return is_subset;
	is_subset = isl_union_map_is_subset(umap2, umap1);
	if (is_subset == -1)
		return is_subset;
	return !is_subset;
}

int isl_union_set_is_strict_subset(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2)
{
	return isl_union_map_is_strict_subset(uset1, uset2);
}

static int sample_entry(void **entry, void *user)
{
	isl_basic_map **sample = (isl_basic_map **)user;
	isl_map *map = *entry;

	*sample = isl_map_sample(isl_map_copy(map));
	if (!*sample)
		return -1;
	if (!isl_basic_map_plain_is_empty(*sample))
		return -1;
	return 0;
}

__isl_give isl_basic_map *isl_union_map_sample(__isl_take isl_union_map *umap)
{
	isl_basic_map *sample = NULL;

	if (!umap)
		return NULL;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				   &sample_entry, &sample) < 0 &&
	    !sample)
		goto error;

	if (!sample)
		sample = isl_basic_map_empty(isl_union_map_get_space(umap));

	isl_union_map_free(umap);

	return sample;
error:
	isl_union_map_free(umap);
	return NULL;
}

__isl_give isl_basic_set *isl_union_set_sample(__isl_take isl_union_set *uset)
{
	return (isl_basic_set *)isl_union_map_sample(uset);
}

struct isl_forall_data {
	int res;
	int (*fn)(__isl_keep isl_map *map);
};

static int forall_entry(void **entry, void *user)
{
	struct isl_forall_data *data = user;
	isl_map *map = *entry;

	data->res = data->fn(map);
	if (data->res < 0)
		return -1;

	if (!data->res)
		return -1;

	return 0;
}

static int union_map_forall(__isl_keep isl_union_map *umap,
	int (*fn)(__isl_keep isl_map *map))
{
	struct isl_forall_data data = { 1, fn };

	if (!umap)
		return -1;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				   &forall_entry, &data) < 0 && data.res)
		return -1;

	return data.res;
}

struct isl_forall_user_data {
	int res;
	int (*fn)(__isl_keep isl_map *map, void *user);
	void *user;
};

static int forall_user_entry(void **entry, void *user)
{
	struct isl_forall_user_data *data = user;
	isl_map *map = *entry;

	data->res = data->fn(map, data->user);
	if (data->res < 0)
		return -1;

	if (!data->res)
		return -1;

	return 0;
}

/* Check if fn(map, user) returns true for all maps "map" in umap.
 */
static int union_map_forall_user(__isl_keep isl_union_map *umap,
	int (*fn)(__isl_keep isl_map *map, void *user), void *user)
{
	struct isl_forall_user_data data = { 1, fn, user };

	if (!umap)
		return -1;

	if (isl_hash_table_foreach(umap->dim->ctx, &umap->table,
				   &forall_user_entry, &data) < 0 && data.res)
		return -1;

	return data.res;
}

int isl_union_map_is_empty(__isl_keep isl_union_map *umap)
{
	return union_map_forall(umap, &isl_map_is_empty);
}

int isl_union_set_is_empty(__isl_keep isl_union_set *uset)
{
	return isl_union_map_is_empty(uset);
}

static int is_subset_of_identity(__isl_keep isl_map *map)
{
	int is_subset;
	isl_space *dim;
	isl_map *id;

	if (!map)
		return -1;

	if (!isl_space_tuple_match(map->dim, isl_dim_in, map->dim, isl_dim_out))
		return 0;

	dim = isl_map_get_space(map);
	id = isl_map_identity(dim);

	is_subset = isl_map_is_subset(map, id);

	isl_map_free(id);

	return is_subset;
}

/* Check if the given map is single-valued.
 * We simply compute
 *
 *	M \circ M^-1
 *
 * and check if the result is a subset of the identity mapping.
 */
int isl_union_map_is_single_valued(__isl_keep isl_union_map *umap)
{
	isl_union_map *test;
	int sv;

	if (isl_union_map_n_map(umap) == 1) {
		isl_map *map;
		umap = isl_union_map_copy(umap);
		map = isl_map_from_union_map(umap);
		sv = isl_map_is_single_valued(map);
		isl_map_free(map);
		return sv;
	}

	test = isl_union_map_reverse(isl_union_map_copy(umap));
	test = isl_union_map_apply_range(test, isl_union_map_copy(umap));

	sv = union_map_forall(test, &is_subset_of_identity);

	isl_union_map_free(test);

	return sv;
}

int isl_union_map_is_injective(__isl_keep isl_union_map *umap)
{
	int in;

	umap = isl_union_map_copy(umap);
	umap = isl_union_map_reverse(umap);
	in = isl_union_map_is_single_valued(umap);
	isl_union_map_free(umap);

	return in;
}

/* Represents a map that has a fixed value (v) for one of its
 * range dimensions.
 * The map in this structure is not reference counted, so it
 * is only valid while the isl_union_map from which it was
 * obtained is still alive.
 */
struct isl_fixed_map {
	isl_int v;
	isl_map *map;
};

static struct isl_fixed_map *alloc_isl_fixed_map_array(isl_ctx *ctx,
	int n)
{
	int i;
	struct isl_fixed_map *v;

	v = isl_calloc_array(ctx, struct isl_fixed_map, n);
	if (!v)
		return NULL;
	for (i = 0; i < n; ++i)
		isl_int_init(v[i].v);
	return v;
}

static void free_isl_fixed_map_array(struct isl_fixed_map *v, int n)
{
	int i;

	if (!v)
		return;
	for (i = 0; i < n; ++i)
		isl_int_clear(v[i].v);
	free(v);
}

/* Compare the "v" field of two isl_fixed_map structs.
 */
static int qsort_fixed_map_cmp(const void *p1, const void *p2)
{
	const struct isl_fixed_map *e1 = (const struct isl_fixed_map *) p1;
	const struct isl_fixed_map *e2 = (const struct isl_fixed_map *) p2;

	return isl_int_cmp(e1->v, e2->v);
}

/* Internal data structure used while checking whether all maps
 * in a union_map have a fixed value for a given output dimension.
 * v is the list of maps, with the fixed value for the dimension
 * n is the number of maps considered so far
 * pos is the output dimension under investigation
 */
struct isl_fixed_dim_data {
	struct isl_fixed_map *v;
	int n;
	int pos;
};

static int fixed_at_pos(__isl_keep isl_map *map, void *user)
{
	struct isl_fixed_dim_data *data = user;

	data->v[data->n].map = map;
	return isl_map_plain_is_fixed(map, isl_dim_out, data->pos,
				      &data->v[data->n++].v);
}

static int plain_injective_on_range(__isl_take isl_union_map *umap,
	int first, int n_range);

/* Given a list of the maps, with their fixed values at output dimension "pos",
 * check whether the ranges of the maps form an obvious partition.
 *
 * We first sort the maps according to their fixed values.
 * If all maps have a different value, then we know the ranges form
 * a partition.
 * Otherwise, we collect the maps with the same fixed value and
 * check whether each such collection is obviously injective
 * based on later dimensions.
 */
static int separates(struct isl_fixed_map *v, int n,
	__isl_take isl_space *dim, int pos, int n_range)
{
	int i;

	if (!v)
		goto error;

	qsort(v, n, sizeof(*v), &qsort_fixed_map_cmp);

	for (i = 0; i + 1 < n; ++i) {
		int j, k;
		isl_union_map *part;
		int injective;

		for (j = i + 1; j < n; ++j)
			if (isl_int_ne(v[i].v, v[j].v))
				break;

		if (j == i + 1)
			continue;

		part = isl_union_map_alloc(isl_space_copy(dim), j - i);
		for (k = i; k < j; ++k)
			part = isl_union_map_add_map(part,
						     isl_map_copy(v[k].map));

		injective = plain_injective_on_range(part, pos + 1, n_range);
		if (injective < 0)
			goto error;
		if (!injective)
			break;

		i = j - 1;
	}

	isl_space_free(dim);
	free_isl_fixed_map_array(v, n);
	return i + 1 >= n;
error:
	isl_space_free(dim);
	free_isl_fixed_map_array(v, n);
	return -1;
}

/* Check whether the maps in umap have obviously distinct ranges.
 * In particular, check for an output dimension in the range
 * [first,n_range) for which all maps have a fixed value
 * and then check if these values, possibly along with fixed values
 * at later dimensions, entail distinct ranges.
 */
static int plain_injective_on_range(__isl_take isl_union_map *umap,
	int first, int n_range)
{
	isl_ctx *ctx;
	int n;
	struct isl_fixed_dim_data data = { NULL };

	ctx = isl_union_map_get_ctx(umap);

	if (!umap)
		goto error;

	n = isl_union_map_n_map(umap);
	if (n <= 1) {
		isl_union_map_free(umap);
		return 1;
	}

	if (first >= n_range) {
		isl_union_map_free(umap);
		return 0;
	}

	data.v = alloc_isl_fixed_map_array(ctx, n);
	if (!data.v)
		goto error;

	for (data.pos = first; data.pos < n_range; ++data.pos) {
		int fixed;
		int injective;
		isl_space *dim;

		data.n = 0;
		fixed = union_map_forall_user(umap, &fixed_at_pos, &data);
		if (fixed < 0)
			goto error;
		if (!fixed)
			continue;
		dim = isl_union_map_get_space(umap);
		injective = separates(data.v, n, dim, data.pos, n_range);
		isl_union_map_free(umap);
		return injective;
	}

	free_isl_fixed_map_array(data.v, n);
	isl_union_map_free(umap);

	return 0;
error:
	free_isl_fixed_map_array(data.v, n);
	isl_union_map_free(umap);
	return -1;
}

/* Check whether the maps in umap that map to subsets of "ran"
 * have obviously distinct ranges.
 */
static int plain_injective_on_range_wrap(__isl_keep isl_set *ran, void *user)
{
	isl_union_map *umap = user;

	umap = isl_union_map_copy(umap);
	umap = isl_union_map_intersect_range(umap,
			isl_union_set_from_set(isl_set_copy(ran)));
	return plain_injective_on_range(umap, 0, isl_set_dim(ran, isl_dim_set));
}

/* Check if the given union_map is obviously injective.
 *
 * In particular, we first check if all individual maps are obviously
 * injective and then check if all the ranges of these maps are
 * obviously disjoint.
 */
int isl_union_map_plain_is_injective(__isl_keep isl_union_map *umap)
{
	int in;
	isl_union_map *univ;
	isl_union_set *ran;

	in = union_map_forall(umap, &isl_map_plain_is_injective);
	if (in < 0)
		return -1;
	if (!in)
		return 0;

	univ = isl_union_map_universe(isl_union_map_copy(umap));
	ran = isl_union_map_range(univ);

	in = union_map_forall_user(ran, &plain_injective_on_range_wrap, umap);

	isl_union_set_free(ran);

	return in;
}

int isl_union_map_is_bijective(__isl_keep isl_union_map *umap)
{
	int sv;

	sv = isl_union_map_is_single_valued(umap);
	if (sv < 0 || !sv)
		return sv;

	return isl_union_map_is_injective(umap);
}

static int zip_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_map **res = user;

	if (!isl_map_can_zip(map))
		return 0;

	*res = isl_union_map_add_map(*res, isl_map_zip(isl_map_copy(map)));

	return 0;
}

__isl_give isl_union_map *isl_union_map_zip(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &zip_entry);
}

static int curry_entry(void **entry, void *user)
{
	isl_map *map = *entry;
	isl_union_map **res = user;

	if (!isl_map_can_curry(map))
		return 0;

	*res = isl_union_map_add_map(*res, isl_map_curry(isl_map_copy(map)));

	return 0;
}

/* Given a union map, take the maps of the form (A -> B) -> C and
 * return the union of the corresponding maps A -> (B -> C).
 */
__isl_give isl_union_map *isl_union_map_curry(__isl_take isl_union_map *umap)
{
	return cond_un_op(umap, &curry_entry);
}

static int lift_entry(void **entry, void *user)
{
	isl_set *set = *entry;
	isl_union_set **res = user;

	*res = isl_union_set_add_set(*res, isl_set_lift(isl_set_copy(set)));

	return 0;
}

__isl_give isl_union_set *isl_union_set_lift(__isl_take isl_union_set *uset)
{
	return cond_un_op(uset, &lift_entry);
}

static int coefficients_entry(void **entry, void *user)
{
	isl_set *set = *entry;
	isl_union_set **res = user;

	set = isl_set_copy(set);
	set = isl_set_from_basic_set(isl_set_coefficients(set));
	*res = isl_union_set_add_set(*res, set);

	return 0;
}

__isl_give isl_union_set *isl_union_set_coefficients(
	__isl_take isl_union_set *uset)
{
	isl_ctx *ctx;
	isl_space *dim;
	isl_union_set *res;

	if (!uset)
		return NULL;

	ctx = isl_union_set_get_ctx(uset);
	dim = isl_space_set_alloc(ctx, 0, 0);
	res = isl_union_map_alloc(dim, uset->table.n);
	if (isl_hash_table_foreach(uset->dim->ctx, &uset->table,
				   &coefficients_entry, &res) < 0)
		goto error;

	isl_union_set_free(uset);
	return res;
error:
	isl_union_set_free(uset);
	isl_union_set_free(res);
	return NULL;
}

static int solutions_entry(void **entry, void *user)
{
	isl_set *set = *entry;
	isl_union_set **res = user;

	set = isl_set_copy(set);
	set = isl_set_from_basic_set(isl_set_solutions(set));
	if (!*res)
		*res = isl_union_set_from_set(set);
	else
		*res = isl_union_set_add_set(*res, set);

	if (!*res)
		return -1;

	return 0;
}

__isl_give isl_union_set *isl_union_set_solutions(
	__isl_take isl_union_set *uset)
{
	isl_union_set *res = NULL;

	if (!uset)
		return NULL;

	if (uset->table.n == 0) {
		res = isl_union_set_empty(isl_union_set_get_space(uset));
		isl_union_set_free(uset);
		return res;
	}

	if (isl_hash_table_foreach(uset->dim->ctx, &uset->table,
				   &solutions_entry, &res) < 0)
		goto error;

	isl_union_set_free(uset);
	return res;
error:
	isl_union_set_free(uset);
	isl_union_set_free(res);
	return NULL;
}
