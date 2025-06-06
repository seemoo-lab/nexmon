/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_LIST_H
#define ISL_LIST_H

#include <isl/ctx.h>
#include <isl/printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ISL_DECLARE_LIST(EL)						\
struct isl_##EL;							\
struct isl_##EL##_list;							\
typedef struct isl_##EL##_list isl_##EL##_list;				\
isl_ctx *isl_##EL##_list_get_ctx(__isl_keep isl_##EL##_list *list);	\
__isl_give isl_##EL##_list *isl_##EL##_list_from_##EL(			\
	__isl_take struct isl_##EL *el);				\
__isl_give isl_##EL##_list *isl_##EL##_list_alloc(isl_ctx *ctx, int n);	\
__isl_give isl_##EL##_list *isl_##EL##_list_copy(			\
	__isl_keep isl_##EL##_list *list);				\
void *isl_##EL##_list_free(__isl_take isl_##EL##_list *list);		\
__isl_give isl_##EL##_list *isl_##EL##_list_add(			\
	__isl_take isl_##EL##_list *list,				\
	__isl_take struct isl_##EL *el);				\
__isl_give isl_##EL##_list *isl_##EL##_list_concat(			\
	__isl_take isl_##EL##_list *list1,				\
	__isl_take isl_##EL##_list *list2);				\
int isl_##EL##_list_n_##EL(__isl_keep isl_##EL##_list *list);		\
__isl_give struct isl_##EL *isl_##EL##_list_get_##EL(			\
	__isl_keep isl_##EL##_list *list, int index);			\
int isl_##EL##_list_foreach(__isl_keep isl_##EL##_list *list,		\
	int (*fn)(__isl_take struct isl_##EL *el, void *user),		\
	void *user);							\
__isl_give isl_printer *isl_printer_print_##EL##_list(			\
	__isl_take isl_printer *p, __isl_keep isl_##EL##_list *list);	\
void isl_##EL##_list_dump(__isl_keep isl_##EL##_list *list);

ISL_DECLARE_LIST(basic_set)
ISL_DECLARE_LIST(set)
ISL_DECLARE_LIST(aff)
ISL_DECLARE_LIST(pw_aff)
ISL_DECLARE_LIST(band)

#if defined(__cplusplus)
}
#endif

#endif
