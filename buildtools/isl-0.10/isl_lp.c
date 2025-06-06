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
#include "isl_lp_piplib.h"
#include <isl/seq.h>
#include "isl_tab.h"
#include <isl_options_private.h>

enum isl_lp_result isl_tab_solve_lp(struct isl_basic_map *bmap, int maximize,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol)
{
	struct isl_tab *tab;
	enum isl_lp_result res;
	unsigned dim = isl_basic_map_total_dim(bmap);

	if (maximize)
		isl_seq_neg(f, f, 1 + dim);

	bmap = isl_basic_map_gauss(bmap, NULL);
	tab = isl_tab_from_basic_map(bmap, 0);
	res = isl_tab_min(tab, f, denom, opt, opt_denom, 0);
	if (res == isl_lp_ok && sol) {
		*sol = isl_tab_get_sample_value(tab);
		if (!*sol)
			res = isl_lp_error;
	}
	isl_tab_free(tab);

	if (maximize)
		isl_seq_neg(f, f, 1 + dim);
	if (maximize && opt)
		isl_int_neg(*opt, *opt);

	return res;
}

/* Given a basic map "bmap" and an affine combination of the variables "f"
 * with denominator "denom", set *opt / *opt_denom to the minimal
 * (or maximal if "maximize" is true) value attained by f/d over "bmap",
 * assuming the basic map is not empty and the expression cannot attain
 * arbitrarily small (or large) values.
 * If opt_denom is NULL, then *opt is rounded up (or down)
 * to the nearest integer.
 * The return value reflects the nature of the result (empty, unbounded,
 * minmimal or maximal value returned in *opt).
 */
enum isl_lp_result isl_basic_map_solve_lp(struct isl_basic_map *bmap, int max,
				      isl_int *f, isl_int d, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol)
{
	if (sol)
		*sol = NULL;

	if (!bmap)
		return isl_lp_error;

	switch (bmap->ctx->opt->lp_solver) {
	case ISL_LP_PIP:
		return isl_pip_solve_lp(bmap, max, f, d, opt, opt_denom, sol);
	case ISL_LP_TAB:
		return isl_tab_solve_lp(bmap, max, f, d, opt, opt_denom, sol);
	default:
		return isl_lp_error;
	}
}

enum isl_lp_result isl_basic_set_solve_lp(struct isl_basic_set *bset, int max,
				      isl_int *f, isl_int d, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol)
{
	return isl_basic_map_solve_lp((struct isl_basic_map *)bset, max,
					f, d, opt, opt_denom, sol);
}

enum isl_lp_result isl_map_solve_lp(__isl_keep isl_map *map, int max,
				      isl_int *f, isl_int d, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol)
{
	int i;
	isl_int o;
	isl_int t;
	isl_int opt_i;
	isl_int opt_denom_i;
	enum isl_lp_result res;
	int max_div;
	isl_vec *v = NULL;

	if (!map)
		return isl_lp_error;
	if (map->n == 0)
		return isl_lp_empty;

	max_div = 0;
	for (i = 0; i < map->n; ++i)
		if (map->p[i]->n_div > max_div)
			max_div = map->p[i]->n_div;
	if (max_div > 0) {
		unsigned total = isl_space_dim(map->dim, isl_dim_all);
		v = isl_vec_alloc(map->ctx, 1 + total + max_div);
		if (!v)
			return isl_lp_error;
		isl_seq_cpy(v->el, f, 1 + total);
		isl_seq_clr(v->el + 1 + total, max_div);
		f = v->el;
	}

	if (!opt && map->n > 1 && sol) {
		isl_int_init(o);
		opt = &o;
	}
	if (map->n > 0)
		isl_int_init(opt_i);
	if (map->n > 0 && opt_denom) {
		isl_int_init(opt_denom_i);
		isl_int_init(t);
	}

	res = isl_basic_map_solve_lp(map->p[0], max, f, d,
					opt, opt_denom, sol);
	if (res == isl_lp_error || res == isl_lp_unbounded)
		goto done;

	if (sol)
		*sol = NULL;

	for (i = 1; i < map->n; ++i) {
		isl_vec *sol_i = NULL;
		enum isl_lp_result res_i;
		int better;

		res_i = isl_basic_map_solve_lp(map->p[i], max, f, d,
					    &opt_i,
					    opt_denom ? &opt_denom_i : NULL,
					    sol ? &sol_i : NULL);
		if (res_i == isl_lp_error || res_i == isl_lp_unbounded) {
			res = res_i;
			goto done;
		}
		if (res_i == isl_lp_empty)
			continue;
		if (res == isl_lp_empty) {
			better = 1;
		} else if (!opt_denom) {
			if (max)
				better = isl_int_gt(opt_i, *opt);
			else
				better = isl_int_lt(opt_i, *opt);
		} else {
			isl_int_mul(t, opt_i, *opt_denom);
			isl_int_submul(t, *opt, opt_denom_i);
			if (max)
				better = isl_int_is_pos(t);
			else
				better = isl_int_is_neg(t);
		}
		if (better) {
			res = res_i;
			if (opt)
				isl_int_set(*opt, opt_i);
			if (opt_denom)
				isl_int_set(*opt_denom, opt_denom_i);
			if (sol) {
				isl_vec_free(*sol);
				*sol = sol_i;
			}
		} else
			isl_vec_free(sol_i);
	}

done:
	isl_vec_free(v);
	if (map->n > 0 && opt_denom) {
		isl_int_clear(opt_denom_i);
		isl_int_clear(t);
	}
	if (map->n > 0)
		isl_int_clear(opt_i);
	if (opt == &o)
		isl_int_clear(o);
	return res;
}

enum isl_lp_result isl_set_solve_lp(__isl_keep isl_set *set, int max,
				      isl_int *f, isl_int d, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol)
{
	return isl_map_solve_lp((struct isl_map *)set, max,
					f, d, opt, opt_denom, sol);
}
