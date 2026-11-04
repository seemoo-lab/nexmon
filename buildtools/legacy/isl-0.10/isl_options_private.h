#ifndef ISL_OPTIONS_PRIVATE_H
#define ISL_OPTIONS_PRIVATE_H

#include <isl/options.h>

struct isl_options {
	#define			ISL_LP_TAB	0
	#define			ISL_LP_PIP	1
	unsigned		lp_solver;

	#define			ISL_ILP_GBR	0
	#define			ISL_ILP_PIP	1
	unsigned		ilp_solver;

	#define			ISL_PIP_TAB	0
	#define			ISL_PIP_PIP	1
	unsigned		pip;

	#define			ISL_CONTEXT_GBR		0
	#define			ISL_CONTEXT_LEXMIN	1
	unsigned		context;

	#define			ISL_GBR_NEVER	0
	#define			ISL_GBR_ONCE	1
	#define			ISL_GBR_ALWAYS	2
	unsigned		gbr;
	unsigned		gbr_only_first;

	#define			ISL_CLOSURE_ISL		0
	#define			ISL_CLOSURE_BOX		1
	unsigned		closure;

	int			bound;
	unsigned		on_error;

	#define			ISL_BERNSTEIN_FACTORS	1
	#define			ISL_BERNSTEIN_INTERVALS	2
	int			bernstein_recurse;

	int			bernstein_triangulate;

	int			pip_symmetry;

	#define			ISL_CONVEX_HULL_WRAP	0
	#define			ISL_CONVEX_HULL_FM	1
	int			convex;

	int			coalesce_bounded_wrapping;

	int			schedule_max_coefficient;
	int			schedule_max_constant_term;
	int			schedule_parametric;
	int			schedule_outer_zero_distance;
	int			schedule_maximize_band_depth;
	int			schedule_split_scaled;
	int			schedule_separate_components;
	unsigned		schedule_algorithm;
	int			schedule_fuse;

	int			tile_scale_tile_loops;

};

#endif
