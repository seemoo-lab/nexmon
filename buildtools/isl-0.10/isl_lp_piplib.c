/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <isl/map.h>
#include <isl/vec.h>
#include <isl/lp.h>
#include "isl_piplib.h"
#include "isl_map_piplib.h"

static void copy_solution(struct isl_vec *vec, int maximize, isl_int *opt,
	isl_int *opt_denom, PipQuast *sol)
{
	int i;
	PipList *list;
	isl_int tmp;

	if (opt) {
		if (opt_denom) {
			isl_seq_cpy_from_pip(opt,
				 &sol->list->vector->the_vector[0], 1);
			isl_seq_cpy_from_pip(opt_denom,
				 &sol->list->vector->the_deno[0], 1);
		} else if (maximize)
			mpz_fdiv_q(*opt, sol->list->vector->the_vector[0],
					 sol->list->vector->the_deno[0]);
		else
			mpz_cdiv_q(*opt, sol->list->vector->the_vector[0],
					 sol->list->vector->the_deno[0]);
	}

	if (!vec)
		return;

	isl_int_init(tmp);
	isl_int_set_si(vec->el[0], 1);
	for (i = 0, list = sol->list->next; list; ++i, list = list->next) {
		isl_seq_cpy_from_pip(&vec->el[1 + i],
			&list->vector->the_deno[0], 1);
		isl_int_lcm(vec->el[0], vec->el[0], vec->el[1 + i]);
	}
	for (i = 0, list = sol->list->next; list; ++i, list = list->next) {
		isl_seq_cpy_from_pip(&tmp, &list->vector->the_deno[0], 1);
		isl_int_divexact(tmp, vec->el[0], tmp);
		isl_seq_cpy_from_pip(&vec->el[1 + i],
			&list->vector->the_vector[0], 1);
		isl_int_mul(vec->el[1 + i], vec->el[1 + i], tmp);
	}
	isl_int_clear(tmp);
}

enum isl_lp_result isl_pip_solve_lp(struct isl_basic_map *bmap, int maximize,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **vec)
{
	enum isl_lp_result res = isl_lp_ok;
	PipMatrix	*domain = NULL;
	PipOptions	*options;
	PipQuast   	*sol;
	unsigned	 total;

	total = isl_basic_map_total_dim(bmap);
	domain = isl_basic_map_to_pip(bmap, 0, 1, 0);
	if (!domain)
		goto error;
	entier_set_si(domain->p[0][1], -1);
	isl_int_set(domain->p[0][domain->NbColumns - 1], f[0]);
	isl_seq_cpy_to_pip(domain->p[0]+2, f+1, total);

	options = pip_options_init();
	if (!options)
		goto error;
	options->Urs_unknowns = -1;
	options->Maximize = maximize;
	options->Nq = 0;
	sol = pip_solve(domain, NULL, -1, options);
	pip_options_free(options);
	if (!sol)
		goto error;

	if (vec) {
		isl_ctx *ctx = isl_basic_map_get_ctx(bmap);
		*vec = isl_vec_alloc(ctx, 1 + total);
	}
	if (vec && !*vec)
		res = isl_lp_error;
	else if (!sol->list)
		res = isl_lp_empty;
	else if (entier_zero_p(sol->list->vector->the_deno[0]))
		res = isl_lp_unbounded;
	else
		copy_solution(*vec, maximize, opt, opt_denom, sol);
	pip_matrix_free(domain);
	pip_quast_free(sol);
	return res;
error:
	if (domain)
		pip_matrix_free(domain);
	return isl_lp_error;
}
