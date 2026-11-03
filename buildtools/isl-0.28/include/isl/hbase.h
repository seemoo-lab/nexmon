#include <isl/ctx.h>
#include <isl/printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define ISL_xFN(TYPE,NAME) TYPE ## _ ## NAME
#define ISL_FN(TYPE,NAME) ISL_xFN(TYPE,NAME)

struct __isl_export ISL_HBASE;
typedef struct ISL_HBASE	ISL_HBASE;

__isl_constructor
__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,alloc)(isl_ctx *ctx, int min_size);
__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,copy)(__isl_keep ISL_HBASE *hbase);
__isl_null ISL_HBASE *ISL_FN(ISL_HBASE,free)(__isl_take ISL_HBASE *hbase);

isl_ctx *ISL_FN(ISL_HBASE,get_ctx)(__isl_keep ISL_HBASE *hbase);

__isl_export
__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,drop)(__isl_take ISL_HBASE *hbase,
	__isl_take ISL_KEY *key);

#ifdef ISL_HBASE_IS_EQUAL
__isl_export
isl_bool ISL_HBASE_IS_EQUAL(__isl_keep ISL_HBASE *hbase1,
	__isl_keep ISL_HBASE *hbase2);
#endif

#ifdef ISL_HBASE_HAVE_READ_FROM_STR
__isl_constructor
__isl_give ISL_HBASE *ISL_FN(ISL_HBASE,read_from_str)(isl_ctx *ctx,
	const char *str);
#endif
__isl_give char *ISL_FN(ISL_HBASE,to_str)(__isl_keep ISL_HBASE *hbase);
__isl_give isl_printer *ISL_FN(isl_printer_print,ISL_HBASE_SUFFIX)(
	__isl_take isl_printer *p, __isl_keep ISL_HBASE *hbase);
void ISL_FN(ISL_HBASE,dump)(__isl_keep ISL_HBASE *hbase);

#undef ISL_xFN
#undef ISL_FN

#if defined(__cplusplus)
}
#endif
