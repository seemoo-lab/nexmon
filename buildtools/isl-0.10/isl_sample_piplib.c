/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <isl/mat.h>
#include <isl/vec.h>
#include <isl/seq.h>
#include "isl_piplib.h"
#include "isl_sample_piplib.h"

struct isl_vec *isl_pip_basic_set_sample(struct isl_basic_set *bset)
{
	PipOptions	*options = NULL;
	PipMatrix	*domain = NULL;
	PipQuast	*sol = NULL;
	struct isl_vec *vec = NULL;
	unsigned	dim;
	struct isl_ctx *ctx;

	if (!bset)
		goto error;
	ctx = isl_basic_set_get_ctx(bset);
	isl_assert(ctx, isl_basic_set_n_param(bset) == 0, goto error);
	isl_assert(ctx, isl_basic_set_dim(bset, isl_dim_div) == 0, goto error);
	dim = isl_basic_set_n_dim(bset);
	domain = isl_basic_map_to_pip((struct isl_basic_map *)bset, 0, 0, 0);
	if (!domain)
		goto error;

	options = pip_options_init();
	if (!options)
		goto error;
	sol = pip_solve(domain, NULL, -1, options);
	if (!sol)
		goto error;
	if (!sol->list)
		vec = isl_vec_alloc(ctx, 0);
	else {
		PipList *l;
		int i;
		vec = isl_vec_alloc(ctx, 1 + dim);
		if (!vec)
			goto error;
		isl_int_set_si(vec->block.data[0], 1);
		for (i = 0, l = sol->list; l && i < dim; ++i, l = l->next) {
			isl_seq_cpy_from_pip(&vec->block.data[1+i],
					&l->vector->the_vector[0], 1);
			isl_assert(ctx, !entier_zero_p(l->vector->the_deno[0]),
					goto error);
		}
		isl_assert(ctx, i == dim, goto error);
	}

	pip_quast_free(sol);
	pip_options_free(options);
	pip_matrix_free(domain);

	isl_basic_set_free(bset);
	return vec;
error:
	isl_vec_free(vec);
	isl_basic_set_free(bset);
	if (sol)
		pip_quast_free(sol);
	if (domain)
		pip_matrix_free(domain);
	return NULL;
}
