/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#include <isl_map_private.h>
#include <isl_morph.h>
#include <isl/seq.h>
#include <isl_mat_private.h>
#include <isl_space_private.h>
#include <isl_equalities.h>

__isl_give isl_morph *isl_morph_alloc(
	__isl_take isl_basic_set *dom, __isl_take isl_basic_set *ran,
	__isl_take isl_mat *map, __isl_take isl_mat *inv)
{
	isl_morph *morph;

	if (!dom || !ran || !map || !inv)
		goto error;

	morph = isl_alloc_type(dom->ctx, struct isl_morph);
	if (!morph)
		goto error;

	morph->ref = 1;
	morph->dom = dom;
	morph->ran = ran;
	morph->map = map;
	morph->inv = inv;

	return morph;
error:
	isl_basic_set_free(dom);
	isl_basic_set_free(ran);
	isl_mat_free(map);
	isl_mat_free(inv);
	return NULL;
}

__isl_give isl_morph *isl_morph_copy(__isl_keep isl_morph *morph)
{
	if (!morph)
		return NULL;

	morph->ref++;
	return morph;
}

__isl_give isl_morph *isl_morph_dup(__isl_keep isl_morph *morph)
{
	if (!morph)
		return NULL;

	return isl_morph_alloc(isl_basic_set_copy(morph->dom),
		isl_basic_set_copy(morph->ran),
		isl_mat_copy(morph->map), isl_mat_copy(morph->inv));
}

__isl_give isl_morph *isl_morph_cow(__isl_take isl_morph *morph)
{
	if (!morph)
		return NULL;

	if (morph->ref == 1)
		return morph;
	morph->ref--;
	return isl_morph_dup(morph);
}

void isl_morph_free(__isl_take isl_morph *morph)
{
	if (!morph)
		return;

	if (--morph->ref > 0)
		return;

	isl_basic_set_free(morph->dom);
	isl_basic_set_free(morph->ran);
	isl_mat_free(morph->map);
	isl_mat_free(morph->inv);
	free(morph);
}

__isl_give isl_space *isl_morph_get_ran_space(__isl_keep isl_morph *morph)
{
	if (!morph)
		return NULL;
	
	return isl_space_copy(morph->ran->dim);
}

unsigned isl_morph_dom_dim(__isl_keep isl_morph *morph, enum isl_dim_type type)
{
	if (!morph)
		return 0;

	return isl_basic_set_dim(morph->dom, type);
}

unsigned isl_morph_ran_dim(__isl_keep isl_morph *morph, enum isl_dim_type type)
{
	if (!morph)
		return 0;

	return isl_basic_set_dim(morph->ran, type);
}

__isl_give isl_morph *isl_morph_remove_dom_dims(__isl_take isl_morph *morph,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	unsigned dom_offset;

	if (n == 0)
		return morph;

	morph = isl_morph_cow(morph);
	if (!morph)
		return NULL;

	dom_offset = 1 + isl_space_offset(morph->dom->dim, type);

	morph->dom = isl_basic_set_remove_dims(morph->dom, type, first, n);

	morph->map = isl_mat_drop_cols(morph->map, dom_offset + first, n);

	morph->inv = isl_mat_drop_rows(morph->inv, dom_offset + first, n);

	if (morph->dom && morph->ran && morph->map && morph->inv)
		return morph;

	isl_morph_free(morph);
	return NULL;
}

__isl_give isl_morph *isl_morph_remove_ran_dims(__isl_take isl_morph *morph,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	unsigned ran_offset;

	if (n == 0)
		return morph;

	morph = isl_morph_cow(morph);
	if (!morph)
		return NULL;

	ran_offset = 1 + isl_space_offset(morph->ran->dim, type);

	morph->ran = isl_basic_set_remove_dims(morph->ran, type, first, n);

	morph->map = isl_mat_drop_rows(morph->map, ran_offset + first, n);

	morph->inv = isl_mat_drop_cols(morph->inv, ran_offset + first, n);

	if (morph->dom && morph->ran && morph->map && morph->inv)
		return morph;

	isl_morph_free(morph);
	return NULL;
}

/* Project domain of morph onto its parameter domain.
 */
__isl_give isl_morph *isl_morph_dom_params(__isl_take isl_morph *morph)
{
	unsigned n;

	morph = isl_morph_cow(morph);
	if (!morph)
		return NULL;
	n = isl_basic_set_dim(morph->dom, isl_dim_set);
	morph = isl_morph_remove_dom_dims(morph, isl_dim_set, 0, n);
	if (!morph)
		return NULL;
	morph->dom = isl_basic_set_params(morph->dom);
	if (morph->dom)
		return morph;

	isl_morph_free(morph);
	return NULL;
}

/* Project range of morph onto its parameter domain.
 */
__isl_give isl_morph *isl_morph_ran_params(__isl_take isl_morph *morph)
{
	unsigned n;

	morph = isl_morph_cow(morph);
	if (!morph)
		return NULL;
	n = isl_basic_set_dim(morph->ran, isl_dim_set);
	morph = isl_morph_remove_ran_dims(morph, isl_dim_set, 0, n);
	if (!morph)
		return NULL;
	morph->ran = isl_basic_set_params(morph->ran);
	if (morph->ran)
		return morph;

	isl_morph_free(morph);
	return NULL;
}

void isl_morph_print_internal(__isl_take isl_morph *morph, FILE *out)
{
	if (!morph)
		return;

	isl_basic_set_print(morph->dom, out, 0, "", "", ISL_FORMAT_ISL);
	isl_basic_set_print(morph->ran, out, 0, "", "", ISL_FORMAT_ISL);
	isl_mat_print_internal(morph->map, out, 4);
	isl_mat_print_internal(morph->inv, out, 4);
}

void isl_morph_dump(__isl_take isl_morph *morph)
{
	isl_morph_print_internal(morph, stderr);
}

__isl_give isl_morph *isl_morph_identity(__isl_keep isl_basic_set *bset)
{
	isl_mat *id;
	isl_basic_set *universe;
	unsigned total;

	if (!bset)
		return NULL;

	total = isl_basic_set_total_dim(bset);
	id = isl_mat_identity(bset->ctx, 1 + total);
	universe = isl_basic_set_universe(isl_space_copy(bset->dim));

	return isl_morph_alloc(universe, isl_basic_set_copy(universe),
		id, isl_mat_copy(id));
}

/* Create a(n identity) morphism between empty sets of the same dimension
 * a "bset".
 */
__isl_give isl_morph *isl_morph_empty(__isl_keep isl_basic_set *bset)
{
	isl_mat *id;
	isl_basic_set *empty;
	unsigned total;

	if (!bset)
		return NULL;

	total = isl_basic_set_total_dim(bset);
	id = isl_mat_identity(bset->ctx, 1 + total);
	empty = isl_basic_set_empty(isl_space_copy(bset->dim));

	return isl_morph_alloc(empty, isl_basic_set_copy(empty),
		id, isl_mat_copy(id));
}

/* Given a matrix that maps a (possibly) parametric domain to
 * a parametric domain, add in rows that map the "nparam" parameters onto
 * themselves.
 */
static __isl_give isl_mat *insert_parameter_rows(__isl_take isl_mat *mat,
	unsigned nparam)
{
	int i;

	if (nparam == 0)
		return mat;
	if (!mat)
		return NULL;

	mat = isl_mat_insert_rows(mat, 1, nparam);
	if (!mat)
		return NULL;

	for (i = 0; i < nparam; ++i) {
		isl_seq_clr(mat->row[1 + i], mat->n_col);
		isl_int_set(mat->row[1 + i][1 + i], mat->row[0][0]);
	}

	return mat;
}

/* Construct a basic set described by the "n" equalities of "bset" starting
 * at "first".
 */
static __isl_give isl_basic_set *copy_equalities(__isl_keep isl_basic_set *bset,
	unsigned first, unsigned n)
{
	int i, k;
	isl_basic_set *eq;
	unsigned total;

	isl_assert(bset->ctx, bset->n_div == 0, return NULL);

	total = isl_basic_set_total_dim(bset);
	eq = isl_basic_set_alloc_space(isl_space_copy(bset->dim), 0, n, 0);
	if (!eq)
		return NULL;
	for (i = 0; i < n; ++i) {
		k = isl_basic_set_alloc_equality(eq);
		if (k < 0)
			goto error;
		isl_seq_cpy(eq->eq[k], bset->eq[first + k], 1 + total);
	}

	return eq;
error:
	isl_basic_set_free(eq);
	return NULL;
}

/* Given a basic set, exploit the equalties in the basic set to construct
 * a morphishm that maps the basic set to a lower-dimensional space.
 * Specifically, the morphism reduces the number of dimensions of type "type".
 *
 * This function is a slight generalization of isl_mat_variable_compression
 * in that it allows the input to be parametric and that it allows for the
 * compression of either parameters or set variables.
 *
 * We first select the equalities of interest, that is those that involve
 * variables of type "type" and no later variables.
 * Denote those equalities as
 *
 *		-C(p) + M x = 0
 *
 * where C(p) depends on the parameters if type == isl_dim_set and
 * is a constant if type == isl_dim_param.
 *
 * First compute the (left) Hermite normal form of M,
 *
 *		M [U1 U2] = M U = H = [H1 0]
 * or
 *		              M = H Q = [H1 0] [Q1]
 *                                             [Q2]
 *
 * with U, Q unimodular, Q = U^{-1} (and H lower triangular).
 * Define the transformed variables as
 *
 *		x = [U1 U2] [ x1' ] = [U1 U2] [Q1] x
 *		            [ x2' ]           [Q2]
 *
 * The equalities then become
 *
 *		-C(p) + H1 x1' = 0   or   x1' = H1^{-1} C(p) = C'(p)
 *
 * If the denominator of the constant term does not divide the
 * the common denominator of the parametric terms, then every
 * integer point is mapped to a non-integer point and then the original set has no
 * integer solutions (since the x' are a unimodular transformation
 * of the x).  In this case, an empty morphism is returned.
 * Otherwise, the transformation is given by
 *
 *		x = U1 H1^{-1} C(p) + U2 x2'
 *
 * The inverse transformation is simply
 *
 *		x2' = Q2 x
 *
 * Both matrices are extended to map the full original space to the full
 * compressed space.
 */
__isl_give isl_morph *isl_basic_set_variable_compression(
	__isl_keep isl_basic_set *bset, enum isl_dim_type type)
{
	unsigned otype;
	unsigned ntype;
	unsigned orest;
	unsigned nrest;
	int f_eq, n_eq;
	isl_space *dim;
	isl_mat *H, *U, *Q, *C = NULL, *H1, *U1, *U2;
	isl_basic_set *dom, *ran;

	if (!bset)
		return NULL;

	if (isl_basic_set_plain_is_empty(bset))
		return isl_morph_empty(bset);

	isl_assert(bset->ctx, bset->n_div == 0, return NULL);

	otype = 1 + isl_space_offset(bset->dim, type);
	ntype = isl_basic_set_dim(bset, type);
	orest = otype + ntype;
	nrest = isl_basic_set_total_dim(bset) - (orest - 1);

	for (f_eq = 0; f_eq < bset->n_eq; ++f_eq)
		if (isl_seq_first_non_zero(bset->eq[f_eq] + orest, nrest) == -1)
			break;
	for (n_eq = 0; f_eq + n_eq < bset->n_eq; ++n_eq)
		if (isl_seq_first_non_zero(bset->eq[f_eq + n_eq] + otype, ntype) == -1)
			break;
	if (n_eq == 0)
		return isl_morph_identity(bset);

	H = isl_mat_sub_alloc6(bset->ctx, bset->eq, f_eq, n_eq, otype, ntype);
	H = isl_mat_left_hermite(H, 0, &U, &Q);
	if (!H || !U || !Q)
		goto error;
	Q = isl_mat_drop_rows(Q, 0, n_eq);
	Q = isl_mat_diagonal(isl_mat_identity(bset->ctx, otype), Q);
	Q = isl_mat_diagonal(Q, isl_mat_identity(bset->ctx, nrest));
	C = isl_mat_alloc(bset->ctx, 1 + n_eq, otype);
	if (!C)
		goto error;
	isl_int_set_si(C->row[0][0], 1);
	isl_seq_clr(C->row[0] + 1, otype - 1);
	isl_mat_sub_neg(C->ctx, C->row + 1, bset->eq + f_eq, n_eq, 0, 0, otype);
	H1 = isl_mat_sub_alloc(H, 0, H->n_row, 0, H->n_row);
	H1 = isl_mat_lin_to_aff(H1);
	C = isl_mat_inverse_product(H1, C);
	if (!C)
		goto error;
	isl_mat_free(H);

	if (!isl_int_is_one(C->row[0][0])) {
		int i;
		isl_int g;

		isl_int_init(g);
		for (i = 0; i < n_eq; ++i) {
			isl_seq_gcd(C->row[1 + i] + 1, otype - 1, &g);
			isl_int_gcd(g, g, C->row[0][0]);
			if (!isl_int_is_divisible_by(C->row[1 + i][0], g))
				break;
		}
		isl_int_clear(g);

		if (i < n_eq) {
			isl_mat_free(C);
			isl_mat_free(U);
			isl_mat_free(Q);
			return isl_morph_empty(bset);
		}

		C = isl_mat_normalize(C);
	}

	U1 = isl_mat_sub_alloc(U, 0, U->n_row, 0, n_eq);
	U1 = isl_mat_lin_to_aff(U1);
	U2 = isl_mat_sub_alloc(U, 0, U->n_row, n_eq, U->n_row - n_eq);
	U2 = isl_mat_lin_to_aff(U2);
	isl_mat_free(U);

	C = isl_mat_product(U1, C);
	C = isl_mat_aff_direct_sum(C, U2);
	C = insert_parameter_rows(C, otype - 1);
	C = isl_mat_diagonal(C, isl_mat_identity(bset->ctx, nrest));

	dim = isl_space_copy(bset->dim);
	dim = isl_space_drop_dims(dim, type, 0, ntype);
	dim = isl_space_add_dims(dim, type, ntype - n_eq);
	ran = isl_basic_set_universe(dim);
	dom = copy_equalities(bset, f_eq, n_eq);

	return isl_morph_alloc(dom, ran, Q, C);
error:
	isl_mat_free(C);
	isl_mat_free(H);
	isl_mat_free(U);
	isl_mat_free(Q);
	return NULL;
}

/* Construct a parameter compression for "bset".
 * We basically just call isl_mat_parameter_compression with the right input
 * and then extend the resulting matrix to include the variables.
 *
 * Let the equalities be given as
 *
 *	B(p) + A x = 0
 *
 * and let [H 0] be the Hermite Normal Form of A, then
 *
 *	H^-1 B(p)
 *
 * needs to be integer, so we impose that each row is divisible by
 * the denominator.
 */
__isl_give isl_morph *isl_basic_set_parameter_compression(
	__isl_keep isl_basic_set *bset)
{
	unsigned nparam;
	unsigned nvar;
	int n_eq;
	isl_mat *H, *B;
	isl_vec *d;
	isl_mat *map, *inv;
	isl_basic_set *dom, *ran;

	if (!bset)
		return NULL;

	if (isl_basic_set_plain_is_empty(bset))
		return isl_morph_empty(bset);
	if (bset->n_eq == 0)
		return isl_morph_identity(bset);

	isl_assert(bset->ctx, bset->n_div == 0, return NULL);

	n_eq = bset->n_eq;
	nparam = isl_basic_set_dim(bset, isl_dim_param);
	nvar = isl_basic_set_dim(bset, isl_dim_set);

	isl_assert(bset->ctx, n_eq <= nvar, return NULL);

	d = isl_vec_alloc(bset->ctx, n_eq);
	B = isl_mat_sub_alloc6(bset->ctx, bset->eq, 0, n_eq, 0, 1 + nparam);
	H = isl_mat_sub_alloc6(bset->ctx, bset->eq, 0, n_eq, 1 + nparam, nvar);
	H = isl_mat_left_hermite(H, 0, NULL, NULL);
	H = isl_mat_drop_cols(H, n_eq, nvar - n_eq);
	H = isl_mat_lin_to_aff(H);
	H = isl_mat_right_inverse(H);
	if (!H || !d)
		goto error;
	d = isl_vec_set(d, H->row[0][0]);
	H = isl_mat_drop_rows(H, 0, 1);
	H = isl_mat_drop_cols(H, 0, 1);
	B = isl_mat_product(H, B);
	inv = isl_mat_parameter_compression(B, d);
	inv = isl_mat_diagonal(inv, isl_mat_identity(bset->ctx, nvar));
	map = isl_mat_right_inverse(isl_mat_copy(inv));

	dom = isl_basic_set_universe(isl_space_copy(bset->dim));
	ran = isl_basic_set_universe(isl_space_copy(bset->dim));

	return isl_morph_alloc(dom, ran, map, inv);
error:
	isl_mat_free(H);
	isl_mat_free(B);
	isl_vec_free(d);
	return NULL;
}

/* Add stride constraints to "bset" based on the inverse mapping
 * that was plugged in.  In particular, if morph maps x' to x,
 * the the constraints of the original input
 *
 *	A x' + b >= 0
 *
 * have been rewritten to
 *
 *	A inv x + b >= 0
 *
 * However, this substitution may loose information on the integrality of x',
 * so we need to impose that
 *
 *	inv x
 *
 * is integral.  If inv = B/d, this means that we need to impose that
 *
 *	B x = 0		mod d
 *
 * or
 *
 *	exists alpha in Z^m: B x = d alpha
 *
 */
static __isl_give isl_basic_set *add_strides(__isl_take isl_basic_set *bset,
	__isl_keep isl_morph *morph)
{
	int i, div, k;
	isl_int gcd;

	if (isl_int_is_one(morph->inv->row[0][0]))
		return bset;

	isl_int_init(gcd);

	for (i = 0; 1 + i < morph->inv->n_row; ++i) {
		isl_seq_gcd(morph->inv->row[1 + i], morph->inv->n_col, &gcd);
		if (isl_int_is_divisible_by(gcd, morph->inv->row[0][0]))
			continue;
		div = isl_basic_set_alloc_div(bset);
		if (div < 0)
			goto error;
		isl_int_set_si(bset->div[div][0], 0);
		k = isl_basic_set_alloc_equality(bset);
		if (k < 0)
			goto error;
		isl_seq_cpy(bset->eq[k], morph->inv->row[1 + i],
			    morph->inv->n_col);
		isl_seq_clr(bset->eq[k] + morph->inv->n_col, bset->n_div);
		isl_int_set(bset->eq[k][morph->inv->n_col + div],
			    morph->inv->row[0][0]);
	}

	isl_int_clear(gcd);

	return bset;
error:
	isl_int_clear(gcd);
	isl_basic_set_free(bset);
	return NULL;
}

/* Apply the morphism to the basic set.
 * We basically just compute the preimage of "bset" under the inverse mapping
 * in morph, add in stride constraints and intersect with the range
 * of the morphism.
 */
__isl_give isl_basic_set *isl_morph_basic_set(__isl_take isl_morph *morph,
	__isl_take isl_basic_set *bset)
{
	isl_basic_set *res = NULL;
	isl_mat *mat = NULL;
	int i, k;
	int max_stride;

	if (!morph || !bset)
		goto error;

	isl_assert(bset->ctx, isl_space_is_equal(bset->dim, morph->dom->dim),
		    goto error);

	max_stride = morph->inv->n_row - 1;
	if (isl_int_is_one(morph->inv->row[0][0]))
		max_stride = 0;
	res = isl_basic_set_alloc_space(isl_space_copy(morph->ran->dim),
		bset->n_div + max_stride, bset->n_eq + max_stride, bset->n_ineq);

	for (i = 0; i < bset->n_div; ++i)
		if (isl_basic_set_alloc_div(res) < 0)
			goto error;

	mat = isl_mat_sub_alloc6(bset->ctx, bset->eq, 0, bset->n_eq,
					0, morph->inv->n_row);
	mat = isl_mat_product(mat, isl_mat_copy(morph->inv));
	if (!mat)
		goto error;
	for (i = 0; i < bset->n_eq; ++i) {
		k = isl_basic_set_alloc_equality(res);
		if (k < 0)
			goto error;
		isl_seq_cpy(res->eq[k], mat->row[i], mat->n_col);
		isl_seq_scale(res->eq[k] + mat->n_col, bset->eq[i] + mat->n_col,
				morph->inv->row[0][0], bset->n_div);
	}
	isl_mat_free(mat);

	mat = isl_mat_sub_alloc6(bset->ctx, bset->ineq, 0, bset->n_ineq,
					0, morph->inv->n_row);
	mat = isl_mat_product(mat, isl_mat_copy(morph->inv));
	if (!mat)
		goto error;
	for (i = 0; i < bset->n_ineq; ++i) {
		k = isl_basic_set_alloc_inequality(res);
		if (k < 0)
			goto error;
		isl_seq_cpy(res->ineq[k], mat->row[i], mat->n_col);
		isl_seq_scale(res->ineq[k] + mat->n_col,
				bset->ineq[i] + mat->n_col,
				morph->inv->row[0][0], bset->n_div);
	}
	isl_mat_free(mat);

	mat = isl_mat_sub_alloc6(bset->ctx, bset->div, 0, bset->n_div,
					1, morph->inv->n_row);
	mat = isl_mat_product(mat, isl_mat_copy(morph->inv));
	if (!mat)
		goto error;
	for (i = 0; i < bset->n_div; ++i) {
		isl_int_mul(res->div[i][0],
				morph->inv->row[0][0], bset->div[i][0]);
		isl_seq_cpy(res->div[i] + 1, mat->row[i], mat->n_col);
		isl_seq_scale(res->div[i] + 1 + mat->n_col,
				bset->div[i] + 1 + mat->n_col,
				morph->inv->row[0][0], bset->n_div);
	}
	isl_mat_free(mat);

	res = add_strides(res, morph);

	if (isl_basic_set_is_rational(bset))
		res = isl_basic_set_set_rational(res);

	res = isl_basic_set_simplify(res);
	res = isl_basic_set_finalize(res);

	res = isl_basic_set_intersect(res, isl_basic_set_copy(morph->ran));

	isl_morph_free(morph);
	isl_basic_set_free(bset);
	return res;
error:
	isl_mat_free(mat);
	isl_morph_free(morph);
	isl_basic_set_free(bset);
	isl_basic_set_free(res);
	return NULL;
}

/* Apply the morphism to the set.
 */
__isl_give isl_set *isl_morph_set(__isl_take isl_morph *morph,
	__isl_take isl_set *set)
{
	int i;

	if (!morph || !set)
		goto error;

	isl_assert(set->ctx, isl_space_is_equal(set->dim, morph->dom->dim), goto error);

	set = isl_set_cow(set);
	if (!set)
		goto error;

	isl_space_free(set->dim);
	set->dim = isl_space_copy(morph->ran->dim);
	if (!set->dim)
		goto error;

	for (i = 0; i < set->n; ++i) {
		set->p[i] = isl_morph_basic_set(isl_morph_copy(morph), set->p[i]);
		if (!set->p[i])
			goto error;
	}

	isl_morph_free(morph);

	ISL_F_CLR(set, ISL_SET_NORMALIZED);

	return set;
error:
	isl_set_free(set);
	isl_morph_free(morph);
	return NULL;
}

/* Construct a morphism that first does morph2 and then morph1.
 */
__isl_give isl_morph *isl_morph_compose(__isl_take isl_morph *morph1,
	__isl_take isl_morph *morph2)
{
	isl_mat *map, *inv;
	isl_basic_set *dom, *ran;

	if (!morph1 || !morph2)
		goto error;

	map = isl_mat_product(isl_mat_copy(morph1->map), isl_mat_copy(morph2->map));
	inv = isl_mat_product(isl_mat_copy(morph2->inv), isl_mat_copy(morph1->inv));
	dom = isl_morph_basic_set(isl_morph_inverse(isl_morph_copy(morph2)),
				  isl_basic_set_copy(morph1->dom));
	dom = isl_basic_set_intersect(dom, isl_basic_set_copy(morph2->dom));
	ran = isl_morph_basic_set(isl_morph_copy(morph1),
				  isl_basic_set_copy(morph2->ran));
	ran = isl_basic_set_intersect(ran, isl_basic_set_copy(morph1->ran));

	isl_morph_free(morph1);
	isl_morph_free(morph2);

	return isl_morph_alloc(dom, ran, map, inv);
error:
	isl_morph_free(morph1);
	isl_morph_free(morph2);
	return NULL;
}

__isl_give isl_morph *isl_morph_inverse(__isl_take isl_morph *morph)
{
	isl_basic_set *bset;
	isl_mat *mat;

	morph = isl_morph_cow(morph);
	if (!morph)
		return NULL;

	bset = morph->dom;
	morph->dom = morph->ran;
	morph->ran = bset;

	mat = morph->map;
	morph->map = morph->inv;
	morph->inv = mat;

	return morph;
}

/* We detect all the equalities first to avoid implicit equalties
 * being discovered during the computations.  In particular,
 * the compression on the variables could expose additional stride
 * constraints on the parameters.  This would result in existentially
 * quantified variables after applying the resulting morph, which
 * in turn could break invariants of the calling functions.
 */
__isl_give isl_morph *isl_basic_set_full_compression(
	__isl_keep isl_basic_set *bset)
{
	isl_morph *morph, *morph2;

	bset = isl_basic_set_copy(bset);
	bset = isl_basic_set_detect_equalities(bset);

	morph = isl_basic_set_variable_compression(bset, isl_dim_param);
	bset = isl_morph_basic_set(isl_morph_copy(morph), bset);

	morph2 = isl_basic_set_parameter_compression(bset);
	bset = isl_morph_basic_set(isl_morph_copy(morph2), bset);

	morph = isl_morph_compose(morph2, morph);

	morph2 = isl_basic_set_variable_compression(bset, isl_dim_set);
	isl_basic_set_free(bset);

	morph = isl_morph_compose(morph2, morph);

	return morph;
}

__isl_give isl_vec *isl_morph_vec(__isl_take isl_morph *morph,
	__isl_take isl_vec *vec)
{
	if (!morph)
		goto error;

	vec = isl_mat_vec_product(isl_mat_copy(morph->map), vec);

	isl_morph_free(morph);
	return vec;
error:
	isl_morph_free(morph);
	isl_vec_free(vec);
	return NULL;
}
