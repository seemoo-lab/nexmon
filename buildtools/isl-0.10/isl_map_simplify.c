/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <strings.h>
#include <isl_ctx_private.h>
#include <isl_map_private.h>
#include "isl_equalities.h"
#include <isl/map.h>
#include <isl/seq.h>
#include "isl_tab.h"
#include <isl_space_private.h>
#include <isl_mat_private.h>

static void swap_equality(struct isl_basic_map *bmap, int a, int b)
{
	isl_int *t = bmap->eq[a];
	bmap->eq[a] = bmap->eq[b];
	bmap->eq[b] = t;
}

static void swap_inequality(struct isl_basic_map *bmap, int a, int b)
{
	if (a != b) {
		isl_int *t = bmap->ineq[a];
		bmap->ineq[a] = bmap->ineq[b];
		bmap->ineq[b] = t;
	}
}

static void constraint_drop_vars(isl_int *c, unsigned n, unsigned rem)
{
	isl_seq_cpy(c, c + n, rem);
	isl_seq_clr(c + rem, n);
}

/* Drop n dimensions starting at first.
 *
 * In principle, this frees up some extra variables as the number
 * of columns remains constant, but we would have to extend
 * the div array too as the number of rows in this array is assumed
 * to be equal to extra.
 */
struct isl_basic_set *isl_basic_set_drop_dims(
		struct isl_basic_set *bset, unsigned first, unsigned n)
{
	int i;

	if (!bset)
		goto error;

	isl_assert(bset->ctx, first + n <= bset->dim->n_out, goto error);

	if (n == 0 && !isl_space_get_tuple_name(bset->dim, isl_dim_set))
		return bset;

	bset = isl_basic_set_cow(bset);
	if (!bset)
		return NULL;

	for (i = 0; i < bset->n_eq; ++i)
		constraint_drop_vars(bset->eq[i]+1+bset->dim->nparam+first, n,
				     (bset->dim->n_out-first-n)+bset->extra);

	for (i = 0; i < bset->n_ineq; ++i)
		constraint_drop_vars(bset->ineq[i]+1+bset->dim->nparam+first, n,
				     (bset->dim->n_out-first-n)+bset->extra);

	for (i = 0; i < bset->n_div; ++i)
		constraint_drop_vars(bset->div[i]+1+1+bset->dim->nparam+first, n,
				     (bset->dim->n_out-first-n)+bset->extra);

	bset->dim = isl_space_drop_outputs(bset->dim, first, n);
	if (!bset->dim)
		goto error;

	ISL_F_CLR(bset, ISL_BASIC_SET_NORMALIZED);
	bset = isl_basic_set_simplify(bset);
	return isl_basic_set_finalize(bset);
error:
	isl_basic_set_free(bset);
	return NULL;
}

struct isl_set *isl_set_drop_dims(
		struct isl_set *set, unsigned first, unsigned n)
{
	int i;

	if (!set)
		goto error;

	isl_assert(set->ctx, first + n <= set->dim->n_out, goto error);

	if (n == 0 && !isl_space_get_tuple_name(set->dim, isl_dim_set))
		return set;
	set = isl_set_cow(set);
	if (!set)
		goto error;
	set->dim = isl_space_drop_outputs(set->dim, first, n);
	if (!set->dim)
		goto error;

	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_drop_dims(set->p[i], first, n);
		if (!set->p[i])
			goto error;
	}

	ISL_F_CLR(set, ISL_SET_NORMALIZED);
	return set;
error:
	isl_set_free(set);
	return NULL;
}

/* Move "n" divs starting at "first" to the end of the list of divs.
 */
static struct isl_basic_map *move_divs_last(struct isl_basic_map *bmap,
	unsigned first, unsigned n)
{
	isl_int **div;
	int i;

	if (first + n == bmap->n_div)
		return bmap;

	div = isl_alloc_array(bmap->ctx, isl_int *, n);
	if (!div)
		goto error;
	for (i = 0; i < n; ++i)
		div[i] = bmap->div[first + i];
	for (i = 0; i < bmap->n_div - first - n; ++i)
		bmap->div[first + i] = bmap->div[first + n + i];
	for (i = 0; i < n; ++i)
		bmap->div[bmap->n_div - n + i] = div[i];
	free(div);
	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

/* Drop "n" dimensions of type "type" starting at "first".
 *
 * In principle, this frees up some extra variables as the number
 * of columns remains constant, but we would have to extend
 * the div array too as the number of rows in this array is assumed
 * to be equal to extra.
 */
struct isl_basic_map *isl_basic_map_drop(struct isl_basic_map *bmap,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;
	unsigned dim;
	unsigned offset;
	unsigned left;

	if (!bmap)
		goto error;

	dim = isl_basic_map_dim(bmap, type);
	isl_assert(bmap->ctx, first + n <= dim, goto error);

	if (n == 0 && !isl_space_is_named_or_nested(bmap->dim, type))
		return bmap;

	bmap = isl_basic_map_cow(bmap);
	if (!bmap)
		return NULL;

	offset = isl_basic_map_offset(bmap, type) + first;
	left = isl_basic_map_total_dim(bmap) - (offset - 1) - n;
	for (i = 0; i < bmap->n_eq; ++i)
		constraint_drop_vars(bmap->eq[i]+offset, n, left);

	for (i = 0; i < bmap->n_ineq; ++i)
		constraint_drop_vars(bmap->ineq[i]+offset, n, left);

	for (i = 0; i < bmap->n_div; ++i)
		constraint_drop_vars(bmap->div[i]+1+offset, n, left);

	if (type == isl_dim_div) {
		bmap = move_divs_last(bmap, first, n);
		if (!bmap)
			goto error;
		isl_basic_map_free_div(bmap, n);
	} else
		bmap->dim = isl_space_drop_dims(bmap->dim, type, first, n);
	if (!bmap->dim)
		goto error;

	ISL_F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	bmap = isl_basic_map_simplify(bmap);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

__isl_give isl_basic_set *isl_basic_set_drop(__isl_take isl_basic_set *bset,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	return (isl_basic_set *)isl_basic_map_drop((isl_basic_map *)bset,
							type, first, n);
}

struct isl_basic_map *isl_basic_map_drop_inputs(
		struct isl_basic_map *bmap, unsigned first, unsigned n)
{
	return isl_basic_map_drop(bmap, isl_dim_in, first, n);
}

struct isl_map *isl_map_drop(struct isl_map *map,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;

	if (!map)
		goto error;

	isl_assert(map->ctx, first + n <= isl_map_dim(map, type), goto error);

	if (n == 0 && !isl_space_get_tuple_name(map->dim, type))
		return map;
	map = isl_map_cow(map);
	if (!map)
		goto error;
	map->dim = isl_space_drop_dims(map->dim, type, first, n);
	if (!map->dim)
		goto error;

	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_drop(map->p[i], type, first, n);
		if (!map->p[i])
			goto error;
	}
	ISL_F_CLR(map, ISL_MAP_NORMALIZED);

	return map;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_set *isl_set_drop(struct isl_set *set,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	return (isl_set *)isl_map_drop((isl_map *)set, type, first, n);
}

struct isl_map *isl_map_drop_inputs(
		struct isl_map *map, unsigned first, unsigned n)
{
	return isl_map_drop(map, isl_dim_in, first, n);
}

/*
 * We don't cow, as the div is assumed to be redundant.
 */
static struct isl_basic_map *isl_basic_map_drop_div(
		struct isl_basic_map *bmap, unsigned div)
{
	int i;
	unsigned pos;

	if (!bmap)
		goto error;

	pos = 1 + isl_space_dim(bmap->dim, isl_dim_all) + div;

	isl_assert(bmap->ctx, div < bmap->n_div, goto error);

	for (i = 0; i < bmap->n_eq; ++i)
		constraint_drop_vars(bmap->eq[i]+pos, 1, bmap->extra-div-1);

	for (i = 0; i < bmap->n_ineq; ++i) {
		if (!isl_int_is_zero(bmap->ineq[i][pos])) {
			isl_basic_map_drop_inequality(bmap, i);
			--i;
			continue;
		}
		constraint_drop_vars(bmap->ineq[i]+pos, 1, bmap->extra-div-1);
	}

	for (i = 0; i < bmap->n_div; ++i)
		constraint_drop_vars(bmap->div[i]+1+pos, 1, bmap->extra-div-1);

	if (div != bmap->n_div - 1) {
		int j;
		isl_int *t = bmap->div[div];

		for (j = div; j < bmap->n_div - 1; ++j)
			bmap->div[j] = bmap->div[j+1];

		bmap->div[bmap->n_div - 1] = t;
	}
	ISL_F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	isl_basic_map_free_div(bmap, 1);

	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_map *isl_basic_map_normalize_constraints(
	struct isl_basic_map *bmap)
{
	int i;
	isl_int gcd;
	unsigned total = isl_basic_map_total_dim(bmap);

	if (!bmap)
		return NULL;

	isl_int_init(gcd);
	for (i = bmap->n_eq - 1; i >= 0; --i) {
		isl_seq_gcd(bmap->eq[i]+1, total, &gcd);
		if (isl_int_is_zero(gcd)) {
			if (!isl_int_is_zero(bmap->eq[i][0])) {
				bmap = isl_basic_map_set_to_empty(bmap);
				break;
			}
			isl_basic_map_drop_equality(bmap, i);
			continue;
		}
		if (ISL_F_ISSET(bmap, ISL_BASIC_MAP_RATIONAL))
			isl_int_gcd(gcd, gcd, bmap->eq[i][0]);
		if (isl_int_is_one(gcd))
			continue;
		if (!isl_int_is_divisible_by(bmap->eq[i][0], gcd)) {
			bmap = isl_basic_map_set_to_empty(bmap);
			break;
		}
		isl_seq_scale_down(bmap->eq[i], bmap->eq[i], gcd, 1+total);
	}

	for (i = bmap->n_ineq - 1; i >= 0; --i) {
		isl_seq_gcd(bmap->ineq[i]+1, total, &gcd);
		if (isl_int_is_zero(gcd)) {
			if (isl_int_is_neg(bmap->ineq[i][0])) {
				bmap = isl_basic_map_set_to_empty(bmap);
				break;
			}
			isl_basic_map_drop_inequality(bmap, i);
			continue;
		}
		if (ISL_F_ISSET(bmap, ISL_BASIC_MAP_RATIONAL))
			isl_int_gcd(gcd, gcd, bmap->ineq[i][0]);
		if (isl_int_is_one(gcd))
			continue;
		isl_int_fdiv_q(bmap->ineq[i][0], bmap->ineq[i][0], gcd);
		isl_seq_scale_down(bmap->ineq[i]+1, bmap->ineq[i]+1, gcd, total);
	}
	isl_int_clear(gcd);

	return bmap;
}

struct isl_basic_set *isl_basic_set_normalize_constraints(
	struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)isl_basic_map_normalize_constraints(
		(struct isl_basic_map *)bset);
}

/* Assumes divs have been ordered if keep_divs is set.
 */
static void eliminate_var_using_equality(struct isl_basic_map *bmap,
	unsigned pos, isl_int *eq, int keep_divs, int *progress)
{
	unsigned total;
	unsigned space_total;
	int k;
	int last_div;

	total = isl_basic_map_total_dim(bmap);
	space_total = isl_space_dim(bmap->dim, isl_dim_all);
	last_div = isl_seq_last_non_zero(eq + 1 + space_total, bmap->n_div);
	for (k = 0; k < bmap->n_eq; ++k) {
		if (bmap->eq[k] == eq)
			continue;
		if (isl_int_is_zero(bmap->eq[k][1+pos]))
			continue;
		if (progress)
			*progress = 1;
		isl_seq_elim(bmap->eq[k], eq, 1+pos, 1+total, NULL);
		isl_seq_normalize(bmap->ctx, bmap->eq[k], 1 + total);
	}

	for (k = 0; k < bmap->n_ineq; ++k) {
		if (isl_int_is_zero(bmap->ineq[k][1+pos]))
			continue;
		if (progress)
			*progress = 1;
		isl_seq_elim(bmap->ineq[k], eq, 1+pos, 1+total, NULL);
		isl_seq_normalize(bmap->ctx, bmap->ineq[k], 1 + total);
		ISL_F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	}

	for (k = 0; k < bmap->n_div; ++k) {
		if (isl_int_is_zero(bmap->div[k][0]))
			continue;
		if (isl_int_is_zero(bmap->div[k][1+1+pos]))
			continue;
		if (progress)
			*progress = 1;
		/* We need to be careful about circular definitions,
		 * so for now we just remove the definition of div k
		 * if the equality contains any divs.
		 * If keep_divs is set, then the divs have been ordered
		 * and we can keep the definition as long as the result
		 * is still ordered.
		 */
		if (last_div == -1 || (keep_divs && last_div < k))
			isl_seq_elim(bmap->div[k]+1, eq,
					1+pos, 1+total, &bmap->div[k][0]);
		else
			isl_seq_clr(bmap->div[k], 1 + total);
		ISL_F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	}
}

/* Assumes divs have been ordered if keep_divs is set.
 */
static void eliminate_div(struct isl_basic_map *bmap, isl_int *eq,
	unsigned div, int keep_divs)
{
	unsigned pos = isl_space_dim(bmap->dim, isl_dim_all) + div;

	eliminate_var_using_equality(bmap, pos, eq, keep_divs, NULL);

	isl_basic_map_drop_div(bmap, div);
}

/* Check if elimination of div "div" using equality "eq" would not
 * result in a div depending on a later div.
 */
static int ok_to_eliminate_div(struct isl_basic_map *bmap, isl_int *eq,
	unsigned div)
{
	int k;
	int last_div;
	unsigned space_total = isl_space_dim(bmap->dim, isl_dim_all);
	unsigned pos = space_total + div;

	last_div = isl_seq_last_non_zero(eq + 1 + space_total, bmap->n_div);
	if (last_div < 0 || last_div <= div)
		return 1;

	for (k = 0; k <= last_div; ++k) {
		if (isl_int_is_zero(bmap->div[k][0]))
			return 1;
		if (!isl_int_is_zero(bmap->div[k][1 + 1 + pos]))
			return 0;
	}

	return 1;
}

/* Elimininate divs based on equalities
 */
static struct isl_basic_map *eliminate_divs_eq(
		struct isl_basic_map *bmap, int *progress)
{
	int d;
	int i;
	int modified = 0;
	unsigned off;

	bmap = isl_basic_map_order_divs(bmap);

	if (!bmap)
		return NULL;

	off = 1 + isl_space_dim(bmap->dim, isl_dim_all);

	for (d = bmap->n_div - 1; d >= 0 ; --d) {
		for (i = 0; i < bmap->n_eq; ++i) {
			if (!isl_int_is_one(bmap->eq[i][off + d]) &&
			    !isl_int_is_negone(bmap->eq[i][off + d]))
				continue;
			if (!ok_to_eliminate_div(bmap, bmap->eq[i], d))
				continue;
			modified = 1;
			*progress = 1;
			eliminate_div(bmap, bmap->eq[i], d, 1);
			isl_basic_map_drop_equality(bmap, i);
			break;
		}
	}
	if (modified)
		return eliminate_divs_eq(bmap, progress);
	return bmap;
}

/* Elimininate divs based on inequalities
 */
static struct isl_basic_map *eliminate_divs_ineq(
		struct isl_basic_map *bmap, int *progress)
{
	int d;
	int i;
	unsigned off;
	struct isl_ctx *ctx;

	if (!bmap)
		return NULL;

	ctx = bmap->ctx;
	off = 1 + isl_space_dim(bmap->dim, isl_dim_all);

	for (d = bmap->n_div - 1; d >= 0 ; --d) {
		for (i = 0; i < bmap->n_eq; ++i)
			if (!isl_int_is_zero(bmap->eq[i][off + d]))
				break;
		if (i < bmap->n_eq)
			continue;
		for (i = 0; i < bmap->n_ineq; ++i)
			if (isl_int_abs_gt(bmap->ineq[i][off + d], ctx->one))
				break;
		if (i < bmap->n_ineq)
			continue;
		*progress = 1;
		bmap = isl_basic_map_eliminate_vars(bmap, (off-1)+d, 1);
		if (!bmap || ISL_F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
			break;
		bmap = isl_basic_map_drop_div(bmap, d);
		if (!bmap)
			break;
	}
	return bmap;
}

struct isl_basic_map *isl_basic_map_gauss(
	struct isl_basic_map *bmap, int *progress)
{
	int k;
	int done;
	int last_var;
	unsigned total_var;
	unsigned total;

	bmap = isl_basic_map_order_divs(bmap);

	if (!bmap)
		return NULL;

	total = isl_basic_map_total_dim(bmap);
	total_var = total - bmap->n_div;

	last_var = total - 1;
	for (done = 0; done < bmap->n_eq; ++done) {
		for (; last_var >= 0; --last_var) {
			for (k = done; k < bmap->n_eq; ++k)
				if (!isl_int_is_zero(bmap->eq[k][1+last_var]))
					break;
			if (k < bmap->n_eq)
				break;
		}
		if (last_var < 0)
			break;
		if (k != done)
			swap_equality(bmap, k, done);
		if (isl_int_is_neg(bmap->eq[done][1+last_var]))
			isl_seq_neg(bmap->eq[done], bmap->eq[done], 1+total);

		eliminate_var_using_equality(bmap, last_var, bmap->eq[done], 1,
						progress);

		if (last_var >= total_var &&
		    isl_int_is_zero(bmap->div[last_var - total_var][0])) {
			unsigned div = last_var - total_var;
			isl_seq_neg(bmap->div[div]+1, bmap->eq[done], 1+total);
			isl_int_set_si(bmap->div[div][1+1+last_var], 0);
			isl_int_set(bmap->div[div][0],
				    bmap->eq[done][1+last_var]);
			ISL_F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
		}
	}
	if (done == bmap->n_eq)
		return bmap;
	for (k = done; k < bmap->n_eq; ++k) {
		if (isl_int_is_zero(bmap->eq[k][0]))
			continue;
		return isl_basic_map_set_to_empty(bmap);
	}
	isl_basic_map_free_equality(bmap, bmap->n_eq-done);
	return bmap;
}

struct isl_basic_set *isl_basic_set_gauss(
	struct isl_basic_set *bset, int *progress)
{
	return (struct isl_basic_set*)isl_basic_map_gauss(
			(struct isl_basic_map *)bset, progress);
}


static unsigned int round_up(unsigned int v)
{
	int old_v = v;

	while (v) {
		old_v = v;
		v ^= v & -v;
	}
	return old_v << 1;
}

static int hash_index(isl_int ***index, unsigned int size, int bits,
			struct isl_basic_map *bmap, int k)
{
	int h;
	unsigned total = isl_basic_map_total_dim(bmap);
	uint32_t hash = isl_seq_get_hash_bits(bmap->ineq[k]+1, total, bits);
	for (h = hash; index[h]; h = (h+1) % size)
		if (&bmap->ineq[k] != index[h] &&
		    isl_seq_eq(bmap->ineq[k]+1, index[h][0]+1, total))
			break;
	return h;
}

static int set_hash_index(isl_int ***index, unsigned int size, int bits,
			  struct isl_basic_set *bset, int k)
{
	return hash_index(index, size, bits, (struct isl_basic_map *)bset, k);
}

/* If we can eliminate more than one div, then we need to make
 * sure we do it from last div to first div, in order not to
 * change the position of the other divs that still need to
 * be removed.
 */
static struct isl_basic_map *remove_duplicate_divs(
	struct isl_basic_map *bmap, int *progress)
{
	unsigned int size;
	int *index;
	int *elim_for;
	int k, l, h;
	int bits;
	struct isl_blk eq;
	unsigned total_var;
	unsigned total;
	struct isl_ctx *ctx;

	if (!bmap || bmap->n_div <= 1)
		return bmap;

	total_var = isl_space_dim(bmap->dim, isl_dim_all);
	total = total_var + bmap->n_div;

	ctx = bmap->ctx;
	for (k = bmap->n_div - 1; k >= 0; --k)
		if (!isl_int_is_zero(bmap->div[k][0]))
			break;
	if (k <= 0)
		return bmap;

	elim_for = isl_calloc_array(ctx, int, bmap->n_div);
	size = round_up(4 * bmap->n_div / 3 - 1);
	bits = ffs(size) - 1;
	index = isl_calloc_array(ctx, int, size);
	if (!index)
		return bmap;
	eq = isl_blk_alloc(ctx, 1+total);
	if (isl_blk_is_error(eq))
		goto out;

	isl_seq_clr(eq.data, 1+total);
	index[isl_seq_get_hash_bits(bmap->div[k], 2+total, bits)] = k + 1;
	for (--k; k >= 0; --k) {
		uint32_t hash;

		if (isl_int_is_zero(bmap->div[k][0]))
			continue;

		hash = isl_seq_get_hash_bits(bmap->div[k], 2+total, bits);
		for (h = hash; index[h]; h = (h+1) % size)
			if (isl_seq_eq(bmap->div[k],
				       bmap->div[index[h]-1], 2+total))
				break;
		if (index[h]) {
			*progress = 1;
			l = index[h] - 1;
			elim_for[l] = k + 1;
		}
		index[h] = k+1;
	}
	for (l = bmap->n_div - 1; l >= 0; --l) {
		if (!elim_for[l])
			continue;
		k = elim_for[l] - 1;
		isl_int_set_si(eq.data[1+total_var+k], -1);
		isl_int_set_si(eq.data[1+total_var+l], 1);
		eliminate_div(bmap, eq.data, l, 0);
		isl_int_set_si(eq.data[1+total_var+k], 0);
		isl_int_set_si(eq.data[1+total_var+l], 0);
	}

	isl_blk_free(ctx, eq);
out:
	free(index);
	free(elim_for);
	return bmap;
}

static int n_pure_div_eq(struct isl_basic_map *bmap)
{
	int i, j;
	unsigned total;

	total = isl_space_dim(bmap->dim, isl_dim_all);
	for (i = 0, j = bmap->n_div-1; i < bmap->n_eq; ++i) {
		while (j >= 0 && isl_int_is_zero(bmap->eq[i][1 + total + j]))
			--j;
		if (j < 0)
			break;
		if (isl_seq_first_non_zero(bmap->eq[i] + 1 + total, j) != -1)
			return 0;
	}
	return i;
}

/* Normalize divs that appear in equalities.
 *
 * In particular, we assume that bmap contains some equalities
 * of the form
 *
 *	a x = m * e_i
 *
 * and we want to replace the set of e_i by a minimal set and
 * such that the new e_i have a canonical representation in terms
 * of the vector x.
 * If any of the equalities involves more than one divs, then
 * we currently simply bail out.
 *
 * Let us first additionally assume that all equalities involve
 * a div.  The equalities then express modulo constraints on the
 * remaining variables and we can use "parameter compression"
 * to find a minimal set of constraints.  The result is a transformation
 *
 *	x = T(x') = x_0 + G x'
 *
 * with G a lower-triangular matrix with all elements below the diagonal
 * non-negative and smaller than the diagonal element on the same row.
 * We first normalize x_0 by making the same property hold in the affine
 * T matrix.
 * The rows i of G with a 1 on the diagonal do not impose any modulo
 * constraint and simply express x_i = x'_i.
 * For each of the remaining rows i, we introduce a div and a corresponding
 * equality.  In particular
 *
 *	g_ii e_j = x_i - g_i(x')
 *
 * where each x'_k is replaced either by x_k (if g_kk = 1) or the
 * corresponding div (if g_kk != 1).
 *
 * If there are any equalities not involving any div, then we
 * first apply a variable compression on the variables x:
 *
 *	x = C x''	x'' = C_2 x
 *
 * and perform the above parameter compression on A C instead of on A.
 * The resulting compression is then of the form
 *
 *	x'' = T(x') = x_0 + G x'
 *
 * and in constructing the new divs and the corresponding equalities,
 * we have to replace each x'', i.e., the x'_k with (g_kk = 1),
 * by the corresponding row from C_2.
 */
static struct isl_basic_map *normalize_divs(
	struct isl_basic_map *bmap, int *progress)
{
	int i, j, k;
	int total;
	int div_eq;
	struct isl_mat *B;
	struct isl_vec *d;
	struct isl_mat *T = NULL;
	struct isl_mat *C = NULL;
	struct isl_mat *C2 = NULL;
	isl_int v;
	int *pos;
	int dropped, needed;

	if (!bmap)
		return NULL;

	if (bmap->n_div == 0)
		return bmap;

	if (bmap->n_eq == 0)
		return bmap;

	if (ISL_F_ISSET(bmap, ISL_BASIC_MAP_NORMALIZED_DIVS))
		return bmap;

	total = isl_space_dim(bmap->dim, isl_dim_all);
	div_eq = n_pure_div_eq(bmap);
	if (div_eq == 0)
		return bmap;

	if (div_eq < bmap->n_eq) {
		B = isl_mat_sub_alloc6(bmap->ctx, bmap->eq, div_eq,
					bmap->n_eq - div_eq, 0, 1 + total);
		C = isl_mat_variable_compression(B, &C2);
		if (!C || !C2)
			goto error;
		if (C->n_col == 0) {
			bmap = isl_basic_map_set_to_empty(bmap);
			isl_mat_free(C);
			isl_mat_free(C2);
			goto done;
		}
	}

	d = isl_vec_alloc(bmap->ctx, div_eq);
	if (!d)
		goto error;
	for (i = 0, j = bmap->n_div-1; i < div_eq; ++i) {
		while (j >= 0 && isl_int_is_zero(bmap->eq[i][1 + total + j]))
			--j;
		isl_int_set(d->block.data[i], bmap->eq[i][1 + total + j]);
	}
	B = isl_mat_sub_alloc6(bmap->ctx, bmap->eq, 0, div_eq, 0, 1 + total);

	if (C) {
		B = isl_mat_product(B, C);
		C = NULL;
	}

	T = isl_mat_parameter_compression(B, d);
	if (!T)
		goto error;
	if (T->n_col == 0) {
		bmap = isl_basic_map_set_to_empty(bmap);
		isl_mat_free(C2);
		isl_mat_free(T);
		goto done;
	}
	isl_int_init(v);
	for (i = 0; i < T->n_row - 1; ++i) {
		isl_int_fdiv_q(v, T->row[1 + i][0], T->row[1 + i][1 + i]);
		if (isl_int_is_zero(v))
			continue;
		isl_mat_col_submul(T, 0, v, 1 + i);
	}
	isl_int_clear(v);
	pos = isl_alloc_array(bmap->ctx, int, T->n_row);
	if (!pos)
		goto error;
	/* We have to be careful because dropping equalities may reorder them */
	dropped = 0;
	for (j = bmap->n_div - 1; j >= 0; --j) {
		for (i = 0; i < bmap->n_eq; ++i)
			if (!isl_int_is_zero(bmap->eq[i][1 + total + j]))
				break;
		if (i < bmap->n_eq) {
			bmap = isl_basic_map_drop_div(bmap, j);
			isl_basic_map_drop_equality(bmap, i);
			++dropped;
		}
	}
	pos[0] = 0;
	needed = 0;
	for (i = 1; i < T->n_row; ++i) {
		if (isl_int_is_one(T->row[i][i]))
			pos[i] = i;
		else
			needed++;
	}
	if (needed > dropped) {
		bmap = isl_basic_map_extend_space(bmap, isl_space_copy(bmap->dim),
				needed, needed, 0);
		if (!bmap)
			goto error;
	}
	for (i = 1; i < T->n_row; ++i) {
		if (isl_int_is_one(T->row[i][i]))
			continue;
		k = isl_basic_map_alloc_div(bmap);
		pos[i] = 1 + total + k;
		isl_seq_clr(bmap->div[k] + 1, 1 + total + bmap->n_div);
		isl_int_set(bmap->div[k][0], T->row[i][i]);
		if (C2)
			isl_seq_cpy(bmap->div[k] + 1, C2->row[i], 1 + total);
		else
			isl_int_set_si(bmap->div[k][1 + i], 1);
		for (j = 0; j < i; ++j) {
			if (isl_int_is_zero(T->row[i][j]))
				continue;
			if (pos[j] < T->n_row && C2)
				isl_seq_submul(bmap->div[k] + 1, T->row[i][j],
						C2->row[pos[j]], 1 + total);
			else
				isl_int_neg(bmap->div[k][1 + pos[j]],
								T->row[i][j]);
		}
		j = isl_basic_map_alloc_equality(bmap);
		isl_seq_neg(bmap->eq[j], bmap->div[k]+1, 1+total+bmap->n_div);
		isl_int_set(bmap->eq[j][pos[i]], bmap->div[k][0]);
	}
	free(pos);
	isl_mat_free(C2);
	isl_mat_free(T);

	if (progress)
		*progress = 1;
done:
	ISL_F_SET(bmap, ISL_BASIC_MAP_NORMALIZED_DIVS);

	return bmap;
error:
	isl_mat_free(C);
	isl_mat_free(C2);
	isl_mat_free(T);
	return bmap;
}

static struct isl_basic_map *set_div_from_lower_bound(
	struct isl_basic_map *bmap, int div, int ineq)
{
	unsigned total = 1 + isl_space_dim(bmap->dim, isl_dim_all);

	isl_seq_neg(bmap->div[div] + 1, bmap->ineq[ineq], total + bmap->n_div);
	isl_int_set(bmap->div[div][0], bmap->ineq[ineq][total + div]);
	isl_int_add(bmap->div[div][1], bmap->div[div][1], bmap->div[div][0]);
	isl_int_sub_ui(bmap->div[div][1], bmap->div[div][1], 1);
	isl_int_set_si(bmap->div[div][1 + total + div], 0);

	return bmap;
}

/* Check whether it is ok to define a div based on an inequality.
 * To avoid the introduction of circular definitions of divs, we
 * do not allow such a definition if the resulting expression would refer to
 * any other undefined divs or if any known div is defined in
 * terms of the unknown div.
 */
static int ok_to_set_div_from_bound(struct isl_basic_map *bmap,
	int div, int ineq)
{
	int j;
	unsigned total = 1 + isl_space_dim(bmap->dim, isl_dim_all);

	/* Not defined in terms of unknown divs */
	for (j = 0; j < bmap->n_div; ++j) {
		if (div == j)
			continue;
		if (isl_int_is_zero(bmap->ineq[ineq][total + j]))
			continue;
		if (isl_int_is_zero(bmap->div[j][0]))
			return 0;
	}

	/* No other div defined in terms of this one => avoid loops */
	for (j = 0; j < bmap->n_div; ++j) {
		if (div == j)
			continue;
		if (isl_int_is_zero(bmap->div[j][0]))
			continue;
		if (!isl_int_is_zero(bmap->div[j][1 + total + div]))
			return 0;
	}

	return 1;
}

/* Given two constraints "k" and "l" that are opposite to each other,
 * except for the constant term, check if we can use them
 * to obtain an expression for one of the hitherto unknown divs.
 * "sum" is the sum of the constant terms of the constraints.
 * If this sum is strictly smaller than the coefficient of one
 * of the divs, then this pair can be used define the div.
 * To avoid the introduction of circular definitions of divs, we
 * do not use the pair if the resulting expression would refer to
 * any other undefined divs or if any known div is defined in
 * terms of the unknown div.
 */
static struct isl_basic_map *check_for_div_constraints(
	struct isl_basic_map *bmap, int k, int l, isl_int sum, int *progress)
{
	int i;
	unsigned total = 1 + isl_space_dim(bmap->dim, isl_dim_all);

	for (i = 0; i < bmap->n_div; ++i) {
		if (!isl_int_is_zero(bmap->div[i][0]))
			continue;
		if (isl_int_is_zero(bmap->ineq[k][total + i]))
			continue;
		if (isl_int_abs_ge(sum, bmap->ineq[k][total + i]))
			continue;
		if (!ok_to_set_div_from_bound(bmap, i, k))
			break;
		if (isl_int_is_pos(bmap->ineq[k][total + i]))
			bmap = set_div_from_lower_bound(bmap, i, k);
		else
			bmap = set_div_from_lower_bound(bmap, i, l);
		if (progress)
			*progress = 1;
		break;
	}
	return bmap;
}

static struct isl_basic_map *remove_duplicate_constraints(
	struct isl_basic_map *bmap, int *progress, int detect_divs)
{
	unsigned int size;
	isl_int ***index;
	int k, l, h;
	int bits;
	unsigned total = isl_basic_map_total_dim(bmap);
	isl_int sum;
	isl_ctx *ctx;

	if (!bmap || bmap->n_ineq <= 1)
		return bmap;

	size = round_up(4 * (bmap->n_ineq+1) / 3 - 1);
	bits = ffs(size) - 1;
	ctx = isl_basic_map_get_ctx(bmap);
	index = isl_calloc_array(ctx, isl_int **, size);
	if (!index)
		return bmap;

	index[isl_seq_get_hash_bits(bmap->ineq[0]+1, total, bits)] = &bmap->ineq[0];
	for (k = 1; k < bmap->n_ineq; ++k) {
		h = hash_index(index, size, bits, bmap, k);
		if (!index[h]) {
			index[h] = &bmap->ineq[k];
			continue;
		}
		if (progress)
			*progress = 1;
		l = index[h] - &bmap->ineq[0];
		if (isl_int_lt(bmap->ineq[k][0], bmap->ineq[l][0]))
			swap_inequality(bmap, k, l);
		isl_basic_map_drop_inequality(bmap, k);
		--k;
	}
	isl_int_init(sum);
	for (k = 0; k < bmap->n_ineq-1; ++k) {
		isl_seq_neg(bmap->ineq[k]+1, bmap->ineq[k]+1, total);
		h = hash_index(index, size, bits, bmap, k);
		isl_seq_neg(bmap->ineq[k]+1, bmap->ineq[k]+1, total);
		if (!index[h])
			continue;
		l = index[h] - &bmap->ineq[0];
		isl_int_add(sum, bmap->ineq[k][0], bmap->ineq[l][0]);
		if (isl_int_is_pos(sum)) {
			if (detect_divs)
				bmap = check_for_div_constraints(bmap, k, l,
								 sum, progress);
			continue;
		}
		if (isl_int_is_zero(sum)) {
			/* We need to break out of the loop after these
			 * changes since the contents of the hash
			 * will no longer be valid.
			 * Plus, we probably we want to regauss first.
			 */
			if (progress)
				*progress = 1;
			isl_basic_map_drop_inequality(bmap, l);
			isl_basic_map_inequality_to_equality(bmap, k);
		} else
			bmap = isl_basic_map_set_to_empty(bmap);
		break;
	}
	isl_int_clear(sum);

	free(index);
	return bmap;
}


struct isl_basic_map *isl_basic_map_simplify(struct isl_basic_map *bmap)
{
	int progress = 1;
	if (!bmap)
		return NULL;
	while (progress) {
		progress = 0;
		bmap = isl_basic_map_normalize_constraints(bmap);
		bmap = remove_duplicate_divs(bmap, &progress);
		bmap = eliminate_divs_eq(bmap, &progress);
		bmap = eliminate_divs_ineq(bmap, &progress);
		bmap = isl_basic_map_gauss(bmap, &progress);
		/* requires equalities in normal form */
		bmap = normalize_divs(bmap, &progress);
		bmap = remove_duplicate_constraints(bmap, &progress, 1);
	}
	return bmap;
}

struct isl_basic_set *isl_basic_set_simplify(struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_simplify((struct isl_basic_map *)bset);
}


int isl_basic_map_is_div_constraint(__isl_keep isl_basic_map *bmap,
	isl_int *constraint, unsigned div)
{
	unsigned pos;

	if (!bmap)
		return -1;

	pos = 1 + isl_space_dim(bmap->dim, isl_dim_all) + div;

	if (isl_int_eq(constraint[pos], bmap->div[div][0])) {
		int neg;
		isl_int_sub(bmap->div[div][1],
				bmap->div[div][1], bmap->div[div][0]);
		isl_int_add_ui(bmap->div[div][1], bmap->div[div][1], 1);
		neg = isl_seq_is_neg(constraint, bmap->div[div]+1, pos);
		isl_int_sub_ui(bmap->div[div][1], bmap->div[div][1], 1);
		isl_int_add(bmap->div[div][1],
				bmap->div[div][1], bmap->div[div][0]);
		if (!neg)
			return 0;
		if (isl_seq_first_non_zero(constraint+pos+1,
					    bmap->n_div-div-1) != -1)
			return 0;
	} else if (isl_int_abs_eq(constraint[pos], bmap->div[div][0])) {
		if (!isl_seq_eq(constraint, bmap->div[div]+1, pos))
			return 0;
		if (isl_seq_first_non_zero(constraint+pos+1,
					    bmap->n_div-div-1) != -1)
			return 0;
	} else
		return 0;

	return 1;
}

int isl_basic_set_is_div_constraint(__isl_keep isl_basic_set *bset,
	isl_int *constraint, unsigned div)
{
	return isl_basic_map_is_div_constraint(bset, constraint, div);
}


/* If the only constraints a div d=floor(f/m)
 * appears in are its two defining constraints
 *
 *	f - m d >=0
 *	-(f - (m - 1)) + m d >= 0
 *
 * then it can safely be removed.
 */
static int div_is_redundant(struct isl_basic_map *bmap, int div)
{
	int i;
	unsigned pos = 1 + isl_space_dim(bmap->dim, isl_dim_all) + div;

	for (i = 0; i < bmap->n_eq; ++i)
		if (!isl_int_is_zero(bmap->eq[i][pos]))
			return 0;

	for (i = 0; i < bmap->n_ineq; ++i) {
		if (isl_int_is_zero(bmap->ineq[i][pos]))
			continue;
		if (!isl_basic_map_is_div_constraint(bmap, bmap->ineq[i], div))
			return 0;
	}

	for (i = 0; i < bmap->n_div; ++i)
		if (!isl_int_is_zero(bmap->div[i][1+pos]))
			return 0;

	return 1;
}

/*
 * Remove divs that don't occur in any of the constraints or other divs.
 * These can arise when dropping some of the variables in a quast
 * returned by piplib.
 */
static struct isl_basic_map *remove_redundant_divs(struct isl_basic_map *bmap)
{
	int i;

	if (!bmap)
		return NULL;

	for (i = bmap->n_div-1; i >= 0; --i) {
		if (!div_is_redundant(bmap, i))
			continue;
		bmap = isl_basic_map_drop_div(bmap, i);
	}
	return bmap;
}

struct isl_basic_map *isl_basic_map_finalize(struct isl_basic_map *bmap)
{
	bmap = remove_redundant_divs(bmap);
	if (!bmap)
		return NULL;
	ISL_F_SET(bmap, ISL_BASIC_SET_FINAL);
	return bmap;
}

struct isl_basic_set *isl_basic_set_finalize(struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_finalize((struct isl_basic_map *)bset);
}

struct isl_set *isl_set_finalize(struct isl_set *set)
{
	int i;

	if (!set)
		return NULL;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_finalize(set->p[i]);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(set);
	return NULL;
}

struct isl_map *isl_map_finalize(struct isl_map *map)
{
	int i;

	if (!map)
		return NULL;
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_finalize(map->p[i]);
		if (!map->p[i])
			goto error;
	}
	ISL_F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	isl_map_free(map);
	return NULL;
}


/* Remove definition of any div that is defined in terms of the given variable.
 * The div itself is not removed.  Functions such as
 * eliminate_divs_ineq depend on the other divs remaining in place.
 */
static struct isl_basic_map *remove_dependent_vars(struct isl_basic_map *bmap,
									int pos)
{
	int i;

	for (i = 0; i < bmap->n_div; ++i) {
		if (isl_int_is_zero(bmap->div[i][0]))
			continue;
		if (isl_int_is_zero(bmap->div[i][1+1+pos]))
			continue;
		isl_int_set_si(bmap->div[i][0], 0);
	}
	return bmap;
}

/* Eliminate the specified variables from the constraints using
 * Fourier-Motzkin.  The variables themselves are not removed.
 */
struct isl_basic_map *isl_basic_map_eliminate_vars(
	struct isl_basic_map *bmap, unsigned pos, unsigned n)
{
	int d;
	int i, j, k;
	unsigned total;
	int need_gauss = 0;

	if (n == 0)
		return bmap;
	if (!bmap)
		return NULL;
	total = isl_basic_map_total_dim(bmap);

	bmap = isl_basic_map_cow(bmap);
	for (d = pos + n - 1; d >= 0 && d >= pos; --d)
		bmap = remove_dependent_vars(bmap, d);

	for (d = pos + n - 1;
	     d >= 0 && d >= total - bmap->n_div && d >= pos; --d)
		isl_seq_clr(bmap->div[d-(total-bmap->n_div)], 2+total);
	for (d = pos + n - 1; d >= 0 && d >= pos; --d) {
		int n_lower, n_upper;
		if (!bmap)
			return NULL;
		for (i = 0; i < bmap->n_eq; ++i) {
			if (isl_int_is_zero(bmap->eq[i][1+d]))
				continue;
			eliminate_var_using_equality(bmap, d, bmap->eq[i], 0, NULL);
			isl_basic_map_drop_equality(bmap, i);
			need_gauss = 1;
			break;
		}
		if (i < bmap->n_eq)
			continue;
		n_lower = 0;
		n_upper = 0;
		for (i = 0; i < bmap->n_ineq; ++i) {
			if (isl_int_is_pos(bmap->ineq[i][1+d]))
				n_lower++;
			else if (isl_int_is_neg(bmap->ineq[i][1+d]))
				n_upper++;
		}
		bmap = isl_basic_map_extend_constraints(bmap,
				0, n_lower * n_upper);
		if (!bmap)
			goto error;
		for (i = bmap->n_ineq - 1; i >= 0; --i) {
			int last;
			if (isl_int_is_zero(bmap->ineq[i][1+d]))
				continue;
			last = -1;
			for (j = 0; j < i; ++j) {
				if (isl_int_is_zero(bmap->ineq[j][1+d]))
					continue;
				last = j;
				if (isl_int_sgn(bmap->ineq[i][1+d]) ==
				    isl_int_sgn(bmap->ineq[j][1+d]))
					continue;
				k = isl_basic_map_alloc_inequality(bmap);
				if (k < 0)
					goto error;
				isl_seq_cpy(bmap->ineq[k], bmap->ineq[i],
						1+total);
				isl_seq_elim(bmap->ineq[k], bmap->ineq[j],
						1+d, 1+total, NULL);
			}
			isl_basic_map_drop_inequality(bmap, i);
			i = last + 1;
		}
		if (n_lower > 0 && n_upper > 0) {
			bmap = isl_basic_map_normalize_constraints(bmap);
			bmap = remove_duplicate_constraints(bmap, NULL, 0);
			bmap = isl_basic_map_gauss(bmap, NULL);
			bmap = isl_basic_map_remove_redundancies(bmap);
			need_gauss = 0;
			if (!bmap)
				goto error;
			if (ISL_F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
				break;
		}
	}
	ISL_F_CLR(bmap, ISL_BASIC_MAP_NORMALIZED);
	if (need_gauss)
		bmap = isl_basic_map_gauss(bmap, NULL);
	return bmap;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_eliminate_vars(
	struct isl_basic_set *bset, unsigned pos, unsigned n)
{
	return (struct isl_basic_set *)isl_basic_map_eliminate_vars(
			(struct isl_basic_map *)bset, pos, n);
}

/* Eliminate the specified n dimensions starting at first from the
 * constraints using Fourier-Motzkin.  The dimensions themselves
 * are not removed.
 */
__isl_give isl_basic_map *isl_basic_map_eliminate(
	__isl_take isl_basic_map *bmap,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	if (!bmap)
		return NULL;
	if (n == 0)
		return bmap;

	if (first + n > isl_basic_map_dim(bmap, type))
		isl_die(bmap->ctx, isl_error_invalid,
			"index out of bounds", goto error);

	first += isl_basic_map_offset(bmap, type) - 1;
	bmap = isl_basic_map_eliminate_vars(bmap, first, n);
	return isl_basic_map_finalize(bmap);
error:
	isl_basic_map_free(bmap);
	return NULL;
}

/* Don't assume equalities are in order, because align_divs
 * may have changed the order of the divs.
 */
static void compute_elimination_index(struct isl_basic_map *bmap, int *elim)
{
	int d, i;
	unsigned total;

	total = isl_space_dim(bmap->dim, isl_dim_all);
	for (d = 0; d < total; ++d)
		elim[d] = -1;
	for (i = 0; i < bmap->n_eq; ++i) {
		for (d = total - 1; d >= 0; --d) {
			if (isl_int_is_zero(bmap->eq[i][1+d]))
				continue;
			elim[d] = i;
			break;
		}
	}
}

static void set_compute_elimination_index(struct isl_basic_set *bset, int *elim)
{
	compute_elimination_index((struct isl_basic_map *)bset, elim);
}

static int reduced_using_equalities(isl_int *dst, isl_int *src,
	struct isl_basic_map *bmap, int *elim)
{
	int d;
	int copied = 0;
	unsigned total;

	total = isl_space_dim(bmap->dim, isl_dim_all);
	for (d = total - 1; d >= 0; --d) {
		if (isl_int_is_zero(src[1+d]))
			continue;
		if (elim[d] == -1)
			continue;
		if (!copied) {
			isl_seq_cpy(dst, src, 1 + total);
			copied = 1;
		}
		isl_seq_elim(dst, bmap->eq[elim[d]], 1 + d, 1 + total, NULL);
	}
	return copied;
}

static int set_reduced_using_equalities(isl_int *dst, isl_int *src,
	struct isl_basic_set *bset, int *elim)
{
	return reduced_using_equalities(dst, src,
					(struct isl_basic_map *)bset, elim);
}

static struct isl_basic_set *isl_basic_set_reduce_using_equalities(
	struct isl_basic_set *bset, struct isl_basic_set *context)
{
	int i;
	int *elim;

	if (!bset || !context)
		goto error;

	if (context->n_eq == 0) {
		isl_basic_set_free(context);
		return bset;
	}

	bset = isl_basic_set_cow(bset);
	if (!bset)
		goto error;

	elim = isl_alloc_array(bset->ctx, int, isl_basic_set_n_dim(bset));
	if (!elim)
		goto error;
	set_compute_elimination_index(context, elim);
	for (i = 0; i < bset->n_eq; ++i)
		set_reduced_using_equalities(bset->eq[i], bset->eq[i],
							context, elim);
	for (i = 0; i < bset->n_ineq; ++i)
		set_reduced_using_equalities(bset->ineq[i], bset->ineq[i],
							context, elim);
	isl_basic_set_free(context);
	free(elim);
	bset = isl_basic_set_simplify(bset);
	bset = isl_basic_set_finalize(bset);
	return bset;
error:
	isl_basic_set_free(bset);
	isl_basic_set_free(context);
	return NULL;
}

static struct isl_basic_set *remove_shifted_constraints(
	struct isl_basic_set *bset, struct isl_basic_set *context)
{
	unsigned int size;
	isl_int ***index;
	int bits;
	int k, h, l;
	isl_ctx *ctx;

	if (!bset)
		return NULL;

	size = round_up(4 * (context->n_ineq+1) / 3 - 1);
	bits = ffs(size) - 1;
	ctx = isl_basic_set_get_ctx(bset);
	index = isl_calloc_array(ctx, isl_int **, size);
	if (!index)
		return bset;

	for (k = 0; k < context->n_ineq; ++k) {
		h = set_hash_index(index, size, bits, context, k);
		index[h] = &context->ineq[k];
	}
	for (k = 0; k < bset->n_ineq; ++k) {
		h = set_hash_index(index, size, bits, bset, k);
		if (!index[h])
			continue;
		l = index[h] - &context->ineq[0];
		if (isl_int_lt(bset->ineq[k][0], context->ineq[l][0]))
			continue;
		bset = isl_basic_set_cow(bset);
		if (!bset)
			goto error;
		isl_basic_set_drop_inequality(bset, k);
		--k;
	}
	free(index);
	return bset;
error:
	free(index);
	return bset;
}

/* Remove all information from bset that is redundant in the context
 * of context.  Both bset and context are assumed to be full-dimensional.
 *
 * We first * remove the inequalities from "bset"
 * that are obviously redundant with respect to some inequality in "context".
 *
 * If there are any inequalities left, we construct a tableau for
 * the context and then add the inequalities of "bset".
 * Before adding these inequalities, we freeze all constraints such that
 * they won't be considered redundant in terms of the constraints of "bset".
 * Then we detect all redundant constraints (among the
 * constraints that weren't frozen), first by checking for redundancy in the
 * the tableau and then by checking if replacing a constraint by its negation
 * would lead to an empty set.  This last step is fairly expensive
 * and could be optimized by more reuse of the tableau.
 * Finally, we update bset according to the results.
 */
static __isl_give isl_basic_set *uset_gist_full(__isl_take isl_basic_set *bset,
	__isl_take isl_basic_set *context)
{
	int i, k;
	isl_basic_set *combined = NULL;
	struct isl_tab *tab = NULL;
	unsigned context_ineq;
	unsigned total;

	if (!bset || !context)
		goto error;

	if (isl_basic_set_is_universe(bset)) {
		isl_basic_set_free(context);
		return bset;
	}

	if (isl_basic_set_is_universe(context)) {
		isl_basic_set_free(context);
		return bset;
	}

	bset = remove_shifted_constraints(bset, context);
	if (!bset)
		goto error;
	if (bset->n_ineq == 0)
		goto done;

	context_ineq = context->n_ineq;
	combined = isl_basic_set_cow(isl_basic_set_copy(context));
	combined = isl_basic_set_extend_constraints(combined, 0, bset->n_ineq);
	tab = isl_tab_from_basic_set(combined, 0);
	for (i = 0; i < context_ineq; ++i)
		if (isl_tab_freeze_constraint(tab, i) < 0)
			goto error;
	tab = isl_tab_extend(tab, bset->n_ineq);
	for (i = 0; i < bset->n_ineq; ++i)
		if (isl_tab_add_ineq(tab, bset->ineq[i]) < 0)
			goto error;
	bset = isl_basic_set_add_constraints(combined, bset, 0);
	combined = NULL;
	if (!bset)
		goto error;
	if (isl_tab_detect_redundant(tab) < 0)
		goto error;
	total = isl_basic_set_total_dim(bset);
	for (i = context_ineq; i < bset->n_ineq; ++i) {
		int is_empty;
		if (tab->con[i].is_redundant)
			continue;
		tab->con[i].is_redundant = 1;
		combined = isl_basic_set_dup(bset);
		combined = isl_basic_set_update_from_tab(combined, tab);
		combined = isl_basic_set_extend_constraints(combined, 0, 1);
		k = isl_basic_set_alloc_inequality(combined);
		if (k < 0)
			goto error;
		isl_seq_neg(combined->ineq[k], bset->ineq[i], 1 + total);
		isl_int_sub_ui(combined->ineq[k][0], combined->ineq[k][0], 1);
		is_empty = isl_basic_set_is_empty(combined);
		if (is_empty < 0)
			goto error;
		isl_basic_set_free(combined);
		combined = NULL;
		if (!is_empty)
			tab->con[i].is_redundant = 0;
	}
	for (i = 0; i < context_ineq; ++i)
		tab->con[i].is_redundant = 1;
	bset = isl_basic_set_update_from_tab(bset, tab);
	if (bset) {
		ISL_F_SET(bset, ISL_BASIC_SET_NO_IMPLICIT);
		ISL_F_SET(bset, ISL_BASIC_SET_NO_REDUNDANT);
	}

	isl_tab_free(tab);
done:
	bset = isl_basic_set_simplify(bset);
	bset = isl_basic_set_finalize(bset);
	isl_basic_set_free(context);
	return bset;
error:
	isl_tab_free(tab);
	isl_basic_set_free(combined);
	isl_basic_set_free(context);
	isl_basic_set_free(bset);
	return NULL;
}

/* Remove all information from bset that is redundant in the context
 * of context.  In particular, equalities that are linear combinations
 * of those in context are removed.  Then the inequalities that are
 * redundant in the context of the equalities and inequalities of
 * context are removed.
 *
 * We first compute the integer affine hull of the intersection,
 * compute the gist inside this affine hull and then add back
 * those equalities that are not implied by the context.
 *
 * If two constraints are mutually redundant, then uset_gist_full
 * will remove the second of those constraints.  We therefore first
 * sort the constraints so that constraints not involving existentially
 * quantified variables are given precedence over those that do.
 * We have to perform this sorting before the variable compression,
 * because that may effect the order of the variables.
 */
static __isl_give isl_basic_set *uset_gist(__isl_take isl_basic_set *bset,
	__isl_take isl_basic_set *context)
{
	isl_mat *eq;
	isl_mat *T, *T2;
	isl_basic_set *aff;
	isl_basic_set *aff_context;
	unsigned total;

	if (!bset || !context)
		goto error;

	bset = isl_basic_set_intersect(bset, isl_basic_set_copy(context));
	if (isl_basic_set_plain_is_empty(bset)) {
		isl_basic_set_free(context);
		return bset;
	}
	bset = isl_basic_set_sort_constraints(bset);
	aff = isl_basic_set_affine_hull(isl_basic_set_copy(bset));
	if (!aff)
		goto error;
	if (isl_basic_set_plain_is_empty(aff)) {
		isl_basic_set_free(aff);
		isl_basic_set_free(context);
		return bset;
	}
	if (aff->n_eq == 0) {
		isl_basic_set_free(aff);
		return uset_gist_full(bset, context);
	}
	total = isl_basic_set_total_dim(bset);
	eq = isl_mat_sub_alloc6(bset->ctx, aff->eq, 0, aff->n_eq, 0, 1 + total);
	eq = isl_mat_cow(eq);
	T = isl_mat_variable_compression(eq, &T2);
	if (T && T->n_col == 0) {
		isl_mat_free(T);
		isl_mat_free(T2);
		isl_basic_set_free(context);
		isl_basic_set_free(aff);
		return isl_basic_set_set_to_empty(bset);
	}

	aff_context = isl_basic_set_affine_hull(isl_basic_set_copy(context));

	bset = isl_basic_set_preimage(bset, isl_mat_copy(T));
	context = isl_basic_set_preimage(context, T);

	bset = uset_gist_full(bset, context);
	bset = isl_basic_set_preimage(bset, T2);
	bset = isl_basic_set_intersect(bset, aff);
	bset = isl_basic_set_reduce_using_equalities(bset, aff_context);

	if (bset) {
		ISL_F_SET(bset, ISL_BASIC_SET_NO_IMPLICIT);
		ISL_F_SET(bset, ISL_BASIC_SET_NO_REDUNDANT);
	}

	return bset;
error:
	isl_basic_set_free(bset);
	isl_basic_set_free(context);
	return NULL;
}

/* Normalize the divs in "bmap" in the context of the equalities in "context".
 * We simply add the equalities in context to bmap and then do a regular
 * div normalizations.  Better results can be obtained by normalizing
 * only the divs in bmap than do not also appear in context.
 * We need to be careful to reduce the divs using the equalities
 * so that later calls to isl_basic_map_overlying_set wouldn't introduce
 * spurious constraints.
 */
static struct isl_basic_map *normalize_divs_in_context(
	struct isl_basic_map *bmap, struct isl_basic_map *context)
{
	int i;
	unsigned total_context;
	int div_eq;

	div_eq = n_pure_div_eq(bmap);
	if (div_eq == 0)
		return bmap;

	if (context->n_div > 0)
		bmap = isl_basic_map_align_divs(bmap, context);

	total_context = isl_basic_map_total_dim(context);
	bmap = isl_basic_map_extend_constraints(bmap, context->n_eq, 0);
	for (i = 0; i < context->n_eq; ++i) {
		int k;
		k = isl_basic_map_alloc_equality(bmap);
		isl_seq_cpy(bmap->eq[k], context->eq[i], 1 + total_context);
		isl_seq_clr(bmap->eq[k] + 1 + total_context,
				isl_basic_map_total_dim(bmap) - total_context);
	}
	bmap = isl_basic_map_gauss(bmap, NULL);
	bmap = normalize_divs(bmap, NULL);
	bmap = isl_basic_map_gauss(bmap, NULL);
	return bmap;
}

struct isl_basic_map *isl_basic_map_gist(struct isl_basic_map *bmap,
	struct isl_basic_map *context)
{
	struct isl_basic_set *bset;

	if (!bmap || !context)
		goto error;

	if (isl_basic_map_is_universe(bmap)) {
		isl_basic_map_free(context);
		return bmap;
	}
	if (isl_basic_map_plain_is_empty(context)) {
		isl_basic_map_free(bmap);
		return context;
	}
	if (isl_basic_map_plain_is_empty(bmap)) {
		isl_basic_map_free(context);
		return bmap;
	}

	bmap = isl_basic_map_remove_redundancies(bmap);
	context = isl_basic_map_remove_redundancies(context);

	if (context->n_eq)
		bmap = normalize_divs_in_context(bmap, context);

	context = isl_basic_map_align_divs(context, bmap);
	bmap = isl_basic_map_align_divs(bmap, context);

	bset = uset_gist(isl_basic_map_underlying_set(isl_basic_map_copy(bmap)),
			 isl_basic_map_underlying_set(context));

	return isl_basic_map_overlying_set(bset, bmap);
error:
	isl_basic_map_free(bmap);
	isl_basic_map_free(context);
	return NULL;
}

/*
 * Assumes context has no implicit divs.
 */
__isl_give isl_map *isl_map_gist_basic_map(__isl_take isl_map *map,
	__isl_take isl_basic_map *context)
{
	int i;

	if (!map || !context)
		goto error;;

	if (isl_basic_map_plain_is_empty(context)) {
		isl_map_free(map);
		return isl_map_from_basic_map(context);
	}

	context = isl_basic_map_remove_redundancies(context);
	map = isl_map_cow(map);
	if (!map || !context)
		goto error;;
	isl_assert(map->ctx, isl_space_is_equal(map->dim, context->dim), goto error);
	map = isl_map_compute_divs(map);
	for (i = 0; i < map->n; ++i)
		context = isl_basic_map_align_divs(context, map->p[i]);
	for (i = map->n - 1; i >= 0; --i) {
		map->p[i] = isl_basic_map_gist(map->p[i],
						isl_basic_map_copy(context));
		if (!map->p[i])
			goto error;
		if (isl_basic_map_plain_is_empty(map->p[i])) {
			isl_basic_map_free(map->p[i]);
			if (i != map->n - 1)
				map->p[i] = map->p[map->n - 1];
			map->n--;
		}
	}
	isl_basic_map_free(context);
	ISL_F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	isl_map_free(map);
	isl_basic_map_free(context);
	return NULL;
}

static __isl_give isl_map *map_gist(__isl_take isl_map *map,
	__isl_take isl_map *context)
{
	context = isl_map_compute_divs(context);
	return isl_map_gist_basic_map(map, isl_map_simple_hull(context));
}

__isl_give isl_map *isl_map_gist(__isl_take isl_map *map,
	__isl_take isl_map *context)
{
	return isl_map_align_params_map_map_and(map, context, &map_gist);
}

struct isl_basic_set *isl_basic_set_gist(struct isl_basic_set *bset,
						struct isl_basic_set *context)
{
	return (struct isl_basic_set *)isl_basic_map_gist(
		(struct isl_basic_map *)bset, (struct isl_basic_map *)context);
}

__isl_give isl_set *isl_set_gist_basic_set(__isl_take isl_set *set,
	__isl_take isl_basic_set *context)
{
	return (struct isl_set *)isl_map_gist_basic_map((struct isl_map *)set,
					(struct isl_basic_map *)context);
}

__isl_give isl_set *isl_set_gist_params_basic_set(__isl_take isl_set *set,
	__isl_take isl_basic_set *context)
{
	isl_space *space = isl_set_get_space(set);
	isl_basic_set *dom_context = isl_basic_set_universe(space);
	dom_context = isl_basic_set_intersect_params(dom_context, context);
	return isl_set_gist_basic_set(set, dom_context);
}

__isl_give isl_set *isl_set_gist(__isl_take isl_set *set,
	__isl_take isl_set *context)
{
	return (struct isl_set *)isl_map_gist((struct isl_map *)set,
					(struct isl_map *)context);
}

__isl_give isl_map *isl_map_gist_domain(__isl_take isl_map *map,
	__isl_take isl_set *context)
{
	isl_map *map_context = isl_map_universe(isl_map_get_space(map));
	map_context = isl_map_intersect_domain(map_context, context);
	return isl_map_gist(map, map_context);
}

__isl_give isl_map *isl_map_gist_range(__isl_take isl_map *map,
	__isl_take isl_set *context)
{
	isl_map *map_context = isl_map_universe(isl_map_get_space(map));
	map_context = isl_map_intersect_range(map_context, context);
	return isl_map_gist(map, map_context);
}

__isl_give isl_map *isl_map_gist_params(__isl_take isl_map *map,
	__isl_take isl_set *context)
{
	isl_map *map_context = isl_map_universe(isl_map_get_space(map));
	map_context = isl_map_intersect_params(map_context, context);
	return isl_map_gist(map, map_context);
}

__isl_give isl_set *isl_set_gist_params(__isl_take isl_set *set,
	__isl_take isl_set *context)
{
	return isl_map_gist_params(set, context);
}

/* Quick check to see if two basic maps are disjoint.
 * In particular, we reduce the equalities and inequalities of
 * one basic map in the context of the equalities of the other
 * basic map and check if we get a contradiction.
 */
int isl_basic_map_plain_is_disjoint(__isl_keep isl_basic_map *bmap1,
	__isl_keep isl_basic_map *bmap2)
{
	struct isl_vec *v = NULL;
	int *elim = NULL;
	unsigned total;
	int i;

	if (!bmap1 || !bmap2)
		return -1;
	isl_assert(bmap1->ctx, isl_space_is_equal(bmap1->dim, bmap2->dim),
			return -1);
	if (bmap1->n_div || bmap2->n_div)
		return 0;
	if (!bmap1->n_eq && !bmap2->n_eq)
		return 0;

	total = isl_space_dim(bmap1->dim, isl_dim_all);
	if (total == 0)
		return 0;
	v = isl_vec_alloc(bmap1->ctx, 1 + total);
	if (!v)
		goto error;
	elim = isl_alloc_array(bmap1->ctx, int, total);
	if (!elim)
		goto error;
	compute_elimination_index(bmap1, elim);
	for (i = 0; i < bmap2->n_eq; ++i) {
		int reduced;
		reduced = reduced_using_equalities(v->block.data, bmap2->eq[i],
							bmap1, elim);
		if (reduced && !isl_int_is_zero(v->block.data[0]) &&
		    isl_seq_first_non_zero(v->block.data + 1, total) == -1)
			goto disjoint;
	}
	for (i = 0; i < bmap2->n_ineq; ++i) {
		int reduced;
		reduced = reduced_using_equalities(v->block.data,
						bmap2->ineq[i], bmap1, elim);
		if (reduced && isl_int_is_neg(v->block.data[0]) &&
		    isl_seq_first_non_zero(v->block.data + 1, total) == -1)
			goto disjoint;
	}
	compute_elimination_index(bmap2, elim);
	for (i = 0; i < bmap1->n_ineq; ++i) {
		int reduced;
		reduced = reduced_using_equalities(v->block.data,
						bmap1->ineq[i], bmap2, elim);
		if (reduced && isl_int_is_neg(v->block.data[0]) &&
		    isl_seq_first_non_zero(v->block.data + 1, total) == -1)
			goto disjoint;
	}
	isl_vec_free(v);
	free(elim);
	return 0;
disjoint:
	isl_vec_free(v);
	free(elim);
	return 1;
error:
	isl_vec_free(v);
	free(elim);
	return -1;
}

int isl_basic_set_plain_is_disjoint(__isl_keep isl_basic_set *bset1,
	__isl_keep isl_basic_set *bset2)
{
	return isl_basic_map_plain_is_disjoint((struct isl_basic_map *)bset1,
					      (struct isl_basic_map *)bset2);
}

int isl_map_plain_is_disjoint(__isl_keep isl_map *map1,
	__isl_keep isl_map *map2)
{
	int i, j;

	if (!map1 || !map2)
		return -1;

	if (isl_map_plain_is_equal(map1, map2))
		return 0;

	for (i = 0; i < map1->n; ++i) {
		for (j = 0; j < map2->n; ++j) {
			int d = isl_basic_map_plain_is_disjoint(map1->p[i],
							       map2->p[j]);
			if (d != 1)
				return d;
		}
	}
	return 1;
}

int isl_set_plain_is_disjoint(__isl_keep isl_set *set1,
	__isl_keep isl_set *set2)
{
	return isl_map_plain_is_disjoint((struct isl_map *)set1,
					(struct isl_map *)set2);
}

int isl_set_fast_is_disjoint(__isl_keep isl_set *set1, __isl_keep isl_set *set2)
{
	return isl_set_plain_is_disjoint(set1, set2);
}

/* Check if we can combine a given div with lower bound l and upper
 * bound u with some other div and if so return that other div.
 * Otherwise return -1.
 *
 * We first check that
 *	- the bounds are opposites of each other (except for the constant
 *	  term)
 *	- the bounds do not reference any other div
 *	- no div is defined in terms of this div
 *
 * Let m be the size of the range allowed on the div by the bounds.
 * That is, the bounds are of the form
 *
 *	e <= a <= e + m - 1
 *
 * with e some expression in the other variables.
 * We look for another div b such that no third div is defined in terms
 * of this second div b and such that in any constraint that contains
 * a (except for the given lower and upper bound), also contains b
 * with a coefficient that is m times that of b.
 * That is, all constraints (execpt for the lower and upper bound)
 * are of the form
 *
 *	e + f (a + m b) >= 0
 *
 * If so, we return b so that "a + m b" can be replaced by
 * a single div "c = a + m b".
 */
static int div_find_coalesce(struct isl_basic_map *bmap, int *pairs,
	unsigned div, unsigned l, unsigned u)
{
	int i, j;
	unsigned dim;
	int coalesce = -1;

	if (bmap->n_div <= 1)
		return -1;
	dim = isl_space_dim(bmap->dim, isl_dim_all);
	if (isl_seq_first_non_zero(bmap->ineq[l] + 1 + dim, div) != -1)
		return -1;
	if (isl_seq_first_non_zero(bmap->ineq[l] + 1 + dim + div + 1,
				   bmap->n_div - div - 1) != -1)
		return -1;
	if (!isl_seq_is_neg(bmap->ineq[l] + 1, bmap->ineq[u] + 1,
			    dim + bmap->n_div))
		return -1;

	for (i = 0; i < bmap->n_div; ++i) {
		if (isl_int_is_zero(bmap->div[i][0]))
			continue;
		if (!isl_int_is_zero(bmap->div[i][1 + 1 + dim + div]))
			return -1;
	}

	isl_int_add(bmap->ineq[l][0], bmap->ineq[l][0], bmap->ineq[u][0]);
	if (isl_int_is_neg(bmap->ineq[l][0])) {
		isl_int_sub(bmap->ineq[l][0],
			    bmap->ineq[l][0], bmap->ineq[u][0]);
		bmap = isl_basic_map_copy(bmap);
		bmap = isl_basic_map_set_to_empty(bmap);
		isl_basic_map_free(bmap);
		return -1;
	}
	isl_int_add_ui(bmap->ineq[l][0], bmap->ineq[l][0], 1);
	for (i = 0; i < bmap->n_div; ++i) {
		if (i == div)
			continue;
		if (!pairs[i])
			continue;
		for (j = 0; j < bmap->n_div; ++j) {
			if (isl_int_is_zero(bmap->div[j][0]))
				continue;
			if (!isl_int_is_zero(bmap->div[j][1 + 1 + dim + i]))
				break;
		}
		if (j < bmap->n_div)
			continue;
		for (j = 0; j < bmap->n_ineq; ++j) {
			int valid;
			if (j == l || j == u)
				continue;
			if (isl_int_is_zero(bmap->ineq[j][1 + dim + div]))
				continue;
			if (isl_int_is_zero(bmap->ineq[j][1 + dim + i]))
				break;
			isl_int_mul(bmap->ineq[j][1 + dim + div],
				    bmap->ineq[j][1 + dim + div],
				    bmap->ineq[l][0]);
			valid = isl_int_eq(bmap->ineq[j][1 + dim + div],
					   bmap->ineq[j][1 + dim + i]);
			isl_int_divexact(bmap->ineq[j][1 + dim + div],
					 bmap->ineq[j][1 + dim + div],
					 bmap->ineq[l][0]);
			if (!valid)
				break;
		}
		if (j < bmap->n_ineq)
			continue;
		coalesce = i;
		break;
	}
	isl_int_sub_ui(bmap->ineq[l][0], bmap->ineq[l][0], 1);
	isl_int_sub(bmap->ineq[l][0], bmap->ineq[l][0], bmap->ineq[u][0]);
	return coalesce;
}

/* Given a lower and an upper bound on div i, construct an inequality
 * that when nonnegative ensures that this pair of bounds always allows
 * for an integer value of the given div.
 * The lower bound is inequality l, while the upper bound is inequality u.
 * The constructed inequality is stored in ineq.
 * g, fl, fu are temporary scalars.
 *
 * Let the upper bound be
 *
 *	-n_u a + e_u >= 0
 *
 * and the lower bound
 *
 *	n_l a + e_l >= 0
 *
 * Let n_u = f_u g and n_l = f_l g, with g = gcd(n_u, n_l).
 * We have
 *
 *	- f_u e_l <= f_u f_l g a <= f_l e_u
 *
 * Since all variables are integer valued, this is equivalent to
 *
 *	- f_u e_l - (f_u - 1) <= f_u f_l g a <= f_l e_u + (f_l - 1)
 *
 * If this interval is at least f_u f_l g, then it contains at least
 * one integer value for a.
 * That is, the test constraint is
 *
 *	f_l e_u + f_u e_l + f_l - 1 + f_u - 1 + 1 >= f_u f_l g
 */
static void construct_test_ineq(struct isl_basic_map *bmap, int i,
	int l, int u, isl_int *ineq, isl_int g, isl_int fl, isl_int fu)
{
	unsigned dim;
	dim = isl_space_dim(bmap->dim, isl_dim_all);

	isl_int_gcd(g, bmap->ineq[l][1 + dim + i], bmap->ineq[u][1 + dim + i]);
	isl_int_divexact(fl, bmap->ineq[l][1 + dim + i], g);
	isl_int_divexact(fu, bmap->ineq[u][1 + dim + i], g);
	isl_int_neg(fu, fu);
	isl_seq_combine(ineq, fl, bmap->ineq[u], fu, bmap->ineq[l],
			1 + dim + bmap->n_div);
	isl_int_add(ineq[0], ineq[0], fl);
	isl_int_add(ineq[0], ineq[0], fu);
	isl_int_sub_ui(ineq[0], ineq[0], 1);
	isl_int_mul(g, g, fl);
	isl_int_mul(g, g, fu);
	isl_int_sub(ineq[0], ineq[0], g);
}

/* Remove more kinds of divs that are not strictly needed.
 * In particular, if all pairs of lower and upper bounds on a div
 * are such that they allow at least one integer value of the div,
 * the we can eliminate the div using Fourier-Motzkin without
 * introducing any spurious solutions.
 */
static struct isl_basic_map *drop_more_redundant_divs(
	struct isl_basic_map *bmap, int *pairs, int n)
{
	struct isl_tab *tab = NULL;
	struct isl_vec *vec = NULL;
	unsigned dim;
	int remove = -1;
	isl_int g, fl, fu;

	isl_int_init(g);
	isl_int_init(fl);
	isl_int_init(fu);

	if (!bmap)
		goto error;

	dim = isl_space_dim(bmap->dim, isl_dim_all);
	vec = isl_vec_alloc(bmap->ctx, 1 + dim + bmap->n_div);
	if (!vec)
		goto error;

	tab = isl_tab_from_basic_map(bmap, 0);

	while (n > 0) {
		int i, l, u;
		int best = -1;
		enum isl_lp_result res;

		for (i = 0; i < bmap->n_div; ++i) {
			if (!pairs[i])
				continue;
			if (best >= 0 && pairs[best] <= pairs[i])
				continue;
			best = i;
		}

		i = best;
		for (l = 0; l < bmap->n_ineq; ++l) {
			if (!isl_int_is_pos(bmap->ineq[l][1 + dim + i]))
				continue;
			for (u = 0; u < bmap->n_ineq; ++u) {
				if (!isl_int_is_neg(bmap->ineq[u][1 + dim + i]))
					continue;
				construct_test_ineq(bmap, i, l, u,
						    vec->el, g, fl, fu);
				res = isl_tab_min(tab, vec->el,
						  bmap->ctx->one, &g, NULL, 0);
				if (res == isl_lp_error)
					goto error;
				if (res == isl_lp_empty) {
					bmap = isl_basic_map_set_to_empty(bmap);
					break;
				}
				if (res != isl_lp_ok || isl_int_is_neg(g))
					break;
			}
			if (u < bmap->n_ineq)
				break;
		}
		if (l == bmap->n_ineq) {
			remove = i;
			break;
		}
		pairs[i] = 0;
		--n;
	}

	isl_tab_free(tab);
	isl_vec_free(vec);

	isl_int_clear(g);
	isl_int_clear(fl);
	isl_int_clear(fu);

	free(pairs);

	if (remove < 0)
		return bmap;

	bmap = isl_basic_map_remove_dims(bmap, isl_dim_div, remove, 1);
	return isl_basic_map_drop_redundant_divs(bmap);
error:
	free(pairs);
	isl_basic_map_free(bmap);
	isl_tab_free(tab);
	isl_vec_free(vec);
	isl_int_clear(g);
	isl_int_clear(fl);
	isl_int_clear(fu);
	return NULL;
}

/* Given a pair of divs div1 and div2 such that, expect for the lower bound l
 * and the upper bound u, div1 always occurs together with div2 in the form 
 * (div1 + m div2), where m is the constant range on the variable div1
 * allowed by l and u, replace the pair div1 and div2 by a single
 * div that is equal to div1 + m div2.
 *
 * The new div will appear in the location that contains div2.
 * We need to modify all constraints that contain
 * div2 = (div - div1) / m
 * (If a constraint does not contain div2, it will also not contain div1.)
 * If the constraint also contains div1, then we know they appear
 * as f (div1 + m div2) and we can simply replace (div1 + m div2) by div,
 * i.e., the coefficient of div is f.
 *
 * Otherwise, we first need to introduce div1 into the constraint.
 * Let the l be
 *
 *	div1 + f >=0
 *
 * and u
 *
 *	-div1 + f' >= 0
 *
 * A lower bound on div2
 *
 *	n div2 + t >= 0
 *
 * can be replaced by
 *
 *	(n * (m div 2 + div1) + m t + n f)/g >= 0
 *
 * with g = gcd(m,n).
 * An upper bound
 *
 *	-n div2 + t >= 0
 *
 * can be replaced by
 *
 *	(-n * (m div2 + div1) + m t + n f')/g >= 0
 *
 * These constraint are those that we would obtain from eliminating
 * div1 using Fourier-Motzkin.
 *
 * After all constraints have been modified, we drop the lower and upper
 * bound and then drop div1.
 */
static struct isl_basic_map *coalesce_divs(struct isl_basic_map *bmap,
	unsigned div1, unsigned div2, unsigned l, unsigned u)
{
	isl_int a;
	isl_int b;
	isl_int m;
	unsigned dim, total;
	int i;

	dim = isl_space_dim(bmap->dim, isl_dim_all);
	total = 1 + dim + bmap->n_div;

	isl_int_init(a);
	isl_int_init(b);
	isl_int_init(m);
	isl_int_add(m, bmap->ineq[l][0], bmap->ineq[u][0]);
	isl_int_add_ui(m, m, 1);

	for (i = 0; i < bmap->n_ineq; ++i) {
		if (i == l || i == u)
			continue;
		if (isl_int_is_zero(bmap->ineq[i][1 + dim + div2]))
			continue;
		if (isl_int_is_zero(bmap->ineq[i][1 + dim + div1])) {
			isl_int_gcd(b, m, bmap->ineq[i][1 + dim + div2]);
			isl_int_divexact(a, m, b);
			isl_int_divexact(b, bmap->ineq[i][1 + dim + div2], b);
			if (isl_int_is_pos(b)) {
				isl_seq_combine(bmap->ineq[i], a, bmap->ineq[i],
						b, bmap->ineq[l], total);
			} else {
				isl_int_neg(b, b);
				isl_seq_combine(bmap->ineq[i], a, bmap->ineq[i],
						b, bmap->ineq[u], total);
			}
		}
		isl_int_set(bmap->ineq[i][1 + dim + div2],
			    bmap->ineq[i][1 + dim + div1]);
		isl_int_set_si(bmap->ineq[i][1 + dim + div1], 0);
	}

	isl_int_clear(a);
	isl_int_clear(b);
	isl_int_clear(m);
	if (l > u) {
		isl_basic_map_drop_inequality(bmap, l);
		isl_basic_map_drop_inequality(bmap, u);
	} else {
		isl_basic_map_drop_inequality(bmap, u);
		isl_basic_map_drop_inequality(bmap, l);
	}
	bmap = isl_basic_map_drop_div(bmap, div1);
	return bmap;
}

/* First check if we can coalesce any pair of divs and
 * then continue with dropping more redundant divs.
 *
 * We loop over all pairs of lower and upper bounds on a div
 * with coefficient 1 and -1, respectively, check if there
 * is any other div "c" with which we can coalesce the div
 * and if so, perform the coalescing.
 */
static struct isl_basic_map *coalesce_or_drop_more_redundant_divs(
	struct isl_basic_map *bmap, int *pairs, int n)
{
	int i, l, u;
	unsigned dim;

	dim = isl_space_dim(bmap->dim, isl_dim_all);

	for (i = 0; i < bmap->n_div; ++i) {
		if (!pairs[i])
			continue;
		for (l = 0; l < bmap->n_ineq; ++l) {
			if (!isl_int_is_one(bmap->ineq[l][1 + dim + i]))
				continue;
			for (u = 0; u < bmap->n_ineq; ++u) {
				int c;

				if (!isl_int_is_negone(bmap->ineq[u][1+dim+i]))
					continue;
				c = div_find_coalesce(bmap, pairs, i, l, u);
				if (c < 0)
					continue;
				free(pairs);
				bmap = coalesce_divs(bmap, i, c, l, u);
				return isl_basic_map_drop_redundant_divs(bmap);
			}
		}
	}

	if (ISL_F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
		return bmap;

	return drop_more_redundant_divs(bmap, pairs, n);
}

/* Remove divs that are not strictly needed.
 * In particular, if a div only occurs positively (or negatively)
 * in constraints, then it can simply be dropped.
 * Also, if a div occurs only occurs in two constraints and if moreover
 * those two constraints are opposite to each other, except for the constant
 * term and if the sum of the constant terms is such that for any value
 * of the other values, there is always at least one integer value of the
 * div, i.e., if one plus this sum is greater than or equal to
 * the (absolute value) of the coefficent of the div in the constraints,
 * then we can also simply drop the div.
 *
 * If any divs are left after these simple checks then we move on
 * to more complicated cases in drop_more_redundant_divs.
 */
struct isl_basic_map *isl_basic_map_drop_redundant_divs(
	struct isl_basic_map *bmap)
{
	int i, j;
	unsigned off;
	int *pairs = NULL;
	int n = 0;

	if (!bmap)
		goto error;

	off = isl_space_dim(bmap->dim, isl_dim_all);
	pairs = isl_calloc_array(bmap->ctx, int, bmap->n_div);
	if (!pairs)
		goto error;

	for (i = 0; i < bmap->n_div; ++i) {
		int pos, neg;
		int last_pos, last_neg;
		int redundant;
		int defined;

		defined = !isl_int_is_zero(bmap->div[i][0]);
		for (j = 0; j < bmap->n_eq; ++j)
			if (!isl_int_is_zero(bmap->eq[j][1 + off + i]))
				break;
		if (j < bmap->n_eq)
			continue;
		++n;
		pos = neg = 0;
		for (j = 0; j < bmap->n_ineq; ++j) {
			if (isl_int_is_pos(bmap->ineq[j][1 + off + i])) {
				last_pos = j;
				++pos;
			}
			if (isl_int_is_neg(bmap->ineq[j][1 + off + i])) {
				last_neg = j;
				++neg;
			}
		}
		pairs[i] = pos * neg;
		if (pairs[i] == 0) {
			for (j = bmap->n_ineq - 1; j >= 0; --j)
				if (!isl_int_is_zero(bmap->ineq[j][1+off+i]))
					isl_basic_map_drop_inequality(bmap, j);
			bmap = isl_basic_map_drop_div(bmap, i);
			free(pairs);
			return isl_basic_map_drop_redundant_divs(bmap);
		}
		if (pairs[i] != 1)
			continue;
		if (!isl_seq_is_neg(bmap->ineq[last_pos] + 1,
				    bmap->ineq[last_neg] + 1,
				    off + bmap->n_div))
			continue;

		isl_int_add(bmap->ineq[last_pos][0],
			    bmap->ineq[last_pos][0], bmap->ineq[last_neg][0]);
		isl_int_add_ui(bmap->ineq[last_pos][0],
			       bmap->ineq[last_pos][0], 1);
		redundant = isl_int_ge(bmap->ineq[last_pos][0],
				bmap->ineq[last_pos][1+off+i]);
		isl_int_sub_ui(bmap->ineq[last_pos][0],
			       bmap->ineq[last_pos][0], 1);
		isl_int_sub(bmap->ineq[last_pos][0],
			    bmap->ineq[last_pos][0], bmap->ineq[last_neg][0]);
		if (!redundant) {
			if (defined ||
			    !ok_to_set_div_from_bound(bmap, i, last_pos)) {
				pairs[i] = 0;
				--n;
				continue;
			}
			bmap = set_div_from_lower_bound(bmap, i, last_pos);
			bmap = isl_basic_map_simplify(bmap);
			free(pairs);
			return isl_basic_map_drop_redundant_divs(bmap);
		}
		if (last_pos > last_neg) {
			isl_basic_map_drop_inequality(bmap, last_pos);
			isl_basic_map_drop_inequality(bmap, last_neg);
		} else {
			isl_basic_map_drop_inequality(bmap, last_neg);
			isl_basic_map_drop_inequality(bmap, last_pos);
		}
		bmap = isl_basic_map_drop_div(bmap, i);
		free(pairs);
		return isl_basic_map_drop_redundant_divs(bmap);
	}

	if (n > 0)
		return coalesce_or_drop_more_redundant_divs(bmap, pairs, n);

	free(pairs);
	return bmap;
error:
	free(pairs);
	isl_basic_map_free(bmap);
	return NULL;
}

struct isl_basic_set *isl_basic_set_drop_redundant_divs(
	struct isl_basic_set *bset)
{
	return (struct isl_basic_set *)
	    isl_basic_map_drop_redundant_divs((struct isl_basic_map *)bset);
}

struct isl_map *isl_map_drop_redundant_divs(struct isl_map *map)
{
	int i;

	if (!map)
		return NULL;
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_drop_redundant_divs(map->p[i]);
		if (!map->p[i])
			goto error;
	}
	ISL_F_CLR(map, ISL_MAP_NORMALIZED);
	return map;
error:
	isl_map_free(map);
	return NULL;
}

struct isl_set *isl_set_drop_redundant_divs(struct isl_set *set)
{
	return (struct isl_set *)
	    isl_map_drop_redundant_divs((struct isl_map *)set);
}
