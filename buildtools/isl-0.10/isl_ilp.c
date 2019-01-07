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
#include <isl/ilp.h>
#include "isl_sample.h"
#include <isl/seq.h>
#include "isl_equalities.h"
#include <isl_aff_private.h>
#include <isl_local_space_private.h>
#include <isl_mat_private.h>

/* Given a basic set "bset", construct a basic set U such that for
 * each element x in U, the whole unit box positioned at x is inside
 * the given basic set.
 * Note that U may not contain all points that satisfy this property.
 *
 * We simply add the sum of all negative coefficients to the constant
 * term.  This ensures that if x satisfies the resulting constraints,
 * then x plus any sum of unit vectors satisfies the original constraints.
 */
static struct isl_basic_set *unit_box_base_points(struct isl_basic_set *bset)
{
	int i, j, k;
	struct isl_basic_set *unit_box = NULL;
	unsigned total;

	if (!bset)
		goto error;

	if (bset->n_eq != 0) {
		unit_box = isl_basic_set_empty_like(bset);
		isl_basic_set_free(bset);
		return unit_box;
	}

	total = isl_basic_set_total_dim(bset);
	unit_box = isl_basic_set_alloc_space(isl_basic_set_get_space(bset),
					0, 0, bset->n_ineq);

	for (i = 0; i < bset->n_ineq; ++i) {
		k = isl_basic_set_alloc_inequality(unit_box);
		if (k < 0)
			goto error;
		isl_seq_cpy(unit_box->ineq[k], bset->ineq[i], 1 + total);
		for (j = 0; j < total; ++j) {
			if (isl_int_is_nonneg(unit_box->ineq[k][1 + j]))
				continue;
			isl_int_add(unit_box->ineq[k][0],
				unit_box->ineq[k][0], unit_box->ineq[k][1 + j]);
		}
	}

	isl_basic_set_free(bset);
	return unit_box;
error:
	isl_basic_set_free(bset);
	isl_basic_set_free(unit_box);
	return NULL;
}

/* Find an integer point in "bset", preferably one that is
 * close to minimizing "f".
 *
 * We first check if we can easily put unit boxes inside bset.
 * If so, we take the best base point of any of the unit boxes we can find
 * and round it up to the nearest integer.
 * If not, we simply pick any integer point in "bset".
 */
static struct isl_vec *initial_solution(struct isl_basic_set *bset, isl_int *f)
{
	enum isl_lp_result res;
	struct isl_basic_set *unit_box;
	struct isl_vec *sol;

	unit_box = unit_box_base_points(isl_basic_set_copy(bset));

	res = isl_basic_set_solve_lp(unit_box, 0, f, bset->ctx->one,
					NULL, NULL, &sol);
	if (res == isl_lp_ok) {
		isl_basic_set_free(unit_box);
		return isl_vec_ceil(sol);
	}

	isl_basic_set_free(unit_box);

	return isl_basic_set_sample_vec(isl_basic_set_copy(bset));
}

/* Restrict "bset" to those points with values for f in the interval [l, u].
 */
static struct isl_basic_set *add_bounds(struct isl_basic_set *bset,
	isl_int *f, isl_int l, isl_int u)
{
	int k;
	unsigned total;

	total = isl_basic_set_total_dim(bset);
	bset = isl_basic_set_extend_constraints(bset, 0, 2);

	k = isl_basic_set_alloc_inequality(bset);
	if (k < 0)
		goto error;
	isl_seq_cpy(bset->ineq[k], f, 1 + total);
	isl_int_sub(bset->ineq[k][0], bset->ineq[k][0], l);

	k = isl_basic_set_alloc_inequality(bset);
	if (k < 0)
		goto error;
	isl_seq_neg(bset->ineq[k], f, 1 + total);
	isl_int_add(bset->ineq[k][0], bset->ineq[k][0], u);

	return bset;
error:
	isl_basic_set_free(bset);
	return NULL;
}

/* Find an integer point in "bset" that minimizes f (in any) such that
 * the value of f lies inside the interval [l, u].
 * Return this integer point if it can be found.
 * Otherwise, return sol.
 *
 * We perform a number of steps until l > u.
 * In each step, we look for an integer point with value in either
 * the whole interval [l, u] or half of the interval [l, l+floor(u-l-1/2)].
 * The choice depends on whether we have found an integer point in the
 * previous step.  If so, we look for the next point in half of the remaining
 * interval.
 * If we find a point, the current solution is updated and u is set
 * to its value minus 1.
 * If no point can be found, we update l to the upper bound of the interval
 * we checked (u or l+floor(u-l-1/2)) plus 1.
 */
static struct isl_vec *solve_ilp_search(struct isl_basic_set *bset,
	isl_int *f, isl_int *opt, struct isl_vec *sol, isl_int l, isl_int u)
{
	isl_int tmp;
	int divide = 1;

	isl_int_init(tmp);

	while (isl_int_le(l, u)) {
		struct isl_basic_set *slice;
		struct isl_vec *sample;

		if (!divide)
			isl_int_set(tmp, u);
		else {
			isl_int_sub(tmp, u, l);
			isl_int_fdiv_q_ui(tmp, tmp, 2);
			isl_int_add(tmp, tmp, l);
		}
		slice = add_bounds(isl_basic_set_copy(bset), f, l, tmp);
		sample = isl_basic_set_sample_vec(slice);
		if (!sample) {
			isl_vec_free(sol);
			sol = NULL;
			break;
		}
		if (sample->size > 0) {
			isl_vec_free(sol);
			sol = sample;
			isl_seq_inner_product(f, sol->el, sol->size, opt);
			isl_int_sub_ui(u, *opt, 1);
			divide = 1;
		} else {
			isl_vec_free(sample);
			if (!divide)
				break;
			isl_int_add_ui(l, tmp, 1);
			divide = 0;
		}
	}

	isl_int_clear(tmp);

	return sol;
}

/* Find an integer point in "bset" that minimizes f (if any).
 * If sol_p is not NULL then the integer point is returned in *sol_p.
 * The optimal value of f is returned in *opt.
 *
 * The algorithm maintains a currently best solution and an interval [l, u]
 * of values of f for which integer solutions could potentially still be found.
 * The initial value of the best solution so far is any solution.
 * The initial value of l is minimal value of f over the rationals
 * (rounded up to the nearest integer).
 * The initial value of u is the value of f at the initial solution minus 1.
 *
 * We then call solve_ilp_search to perform a binary search on the interval.
 */
static enum isl_lp_result solve_ilp(struct isl_basic_set *bset,
				      isl_int *f, isl_int *opt,
				      struct isl_vec **sol_p)
{
	enum isl_lp_result res;
	isl_int l, u;
	struct isl_vec *sol;

	res = isl_basic_set_solve_lp(bset, 0, f, bset->ctx->one,
					opt, NULL, &sol);
	if (res == isl_lp_ok && isl_int_is_one(sol->el[0])) {
		if (sol_p)
			*sol_p = sol;
		else
			isl_vec_free(sol);
		return isl_lp_ok;
	}
	isl_vec_free(sol);
	if (res == isl_lp_error || res == isl_lp_empty)
		return res;

	sol = initial_solution(bset, f);
	if (!sol)
		return isl_lp_error;
	if (sol->size == 0) {
		isl_vec_free(sol);
		return isl_lp_empty;
	}
	if (res == isl_lp_unbounded) {
		isl_vec_free(sol);
		return isl_lp_unbounded;
	}

	isl_int_init(l);
	isl_int_init(u);

	isl_int_set(l, *opt);

	isl_seq_inner_product(f, sol->el, sol->size, opt);
	isl_int_sub_ui(u, *opt, 1);

	sol = solve_ilp_search(bset, f, opt, sol, l, u);
	if (!sol)
		res = isl_lp_error;

	isl_int_clear(l);
	isl_int_clear(u);

	if (sol_p)
		*sol_p = sol;
	else
		isl_vec_free(sol);

	return res;
}

static enum isl_lp_result solve_ilp_with_eq(struct isl_basic_set *bset, int max,
				      isl_int *f, isl_int *opt,
				      struct isl_vec **sol_p)
{
	unsigned dim;
	enum isl_lp_result res;
	struct isl_mat *T = NULL;
	struct isl_vec *v;

	bset = isl_basic_set_copy(bset);
	dim = isl_basic_set_total_dim(bset);
	v = isl_vec_alloc(bset->ctx, 1 + dim);
	if (!v)
		goto error;
	isl_seq_cpy(v->el, f, 1 + dim);
	bset = isl_basic_set_remove_equalities(bset, &T, NULL);
	v = isl_vec_mat_product(v, isl_mat_copy(T));
	if (!v)
		goto error;
	res = isl_basic_set_solve_ilp(bset, max, v->el, opt, sol_p);
	isl_vec_free(v);
	if (res == isl_lp_ok && sol_p) {
		*sol_p = isl_mat_vec_product(T, *sol_p);
		if (!*sol_p)
			res = isl_lp_error;
	} else
		isl_mat_free(T);
	isl_basic_set_free(bset);
	return res;
error:
	isl_mat_free(T);
	isl_basic_set_free(bset);
	return isl_lp_error;
}

/* Find an integer point in "bset" that minimizes (or maximizes if max is set)
 * f (if any).
 * If sol_p is not NULL then the integer point is returned in *sol_p.
 * The optimal value of f is returned in *opt.
 *
 * If there is any equality among the points in "bset", then we first
 * project it out.  Otherwise, we continue with solve_ilp above.
 */
enum isl_lp_result isl_basic_set_solve_ilp(struct isl_basic_set *bset, int max,
				      isl_int *f, isl_int *opt,
				      struct isl_vec **sol_p)
{
	unsigned dim;
	enum isl_lp_result res;

	if (!bset)
		return isl_lp_error;
	if (sol_p)
		*sol_p = NULL;

	isl_assert(bset->ctx, isl_basic_set_n_param(bset) == 0, goto error);

	if (isl_basic_set_plain_is_empty(bset))
		return isl_lp_empty;

	if (bset->n_eq)
		return solve_ilp_with_eq(bset, max, f, opt, sol_p);

	dim = isl_basic_set_total_dim(bset);

	if (max)
		isl_seq_neg(f, f, 1 + dim);

	res = solve_ilp(bset, f, opt, sol_p);

	if (max) {
		isl_seq_neg(f, f, 1 + dim);
		isl_int_neg(*opt, *opt);
	}

	return res;
error:
	isl_basic_set_free(bset);
	return isl_lp_error;
}

static enum isl_lp_result basic_set_opt(__isl_keep isl_basic_set *bset, int max,
	__isl_keep isl_aff *obj, isl_int *opt)
{
	enum isl_lp_result res;

	if (!obj)
		return isl_lp_error;
	bset = isl_basic_set_copy(bset);
	bset = isl_basic_set_underlying_set(bset);
	res = isl_basic_set_solve_ilp(bset, max, obj->v->el + 1, opt, NULL);
	isl_basic_set_free(bset);
	return res;
}

static __isl_give isl_mat *extract_divs(__isl_keep isl_basic_set *bset)
{
	int i;
	isl_ctx *ctx = isl_basic_set_get_ctx(bset);
	isl_mat *div;

	div = isl_mat_alloc(ctx, bset->n_div,
			    1 + 1 + isl_basic_set_total_dim(bset));
	if (!div)
		return NULL;

	for (i = 0; i < bset->n_div; ++i)
		isl_seq_cpy(div->row[i], bset->div[i], div->n_col);

	return div;
}

enum isl_lp_result isl_basic_set_opt(__isl_keep isl_basic_set *bset, int max,
	__isl_keep isl_aff *obj, isl_int *opt)
{
	int *exp1 = NULL;
	int *exp2 = NULL;
	isl_ctx *ctx;
	isl_mat *bset_div = NULL;
	isl_mat *div = NULL;
	enum isl_lp_result res;

	if (!bset || !obj)
		return isl_lp_error;

	ctx = isl_aff_get_ctx(obj);
	if (!isl_space_is_equal(bset->dim, obj->ls->dim))
		isl_die(ctx, isl_error_invalid,
			"spaces don't match", return isl_lp_error);
	if (!isl_int_is_one(obj->v->el[0]))
		isl_die(ctx, isl_error_unsupported,
			"expecting integer affine expression",
			return isl_lp_error);

	if (bset->n_div == 0 && obj->ls->div->n_row == 0)
		return basic_set_opt(bset, max, obj, opt);

	bset = isl_basic_set_copy(bset);
	obj = isl_aff_copy(obj);

	bset_div = extract_divs(bset);
	exp1 = isl_alloc_array(ctx, int, bset_div->n_row);
	exp2 = isl_alloc_array(ctx, int, obj->ls->div->n_row);
	if (!bset_div || !exp1 || !exp2)
		goto error;

	div = isl_merge_divs(bset_div, obj->ls->div, exp1, exp2);

	bset = isl_basic_set_expand_divs(bset, isl_mat_copy(div), exp1);
	obj = isl_aff_expand_divs(obj, isl_mat_copy(div), exp2);

	res = basic_set_opt(bset, max, obj, opt);

	isl_mat_free(bset_div);
	isl_mat_free(div);
	free(exp1);
	free(exp2);
	isl_basic_set_free(bset);
	isl_aff_free(obj);

	return res;
error:
	isl_mat_free(div);
	isl_mat_free(bset_div);
	free(exp1);
	free(exp2);
	isl_basic_set_free(bset);
	isl_aff_free(obj);
	return isl_lp_error;
}

/* Compute the minimum (maximum if max is set) of the integer affine
 * expression obj over the points in set and put the result in *opt.
 */
enum isl_lp_result isl_set_opt(__isl_keep isl_set *set, int max,
	__isl_keep isl_aff *obj, isl_int *opt)
{
	int i;
	enum isl_lp_result res;
	int empty = 1;
	isl_int opt_i;

	if (!set || !obj)
		return isl_lp_error;
	if (set->n == 0)
		return isl_lp_empty;

	res = isl_basic_set_opt(set->p[0], max, obj, opt);
	if (res == isl_lp_error || res == isl_lp_unbounded)
		return res;
	if (set->n == 1)
		return res;
	if (res == isl_lp_ok)
		empty = 0;

	isl_int_init(opt_i);
	for (i = 1; i < set->n; ++i) {
		res = isl_basic_set_opt(set->p[i], max, obj, &opt_i);
		if (res == isl_lp_error || res == isl_lp_unbounded) {
			isl_int_clear(opt_i);
			return res;
		}
		if (res == isl_lp_ok)
			empty = 0;
		if (isl_int_gt(opt_i, *opt))
			isl_int_set(*opt, opt_i);
	}
	isl_int_clear(opt_i);

	return empty ? isl_lp_empty : isl_lp_ok;
}

enum isl_lp_result isl_basic_set_max(__isl_keep isl_basic_set *bset,
	__isl_keep isl_aff *obj, isl_int *opt)
{
	return isl_basic_set_opt(bset, 1, obj, opt);
}

enum isl_lp_result isl_set_max(__isl_keep isl_set *set,
	__isl_keep isl_aff *obj, isl_int *opt)
{
	return isl_set_opt(set, 1, obj, opt);
}

enum isl_lp_result isl_set_min(__isl_keep isl_set *set,
	__isl_keep isl_aff *obj, isl_int *opt)
{
	return isl_set_opt(set, 0, obj, opt);
}
