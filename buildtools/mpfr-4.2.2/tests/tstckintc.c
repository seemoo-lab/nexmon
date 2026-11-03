/* Test file for mpfr_custom_*

Copyright 2005-2025 Free Software Foundation, Inc.
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

#include "mpfr-test.h"

/*************************************************************************/

/* Bug found on 2022-05-05 in the mpfr_custom_get_size macro.
   With the buggy macro, this test will fail if mpfr_prec_t > int,
   typically on LP64 platforms (usual 64-bit machines).
   This bug could occur only for precisions close to INT_MAX or higher. */
static void
test_get_size (void)
{
  unsigned int u = UINT_MAX;
  int i = INT_MAX;

  if (u <= MPFR_PREC_MAX)
    {
      mpfr_prec_t p = u;
      size_t s1, s2;

      s1 = mpfr_custom_get_size (p);
      s2 = mpfr_custom_get_size (u);
      if (s1 != s2)
        {
          printf ("Error 1 in test_get_size (mpfr_custom_get_size macro).\n");
          printf ("Expected %lu\n", (unsigned long) s1);
          printf ("Got      %lu\n", (unsigned long) s2);
          exit (1);
        }
    }

  if (i <= MPFR_PREC_MAX)
    {
      mpfr_prec_t p = i;
      size_t s1, s2;

      s1 = mpfr_custom_get_size (p);
      s2 = mpfr_custom_get_size (i);
      if (s1 != s2)
        {
          printf ("Error 2 in test_get_size (mpfr_custom_get_size macro).\n");
          printf ("Expected %lu\n", (unsigned long) s1);
          printf ("Got      %lu\n", (unsigned long) s2);
          exit (1);
        }
    }
}

/*************************************************************************/

#define BUFFER_SIZE 250
#define PREC_TESTED 200

long Buffer[BUFFER_SIZE];
char *stack = (char *) Buffer;
long *org = (long *) Buffer;
mpfr_prec_t p = PREC_TESTED;

#define ALIGNED(s) (((s) + sizeof (long) - 1) / sizeof (long) * sizeof (long))

/* This code ensures alignment to "long". However, this might not be
   sufficient on some platforms. GCC's -Wcast-align=strict option can
   be useful, but this needs successive casts to help GCC, e.g.

   newx = (mpfr_ptr) (long *) (void *) old_stack;

   successively casts old_stack (of type char *) to
   - void *: avoid a false positive for the following cast to long *
     (as the code takes care of alignment to "long");
   - long *: this corresponds to the alignment checked by MPFR; coming
     from void *, it will not trigger a warning (even if incorrect);
   - mpfr_ptr: -Wcast-align=strict will emit a warning if mpfr_ptr has
     an alignment requirement stronger than long *. In such a case,
     the code will have to be fixed.
*/

static void *
new_st (size_t s)
{
  void *p = (void *) stack;

  if (MPFR_UNLIKELY (s > (char *) &Buffer[BUFFER_SIZE] - stack))
    {
      printf ("[INTERNAL TEST ERROR] Stack overflow.\n");
      exit (1);
    }
  stack += ALIGNED (s);
  return p;
}

static void
reset_stack (void)
{
  stack = (char *) Buffer;
}

/*************************************************************************/

/* Alloc a new mpfr_t on the main stack */
static mpfr_ptr
new_mpfr (mpfr_prec_t p)
{
  mpfr_ptr x = (mpfr_ptr) new_st (sizeof (mpfr_t));
  void *mantissa = new_st (mpfr_custom_get_size (p));

  mpfr_custom_init (mantissa, p);
  mpfr_custom_init_set (x, 0, 0, p, mantissa);
  return x;
}

/* Alloc a new mpfr_t on the main stack */
static mpfr_ptr
new_nan1 (mpfr_prec_t p)
{
  mpfr_ptr x = (mpfr_ptr) new_st (sizeof (mpfr_t));
  void *mantissa = new_st ((mpfr_custom_get_size) (p));

  (mpfr_custom_init) (mantissa, p);
  (mpfr_custom_init_set) (x, MPFR_NAN_KIND, 0, p, mantissa);
  return x;
}

/* Alloc a new mpfr_t on the main stack */
static mpfr_ptr
new_nan2 (mpfr_prec_t p)
{
  mpfr_ptr x = (mpfr_ptr) new_st (sizeof (mpfr_t));
  void *mantissa = new_st ((mpfr_custom_get_size) (p));
  int i1, i2, i3, i4, i5;

  /* Check side effects. */
  i1 = i2 = 0;
  mpfr_custom_init ((i1++, mantissa), (i2++, p));
  MPFR_ASSERTN (i1 == 1);
  MPFR_ASSERTN (i2 == 1);

  /* Check that the type "void *" can be used in C, like with the function
     (forbidden in C++). Also check side effects. */
  i1 = i2 = i3 = i4 = i5 = 0;
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
  mpfr_custom_init_set ((i1++, VOIDP_CAST(x)),
                        (i2++, MPFR_NAN_KIND),
                        (i3++, 0),
                        (i4++, p),
                        (i5++, mantissa));
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
  MPFR_ASSERTN (i1 == 1);
  MPFR_ASSERTN (i2 == 1);
  MPFR_ASSERTN (i3 == 1);
  MPFR_ASSERTN (i4 == 1);
  MPFR_ASSERTN (i5 == 1);

  return x;
}

/* Alloc a new mpfr_t on the main stack */
static mpfr_ptr
new_inf (mpfr_prec_t p)
{
  mpfr_ptr x = (mpfr_ptr) new_st (sizeof (mpfr_t));
  void *mantissa = new_st ((mpfr_custom_get_size) (p));

  (mpfr_custom_init) (mantissa, p);
  (mpfr_custom_init_set) (x, -MPFR_INF_KIND, 0, p, mantissa);
  return x;
}

/* Garbage the stack by keeping only x and save it in old_stack */
static mpfr_ptr
return_mpfr (mpfr_ptr x, char *old_stack)
{
  void *mantissa       = mpfr_custom_get_significand (x);
  size_t size_mantissa = mpfr_custom_get_size (mpfr_get_prec (x));
  mpfr_ptr newx;
  long *newx2;
  int i1, i2;

  /* Check that the type "void *" can be used in C, like with the function
     (forbidden in C++). Also check side effects. */
  i1 = 0;
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
  MPFR_ASSERTN (mpfr_custom_get_significand ((i1++, VOIDP_CAST(x)))
                == mantissa);
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
  MPFR_ASSERTN (i1 == 1);

  memmove (old_stack, x, sizeof (mpfr_t));
  memmove (old_stack + ALIGNED (sizeof (mpfr_t)), mantissa, size_mantissa);
  newx = (mpfr_ptr) (long *) (void *) old_stack;
  newx2 = (long *) (void *) (old_stack + ALIGNED (sizeof (mpfr_t)));

  /* Check that the type "void *" can be used in C, like with the function
     (forbidden in C++). Also check side effects. */
  i1 = i2 = 0;
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
  mpfr_custom_move ((i1++, VOIDP_CAST(newx)), (i2++, newx2));
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
  MPFR_ASSERTN (i1 == 1);
  MPFR_ASSERTN (i2 == 1);

  stack = (char *) newx2 + ALIGNED (size_mantissa);
  return newx;
}

/* Garbage the stack by keeping only x and save it in old_stack */
static mpfr_ptr
return_mpfr_func (mpfr_ptr x, char *old_stack)
{
  void *mantissa       = (mpfr_custom_get_significand) (x);
  size_t size_mantissa = (mpfr_custom_get_size) (mpfr_get_prec (x));
  mpfr_ptr newx;

  memmove (old_stack, x, sizeof (mpfr_t));
  memmove (old_stack + ALIGNED (sizeof (mpfr_t)), mantissa, size_mantissa);
  newx = (mpfr_ptr) (long *) (void *) old_stack;
  (mpfr_custom_move) (newx, old_stack + ALIGNED (sizeof (mpfr_t)));
  stack = old_stack + ALIGNED (sizeof (mpfr_t)) + ALIGNED (size_mantissa);
  return newx;
}

/*************************************************************************/

static void
test1 (void)
{
  mpfr_ptr x, y;

  reset_stack ();
  org = (long *) (void *) stack;

  x = new_mpfr (p);
  y = new_mpfr (p);
  mpfr_set_ui (x, 42, MPFR_RNDN);
  mpfr_set_ui (y, 17, MPFR_RNDN);
  mpfr_add (y, x, y, MPFR_RNDN);
  y = return_mpfr (y, (char *) org);
  if ((long *) y != org || mpfr_cmp_ui (y, 59) != 0)
    {
      printf ("Compact (1) failed!\n");
      exit (1);
    }

  x = new_mpfr (p);
  y = new_mpfr (p);
  mpfr_set_ui (x, 4217, MPFR_RNDN);
  mpfr_set_ui (y, 1742, MPFR_RNDN);
  mpfr_add (y, x, y, MPFR_RNDN);
  y = return_mpfr_func (y, (char *) org);
  if ((long *) y != org || mpfr_cmp_ui (y, 5959) != 0)
    {
      printf ("Compact (5) failed!\n");
      exit (1);
    }

  reset_stack ();
}

static void
test_nan_inf_zero (void)
{
  mpfr_ptr val;
  mpfr_srcptr sval;  /* for compilation error checking */
  int sign;
  int kind;

  reset_stack ();

  val = new_mpfr (MPFR_PREC_MIN);
  sval = val;
  mpfr_set_nan (val);
  kind = (mpfr_custom_get_kind) (sval);
  if (kind != MPFR_NAN_KIND)
    {
      printf ("mpfr_custom_get_kind error: ");
      mpfr_dump (val);
      printf (" is kind %d instead of %d\n", kind, (int) MPFR_NAN_KIND);
      exit (1);
    }

  val = new_nan1 (MPFR_PREC_MIN);
  if (!mpfr_nan_p(val))
    {
      printf ("Error: mpfr_custom_init_set doesn't set NAN mpfr (1).\n");
      exit (1);
    }

  val = new_nan2 (MPFR_PREC_MIN);
  if (!mpfr_nan_p(val))
    {
      printf ("Error: mpfr_custom_init_set doesn't set NAN mpfr (2).\n");
      exit (1);
    }

  val = new_inf (MPFR_PREC_MIN);
  if (!mpfr_inf_p(val) || mpfr_sgn(val) >= 0)
    {
      printf ("Error: mpfr_custom_init_set doesn't set -INF mpfr.\n");
      exit (1);
    }

  sign = 1;
  mpfr_set_inf (val, sign);
  kind = (mpfr_custom_get_kind) (val);
  if ((ABS (kind) != MPFR_INF_KIND) || (VSIGN (kind) != VSIGN (sign)))
    {
      printf ("mpfr_custom_get_kind error: ");
      mpfr_dump (val);
      printf (" is kind %d instead of %d\n", kind, (int) MPFR_INF_KIND);
      printf (" have sign %d instead of %d\n", VSIGN (kind), VSIGN (sign));
      exit (1);
    }

  sign = -1;
  mpfr_set_zero (val, sign);
  kind = (mpfr_custom_get_kind) (val);
  if ((ABS (kind) != MPFR_ZERO_KIND) || (VSIGN (kind) != VSIGN (sign)))
    {
      printf ("mpfr_custom_get_kind error: ");
      mpfr_dump (val);
      printf (" is kind %d instead of %d\n", kind, (int) MPFR_ZERO_KIND);
      printf (" have sign %d instead of %d\n", VSIGN (kind), VSIGN (sign));
      exit (1);
    }

  reset_stack ();
}

/*************************************************************************/

/* We build the MPFR variable each time it is needed */
/* a[0] is the kind, a[1] is the exponent, &a[2] is the mantissa */
static long *
dummy_new (void)
{
  long *r;

  r = (long *) new_st (ALIGNED (2 * sizeof (long)) +
                       ALIGNED (mpfr_custom_get_size (p)));
  (mpfr_custom_init) (&r[2], p);
  r[0] = (int) MPFR_NAN_KIND;
  r[1] = 0;
  return r;
}

static long *
dummy_set_si (long si)
{
  mpfr_t x;
  mpfr_srcptr px;  /* for compilation error checking */
  long *r = dummy_new ();
  int i1, i2, i3, i4, i5;

  /* Check that the type "void *" can be used, like with the function.
     Also check side effects. */
  i1 = i2 = i3 = i4 = i5 = 0;
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
  mpfr_custom_init_set ((i1++, VOIDP_CAST(x)),
                        (i2++, MPFR_REGULAR_KIND),
                        (i3++, 0),
                        (i4++, p),
                        (i5++, &r[2]));
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
  MPFR_ASSERTN (i1 == 1);
  MPFR_ASSERTN (i2 == 1);
  MPFR_ASSERTN (i3 == 1);
  MPFR_ASSERTN (i4 == 1);
  MPFR_ASSERTN (i5 == 1);

  mpfr_set_si (x, si, MPFR_RNDN);
  px = x;
  r[0] = mpfr_custom_get_kind (px);

  /* Check that the type "void *" can be used in C, like with the function
     (forbidden in C++). Also check side effects. */
  i1 = 0;
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
  MPFR_ASSERTN (mpfr_custom_get_kind ((i1++, VOIDP_CAST(x))) == r[0]);
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
  MPFR_ASSERTN (i1 == 1);

  r[1] = mpfr_custom_get_exp (x);

  /* Check that the type "void *" can be used in C, like with the function
     (forbidden in C++). Also check side effects. */
  i1 = 0;
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++-compat"
#endif
  MPFR_ASSERTN (mpfr_custom_get_exp ((i1++, VOIDP_CAST(x))) == r[1]);
#ifdef IGNORE_CPP_COMPAT
#pragma GCC diagnostic pop
#endif
  MPFR_ASSERTN (i1 == 1);

  return r;
}

static long *
dummy_add (long *a, long *b)
{
  mpfr_t x, y, z;
  long *r = dummy_new ();

  mpfr_custom_init_set (x, 0 + MPFR_REGULAR_KIND, 0, p, &r[2]);
  (mpfr_custom_init_set) (y, a[0], a[1], p, &a[2]);
  mpfr_custom_init_set (z, 0 + b[0], b[1], p, &b[2]);
  mpfr_add (x, y, z, MPFR_RNDN);
  r[0] = (mpfr_custom_get_kind) (x);
  r[1] = (mpfr_custom_get_exp) (x);
  return r;
}

static long *
dummy_compact (long *r, long *org_stack)
{
  memmove (org_stack, r,
           ALIGNED (2*sizeof (long)) + ALIGNED ((mpfr_custom_get_size) (p)));
  return org_stack;
}

/*************************************************************************/

static void
test2 (void)
{
  mpfr_t x;
  long *a, *b, *c;

  reset_stack ();
  org = (long *) (void *) stack;

  a = dummy_set_si (42);
  b = dummy_set_si (17);
  c = dummy_add (a, b);
  c = dummy_compact (c, org);
  (mpfr_custom_init_set) (x, c[0], c[1], p, &c[2]);
  if (c != org || mpfr_cmp_ui (x, 59) != 0)
    {
      printf ("Compact (2) failed! c=%p a=%p\n", (void *) c, (void *) a);
      mpfr_dump (x);
      exit (1);
    }

  a = dummy_set_si (42);
  b = dummy_set_si (-17);
  c = dummy_add (a, b);
  c = dummy_compact (c, org);
  (mpfr_custom_init_set) (x, c[0], c[1], p, &c[2]);
  if (c != org || mpfr_cmp_ui (x, 25) != 0)
    {
      printf ("Compact (6) failed! c=%p a=%p\n", (void *) c, (void *) a);
      mpfr_dump (x);
      exit (1);
    }

  reset_stack ();
}


int
main (void)
{
  tests_start_mpfr ();
  test_get_size ();
  /* We test iff long = mp_limb_t */
  if (sizeof (long) == sizeof (mp_limb_t))
    {
      test1 ();
      test2 ();
      test_nan_inf_zero ();
    }
  tests_end_mpfr ();
  return 0;
}
