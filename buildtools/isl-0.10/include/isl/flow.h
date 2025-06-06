#ifndef ISL_FLOW_H
#define ISL_FLOW_H

#include <isl/set_type.h>
#include <isl/map_type.h>
#include <isl/union_set_type.h>
#include <isl/union_map_type.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* Let n (>= 0) be the number of iterators shared by first and second.
 * If first precedes second textually return 2 * n + 1,
 * otherwise return 2 * n.
 */
typedef int (*isl_access_level_before)(void *first, void *second);

struct isl_restriction;
typedef struct isl_restriction isl_restriction;

void *isl_restriction_free(__isl_take isl_restriction *restr);
__isl_give isl_restriction *isl_restriction_empty(
	__isl_take isl_map *source_map);
__isl_give isl_restriction *isl_restriction_none(
	__isl_take isl_map *source_map);
__isl_give isl_restriction *isl_restriction_input(
	__isl_take isl_set *source_restr, __isl_take isl_set *sink_restr);
__isl_give isl_restriction *isl_restriction_output(
	__isl_take isl_set *source_restr);

isl_ctx *isl_restriction_get_ctx(__isl_keep isl_restriction *restr);

typedef __isl_give isl_restriction *(*isl_access_restrict)(
	__isl_keep isl_map *source_map, __isl_keep isl_set *sink,
	void *source_user, void *user);

struct isl_access_info;
typedef struct isl_access_info isl_access_info;
struct isl_flow;
typedef struct isl_flow isl_flow;

__isl_give isl_access_info *isl_access_info_alloc(__isl_take isl_map *sink,
	void *sink_user, isl_access_level_before fn, int max_source);
__isl_give isl_access_info *isl_access_info_set_restrict(
	__isl_take isl_access_info *acc, isl_access_restrict fn, void *user);
__isl_give isl_access_info *isl_access_info_add_source(
	__isl_take isl_access_info *acc, __isl_take isl_map *source,
	int must, void *source_user);
void isl_access_info_free(__isl_take isl_access_info *acc);

isl_ctx *isl_access_info_get_ctx(__isl_keep isl_access_info *acc);

__isl_give isl_flow *isl_access_info_compute_flow(__isl_take isl_access_info *acc);
int isl_flow_foreach(__isl_keep isl_flow *deps,
	int (*fn)(__isl_take isl_map *dep, int must, void *dep_user, void *user),
	void *user);
__isl_give isl_map *isl_flow_get_no_source(__isl_keep isl_flow *deps, int must);
void isl_flow_free(__isl_take isl_flow *deps);

isl_ctx *isl_flow_get_ctx(__isl_keep isl_flow *deps);

int isl_union_map_compute_flow(__isl_take isl_union_map *sink,
	__isl_take isl_union_map *must_source,
	__isl_take isl_union_map *may_source,
	__isl_take isl_union_map *schedule,
	__isl_give isl_union_map **must_dep, __isl_give isl_union_map **may_dep,
	__isl_give isl_union_map **must_no_source,
	__isl_give isl_union_map **may_no_source);

#if defined(__cplusplus)
}
#endif

#endif
