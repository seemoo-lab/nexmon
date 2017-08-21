/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_MAP_PIPLIB_H
#define ISL_MAP_PIPLIB_H

#include <isl/map.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct isl_map *isl_pip_basic_map_lexopt(
		struct isl_basic_map *bmap, struct isl_basic_set *dom,
		struct isl_set **empty, int max);

#if defined(__cplusplus)
}
#endif

#endif
