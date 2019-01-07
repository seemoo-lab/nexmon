/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <isl_ctx_private.h>
#include <isl_map_private.h>
#include <isl/lp.h>
#include <isl/map.h>
#include <isl_mat_private.h>
#include <isl/set.h>
#include <isl/seq.h>
#include <isl_options_private.h>
#include "isl_equalities.h"
#include "isl_tab.h"

static struct isl_basic_set *uset_convex_hull_wrap_bounded(struct isl_set *set);

/* Return 1 if constraint c is redundant with respect to the constraints
 * in bmap.  If c is a lower [upper] bound in some variable and bmap
 * does not have a lower [upper] bound in that variable, then c cannot
 * be redundant and we do not need solve any lp.
 */
int isl_basic_map_constraint_is_redundant(struct isl_basic_map **bmap,
	isl_int *c, isl_int *opt_n, isl_int *opt_d)
{
	enum isl_lp_result res;
	unsigned total;
	int i, j;

	if (!bmap)
		return -1;

	total = isl_basic_map_total_dim(*bmap);
	for (i = 0; i < total; ++i) {
		int sign;
		if (isl_int_is_zero(c[1+i]))
			continue;
		sign = isl_int_sgn(c[1+i]);
		for (j = 0; j < (*bmap)->n_ineq; ++j)
			if (sign == isl_int_sgn((*bmap)->ineq[j][1+i]))
				break;
		if (j == (*bmap)->n_ineq)
			break;
	}
	if (i < total)
		return 0;

	res = isl_basic_map_solve_lp(*bmap, 0, c, (*bmap)->ctx->one,
					opt_n, opt_d, NULL);
	if (res == isl_lp_unbounded)
		return 0;
	if (res == isl_lp_error)
		return -1;
	if (res == isl_lp_empty) {
		*bmap = isl_basic_map_set_to_empty(*bmap);
		return 0;
	}
	return !isl_int_is_neg(*opt_n);
}

int isl_basic_set_constraint_is_redundant(struct isl_basic_set **bset,
	isl_int *c, isl_int *opt_n, isl_int *opt_d)
{
	return isl_basic_map_constraint_is_redundant(
			(struct isl_basic_map **)bset, c, opt_n, opt_d);
}

/* Remove redundant
 * constraints.  If the minimal value along the normal of a constraint
 * is the same if the constraint is removed, then the constraint is redundant.
 *
 * Alternatively, we could have intersected the basic map with the
 * corresponding equality and the checked if the dimension was that
 * of a facet.
 */
__isl_give isl_basic_map *isl_basic_map_remove_redundancies(
	__isl_take isl_basic_map *bmap)
{
	struct isl_tab *tab;

	if (!bmap)
		return NULL;

	bmap = isl_basic_map_gauss(bmap, NULL);
	if (ISL_F_ISSET(bmap, ISL_BASIC_MAP_EMPTY))
		return bmap;
	if (ISL_F_ISSET(bmap, ISL_BASIC_MAP_NO_REDUNDANT))
		return bmap;
	if (bmap->n_ineq <= 1)
		return bmap;

	tab = isl_tab_from_basic_map(bmap, 0);
	if (isl_tab_detect_implicit_equalities(tab) < 0)
		goto error;
	if (isl_tab_detect_redundant(tab) < 0)
		goto error;
	bmap = isl_basic_map_update_from_tab(bmap, tab);
	isl_tab_free(tab);
	ISL_F_SET(bmap, ISL_BASIC_MAP_NO_IMPLICIT);
	ISL_F_SET(bmap, ISL_BASIC_MAP_NO_REDUNDANT);
	return bmap;
error:
	isl_tab_free(tab);
	isl_basic_map_free(bmap);
	return NULL;
}

__isl_give isl_basic_set *isl_basic_set_remove_redundancies(
	__isl_take isl_basic_set *bset)
{
	return (struct isl_basic_set *)
		isl_basic_map_remove_redundancies((struct isl_basic_map *)bset);
}

/* Remove redundant constraints in each of the basic maps.
 */
__isl_give isl_map *isl_map_remove_redundancies(__isl_take isl_map *map)
{
	return isl_map_inline_foreach_basic_map(map,
					    &isl_basic_map_remove_redundancies);
}

__isl_give isl_set *isl_set_remove_redundancies(__isl_take isl_set *set)
{
	return isl_map_remove_redundancies(set);
}

/* Check if the set set is bound in the direction of the affine
 * constraint c and if so, set the constant term such that the
 * resulting constraint is a bounding constraint for the set.
 */
static int uset_is_bound(struct isl_set *set, isl_int *c, unsigned len)
{
	int first;
	int j;
	isl_int opt;
	isl_int opt_denom;

	isl_int_init(opt);
	isl_int_init(opt_denom);
	first = 1;
	for (j = 0; j < set->n; ++j) {
		enum isl_lp_result res;

		if (ISL_F_ISSET(set->p[j], ISL_BASIC_SET_EMPTY))
			continue;

		res = isl_basic_set_solve_lp(set->p[j],
				0, c, set->ctx->one, &opt, &opt_denom, NULL);
		if (res == isl_lp_unbounded)
			break;
		if (res == isl_lp_error)
			goto error;
		if (res == isl_lp_empty) {
			set->p[j] = isl_basic_set_set_to_empty(set->p[j]);
			if (!set->p[j])
				goto error;
			continue;
		}
		if (first || isl_int_is_neg(opt)) {
			if (!isl_int_is_one(opt_denom))
				isl_seq_scale(c, c, opt_denom, len);
			isl_int_sub(c[0], c[0], opt);
		}
		first = 0;
	}
	isl_int_clear(opt);
	isl_int_clear(opt_denom);
	return j >= set->n;
error:
	isl_int_clear(opt);
	isl_int_clear(opt_denom);
	return -1;
}

__isl_give isl_basic_map *isl_basic_map_set_rational(
	__isl_take isl_basic_set *bmap)
{
	if (!bmap)
		return NULL;

	if (ISL_F_ISSET(bmap, ISL_BASIC_MAP_RATIONAL))
		return bmap;

	bmap = isl_basic_map_cow(bmap);
	if (!bmap)
		return NULL;

	ISL_F_SET(bmap, ISL_BASIC_MAP_RATIONAL);

	return isl_basic_map_finalize(bmap);
}

__isl_give isl_basic_set *isl_basic_set_set_rational(
	__isl_take isl_basic_set *bset)
{
	return isl_basic_map_set_rational(bset);
}

__isl_give isl_map *isl_map_set_rational(__isl_take isl_map *map)
{
	int i;

	map = isl_map_cow(map);
	if (!map)
		return NULL;
	for (i = 0; i < map->n; ++i) {
		map->p[i] = isl_basic_map_set_rational(map->p[i]);
		if (!map->p[i])
			goto error;
	}
	return map;
error:
	isl_map_free(map);
	return NULL;
}

__isl_give isl_set *isl_set_set_rational(__isl_take isl_set *set)
{
	return isl_map_set_rational(set);
}

static struct isl_basic_set *isl_basic_set_add_equality(
	struct isl_basic_set *bset, isl_int *c)
{
	int i;
	unsigned dim;

	if (!bset)
		return NULL;

	if (ISL_F_ISSET(bset, ISL_BASIC_SET_EMPTY))
		return bset;

	isl_assert(bset->ctx, isl_basic_set_n_param(bset) == 0, goto error);
	isl_assert(bset->ctx, bset->n_div == 0, goto error);
	dim = isl_basic_set_n_dim(bset);
	bset = isl_basic_set_cow(bset);
	bset = isl_basic_set_extend(bset, 0, dim, 0, 1, 0);
	i = isl_basic_set_alloc_equality(bset);
	if (i < 0)
		goto error;
	isl_seq_cpy(bset->eq[i], c, 1 + dim);
	return bset;
error:
	isl_basic_set_free(bset);
	return NULL;
}

static struct isl_set *isl_set_add_basic_set_equality(struct isl_set *set, isl_int *c)
{
	int i;

	set = isl_set_cow(set);
	if (!set)
		return NULL;
	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_add_equality(set->p[i], c);
		if (!set->p[i])
			goto error;
	}
	return set;
error:
	isl_set_free(set);
	return NULL;
}

/* Given a union of basic sets, construct the constraints for wrapping
 * a facet around one of its ridges.
 * In particular, if each of n the d-dimensional basic sets i in "set"
 * contains the origin, satisfies the constraints x_1 >= 0 and x_2 >= 0
 * and is defined by the constraints
 *				    [ 1 ]
 *				A_i [ x ]  >= 0
 *
 * then the resulting set is of dimension n*(1+d) and has as constraints
 *
 *				    [ a_i ]
 *				A_i [ x_i ] >= 0
 *
 *				      a_i   >= 0
 *
 *			\sum_i x_{i,1} = 1
 */
static struct isl_basic_set *wrap_constraints(struct isl_set *set)
{
	struct isl_basic_set *lp;
	unsigned n_eq;
	unsigned n_ineq;
	int i, j, k;
	unsigned dim, lp_dim;

	if (!set)
		return NULL;

	dim = 1 + isl_set_n_dim(set);
	n_eq = 1;
	n_ineq = set->n;
	for (i = 0; i < set->n; ++i) {
		n_eq += set->p[i]->n_eq;
		n_ineq += set->p[i]->n_ineq;
	}
	lp = isl_basic_set_alloc(set->ctx, 0, dim * set->n, 0, n_eq, n_ineq);
	lp = isl_basic_set_set_rational(lp);
	if (!lp)
		return NULL;
	lp_dim = isl_basic_set_n_dim(lp);
	k = isl_basic_set_alloc_equality(lp);
	isl_int_set_si(lp->eq[k][0], -1);
	for (i = 0; i < set->n; ++i) {
		isl_int_set_si(lp->eq[k][1+dim*i], 0);
		isl_int_set_si(lp->eq[k][1+dim*i+1], 1);
		isl_seq_clr(lp->eq[k]+1+dim*i+2, dim-2);
	}
	for (i = 0; i < set->n; ++i) {
		k = isl_basic_set_alloc_inequality(lp);
		isl_seq_clr(lp->ineq[k], 1+lp_dim);
		isl_int_set_si(lp->ineq[k][1+dim*i], 1);

		for (j = 0; j < set->p[i]->n_eq; ++j) {
			k = isl_basic_set_alloc_equality(lp);
			isl_seq_clr(lp->eq[k], 1+dim*i);
			isl_seq_cpy(lp->eq[k]+1+dim*i, set->p[i]->eq[j], dim);
			isl_seq_clr(lp->eq[k]+1+dim*(i+1), dim*(set->n-i-1));
		}

		for (j = 0; j < set->p[i]->n_ineq; ++j) {
			k = isl_basic_set_alloc_inequality(lp);
			isl_seq_clr(lp->ineq[k], 1+dim*i);
			isl_seq_cpy(lp->ineq[k]+1+dim*i, set->p[i]->ineq[j], dim);
			isl_seq_clr(lp->ineq[k]+1+dim*(i+1), dim*(set->n-i-1));
		}
	}
	return lp;
}

/* Given a facet "facet" of the convex hull of "set" and a facet "ridge"
 * of that facet, compute the other facet of the convex hull that contains
 * the ridge.
 *
 * We first transform the set such that the facet constraint becomes
 *
 *			x_1 >= 0
 *
 * I.e., the facet lies in
 *
 *			x_1 = 0
 *
 * and on that facet, the constraint that defines the ridge is
 *
 *			x_2 >= 0
 *
 * (This transformation is not strictly needed, all that is needed is
 * that the ridge contains the origin.)
 *
 * Since the ridge contains the origin, the cone of the convex hull
 * will be of the form
 *
 *			x_1 >= 0
 *			x_2 >= a x_1
 *
 * with this second constraint defining the new facet.
 * The constant a is obtained by settting x_1 in the cone of the
 * convex hull to 1 and minimizing x_2.
 * Now, each element in the cone of the convex hull is the sum
 * of elements in the cones of the basic sets.
 * If a_i is the dilation factor of basic set i, then the problem
 * we need to solve is
 *
 *			min \sum_i x_{i,2}
 *			st
 *				\sum_i x_{i,1} = 1
 *				    a_i   >= 0
 *				  [ a_i ]
 *				A [ x_i ] >= 0
 *
 * with
 *				    [  1  ]
 *				A_i [ x_i ] >= 0
 *
 * the constraints of each (transformed) basic set.
 * If a = n/d, then the constraint defining the new facet (in the transformed
 * space) is
 *
 *			-n x_1 + d x_2 >= 0
 *
 * In the original space, we need to take the same combination of the
 * corresponding constraints "facet" and "ridge".
 *
 * If a = -infty = "-1/0", then we just return the original facet constraint.
 * This means that the facet is unbounded, but has a bounded intersection
 * with the union of sets.
 */
isl_int *isl_set_wrap_facet(__isl_keep isl_set *set,
	isl_int *facet, isl_int *ridge)
{
	int i;
	isl_ctx *ctx;
	struct isl_mat *T = NULL;
	struct isl_basic_set *lp = NULL;
	struct isl_vec *obj;
	enum isl_lp_result res;
	isl_int num, den;
	unsigned dim;

	if (!set)
		return NULL;
	ctx = set->ctx;
	set = isl_set_copy(set);
	set = isl_set_set_rational(set);

	dim = 1 + isl_set_n_dim(set);
	T = isl_mat_alloc(ctx, 3, dim);
	if (!T)
		goto error;
	isl_int_set_si(T->row[0][0], 1);
	isl_seq_clr(T->row[0]+1, dim - 1);
	isl_seq_cpy(T->row[1], facet, dim);
	isl_seq_cpy(T->row[2], ridge, dim);
	T = isl_mat_right_inverse(T);
	set = isl_set_preimage(set, T);
	T = NULL;
	if (!set)
		goto error;
	lp = wrap_constraints(set);
	obj = isl_vec_alloc(ctx, 1 + dim*set->n);
	if (!obj)
		goto error;
	isl_int_set_si(obj->block.data[0], 0);
	for (i = 0; i < set->n; ++i) {
		isl_seq_clr(obj->block.data + 1 + dim*i, 2);
		isl_int_set_si(obj->block.data[1 + dim*i+2], 1);
		isl_seq_clr(obj->block.data + 1 + dim*i+3, dim-3);
	}
	isl_int_init(num);
	isl_int_init(den);
	res = isl_basic_set_solve_lp(lp, 0,
			    obj->block.data, ctx->one, &num, &den, NULL);
	if (res == isl_lp_ok) {
		isl_int_neg(num, num);
		isl_seq_combine(facet, num, facet, den, ridge, dim);
		isl_seq_normalize(ctx, facet, dim);
	}
	isl_int_clear(num);
	isl_int_clear(den);
	isl_vec_free(obj);
	isl_basic_set_free(lp);
	isl_set_free(set);
	if (res == isl_lp_error)
		return NULL;
	isl_assert(ctx, res == isl_lp_ok || res == isl_lp_unbounded, 
		   return NULL);
	return facet;
error:
	isl_basic_set_free(lp);
	isl_mat_free(T);
	isl_set_free(set);
	return NULL;
}

/* Compute the constraint of a facet of "set".
 *
 * We first compute the intersection with a bounding constraint
 * that is orthogonal to one of the coordinate axes.
 * If the affine hull of this intersection has only one equality,
 * we have found a facet.
 * Otherwise, we wrap the current bounding constraint around
 * one of the equalities of the face (one that is not equal to
 * the current bounding constraint).
 * This process continues until we have found a facet.
 * The dimension of the intersection increases by at least
 * one on each iteration, so termination is guaranteed.
 */
static __isl_give isl_mat *initial_facet_constraint(__isl_keep isl_set *set)
{
	struct isl_set *slice = NULL;
	struct isl_basic_set *face = NULL;
	int i;
	unsigned dim = isl_set_n_dim(set);
	int is_bound;
	isl_mat *bounds;

	isl_assert(set->ctx, set->n > 0, goto error);
	bounds = isl_mat_alloc(set->ctx, 1, 1 + dim);
	if (!bounds)
		return NULL;

	isl_seq_clr(bounds->row[0], dim);
	isl_int_set_si(bounds->row[0][1 + dim - 1], 1);
	is_bound = uset_is_bound(set, bounds->row[0], 1 + dim);
	if (is_bound < 0)
		goto error;
	isl_assert(set->ctx, is_bound, goto error);
	isl_seq_normalize(set->ctx, bounds->row[0], 1 + dim);
	bounds->n_row = 1;

	for (;;) {
		slice = isl_set_copy(set);
		slice = isl_set_add_basic_set_equality(slice, bounds->row[0]);
		face = isl_set_affine_hull(slice);
		if (!face)
			goto error;
		if (face->n_eq == 1) {
			isl_basic_set_free(face);
			break;
		}
		for (i = 0; i < face->n_eq; ++i)
			if (!isl_seq_eq(bounds->row[0], face->eq[i], 1 + dim) &&
			    !isl_seq_is_neg(bounds->row[0],
						face->eq[i], 1 + dim))
				break;
		isl_assert(set->ctx, i < face->n_eq, goto error);
		if (!isl_set_wrap_facet(set, bounds->row[0], face->eq[i]))
			goto error;
		isl_seq_normalize(set->ctx, bounds->row[0], bounds->n_col);
		isl_basic_set_free(face);
	}

	return bounds;
error:
	isl_basic_set_free(face);
	isl_mat_free(bounds);
	return NULL;
}

/* Given the bounding constraint "c" of a facet of the convex hull of "set",
 * compute a hyperplane description of the facet, i.e., compute the facets
 * of the facet.
 *
 * We compute an affine transformation that transforms the constraint
 *
 *			  [ 1 ]
 *			c [ x ] = 0
 *
 * to the constraint
 *
 *			   z_1  = 0
 *
 * by computing the right inverse U of a matrix that starts with the rows
 *
 *			[ 1 0 ]
 *			[  c  ]
 *
 * Then
 *			[ 1 ]     [ 1 ]
 *			[ x ] = U [ z ]
 * and
 *			[ 1 ]     [ 1 ]
 *			[ z ] = Q [ x ]
 *
 * with Q = U^{-1}
 * Since z_1 is zero, we can drop this variable as well as the corresponding
 * column of U to obtain
 *
 *			[ 1 ]      [ 1  ]
 *			[ x ] = U' [ z' ]
 * and
 *			[ 1  ]      [ 1 ]
 *			[ z' ] = Q' [ x ]
 *
 * with Q' equal to Q, but without the corresponding row.
 * After computing the facets of the facet in the z' space,
 * we convert them back to the x space through Q.
 */
static struct isl_basic_set *compute_facet(struct isl_set *set, isl_int *c)
{
	struct isl_mat *m, *U, *Q;
	struct isl_basic_set *facet = NULL;
	struct isl_ctx *ctx;
	unsigned dim;

	ctx = set->ctx;
	set = isl_set_copy(set);
	dim = isl_set_n_dim(set);
	m = isl_mat_alloc(set->ctx, 2, 1 + dim);
	if (!m)
		goto error;
	isl_int_set_si(m->row[0][0], 1);
	isl_seq_clr(m->row[0]+1, dim);
	isl_seq_cpy(m->row[1], c, 1+dim);
	U = isl_mat_right_inverse(m);
	Q = isl_mat_right_inverse(isl_mat_copy(U));
	U = isl_mat_drop_cols(U, 1, 1);
	Q = isl_mat_drop_rows(Q, 1, 1);
	set = isl_set_preimage(set, U);
	facet = uset_convex_hull_wrap_bounded(set);
	facet = isl_basic_set_preimage(facet, Q);
	if (facet)
		isl_assert(ctx, facet->n_eq == 0, goto error);
	return facet;
error:
	isl_basic_set_free(facet);
	isl_set_free(set);
	return NULL;
}

/* Given an initial facet constraint, compute the remaining facets.
 * We do this by running through all facets found so far and computing
 * the adjacent facets through wrapping, adding those facets that we
 * hadn't already found before.
 *
 * For each facet we have found so far, we first compute its facets
 * in the resulting convex hull.  That is, we compute the ridges
 * of the resulting convex hull contained in the facet.
 * We also compute the corresponding facet in the current approximation
 * of the convex hull.  There is no need to wrap around the ridges
 * in this facet since that would result in a facet that is already
 * present in the current approximation.
 *
 * This function can still be significantly optimized by checking which of
 * the facets of the basic sets are also facets of the convex hull and
 * using all the facets so far to help in constructing the facets of the
 * facets
 * and/or
 * using the technique in section "3.1 Ridge Generation" of
 * "Extended Convex Hull" by Fukuda et al.
 */
static struct isl_basic_set *extend(struct isl_basic_set *hull,
	struct isl_set *set)
{
	int i, j, f;
	int k;
	struct isl_basic_set *facet = NULL;
	struct isl_basic_set *hull_facet = NULL;
	unsigned dim;

	if (!hull)
		return NULL;

	isl_assert(set->ctx, set->n > 0, goto error);

	dim = isl_set_n_dim(set);

	for (i = 0; i < hull->n_ineq; ++i) {
		facet = compute_facet(set, hull->ineq[i]);
		facet = isl_basic_set_add_equality(facet, hull->ineq[i]);
		facet = isl_basic_set_gauss(facet, NULL);
		facet = isl_basic_set_normalize_constraints(facet);
		hull_facet = isl_basic_set_copy(hull);
		hull_facet = isl_basic_set_add_equality(hull_facet, hull->ineq[i]);
		hull_facet = isl_basic_set_gauss(hull_facet, NULL);
		hull_facet = isl_basic_set_normalize_constraints(hull_facet);
		if (!facet || !hull_facet)
			goto error;
		hull = isl_basic_set_cow(hull);
		hull = isl_basic_set_extend_space(hull,
			isl_space_copy(hull->dim), 0, 0, facet->n_ineq);
		if (!hull)
			goto error;
		for (j = 0; j < facet->n_ineq; ++j) {
			for (f = 0; f < hull_facet->n_ineq; ++f)
				if (isl_seq_eq(facet->ineq[j],
						hull_facet->ineq[f], 1 + dim))
					break;
			if (f < hull_facet->n_ineq)
				continue;
			k = isl_basic_set_alloc_inequality(hull);
			if (k < 0)
				goto error;
			isl_seq_cpy(hull->ineq[k], hull->ineq[i], 1+dim);
			if (!isl_set_wrap_facet(set, hull->ineq[k], facet->ineq[j]))
				goto error;
		}
		isl_basic_set_free(hull_facet);
		isl_basic_set_free(facet);
	}
	hull = isl_basic_set_simplify(hull);
	hull = isl_basic_set_finalize(hull);
	return hull;
error:
	isl_basic_set_free(hull_facet);
	isl_basic_set_free(facet);
	isl_basic_set_free(hull);
	return NULL;
}

/* Special case for computing the convex hull of a one dimensional set.
 * We simply collect the lower and upper bounds of each basic set
 * and the biggest of those.
 */
static struct isl_basic_set *convex_hull_1d(struct isl_set *set)
{
	struct isl_mat *c = NULL;
	isl_int *lower = NULL;
	isl_int *upper = NULL;
	int i, j, k;
	isl_int a, b;
	struct isl_basic_set *hull;

	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_basic_set_simplify(set->p[i]);
		if (!set->p[i])
			goto error;
	}
	set = isl_set_remove_empty_parts(set);
	if (!set)
		goto error;
	isl_assert(set->ctx, set->n > 0, goto error);
	c = isl_mat_alloc(set->ctx, 2, 2);
	if (!c)
		goto error;

	if (set->p[0]->n_eq > 0) {
		isl_assert(set->ctx, set->p[0]->n_eq == 1, goto error);
		lower = c->row[0];
		upper = c->row[1];
		if (isl_int_is_pos(set->p[0]->eq[0][1])) {
			isl_seq_cpy(lower, set->p[0]->eq[0], 2);
			isl_seq_neg(upper, set->p[0]->eq[0], 2);
		} else {
			isl_seq_neg(lower, set->p[0]->eq[0], 2);
			isl_seq_cpy(upper, set->p[0]->eq[0], 2);
		}
	} else {
		for (j = 0; j < set->p[0]->n_ineq; ++j) {
			if (isl_int_is_pos(set->p[0]->ineq[j][1])) {
				lower = c->row[0];
				isl_seq_cpy(lower, set->p[0]->ineq[j], 2);
			} else {
				upper = c->row[1];
				isl_seq_cpy(upper, set->p[0]->ineq[j], 2);
			}
		}
	}

	isl_int_init(a);
	isl_int_init(b);
	for (i = 0; i < set->n; ++i) {
		struct isl_basic_set *bset = set->p[i];
		int has_lower = 0;
		int has_upper = 0;

		for (j = 0; j < bset->n_eq; ++j) {
			has_lower = 1;
			has_upper = 1;
			if (lower) {
				isl_int_mul(a, lower[0], bset->eq[j][1]);
				isl_int_mul(b, lower[1], bset->eq[j][0]);
				if (isl_int_lt(a, b) && isl_int_is_pos(bset->eq[j][1]))
					isl_seq_cpy(lower, bset->eq[j], 2);
				if (isl_int_gt(a, b) && isl_int_is_neg(bset->eq[j][1]))
					isl_seq_neg(lower, bset->eq[j], 2);
			}
			if (upper) {
				isl_int_mul(a, upper[0], bset->eq[j][1]);
				isl_int_mul(b, upper[1], bset->eq[j][0]);
				if (isl_int_lt(a, b) && isl_int_is_pos(bset->eq[j][1]))
					isl_seq_neg(upper, bset->eq[j], 2);
				if (isl_int_gt(a, b) && isl_int_is_neg(bset->eq[j][1]))
					isl_seq_cpy(upper, bset->eq[j], 2);
			}
		}
		for (j = 0; j < bset->n_ineq; ++j) {
			if (isl_int_is_pos(bset->ineq[j][1]))
				has_lower = 1;
			if (isl_int_is_neg(bset->ineq[j][1]))
				has_upper = 1;
			if (lower && isl_int_is_pos(bset->ineq[j][1])) {
				isl_int_mul(a, lower[0], bset->ineq[j][1]);
				isl_int_mul(b, lower[1], bset->ineq[j][0]);
				if (isl_int_lt(a, b))
					isl_seq_cpy(lower, bset->ineq[j], 2);
			}
			if (upper && isl_int_is_neg(bset->ineq[j][1])) {
				isl_int_mul(a, upper[0], bset->ineq[j][1]);
				isl_int_mul(b, upper[1], bset->ineq[j][0]);
				if (isl_int_gt(a, b))
					isl_seq_cpy(upper, bset->ineq[j], 2);
			}
		}
		if (!has_lower)
			lower = NULL;
		if (!has_upper)
			upper = NULL;
	}
	isl_int_clear(a);
	isl_int_clear(b);

	hull = isl_basic_set_alloc(set->ctx, 0, 1, 0, 0, 2);
	hull = isl_basic_set_set_rational(hull);
	if (!hull)
		goto error;
	if (lower) {
		k = isl_basic_set_alloc_inequality(hull);
		isl_seq_cpy(hull->ineq[k], lower, 2);
	}
	if (upper) {
		k = isl_basic_set_alloc_inequality(hull);
		isl_seq_cpy(hull->ineq[k], upper, 2);
	}
	hull = isl_basic_set_finalize(hull);
	isl_set_free(set);
	isl_mat_free(c);
	return hull;
error:
	isl_set_free(set);
	isl_mat_free(c);
	return NULL;
}

static struct isl_basic_set *convex_hull_0d(struct isl_set *set)
{
	struct isl_basic_set *convex_hull;

	if (!set)
		return NULL;

	if (isl_set_is_empty(set))
		convex_hull = isl_basic_set_empty(isl_space_copy(set->dim));
	else
		convex_hull = isl_basic_set_universe(isl_space_copy(set->dim));
	isl_set_free(set);
	return convex_hull;
}

/* Compute the convex hull of a pair of basic sets without any parameters or
 * integer divisions using Fourier-Motzkin elimination.
 * The convex hull is the set of all points that can be written as
 * the sum of points from both basic sets (in homogeneous coordinates).
 * We set up the constraints in a space with dimensions for each of
 * the three sets and then project out the dimensions corresponding
 * to the two original basic sets, retaining only those corresponding
 * to the convex hull.
 */
static struct isl_basic_set *convex_hull_pair_elim(struct isl_basic_set *bset1,
	struct isl_basic_set *bset2)
{
	int i, j, k;
	struct isl_basic_set *bset[2];
	struct isl_basic_set *hull = NULL;
	unsigned dim;

	if (!bset1 || !bset2)
		goto error;

	dim = isl_basic_set_n_dim(bset1);
	hull = isl_basic_set_alloc(bset1->ctx, 0, 2 + 3 * dim, 0,
				1 + dim + bset1->n_eq + bset2->n_eq,
				2 + bset1->n_ineq + bset2->n_ineq);
	bset[0] = bset1;
	bset[1] = bset2;
	for (i = 0; i < 2; ++i) {
		for (j = 0; j < bset[i]->n_eq; ++j) {
			k = isl_basic_set_alloc_equality(hull);
			if (k < 0)
				goto error;
			isl_seq_clr(hull->eq[k], (i+1) * (1+dim));
			isl_seq_clr(hull->eq[k]+(i+2)*(1+dim), (1-i)*(1+dim));
			isl_seq_cpy(hull->eq[k]+(i+1)*(1+dim), bset[i]->eq[j],
					1+dim);
		}
		for (j = 0; j < bset[i]->n_ineq; ++j) {
			k = isl_basic_set_alloc_inequality(hull);
			if (k < 0)
				goto error;
			isl_seq_clr(hull->ineq[k], (i+1) * (1+dim));
			isl_seq_clr(hull->ineq[k]+(i+2)*(1+dim), (1-i)*(1+dim));
			isl_seq_cpy(hull->ineq[k]+(i+1)*(1+dim),
					bset[i]->ineq[j], 1+dim);
		}
		k = isl_basic_set_alloc_inequality(hull);
		if (k < 0)
			goto error;
		isl_seq_clr(hull->ineq[k], 1+2+3*dim);
		isl_int_set_si(hull->ineq[k][(i+1)*(1+dim)], 1);
	}
	for (j = 0; j < 1+dim; ++j) {
		k = isl_basic_set_alloc_equality(hull);
		if (k < 0)
			goto error;
		isl_seq_clr(hull->eq[k], 1+2+3*dim);
		isl_int_set_si(hull->eq[k][j], -1);
		isl_int_set_si(hull->eq[k][1+dim+j], 1);
		isl_int_set_si(hull->eq[k][2*(1+dim)+j], 1);
	}
	hull = isl_basic_set_set_rational(hull);
	hull = isl_basic_set_remove_dims(hull, isl_dim_set, dim, 2*(1+dim));
	hull = isl_basic_set_remove_redundancies(hull);
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return hull;
error:
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	isl_basic_set_free(hull);
	return NULL;
}

/* Is the set bounded for each value of the parameters?
 */
int isl_basic_set_is_bounded(__isl_keep isl_basic_set *bset)
{
	struct isl_tab *tab;
	int bounded;

	if (!bset)
		return -1;
	if (isl_basic_set_plain_is_empty(bset))
		return 1;

	tab = isl_tab_from_recession_cone(bset, 1);
	bounded = isl_tab_cone_is_bounded(tab);
	isl_tab_free(tab);
	return bounded;
}

/* Is the image bounded for each value of the parameters and
 * the domain variables?
 */
int isl_basic_map_image_is_bounded(__isl_keep isl_basic_map *bmap)
{
	unsigned nparam = isl_basic_map_dim(bmap, isl_dim_param);
	unsigned n_in = isl_basic_map_dim(bmap, isl_dim_in);
	int bounded;

	bmap = isl_basic_map_copy(bmap);
	bmap = isl_basic_map_cow(bmap);
	bmap = isl_basic_map_move_dims(bmap, isl_dim_param, nparam,
					isl_dim_in, 0, n_in);
	bounded = isl_basic_set_is_bounded((isl_basic_set *)bmap);
	isl_basic_map_free(bmap);

	return bounded;
}

/* Is the set bounded for each value of the parameters?
 */
int isl_set_is_bounded(__isl_keep isl_set *set)
{
	int i;

	if (!set)
		return -1;

	for (i = 0; i < set->n; ++i) {
		int bounded = isl_basic_set_is_bounded(set->p[i]);
		if (!bounded || bounded < 0)
			return bounded;
	}
	return 1;
}

/* Compute the lineality space of the convex hull of bset1 and bset2.
 *
 * We first compute the intersection of the recession cone of bset1
 * with the negative of the recession cone of bset2 and then compute
 * the linear hull of the resulting cone.
 */
static struct isl_basic_set *induced_lineality_space(
	struct isl_basic_set *bset1, struct isl_basic_set *bset2)
{
	int i, k;
	struct isl_basic_set *lin = NULL;
	unsigned dim;

	if (!bset1 || !bset2)
		goto error;

	dim = isl_basic_set_total_dim(bset1);
	lin = isl_basic_set_alloc_space(isl_basic_set_get_space(bset1), 0,
					bset1->n_eq + bset2->n_eq,
					bset1->n_ineq + bset2->n_ineq);
	lin = isl_basic_set_set_rational(lin);
	if (!lin)
		goto error;
	for (i = 0; i < bset1->n_eq; ++i) {
		k = isl_basic_set_alloc_equality(lin);
		if (k < 0)
			goto error;
		isl_int_set_si(lin->eq[k][0], 0);
		isl_seq_cpy(lin->eq[k] + 1, bset1->eq[i] + 1, dim);
	}
	for (i = 0; i < bset1->n_ineq; ++i) {
		k = isl_basic_set_alloc_inequality(lin);
		if (k < 0)
			goto error;
		isl_int_set_si(lin->ineq[k][0], 0);
		isl_seq_cpy(lin->ineq[k] + 1, bset1->ineq[i] + 1, dim);
	}
	for (i = 0; i < bset2->n_eq; ++i) {
		k = isl_basic_set_alloc_equality(lin);
		if (k < 0)
			goto error;
		isl_int_set_si(lin->eq[k][0], 0);
		isl_seq_neg(lin->eq[k] + 1, bset2->eq[i] + 1, dim);
	}
	for (i = 0; i < bset2->n_ineq; ++i) {
		k = isl_basic_set_alloc_inequality(lin);
		if (k < 0)
			goto error;
		isl_int_set_si(lin->ineq[k][0], 0);
		isl_seq_neg(lin->ineq[k] + 1, bset2->ineq[i] + 1, dim);
	}

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return isl_basic_set_affine_hull(lin);
error:
	isl_basic_set_free(lin);
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return NULL;
}

static struct isl_basic_set *uset_convex_hull(struct isl_set *set);

/* Given a set and a linear space "lin" of dimension n > 0,
 * project the linear space from the set, compute the convex hull
 * and then map the set back to the original space.
 *
 * Let
 *
 *	M x = 0
 *
 * describe the linear space.  We first compute the Hermite normal
 * form H = M U of M = H Q, to obtain
 *
 *	H Q x = 0
 *
 * The last n rows of H will be zero, so the last n variables of x' = Q x
 * are the one we want to project out.  We do this by transforming each
 * basic set A x >= b to A U x' >= b and then removing the last n dimensions.
 * After computing the convex hull in x'_1, i.e., A' x'_1 >= b',
 * we transform the hull back to the original space as A' Q_1 x >= b',
 * with Q_1 all but the last n rows of Q.
 */
static struct isl_basic_set *modulo_lineality(struct isl_set *set,
	struct isl_basic_set *lin)
{
	unsigned total = isl_basic_set_total_dim(lin);
	unsigned lin_dim;
	struct isl_basic_set *hull;
	struct isl_mat *M, *U, *Q;

	if (!set || !lin)
		goto error;
	lin_dim = total - lin->n_eq;
	M = isl_mat_sub_alloc6(set->ctx, lin->eq, 0, lin->n_eq, 1, total);
	M = isl_mat_left_hermite(M, 0, &U, &Q);
	if (!M)
		goto error;
	isl_mat_free(M);
	isl_basic_set_free(lin);

	Q = isl_mat_drop_rows(Q, Q->n_row - lin_dim, lin_dim);

	U = isl_mat_lin_to_aff(U);
	Q = isl_mat_lin_to_aff(Q);

	set = isl_set_preimage(set, U);
	set = isl_set_remove_dims(set, isl_dim_set, total - lin_dim, lin_dim);
	hull = uset_convex_hull(set);
	hull = isl_basic_set_preimage(hull, Q);

	return hull;
error:
	isl_basic_set_free(lin);
	isl_set_free(set);
	return NULL;
}

/* Given two polyhedra with as constraints h_{ij} x >= 0 in homegeneous space,
 * set up an LP for solving
 *
 *	\sum_j \alpha_{1j} h_{1j} = \sum_j \alpha_{2j} h_{2j}
 *
 * \alpha{i0} corresponds to the (implicit) positivity constraint 1 >= 0
 * The next \alpha{ij} correspond to the equalities and come in pairs.
 * The final \alpha{ij} correspond to the inequalities.
 */
static struct isl_basic_set *valid_direction_lp(
	struct isl_basic_set *bset1, struct isl_basic_set *bset2)
{
	isl_space *dim;
	struct isl_basic_set *lp;
	unsigned d;
	int n;
	int i, j, k;

	if (!bset1 || !bset2)
		goto error;
	d = 1 + isl_basic_set_total_dim(bset1);
	n = 2 +
	    2 * bset1->n_eq + bset1->n_ineq + 2 * bset2->n_eq + bset2->n_ineq;
	dim = isl_space_set_alloc(bset1->ctx, 0, n);
	lp = isl_basic_set_alloc_space(dim, 0, d, n);
	if (!lp)
		goto error;
	for (i = 0; i < n; ++i) {
		k = isl_basic_set_alloc_inequality(lp);
		if (k < 0)
			goto error;
		isl_seq_clr(lp->ineq[k] + 1, n);
		isl_int_set_si(lp->ineq[k][0], -1);
		isl_int_set_si(lp->ineq[k][1 + i], 1);
	}
	for (i = 0; i < d; ++i) {
		k = isl_basic_set_alloc_equality(lp);
		if (k < 0)
			goto error;
		n = 0;
		isl_int_set_si(lp->eq[k][n], 0); n++;
		/* positivity constraint 1 >= 0 */
		isl_int_set_si(lp->eq[k][n], i == 0); n++;
		for (j = 0; j < bset1->n_eq; ++j) {
			isl_int_set(lp->eq[k][n], bset1->eq[j][i]); n++;
			isl_int_neg(lp->eq[k][n], bset1->eq[j][i]); n++;
		}
		for (j = 0; j < bset1->n_ineq; ++j) {
			isl_int_set(lp->eq[k][n], bset1->ineq[j][i]); n++;
		}
		/* positivity constraint 1 >= 0 */
		isl_int_set_si(lp->eq[k][n], -(i == 0)); n++;
		for (j = 0; j < bset2->n_eq; ++j) {
			isl_int_neg(lp->eq[k][n], bset2->eq[j][i]); n++;
			isl_int_set(lp->eq[k][n], bset2->eq[j][i]); n++;
		}
		for (j = 0; j < bset2->n_ineq; ++j) {
			isl_int_neg(lp->eq[k][n], bset2->ineq[j][i]); n++;
		}
	}
	lp = isl_basic_set_gauss(lp, NULL);
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return lp;
error:
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return NULL;
}

/* Compute a vector s in the homogeneous space such that <s, r> > 0
 * for all rays in the homogeneous space of the two cones that correspond
 * to the input polyhedra bset1 and bset2.
 *
 * We compute s as a vector that satisfies
 *
 *	s = \sum_j \alpha_{ij} h_{ij}	for i = 1,2			(*)
 *
 * with h_{ij} the normals of the facets of polyhedron i
 * (including the "positivity constraint" 1 >= 0) and \alpha_{ij}
 * strictly positive numbers.  For simplicity we impose \alpha_{ij} >= 1.
 * We first set up an LP with as variables the \alpha{ij}.
 * In this formulation, for each polyhedron i,
 * the first constraint is the positivity constraint, followed by pairs
 * of variables for the equalities, followed by variables for the inequalities.
 * We then simply pick a feasible solution and compute s using (*).
 *
 * Note that we simply pick any valid direction and make no attempt
 * to pick a "good" or even the "best" valid direction.
 */
static struct isl_vec *valid_direction(
	struct isl_basic_set *bset1, struct isl_basic_set *bset2)
{
	struct isl_basic_set *lp;
	struct isl_tab *tab;
	struct isl_vec *sample = NULL;
	struct isl_vec *dir;
	unsigned d;
	int i;
	int n;

	if (!bset1 || !bset2)
		goto error;
	lp = valid_direction_lp(isl_basic_set_copy(bset1),
				isl_basic_set_copy(bset2));
	tab = isl_tab_from_basic_set(lp, 0);
	sample = isl_tab_get_sample_value(tab);
	isl_tab_free(tab);
	isl_basic_set_free(lp);
	if (!sample)
		goto error;
	d = isl_basic_set_total_dim(bset1);
	dir = isl_vec_alloc(bset1->ctx, 1 + d);
	if (!dir)
		goto error;
	isl_seq_clr(dir->block.data + 1, dir->size - 1);
	n = 1;
	/* positivity constraint 1 >= 0 */
	isl_int_set(dir->block.data[0], sample->block.data[n]); n++;
	for (i = 0; i < bset1->n_eq; ++i) {
		isl_int_sub(sample->block.data[n],
			    sample->block.data[n], sample->block.data[n+1]);
		isl_seq_combine(dir->block.data,
				bset1->ctx->one, dir->block.data,
				sample->block.data[n], bset1->eq[i], 1 + d);

		n += 2;
	}
	for (i = 0; i < bset1->n_ineq; ++i)
		isl_seq_combine(dir->block.data,
				bset1->ctx->one, dir->block.data,
				sample->block.data[n++], bset1->ineq[i], 1 + d);
	isl_vec_free(sample);
	isl_seq_normalize(bset1->ctx, dir->el, dir->size);
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return dir;
error:
	isl_vec_free(sample);
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return NULL;
}

/* Given a polyhedron b_i + A_i x >= 0 and a map T = S^{-1},
 * compute b_i' + A_i' x' >= 0, with
 *
 *	[ b_i A_i ]        [ y' ]		              [ y' ]
 *	[  1   0  ] S^{-1} [ x' ] >= 0	or	[ b_i' A_i' ] [ x' ] >= 0
 *
 * In particular, add the "positivity constraint" and then perform
 * the mapping.
 */
static struct isl_basic_set *homogeneous_map(struct isl_basic_set *bset,
	struct isl_mat *T)
{
	int k;

	if (!bset)
		goto error;
	bset = isl_basic_set_extend_constraints(bset, 0, 1);
	k = isl_basic_set_alloc_inequality(bset);
	if (k < 0)
		goto error;
	isl_seq_clr(bset->ineq[k] + 1, isl_basic_set_total_dim(bset));
	isl_int_set_si(bset->ineq[k][0], 1);
	bset = isl_basic_set_preimage(bset, T);
	return bset;
error:
	isl_mat_free(T);
	isl_basic_set_free(bset);
	return NULL;
}

/* Compute the convex hull of a pair of basic sets without any parameters or
 * integer divisions, where the convex hull is known to be pointed,
 * but the basic sets may be unbounded.
 *
 * We turn this problem into the computation of a convex hull of a pair
 * _bounded_ polyhedra by "changing the direction of the homogeneous
 * dimension".  This idea is due to Matthias Koeppe.
 *
 * Consider the cones in homogeneous space that correspond to the
 * input polyhedra.  The rays of these cones are also rays of the
 * polyhedra if the coordinate that corresponds to the homogeneous
 * dimension is zero.  That is, if the inner product of the rays
 * with the homogeneous direction is zero.
 * The cones in the homogeneous space can also be considered to
 * correspond to other pairs of polyhedra by chosing a different
 * homogeneous direction.  To ensure that both of these polyhedra
 * are bounded, we need to make sure that all rays of the cones
 * correspond to vertices and not to rays.
 * Let s be a direction such that <s, r> > 0 for all rays r of both cones.
 * Then using s as a homogeneous direction, we obtain a pair of polytopes.
 * The vector s is computed in valid_direction.
 *
 * Note that we need to consider _all_ rays of the cones and not just
 * the rays that correspond to rays in the polyhedra.  If we were to
 * only consider those rays and turn them into vertices, then we
 * may inadvertently turn some vertices into rays.
 *
 * The standard homogeneous direction is the unit vector in the 0th coordinate.
 * We therefore transform the two polyhedra such that the selected
 * direction is mapped onto this standard direction and then proceed
 * with the normal computation.
 * Let S be a non-singular square matrix with s as its first row,
 * then we want to map the polyhedra to the space
 *
 *	[ y' ]     [ y ]		[ y ]          [ y' ]
 *	[ x' ] = S [ x ]	i.e.,	[ x ] = S^{-1} [ x' ]
 *
 * We take S to be the unimodular completion of s to limit the growth
 * of the coefficients in the following computations.
 *
 * Let b_i + A_i x >= 0 be the constraints of polyhedron i.
 * We first move to the homogeneous dimension
 *
 *	b_i y + A_i x >= 0		[ b_i A_i ] [ y ]    [ 0 ]
 *	    y         >= 0	or	[  1   0  ] [ x ] >= [ 0 ]
 *
 * Then we change directoin
 *
 *	[ b_i A_i ]        [ y' ]		              [ y' ]
 *	[  1   0  ] S^{-1} [ x' ] >= 0	or	[ b_i' A_i' ] [ x' ] >= 0
 *
 * Then we compute the convex hull of the polytopes b_i' + A_i' x' >= 0
 * resulting in b' + A' x' >= 0, which we then convert back
 *
 *	            [ y ]		        [ y ]
 *	[ b' A' ] S [ x ] >= 0	or	[ b A ] [ x ] >= 0
 *
 * The polyhedron b + A x >= 0 is then the convex hull of the input polyhedra.
 */
static struct isl_basic_set *convex_hull_pair_pointed(
	struct isl_basic_set *bset1, struct isl_basic_set *bset2)
{
	struct isl_ctx *ctx = NULL;
	struct isl_vec *dir = NULL;
	struct isl_mat *T = NULL;
	struct isl_mat *T2 = NULL;
	struct isl_basic_set *hull;
	struct isl_set *set;

	if (!bset1 || !bset2)
		goto error;
	ctx = bset1->ctx;
	dir = valid_direction(isl_basic_set_copy(bset1),
				isl_basic_set_copy(bset2));
	if (!dir)
		goto error;
	T = isl_mat_alloc(bset1->ctx, dir->size, dir->size);
	if (!T)
		goto error;
	isl_seq_cpy(T->row[0], dir->block.data, dir->size);
	T = isl_mat_unimodular_complete(T, 1);
	T2 = isl_mat_right_inverse(isl_mat_copy(T));

	bset1 = homogeneous_map(bset1, isl_mat_copy(T2));
	bset2 = homogeneous_map(bset2, T2);
	set = isl_set_alloc_space(isl_basic_set_get_space(bset1), 2, 0);
	set = isl_set_add_basic_set(set, bset1);
	set = isl_set_add_basic_set(set, bset2);
	hull = uset_convex_hull(set);
	hull = isl_basic_set_preimage(hull, T);
	 
	isl_vec_free(dir);

	return hull;
error:
	isl_vec_free(dir);
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return NULL;
}

static struct isl_basic_set *uset_convex_hull_wrap(struct isl_set *set);
static struct isl_basic_set *modulo_affine_hull(
	struct isl_set *set, struct isl_basic_set *affine_hull);

/* Compute the convex hull of a pair of basic sets without any parameters or
 * integer divisions.
 *
 * This function is called from uset_convex_hull_unbounded, which
 * means that the complete convex hull is unbounded.  Some pairs
 * of basic sets may still be bounded, though.
 * They may even lie inside a lower dimensional space, in which
 * case they need to be handled inside their affine hull since
 * the main algorithm assumes that the result is full-dimensional.
 *
 * If the convex hull of the two basic sets would have a non-trivial
 * lineality space, we first project out this lineality space.
 */
static struct isl_basic_set *convex_hull_pair(struct isl_basic_set *bset1,
	struct isl_basic_set *bset2)
{
	isl_basic_set *lin, *aff;
	int bounded1, bounded2;

	if (bset1->ctx->opt->convex == ISL_CONVEX_HULL_FM)
		return convex_hull_pair_elim(bset1, bset2);

	aff = isl_set_affine_hull(isl_basic_set_union(isl_basic_set_copy(bset1),
						    isl_basic_set_copy(bset2)));
	if (!aff)
		goto error;
	if (aff->n_eq != 0) 
		return modulo_affine_hull(isl_basic_set_union(bset1, bset2), aff);
	isl_basic_set_free(aff);

	bounded1 = isl_basic_set_is_bounded(bset1);
	bounded2 = isl_basic_set_is_bounded(bset2);

	if (bounded1 < 0 || bounded2 < 0)
		goto error;

	if (bounded1 && bounded2)
		uset_convex_hull_wrap(isl_basic_set_union(bset1, bset2));

	if (bounded1 || bounded2)
		return convex_hull_pair_pointed(bset1, bset2);

	lin = induced_lineality_space(isl_basic_set_copy(bset1),
				      isl_basic_set_copy(bset2));
	if (!lin)
		goto error;
	if (isl_basic_set_is_universe(lin)) {
		isl_basic_set_free(bset1);
		isl_basic_set_free(bset2);
		return lin;
	}
	if (lin->n_eq < isl_basic_set_total_dim(lin)) {
		struct isl_set *set;
		set = isl_set_alloc_space(isl_basic_set_get_space(bset1), 2, 0);
		set = isl_set_add_basic_set(set, bset1);
		set = isl_set_add_basic_set(set, bset2);
		return modulo_lineality(set, lin);
	}
	isl_basic_set_free(lin);

	return convex_hull_pair_pointed(bset1, bset2);
error:
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	return NULL;
}

/* Compute the lineality space of a basic set.
 * We currently do not allow the basic set to have any divs.
 * We basically just drop the constants and turn every inequality
 * into an equality.
 */
struct isl_basic_set *isl_basic_set_lineality_space(struct isl_basic_set *bset)
{
	int i, k;
	struct isl_basic_set *lin = NULL;
	unsigned dim;

	if (!bset)
		goto error;
	isl_assert(bset->ctx, bset->n_div == 0, goto error);
	dim = isl_basic_set_total_dim(bset);

	lin = isl_basic_set_alloc_space(isl_basic_set_get_space(bset), 0, dim, 0);
	if (!lin)
		goto error;
	for (i = 0; i < bset->n_eq; ++i) {
		k = isl_basic_set_alloc_equality(lin);
		if (k < 0)
			goto error;
		isl_int_set_si(lin->eq[k][0], 0);
		isl_seq_cpy(lin->eq[k] + 1, bset->eq[i] + 1, dim);
	}
	lin = isl_basic_set_gauss(lin, NULL);
	if (!lin)
		goto error;
	for (i = 0; i < bset->n_ineq && lin->n_eq < dim; ++i) {
		k = isl_basic_set_alloc_equality(lin);
		if (k < 0)
			goto error;
		isl_int_set_si(lin->eq[k][0], 0);
		isl_seq_cpy(lin->eq[k] + 1, bset->ineq[i] + 1, dim);
		lin = isl_basic_set_gauss(lin, NULL);
		if (!lin)
			goto error;
	}
	isl_basic_set_free(bset);
	return lin;
error:
	isl_basic_set_free(lin);
	isl_basic_set_free(bset);
	return NULL;
}

/* Compute the (linear) hull of the lineality spaces of the basic sets in the
 * "underlying" set "set".
 */
static struct isl_basic_set *uset_combined_lineality_space(struct isl_set *set)
{
	int i;
	struct isl_set *lin = NULL;

	if (!set)
		return NULL;
	if (set->n == 0) {
		isl_space *dim = isl_set_get_space(set);
		isl_set_free(set);
		return isl_basic_set_empty(dim);
	}

	lin = isl_set_alloc_space(isl_set_get_space(set), set->n, 0);
	for (i = 0; i < set->n; ++i)
		lin = isl_set_add_basic_set(lin,
		    isl_basic_set_lineality_space(isl_basic_set_copy(set->p[i])));
	isl_set_free(set);
	return isl_set_affine_hull(lin);
}

/* Compute the convex hull of a set without any parameters or
 * integer divisions.
 * In each step, we combined two basic sets until only one
 * basic set is left.
 * The input basic sets are assumed not to have a non-trivial
 * lineality space.  If any of the intermediate results has
 * a non-trivial lineality space, it is projected out.
 */
static struct isl_basic_set *uset_convex_hull_unbounded(struct isl_set *set)
{
	struct isl_basic_set *convex_hull = NULL;

	convex_hull = isl_set_copy_basic_set(set);
	set = isl_set_drop_basic_set(set, convex_hull);
	if (!set)
		goto error;
	while (set->n > 0) {
		struct isl_basic_set *t;
		t = isl_set_copy_basic_set(set);
		if (!t)
			goto error;
		set = isl_set_drop_basic_set(set, t);
		if (!set)
			goto error;
		convex_hull = convex_hull_pair(convex_hull, t);
		if (set->n == 0)
			break;
		t = isl_basic_set_lineality_space(isl_basic_set_copy(convex_hull));
		if (!t)
			goto error;
		if (isl_basic_set_is_universe(t)) {
			isl_basic_set_free(convex_hull);
			convex_hull = t;
			break;
		}
		if (t->n_eq < isl_basic_set_total_dim(t)) {
			set = isl_set_add_basic_set(set, convex_hull);
			return modulo_lineality(set, t);
		}
		isl_basic_set_free(t);
	}
	isl_set_free(set);
	return convex_hull;
error:
	isl_set_free(set);
	isl_basic_set_free(convex_hull);
	return NULL;
}

/* Compute an initial hull for wrapping containing a single initial
 * facet.
 * This function assumes that the given set is bounded.
 */
static struct isl_basic_set *initial_hull(struct isl_basic_set *hull,
	struct isl_set *set)
{
	struct isl_mat *bounds = NULL;
	unsigned dim;
	int k;

	if (!hull)
		goto error;
	bounds = initial_facet_constraint(set);
	if (!bounds)
		goto error;
	k = isl_basic_set_alloc_inequality(hull);
	if (k < 0)
		goto error;
	dim = isl_set_n_dim(set);
	isl_assert(set->ctx, 1 + dim == bounds->n_col, goto error);
	isl_seq_cpy(hull->ineq[k], bounds->row[0], bounds->n_col);
	isl_mat_free(bounds);

	return hull;
error:
	isl_basic_set_free(hull);
	isl_mat_free(bounds);
	return NULL;
}

struct max_constraint {
	struct isl_mat *c;
	int	 	count;
	int		ineq;
};

static int max_constraint_equal(const void *entry, const void *val)
{
	struct max_constraint *a = (struct max_constraint *)entry;
	isl_int *b = (isl_int *)val;

	return isl_seq_eq(a->c->row[0] + 1, b, a->c->n_col - 1);
}

static void update_constraint(struct isl_ctx *ctx, struct isl_hash_table *table,
	isl_int *con, unsigned len, int n, int ineq)
{
	struct isl_hash_table_entry *entry;
	struct max_constraint *c;
	uint32_t c_hash;

	c_hash = isl_seq_get_hash(con + 1, len);
	entry = isl_hash_table_find(ctx, table, c_hash, max_constraint_equal,
			con + 1, 0);
	if (!entry)
		return;
	c = entry->data;
	if (c->count < n) {
		isl_hash_table_remove(ctx, table, entry);
		return;
	}
	c->count++;
	if (isl_int_gt(c->c->row[0][0], con[0]))
		return;
	if (isl_int_eq(c->c->row[0][0], con[0])) {
		if (ineq)
			c->ineq = ineq;
		return;
	}
	c->c = isl_mat_cow(c->c);
	isl_int_set(c->c->row[0][0], con[0]);
	c->ineq = ineq;
}

/* Check whether the constraint hash table "table" constains the constraint
 * "con".
 */
static int has_constraint(struct isl_ctx *ctx, struct isl_hash_table *table,
	isl_int *con, unsigned len, int n)
{
	struct isl_hash_table_entry *entry;
	struct max_constraint *c;
	uint32_t c_hash;

	c_hash = isl_seq_get_hash(con + 1, len);
	entry = isl_hash_table_find(ctx, table, c_hash, max_constraint_equal,
			con + 1, 0);
	if (!entry)
		return 0;
	c = entry->data;
	if (c->count < n)
		return 0;
	return isl_int_eq(c->c->row[0][0], con[0]);
}

/* Check for inequality constraints of a basic set without equalities
 * such that the same or more stringent copies of the constraint appear
 * in all of the basic sets.  Such constraints are necessarily facet
 * constraints of the convex hull.
 *
 * If the resulting basic set is by chance identical to one of
 * the basic sets in "set", then we know that this basic set contains
 * all other basic sets and is therefore the convex hull of set.
 * In this case we set *is_hull to 1.
 */
static struct isl_basic_set *common_constraints(struct isl_basic_set *hull,
	struct isl_set *set, int *is_hull)
{
	int i, j, s, n;
	int min_constraints;
	int best;
	struct max_constraint *constraints = NULL;
	struct isl_hash_table *table = NULL;
	unsigned total;

	*is_hull = 0;

	for (i = 0; i < set->n; ++i)
		if (set->p[i]->n_eq == 0)
			break;
	if (i >= set->n)
		return hull;
	min_constraints = set->p[i]->n_ineq;
	best = i;
	for (i = best + 1; i < set->n; ++i) {
		if (set->p[i]->n_eq != 0)
			continue;
		if (set->p[i]->n_ineq >= min_constraints)
			continue;
		min_constraints = set->p[i]->n_ineq;
		best = i;
	}
	constraints = isl_calloc_array(hull->ctx, struct max_constraint,
					min_constraints);
	if (!constraints)
		return hull;
	table = isl_alloc_type(hull->ctx, struct isl_hash_table);
	if (isl_hash_table_init(hull->ctx, table, min_constraints))
		goto error;

	total = isl_space_dim(set->dim, isl_dim_all);
	for (i = 0; i < set->p[best]->n_ineq; ++i) {
		constraints[i].c = isl_mat_sub_alloc6(hull->ctx,
			set->p[best]->ineq + i, 0, 1, 0, 1 + total);
		if (!constraints[i].c)
			goto error;
		constraints[i].ineq = 1;
	}
	for (i = 0; i < min_constraints; ++i) {
		struct isl_hash_table_entry *entry;
		uint32_t c_hash;
		c_hash = isl_seq_get_hash(constraints[i].c->row[0] + 1, total);
		entry = isl_hash_table_find(hull->ctx, table, c_hash,
			max_constraint_equal, constraints[i].c->row[0] + 1, 1);
		if (!entry)
			goto error;
		isl_assert(hull->ctx, !entry->data, goto error);
		entry->data = &constraints[i];
	}

	n = 0;
	for (s = 0; s < set->n; ++s) {
		if (s == best)
			continue;

		for (i = 0; i < set->p[s]->n_eq; ++i) {
			isl_int *eq = set->p[s]->eq[i];
			for (j = 0; j < 2; ++j) {
				isl_seq_neg(eq, eq, 1 + total);
				update_constraint(hull->ctx, table,
							    eq, total, n, 0);
			}
		}
		for (i = 0; i < set->p[s]->n_ineq; ++i) {
			isl_int *ineq = set->p[s]->ineq[i];
			update_constraint(hull->ctx, table, ineq, total, n,
				set->p[s]->n_eq == 0);
		}
		++n;
	}

	for (i = 0; i < min_constraints; ++i) {
		if (constraints[i].count < n)
			continue;
		if (!constraints[i].ineq)
			continue;
		j = isl_basic_set_alloc_inequality(hull);
		if (j < 0)
			goto error;
		isl_seq_cpy(hull->ineq[j], constraints[i].c->row[0], 1 + total);
	}

	for (s = 0; s < set->n; ++s) {
		if (set->p[s]->n_eq)
			continue;
		if (set->p[s]->n_ineq != hull->n_ineq)
			continue;
		for (i = 0; i < set->p[s]->n_ineq; ++i) {
			isl_int *ineq = set->p[s]->ineq[i];
			if (!has_constraint(hull->ctx, table, ineq, total, n))
				break;
		}
		if (i == set->p[s]->n_ineq)
			*is_hull = 1;
	}

	isl_hash_table_clear(table);
	for (i = 0; i < min_constraints; ++i)
		isl_mat_free(constraints[i].c);
	free(constraints);
	free(table);
	return hull;
error:
	isl_hash_table_clear(table);
	free(table);
	if (constraints)
		for (i = 0; i < min_constraints; ++i)
			isl_mat_free(constraints[i].c);
	free(constraints);
	return hull;
}

/* Create a template for the convex hull of "set" and fill it up
 * obvious facet constraints, if any.  If the result happens to
 * be the convex hull of "set" then *is_hull is set to 1.
 */
static struct isl_basic_set *proto_hull(struct isl_set *set, int *is_hull)
{
	struct isl_basic_set *hull;
	unsigned n_ineq;
	int i;

	n_ineq = 1;
	for (i = 0; i < set->n; ++i) {
		n_ineq += set->p[i]->n_eq;
		n_ineq += set->p[i]->n_ineq;
	}
	hull = isl_basic_set_alloc_space(isl_space_copy(set->dim), 0, 0, n_ineq);
	hull = isl_basic_set_set_rational(hull);
	if (!hull)
		return NULL;
	return common_constraints(hull, set, is_hull);
}

static struct isl_basic_set *uset_convex_hull_wrap(struct isl_set *set)
{
	struct isl_basic_set *hull;
	int is_hull;

	hull = proto_hull(set, &is_hull);
	if (hull && !is_hull) {
		if (hull->n_ineq == 0)
			hull = initial_hull(hull, set);
		hull = extend(hull, set);
	}
	isl_set_free(set);

	return hull;
}

/* Compute the convex hull of a set without any parameters or
 * integer divisions.  Depending on whether the set is bounded,
 * we pass control to the wrapping based convex hull or
 * the Fourier-Motzkin elimination based convex hull.
 * We also handle a few special cases before checking the boundedness.
 */
static struct isl_basic_set *uset_convex_hull(struct isl_set *set)
{
	struct isl_basic_set *convex_hull = NULL;
	struct isl_basic_set *lin;

	if (isl_set_n_dim(set) == 0)
		return convex_hull_0d(set);

	set = isl_set_coalesce(set);
	set = isl_set_set_rational(set);

	if (!set)
		goto error;
	if (!set)
		return NULL;
	if (set->n == 1) {
		convex_hull = isl_basic_set_copy(set->p[0]);
		isl_set_free(set);
		return convex_hull;
	}
	if (isl_set_n_dim(set) == 1)
		return convex_hull_1d(set);

	if (isl_set_is_bounded(set) &&
	    set->ctx->opt->convex == ISL_CONVEX_HULL_WRAP)
		return uset_convex_hull_wrap(set);

	lin = uset_combined_lineality_space(isl_set_copy(set));
	if (!lin)
		goto error;
	if (isl_basic_set_is_universe(lin)) {
		isl_set_free(set);
		return lin;
	}
	if (lin->n_eq < isl_basic_set_total_dim(lin))
		return modulo_lineality(set, lin);
	isl_basic_set_free(lin);

	return uset_convex_hull_unbounded(set);
error:
	isl_set_free(set);
	isl_basic_set_free(convex_hull);
	return NULL;
}

/* This is the core procedure, where "set" is a "pure" set, i.e.,
 * without parameters or divs and where the convex hull of set is
 * known to be full-dimensional.
 */
static struct isl_basic_set *uset_convex_hull_wrap_bounded(struct isl_set *set)
{
	struct isl_basic_set *convex_hull = NULL;

	if (!set)
		goto error;

	if (isl_set_n_dim(set) == 0) {
		convex_hull = isl_basic_set_universe(isl_space_copy(set->dim));
		isl_set_free(set);
		convex_hull = isl_basic_set_set_rational(convex_hull);
		return convex_hull;
	}

	set = isl_set_set_rational(set);
	set = isl_set_coalesce(set);
	if (!set)
		goto error;
	if (set->n == 1) {
		convex_hull = isl_basic_set_copy(set->p[0]);
		isl_set_free(set);
		convex_hull = isl_basic_map_remove_redundancies(convex_hull);
		return convex_hull;
	}
	if (isl_set_n_dim(set) == 1)
		return convex_hull_1d(set);

	return uset_convex_hull_wrap(set);
error:
	isl_set_free(set);
	return NULL;
}

/* Compute the convex hull of set "set" with affine hull "affine_hull",
 * We first remove the equalities (transforming the set), compute the
 * convex hull of the transformed set and then add the equalities back
 * (after performing the inverse transformation.
 */
static struct isl_basic_set *modulo_affine_hull(
	struct isl_set *set, struct isl_basic_set *affine_hull)
{
	struct isl_mat *T;
	struct isl_mat *T2;
	struct isl_basic_set *dummy;
	struct isl_basic_set *convex_hull;

	dummy = isl_basic_set_remove_equalities(
			isl_basic_set_copy(affine_hull), &T, &T2);
	if (!dummy)
		goto error;
	isl_basic_set_free(dummy);
	set = isl_set_preimage(set, T);
	convex_hull = uset_convex_hull(set);
	convex_hull = isl_basic_set_preimage(convex_hull, T2);
	convex_hull = isl_basic_set_intersect(convex_hull, affine_hull);
	return convex_hull;
error:
	isl_basic_set_free(affine_hull);
	isl_set_free(set);
	return NULL;
}

/* Compute the convex hull of a map.
 *
 * The implementation was inspired by "Extended Convex Hull" by Fukuda et al.,
 * specifically, the wrapping of facets to obtain new facets.
 */
struct isl_basic_map *isl_map_convex_hull(struct isl_map *map)
{
	struct isl_basic_set *bset;
	struct isl_basic_map *model = NULL;
	struct isl_basic_set *affine_hull = NULL;
	struct isl_basic_map *convex_hull = NULL;
	struct isl_set *set = NULL;
	struct isl_ctx *ctx;

	if (!map)
		goto error;

	ctx = map->ctx;
	if (map->n == 0) {
		convex_hull = isl_basic_map_empty_like_map(map);
		isl_map_free(map);
		return convex_hull;
	}

	map = isl_map_detect_equalities(map);
	map = isl_map_align_divs(map);
	if (!map)
		goto error;
	model = isl_basic_map_copy(map->p[0]);
	set = isl_map_underlying_set(map);
	if (!set)
		goto error;

	affine_hull = isl_set_affine_hull(isl_set_copy(set));
	if (!affine_hull)
		goto error;
	if (affine_hull->n_eq != 0)
		bset = modulo_affine_hull(set, affine_hull);
	else {
		isl_basic_set_free(affine_hull);
		bset = uset_convex_hull(set);
	}

	convex_hull = isl_basic_map_overlying_set(bset, model);
	if (!convex_hull)
		return NULL;

	ISL_F_SET(convex_hull, ISL_BASIC_MAP_NO_IMPLICIT);
	ISL_F_SET(convex_hull, ISL_BASIC_MAP_ALL_EQUALITIES);
	ISL_F_CLR(convex_hull, ISL_BASIC_MAP_RATIONAL);
	return convex_hull;
error:
	isl_set_free(set);
	isl_basic_map_free(model);
	return NULL;
}

struct isl_basic_set *isl_set_convex_hull(struct isl_set *set)
{
	return (struct isl_basic_set *)
		isl_map_convex_hull((struct isl_map *)set);
}

__isl_give isl_basic_map *isl_map_polyhedral_hull(__isl_take isl_map *map)
{
	isl_basic_map *hull;

	hull = isl_map_convex_hull(map);
	return isl_basic_map_remove_divs(hull);
}

__isl_give isl_basic_set *isl_set_polyhedral_hull(__isl_take isl_set *set)
{
	return (isl_basic_set *)isl_map_polyhedral_hull((isl_map *)set);
}

struct sh_data_entry {
	struct isl_hash_table	*table;
	struct isl_tab		*tab;
};

/* Holds the data needed during the simple hull computation.
 * In particular,
 *	n		the number of basic sets in the original set
 *	hull_table	a hash table of already computed constraints
 *			in the simple hull
 *	p		for each basic set,
 *		table		a hash table of the constraints
 *		tab		the tableau corresponding to the basic set
 */
struct sh_data {
	struct isl_ctx		*ctx;
	unsigned		n;
	struct isl_hash_table	*hull_table;
	struct sh_data_entry	p[1];
};

static void sh_data_free(struct sh_data *data)
{
	int i;

	if (!data)
		return;
	isl_hash_table_free(data->ctx, data->hull_table);
	for (i = 0; i < data->n; ++i) {
		isl_hash_table_free(data->ctx, data->p[i].table);
		isl_tab_free(data->p[i].tab);
	}
	free(data);
}

struct ineq_cmp_data {
	unsigned	len;
	isl_int		*p;
};

static int has_ineq(const void *entry, const void *val)
{
	isl_int *row = (isl_int *)entry;
	struct ineq_cmp_data *v = (struct ineq_cmp_data *)val;

	return isl_seq_eq(row + 1, v->p + 1, v->len) ||
	       isl_seq_is_neg(row + 1, v->p + 1, v->len);
}

static int hash_ineq(struct isl_ctx *ctx, struct isl_hash_table *table,
			isl_int *ineq, unsigned len)
{
	uint32_t c_hash;
	struct ineq_cmp_data v;
	struct isl_hash_table_entry *entry;

	v.len = len;
	v.p = ineq;
	c_hash = isl_seq_get_hash(ineq + 1, len);
	entry = isl_hash_table_find(ctx, table, c_hash, has_ineq, &v, 1);
	if (!entry)
		return - 1;
	entry->data = ineq;
	return 0;
}

/* Fill hash table "table" with the constraints of "bset".
 * Equalities are added as two inequalities.
 * The value in the hash table is a pointer to the (in)equality of "bset".
 */
static int hash_basic_set(struct isl_hash_table *table,
				struct isl_basic_set *bset)
{
	int i, j;
	unsigned dim = isl_basic_set_total_dim(bset);

	for (i = 0; i < bset->n_eq; ++i) {
		for (j = 0; j < 2; ++j) {
			isl_seq_neg(bset->eq[i], bset->eq[i], 1 + dim);
			if (hash_ineq(bset->ctx, table, bset->eq[i], dim) < 0)
				return -1;
		}
	}
	for (i = 0; i < bset->n_ineq; ++i) {
		if (hash_ineq(bset->ctx, table, bset->ineq[i], dim) < 0)
			return -1;
	}
	return 0;
}

static struct sh_data *sh_data_alloc(struct isl_set *set, unsigned n_ineq)
{
	struct sh_data *data;
	int i;

	data = isl_calloc(set->ctx, struct sh_data,
		sizeof(struct sh_data) +
		(set->n - 1) * sizeof(struct sh_data_entry));
	if (!data)
		return NULL;
	data->ctx = set->ctx;
	data->n = set->n;
	data->hull_table = isl_hash_table_alloc(set->ctx, n_ineq);
	if (!data->hull_table)
		goto error;
	for (i = 0; i < set->n; ++i) {
		data->p[i].table = isl_hash_table_alloc(set->ctx,
				    2 * set->p[i]->n_eq + set->p[i]->n_ineq);
		if (!data->p[i].table)
			goto error;
		if (hash_basic_set(data->p[i].table, set->p[i]) < 0)
			goto error;
	}
	return data;
error:
	sh_data_free(data);
	return NULL;
}

/* Check if inequality "ineq" is a bound for basic set "j" or if
 * it can be relaxed (by increasing the constant term) to become
 * a bound for that basic set.  In the latter case, the constant
 * term is updated.
 * Return 1 if "ineq" is a bound
 *	  0 if "ineq" may attain arbitrarily small values on basic set "j"
 *	 -1 if some error occurred
 */
static int is_bound(struct sh_data *data, struct isl_set *set, int j,
			isl_int *ineq)
{
	enum isl_lp_result res;
	isl_int opt;

	if (!data->p[j].tab) {
		data->p[j].tab = isl_tab_from_basic_set(set->p[j], 0);
		if (!data->p[j].tab)
			return -1;
	}

	isl_int_init(opt);

	res = isl_tab_min(data->p[j].tab, ineq, data->ctx->one,
				&opt, NULL, 0);
	if (res == isl_lp_ok && isl_int_is_neg(opt))
		isl_int_sub(ineq[0], ineq[0], opt);

	isl_int_clear(opt);

	return (res == isl_lp_ok || res == isl_lp_empty) ? 1 :
	       res == isl_lp_unbounded ? 0 : -1;
}

/* Check if inequality "ineq" from basic set "i" can be relaxed to
 * become a bound on the whole set.  If so, add the (relaxed) inequality
 * to "hull".
 *
 * We first check if "hull" already contains a translate of the inequality.
 * If so, we are done.
 * Then, we check if any of the previous basic sets contains a translate
 * of the inequality.  If so, then we have already considered this
 * inequality and we are done.
 * Otherwise, for each basic set other than "i", we check if the inequality
 * is a bound on the basic set.
 * For previous basic sets, we know that they do not contain a translate
 * of the inequality, so we directly call is_bound.
 * For following basic sets, we first check if a translate of the
 * inequality appears in its description and if so directly update
 * the inequality accordingly.
 */
static struct isl_basic_set *add_bound(struct isl_basic_set *hull,
	struct sh_data *data, struct isl_set *set, int i, isl_int *ineq)
{
	uint32_t c_hash;
	struct ineq_cmp_data v;
	struct isl_hash_table_entry *entry;
	int j, k;

	if (!hull)
		return NULL;

	v.len = isl_basic_set_total_dim(hull);
	v.p = ineq;
	c_hash = isl_seq_get_hash(ineq + 1, v.len);

	entry = isl_hash_table_find(hull->ctx, data->hull_table, c_hash,
					has_ineq, &v, 0);
	if (entry)
		return hull;

	for (j = 0; j < i; ++j) {
		entry = isl_hash_table_find(hull->ctx, data->p[j].table,
						c_hash, has_ineq, &v, 0);
		if (entry)
			break;
	}
	if (j < i)
		return hull;

	k = isl_basic_set_alloc_inequality(hull);
	isl_seq_cpy(hull->ineq[k], ineq, 1 + v.len);
	if (k < 0)
		goto error;

	for (j = 0; j < i; ++j) {
		int bound;
		bound = is_bound(data, set, j, hull->ineq[k]);
		if (bound < 0)
			goto error;
		if (!bound)
			break;
	}
	if (j < i) {
		isl_basic_set_free_inequality(hull, 1);
		return hull;
	}

	for (j = i + 1; j < set->n; ++j) {
		int bound, neg;
		isl_int *ineq_j;
		entry = isl_hash_table_find(hull->ctx, data->p[j].table,
						c_hash, has_ineq, &v, 0);
		if (entry) {
			ineq_j = entry->data;
			neg = isl_seq_is_neg(ineq_j + 1,
					     hull->ineq[k] + 1, v.len);
			if (neg)
				isl_int_neg(ineq_j[0], ineq_j[0]);
			if (isl_int_gt(ineq_j[0], hull->ineq[k][0]))
				isl_int_set(hull->ineq[k][0], ineq_j[0]);
			if (neg)
				isl_int_neg(ineq_j[0], ineq_j[0]);
			continue;
		}
		bound = is_bound(data, set, j, hull->ineq[k]);
		if (bound < 0)
			goto error;
		if (!bound)
			break;
	}
	if (j < set->n) {
		isl_basic_set_free_inequality(hull, 1);
		return hull;
	}

	entry = isl_hash_table_find(hull->ctx, data->hull_table, c_hash,
					has_ineq, &v, 1);
	if (!entry)
		goto error;
	entry->data = hull->ineq[k];

	return hull;
error:
	isl_basic_set_free(hull);
	return NULL;
}

/* Check if any inequality from basic set "i" can be relaxed to
 * become a bound on the whole set.  If so, add the (relaxed) inequality
 * to "hull".
 */
static struct isl_basic_set *add_bounds(struct isl_basic_set *bset,
	struct sh_data *data, struct isl_set *set, int i)
{
	int j, k;
	unsigned dim = isl_basic_set_total_dim(bset);

	for (j = 0; j < set->p[i]->n_eq; ++j) {
		for (k = 0; k < 2; ++k) {
			isl_seq_neg(set->p[i]->eq[j], set->p[i]->eq[j], 1+dim);
			bset = add_bound(bset, data, set, i, set->p[i]->eq[j]);
		}
	}
	for (j = 0; j < set->p[i]->n_ineq; ++j)
		bset = add_bound(bset, data, set, i, set->p[i]->ineq[j]);
	return bset;
}

/* Compute a superset of the convex hull of set that is described
 * by only translates of the constraints in the constituents of set.
 */
static struct isl_basic_set *uset_simple_hull(struct isl_set *set)
{
	struct sh_data *data = NULL;
	struct isl_basic_set *hull = NULL;
	unsigned n_ineq;
	int i;

	if (!set)
		return NULL;

	n_ineq = 0;
	for (i = 0; i < set->n; ++i) {
		if (!set->p[i])
			goto error;
		n_ineq += 2 * set->p[i]->n_eq + set->p[i]->n_ineq;
	}

	hull = isl_basic_set_alloc_space(isl_space_copy(set->dim), 0, 0, n_ineq);
	if (!hull)
		goto error;

	data = sh_data_alloc(set, n_ineq);
	if (!data)
		goto error;

	for (i = 0; i < set->n; ++i)
		hull = add_bounds(hull, data, set, i);

	sh_data_free(data);
	isl_set_free(set);

	return hull;
error:
	sh_data_free(data);
	isl_basic_set_free(hull);
	isl_set_free(set);
	return NULL;
}

/* Compute a superset of the convex hull of map that is described
 * by only translates of the constraints in the constituents of map.
 */
struct isl_basic_map *isl_map_simple_hull(struct isl_map *map)
{
	struct isl_set *set = NULL;
	struct isl_basic_map *model = NULL;
	struct isl_basic_map *hull;
	struct isl_basic_map *affine_hull;
	struct isl_basic_set *bset = NULL;

	if (!map)
		return NULL;
	if (map->n == 0) {
		hull = isl_basic_map_empty_like_map(map);
		isl_map_free(map);
		return hull;
	}
	if (map->n == 1) {
		hull = isl_basic_map_copy(map->p[0]);
		isl_map_free(map);
		return hull;
	}

	map = isl_map_detect_equalities(map);
	affine_hull = isl_map_affine_hull(isl_map_copy(map));
	map = isl_map_align_divs(map);
	model = isl_basic_map_copy(map->p[0]);

	set = isl_map_underlying_set(map);

	bset = uset_simple_hull(set);

	hull = isl_basic_map_overlying_set(bset, model);

	hull = isl_basic_map_intersect(hull, affine_hull);
	hull = isl_basic_map_remove_redundancies(hull);

	if (!hull)
		return NULL;
	ISL_F_SET(hull, ISL_BASIC_MAP_NO_IMPLICIT);
	ISL_F_SET(hull, ISL_BASIC_MAP_ALL_EQUALITIES);

	return hull;
}

struct isl_basic_set *isl_set_simple_hull(struct isl_set *set)
{
	return (struct isl_basic_set *)
		isl_map_simple_hull((struct isl_map *)set);
}

/* Given a set "set", return parametric bounds on the dimension "dim".
 */
static struct isl_basic_set *set_bounds(struct isl_set *set, int dim)
{
	unsigned set_dim = isl_set_dim(set, isl_dim_set);
	set = isl_set_copy(set);
	set = isl_set_eliminate_dims(set, dim + 1, set_dim - (dim + 1));
	set = isl_set_eliminate_dims(set, 0, dim);
	return isl_set_convex_hull(set);
}

/* Computes a "simple hull" and then check if each dimension in the
 * resulting hull is bounded by a symbolic constant.  If not, the
 * hull is intersected with the corresponding bounds on the whole set.
 */
struct isl_basic_set *isl_set_bounded_simple_hull(struct isl_set *set)
{
	int i, j;
	struct isl_basic_set *hull;
	unsigned nparam, left;
	int removed_divs = 0;

	hull = isl_set_simple_hull(isl_set_copy(set));
	if (!hull)
		goto error;

	nparam = isl_basic_set_dim(hull, isl_dim_param);
	for (i = 0; i < isl_basic_set_dim(hull, isl_dim_set); ++i) {
		int lower = 0, upper = 0;
		struct isl_basic_set *bounds;

		left = isl_basic_set_total_dim(hull) - nparam - i - 1;
		for (j = 0; j < hull->n_eq; ++j) {
			if (isl_int_is_zero(hull->eq[j][1 + nparam + i]))
				continue;
			if (isl_seq_first_non_zero(hull->eq[j]+1+nparam+i+1,
						    left) == -1)
				break;
		}
		if (j < hull->n_eq)
			continue;

		for (j = 0; j < hull->n_ineq; ++j) {
			if (isl_int_is_zero(hull->ineq[j][1 + nparam + i]))
				continue;
			if (isl_seq_first_non_zero(hull->ineq[j]+1+nparam+i+1,
						    left) != -1 ||
			    isl_seq_first_non_zero(hull->ineq[j]+1+nparam,
						    i) != -1)
				continue;
			if (isl_int_is_pos(hull->ineq[j][1 + nparam + i]))
				lower = 1;
			else
				upper = 1;
			if (lower && upper)
				break;
		}

		if (lower && upper)
			continue;

		if (!removed_divs) {
			set = isl_set_remove_divs(set);
			if (!set)
				goto error;
			removed_divs = 1;
		}
		bounds = set_bounds(set, i);
		hull = isl_basic_set_intersect(hull, bounds);
		if (!hull)
			goto error;
	}

	isl_set_free(set);
	return hull;
error:
	isl_set_free(set);
	return NULL;
}
