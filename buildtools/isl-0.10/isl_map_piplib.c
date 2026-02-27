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
#include <isl/set.h>
#include <isl/map.h>
#include <isl/mat.h>
#include <isl/seq.h>
#include "isl_piplib.h"
#include "isl_map_piplib.h"

static void copy_values_from(isl_int *dst, Entier *src, unsigned n)
{
	int i;

	for (i = 0; i < n; ++i)
		entier_assign(dst[i], src[i]);
}

static void add_value(isl_int *dst, Entier *src)
{
	mpz_add(*dst, *dst, *src);
}

static void copy_constraint_from(isl_int *dst, PipVector *src,
		unsigned nparam, unsigned n_in, unsigned n_out,
		unsigned extra, int *pos)
{
	int i;

	copy_values_from(dst, src->the_vector+src->nb_elements-1, 1);
	copy_values_from(dst+1, src->the_vector, nparam+n_in);
	isl_seq_clr(dst+1+nparam+n_in, n_out);
	isl_seq_clr(dst+1+nparam+n_in+n_out, extra);
	for (i = 0; i + n_in + nparam < src->nb_elements-1; ++i) {
		int p = pos[i];
		add_value(&dst[1+nparam+n_in+n_out+p],
			  &src->the_vector[n_in+nparam+i]);
	}
}

static int add_inequality(struct isl_ctx *ctx,
		   struct isl_basic_map *bmap, int *pos, PipVector *vec)
{
	unsigned nparam = isl_basic_map_n_param(bmap);
	unsigned n_in = isl_basic_map_n_in(bmap);
	unsigned n_out = isl_basic_map_n_out(bmap);
	unsigned n_div = isl_basic_map_n_div(bmap);
	int i = isl_basic_map_alloc_inequality(bmap);
	if (i < 0)
		return -1;
	copy_constraint_from(bmap->ineq[i], vec,
	    nparam, n_in, n_out, n_div, pos);

	return i;
}

/* For a div d = floor(f/m), add the constraints
 *
 *		f - m d >= 0
 *		-(f-(n-1)) + m d >= 0
 *
 * Note that the second constraint is the negation of
 *
 *		f - m d >= n
 */
static int add_div_constraints(struct isl_ctx *ctx,
	struct isl_basic_map *bmap, int *pos, PipNewparm *p, unsigned div)
{
	int i, j;
	unsigned total = isl_basic_map_total_dim(bmap);
	unsigned div_pos = 1 + total - bmap->n_div + div;

	i = add_inequality(ctx, bmap, pos, p->vector);
	if (i < 0)
		return -1;
	copy_values_from(&bmap->ineq[i][div_pos], &p->deno, 1);
	isl_int_neg(bmap->ineq[i][div_pos], bmap->ineq[i][div_pos]);

	j = isl_basic_map_alloc_inequality(bmap);
	if (j < 0)
		return -1;
	isl_seq_neg(bmap->ineq[j], bmap->ineq[i], 1 + total);
	isl_int_add(bmap->ineq[j][0], bmap->ineq[j][0], bmap->ineq[j][div_pos]);
	isl_int_sub_ui(bmap->ineq[j][0], bmap->ineq[j][0], 1);
	return j;
}

static int add_equality(struct isl_ctx *ctx,
		   struct isl_basic_map *bmap, int *pos,
		   unsigned var, PipVector *vec)
{
	int i;
	unsigned nparam = isl_basic_map_n_param(bmap);
	unsigned n_in = isl_basic_map_n_in(bmap);
	unsigned n_out = isl_basic_map_n_out(bmap);

	isl_assert(ctx, var < n_out, return -1);

	i = isl_basic_map_alloc_equality(bmap);
	if (i < 0)
		return -1;
	copy_constraint_from(bmap->eq[i], vec,
	    nparam, n_in, n_out, bmap->extra, pos);
	isl_int_set_si(bmap->eq[i][1+nparam+n_in+var], -1);

	return i;
}

static int find_div(struct isl_ctx *ctx,
		   struct isl_basic_map *bmap, int *pos, PipNewparm *p)
{
	int i, j;
	unsigned nparam = isl_basic_map_n_param(bmap);
	unsigned n_in = isl_basic_map_n_in(bmap);
	unsigned n_out = isl_basic_map_n_out(bmap);

	i = isl_basic_map_alloc_div(bmap);
	if (i < 0)
		return -1;

	copy_constraint_from(bmap->div[i]+1, p->vector,
	    nparam, n_in, n_out, bmap->extra, pos);

	copy_values_from(bmap->div[i], &p->deno, 1);
	for (j = 0; j < i; ++j)
		if (isl_seq_eq(bmap->div[i], bmap->div[j],
				1+1+isl_basic_map_total_dim(bmap)+j)) {
			isl_basic_map_free_div(bmap, 1);
			return j;
		}

	if (add_div_constraints(ctx, bmap, pos, p, i) < 0)
		return -1;

	return i;
}

/* Count some properties of a quast
 * - maximal number of new parameters
 * - maximal depth
 * - total number of solutions
 * - total number of empty branches
 */
static void quast_count(PipQuast *q, int *maxnew, int depth, int *maxdepth,
		        int *sol, int *nosol)
{
	PipNewparm *p;

	for (p = q->newparm; p; p = p->next)
		if (p->rank > *maxnew)
			*maxnew = p->rank;
	if (q->condition) {
		if (++depth > *maxdepth)
			*maxdepth = depth;
		quast_count(q->next_else, maxnew, depth, maxdepth, sol, nosol);
		quast_count(q->next_then, maxnew, depth, maxdepth, sol, nosol);
	} else {
		if (q->list)
			++(*sol);
		else
			++(*nosol);
	}
}

/*
 * pos: array of length bmap->set.extra, mapping each of the existential
 *		variables PIP proposes to an existential variable in bmap
 * bmap: collects the currently active constraints
 * rest: collects the empty leaves of the quast (if not NULL)
 */
struct scan_data {
	struct isl_ctx 			*ctx;
	struct isl_basic_map 		*bmap;
	struct isl_set			**rest;
	int	   *pos;
};

/*
 * New existentially quantified variables are places after the existing ones.
 */
static struct isl_map *scan_quast_r(struct scan_data *data, PipQuast *q,
				    struct isl_map *map)
{
	PipNewparm *p;
	struct isl_basic_map *bmap = data->bmap;
	unsigned old_n_div = bmap->n_div;
	unsigned nparam = isl_basic_map_n_param(bmap);
	unsigned n_in = isl_basic_map_n_in(bmap);
	unsigned n_out = isl_basic_map_n_out(bmap);

	if (!map)
		goto error;

	for (p = q->newparm; p; p = p->next) {
		int pos;
		unsigned pip_param = nparam + n_in;

		pos = find_div(data->ctx, bmap, data->pos, p);
		if (pos < 0)
			goto error;
		data->pos[p->rank - pip_param] = pos;
	}

	if (q->condition) {
		int pos = add_inequality(data->ctx, bmap, data->pos,
					 q->condition);
		if (pos < 0)
			goto error;
		map = scan_quast_r(data, q->next_then, map);

		if (isl_inequality_negate(bmap, pos))
			goto error;
		map = scan_quast_r(data, q->next_else, map);

		if (isl_basic_map_free_inequality(bmap, 1))
			goto error;
	} else if (q->list) {
		PipList *l;
		int j;
		/* if bmap->n_out is zero, we are only interested in the domains
		 * where a solution exists and not in the actual solution
		 */
		for (j = 0, l = q->list; j < n_out && l; ++j, l = l->next)
			if (add_equality(data->ctx, bmap, data->pos, j,
						l->vector) < 0)
				goto error;
		map = isl_map_add_basic_map(map, isl_basic_map_copy(bmap));
		if (isl_basic_map_free_equality(bmap, n_out))
			goto error;
	} else if (data->rest) {
		struct isl_basic_set *bset;
		bset = isl_basic_set_from_basic_map(isl_basic_map_copy(bmap));
		bset = isl_basic_set_drop_dims(bset, n_in, n_out);
		if (!bset)
			goto error;
		*data->rest = isl_set_add_basic_set(*data->rest, bset);
	}

	if (isl_basic_map_free_inequality(bmap, 2*(bmap->n_div - old_n_div)))
		goto error;
	if (isl_basic_map_free_div(bmap, bmap->n_div - old_n_div))
		goto error;
	return map;
error:
	isl_map_free(map);
	return NULL;
}

/*
 * Returns a map of dimension "keep_dim" with "context" as domain and
 * as range the first "isl_space_dim(keep_dim, isl_dim_out)" variables
 * in the quast lists.
 */
static struct isl_map *isl_map_from_quast(struct isl_ctx *ctx, PipQuast *q,
		isl_space *keep_dim,
		struct isl_basic_set *context,
		struct isl_set **rest)
{
	int		pip_param;
	int		nexist;
	int		max_depth;
	int		n_sol, n_nosol;
	struct scan_data	data;
	struct isl_map		*map = NULL;
	isl_space		*dims;
	unsigned		nparam;
	unsigned		dim;
	unsigned		keep;

	data.ctx = ctx;
	data.rest = rest;
	data.bmap = NULL;
	data.pos = NULL;

	if (!context || !keep_dim)
		goto error;

	dim = isl_basic_set_n_dim(context);
	nparam = isl_basic_set_n_param(context);
	keep = isl_space_dim(keep_dim, isl_dim_out);
	pip_param = nparam + dim;

	max_depth = 0;
	n_sol = 0;
	n_nosol = 0;
	nexist = pip_param-1;
	quast_count(q, &nexist, 0, &max_depth, &n_sol, &n_nosol);
	nexist -= pip_param-1;

	if (rest) {
		*rest = isl_set_alloc_space(isl_space_copy(context->dim), n_nosol,
					ISL_MAP_DISJOINT);
		if (!*rest)
			goto error;
	}
	map = isl_map_alloc_space(isl_space_copy(keep_dim), n_sol,
				ISL_MAP_DISJOINT);
	if (!map)
		goto error;

	dims = isl_space_reverse(isl_space_copy(context->dim));
	data.bmap = isl_basic_map_from_basic_set(context, dims);
	data.bmap = isl_basic_map_extend_space(data.bmap,
		keep_dim, nexist, keep, max_depth+2*nexist);
	if (!data.bmap)
		goto error2;

	if (data.bmap->extra) {
		int i;
		data.pos = isl_alloc_array(ctx, int, data.bmap->extra);
		if (!data.pos)
			goto error;
		for (i = 0; i < data.bmap->n_div; ++i)
			data.pos[i] = i;
	}

	map = scan_quast_r(&data, q, map);
	map = isl_map_finalize(map);
	if (!map)
		goto error2;
	if (rest) {
		*rest = isl_set_finalize(*rest);
		if (!*rest)
			goto error2;
	}
	isl_basic_map_free(data.bmap);
	if (data.pos)
		free(data.pos);
	return map;
error:
	isl_basic_set_free(context);
	isl_space_free(keep_dim);
error2:
	if (data.pos)
		free(data.pos);
	isl_basic_map_free(data.bmap);
	isl_map_free(map);
	if (rest) {
		isl_set_free(*rest);
		*rest = NULL;
	}
	return NULL;
}

static void copy_values_to(Entier *dst, isl_int *src, unsigned n)
{
	int i;

	for (i = 0; i < n; ++i)
		entier_assign(dst[i], src[i]);
}

static void copy_constraint_to(Entier *dst, isl_int *src,
		unsigned pip_param, unsigned pip_var,
		unsigned extra_front, unsigned extra_back)
{
	copy_values_to(dst+1+extra_front+pip_var+pip_param+extra_back, src, 1);
	copy_values_to(dst+1+extra_front+pip_var, src+1, pip_param);
	copy_values_to(dst+1+extra_front, src+1+pip_param, pip_var);
}

PipMatrix *isl_basic_map_to_pip(struct isl_basic_map *bmap, unsigned pip_param,
			 unsigned extra_front, unsigned extra_back)
{
	int i;
	unsigned nrow;
	unsigned ncol;
	PipMatrix *M;
	unsigned off;
	unsigned pip_var = isl_basic_map_total_dim(bmap) - pip_param;

	nrow = extra_front + bmap->n_eq + bmap->n_ineq;
	ncol = 1 + extra_front + pip_var + pip_param + extra_back + 1;
	M = pip_matrix_alloc(nrow, ncol);
	if (!M)
		return NULL;

	off = extra_front;
	for (i = 0; i < bmap->n_eq; ++i) {
		entier_set_si(M->p[off+i][0], 0);
		copy_constraint_to(M->p[off+i], bmap->eq[i],
				   pip_param, pip_var, extra_front, extra_back);
	}
	off += bmap->n_eq;
	for (i = 0; i < bmap->n_ineq; ++i) {
		entier_set_si(M->p[off+i][0], 1);
		copy_constraint_to(M->p[off+i], bmap->ineq[i],
				   pip_param, pip_var, extra_front, extra_back);
	}
	return M;
}

PipMatrix *isl_basic_set_to_pip(struct isl_basic_set *bset, unsigned pip_param,
			 unsigned extra_front, unsigned extra_back)
{
	return isl_basic_map_to_pip((struct isl_basic_map *)bset,
					pip_param, extra_front, extra_back);
}

struct isl_map *isl_pip_basic_map_lexopt(
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty, int max)
{
	PipOptions	*options;
	PipQuast	*sol;
	struct isl_map	*map;
	struct isl_ctx  *ctx;
	PipMatrix *domain = NULL, *context = NULL;
	unsigned	 nparam, n_in, n_out;

	bmap = isl_basic_map_detect_equalities(bmap);
	if (!bmap || !dom)
		goto error;

	ctx = bmap->ctx;
	isl_assert(ctx, isl_basic_map_compatible_domain(bmap, dom), goto error);
	nparam = isl_basic_map_n_param(bmap);
	n_in = isl_basic_map_n_in(bmap);
	n_out = isl_basic_map_n_out(bmap);

	domain = isl_basic_map_to_pip(bmap, nparam + n_in, 0, dom->n_div);
	if (!domain)
		goto error;
	context = isl_basic_map_to_pip((struct isl_basic_map *)dom, 0, 0, 0);
	if (!context)
		goto error;

	options = pip_options_init();
	options->Simplify = 1;
	options->Maximize = max;
	options->Urs_unknowns = -1;
	options->Urs_parms = -1;
	sol = pip_solve(domain, context, -1, options);

	if (sol) {
		struct isl_basic_set *copy;
		copy = isl_basic_set_copy(dom);
		map = isl_map_from_quast(ctx, sol,
				isl_space_copy(bmap->dim), copy, empty);
	} else {
		map = isl_map_empty_like_basic_map(bmap);
		if (empty)
			*empty = NULL;
	}
	if (!map)
		goto error;
	if (map->n == 0 && empty) {
		isl_set_free(*empty);
		*empty = isl_set_from_basic_set(dom);
	} else
		isl_basic_set_free(dom);
	isl_basic_map_free(bmap);

	pip_quast_free(sol);
	pip_options_free(options);
	pip_matrix_free(domain);
	pip_matrix_free(context);

	return map;
error:
	if (domain)
		pip_matrix_free(domain);
	if (context)
		pip_matrix_free(context);
	isl_basic_map_free(bmap);
	isl_basic_set_free(dom);
	return NULL;
}
