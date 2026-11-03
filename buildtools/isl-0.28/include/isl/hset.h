#include <isl/ctx.h>

#define ISL_KEY				ISL_HSET_EL

#define ISL_HBASE			ISL_HSET
#define ISL_HBASE_SUFFIX		ISL_HSET_SUFFIX
#ifdef ISL_HSET_IS_EQUAL
#define ISL_HBASE_IS_EQUAL		ISL_HSET_IS_EQUAL
#endif
#ifdef ISL_HSET_HAVE_READ_FROM_STR
#define ISL_HBASE_HAVE_READ_FROM_STR
#endif

#include <isl/hbase.h>

#undef ISL_HBASE
#undef ISL_HBASE_SUFFIX
#undef ISL_HBASE_IS_EQUAL
#undef ISL_HBASE_HAVE_READ_FROM_STR

#undef ISL_KEY

#if defined(__cplusplus)
extern "C" {
#endif

#define ISL_xFN(TYPE,NAME) TYPE ## _ ## NAME
#define ISL_FN(TYPE,NAME) ISL_xFN(TYPE,NAME)

isl_bool ISL_FN(ISL_HSET,has)(__isl_keep ISL_HSET *hset,
	__isl_keep ISL_HSET_EL *el);
__isl_export
__isl_give ISL_HSET *ISL_FN(ISL_HSET,insert)(__isl_take ISL_HSET *hset,
	__isl_take ISL_HSET_EL *el);

#undef ISL_xFN
#undef ISL_FN

#if defined(__cplusplus)
}
#endif
