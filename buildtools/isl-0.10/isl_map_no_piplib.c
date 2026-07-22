/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include "isl_map_piplib.h"
#include <isl/set.h>

struct isl_map *isl_pip_basic_map_lexopt(
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty, int max)
{
	isl_basic_map_free(bmap);
	isl_basic_set_free(dom);
	return NULL;
}
