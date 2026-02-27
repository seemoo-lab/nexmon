#ifndef ISL_MULTI_H
#define ISL_MULTI_H

#include <isl/list.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ISL_DECLARE_MULTI(BASE)						\
__isl_give isl_multi_##BASE *isl_multi_##BASE##_from_##BASE##_list(	\
	__isl_take isl_space *space, __isl_take isl_##BASE##_list *list); \
const char *isl_multi_##BASE##_get_tuple_name(				\
	__isl_keep isl_multi_##BASE *multi, enum isl_dim_type type);	\
__isl_give isl_multi_##BASE *isl_multi_##BASE##_set_##BASE(		\
	__isl_take isl_multi_##BASE *multi, int pos,			\
	__isl_take isl_##BASE *el);

ISL_DECLARE_MULTI(aff)

#if defined(__cplusplus)
}
#endif

#endif
