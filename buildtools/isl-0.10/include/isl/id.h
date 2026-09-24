#ifndef ISL_ID_H
#define ISL_ID_H

#include <isl/ctx.h>
#include <isl/printer.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_id;
typedef struct isl_id isl_id;

isl_ctx *isl_id_get_ctx(__isl_keep isl_id *id);

__isl_give isl_id *isl_id_alloc(isl_ctx *ctx,
	__isl_keep const char *name, void *user);
__isl_give isl_id *isl_id_copy(isl_id *id);
void *isl_id_free(__isl_take isl_id *id);

void *isl_id_get_user(__isl_keep isl_id *id);
__isl_keep const char *isl_id_get_name(__isl_keep isl_id *id);

__isl_give isl_printer *isl_printer_print_id(__isl_take isl_printer *p,
	__isl_keep isl_id *id);
void isl_id_dump(__isl_keep isl_id *id);

#if defined(__cplusplus)
}
#endif

#endif
