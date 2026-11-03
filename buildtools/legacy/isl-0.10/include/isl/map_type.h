#ifndef ISL_MAP_TYPE_H
#define ISL_MAP_TYPE_H

#include <isl/ctx.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct __isl_subclass(isl_map) isl_basic_map;
typedef struct isl_basic_map isl_basic_map;
struct __isl_subclass(isl_union_map) isl_map;
typedef struct isl_map isl_map;

#ifndef isl_basic_set
struct __isl_subclass(isl_set) isl_basic_set;
typedef struct isl_basic_set isl_basic_set;
#endif
#ifndef isl_set
struct __isl_subclass(isl_union_set) isl_set;
typedef struct isl_set isl_set;
#endif

#if defined(__cplusplus)
}
#endif

#endif
