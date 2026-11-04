/* talloc-cache  -- test file concerning memory allocation and cache

Copyright 2016-2025 Free Software Foundation, Inc.
Contributed by the Pascaline and Caramba projects, INRIA.

This file is part of the GNU MPFR Library.

The GNU MPFR Library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

The GNU MPFR Library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details.

You should have received a copy of the GNU Lesser General Public License
along with the GNU MPFR Library; see the file COPYING.LESSER.
If not, see <https://www.gnu.org/licenses/>. */

#include <stdlib.h>

#include "mpfr-test.h"

#define A 4096
#define I 10000

/* Variable v is used to check that the allocator is the current GMP one.
   The assertions on v in would typically fail with MPFR 3.1.6, where the
   allocation functions are memorized the first time they are seen. */
static int v = 0;

static void *
my_alloc1 (size_t s)
{
  void *p = malloc (s + A);
  if (p == NULL)
    abort ();
  MPFR_ASSERTN (v == 1);
  *(int *) p = 1;
  return (void *) ((char *) p + A);
}

static void *
my_realloc1 (void *p, size_t t, size_t s)
{
  p = (void *) ((char *) p - A);
  MPFR_ASSERTN (v == 1);
  MPFR_ASSERTN (*(int *) p == 1);
  return (void *) ((char *) realloc (p, s + A) + A);
}

static void
my_free1 (void *p, size_t t)
{
  p = (void *) ((char *) p - A);
  MPFR_ASSERTN (v == 1);
  MPFR_ASSERTN (*(int *) p == 1);
  free (p);
}

static void *
my_alloc2 (size_t s)
{
  void *p = malloc (s + A);
  if (p == NULL)
    abort ();
  MPFR_ASSERTN (v == 2);
  *(int *) p = 2;
  return (void *) ((char *) p + A);
}

static void *
my_realloc2 (void *p, size_t t, size_t s)
{
  p = (void *) ((char *) p - A);
  MPFR_ASSERTN (v == 2);
  MPFR_ASSERTN (*(int *) p == 2);
  return (void *) ((char *) realloc (p, s + A) + A);
}

static void
my_free2 (void *p, size_t t)
{
  p = (void *) ((char *) p - A);
  MPFR_ASSERTN (v == 2);
  MPFR_ASSERTN (*(int *) p == 2);
  free (p);
}

int
main (void)
{
  mpfr_t x;
  int err;

  tests_memory_disabled = 2;
  tests_start_mpfr ();

  err = mpfr_mp_memory_cleanup ();
  MPFR_ASSERTN (err == 0);
  mp_set_memory_functions (my_alloc1, my_realloc1, my_free1);
  v = 1;

  mpfr_init2 (x, 53);
  mpfr_set_ui (x, I, MPFR_RNDN);
  mpfr_sin (x, x, MPFR_RNDN);
  mpfr_clear (x);

  err = mpfr_mp_memory_cleanup ();
  MPFR_ASSERTN (err == 0);
  mp_set_memory_functions (my_alloc2, my_realloc2, my_free2);
  v = 2;

  mpfr_init2 (x, 1000);
  mpfr_set_ui (x, I, MPFR_RNDN);
  mpfr_sin (x, x, MPFR_RNDN);
  mpfr_clear (x);

  tests_end_mpfr ();
  return 0;
}
