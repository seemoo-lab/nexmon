/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_LP_PIPLIB_H
#define ISL_LP_PIPLIB_H

#include <isl/lp.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum isl_lp_result isl_pip_solve_lp(struct isl_basic_map *bmap, int maximize,
				      isl_int *f, isl_int denom, isl_int *opt,
				      isl_int *opt_denom,
				      struct isl_vec **sol);

#if defined(__cplusplus)
}
#endif

#endif
