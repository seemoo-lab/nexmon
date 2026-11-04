/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <isl_ctx_private.h>
#include <isl_map_private.h>
#include <isl/aff.h>
#include <isl/set.h>
#include <isl/flow.h>
#include <isl/constraint.h>
#include <isl/polynomial.h>
#include <isl/union_map.h>
#include <isl_factorization.h>
#include <isl/schedule.h>
#include <isl_options_private.h>
#include <isl/vertices.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

static char *srcdir;

static char *get_filename(isl_ctx *ctx, const char *name, const char *suffix) {
	char *filename;
	int length;
	char *pattern = "%s/test_inputs/%s.%s";

	length = strlen(pattern) - 6 + strlen(srcdir) + strlen(name)
		+ strlen(suffix) + 1;
	filename = isl_alloc_array(ctx, char, length);

	if (!filename)
		return NULL;

	sprintf(filename, pattern, srcdir, name, suffix);

	return filename;
}

void test_parse_map(isl_ctx *ctx, const char *str)
{
	isl_map *map;

	map = isl_map_read_from_str(ctx, str);
	assert(map);
	isl_map_free(map);
}

int test_parse_map_equal(isl_ctx *ctx, const char *str, const char *str2)
{
	isl_map *map, *map2;
	int equal;

	map = isl_map_read_from_str(ctx, str);
	map2 = isl_map_read_from_str(ctx, str2);
	equal = isl_map_is_equal(map, map2);
	isl_map_free(map);
	isl_map_free(map2);

	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown, "maps not equal",
			return -1);

	return 0;
}

void test_parse_pwqp(isl_ctx *ctx, const char *str)
{
	isl_pw_qpolynomial *pwqp;

	pwqp = isl_pw_qpolynomial_read_from_str(ctx, str);
	assert(pwqp);
	isl_pw_qpolynomial_free(pwqp);
}

static void test_parse_pwaff(isl_ctx *ctx, const char *str)
{
	isl_pw_aff *pwaff;

	pwaff = isl_pw_aff_read_from_str(ctx, str);
	assert(pwaff);
	isl_pw_aff_free(pwaff);
}

int test_parse(struct isl_ctx *ctx)
{
	isl_map *map, *map2;
	const char *str, *str2;

	str = "{ [i] -> [-i] }";
	map = isl_map_read_from_str(ctx, str);
	assert(map);
	isl_map_free(map);

	str = "{ A[i] -> L[([i/3])] }";
	map = isl_map_read_from_str(ctx, str);
	assert(map);
	isl_map_free(map);

	test_parse_map(ctx, "{[[s] -> A[i]] -> [[s+1] -> A[i]]}");
	test_parse_map(ctx, "{ [p1, y1, y2] -> [2, y1, y2] : "
				"p1 = 1 && (y1 <= y2 || y2 = 0) }");

	str = "{ [x,y]  : [([x/2]+y)/3] >= 1 }";
	str2 = "{ [x, y] : 2y >= 6 - x }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	if (test_parse_map_equal(ctx, "{ [x,y] : x <= min(y, 2*y+3) }",
				      "{ [x,y] : x <= y, 2*y + 3 }") < 0)
		return -1;
	str = "{ [x, y] : (y <= x and y >= -3) or (2y <= -3 + x and y <= -4) }";
	if (test_parse_map_equal(ctx, "{ [x,y] : x >= min(y, 2*y+3) }",
					str) < 0)
		return -1;

	str = "{[new,old] -> [new+1-2*[(new+1)/2],old+1-2*[(old+1)/2]]}";
	map = isl_map_read_from_str(ctx, str);
	str = "{ [new, old] -> [o0, o1] : "
	       "exists (e0 = [(-1 - new + o0)/2], e1 = [(-1 - old + o1)/2]: "
	       "2e0 = -1 - new + o0 and 2e1 = -1 - old + o1 and o0 >= 0 and "
	       "o0 <= 1 and o1 >= 0 and o1 <= 1) }";
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{[new,old] -> [new+1-2*[(new+1)/2],old+1-2*[(old+1)/2]]}";
	map = isl_map_read_from_str(ctx, str);
	str = "{[new,old] -> [(new+1)%2,(old+1)%2]}";
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[n] -> { [c1] : c1>=0 and c1<=floord(n-4,3) }";
	str2 = "[n] -> { [c1] : c1 >= 0 and 3c1 <= -4 + n }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	str = "{ [i,j] -> [i] : i < j; [i,j] -> [j] : j <= i }";
	str2 = "{ [i,j] -> [min(i,j)] }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	str = "{ [i,j] : i != j }";
	str2 = "{ [i,j] : i < j or i > j }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	str = "{ [i,j] : (i+1)*2 >= j }";
	str2 = "{ [i, j] : j <= 2 + 2i }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	str = "{ [i] -> [i > 0 ? 4 : 5] }";
	str2 = "{ [i] -> [5] : i <= 0; [i] -> [4] : i >= 1 }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	str = "[N=2,M] -> { [i=[(M+N)/4]] }";
	str2 = "[N, M] -> { [i] : N = 2 and 4i <= 2 + M and 4i >= -1 + M }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	str = "{ [x] : x >= 0 }";
	str2 = "{ [x] : x-0 >= 0 }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	str = "{ [i] : ((i > 10)) }";
	str2 = "{ [i] : i >= 11 }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	str = "{ [i] -> [0] }";
	str2 = "{ [i] -> [0 * i] }";
	if (test_parse_map_equal(ctx, str, str2) < 0)
		return -1;

	test_parse_pwqp(ctx, "{ [i] -> i + [ (i + [i/3])/2 ] }");
	test_parse_map(ctx, "{ S1[i] -> [([i/10]),i%10] : 0 <= i <= 45 }");
	test_parse_pwaff(ctx, "{ [i] -> [i + 1] : i > 0; [a] -> [a] : a < 0 }");
	test_parse_pwqp(ctx, "{ [x] -> ([(x)/2] * [(x)/3]) }");

	if (test_parse_map_equal(ctx, "{ [a] -> [b] : (not false) }",
				      "{ [a] -> [b] : true }") < 0)
		return -1;

	return 0;
}

void test_read(struct isl_ctx *ctx)
{
	char *filename;
	FILE *input;
	struct isl_basic_set *bset1, *bset2;
	const char *str = "{[y]: Exists ( alpha : 2alpha = y)}";

	filename = get_filename(ctx, "set", "omega");
	assert(filename);
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input);
	bset2 = isl_basic_set_read_from_str(ctx, str);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	free(filename);

	fclose(input);
}

void test_bounded(struct isl_ctx *ctx)
{
	isl_set *set;
	int bounded;

	set = isl_set_read_from_str(ctx, "[n] -> {[i] : 0 <= i <= n }");
	bounded = isl_set_is_bounded(set);
	assert(bounded);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx, "{[n, i] : 0 <= i <= n }");
	bounded = isl_set_is_bounded(set);
	assert(!bounded);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx, "[n] -> {[i] : i <= n }");
	bounded = isl_set_is_bounded(set);
	assert(!bounded);
	isl_set_free(set);
}

/* Construct the basic set { [i] : 5 <= i <= N } */
void test_construction(struct isl_ctx *ctx)
{
	isl_int v;
	isl_space *dim;
	isl_local_space *ls;
	struct isl_basic_set *bset;
	struct isl_constraint *c;

	isl_int_init(v);

	dim = isl_space_set_alloc(ctx, 1, 1);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_inequality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_param, 0, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_inequality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -5);
	isl_constraint_set_constant(c, v);
	bset = isl_basic_set_add_constraint(bset, c);

	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	isl_int_clear(v);
}

void test_dim(struct isl_ctx *ctx)
{
	const char *str;
	isl_map *map1, *map2;

	map1 = isl_map_read_from_str(ctx,
	    "[n] -> { [i] -> [j] : exists (a = [i/10] : i - 10a <= n ) }");
	map1 = isl_map_add_dims(map1, isl_dim_in, 1);
	map2 = isl_map_read_from_str(ctx,
	    "[n] -> { [i,k] -> [j] : exists (a = [i/10] : i - 10a <= n ) }");
	assert(isl_map_is_equal(map1, map2));
	isl_map_free(map2);

	map1 = isl_map_project_out(map1, isl_dim_in, 0, 1);
	map2 = isl_map_read_from_str(ctx, "[n] -> { [i] -> [j] : n >= 0 }");
	assert(isl_map_is_equal(map1, map2));

	isl_map_free(map1);
	isl_map_free(map2);

	str = "[n] -> { [i] -> [] : exists a : 0 <= i <= n and i = 2 a }";
	map1 = isl_map_read_from_str(ctx, str);
	str = "{ [i] -> [j] : exists a : 0 <= i <= j and i = 2 a }";
	map2 = isl_map_read_from_str(ctx, str);
	map1 = isl_map_move_dims(map1, isl_dim_out, 0, isl_dim_param, 0, 1);
	assert(isl_map_is_equal(map1, map2));

	isl_map_free(map1);
	isl_map_free(map2);
}

void test_div(struct isl_ctx *ctx)
{
	isl_int v;
	isl_space *dim;
	isl_local_space *ls;
	struct isl_basic_set *bset;
	struct isl_constraint *c;

	isl_int_init(v);

	/* test 1 */
	dim = isl_space_set_alloc(ctx, 0, 3);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 1, 2);

	assert(bset && bset->n_div == 1);
	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	/* test 2 */
	dim = isl_space_set_alloc(ctx, 0, 3);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 1, 2);

	assert(bset && bset->n_div == 1);
	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	/* test 3 */
	dim = isl_space_set_alloc(ctx, 0, 3);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -3);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 4);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 1, 2);

	assert(bset && bset->n_div == 1);
	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	/* test 4 */
	dim = isl_space_set_alloc(ctx, 0, 3);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 2);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_constant(c, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 6);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 1, 2);

	assert(isl_basic_set_is_empty(bset));
	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	/* test 5 */
	dim = isl_space_set_alloc(ctx, 0, 3);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 2, 1);

	assert(bset && bset->n_div == 0);
	isl_basic_set_free(bset);
	isl_local_space_free(ls);

	/* test 6 */
	dim = isl_space_set_alloc(ctx, 0, 3);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 6);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 2, 1);

	assert(bset && bset->n_div == 1);
	isl_basic_set_free(bset);
	isl_local_space_free(ls);

	/* test 7 */
	/* This test is a bit tricky.  We set up an equality
	 *		a + 3b + 3c = 6 e0
	 * Normalization of divs creates _two_ divs
	 *		a = 3 e0
	 *		c - b - e0 = 2 e1
	 * Afterwards e0 is removed again because it has coefficient -1
	 * and we end up with the original equality and div again.
	 * Perhaps we can avoid the introduction of this temporary div.
	 */
	dim = isl_space_set_alloc(ctx, 0, 4);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	isl_int_set_si(v, 6);
	isl_constraint_set_coefficient(c, isl_dim_set, 3, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 3, 1);

	/* Test disabled for now */
	/*
	assert(bset && bset->n_div == 1);
	*/
	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	/* test 8 */
	dim = isl_space_set_alloc(ctx, 0, 5);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	isl_int_set_si(v, -3);
	isl_constraint_set_coefficient(c, isl_dim_set, 3, v);
	isl_int_set_si(v, 6);
	isl_constraint_set_coefficient(c, isl_dim_set, 4, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	isl_int_set_si(v, 1);
	isl_constraint_set_constant(c, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 4, 1);

	/* Test disabled for now */
	/*
	assert(bset && bset->n_div == 1);
	*/
	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	/* test 9 */
	dim = isl_space_set_alloc(ctx, 0, 4);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 1, v);
	isl_int_set_si(v, -2);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	bset = isl_basic_set_add_constraint(bset, c);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, -1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, 3);
	isl_constraint_set_coefficient(c, isl_dim_set, 3, v);
	isl_int_set_si(v, 2);
	isl_constraint_set_constant(c, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 2, 2);

	bset = isl_basic_set_fix_si(bset, isl_dim_set, 0, 2);

	assert(!isl_basic_set_is_empty(bset));

	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	/* test 10 */
	dim = isl_space_set_alloc(ctx, 0, 3);
	bset = isl_basic_set_universe(isl_space_copy(dim));
	ls = isl_local_space_from_space(dim);

	c = isl_equality_alloc(isl_local_space_copy(ls));
	isl_int_set_si(v, 1);
	isl_constraint_set_coefficient(c, isl_dim_set, 0, v);
	isl_int_set_si(v, -2);
	isl_constraint_set_coefficient(c, isl_dim_set, 2, v);
	bset = isl_basic_set_add_constraint(bset, c);

	bset = isl_basic_set_project_out(bset, isl_dim_set, 2, 1);

	bset = isl_basic_set_fix_si(bset, isl_dim_set, 0, 2);

	isl_local_space_free(ls);
	isl_basic_set_free(bset);

	isl_int_clear(v);
}

void test_application_case(struct isl_ctx *ctx, const char *name)
{
	char *filename;
	FILE *input;
	struct isl_basic_set *bset1, *bset2;
	struct isl_basic_map *bmap;

	filename = get_filename(ctx, name, "omega");
	assert(filename);
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input);
	bmap = isl_basic_map_read_from_file(ctx, input);

	bset1 = isl_basic_set_apply(bset1, bmap);

	bset2 = isl_basic_set_read_from_file(ctx, input);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	free(filename);

	fclose(input);
}

void test_application(struct isl_ctx *ctx)
{
	test_application_case(ctx, "application");
	test_application_case(ctx, "application2");
}

void test_affine_hull_case(struct isl_ctx *ctx, const char *name)
{
	char *filename;
	FILE *input;
	struct isl_basic_set *bset1, *bset2;

	filename = get_filename(ctx, name, "polylib");
	assert(filename);
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input);
	bset2 = isl_basic_set_read_from_file(ctx, input);

	bset1 = isl_basic_set_affine_hull(bset1);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	free(filename);

	fclose(input);
}

int test_affine_hull(struct isl_ctx *ctx)
{
	const char *str;
	isl_set *set;
	isl_basic_set *bset;
	int n;

	test_affine_hull_case(ctx, "affine2");
	test_affine_hull_case(ctx, "affine");
	test_affine_hull_case(ctx, "affine3");

	str = "[m] -> { [i0] : exists (e0, e1: e1 <= 1 + i0 and "
			"m >= 3 and 4i0 <= 2 + m and e1 >= i0 and "
			"e1 >= 0 and e1 <= 2 and e1 >= 1 + 2e0 and "
			"2e1 <= 1 + m + 4e0 and 2e1 >= 2 - m + 4i0 - 4e0) }";
	set = isl_set_read_from_str(ctx, str);
	bset = isl_set_affine_hull(set);
	n = isl_basic_set_dim(bset, isl_dim_div);
	isl_basic_set_free(bset);
	if (n != 0)
		isl_die(ctx, isl_error_unknown, "not expecting any divs",
			return -1);

	return 0;
}

void test_convex_hull_case(struct isl_ctx *ctx, const char *name)
{
	char *filename;
	FILE *input;
	struct isl_basic_set *bset1, *bset2;
	struct isl_set *set;

	filename = get_filename(ctx, name, "polylib");
	assert(filename);
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input);
	bset2 = isl_basic_set_read_from_file(ctx, input);

	set = isl_basic_set_union(bset1, bset2);
	bset1 = isl_set_convex_hull(set);

	bset2 = isl_basic_set_read_from_file(ctx, input);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	free(filename);

	fclose(input);
}

void test_convex_hull_algo(struct isl_ctx *ctx, int convex)
{
	const char *str1, *str2;
	isl_set *set1, *set2;
	int orig_convex = ctx->opt->convex;
	ctx->opt->convex = convex;

	test_convex_hull_case(ctx, "convex0");
	test_convex_hull_case(ctx, "convex1");
	test_convex_hull_case(ctx, "convex2");
	test_convex_hull_case(ctx, "convex3");
	test_convex_hull_case(ctx, "convex4");
	test_convex_hull_case(ctx, "convex5");
	test_convex_hull_case(ctx, "convex6");
	test_convex_hull_case(ctx, "convex7");
	test_convex_hull_case(ctx, "convex8");
	test_convex_hull_case(ctx, "convex9");
	test_convex_hull_case(ctx, "convex10");
	test_convex_hull_case(ctx, "convex11");
	test_convex_hull_case(ctx, "convex12");
	test_convex_hull_case(ctx, "convex13");
	test_convex_hull_case(ctx, "convex14");
	test_convex_hull_case(ctx, "convex15");

	str1 = "{ [i0, i1, i2] : (i2 = 1 and i0 = 0 and i1 >= 0) or "
	       "(i0 = 1 and i1 = 0 and i2 = 1) or "
	       "(i0 = 0 and i1 = 0 and i2 = 0) }";
	str2 = "{ [i0, i1, i2] : i0 >= 0 and i2 >= i0 and i2 <= 1 and i1 >= 0 }";
	set1 = isl_set_read_from_str(ctx, str1);
	set2 = isl_set_read_from_str(ctx, str2);
	set1 = isl_set_from_basic_set(isl_set_convex_hull(set1));
	assert(isl_set_is_equal(set1, set2));
	isl_set_free(set1);
	isl_set_free(set2);

	ctx->opt->convex = orig_convex;
}

void test_convex_hull(struct isl_ctx *ctx)
{
	test_convex_hull_algo(ctx, ISL_CONVEX_HULL_FM);
	test_convex_hull_algo(ctx, ISL_CONVEX_HULL_WRAP);
}

void test_gist_case(struct isl_ctx *ctx, const char *name)
{
	char *filename;
	FILE *input;
	struct isl_basic_set *bset1, *bset2;

	filename = get_filename(ctx, name, "polylib");
	assert(filename);
	input = fopen(filename, "r");
	assert(input);

	bset1 = isl_basic_set_read_from_file(ctx, input);
	bset2 = isl_basic_set_read_from_file(ctx, input);

	bset1 = isl_basic_set_gist(bset1, bset2);

	bset2 = isl_basic_set_read_from_file(ctx, input);

	assert(isl_basic_set_is_equal(bset1, bset2) == 1);

	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	free(filename);

	fclose(input);
}

void test_gist(struct isl_ctx *ctx)
{
	const char *str;
	isl_basic_set *bset1, *bset2;

	test_gist_case(ctx, "gist1");

	str = "[p0, p2, p3, p5, p6, p10] -> { [] : "
	    "exists (e0 = [(15 + p0 + 15p6 + 15p10)/16], e1 = [(p5)/8], "
	    "e2 = [(p6)/128], e3 = [(8p2 - p5)/128], "
	    "e4 = [(128p3 - p6)/4096]: 8e1 = p5 and 128e2 = p6 and "
	    "128e3 = 8p2 - p5 and 4096e4 = 128p3 - p6 and p2 >= 0 and "
	    "16e0 >= 16 + 16p6 + 15p10 and  p2 <= 15 and p3 >= 0 and "
	    "p3 <= 31 and  p6 >= 128p3 and p5 >= 8p2 and p10 >= 0 and "
	    "16e0 <= 15 + p0 + 15p6 + 15p10 and 16e0 >= p0 + 15p6 + 15p10 and "
	    "p10 <= 15 and p10 <= -1 + p0 - p6) }";
	bset1 = isl_basic_set_read_from_str(ctx, str);
	str = "[p0, p2, p3, p5, p6, p10] -> { [] : exists (e0 = [(p5)/8], "
	    "e1 = [(p6)/128], e2 = [(8p2 - p5)/128], "
	    "e3 = [(128p3 - p6)/4096]: 8e0 = p5 and 128e1 = p6 and "
	    "128e2 = 8p2 - p5 and 4096e3 = 128p3 - p6 and p5 >= -7 and "
	    "p2 >= 0 and 8p2 <= -1 + p0 and p2 <= 15 and p3 >= 0 and "
	    "p3 <= 31 and 128p3 <= -1 + p0 and p6 >= -127 and "
	    "p5 <= -1 + p0 and p6 <= -1 + p0 and p6 >= 128p3 and "
	    "p0 >= 1 and p5 >= 8p2 and p10 >= 0 and p10 <= 15 ) }";
	bset2 = isl_basic_set_read_from_str(ctx, str);
	bset1 = isl_basic_set_gist(bset1, bset2);
	assert(bset1 && bset1->n_div == 0);
	isl_basic_set_free(bset1);
}

int test_coalesce_set(isl_ctx *ctx, const char *str, int check_one)
{
	isl_set *set, *set2;
	int equal;
	int one;

	set = isl_set_read_from_str(ctx, str);
	set = isl_set_coalesce(set);
	set2 = isl_set_read_from_str(ctx, str);
	equal = isl_set_is_equal(set, set2);
	one = set && set->n == 1;
	isl_set_free(set);
	isl_set_free(set2);

	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown,
			"coalesced set not equal to input", return -1);
	if (check_one && !one)
		isl_die(ctx, isl_error_unknown,
			"coalesced set should not be a union", return -1);

	return 0;
}

int test_coalesce_unbounded_wrapping(isl_ctx *ctx)
{
	int r = 0;
	int bounded;

	bounded = isl_options_get_coalesce_bounded_wrapping(ctx);
	isl_options_set_coalesce_bounded_wrapping(ctx, 0);

	if (test_coalesce_set(ctx,
		"{[x,y,z] : y + 2 >= 0 and x - y + 1 >= 0 and "
			"-x - y + 1 >= 0 and -3 <= z <= 3;"
		"[x,y,z] : -x+z + 20 >= 0 and -x-z + 20 >= 0 and "
			"x-z + 20 >= 0 and x+z + 20 >= 0 and "
			"-10 <= y <= 0}", 1) < 0)
		goto error;
	if (test_coalesce_set(ctx,
		"{[x,y] : 0 <= x,y <= 10; [5,y]: 4 <=y <= 11}", 1) < 0)
		goto error;
	if (test_coalesce_set(ctx,
		"{[x,0,0] : -5 <= x <= 5; [0,y,1] : -5 <= y <= 5 }", 1) < 0)
		goto error;

	if (0) {
error:
		r = -1;
	}

	isl_options_set_coalesce_bounded_wrapping(ctx, bounded);

	return r;
}

int test_coalesce(struct isl_ctx *ctx)
{
	const char *str;
	struct isl_set *set, *set2;
	struct isl_map *map, *map2;

	set = isl_set_read_from_str(ctx,
		"{[x,y]: x >= 0 & x <= 10 & y >= 0 & y <= 10 or "
		       "y >= x & x >= 2 & 5 >= y }");
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & 2x + y <= 30 & y <= 10 & x >= 0 or "
		       "x + y >= 10 & y <= x & x + y <= 20 & y >= 0}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & 2x + y <= 30 & y <= 10 & x >= 0 or "
		       "x + y >= 10 & y <= x & x + y <= 19 & y >= 0}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x >= 6 & x <= 10 & y <= x}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x >= 7 & x <= 10 & y <= x}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x >= 6 & x <= 10 & y + 1 <= x}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 6 & y <= 6}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 7 & y <= 6}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 6 & y <= 5}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	set2 = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 6 & y <= 5}");
	assert(isl_set_is_equal(set, set2));
	isl_set_free(set);
	isl_set_free(set2);

	set = isl_set_read_from_str(ctx,
		"{[x,y]: y >= 0 & x <= 5 & y <= x or "
		       "y >= 0 & x = 6 & y <= 7}");
	set = isl_set_coalesce(set);
	assert(set && set->n == 2);
	isl_set_free(set);

	set = isl_set_read_from_str(ctx,
		"[n] -> { [i] : i = 1 and n >= 2 or 2 <= i and i <= n }");
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	set2 = isl_set_read_from_str(ctx,
		"[n] -> { [i] : i = 1 and n >= 2 or 2 <= i and i <= n }");
	assert(isl_set_is_equal(set, set2));
	isl_set_free(set);
	isl_set_free(set2);

	set = isl_set_read_from_str(ctx,
		"{[x,y] : x >= 0 and y >= 0 or 0 <= y and y <= 5 and x = -1}");
	set = isl_set_coalesce(set);
	set2 = isl_set_read_from_str(ctx,
		"{[x,y] : x >= 0 and y >= 0 or 0 <= y and y <= 5 and x = -1}");
	assert(isl_set_is_equal(set, set2));
	isl_set_free(set);
	isl_set_free(set2);

	set = isl_set_read_from_str(ctx,
		"[n] -> { [i] : 1 <= i and i <= n - 1 or "
				"2 <= i and i <= n }");
	set = isl_set_coalesce(set);
	assert(set && set->n == 1);
	set2 = isl_set_read_from_str(ctx,
		"[n] -> { [i] : 1 <= i and i <= n - 1 or "
				"2 <= i and i <= n }");
	assert(isl_set_is_equal(set, set2));
	isl_set_free(set);
	isl_set_free(set2);

	map = isl_map_read_from_str(ctx,
		"[n] -> { [i0] -> [o0] : exists (e0 = [(i0)/4], e1 = [(o0)/4], "
		"e2 = [(n)/2], e3 = [(-2 + i0)/4], e4 = [(-2 + o0)/4], "
		"e5 = [(-2n + i0)/4]: 2e2 = n and 4e3 = -2 + i0 and "
		"4e4 = -2 + o0 and i0 >= 8 + 2n and o0 >= 2 + i0 and "
		"o0 <= 56 + 2n and o0 <= -12 + 4n and i0 <= 57 + 2n and "
		"i0 <= -11 + 4n and o0 >= 6 + 2n and 4e0 <= i0 and "
		"4e0 >= -3 + i0 and 4e1 <= o0 and 4e1 >= -3 + o0 and "
		"4e5 <= -2n + i0 and 4e5 >= -3 - 2n + i0);"
		"[i0] -> [o0] : exists (e0 = [(i0)/4], e1 = [(o0)/4], "
		"e2 = [(n)/2], e3 = [(-2 + i0)/4], e4 = [(-2 + o0)/4], "
		"e5 = [(-2n + i0)/4]: 2e2 = n and 4e3 = -2 + i0 and "
		"4e4 = -2 + o0 and 2e0 >= 3 + n and e0 <= -4 + n and "
		"2e0 <= 27 + n and e1 <= -4 + n and 2e1 <= 27 + n and "
		"2e1 >= 2 + n and e1 >= 1 + e0 and i0 >= 7 + 2n and "
		"i0 <= -11 + 4n and i0 <= 57 + 2n and 4e0 <= -2 + i0 and "
		"4e0 >= -3 + i0 and o0 >= 6 + 2n and o0 <= -11 + 4n and "
		"o0 <= 57 + 2n and 4e1 <= -2 + o0 and 4e1 >= -3 + o0 and "
		"4e5 <= -2n + i0 and 4e5 >= -3 - 2n + i0 ) }");
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i0] -> [o0] : exists (e0 = [(i0)/4], e1 = [(o0)/4], "
		"e2 = [(n)/2], e3 = [(-2 + i0)/4], e4 = [(-2 + o0)/4], "
		"e5 = [(-2n + i0)/4]: 2e2 = n and 4e3 = -2 + i0 and "
		"4e4 = -2 + o0 and i0 >= 8 + 2n and o0 >= 2 + i0 and "
		"o0 <= 56 + 2n and o0 <= -12 + 4n and i0 <= 57 + 2n and "
		"i0 <= -11 + 4n and o0 >= 6 + 2n and 4e0 <= i0 and "
		"4e0 >= -3 + i0 and 4e1 <= o0 and 4e1 >= -3 + o0 and "
		"4e5 <= -2n + i0 and 4e5 >= -3 - 2n + i0);"
		"[i0] -> [o0] : exists (e0 = [(i0)/4], e1 = [(o0)/4], "
		"e2 = [(n)/2], e3 = [(-2 + i0)/4], e4 = [(-2 + o0)/4], "
		"e5 = [(-2n + i0)/4]: 2e2 = n and 4e3 = -2 + i0 and "
		"4e4 = -2 + o0 and 2e0 >= 3 + n and e0 <= -4 + n and "
		"2e0 <= 27 + n and e1 <= -4 + n and 2e1 <= 27 + n and "
		"2e1 >= 2 + n and e1 >= 1 + e0 and i0 >= 7 + 2n and "
		"i0 <= -11 + 4n and i0 <= 57 + 2n and 4e0 <= -2 + i0 and "
		"4e0 >= -3 + i0 and o0 >= 6 + 2n and o0 <= -11 + 4n and "
		"o0 <= 57 + 2n and 4e1 <= -2 + o0 and 4e1 >= -3 + o0 and "
		"4e5 <= -2n + i0 and 4e5 >= -3 - 2n + i0 ) }");
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[n, m] -> { [] -> [o0, o2, o3] : (o3 = 1 and o0 >= 1 + m and "
	      "o0 <= n + m and o2 <= m and o0 >= 2 + n and o2 >= 3) or "
	      "(o0 >= 2 + n and o0 >= 1 + m and o0 <= n + m and n >= 1 and "
	      "o3 <= -1 + o2 and o3 >= 1 - m + o2 and o3 >= 2 and o3 <= n) }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[M, N] -> { [i0, i1, i2, i3, i4, i5, i6] -> "
	  "[o0, o1, o2, o3, o4, o5, o6] : "
	  "(o6 <= -4 + 2M - 2N + i0 + i1 - i2 + i6 - o0 - o1 + o2 and "
	  "o3 <= -2 + i3 and o6 >= 2 + i0 + i3 + i6 - o0 - o3 and "
	  "o6 >= 2 - M + N + i3 + i4 + i6 - o3 - o4 and o0 <= -1 + i0 and "
	  "o4 >= 4 - 3M + 3N - i0 - i1 + i2 + 2i3 + i4 + o0 + o1 - o2 - 2o3 "
	  "and o6 <= -3 + 2M - 2N + i3 + i4 - i5 + i6 - o3 - o4 + o5 and "
	  "2o6 <= -5 + 5M - 5N + 2i0 + i1 - i2 - i5 + 2i6 - 2o0 - o1 + o2 + o5 "
	  "and o6 >= 2i0 + i1 + i6 - 2o0 - o1 and "
	  "3o6 <= -5 + 4M - 4N + 2i0 + i1 - i2 + 2i3 + i4 - i5 + 3i6 "
	  "- 2o0 - o1 + o2 - 2o3 - o4 + o5) or "
	  "(N >= 2 and o3 <= -1 + i3 and o0 <= -1 + i0 and "
	  "o6 >= i3 + i6 - o3 and M >= 0 and "
	  "2o6 >= 1 + i0 + i3 + 2i6 - o0 - o3 and "
	  "o6 >= 1 - M + i0 + i6 - o0 and N >= 2M and o6 >= i0 + i6 - o0) }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[M, N] -> { [] -> [o0] : (o0 = 0 and M >= 1 and N >= 2) or "
		"(o0 = 0 and M >= 1 and N >= 2M and N >= 2 + M) or "
		"(o0 = 0 and M >= 2 and N >= 3) or "
		"(M = 0 and o0 = 0 and N >= 3) }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{ [i0, i1, i2, i3] : (i1 = 10i0 and i0 >= 1 and 10i0 <= 100 and "
		"i3 <= 9 + 10 i2 and i3 >= 1 + 10i2 and i3 >= 0) or "
		"(i1 <= 9 + 10i0 and i1 >= 1 + 10i0 and i2 >= 0 and "
		"i0 >= 0 and i1 <= 100 and i3 <= 9 + 10i2 and i3 >= 1 + 10i2) }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_coalesce(map);
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	test_coalesce_set(ctx,
		"[M] -> { [i1] : (i1 >= 2 and i1 <= M) or "
				"(i1 = M and M >= 1) }", 0);
	test_coalesce_set(ctx,
		"{[x,y] : x,y >= 0; [x,y] : 10 <= x <= 20 and y >= -1 }", 0);
	test_coalesce_set(ctx,
		"{ [x, y] : (x >= 1 and y >= 1 and x <= 2 and y <= 2) or "
		"(y = 3 and x = 1) }", 1);
	test_coalesce_set(ctx,
		"[M] -> { [i0, i1, i2, i3, i4] : (i1 >= 3 and i4 >= 2 + i2 and "
		"i2 >= 2 and i0 >= 2 and i3 >= 1 + i2 and i0 <= M and "
		"i1 <= M and i3 <= M and i4 <= M) or "
		"(i1 >= 2 and i4 >= 1 + i2 and i2 >= 2 and i0 >= 2 and "
		"i3 >= 1 + i2 and i0 <= M and i1 <= -1 + M and i3 <= M and "
		"i4 <= -1 + M) }", 1);
	test_coalesce_set(ctx,
		"{ [x, y] : (x >= 0 and y >= 0 and x <= 10 and y <= 10) or "
		"(x >= 1 and y >= 1 and x <= 11 and y <= 11) }", 1);
	if (test_coalesce_unbounded_wrapping(ctx) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{[x,0] : x >= 0; [x,1] : x <= 20}", 0) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [x, 1 - x] : 0 <= x <= 1; [0,0] }", 1) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [0,0]; [i,i] : 1 <= i <= 10 }", 1) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [0,0]; [i,j] : 1 <= i,j <= 10 }", 0) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [0,0]; [i,2i] : 1 <= i <= 10 }", 1) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [0,0]; [i,2i] : 2 <= i <= 10 }", 0) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [1,0]; [i,2i] : 1 <= i <= 10 }", 0) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [0,1]; [i,2i] : 1 <= i <= 10 }", 0) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [a, b] : exists e : 2e = a and "
		    "a >= 0 and (a <= 3 or (b <= 0 and b >= -4 + a)) }", 0) < 0)
		return -1;
	if (test_coalesce_set(ctx,
		"{ [i, j, i', j'] : i <= 2 and j <= 2 and "
			"j' >= -1 + 2i + j - 2i' and i' <= -1 + i and "
			"j >= 1 and j' <= i + j - i' and i >= 1; "
		"[1, 1, 1, 1] }", 0) < 0)
		return -1;
	if (test_coalesce_set(ctx, "{ [i,j] : exists a,b : i = 2a and j = 3b; "
				     "[i,j] : exists a : j = 3a }", 1) < 0)
		return -1;
	return 0;
}

void test_closure(struct isl_ctx *ctx)
{
	const char *str;
	isl_set *dom;
	isl_map *up, *right;
	isl_map *map, *map2;
	int exact;

	/* COCOA example 1 */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 1 and j2 = j + 1 and "
			"1 <= i and i < n and 1 <= j and j < n or "
			"i2 = i + 1 and j2 = j - 1 and "
			"1 <= i and i < n and 2 <= j and j <= n }");
	map = isl_map_power(map, &exact);
	assert(exact);
	isl_map_free(map);

	/* COCOA example 1 */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 1 and j2 = j + 1 and "
			"1 <= i and i < n and 1 <= j and j < n or "
			"i2 = i + 1 and j2 = j - 1 and "
			"1 <= i and i < n and 2 <= j and j <= n }");
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : exists (k1,k2,k : "
			"1 <= i and i < n and 1 <= j and j <= n and "
			"2 <= i2 and i2 <= n and 1 <= j2 and j2 <= n and "
			"i2 = i + k1 + k2 and j2 = j + k1 - k2 and "
			"k1 >= 0 and k2 >= 0 and k1 + k2 = k and k >= 1 )}");
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	map = isl_map_read_from_str(ctx,
		"[n] -> { [x] -> [y] : y = x + 1 and 0 <= x and x <= n and "
				     " 0 <= y and y <= n }");
	map = isl_map_transitive_closure(map, &exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [x] -> [y] : y > x and 0 <= x and x <= n and "
				     " 0 <= y and y <= n }");
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	/* COCOA example 2 */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 2 and j2 = j + 2 and "
			"1 <= i and i < n - 1 and 1 <= j and j < n - 1 or "
			"i2 = i + 2 and j2 = j - 2 and "
			"1 <= i and i < n - 1 and 3 <= j and j <= n }");
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : exists (k1,k2,k : "
			"1 <= i and i < n - 1 and 1 <= j and j <= n and "
			"3 <= i2 and i2 <= n and 1 <= j2 and j2 <= n and "
			"i2 = i + 2 k1 + 2 k2 and j2 = j + 2 k1 - 2 k2 and "
			"k1 >= 0 and k2 >= 0 and k1 + k2 = k and k >= 1) }");
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	/* COCOA Fig.2 left */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 2 and j2 = j and "
			"i <= 2 j - 3 and i <= n - 2 and j <= 2 i - 1 and "
			"j <= n or "
			"i2 = i and j2 = j + 2 and i <= 2 j - 1 and i <= n and "
			"j <= 2 i - 3 and j <= n - 2 or "
			"i2 = i + 1 and j2 = j + 1 and i <= 2 j - 1 and "
			"i <= n - 1 and j <= 2 i - 1 and j <= n - 1 }");
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	isl_map_free(map);

	/* COCOA Fig.2 right */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 3 and j2 = j and "
			"i <= 2 j - 4 and i <= n - 3 and j <= 2 i - 1 and "
			"j <= n or "
			"i2 = i and j2 = j + 3 and i <= 2 j - 1 and i <= n and "
			"j <= 2 i - 4 and j <= n - 3 or "
			"i2 = i + 1 and j2 = j + 1 and i <= 2 j - 1 and "
			"i <= n - 1 and j <= 2 i - 1 and j <= n - 1 }");
	map = isl_map_power(map, &exact);
	assert(exact);
	isl_map_free(map);

	/* COCOA Fig.2 right */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i + 3 and j2 = j and "
			"i <= 2 j - 4 and i <= n - 3 and j <= 2 i - 1 and "
			"j <= n or "
			"i2 = i and j2 = j + 3 and i <= 2 j - 1 and i <= n and "
			"j <= 2 i - 4 and j <= n - 3 or "
			"i2 = i + 1 and j2 = j + 1 and i <= 2 j - 1 and "
			"i <= n - 1 and j <= 2 i - 1 and j <= n - 1 }");
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : exists (k1,k2,k3,k : "
			"i <= 2 j - 1 and i <= n and j <= 2 i - 1 and "
			"j <= n and 3 + i + 2 j <= 3 n and "
			"3 + 2 i + j <= 3n and i2 <= 2 j2 -1 and i2 <= n and "
			"i2 <= 3 j2 - 4 and j2 <= 2 i2 -1 and j2 <= n and "
			"13 + 4 j2 <= 11 i2 and i2 = i + 3 k1 + k3 and "
			"j2 = j + 3 k2 + k3 and k1 >= 0 and k2 >= 0 and "
			"k3 >= 0 and k1 + k2 + k3 = k and k > 0) }");
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	/* COCOA Fig.1 right */
	dom = isl_set_read_from_str(ctx,
		"{ [x,y] : x >= 0 and -2 x + 3 y >= 0 and x <= 3 and "
			"2 x - 3 y + 3 >= 0 }");
	right = isl_map_read_from_str(ctx,
		"{ [x,y] -> [x2,y2] : x2 = x + 1 and y2 = y }");
	up = isl_map_read_from_str(ctx,
		"{ [x,y] -> [x2,y2] : x2 = x and y2 = y + 1 }");
	right = isl_map_intersect_domain(right, isl_set_copy(dom));
	right = isl_map_intersect_range(right, isl_set_copy(dom));
	up = isl_map_intersect_domain(up, isl_set_copy(dom));
	up = isl_map_intersect_range(up, dom);
	map = isl_map_union(up, right);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"{ [0,0] -> [0,1]; [0,0] -> [1,1]; [0,1] -> [1,1]; "
		"  [2,2] -> [3,2]; [2,2] -> [3,3]; [3,2] -> [3,3] }");
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	/* COCOA Theorem 1 counter example */
	map = isl_map_read_from_str(ctx,
		"{ [i,j] -> [i2,j2] : i = 0 and 0 <= j and j <= 1 and "
			"i2 = 1 and j2 = j or "
			"i = 0 and j = 0 and i2 = 0 and j2 = 1 }");
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	isl_map_free(map);

	map = isl_map_read_from_str(ctx,
		"[m,n] -> { [i,j] -> [i2,j2] : i2 = i and j2 = j + 2 and "
			"1 <= i,i2 <= n and 1 <= j,j2 <= m or "
			"i2 = i + 1 and 3 <= j2 - j <= 4 and "
			"1 <= i,i2 <= n and 1 <= j,j2 <= m }");
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	isl_map_free(map);

	/* Kelly et al 1996, fig 12 */
	map = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : i2 = i and j2 = j + 1 and "
			"1 <= i,j,j+1 <= n or "
			"j = n and j2 = 1 and i2 = i + 1 and "
			"1 <= i,i+1 <= n }");
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [i,j] -> [i2,j2] : 1 <= j < j2 <= n and "
			"1 <= i <= n and i = i2 or "
			"1 <= i < i2 <= n and 1 <= j <= n and "
			"1 <= j2 <= n }");
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map2);
	isl_map_free(map);

	/* Omega's closure4 */
	map = isl_map_read_from_str(ctx,
		"[m,n] -> { [x,y] -> [x2,y2] : x2 = x and y2 = y + 1 and "
			"1 <= x,y <= 10 or "
			"x2 = x + 1 and y2 = y and "
			"1 <= x <= 20 && 5 <= y <= 15 }");
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	isl_map_free(map);

	map = isl_map_read_from_str(ctx,
		"[n] -> { [x] -> [y]: 1 <= n <= y - x <= 10 }");
	map = isl_map_transitive_closure(map, &exact);
	assert(!exact);
	map2 = isl_map_read_from_str(ctx,
		"[n] -> { [x] -> [y] : 1 <= n <= 10 and y >= n + x }");
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[n, m] -> { [i0, i1, i2, i3] -> [o0, o1, o2, o3] : "
	    "i3 = 1 and o0 = i0 and o1 = -1 + i1 and o2 = -1 + i2 and "
	    "o3 = -2 + i2 and i1 <= -1 + i0 and i1 >= 1 - m + i0 and "
	    "i1 >= 2 and i1 <= n and i2 >= 3 and i2 <= 1 + n and i2 <= m }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{[0] -> [1]; [2] -> [3]}";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_transitive_closure(map, &exact);
	assert(exact);
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "[n] -> { [[i0, i1, 1, 0, i0] -> [i5, 1]] -> "
	    "[[i0, -1 + i1, 2, 0, i0] -> [-1 + i5, 2]] : "
	    "exists (e0 = [(3 - n)/3]: i5 >= 2 and i1 >= 2 and "
	    "3i0 <= -1 + n and i1 <= -1 + n and i5 <= -1 + n and "
	    "3e0 >= 1 - n and 3e0 <= 2 - n and 3i0 >= -2 + n); "
	    "[[i0, i1, 2, 0, i0] -> [i5, 1]] -> "
	    "[[i0, i1, 1, 0, i0] -> [-1 + i5, 2]] : "
	    "exists (e0 = [(3 - n)/3]: i5 >= 2 and i1 >= 1 and "
	    "3i0 <= -1 + n and i1 <= -1 + n and i5 <= -1 + n and "
	    "3e0 >= 1 - n and 3e0 <= 2 - n and 3i0 >= -2 + n); "
	    "[[i0, i1, 1, 0, i0] -> [i5, 2]] -> "
	    "[[i0, -1 + i1, 2, 0, i0] -> [i5, 1]] : "
	    "exists (e0 = [(3 - n)/3]: i1 >= 2 and i5 >= 1 and "
	    "3i0 <= -1 + n and i1 <= -1 + n and i5 <= -1 + n and "
	    "3e0 >= 1 - n and 3e0 <= 2 - n and 3i0 >= -2 + n); "
	    "[[i0, i1, 2, 0, i0] -> [i5, 2]] -> "
	    "[[i0, i1, 1, 0, i0] -> [i5, 1]] : "
	    "exists (e0 = [(3 - n)/3]: i5 >= 1 and i1 >= 1 and "
	    "3i0 <= -1 + n and i1 <= -1 + n and i5 <= -1 + n and "
	    "3e0 >= 1 - n and 3e0 <= 2 - n and 3i0 >= -2 + n) }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_transitive_closure(map, NULL);
	assert(map);
	isl_map_free(map);
}

void test_lex(struct isl_ctx *ctx)
{
	isl_space *dim;
	isl_map *map;

	dim = isl_space_set_alloc(ctx, 0, 0);
	map = isl_map_lex_le(dim);
	assert(!isl_map_is_empty(map));
	isl_map_free(map);
}

void test_lexmin(struct isl_ctx *ctx)
{
	const char *str;
	isl_basic_map *bmap;
	isl_map *map, *map2;
	isl_set *set;
	isl_set *set2;
	isl_pw_multi_aff *pma;

	str = "[p0, p1] -> { [] -> [] : "
	    "exists (e0 = [(2p1)/3], e1, e2, e3 = [(3 - p1 + 3e0)/3], "
	    "e4 = [(p1)/3], e5 = [(p1 + 3e4)/3]: "
	    "3e0 >= -2 + 2p1 and 3e0 >= p1 and 3e3 >= 1 - p1 + 3e0 and "
	    "3e0 <= 2p1 and 3e3 >= -2 + p1 and 3e3 <= -1 + p1 and p1 >= 3 and "
	    "3e5 >= -2 + 2p1 and 3e5 >= p1 and 3e5 <= -1 + p1 + 3e4 and "
	    "3e4 <= p1 and 3e4 >= -2 + p1 and e3 <= -1 + e0 and "
	    "3e4 >= 6 - p1 + 3e1 and 3e1 >= p1 and 3e5 >= -2 + p1 + 3e4 and "
	    "2e4 >= 3 - p1 + 2e1 and e4 <= e1 and 3e3 <= 2 - p1 + 3e0 and "
	    "e5 >= 1 + e1 and 3e4 >= 6 - 2p1 + 3e1 and "
	    "p0 >= 2 and p1 >= p0 and 3e2 >= p1 and 3e4 >= 6 - p1 + 3e2 and "
	    "e2 <= e1 and e3 >= 1 and e4 <= e2) }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_lexmin(map);
	isl_map_free(map);

	str = "[C] -> { [obj,a,b,c] : obj <= 38 a + 7 b + 10 c and "
	    "a + b <= 1 and c <= 10 b and c <= C and a,b,c,C >= 0 }";
	set = isl_set_read_from_str(ctx, str);
	set = isl_set_lexmax(set);
	str = "[C] -> { [obj,a,b,c] : C = 8 }";
	set2 = isl_set_read_from_str(ctx, str);
	set = isl_set_intersect(set, set2);
	assert(!isl_set_is_empty(set));
	isl_set_free(set);

	str = "{ [x] -> [y] : x <= y <= 10; [x] -> [5] : -8 <= x <= 8 }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_lexmin(map);
	str = "{ [x] -> [5] : 6 <= x <= 8; "
		"[x] -> [x] : x <= 5 or (9 <= x <= 10) }";
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{ [x] -> [y] : 4y = x or 4y = -1 + x or 4y = -2 + x }";
	map = isl_map_read_from_str(ctx, str);
	map2 = isl_map_copy(map);
	map = isl_map_lexmin(map);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{ [x] -> [y] : x = 4y; [x] -> [y] : x = 2y }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_lexmin(map);
	str = "{ [x] -> [y] : (4y = x and x >= 0) or "
		"(exists (e0 = [(x)/4], e1 = [(-2 + x)/4]: 2y = x and "
		"4e1 = -2 + x and 4e0 <= -1 + x and 4e0 >= -3 + x)) or "
		"(exists (e0 = [(x)/4]: 2y = x and 4e0 = x and x <= -4)) }";
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{ [i] -> [i', j] : j = i - 8i' and i' >= 0 and i' <= 7 and "
				" 8i' <= i and 8i' >= -7 + i }";
	bmap = isl_basic_map_read_from_str(ctx, str);
	pma = isl_basic_map_lexmin_pw_multi_aff(isl_basic_map_copy(bmap));
	map2 = isl_map_from_pw_multi_aff(pma);
	map = isl_map_from_basic_map(bmap);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);

	str = "{ T[a] -> S[b, c] : a = 4b-2c and c >= b }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_lexmin(map);
	str = "{ T[a] -> S[b, c] : 2b = a and 2c = a }";
	map2 = isl_map_read_from_str(ctx, str);
	assert(isl_map_is_equal(map, map2));
	isl_map_free(map);
	isl_map_free(map2);
}

struct must_may {
	isl_map *must;
	isl_map *may;
};

static int collect_must_may(__isl_take isl_map *dep, int must,
	void *dep_user, void *user)
{
	struct must_may *mm = (struct must_may *)user;

	if (must)
		mm->must = isl_map_union(mm->must, dep);
	else
		mm->may = isl_map_union(mm->may, dep);

	return 0;
}

static int common_space(void *first, void *second)
{
	int depth = *(int *)first;
	return 2 * depth;
}

static int map_is_equal(__isl_keep isl_map *map, const char *str)
{
	isl_map *map2;
	int equal;

	if (!map)
		return -1;

	map2 = isl_map_read_from_str(map->ctx, str);
	equal = isl_map_is_equal(map, map2);
	isl_map_free(map2);

	return equal;
}

static int map_check_equal(__isl_keep isl_map *map, const char *str)
{
	int equal;

	equal = map_is_equal(map, str);
	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(isl_map_get_ctx(map), isl_error_unknown,
			"result not as expected", return -1);
	return 0;
}

void test_dep(struct isl_ctx *ctx)
{
	const char *str;
	isl_space *dim;
	isl_map *map;
	isl_access_info *ai;
	isl_flow *flow;
	int depth;
	struct must_may mm;

	depth = 3;

	str = "{ [2,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 1, &depth);

	str = "{ [1,i,0] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 1, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_space_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_space_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [2,i,0] : (0 <= i <= 4) or (6 <= i <= 10); "
	      "  [1,10,0] -> [2,5,0] }";
	assert(map_is_equal(mm.must, str));
	str = "{ [i,j,k] -> [l,m,n] : 1 = 0 }";
	assert(map_is_equal(mm.may, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	str = "{ [2,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 1, &depth);

	str = "{ [1,i,0] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_space_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_space_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [2,i,0] : (0 <= i <= 4) or (6 <= i <= 10) }";
	assert(map_is_equal(mm.must, str));
	str = "{ [0,5,0] -> [2,5,0]; [1,i,0] -> [2,5,0] : 0 <= i <= 10 }";
	assert(map_is_equal(mm.may, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	str = "{ [2,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	str = "{ [1,i,0] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_space_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_space_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [2,i,0] : 0 <= i <= 10; "
	      "  [1,i,0] -> [2,5,0] : 0 <= i <= 10 }";
	assert(map_is_equal(mm.may, str));
	str = "{ [i,j,k] -> [l,m,n] : 1 = 0 }";
	assert(map_is_equal(mm.must, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	str = "{ [0,i,2] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	str = "{ [0,i,1] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_space_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_space_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [0,i,2] : 0 <= i <= 10; "
	      "  [0,i,1] -> [0,5,2] : 0 <= i <= 5 }";
	assert(map_is_equal(mm.may, str));
	str = "{ [i,j,k] -> [l,m,n] : 1 = 0 }";
	assert(map_is_equal(mm.must, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	str = "{ [0,i,1] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_alloc(map, &depth, &common_space, 2);

	str = "{ [0,i,0] -> [i] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	str = "{ [0,i,2] -> [5] : 0 <= i <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 0, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_space_alloc(ctx, 0, 3, 3);
	mm.must = isl_map_empty(isl_space_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0] -> [0,i,1] : 0 <= i <= 10; "
	      "  [0,i,2] -> [0,5,1] : 0 <= i <= 4 }";
	assert(map_is_equal(mm.may, str));
	str = "{ [i,j,k] -> [l,m,n] : 1 = 0 }";
	assert(map_is_equal(mm.must, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);


	depth = 5;

	str = "{ [1,i,0,0,0] -> [i,j] : 0 <= i <= 10 and 0 <= j <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_alloc(map, &depth, &common_space, 1);

	str = "{ [0,i,0,j,0] -> [i,j] : 0 <= i <= 10 and 0 <= j <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	ai = isl_access_info_add_source(ai, map, 1, &depth);

	flow = isl_access_info_compute_flow(ai);
	dim = isl_space_alloc(ctx, 0, 5, 5);
	mm.must = isl_map_empty(isl_space_copy(dim));
	mm.may = isl_map_empty(dim);

	isl_flow_foreach(flow, collect_must_may, &mm);

	str = "{ [0,i,0,j,0] -> [1,i,0,0,0] : 0 <= i,j <= 10 }";
	assert(map_is_equal(mm.must, str));
	str = "{ [0,0,0,0,0] -> [0,0,0,0,0] : 1 = 0 }";
	assert(map_is_equal(mm.may, str));

	isl_map_free(mm.must);
	isl_map_free(mm.may);
	isl_flow_free(flow);
}

int test_sv(isl_ctx *ctx)
{
	const char *str;
	isl_map *map;
	isl_union_map *umap;
	int sv;

	str = "[N] -> { [i] -> [f] : 0 <= i <= N and 0 <= i - 10 f <= 9 }";
	map = isl_map_read_from_str(ctx, str);
	sv = isl_map_is_single_valued(map);
	isl_map_free(map);
	if (sv < 0)
		return -1;
	if (!sv)
		isl_die(ctx, isl_error_internal,
			"map not detected as single valued", return -1);

	str = "[N] -> { [i] -> [f] : 0 <= i <= N and 0 <= i - 10 f <= 10 }";
	map = isl_map_read_from_str(ctx, str);
	sv = isl_map_is_single_valued(map);
	isl_map_free(map);
	if (sv < 0)
		return -1;
	if (sv)
		isl_die(ctx, isl_error_internal,
			"map detected as single valued", return -1);

	str = "{ S1[i] -> [i] : 0 <= i <= 9; S2[i] -> [i] : 0 <= i <= 9 }";
	umap = isl_union_map_read_from_str(ctx, str);
	sv = isl_union_map_is_single_valued(umap);
	isl_union_map_free(umap);
	if (sv < 0)
		return -1;
	if (!sv)
		isl_die(ctx, isl_error_internal,
			"map not detected as single valued", return -1);

	str = "{ [i] -> S1[i] : 0 <= i <= 9; [i] -> S2[i] : 0 <= i <= 9 }";
	umap = isl_union_map_read_from_str(ctx, str);
	sv = isl_union_map_is_single_valued(umap);
	isl_union_map_free(umap);
	if (sv < 0)
		return -1;
	if (sv)
		isl_die(ctx, isl_error_internal,
			"map detected as single valued", return -1);

	return 0;
}

void test_bijective_case(struct isl_ctx *ctx, const char *str, int bijective)
{
	isl_map *map;

	map = isl_map_read_from_str(ctx, str);
	if (bijective)
		assert(isl_map_is_bijective(map));
	else
		assert(!isl_map_is_bijective(map));
	isl_map_free(map);
}

void test_bijective(struct isl_ctx *ctx)
{
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i] : j=i}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i] : j=0}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i] : j=N}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [j,i]}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i+j]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> []}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i,j,N]}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [2i]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [i,i]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [2i,i]}", 0);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [2i,j]}", 1);
	test_bijective_case(ctx, "[N,M]->{[i,j] -> [x,y] : 2x=i & y =j}", 1);
}

void test_pwqp(struct isl_ctx *ctx)
{
	const char *str;
	isl_set *set;
	isl_pw_qpolynomial *pwqp1, *pwqp2;

	str = "{ [i,j,k] -> 1 + 9 * [i/5] + 7 * [j/11] + 4 * [k/13] }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_move_dims(pwqp1, isl_dim_param, 0,
						isl_dim_in, 1, 1);

	str = "[j] -> { [i,k] -> 1 + 9 * [i/5] + 7 * [j/11] + 4 * [k/13] }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> i }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [k] : exists a : k = 2a }";
	set = isl_set_read_from_str(ctx, str);
	pwqp1 = isl_pw_qpolynomial_gist(pwqp1, set);
	str = "{ [i] -> i }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> i + [ (i + [i/3])/2 ] }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [10] }";
	set = isl_set_read_from_str(ctx, str);
	pwqp1 = isl_pw_qpolynomial_gist(pwqp1, set);
	str = "{ [i] -> 16 }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> ([(i)/2]) }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [k] : exists a : k = 2a+1 }";
	set = isl_set_read_from_str(ctx, str);
	pwqp1 = isl_pw_qpolynomial_gist(pwqp1, set);
	str = "{ [i] -> -1/2 + 1/2 * i }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> ([([i/2] + [i/2])/5]) }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [i] -> ([(2 * [i/2])/5]) }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [x] -> ([x/2] + [(x+1)/2]) }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	str = "{ [x] -> x }";
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);

	assert(isl_pw_qpolynomial_is_zero(pwqp1));

	isl_pw_qpolynomial_free(pwqp1);

	str = "{ [i] -> ([i/2]) : i >= 0; [i] -> ([i/3]) : i < 0 }";
	pwqp1 = isl_pw_qpolynomial_read_from_str(ctx, str);
	pwqp2 = isl_pw_qpolynomial_read_from_str(ctx, str);
	pwqp1 = isl_pw_qpolynomial_coalesce(pwqp1);
	pwqp1 = isl_pw_qpolynomial_sub(pwqp1, pwqp2);
	assert(isl_pw_qpolynomial_is_zero(pwqp1));
	isl_pw_qpolynomial_free(pwqp1);
}

void test_split_periods(isl_ctx *ctx)
{
	const char *str;
	isl_pw_qpolynomial *pwqp;

	str = "{ [U,V] -> 1/3 * U + 2/3 * V - [(U + 2V)/3] + [U/2] : "
		"U + 2V + 3 >= 0 and - U -2V  >= 0 and - U + 10 >= 0 and "
		"U  >= 0; [U,V] -> U^2 : U >= 100 }";
	pwqp = isl_pw_qpolynomial_read_from_str(ctx, str);

	pwqp = isl_pw_qpolynomial_split_periods(pwqp, 2);
	assert(pwqp);

	isl_pw_qpolynomial_free(pwqp);
}

void test_union(isl_ctx *ctx)
{
	const char *str;
	isl_union_set *uset1, *uset2;
	isl_union_map *umap1, *umap2;

	str = "{ [i] : 0 <= i <= 1 }";
	uset1 = isl_union_set_read_from_str(ctx, str);
	str = "{ [1] -> [0] }";
	umap1 = isl_union_map_read_from_str(ctx, str);

	umap2 = isl_union_set_lex_gt_union_set(isl_union_set_copy(uset1), uset1);
	assert(isl_union_map_is_equal(umap1, umap2));

	isl_union_map_free(umap1);
	isl_union_map_free(umap2);

	str = "{ A[i] -> B[i]; B[i] -> C[i]; A[0] -> C[1] }";
	umap1 = isl_union_map_read_from_str(ctx, str);
	str = "{ A[i]; B[i] }";
	uset1 = isl_union_set_read_from_str(ctx, str);

	uset2 = isl_union_map_domain(umap1);

	assert(isl_union_set_is_equal(uset1, uset2));

	isl_union_set_free(uset1);
	isl_union_set_free(uset2);
}

void test_bound(isl_ctx *ctx)
{
	const char *str;
	isl_pw_qpolynomial *pwqp;
	isl_pw_qpolynomial_fold *pwf;

	str = "{ [[a, b, c, d] -> [e]] -> 0 }";
	pwqp = isl_pw_qpolynomial_read_from_str(ctx, str);
	pwf = isl_pw_qpolynomial_bound(pwqp, isl_fold_max, NULL);
	assert(isl_pw_qpolynomial_fold_dim(pwf, isl_dim_in) == 4);
	isl_pw_qpolynomial_fold_free(pwf);

	str = "{ [[x]->[x]] -> 1 : exists a : x = 2 a }";
	pwqp = isl_pw_qpolynomial_read_from_str(ctx, str);
	pwf = isl_pw_qpolynomial_bound(pwqp, isl_fold_max, NULL);
	assert(isl_pw_qpolynomial_fold_dim(pwf, isl_dim_in) == 1);
	isl_pw_qpolynomial_fold_free(pwf);
}

void test_lift(isl_ctx *ctx)
{
	const char *str;
	isl_basic_map *bmap;
	isl_basic_set *bset;

	str = "{ [i0] : exists e0 : i0 = 4e0 }";
	bset = isl_basic_set_read_from_str(ctx, str);
	bset = isl_basic_set_lift(bset);
	bmap = isl_basic_map_from_range(bset);
	bset = isl_basic_map_domain(bmap);
	isl_basic_set_free(bset);
}

void test_subset(isl_ctx *ctx)
{
	const char *str;
	isl_set *set1, *set2;

	str = "{ [112, 0] }";
	set1 = isl_set_read_from_str(ctx, str);
	str = "{ [i0, i1] : exists (e0 = [(i0 - i1)/16], e1: "
		"16e0 <= i0 - i1 and 16e0 >= -15 + i0 - i1 and "
		"16e1 <= i1 and 16e0 >= -i1 and 16e1 >= -i0 + i1) }";
	set2 = isl_set_read_from_str(ctx, str);
	assert(isl_set_is_subset(set1, set2));
	isl_set_free(set1);
	isl_set_free(set2);
}

int test_factorize(isl_ctx *ctx)
{
	const char *str;
	isl_basic_set *bset;
	isl_factorizer *f;

	str = "{ [i0, i1, i2, i3, i4, i5, i6, i7] : 3i5 <= 2 - 2i0 and "
	    "i0 >= -2 and i6 >= 1 + i3 and i7 >= 0 and 3i5 >= -2i0 and "
	    "2i4 <= i2 and i6 >= 1 + 2i0 + 3i1 and i4 <= -1 and "
	    "i6 >= 1 + 2i0 + 3i5 and i6 <= 2 + 2i0 + 3i5 and "
	    "3i5 <= 2 - 2i0 - i2 + 3i4 and i6 <= 2 + 2i0 + 3i1 and "
	    "i0 <= -1 and i7 <= i2 + i3 - 3i4 - i6 and "
	    "3i5 >= -2i0 - i2 + 3i4 }";
	bset = isl_basic_set_read_from_str(ctx, str);
	f = isl_basic_set_factorizer(bset);
	isl_basic_set_free(bset);
	isl_factorizer_free(f);
	if (!f)
		isl_die(ctx, isl_error_unknown,
			"failed to construct factorizer", return -1);

	str = "{ [i0, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12] : "
	    "i12 <= 2 + i0 - i11 and 2i8 >= -i4 and i11 >= i1 and "
	    "3i5 <= -i2 and 2i11 >= -i4 - 2i7 and i11 <= 3 + i0 + 3i9 and "
	    "i11 <= -i4 - 2i7 and i12 >= -i10 and i2 >= -2 and "
	    "i11 >= i1 + 3i10 and i11 >= 1 + i0 + 3i9 and "
	    "i11 <= 1 - i4 - 2i8 and 6i6 <= 6 - i2 and 3i6 >= 1 - i2 and "
	    "i11 <= 2 + i1 and i12 <= i4 + i11 and i12 >= i0 - i11 and "
	    "3i5 >= -2 - i2 and i12 >= -1 + i4 + i11 and 3i3 <= 3 - i2 and "
	    "9i6 <= 11 - i2 + 6i5 and 3i3 >= 1 - i2 and "
	    "9i6 <= 5 - i2 + 6i3 and i12 <= -1 and i2 <= 0 }";
	bset = isl_basic_set_read_from_str(ctx, str);
	f = isl_basic_set_factorizer(bset);
	isl_basic_set_free(bset);
	isl_factorizer_free(f);
	if (!f)
		isl_die(ctx, isl_error_unknown,
			"failed to construct factorizer", return -1);

	return 0;
}

static int check_injective(__isl_take isl_map *map, void *user)
{
	int *injective = user;

	*injective = isl_map_is_injective(map);
	isl_map_free(map);

	if (*injective < 0 || !*injective)
		return -1;

	return 0;
}

int test_one_schedule(isl_ctx *ctx, const char *d, const char *w,
	const char *r, const char *s, int tilable, int parallel)
{
	int i;
	isl_union_set *D;
	isl_union_map *W, *R, *S;
	isl_union_map *empty;
	isl_union_map *dep_raw, *dep_war, *dep_waw, *dep;
	isl_union_map *validity, *proximity;
	isl_union_map *schedule;
	isl_union_map *test;
	isl_union_set *delta;
	isl_union_set *domain;
	isl_set *delta_set;
	isl_set *slice;
	isl_set *origin;
	isl_schedule *sched;
	int is_nonneg, is_parallel, is_tilable, is_injection, is_complete;

	D = isl_union_set_read_from_str(ctx, d);
	W = isl_union_map_read_from_str(ctx, w);
	R = isl_union_map_read_from_str(ctx, r);
	S = isl_union_map_read_from_str(ctx, s);

	W = isl_union_map_intersect_domain(W, isl_union_set_copy(D));
	R = isl_union_map_intersect_domain(R, isl_union_set_copy(D));

	empty = isl_union_map_empty(isl_union_map_get_space(S));
        isl_union_map_compute_flow(isl_union_map_copy(R),
				   isl_union_map_copy(W), empty,
				   isl_union_map_copy(S),
				   &dep_raw, NULL, NULL, NULL);
        isl_union_map_compute_flow(isl_union_map_copy(W),
				   isl_union_map_copy(W),
				   isl_union_map_copy(R),
				   isl_union_map_copy(S),
				   &dep_waw, &dep_war, NULL, NULL);

	dep = isl_union_map_union(dep_waw, dep_war);
	dep = isl_union_map_union(dep, dep_raw);
	validity = isl_union_map_copy(dep);
	proximity = isl_union_map_copy(dep);

	sched = isl_union_set_compute_schedule(isl_union_set_copy(D),
					       validity, proximity);
	schedule = isl_schedule_get_map(sched);
	isl_schedule_free(sched);
	isl_union_map_free(W);
	isl_union_map_free(R);
	isl_union_map_free(S);

	is_injection = 1;
	isl_union_map_foreach_map(schedule, &check_injective, &is_injection);

	domain = isl_union_map_domain(isl_union_map_copy(schedule));
	is_complete = isl_union_set_is_subset(D, domain);
	isl_union_set_free(D);
	isl_union_set_free(domain);

	test = isl_union_map_reverse(isl_union_map_copy(schedule));
	test = isl_union_map_apply_range(test, dep);
	test = isl_union_map_apply_range(test, schedule);

	delta = isl_union_map_deltas(test);
	if (isl_union_set_n_set(delta) == 0) {
		is_tilable = 1;
		is_parallel = 1;
		is_nonneg = 1;
		isl_union_set_free(delta);
	} else {
		delta_set = isl_set_from_union_set(delta);

		slice = isl_set_universe(isl_set_get_space(delta_set));
		for (i = 0; i < tilable; ++i)
			slice = isl_set_lower_bound_si(slice, isl_dim_set, i, 0);
		is_tilable = isl_set_is_subset(delta_set, slice);
		isl_set_free(slice);

		slice = isl_set_universe(isl_set_get_space(delta_set));
		for (i = 0; i < parallel; ++i)
			slice = isl_set_fix_si(slice, isl_dim_set, i, 0);
		is_parallel = isl_set_is_subset(delta_set, slice);
		isl_set_free(slice);

		origin = isl_set_universe(isl_set_get_space(delta_set));
		for (i = 0; i < isl_set_dim(origin, isl_dim_set); ++i)
			origin = isl_set_fix_si(origin, isl_dim_set, i, 0);

		delta_set = isl_set_union(delta_set, isl_set_copy(origin));
		delta_set = isl_set_lexmin(delta_set);

		is_nonneg = isl_set_is_equal(delta_set, origin);

		isl_set_free(origin);
		isl_set_free(delta_set);
	}

	if (is_nonneg < 0 || is_parallel < 0 || is_tilable < 0 ||
	    is_injection < 0 || is_complete < 0)
		return -1;
	if (!is_complete)
		isl_die(ctx, isl_error_unknown,
			"generated schedule incomplete", return -1);
	if (!is_injection)
		isl_die(ctx, isl_error_unknown,
			"generated schedule not injective on each statement",
			return -1);
	if (!is_nonneg)
		isl_die(ctx, isl_error_unknown,
			"negative dependences in generated schedule",
			return -1);
	if (!is_tilable)
		isl_die(ctx, isl_error_unknown,
			"generated schedule not as tilable as expected",
			return -1);
	if (!is_parallel)
		isl_die(ctx, isl_error_unknown,
			"generated schedule not as parallel as expected",
			return -1);

	return 0;
}

int test_special_schedule(isl_ctx *ctx, const char *domain,
	const char *validity, const char *proximity, const char *expected_sched)
{
	isl_union_set *dom;
	isl_union_map *dep;
	isl_union_map *prox;
	isl_union_map *sched1, *sched2;
	isl_schedule *schedule;
	int equal;

	dom = isl_union_set_read_from_str(ctx, domain);
	dep = isl_union_map_read_from_str(ctx, validity);
	prox = isl_union_map_read_from_str(ctx, proximity);
	schedule = isl_union_set_compute_schedule(dom, dep, prox);
	sched1 = isl_schedule_get_map(schedule);
	isl_schedule_free(schedule);

	sched2 = isl_union_map_read_from_str(ctx, expected_sched);

	equal = isl_union_map_is_equal(sched1, sched2);
	isl_union_map_free(sched1);
	isl_union_map_free(sched2);

	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown, "unexpected schedule",
			return -1);

	return 0;
}

int test_schedule(isl_ctx *ctx)
{
	const char *D, *W, *R, *V, *P, *S;

	/* Jacobi */
	D = "[T,N] -> { S1[t,i] : 1 <= t <= T and 2 <= i <= N - 1 }";
	W = "{ S1[t,i] -> a[t,i] }";
	R = "{ S1[t,i] -> a[t-1,i]; S1[t,i] -> a[t-1,i-1]; "
	    	"S1[t,i] -> a[t-1,i+1] }";
	S = "{ S1[t,i] -> [t,i] }";
	if (test_one_schedule(ctx, D, W, R, S, 2, 0) < 0)
		return -1;

	/* Fig. 5 of CC2008 */
	D = "[N] -> { S_0[i, j] : i >= 0 and i <= -1 + N and j >= 2 and "
				"j <= -1 + N }";
	W = "[N] -> { S_0[i, j] -> a[i, j] : i >= 0 and i <= -1 + N and "
				"j >= 2 and j <= -1 + N }";
	R = "[N] -> { S_0[i, j] -> a[j, i] : i >= 0 and i <= -1 + N and "
				"j >= 2 and j <= -1 + N; "
		    "S_0[i, j] -> a[i, -1 + j] : i >= 0 and i <= -1 + N and "
				"j >= 2 and j <= -1 + N }";
	S = "[N] -> { S_0[i, j] -> [0, i, 0, j, 0] }";
	if (test_one_schedule(ctx, D, W, R, S, 2, 0) < 0)
		return -1;

	D = "{ S1[i] : 0 <= i <= 10; S2[i] : 0 <= i <= 9 }";
	W = "{ S1[i] -> a[i] }";
	R = "{ S2[i] -> a[i+1] }";
	S = "{ S1[i] -> [0,i]; S2[i] -> [1,i] }";
	if (test_one_schedule(ctx, D, W, R, S, 1, 1) < 0)
		return -1;

	D = "{ S1[i] : 0 <= i < 10; S2[i] : 0 <= i < 10 }";
	W = "{ S1[i] -> a[i] }";
	R = "{ S2[i] -> a[9-i] }";
	S = "{ S1[i] -> [0,i]; S2[i] -> [1,i] }";
	if (test_one_schedule(ctx, D, W, R, S, 1, 1) < 0)
		return -1;

	D = "[N] -> { S1[i] : 0 <= i < N; S2[i] : 0 <= i < N }";
	W = "{ S1[i] -> a[i] }";
	R = "[N] -> { S2[i] -> a[N-1-i] }";
	S = "{ S1[i] -> [0,i]; S2[i] -> [1,i] }";
	if (test_one_schedule(ctx, D, W, R, S, 1, 1) < 0)
		return -1;
	
	D = "{ S1[i] : 0 < i < 10; S2[i] : 0 <= i < 10 }";
	W = "{ S1[i] -> a[i]; S2[i] -> b[i] }";
	R = "{ S2[i] -> a[i]; S1[i] -> b[i-1] }";
	S = "{ S1[i] -> [i,0]; S2[i] -> [i,1] }";
	if (test_one_schedule(ctx, D, W, R, S, 0, 0) < 0)
		return -1;

	D = "[N] -> { S1[i] : 1 <= i <= N; S2[i,j] : 1 <= i,j <= N }";
	W = "{ S1[i] -> a[0,i]; S2[i,j] -> a[i,j] }";
	R = "{ S2[i,j] -> a[i-1,j] }";
	S = "{ S1[i] -> [0,i,0]; S2[i,j] -> [1,i,j] }";
	if (test_one_schedule(ctx, D, W, R, S, 2, 1) < 0)
		return -1;

	D = "[N] -> { S1[i] : 1 <= i <= N; S2[i,j] : 1 <= i,j <= N }";
	W = "{ S1[i] -> a[i,0]; S2[i,j] -> a[i,j] }";
	R = "{ S2[i,j] -> a[i,j-1] }";
	S = "{ S1[i] -> [0,i,0]; S2[i,j] -> [1,i,j] }";
	if (test_one_schedule(ctx, D, W, R, S, 2, 1) < 0)
		return -1;

	D = "[N] -> { S_0[]; S_1[i] : i >= 0 and i <= -1 + N; S_2[] }";
	W = "[N] -> { S_0[] -> a[0]; S_2[] -> b[0]; "
		    "S_1[i] -> a[1 + i] : i >= 0 and i <= -1 + N }";
	R = "[N] -> { S_2[] -> a[N]; S_1[i] -> a[i] : i >= 0 and i <= -1 + N }";
	S = "[N] -> { S_1[i] -> [1, i, 0]; S_2[] -> [2, 0, 1]; "
		    "S_0[] -> [0, 0, 0] }";
	if (test_one_schedule(ctx, D, W, R, S, 1, 0) < 0)
		return -1;
	ctx->opt->schedule_parametric = 0;
	if (test_one_schedule(ctx, D, W, R, S, 0, 0) < 0)
		return -1;
	ctx->opt->schedule_parametric = 1;

	D = "[N] -> { S1[i] : 1 <= i <= N; S2[i] : 1 <= i <= N; "
		    "S3[i,j] : 1 <= i,j <= N; S4[i] : 1 <= i <= N }";
	W = "{ S1[i] -> a[i,0]; S2[i] -> a[0,i]; S3[i,j] -> a[i,j] }";
	R = "[N] -> { S3[i,j] -> a[i-1,j]; S3[i,j] -> a[i,j-1]; "
		    "S4[i] -> a[i,N] }";
	S = "{ S1[i] -> [0,i,0]; S2[i] -> [1,i,0]; S3[i,j] -> [2,i,j]; "
		"S4[i] -> [4,i,0] }";
	if (test_one_schedule(ctx, D, W, R, S, 2, 0) < 0)
		return -1;

	D = "[N] -> { S_0[i, j] : i >= 1 and i <= N and j >= 1 and j <= N }";
	W = "[N] -> { S_0[i, j] -> s[0] : i >= 1 and i <= N and j >= 1 and "
					"j <= N }";
	R = "[N] -> { S_0[i, j] -> s[0] : i >= 1 and i <= N and j >= 1 and "
					"j <= N; "
		    "S_0[i, j] -> a[i, j] : i >= 1 and i <= N and j >= 1 and "
					"j <= N }";
	S = "[N] -> { S_0[i, j] -> [0, i, 0, j, 0] }";
	if (test_one_schedule(ctx, D, W, R, S, 0, 0) < 0)
		return -1;

	D = "[N] -> { S_0[t] : t >= 0 and t <= -1 + N; "
		    " S_2[t] : t >= 0 and t <= -1 + N; "
		    " S_1[t, i] : t >= 0 and t <= -1 + N and i >= 0 and "
				"i <= -1 + N }";
	W = "[N] -> { S_0[t] -> a[t, 0] : t >= 0 and t <= -1 + N; "
		    " S_2[t] -> b[t] : t >= 0 and t <= -1 + N; "
		    " S_1[t, i] -> a[t, 1 + i] : t >= 0 and t <= -1 + N and "
						"i >= 0 and i <= -1 + N }";
	R = "[N] -> { S_1[t, i] -> a[t, i] : t >= 0 and t <= -1 + N and "
					    "i >= 0 and i <= -1 + N; "
		    " S_2[t] -> a[t, N] : t >= 0 and t <= -1 + N }";
	S = "[N] -> { S_2[t] -> [0, t, 2]; S_1[t, i] -> [0, t, 1, i, 0]; "
		    " S_0[t] -> [0, t, 0] }";

	if (test_one_schedule(ctx, D, W, R, S, 2, 1) < 0)
		return -1;
	ctx->opt->schedule_parametric = 0;
	if (test_one_schedule(ctx, D, W, R, S, 0, 0) < 0)
		return -1;
	ctx->opt->schedule_parametric = 1;

	D = "[N] -> { S1[i,j] : 0 <= i,j < N; S2[i,j] : 0 <= i,j < N }";
	S = "{ S1[i,j] -> [0,i,j]; S2[i,j] -> [1,i,j] }";
	if (test_one_schedule(ctx, D, "{}", "{}", S, 2, 2) < 0)
		return -1;

	D = "[M, N] -> { S_1[i] : i >= 0 and i <= -1 + M; "
	    "S_0[i, j] : i >= 0 and i <= -1 + M and j >= 0 and j <= -1 + N }";
	W = "[M, N] -> { S_0[i, j] -> a[j] : i >= 0 and i <= -1 + M and "
					    "j >= 0 and j <= -1 + N; "
			"S_1[i] -> b[0] : i >= 0 and i <= -1 + M }";
	R = "[M, N] -> { S_0[i, j] -> a[0] : i >= 0 and i <= -1 + M and "
					    "j >= 0 and j <= -1 + N; "
			"S_1[i] -> b[0] : i >= 0 and i <= -1 + M }";
	S = "[M, N] -> { S_1[i] -> [1, i, 0]; S_0[i, j] -> [0, i, 0, j, 0] }";
	if (test_one_schedule(ctx, D, W, R, S, 0, 0) < 0)
		return -1;

	D = "{ S_0[i] : i >= 0 }";
	W = "{ S_0[i] -> a[i] : i >= 0 }";
	R = "{ S_0[i] -> a[0] : i >= 0 }";
	S = "{ S_0[i] -> [0, i, 0] }";
	if (test_one_schedule(ctx, D, W, R, S, 0, 0) < 0)
		return -1;

	D = "{ S_0[i] : i >= 0; S_1[i] : i >= 0 }";
	W = "{ S_0[i] -> a[i] : i >= 0; S_1[i] -> b[i] : i >= 0 }";
	R = "{ S_0[i] -> b[0] : i >= 0; S_1[i] -> a[i] : i >= 0 }";
	S = "{ S_1[i] -> [0, i, 1]; S_0[i] -> [0, i, 0] }";
	if (test_one_schedule(ctx, D, W, R, S, 0, 0) < 0)
		return -1;

	D = "[n] -> { S_0[j, k] : j <= -1 + n and j >= 0 and "
				"k <= -1 + n and k >= 0 }";
	W = "[n] -> { S_0[j, k] -> B[j] : j <= -1 + n and j >= 0 and "							"k <= -1 + n and k >= 0 }";
	R = "[n] -> { S_0[j, k] -> B[j] : j <= -1 + n and j >= 0 and "
					"k <= -1 + n and k >= 0; "
		    "S_0[j, k] -> B[k] : j <= -1 + n and j >= 0 and "
					"k <= -1 + n and k >= 0; "
		    "S_0[j, k] -> A[k] : j <= -1 + n and j >= 0 and "
					"k <= -1 + n and k >= 0 }";
	S = "[n] -> { S_0[j, k] -> [2, j, k] }";
	ctx->opt->schedule_outer_zero_distance = 1;
	if (test_one_schedule(ctx, D, W, R, S, 0, 0) < 0)
		return -1;
	ctx->opt->schedule_outer_zero_distance = 0;

	D = "{Stmt_for_body24[i0, i1, i2, i3]:"
		"i0 >= 0 and i0 <= 1 and i1 >= 0 and i1 <= 6 and i2 >= 2 and "
		"i2 <= 6 - i1 and i3 >= 0 and i3 <= -1 + i2;"
	     "Stmt_for_body24[i0, i1, 1, 0]:"
		"i0 >= 0 and i0 <= 1 and i1 >= 0 and i1 <= 5;"
	     "Stmt_for_body7[i0, i1, i2]:"
		"i0 >= 0 and i0 <= 1 and i1 >= 0 and i1 <= 7 and i2 >= 0 and "
		"i2 <= 7 }";

	V = "{Stmt_for_body24[0, i1, i2, i3] -> "
		"Stmt_for_body24[1, i1, i2, i3]:"
		"i3 >= 0 and i3 <= -1 + i2 and i1 >= 0 and i2 <= 6 - i1 and "
		"i2 >= 1;"
	     "Stmt_for_body24[0, i1, i2, i3] -> "
		"Stmt_for_body7[1, 1 + i1 + i3, 1 + i1 + i2]:"
		"i3 <= -1 + i2 and i2 <= 6 - i1 and i2 >= 1 and i1 >= 0 and "
		"i3 >= 0;"
	      "Stmt_for_body24[0, i1, i2, i3] ->"
		"Stmt_for_body7[1, i1, 1 + i1 + i3]:"
		"i3 >= 0 and i2 <= 6 - i1 and i1 >= 0 and i3 <= -1 + i2;"
	      "Stmt_for_body7[0, i1, i2] -> Stmt_for_body7[1, i1, i2]:"
		"(i2 >= 1 + i1 and i2 <= 6 and i1 >= 0 and i1 <= 4) or "
		"(i2 >= 3 and i2 <= 7 and i1 >= 1 and i2 >= 1 + i1) or "
		"(i2 >= 0 and i2 <= i1 and i2 >= -7 + i1 and i1 <= 7);"
	      "Stmt_for_body7[0, i1, 1 + i1] -> Stmt_for_body7[1, i1, 1 + i1]:"
		"i1 <= 6 and i1 >= 0;"
	      "Stmt_for_body7[0, 0, 7] -> Stmt_for_body7[1, 0, 7];"
	      "Stmt_for_body7[i0, i1, i2] -> "
		"Stmt_for_body24[i0, o1, -1 + i2 - o1, -1 + i1 - o1]:"
		"i0 >= 0 and i0 <= 1 and o1 >= 0 and i2 >= 1 + i1 and "
		"o1 <= -2 + i2 and i2 <= 7 and o1 <= -1 + i1;"
	      "Stmt_for_body7[i0, i1, i2] -> "
		"Stmt_for_body24[i0, i1, o2, -1 - i1 + i2]:"
		"i0 >= 0 and i0 <= 1 and i1 >= 0 and o2 >= -i1 + i2 and "
		"o2 >= 1 and o2 <= 6 - i1 and i2 >= 1 + i1 }";
	P = V;
	S = "{ Stmt_for_body24[i0, i1, i2, i3] -> "
		"[i0, 5i0 + i1, 6i0 + i1 + i2, 1 + 6i0 + i1 + i2 + i3, 1];"
	    "Stmt_for_body7[i0, i1, i2] -> [0, 5i0, 6i0 + i1, 6i0 + i2, 0] }";

	if (test_special_schedule(ctx, D, V, P, S) < 0)
		return -1;

	D = "{ S_0[i, j] : i >= 1 and i <= 10 and j >= 1 and j <= 8 }";
	V = "{ S_0[i, j] -> S_0[i, 1 + j] : i >= 1 and i <= 10 and "
					   "j >= 1 and j <= 7;"
		"S_0[i, j] -> S_0[1 + i, j] : i >= 1 and i <= 9 and "
					     "j >= 1 and j <= 8 }";
	P = "{ }";
	S = "{ S_0[i, j] -> [i + j, j] }";
	ctx->opt->schedule_algorithm = ISL_SCHEDULE_ALGORITHM_FEAUTRIER;
	if (test_special_schedule(ctx, D, V, P, S) < 0)
		return -1;
	ctx->opt->schedule_algorithm = ISL_SCHEDULE_ALGORITHM_ISL;

	/* Fig. 1 from Feautrier's "Some Efficient Solutions..." pt. 2, 1992 */
	D = "[N] -> { S_0[i, j] : i >= 0 and i <= -1 + N and "
				 "j >= 0 and j <= -1 + i }";
	V = "[N] -> { S_0[i, j] -> S_0[i, 1 + j] : j <= -2 + i and "
					"i <= -1 + N and j >= 0;"
		     "S_0[i, -1 + i] -> S_0[1 + i, 0] : i >= 1 and "
					"i <= -2 + N }";
	P = "{ }";
	S = "{ S_0[i, j] -> [i, j] }";
	ctx->opt->schedule_algorithm = ISL_SCHEDULE_ALGORITHM_FEAUTRIER;
	if (test_special_schedule(ctx, D, V, P, S) < 0)
		return -1;
	ctx->opt->schedule_algorithm = ISL_SCHEDULE_ALGORITHM_ISL;

	/* Test both algorithms on a case with only proximity dependences. */
	D = "{ S[i,j] : 0 <= i <= 10 }";
	V = "{ }";
	P = "{ S[i,j] -> S[i+1,j] : 0 <= i,j <= 10 }";
	S = "{ S[i, j] -> [j, i] }";
	ctx->opt->schedule_algorithm = ISL_SCHEDULE_ALGORITHM_FEAUTRIER;
	if (test_special_schedule(ctx, D, V, P, S) < 0)
		return -1;
	ctx->opt->schedule_algorithm = ISL_SCHEDULE_ALGORITHM_ISL;
	return test_special_schedule(ctx, D, V, P, S);
}

int test_plain_injective(isl_ctx *ctx, const char *str, int injective)
{
	isl_union_map *umap;
	int test;

	umap = isl_union_map_read_from_str(ctx, str);
	test = isl_union_map_plain_is_injective(umap);
	isl_union_map_free(umap);
	if (test < 0)
		return -1;
	if (test == injective)
		return 0;
	if (injective)
		isl_die(ctx, isl_error_unknown,
			"map not detected as injective", return -1);
	else
		isl_die(ctx, isl_error_unknown,
			"map detected as injective", return -1);
}

int test_injective(isl_ctx *ctx)
{
	const char *str;

	if (test_plain_injective(ctx, "{S[i,j] -> A[0]; T[i,j] -> B[1]}", 0))
		return -1;
	if (test_plain_injective(ctx, "{S[] -> A[0]; T[] -> B[0]}", 1))
		return -1;
	if (test_plain_injective(ctx, "{S[] -> A[0]; T[] -> A[1]}", 1))
		return -1;
	if (test_plain_injective(ctx, "{S[] -> A[0]; T[] -> A[0]}", 0))
		return -1;
	if (test_plain_injective(ctx, "{S[i] -> A[i,0]; T[i] -> A[i,1]}", 1))
		return -1;
	if (test_plain_injective(ctx, "{S[i] -> A[i]; T[i] -> A[i]}", 0))
		return -1;
	if (test_plain_injective(ctx, "{S[] -> A[0,0]; T[] -> A[0,1]}", 1))
		return -1;
	if (test_plain_injective(ctx, "{S[] -> A[0,0]; T[] -> A[1,0]}", 1))
		return -1;

	str = "{S[] -> A[0,0]; T[] -> A[0,1]; U[] -> A[1,0]}";
	if (test_plain_injective(ctx, str, 1))
		return -1;
	str = "{S[] -> A[0,0]; T[] -> A[0,1]; U[] -> A[0,0]}";
	if (test_plain_injective(ctx, str, 0))
		return -1;

	return 0;
}

static int aff_plain_is_equal(__isl_keep isl_aff *aff, const char *str)
{
	isl_aff *aff2;
	int equal;

	if (!aff)
		return -1;

	aff2 = isl_aff_read_from_str(isl_aff_get_ctx(aff), str);
	equal = isl_aff_plain_is_equal(aff, aff2);
	isl_aff_free(aff2);

	return equal;
}

static int aff_check_plain_equal(__isl_keep isl_aff *aff, const char *str)
{
	int equal;

	equal = aff_plain_is_equal(aff, str);
	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(isl_aff_get_ctx(aff), isl_error_unknown,
			"result not as expected", return -1);
	return 0;
}

int test_aff(isl_ctx *ctx)
{
	const char *str;
	isl_set *set;
	isl_space *space;
	isl_local_space *ls;
	isl_aff *aff;
	int zero, equal;

	space = isl_space_set_alloc(ctx, 0, 1);
	ls = isl_local_space_from_space(space);
	aff = isl_aff_zero_on_domain(ls);

	aff = isl_aff_add_coefficient_si(aff, isl_dim_in, 0, 1);
	aff = isl_aff_scale_down_ui(aff, 3);
	aff = isl_aff_floor(aff);
	aff = isl_aff_add_coefficient_si(aff, isl_dim_in, 0, 1);
	aff = isl_aff_scale_down_ui(aff, 2);
	aff = isl_aff_floor(aff);
	aff = isl_aff_add_coefficient_si(aff, isl_dim_in, 0, 1);

	str = "{ [10] }";
	set = isl_set_read_from_str(ctx, str);
	aff = isl_aff_gist(aff, set);

	aff = isl_aff_add_constant_si(aff, -16);
	zero = isl_aff_plain_is_zero(aff);
	isl_aff_free(aff);

	if (zero < 0)
		return -1;
	if (!zero)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	aff = isl_aff_read_from_str(ctx, "{ [-1] }");
	aff = isl_aff_scale_down_ui(aff, 64);
	aff = isl_aff_floor(aff);
	equal = aff_check_plain_equal(aff, "{ [-1] }");
	isl_aff_free(aff);
	if (equal < 0)
		return -1;

	return 0;
}

int test_dim_max(isl_ctx *ctx)
{
	int equal;
	const char *str;
	isl_set *set1, *set2;
	isl_set *set;
	isl_map *map;
	isl_pw_aff *pwaff;

	str = "[N] -> { [i] : 0 <= i <= min(N,10) }";
	set = isl_set_read_from_str(ctx, str);
	pwaff = isl_set_dim_max(set, 0);
	set1 = isl_set_from_pw_aff(pwaff);
	str = "[N] -> { [10] : N >= 10; [N] : N <= 9 and N >= 0 }";
	set2 = isl_set_read_from_str(ctx, str);
	equal = isl_set_is_equal(set1, set2);
	isl_set_free(set1);
	isl_set_free(set2);
	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	str = "[N] -> { [i] : 0 <= i <= max(2N,N+6) }";
	set = isl_set_read_from_str(ctx, str);
	pwaff = isl_set_dim_max(set, 0);
	set1 = isl_set_from_pw_aff(pwaff);
	str = "[N] -> { [6 + N] : -6 <= N <= 5; [2N] : N >= 6 }";
	set2 = isl_set_read_from_str(ctx, str);
	equal = isl_set_is_equal(set1, set2);
	isl_set_free(set1);
	isl_set_free(set2);
	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	str = "[N] -> { [i] : 0 <= i <= 2N or 0 <= i <= N+6 }";
	set = isl_set_read_from_str(ctx, str);
	pwaff = isl_set_dim_max(set, 0);
	set1 = isl_set_from_pw_aff(pwaff);
	str = "[N] -> { [6 + N] : -6 <= N <= 5; [2N] : N >= 6 }";
	set2 = isl_set_read_from_str(ctx, str);
	equal = isl_set_is_equal(set1, set2);
	isl_set_free(set1);
	isl_set_free(set2);
	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	str = "[N,M] -> { [i,j] -> [([i/16]), i%16, ([j/16]), j%16] : "
			"0 <= i < N and 0 <= j < M }";
	map = isl_map_read_from_str(ctx, str);
	set = isl_map_range(map);

	pwaff = isl_set_dim_max(isl_set_copy(set), 0);
	set1 = isl_set_from_pw_aff(pwaff);
	str = "[N,M] -> { [([(N-1)/16])] : M,N > 0 }";
	set2 = isl_set_read_from_str(ctx, str);
	equal = isl_set_is_equal(set1, set2);
	isl_set_free(set1);
	isl_set_free(set2);

	pwaff = isl_set_dim_max(isl_set_copy(set), 3);
	set1 = isl_set_from_pw_aff(pwaff);
	str = "[N,M] -> { [t] : t = min(M-1,15) and M,N > 0 }";
	set2 = isl_set_read_from_str(ctx, str);
	if (equal >= 0 && equal)
		equal = isl_set_is_equal(set1, set2);
	isl_set_free(set1);
	isl_set_free(set2);

	isl_set_free(set);

	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	return 0;
}

int test_product(isl_ctx *ctx)
{
	const char *str;
	isl_set *set;
	isl_union_set *uset1, *uset2;
	int ok;

	str = "{ A[i] }";
	set = isl_set_read_from_str(ctx, str);
	set = isl_set_product(set, isl_set_copy(set));
	ok = isl_set_is_wrapping(set);
	isl_set_free(set);
	if (ok < 0)
		return -1;
	if (!ok)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	str = "{ [] }";
	uset1 = isl_union_set_read_from_str(ctx, str);
	uset1 = isl_union_set_product(uset1, isl_union_set_copy(uset1));
	str = "{ [[] -> []] }";
	uset2 = isl_union_set_read_from_str(ctx, str);
	ok = isl_union_set_is_equal(uset1, uset2);
	isl_union_set_free(uset1);
	isl_union_set_free(uset2);
	if (ok < 0)
		return -1;
	if (!ok)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	return 0;
}

int test_equal(isl_ctx *ctx)
{
	const char *str;
	isl_set *set, *set2;
	int equal;

	str = "{ S_6[i] }";
	set = isl_set_read_from_str(ctx, str);
	str = "{ S_7[i] }";
	set2 = isl_set_read_from_str(ctx, str);
	equal = isl_set_is_equal(set, set2);
	isl_set_free(set);
	isl_set_free(set2);
	if (equal < 0)
		return -1;
	if (equal)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	return 0;
}

static int test_plain_fixed(isl_ctx *ctx, __isl_take isl_map *map,
	enum isl_dim_type type, unsigned pos, int fixed)
{
	int test;

	test = isl_map_plain_is_fixed(map, type, pos, NULL);
	isl_map_free(map);
	if (test < 0)
		return -1;
	if (test == fixed)
		return 0;
	if (fixed)
		isl_die(ctx, isl_error_unknown,
			"map not detected as fixed", return -1);
	else
		isl_die(ctx, isl_error_unknown,
			"map detected as fixed", return -1);
}

int test_fixed(isl_ctx *ctx)
{
	const char *str;
	isl_map *map;

	str = "{ [i] -> [i] }";
	map = isl_map_read_from_str(ctx, str);
	if (test_plain_fixed(ctx, map, isl_dim_out, 0, 0))
		return -1;
	str = "{ [i] -> [1] }";
	map = isl_map_read_from_str(ctx, str);
	if (test_plain_fixed(ctx, map, isl_dim_out, 0, 1))
		return -1;
	str = "{ S_1[p1] -> [o0] : o0 = -2 and p1 >= 1 and p1 <= 7 }";
	map = isl_map_read_from_str(ctx, str);
	if (test_plain_fixed(ctx, map, isl_dim_out, 0, 1))
		return -1;
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_neg(map);
	if (test_plain_fixed(ctx, map, isl_dim_out, 0, 1))
		return -1;

	return 0;
}

int test_vertices(isl_ctx *ctx)
{
	const char *str;
	isl_basic_set *bset;
	isl_vertices *vertices;

	str = "{ A[t, i] : t = 12 and i >= 4 and i <= 12 }";
	bset = isl_basic_set_read_from_str(ctx, str);
	vertices = isl_basic_set_compute_vertices(bset);
	isl_basic_set_free(bset);
	isl_vertices_free(vertices);
	if (!vertices)
		return -1;

	str = "{ A[t, i] : t = 14 and i = 1 }";
	bset = isl_basic_set_read_from_str(ctx, str);
	vertices = isl_basic_set_compute_vertices(bset);
	isl_basic_set_free(bset);
	isl_vertices_free(vertices);
	if (!vertices)
		return -1;

	return 0;
}

int test_union_pw(isl_ctx *ctx)
{
	int equal;
	const char *str;
	isl_union_set *uset;
	isl_union_pw_qpolynomial *upwqp1, *upwqp2;

	str = "{ [x] -> x^2 }";
	upwqp1 = isl_union_pw_qpolynomial_read_from_str(ctx, str);
	upwqp2 = isl_union_pw_qpolynomial_copy(upwqp1);
	uset = isl_union_pw_qpolynomial_domain(upwqp1);
	upwqp1 = isl_union_pw_qpolynomial_copy(upwqp2);
	upwqp1 = isl_union_pw_qpolynomial_intersect_domain(upwqp1, uset);
	equal = isl_union_pw_qpolynomial_plain_is_equal(upwqp1, upwqp2);
	isl_union_pw_qpolynomial_free(upwqp1);
	isl_union_pw_qpolynomial_free(upwqp2);
	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	return 0;
}

int test_output(isl_ctx *ctx)
{
	char *s;
	const char *str;
	isl_pw_aff *pa;
	isl_printer *p;
	int equal;

	str = "[x] -> { [1] : x % 4 <= 2; [2] : x = 3 }";
	pa = isl_pw_aff_read_from_str(ctx, str);

	p = isl_printer_to_str(ctx);
	p = isl_printer_set_output_format(p, ISL_FORMAT_C);
	p = isl_printer_print_pw_aff(p, pa);
	s = isl_printer_get_str(p);
	isl_printer_free(p);
	isl_pw_aff_free(pa);
	equal = !strcmp(s, "(2 - x + 4*floord(x, 4) >= 0) ? (1) : 2");
	free(s);
	if (equal < 0)
		return -1;
	if (!equal)
		isl_die(ctx, isl_error_unknown, "unexpected result", return -1);

	return 0;
}

int test_sample(isl_ctx *ctx)
{
	const char *str;
	isl_basic_set *bset1, *bset2;
	int empty, subset;

	str = "{ [a, b, c, d, e, f, g, h, i, j, k] : "
	    "3i >= 1073741823b - c - 1073741823e + f and c >= 0 and "
	    "3i >= -1 + 3221225466b + c + d - 3221225466e - f and "
	    "2e >= a - b and 3e <= 2a and 3k <= -a and f <= -1 + a and "
	    "3i <= 4 - a + 4b + 2c - e - 2f and 3k <= -a + c - f and "
	    "3h >= -2 + a and 3g >= -3 - a and 3k >= -2 - a and "
	    "3i >= -2 - a - 2c + 3e + 2f and 3h <= a + c - f and "
	    "3h >= a + 2147483646b + 2c - 2147483646e - 2f and "
	    "3g <= -1 - a and 3i <= 1 + c + d - f and a <= 1073741823 and "
	    "f >= 1 - a + 1073741822b + c + d - 1073741822e and "
	    "3i >= 1 + 2b - 2c + e + 2f + 3g and "
	    "1073741822f <= 1073741822 - a + 1073741821b + 1073741822c +"
		"d - 1073741821e and "
	    "3j <= 3 - a + 3b and 3g <= -2 - 2b + c + d - e - f and "
	    "3j >= 1 - a + b + 2e and "
	    "3f >= -3 + a + 3221225462b + 3c + d - 3221225465e and "
	    "3i <= 4 - a + 4b - e and "
	    "f <= 1073741822 + 1073741822b - 1073741822e and 3h <= a and "
	    "f >= 0 and 2e <= 4 - a + 5b - d and 2e <= a - b + d and "
	    "c <= -1 + a and 3i >= -2 - a + 3e and "
	    "1073741822e <= 1073741823 - a + 1073741822b + c and "
	    "3g >= -4 + 3221225464b + 3c + d - 3221225467e - 3f and "
	    "3i >= -1 + 3221225466b + 3c + d - 3221225466e - 3f and "
	    "1073741823e >= 1 + 1073741823b - d and "
	    "3i >= 1073741823b + c - 1073741823e - f and "
	    "3i >= 1 + 2b + e + 3g }";
	bset1 = isl_basic_set_read_from_str(ctx, str);
	bset2 = isl_basic_set_sample(isl_basic_set_copy(bset1));
	empty = isl_basic_set_is_empty(bset2);
	subset = isl_basic_set_is_subset(bset2, bset1);
	isl_basic_set_free(bset1);
	isl_basic_set_free(bset2);
	if (empty)
		isl_die(ctx, isl_error_unknown, "point not found", return -1);
	if (!subset)
		isl_die(ctx, isl_error_unknown, "bad point found", return -1);

	return 0;
}

int test_fixed_power(isl_ctx *ctx)
{
	const char *str;
	isl_map *map;
	isl_int exp;
	int equal;

	isl_int_init(exp);
	str = "{ [i] -> [i + 1] }";
	map = isl_map_read_from_str(ctx, str);
	isl_int_set_si(exp, 23);
	map = isl_map_fixed_power(map, exp);
	equal = map_check_equal(map, "{ [i] -> [i + 23] }");
	isl_int_clear(exp);
	isl_map_free(map);
	if (equal < 0)
		return -1;

	return 0;
}

int test_slice(isl_ctx *ctx)
{
	const char *str;
	isl_map *map;
	int equal;

	str = "{ [i] -> [j] }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_equate(map, isl_dim_in, 0, isl_dim_out, 0);
	equal = map_check_equal(map, "{ [i] -> [i] }");
	isl_map_free(map);
	if (equal < 0)
		return -1;

	str = "{ [i] -> [j] }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_equate(map, isl_dim_in, 0, isl_dim_in, 0);
	equal = map_check_equal(map, "{ [i] -> [j] }");
	isl_map_free(map);
	if (equal < 0)
		return -1;

	str = "{ [i] -> [j] }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_oppose(map, isl_dim_in, 0, isl_dim_out, 0);
	equal = map_check_equal(map, "{ [i] -> [-i] }");
	isl_map_free(map);
	if (equal < 0)
		return -1;

	str = "{ [i] -> [j] }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_oppose(map, isl_dim_in, 0, isl_dim_in, 0);
	equal = map_check_equal(map, "{ [0] -> [j] }");
	isl_map_free(map);
	if (equal < 0)
		return -1;

	str = "{ [i] -> [j] }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_order_gt(map, isl_dim_in, 0, isl_dim_out, 0);
	equal = map_check_equal(map, "{ [i] -> [j] : i > j }");
	isl_map_free(map);
	if (equal < 0)
		return -1;

	str = "{ [i] -> [j] }";
	map = isl_map_read_from_str(ctx, str);
	map = isl_map_order_gt(map, isl_dim_in, 0, isl_dim_in, 0);
	equal = map_check_equal(map, "{ [i] -> [j] : false }");
	isl_map_free(map);
	if (equal < 0)
		return -1;

	return 0;
}

struct {
	const char *name;
	int (*fn)(isl_ctx *ctx);
} tests [] = {
	{ "slice", &test_slice },
	{ "fixed power", &test_fixed_power },
	{ "sample", &test_sample },
	{ "output", &test_output },
	{ "vertices", &test_vertices },
	{ "fixed", &test_fixed },
	{ "equal", &test_equal },
	{ "product", &test_product },
	{ "dim_max", &test_dim_max },
	{ "affine", &test_aff },
	{ "injective", &test_injective },
	{ "schedule", &test_schedule },
	{ "union_pw", &test_union_pw },
	{ "parse", &test_parse },
	{ "single-valued", &test_sv },
	{ "affine hull", &test_affine_hull },
	{ "coalesce", &test_coalesce },
	{ "factorize", &test_factorize },
};

int main()
{
	int i;
	struct isl_ctx *ctx;

	srcdir = getenv("srcdir");
	assert(srcdir);

	ctx = isl_ctx_alloc();
	for (i = 0; i < ARRAY_SIZE(tests); ++i) {
		printf("%s\n", tests[i].name);
		if (tests[i].fn(ctx) < 0)
			goto error;
	}
	test_subset(ctx);
	test_lift(ctx);
	test_bound(ctx);
	test_union(ctx);
	test_split_periods(ctx);
	test_pwqp(ctx);
	test_lex(ctx);
	test_bijective(ctx);
	test_dep(ctx);
	test_read(ctx);
	test_bounded(ctx);
	test_construction(ctx);
	test_dim(ctx);
	test_div(ctx);
	test_application(ctx);
	test_convex_hull(ctx);
	test_gist(ctx);
	test_closure(ctx);
	test_lexmin(ctx);
	isl_ctx_free(ctx);
	return 0;
error:
	isl_ctx_free(ctx);
	return -1;
}
