#ifndef ISL_AFF_H
#define ISL_AFF_H

#include <isl/local_space.h>
#include <isl/printer.h>
#include <isl/set_type.h>
#include <isl/aff_type.h>
#include <isl/list.h>
#include <isl/multi.h>
#include <isl/union_set_type.h>

#if defined(__cplusplus)
extern "C" {
#endif

__isl_give isl_aff *isl_aff_zero_on_domain(__isl_take isl_local_space *ls);

__isl_give isl_aff *isl_aff_copy(__isl_keep isl_aff *aff);
void *isl_aff_free(__isl_take isl_aff *aff);

isl_ctx *isl_aff_get_ctx(__isl_keep isl_aff *aff);

int isl_aff_dim(__isl_keep isl_aff *aff, enum isl_dim_type type);
int isl_aff_involves_dims(__isl_keep isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_space *isl_aff_get_domain_space(__isl_keep isl_aff *aff);
__isl_give isl_space *isl_aff_get_space(__isl_keep isl_aff *aff);
__isl_give isl_local_space *isl_aff_get_domain_local_space(
	__isl_keep isl_aff *aff);
__isl_give isl_local_space *isl_aff_get_local_space(__isl_keep isl_aff *aff);

const char *isl_aff_get_dim_name(__isl_keep isl_aff *aff,
	enum isl_dim_type type, unsigned pos);
int isl_aff_get_constant(__isl_keep isl_aff *aff, isl_int *v);
int isl_aff_get_coefficient(__isl_keep isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int *v);
int isl_aff_get_denominator(__isl_keep isl_aff *aff, isl_int *v);
__isl_give isl_aff *isl_aff_set_constant(__isl_take isl_aff *aff, isl_int v);
__isl_give isl_aff *isl_aff_set_constant_si(__isl_take isl_aff *aff, int v);
__isl_give isl_aff *isl_aff_set_coefficient(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int v);
__isl_give isl_aff *isl_aff_set_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v);
__isl_give isl_aff *isl_aff_set_denominator(__isl_take isl_aff *aff, isl_int v);
__isl_give isl_aff *isl_aff_add_constant(__isl_take isl_aff *aff, isl_int v);
__isl_give isl_aff *isl_aff_add_constant_si(__isl_take isl_aff *aff, int v);
__isl_give isl_aff *isl_aff_add_coefficient(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, isl_int v);
__isl_give isl_aff *isl_aff_add_coefficient_si(__isl_take isl_aff *aff,
	enum isl_dim_type type, int pos, int v);

int isl_aff_is_cst(__isl_keep isl_aff *aff);

__isl_give isl_aff *isl_aff_set_dim_name(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned pos, const char *s);
__isl_give isl_aff *isl_aff_set_dim_id(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned pos, __isl_take isl_id *id);

int isl_aff_plain_is_equal(__isl_keep isl_aff *aff1, __isl_keep isl_aff *aff2);
int isl_aff_plain_is_zero(__isl_keep isl_aff *aff);

__isl_give isl_aff *isl_aff_get_div(__isl_keep isl_aff *aff, int pos);

__isl_give isl_aff *isl_aff_neg(__isl_take isl_aff *aff);
__isl_give isl_aff *isl_aff_ceil(__isl_take isl_aff *aff);
__isl_give isl_aff *isl_aff_floor(__isl_take isl_aff *aff);
__isl_give isl_aff *isl_aff_mod(__isl_take isl_aff *aff, isl_int mod);

__isl_give isl_aff *isl_aff_mul(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2);
__isl_give isl_aff *isl_aff_add(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2);
__isl_give isl_aff *isl_aff_sub(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2);

__isl_give isl_aff *isl_aff_scale(__isl_take isl_aff *aff, isl_int f);
__isl_give isl_aff *isl_aff_scale_down(__isl_take isl_aff *aff, isl_int f);
__isl_give isl_aff *isl_aff_scale_down_ui(__isl_take isl_aff *aff, unsigned f);

__isl_give isl_aff *isl_aff_insert_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_aff *isl_aff_add_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned n);
__isl_give isl_aff *isl_aff_drop_dims(__isl_take isl_aff *aff,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_aff *isl_aff_project_domain_on_params(__isl_take isl_aff *aff);

__isl_give isl_aff *isl_aff_align_params(__isl_take isl_aff *aff,
	__isl_take isl_space *model);

__isl_give isl_aff *isl_aff_gist(__isl_take isl_aff *aff,
	__isl_take isl_set *context);
__isl_give isl_aff *isl_aff_gist_params(__isl_take isl_aff *aff,
	__isl_take isl_set *context);

__isl_give isl_basic_set *isl_aff_le_basic_set(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2);
__isl_give isl_basic_set *isl_aff_ge_basic_set(__isl_take isl_aff *aff1,
	__isl_take isl_aff *aff2);

__isl_give isl_aff *isl_aff_read_from_str(isl_ctx *ctx, const char *str);
__isl_give isl_printer *isl_printer_print_aff(__isl_take isl_printer *p,
	__isl_keep isl_aff *aff);
void isl_aff_dump(__isl_keep isl_aff *aff);

isl_ctx *isl_pw_aff_get_ctx(__isl_keep isl_pw_aff *pwaff);
__isl_give isl_space *isl_pw_aff_get_domain_space(__isl_keep isl_pw_aff *pwaff);
__isl_give isl_space *isl_pw_aff_get_space(__isl_keep isl_pw_aff *pwaff);

__isl_give isl_pw_aff *isl_pw_aff_from_aff(__isl_take isl_aff *aff);
__isl_give isl_pw_aff *isl_pw_aff_empty(__isl_take isl_space *dim);
__isl_give isl_pw_aff *isl_pw_aff_alloc(__isl_take isl_set *set,
	__isl_take isl_aff *aff);

__isl_give isl_pw_aff *isl_set_indicator_function(__isl_take isl_set *set);

const char *isl_pw_aff_get_dim_name(__isl_keep isl_pw_aff *pa,
	enum isl_dim_type type, unsigned pos);
int isl_pw_aff_has_dim_id(__isl_keep isl_pw_aff *pa,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_id *isl_pw_aff_get_dim_id(__isl_keep isl_pw_aff *pa,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_pw_aff *isl_pw_aff_set_dim_id(__isl_take isl_pw_aff *pma,
	enum isl_dim_type type, unsigned pos, __isl_take isl_id *id);

int isl_pw_aff_is_empty(__isl_keep isl_pw_aff *pwaff);
int isl_pw_aff_plain_is_equal(__isl_keep isl_pw_aff *pwaff1,
	__isl_keep isl_pw_aff *pwaff2);

__isl_give isl_pw_aff *isl_pw_aff_union_min(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_union_max(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_union_add(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);

__isl_give isl_pw_aff *isl_pw_aff_copy(__isl_keep isl_pw_aff *pwaff);
void *isl_pw_aff_free(__isl_take isl_pw_aff *pwaff);

unsigned isl_pw_aff_dim(__isl_keep isl_pw_aff *pwaff, enum isl_dim_type type);
int isl_pw_aff_involves_dims(__isl_keep isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned first, unsigned n);

int isl_pw_aff_is_cst(__isl_keep isl_pw_aff *pwaff);

__isl_give isl_pw_aff *isl_pw_aff_align_params(__isl_take isl_pw_aff *pwaff,
	__isl_take isl_space *model);

__isl_give isl_pw_aff *isl_pw_aff_set_tuple_id(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, __isl_take isl_id *id);

__isl_give isl_set *isl_pw_aff_domain(__isl_take isl_pw_aff *pwaff);

__isl_give isl_pw_aff *isl_pw_aff_min(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_max(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_mul(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_add(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_sub(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_pw_aff *isl_pw_aff_neg(__isl_take isl_pw_aff *pwaff);
__isl_give isl_pw_aff *isl_pw_aff_ceil(__isl_take isl_pw_aff *pwaff);
__isl_give isl_pw_aff *isl_pw_aff_floor(__isl_take isl_pw_aff *pwaff);
__isl_give isl_pw_aff *isl_pw_aff_mod(__isl_take isl_pw_aff *pwaff,
	isl_int mod);

__isl_give isl_pw_aff *isl_pw_aff_intersect_params(__isl_take isl_pw_aff *pa,
	__isl_take isl_set *set);
__isl_give isl_pw_aff *isl_pw_aff_intersect_domain(__isl_take isl_pw_aff *pa,
	__isl_take isl_set *set);

__isl_give isl_pw_aff *isl_pw_aff_cond(__isl_take isl_pw_aff *cond,
	__isl_take isl_pw_aff *pwaff_true, __isl_take isl_pw_aff *pwaff_false);

__isl_give isl_pw_aff *isl_pw_aff_scale(__isl_take isl_pw_aff *pwaff,
	isl_int f);
__isl_give isl_pw_aff *isl_pw_aff_scale_down(__isl_take isl_pw_aff *pwaff,
	isl_int f);

__isl_give isl_pw_aff *isl_pw_aff_insert_dims(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_pw_aff *isl_pw_aff_add_dims(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned n);
__isl_give isl_pw_aff *isl_pw_aff_drop_dims(__isl_take isl_pw_aff *pwaff,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_pw_aff *isl_pw_aff_coalesce(__isl_take isl_pw_aff *pwqp);
__isl_give isl_pw_aff *isl_pw_aff_gist(__isl_take isl_pw_aff *pwaff,
	__isl_take isl_set *context);
__isl_give isl_pw_aff *isl_pw_aff_gist_params(__isl_take isl_pw_aff *pwaff,
	__isl_take isl_set *context);

int isl_pw_aff_n_piece(__isl_keep isl_pw_aff *pwaff);
int isl_pw_aff_foreach_piece(__isl_keep isl_pw_aff *pwaff,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_aff *aff,
		    void *user), void *user);

__isl_give isl_set *isl_set_from_pw_aff(__isl_take isl_pw_aff *pwaff);
__isl_give isl_map *isl_map_from_pw_aff(__isl_take isl_pw_aff *pwaff);

__isl_give isl_set *isl_pw_aff_nonneg_set(__isl_take isl_pw_aff *pwaff);
__isl_give isl_set *isl_pw_aff_zero_set(__isl_take isl_pw_aff *pwaff);
__isl_give isl_set *isl_pw_aff_non_zero_set(__isl_take isl_pw_aff *pwaff);

__isl_give isl_set *isl_pw_aff_eq_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_ne_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_le_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_lt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_ge_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);
__isl_give isl_set *isl_pw_aff_gt_set(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2);

__isl_give isl_pw_aff *isl_pw_aff_read_from_str(isl_ctx *ctx, const char *str);
__isl_give isl_printer *isl_printer_print_pw_aff(__isl_take isl_printer *p,
	__isl_keep isl_pw_aff *pwaff);
void isl_pw_aff_dump(__isl_keep isl_pw_aff *pwaff);

__isl_give isl_pw_aff *isl_pw_aff_list_min(__isl_take isl_pw_aff_list *list);
__isl_give isl_pw_aff *isl_pw_aff_list_max(__isl_take isl_pw_aff_list *list);

__isl_give isl_set *isl_pw_aff_list_eq_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2);
__isl_give isl_set *isl_pw_aff_list_ne_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2);
__isl_give isl_set *isl_pw_aff_list_le_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2);
__isl_give isl_set *isl_pw_aff_list_lt_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2);
__isl_give isl_set *isl_pw_aff_list_ge_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2);
__isl_give isl_set *isl_pw_aff_list_gt_set(__isl_take isl_pw_aff_list *list1,
	__isl_take isl_pw_aff_list *list2);

__isl_give isl_multi_aff *isl_multi_aff_zero(__isl_take isl_space *space);

isl_ctx *isl_multi_aff_get_ctx(__isl_keep isl_multi_aff *maff);
__isl_give isl_space *isl_multi_aff_get_space(__isl_keep isl_multi_aff *maff);
__isl_give isl_multi_aff *isl_multi_aff_set_tuple_id(
	__isl_take isl_multi_aff *maff,
	enum isl_dim_type type, __isl_take isl_id *id);
__isl_give isl_multi_aff *isl_multi_aff_copy(__isl_keep isl_multi_aff *maff);
void *isl_multi_aff_free(__isl_take isl_multi_aff *maff);

unsigned isl_multi_aff_dim(__isl_keep isl_multi_aff *maff,
	enum isl_dim_type type);
__isl_give isl_aff *isl_multi_aff_get_aff(__isl_keep isl_multi_aff *multi,
	int pos);

__isl_give isl_multi_aff *isl_multi_aff_drop_dims(
	__isl_take isl_multi_aff *maff,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_multi_aff *isl_multi_aff_set_dim_name(
	__isl_take isl_multi_aff *maff,
	enum isl_dim_type type, unsigned pos, const char *s);

int isl_multi_aff_plain_is_equal(__isl_keep isl_multi_aff *maff1,
	__isl_keep isl_multi_aff *maff2);

__isl_give isl_multi_aff *isl_multi_aff_add(__isl_take isl_multi_aff *maff1,
	__isl_take isl_multi_aff *maff2);

__isl_give isl_multi_aff *isl_multi_aff_scale(__isl_take isl_multi_aff *maff,
	isl_int f);

__isl_give isl_multi_aff *isl_multi_aff_flat_range_product(
	__isl_take isl_multi_aff *ma1, __isl_take isl_multi_aff *ma2);

__isl_give isl_multi_aff *isl_multi_aff_gist_params(
	__isl_take isl_multi_aff *maff, __isl_take isl_set *context);
__isl_give isl_multi_aff *isl_multi_aff_gist(__isl_take isl_multi_aff *maff,
	__isl_take isl_set *context);

__isl_give isl_multi_aff *isl_multi_aff_lift(__isl_take isl_multi_aff *maff,
	__isl_give isl_local_space **ls);

__isl_give isl_printer *isl_printer_print_multi_aff(__isl_take isl_printer *p,
	__isl_keep isl_multi_aff *maff);

__isl_give isl_multi_aff *isl_multi_aff_read_from_str(isl_ctx *ctx,
		const char *str);
void isl_multi_aff_dump(__isl_keep isl_multi_aff *maff);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_from_multi_aff(
	__isl_take isl_multi_aff *ma);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_alloc(__isl_take isl_set *set,
	__isl_take isl_multi_aff *maff);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_copy(
	__isl_keep isl_pw_multi_aff *pma);
void *isl_pw_multi_aff_free(__isl_take isl_pw_multi_aff *pma);

unsigned isl_pw_multi_aff_dim(__isl_keep isl_pw_multi_aff *pma,
	enum isl_dim_type type);
__isl_give isl_pw_aff *isl_pw_multi_aff_get_pw_aff(
	__isl_keep isl_pw_multi_aff *pma, int pos);

isl_ctx *isl_pw_multi_aff_get_ctx(__isl_keep isl_pw_multi_aff *pma);
__isl_give isl_space *isl_pw_multi_aff_get_domain_space(
	__isl_keep isl_pw_multi_aff *pma);
__isl_give isl_space *isl_pw_multi_aff_get_space(
	__isl_keep isl_pw_multi_aff *pma);
const char *isl_pw_multi_aff_get_tuple_name(__isl_keep isl_pw_multi_aff *pma,
	enum isl_dim_type type);
__isl_give isl_id *isl_pw_multi_aff_get_tuple_id(
	__isl_keep isl_pw_multi_aff *pma, enum isl_dim_type type);
int isl_pw_multi_aff_has_tuple_id(__isl_keep isl_pw_multi_aff *pma,
	enum isl_dim_type type);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_set_tuple_id(
	__isl_take isl_pw_multi_aff *pma,
	enum isl_dim_type type, __isl_take isl_id *id);

__isl_give isl_set *isl_pw_multi_aff_domain(__isl_take isl_pw_multi_aff *pma);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_empty(__isl_take isl_space *space);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_from_domain(
	__isl_take isl_set *set);

const char *isl_pw_multi_aff_get_dim_name(__isl_keep isl_pw_multi_aff *pma,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_id *isl_pw_multi_aff_get_dim_id(
	__isl_keep isl_pw_multi_aff *pma, enum isl_dim_type type,
	unsigned pos);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_set_dim_id(
	__isl_take isl_pw_multi_aff *pma,
	enum isl_dim_type type, unsigned pos, __isl_take isl_id *id);

int isl_pw_multi_aff_plain_is_equal(__isl_keep isl_pw_multi_aff *pma1,
	__isl_keep isl_pw_multi_aff *pma2);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_union_add(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_add(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_flat_range_product(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_intersect_params(
	__isl_take isl_pw_multi_aff *pma, __isl_take isl_set *set);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_intersect_domain(
	__isl_take isl_pw_multi_aff *pma, __isl_take isl_set *set);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_coalesce(
	__isl_take isl_pw_multi_aff *pma);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_gist_params(
	__isl_take isl_pw_multi_aff *pma, __isl_take isl_set *set);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_gist(
	__isl_take isl_pw_multi_aff *pma, __isl_take isl_set *set);

int isl_pw_multi_aff_foreach_piece(__isl_keep isl_pw_multi_aff *pma,
	int (*fn)(__isl_take isl_set *set, __isl_take isl_multi_aff *maff,
		    void *user), void *user);

__isl_give isl_map *isl_map_from_pw_multi_aff(__isl_take isl_pw_multi_aff *pma);
__isl_give isl_set *isl_set_from_pw_multi_aff(__isl_take isl_pw_multi_aff *pma);

__isl_give isl_printer *isl_printer_print_pw_multi_aff(__isl_take isl_printer *p,
	__isl_keep isl_pw_multi_aff *pma);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_from_set(__isl_take isl_set *set);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_from_map(__isl_take isl_map *map);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_read_from_str(isl_ctx *ctx,
	const char *str);
void isl_pw_multi_aff_dump(__isl_keep isl_pw_multi_aff *pma);


__isl_give isl_union_pw_multi_aff *isl_union_pw_multi_aff_empty(
	__isl_take isl_space *space);
__isl_give isl_union_pw_multi_aff *isl_union_pw_multi_aff_from_domain(
	__isl_take isl_union_set *uset);
__isl_give isl_union_pw_multi_aff *isl_union_pw_multi_aff_copy(
	__isl_keep isl_union_pw_multi_aff *upma);
void *isl_union_pw_multi_aff_free(__isl_take isl_union_pw_multi_aff *upma);

__isl_give isl_union_pw_multi_aff *isl_union_pw_multi_aff_add_pw_multi_aff(
	__isl_take isl_union_pw_multi_aff *upma,
	__isl_take isl_pw_multi_aff *pma);

isl_ctx *isl_union_pw_multi_aff_get_ctx(
	__isl_keep isl_union_pw_multi_aff *upma);
__isl_give isl_space *isl_union_pw_multi_aff_get_space(
	__isl_keep isl_union_pw_multi_aff *upma);

int isl_union_pw_multi_aff_foreach_pw_multi_aff(
	__isl_keep isl_union_pw_multi_aff *upma,
	int (*fn)(__isl_take isl_pw_multi_aff *pma, void *user), void *user);

__isl_give isl_union_set *isl_union_pw_multi_aff_domain(
	__isl_take isl_union_pw_multi_aff *upma);

__isl_give isl_union_pw_multi_aff *isl_union_pw_multi_aff_add(
	__isl_take isl_union_pw_multi_aff *upma1,
	__isl_take isl_union_pw_multi_aff *upma2);

__isl_give isl_union_pw_multi_aff *isl_union_pw_multi_aff_flat_range_product(
	__isl_take isl_union_pw_multi_aff *upma1,
	__isl_take isl_union_pw_multi_aff *upma2);

__isl_give isl_union_map *isl_union_map_from_union_pw_multi_aff(
	__isl_take isl_union_pw_multi_aff *upma);

__isl_give isl_printer *isl_printer_print_union_pw_multi_aff(
	__isl_take isl_printer *p, __isl_keep isl_union_pw_multi_aff *upma);

#if defined(__cplusplus)
}
#endif

#include <isl/dim.h>

#endif
