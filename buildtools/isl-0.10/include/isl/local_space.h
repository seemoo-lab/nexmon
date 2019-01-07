#ifndef ISL_LOCAL_SPACE_H
#define ISL_LOCAL_SPACE_H

#include <isl/aff_type.h>
#include <isl/space.h>
#include <isl/printer.h>
#include <isl/map_type.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_local_space;
typedef struct isl_local_space isl_local_space;

isl_ctx *isl_local_space_get_ctx(__isl_keep isl_local_space *ls);

__isl_give isl_local_space *isl_local_space_from_space(__isl_take isl_space *dim);

__isl_give isl_local_space *isl_local_space_copy(
	__isl_keep isl_local_space *ls);
void *isl_local_space_free(__isl_take isl_local_space *ls);

int isl_local_space_is_set(__isl_keep isl_local_space *ls);

int isl_local_space_dim(__isl_keep isl_local_space *ls,
	enum isl_dim_type type);
int isl_local_space_has_dim_name(__isl_keep isl_local_space *ls,
	enum isl_dim_type type, unsigned pos);
const char *isl_local_space_get_dim_name(__isl_keep isl_local_space *ls,
	enum isl_dim_type type, unsigned pos);
__isl_give isl_local_space *isl_local_space_set_dim_name(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned pos, const char *s);
__isl_give isl_local_space *isl_local_space_set_dim_id(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned pos, __isl_take isl_id *id);
__isl_give isl_space *isl_local_space_get_space(__isl_keep isl_local_space *ls);
__isl_give isl_aff *isl_local_space_get_div(__isl_keep isl_local_space *ls,
	int pos);

__isl_give isl_local_space *isl_local_space_domain(
	__isl_take isl_local_space *ls);
__isl_give isl_local_space *isl_local_space_range(
	__isl_take isl_local_space *ls);
__isl_give isl_local_space *isl_local_space_from_domain(
	__isl_take isl_local_space *ls);
__isl_give isl_local_space *isl_local_space_add_dims(
	__isl_take isl_local_space *ls, enum isl_dim_type type, unsigned n);
__isl_give isl_local_space *isl_local_space_drop_dims(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned first, unsigned n);
__isl_give isl_local_space *isl_local_space_insert_dims(
	__isl_take isl_local_space *ls,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_local_space *isl_local_space_intersect(
	__isl_take isl_local_space *ls1, __isl_take isl_local_space *ls2);

int isl_local_space_is_equal(__isl_keep isl_local_space *ls1,
	__isl_keep isl_local_space *ls2);

__isl_give isl_basic_map *isl_local_space_lifting(
	__isl_take isl_local_space *ls);

__isl_give isl_printer *isl_printer_print_local_space(__isl_take isl_printer *p,
	__isl_keep isl_local_space *ls);
void isl_local_space_dump(__isl_keep isl_local_space *ls);

#if defined(__cplusplus)
}
#endif

#endif
