/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_SET_H
#define ISL_SET_H

#include <isl/map_type.h>
#include <isl/aff_type.h>
#include <isl/list.h>
#include <isl/mat.h>
#include <isl/point.h>
#include <isl/local_space.h>

#if defined(__cplusplus)
extern "C" {
#endif

unsigned isl_basic_set_n_dim(__isl_keep isl_basic_set *bset);
unsigned isl_basic_set_n_param(__isl_keep isl_basic_set *bset);
unsigned isl_basic_set_total_dim(const struct isl_basic_set *bset);
unsigned isl_basic_set_dim(__isl_keep isl_basic_set *bset,
				enum isl_dim_type type);

unsigned isl_set_n_dim(__isl_keep isl_set *set);
unsigned isl_set_n_param(__isl_keep isl_set *set);
unsigned isl_set_dim(__isl_keep isl_set *set, enum isl_dim_type type);

isl_ctx *isl_basic_set_get_ctx(__isl_keep isl_basic_set *bset);
isl_ctx *isl_set_get_ctx(__isl_keep isl_set *set);
__isl_give isl_space *isl_basic_set_get_space(__isl_keep isl_basic_set *bset);
__isl_give isl_space *isl_set_get_space(__isl_keep isl_set *set);
__isl_give isl_set *isl_set_reset_space(__isl_take isl_set *set,
	__isl_take isl_space *dim);

__isl_give isl_aff *isl_basic_set_get_div(__isl_keep isl_basic_set *bset,
	int pos);

__isl_give isl_local_space *isl_basic_set_get_local_space(
	__isl_keep isl_basic_set *bset);

const char *isl_basic_set_get_tuple_name(__isl_keep isl_basic_set *bset);
int isl_set_has_tuple_name(__isl_keep isl_set *set);
const char *isl_set_get_tuple_name(__isl_keep isl_set *set);
__isl_give isl_basic_set *isl_basic_set_set_tuple_name(
	__isl_take isl_basic_set *set, const char *s);
__isl_give isl_set *isl_set_set_tuple_name(__isl_take isl_set *set,
	const char *s);
const char *isl_basic_set_get_dim_name(__isl_keep isl_basic_set *bset,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_basic_set *isl_basic_set_set_dim_name(
	__isl_take isl_basic_set *bset,
	enum isl_dim_type type, unsigned pos, const char *s);
int isl_set_has_dim_name(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned pos);
const char *isl_set_get_dim_name(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_set *isl_set_set_dim_name(__isl_take isl_set *set,
	enum isl_dim_type type, unsigned pos, const char *s);

__isl_give isl_id *isl_basic_set_get_dim_id(__isl_keep isl_basic_set *bset,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_set *isl_set_set_dim_id(__isl_take isl_set *set,
	enum isl_dim_type type, unsigned pos, __isl_take isl_id *id);
int isl_set_has_dim_id(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_id *isl_set_get_dim_id(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_set *isl_set_set_tuple_id(__isl_take isl_set *set,
	__isl_take isl_id *id);
__isl_give isl_set *isl_set_reset_tuple_id(__isl_take isl_set *set);
int isl_set_has_tuple_id(__isl_keep isl_set *set);
__isl_give isl_id *isl_set_get_tuple_id(__isl_keep isl_set *set);

int isl_set_find_dim_by_id(__isl_keep isl_set *set, enum isl_dim_type type,
	__isl_keep isl_id *id);
int isl_set_find_dim_by_name(__isl_keep isl_set *set, enum isl_dim_type type,
	const char *name);

int isl_basic_set_is_rational(__isl_keep isl_basic_set *bset);

struct isl_basic_set *isl_basic_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq);
struct isl_basic_set *isl_basic_set_extend(struct isl_basic_set *base,
		unsigned nparam, unsigned dim, unsigned extra,
		unsigned n_eq, unsigned n_ineq);
struct isl_basic_set *isl_basic_set_extend_constraints(
		struct isl_basic_set *base, unsigned n_eq, unsigned n_ineq);
struct isl_basic_set *isl_basic_set_finalize(struct isl_basic_set *bset);
void isl_basic_set_free(__isl_take isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_copy(__isl_keep isl_basic_set *bset);
struct isl_basic_set *isl_basic_set_dup(struct isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_empty(__isl_take isl_space *dim);
struct isl_basic_set *isl_basic_set_empty_like(struct isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_universe(__isl_take isl_space *dim);
__isl_give isl_basic_set *isl_basic_set_nat_universe(__isl_take isl_space *dim);
struct isl_basic_set *isl_basic_set_universe_like(struct isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_universe_like_set(
	__isl_keep isl_set *model);
struct isl_basic_set *isl_basic_set_interval(struct isl_ctx *ctx,
	isl_int min, isl_int max);
__isl_give isl_basic_set *isl_basic_set_positive_orthant(
	__isl_take isl_space *space);
void isl_basic_set_print_internal(__isl_keep isl_basic_set *bset,
				FILE *out, int indent);
__isl_export
__isl_give isl_basic_set *isl_basic_set_intersect(
		__isl_take isl_basic_set *bset1,
		__isl_take isl_basic_set *bset2);
__isl_export
__isl_give isl_basic_set *isl_basic_set_intersect_params(
	__isl_take isl_basic_set *bset1, __isl_take isl_basic_set *bset2);
__isl_export
__isl_give isl_basic_set *isl_basic_set_apply(
		__isl_take isl_basic_set *bset,
		__isl_take isl_basic_map *bmap);
__isl_export
__isl_give isl_basic_set *isl_basic_set_affine_hull(
		__isl_take isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_remove_dims(
	__isl_take isl_basic_set *bset,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_export
__isl_give isl_basic_set *isl_basic_set_sample(__isl_take isl_basic_set *bset);
struct isl_basic_set *isl_basic_set_simplify(struct isl_basic_set *bset);
__isl_export
__isl_give isl_basic_set *isl_basic_set_detect_equalities(
						__isl_take isl_basic_set *bset);
__isl_give isl_basic_set *isl_basic_set_remove_redundancies(
	__isl_take isl_basic_set *bset);
__isl_give isl_set *isl_set_remove_redundancies(__isl_take isl_set *set);
__isl_give isl_basic_set *isl_basic_set_list_product(
	__isl_take struct isl_basic_set_list *list);

__isl_give isl_basic_set *isl_basic_set_read_from_file(isl_ctx *ctx,
	FILE *input);
__isl_constructor
__isl_give isl_basic_set *isl_basic_set_read_from_str(isl_ctx *ctx,
	const char *str);
__isl_give isl_set *isl_set_read_from_file(isl_ctx *ctx, FILE *input);
__isl_constructor
__isl_give isl_set *isl_set_read_from_str(isl_ctx *ctx, const char *str);
void isl_basic_set_dump(__isl_keep isl_basic_set *bset);
void isl_set_dump(__isl_keep isl_set *set);
__isl_give isl_printer *isl_printer_print_basic_set(
	__isl_take isl_printer *printer, __isl_keep isl_basic_set *bset);
__isl_give isl_printer *isl_printer_print_set(__isl_take isl_printer *printer,
	__isl_keep isl_set *map);
void isl_basic_set_print(__isl_keep isl_basic_set *bset, FILE *out, int indent,
	const char *prefix, const char *suffix, unsigned output_format);
void isl_set_print(__isl_keep struct isl_set *set, FILE *out, int indent,
	unsigned output_format);
__isl_give isl_basic_set *isl_basic_set_fix(__isl_take isl_basic_set *bset,
		enum isl_dim_type type, unsigned pos, isl_int value);
__isl_give isl_basic_set *isl_basic_set_fix_si(__isl_take isl_basic_set *bset,
		enum isl_dim_type type, unsigned pos, int value);
__isl_give isl_set *isl_set_fix_si(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned pos, int value);
__isl_give isl_set *isl_set_lower_bound_si(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned pos, int value);
__isl_give isl_set *isl_set_lower_bound(__isl_take isl_set *set,
	enum isl_dim_type type, unsigned pos, isl_int value);
__isl_give isl_set *isl_set_upper_bound_si(__isl_take isl_set *set,
	enum isl_dim_type type, unsigned pos, int value);
__isl_give isl_set *isl_set_upper_bound(__isl_take isl_set *set,
	enum isl_dim_type type, unsigned pos, isl_int value);

__isl_give isl_set *isl_set_equate(__isl_take isl_set *set,
	enum isl_dim_type type1, int pos1, enum isl_dim_type type2, int pos2);

struct isl_basic_set *isl_basic_set_from_underlying_set(
	struct isl_basic_set *bset, struct isl_basic_set *like);
struct isl_set *isl_set_from_underlying_set(
	struct isl_set *set, struct isl_basic_set *like);
struct isl_set *isl_set_to_underlying_set(struct isl_set *set);

__isl_export
int isl_basic_set_is_equal(
		struct isl_basic_set *bset1, struct isl_basic_set *bset2);

__isl_give isl_set *isl_basic_set_partial_lexmin(
		__isl_take isl_basic_set *bset, __isl_take isl_basic_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_set *isl_basic_set_partial_lexmax(
		__isl_take isl_basic_set *bset, __isl_take isl_basic_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_set *isl_set_partial_lexmin(
		__isl_take isl_set *set, __isl_take isl_set *dom,
		__isl_give isl_set **empty);
__isl_give isl_set *isl_set_partial_lexmax(
		__isl_take isl_set *set, __isl_take isl_set *dom,
		__isl_give isl_set **empty);
__isl_export
__isl_give isl_set *isl_basic_set_lexmin(__isl_take isl_basic_set *bset);
__isl_export
__isl_give isl_set *isl_basic_set_lexmax(__isl_take isl_basic_set *bset);
__isl_export
__isl_give isl_set *isl_set_lexmin(__isl_take isl_set *set);
__isl_export
__isl_give isl_set *isl_set_lexmax(__isl_take isl_set *set);
__isl_give isl_pw_multi_aff *isl_basic_set_partial_lexmin_pw_multi_aff(
	__isl_take isl_basic_set *bset, __isl_take isl_basic_set *dom,
	__isl_give isl_set **empty);
__isl_give isl_pw_multi_aff *isl_basic_set_partial_lexmax_pw_multi_aff(
	__isl_take isl_basic_set *bset, __isl_take isl_basic_set *dom,
	__isl_give isl_set **empty);

__isl_export
__isl_give isl_set *isl_basic_set_union(
		__isl_take isl_basic_set *bset1,
		__isl_take isl_basic_set *bset2);

int isl_basic_set_compare_at(struct isl_basic_set *bset1,
	struct isl_basic_set *bset2, int pos);
int isl_set_follows_at(__isl_keep isl_set *set1,
	__isl_keep isl_set *set2, int pos);

__isl_give isl_basic_set *isl_basic_set_params(__isl_take isl_basic_set *bset);
__isl_give isl_set *isl_set_params(__isl_take isl_set *set);
__isl_give isl_set *isl_set_from_params(__isl_take isl_set *set);

int isl_basic_set_dims_get_sign(__isl_keep isl_basic_set *bset,
	enum isl_dim_type type, unsigned pos, unsigned n, int *signs);

int isl_basic_set_is_universe(__isl_keep isl_basic_set *bset);
int isl_basic_set_plain_is_empty(__isl_keep isl_basic_set *bset);
int isl_basic_set_fast_is_empty(__isl_keep isl_basic_set *bset);
__isl_export
int isl_basic_set_is_empty(__isl_keep isl_basic_set *bset);
int isl_basic_set_is_bounded(__isl_keep isl_basic_set *bset);
__isl_export
int isl_basic_set_is_subset(__isl_keep isl_basic_set *bset1,
	__isl_keep isl_basic_set *bset2);

struct isl_set *isl_set_alloc(struct isl_ctx *ctx,
		unsigned nparam, unsigned dim, int n, unsigned flags);
struct isl_set *isl_set_extend(struct isl_set *base,
		unsigned nparam, unsigned dim);
__isl_give isl_set *isl_set_empty(__isl_take isl_space *dim);
struct isl_set *isl_set_empty_like(struct isl_set *set);
__isl_give isl_set *isl_set_universe(__isl_take isl_space *dim);
__isl_give isl_set *isl_set_nat_universe(__isl_take isl_space *dim);
__isl_give isl_set *isl_set_universe_like(__isl_keep isl_set *model);
__isl_give isl_set *isl_set_add_basic_set(__isl_take isl_set *set,
						__isl_take isl_basic_set *bset);
struct isl_set *isl_set_finalize(struct isl_set *set);
__isl_give isl_set *isl_set_copy(__isl_keep isl_set *set);
void isl_set_free(__isl_take isl_set *set);
struct isl_set *isl_set_dup(struct isl_set *set);
__isl_constructor
__isl_give isl_set *isl_set_from_basic_set(__isl_take isl_basic_set *bset);
__isl_export
__isl_give isl_basic_set *isl_set_sample(__isl_take isl_set *set);
__isl_give isl_point *isl_basic_set_sample_point(__isl_take isl_basic_set *bset);
__isl_give isl_point *isl_set_sample_point(__isl_take isl_set *set);
__isl_export
__isl_give isl_set *isl_set_detect_equalities(__isl_take isl_set *set);
__isl_export
__isl_give isl_basic_set *isl_set_affine_hull(__isl_take isl_set *set);
__isl_give isl_basic_set *isl_set_convex_hull(__isl_take isl_set *set);
__isl_export
__isl_give isl_basic_set *isl_set_polyhedral_hull(__isl_take isl_set *set);
__isl_give isl_basic_set *isl_set_simple_hull(__isl_take isl_set *set);
struct isl_basic_set *isl_set_bounded_simple_hull(struct isl_set *set);
__isl_give isl_set *isl_set_recession_cone(__isl_take isl_set *set);

struct isl_set *isl_set_union_disjoint(
			struct isl_set *set1, struct isl_set *set2);
__isl_export
__isl_give isl_set *isl_set_union(
		__isl_take isl_set *set1,
		__isl_take isl_set *set2);
__isl_give isl_set *isl_set_product(__isl_take isl_set *set1,
	__isl_take isl_set *set2);
__isl_give isl_basic_set *isl_basic_set_flat_product(
	__isl_take isl_basic_set *bset1, __isl_take isl_basic_set *bset2);
__isl_give isl_set *isl_set_flat_product(__isl_take isl_set *set1,
	__isl_take isl_set *set2);
__isl_export
__isl_give isl_set *isl_set_intersect(
		__isl_take isl_set *set1,
		__isl_take isl_set *set2);
__isl_export
__isl_give isl_set *isl_set_intersect_params(__isl_take isl_set *set,
		__isl_take isl_set *params);
__isl_export
__isl_give isl_set *isl_set_subtract(
		__isl_take isl_set *set1,
		__isl_take isl_set *set2);
__isl_export
__isl_give isl_set *isl_set_complement(__isl_take isl_set *set);
__isl_export
__isl_give isl_set *isl_set_apply(
		__isl_take isl_set *set,
		__isl_take isl_map *map);
__isl_give isl_set *isl_set_fix(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned pos, isl_int value);
struct isl_set *isl_set_fix_dim_si(struct isl_set *set,
		unsigned dim, int value);
struct isl_set *isl_set_lower_bound_dim(struct isl_set *set,
		unsigned dim, isl_int value);
__isl_give isl_set *isl_set_insert_dims(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned pos, unsigned n);
__isl_give isl_basic_set *isl_basic_set_add(__isl_take isl_basic_set *bset,
		enum isl_dim_type type, unsigned n);
__isl_give isl_set *isl_set_add_dims(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned n);
__isl_give isl_basic_set *isl_basic_set_move_dims(__isl_take isl_basic_set *bset,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n);
__isl_give isl_set *isl_set_move_dims(__isl_take isl_set *set,
	enum isl_dim_type dst_type, unsigned dst_pos,
	enum isl_dim_type src_type, unsigned src_pos, unsigned n);
__isl_give isl_basic_set *isl_basic_set_project_out(
		__isl_take isl_basic_set *bset,
		enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_set *isl_set_project_out(__isl_take isl_set *set,
		enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_basic_set *isl_basic_set_remove_divs(
	__isl_take isl_basic_set *bset);
__isl_give isl_set *isl_set_eliminate(__isl_take isl_set *set,
	enum isl_dim_type type, unsigned first, unsigned n);
struct isl_set *isl_set_eliminate_dims(struct isl_set *set,
		unsigned first, unsigned n);
__isl_give isl_set *isl_set_remove_dims(__isl_take isl_set *bset,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_set *isl_set_remove_divs_involving_dims(__isl_take isl_set *set,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_set *isl_set_remove_unknown_divs(__isl_take isl_set *set);
__isl_give isl_set *isl_set_remove_divs(__isl_take isl_set *set);
__isl_give isl_set *isl_set_split_dims(__isl_take isl_set *set,
	enum isl_dim_type type, unsigned first, unsigned n);

int isl_basic_set_involves_dims(__isl_keep isl_basic_set *bset,
	enum isl_dim_type type, unsigned first, unsigned n);
int isl_set_involves_dims(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned first, unsigned n);

void isl_set_print_internal(__isl_keep isl_set *set, FILE *out, int indent);
int isl_set_plain_is_empty(__isl_keep isl_set *set);
int isl_set_fast_is_empty(__isl_keep isl_set *set);
int isl_set_plain_is_universe(__isl_keep isl_set *set);
int isl_set_fast_is_universe(__isl_keep isl_set *set);
int isl_set_is_params(__isl_keep isl_set *set);
__isl_export
int isl_set_is_empty(__isl_keep isl_set *set);
int isl_set_is_bounded(__isl_keep isl_set *set);
__isl_export
int isl_set_is_subset(__isl_keep isl_set *set1, __isl_keep isl_set *set2);
__isl_export
int isl_set_is_strict_subset(__isl_keep isl_set *set1, __isl_keep isl_set *set2);
__isl_export
int isl_set_is_equal(__isl_keep isl_set *set1, __isl_keep isl_set *set2);
int isl_set_is_singleton(__isl_keep isl_set *set);
int isl_set_is_box(__isl_keep isl_set *set);
int isl_set_has_equal_space(__isl_keep isl_set *set1, __isl_keep isl_set *set2);

__isl_give isl_set *isl_set_sum(__isl_take isl_set *set1,
	__isl_take isl_set *set2);
__isl_give isl_basic_set *isl_basic_set_neg(__isl_take isl_basic_set *bset);
__isl_give isl_set *isl_set_neg(__isl_take isl_set *set);

__isl_give isl_set *isl_set_make_disjoint(__isl_take isl_set *set);
struct isl_set *isl_basic_set_compute_divs(struct isl_basic_set *bset);
__isl_give isl_set *isl_set_compute_divs(__isl_take isl_set *set);
__isl_give isl_set *isl_set_align_divs(__isl_take isl_set *set);

struct isl_basic_set *isl_set_copy_basic_set(struct isl_set *set);
struct isl_set *isl_set_drop_basic_set(struct isl_set *set,
						struct isl_basic_set *bset);

int isl_basic_set_plain_dim_is_fixed(__isl_keep isl_basic_set *bset,
	unsigned dim, isl_int *val);

int isl_set_plain_is_fixed(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned pos, isl_int *val);
int isl_set_plain_dim_is_fixed(__isl_keep isl_set *set,
	unsigned dim, isl_int *val);
int isl_set_fast_dim_is_fixed(__isl_keep isl_set *set,
	unsigned dim, isl_int *val);
int isl_set_plain_dim_has_fixed_lower_bound(__isl_keep isl_set *set,
	unsigned dim, isl_int *val);
int isl_set_dim_is_bounded(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned pos);
int isl_set_dim_has_lower_bound(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned pos);
int isl_set_dim_has_upper_bound(__isl_keep isl_set *set,
	enum isl_dim_type type, unsigned pos);

__isl_export
__isl_give isl_basic_set *isl_basic_set_gist(__isl_take isl_basic_set *bset,
					    __isl_take isl_basic_set *context);
__isl_give isl_set *isl_set_gist_basic_set(__isl_take isl_set *set,
	__isl_take isl_basic_set *context);
__isl_export
__isl_give isl_set *isl_set_gist(__isl_take isl_set *set,
	__isl_take isl_set *context);
__isl_give isl_set *isl_set_gist_params(__isl_take isl_set *set,
	__isl_take isl_set *context);
int isl_basic_set_dim_residue_class(struct isl_basic_set *bset,
	int pos, isl_int *modulo, isl_int *residue);
int isl_set_dim_residue_class(struct isl_set *set,
	int pos, isl_int *modulo, isl_int *residue);

__isl_export
__isl_give isl_set *isl_set_coalesce(__isl_take isl_set *set);

int isl_set_plain_is_equal(__isl_keep isl_set *set1, __isl_keep isl_set *set2);
int isl_set_fast_is_equal(__isl_keep isl_set *set1, __isl_keep isl_set *set2);
int isl_set_plain_is_disjoint(__isl_keep isl_set *set1,
	__isl_keep isl_set *set2);
int isl_set_fast_is_disjoint(__isl_keep isl_set *set1,
	__isl_keep isl_set *set2);

uint32_t isl_set_get_hash(struct isl_set *set);

int isl_set_dim_is_unique(struct isl_set *set, unsigned dim);

int isl_set_n_basic_set(__isl_keep isl_set *set);
__isl_export
int isl_set_foreach_basic_set(__isl_keep isl_set *set,
	int (*fn)(__isl_take isl_basic_set *bset, void *user), void *user);

int isl_set_foreach_point(__isl_keep isl_set *set,
	int (*fn)(__isl_take isl_point *pnt, void *user), void *user);
int isl_set_count(__isl_keep isl_set *set, isl_int *count);
int isl_basic_set_count_upto(__isl_keep isl_basic_set *bset,
	isl_int max, isl_int *count);
int isl_set_count_upto(__isl_keep isl_set *set, isl_int max, isl_int *count);

__isl_give isl_basic_set *isl_basic_set_from_point(__isl_take isl_point *pnt);
__isl_give isl_set *isl_set_from_point(__isl_take isl_point *pnt);
__isl_give isl_basic_set *isl_basic_set_box_from_points(
	__isl_take isl_point *pnt1, __isl_take isl_point *pnt2);
__isl_give isl_set *isl_set_box_from_points(__isl_take isl_point *pnt1,
	__isl_take isl_point *pnt2);

__isl_give isl_basic_set *isl_basic_set_lift(__isl_take isl_basic_set *bset);
__isl_give isl_set *isl_set_lift(__isl_take isl_set *set);

__isl_give isl_map *isl_set_lex_le_set(__isl_take isl_set *set1,
	__isl_take isl_set *set2);
__isl_give isl_map *isl_set_lex_lt_set(__isl_take isl_set *set1,
	__isl_take isl_set *set2);
__isl_give isl_map *isl_set_lex_ge_set(__isl_take isl_set *set1,
	__isl_take isl_set *set2);
__isl_give isl_map *isl_set_lex_gt_set(__isl_take isl_set *set1,
	__isl_take isl_set *set2);

int isl_set_size(__isl_keep isl_set *set);

__isl_give isl_set *isl_set_align_params(__isl_take isl_set *set,
	__isl_take isl_space *model);

__isl_give isl_mat *isl_basic_set_equalities_matrix(
	__isl_keep isl_basic_set *bset, enum isl_dim_type c1,
	enum isl_dim_type c2, enum isl_dim_type c3, enum isl_dim_type c4);
__isl_give isl_mat *isl_basic_set_inequalities_matrix(
	__isl_keep isl_basic_set *bset, enum isl_dim_type c1,
	enum isl_dim_type c2, enum isl_dim_type c3, enum isl_dim_type c4);
__isl_give isl_basic_set *isl_basic_set_from_constraint_matrices(
	__isl_take isl_space *dim,
	__isl_take isl_mat *eq, __isl_take isl_mat *ineq, enum isl_dim_type c1,
	enum isl_dim_type c2, enum isl_dim_type c3, enum isl_dim_type c4);

__isl_give isl_mat *isl_basic_set_reduced_basis(__isl_keep isl_basic_set *bset);

__isl_give isl_basic_set *isl_basic_set_coefficients(
	__isl_take isl_basic_set *bset);
__isl_give isl_basic_set *isl_set_coefficients(__isl_take isl_set *set);
__isl_give isl_basic_set *isl_basic_set_solutions(
	__isl_take isl_basic_set *bset);
__isl_give isl_basic_set *isl_set_solutions(__isl_take isl_set *set);

__isl_give isl_pw_aff *isl_set_dim_max(__isl_take isl_set *set, int pos);
__isl_give isl_pw_aff *isl_set_dim_min(__isl_take isl_set *set, int pos);

__isl_give char *isl_set_to_str(__isl_keep isl_set *set);

#if defined(__cplusplus)
}
#endif

#include <isl/dim.h>

#endif
