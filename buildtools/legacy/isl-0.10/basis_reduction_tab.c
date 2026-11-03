/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <assert.h>
#include <isl_map_private.h>
#include <isl/seq.h>
#include "isl_tab.h"

struct tab_lp {
	struct isl_ctx  *ctx;
	struct isl_vec  *row;
	struct isl_tab  *tab;
	struct isl_tab_undo	**stack;
	isl_int		*obj;
	isl_int		 opt;
	isl_int		 opt_denom;
	isl_int		 tmp;
	isl_int		 tmp2;
	int	         neq;
	unsigned	 dim;
	/* number of constraints in initial product tableau */
	int		 con_offset;
	/* objective function has fixed or no integer value */
	int		 is_fixed;
};

static struct tab_lp *init_lp(struct isl_tab *tab);
static void set_lp_obj(struct tab_lp *lp, isl_int *row, int dim);
static int solve_lp(struct tab_lp *lp);
static void get_obj_val(struct tab_lp* lp, mpq_t *F);
static void delete_lp(struct tab_lp *lp);
static int add_lp_row(struct tab_lp *lp, isl_int *row, int dim);
static void get_alpha(struct tab_lp* lp, int row, mpq_t *alpha);
static int del_lp_row(struct tab_lp *lp) WARN_UNUSED;
static int cut_lp_to_hyperplane(struct tab_lp *lp, isl_int *row);

#define GBR_LP			    	    struct tab_lp
#define GBR_type		    	    mpq_t
#define GBR_init(v)		    	    mpq_init(v)
#define GBR_clear(v)		    	    mpq_clear(v)
#define GBR_set(a,b)			    mpq_set(a,b)
#define GBR_set_ui(a,b)			    mpq_set_ui(a,b,1)
#define GBR_mul(a,b,c)			    mpq_mul(a,b,c)
#define GBR_lt(a,b)			    (mpq_cmp(a,b) < 0)
#define GBR_is_zero(a)			    (mpq_sgn(a) == 0)
#define GBR_floor(a,b)			    mpz_fdiv_q(a,mpq_numref(b),mpq_denref(b))
#define GBR_ceil(a,b)			    mpz_cdiv_q(a,mpq_numref(b),mpq_denref(b))
#define GBR_lp_init(P)		    	    init_lp(P)
#define GBR_lp_set_obj(lp, obj, dim)	    set_lp_obj(lp, obj, dim)
#define GBR_lp_solve(lp)		    solve_lp(lp)
#define GBR_lp_get_obj_val(lp, F)	    get_obj_val(lp, F)
#define GBR_lp_delete(lp)		    delete_lp(lp)
#define GBR_lp_next_row(lp)		    lp->neq
#define GBR_lp_add_row(lp, row, dim)	    add_lp_row(lp, row, dim)
#define GBR_lp_get_alpha(lp, row, alpha)    get_alpha(lp, row, alpha)
#define GBR_lp_del_row(lp)		    del_lp_row(lp)
#define GBR_lp_is_fixed(lp)		    (lp)->is_fixed
#define GBR_lp_cut(lp, obj)	    	    cut_lp_to_hyperplane(lp, obj)
#include "basis_reduction_templ.c"

/* Set up a tableau for the Cartesian product of bset with itself.
 * This could be optimized by first setting up a tableau for bset
 * and then performing the Cartesian product on the tableau.
 */
static struct isl_tab *gbr_tab(struct isl_tab *tab, struct isl_vec *row)
{
	unsigned dim;
	struct isl_tab *prod;

	if (!tab || !row)
		return NULL;

	dim = tab->n_var;
	prod = isl_tab_product(tab, tab);
	if (isl_tab_extend_cons(prod, 3 * dim + 1) < 0) {
		isl_tab_free(prod);
		return NULL;
	}
	return prod;
}

static struct tab_lp *init_lp(struct isl_tab *tab)
{
	struct tab_lp *lp = NULL;

	if (!tab)
		return NULL;

	lp = isl_calloc_type(tab->mat->ctx, struct tab_lp);
	if (!lp)
		return NULL;

	isl_int_init(lp->opt);
	isl_int_init(lp->opt_denom);
	isl_int_init(lp->tmp);
	isl_int_init(lp->tmp2);

	lp->dim = tab->n_var;

	lp->ctx = tab->mat->ctx;
	isl_ctx_ref(lp->ctx);

	lp->stack = isl_alloc_array(lp->ctx, struct isl_tab_undo *, lp->dim);

	lp->row = isl_vec_alloc(lp->ctx, 1 + 2 * lp->dim);
	if (!lp->row)
		goto error;
	lp->tab = gbr_tab(tab, lp->row);
	if (!lp->tab)
		goto error;
	lp->con_offset = lp->tab->n_con;
	lp->obj = NULL;
	lp->neq = 0;

	return lp;
error:
	delete_lp(lp);
	return NULL;
}

static void set_lp_obj(struct tab_lp *lp, isl_int *row, int dim)
{
	lp->obj = row;
}

static int solve_lp(struct tab_lp *lp)
{
	enum isl_lp_result res;
	unsigned flags = 0;

	lp->is_fixed = 0;

	isl_int_set_si(lp->row->el[0], 0);
	isl_seq_cpy(lp->row->el + 1, lp->obj, lp->dim);
	isl_seq_neg(lp->row->el + 1 + lp->dim, lp->obj, lp->dim);
	if (lp->neq)
		flags = ISL_TAB_SAVE_DUAL;
	res = isl_tab_min(lp->tab, lp->row->el, lp->ctx->one,
			  &lp->opt, &lp->opt_denom, flags);
	isl_int_mul_ui(lp->opt_denom, lp->opt_denom, 2);
	if (isl_int_abs_lt(lp->opt, lp->opt_denom)) {
		struct isl_vec *sample = isl_tab_get_sample_value(lp->tab);
		if (!sample)
			return -1;
		isl_seq_inner_product(lp->obj, sample->el + 1, lp->dim, &lp->tmp);
		isl_seq_inner_product(lp->obj, sample->el + 1 + lp->dim, lp->dim, &lp->tmp2);
		isl_int_cdiv_q(lp->tmp, lp->tmp, sample->el[0]);
		isl_int_fdiv_q(lp->tmp2, lp->tmp2, sample->el[0]);
		if (isl_int_ge(lp->tmp, lp->tmp2))
			lp->is_fixed = 1;
		isl_vec_free(sample);
	}
	isl_int_divexact_ui(lp->opt_denom, lp->opt_denom, 2);
	if (res != isl_lp_ok)
		return -1;
	return 0;
}

/* The current objective function has a fixed (or no) integer value.
 * Cut the tableau to the hyperplane that fixes this value in
 * both halves of the tableau.
 * Return 1 if the resulting tableau is empty.
 */
static int cut_lp_to_hyperplane(struct tab_lp *lp, isl_int *row)
{
	enum isl_lp_result res;

	isl_int_set_si(lp->row->el[0], 0);
	isl_seq_cpy(lp->row->el + 1, row, lp->dim);
	isl_seq_clr(lp->row->el + 1 + lp->dim, lp->dim);
	res = isl_tab_min(lp->tab, lp->row->el, lp->ctx->one,
			  &lp->tmp, NULL, 0);
	if (res != isl_lp_ok)
		return -1;

	isl_int_neg(lp->row->el[0], lp->tmp);
	if (isl_tab_add_eq(lp->tab, lp->row->el) < 0)
		return -1;

	isl_seq_cpy(lp->row->el + 1 + lp->dim, row, lp->dim);
	isl_seq_clr(lp->row->el + 1, lp->dim);
	if (isl_tab_add_eq(lp->tab, lp->row->el) < 0)
		return -1;

	lp->con_offset += 2;

	return lp->tab->empty;
}

static void get_obj_val(struct tab_lp* lp, mpq_t *F)
{
	isl_int_neg(mpq_numref(*F), lp->opt);
	isl_int_set(mpq_denref(*F), lp->opt_denom);
}

static void delete_lp(struct tab_lp *lp)
{
	if (!lp)
		return;

	isl_int_clear(lp->opt);
	isl_int_clear(lp->opt_denom);
	isl_int_clear(lp->tmp);
	isl_int_clear(lp->tmp2);
	isl_vec_free(lp->row);
	free(lp->stack);
	isl_tab_free(lp->tab);
	isl_ctx_deref(lp->ctx);
	free(lp);
}

static int add_lp_row(struct tab_lp *lp, isl_int *row, int dim)
{
	lp->stack[lp->neq] = isl_tab_snap(lp->tab);

	isl_int_set_si(lp->row->el[0], 0);
	isl_seq_cpy(lp->row->el + 1, row, lp->dim);
	isl_seq_neg(lp->row->el + 1 + lp->dim, row, lp->dim);

	if (isl_tab_add_valid_eq(lp->tab, lp->row->el) < 0)
		return -1;

	return lp->neq++;
}

static void get_alpha(struct tab_lp* lp, int row, mpq_t *alpha)
{
	row += lp->con_offset;
	isl_int_neg(mpq_numref(*alpha), lp->tab->dual->el[1 + row]);
	isl_int_set(mpq_denref(*alpha), lp->tab->dual->el[0]);
}

static int del_lp_row(struct tab_lp *lp)
{
	lp->neq--;
	return isl_tab_rollback(lp->tab, lp->stack[lp->neq]);
}
