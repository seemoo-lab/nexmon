/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include "isl_sample_piplib.h"

struct isl_vec *isl_pip_basic_set_sample(struct isl_basic_set *bset)
{
	isl_basic_set_free(bset);
	return NULL;
}
