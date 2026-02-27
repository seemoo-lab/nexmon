/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 * Copyright 2011      INRIA Saclay
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 * and INRIA Saclay - Ile-de-France, Parc Club Orsay Universite,
 * ZAC des vignes, 4 rue Jacques Monod, 91893 Orsay, France
 */

#define xCAT(A,B) A ## B
#define CAT(A,B) xCAT(A,B)
#undef EL
#define EL CAT(isl_,BASE)
#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)
#define xLIST(EL) EL ## _list
#define LIST(EL) xLIST(EL)

isl_ctx *FN(LIST(EL),get_ctx)(__isl_keep LIST(EL) *list)
{
	return list ? list->ctx : NULL;
}

__isl_give LIST(EL) *FN(LIST(EL),alloc)(isl_ctx *ctx, int n)
{
	LIST(EL) *list;

	if (n < 0)
		isl_die(ctx, isl_error_invalid,
			"cannot create list of negative length",
			return NULL);
	list = isl_alloc(ctx, LIST(EL),
			 sizeof(LIST(EL)) + (n - 1) * sizeof(struct EL *));
	if (!list)
		return NULL;

	list->ctx = ctx;
	isl_ctx_ref(ctx);
	list->ref = 1;
	list->size = n;
	list->n = 0;
	return list;
}

__isl_give LIST(EL) *FN(LIST(EL),copy)(__isl_keep LIST(EL) *list)
{
	if (!list)
		return NULL;

	list->ref++;
	return list;
}

__isl_give LIST(EL) *FN(LIST(EL),dup)(__isl_keep LIST(EL) *list)
{
	int i;
	LIST(EL) *dup;

	if (!list)
		return NULL;

	dup = FN(LIST(EL),alloc)(FN(LIST(EL),get_ctx)(list), list->n);
	if (!dup)
		return NULL;
	for (i = 0; i < list->n; ++i)
		dup = FN(LIST(EL),add)(dup, FN(EL,copy)(list->p[i]));
	return dup;
}

__isl_give LIST(EL) *FN(LIST(EL),add)(__isl_take LIST(EL) *list,
	__isl_take struct EL *el)
{
	if (!list || !el)
		goto error;
	isl_assert(list->ctx, list->n < list->size, goto error);
	list->p[list->n] = el;
	list->n++;
	return list;
error:
	FN(EL,free)(el);
	FN(LIST(EL),free)(list);
	return NULL;
}

void *FN(LIST(EL),free)(__isl_take LIST(EL) *list)
{
	int i;

	if (!list)
		return NULL;

	if (--list->ref > 0)
		return NULL;

	isl_ctx_deref(list->ctx);
	for (i = 0; i < list->n; ++i)
		FN(EL,free)(list->p[i]);
	free(list);

	return NULL;
}

int FN(FN(LIST(EL),n),BASE)(__isl_keep LIST(EL) *list)
{
	return list ? list->n : 0;
}

__isl_give EL *FN(FN(LIST(EL),get),BASE)(__isl_keep LIST(EL) *list, int index)
{
	if (!list)
		return NULL;
	if (index < 0 || index >= list->n)
		isl_die(list->ctx, isl_error_invalid,
			"index out of bounds", return NULL);
	return FN(EL,copy)(list->p[index]);
}

int FN(LIST(EL),foreach)(__isl_keep LIST(EL) *list,
	int (*fn)(__isl_take EL *el, void *user), void *user)
{
	int i;

	if (!list)
		return -1;

	for (i = 0; i < list->n; ++i) {
		EL *el = FN(EL,copy(list->p[i]));
		if (!el)
			return -1;
		if (fn(el, user) < 0)
			return -1;
	}

	return 0;
}

__isl_give LIST(EL) *FN(FN(LIST(EL),from),BASE)(__isl_take EL *el)
{
	isl_ctx *ctx;
	LIST(EL) *list;

	if (!el)
		return NULL;
	ctx = FN(EL,get_ctx)(el);
	list = FN(LIST(EL),alloc)(ctx, 1);
	if (!list)
		goto error;
	list = FN(LIST(EL),add)(list, el);
	return list;
error:
	FN(EL,free)(el);
	return NULL;
}

__isl_give LIST(EL) *FN(LIST(EL),concat)(__isl_take LIST(EL) *list1,
	__isl_take LIST(EL) *list2)
{
	int i;
	isl_ctx *ctx;
	LIST(EL) *res;

	if (!list1 || !list2)
		goto error;

	ctx = FN(LIST(EL),get_ctx)(list1);
	res = FN(LIST(EL),alloc)(ctx, list1->n + list2->n);
	for (i = 0; i < list1->n; ++i)
		res = FN(LIST(EL),add)(res, FN(EL,copy)(list1->p[i]));
	for (i = 0; i < list2->n; ++i)
		res = FN(LIST(EL),add)(res, FN(EL,copy)(list2->p[i]));

	FN(LIST(EL),free)(list1);
	FN(LIST(EL),free)(list2);
	return res;
error:
	FN(LIST(EL),free)(list1);
	FN(LIST(EL),free)(list2);
	return NULL;
}

__isl_give isl_printer *CAT(isl_printer_print_,LIST(BASE))(
	__isl_take isl_printer *p, __isl_keep LIST(EL) *list)
{
	int i;

	if (!p || !list)
		goto error;
	p = isl_printer_print_str(p, "(");
	for (i = 0; i < list->n; ++i) {
		if (i)
			p = isl_printer_print_str(p, ",");
		p = CAT(isl_printer_print_,BASE)(p, list->p[i]);
	}
	p = isl_printer_print_str(p, ")");
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

void FN(LIST(EL),dump)(__isl_keep LIST(EL) *list)
{
	isl_printer *printer;

	if (!list)
		return;

	printer = isl_printer_to_file(FN(LIST(EL),get_ctx)(list), stderr);
	printer = CAT(isl_printer_print_,LIST(BASE))(printer, list);
	printer = isl_printer_end_line(printer);

	isl_printer_free(printer);
}
