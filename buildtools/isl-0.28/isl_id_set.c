#include <isl/id.h>
#include <isl/id_set.h>

#define isl_id_is_equal(id1,id2)	isl_bool_ok(id1 == id2)

#define ISL_HSET		isl_id_set
#define ISL_HSET_SUFFIX		id_set
#define ISL_HSET_EL		isl_id
#define ISL_HSET_IS_EQUAL	isl_id_set_is_equal
#define ISL_HSET_EL_IS_EQUAL	isl_id_is_equal
#define ISL_HSET_EL_PRINT	isl_printer_print_id
#define ISL_HSET_HAVE_READ_FROM_STR
#define ISL_HSET_EL_READ	isl_stream_read_id

#include <isl/hset_templ.c>
