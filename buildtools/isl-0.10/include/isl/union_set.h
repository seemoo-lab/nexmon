#ifndef ISL_UNION_SET_H
#define ISL_UNION_SET_H

#include <isl/point.h>
#include <isl/union_map.h>

#if defined(__cplusplus)
extern "C" {
#endif

__isl_constructor
__isl_give isl_union_set *isl_union_set_from_set(__isl_take isl_set *set);
__isl_give isl_union_set *isl_union_set_empty(__isl_take isl_space *dim);
__isl_give isl_union_set *isl_union_set_copy(__isl_keep isl_union_set *uset);
void *isl_union_set_free(__isl_take isl_union_set *uset);

isl_ctx *isl_union_set_get_ctx(__isl_keep isl_union_set *uset);
__isl_give isl_space *isl_union_set_get_space(__isl_keep isl_union_set *uset);

__isl_give isl_union_set *isl_union_set_universe(
	__isl_take isl_union_set *uset);
__isl_give isl_set *isl_union_set_params(__isl_take isl_union_set *uset);

__isl_export
__isl_give isl_union_set *isl_union_set_detect_equalities(
	__isl_take isl_union_set *uset);
__isl_export
__isl_give isl_union_set *isl_union_set_affine_hull(
	__isl_take isl_union_set *uset);
__isl_export
__isl_give isl_union_set *isl_union_set_polyhedral_hull(
	__isl_take isl_union_set *uset);
__isl_give isl_union_set *isl_union_set_simple_hull(
	__isl_take isl_union_set *uset);
__isl_export
__isl_give isl_union_set *isl_union_set_coalesce(
	__isl_take isl_union_set *uset);
__isl_give isl_union_set *isl_union_set_compute_divs(
	__isl_take isl_union_set *uset);
__isl_export
__isl_give isl_union_set *isl_union_set_lexmin(__isl_take isl_union_set *uset);
__isl_export
__isl_give isl_union_set *isl_union_set_lexmax(__isl_take isl_union_set *uset);

__isl_give isl_union_set *isl_union_set_add_set(__isl_take isl_union_set *uset,
	__isl_take isl_set *set);
__isl_give isl_union_set *isl_union_set_union(__isl_take isl_union_set *uset1,
	__isl_take isl_union_set *uset2);
__isl_export
__isl_give isl_union_set *isl_union_set_subtract(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2);
__isl_export
__isl_give isl_union_set *isl_union_set_intersect(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2);
__isl_export
__isl_give isl_union_set *isl_union_set_intersect_params(
	__isl_take isl_union_set *uset, __isl_take isl_set *set);
__isl_give isl_union_set *isl_union_set_product(__isl_take isl_union_set *uset1,
	__isl_take isl_union_set *uset2);
__isl_export
__isl_give isl_union_set *isl_union_set_gist(__isl_take isl_union_set *uset,
	__isl_take isl_union_set *context);
__isl_export
__isl_give isl_union_set *isl_union_set_gist_params(
	__isl_take isl_union_set *uset, __isl_take isl_set *set);

__isl_export
__isl_give isl_union_set *isl_union_set_apply(
	__isl_take isl_union_set *uset, __isl_take isl_union_map *umap);

int isl_union_set_is_params(__isl_keep isl_union_set *uset);
__isl_export
int isl_union_set_is_empty(__isl_keep isl_union_set *uset);

__isl_export
int isl_union_set_is_subset(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2);
__isl_export
int isl_union_set_is_equal(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2);
__isl_export
int isl_union_set_is_strict_subset(__isl_keep isl_union_set *uset1,
	__isl_keep isl_union_set *uset2);

int isl_union_set_n_set(__isl_keep isl_union_set *uset);
__isl_export
int isl_union_set_foreach_set(__isl_keep isl_union_set *uset,
	int (*fn)(__isl_take isl_set *set, void *user), void *user);
__isl_give int isl_union_set_contains(__isl_keep isl_union_set *uset,
	__isl_keep isl_space *dim);
__isl_give isl_set *isl_union_set_extract_set(__isl_keep isl_union_set *uset,
	__isl_take isl_space *dim);
__isl_give isl_set *isl_set_from_union_set(__isl_take isl_union_set *uset);
int isl_union_set_foreach_point(__isl_keep isl_union_set *uset,
	int (*fn)(__isl_take isl_point *pnt, void *user), void *user);

__isl_give isl_basic_set *isl_union_set_sample(__isl_take isl_union_set *uset);

__isl_give isl_union_set *isl_union_set_lift(__isl_take isl_union_set *uset);

__isl_give isl_union_map *isl_union_set_lex_lt_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2);
__isl_give isl_union_map *isl_union_set_lex_le_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2);
__isl_give isl_union_map *isl_union_set_lex_gt_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2);
__isl_give isl_union_map *isl_union_set_lex_ge_union_set(
	__isl_take isl_union_set *uset1, __isl_take isl_union_set *uset2);

__isl_give isl_union_set *isl_union_set_coefficients(
	__isl_take isl_union_set *bset);
__isl_give isl_union_set *isl_union_set_solutions(
	__isl_take isl_union_set *bset);

__isl_give isl_union_set *isl_union_set_read_from_file(isl_ctx *ctx,
	FILE *input);
__isl_constructor
__isl_give isl_union_set *isl_union_set_read_from_str(isl_ctx *ctx,
	const char *str);
__isl_give isl_printer *isl_printer_print_union_set(__isl_take isl_printer *p,
	__isl_keep isl_union_set *uset);
void isl_union_set_dump(__isl_keep isl_union_set *uset);

#if defined(__cplusplus)
}
#endif

#include <isl/dim.h>

#endif
