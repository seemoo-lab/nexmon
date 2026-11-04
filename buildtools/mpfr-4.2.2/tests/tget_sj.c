/* Test file for mpfr_get_sj and mpfr_get_uj.

Copyright 2004-2025 Free Software Foundation, Inc.
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

#define MPFR_NEED_INTMAX_H
#include "mpfr-test.h"

#ifndef _MPFR_H_HAVE_INTMAX_T

int
main (void)
{
  return 77;
}

#else

#ifndef NPRINTF_J
#define PRMAX(SPEC,V) printf (" %j" SPEC ",", V)
#else
#define PRMAX(SPEC,V) (void) 0
#endif

static void
check_sj (intmax_t s, mpfr_ptr x)
{
  mpfr_exp_t emin, emax;
  mpfr_t y;
  int i;

  mpfr_init2 (y, MPFR_PREC (x) + 2);

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  for (i = -1; i <= 1; i++)
    {
      int rnd;
      int inex;
      int fi, e;
      mpfr_flags_t flags[2] = { 0, MPFR_FLAGS_ALL }, ex_flags, gt_flags;

      inex = mpfr_set_si_2exp (y, i, -2, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      inex = mpfr_add (y, y, x, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      /* y = x + i/4, with -1 <= i <= 1 */
      RND_LOOP (rnd)
        for (fi = 0; fi < numberof (flags); fi++)
          {
            intmax_t r;

            if (rnd == MPFR_RNDZ && i < 0 && s >= 0)
              continue;
            if (rnd == MPFR_RNDZ && i > 0 && s <= 0)
              continue;
            if (rnd == MPFR_RNDD && i < 0)
              continue;
            if (rnd == MPFR_RNDU && i > 0)
              continue;
            if (rnd == MPFR_RNDA && ((MPFR_IS_POS(y) && i > 0) ||
                                     (MPFR_IS_NEG(y) && i < 0)))
              continue;

            for (e = 0; e < 2; e++)
              {
                if (e)
                  {
                    mpfr_exp_t ey;

                    if (MPFR_IS_ZERO (y))
                      break;
                    ey = MPFR_GET_EXP (y);
                    set_emin (ey);
                    set_emax (ey);
                  }
                /* rint (y) == x == s */
                __gmpfr_flags = ex_flags = flags[fi];
                if (i != 0)
                  ex_flags |= MPFR_FLAGS_INEXACT;
                r = mpfr_get_sj (y, (mpfr_rnd_t) rnd);
                gt_flags = __gmpfr_flags;
                set_emin (emin);
                set_emax (emax);
                if ((r != s || gt_flags != ex_flags) && rnd != MPFR_RNDF)
                  {
                    printf ("Error in check_sj for fi = %d, y = ", fi);
                    mpfr_out_str (stdout, 2, 0, y, MPFR_RNDN);
                    printf (" in %s%s\n",
                            mpfr_print_rnd_mode ((mpfr_rnd_t) rnd),
                            e ? ", reduced exponent range" : "");
                    printf ("Expected:");
                    PRMAX ("d", s);
                    flags_out (ex_flags);
                    printf ("Got:     ");
                    PRMAX ("d", r);
                    flags_out (gt_flags);
                    exit (1);
                  }
              }
          }
    }

  mpfr_clear (y);
}

static void
check_uj (uintmax_t u, mpfr_ptr x)
{
  mpfr_exp_t emin, emax;
  mpfr_t y;
  int i;

  mpfr_init2 (y, MPFR_PREC (x) + 2);

  emin = mpfr_get_emin ();
  emax = mpfr_get_emax ();

  for (i = -1; i <= 1; i++)
    {
      int rnd;
      int inex;
      int fi, e;
      mpfr_flags_t flags[2] = { 0, MPFR_FLAGS_ALL }, ex_flags, gt_flags;

      inex = mpfr_set_si_2exp (y, i, -2, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      inex = mpfr_add (y, y, x, MPFR_RNDN);
      MPFR_ASSERTN (inex == 0);
      /* y = x + i/4, with -1 <= i <= 1 */
      RND_LOOP (rnd)
        for (fi = 0; fi < numberof (flags); fi++)
          {
            uintmax_t r;

            if (rnd == MPFR_RNDZ && i < 0)
              continue;
            if (rnd == MPFR_RNDD && i < 0)
              continue;
            if (rnd == MPFR_RNDU && i > 0)
              continue;
            if (rnd == MPFR_RNDA && ((MPFR_IS_POS(y) && i > 0) ||
                                     (MPFR_IS_NEG(y) && i < 0)))
              continue;

            for (e = 0; e < 2; e++)
              {
                if (e)
                  {
                    mpfr_exp_t ey;

                    if (MPFR_IS_ZERO (y))
                      break;
                    ey = MPFR_GET_EXP (y);
                    set_emin (ey);
                    set_emax (ey);
                  }
                /* rint (y) == x == u */
                __gmpfr_flags = ex_flags = flags[fi];
                if (i != 0)
                  ex_flags |= MPFR_FLAGS_INEXACT;
                r = mpfr_get_uj (y, (mpfr_rnd_t) rnd);
                gt_flags = __gmpfr_flags;
                set_emin (emin);
                set_emax (emax);
                if ((r != u || gt_flags != ex_flags) && rnd != MPFR_RNDF)
                  {
                    printf ("Error in check_uj for fi = %d, y = ", fi);
                    mpfr_out_str (stdout, 2, 0, y, MPFR_RNDN);
                    printf (" in %s%s\n",
                            mpfr_print_rnd_mode ((mpfr_rnd_t) rnd),
                            e ? ", reduced exponent range" : "");
                    printf ("Expected:");
                    PRMAX ("u", u);
                    flags_out (ex_flags);
                    printf ("Got:     ");
                    PRMAX ("u", r);
                    flags_out (gt_flags);
                    exit (1);
                  }
              }
          }
    }

  mpfr_clear (y);
}

#define CHECK_ERANGE(F,FMT,RES,INPUT,VALUE,E)                           \
  do                                                                    \
    {                                                                   \
      __gmpfr_flags = ex_flags = flags[fi];                             \
      RES = F (x, (mpfr_rnd_t) rnd);                                    \
      gt_flags = __gmpfr_flags;                                         \
      if (E)                                                            \
        ex_flags |= MPFR_FLAGS_ERANGE;                                  \
      if (RES == VALUE && gt_flags == ex_flags)                         \
        continue;                                                       \
      printf ("Error in check_erange for %s, %s, fi = %d on %s\n",      \
              #F, mpfr_print_rnd_mode ((mpfr_rnd_t) rnd), fi, INPUT);   \
      printf ("Expected:");                                             \
      PRMAX (FMT, VALUE);                                               \
      flags_out (ex_flags);                                             \
      printf ("Got:     ");                                             \
      PRMAX (FMT, RES);                                                 \
      flags_out (gt_flags);                                             \
      exit (1);                                                         \
    }                                                                   \
  while (0)

#define CHECK_ERANGE_U(INPUT,VALUE,E) \
  CHECK_ERANGE (mpfr_get_uj, "u", u, INPUT, (uintmax_t) VALUE, E)
#define CHECK_ERANGE_S(INPUT,VALUE,E) \
  CHECK_ERANGE (mpfr_get_sj, "d", d, INPUT, (intmax_t) VALUE, E)

static void
check_erange (void)
{
  mpfr_t x;
  uintmax_t u;
  intmax_t d;
  int rnd;
  int fi;
  mpfr_flags_t flags[3] = { 0, MPFR_FLAGS_ALL ^ MPFR_FLAGS_ERANGE,
                            MPFR_FLAGS_ALL }, ex_flags, gt_flags;

  /* Test for ERANGE flag + correct behavior if overflow */

  mpfr_init2 (x, 256);

  RND_LOOP (rnd)
    for (fi = 0; fi < numberof (flags); fi++)
      {
        mpfr_set_uj (x, UINTMAX_MAX, MPFR_RNDN);
        CHECK_ERANGE_U ("UINTMAX_MAX", UINTMAX_MAX, 0);
        mpfr_add_ui (x, x, 1, MPFR_RNDN);
        CHECK_ERANGE_U ("UINTMAX_MAX+1", UINTMAX_MAX, 1);
        mpfr_set_sj (x, -1, MPFR_RNDN);
        CHECK_ERANGE_U ("-1", 0, 1);
        mpfr_set_sj (x, INTMAX_MAX, MPFR_RNDN);
        CHECK_ERANGE_S ("INTMAX_MAX", INTMAX_MAX, 0);
        mpfr_add_ui (x, x, 1, MPFR_RNDN);
        CHECK_ERANGE_S ("INTMAX_MAX+1", INTMAX_MAX, 1);
        mpfr_set_sj (x, INTMAX_MIN, MPFR_RNDN);
        CHECK_ERANGE_S ("INTMAX_MIN", INTMAX_MIN, 0);
        mpfr_sub_ui (x, x, 1, MPFR_RNDN);
        CHECK_ERANGE_S ("INTMAX_MIN-1", INTMAX_MIN, 1);
        mpfr_set_nan (x);
        CHECK_ERANGE_U ("NaN", 0, 1);
        CHECK_ERANGE_S ("NaN", 0, 1);
      }

  mpfr_clear (x);
}

static void
test_get_uj_smallneg (void)
{
  mpfr_t x;
  int i;

  mpfr_init2 (x, 64);

  for (i = 1; i <= 4; i++)
    {
      int r;

      mpfr_set_si_2exp (x, -i, -2, MPFR_RNDN);
      RND_LOOP (r)
        {
          intmax_t s;
          uintmax_t u;

          mpfr_clear_erangeflag ();
          s = mpfr_get_sj (x, r != MPFR_RNDF ? (mpfr_rnd_t) r : MPFR_RNDA);
          if (mpfr_erangeflag_p ())
            {
              printf ("ERROR for get_sj + ERANGE + small negative op"
                      " for rnd = %s and x = -%d/4\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              exit (1);
            }
          u = mpfr_get_uj (x, (mpfr_rnd_t) r);
          if (u != 0)
            {
              printf ("ERROR for get_uj + ERANGE + small negative op"
                      " for rnd = %s and x = -%d/4\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
#ifndef NPRINTF_J
              printf ("Expected 0, got %ju\n", u);
#endif
              exit (1);
            }
          if ((s == 0) ^ !mpfr_erangeflag_p ())
            {
              const char *Not = s == 0 ? "" : " not";

              printf ("ERROR for get_uj + ERANGE + small negative op"
                      " for rnd = %s and x = -%d/4\n",
                      mpfr_print_rnd_mode ((mpfr_rnd_t) r), i);
              printf ("The rounded integer ");
#ifndef NPRINTF_J
              printf("(%jd) ", s);
#endif
              printf("is%s representable in unsigned long,\n"
                     "but the erange flag is%s set.\n", Not, Not);
              exit (1);
            }
        }
    }

  mpfr_clear (x);
}

int
main (void)
{
  mpfr_prec_t prec;
  mpfr_t x, y;
  intmax_t s;
  uintmax_t u;

  tests_start_mpfr ();

  for (u = UINTMAX_MAX, prec = 0; u != 0; u /= 2, prec++)
    { }

  mpfr_init2 (x, prec + 4);
  mpfr_init2 (y, prec + 4);

  mpfr_set_ui (x, 0, MPFR_RNDN);
  check_sj (0, x);
  check_uj (0, x);

  mpfr_set_ui (x, 1, MPFR_RNDN);
  check_sj (1, x);
  check_uj (1, x);

  mpfr_neg (x, x, MPFR_RNDN);
  check_sj (-1, x);

  mpfr_set_si_2exp (x, 1, prec, MPFR_RNDN);
  mpfr_sub_ui (x, x, 1, MPFR_RNDN); /* UINTMAX_MAX */

  mpfr_div_ui (y, x, 2, MPFR_RNDZ);
  mpfr_trunc (y, y); /* INTMAX_MAX */
  for (s = INTMAX_MAX; s != 0; s /= 17)
    {
      check_sj (s, y);
      mpfr_div_ui (y, y, 17, MPFR_RNDZ);
      mpfr_trunc (y, y);
    }

  mpfr_div_ui (y, x, 2, MPFR_RNDZ);
  mpfr_trunc (y, y); /* INTMAX_MAX */
  mpfr_neg (y, y, MPFR_RNDN);
  if (INTMAX_MIN + INTMAX_MAX != 0)
    mpfr_sub_ui (y, y, 1, MPFR_RNDN); /* INTMAX_MIN */
  for (s = INTMAX_MIN; s != 0; s /= 17)
    {
      check_sj (s, y);
      mpfr_div_ui (y, y, 17, MPFR_RNDZ);
      mpfr_trunc (y, y);
    }

  for (u = UINTMAX_MAX; u != 0; u /= 17)
    {
      check_uj (u, x);
      mpfr_div_ui (x, x, 17, MPFR_RNDZ);
      mpfr_trunc (x, x);
    }

  mpfr_clear (x);
  mpfr_clear (y);

  check_erange ();
  test_get_uj_smallneg ();

  tests_end_mpfr ();
  return 0;
}

#endif
