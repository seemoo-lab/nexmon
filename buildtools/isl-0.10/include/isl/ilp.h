/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_ILP_H
#define ISL_ILP_H

#include <isl/aff_type.h>
#include <isl/lp.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum isl_lp_result isl_basic_set_solve_ilp(struct isl_basic_set *bset, int max,
				      isl_int *f, isl_int *opt,
				      struct isl_vec **sol_p);
enum isl_lp_result isl_basic_set_max(__isl_keep isl_basic_set *bset,
	__isl_keep isl_aff *obj, isl_int *opt);
enum isl_lp_result isl_set_min(__isl_keep isl_set *set,
	__isl_keep isl_aff *obj, isl_int *opt);
enum isl_lp_result isl_set_max(__isl_keep isl_set *set,
	__isl_keep isl_aff *obj, isl_int *opt);

#if defined(__cplusplus)
}
#endif

#endif
