/*
 * Copyright 2011      INRIA Saclay
 * Copyright 2012      Ecole Normale Superieure
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France
 * and Ecole Normale Superieure, 45 rue d’Ulm, 75230 Paris, France
 */

#include <isl_ctx_private.h>
#include <isl_map_private.h>
#include <isl_local_space_private.h>
#include <isl_space_private.h>
#include <isl_mat_private.h>
#include <isl_aff_private.h>
#include <isl/seq.h>

isl_ctx *isl_local_space_get_ctx(__isl_keep isl_local_space *ls)
{
	return ls ? ls->dim->ctx : NULL;
}

__isl_give isl_local_space *isl_local_space_alloc_div(__isl_take isl_space *dim,
	__isl_take isl_mat *div)
{
	isl_ctx *ctx;
	isl_local_space *ls = NULL;

	if (!dim || !div)
		goto error;

	ctx = isl_space_get_ctx(dim);
	ls = isl_calloc_type(ctx, struct isl_local_space);
	if (!ls)
		goto error;

	ls->ref = 1;
	ls->dim = dim;
	ls->div = div;

	return ls;
error:
	isl_mat_free(div);
	isl_space_free(dim);
	isl_local_space_free(ls);
	return NULL;
}

__isl_give isl_local_space *isl_local_space_alloc(__isl_take isl_space *dim,
	unsigned n_div)
{
	isl_ctx *ctx;
	isl_mat *div;
	unsigned total;

	if (!dim)
		return NULL;

	total = isl_space_dim(dim, isl_dim_all);

	ctx = isl_space_get_ctx(dim);
	div = isl_mat_alloc(ctx, n_div, 1 + 1 + total + n_div);
	return isl_local_space_alloc_div(dim, div);
}

__isl_give isl_local_space *isl_local_space_from_space(__isl_take isl_space *dim)
{
	return isl_local_space_alloc(dim, 0);
}

__isl_give isl_local_space *isl_local_space_copy(__isl_keep isl_local_space *ls)
{
	if (!ls)
		return NULL;

	ls->ref++;
	return ls;
}

__isl_give isl_local_space *isl_local_space_dup(__isl_keep isl_local_space *ls)
{
	if (!ls)
		return NULL;

	return isl_local_space_alloc_div(isl_space_copy(ls->dim),
					 isl_mat_copy(ls->div));

}

__isl_give isl_local_space *isl_local_space_cow(__isl_take isl_local_space *ls)
{
	if (!ls)
		return NULL;

	if (ls->ref == 1)
		return ls;
	ls->ref--;
	return isl_local_space_dup(ls);
}

void *isl_local_space_free(__isl_take isl_local_space *ls)
{
	if (!ls)
		return NULL;

	if (--ls->ref > 0)
		return NULL;

	isl_space_free(ls->dim);
	isl_mat_free(ls->div);

	free(ls);

	return NULL;
}

/* Is the local space that of a set?
 */
int isl_local_space_is_set(__isl_keep isl_local_space *ls)
{
	return ls ? isl_space_is_set(ls->dim) : -1;
}

/* Return true if the two local spaces are identical, with identical
 * expressions for the integer divisions.
 */
int isl_local_space_is_equal(__isl_keep isl_local_space *ls1,
	__isl_keep isl_local_space *ls2)
{
	int equal;

	if (!ls1 || !ls2)
		return -1;

	equal = isl_space_is_equal(ls1->dim, ls2->dim);
	if (equal < 0 || !equal)
		return equal;

	if (!isl_local_space_divs_known(ls1))
		return 0;
	if (!isl_local_space_divs_known(ls2))
		return 0;

	return isl_mat_is_equal(ls1->div, ls2->div);
}

int isl_local_space_dim(__isl_keep isl_local_space *ls,
	enum isl_dim_type type)
{
	if (!ls)
		return 0;
	if (type == isl_dim_div)
		return ls->div->n_row;
	if (type == isl_dim_all)
		return isl_space_dim(ls->dim, isl_dim_all) + ls->div->n_row;
	return isl_space_dim(ls->dim, type);
}

unsigned isl_local_space_offset(__isl_keep isl_local_space *ls,
	enum isl_dim_type type)
{
	isl_space *dim;

	if (!ls)
		return 0;

	dim = ls->dim;
	switch (type) {
	case isl_dim_cst:	return 0;
	case isl_dim_param:	return 1;
	case isl_dim_in:	return 1 + dim->nparam;
	case isl_dim_out:	return 1 + dim->nparam + dim->n_in;
	case isl_dim_div:	return 1 + dim->nparam + dim->n_in + dim->n_out;
	default:		return 0;
	}
}

/* Does the given dimension have a name?
 */
int isl_local_space_has_dim_name(__isl_keep isl_local_space *ls,
	enum isl_dim_type type, unsigned pos)
{
	return ls ? isl_space_has_dim_name(ls->dim, type, pos) : -1;
}

const char *isl_local_space_get_dim_name(__isl_keep isl_local_space *ls,
	enum isl_dim_type type, unsigned pos)
{
	return ls ? isl_space_get_dim_name(ls->dim, type, pos) : NULL;
}

__isl_give isl_aff *isl_local_space_get_div(__isl_keep isl_local_space *ls,
	int pos)
{
	isl_aff *aff;

	if (!ls)
		return NULL;

	if (pos < 0 || pos >= ls->div->n_row)
		isl_die(isl_local_space_get_ctx(ls), isl_error_invalid,
			"index out of bounds", return NULL);

	if (isl_int_is_zero(ls->div->row[pos][0]))
		isl_die(isl_local_space_get_ctx(ls), isl_error_invalid,
			"expression of div unknown", return NULL);

	aff = isl_aff_alloc(isl_local_space_copy(ls));
	if (!aff)
		return NULL;
	isl_seq_cpy(aff->v->el, ls->div->row[pos], aff->v->size);
	return aff;
}

__isl_give isl_space *isl_local_space_get_space(__isl_keep isl_local_space *ls)
{
	if (!ls)
		return NULL;

	return isl_space_copy(ls->dim);
}

__isl_give isl_local_space *isl_local_space_set_dim_name(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned pos, const char *s)
{
	ls = isl_local_space_cow(ls);
	if (!ls)
		return NULL;
	ls->dim = isl_space_set_dim_name(ls->dim, type, pos, s);
	if (!ls->dim)
		return isl_local_space_free(ls);

	return ls;
}

__isl_give isl_local_space *isl_local_space_set_dim_id(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned pos, __isl_take isl_id *id)
{
	ls = isl_local_space_cow(ls);
	if (!ls)
		return isl_id_free(id);
	ls->dim = isl_space_set_dim_id(ls->dim, type, pos, id);
	if (!ls->dim)
		return isl_local_space_free(ls);

	return ls;
}

__isl_give isl_local_space *isl_local_space_reset_space(
	__isl_take isl_local_space *ls, __isl_take isl_space *dim)
{
	ls = isl_local_space_cow(ls);
	if (!ls || !dim)
		goto error;

	isl_space_free(ls->dim);
	ls->dim = dim;

	return ls;
error:
	isl_local_space_free(ls);
	isl_space_free(dim);
	return NULL;
}

/* Reorder the columns of the given div definitions according to the
 * given reordering.
 * The order of the divs themselves is assumed not to change.
 */
static __isl_give isl_mat *reorder_divs(__isl_take isl_mat *div,
	__isl_take isl_reordering *r)
{
	int i, j;
	isl_mat *mat;
	int extra;

	if (!div || !r)
		goto error;

	extra = isl_space_dim(r->dim, isl_dim_all) + div->n_row - r->len;
	mat = isl_mat_alloc(div->ctx, div->n_row, div->n_col + extra);
	if (!mat)
		goto error;

	for (i = 0; i < div->n_row; ++i) {
		isl_seq_cpy(mat->row[i], div->row[i], 2);
		isl_seq_clr(mat->row[i] + 2, mat->n_col - 2);
		for (j = 0; j < r->len; ++j)
			isl_int_set(mat->row[i][2 + r->pos[j]],
				    div->row[i][2 + j]);
	}

	isl_reordering_free(r);
	isl_mat_free(div);
	return mat;
error:
	isl_reordering_free(r);
	isl_mat_free(div);
	return NULL;
}

/* Reorder the dimensions of "ls" according to the given reordering.
 * The reordering r is assumed to have been extended with the local
 * variables, leaving them in the same order.
 */
__isl_give isl_local_space *isl_local_space_realign(
	__isl_take isl_local_space *ls, __isl_take isl_reordering *r)
{
	ls = isl_local_space_cow(ls);
	if (!ls || !r)
		goto error;

	ls->div = reorder_divs(ls->div, isl_reordering_copy(r));
	if (!ls->div)
		goto error;

	ls = isl_local_space_reset_space(ls, isl_space_copy(r->dim));

	isl_reordering_free(r);
	return ls;
error:
	isl_local_space_free(ls);
	isl_reordering_free(r);
	return NULL;
}

__isl_give isl_local_space *isl_local_space_add_div(
	__isl_take isl_local_space *ls, __isl_take isl_vec *div)
{
	ls = isl_local_space_cow(ls);
	if (!ls || !div)
		goto error;

	if (ls->div->n_col != div->size)
		isl_die(isl_local_space_get_ctx(ls), isl_error_invalid,
			"incompatible dimensions", goto error);

	ls->div = isl_mat_add_zero_cols(ls->div, 1);
	ls->div = isl_mat_add_rows(ls->div, 1);
	if (!ls->div)
		goto error;

	isl_seq_cpy(ls->div->row[ls->div->n_row - 1], div->el, div->size);
	isl_int_set_si(ls->div->row[ls->div->n_row - 1][div->size], 0);

	isl_vec_free(div);
	return ls;
error:
	isl_local_space_free(ls);
	isl_vec_free(div);
	return NULL;
}

__isl_give isl_local_space *isl_local_space_replace_divs(
	__isl_take isl_local_space *ls, __isl_take isl_mat *div)
{
	ls = isl_local_space_cow(ls);

	if (!ls || !div)
		goto error;

	isl_mat_free(ls->div);
	ls->div = div;
	return ls;
error:
	isl_mat_free(div);
	isl_local_space_free(ls);
	return NULL;
}

/* Copy row "s" of "src" to row "d" of "dst", applying the expansion
 * defined by "exp".
 */
static void expand_row(__isl_keep isl_mat *dst, int d,
	__isl_keep isl_mat *src, int s, int *exp)
{
	int i;
	unsigned c = src->n_col - src->n_row;

	isl_seq_cpy(dst->row[d], src->row[s], c);
	isl_seq_clr(dst->row[d] + c, dst->n_col - c);

	for (i = 0; i < s; ++i)
		isl_int_set(dst->row[d][c + exp[i]], src->row[s][c + i]);
}

/* Compare (known) divs.
 * Return non-zero if at least one of the two divs is unknown.
 * In particular, if both divs are unknown, we respect their
 * current order.  Otherwise, we sort the known div after the unknown
 * div only if the known div depends on the unknown div.
 */
static int cmp_row(isl_int *row_i, isl_int *row_j, int i, int j,
	unsigned n_row, unsigned n_col)
{
	int li, lj;
	int unknown_i, unknown_j;

	unknown_i = isl_int_is_zero(row_i[0]);
	unknown_j = isl_int_is_zero(row_j[0]);

	if (unknown_i && unknown_j)
		return i - j;

	if (unknown_i)
		li = n_col - n_row + i;
	else
		li = isl_seq_last_non_zero(row_i, n_col);
	if (unknown_j)
		lj = n_col - n_row + j;
	else
		lj = isl_seq_last_non_zero(row_j, n_col);

	if (li != lj)
		return li - lj;

	return isl_seq_cmp(row_i, row_j, n_col);
}

/* Call cmp_row for divs in a matrix.
 */
static int mat_cmp_row(__isl_keep isl_mat *div, int i, int j)
{
	return cmp_row(div->row[i], div->row[j], i, j, div->n_row, div->n_col);
}

/* Call cmp_row for divs in a basic map.
 */
static int bmap_cmp_row(__isl_keep isl_basic_map *bmap, int i, int j,
	unsigned total)
{
	return cmp_row(bmap->div[i], bmap->div[j], i, j, bmap->n_div, total);
}

/* Sort the divs in "bmap".
 *
 * We first make sure divs are placed after divs on which they depend.
 * Then we perform a simple insertion sort based on the same ordering
 * that is used in isl_merge_divs.
 */
__isl_give isl_basic_map *isl_basic_map_sort_divs(
	__isl_take isl_basic_map *bmap)
{
	int i, j;
	unsigned total;

	bmap = isl_basic_map_order_divs(bmap);
	if (!bmap)
		return NULL;
	if (bmap->n_div <= 1)
		return bmap;

	total = 2 + isl_basic_map_total_dim(bmap);
	for (i = 1; i < bmap->n_div; ++i) {
		for (j = i - 1; j >= 0; --j) {
			if (bmap_cmp_row(bmap, j, j + 1, total) <= 0)
				break;
			isl_basic_map_swap_div(bmap, j, j + 1);
		}
	}

	return bmap;
}

/* Sort the divs in the basic maps of "map".
 */
__isl_give isl_map *isl_map_sort_divs(__isl_take isl_map *map)
{
	return isl_map_inline_foreach_basic_map(map, &isl_basic_map_sort_divs);
}

/* Combine the two lists of divs into a single list.
 * For each row i in div1, exp1[i] is set to the position of the corresponding
 * row in the result.  Similarly for div2 and exp2.
 * This function guarantees
 *	exp1[i] >= i
 *	exp1[i+1] > exp1[i]
 * For optimal merging, the two input list should have been sorted.
 */
__isl_give isl_mat *isl_merge_divs(__isl_keep isl_mat *div1,
	__isl_keep isl_mat *div2, int *exp1, int *exp2)
{
	int i, j, k;
	isl_mat *div = NULL;
	unsigned d;

	if (!div1 || !div2)
		return NULL;

	d = div1->n_col - div1->n_row;
	div = isl_mat_alloc(div1->ctx, 1 + div1->n_row + div2->n_row,
				d + div1->n_row + div2->n_row);
	if (!div)
		return NULL;

	for (i = 0, j = 0, k = 0; i < div1->n_row && j < div2->n_row; ++k) {
		int cmp;

		expand_row(div, k, div1, i, exp1);
		expand_row(div, k + 1, div2, j, exp2);

		cmp = mat_cmp_row(div, k, k + 1);
		if (cmp == 0) {
			exp1[i++] = k;
			exp2[j++] = k;
		} else if (cmp < 0) {
			exp1[i++] = k;
		} else {
			exp2[j++] = k;
			isl_seq_cpy(div->row[k], div->row[k + 1], div->n_col);
		}
	}
	for (; i < div1->n_row; ++i, ++k) {
		expand_row(div, k, div1, i, exp1);
		exp1[i] = k;
	}
	for (; j < div2->n_row; ++j, ++k) {
		expand_row(div, k, div2, j, exp2);
		exp2[j] = k;
	}

	div->n_row = k;
	div->n_col = d + k;

	return div;
}

/* Construct a local space that contains all the divs in either
 * "ls1" or "ls2".
 */
__isl_give isl_local_space *isl_local_space_intersect(
	__isl_take isl_local_space *ls1, __isl_take isl_local_space *ls2)
{
	isl_ctx *ctx;
	int *exp1 = NULL;
	int *exp2 = NULL;
	isl_mat *div;

	if (!ls1 || !ls2)
		goto error;

	ctx = isl_local_space_get_ctx(ls1);
	if (!isl_space_is_equal(ls1->dim, ls2->dim))
		isl_die(ctx, isl_error_invalid,
			"spaces should be identical", goto error);

	if (ls2->div->n_row == 0) {
		isl_local_space_free(ls2);
		return ls1;
	}

	if (ls1->div->n_row == 0) {
		isl_local_space_free(ls1);
		return ls2;
	}

	exp1 = isl_alloc_array(ctx, int, ls1->div->n_row);
	exp2 = isl_alloc_array(ctx, int, ls2->div->n_row);
	if (!exp1 || !exp2)
		goto error;

	div = isl_merge_divs(ls1->div, ls2->div, exp1, exp2);
	if (!div)
		goto error;

	free(exp1);
	free(exp2);
	isl_local_space_free(ls2);
	isl_mat_free(ls1->div);
	ls1->div = div;

	return ls1;
error:
	free(exp1);
	free(exp2);
	isl_local_space_free(ls1);
	isl_local_space_free(ls2);
	return NULL;
}

int isl_local_space_divs_known(__isl_keep isl_local_space *ls)
{
	int i;

	if (!ls)
		return -1;

	for (i = 0; i < ls->div->n_row; ++i)
		if (isl_int_is_zero(ls->div->row[i][0]))
			return 0;

	return 1;
}

__isl_give isl_local_space *isl_local_space_domain(
	__isl_take isl_local_space *ls)
{
	ls = isl_local_space_drop_dims(ls, isl_dim_out,
					0, isl_local_space_dim(ls, isl_dim_out));
	ls = isl_local_space_cow(ls);
	if (!ls)
		return NULL;
	ls->dim = isl_space_domain(ls->dim);
	if (!ls->dim)
		return isl_local_space_free(ls);
	return ls;
}

__isl_give isl_local_space *isl_local_space_range(
	__isl_take isl_local_space *ls)
{
	ls = isl_local_space_drop_dims(ls, isl_dim_in,
					0, isl_local_space_dim(ls, isl_dim_in));
	ls = isl_local_space_cow(ls);
	if (!ls)
		return NULL;

	ls->dim = isl_space_range(ls->dim);
	if (!ls->dim)
		return isl_local_space_free(ls);
	return ls;
}

/* Construct a local space for a map that has the given local
 * space as domain and that has a zero-dimensional range.
 */
__isl_give isl_local_space *isl_local_space_from_domain(
	__isl_take isl_local_space *ls)
{
	ls = isl_local_space_cow(ls);
	if (!ls)
		return NULL;
	ls->dim = isl_space_from_domain(ls->dim);
	if (!ls->dim)
		return isl_local_space_free(ls);
	return ls;
}

__isl_give isl_local_space *isl_local_space_add_dims(
	__isl_take isl_local_space *ls, enum isl_dim_type type, unsigned n)
{
	int pos;

	if (!ls)
		return NULL;
	pos = isl_local_space_dim(ls, type);
	return isl_local_space_insert_dims(ls, type, pos, n);
}

/* Remove common factor of non-constant terms and denominator.
 */
static void normalize_div(__isl_keep isl_local_space *ls, int div)
{
	isl_ctx *ctx = ls->div->ctx;
	unsigned total = ls->div->n_col - 2;

	isl_seq_gcd(ls->div->row[div] + 2, total, &ctx->normalize_gcd);
	isl_int_gcd(ctx->normalize_gcd,
		    ctx->normalize_gcd, ls->div->row[div][0]);
	if (isl_int_is_one(ctx->normalize_gcd))
		return;

	isl_seq_scale_down(ls->div->row[div] + 2, ls->div->row[div] + 2,
			    ctx->normalize_gcd, total);
	isl_int_divexact(ls->div->row[div][0], ls->div->row[div][0],
			    ctx->normalize_gcd);
	isl_int_fdiv_q(ls->div->row[div][1], ls->div->row[div][1],
			    ctx->normalize_gcd);
}

/* Exploit the equalities in "eq" to simplify the expressions of
 * the integer divisions in "ls".
 * The integer divisions in "ls" are assumed to appear as regular
 * dimensions in "eq".
 */
__isl_give isl_local_space *isl_local_space_substitute_equalities(
	__isl_take isl_local_space *ls, __isl_take isl_basic_set *eq)
{
	int i, j, k;
	unsigned total;
	unsigned n_div;

	ls = isl_local_space_cow(ls);
	if (!ls || !eq)
		goto error;

	total = isl_space_dim(eq->dim, isl_dim_all);
	if (isl_local_space_dim(ls, isl_dim_all) != total)
		isl_die(isl_local_space_get_ctx(ls), isl_error_invalid,
			"dimensions don't match", goto error);
	total++;
	n_div = eq->n_div;
	for (i = 0; i < eq->n_eq; ++i) {
		j = isl_seq_last_non_zero(eq->eq[i], total + n_div);
		if (j < 0 || j == 0 || j >= total)
			continue;

		for (k = 0; k < ls->div->n_row; ++k) {
			if (isl_int_is_zero(ls->div->row[k][1 + j]))
				continue;
			isl_seq_elim(ls->div->row[k] + 1, eq->eq[i], j, total,
					&ls->div->row[k][0]);
			normalize_div(ls, k);
		}
	}

	isl_basic_set_free(eq);
	return ls;
error:
	isl_basic_set_free(eq);
	isl_local_space_free(ls);
	return NULL;
}

/* Plug in "subs" for dimension "type", "pos" in the integer divisions
 * of "ls".
 *
 * Let i be the dimension to replace and let "subs" be of the form
 *
 *	f/d
 *
 * Any integer division with a non-zero coefficient for i,
 *
 *	floor((a i + g)/m)
 *
 * is replaced by
 *
 *	floor((a f + d g)/(m d))
 */
__isl_give isl_local_space *isl_local_space_substitute(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned pos, __isl_keep isl_aff *subs)
{
	int i;
	isl_int v;

	ls = isl_local_space_cow(ls);
	if (!ls || !subs)
		return isl_local_space_free(ls);

	if (!isl_space_is_equal(ls->dim, subs->ls->dim))
		isl_die(isl_local_space_get_ctx(ls), isl_error_invalid,
			"spaces don't match", return isl_local_space_free(ls));
	if (isl_local_space_dim(subs->ls, isl_dim_div) != 0)
		isl_die(isl_local_space_get_ctx(ls), isl_error_unsupported,
			"cannot handle divs yet",
			return isl_local_space_free(ls));

	pos += isl_local_space_offset(ls, type);

	isl_int_init(v);
	for (i = 0; i < ls->div->n_row; ++i) {
		if (isl_int_is_zero(ls->div->row[i][1 + pos]))
			continue;
		isl_int_set(v, ls->div->row[i][1 + pos]);
		isl_int_set_si(ls->div->row[i][1 + pos], 0);
		isl_seq_combine(ls->div->row[i] + 1,
				subs->v->el[0], ls->div->row[i] + 1,
				v, subs->v->el + 1, subs->v->size - 1);
		isl_int_mul(ls->div->row[i][0],
			    ls->div->row[i][0], subs->v->el[0]);
		normalize_div(ls, i);
	}
	isl_int_clear(v);

	return ls;
}

int isl_local_space_is_named_or_nested(__isl_keep isl_local_space *ls,
	enum isl_dim_type type)
{
	if (!ls)
		return -1;
	return isl_space_is_named_or_nested(ls->dim, type);
}

__isl_give isl_local_space *isl_local_space_drop_dims(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	isl_ctx *ctx;

	if (!ls)
		return NULL;
	if (n == 0 && !isl_local_space_is_named_or_nested(ls, type))
		return ls;

	ctx = isl_local_space_get_ctx(ls);
	if (first + n > isl_local_space_dim(ls, type))
		isl_die(ctx, isl_error_invalid, "range out of bounds",
			return isl_local_space_free(ls));

	ls = isl_local_space_cow(ls);
	if (!ls)
		return NULL;

	if (type == isl_dim_div) {
		ls->div = isl_mat_drop_rows(ls->div, first, n);
	} else {
		ls->dim = isl_space_drop_dims(ls->dim, type, first, n);
		if (!ls->dim)
			return isl_local_space_free(ls);
	}

	first += 1 + isl_local_space_offset(ls, type);
	ls->div = isl_mat_drop_cols(ls->div, first, n);
	if (!ls->div)
		return isl_local_space_free(ls);

	return ls;
}

__isl_give isl_local_space *isl_local_space_insert_dims(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	isl_ctx *ctx;

	if (!ls)
		return NULL;
	if (n == 0 && !isl_local_space_is_named_or_nested(ls, type))
		return ls;

	ctx = isl_local_space_get_ctx(ls);
	if (first > isl_local_space_dim(ls, type))
		isl_die(ctx, isl_error_invalid, "position out of bounds",
			return isl_local_space_free(ls));

	ls = isl_local_space_cow(ls);
	if (!ls)
		return NULL;

	if (type == isl_dim_div) {
		ls->div = isl_mat_insert_zero_rows(ls->div, first, n);
	} else {
		ls->dim = isl_space_insert_dims(ls->dim, type, first, n);
		if (!ls->dim)
			return isl_local_space_free(ls);
	}

	first += 1 + isl_local_space_offset(ls, type);
	ls->div = isl_mat_insert_zero_cols(ls->div, first, n);
	if (!ls->div)
		return isl_local_space_free(ls);

	return ls;
}

/* Check if the constraints pointed to by "constraint" is a div
 * constraint corresponding to div "div" in "ls".
 *
 * That is, if div = floor(f/m), then check if the constraint is
 *
 *		f - m d >= 0
 * or
 *		-(f-(m-1)) + m d >= 0
 */
int isl_local_space_is_div_constraint(__isl_keep isl_local_space *ls,
	isl_int *constraint, unsigned div)
{
	unsigned pos;

	if (!ls)
		return -1;

	if (isl_int_is_zero(ls->div->row[div][0]))
		return 0;

	pos = isl_local_space_offset(ls, isl_dim_div) + div;

	if (isl_int_eq(constraint[pos], ls->div->row[div][0])) {
		int neg;
		isl_int_sub(ls->div->row[div][1],
				ls->div->row[div][1], ls->div->row[div][0]);
		isl_int_add_ui(ls->div->row[div][1], ls->div->row[div][1], 1);
		neg = isl_seq_is_neg(constraint, ls->div->row[div]+1, pos);
		isl_int_sub_ui(ls->div->row[div][1], ls->div->row[div][1], 1);
		isl_int_add(ls->div->row[div][1],
				ls->div->row[div][1], ls->div->row[div][0]);
		if (!neg)
			return 0;
		if (isl_seq_first_non_zero(constraint+pos+1,
					    ls->div->n_row-div-1) != -1)
			return 0;
	} else if (isl_int_abs_eq(constraint[pos], ls->div->row[div][0])) {
		if (!isl_seq_eq(constraint, ls->div->row[div]+1, pos))
			return 0;
		if (isl_seq_first_non_zero(constraint+pos+1,
					    ls->div->n_row-div-1) != -1)
			return 0;
	} else
		return 0;

	return 1;
}

/*
 * Set active[i] to 1 if the dimension at position i is involved
 * in the linear expression l.
 */
int *isl_local_space_get_active(__isl_keep isl_local_space *ls, isl_int *l)
{
	int i, j;
	isl_ctx *ctx;
	int *active = NULL;
	unsigned total;
	unsigned offset;

	ctx = isl_local_space_get_ctx(ls);
	total = isl_local_space_dim(ls, isl_dim_all);
	active = isl_calloc_array(ctx, int, total);
	if (!active)
		return NULL;

	for (i = 0; i < total; ++i)
		active[i] = !isl_int_is_zero(l[i]);

	offset = isl_local_space_offset(ls, isl_dim_div) - 1;
	for (i = ls->div->n_row - 1; i >= 0; --i) {
		if (!active[offset + i])
			continue;
		for (j = 0; j < total; ++j)
			active[j] |= !isl_int_is_zero(ls->div->row[i][2 + j]);
	}

	return active;
}

/* Given a local space "ls" of a set, create a local space
 * for the lift of the set.  In particular, the result
 * is of the form [dim -> local[..]], with ls->div->n_row variables in the
 * range of the wrapped map.
 */
__isl_give isl_local_space *isl_local_space_lift(
	__isl_take isl_local_space *ls)
{
	ls = isl_local_space_cow(ls);
	if (!ls)
		return NULL;

	ls->dim = isl_space_lift(ls->dim, ls->div->n_row);
	ls->div = isl_mat_drop_rows(ls->div, 0, ls->div->n_row);
	if (!ls->dim || !ls->div)
		return isl_local_space_free(ls);

	return ls;
}

/* Construct a basic map that maps a set living in local space "ls"
 * to the corresponding lifted local space.
 */
__isl_give isl_basic_map *isl_local_space_lifting(
	__isl_take isl_local_space *ls)
{
	isl_basic_map *lifting;
	isl_basic_set *bset;

	if (!ls)
		return NULL;
	if (!isl_local_space_is_set(ls))
		isl_die(isl_local_space_get_ctx(ls), isl_error_invalid,
			"lifting only defined on set spaces",
			return isl_local_space_free(ls));

	bset = isl_basic_set_from_local_space(ls);
	lifting = isl_basic_set_unwrap(isl_basic_set_lift(bset));
	lifting = isl_basic_map_domain_map(lifting);
	lifting = isl_basic_map_reverse(lifting);

	return lifting;
}
