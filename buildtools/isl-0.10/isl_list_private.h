#ifndef ISL_LIST_PRIVATE_H
#define ISL_LIST_PRIVATE_H

#include <isl/list.h>

#undef EL
#define EL isl_basic_set

#include <isl_list_templ.h>

#undef EL
#define EL isl_set

#include <isl_list_templ.h>

#undef EL
#define EL isl_aff

#include <isl_list_templ.h>

#undef EL
#define EL isl_pw_aff

#include <isl_list_templ.h>

#undef EL
#define EL isl_band

#include <isl_list_templ.h>

#endif
