/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include "isl_piplib.h"

void isl_seq_cpy_to_pip(Entier *dst, isl_int *src, unsigned len)
{
	int i;

	for (i = 0; i < len; ++i)
		entier_assign(dst[i], src[i]);
}

void isl_seq_cpy_from_pip(isl_int *dst, Entier *src, unsigned len)
{
	int i;

	for (i = 0; i < len; ++i)
		entier_assign(dst[i], src[i]);
}
