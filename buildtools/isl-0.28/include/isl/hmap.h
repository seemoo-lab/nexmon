#include <isl/ctx.h>
#include <isl/maybe.h>

#define ISL_HBASE			ISL_HMAP
#define ISL_HBASE_SUFFIX		ISL_HMAP_SUFFIX
#ifdef ISL_HMAP_IS_EQUAL
#define ISL_HBASE_IS_EQUAL		ISL_HMAP_IS_EQUAL
#endif
#ifdef ISL_HMAP_HAVE_READ_FROM_STR
#define ISL_HBASE_HAVE_READ_FROM_STR
#endif

#include <isl/hbase.h>

#undef ISL_HBASE
#undef ISL_HBASE_SUFFIX
#undef ISL_HBASE_IS_EQUAL
#undef ISL_HBASE_HAVE_READ_FROM_STR

#if defined(__cplusplus)
extern "C" {
#endif

#define ISL_xCAT(A,B) A ## B
#define ISL_CAT(A,B) ISL_xCAT(A,B)
#define ISL_xFN(TYPE,NAME) TYPE ## _ ## NAME
#define ISL_FN(TYPE,NAME) ISL_xFN(TYPE,NAME)

__isl_give ISL_MAYBE(ISL_VAL) ISL_FN(ISL_HMAP,try_get)(
	__isl_keep ISL_HMAP *hmap, __isl_keep ISL_KEY *key);
isl_bool ISL_FN(ISL_HMAP,has)(__isl_keep ISL_HMAP *hmap,
	__isl_keep ISL_KEY *key);
__isl_give ISL_VAL *ISL_FN(ISL_HMAP,get)(__isl_keep ISL_HMAP *hmap,
	__isl_take ISL_KEY *key);
__isl_export
__isl_give ISL_HMAP *ISL_FN(ISL_HMAP,set)(__isl_take ISL_HMAP *hmap,
	__isl_take ISL_KEY *key, __isl_take ISL_VAL *val);

isl_stat ISL_FN(ISL_HMAP,foreach)(__isl_keep ISL_HMAP *hmap,
	isl_stat (*fn)(__isl_take ISL_KEY *key, __isl_take ISL_VAL *val,
		void *user),
	void *user);
isl_bool ISL_FN(ISL_HMAP,every)(__isl_keep ISL_HMAP *hmap,
	isl_bool (*test)(__isl_keep ISL_KEY *key, __isl_keep ISL_VAL *val,
		void *user),
	void *user);

#undef ISL_xCAT
#undef ISL_CAT
#undef ISL_KEY
#undef ISL_VAL
#undef ISL_xFN
#undef ISL_FN
#undef ISL_xHMAP
#undef ISL_yHMAP
#undef ISL_HMAP

#if defined(__cplusplus)
}
#endif
