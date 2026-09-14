/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <isl_list_private.h>
#include <isl/set.h>
#include <isl/aff.h>
#include <isl/band.h>

#undef BASE
#define BASE basic_set

#include <isl_list_templ.c>

#undef BASE
#define BASE set

#include <isl_list_templ.c>

#undef BASE
#define BASE aff

#include <isl_list_templ.c>

#undef BASE
#define BASE pw_aff

#include <isl_list_templ.c>

#undef BASE
#define BASE band

#include <isl_list_templ.c>
