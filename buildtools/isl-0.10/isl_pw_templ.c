#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)
#define xS(TYPE,NAME) struct TYPE ## _ ## NAME
#define S(TYPE,NAME) xS(TYPE,NAME)

#ifdef HAS_TYPE
__isl_give PW *FN(PW,alloc_size)(__isl_take isl_space *dim,
	enum isl_fold type, int n)
#else
__isl_give PW *FN(PW,alloc_size)(__isl_take isl_space *dim, int n)
#endif
{
	isl_ctx *ctx;
	struct PW *pw;

	if (!dim)
		return NULL;
	ctx = isl_space_get_ctx(dim);
	isl_assert(ctx, n >= 0, goto error);
	pw = isl_alloc(ctx, struct PW,
			sizeof(struct PW) + (n - 1) * sizeof(S(PW,piece)));
	if (!pw)
		goto error;

	pw->ref = 1;
#ifdef HAS_TYPE
	pw->type = type;
#endif
	pw->size = n;
	pw->n = 0;
	pw->dim = dim;
	return pw;
error:
	isl_space_free(dim);
	return NULL;
}

#ifdef HAS_TYPE
__isl_give PW *FN(PW,ZERO)(__isl_take isl_space *dim, enum isl_fold type)
{
	return FN(PW,alloc_size)(dim, type, 0);
}
#else
__isl_give PW *FN(PW,ZERO)(__isl_take isl_space *dim)
{
	return FN(PW,alloc_size)(dim, 0);
}
#endif

__isl_give PW *FN(PW,add_piece)(__isl_take PW *pw,
	__isl_take isl_set *set, __isl_take EL *el)
{
	isl_ctx *ctx;
	isl_space *el_dim = NULL;

	if (!pw || !set || !el)
		goto error;

	if (isl_set_plain_is_empty(set) || FN(EL,EL_IS_ZERO)(el)) {
		isl_set_free(set);
		FN(EL,free)(el);
		return pw;
	}

	ctx = isl_set_get_ctx(set);
#ifdef HAS_TYPE
	if (pw->type != el->type)
		isl_die(ctx, isl_error_invalid, "fold types don't match",
			goto error);
#endif
	el_dim = FN(EL,get_space(el));
	isl_assert(ctx, isl_space_is_equal(pw->dim, el_dim), goto error);
	isl_assert(ctx, pw->n < pw->size, goto error);

	pw->p[pw->n].set = set;
	pw->p[pw->n].FIELD = el;
	pw->n++;
	
	isl_space_free(el_dim);
	return pw;
error:
	isl_space_free(el_dim);
	FN(PW,free)(pw);
	isl_set_free(set);
	FN(EL,free)(el);
	return NULL;
}

#ifdef HAS_TYPE
__isl_give PW *FN(PW,alloc)(enum isl_fold type,
	__isl_take isl_set *set, __isl_take EL *el)
#else
__isl_give PW *FN(PW,alloc)(__isl_take isl_set *set, __isl_take EL *el)
#endif
{
	PW *pw;

	if (!set || !el)
		goto error;

#ifdef HAS_TYPE
	pw = FN(PW,alloc_size)(FN(EL,get_space)(el), type, 1);
#else
	pw = FN(PW,alloc_size)(FN(EL,get_space)(el), 1);
#endif

	return FN(PW,add_piece)(pw, set, el);
error:
	isl_set_free(set);
	FN(EL,free)(el);
	return NULL;
}

__isl_give PW *FN(PW,dup)(__isl_keep PW *pw)
{
	int i;
	PW *dup;

	if (!pw)
		return NULL;

#ifdef HAS_TYPE
	dup = FN(PW,alloc_size)(isl_space_copy(pw->dim), pw->type, pw->n);
#else
	dup = FN(PW,alloc_size)(isl_space_copy(pw->dim), pw->n);
#endif
	if (!dup)
		return NULL;

	for (i = 0; i < pw->n; ++i)
		dup = FN(PW,add_piece)(dup, isl_set_copy(pw->p[i].set),
					    FN(EL,copy)(pw->p[i].FIELD));

	return dup;
}

__isl_give PW *FN(PW,cow)(__isl_take PW *pw)
{
	if (!pw)
		return NULL;

	if (pw->ref == 1)
		return pw;
	pw->ref--;
	return FN(PW,dup)(pw);
}

__isl_give PW *FN(PW,copy)(__isl_keep PW *pw)
{
	if (!pw)
		return NULL;

	pw->ref++;
	return pw;
}

void *FN(PW,free)(__isl_take PW *pw)
{
	int i;

	if (!pw)
		return NULL;
	if (--pw->ref > 0)
		return NULL;

	for (i = 0; i < pw->n; ++i) {
		isl_set_free(pw->p[i].set);
		FN(EL,free)(pw->p[i].FIELD);
	}
	isl_space_free(pw->dim);
	free(pw);

	return NULL;
}

const char *FN(PW,get_dim_name)(__isl_keep PW *pw, enum isl_dim_type type,
	unsigned pos)
{
	return pw ? isl_space_get_dim_name(pw->dim, type, pos) : NULL;
}

int FN(PW,has_dim_id)(__isl_keep PW *pw, enum isl_dim_type type, unsigned pos)
{
	return pw ? isl_space_has_dim_id(pw->dim, type, pos) : -1;
}

__isl_give isl_id *FN(PW,get_dim_id)(__isl_keep PW *pw, enum isl_dim_type type,
	unsigned pos)
{
	return pw ? isl_space_get_dim_id(pw->dim, type, pos) : NULL;
}

const char *FN(PW,get_tuple_name)(__isl_keep PW *pw, enum isl_dim_type type)
{
	return pw ? isl_space_get_tuple_name(pw->dim, type) : NULL;
}

int FN(PW,has_tuple_id)(__isl_keep PW *pw, enum isl_dim_type type)
{
	return pw ? isl_space_has_tuple_id(pw->dim, type) : -1;
}

__isl_give isl_id *FN(PW,get_tuple_id)(__isl_keep PW *pw, enum isl_dim_type type)
{
	return pw ? isl_space_get_tuple_id(pw->dim, type) : NULL;
}

int FN(PW,IS_ZERO)(__isl_keep PW *pw)
{
	if (!pw)
		return -1;

	return pw->n == 0;
}

#ifndef NO_REALIGN
__isl_give PW *FN(PW,realign_domain)(__isl_take PW *pw,
	__isl_take isl_reordering *exp)
{
	int i;

	pw = FN(PW,cow)(pw);
	if (!pw || !exp)
		return NULL;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_realign(pw->p[i].set,
						    isl_reordering_copy(exp));
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,realign_domain)(pw->p[i].FIELD,
						    isl_reordering_copy(exp));
		if (!pw->p[i].FIELD)
			goto error;
	}

	pw = FN(PW,reset_domain_space)(pw, isl_space_copy(exp->dim));

	isl_reordering_free(exp);
	return pw;
error:
	isl_reordering_free(exp);
	FN(PW,free)(pw);
	return NULL;
}

/* Align the parameters of "pw" to those of "model".
 */
__isl_give PW *FN(PW,align_params)(__isl_take PW *pw, __isl_take isl_space *model)
{
	isl_ctx *ctx;

	if (!pw || !model)
		goto error;

	ctx = isl_space_get_ctx(model);
	if (!isl_space_has_named_params(model))
		isl_die(ctx, isl_error_invalid,
			"model has unnamed parameters", goto error);
	if (!isl_space_has_named_params(pw->dim))
		isl_die(ctx, isl_error_invalid,
			"input has unnamed parameters", goto error);
	if (!isl_space_match(pw->dim, isl_dim_param, model, isl_dim_param)) {
		isl_reordering *exp;

		model = isl_space_drop_dims(model, isl_dim_in,
					0, isl_space_dim(model, isl_dim_in));
		model = isl_space_drop_dims(model, isl_dim_out,
					0, isl_space_dim(model, isl_dim_out));
		exp = isl_parameter_alignment_reordering(pw->dim, model);
		exp = isl_reordering_extend_space(exp,
					FN(PW,get_domain_space)(pw));
		pw = FN(PW,realign_domain)(pw, exp);
	}

	isl_space_free(model);
	return pw;
error:
	isl_space_free(model);
	FN(PW,free)(pw);
	return NULL;
}

static __isl_give PW *FN(PW,align_params_pw_pw_and)(__isl_take PW *pw1,
	__isl_take PW *pw2,
	__isl_give PW *(*fn)(__isl_take PW *pw1, __isl_take PW *pw2))
{
	isl_ctx *ctx;

	if (!pw1 || !pw2)
		goto error;
	if (isl_space_match(pw1->dim, isl_dim_param, pw2->dim, isl_dim_param))
		return fn(pw1, pw2);
	ctx = FN(PW,get_ctx)(pw1);
	if (!isl_space_has_named_params(pw1->dim) ||
	    !isl_space_has_named_params(pw2->dim))
		isl_die(ctx, isl_error_invalid,
			"unaligned unnamed parameters", goto error);
	pw1 = FN(PW,align_params)(pw1, FN(PW,get_space)(pw2));
	pw2 = FN(PW,align_params)(pw2, FN(PW,get_space)(pw1));
	return fn(pw1, pw2);
error:
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return NULL;
}

static __isl_give PW *FN(PW,align_params_pw_set_and)(__isl_take PW *pw,
	__isl_take isl_set *set,
	__isl_give PW *(*fn)(__isl_take PW *pw, __isl_take isl_set *set))
{
	isl_ctx *ctx;

	if (!pw || !set)
		goto error;
	if (isl_space_match(pw->dim, isl_dim_param, set->dim, isl_dim_param))
		return fn(pw, set);
	ctx = FN(PW,get_ctx)(pw);
	if (!isl_space_has_named_params(pw->dim) ||
	    !isl_space_has_named_params(set->dim))
		isl_die(ctx, isl_error_invalid,
			"unaligned unnamed parameters", goto error);
	pw = FN(PW,align_params)(pw, isl_set_get_space(set));
	set = isl_set_align_params(set, FN(PW,get_space)(pw));
	return fn(pw, set);
error:
	FN(PW,free)(pw);
	isl_set_free(set);
	return NULL;
}
#endif

static __isl_give PW *FN(PW,union_add_aligned)(__isl_take PW *pw1,
	__isl_take PW *pw2)
{
	int i, j, n;
	struct PW *res;
	isl_ctx *ctx;
	isl_set *set;

	if (!pw1 || !pw2)
		goto error;

	ctx = isl_space_get_ctx(pw1->dim);
#ifdef HAS_TYPE
	if (pw1->type != pw2->type)
		isl_die(ctx, isl_error_invalid,
			"fold types don't match", goto error);
#endif
	isl_assert(ctx, isl_space_is_equal(pw1->dim, pw2->dim), goto error);

	if (FN(PW,IS_ZERO)(pw1)) {
		FN(PW,free)(pw1);
		return pw2;
	}

	if (FN(PW,IS_ZERO)(pw2)) {
		FN(PW,free)(pw2);
		return pw1;
	}

	n = (pw1->n + 1) * (pw2->n + 1);
#ifdef HAS_TYPE
	res = FN(PW,alloc_size)(isl_space_copy(pw1->dim), pw1->type, n);
#else
	res = FN(PW,alloc_size)(isl_space_copy(pw1->dim), n);
#endif

	for (i = 0; i < pw1->n; ++i) {
		set = isl_set_copy(pw1->p[i].set);
		for (j = 0; j < pw2->n; ++j) {
			struct isl_set *common;
			EL *sum;
			common = isl_set_intersect(isl_set_copy(pw1->p[i].set),
						isl_set_copy(pw2->p[j].set));
			if (isl_set_plain_is_empty(common)) {
				isl_set_free(common);
				continue;
			}
			set = isl_set_subtract(set,
					isl_set_copy(pw2->p[j].set));

			sum = FN(EL,add_on_domain)(common,
						   FN(EL,copy)(pw1->p[i].FIELD),
						   FN(EL,copy)(pw2->p[j].FIELD));

			res = FN(PW,add_piece)(res, common, sum);
		}
		res = FN(PW,add_piece)(res, set, FN(EL,copy)(pw1->p[i].FIELD));
	}

	for (j = 0; j < pw2->n; ++j) {
		set = isl_set_copy(pw2->p[j].set);
		for (i = 0; i < pw1->n; ++i)
			set = isl_set_subtract(set,
					isl_set_copy(pw1->p[i].set));
		res = FN(PW,add_piece)(res, set, FN(EL,copy)(pw2->p[j].FIELD));
	}

	FN(PW,free)(pw1);
	FN(PW,free)(pw2);

	return res;
error:
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return NULL;
}

/* Private version of "union_add".  For isl_pw_qpolynomial and
 * isl_pw_qpolynomial_fold, we prefer to simply call it "add".
 */
static __isl_give PW *FN(PW,union_add_)(__isl_take PW *pw1, __isl_take PW *pw2)
{
	return FN(PW,align_params_pw_pw_and)(pw1, pw2,
						&FN(PW,union_add_aligned));
}

/* Make sure "pw" has room for at least "n" more pieces.
 *
 * If there is only one reference to pw, we extend it in place.
 * Otherwise, we create a new PW and copy the pieces.
 */
static __isl_give PW *FN(PW,grow)(__isl_take PW *pw, int n)
{
	int i;
	isl_ctx *ctx;
	PW *res;

	if (!pw)
		return NULL;
	if (pw->n + n <= pw->size)
		return pw;
	ctx = FN(PW,get_ctx)(pw);
	n += pw->n;
	if (pw->ref == 1) {
		res = isl_realloc(ctx, pw, struct PW,
			    sizeof(struct PW) + (n - 1) * sizeof(S(PW,piece)));
		if (!res)
			return FN(PW,free)(pw);
		res->size = n;
		return res;
	}
#ifdef HAS_TYPE
	res = FN(PW,alloc_size)(isl_space_copy(pw->dim), pw->type, n);
#else
	res = FN(PW,alloc_size)(isl_space_copy(pw->dim), n);
#endif
	if (!res)
		return FN(PW,free)(pw);
	for (i = 0; i < pw->n; ++i)
		res = FN(PW,add_piece)(res, isl_set_copy(pw->p[i].set),
					    FN(EL,copy)(pw->p[i].FIELD));
	FN(PW,free)(pw);
	return res;
}

static __isl_give PW *FN(PW,add_disjoint_aligned)(__isl_take PW *pw1,
	__isl_take PW *pw2)
{
	int i;
	isl_ctx *ctx;

	if (!pw1 || !pw2)
		goto error;

	if (pw1->size < pw1->n + pw2->n && pw1->n < pw2->n)
		return FN(PW,add_disjoint_aligned)(pw2, pw1);

	ctx = isl_space_get_ctx(pw1->dim);
#ifdef HAS_TYPE
	if (pw1->type != pw2->type)
		isl_die(ctx, isl_error_invalid,
			"fold types don't match", goto error);
#endif
	isl_assert(ctx, isl_space_is_equal(pw1->dim, pw2->dim), goto error);

	if (FN(PW,IS_ZERO)(pw1)) {
		FN(PW,free)(pw1);
		return pw2;
	}

	if (FN(PW,IS_ZERO)(pw2)) {
		FN(PW,free)(pw2);
		return pw1;
	}

	pw1 = FN(PW,grow)(pw1, pw2->n);
	if (!pw1)
		goto error;

	for (i = 0; i < pw2->n; ++i)
		pw1 = FN(PW,add_piece)(pw1,
				isl_set_copy(pw2->p[i].set),
				FN(EL,copy)(pw2->p[i].FIELD));

	FN(PW,free)(pw2);

	return pw1;
error:
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return NULL;
}

__isl_give PW *FN(PW,add_disjoint)(__isl_take PW *pw1, __isl_take PW *pw2)
{
	return FN(PW,align_params_pw_pw_and)(pw1, pw2,
						&FN(PW,add_disjoint_aligned));
}

/* This function is currently only used from isl_aff.c
 */
static __isl_give PW *FN(PW,on_shared_domain_in)(__isl_take PW *pw1,
	__isl_take PW *pw2, __isl_take isl_space *space,
	__isl_give EL *(*fn)(__isl_take EL *el1, __isl_take EL *el2))
	__attribute__ ((unused));

/* Apply "fn" to pairs of elements from pw1 and pw2 on shared domains.
 * The result of "fn" (and therefore also of this function) lives in "space".
 */
static __isl_give PW *FN(PW,on_shared_domain_in)(__isl_take PW *pw1,
	__isl_take PW *pw2, __isl_take isl_space *space,
	__isl_give EL *(*fn)(__isl_take EL *el1, __isl_take EL *el2))
{
	int i, j, n;
	PW *res = NULL;

	if (!pw1 || !pw2)
		goto error;

	n = pw1->n * pw2->n;
#ifdef HAS_TYPE
	res = FN(PW,alloc_size)(space, pw1->type, n);
#else
	res = FN(PW,alloc_size)(space, n);
#endif

	for (i = 0; i < pw1->n; ++i) {
		for (j = 0; j < pw2->n; ++j) {
			isl_set *common;
			EL *res_ij;
			int empty;

			common = isl_set_intersect(
					isl_set_copy(pw1->p[i].set),
					isl_set_copy(pw2->p[j].set));
			empty = isl_set_plain_is_empty(common);
			if (empty < 0 || empty) {
				isl_set_free(common);
				if (empty < 0)
					goto error;
				continue;
			}

			res_ij = fn(FN(EL,copy)(pw1->p[i].FIELD),
				    FN(EL,copy)(pw2->p[j].FIELD));
			res_ij = FN(EL,gist)(res_ij, isl_set_copy(common));

			res = FN(PW,add_piece)(res, common, res_ij);
		}
	}

	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return res;
error:
	isl_space_free(space);
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	FN(PW,free)(res);
	return NULL;
}

/* This function is currently only used from isl_aff.c
 */
static __isl_give PW *FN(PW,on_shared_domain)(__isl_take PW *pw1,
	__isl_take PW *pw2,
	__isl_give EL *(*fn)(__isl_take EL *el1, __isl_take EL *el2))
	__attribute__ ((unused));

/* Apply "fn" to pairs of elements from pw1 and pw2 on shared domains.
 * The result of "fn" is assumed to live in the same space as "pw1" and "pw2".
 */
static __isl_give PW *FN(PW,on_shared_domain)(__isl_take PW *pw1,
	__isl_take PW *pw2,
	__isl_give EL *(*fn)(__isl_take EL *el1, __isl_take EL *el2))
{
	isl_space *space;

	if (!pw1 || !pw2)
		goto error;

	space = isl_space_copy(pw1->dim);
	return FN(PW,on_shared_domain_in)(pw1, pw2, space, fn);
error:
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return NULL;
}

#ifndef NO_NEG
__isl_give PW *FN(PW,neg)(__isl_take PW *pw)
{
	int i;

	if (!pw)
		return NULL;

	if (FN(PW,IS_ZERO)(pw))
		return pw;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].FIELD = FN(EL,neg)(pw->p[i].FIELD);
		if (!pw->p[i].FIELD)
			return FN(PW,free)(pw);
	}

	return pw;
}

__isl_give PW *FN(PW,sub)(__isl_take PW *pw1, __isl_take PW *pw2)
{
	return FN(PW,add)(pw1, FN(PW,neg)(pw2));
}
#endif

#ifndef NO_EVAL
__isl_give isl_qpolynomial *FN(PW,eval)(__isl_take PW *pw,
	__isl_take isl_point *pnt)
{
	int i;
	int found = 0;
	isl_ctx *ctx;
	isl_space *pnt_dim = NULL;
	isl_qpolynomial *qp;

	if (!pw || !pnt)
		goto error;
	ctx = isl_point_get_ctx(pnt);
	pnt_dim = isl_point_get_space(pnt);
	isl_assert(ctx, isl_space_is_domain(pnt_dim, pw->dim), goto error);

	for (i = 0; i < pw->n; ++i) {
		found = isl_set_contains_point(pw->p[i].set, pnt);
		if (found < 0)
			goto error;
		if (found)
			break;
	}
	if (found)
		qp = FN(EL,eval)(FN(EL,copy)(pw->p[i].FIELD),
					    isl_point_copy(pnt));
	else
		qp = isl_qpolynomial_zero_on_domain(FN(PW,get_domain_space)(pw));
	FN(PW,free)(pw);
	isl_space_free(pnt_dim);
	isl_point_free(pnt);
	return qp;
error:
	FN(PW,free)(pw);
	isl_space_free(pnt_dim);
	isl_point_free(pnt);
	return NULL;
}
#endif

__isl_give isl_set *FN(PW,domain)(__isl_take PW *pw)
{
	int i;
	isl_set *dom;

	if (!pw)
		return NULL;

	dom = isl_set_empty(FN(PW,get_domain_space)(pw));
	for (i = 0; i < pw->n; ++i)
		dom = isl_set_union_disjoint(dom, isl_set_copy(pw->p[i].set));

	FN(PW,free)(pw);

	return dom;
}

/* Restrict the domain of "pw" by combining each cell
 * with "set" through a call to "fn", where "fn" may be
 * isl_set_intersect or isl_set_intersect_params.
 */
static __isl_give PW *FN(PW,intersect_aligned)(__isl_take PW *pw,
	__isl_take isl_set *set,
	__isl_give isl_set *(*fn)(__isl_take isl_set *set1,
				    __isl_take isl_set *set2))
{
	int i;

	if (!pw || !set)
		goto error;

	if (pw->n == 0) {
		isl_set_free(set);
		return pw;
	}

	pw = FN(PW,cow)(pw);
	if (!pw)
		goto error;

	for (i = pw->n - 1; i >= 0; --i) {
		isl_basic_set *aff;
		pw->p[i].set = fn(pw->p[i].set, isl_set_copy(set));
		if (!pw->p[i].set)
			goto error;
		aff = isl_set_affine_hull(isl_set_copy(pw->p[i].set));
		pw->p[i].FIELD = FN(EL,substitute_equalities)(pw->p[i].FIELD,
								aff);
		if (!pw->p[i].FIELD)
			goto error;
		if (isl_set_plain_is_empty(pw->p[i].set)) {
			isl_set_free(pw->p[i].set);
			FN(EL,free)(pw->p[i].FIELD);
			if (i != pw->n - 1)
				pw->p[i] = pw->p[pw->n - 1];
			pw->n--;
		}
	}
	
	isl_set_free(set);
	return pw;
error:
	isl_set_free(set);
	FN(PW,free)(pw);
	return NULL;
}

static __isl_give PW *FN(PW,intersect_domain_aligned)(__isl_take PW *pw,
	__isl_take isl_set *set)
{
	return FN(PW,intersect_aligned)(pw, set, &isl_set_intersect);
}

__isl_give PW *FN(PW,intersect_domain)(__isl_take PW *pw,
	__isl_take isl_set *context)
{
	return FN(PW,align_params_pw_set_and)(pw, context,
					&FN(PW,intersect_domain_aligned));
}

static __isl_give PW *FN(PW,intersect_params_aligned)(__isl_take PW *pw,
	__isl_take isl_set *set)
{
	return FN(PW,intersect_aligned)(pw, set, &isl_set_intersect_params);
}

/* Intersect the domain of "pw" with the parameter domain "context".
 */
__isl_give PW *FN(PW,intersect_params)(__isl_take PW *pw,
	__isl_take isl_set *context)
{
	return FN(PW,align_params_pw_set_and)(pw, context,
					&FN(PW,intersect_params_aligned));
}

static __isl_give PW *FN(PW,gist_aligned)(__isl_take PW *pw,
	__isl_take isl_set *context,
	__isl_give EL *(*fn_el)(__isl_take EL *el,
				    __isl_take isl_set *set),
	__isl_give isl_set *(*fn_dom)(__isl_take isl_set *set,
				    __isl_take isl_basic_set *bset))
{
	int i;
	isl_basic_set *hull = NULL;

	if (!pw || !context)
		goto error;

	if (pw->n == 0) {
		isl_set_free(context);
		return pw;
	}

	if (!isl_space_match(pw->dim, isl_dim_param,
				context->dim, isl_dim_param)) {
		pw = FN(PW,align_params)(pw, isl_set_get_space(context));
		context = isl_set_align_params(context, FN(PW,get_space)(pw));
	}

	context = isl_set_compute_divs(context);
	hull = isl_set_simple_hull(isl_set_copy(context));

	pw = FN(PW,cow)(pw);
	if (!pw)
		goto error;

	for (i = pw->n - 1; i >= 0; --i) {
		pw->p[i].set = isl_set_intersect(pw->p[i].set,
						 isl_set_copy(context));
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = fn_el(pw->p[i].FIELD,
					     isl_set_copy(pw->p[i].set));
		pw->p[i].set = fn_dom(pw->p[i].set, isl_basic_set_copy(hull));
		if (!pw->p[i].set)
			goto error;
		if (isl_set_plain_is_empty(pw->p[i].set)) {
			isl_set_free(pw->p[i].set);
			FN(EL,free)(pw->p[i].FIELD);
			if (i != pw->n - 1)
				pw->p[i] = pw->p[pw->n - 1];
			pw->n--;
		}
	}

	isl_basic_set_free(hull);
	isl_set_free(context);

	return pw;
error:
	FN(PW,free)(pw);
	isl_basic_set_free(hull);
	isl_set_free(context);
	return NULL;
}

static __isl_give PW *FN(PW,gist_domain_aligned)(__isl_take PW *pw,
	__isl_take isl_set *set)
{
	return FN(PW,gist_aligned)(pw, set, &FN(EL,gist),
					&isl_set_gist_basic_set);
}

__isl_give PW *FN(PW,gist)(__isl_take PW *pw, __isl_take isl_set *context)
{
	return FN(PW,align_params_pw_set_and)(pw, context,
						&FN(PW,gist_domain_aligned));
}

static __isl_give PW *FN(PW,gist_params_aligned)(__isl_take PW *pw,
	__isl_take isl_set *set)
{
	return FN(PW,gist_aligned)(pw, set, &FN(EL,gist_params),
					&isl_set_gist_params_basic_set);
}

__isl_give PW *FN(PW,gist_params)(__isl_take PW *pw,
	__isl_take isl_set *context)
{
	return FN(PW,align_params_pw_set_and)(pw, context,
						&FN(PW,gist_params_aligned));
}

__isl_give PW *FN(PW,coalesce)(__isl_take PW *pw)
{
	int i, j;

	if (!pw)
		return NULL;
	if (pw->n == 0)
		return pw;

	for (i = pw->n - 1; i >= 0; --i) {
		for (j = i - 1; j >= 0; --j) {
			if (!FN(EL,plain_is_equal)(pw->p[i].FIELD,
							pw->p[j].FIELD))
				continue;
			pw->p[j].set = isl_set_union(pw->p[j].set,
							pw->p[i].set);
			FN(EL,free)(pw->p[i].FIELD);
			if (i != pw->n - 1)
				pw->p[i] = pw->p[pw->n - 1];
			pw->n--;
			break;
		}
		if (j >= 0)
			continue;
		pw->p[i].set = isl_set_coalesce(pw->p[i].set);
		if (!pw->p[i].set)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

isl_ctx *FN(PW,get_ctx)(__isl_keep PW *pw)
{
	return pw ? isl_space_get_ctx(pw->dim) : NULL;
}

#ifndef NO_INVOLVES_DIMS
int FN(PW,involves_dims)(__isl_keep PW *pw, enum isl_dim_type type,
	unsigned first, unsigned n)
{
	int i;
	enum isl_dim_type set_type;

	if (!pw)
		return -1;
	if (pw->n == 0 || n == 0)
		return 0;

	set_type = type == isl_dim_in ? isl_dim_set : type;

	for (i = 0; i < pw->n; ++i) {
		int involves = FN(EL,involves_dims)(pw->p[i].FIELD,
							type, first, n);
		if (involves < 0 || involves)
			return involves;
		involves = isl_set_involves_dims(pw->p[i].set,
							set_type, first, n);
		if (involves < 0 || involves)
			return involves;
	}
	return 0;
}
#endif

__isl_give PW *FN(PW,set_dim_name)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned pos, const char *s)
{
	int i;
	enum isl_dim_type set_type;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;

	set_type = type == isl_dim_in ? isl_dim_set : type;

	pw->dim = isl_space_set_dim_name(pw->dim, type, pos, s);
	if (!pw->dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_set_dim_name(pw->p[i].set,
							set_type, pos, s);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,set_dim_name)(pw->p[i].FIELD, type, pos, s);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

#ifndef NO_DROP_DIMS
__isl_give PW *FN(PW,drop_dims)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;
	enum isl_dim_type set_type;

	if (!pw)
		return NULL;
	if (n == 0 && !isl_space_get_tuple_name(pw->dim, type))
		return pw;

	set_type = type == isl_dim_in ? isl_dim_set : type;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	pw->dim = isl_space_drop_dims(pw->dim, type, first, n);
	if (!pw->dim)
		goto error;
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_drop(pw->p[i].set, set_type, first, n);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,drop_dims)(pw->p[i].FIELD, type, first, n);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

/* This function is very similar to drop_dims.
 * The only difference is that the cells may still involve
 * the specified dimensions.  They are removed using
 * isl_set_project_out instead of isl_set_drop.
 */
__isl_give PW *FN(PW,project_out)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;
	enum isl_dim_type set_type;

	if (!pw)
		return NULL;
	if (n == 0 && !isl_space_get_tuple_name(pw->dim, type))
		return pw;

	set_type = type == isl_dim_in ? isl_dim_set : type;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	pw->dim = isl_space_drop_dims(pw->dim, type, first, n);
	if (!pw->dim)
		goto error;
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_project_out(pw->p[i].set,
							set_type, first, n);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,drop_dims)(pw->p[i].FIELD, type, first, n);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

/* Project the domain of pw onto its parameter space.
 */
__isl_give PW *FN(PW,project_domain_on_params)(__isl_take PW *pw)
{
	isl_space *space;
	unsigned n;

	n = FN(PW,dim)(pw, isl_dim_in);
	pw = FN(PW,project_out)(pw, isl_dim_in, 0, n);
	space = FN(PW,get_domain_space)(pw);
	space = isl_space_params(space);
	pw = FN(PW,reset_domain_space)(pw, space);
	return pw;
}
#endif

#ifndef NO_INSERT_DIMS
__isl_give PW *FN(PW,insert_dims)(__isl_take PW *pw, enum isl_dim_type type,
	unsigned first, unsigned n)
{
	int i;
	enum isl_dim_type set_type;

	if (!pw)
		return NULL;
	if (n == 0 && !isl_space_is_named_or_nested(pw->dim, type))
		return pw;

	set_type = type == isl_dim_in ? isl_dim_set : type;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;

	pw->dim = isl_space_insert_dims(pw->dim, type, first, n);
	if (!pw->dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_insert_dims(pw->p[i].set,
							    set_type, first, n);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,insert_dims)(pw->p[i].FIELD,
								type, first, n);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}
#endif

__isl_give PW *FN(PW,fix_dim)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned pos, isl_int v)
{
	int i;

	if (!pw)
		return NULL;

	if (type == isl_dim_in)
		type = isl_dim_set;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_fix(pw->p[i].set, type, pos, v);
		if (!pw->p[i].set)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

unsigned FN(PW,dim)(__isl_keep PW *pw, enum isl_dim_type type)
{
	return pw ? isl_space_dim(pw->dim, type) : 0;
}

__isl_give PW *FN(PW,split_dims)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned first, unsigned n)
{
	int i;

	if (!pw)
		return NULL;
	if (n == 0)
		return pw;

	if (type == isl_dim_in)
		type = isl_dim_set;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	if (!pw->dim)
		goto error;
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_split_dims(pw->p[i].set, type, first, n);
		if (!pw->p[i].set)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

#ifndef NO_OPT
/* Compute the maximal value attained by the piecewise quasipolynomial
 * on its domain or zero if the domain is empty.
 * In the worst case, the domain is scanned completely,
 * so the domain is assumed to be bounded.
 */
__isl_give isl_qpolynomial *FN(PW,opt)(__isl_take PW *pw, int max)
{
	int i;
	isl_qpolynomial *opt;

	if (!pw)
		return NULL;

	if (pw->n == 0) {
		isl_space *dim = isl_space_copy(pw->dim);
		FN(PW,free)(pw);
		return isl_qpolynomial_zero_on_domain(isl_space_domain(dim));
	}

	opt = FN(EL,opt_on_domain)(FN(EL,copy)(pw->p[0].FIELD),
					isl_set_copy(pw->p[0].set), max);
	for (i = 1; i < pw->n; ++i) {
		isl_qpolynomial *opt_i;
		opt_i = FN(EL,opt_on_domain)(FN(EL,copy)(pw->p[i].FIELD),
						isl_set_copy(pw->p[i].set), max);
		if (max)
			opt = isl_qpolynomial_max_cst(opt, opt_i);
		else
			opt = isl_qpolynomial_min_cst(opt, opt_i);
	}

	FN(PW,free)(pw);
	return opt;
}

__isl_give isl_qpolynomial *FN(PW,max)(__isl_take PW *pw)
{
	return FN(PW,opt)(pw, 1);
}

__isl_give isl_qpolynomial *FN(PW,min)(__isl_take PW *pw)
{
	return FN(PW,opt)(pw, 0);
}
#endif

__isl_give isl_space *FN(PW,get_space)(__isl_keep PW *pw)
{
	return pw ? isl_space_copy(pw->dim) : NULL;
}

__isl_give isl_space *FN(PW,get_domain_space)(__isl_keep PW *pw)
{
	return pw ? isl_space_domain(isl_space_copy(pw->dim)) : NULL;
}

#ifndef NO_RESET_DIM
/* Reset the space of "pw".  Since we don't know if the elements
 * represent the spaces themselves or their domains, we pass along
 * both when we call their reset_space_and_domain.
 */
static __isl_give PW *FN(PW,reset_space_and_domain)(__isl_take PW *pw,
	__isl_take isl_space *space, __isl_take isl_space *domain)
{
	int i;

	pw = FN(PW,cow)(pw);
	if (!pw || !space || !domain)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_reset_space(pw->p[i].set,
						 isl_space_copy(domain));
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,reset_space_and_domain)(pw->p[i].FIELD,
			      isl_space_copy(space), isl_space_copy(domain));
		if (!pw->p[i].FIELD)
			goto error;
	}

	isl_space_free(domain);

	isl_space_free(pw->dim);
	pw->dim = space;

	return pw;
error:
	isl_space_free(domain);
	isl_space_free(space);
	FN(PW,free)(pw);
	return NULL;
}

__isl_give PW *FN(PW,reset_domain_space)(__isl_take PW *pw,
	__isl_take isl_space *domain)
{
	isl_space *space;

	space = isl_space_extend_domain_with_range(isl_space_copy(domain),
						   FN(PW,get_space)(pw));
	return FN(PW,reset_space_and_domain)(pw, space, domain);
}

__isl_give PW *FN(PW,reset_space)(__isl_take PW *pw, __isl_take isl_space *dim)
{
	isl_space *domain;

	domain = isl_space_domain(isl_space_copy(dim));
	return FN(PW,reset_space_and_domain)(pw, dim, domain);
}

__isl_give PW *FN(PW,set_tuple_id)(__isl_keep PW *pw, enum isl_dim_type type,
	__isl_take isl_id *id)
{
	isl_space *space;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return isl_id_free(id);

	space = FN(PW,get_space)(pw);
	space = isl_space_set_tuple_id(space, type, id);

	return FN(PW,reset_space)(pw, space);
}

__isl_give PW *FN(PW,set_dim_id)(__isl_take PW *pw,
	enum isl_dim_type type, unsigned pos, __isl_take isl_id *id)
{
	pw = FN(PW,cow)(pw);
	if (!pw)
		return isl_id_free(id);
	pw->dim = isl_space_set_dim_id(pw->dim, type, pos, id);
	return FN(PW,reset_space)(pw, isl_space_copy(pw->dim));
}
#endif

int FN(PW,has_equal_space)(__isl_keep PW *pw1, __isl_keep PW *pw2)
{
	if (!pw1 || !pw2)
		return -1;

	return isl_space_is_equal(pw1->dim, pw2->dim);
}

#ifndef NO_MORPH
__isl_give PW *FN(PW,morph_domain)(__isl_take PW *pw,
	__isl_take isl_morph *morph)
{
	int i;
	isl_ctx *ctx;

	if (!pw || !morph)
		goto error;

	ctx = isl_space_get_ctx(pw->dim);
	isl_assert(ctx, isl_space_is_domain(morph->dom->dim, pw->dim),
		goto error);

	pw = FN(PW,cow)(pw);
	if (!pw)
		goto error;
	pw->dim = isl_space_extend_domain_with_range(
			isl_space_copy(morph->ran->dim), pw->dim);
	if (!pw->dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_morph_set(isl_morph_copy(morph), pw->p[i].set);
		if (!pw->p[i].set)
			goto error;
		pw->p[i].FIELD = FN(EL,morph_domain)(pw->p[i].FIELD,
						isl_morph_copy(morph));
		if (!pw->p[i].FIELD)
			goto error;
	}

	isl_morph_free(morph);

	return pw;
error:
	FN(PW,free)(pw);
	isl_morph_free(morph);
	return NULL;
}
#endif

int FN(PW,n_piece)(__isl_keep PW *pw)
{
	return pw ? pw->n : 0;
}

int FN(PW,foreach_piece)(__isl_keep PW *pw,
	int (*fn)(__isl_take isl_set *set, __isl_take EL *el, void *user),
	void *user)
{
	int i;

	if (!pw)
		return -1;

	for (i = 0; i < pw->n; ++i)
		if (fn(isl_set_copy(pw->p[i].set),
				FN(EL,copy)(pw->p[i].FIELD), user) < 0)
			return -1;

	return 0;
}

#ifndef NO_LIFT
static int any_divs(__isl_keep isl_set *set)
{
	int i;

	if (!set)
		return -1;

	for (i = 0; i < set->n; ++i)
		if (set->p[i]->n_div > 0)
			return 1;

	return 0;
}

static int foreach_lifted_subset(__isl_take isl_set *set, __isl_take EL *el,
	int (*fn)(__isl_take isl_set *set, __isl_take EL *el,
		    void *user), void *user)
{
	int i;

	if (!set || !el)
		goto error;

	for (i = 0; i < set->n; ++i) {
		isl_set *lift;
		EL *copy;

		lift = isl_set_from_basic_set(isl_basic_set_copy(set->p[i]));
		lift = isl_set_lift(lift);

		copy = FN(EL,copy)(el);
		copy = FN(EL,lift)(copy, isl_set_get_space(lift));

		if (fn(lift, copy, user) < 0)
			goto error;
	}

	isl_set_free(set);
	FN(EL,free)(el);

	return 0;
error:
	isl_set_free(set);
	FN(EL,free)(el);
	return -1;
}

int FN(PW,foreach_lifted_piece)(__isl_keep PW *pw,
	int (*fn)(__isl_take isl_set *set, __isl_take EL *el,
		    void *user), void *user)
{
	int i;

	if (!pw)
		return -1;

	for (i = 0; i < pw->n; ++i) {
		isl_set *set;
		EL *el;

		set = isl_set_copy(pw->p[i].set);
		el = FN(EL,copy)(pw->p[i].FIELD);
		if (!any_divs(set)) {
			if (fn(set, el, user) < 0)
				return -1;
			continue;
		}
		if (foreach_lifted_subset(set, el, fn, user) < 0)
			return -1;
	}

	return 0;
}
#endif

#ifndef NO_MOVE_DIMS
__isl_give PW *FN(PW,move_dims)(__isl_take PW *pw,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n)
{
	int i;

	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;

	pw->dim = isl_space_move_dims(pw->dim, dst_type, dst_pos, src_type, src_pos, n);
	if (!pw->dim)
		goto error;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].FIELD = FN(EL,move_dims)(pw->p[i].FIELD,
					dst_type, dst_pos, src_type, src_pos, n);
		if (!pw->p[i].FIELD)
			goto error;
	}

	if (dst_type == isl_dim_in)
		dst_type = isl_dim_set;
	if (src_type == isl_dim_in)
		src_type = isl_dim_set;

	for (i = 0; i < pw->n; ++i) {
		pw->p[i].set = isl_set_move_dims(pw->p[i].set,
						dst_type, dst_pos,
						src_type, src_pos, n);
		if (!pw->p[i].set)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}
#endif

__isl_give PW *FN(PW,mul_isl_int)(__isl_take PW *pw, isl_int v)
{
	int i;

	if (isl_int_is_one(v))
		return pw;
	if (pw && DEFAULT_IS_ZERO && isl_int_is_zero(v)) {
		PW *zero;
		isl_space *dim = FN(PW,get_space)(pw);
#ifdef HAS_TYPE
		zero = FN(PW,ZERO)(dim, pw->type);
#else
		zero = FN(PW,ZERO)(dim);
#endif
		FN(PW,free)(pw);
		return zero;
	}
	pw = FN(PW,cow)(pw);
	if (!pw)
		return NULL;
	if (pw->n == 0)
		return pw;

#ifdef HAS_TYPE
	if (isl_int_is_neg(v))
		pw->type = isl_fold_type_negate(pw->type);
#endif
	for (i = 0; i < pw->n; ++i) {
		pw->p[i].FIELD = FN(EL,scale)(pw->p[i].FIELD, v);
		if (!pw->p[i].FIELD)
			goto error;
	}

	return pw;
error:
	FN(PW,free)(pw);
	return NULL;
}

__isl_give PW *FN(PW,scale)(__isl_take PW *pw, isl_int v)
{
	return FN(PW,mul_isl_int)(pw, v);
}

static int FN(PW,qsort_set_cmp)(const void *p1, const void *p2)
{
	const isl_set *set1 = *(const isl_set **)p1;
	const isl_set *set2 = *(const isl_set **)p2;

	return isl_set_plain_cmp(set1, set2);
}

/* We normalize in place, but if anything goes wrong we need
 * to return NULL, so we need to make sure we don't change the
 * meaning of any possible other copies of map.
 */
__isl_give PW *FN(PW,normalize)(__isl_take PW *pw)
{
	int i, j;
	isl_set *set;

	if (!pw)
		return NULL;
	for (i = 0; i < pw->n; ++i) {
		set = isl_set_normalize(isl_set_copy(pw->p[i].set));
		if (!set)
			return FN(PW,free)(pw);
		isl_set_free(pw->p[i].set);
		pw->p[i].set = set;
	}
	qsort(pw->p, pw->n, sizeof(pw->p[0]), &FN(PW,qsort_set_cmp));
	for (i = pw->n - 1; i >= 1; --i) {
		if (!isl_set_plain_is_equal(pw->p[i - 1].set, pw->p[i].set))
			continue;
		if (!FN(EL,plain_is_equal)(pw->p[i - 1].FIELD, pw->p[i].FIELD))
			continue;
		set = isl_set_union(isl_set_copy(pw->p[i - 1].set),
				    isl_set_copy(pw->p[i].set));
		if (!set)
			return FN(PW,free)(pw);
		isl_set_free(pw->p[i].set);
		FN(EL,free)(pw->p[i].FIELD);
		isl_set_free(pw->p[i - 1].set);
		pw->p[i - 1].set = set;
		for (j = i + 1; j < pw->n; ++j)
			pw->p[j - 1] = pw->p[j];
		pw->n--;
	}

	return pw;
}

/* Is pw1 obviously equal to pw2?
 * That is, do they have obviously identical cells and obviously identical
 * elements on each cell?
 */
int FN(PW,plain_is_equal)(__isl_keep PW *pw1, __isl_keep PW *pw2)
{
	int i;
	int equal;

	if (!pw1 || !pw2)
		return -1;

	if (pw1 == pw2)
		return 1;
	if (!isl_space_is_equal(pw1->dim, pw2->dim))
		return 0;

	pw1 = FN(PW,copy)(pw1);
	pw2 = FN(PW,copy)(pw2);
	pw1 = FN(PW,normalize)(pw1);
	pw2 = FN(PW,normalize)(pw2);
	if (!pw1 || !pw2)
		goto error;

	equal = pw1->n == pw2->n;
	for (i = 0; equal && i < pw1->n; ++i) {
		equal = isl_set_plain_is_equal(pw1->p[i].set, pw2->p[i].set);
		if (equal < 0)
			goto error;
		if (!equal)
			break;
		equal = FN(EL,plain_is_equal)(pw1->p[i].FIELD, pw2->p[i].FIELD);
		if (equal < 0)
			goto error;
	}

	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return equal;
error:
	FN(PW,free)(pw1);
	FN(PW,free)(pw2);
	return -1;
}
