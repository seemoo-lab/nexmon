#ifndef ISL_UNION_MAP_H
#define ISL_UNION_MAP_H

#include <isl/space.h>
#include <isl/map_type.h>
#include <isl/union_map_type.h>
#include <isl/printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

__isl_constructor
__isl_give isl_union_map *isl_union_map_from_map(__isl_take isl_map *map);
__isl_give isl_union_map *isl_union_map_empty(__isl_take isl_space *dim);
__isl_give isl_union_map *isl_union_map_copy(__isl_keep isl_union_map *umap);
void *isl_union_map_free(__isl_take isl_union_map *umap);

isl_ctx *isl_union_map_get_ctx(__isl_keep isl_union_map *umap);
__isl_give isl_space *isl_union_map_get_space(__isl_keep isl_union_map *umap);

__isl_give isl_union_map *isl_union_map_universe(
	__isl_take isl_union_map *umap);
__isl_give isl_set *isl_union_map_params(__isl_take isl_union_map *umap);
__isl_give isl_union_set *isl_union_map_domain(__isl_take isl_union_map *umap);
__isl_give isl_union_set *isl_union_map_range(__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_map_domain_map(
	__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_map_range_map(
	__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_map_from_domain(
	__isl_take isl_union_set *uset);
__isl_give isl_union_map *isl_union_map_from_range(
	__isl_take isl_union_set *uset);

__isl_export
__isl_give isl_union_map *isl_union_map_affine_hull(
	__isl_take isl_union_map *umap);
__isl_export
__isl_give isl_union_map *isl_union_map_polyhedral_hull(
	__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_map_simple_hull(
	__isl_take isl_union_map *umap);
__isl_export
__isl_give isl_union_map *isl_union_map_coalesce(
	__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_map_compute_divs(
	__isl_take isl_union_map *umap);
__isl_export
__isl_give isl_union_map *isl_union_map_lexmin(__isl_take isl_union_map *umap);
__isl_export
__isl_give isl_union_map *isl_union_map_lexmax(__isl_take isl_union_map *umap);

__isl_give isl_union_map *isl_union_map_add_map(__isl_take isl_union_map *umap,
	__isl_take isl_map *map);
__isl_export
__isl_give isl_union_map *isl_union_map_union(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2);
__isl_export
__isl_give isl_union_map *isl_union_map_subtract(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_export
__isl_give isl_union_map *isl_union_map_intersect(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_export
__isl_give isl_union_map *isl_union_map_intersect_params(
	__isl_take isl_union_map *umap, __isl_take isl_set *set);
__isl_give isl_union_map *isl_union_map_product(__isl_take isl_union_map *umap1,
	__isl_take isl_union_map *umap2);
__isl_give isl_union_map *isl_union_map_range_product(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_give isl_union_map *isl_union_map_flat_range_product(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_export
__isl_give isl_union_map *isl_union_map_gist(__isl_take isl_union_map *umap,
	__isl_take isl_union_map *context);
__isl_export
__isl_give isl_union_map *isl_union_map_gist_params(
	__isl_take isl_union_map *umap, __isl_take isl_set *set);
__isl_export
__isl_give isl_union_map *isl_union_map_gist_domain(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset);
__isl_export
__isl_give isl_union_map *isl_union_map_gist_range(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset);

__isl_export
__isl_give isl_union_map *isl_union_map_intersect_domain(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset);
__isl_export
__isl_give isl_union_map *isl_union_map_intersect_range(
	__isl_take isl_union_map *umap, __isl_take isl_union_set *uset);

__isl_export
__isl_give isl_union_map *isl_union_map_apply_domain(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_export
__isl_give isl_union_map *isl_union_map_apply_range(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_export
__isl_give isl_union_map *isl_union_map_reverse(__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_map_from_domain_and_range(
	__isl_take isl_union_set *domain, __isl_take isl_union_set *range);

__isl_export
__isl_give isl_union_map *isl_union_map_detect_equalities(
	__isl_keep isl_union_map *umap);
__isl_export
__isl_give isl_union_set *isl_union_map_deltas(__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_map_deltas_map(
	__isl_take isl_union_map *umap);
__isl_export
__isl_give isl_union_map *isl_union_set_identity(__isl_take isl_union_set *uset);

__isl_export
int isl_union_map_is_empty(__isl_keep isl_union_map *umap);
__isl_export
int isl_union_map_is_single_valued(__isl_keep isl_union_map *umap);
int isl_union_map_plain_is_injective(__isl_keep isl_union_map *umap);
__isl_export
int isl_union_map_is_injective(__isl_keep isl_union_map *umap);
__isl_export
int isl_union_map_is_bijective(__isl_keep isl_union_map *umap);

__isl_export
int isl_union_map_is_subset(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2);
__isl_export
int isl_union_map_is_equal(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2);
__isl_export
int isl_union_map_is_strict_subset(__isl_keep isl_union_map *umap1,
	__isl_keep isl_union_map *umap2);

int isl_union_map_n_map(__isl_keep isl_union_map *umap);
__isl_export
int isl_union_map_foreach_map(__isl_keep isl_union_map *umap,
	int (*fn)(__isl_take isl_map *map, void *user), void *user);
__isl_give int isl_union_map_contains(__isl_keep isl_union_map *umap,
	__isl_keep isl_space *dim);
__isl_give isl_map *isl_union_map_extract_map(__isl_keep isl_union_map *umap,
	__isl_take isl_space *dim);
__isl_give isl_map *isl_map_from_union_map(__isl_take isl_union_map *umap);

__isl_give isl_basic_map *isl_union_map_sample(__isl_take isl_union_map *umap);

__isl_give isl_union_map *isl_union_map_fixed_power(
	__isl_take isl_union_map *umap, isl_int exp);
__isl_give isl_union_map *isl_union_map_power(__isl_take isl_union_map *umap,
	int *exact);
__isl_give isl_union_map *isl_union_map_transitive_closure(
	__isl_take isl_union_map *umap, int *exact);

__isl_give isl_union_map *isl_union_map_lex_lt_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_give isl_union_map *isl_union_map_lex_le_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_give isl_union_map *isl_union_map_lex_gt_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);
__isl_give isl_union_map *isl_union_map_lex_ge_union_map(
	__isl_take isl_union_map *umap1, __isl_take isl_union_map *umap2);

__isl_give isl_union_map *isl_union_map_read_from_file(isl_ctx *ctx,
	FILE *input);
__isl_constructor
__isl_give isl_union_map *isl_union_map_read_from_str(isl_ctx *ctx,
	const char *str);
__isl_give isl_printer *isl_printer_print_union_map(__isl_take isl_printer *p,
	__isl_keep isl_union_map *umap);
void isl_union_map_dump(__isl_keep isl_union_map *umap);

__isl_give isl_union_set *isl_union_map_wrap(__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_set_unwrap(__isl_take isl_union_set *uset);

__isl_give isl_union_map *isl_union_map_zip(__isl_take isl_union_map *umap);
__isl_give isl_union_map *isl_union_map_curry(__isl_take isl_union_map *umap);

__isl_give isl_union_map *isl_union_map_align_params(
	__isl_take isl_union_map *umap, __isl_take isl_space *model);
__isl_give isl_union_set *isl_union_set_align_params(
	__isl_take isl_union_set *uset, __isl_take isl_space *model);

#if defined(__cplusplus)
}
#endif

#include <isl/dim.h>

#endif
