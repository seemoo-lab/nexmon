/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_CONSTRAINT_H
#define ISL_CONSTRAINT_H

#include <isl/local_space.h>
#include <isl/space.h>
#include <isl/aff_type.h>
#include <isl/set_type.h>
#include <isl/printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_constraint;
typedef struct isl_constraint isl_constraint;

isl_ctx *isl_constraint_get_ctx(__isl_keep isl_constraint *c);

__isl_give isl_constraint *isl_equality_alloc(__isl_take isl_local_space *ls);
__isl_give isl_constraint *isl_inequality_alloc(__isl_take isl_local_space *ls);

struct isl_constraint *isl_constraint_cow(struct isl_constraint *c);
struct isl_constraint *isl_constraint_copy(struct isl_constraint *c);
void *isl_constraint_free(__isl_take isl_constraint *c);

int isl_basic_set_n_constraint(__isl_keep isl_basic_set *bset);
int isl_basic_map_foreach_constraint(__isl_keep isl_basic_map *bmap,
	int (*fn)(__isl_take isl_constraint *c, void *user), void *user);
int isl_basic_set_foreach_constraint(__isl_keep isl_basic_set *bset,
	int (*fn)(__isl_take isl_constraint *c, void *user), void *user);
int isl_constraint_is_equal(struct isl_constraint *constraint1,
			    struct isl_constraint *constraint2);

int isl_basic_set_foreach_bound_pair(__isl_keep isl_basic_set *bset,
	enum isl_dim_type type, unsigned pos,
	int (*fn)(__isl_take isl_constraint *lower,
		  __isl_take isl_constraint *upper,
		  __isl_take isl_basic_set *bset, void *user), void *user);

__isl_give isl_basic_map *isl_basic_map_add_constraint(
	__isl_take isl_basic_map *bmap, __isl_take isl_constraint *constraint);
__isl_give isl_basic_set *isl_basic_set_add_constraint(
	__isl_take isl_basic_set *bset, __isl_take isl_constraint *constraint);
__isl_give isl_map *isl_map_add_constraint(__isl_take isl_map *map,
	__isl_take isl_constraint *constraint);
__isl_give isl_set *isl_set_add_constraint(__isl_take isl_set *set,
	__isl_take isl_constraint *constraint);

int isl_basic_map_has_defining_equality(
	__isl_keep isl_basic_map *bmap, enum isl_dim_type type, int pos,
	__isl_give isl_constraint **c);
int isl_basic_set_has_defining_equality(
	struct isl_basic_set *bset, enum isl_dim_type type, int pos,
	struct isl_constraint **constraint);
int isl_basic_set_has_defining_inequalities(
	struct isl_basic_set *bset, enum isl_dim_type type, int pos,
	struct isl_constraint **lower,
	struct isl_constraint **upper);

__isl_give isl_space *isl_constraint_get_space(
	__isl_keep isl_constraint *constraint);
__isl_give isl_local_space *isl_constraint_get_local_space(
	__isl_keep isl_constraint *constraint);
int isl_constraint_dim(struct isl_constraint *constraint,
	enum isl_dim_type type);

int isl_constraint_involves_dims(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, unsigned first, unsigned n);

const char *isl_constraint_get_dim_name(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, unsigned pos);
void isl_constraint_get_constant(__isl_keep isl_constraint *constraint,
	isl_int *v);
void isl_constraint_get_coefficient(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, int pos, isl_int *v);
__isl_give isl_constraint *isl_constraint_set_constant(
	__isl_take isl_constraint *constraint, isl_int v);
__isl_give isl_constraint *isl_constraint_set_constant_si(
	__isl_take isl_constraint *constraint, int v);
__isl_give isl_constraint *isl_constraint_set_coefficient(
	__isl_take isl_constraint *constraint,
	enum isl_dim_type type, int pos, isl_int v);
__isl_give isl_constraint *isl_constraint_set_coefficient_si(
	__isl_take isl_constraint *constraint,
	enum isl_dim_type type, int pos, int v);

__isl_give isl_aff *isl_constraint_get_div(__isl_keep isl_constraint *constraint,
	int pos);

struct isl_constraint *isl_constraint_negate(struct isl_constraint *constraint);

int isl_constraint_is_equality(__isl_keep isl_constraint *constraint);
int isl_constraint_is_div_constraint(__isl_keep isl_constraint *constraint);

int isl_constraint_is_lower_bound(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, unsigned pos);
int isl_constraint_is_upper_bound(__isl_keep isl_constraint *constraint,
	enum isl_dim_type type, unsigned pos);

__isl_give isl_basic_map *isl_basic_map_from_constraint(
	__isl_take isl_constraint *constraint);
struct isl_basic_set *isl_basic_set_from_constraint(
	struct isl_constraint *constraint);

__isl_give isl_aff *isl_constraint_get_bound(
	__isl_keep isl_constraint *constraint, enum isl_dim_type type, int pos);
__isl_give isl_aff *isl_constraint_get_aff(
	__isl_keep isl_constraint *constraint);
__isl_give isl_constraint *isl_equality_from_aff(__isl_take isl_aff *aff);
__isl_give isl_constraint *isl_inequality_from_aff(__isl_take isl_aff *aff);

__isl_give isl_basic_set *isl_basic_set_drop_constraint(
	__isl_take isl_basic_set *bset, __isl_take isl_constraint *constraint);

__isl_give isl_printer *isl_printer_print_constraint(__isl_take isl_printer *p,
	__isl_keep isl_constraint *c);
void isl_constraint_dump(__isl_keep isl_constraint *c);

#if defined(__cplusplus)
}
#endif

#include <isl/dim.h>

#endif
