/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_SAMPLE_PIP_H
#define ISL_SAMPLE_PIP

#include <isl/set.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_vec *isl_pip_basic_set_sample(struct isl_basic_set *bset);

#if defined(__cplusplus)
}
#endif

#endif
