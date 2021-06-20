/*
 * Copyright 2010      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, INRIA Saclay - Ile-de-France,
 * Parc Club Orsay Universite, ZAC des vignes, 4 rue Jacques Monod,
 * 91893 Orsay, France 
 */

#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)
#define xS(TYPE,NAME) struct TYPE ## _ ## NAME
#define S(TYPE,NAME) xS(TYPE,NAME)

struct UNION {
	int ref;
#ifdef HAS_TYPE
	enum isl_fold type;
#endif
	isl_space *dim;

	struct isl_hash_table	table;
};

__isl_give UNION *FN(UNION,cow)(__isl_take UNION *u);

isl_ctx *FN(UNION,get_ctx)(__isl_keep UNION *u)
{
	return u ? u->dim->ctx : NULL;
}

__isl_give isl_space *FN(UNION,get_space)(__isl_keep UNION *u)
{
	if (!u)
		return NULL;
	return isl_space_copy(u->dim);
}

#ifdef HAS_TYPE
static __isl_give UNION *FN(UNION,alloc)(__isl_take isl_space *dim,
	enum isl_fold type, int size)
#else
static __isl_give UNION *FN(UNION,alloc)(__isl_take isl_space *dim, int size)
#endif
{
	UNION *u;

	dim = isl_space_params(dim);
	if (!dim)
		return NULL;

	u = isl_calloc_type(dim->ctx, UNION);
	if (!u)
		return NULL;

	u->ref = 1;
#ifdef HAS_TYPE
	u->type = type;
#endif
	u->dim = dim;
	if (isl_hash_table_init(dim->ctx, &u->table, size) < 0)
		goto error;

	return u;
error:
	isl_space_free(dim);
	FN(UNION,free)(u);
	return NULL;
}

#ifdef HAS_TYPE
__isl_give UNION *FN(UNION,ZERO)(__isl_take isl_space *dim, enum isl_fold type)
{
	return FN(UNION,alloc)(dim, type, 16);
}
#else
__isl_give UNION *FN(UNION,ZERO)(__isl_take isl_space *dim)
{
	return FN(UNION,alloc)(dim, 16);
}
#endif

__isl_give UNION *FN(UNION,copy)(__isl_keep UNION *u)
{
	if (!u)
		return NULL;

	u->ref++;
	return u;
}

S(UNION,foreach_data)
{
	int (*fn)(__isl_take PART *part, void *user);
	void *user;
};

static int call_on_copy(void **entry, void *user)
{
	PART *part = *entry;
	S(UNION,foreach_data) *data = (S(UNION,foreach_data) *)user;

	return data->fn(FN(PART,copy)(part), data->user);
}

int FN(FN(UNION,foreach),PARTS)(__isl_keep UNION *u,
	int (*fn)(__isl_take PART *part, void *user), void *user)
{
	S(UNION,foreach_data) data = { fn, user };

	if (!u)
		return -1;

	return isl_hash_table_foreach(u->dim->ctx, &u->table,
				      &call_on_copy, &data);
}

static int has_dim(const void *entry, const void *val)
{
	PART *part = (PART *)entry;
	isl_space *dim = (isl_space *)val;

	return isl_space_is_equal(part->dim, dim);
}

__isl_give PART *FN(FN(UNION,extract),PARTS)(__isl_keep UNION *u,
	__isl_take isl_space *dim)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (!u || !dim)
		goto error;

	hash = isl_space_get_hash(dim);
	entry = isl_hash_table_find(u->dim->ctx, &u->table, hash,
				    &has_dim, dim, 0);
	if (!entry)
#ifdef HAS_TYPE
		return FN(PART,ZERO)(dim, u->type);
#else
		return FN(PART,ZERO)(dim);
#endif
	isl_space_free(dim);
	return FN(PART,copy)(entry->data);
error:
	isl_space_free(dim);
	return NULL;
}

__isl_give UNION *FN(FN(UNION,add),PARTS)(__isl_take UNION *u,
	__isl_take PART *part)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;

	if (!part)
		goto error;

	if (DEFAULT_IS_ZERO && FN(PART,IS_ZERO)(part)) {
		FN(PART,free)(part);
		return u;
	}

	u = FN(UNION,cow)(u);

	if (!u)
		goto error;

	isl_assert(u->dim->ctx, isl_space_match(part->dim, isl_dim_param, u->dim,
					      isl_dim_param), goto error);

	hash = isl_space_get_hash(part->dim);
	entry = isl_hash_table_find(u->dim->ctx, &u->table, hash,
				    &has_dim, part->dim, 1);
	if (!entry)
		goto error;

	if (!entry->data)
		entry->data = part;
	else {
		entry->data = FN(PART,add)(entry->data, FN(PART,copy)(part));
		if (!entry->data)
			goto error;
		FN(PART,free)(part);
		if (DEFAULT_IS_ZERO && FN(PART,IS_ZERO)(entry->data)) {
			FN(PART,free)(entry->data);
			isl_hash_table_remove(u->dim->ctx, &u->table, entry);
		}
	}

	return u;
error:
	FN(PART,free)(part);
	FN(UNION,free)(u);
	return NULL;
}

static int add_part(__isl_take PART *part, void *user)
{
	UNION **u = (UNION **)user;

	*u = FN(FN(UNION,add),PARTS)(*u, part);

	return 0;
}

__isl_give UNION *FN(UNION,dup)(__isl_keep UNION *u)
{
	UNION *dup;

	if (!u)
		return NULL;

#ifdef HAS_TYPE
	dup = FN(UNION,ZERO)(isl_space_copy(u->dim), u->type);
#else
	dup = FN(UNION,ZERO)(isl_space_copy(u->dim));
#endif
	if (FN(FN(UNION,foreach),PARTS)(u, &add_part, &dup) < 0)
		goto error;
	return dup;
error:
	FN(UNION,free)(dup);
	return NULL;
}

__isl_give UNION *FN(UNION,cow)(__isl_take UNION *u)
{
	if (!u)
		return NULL;

	if (u->ref == 1)
		return u;
	u->ref--;
	return FN(UNION,dup)(u);
}

static int free_u_entry(void **entry, void *user)
{
	PART *part = *entry;
	FN(PART,free)(part);
	return 0;
}

void *FN(UNION,free)(__isl_take UNION *u)
{
	if (!u)
		return NULL;

	if (--u->ref > 0)
		return NULL;

	isl_hash_table_foreach(u->dim->ctx, &u->table, &free_u_entry, NULL);
	isl_hash_table_clear(&u->table);
	isl_space_free(u->dim);
	free(u);
	return NULL;
}

S(UNION,align) {
	isl_reordering *exp;
	UNION *res;
};

#ifdef ALIGN_DOMAIN
static int align_entry(__isl_take PART *part, void *user)
{
	isl_reordering *exp;
	S(UNION,align) *data = user;

	exp = isl_reordering_extend_space(isl_reordering_copy(data->exp),
				    FN(PART,get_domain_space)(part));

	data->res = FN(FN(UNION,add),PARTS)(data->res,
					    FN(PART,realign_domain)(part, exp));

	return 0;
}
#else
static int align_entry(__isl_take PART *part, void *user)
{
	isl_reordering *exp;
	S(UNION,align) *data = user;

	exp = isl_reordering_extend_space(isl_reordering_copy(data->exp),
				    FN(PART,get_space)(part));

	data->res = FN(FN(UNION,add),PARTS)(data->res,
					    FN(PART,realign)(part, exp));

	return 0;
}
#endif

__isl_give UNION *FN(UNION,align_params)(__isl_take UNION *u,
	__isl_take isl_space *model)
{
	S(UNION,align) data = { NULL, NULL };

	if (!u || !model)
		goto error;

	if (isl_space_match(u->dim, isl_dim_param, model, isl_dim_param)) {
		isl_space_free(model);
		return u;
	}

	data.exp = isl_parameter_alignment_reordering(u->dim, model);
	if (!data.exp)
		goto error;

#ifdef HAS_TYPE
	data.res = FN(UNION,alloc)(isl_space_copy(data.exp->dim),
						u->type, u->table.n);
#else
	data.res = FN(UNION,alloc)(isl_space_copy(data.exp->dim), u->table.n);
#endif
	if (FN(FN(UNION,foreach),PARTS)(u, &align_entry, &data) < 0)
		goto error;

	isl_reordering_free(data.exp);
	FN(UNION,free)(u);
	isl_space_free(model);
	return data.res;
error:
	isl_reordering_free(data.exp);
	FN(UNION,free)(u);
	FN(UNION,free)(data.res);
	isl_space_free(model);
	return NULL;
}

__isl_give UNION *FN(UNION,add)(__isl_take UNION *u1, __isl_take UNION *u2)
{
	u1 = FN(UNION,align_params)(u1, FN(UNION,get_space)(u2));
	u2 = FN(UNION,align_params)(u2, FN(UNION,get_space)(u1));

	u1 = FN(UNION,cow)(u1);

	if (!u1 || !u2)
		goto error;

	if (FN(FN(UNION,foreach),PARTS)(u2, &add_part, &u1) < 0)
		goto error;

	FN(UNION,free)(u2);

	return u1;
error:
	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	return NULL;
}

__isl_give UNION *FN(FN(UNION,from),PARTS)(__isl_take PART *part)
{
	isl_space *dim;
	UNION *u;

	if (!part)
		return NULL;

	dim = FN(PART,get_space)(part);
	dim = isl_space_drop_dims(dim, isl_dim_in, 0, isl_space_dim(dim, isl_dim_in));
	dim = isl_space_drop_dims(dim, isl_dim_out, 0, isl_space_dim(dim, isl_dim_out));
#ifdef HAS_TYPE
	u = FN(UNION,ZERO)(dim, part->type);
#else
	u = FN(UNION,ZERO)(dim);
#endif
	u = FN(FN(UNION,add),PARTS)(u, part);

	return u;
}

S(UNION,match_bin_data) {
	UNION *u2;
	UNION *res;
	__isl_give PART *(*fn)(__isl_take PART *, __isl_take PART *);
};

/* Check if data->u2 has an element living in the same space as *entry.
 * If so, call data->fn on the two elements and add the result to
 * data->res.
 */
static int match_bin_entry(void **entry, void *user)
{
	S(UNION,match_bin_data) *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	isl_space *space;
	PART *part = *entry;

	space = FN(PART,get_space)(part);
	hash = isl_space_get_hash(space);
	entry2 = isl_hash_table_find(data->u2->dim->ctx, &data->u2->table,
				     hash, &has_dim, space, 0);
	isl_space_free(space);
	if (!entry2)
		return 0;

	part = FN(PART, copy)(part);
	part = data->fn(part, FN(PART, copy)(entry2->data));

	if (DEFAULT_IS_ZERO) {
		int empty;

		empty = FN(PART,IS_ZERO)(part);
		if (empty < 0) {
			FN(PART,free)(part);
			return -1;
		}
		if (empty) {
			FN(PART,free)(part);
			return 0;
		}
	}

	data->res = FN(FN(UNION,add),PARTS)(data->res, part);

	return 0;
}

/* This function is currently only used from isl_polynomial.c
 * and not from isl_fold.c.
 */
static __isl_give UNION *match_bin_op(__isl_take UNION *u1,
	__isl_take UNION *u2,
	__isl_give PART *(*fn)(__isl_take PART *, __isl_take PART *))
	__attribute__ ((unused));
/* For each pair of elements in "u1" and "u2" living in the same space,
 * call "fn" and collect the results.
 */
static __isl_give UNION *match_bin_op(__isl_take UNION *u1,
	__isl_take UNION *u2,
	__isl_give PART *(*fn)(__isl_take PART *, __isl_take PART *))
{
	S(UNION,match_bin_data) data = { NULL, NULL, fn };

	u1 = FN(UNION,align_params)(u1, FN(UNION,get_space)(u2));
	u2 = FN(UNION,align_params)(u2, FN(UNION,get_space)(u1));

	if (!u1 || !u2)
		goto error;

	data.u2 = u2;
#ifdef HAS_TYPE
	data.res = FN(UNION,alloc)(isl_space_copy(u1->dim), u1->type, u1->table.n);
#else
	data.res = FN(UNION,alloc)(isl_space_copy(u1->dim), u1->table.n);
#endif
	if (isl_hash_table_foreach(u1->dim->ctx, &u1->table,
				    &match_bin_entry, &data) < 0)
		goto error;

	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	return data.res;
error:
	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	FN(UNION,free)(data.res);
	return NULL;
}

S(UNION,any_set_data) {
	isl_set *set;
	UNION *res;
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*);
};

static int any_set_entry(void **entry, void *user)
{
	S(UNION,any_set_data) *data = user;
	PW *pw = *entry;

	pw = FN(PW,copy)(pw);
	pw = data->fn(pw, isl_set_copy(data->set));

	if (DEFAULT_IS_ZERO) {
		int empty;

		empty = FN(PW,IS_ZERO)(pw);
		if (empty < 0) {
			FN(PW,free)(pw);
			return -1;
		}
		if (empty) {
			FN(PW,free)(pw);
			return 0;
		}
	}

	data->res = FN(FN(UNION,add),PARTS)(data->res, pw);

	return 0;
}

/* Update each element of "u" by calling "fn" on the element and "set".
 */
static __isl_give UNION *any_set_op(__isl_take UNION *u,
	__isl_take isl_set *set,
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*))
{
	S(UNION,any_set_data) data = { NULL, NULL, fn };

	u = FN(UNION,align_params)(u, isl_set_get_space(set));
	set = isl_set_align_params(set, FN(UNION,get_space)(u));

	if (!u || !set)
		goto error;

	data.set = set;
#ifdef HAS_TYPE
	data.res = FN(UNION,alloc)(isl_space_copy(u->dim), u->type, u->table.n);
#else
	data.res = FN(UNION,alloc)(isl_space_copy(u->dim), u->table.n);
#endif
	if (isl_hash_table_foreach(u->dim->ctx, &u->table,
				   &any_set_entry, &data) < 0)
		goto error;

	FN(UNION,free)(u);
	isl_set_free(set);
	return data.res;
error:
	FN(UNION,free)(u);
	isl_set_free(set);
	FN(UNION,free)(data.res);
	return NULL;
}

/* Intersect the domain of "u" with the parameter domain "context".
 */
__isl_give UNION *FN(UNION,intersect_params)(__isl_take UNION *u,
	__isl_take isl_set *set)
{
	return any_set_op(u, set, &FN(PW,intersect_params));
}

/* Compute the gist of the domain of "u" with respect to
 * the parameter domain "context".
 */
__isl_give UNION *FN(UNION,gist_params)(__isl_take UNION *u,
	__isl_take isl_set *set)
{
	return any_set_op(u, set, &FN(PW,gist_params));
}

S(UNION,match_domain_data) {
	isl_union_set *uset;
	UNION *res;
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*);
};

static int set_has_dim(const void *entry, const void *val)
{
	isl_set *set = (isl_set *)entry;
	isl_space *dim = (isl_space *)val;

	return isl_space_is_equal(set->dim, dim);
}

/* Find the set in data->uset that live in the same space as the domain
 * of *entry, apply data->fn to *entry and this set (if any), and add
 * the result to data->res.
 */
static int match_domain_entry(void **entry, void *user)
{
	S(UNION,match_domain_data) *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	PW *pw = *entry;
	isl_space *space;

	space = FN(PW,get_domain_space)(pw);
	hash = isl_space_get_hash(space);
	entry2 = isl_hash_table_find(data->uset->dim->ctx, &data->uset->table,
				     hash, &set_has_dim, space, 0);
	isl_space_free(space);
	if (!entry2)
		return 0;

	pw = FN(PW,copy)(pw);
	pw = data->fn(pw, isl_set_copy(entry2->data));

	if (DEFAULT_IS_ZERO) {
		int empty;

		empty = FN(PW,IS_ZERO)(pw);
		if (empty < 0) {
			FN(PW,free)(pw);
			return -1;
		}
		if (empty) {
			FN(PW,free)(pw);
			return 0;
		}
	}

	data->res = FN(FN(UNION,add),PARTS)(data->res, pw);

	return 0;
}

/* Apply fn to each pair of PW in u and set in uset such that
 * the set lives in the same space as the domain of PW
 * and collect the results.
 */
static __isl_give UNION *match_domain_op(__isl_take UNION *u,
	__isl_take isl_union_set *uset,
	__isl_give PW *(*fn)(__isl_take PW*, __isl_take isl_set*))
{
	S(UNION,match_domain_data) data = { NULL, NULL, fn };

	u = FN(UNION,align_params)(u, isl_union_set_get_space(uset));
	uset = isl_union_set_align_params(uset, FN(UNION,get_space)(u));

	if (!u || !uset)
		goto error;

	data.uset = uset;
#ifdef HAS_TYPE
	data.res = FN(UNION,alloc)(isl_space_copy(u->dim), u->type, u->table.n);
#else
	data.res = FN(UNION,alloc)(isl_space_copy(u->dim), u->table.n);
#endif
	if (isl_hash_table_foreach(u->dim->ctx, &u->table,
				   &match_domain_entry, &data) < 0)
		goto error;

	FN(UNION,free)(u);
	isl_union_set_free(uset);
	return data.res;
error:
	FN(UNION,free)(u);
	isl_union_set_free(uset);
	FN(UNION,free)(data.res);
	return NULL;
}

/* Intersect the domain of "u" with "uset".
 * If "uset" is a parameters domain, then intersect the parameter
 * domain of "u" with this set.
 */
__isl_give UNION *FN(UNION,intersect_domain)(__isl_take UNION *u,
	__isl_take isl_union_set *uset)
{
	if (isl_union_set_is_params(uset))
		return FN(UNION,intersect_params)(u,
						isl_set_from_union_set(uset));
	return match_domain_op(u, uset, &FN(PW,intersect_domain));
}

__isl_give UNION *FN(UNION,gist)(__isl_take UNION *u,
	__isl_take isl_union_set *uset)
{
	if (isl_union_set_is_params(uset))
		return FN(UNION,gist_params)(u, isl_set_from_union_set(uset));
	return match_domain_op(u, uset, &FN(PW,gist));
}

#ifndef NO_EVAL
__isl_give isl_qpolynomial *FN(UNION,eval)(__isl_take UNION *u,
	__isl_take isl_point *pnt)
{
	uint32_t hash;
	struct isl_hash_table_entry *entry;
	isl_space *space;
	isl_qpolynomial *qp;

	if (!u || !pnt)
		goto error;

	space = isl_space_copy(pnt->dim);
	space = isl_space_from_domain(space);
	space = isl_space_add_dims(space, isl_dim_out, 1);
	if (!space)
		goto error;
	hash = isl_space_get_hash(space);
	entry = isl_hash_table_find(u->dim->ctx, &u->table,
				    hash, &has_dim, space, 0);
	isl_space_free(space);
	if (!entry) {
		qp = isl_qpolynomial_zero_on_domain(isl_space_copy(pnt->dim));
		isl_point_free(pnt);
	} else {
		qp = FN(PART,eval)(FN(PART,copy)(entry->data), pnt);
	}
	FN(UNION,free)(u);
	return qp;
error:
	FN(UNION,free)(u);
	isl_point_free(pnt);
	return NULL;
}
#endif

static int coalesce_entry(void **entry, void *user)
{
	PW **pw = (PW **)entry;

	*pw = FN(PW,coalesce)(*pw);
	if (!*pw)
		return -1;

	return 0;
}

__isl_give UNION *FN(UNION,coalesce)(__isl_take UNION *u)
{
	if (!u)
		return NULL;

	if (isl_hash_table_foreach(u->dim->ctx, &u->table,
				   &coalesce_entry, NULL) < 0)
		goto error;

	return u;
error:
	FN(UNION,free)(u);
	return NULL;
}

static int domain(__isl_take PART *part, void *user)
{
	isl_union_set **uset = (isl_union_set **)user;

	*uset = isl_union_set_add_set(*uset, FN(PART,domain)(part));

	return 0;
}

__isl_give isl_union_set *FN(UNION,domain)(__isl_take UNION *u)
{
	isl_union_set *uset;

	uset = isl_union_set_empty(FN(UNION,get_space)(u));
	if (FN(FN(UNION,foreach),PARTS)(u, &domain, &uset) < 0)
		goto error;

	FN(UNION,free)(u);
	
	return uset;
error:
	isl_union_set_free(uset);
	FN(UNION,free)(u);
	return NULL;
}

static int mul_isl_int(void **entry, void *user)
{
	PW **pw = (PW **)entry;
	isl_int *v = user;

	*pw = FN(PW,mul_isl_int)(*pw, *v);
	if (!*pw)
		return -1;

	return 0;
}

__isl_give UNION *FN(UNION,mul_isl_int)(__isl_take UNION *u, isl_int v)
{
	if (isl_int_is_one(v))
		return u;

	if (DEFAULT_IS_ZERO && u && isl_int_is_zero(v)) {
		UNION *zero;
		isl_space *dim = FN(UNION,get_space)(u);
#ifdef HAS_TYPE
		zero = FN(UNION,ZERO)(dim, u->type);
#else
		zero = FN(UNION,ZERO)(dim);
#endif
		FN(UNION,free)(u);
		return zero;
	}

	u = FN(UNION,cow)(u);
	if (!u)
		return NULL;

#ifdef HAS_TYPE
	if (isl_int_is_neg(v))
		u->type = isl_fold_type_negate(u->type);
#endif
	if (isl_hash_table_foreach(u->dim->ctx, &u->table, &mul_isl_int, v) < 0)
		goto error;

	return u;
error:
	FN(UNION,free)(u);
	return NULL;
}

S(UNION,plain_is_equal_data)
{
	UNION *u2;
	int is_equal;
};

static int plain_is_equal_entry(void **entry, void *user)
{
	S(UNION,plain_is_equal_data) *data = user;
	uint32_t hash;
	struct isl_hash_table_entry *entry2;
	PW *pw = *entry;

	hash = isl_space_get_hash(pw->dim);
	entry2 = isl_hash_table_find(data->u2->dim->ctx, &data->u2->table,
				     hash, &has_dim, pw->dim, 0);
	if (!entry2) {
		data->is_equal = 0;
		return -1;
	}

	data->is_equal = FN(PW,plain_is_equal)(pw, entry2->data);
	if (data->is_equal < 0 || !data->is_equal)
		return -1;

	return 0;
}

int FN(UNION,plain_is_equal)(__isl_keep UNION *u1, __isl_keep UNION *u2)
{
	S(UNION,plain_is_equal_data) data = { NULL, 1 };

	if (!u1 || !u2)
		return -1;
	if (u1 == u2)
		return 1;
	if (u1->table.n != u2->table.n)
		return 0;

	u1 = FN(UNION,copy)(u1);
	u2 = FN(UNION,copy)(u2);
	u1 = FN(UNION,align_params)(u1, FN(UNION,get_space)(u2));
	u2 = FN(UNION,align_params)(u2, FN(UNION,get_space)(u1));
	if (!u1 || !u2)
		goto error;

	data.u2 = u2;
	if (isl_hash_table_foreach(u1->dim->ctx, &u1->table,
				   &plain_is_equal_entry, &data) < 0 &&
	    data.is_equal)
		goto error;

	FN(UNION,free)(u1);
	FN(UNION,free)(u2);

	return data.is_equal;
error:
	FN(UNION,free)(u1);
	FN(UNION,free)(u2);
	return -1;
}
