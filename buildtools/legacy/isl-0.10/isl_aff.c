/*
 * Copyright 2011      INRIA Saclay
 * Copyright 2011      Sven Verdoolaege
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
#define ISL_DIM_H
#include <isl_map_private.h>
#include <isl_union_map_private.h>
#include <isl_aff_private.h>
#include <isl_space_private.h>
#include <isl_local_space_private.h>
#include <isl_mat_private.h>
#include <isl_list_private.h>
#include <isl/constraint.h>
#include <isl/seq.h>
#include <isl/set.h>
#include <isl_config.h>

__isl_give isl_aff *isl_aff_alloc_vec(__isl_take isl_local_space *ls,
	__isl_take isl_vec *v)
{
	isl_aff *aff;

	if (!ls || !v)
		goto error;

	aff = isl_calloc_type(v->ctx, struct isl_aff);
	if (!aff)
		goto error;

	aff->ref = 1;
	aff->ls = ls;
	aff->v = v;

	return aff;
error:
	isl_local_space_free(ls);
	isl_vec_free(v);
	return NULL;
}

__isl_give isl_aff *isl_aff_alloc(__isl_take isl_local_space *ls)
{
	isl_ctx *ctx;
	isl_vec *v;
	unsigned total;

	if (!ls)
		return NULL;

	ctx = isl_local_space_get_ctx(ls);
	if (!isl_local_space_divs_known(ls))
		isl_die(ctx, isl_error_invalid, "local space has unknown divs",
			goto error);
	if (!isl_local_space_is_set(ls))
		isl_die(ctx, isl_error_invalid,
			"domain of affine expression should be a set",
			goto error);

	total = isl_local_space_dim(ls, isl_dim_all);
	v = isl_vec_alloc(ctx, 1 + 1 + total);
	return isl_aff_alloc_vec(ls, v);
error:
	isl_local_space_free(ls);
	return NULL;
}

__isl_give isl_aff *isl_aff_zero_on_domain(__isl_take isl_local_space *ls)
{
	isl_aff *aff;

	aff = isl_aff_alloc(ls);
	if (!aff)
		return NULL;

	isl_int_set_si(aff->v->el[0], 1);
	isl_seq_clr(aff->v->el + 1, aff->v->size - 1);

	return aff;
}

__isl_give isl_aff *isl_aff_copy(__isl_keep isl_aff *aff)
{
	if (!aff)
		return NULL;

	aff->ref++;
	return aff;
}

__isl_give isl_aff *isl_aff_dup(__isl_keep isl_aff *aff)
{
	if (!aff)
		return NULL;

	return isl_aff_alloc_vec(isl_local_space_copy(aff->ls),
				 isl_vec_copy(aff->v));
}

__isl_give isl_aff *isl_aff_cow(__isl_take isl_aff *aff)
{
	if (!aff)
		return NULL;

	if (aff->ref == 1)
		return aff;
	aff->ref--;
	return isl_aff_dup(aff);
}

void *isl_aff_free(__isl_take isl_aff *aff)
{
	if (!aff)
		return NULL;

	if (--aff->ref > 0)
		return NULL;

	isl_local_space_free(aff->ls);
	isl_vec_free(aff->v);

	free(aff);

	return NULL;
}

isl_ctx *isl_aff_get_ctx(__isl_keep isl_aff *aff)
{
	return aff ? isl_local_space_get_ctx(aff->ls) : NULL;
}

/* Externally, an isl_aff has a map space, but internally, the
 * ls field corresponds to the domain of that space.
 */
int isl_aff_dim(__isl_keep isl_aff *aff, enum isl_dim_type type)
{
	if (!aff)
		return 0;
	if (type == isl_dim_out)
		return 1;
	if (type == isl_dim_in)
		type = isl_dim_set;
	return isl_local_space_dim(aff->ls, type);
}

__isl_give isl_space *isl_aff_get_domain_space(__isl_keep isl_aff *aff)
{
	return aff ? isl_local_space_get_space(aff->ls) : NULL;
}

__isl_give isl_space *isl_aff_get_space(__isl_keep isl_aff *aff)
{
	isl_space *space;
	if (!aff)
		return NULL;
	space = isl_local_space_get_space(aff->ls);
	space = isl_space_from_domain(space);
	space = isl_space_add_dims(space, isl_dim_out, 1);
	return space;
}

__isl_give isl_local_space *isl_aff_get_domain_local_space(
	__isl_keep isl_aff *aff)
{
	return aff ? isl_local_space_copy(aff->ls) : NULL;
}

__isl_give isl_local_space *isl_aff_get_local_space(__isl_keep isl_aff *aff)
{
	isl_local_space *ls;
	if (!aff)
		return NULL;
	ls = isl_local_space_copy(aff->ls);
	ls = isl_local_space_from_domain(ls);
	ls = isl_local_space_add_dims(ls, isl_dim_out, 1);
	return ls;
}

/* Externally, an isl_aff has a map space, but internally, the
 * ls field corresponds to the domain of that space.
 */
const char *isl_aff_get_dim_name(__isl_keep isl_aff *aff,
	enum isl_dim_type type, unsigned pos)
{
	if (!aff)
		return NULL;
	if (type == isl_dim_out)
		return NULL;
	if (type == isl_dim_in)
		type = isl_dim_set;
	return isl_local_space_get_dim_name(aff->ls, type, pos);
}

__isl_give isl_aff *isl_aff_reset_domain_space(__isl_take isl_aff *aff,
	__isl_take isl_space *dim)
{
	aff = isl_aff_cow(aff);
	if (!aff || !dim)
		goto error;

	aff->ls = isl_local_space_reset_space(aff->ls, dim);
	if (!aff->ls)
		return isl_aff_free(aff);

	return aff;
error:
	isl_aff_free(aff);
	isl_space_free(dim);
	return NULL;
}

/* Reset the space of "aff".  This function is called from isl_pw_templ.c
 * and doesn't know if the space of an element object is represented
 * directly or through its domain.  It therefore passes along both.
 */
__isl_give isl_aff *isl_aff_reset_space_and_domain(__isl_take isl_aff *aff,
	__isl_take isl_space *space, __isl_take isl_space *domain)
{
	isl_space_free(space);
	return isl_aff_reset_domain_space(aff, domain);
}

/* Reorder the coefficients of the affine expression based
 * on the given reodering.
 * The reordering r is assumed to have been extended with the local
 * variables.
 */
static __isl_give isl_vec *vec_reorder(__isl_take isl_vec *vec,
	__isl_take isl_reordering *r, int n_div)
{
	isl_vec *res;
	int i;

	if (!vec || !r)
		goto error;

	res = isl_vec_alloc(vec->ctx,
			    2 + isl_space_dim(r->dim, isl_dim_all) + n_div);
	isl_seq_cpy(res->el, vec->el, 2);
	isl_seq_clr(res->el + 2, res->size - 2);
	for (i = 0; i < r->len; ++i)
		isl_int_set(res->el[2 + r->pos[i]], vec->el[2 + i]);

	isl_reordering_free(r);
	isl_vec_free(vec);
	return res;
error:
	isl_vec_free(vec);
	isl_reordering_free(r);
	return NULL;
}

/* Reorder the dimensions of the domain of "aff" according
 * to the given reordering.
 */
__isl_give isl_aff *isl_aff_realign_domain(__isl_take isl_aff *aff,
	__isl_take isl_reordering *r)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		goto error;

	r = isl_reordering_extend(r, aff->ls->div->n_row);
	aff->v = vec_reorder(aff->v, isl_reordering_copy(r),
				aff->ls->div->n_row);
	aff->ls = isl_local_space_realign(aff->ls, r);

	if (!aff->v || !aff->ls)
		return isl_aff_free(aff);

	return aff;
error:
	isl_aff_free(aff);
	isl_reordering_free(r);
	return NULL;
}

__isl_give isl_aff *isl_aff_align_params(__isl_take isl_aff *aff,
	__isl_take isl_space *model)
{
	if (!aff || !model)
		goto error;

	if (!isl_space_match(aff->ls->dim, isl_dim_param,
			     model, isl_dim_param)) {
		isl_reordering *exp;

		model = isl_space_drop_dims(model, isl_dim_in,
					0, isl_space_dim(model, isl_dim_in));
		model = isl_space_drop_dims(model, isl_dim_out,
					0, isl_space_dim(model, isl_dim_out));
		exp = isl_parameter_alignment_reordering(aff->ls->dim, model);
		exp = isl_reordering_extend_space(exp,
					isl_aff_get_domain_space(aff));
		aff = isl_aff_realign_domain(aff, exp);
	}

	isl_space_free(model);
	return aff;
error:
	isl_space_free(model);
	isl_aff_free(aff);
	return NULL;
}

int isl_aff_plain_is_zero(__isl_keep isl_aff *aff)
{
	if (!aff)
		return -1;

	return isl_seq_first_non_zero(aff->v->el + 1, aff->v->size - 1) < 0;
}

int isl_aff_plain_is_equal(__isl_keep isl_aff *aff1, __isl_keep isl_aff *aff2)
{
	int equal;

	if (!aff1 || !aff2)
		return -1;

	equal = isl_local_space_is_equal(aff1->ls, aff2->ls);
	if (equal < 0 || !equal)
		return equal;

	return isl_vec_is_equal(aff1->v, aff2->v);
}

int isl_aff_get_denominator(__isl_keep isl_aff *aff, isl_int *v)
{
	if (!aff)
		return -1;
	isl_int_set(*v, aff->v->el[0]);
	return 0;
}

int isl_aff_get_constant(__isl_keep isl_aff *aff, isl_int *v)
{
	if (!aff)
		return -1;
	isl_int_set(*v, aff->v->el[1]);
	return 0;
}

int isl_aff_get_coefficient(__isl_keep isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int *v)
{
	if (!aff)
		return -1;

	if (type == isl_dim_out)
		isl_die(aff->v->ctx, isl_error_invalid,
			"output/set dimension does not have a coefficient",
			return -1);
	if (type == isl_dim_in)
		type = isl_dim_set;

	if (pos >= isl_local_space_dim(aff->ls, type))
		isl_die(aff->v->ctx, isl_error_invalid,
			"position out of bounds", return -1);

	pos += isl_local_space_offset(aff->ls, type);
	isl_int_set(*v, aff->v->el[1 + pos]);

	return 0;
}

__isl_give isl_aff *isl_aff_set_denominator(__isl_take isl_aff *aff, isl_int v)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_set(aff->v->el[0], v);

	return aff;
}

__isl_give isl_aff *isl_aff_set_constant(__isl_take isl_aff *aff, isl_int v)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_set(aff->v->el[1], v);

	return aff;
}

__isl_give isl_aff *isl_aff_add_constant(__isl_take isl_aff *aff, isl_int v)
{
	if (isl_int_is_zero(v))
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_addmul(aff->v->el[1], aff->v->el[0], v);

	return aff;
}

__isl_give isl_aff *isl_aff_add_constant_si(__isl_take isl_aff *aff, int v)
{
	isl_int t;

	isl_int_init(t);
	isl_int_set_si(t, v);
	aff = isl_aff_add_constant(aff, t);
	isl_int_clear(t);

	return aff;
}

__isl_give isl_aff *isl_aff_set_constant_si(__isl_take isl_aff *aff, int v)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_set_si(aff->v->el[1], v);

	return aff;
}

__isl_give isl_aff *isl_aff_set_coefficient(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int v)
{
	if (!aff)
		return NULL;

	if (type == isl_dim_out)
		isl_die(aff->v->ctx, isl_error_invalid,
			"output/set dimension does not have a coefficient",
			return isl_aff_free(aff));
	if (type == isl_dim_in)
		type = isl_dim_set;

	if (pos >= isl_local_space_dim(aff->ls, type))
		isl_die(aff->v->ctx, isl_error_invalid,
			"position out of bounds", return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	pos += isl_local_space_offset(aff->ls, type);
	isl_int_set(aff->v->el[1 + pos], v);

	return aff;
}

__isl_give isl_aff *isl_aff_set_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v)
{
	if (!aff)
		return NULL;

	if (type == isl_dim_out)
		isl_die(aff->v->ctx, isl_error_invalid,
			"output/set dimension does not have a coefficient",
			return isl_aff_free(aff));
	if (type == isl_dim_in)
		type = isl_dim_set;

	if (pos >= isl_local_space_dim(aff->ls, type))
		isl_die(aff->v->ctx, isl_error_invalid,
			"position out of bounds", return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	pos += isl_local_space_offset(aff->ls, type);
	isl_int_set_si(aff->v->el[1 + pos], v);

	return aff;
}

__isl_give isl_aff *isl_aff_add_coefficient(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int v)
{
	if (!aff)
		return NULL;

	if (type == isl_dim_out)
		isl_die(aff->v->ctx, isl_error_invalid,
			"output/set dimension does not have a coefficient",
			return isl_aff_free(aff));
	if (type == isl_dim_in)
		type = isl_dim_set;

	if (pos >= isl_local_space_dim(aff->ls, type))
		isl_die(aff->v->ctx, isl_error_invalid,
			"position out of bounds", return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	pos += isl_local_space_offset(aff->ls, type);
	isl_int_addmul(aff->v->el[1 + pos], aff->v->el[0], v);

	return aff;
}

__isl_give isl_aff *isl_aff_add_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v)
{
	isl_int t;

	isl_int_init(t);
	isl_int_set_si(t, v);
	aff = isl_aff_add_coefficient(aff, type, pos, t);
	isl_int_clear(t);

	return aff;
}

__isl_give isl_aff *isl_aff_get_div(__isl_keep isl_aff *aff, int pos)
{
	if (!aff)
		return NULL;

	return isl_local_space_get_div(aff->ls, pos);
}

__isl_give isl_aff *isl_aff_neg(__isl_take isl_aff *aff)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;
	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_seq_neg(aff->v->el + 1, aff->v->el + 1, aff->v->size - 1);

	return aff;
}

/* Remove divs from the local space that do not appear in the affine
 * expression.
 * We currently only remove divs at the end.
 * Some intermediate divs may also not appear directly in the affine
 * expression, but we would also need to check that no other divs are
 * defined in terms of them.
 */
__isl_give isl_aff *isl_aff_remove_unused_divs( __isl_take isl_aff *aff)
{
	int pos;
	int off;
	int n;

	if (!aff)
		return NULL;

	n = isl_local_space_dim(aff->ls, isl_dim_div);
	off = isl_local_space_offset(aff->ls, isl_dim_div);

	pos = isl_seq_last_non_zero(aff->v->el + 1 + off, n) + 1;
	if (pos == n)
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->ls = isl_local_space_drop_dims(aff->ls, isl_dim_div, pos, n - pos);
	aff->v = isl_vec_drop_els(aff->v, 1 + off + pos, n - pos);
	if (!aff->ls || !aff->v)
		return isl_aff_free(aff);

	return aff;
}

__isl_give isl_aff *isl_aff_normalize(__isl_take isl_aff *aff)
{
	if (!aff)
		return NULL;
	aff->v = isl_vec_normalize(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);
	aff = isl_aff_remove_unused_divs(aff);
	return aff;
}

/* Given f, return floor(f).
 * If f is an integer expression, then just return f.
 * If f is a constant, then return the constant floor(f).
 * Otherwise, if f = g/m, write g = q m + r,
 * create a new div d = [r/m] and return the expression q + d.
 * The coefficients in r are taken to lie between -m/2 and m/2.
 */
__isl_give isl_aff *isl_aff_floor(__isl_take isl_aff *aff)
{
	int i;
	int size;
	isl_ctx *ctx;
	isl_vec *div;

	if (!aff)
		return NULL;

	if (isl_int_is_one(aff->v->el[0]))
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	if (isl_aff_is_cst(aff)) {
		isl_int_fdiv_q(aff->v->el[1], aff->v->el[1], aff->v->el[0]);
		isl_int_set_si(aff->v->el[0], 1);
		return aff;
	}

	div = isl_vec_copy(aff->v);
	div = isl_vec_cow(div);
	if (!div)
		return isl_aff_free(aff);

	ctx = isl_aff_get_ctx(aff);
	isl_int_fdiv_q(aff->v->el[0], aff->v->el[0], ctx->two);
	for (i = 1; i < aff->v->size; ++i) {
		isl_int_fdiv_r(div->el[i], div->el[i], div->el[0]);
		isl_int_fdiv_q(aff->v->el[i], aff->v->el[i], div->el[0]);
		if (isl_int_gt(div->el[i], aff->v->el[0])) {
			isl_int_sub(div->el[i], div->el[i], div->el[0]);
			isl_int_add_ui(aff->v->el[i], aff->v->el[i], 1);
		}
	}

	aff->ls = isl_local_space_add_div(aff->ls, div);
	if (!aff->ls)
		return isl_aff_free(aff);

	size = aff->v->size;
	aff->v = isl_vec_extend(aff->v, size + 1);
	if (!aff->v)
		return isl_aff_free(aff);
	isl_int_set_si(aff->v->el[0], 1);
	isl_int_set_si(aff->v->el[size], 1);

	return aff;
}

/* Compute
 *
 *	aff mod m = aff - m * floor(aff/m)
 */
__isl_give isl_aff *isl_aff_mod(__isl_take isl_aff *aff, isl_int m)
{
	isl_aff *res;

	res = isl_aff_copy(aff);
	aff = isl_aff_scale_down(aff, m);
	aff = isl_aff_floor(aff);
	aff = isl_aff_scale(aff, m);
	res = isl_aff_sub(res, aff);

	return res;
}

/* Compute
 *
 *	pwaff mod m = pwaff - m * floor(pwaff/m)
 */
__isl_give isl_pw_aff *isl_pw_aff_mod(__isl_take isl_pw_aff *pwaff, isl_int m)
{
	isl_pw_aff *res;

	res = isl_pw_aff_copy(pwaff);
	pwaff = isl_pw_aff_scale_down(pwaff, m);
	pwaff = isl_pw_aff_floor(pwaff);
	pwaff = isl_pw_aff_scale(pwaff, m);
	res = isl_pw_aff_sub(res, pwaff);

	return res;
}

/* Given f, return ceil(f).
 * If f is an integer expression, then just return f.
 * Otherwise, create a new div d = [-f] and return the expression -d.
 */
__isl_give isl_aff *isl_aff_ceil(__isl_take isl_aff *aff)
{
	if (!aff)
		return NULL;

	if (isl_int_is_one(aff->v->el[0]))
		return aff;

	aff = isl_aff_neg(aff);
	aff = isl_aff_floor(aff);
	aff = isl_aff_neg(aff);

	return aff;
}

/* Apply the expansion computed by isl_merge_divs.
 * The expansion itself is given by "exp" while the resulting
 * list of divs is given by "div".
 */
__isl_give isl_aff *isl_aff_expand_divs( __isl_take isl_aff *aff,
	__isl_take isl_mat *div, int *exp)
{
	int i, j;
	int old_n_div;
	int new_n_div;
	int offset;

	aff = isl_aff_cow(aff);
	if (!aff || !div)
		goto error;

	old_n_div = isl_local_space_dim(aff->ls, isl_dim_div);
	new_n_div = isl_mat_rows(div);
	if (new_n_div < old_n_div)
		isl_die(isl_mat_get_ctx(div), isl_error_invalid,
			"not an expansion", goto error);

	aff->v = isl_vec_extend(aff->v, aff->v->size + new_n_div - old_n_div);
	if (!aff->v)
		goto error;

	offset = 1 + isl_local_space_offset(aff->ls, isl_dim_div);
	j = old_n_div - 1;
	for (i = new_n_div - 1; i >= 0; --i) {
		if (j >= 0 && exp[j] == i) {
			if (i != j)
				isl_int_swap(aff->v->el[offset + i],
					     aff->v->el[offset + j]);
			j--;
		} else
			isl_int_set_si(aff->v->el[offset + i], 0);
	}

	aff->ls = isl_local_space_replace_divs(aff->ls, isl_mat_copy(div));
	if (!aff->ls)
		goto error;
	isl_mat_free(div);
	return aff;
error:
	isl_aff_free(aff);
	isl_mat_free(div);
	return NULL;
}

/* Add two affine expressions that live in the same local space.
 */
static __isl_give isl_aff *add_expanded(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	isl_int gcd, f;

	aff1 = isl_aff_cow(aff1);
	if (!aff1 || !aff2)
		goto error;

	aff1->v = isl_vec_cow(aff1->v);
	if (!aff1->v)
		goto error;

	isl_int_init(gcd);
	isl_int_init(f);
	isl_int_gcd(gcd, aff1->v->el[0], aff2->v->el[0]);
	isl_int_divexact(f, aff2->v->el[0], gcd);
	isl_seq_scale(aff1->v->el + 1, aff1->v->el + 1, f, aff1->v->size - 1);
	isl_int_divexact(f, aff1->v->el[0], gcd);
	isl_seq_addmul(aff1->v->el + 1, f, aff2->v->el + 1, aff1->v->size - 1);
	isl_int_divexact(f, aff2->v->el[0], gcd);
	isl_int_mul(aff1->v->el[0], aff1->v->el[0], f);
	isl_int_clear(f);
	isl_int_clear(gcd);

	isl_aff_free(aff2);
	return aff1;
error:
	isl_aff_free(aff1);
	isl_aff_free(aff2);
	return NULL;
}

__isl_give isl_aff *isl_aff_add(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	isl_ctx *ctx;
	int *exp1 = NULL;
	int *exp2 = NULL;
	isl_mat *div;

	if (!aff1 || !aff2)
		goto error;

	ctx = isl_aff_get_ctx(aff1);
	if (!isl_space_is_equal(aff1->ls->dim, aff2->ls->dim))
		isl_die(ctx, isl_error_invalid,
			"spaces don't match", goto error);

	if (aff1->ls->div->n_row == 0 && aff2->ls->div->n_row == 0)
		return add_expanded(aff1, aff2);

	exp1 = isl_alloc_array(ctx, int, aff1->ls->div->n_row);
	exp2 = isl_alloc_array(ctx, int, aff2->ls->div->n_row);
	if (!exp1 || !exp2)
		goto error;

	div = isl_merge_divs(aff1->ls->div, aff2->ls->div, exp1, exp2);
	aff1 = isl_aff_expand_divs(aff1, isl_mat_copy(div), exp1);
	aff2 = isl_aff_expand_divs(aff2, div, exp2);
	free(exp1);
	free(exp2);

	return add_expanded(aff1, aff2);
error:
	free(exp1);
	free(exp2);
	isl_aff_free(aff1);
	isl_aff_free(aff2);
	return NULL;
}

__isl_give isl_aff *isl_aff_sub(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	return isl_aff_add(aff1, isl_aff_neg(aff2));
}

__isl_give isl_aff *isl_aff_scale(__isl_take isl_aff *aff, isl_int f)
{
	isl_int gcd;

	if (isl_int_is_one(f))
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;
	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_init(gcd);
	isl_int_gcd(gcd, aff->v->el[0], f);
	isl_int_divexact(aff->v->el[0], aff->v->el[0], gcd);
	isl_int_divexact(gcd, f, gcd);
	isl_seq_scale(aff->v->el + 1, aff->v->el + 1, gcd, aff->v->size - 1);
	isl_int_clear(gcd);

	return aff;
}

__isl_give isl_aff *isl_aff_scale_down(__isl_take isl_aff *aff, isl_int f)
{
	isl_int gcd;

	if (isl_int_is_one(f))
		return aff;

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;
	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	isl_int_init(gcd);
	isl_seq_gcd(aff->v->el + 1, aff->v->size - 1, &gcd);
	isl_int_gcd(gcd, gcd, f);
	isl_seq_scale_down(aff->v->el + 1, aff->v->el + 1, gcd, aff->v->size - 1);
	isl_int_divexact(gcd, f, gcd);
	isl_int_mul(aff->v->el[0], aff->v->el[0], gcd);
	isl_int_clear(gcd);

	return aff;
}

__isl_give isl_aff *isl_aff_scale_down_ui(__isl_take isl_aff *aff, unsigned f)
{
	isl_int v;

	if (f == 1)
		return aff;

	isl_int_init(v);
	isl_int_set_ui(v, f);
	aff = isl_aff_scale_down(aff, v);
	isl_int_clear(v);

	return aff;
}

__isl_give isl_aff *isl_aff_set_dim_name(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned pos, const char *s)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;
	if (type == isl_dim_out)
		isl_die(aff->v->ctx, isl_error_invalid,
			"cannot set name of output/set dimension",
			return isl_aff_free(aff));
	if (type == isl_dim_in)
		type = isl_dim_set;
	aff->ls = isl_local_space_set_dim_name(aff->ls, type, pos, s);
	if (!aff->ls)
		return isl_aff_free(aff);

	return aff;
}

__isl_give isl_aff *isl_aff_set_dim_id(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned pos, __isl_take isl_id *id)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return isl_id_free(id);
	if (type == isl_dim_out)
		isl_die(aff->v->ctx, isl_error_invalid,
			"cannot set name of output/set dimension",
			goto error);
	if (type == isl_dim_in)
		type = isl_dim_set;
	aff->ls = isl_local_space_set_dim_id(aff->ls, type, pos, id);
	if (!aff->ls)
		return isl_aff_free(aff);

	return aff;
error:
	isl_id_free(id);
	isl_aff_free(aff);
	return NULL;
}

/* Exploit the equalities in "eq" to simplify the affine expression
 * and the expressions of the integer divisions in the local space.
 * The integer divisions in this local space are assumed to appear
 * as regular dimensions in "eq".
 */
static __isl_give isl_aff *isl_aff_substitute_equalities_lifted(
	__isl_take isl_aff *aff, __isl_take isl_basic_set *eq)
{
	int i, j;
	unsigned total;
	unsigned n_div;

	if (!eq)
		goto error;
	if (eq->n_eq == 0) {
		isl_basic_set_free(eq);
		return aff;
	}

	aff = isl_aff_cow(aff);
	if (!aff)
		goto error;

	aff->ls = isl_local_space_substitute_equalities(aff->ls,
							isl_basic_set_copy(eq));
	if (!aff->ls)
		goto error;

	total = 1 + isl_space_dim(eq->dim, isl_dim_all);
	n_div = eq->n_div;
	for (i = 0; i < eq->n_eq; ++i) {
		j = isl_seq_last_non_zero(eq->eq[i], total + n_div);
		if (j < 0 || j == 0 || j >= total)
			continue;

		isl_seq_elim(aff->v->el + 1, eq->eq[i], j, total,
				&aff->v->el[0]);
	}

	isl_basic_set_free(eq);
	aff = isl_aff_normalize(aff);
	return aff;
error:
	isl_basic_set_free(eq);
	isl_aff_free(aff);
	return NULL;
}

/* Exploit the equalities in "eq" to simplify the affine expression
 * and the expressions of the integer divisions in the local space.
 */
static __isl_give isl_aff *isl_aff_substitute_equalities(
	__isl_take isl_aff *aff, __isl_take isl_basic_set *eq)
{
	int n_div;

	if (!aff || !eq)
		goto error;
	n_div = isl_local_space_dim(aff->ls, isl_dim_div);
	if (n_div > 0)
		eq = isl_basic_set_add(eq, isl_dim_set, n_div);
	return isl_aff_substitute_equalities_lifted(aff, eq);
error:
	isl_basic_set_free(eq);
	isl_aff_free(aff);
	return NULL;
}

/* Look for equalities among the variables shared by context and aff
 * and the integer divisions of aff, if any.
 * The equalities are then used to eliminate coefficients and/or integer
 * divisions from aff.
 */
__isl_give isl_aff *isl_aff_gist(__isl_take isl_aff *aff,
	__isl_take isl_set *context)
{
	isl_basic_set *hull;
	int n_div;

	if (!aff)
		goto error;
	n_div = isl_local_space_dim(aff->ls, isl_dim_div);
	if (n_div > 0) {
		isl_basic_set *bset;
		isl_local_space *ls;
		context = isl_set_add_dims(context, isl_dim_set, n_div);
		ls = isl_aff_get_domain_local_space(aff);
		bset = isl_basic_set_from_local_space(ls);
		bset = isl_basic_set_lift(bset);
		bset = isl_basic_set_flatten(bset);
		context = isl_set_intersect(context,
					    isl_set_from_basic_set(bset));
	}

	hull = isl_set_affine_hull(context);
	return isl_aff_substitute_equalities_lifted(aff, hull);
error:
	isl_aff_free(aff);
	isl_set_free(context);
	return NULL;
}

__isl_give isl_aff *isl_aff_gist_params(__isl_take isl_aff *aff,
	__isl_take isl_set *context)
{
	isl_set *dom_context = isl_set_universe(isl_aff_get_domain_space(aff));
	dom_context = isl_set_intersect_params(dom_context, context);
	return isl_aff_gist(aff, dom_context);
}

/* Return a basic set containing those elements in the space
 * of aff where it is non-negative.
 */
__isl_give isl_basic_set *isl_aff_nonneg_basic_set(__isl_take isl_aff *aff)
{
	isl_constraint *ineq;
	isl_basic_set *bset;

	ineq = isl_inequality_from_aff(aff);

	bset = isl_basic_set_from_constraint(ineq);
	bset = isl_basic_set_simplify(bset);
	return bset;
}

/* Return a basic set containing those elements in the space
 * of aff where it is zero.
 */
__isl_give isl_basic_set *isl_aff_zero_basic_set(__isl_take isl_aff *aff)
{
	isl_constraint *ineq;
	isl_basic_set *bset;

	ineq = isl_equality_from_aff(aff);

	bset = isl_basic_set_from_constraint(ineq);
	bset = isl_basic_set_simplify(bset);
	return bset;
}

/* Return a basic set containing those elements in the shared space
 * of aff1 and aff2 where aff1 is greater than or equal to aff2.
 */
__isl_give isl_basic_set *isl_aff_ge_basic_set(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	aff1 = isl_aff_sub(aff1, aff2);

	return isl_aff_nonneg_basic_set(aff1);
}

/* Return a basic set containing those elements in the shared space
 * of aff1 and aff2 where aff1 is smaller than or equal to aff2.
 */
__isl_give isl_basic_set *isl_aff_le_basic_set(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	return isl_aff_ge_basic_set(aff2, aff1);
}

__isl_give isl_aff *isl_aff_add_on_domain(__isl_keep isl_set *dom,
	__isl_take isl_aff *aff1, __isl_take isl_aff *aff2)
{
	aff1 = isl_aff_add(aff1, aff2);
	aff1 = isl_aff_gist(aff1, isl_set_copy(dom));
	return aff1;
}

int isl_aff_is_empty(__isl_keep isl_aff *aff)
{
	if (!aff)
		return -1;

	return 0;
}

/* Check whether the given affine expression has non-zero coefficient
 * for any dimension in the given range or if any of these dimensions
 * appear with non-zero coefficients in any of the integer divisions
 * involved in the affine expression.
 */
int isl_aff_involves_dims(__isl_keep isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;
	isl_ctx *ctx;
	int *active = NULL;
	int involves = 0;

	if (!aff)
		return -1;
	if (n == 0)
		return 0;

	ctx = isl_aff_get_ctx(aff);
	if (first + n > isl_aff_dim(aff, type))
		isl_die(ctx, isl_error_invalid,
			"range out of bounds", return -1);

	active = isl_local_space_get_active(aff->ls, aff->v->el + 2);
	if (!active)
		goto error;

	first += isl_local_space_offset(aff->ls, type) - 1;
	for (i = 0; i < n; ++i)
		if (active[first + i]) {
			involves = 1;
			break;
		}

	free(active);

	return involves;
error:
	free(active);
	return -1;
}

__isl_give isl_aff *isl_aff_drop_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	isl_ctx *ctx;

	if (!aff)
		return NULL;
	if (type == isl_dim_out)
		isl_die(aff->v->ctx, isl_error_invalid,
			"cannot drop output/set dimension",
			return isl_aff_free(aff));
	if (type == isl_dim_in)
		type = isl_dim_set;
	if (n == 0 && !isl_local_space_is_named_or_nested(aff->ls, type))
		return aff;

	ctx = isl_aff_get_ctx(aff);
	if (first + n > isl_local_space_dim(aff->ls, type))
		isl_die(ctx, isl_error_invalid, "range out of bounds",
			return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->ls = isl_local_space_drop_dims(aff->ls, type, first, n);
	if (!aff->ls)
		return isl_aff_free(aff);

	first += 1 + isl_local_space_offset(aff->ls, type);
	aff->v = isl_vec_drop_els(aff->v, first, n);
	if (!aff->v)
		return isl_aff_free(aff);

	return aff;
}

/* Project the domain of the affine expression onto its parameter space.
 * The affine expression may not involve any of the domain dimensions.
 */
__isl_give isl_aff *isl_aff_project_domain_on_params(__isl_take isl_aff *aff)
{
	isl_space *space;
	unsigned n;
	int involves;

	n = isl_aff_dim(aff, isl_dim_in);
	involves = isl_aff_involves_dims(aff, isl_dim_in, 0, n);
	if (involves < 0)
		return isl_aff_free(aff);
	if (involves)
		isl_die(isl_aff_get_ctx(aff), isl_error_invalid,
		    "affine expression involves some of the domain dimensions",
		    return isl_aff_free(aff));
	aff = isl_aff_drop_dims(aff, isl_dim_in, 0, n);
	space = isl_aff_get_domain_space(aff);
	space = isl_space_params(space);
	aff = isl_aff_reset_domain_space(aff, space);
	return aff;
}

__isl_give isl_aff *isl_aff_insert_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	isl_ctx *ctx;

	if (!aff)
		return NULL;
	if (type == isl_dim_out)
		isl_die(aff->v->ctx, isl_error_invalid,
			"cannot insert output/set dimensions",
			return isl_aff_free(aff));
	if (type == isl_dim_in)
		type = isl_dim_set;
	if (n == 0 && !isl_local_space_is_named_or_nested(aff->ls, type))
		return aff;

	ctx = isl_aff_get_ctx(aff);
	if (first > isl_local_space_dim(aff->ls, type))
		isl_die(ctx, isl_error_invalid, "position out of bounds",
			return isl_aff_free(aff));

	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->ls = isl_local_space_insert_dims(aff->ls, type, first, n);
	if (!aff->ls)
		return isl_aff_free(aff);

	first += 1 + isl_local_space_offset(aff->ls, type);
	aff->v = isl_vec_insert_zero_els(aff->v, first, n);
	if (!aff->v)
		return isl_aff_free(aff);

	return aff;
}

__isl_give isl_aff *isl_aff_add_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned n)
{
	unsigned pos;

	pos = isl_aff_dim(aff, type);

	return isl_aff_insert_dims(aff, type, pos, n);
}

__isl_give isl_pw_aff *isl_pw_aff_add_dims(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned n)
{
	unsigned pos;

	pos = isl_pw_aff_dim(pwaff, type);

	return isl_pw_aff_insert_dims(pwaff, type, pos, n);
}

__isl_give isl_pw_aff *isl_pw_aff_from_aff(__isl_take isl_aff *aff)
{
	isl_set *dom = isl_set_universe(isl_aff_get_domain_space(aff));
	return isl_pw_aff_alloc(dom, aff);
}

#undef PW
#define PW isl_pw_aff
#undef EL
#define EL isl_aff
#undef EL_IS_ZERO
#define EL_IS_ZERO is_empty
#undef ZERO
#define ZERO empty
#undef IS_ZERO
#define IS_ZERO is_empty
#undef FIELD
#define FIELD aff
#undef DEFAULT_IS_ZERO
#define DEFAULT_IS_ZERO 0

#define NO_EVAL
#define NO_OPT
#define NO_MOVE_DIMS
#define NO_LIFT
#define NO_MORPH

#include <isl_pw_templ.c>

static __isl_give isl_set *align_params_pw_pw_set_and(
	__isl_take isl_pw_aff *pwaff1, __isl_take isl_pw_aff *pwaff2,
	__isl_give isl_set *(*fn)(__isl_take isl_pw_aff *pwaff1,
				    __isl_take isl_pw_aff *pwaff2))
{
	if (!pwaff1 || !pwaff2)
		goto error;
	if (isl_space_match(pwaff1->dim, isl_dim_param,
			  pwaff2->dim, isl_dim_param))
		return fn(pwaff1, pwaff2);
	if (!isl_space_has_named_params(pwaff1->dim) ||
	    !isl_space_has_named_params(pwaff2->dim))
		isl_die(isl_pw_aff_get_ctx(pwaff1), isl_error_invalid,
			"unaligned unnamed parameters", goto error);
	pwaff1 = isl_pw_aff_align_params(pwaff1, isl_pw_aff_get_space(pwaff2));
	pwaff2 = isl_pw_aff_align_params(pwaff2, isl_pw_aff_get_space(pwaff1));
	return fn(pwaff1, pwaff2);
error:
	isl_pw_aff_free(pwaff1);
	isl_pw_aff_free(pwaff2);
	return NULL;
}

/* Compute a piecewise quasi-affine expression with a domain that
 * is the union of those of pwaff1 and pwaff2 and such that on each
 * cell, the quasi-affine expression is the better (according to cmp)
 * of those of pwaff1 and pwaff2.  If only one of pwaff1 or pwaff2
 * is defined on a given cell, then the associated expression
 * is the defined one.
 */
static __isl_give isl_pw_aff *pw_aff_union_opt(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2,
	__isl_give isl_basic_set *(*cmp)(__isl_take isl_aff *aff1,
					__isl_take isl_aff *aff2))
{
	int i, j, n;
	isl_pw_aff *res;
	isl_ctx *ctx;
	isl_set *set;

	if (!pwaff1 || !pwaff2)
		goto error;

	ctx = isl_space_get_ctx(pwaff1->dim);
	if (!isl_space_is_equal(pwaff1->dim, pwaff2->dim))
		isl_die(ctx, isl_error_invalid,
			"arguments should live in same space", goto error);

	if (isl_pw_aff_is_empty(pwaff1)) {
		isl_pw_aff_free(pwaff1);
		return pwaff2;
	}

	if (isl_pw_aff_is_empty(pwaff2)) {
		isl_pw_aff_free(pwaff2);
		return pwaff1;
	}

	n = 2 * (pwaff1->n + 1) * (pwaff2->n + 1);
	res = isl_pw_aff_alloc_size(isl_space_copy(pwaff1->dim), n);

	for (i = 0; i < pwaff1->n; ++i) {
		set = isl_set_copy(pwaff1->p[i].set);
		for (j = 0; j < pwaff2->n; ++j) {
			struct isl_set *common;
			isl_set *better;

			common = isl_set_intersect(
					isl_set_copy(pwaff1->p[i].set),
					isl_set_copy(pwaff2->p[j].set));
			better = isl_set_from_basic_set(cmp(
					isl_aff_copy(pwaff2->p[j].aff),
					isl_aff_copy(pwaff1->p[i].aff)));
			better = isl_set_intersect(common, better);
			if (isl_set_plain_is_empty(better)) {
				isl_set_free(better);
				continue;
			}
			set = isl_set_subtract(set, isl_set_copy(better));

			res = isl_pw_aff_add_piece(res, better,
						isl_aff_copy(pwaff2->p[j].aff));
		}
		res = isl_pw_aff_add_piece(res, set,
						isl_aff_copy(pwaff1->p[i].aff));
	}

	for (j = 0; j < pwaff2->n; ++j) {
		set = isl_set_copy(pwaff2->p[j].set);
		for (i = 0; i < pwaff1->n; ++i)
			set = isl_set_subtract(set,
					isl_set_copy(pwaff1->p[i].set));
		res = isl_pw_aff_add_piece(res, set,
						isl_aff_copy(pwaff2->p[j].aff));
	}

	isl_pw_aff_free(pwaff1);
	isl_pw_aff_free(pwaff2);

	return res;
error:
	isl_pw_aff_free(pwaff1);
	isl_pw_aff_free(pwaff2);
	return NULL;
}

/* Compute a piecewise quasi-affine expression with a domain that
 * is the union of those of pwaff1 and pwaff2 and such that on each
 * cell, the quasi-affine expression is the maximum of those of pwaff1
 * and pwaff2.  If only one of pwaff1 or pwaff2 is defined on a given
 * cell, then the associated expression is the defined one.
 */
static __isl_give isl_pw_aff *pw_aff_union_max(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return pw_aff_union_opt(pwaff1, pwaff2, &isl_aff_ge_basic_set);
}

__isl_give isl_pw_aff *isl_pw_aff_union_max(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_align_params_pw_pw_and(pwaff1, pwaff2,
							&pw_aff_union_max);
}

/* Compute a piecewise quasi-affine expression with a domain that
 * is the union of those of pwaff1 and pwaff2 and such that on each
 * cell, the quasi-affine expression is the minimum of those of pwaff1
 * and pwaff2.  If only one of pwaff1 or pwaff2 is defined on a given
 * cell, then the associated expression is the defined one.
 */
static __isl_give isl_pw_aff *pw_aff_union_min(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return pw_aff_union_opt(pwaff1, pwaff2, &isl_aff_le_basic_set);
}

__isl_give isl_pw_aff *isl_pw_aff_union_min(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_align_params_pw_pw_and(pwaff1, pwaff2,
							&pw_aff_union_min);
}

__isl_give isl_pw_aff *isl_pw_aff_union_opt(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2, int max)
{
	if (max)
		return isl_pw_aff_union_max(pwaff1, pwaff2);
	else
		return isl_pw_aff_union_min(pwaff1, pwaff2);
}

/* Construct a map with as domain the domain of pwaff and
 * one-dimensional range corresponding to the affine expressions.
 */
static __isl_give isl_map *map_from_pw_aff(__isl_take isl_pw_aff *pwaff)
{
	int i;
	isl_space *dim;
	isl_map *map;

	if (!pwaff)
		return NULL;

	dim = isl_pw_aff_get_space(pwaff);
	map = isl_map_empty(dim);

	for (i = 0; i < pwaff->n; ++i) {
		isl_basic_map *bmap;
		isl_map *map_i;

		bmap = isl_basic_map_from_aff(isl_aff_copy(pwaff->p[i].aff));
		map_i = isl_map_from_basic_map(bmap);
		map_i = isl_map_intersect_domain(map_i,
						isl_set_copy(pwaff->p[i].set));
		map = isl_map_union_disjoint(map, map_i);
	}

	isl_pw_aff_free(pwaff);

	return map;
}

/* Construct a map with as domain the domain of pwaff and
 * one-dimensional range corresponding to the affine expressions.
 */
__isl_give isl_map *isl_map_from_pw_aff(__isl_take isl_pw_aff *pwaff)
{
	if (!pwaff)
		return NULL;
	if (isl_space_is_set(pwaff->dim))
		isl_die(isl_pw_aff_get_ctx(pwaff), isl_error_invalid,
			"space of input is not a map",
			return isl_pw_aff_free(pwaff));
	return map_from_pw_aff(pwaff);
}

/* Construct a one-dimensional set with as parameter domain
 * the domain of pwaff and the single set dimension
 * corresponding to the affine expressions.
 */
__isl_give isl_set *isl_set_from_pw_aff(__isl_take isl_pw_aff *pwaff)
{
	if (!pwaff)
		return NULL;
	if (!isl_space_is_set(pwaff->dim))
		isl_die(isl_pw_aff_get_ctx(pwaff), isl_error_invalid,
			"space of input is not a set",
			return isl_pw_aff_free(pwaff));
	return map_from_pw_aff(pwaff);
}

/* Return a set containing those elements in the domain
 * of pwaff where it is non-negative.
 */
__isl_give isl_set *isl_pw_aff_nonneg_set(__isl_take isl_pw_aff *pwaff)
{
	int i;
	isl_set *set;

	if (!pwaff)
		return NULL;

	set = isl_set_empty(isl_pw_aff_get_domain_space(pwaff));

	for (i = 0; i < pwaff->n; ++i) {
		isl_basic_set *bset;
		isl_set *set_i;

		bset = isl_aff_nonneg_basic_set(isl_aff_copy(pwaff->p[i].aff));
		set_i = isl_set_from_basic_set(bset);
		set_i = isl_set_intersect(set_i, isl_set_copy(pwaff->p[i].set));
		set = isl_set_union_disjoint(set, set_i);
	}

	isl_pw_aff_free(pwaff);

	return set;
}

/* Return a set containing those elements in the domain
 * of pwaff where it is zero (if complement is 0) or not zero
 * (if complement is 1).
 */
static __isl_give isl_set *pw_aff_zero_set(__isl_take isl_pw_aff *pwaff,
	int complement)
{
	int i;
	isl_set *set;

	if (!pwaff)
		return NULL;

	set = isl_set_empty(isl_pw_aff_get_domain_space(pwaff));

	for (i = 0; i < pwaff->n; ++i) {
		isl_basic_set *bset;
		isl_set *set_i, *zero;

		bset = isl_aff_zero_basic_set(isl_aff_copy(pwaff->p[i].aff));
		zero = isl_set_from_basic_set(bset);
		set_i = isl_set_copy(pwaff->p[i].set);
		if (complement)
			set_i = isl_set_subtract(set_i, zero);
		else
			set_i = isl_set_intersect(set_i, zero);
		set = isl_set_union_disjoint(set, set_i);
	}

	isl_pw_aff_free(pwaff);

	return set;
}

/* Return a set containing those elements in the domain
 * of pwaff where it is zero.
 */
__isl_give isl_set *isl_pw_aff_zero_set(__isl_take isl_pw_aff *pwaff)
{
	return pw_aff_zero_set(pwaff, 0);
}

/* Return a set containing those elements in the domain
 * of pwaff where it is not zero.
 */
__isl_give isl_set *isl_pw_aff_non_zero_set(__isl_take isl_pw_aff *pwaff)
{
	return pw_aff_zero_set(pwaff, 1);
}

/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is greater than (or equal) to pwaff2.
 *
 * We compute the difference on the shared domain and then construct
 * the set of values where this difference is non-negative.
 * If strict is set, we first subtract 1 from the difference.
 * If equal is set, we only return the elements where pwaff1 and pwaff2
 * are equal.
 */
static __isl_give isl_set *pw_aff_gte_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2, int strict, int equal)
{
	isl_set *set1, *set2;

	set1 = isl_pw_aff_domain(isl_pw_aff_copy(pwaff1));
	set2 = isl_pw_aff_domain(isl_pw_aff_copy(pwaff2));
	set1 = isl_set_intersect(set1, set2);
	pwaff1 = isl_pw_aff_intersect_domain(pwaff1, isl_set_copy(set1));
	pwaff2 = isl_pw_aff_intersect_domain(pwaff2, isl_set_copy(set1));
	pwaff1 = isl_pw_aff_add(pwaff1, isl_pw_aff_neg(pwaff2));

	if (strict) {
		isl_space *dim = isl_set_get_space(set1);
		isl_aff *aff;
		aff = isl_aff_zero_on_domain(isl_local_space_from_space(dim));
		aff = isl_aff_add_constant_si(aff, -1);
		pwaff1 = isl_pw_aff_add(pwaff1, isl_pw_aff_alloc(set1, aff));
	} else
		isl_set_free(set1);

	if (equal)
		return isl_pw_aff_zero_set(pwaff1);
	return isl_pw_aff_nonneg_set(pwaff1);
}

/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is equal to pwaff2.
 */
static __isl_give isl_set *pw_aff_eq_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return pw_aff_gte_set(pwaff1, pwaff2, 0, 1);
}

__isl_give isl_set *isl_pw_aff_eq_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return align_params_pw_pw_set_and(pwaff1, pwaff2, &pw_aff_eq_set);
}

/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is greater than or equal to pwaff2.
 */
static __isl_give isl_set *pw_aff_ge_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return pw_aff_gte_set(pwaff1, pwaff2, 0, 0);
}

__isl_give isl_set *isl_pw_aff_ge_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return align_params_pw_pw_set_and(pwaff1, pwaff2, &pw_aff_ge_set);
}

/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is strictly greater than pwaff2.
 */
static __isl_give isl_set *pw_aff_gt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return pw_aff_gte_set(pwaff1, pwaff2, 1, 0);
}

__isl_give isl_set *isl_pw_aff_gt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return align_params_pw_pw_set_and(pwaff1, pwaff2, &pw_aff_gt_set);
}

__isl_give isl_set *isl_pw_aff_le_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_ge_set(pwaff2, pwaff1);
}

__isl_give isl_set *isl_pw_aff_lt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_gt_set(pwaff2, pwaff1);
}

/* Return a set containing those elements in the shared domain
 * of the elements of list1 and list2 where each element in list1
 * has the relation specified by "fn" with each element in list2.
 */
static __isl_give isl_set *pw_aff_list_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2,
	__isl_give isl_set *(*fn)(__isl_take isl_pw_aff *pwaff1,
				    __isl_take isl_pw_aff *pwaff2))
{
	int i, j;
	isl_ctx *ctx;
	isl_set *set;

	if (!list1 || !list2)
		goto error;

	ctx = isl_pw_aff_list_get_ctx(list1);
	if (list1->n < 1 || list2->n < 1)
		isl_die(ctx, isl_error_invalid,
			"list should contain at least one element", goto error);

	set = isl_set_universe(isl_pw_aff_get_domain_space(list1->p[0]));
	for (i = 0; i < list1->n; ++i)
		for (j = 0; j < list2->n; ++j) {
			isl_set *set_ij;

			set_ij = fn(isl_pw_aff_copy(list1->p[i]),
				    isl_pw_aff_copy(list2->p[j]));
			set = isl_set_intersect(set, set_ij);
		}

	isl_pw_aff_list_free(list1);
	isl_pw_aff_list_free(list2);
	return set;
error:
	isl_pw_aff_list_free(list1);
	isl_pw_aff_list_free(list2);
	return NULL;
}

/* Return a set containing those elements in the shared domain
 * of the elements of list1 and list2 where each element in list1
 * is equal to each element in list2.
 */
__isl_give isl_set *isl_pw_aff_list_eq_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2)
{
	return pw_aff_list_set(list1, list2, &isl_pw_aff_eq_set);
}

__isl_give isl_set *isl_pw_aff_list_ne_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2)
{
	return pw_aff_list_set(list1, list2, &isl_pw_aff_ne_set);
}

/* Return a set containing those elements in the shared domain
 * of the elements of list1 and list2 where each element in list1
 * is less than or equal to each element in list2.
 */
__isl_give isl_set *isl_pw_aff_list_le_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2)
{
	return pw_aff_list_set(list1, list2, &isl_pw_aff_le_set);
}

__isl_give isl_set *isl_pw_aff_list_lt_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2)
{
	return pw_aff_list_set(list1, list2, &isl_pw_aff_lt_set);
}

__isl_give isl_set *isl_pw_aff_list_ge_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2)
{
	return pw_aff_list_set(list1, list2, &isl_pw_aff_ge_set);
}

__isl_give isl_set *isl_pw_aff_list_gt_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2)
{
	return pw_aff_list_set(list1, list2, &isl_pw_aff_gt_set);
}


/* Return a set containing those elements in the shared domain
 * of pwaff1 and pwaff2 where pwaff1 is not equal to pwaff2.
 */
static __isl_give isl_set *pw_aff_ne_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	isl_set *set_lt, *set_gt;

	set_lt = isl_pw_aff_lt_set(isl_pw_aff_copy(pwaff1),
				   isl_pw_aff_copy(pwaff2));
	set_gt = isl_pw_aff_gt_set(pwaff1, pwaff2);
	return isl_set_union_disjoint(set_lt, set_gt);
}

__isl_give isl_set *isl_pw_aff_ne_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return align_params_pw_pw_set_and(pwaff1, pwaff2, &pw_aff_ne_set);
}

__isl_give isl_pw_aff *isl_pw_aff_scale_down(__isl_take isl_pw_aff *pwaff,
	isl_int v)
{
	int i;

	if (isl_int_is_one(v))
		return pwaff;
	if (!isl_int_is_pos(v))
		isl_die(isl_pw_aff_get_ctx(pwaff), isl_error_invalid,
			"factor needs to be positive",
			return isl_pw_aff_free(pwaff));
	pwaff = isl_pw_aff_cow(pwaff);
	if (!pwaff)
		return NULL;
	if (pwaff->n == 0)
		return pwaff;

	for (i = 0; i < pwaff->n; ++i) {
		pwaff->p[i].aff = isl_aff_scale_down(pwaff->p[i].aff, v);
		if (!pwaff->p[i].aff)
			return isl_pw_aff_free(pwaff);
	}

	return pwaff;
}

__isl_give isl_pw_aff *isl_pw_aff_floor(__isl_take isl_pw_aff *pwaff)
{
	int i;

	pwaff = isl_pw_aff_cow(pwaff);
	if (!pwaff)
		return NULL;
	if (pwaff->n == 0)
		return pwaff;

	for (i = 0; i < pwaff->n; ++i) {
		pwaff->p[i].aff = isl_aff_floor(pwaff->p[i].aff);
		if (!pwaff->p[i].aff)
			return isl_pw_aff_free(pwaff);
	}

	return pwaff;
}

__isl_give isl_pw_aff *isl_pw_aff_ceil(__isl_take isl_pw_aff *pwaff)
{
	int i;

	pwaff = isl_pw_aff_cow(pwaff);
	if (!pwaff)
		return NULL;
	if (pwaff->n == 0)
		return pwaff;

	for (i = 0; i < pwaff->n; ++i) {
		pwaff->p[i].aff = isl_aff_ceil(pwaff->p[i].aff);
		if (!pwaff->p[i].aff)
			return isl_pw_aff_free(pwaff);
	}

	return pwaff;
}

/* Assuming that "cond1" and "cond2" are disjoint,
 * return an affine expression that is equal to pwaff1 on cond1
 * and to pwaff2 on cond2.
 */
static __isl_give isl_pw_aff *isl_pw_aff_select(
	__isl_take isl_set *cond1, __isl_take isl_pw_aff *pwaff1,
	__isl_take isl_set *cond2, __isl_take isl_pw_aff *pwaff2)
{
	pwaff1 = isl_pw_aff_intersect_domain(pwaff1, cond1);
	pwaff2 = isl_pw_aff_intersect_domain(pwaff2, cond2);

	return isl_pw_aff_add_disjoint(pwaff1, pwaff2);
}

/* Return an affine expression that is equal to pwaff_true for elements
 * where "cond" is non-zero and to pwaff_false for elements where "cond"
 * is zero.
 * That is, return cond ? pwaff_true : pwaff_false;
 */
__isl_give isl_pw_aff *isl_pw_aff_cond(__isl_take isl_pw_aff *cond,
	__isl_take isl_pw_aff *pwaff_true, __isl_take isl_pw_aff *pwaff_false)
{
	isl_set *cond_true, *cond_false;

	cond_true = isl_pw_aff_non_zero_set(isl_pw_aff_copy(cond));
	cond_false = isl_pw_aff_zero_set(cond);
	return isl_pw_aff_select(cond_true, pwaff_true,
				 cond_false, pwaff_false);
}

int isl_aff_is_cst(__isl_keep isl_aff *aff)
{
	if (!aff)
		return -1;

	return isl_seq_first_non_zero(aff->v->el + 2, aff->v->size - 2) == -1;
}

/* Check whether pwaff is a piecewise constant.
 */
int isl_pw_aff_is_cst(__isl_keep isl_pw_aff *pwaff)
{
	int i;

	if (!pwaff)
		return -1;

	for (i = 0; i < pwaff->n; ++i) {
		int is_cst = isl_aff_is_cst(pwaff->p[i].aff);
		if (is_cst < 0 || !is_cst)
			return is_cst;
	}

	return 1;
}

__isl_give isl_aff *isl_aff_mul(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2)
{
	if (!isl_aff_is_cst(aff2) && isl_aff_is_cst(aff1))
		return isl_aff_mul(aff2, aff1);

	if (!isl_aff_is_cst(aff2))
		isl_die(isl_aff_get_ctx(aff1), isl_error_invalid,
			"at least one affine expression should be constant",
			goto error);

	aff1 = isl_aff_cow(aff1);
	if (!aff1 || !aff2)
		goto error;

	aff1 = isl_aff_scale(aff1, aff2->v->el[1]);
	aff1 = isl_aff_scale_down(aff1, aff2->v->el[0]);

	isl_aff_free(aff2);
	return aff1;
error:
	isl_aff_free(aff1);
	isl_aff_free(aff2);
	return NULL;
}

static __isl_give isl_pw_aff *pw_aff_add(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_on_shared_domain(pwaff1, pwaff2, &isl_aff_add);
}

__isl_give isl_pw_aff *isl_pw_aff_add(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_align_params_pw_pw_and(pwaff1, pwaff2, &pw_aff_add);
}

__isl_give isl_pw_aff *isl_pw_aff_union_add(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_union_add_(pwaff1, pwaff2);
}

static __isl_give isl_pw_aff *pw_aff_mul(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_on_shared_domain(pwaff1, pwaff2, &isl_aff_mul);
}

__isl_give isl_pw_aff *isl_pw_aff_mul(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_align_params_pw_pw_and(pwaff1, pwaff2, &pw_aff_mul);
}

static __isl_give isl_pw_aff *pw_aff_min(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	isl_set *le;
	isl_set *dom;

	dom = isl_set_intersect(isl_pw_aff_domain(isl_pw_aff_copy(pwaff1)),
				isl_pw_aff_domain(isl_pw_aff_copy(pwaff2)));
	le = isl_pw_aff_le_set(isl_pw_aff_copy(pwaff1),
				isl_pw_aff_copy(pwaff2));
	dom = isl_set_subtract(dom, isl_set_copy(le));
	return isl_pw_aff_select(le, pwaff1, dom, pwaff2);
}

__isl_give isl_pw_aff *isl_pw_aff_min(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_align_params_pw_pw_and(pwaff1, pwaff2, &pw_aff_min);
}

static __isl_give isl_pw_aff *pw_aff_max(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	isl_set *ge;
	isl_set *dom;

	dom = isl_set_intersect(isl_pw_aff_domain(isl_pw_aff_copy(pwaff1)),
				isl_pw_aff_domain(isl_pw_aff_copy(pwaff2)));
	ge = isl_pw_aff_ge_set(isl_pw_aff_copy(pwaff1),
				isl_pw_aff_copy(pwaff2));
	dom = isl_set_subtract(dom, isl_set_copy(ge));
	return isl_pw_aff_select(ge, pwaff1, dom, pwaff2);
}

__isl_give isl_pw_aff *isl_pw_aff_max(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2)
{
	return isl_pw_aff_align_params_pw_pw_and(pwaff1, pwaff2, &pw_aff_max);
}

static __isl_give isl_pw_aff *pw_aff_list_reduce(
	__isl_take isl_pw_aff_list *list,
	__isl_give isl_pw_aff *(*fn)(__isl_take isl_pw_aff *pwaff1,
					__isl_take isl_pw_aff *pwaff2))
{
	int i;
	isl_ctx *ctx;
	isl_pw_aff *res;

	if (!list)
		return NULL;

	ctx = isl_pw_aff_list_get_ctx(list);
	if (list->n < 1)
		isl_die(ctx, isl_error_invalid,
			"list should contain at least one element",
			return isl_pw_aff_list_free(list));

	res = isl_pw_aff_copy(list->p[0]);
	for (i = 1; i < list->n; ++i)
		res = fn(res, isl_pw_aff_copy(list->p[i]));

	isl_pw_aff_list_free(list);
	return res;
}

/* Return an isl_pw_aff that maps each element in the intersection of the
 * domains of the elements of list to the minimal corresponding affine
 * expression.
 */
__isl_give isl_pw_aff *isl_pw_aff_list_min(__isl_take isl_pw_aff_list *list)
{
	return pw_aff_list_reduce(list, &isl_pw_aff_min);
}

/* Return an isl_pw_aff that maps each element in the intersection of the
 * domains of the elements of list to the maximal corresponding affine
 * expression.
 */
__isl_give isl_pw_aff *isl_pw_aff_list_max(__isl_take isl_pw_aff_list *list)
{
	return pw_aff_list_reduce(list, &isl_pw_aff_max);
}

#undef BASE
#define BASE aff

#include <isl_multi_templ.c>

/* Construct an isl_multi_aff in the given space with value zero in
 * each of the output dimensions.
 */
__isl_give isl_multi_aff *isl_multi_aff_zero(__isl_take isl_space *space)
{
	int n;
	isl_multi_aff *ma;

	if (!space)
		return NULL;

	n = isl_space_dim(space , isl_dim_out);
	ma = isl_multi_aff_alloc(isl_space_copy(space));

	if (!n)
		isl_space_free(space);
	else {
		int i;
		isl_local_space *ls;
		isl_aff *aff;

		space = isl_space_domain(space);
		ls = isl_local_space_from_space(space);
		aff = isl_aff_zero_on_domain(ls);

		for (i = 0; i < n; ++i)
			ma = isl_multi_aff_set_aff(ma, i, isl_aff_copy(aff));

		isl_aff_free(aff);
	}

	return ma;
}

/* Create an isl_pw_multi_aff with the given isl_multi_aff on a universe
 * domain.
 */
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_from_multi_aff(
	__isl_take isl_multi_aff *ma)
{
	isl_set *dom = isl_set_universe(isl_multi_aff_get_domain_space(ma));
	return isl_pw_multi_aff_alloc(dom, ma);
}

__isl_give isl_multi_aff *isl_multi_aff_add(__isl_take isl_multi_aff *maff1,
	__isl_take isl_multi_aff *maff2)
{
	int i;
	isl_ctx *ctx;

	maff1 = isl_multi_aff_cow(maff1);
	if (!maff1 || !maff2)
		goto error;

	ctx = isl_multi_aff_get_ctx(maff1);
	if (!isl_space_is_equal(maff1->space, maff2->space))
		isl_die(ctx, isl_error_invalid,
			"spaces don't match", goto error);

	for (i = 0; i < maff1->n; ++i) {
		maff1->p[i] = isl_aff_add(maff1->p[i],
					    isl_aff_copy(maff2->p[i]));
		if (!maff1->p[i])
			goto error;
	}

	isl_multi_aff_free(maff2);
	return maff1;
error:
	isl_multi_aff_free(maff1);
	isl_multi_aff_free(maff2);
	return NULL;
}

/* Exploit the equalities in "eq" to simplify the affine expressions.
 */
static __isl_give isl_multi_aff *isl_multi_aff_substitute_equalities(
	__isl_take isl_multi_aff *maff, __isl_take isl_basic_set *eq)
{
	int i;

	maff = isl_multi_aff_cow(maff);
	if (!maff || !eq)
		goto error;

	for (i = 0; i < maff->n; ++i) {
		maff->p[i] = isl_aff_substitute_equalities(maff->p[i],
						    isl_basic_set_copy(eq));
		if (!maff->p[i])
			goto error;
	}

	isl_basic_set_free(eq);
	return maff;
error:
	isl_basic_set_free(eq);
	isl_multi_aff_free(maff);
	return NULL;
}

__isl_give isl_multi_aff *isl_multi_aff_scale(__isl_take isl_multi_aff *maff,
	isl_int f)
{
	int i;

	maff = isl_multi_aff_cow(maff);
	if (!maff)
		return NULL;

	for (i = 0; i < maff->n; ++i) {
		maff->p[i] = isl_aff_scale(maff->p[i], f);
		if (!maff->p[i])
			return isl_multi_aff_free(maff);
	}

	return maff;
}

__isl_give isl_multi_aff *isl_multi_aff_add_on_domain(__isl_keep isl_set *dom,
	__isl_take isl_multi_aff *maff1, __isl_take isl_multi_aff *maff2)
{
	maff1 = isl_multi_aff_add(maff1, maff2);
	maff1 = isl_multi_aff_gist(maff1, isl_set_copy(dom));
	return maff1;
}

int isl_multi_aff_is_empty(__isl_keep isl_multi_aff *maff)
{
	if (!maff)
		return -1;

	return 0;
}

int isl_multi_aff_plain_is_equal(__isl_keep isl_multi_aff *maff1,
	__isl_keep isl_multi_aff *maff2)
{
	int i;
	int equal;

	if (!maff1 || !maff2)
		return -1;
	if (maff1->n != maff2->n)
		return 0;
	equal = isl_space_is_equal(maff1->space, maff2->space);
	if (equal < 0 || !equal)
		return equal;

	for (i = 0; i < maff1->n; ++i) {
		equal = isl_aff_plain_is_equal(maff1->p[i], maff2->p[i]);
		if (equal < 0 || !equal)
			return equal;
	}

	return 1;
}

__isl_give isl_multi_aff *isl_multi_aff_set_dim_name(
	__isl_take isl_multi_aff *maff,
	enum isl_dim_type type, unsigned pos, const char *s)
{
	int i;

	maff = isl_multi_aff_cow(maff);
	if (!maff)
		return NULL;

	maff->space = isl_space_set_dim_name(maff->space, type, pos, s);
	if (!maff->space)
		return isl_multi_aff_free(maff);
	for (i = 0; i < maff->n; ++i) {
		maff->p[i] = isl_aff_set_dim_name(maff->p[i], type, pos, s);
		if (!maff->p[i])
			return isl_multi_aff_free(maff);
	}

	return maff;
}

__isl_give isl_multi_aff *isl_multi_aff_drop_dims(__isl_take isl_multi_aff *maff,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;

	maff = isl_multi_aff_cow(maff);
	if (!maff)
		return NULL;

	maff->space = isl_space_drop_dims(maff->space, type, first, n);
	if (!maff->space)
		return isl_multi_aff_free(maff);

	if (type == isl_dim_out) {
		for (i = 0; i < n; ++i)
			isl_aff_free(maff->p[first + i]);
		for (i = first; i + n < maff->n; ++i)
			maff->p[i] = maff->p[i + n];
		maff->n -= n;
		return maff;
	}

	for (i = 0; i < maff->n; ++i) {
		maff->p[i] = isl_aff_drop_dims(maff->p[i], type, first, n);
		if (!maff->p[i])
			return isl_multi_aff_free(maff);
	}

	return maff;
}

#undef PW
#define PW isl_pw_multi_aff
#undef EL
#define EL isl_multi_aff
#undef EL_IS_ZERO
#define EL_IS_ZERO is_empty
#undef ZERO
#define ZERO empty
#undef IS_ZERO
#define IS_ZERO is_empty
#undef FIELD
#define FIELD maff
#undef DEFAULT_IS_ZERO
#define DEFAULT_IS_ZERO 0

#define NO_NEG
#define NO_EVAL
#define NO_OPT
#define NO_INVOLVES_DIMS
#define NO_MOVE_DIMS
#define NO_INSERT_DIMS
#define NO_LIFT
#define NO_MORPH

#include <isl_pw_templ.c>

#undef UNION
#define UNION isl_union_pw_multi_aff
#undef PART
#define PART isl_pw_multi_aff
#undef PARTS
#define PARTS pw_multi_aff
#define ALIGN_DOMAIN

#define NO_EVAL

#include <isl_union_templ.c>

static __isl_give isl_pw_multi_aff *pw_multi_aff_add(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2)
{
	return isl_pw_multi_aff_on_shared_domain(pma1, pma2,
						&isl_multi_aff_add);
}

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_add(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2)
{
	return isl_pw_multi_aff_align_params_pw_pw_and(pma1, pma2,
						&pw_multi_aff_add);
}

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_union_add(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2)
{
	return isl_pw_multi_aff_union_add_(pma1, pma2);
}

/* Construct a map mapping the domain of the piecewise multi-affine expression
 * to its range, with each dimension in the range equated to the
 * corresponding affine expression on its cell.
 */
__isl_give isl_map *isl_map_from_pw_multi_aff(__isl_take isl_pw_multi_aff *pma)
{
	int i;
	isl_map *map;

	if (!pma)
		return NULL;

	map = isl_map_empty(isl_pw_multi_aff_get_space(pma));

	for (i = 0; i < pma->n; ++i) {
		isl_multi_aff *maff;
		isl_basic_map *bmap;
		isl_map *map_i;

		maff = isl_multi_aff_copy(pma->p[i].maff);
		bmap = isl_basic_map_from_multi_aff(maff);
		map_i = isl_map_from_basic_map(bmap);
		map_i = isl_map_intersect_domain(map_i,
						isl_set_copy(pma->p[i].set));
		map = isl_map_union_disjoint(map, map_i);
	}

	isl_pw_multi_aff_free(pma);
	return map;
}

__isl_give isl_set *isl_set_from_pw_multi_aff(__isl_take isl_pw_multi_aff *pma)
{
	if (!isl_space_is_set(pma->dim))
		isl_die(isl_pw_multi_aff_get_ctx(pma), isl_error_invalid,
			"isl_pw_multi_aff cannot be converted into an isl_set",
			return isl_pw_multi_aff_free(pma));

	return isl_map_from_pw_multi_aff(pma);
}

/* Given a basic map with a single output dimension that is defined
 * in terms of the parameters and input dimensions using an equality,
 * extract an isl_aff that expresses the output dimension in terms
 * of the parameters and input dimensions.
 *
 * Since some applications expect the result of isl_pw_multi_aff_from_map
 * to only contain integer affine expressions, we compute the floor
 * of the expression before returning.
 *
 * This function shares some similarities with
 * isl_basic_map_has_defining_equality and isl_constraint_get_bound.
 */
static __isl_give isl_aff *extract_isl_aff_from_basic_map(
	__isl_take isl_basic_map *bmap)
{
	int i;
	unsigned offset;
	unsigned total;
	isl_local_space *ls;
	isl_aff *aff;

	if (!bmap)
		return NULL;
	if (isl_basic_map_dim(bmap, isl_dim_out) != 1)
		isl_die(isl_basic_map_get_ctx(bmap), isl_error_invalid,
			"basic map should have a single output dimension",
			goto error);
	offset = isl_basic_map_offset(bmap, isl_dim_out);
	total = isl_basic_map_total_dim(bmap);
	for (i = 0; i < bmap->n_eq; ++i) {
		if (isl_int_is_zero(bmap->eq[i][offset]))
			continue;
		if (isl_seq_first_non_zero(bmap->eq[i] + offset + 1,
					   1 + total - (offset + 1)) != -1)
			continue;
		break;
	}
	if (i >= bmap->n_eq)
		isl_die(isl_basic_map_get_ctx(bmap), isl_error_invalid,
			"unable to find suitable equality", goto error);
	ls = isl_basic_map_get_local_space(bmap);
	aff = isl_aff_alloc(isl_local_space_domain(ls));
	if (!aff)
		goto error;
	if (isl_int_is_neg(bmap->eq[i][offset]))
		isl_seq_cpy(aff->v->el + 1, bmap->eq[i], offset);
	else
		isl_seq_neg(aff->v->el + 1, bmap->eq[i], offset);
	isl_seq_clr(aff->v->el + 1 + offset, aff->v->size - (1 + offset));
	isl_int_abs(aff->v->el[0], bmap->eq[i][offset]);
	isl_basic_map_free(bmap);

	aff = isl_aff_remove_unused_divs(aff);
	aff = isl_aff_floor(aff);
	return aff;
error:
	isl_basic_map_free(bmap);
	return NULL;
}

/* Given a basic map where each output dimension is defined
 * in terms of the parameters and input dimensions using an equality,
 * extract an isl_multi_aff that expresses the output dimensions in terms
 * of the parameters and input dimensions.
 */
static __isl_give isl_multi_aff *extract_isl_multi_aff_from_basic_map(
	__isl_take isl_basic_map *bmap)
{
	int i;
	unsigned n_out;
	isl_multi_aff *ma;

	if (!bmap)
		return NULL;

	ma = isl_multi_aff_alloc(isl_basic_map_get_space(bmap));
	n_out = isl_basic_map_dim(bmap, isl_dim_out);

	for (i = 0; i < n_out; ++i) {
		isl_basic_map *bmap_i;
		isl_aff *aff;

		bmap_i = isl_basic_map_copy(bmap);
		bmap_i = isl_basic_map_project_out(bmap_i, isl_dim_out,
							i + 1, n_out - (1 + i));
		bmap_i = isl_basic_map_project_out(bmap_i, isl_dim_out, 0, i);
		aff = extract_isl_aff_from_basic_map(bmap_i);
		ma = isl_multi_aff_set_aff(ma, i, aff);
	}

	isl_basic_map_free(bmap);

	return ma;
}

/* Create an isl_pw_multi_aff that is equivalent to
 * isl_map_intersect_domain(isl_map_from_basic_map(bmap), domain).
 * The given basic map is such that each output dimension is defined
 * in terms of the parameters and input dimensions using an equality.
 */
static __isl_give isl_pw_multi_aff *plain_pw_multi_aff_from_map(
	__isl_take isl_set *domain, __isl_take isl_basic_map *bmap)
{
	isl_multi_aff *ma;

	ma = extract_isl_multi_aff_from_basic_map(bmap);
	return isl_pw_multi_aff_alloc(domain, ma);
}

/* Try and create an isl_pw_multi_aff that is equivalent to the given isl_map.
 * This obivously only works if the input "map" is single-valued.
 * If so, we compute the lexicographic minimum of the image in the form
 * of an isl_pw_multi_aff.  Since the image is unique, it is equal
 * to its lexicographic minimum.
 * If the input is not single-valued, we produce an error.
 *
 * As a special case, we first check if all output dimensions are uniquely
 * defined in terms of the parameters and input dimensions over the entire
 * domain.  If so, we extract the desired isl_pw_multi_aff directly
 * from the affine hull of "map" and its domain.
 */
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_from_map(__isl_take isl_map *map)
{
	int i;
	int sv;
	isl_pw_multi_aff *pma;
	isl_basic_map *hull;

	if (!map)
		return NULL;

	hull = isl_map_affine_hull(isl_map_copy(map));
	sv = isl_basic_map_plain_is_single_valued(hull);
	if (sv >= 0 && sv)
		return plain_pw_multi_aff_from_map(isl_map_domain(map), hull);
	isl_basic_map_free(hull);
	if (sv < 0)
		goto error;

	sv = isl_map_is_single_valued(map);
	if (sv < 0)
		goto error;
	if (!sv)
		isl_die(isl_map_get_ctx(map), isl_error_invalid,
			"map is not single-valued", goto error);
	map = isl_map_make_disjoint(map);
	if (!map)
		return NULL;

	pma = isl_pw_multi_aff_empty(isl_map_get_space(map));

	for (i = 0; i < map->n; ++i) {
		isl_pw_multi_aff *pma_i;
		isl_basic_map *bmap;
		bmap = isl_basic_map_copy(map->p[i]);
		pma_i = isl_basic_map_lexmin_pw_multi_aff(bmap);
		pma = isl_pw_multi_aff_add_disjoint(pma, pma_i);
	}

	isl_map_free(map);
	return pma;
error:
	isl_map_free(map);
	return NULL;
}

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_from_set(__isl_take isl_set *set)
{
	return isl_pw_multi_aff_from_map(set);
}

/* Return the piecewise affine expression "set ? 1 : 0".
 */
__isl_give isl_pw_aff *isl_set_indicator_function(__isl_take isl_set *set)
{
	isl_pw_aff *pa;
	isl_space *space = isl_set_get_space(set);
	isl_local_space *ls = isl_local_space_from_space(space);
	isl_aff *zero = isl_aff_zero_on_domain(isl_local_space_copy(ls));
	isl_aff *one = isl_aff_zero_on_domain(ls);

	one = isl_aff_add_constant_si(one, 1);
	pa = isl_pw_aff_alloc(isl_set_copy(set), one);
	set = isl_set_complement(set);
	pa = isl_pw_aff_add_disjoint(pa, isl_pw_aff_alloc(set, zero));

	return pa;
}

/* Plug in "subs" for dimension "type", "pos" of "aff".
 *
 * Let i be the dimension to replace and let "subs" be of the form
 *
 *	f/d
 *
 * and "aff" of the form
 *
 *	(a i + g)/m
 *
 * The result is
 *
 *	floor((a f + d g')/(m d))
 *
 * where g' is the result of plugging in "subs" in each of the integer
 * divisions in g.
 */
__isl_give isl_aff *isl_aff_substitute(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned pos, __isl_keep isl_aff *subs)
{
	isl_ctx *ctx;
	isl_int v;

	aff = isl_aff_cow(aff);
	if (!aff || !subs)
		return isl_aff_free(aff);

	ctx = isl_aff_get_ctx(aff);
	if (!isl_space_is_equal(aff->ls->dim, subs->ls->dim))
		isl_die(ctx, isl_error_invalid,
			"spaces don't match", return isl_aff_free(aff));
	if (isl_local_space_dim(subs->ls, isl_dim_div) != 0)
		isl_die(ctx, isl_error_unsupported,
			"cannot handle divs yet", return isl_aff_free(aff));

	aff->ls = isl_local_space_substitute(aff->ls, type, pos, subs);
	if (!aff->ls)
		return isl_aff_free(aff);

	aff->v = isl_vec_cow(aff->v);
	if (!aff->v)
		return isl_aff_free(aff);

	pos += isl_local_space_offset(aff->ls, type);

	isl_int_init(v);
	isl_int_set(v, aff->v->el[1 + pos]);
	isl_int_set_si(aff->v->el[1 + pos], 0);
	isl_seq_combine(aff->v->el + 1, subs->v->el[0], aff->v->el + 1,
			v, subs->v->el + 1, subs->v->size - 1);
	isl_int_mul(aff->v->el[0], aff->v->el[0], subs->v->el[0]);
	isl_int_clear(v);

	return aff;
}

/* Plug in "subs" for dimension "type", "pos" in each of the affine
 * expressions in "maff".
 */
__isl_give isl_multi_aff *isl_multi_aff_substitute(
	__isl_take isl_multi_aff *maff, enum isl_dim_type type, unsigned pos,
	__isl_keep isl_aff *subs)
{
	int i;

	maff = isl_multi_aff_cow(maff);
	if (!maff || !subs)
		return isl_multi_aff_free(maff);

	if (type == isl_dim_in)
		type = isl_dim_set;

	for (i = 0; i < maff->n; ++i) {
		maff->p[i] = isl_aff_substitute(maff->p[i], type, pos, subs);
		if (!maff->p[i])
			return isl_multi_aff_free(maff);
	}

	return maff;
}

/* Plug in "subs" for dimension "type", "pos" of "pma".
 *
 * pma is of the form
 *
 *	A_i(v) -> M_i(v)
 *
 * while subs is of the form
 *
 *	v' = B_j(v) -> S_j
 *
 * Each pair i,j such that C_ij = A_i \cap B_i is non-empty
 * has a contribution in the result, in particular
 *
 *	C_ij(S_j) -> M_i(S_j)
 *
 * Note that plugging in S_j in C_ij may also result in an empty set
 * and this contribution should simply be discarded.
 */
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_substitute(
	__isl_take isl_pw_multi_aff *pma, enum isl_dim_type type, unsigned pos,
	__isl_keep isl_pw_aff *subs)
{
	int i, j, n;
	isl_pw_multi_aff *res;

	if (!pma || !subs)
		return isl_pw_multi_aff_free(pma);

	n = pma->n * subs->n;
	res = isl_pw_multi_aff_alloc_size(isl_space_copy(pma->dim), n);

	for (i = 0; i < pma->n; ++i) {
		for (j = 0; j < subs->n; ++j) {
			isl_set *common;
			isl_multi_aff *res_ij;
			common = isl_set_intersect(
					isl_set_copy(pma->p[i].set),
					isl_set_copy(subs->p[j].set));
			common = isl_set_substitute(common,
					type, pos, subs->p[j].aff);
			if (isl_set_plain_is_empty(common)) {
				isl_set_free(common);
				continue;
			}

			res_ij = isl_multi_aff_substitute(
					isl_multi_aff_copy(pma->p[i].maff),
					type, pos, subs->p[j].aff);

			res = isl_pw_multi_aff_add_piece(res, common, res_ij);
		}
	}

	isl_pw_multi_aff_free(pma);
	return res;
}

/* Extend the local space of "dst" to include the divs
 * in the local space of "src".
 */
__isl_give isl_aff *isl_aff_align_divs(__isl_take isl_aff *dst,
	__isl_keep isl_aff *src)
{
	isl_ctx *ctx;
	int *exp1 = NULL;
	int *exp2 = NULL;
	isl_mat *div;

	if (!src || !dst)
		return isl_aff_free(dst);

	ctx = isl_aff_get_ctx(src);
	if (!isl_space_is_equal(src->ls->dim, dst->ls->dim))
		isl_die(ctx, isl_error_invalid,
			"spaces don't match", goto error);

	if (src->ls->div->n_row == 0)
		return dst;

	exp1 = isl_alloc_array(ctx, int, src->ls->div->n_row);
	exp2 = isl_alloc_array(ctx, int, dst->ls->div->n_row);
	if (!exp1 || !exp2)
		goto error;

	div = isl_merge_divs(src->ls->div, dst->ls->div, exp1, exp2);
	dst = isl_aff_expand_divs(dst, div, exp2);
	free(exp1);
	free(exp2);

	return dst;
error:
	free(exp1);
	free(exp2);
	return isl_aff_free(dst);
}

/* Adjust the local spaces of the affine expressions in "maff"
 * such that they all have the save divs.
 */
__isl_give isl_multi_aff *isl_multi_aff_align_divs(
	__isl_take isl_multi_aff *maff)
{
	int i;

	if (!maff)
		return NULL;
	if (maff->n == 0)
		return maff;
	maff = isl_multi_aff_cow(maff);
	if (!maff)
		return NULL;

	for (i = 1; i < maff->n; ++i)
		maff->p[0] = isl_aff_align_divs(maff->p[0], maff->p[i]);
	for (i = 1; i < maff->n; ++i) {
		maff->p[i] = isl_aff_align_divs(maff->p[i], maff->p[0]);
		if (!maff->p[i])
			return isl_multi_aff_free(maff);
	}

	return maff;
}

__isl_give isl_aff *isl_aff_lift(__isl_take isl_aff *aff)
{
	aff = isl_aff_cow(aff);
	if (!aff)
		return NULL;

	aff->ls = isl_local_space_lift(aff->ls);
	if (!aff->ls)
		return isl_aff_free(aff);

	return aff;
}

/* Lift "maff" to a space with extra dimensions such that the result
 * has no more existentially quantified variables.
 * If "ls" is not NULL, then *ls is assigned the local space that lies
 * at the basis of the lifting applied to "maff".
 */
__isl_give isl_multi_aff *isl_multi_aff_lift(__isl_take isl_multi_aff *maff,
	__isl_give isl_local_space **ls)
{
	int i;
	isl_space *space;
	unsigned n_div;

	if (ls)
		*ls = NULL;

	if (!maff)
		return NULL;

	if (maff->n == 0) {
		if (ls) {
			isl_space *space = isl_multi_aff_get_domain_space(maff);
			*ls = isl_local_space_from_space(space);
			if (!*ls)
				return isl_multi_aff_free(maff);
		}
		return maff;
	}

	maff = isl_multi_aff_cow(maff);
	maff = isl_multi_aff_align_divs(maff);
	if (!maff)
		return NULL;

	n_div = isl_aff_dim(maff->p[0], isl_dim_div);
	space = isl_multi_aff_get_space(maff);
	space = isl_space_lift(isl_space_domain(space), n_div);
	space = isl_space_extend_domain_with_range(space,
						isl_multi_aff_get_space(maff));
	if (!space)
		return isl_multi_aff_free(maff);
	isl_space_free(maff->space);
	maff->space = space;

	if (ls) {
		*ls = isl_aff_get_domain_local_space(maff->p[0]);
		if (!*ls)
			return isl_multi_aff_free(maff);
	}

	for (i = 0; i < maff->n; ++i) {
		maff->p[i] = isl_aff_lift(maff->p[i]);
		if (!maff->p[i])
			goto error;
	}

	return maff;
error:
	if (ls)
		isl_local_space_free(*ls);
	return isl_multi_aff_free(maff);
}


/* Extract an isl_pw_aff corresponding to output dimension "pos" of "pma".
 */
__isl_give isl_pw_aff *isl_pw_multi_aff_get_pw_aff(
	__isl_keep isl_pw_multi_aff *pma, int pos)
{
	int i;
	int n_out;
	isl_space *space;
	isl_pw_aff *pa;

	if (!pma)
		return NULL;

	n_out = isl_pw_multi_aff_dim(pma, isl_dim_out);
	if (pos < 0 || pos >= n_out)
		isl_die(isl_pw_multi_aff_get_ctx(pma), isl_error_invalid,
			"index out of bounds", return NULL);

	space = isl_pw_multi_aff_get_space(pma);
	space = isl_space_drop_dims(space, isl_dim_out,
				    pos + 1, n_out - pos - 1);
	space = isl_space_drop_dims(space, isl_dim_out, 0, pos);

	pa = isl_pw_aff_alloc_size(space, pma->n);
	for (i = 0; i < pma->n; ++i) {
		isl_aff *aff;
		aff = isl_multi_aff_get_aff(pma->p[i].maff, pos);
		pa = isl_pw_aff_add_piece(pa, isl_set_copy(pma->p[i].set), aff);
	}

	return pa;
}

/* Return an isl_pw_multi_aff with the given "set" as domain and
 * an unnamed zero-dimensional range.
 */
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_from_domain(
	__isl_take isl_set *set)
{
	isl_multi_aff *ma;
	isl_space *space;

	space = isl_set_get_space(set);
	space = isl_space_from_domain(space);
	ma = isl_multi_aff_zero(space);
	return isl_pw_multi_aff_alloc(set, ma);
}

/* Add an isl_pw_multi_aff with the given "set" as domain and
 * an unnamed zero-dimensional range to *user.
 */
static int add_pw_multi_aff_from_domain(__isl_take isl_set *set, void *user)
{
	isl_union_pw_multi_aff **upma = user;
	isl_pw_multi_aff *pma;

	pma = isl_pw_multi_aff_from_domain(set);
	*upma = isl_union_pw_multi_aff_add_pw_multi_aff(*upma, pma);

	return 0;
}

/* Return an isl_union_pw_multi_aff with the given "uset" as domain and
 * an unnamed zero-dimensional range.
 */
__isl_give isl_union_pw_multi_aff *isl_union_pw_multi_aff_from_domain(
	__isl_take isl_union_set *uset)
{
	isl_space *space;
	isl_union_pw_multi_aff *upma;

	if (!uset)
		return NULL;

	space = isl_union_set_get_space(uset);
	upma = isl_union_pw_multi_aff_empty(space);

	if (isl_union_set_foreach_set(uset,
				    &add_pw_multi_aff_from_domain, &upma) < 0)
		goto error;

	isl_union_set_free(uset);
	return upma;
error:
	isl_union_set_free(uset);
	isl_union_pw_multi_aff_free(upma);
	return NULL;
}

/* Convert "pma" to an isl_map and add it to *umap.
 */
static int map_from_pw_multi_aff(__isl_take isl_pw_multi_aff *pma, void *user)
{
	isl_union_map **umap = user;
	isl_map *map;

	map = isl_map_from_pw_multi_aff(pma);
	*umap = isl_union_map_add_map(*umap, map);

	return 0;
}

/* Construct a union map mapping the domain of the union
 * piecewise multi-affine expression to its range, with each dimension
 * in the range equated to the corresponding affine expression on its cell.
 */
__isl_give isl_union_map *isl_union_map_from_union_pw_multi_aff(
	__isl_take isl_union_pw_multi_aff *upma)
{
	isl_space *space;
	isl_union_map *umap;

	if (!upma)
		return NULL;

	space = isl_union_pw_multi_aff_get_space(upma);
	umap = isl_union_map_empty(space);

	if (isl_union_pw_multi_aff_foreach_pw_multi_aff(upma,
					&map_from_pw_multi_aff, &umap) < 0)
		goto error;

	isl_union_pw_multi_aff_free(upma);
	return umap;
error:
	isl_union_pw_multi_aff_free(upma);
	isl_union_map_free(umap);
	return NULL;
}

/* Local data for bin_entry and the callback "fn".
 */
struct isl_union_pw_multi_aff_bin_data {
	isl_union_pw_multi_aff *upma2;
	isl_union_pw_multi_aff *res;
	isl_pw_multi_aff *pma;
	int (*fn)(void **entry, void *user);
};

/* Given an isl_pw_multi_aff from upma1, store it in data->pma
 * and call data->fn for each isl_pw_multi_aff in data->upma2.
 */
static int bin_entry(void **entry, void *user)
{
	struct isl_union_pw_multi_aff_bin_data *data = user;
	isl_pw_multi_aff *pma = *entry;

	data->pma = pma;
	if (isl_hash_table_foreach(data->upma2->dim->ctx, &data->upma2->table,
				   data->fn, data) < 0)
		return -1;

	return 0;
}

/* Call "fn" on each pair of isl_pw_multi_affs in "upma1" and "upma2".
 * The isl_pw_multi_aff from upma1 is stored in data->pma (where data is
 * passed as user field) and the isl_pw_multi_aff from upma2 is available
 * as *entry.  The callback should adjust data->res if desired.
 */
static __isl_give isl_union_pw_multi_aff *bin_op(
	__isl_take isl_union_pw_multi_aff *upma1,
	__isl_take isl_union_pw_multi_aff *upma2,
	int (*fn)(void **entry, void *user))
{
	isl_space *space;
	struct isl_union_pw_multi_aff_bin_data data = { NULL, NULL, NULL, fn };

	space = isl_union_pw_multi_aff_get_space(upma2);
	upma1 = isl_union_pw_multi_aff_align_params(upma1, space);
	space = isl_union_pw_multi_aff_get_space(upma1);
	upma2 = isl_union_pw_multi_aff_align_params(upma2, space);

	if (!upma1 || !upma2)
		goto error;

	data.upma2 = upma2;
	data.res = isl_union_pw_multi_aff_alloc(isl_space_copy(upma1->dim),
				       upma1->table.n);
	if (isl_hash_table_foreach(upma1->dim->ctx, &upma1->table,
				   &bin_entry, &data) < 0)
		goto error;

	isl_union_pw_multi_aff_free(upma1);
	isl_union_pw_multi_aff_free(upma2);
	return data.res;
error:
	isl_union_pw_multi_aff_free(upma1);
	isl_union_pw_multi_aff_free(upma2);
	isl_union_pw_multi_aff_free(data.res);
	return NULL;
}

/* Given two isl_multi_affs A -> B and C -> D,
 * construct an isl_multi_aff (A * C) -> (B, D).
 */
__isl_give isl_multi_aff *isl_multi_aff_flat_range_product(
	__isl_take isl_multi_aff *ma1, __isl_take isl_multi_aff *ma2)
{
	int i, n1, n2;
	isl_aff *aff;
	isl_space *space;
	isl_multi_aff *res;

	if (!ma1 || !ma2)
		goto error;

	space = isl_space_range_product(isl_multi_aff_get_space(ma1),
					isl_multi_aff_get_space(ma2));
	space = isl_space_flatten_range(space);
	res = isl_multi_aff_alloc(space);

	n1 = isl_multi_aff_dim(ma1, isl_dim_out);
	n2 = isl_multi_aff_dim(ma2, isl_dim_out);

	for (i = 0; i < n1; ++i) {
		aff = isl_multi_aff_get_aff(ma1, i);
		res = isl_multi_aff_set_aff(res, i, aff);
	}

	for (i = 0; i < n2; ++i) {
		aff = isl_multi_aff_get_aff(ma2, i);
		res = isl_multi_aff_set_aff(res, n1 + i, aff);
	}

	isl_multi_aff_free(ma1);
	isl_multi_aff_free(ma2);
	return res;
error:
	isl_multi_aff_free(ma1);
	isl_multi_aff_free(ma2);
	return NULL;
}

/* Given two aligned isl_pw_multi_affs A -> B and C -> D,
 * construct an isl_pw_multi_aff (A * C) -> (B, D).
 */
static __isl_give isl_pw_multi_aff *pw_multi_aff_flat_range_product(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2)
{
	isl_space *space;

	space = isl_space_range_product(isl_pw_multi_aff_get_space(pma1),
					isl_pw_multi_aff_get_space(pma2));
	space = isl_space_flatten_range(space);
	return isl_pw_multi_aff_on_shared_domain_in(pma1, pma2, space,
					    &isl_multi_aff_flat_range_product);
}

/* Given two isl_pw_multi_affs A -> B and C -> D,
 * construct an isl_pw_multi_aff (A * C) -> (B, D).
 */
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_flat_range_product(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2)
{
	return isl_pw_multi_aff_align_params_pw_pw_and(pma1, pma2,
					    &pw_multi_aff_flat_range_product);
}

/* If data->pma and *entry have the same domain space, then compute
 * their flat range product and the result to data->res.
 */
static int flat_range_product_entry(void **entry, void *user)
{
	struct isl_union_pw_multi_aff_bin_data *data = user;
	isl_pw_multi_aff *pma2 = *entry;

	if (!isl_space_tuple_match(data->pma->dim, isl_dim_in,
				 pma2->dim, isl_dim_in))
		return 0;

	pma2 = isl_pw_multi_aff_flat_range_product(
					isl_pw_multi_aff_copy(data->pma),
					isl_pw_multi_aff_copy(pma2));

	data->res = isl_union_pw_multi_aff_add_pw_multi_aff(data->res, pma2);

	return 0;
}

/* Given two isl_union_pw_multi_affs A -> B and C -> D,
 * construct an isl_union_pw_multi_aff (A * C) -> (B, D).
 */
__isl_give isl_union_pw_multi_aff *isl_union_pw_multi_aff_flat_range_product(
	__isl_take isl_union_pw_multi_aff *upma1,
	__isl_take isl_union_pw_multi_aff *upma2)
{
	return bin_op(upma1, upma2, &flat_range_product_entry);
}
