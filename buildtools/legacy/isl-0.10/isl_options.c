/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isl/ctx.h>
#include <isl_options_private.h>
#include <isl/schedule.h>
#include <isl/version.h>

struct isl_arg_choice isl_lp_solver_choice[] = {
	{"tab",		ISL_LP_TAB},
#ifdef ISL_PIPLIB
	{"pip",		ISL_LP_PIP},
#endif
	{0}
};

struct isl_arg_choice isl_ilp_solver_choice[] = {
	{"gbr",		ISL_ILP_GBR},
#ifdef ISL_PIPLIB
	{"pip",		ISL_ILP_PIP},
#endif
	{0}
};

struct isl_arg_choice isl_pip_solver_choice[] = {
	{"tab",		ISL_PIP_TAB},
#ifdef ISL_PIPLIB
	{"pip",		ISL_PIP_PIP},
#endif
	{0}
};

struct isl_arg_choice isl_pip_context_choice[] = {
	{"gbr",		ISL_CONTEXT_GBR},
	{"lexmin",	ISL_CONTEXT_LEXMIN},
	{0}
};

struct isl_arg_choice isl_gbr_choice[] = {
	{"never",	ISL_GBR_NEVER},
	{"once",	ISL_GBR_ONCE},
	{"always",	ISL_GBR_ALWAYS},
	{0}
};

struct isl_arg_choice isl_closure_choice[] = {
	{"isl",		ISL_CLOSURE_ISL},
	{"box",		ISL_CLOSURE_BOX},
	{0}
};

static struct isl_arg_choice bound[] = {
	{"bernstein",	ISL_BOUND_BERNSTEIN},
	{"range",	ISL_BOUND_RANGE},
	{0}
};

static struct isl_arg_choice on_error[] = {
	{"warn",	ISL_ON_ERROR_WARN},
	{"continue",	ISL_ON_ERROR_CONTINUE},
	{"abort",	ISL_ON_ERROR_ABORT},
	{0}
};

static struct isl_arg_choice isl_schedule_algorithm_choice[] = {
	{"isl",		ISL_SCHEDULE_ALGORITHM_ISL},
	{"feautrier",   ISL_SCHEDULE_ALGORITHM_FEAUTRIER},
	{0}
};

static struct isl_arg_flags bernstein_recurse[] = {
	{"none",	ISL_BERNSTEIN_FACTORS | ISL_BERNSTEIN_INTERVALS, 0},
	{"factors",	ISL_BERNSTEIN_FACTORS | ISL_BERNSTEIN_INTERVALS,
			ISL_BERNSTEIN_FACTORS},
	{"intervals",	ISL_BERNSTEIN_FACTORS | ISL_BERNSTEIN_INTERVALS,
			ISL_BERNSTEIN_INTERVALS},
	{"full",	ISL_BERNSTEIN_FACTORS | ISL_BERNSTEIN_INTERVALS,
			ISL_BERNSTEIN_FACTORS | ISL_BERNSTEIN_INTERVALS},
	{0}
};

static struct isl_arg_choice convex[] = {
	{"wrap",	ISL_CONVEX_HULL_WRAP},
	{"fm",		ISL_CONVEX_HULL_FM},
	{0}
};

static struct isl_arg_choice fuse[] = {
	{"max",		ISL_SCHEDULE_FUSE_MAX},
	{"min",		ISL_SCHEDULE_FUSE_MIN},
	{0}
};

static void print_version(void)
{
	printf("%s", isl_version());
}

ISL_ARGS_START(struct isl_options, isl_options_args)
ISL_ARG_CHOICE(struct isl_options, lp_solver, 0, "lp-solver", \
	isl_lp_solver_choice,	ISL_LP_TAB, "lp solver to use")
ISL_ARG_CHOICE(struct isl_options, ilp_solver, 0, "ilp-solver", \
	isl_ilp_solver_choice,	ISL_ILP_GBR, "ilp solver to use")
ISL_ARG_CHOICE(struct isl_options, pip, 0, "pip", \
	isl_pip_solver_choice,	ISL_PIP_TAB, "pip solver to use")
ISL_ARG_CHOICE(struct isl_options, context, 0, "context", \
	isl_pip_context_choice,	ISL_CONTEXT_GBR,
	"how to handle the pip context tableau")
ISL_ARG_CHOICE(struct isl_options, gbr, 0, "gbr", \
	isl_gbr_choice,	ISL_GBR_ALWAYS,
	"how often to use generalized basis reduction")
ISL_ARG_CHOICE(struct isl_options, closure, 0, "closure", \
	isl_closure_choice,	ISL_CLOSURE_ISL,
	"closure operation to use")
ISL_ARG_BOOL(struct isl_options, gbr_only_first, 0, "gbr-only-first", 0,
	"only perform basis reduction in first direction")
ISL_ARG_CHOICE(struct isl_options, bound, 0, "bound", bound,
	ISL_BOUND_BERNSTEIN, "algorithm to use for computing bounds")
ISL_ARG_CHOICE(struct isl_options, on_error, 0, "on-error", on_error,
	ISL_ON_ERROR_WARN, "how to react if an error is detected")
ISL_ARG_FLAGS(struct isl_options, bernstein_recurse, 0,
	"bernstein-recurse", bernstein_recurse, ISL_BERNSTEIN_FACTORS, NULL)
ISL_ARG_BOOL(struct isl_options, bernstein_triangulate, 0,
	"bernstein-triangulate", 1,
	"triangulate domains during Bernstein expansion")
ISL_ARG_BOOL(struct isl_options, pip_symmetry, 0, "pip-symmetry", 1,
	"detect simple symmetries in PIP input")
ISL_ARG_CHOICE(struct isl_options, convex, 0, "convex-hull", \
	convex,	ISL_CONVEX_HULL_WRAP, "convex hull algorithm to use")
ISL_ARG_BOOL(struct isl_options, coalesce_bounded_wrapping, 0,
	"coalesce-bounded-wrapping", 1, "bound wrapping during coalescing")
ISL_ARG_INT(struct isl_options, schedule_max_coefficient, 0,
	"schedule-max-coefficient", "limit", -1, "Only consider schedules "
	"where the coefficients of the variable and parameter dimensions "
        "do not exceed <limit>. A value of -1 allows arbitrary coefficients.")
ISL_ARG_INT(struct isl_options, schedule_max_constant_term, 0,
	"schedule-max-constant-term", "limit", -1, "Only consider schedules "
	"where the coefficients of the constant dimension do not exceed "
	"<limit>. A value of -1 allows arbitrary coefficients.")
ISL_ARG_BOOL(struct isl_options, schedule_parametric, 0,
	"schedule-parametric", 1, "construct possibly parametric schedules")
ISL_ARG_BOOL(struct isl_options, schedule_outer_zero_distance, 0,
	"schedule-outer-zero-distance", 0,
	"try to construct schedules with outer zero distances over "
	"proximity dependences")
ISL_ARG_BOOL(struct isl_options, schedule_maximize_band_depth, 0,
	"schedule-maximize-band-depth", 0,
	"maximize the number of scheduling dimensions in a band")
ISL_ARG_BOOL(struct isl_options, schedule_split_scaled, 0,
	"schedule-split-scaled", 1,
	"split non-tilable bands with scaled schedules")
ISL_ARG_BOOL(struct isl_options, schedule_separate_components, 0,
	"schedule-separate-components", 1,
	"separate components in dependence graph")
ISL_ARG_CHOICE(struct isl_options, schedule_algorithm, 0,
	"schedule-algorithm", isl_schedule_algorithm_choice,
	ISL_SCHEDULE_ALGORITHM_ISL, "scheduling algorithm to use")
ISL_ARG_CHOICE(struct isl_options, schedule_fuse, 0, "schedule-fuse", fuse,
	ISL_SCHEDULE_FUSE_MAX, "level of fusion during scheduling")
ISL_ARG_BOOL(struct isl_options, tile_scale_tile_loops, 0,
	"tile-scale-tile-loops", 1, "scale tile loops")
ISL_ARG_VERSION(print_version)
ISL_ARGS_END

ISL_ARG_DEF(isl_options, struct isl_options, isl_options_args)

ISL_ARG_CTX_DEF(isl_options, struct isl_options, isl_options_args)

ISL_CTX_SET_CHOICE_DEF(isl_options, struct isl_options, isl_options_args, bound)
ISL_CTX_GET_CHOICE_DEF(isl_options, struct isl_options, isl_options_args, bound)

ISL_CTX_SET_CHOICE_DEF(isl_options, struct isl_options, isl_options_args,
	on_error)
ISL_CTX_GET_CHOICE_DEF(isl_options, struct isl_options, isl_options_args,
	on_error)

ISL_CTX_SET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	coalesce_bounded_wrapping)
ISL_CTX_GET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	coalesce_bounded_wrapping)

ISL_CTX_SET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	gbr_only_first)
ISL_CTX_GET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	gbr_only_first)

ISL_CTX_SET_INT_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_max_coefficient)
ISL_CTX_GET_INT_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_max_coefficient)

ISL_CTX_SET_INT_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_max_constant_term)
ISL_CTX_GET_INT_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_max_constant_term)

ISL_CTX_SET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_maximize_band_depth)
ISL_CTX_GET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_maximize_band_depth)

ISL_CTX_SET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_split_scaled)
ISL_CTX_GET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_split_scaled)

ISL_CTX_SET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_separate_components)
ISL_CTX_GET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_separate_components)

ISL_CTX_SET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_outer_zero_distance)
ISL_CTX_GET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_outer_zero_distance)

ISL_CTX_SET_CHOICE_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_algorithm)
ISL_CTX_GET_CHOICE_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_algorithm)

ISL_CTX_SET_CHOICE_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_fuse)
ISL_CTX_GET_CHOICE_DEF(isl_options, struct isl_options, isl_options_args,
	schedule_fuse)

ISL_CTX_SET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	tile_scale_tile_loops)
ISL_CTX_GET_BOOL_DEF(isl_options, struct isl_options, isl_options_args,
	tile_scale_tile_loops)
