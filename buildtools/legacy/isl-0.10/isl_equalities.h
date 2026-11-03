/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_EQUALITIES_H
#define ISL_EQUALITIES_H

#include <isl/set.h>
#include <isl/mat.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_mat *isl_mat_variable_compression(
			struct isl_mat *B, struct isl_mat **T2);
struct isl_mat *isl_mat_parameter_compression(
			struct isl_mat *B, struct isl_vec *d);
struct isl_basic_set *isl_basic_set_remove_equalities(
	struct isl_basic_set *bset, struct isl_mat **T, struct isl_mat **T2);

#if defined(__cplusplus)
}
#endif

#endif
