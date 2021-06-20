/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_LP_H
#define ISL_LP_H

#include <isl/int.h>
#include <isl/vec.h>
#include <isl/map_type.h>
#include <isl/set_type.h>

enum isl_lp_result {
	isl_lp_error = -1,
	isl_lp_ok = 0,
	isl_lp_unbounded,
	isl_lp_empty
};

#if defined(__cplusplus)
extern "C" {
#endif

enum isl_lp_result isl_basic_map_solve_lp(struct isl_basic_map *bmap, int max,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol);
enum isl_lp_result isl_basic_set_solve_lp(struct isl_basic_set *bset, int max,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol);
enum isl_lp_result isl_map_solve_lp(__isl_keep isl_map *map, int max,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol);
enum isl_lp_result isl_set_solve_lp(__isl_keep isl_set *set, int max,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol);

#if defined(__cplusplus)
}
#endif

#endif
