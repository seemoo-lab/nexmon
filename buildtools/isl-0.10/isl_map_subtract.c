/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <isl_map_private.h>
#include <isl/seq.h>
#include <isl/set.h>
#include <isl/map.h>
#include "isl_tab.h"
#include <isl_point_private.h>

static void expand_constraint(isl_vec *v, unsigned dim,
	isl_int *c, int *div_map, unsigned n_div)
{
	int i;

	isl_seq_cpy(v->el, c, 1 + dim);
	isl_seq_clr(v->el + 1 + dim, v->size - (1 + dim));

	for (i = 0; i < n_div; ++i)
		isl_int_set(v->el[1 + dim + div_map[i]], c[1 + dim + i]);
}

/* Add all constraints of bmap to tab.  The equalities of bmap
 * are added as a pair of inequalities.
 */
static int tab_add_constraints(struct isl_tab *tab,
	__isl_keep isl_basic_map *bmap, int *div_map)
{
	int i;
	unsigned dim;
	unsigned tab_total;
	unsigned bmap_total;
	isl_vec *v;

	if (!tab || !bmap)
		return -1;

	tab_total = isl_basic_map_total_dim(tab->bmap);
	bmap_total = isl_basic_map_total_dim(bmap);
	dim = isl_space_dim(tab->bmap->dim, isl_dim_all);

	if (isl_tab_extend_cons(tab, 2 * bmap->n_eq + bmap->n_ineq) < 0)
		return -1;

	v = isl_vec_alloc(bmap->ctx, 1 + tab_total);
	if (!v)
		return -1;

	for (i = 0; i < bmap->n_eq; ++i) {
		expand_constraint(v, dim, bmap->eq[i], div_map, bmap->n_div);
		if (isl_tab_add_ineq(tab, v->el) < 0)
			goto error;
		isl_seq_neg(bmap->eq[i], bmap->eq[i], 1 + bmap_total);
		expand_constraint(v, dim, bmap->eq[i], div_map, bmap->n_div);
		if (isl_tab_add_ineq(tab, v->el) < 0)
			goto error;
		isl_seq_neg(bmap->eq[i], bmap->eq[i], 1 + bmap_total);
		if (tab->empty)
			break;
	}

	for (i = 0; i < bmap->n_ineq; ++i) {
		expand_constraint(v, dim, bmap->ineq[i], div_map, bmap->n_div);
		if (isl_tab_add_ineq(tab, v->el) < 0)
			goto error;
		if (tab->empty)
			break;
	}

	isl_vec_free(v);
	return 0;
error:
	isl_vec_free(v);
	return -1;
}

/* Add a specific constraint of bmap (or its opposite) to tab.
 * The position of the constraint is specified by "c", where
 * the equalities of bmap are counted twice, once for the inequality
 * that is equal to the equality, and once for its negation.
 */
static int tab_add_constraint(struct isl_tab *tab,
	__isl_keep isl_basic_map *bmap, int *div_map, int c, int oppose)
{
	unsigned dim;
	unsigned tab_total;
	unsigned bmap_total;
	isl_vec *v;
	int r;

	if (!tab || !bmap)
		return -1;

	tab_total = isl_basic_map_total_dim(tab->bmap);
	bmap_total = isl_basic_map_total_dim(bmap);
	dim = isl_space_dim(tab->bmap->dim, isl_dim_all);

	v = isl_vec_alloc(bmap->ctx, 1 + tab_total);
	if (!v)
		return -1;

	if (c < 2 * bmap->n_eq) {
		if ((c % 2) != oppose)
			isl_seq_neg(bmap->eq[c/2], bmap->eq[c/2],
					1 + bmap_total);
		if (oppose)
			isl_int_sub_ui(bmap->eq[c/2][0], bmap->eq[c/2][0], 1);
		expand_constraint(v, dim, bmap->eq[c/2], div_map, bmap->n_div);
		r = isl_tab_add_ineq(tab, v->el);
		if (oppose)
			isl_int_add_ui(bmap->eq[c/2][0], bmap->eq[c/2][0], 1);
		if ((c % 2) != oppose)
			isl_seq_neg(bmap->eq[c/2], bmap->eq[c/2],
					1 + bmap_total);
	} else {
		c -= 2 * bmap->n_eq;
		if (oppose) {
			isl_seq_neg(bmap->ineq[c], bmap->ineq[c],
					1 + bmap_total);
			isl_int_sub_ui(bmap->ineq[c][0], bmap->ineq[c][0], 1);
		}
		expand_constraint(v, dim, bmap->ineq[c], div_map, bmap->n_div);
		r = isl_tab_add_ineq(tab, v->el);
		if (oppose) {
			isl_int_add_ui(bmap->ineq[c][0], bmap->ineq[c][0], 1);
			isl_seq_neg(bmap->ineq[c], bmap->ineq[c],
					1 + bmap_total);
		}
	}

	isl_vec_free(v);
	return r;
}

static int tab_add_divs(struct isl_tab *tab, __isl_keep isl_basic_map *bmap,
	int **div_map)
{
	int i, j;
	struct isl_vec *vec;
	unsigned total;
	unsigned dim;

	if (!bmap)
		return -1;
	if (!bmap->n_div)
		return 0;

	if (!*div_map)
		*div_map = isl_alloc_array(bmap->ctx, int, bmap->n_div);
	if (!*div_map)
		return -1;

	total = isl_basic_map_total_dim(tab->bmap);
	dim = total - tab->bmap->n_div;
	vec = isl_vec_alloc(bmap->ctx, 2 + total + bmap->n_div);
	if (!vec)
		return -1;

	for (i = 0; i < bmap->n_div; ++i) {
		isl_seq_cpy(vec->el, bmap->div[i], 2 + dim);
		isl_seq_clr(vec->el + 2 + dim, tab->bmap->n_div);
		for (j = 0; j < i; ++j)
			isl_int_set(vec->el[2 + dim + (*div_map)[j]],
					bmap->div[i][2 + dim + j]);
		for (j = 0; j < tab->bmap->n_div; ++j)
			if (isl_seq_eq(tab->bmap->div[j],
					vec->el, 2 + dim + tab->bmap->n_div))
				break;
		(*div_map)[i] = j;
		if (j == tab->bmap->n_div) {
			vec->size = 2 + dim + tab->bmap->n_div;
			if (isl_tab_add_div(tab, vec, NULL, NULL) < 0)
				goto error;
		}
	}

	isl_vec_free(vec);

	return 0;
error:
	isl_vec_free(vec);

	return -1;
}

/* Freeze all constraints of tableau tab.
 */
static int tab_freeze_constraints(struct isl_tab *tab)
{
	int i;

	for (i = 0; i < tab->n_con; ++i)
		if (isl_tab_freeze_constraint(tab, i) < 0)
			return -1;

	return 0;
}

/* Check for redundant constraints starting at offset.
 * Put the indices of the redundant constraints in index
 * and return the number of redundant constraints.
 */
static int n_non_redundant(isl_ctx *ctx, struct isl_tab *tab,
	int offset, int **index)
{
	int i, n;
	int n_test = tab->n_con - offset;

	if (isl_tab_detect_redundant(tab) < 0)
		return -1;

	if (!*index)
		*index = isl_alloc_array(ctx, int, n_test);
	if (!*index)
		return -1;

	for (n = 0, i = 0; i < n_test; ++i) {
		int r;
		r = isl_tab_is_redundant(tab, offset + i);
		if (r < 0)
			return -1;
		if (r)
			continue;
		(*index)[n++] = i;
	}

	return n;
}

/* basic_map_collect_diff calls add on each of the pieces of
 * the set difference between bmap and map until the add method
 * return a negative value.
 */
struct isl_diff_collector {
	int (*add)(struct isl_diff_collector *dc,
		    __isl_take isl_basic_map *bmap);
};

/* Compute the set difference between bmap and map and call
 * dc->add on each of the piece until this function returns
 * a negative value.
 * Return 0 on success and -1 on error.  dc->add returning
 * a negative value is treated as an error, but the calling
 * function can interpret the results based on the state of dc.
 *
 * Assumes that map has known divs.
 *
 * The difference is computed by a backtracking algorithm.
 * Each level corresponds to a basic map in "map".
 * When a node in entered for the first time, we check
 * if the corresonding basic map intersects the current piece
 * of "bmap".  If not, we move to the next level.
 * Otherwise, we split the current piece into as many
 * pieces as there are non-redundant constraints of the current
 * basic map in the intersection.  Each of these pieces is
 * handled by a child of the current node.
 * In particular, if there are n non-redundant constraints,
 * then for each 0 <= i < n, a piece is cut off by adding
 * constraints 0 <= j < i and adding the opposite of constraint i.
 * If there are no non-redundant constraints, meaning that the current
 * piece is a subset of the current basic map, then we simply backtrack.
 *
 * In the leaves, we check if the remaining piece has any integer points
 * and if so, pass it along to dc->add.  As a special case, if nothing
 * has been removed when we end up in a leaf, we simply pass along
 * the original basic map.
 */
static int basic_map_collect_diff(__isl_take isl_basic_map *bmap,
	__isl_take isl_map *map, struct isl_diff_collector *dc)
{
	int i;
	int modified;
	int level;
	int init;
	int empty;
	isl_ctx *ctx;
	struct isl_tab *tab = NULL;
	struct isl_tab_undo **snap = NULL;
	int *k = NULL;
	int *n = NULL;
	int **index = NULL;
	int **div_map = NULL;

	empty = isl_basic_map_is_empty(bmap);
	if (empty) {
		isl_basic_map_free(bmap);
		isl_map_free(map);
		return empty < 0 ? -1 : 0;
	}

	bmap = isl_basic_map_cow(bmap);
	map = isl_map_cow(map);

	if (!bmap || !map)
		goto error;

	ctx = map->ctx;
	snap = isl_alloc_array(map->ctx, struct isl_tab_undo *, map->n);
	k = isl_alloc_array(map->ctx, int, map->n);
	n = isl_alloc_array(map->ctx, int, map->n);
	index = isl_calloc_array(map->ctx, int *, map->n);
	div_map = isl_calloc_array(map->ctx, int *, map->n);
	if (!snap || !k || !n || !index || !div_map)
		goto error;

	bmap = isl_basic_map_order_divs(bmap);
	map = isl_map_order_divs(map);

	tab = isl_tab_from_basic_map(bmap, 1);

	modified = 0;
	level = 0;
	init = 1;

	while (level >= 0) {
		if (level >= map->n) {
			int empty;
			struct isl_basic_map *bm;
			if (!modified) {
				if (dc->add(dc, isl_basic_map_copy(bmap)) < 0)
					goto error;
				break;
			}
			bm = isl_basic_map_copy(tab->bmap);
			bm = isl_basic_map_cow(bm);
			bm = isl_basic_map_update_from_tab(bm, tab);
			bm = isl_basic_map_simplify(bm);
			bm = isl_basic_map_finalize(bm);
			empty = isl_basic_map_is_empty(bm);
			if (empty)
				isl_basic_map_free(bm);
			else if (dc->add(dc, bm) < 0)
				goto error;
			if (empty < 0)
				goto error;
			level--;
			init = 0;
			continue;
		}
		if (init) {
			int offset;
			struct isl_tab_undo *snap2;
			snap2 = isl_tab_snap(tab);
			if (tab_add_divs(tab, map->p[level],
					 &div_map[level]) < 0)
				goto error;
			offset = tab->n_con;
			snap[level] = isl_tab_snap(tab);
			if (tab_freeze_constraints(tab) < 0)
				goto error;
			if (tab_add_constraints(tab, map->p[level],
						div_map[level]) < 0)
				goto error;
			k[level] = 0;
			n[level] = 0;
			if (tab->empty) {
				if (isl_tab_rollback(tab, snap2) < 0)
					goto error;
				level++;
				continue;
			}
			modified = 1;
			n[level] = n_non_redundant(ctx, tab, offset,
						    &index[level]);
			if (n[level] < 0)
				goto error;
			if (n[level] == 0) {
				level--;
				init = 0;
				continue;
			}
			if (isl_tab_rollback(tab, snap[level]) < 0)
				goto error;
			if (tab_add_constraint(tab, map->p[level],
					div_map[level], index[level][0], 1) < 0)
				goto error;
			level++;
			continue;
		} else {
			if (k[level] + 1 >= n[level]) {
				level--;
				continue;
			}
			if (isl_tab_rollback(tab, snap[level]) < 0)
				goto error;
			if (tab_add_constraint(tab, map->p[level],
						div_map[level],
						index[level][k[level]], 0) < 0)
				goto error;
			snap[level] = isl_tab_snap(tab);
			k[level]++;
			if (tab_add_constraint(tab, map->p[level],
						div_map[level],
						index[level][k[level]], 1) < 0)
				goto error;
			level++;
			init = 1;
			continue;
		}
	}

	isl_tab_free(tab);
	free(snap);
	free(n);
	free(k);
	for (i = 0; index && i < map->n; ++i)
		free(index[i]);
	free(index);
	for (i = 0; div_map && i < map->n; ++i)
		free(div_map[i]);
	free(div_map);

	isl_basic_map_free(bmap);
	isl_map_free(map);

	return 0;
error:
	isl_tab_free(tab);
	free(snap);
	free(n);
	free(k);
	for (i = 0; index && i < map->n; ++i)
		free(index[i]);
	free(index);
	for (i = 0; div_map && i < map->n; ++i)
		free(div_map[i]);
	free(div_map);
	isl_basic_map_free(bmap);
	isl_map_free(map);
	return -1;
}

/* A diff collector that actually collects all parts of the
 * set difference in the field diff.
 */
struct isl_subtract_diff_collector {
	struct isl_diff_collector dc;
	struct isl_map *diff;
};

/* isl_subtract_diff_collector callback.
 */
static int basic_map_subtract_add(struct isl_diff_collector *dc,
			    __isl_take isl_basic_map *bmap)
{
	struct isl_subtract_diff_collector *sdc;
	sdc = (struct isl_subtract_diff_collector *)dc;

	sdc->diff = isl_map_union_disjoint(sdc->diff,
			isl_map_from_basic_map(bmap));

	return sdc->diff ? 0 : -1;
}

/* Return the set difference between bmap and map.
 */
static __isl_give isl_map *basic_map_subtract(__isl_take isl_basic_map *bmap,
	__isl_take isl_map *map)
{
	struct isl_subtract_diff_collector sdc;
	sdc.dc.add = &basic_map_subtract_add;
	sdc.diff = isl_map_empty_like_basic_map(bmap);
	if (basic_map_collect_diff(bmap, map, &sdc.dc) < 0) {
		isl_map_free(sdc.diff);
		sdc.diff = NULL;
	}
	return sdc.diff;
}

/* Return the set difference between map1 and map2.
 * (U_i A_i) \ (U_j B_j) is computed as U_i (A_i \ (U_j B_j))
 */
static __isl_give isl_map *map_subtract( __isl_take isl_map *map1,
	__isl_take isl_map *map2)
{
	int i;
	struct isl_map *diff;

	if (!map1 || !map2)
		goto error;

	isl_assert(map1->ctx, isl_space_is_equal(map1->dim, map2->dim), goto error);

	if (isl_map_is_empty(map2)) {
		isl_map_free(map2);
		return map1;
	}

	map1 = isl_map_compute_divs(map1);
	map2 = isl_map_compute_divs(map2);
	if (!map1 || !map2)
		goto error;

	map1 = isl_map_remove_empty_parts(map1);
	map2 = isl_map_remove_empty_parts(map2);

	diff = isl_map_empty_like(map1);
	for (i = 0; i < map1->n; ++i) {
		struct isl_map *d;
		d = basic_map_subtract(isl_basic_map_copy(map1->p[i]),
				       isl_map_copy(map2));
		if (ISL_F_ISSET(map1, ISL_MAP_DISJOINT))
			diff = isl_map_union_disjoint(diff, d);
		else
			diff = isl_map_union(diff, d);
	}

	isl_map_free(map1);
	isl_map_free(map2);

	return diff;
error:
	isl_map_free(map1);
	isl_map_free(map2);
	return NULL;
}

__isl_give isl_map *isl_map_subtract( __isl_take isl_map *map1,
	__isl_take isl_map *map2)
{
	return isl_map_align_params_map_map_and(map1, map2, &map_subtract);
}

struct isl_set *isl_set_subtract(struct isl_set *set1, struct isl_set *set2)
{
	return (struct isl_set *)
		isl_map_subtract(
			(struct isl_map *)set1, (struct isl_map *)set2);
}

/* Remove the elements of "dom" from the domain of "map".
 */
static __isl_give isl_map *map_subtract_domain(__isl_take isl_map *map,
	__isl_take isl_set *dom)
{
	isl_map *ext_dom;

	if (!isl_map_compatible_domain(map, dom))
		isl_die(isl_set_get_ctx(dom), isl_error_invalid,
			"incompatible spaces", goto error);
	
	ext_dom = isl_map_universe(isl_map_get_space(map));
	ext_dom = isl_map_intersect_domain(ext_dom, dom);
	return isl_map_subtract(map, ext_dom);
error:
	isl_map_free(map);
	isl_set_free(dom);
	return NULL;
}

__isl_give isl_map *isl_map_subtract_domain(__isl_take isl_map *map,
	__isl_take isl_set *dom)
{
	return isl_map_align_params_map_map_and(map, dom, &map_subtract_domain);
}

/* Remove the elements of "dom" from the range of "map".
 */
static __isl_give isl_map *map_subtract_range(__isl_take isl_map *map,
	__isl_take isl_set *dom)
{
	isl_map *ext_dom;

	if (!isl_map_compatible_range(map, dom))
		isl_die(isl_set_get_ctx(dom), isl_error_invalid,
			"incompatible spaces", goto error);
	
	ext_dom = isl_map_universe(isl_map_get_space(map));
	ext_dom = isl_map_intersect_range(ext_dom, dom);
	return isl_map_subtract(map, ext_dom);
error:
	isl_map_free(map);
	isl_set_free(dom);
	return NULL;
}

__isl_give isl_map *isl_map_subtract_range(__isl_take isl_map *map,
	__isl_take isl_set *dom)
{
	return isl_map_align_params_map_map_and(map, dom, &map_subtract_range);
}

/* A diff collector that aborts as soon as its add function is called,
 * setting empty to 0.
 */
struct isl_is_empty_diff_collector {
	struct isl_diff_collector dc;
	int empty;
};

/* isl_is_empty_diff_collector callback.
 */
static int basic_map_is_empty_add(struct isl_diff_collector *dc,
			    __isl_take isl_basic_map *bmap)
{
	struct isl_is_empty_diff_collector *edc;
	edc = (struct isl_is_empty_diff_collector *)dc;

	edc->empty = 0;

	isl_basic_map_free(bmap);
	return -1;
}

/* Check if bmap \ map is empty by computing this set difference
 * and breaking off as soon as the difference is known to be non-empty.
 */
static int basic_map_diff_is_empty(__isl_keep isl_basic_map *bmap,
	__isl_keep isl_map *map)
{
	int r;
	struct isl_is_empty_diff_collector edc;

	r = isl_basic_map_plain_is_empty(bmap);
	if (r)
		return r;

	edc.dc.add = &basic_map_is_empty_add;
	edc.empty = 1;
	r = basic_map_collect_diff(isl_basic_map_copy(bmap),
				   isl_map_copy(map), &edc.dc);
	if (!edc.empty)
		return 0;

	return r < 0 ? -1 : 1;
}

/* Check if map1 \ map2 is empty by checking if the set difference is empty
 * for each of the basic maps in map1.
 */
static int map_diff_is_empty(__isl_keep isl_map *map1, __isl_keep isl_map *map2)
{
	int i;
	int is_empty = 1;

	if (!map1 || !map2)
		return -1;
	
	for (i = 0; i < map1->n; ++i) {
		is_empty = basic_map_diff_is_empty(map1->p[i], map2);
		if (is_empty < 0 || !is_empty)
			 break;
	}

	return is_empty;
}

/* Return 1 if "bmap" contains a single element.
 */
int isl_basic_map_plain_is_singleton(__isl_keep isl_basic_map *bmap)
{
	if (!bmap)
		return -1;
	if (bmap->n_div)
		return 0;
	if (bmap->n_ineq)
		return 0;
	return bmap->n_eq == isl_basic_map_total_dim(bmap);
}

/* Return 1 if "map" contains a single element.
 */
int isl_map_plain_is_singleton(__isl_keep isl_map *map)
{
	if (!map)
		return -1;
	if (map->n != 1)
		return 0;

	return isl_basic_map_plain_is_singleton(map->p[0]);
}

/* Given a singleton basic map, extract the single element
 * as an isl_point.
 */
static __isl_give isl_point *singleton_extract_point(
	__isl_keep isl_basic_map *bmap)
{
	int j;
	unsigned dim;
	struct isl_vec *point;
	isl_int m;

	if (!bmap)
		return NULL;

	dim = isl_basic_map_total_dim(bmap);
	isl_assert(bmap->ctx, bmap->n_eq == dim, return NULL);
	point = isl_vec_alloc(bmap->ctx, 1 + dim);
	if (!point)
		return NULL;

	isl_int_init(m);

	isl_int_set_si(point->el[0], 1);
	for (j = 0; j < bmap->n_eq; ++j) {
		int i = dim - 1 - j;
		isl_assert(bmap->ctx,
		    isl_seq_first_non_zero(bmap->eq[j] + 1, i) == -1,
		    goto error);
		isl_assert(bmap->ctx,
		    isl_int_is_one(bmap->eq[j][1 + i]) ||
		    isl_int_is_negone(bmap->eq[j][1 + i]),
		    goto error);
		isl_assert(bmap->ctx,
		    isl_seq_first_non_zero(bmap->eq[j]+1+i+1, dim-i-1) == -1,
		    goto error);

		isl_int_gcd(m, point->el[0], bmap->eq[j][1 + i]);
		isl_int_divexact(m, bmap->eq[j][1 + i], m);
		isl_int_abs(m, m);
		isl_seq_scale(point->el, point->el, m, 1 + i);
		isl_int_divexact(m, point->el[0], bmap->eq[j][1 + i]);
		isl_int_neg(m, m);
		isl_int_mul(point->el[1 + i], m, bmap->eq[j][0]);
	}

	isl_int_clear(m);
	return isl_point_alloc(isl_basic_map_get_space(bmap), point);
error:
	isl_int_clear(m);
	isl_vec_free(point);
	return NULL;
}

/* Return 1 is the singleton map "map1" is a subset of "map2",
 * i.e., if the single element of "map1" is also an element of "map2".
 * Assumes "map2" has known divs.
 */
static int map_is_singleton_subset(__isl_keep isl_map *map1,
	__isl_keep isl_map *map2)
{
	int i;
	int is_subset = 0;
	struct isl_point *point;

	if (!map1 || !map2)
		return -1;
	if (map1->n != 1)
		return -1;

	point = singleton_extract_point(map1->p[0]);
	if (!point)
		return -1;

	for (i = 0; i < map2->n; ++i) {
		is_subset = isl_basic_map_contains_point(map2->p[i], point);
		if (is_subset)
			break;
	}

	isl_point_free(point);
	return is_subset;
}

static int map_is_subset(__isl_keep isl_map *map1, __isl_keep isl_map *map2)
{
	int is_subset = 0;

	if (!map1 || !map2)
		return -1;

	if (!isl_map_has_equal_space(map1, map2))
		return 0;

	if (isl_map_is_empty(map1))
		return 1;

	if (isl_map_is_empty(map2))
		return 0;

	if (isl_map_plain_is_universe(map2))
		return 1;

	map2 = isl_map_compute_divs(isl_map_copy(map2));
	if (isl_map_plain_is_singleton(map1)) {
		is_subset = map_is_singleton_subset(map1, map2);
		isl_map_free(map2);
		return is_subset;
	}
	is_subset = map_diff_is_empty(map1, map2);
	isl_map_free(map2);

	return is_subset;
}

int isl_map_is_subset(__isl_keep isl_map *map1, __isl_keep isl_map *map2)
{
	return isl_map_align_params_map_map_and_test(map1, map2,
							&map_is_subset);
}

int isl_set_is_subset(struct isl_set *set1, struct isl_set *set2)
{
	return isl_map_is_subset(
			(struct isl_map *)set1, (struct isl_map *)set2);
}

__isl_give isl_map *isl_map_make_disjoint(__isl_take isl_map *map)
{
	int i;
	struct isl_subtract_diff_collector sdc;
	sdc.dc.add = &basic_map_subtract_add;

	if (!map)
		return NULL;
	if (ISL_F_ISSET(map, ISL_MAP_DISJOINT))
		return map;
	if (map->n <= 1)
		return map;

	map = isl_map_compute_divs(map);
	map = isl_map_remove_empty_parts(map);

	if (!map || map->n <= 1)
		return map;

	sdc.diff = isl_map_from_basic_map(isl_basic_map_copy(map->p[0]));

	for (i = 1; i < map->n; ++i) {
		struct isl_basic_map *bmap = isl_basic_map_copy(map->p[i]);
		struct isl_map *copy = isl_map_copy(sdc.diff);
		if (basic_map_collect_diff(bmap, copy, &sdc.dc) < 0) {
			isl_map_free(sdc.diff);
			sdc.diff = NULL;
			break;
		}
	}

	isl_map_free(map);

	return sdc.diff;
}

__isl_give isl_set *isl_set_make_disjoint(__isl_take isl_set *set)
{
	return (struct isl_set *)isl_map_make_disjoint((struct isl_map *)set);
}

__isl_give isl_map *isl_map_complement(__isl_take isl_map *map)
{
	isl_map *universe;

	if (!map)
		return NULL;

	universe = isl_map_universe(isl_map_get_space(map));

	return isl_map_subtract(universe, map);
}

__isl_give isl_set *isl_set_complement(__isl_take isl_set *set)
{
	return isl_map_complement(set);
}
