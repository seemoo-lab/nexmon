/* Generic test file for functions with one or two arguments (the second being
   either mpfr_t or double or unsigned long).

Copyright 2001-2025 Free Software Foundation, Inc.
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

/* Define TWO_ARGS for two-argument functions like mpfr_pow.
   Define DOUBLE_ARG1 or DOUBLE_ARG2 for function with a double operand in
   first or second place like sub_d or d_sub.
   Define ULONG_ARG1 or ULONG_ARG2 for function with an unsigned long
   operand in first or second place like sub_ui or ui_sub.
   Define THREE_ARGS for three-argument functions like mpfr_atan2u. */

/* TODO: Add support for type long and extreme integer values, as done
   in tgeneric_ui.c; then tgeneric_ui.c could probably disappear. */

#ifdef THREE_ARGS
/* This is like TWO_ARGS, but with an additional argument. */
#define TWO_ARGS
#endif

#if defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
#define DOUBLE_ARG
#else
#undef DOUBLE_ARG
#endif

#if defined(TWO_ARGS) || defined(DOUBLE_ARG)
#define TWO_ARGS_ALL
#define NSPEC 9
#else
#undef TWO_ARGS_ALL
#define NSPEC 5
#endif

#if defined(ULONG_ARG1) || defined(ULONG_ARG2) || defined(THREE_ARGS)
#define ULONG_ARG
#else
#undef ULONG_ARG
#endif

#ifndef TEST_RANDOM_POS
/* For the random function: one number on two is negative. */
#define TEST_RANDOM_POS 256
#endif

#ifndef TEST_RANDOM_POS2
/* For the random function: one number on two is negative. */
#define TEST_RANDOM_POS2 256
#endif

#ifndef TEST_RANDOM_EMIN
#define TEST_RANDOM_EMIN -256
#endif

#ifndef TEST_RANDOM_EMAX
#define TEST_RANDOM_EMAX 255
#endif

#ifndef TEST_RANDOM_ALWAYS_SCALE
#define TEST_RANDOM_ALWAYS_SCALE 0
#endif

/* If the MPFR_SUSPICIOUS_OVERFLOW test fails but this is not a bug,
   then define TGENERIC_SO_TEST with an adequate test (possibly 0) to
   omit this particular case. */
#ifndef TGENERIC_SO_TEST
#define TGENERIC_SO_TEST 1
#endif

#define TGENERIC_DUMPARGS(X1,X2,U)                                      \
  do                                                                    \
    {                                                                   \
      printf ("x1 = ");                                                 \
      mpfr_dump (X1);                                                   \
      if ((X2) != 0)                                                    \
        {                                                               \
          printf ("x2 = ");                                             \
          mpfr_dump (X2);                                               \
        }                                                               \
      if ((U) >= 0)                                                     \
        printf ("u = %lu\n", (unsigned long) U);                        \
    }                                                                   \
  while (0)

#define TGENERIC_FAIL(S,X1,X2,U)                                        \
  do                                                                    \
    {                                                                   \
      printf ("tgeneric: %s\n", (S));                                   \
      TGENERIC_DUMPARGS (X1, X2, U);                                    \
      printf ("yprec = %u, rnd_mode = %s, inexact = %d\nflags =",       \
              (unsigned int) yprec, mpfr_print_rnd_mode (rnd),          \
              compare);                                                 \
      flags_out (flags);                                                \
      exit (1);                                                         \
    }                                                                   \
  while (0)

#define TGENERIC_CHECK_AUX(S,EXPR,X2,U)                                 \
  do                                                                    \
    if (!(EXPR))                                                        \
      TGENERIC_FAIL (S " for " MAKE_STR(TEST_FUNCTION), x, X2, U);      \
  while (0)

#undef TGENERIC_CHECK
#if defined(THREE_ARGS)
#define TGENERIC_CHECK(S,EXPR) TGENERIC_CHECK_AUX (S, EXPR, x2, u)
#elif defined(TWO_ARGS_ALL)
#define TGENERIC_CHECK(S,EXPR) TGENERIC_CHECK_AUX (S, EXPR, x2, -1)
#elif defined(ULONG_ARG)
#define TGENERIC_CHECK(S,EXPR) TGENERIC_CHECK_AUX (S, EXPR, 0, u)
#else
#define TGENERIC_CHECK(S,EXPR) TGENERIC_CHECK_AUX (S, EXPR, 0, -1)
#endif

#ifdef MPFR_DEBUG_TGENERIC
#define TGENERIC_IAUX(P,X1,X2,U)                                        \
  do                                                                    \
    {                                                                   \
      printf ("tgeneric: testing function " MAKE_STR(TEST_FUNCTION)     \
              ", %s, target prec = %u\n",                               \
              mpfr_print_rnd_mode (rnd), (unsigned int) (P));           \
      TGENERIC_DUMPARGS (X1, X2, U);                                    \
    }                                                                   \
  while (0)
#undef TGENERIC_INFO
#if defined(THREE_ARGS)
#define TGENERIC_INFO(P) TGENERIC_IAUX (P, x, x2, u)
#elif defined(TWO_ARGS_ALL)
#define TGENERIC_INFO(P) TGENERIC_IAUX (P, x, x2, -1)
#elif defined(ULONG_ARG)
#define TGENERIC_INFO(P) TGENERIC_IAUX (P, x, 0, u)
#else
#define TGENERIC_INFO(P) TGENERIC_IAUX (P, x, 0, -1)
#endif
#endif  /* MPFR_DEBUG_TGENERIC */

/* For some functions (for example cos), the argument reduction is too
   expensive when using mpfr_get_emax(). Then simply define REDUCE_EMAX
   to some reasonable value before including tgeneric.c. */
#ifndef REDUCE_EMAX
#define REDUCE_EMAX mpfr_get_emax ()
#endif

/* same for mpfr_get_emin() */
#ifndef REDUCE_EMIN
#define REDUCE_EMIN mpfr_get_emin ()
#endif

static void
test_generic (mpfr_prec_t p0, mpfr_prec_t p1, unsigned int nmax)
{
  mpfr_prec_t prec, xprec, yprec;
  mpfr_t x, y, z, t, w, yd, yu;
#ifdef TWO_ARGS_ALL
  mpfr_t x2;
#endif
#if defined(DOUBLE_ARG1) || defined(DOUBLE_ARG2)
  double d;
#endif
#ifdef ULONG_ARG
  unsigned long u;
#endif
  mpfr_rnd_t rnd;
  int inexact, compare, compare2;
  unsigned int n;
  unsigned long ctrt = 0, ctrn = 0;
  mpfr_exp_t old_emin, old_emax;

  old_emin = mpfr_get_emin ();
  old_emax = mpfr_get_emax ();

  mpfr_inits2 (MPFR_PREC_MIN, x, y, yd, yu, z, t, w, (mpfr_ptr) 0);
#if defined(TWO_ARGS_ALL)
  mpfr_init2 (x2, MPFR_PREC_MIN);
#endif

  /* generic tests */
  for (prec = p0; prec <= p1; prec++)
    {
      /* Number of overflow/underflow tests for each precision.
         Since MPFR uses several algorithms and there may also be
         early overflow/underflow detection, several tests may be
         needed to detect a bug. */
      int test_of = 3, test_uf = 3;

      mpfr_set_prec (z, prec);
      mpfr_set_prec (t, prec);
      yprec = prec + 20;
      mpfr_set_prec (y, yprec);
      mpfr_set_prec (yd, yprec);
      mpfr_set_prec (yu, yprec);
      mpfr_set_prec (w, yprec);

      /* Note: in precision p1, we test NSPEC special cases. */
      for (n = 0; n < (prec == p1 ? nmax + NSPEC : nmax); n++)
        {
          int infinite_input = 0;
          mpfr_flags_t flags;
          mpfr_exp_t oemin, oemax;

          xprec = prec;
          if (RAND_BOOL ())
            {
              /* In half cases, modify the precision of the inputs:
                 If the base precision (for the result) is small,
                 take a larger input precision in general, else
                 take a smaller precision. */
              xprec *= (prec < 16 ? 256.0 : 1.0) *
                (double) randlimb () / (double) MPFR_LIMB_MAX;
              if (xprec < MPFR_PREC_MIN)
                xprec = MPFR_PREC_MIN;
            }
          mpfr_set_prec (x, xprec);
#if defined(TWO_ARGS)
          mpfr_set_prec (x2, xprec);
#elif defined(DOUBLE_ARG)
          mpfr_set_prec (x2, IEEE_DBL_MANT_DIG);
#endif

#ifdef MPFR_DEBUG_TGENERIC
          printf ("prec = %u, n = %u, xprec = %u\n",
                  (unsigned int) prec, n, (unsigned int) xprec);
#endif

          /* Generate random arguments, even in the special cases
             (this may not be needed, but this is simpler).
             Note that if RAND_FUNCTION is defined, this specific
             random function is used for all arguments; this is
             typically mpfr_random2, which generates a positive
             random mpfr_t with long runs of consecutive ones and
             zeros in the binary representation. */

#if defined(RAND_FUNCTION)
          RAND_FUNCTION (x);
#if defined(TWO_ARGS_ALL)
          RAND_FUNCTION (x2);
#endif
#else  /* ! defined(RAND_FUNCTION) */
          tests_default_random (x, TEST_RANDOM_POS,
                                TEST_RANDOM_EMIN, TEST_RANDOM_EMAX,
                                TEST_RANDOM_ALWAYS_SCALE);
#if defined(TWO_ARGS_ALL)
          tests_default_random (x2, TEST_RANDOM_POS2,
                                TEST_RANDOM_EMIN, TEST_RANDOM_EMAX,
                                TEST_RANDOM_ALWAYS_SCALE);
#endif
#endif  /* ! defined(RAND_FUNCTION) */

#if defined(ULONG_ARG)
          /* FIXME: If MPFR_LIMB_MAX < ULONG_MAX, large values will
             never be tested. */
          u = randlimb ();
#endif

          if (n < NSPEC && prec == p1)
            {
              /* Special cases tested in precision p1 if n < NSPEC. They are
                 useful really in the extended exponent range. */
              /* TODO: x2 is set even when it is associated with a double;
                 check whether this really makes sense. */
#if defined(DOUBLE_ARG) && defined(MPFR_ERRDIVZERO)
              goto next_n;
#endif
              set_emin (MPFR_EMIN_MIN);
              set_emax (MPFR_EMAX_MAX);
              if (n == 0)
                {
                  mpfr_set_nan (x);
                }
              else if (n <= 2)
                {
                  MPFR_ASSERTN (n == 1 || n == 2);
                  mpfr_set_si (x, n == 1 ? 1 : -1, MPFR_RNDN);
                  mpfr_set_exp (x, REDUCE_EMIN);
#if defined(TWO_ARGS_ALL)
                  mpfr_set_si (x2, RAND_BOOL () ? -1 : 1, MPFR_RNDN);
                  mpfr_set_exp (x2, REDUCE_EMIN);
#endif
                }
              else if (n <= 4)
                {
                  MPFR_ASSERTN (n == 3 || n == 4);
                  mpfr_set_si (x, n == 3 ? 1 : -1, MPFR_RNDN);
                  mpfr_setmax (x, REDUCE_EMAX);
#if defined(TWO_ARGS_ALL)
                  mpfr_set_si (x2, RAND_BOOL () ? -1 : 1, MPFR_RNDN);
                  mpfr_setmax (x2, REDUCE_EMAX);
#endif
                }
#if defined(TWO_ARGS_ALL)
              else if (n <= 6)
                {
                  MPFR_ASSERTN (n == 5 || n == 6);
                  mpfr_set_si (x, n == 5 ? 1 : -1, MPFR_RNDN);
                  mpfr_set_exp (x, REDUCE_EMIN);
                  mpfr_set_si (x2, RAND_BOOL () ? -1 : 1, MPFR_RNDN);
                  mpfr_setmax (x2, REDUCE_EMAX);
                }
              else
                {
                  MPFR_ASSERTN (n == 7 || n == 8);
                  mpfr_set_si (x, n == 7 ? 1 : -1, MPFR_RNDN);
                  mpfr_setmax (x, REDUCE_EMAX);
                  mpfr_set_si (x2, RAND_BOOL () ? -1 : 1, MPFR_RNDN);
                  mpfr_set_exp (x2, REDUCE_EMIN);
                }
#endif  /* two arguments */
            }

          /* Exponent range for the test. */
          oemin = mpfr_get_emin ();
          oemax = mpfr_get_emax ();

          rnd = RND_RAND ();
          mpfr_clear_flags ();
#ifdef MPFR_DEBUG_TGENERIC
          TGENERIC_INFO (MPFR_PREC (y));
#endif
#if defined(THREE_ARGS)
          compare = TEST_FUNCTION (y, x, x2, u, rnd);
#elif defined(TWO_ARGS)
          compare = TEST_FUNCTION (y, x, x2, rnd);
#elif defined(DOUBLE_ARG)
          d = mpfr_get_d (x2, rnd);
# if defined(DOUBLE_ARG1)
          compare = TEST_FUNCTION (y, d, x, rnd);
# elif defined(DOUBLE_ARG2)
          compare = TEST_FUNCTION (y, x, d, rnd);
# else
#  error "cannot occur"
# endif
          /* d can be infinite due to overflow in mpfr_get_d */
          infinite_input |= DOUBLE_ISINF (d);
#elif defined(ULONG_ARG1) && defined(ONE_ARG)
          compare = TEST_FUNCTION (y, u, rnd);
#elif defined(ULONG_ARG1)
          compare = TEST_FUNCTION (y, u, x, rnd);
#elif defined(ULONG_ARG2)
          compare = TEST_FUNCTION (y, x, u, rnd);
#else
          compare = TEST_FUNCTION (y, x, rnd);
#endif
          flags = __gmpfr_flags;
          if (mpfr_get_emin () != oemin ||
              mpfr_get_emax () != oemax)
            {
              printf ("tgeneric: the exponent range has been modified"
                      " by the tested function!\n");
              exit (1);
            }
          if (rnd != MPFR_RNDF)
            TGENERIC_CHECK ("bad inexact flag",
                            (compare != 0) ^ (mpfr_inexflag_p () == 0));
          ctrt++;

          /* If rnd = RNDF, check that we obtain the same result as
             RNDD or RNDU. */
          if (rnd == MPFR_RNDF)
            {
#if defined(THREE_ARGS)
              TEST_FUNCTION (yd, x, x2, u, MPFR_RNDD);
              TEST_FUNCTION (yu, x, x2, u, MPFR_RNDU);
#elif defined(TWO_ARGS)
              TEST_FUNCTION (yd, x, x2, MPFR_RNDD);
              TEST_FUNCTION (yu, x, x2, MPFR_RNDU);
#elif defined(DOUBLE_ARG1)
              d = mpfr_get_d (x2, MPFR_RNDD);
              TEST_FUNCTION (yd, d, x, MPFR_RNDD);
              d = mpfr_get_d (x2, MPFR_RNDU);
              TEST_FUNCTION (yu, d, x, MPFR_RNDU);
#elif defined(DOUBLE_ARG2)
              d = mpfr_get_d (x2, MPFR_RNDD);
              TEST_FUNCTION (yd, x, d, MPFR_RNDD);
              d = mpfr_get_d (x2, MPFR_RNDU);
              TEST_FUNCTION (yu, x, d, MPFR_RNDU);
#elif defined(ULONG_ARG1) && defined(ONE_ARG)
              TEST_FUNCTION (yd, u, MPFR_RNDD);
              TEST_FUNCTION (yu, u, MPFR_RNDU);
#elif defined(ULONG_ARG1)
              TEST_FUNCTION (yd, u, x, MPFR_RNDD);
              TEST_FUNCTION (yu, u, x, MPFR_RNDU);
#elif defined(ULONG_ARG2)
              TEST_FUNCTION (yd, x, u, MPFR_RNDD);
              TEST_FUNCTION (yu, x, u, MPFR_RNDU);
#else
              TEST_FUNCTION (yd, x, MPFR_RNDD);
              TEST_FUNCTION (yu, x, MPFR_RNDU);
#endif
              if (! (SAME_VAL (y, yd) || SAME_VAL (y, yu)))
                {
                  printf ("tgeneric: error for" MAKE_STR(TEST_FUNCTION)
                          ", RNDF; result matches neither RNDD nor RNDU\n");
                  printf ("x1 = "); mpfr_dump (x);
#ifdef TWO_ARGS_ALL
                  printf ("x2 = "); mpfr_dump (x2);
#endif
#ifdef ULONG_ARG
                  printf ("u = %lu\n", u);
#endif
                  printf ("yd (RNDD) = "); mpfr_dump (yd);
                  printf ("yu (RNDU) = "); mpfr_dump (yu);
                  printf ("y  (RNDF) = "); mpfr_dump (y);
                  exit (1);
                }
            }

          /* Tests in a reduced exponent range. */
          {
            mpfr_flags_t oldflags = flags;
            mpfr_exp_t e, emin, emax;

            /* Determine the smallest exponent range containing the
               exponents of the mpfr_t inputs (x, and u if TWO_ARGS)
               and output (y). */
            emin = MPFR_EMAX_MAX;
            emax = MPFR_EMIN_MIN;
            if (MPFR_IS_PURE_FP (x))
              {
                e = MPFR_GET_EXP (x);
                if (e < emin)
                  emin = e;
                if (e > emax)
                  emax = e;
              }
#if defined(TWO_ARGS)
            if (MPFR_IS_PURE_FP (x2))
              {
                e = MPFR_GET_EXP (x2);
                if (e < emin)
                  emin = e;
                if (e > emax)
                  emax = e;
              }
#endif
            if (MPFR_IS_PURE_FP (y))
              {
                e = MPFR_GET_EXP (y);  /* exponent of the result */

                if (test_of > 0 && e - 1 >= emax)  /* overflow test */
                  {
                    mpfr_flags_t ex_flags;

                    /* Exponent e of the result > exponents of the inputs;
                       let's set emax to e - 1, so that one should get an
                       overflow. */
                    set_emax (e - 1);
#ifdef MPFR_DEBUG_TGENERIC
                    printf ("tgeneric: overflow test (emax = %"
                            MPFR_EXP_FSPEC "d)\n",
                            (mpfr_eexp_t) __gmpfr_emax);
#endif
                    mpfr_clear_flags ();
#if defined(THREE_ARGS)
                    inexact = TEST_FUNCTION (w, x, x2, u, rnd);
#elif defined(TWO_ARGS)
                    inexact = TEST_FUNCTION (w, x, x2, rnd);
#elif defined(DOUBLE_ARG1)
                    inexact = TEST_FUNCTION (w, d, x, rnd);
#elif defined(DOUBLE_ARG2)
                    inexact = TEST_FUNCTION (w, x, d, rnd);
#elif defined(ULONG_ARG1) && defined(ONE_ARG)
                    inexact = TEST_FUNCTION (w, u, rnd);
#elif defined(ULONG_ARG1)
                    inexact = TEST_FUNCTION (w, u, x, rnd);
#elif defined(ULONG_ARG2)
                    inexact = TEST_FUNCTION (w, x, u, rnd);
#else
                    inexact = TEST_FUNCTION (w, x, rnd);
#endif
                    flags = __gmpfr_flags;
                    set_emax (oemax);
                    ex_flags = MPFR_FLAGS_OVERFLOW | MPFR_FLAGS_INEXACT;
                    /* For RNDF, this test makes no sense, since RNDF
                       might return either the maximal floating-point
                       value or infinity, and the flags might differ in
                       those two cases. */
                    if (flags != ex_flags && rnd != MPFR_RNDF)
                      {
                        printf ("tgeneric: error for " MAKE_STR(TEST_FUNCTION)
                                ", reduced exponent range [%"
                                MPFR_EXP_FSPEC "d,%" MPFR_EXP_FSPEC
                                "d] (overflow test) on:\n",
                                (mpfr_eexp_t) oemin, (mpfr_eexp_t) e - 1);
                        printf ("x1 = "); mpfr_dump (x);
#ifdef TWO_ARGS_ALL
                        printf ("x2 = "); mpfr_dump (x2);
#endif
#ifdef ULONG_ARG
                        printf ("u = %lu\n", u);
#endif
                        printf ("yprec = %u, rnd_mode = %s\n",
                                (unsigned int) yprec,
                                mpfr_print_rnd_mode (rnd));
                        printf ("Expected flags =");
                        flags_out (ex_flags);
                        printf ("     got flags =");
                        flags_out (flags);
                        printf ("inex = %d, w = ", inexact);
                        mpfr_dump (w);
                        exit (1);
                      }
                    test_of--;
                  }

                if (test_uf > 0 && e + 1 <= emin)  /* underflow test */
                  {
                    mpfr_flags_t ex_flags;

                    /* Exponent e of the result < exponents of the inputs;
                       let's set emin to e + 1, so that one should get an
                       underflow. */
                    set_emin (e + 1);
#ifdef MPFR_DEBUG_TGENERIC
                    printf ("tgeneric: underflow test (emin = %"
                            MPFR_EXP_FSPEC "d)\n",
                            (mpfr_eexp_t) __gmpfr_emin);
#endif
                    mpfr_clear_flags ();
#if defined(THREE_ARGS)
                    inexact = TEST_FUNCTION (w, x, x2, u, rnd);
#elif defined(TWO_ARGS)
                    inexact = TEST_FUNCTION (w, x, x2, rnd);
#elif defined(DOUBLE_ARG1)
                    inexact = TEST_FUNCTION (w, d, x, rnd);
#elif defined(DOUBLE_ARG2)
                    inexact = TEST_FUNCTION (w, x, d, rnd);
#elif defined(ULONG_ARG1) && defined(ONE_ARG)
                    inexact = TEST_FUNCTION (w, u, rnd);
#elif defined(ULONG_ARG1)
                    inexact = TEST_FUNCTION (w, u, x, rnd);
#elif defined(ULONG_ARG2)
                    inexact = TEST_FUNCTION (w, x, u, rnd);
#else
                    inexact = TEST_FUNCTION (w, x, rnd);
#endif
                    flags = __gmpfr_flags;
                    set_emin (oemin);
                    ex_flags = MPFR_FLAGS_UNDERFLOW | MPFR_FLAGS_INEXACT;
                    /* For RNDF, this test makes no sense, since RNDF
                       might return either the maximal floating-point
                       value or infinity, and the flags might differ in
                       those two cases. */
                    if (flags != ex_flags && rnd != MPFR_RNDF)
                      {
                        printf ("tgeneric: error for " MAKE_STR(TEST_FUNCTION)
                                ", reduced exponent range [%"
                                MPFR_EXP_FSPEC "d,%" MPFR_EXP_FSPEC
                                "d] (underflow test) on:\n",
                                (mpfr_eexp_t) e + 1, (mpfr_eexp_t) oemax);
                        printf ("x1 = "); mpfr_dump (x);
#ifdef TWO_ARGS_ALL
                        printf ("x2 = "); mpfr_dump (x2);
#endif
#ifdef ULONG_ARG
                        printf ("u = %lu\n", u);
#endif
                        printf ("yprec = %u, rnd_mode = %s\n",
                                (unsigned int) yprec,
                                mpfr_print_rnd_mode (rnd));
                        printf ("Expected flags =");
                        flags_out (ex_flags);
                        printf ("     got flags =");
                        flags_out (flags);
                        printf ("inex = %d, w = ", inexact);
                        mpfr_dump (w);
                        exit (1);
                      }
                    test_uf--;
                  }

                if (e < emin)
                  emin = e;
                if (e > emax)
                  emax = e;
              }  /* MPFR_IS_PURE_FP (y) */

            if (emin > emax)
              emin = emax;  /* case where all values are singular */

            /* Consistency test in a reduced exponent range. Doing it
               for the first 10 samples and for prec == p1 (which has
               some special cases) should be sufficient. */
            if (ctrt <= 10 || prec == p1)
              {
                set_emin (emin);
                set_emax (emax);
#ifdef MPFR_DEBUG_TGENERIC
                /* Useful information in case of assertion failure. */
                printf ("tgeneric: reduced exponent range [%"
                        MPFR_EXP_FSPEC "d,%" MPFR_EXP_FSPEC "d]\n",
                        (mpfr_eexp_t) emin, (mpfr_eexp_t) emax);
#endif
                mpfr_clear_flags ();
#if defined(THREE_ARGS)
                inexact = TEST_FUNCTION (w, x, x2, u, rnd);
#elif defined(TWO_ARGS)
                inexact = TEST_FUNCTION (w, x, x2, rnd);
#elif defined(DOUBLE_ARG1)
                inexact = TEST_FUNCTION (w, d, x, rnd);
#elif defined(DOUBLE_ARG2)
                inexact = TEST_FUNCTION (w, x, d, rnd);
#elif defined(ULONG_ARG1) && defined(ONE_ARG)
                inexact = TEST_FUNCTION (w, u, rnd);
#elif defined(ULONG_ARG1)
                inexact = TEST_FUNCTION (w, u, x, rnd);
#elif defined(ULONG_ARG2)
                inexact = TEST_FUNCTION (w, x, u, rnd);
#else
                inexact = TEST_FUNCTION (w, x, rnd);
#endif
                flags = __gmpfr_flags;
                set_emin (oemin);
                set_emax (oemax);
                /* That test makes no sense for RNDF. */
                if (rnd != MPFR_RNDF && ! (SAME_VAL (w, y) &&
                       SAME_SIGN (inexact, compare) &&
                       flags == oldflags))
                  {
                    printf ("tgeneric: error for " MAKE_STR(TEST_FUNCTION)
                            ", reduced exponent range [%"
                            MPFR_EXP_FSPEC "d,%" MPFR_EXP_FSPEC "d] on:\n",
                            (mpfr_eexp_t) emin, (mpfr_eexp_t) emax);
                    printf ("x1 = "); mpfr_dump (x);
#ifdef TWO_ARGS_ALL
                    printf ("x2 = "); mpfr_dump (x2);
#endif
#ifdef ULONG_ARG
                    printf ("u = %lu\n", u);
#endif
                    printf ("yprec = %u, rnd_mode = %s\n",
                            (unsigned int) yprec, mpfr_print_rnd_mode (rnd));
                    printf ("Expected:\n  y = ");
                    mpfr_dump (y);
                    printf ("  inex = %d, flags =", compare);
                    flags_out (oldflags);
                    printf ("Got:\n  w = ");
                    mpfr_dump (w);
                    printf ("  inex = %d, flags =", inexact);
                    flags_out (flags);
                    exit (1);
                  }
              }

            __gmpfr_flags = oldflags;  /* restore the flags */
          }  /* tests in a reduced exponent range */

          if (MPFR_IS_SINGULAR (y))
            {
              if (MPFR_IS_NAN (y) || mpfr_nanflag_p ())
                TGENERIC_CHECK ("bad NaN flag",
                                MPFR_IS_NAN (y) && mpfr_nanflag_p ());
              else if (MPFR_IS_INF (y))
                {
                  TGENERIC_CHECK ("bad overflow flag",
                                  (compare != 0) ^ (mpfr_overflow_p () == 0));
                  TGENERIC_CHECK ("bad divide-by-zero flag",
                                  (compare == 0 && !infinite_input) ^
                                  (mpfr_divby0_p () == 0));
                }
              else if (MPFR_IS_ZERO (y))
                TGENERIC_CHECK ("bad underflow flag",
                                (compare != 0) ^ (mpfr_underflow_p () == 0));
            }
          else if (mpfr_divby0_p ())
            {
              TGENERIC_CHECK ("both overflow and divide-by-zero",
                              ! mpfr_overflow_p ());
              TGENERIC_CHECK ("both underflow and divide-by-zero",
                              ! mpfr_underflow_p ());
              TGENERIC_CHECK ("bad compare value (divide-by-zero)",
                              compare == 0);
            }
          else if (mpfr_overflow_p ())
            {
              TGENERIC_CHECK ("both underflow and overflow",
                              ! mpfr_underflow_p ());
              TGENERIC_CHECK ("bad compare value (overflow)", compare != 0);
              mpfr_nexttoinf (y);
              TGENERIC_CHECK ("should have been max MPFR number (overflow)",
                              MPFR_IS_INF (y));
            }
          else if (mpfr_underflow_p ())
            {
              TGENERIC_CHECK ("bad compare value (underflow)", compare != 0);
              mpfr_nexttozero (y);
              TGENERIC_CHECK ("should have been min MPFR number (underflow)",
                              MPFR_IS_ZERO (y));
            }
          else if (compare == 0 || rnd == MPFR_RNDF ||
                   mpfr_can_round (y, yprec, rnd, rnd, prec))
            {
              ctrn++;
              mpfr_set (t, y, rnd);
              /* Risk of failures are known when some flags are already set
                 before the function call. Do not set the erange flag, as
                 it will remain set after the function call and no checks
                 are performed in such a case (see the mpfr_erangeflag_p
                 test below). */
              if (RAND_BOOL ())
                __gmpfr_flags = MPFR_FLAGS_ALL ^ MPFR_FLAGS_ERANGE;
#ifdef MPFR_DEBUG_TGENERIC
              TGENERIC_INFO (MPFR_PREC (z));
#endif
              /* Let's increase the precision of the inputs in a random way.
                 In most cases, this doesn't make any difference, but for
                 the mpfr_fmod bug fixed in r6230, this triggers the bug. */
              mpfr_prec_round (x, mpfr_get_prec (x) + (randlimb () & 15),
                               MPFR_RNDN);
#if defined(TWO_ARGS)
              mpfr_prec_round (x2, mpfr_get_prec (x2) + (randlimb () & 15),
                               MPFR_RNDN);
#if defined(THREE_ARGS)
              inexact = TEST_FUNCTION (z, x, x2, u, rnd);
#else
              inexact = TEST_FUNCTION (z, x, x2, rnd);
#endif
#elif defined(DOUBLE_ARG1)
              inexact = TEST_FUNCTION (z, d, x, rnd);
#elif defined(DOUBLE_ARG2)
              inexact = TEST_FUNCTION (z, x, d, rnd);
#elif defined(ULONG_ARG1) && defined(ONE_ARG)
              inexact = TEST_FUNCTION (z, u, rnd);
#elif defined(ULONG_ARG1)
              inexact = TEST_FUNCTION (z, u, x, rnd);
#elif defined(ULONG_ARG2)
              inexact = TEST_FUNCTION (z, x, u, rnd);
#else
              inexact = TEST_FUNCTION (z, x, rnd);
#endif
              if (mpfr_erangeflag_p ())
                goto next_n;
              if (! mpfr_equal_p (t, z) && rnd != MPFR_RNDF)
                {
                  printf ("tgeneric: results differ for "
                          MAKE_STR(TEST_FUNCTION) " on\n");
                  printf ("x1[%u] = ", (unsigned int) mpfr_get_prec (x));
                  mpfr_dump (x);
#ifdef TWO_ARGS_ALL
                  printf ("x2[%u] = ", (unsigned int) mpfr_get_prec (x2));
                  mpfr_dump (x2);
#endif
#ifdef ULONG_ARG
                  printf ("u = %lu\n", u);
#endif
                  printf ("prec = %u, rnd_mode = %s\n",
                          (unsigned int) prec, mpfr_print_rnd_mode (rnd));
                  printf ("Got      ");
                  mpfr_dump (z);
                  printf ("Expected ");
                  mpfr_dump (t);
                  printf ("Approx   ");
                  mpfr_dump (y);
                  exit (1);
                }
              compare2 = mpfr_cmp (t, y);
              /* if rounding to nearest, cannot know the sign of t - f(x)
                 because of composed rounding: y = o(f(x)) and t = o(y) */
              if (compare * compare2 >= 0)
                compare = compare + compare2;
              else
                compare = inexact; /* cannot determine sign(t-f(x)) */
              if (! SAME_SIGN (inexact, compare) && rnd != MPFR_RNDF)
                {
                  printf ("Wrong inexact flag for rnd=%s: expected %d, got %d"
                          "\n", mpfr_print_rnd_mode (rnd), compare, inexact);
                  printf ("x1[%u] = ", (unsigned int) mpfr_get_prec (x));
                  mpfr_dump (x);
#ifdef TWO_ARGS_ALL
                  printf ("x2[%u] = ", (unsigned int) mpfr_get_prec (x2));
                  mpfr_dump (x2);
#endif
#ifdef ULONG_ARG
                  printf ("u = %lu\n", u);
#endif
                  printf ("y = ");
                  mpfr_dump (y);
                  printf ("t = ");
                  mpfr_dump (t);
                  exit (1);
                }
            }
          else if (getenv ("MPFR_SUSPICIOUS_OVERFLOW") != NULL)
            {
              /* For developers only! */
              MPFR_ASSERTN (MPFR_IS_PURE_FP (y));
              mpfr_nexttoinf (y);
              if (MPFR_IS_INF (y) && MPFR_IS_LIKE_RNDZ (rnd, MPFR_IS_NEG (y))
                  && !mpfr_overflow_p () && TGENERIC_SO_TEST)
                {
                  printf ("Possible bug! |y| is the maximum finite number "
                          "(with yprec = %u) and has\nbeen obtained when "
                          "rounding toward zero (%s). Thus there is a very\n"
                          "probable overflow, but the overflow flag is not "
                          "set!\n",
                          (unsigned int) yprec, mpfr_print_rnd_mode (rnd));
                  printf ("x1[%u] = ", (unsigned int) mpfr_get_prec (x));
                  mpfr_dump (x);
#ifdef TWO_ARGS_ALL
                  printf ("x2[%u] = ", (unsigned int) mpfr_get_prec (x2));
                  mpfr_dump (x2);
#endif
#ifdef ULONG_ARG
                  printf ("u = %lu\n", u);
#endif
                  exit (1);
                }
            }

        next_n:
          /* In case the exponent range has been changed by
             tests_default_random() or for special values... */
          set_emin (old_emin);
          set_emax (old_emax);
        }
    }

if (getenv ("MPFR_TGENERIC_STAT") != NULL)
  printf ("tgeneric: normal cases / total = %lu / %lu\n", ctrn, ctrt);

#ifndef TGENERIC_NOWARNING
  if (3 * ctrn < 2 * ctrt)
    printf ("Warning! Too few normal cases in generic tests (%lu / %lu)\n",
            ctrn, ctrt);
#endif

  mpfr_clears (x, y, yd, yu, z, t, w, (mpfr_ptr) 0);
#ifdef TWO_ARGS_ALL
  mpfr_clear (x2);
#endif
}

#undef TEST_RANDOM_POS
#undef TEST_RANDOM_POS2
#undef TEST_RANDOM_EMIN
#undef TEST_RANDOM_EMAX
#undef TEST_RANDOM_ALWAYS_SCALE
#undef RAND_FUNCTION
#undef THREE_ARGS
#undef TWO_ARGS
#undef TWO_ARGS_ALL
#undef ULONG_ARG
#undef DOUBLE_ARG
#undef DOUBLE_ARG1
#undef DOUBLE_ARG2
#undef ULONG_ARG1
#undef ULONG_ARG2
#undef TEST_FUNCTION
#undef test_generic
#undef NSPEC
