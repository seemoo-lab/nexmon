/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#ifndef ISL_PIPLIB_H
#define ISL_PIPLIB_H

#include <isl/ctx.h>
#include <isl/int.h>
#include <isl/map.h>
#ifndef ISL_PIPLIB
#error "no piplib"
#endif

#include <piplib/piplibMP.h>

void isl_seq_cpy_to_pip(Entier *dst, isl_int *src, unsigned len);
void isl_seq_cpy_from_pip(isl_int *dst, Entier *src, unsigned len);

PipMatrix *isl_basic_map_to_pip(struct isl_basic_map *bmap, unsigned pip_param,
			 unsigned extra_front, unsigned extra_back);

#endif
